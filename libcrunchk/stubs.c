// malloc_type
#include <sys/malloc.h> 
// CTASSERT
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
// sysctl
#include <sys/types.h>
#include <sys/sysctl.h>

/* Debug printouts */
#if DEBUG_STUBS == 1
#include <sys/ktr.h>
  #define PRINTD(x) CTR0(KTR_PTRACE, x)
  #define PRINTD1(f, x) CTR1(KTR_PTRACE, f, x)
  #define PRINTD2(f, x1, x2) CTR2(KTR_PTRACE, f, x1, x2)
#elif DEBUG_STUBS == 2
  extern printf(const char* format, ... );
  #define PRINTD(x) printf(x "\n")
  #define PRINTD1(f, x) printf(f "\n", x)
  #define PRINTD2(f, x1, x2) printf(f "\n", x1, x2)
#else
  #define PRINTD(x) if(0 && x)
  #define PRINTD1(f, x) if(0 && f && x)
  #define PRINTD2(f, x1, x2) if(0 && f && x1 && x2)
#endif


/*
 * libcrunch_cil_inlines.h
*/
#ifndef unlikely
#define __libcrunch_defined_unlikely
#define unlikely(cond) (__builtin_expect( (cond), 0 ))
#endif
#ifndef likely
#define __libcrunch_defined_likely
#define likely(cond)   (__builtin_expect( (cond), 1 ))
#endif



/*
 * malloc wrappers
*/

int __currently_freeing __attribute__((weak));  // TODO
int __currently_allocating __attribute__((weak));

/* The real malloc function, sig taken from malloc(9) man page */
void *__real_malloc(unsigned long size, struct malloc_type *type, int flags);

/* The hook */
void *__wrap_malloc(unsigned long size, struct malloc_type *type, int flags)
{
	PRINTD1("malloc called, size: %u", size);
	if (!type) PRINTD("malloc, no type!");
	else PRINTD1("malloc called, type: %s", type->ks_shortdesc);

	// Keep track of if we're first to set currently_allocating...
	_Bool set_currently_allocating = !__currently_allocating;
	__currently_allocating = 1;

	void *ret;
	ret = __real_malloc(size, type, flags);

	// ...and only unset it if that's the case
	if (set_currently_allocating) __currently_allocating = 0;
	return ret;
}



/*
 * uniqtype stuff
*/

struct uniqtype;

/* For a struct, we have at least three fields: name, offset, type. 	 */
/* In fact we might want *more* than that: a flag to say whether it's an */
/* offset or an absolute address (to encode the mcontext case). HMM.	 */
/* To avoid storing loads of pointers to close-by strings, each needing  */
/* relocation, we point to a names *vector* from a separate related[] entry. */
struct uniqtype_rel_info
{
   union {
	   struct {
		   struct uniqtype *ptr;
	   } t;
	   struct {
		   unsigned long val;
		   /* const char *name; might as well? NO, to save on relocations */
	   } enumerator;
	   /* For struct members, the main complexity is modelling stack frames  */
	   /* in the same formalism. Do we want to model locals that are in fact */
	   /* not stored in a manifest (type-directed) rep but are recoverable   */
	   /* (this is DW_OP_stack_value)? What about not stored at all?         */
	   /* We could do "stored (in frame, stable rep) (i.e .the good case)",  */
	   /* "stored (in register or static storage, stable rep) (common)",     */
	   /* "recoverable" (getter/setter), "absent"?                           */
	   struct {
		   struct uniqtype *ptr;
		   unsigned long off:56;
		   unsigned long is_absolute_address:1;
		   unsigned long may_be_invalid:1;
	   } memb;
	   struct {
		   const char **n; /* names vector */
	   } memb_names;
   } un;
};

struct alloc_addr_info
{
	unsigned long addr:47;
	unsigned flag:1;
	unsigned bits:16;
};

struct mcontext;
/* "make_precise" func for encoding dynamic / data-dependent reps (e.g. stack frame, hash table) */
typedef struct uniqtype *make_precise_fn_t(
	struct uniqtype *in,
	struct uniqtype *out,
	unsigned long out_len,
	void *obj,
	void *alloc_base,
	unsigned long alloc_sz,
	void *ip,
	struct mcontext *ctxt
);

struct uniqtype
{
   struct alloc_addr_info cache_word;
   unsigned pos_maxoff; /* positive size in bytes, or UINT_MAX for unbounded/unrep'able */
   union {
       struct {
           unsigned kind:4;
           unsigned unused_:28;
       } info;
       struct {
           unsigned kind:4;
       } _void;
       struct {
           unsigned kind:4;
           unsigned enc:6; /* i.e. up to 64 distinct encodings */
           unsigned one_plus_log_bit_size_delta:4; /* i.e. up to 15 i.e. delta of up to 2^14 less than implied bit size (8 * byte size) */
             signed bit_size_delta_delta:8; /* i.e. vary the delta +/- 127 bits */
             signed bit_off:10; /* i.e. bit offsets up to 2^9 *from either end* */
       } base; /* related[0] is signedness complement; could also do same-twice-as-big, same-twice-as-small? */
       struct {
           unsigned kind:4;
           unsigned is_contiguous:1; /* idea */
           unsigned is_log_spaced:1; /* idea (inefficiency: implies not is_contiguous) */
           unsigned nenum:26; /* HMM */
       } enumeration; /* related[0] is base type, related[1..nenum] are enumerators -- HMM, t is the *value*? */
       struct {
           unsigned kind:4;
           unsigned nmemb:20; /* 1M members should be enough */
           unsigned not_simultaneous:1; /* i.e. whether any member may be invalid */
       } composite; /* related[nmemb] is names ptr could also do "refines" i.e. templatey relations? */
       struct {
           unsigned kind:4;
           unsigned indir_level:5; /* contractually, after how many valid derefs might we get a non-address */
           unsigned genericity:1; /* I wrote "we need this when deciding whether or not to overwrite" -- can't remember what this means*/
           unsigned log_min_align:6; /* useful? just an idea */
       } address; /* related[0] is immediate pointee type; could also do ultimate non-pointer type? */
       struct {
           unsigned kind:4;
           unsigned narg:10; /* 1023 arguments is quite a lot */
           unsigned nret:10; /* sim. return values */
           unsigned is_va:1; /* is variadic */
           unsigned cc:7;    /* calling convention */
       } subprogram; /* related[0..nret] are return types; contained[nret..nret+narg] are args */
       struct {
           unsigned kind:4;
           unsigned min:14;
           unsigned max:14;
       } subrange; /* related[0] is host type */
       struct {
           unsigned is_array:1; /* because ARRAY is 8, i.e. top bit set */
           unsigned nelems:31; /* for consistency with pos_maxoff, use -1 for unbounded/unknown */
       } array; /* related[0] is element type */
       unsigned as_word; /* for funky optimizations, e.g. "== -1" to test for unbounded array */
   } un;
   make_precise_fn_t *make_precise; /* NULL means identity function */
   struct uniqtype_rel_info related[];
};

#define UNIQTYPE_NAME(u) __liballocs_uniqtype_name(u) /* helper in liballocs.c */
#define NAME_FOR_UNIQTYPE(u) UNIQTYPE_NAME(u)
#define UNIQTYPE_POS_MAXOFF_UNBOUNDED ((1ul << (8*sizeof(unsigned int)))-1) /* UINT_MAX */


/*
 * liballocs.c
*/

#define ALLOC_IS_DYNAMICALLY_SIZED(all, as) ((all) != (as))

struct liballocs_err
{
	const char *message;
};
typedef struct liballocs_err *liballocs_err_t;

#define WORD_BITSIZE ((sizeof (void*))<<3)
#if defined(__x86_64__) || defined(x86_64)
#define ADDR_BITSIZE 48
#else
#define ADDR_BITSIZE WORD_BITSIZE
#endif

struct entry
{
	unsigned present:1;
	unsigned removed:1;  /* whether this link is in the "removed" state in Harris's algorithm */
	unsigned distance:6; /* distance from the base of this entry's region, in 8-byte units */
} __attribute__((packed));

struct ptrs
{
	struct entry next;
	struct entry prev;
} __attribute__((packed));

struct insert
{
	unsigned alloc_site_flag:1;
	unsigned long alloc_site:(ADDR_BITSIZE-1);
	union  __attribute__((packed))
	{
		struct ptrs ptrs;
		unsigned bits:16;
	} un;
} __attribute__((packed));

struct allocator;

struct liballocs_err err_not_impl = {.message="Not implemented"};
inline struct liballocs_err *__liballocs_get_alloc_info(
	const void *obj,
	struct allocator **out_allocator,
	const void **out_alloc_start,
	unsigned long *out_alloc_size_bytes,
	struct uniqtype **out_alloc_uniqtype,
	const void **out_alloc_site
) {
	PRINTD1("__liballocs_get_alloc_info: %u", obj);
	// TODO liballocs.h
	struct liballocs_err *err = &err_not_impl;
	return err;
}

/* A client-friendly lookup function with cache. */
struct insert *lookup_object_info(
	const void *mem,
	void **out_object_start,
	size_t *out_object_size,
	void **ignored
) {
	PRINTD1("lookup_object_info: %u", mem);
	// TODO generic_malloc.c
	return NULL;
}

/* A client-friendly lookup function that knows about bigallocs.
 * FIXME: this needs to go away! Clients shouldn't have to know about inserts,
 * and not all allocators maintain them. */
struct insert *__liballocs_get_insert(const void *mem)
{
	PRINTD1("__liballocs_get_insert: %u", mem);
	// TODO
	return lookup_object_info(mem, NULL, NULL, NULL);
}

inline _Bool __liballocs_find_matching_subobject(
	signed target_offset_within_uniqtype,
	struct uniqtype *cur_obj_uniqtype,
	struct uniqtype *test_uniqtype,
	struct uniqtype **last_attempted_uniqtype,
	signed *last_uniqtype_offset,
	signed *p_cumulative_offset_searched,
	struct uniqtype **p_cur_containing_uniqtype,
	struct uniqtype_rel_info **p_cur_contained_pos
) {
	PRINTD("__liballocs_find_matching_subobject");
	// TODO liballocs.h
	return 0;
}

inline void __liballocs_ensure_init(void)
{
	PRINTD("__liballocs_ensure_init");
	// TODO liballocs.h
	return;
}

const char *(__attribute__((pure)) __liballocs_uniqtype_name)(
	const struct uniqtype *u
) {
	// TODO liballocs.c
	return "(no type)";
}


/*
 * allocator stuff
*/

/* The idealised base-level allocator protocol. These operations are mostly
   to be considered logically; some allocators (e.g. stack, GC) "inline" them
   rather than defining them as entry points. However, some allocators do define
   corresponding entry points, like malloc; here it would make sense to implement 
   these operations as an abstraction layer. I'm not yet sure how useful this is. */
#define ALLOC_BASE_API(fun, arg) \
fun(struct allocated_chunk *,new_uninit     ,arg(size_t, sz),arg(size_t,align),arg(struct uniqtype *,t)) \
fun(struct allocated_chunk *,new_zero       ,arg(size_t, sz),arg(size_t,align),arg(struct uniqtype *,t)) \
fun(void,                    delete         ,arg(struct allocated_chunk *,start)) \
fun(_Bool,                   resize_in_place,arg(struct allocated_chunk *,start),arg(size_t,new_sz)) \
fun(struct allocated_chunk *,safe_migrate,   arg(struct allocated_chunk *,start),arg(struct allocator *,recipient)) /* may fail */\
fun(struct allocated_chunk *,unsafe_migrate, arg(struct allocated_chunk *,start),arg(struct allocator *,recipient)) /* needn't free existing (stack) */\
fun(void,                    register_suballoc,arg(struct allocated_chunk *,start),arg(struct allocator *,suballoc))

#define ALLOC_REFLECTIVE_API(fun, arg) \
/* The *process-wide* reflective interface of liballocs */ \
fun(struct uniqtype *  ,get_type,      arg(void *, obj)) /* what type? */ \
fun(void *             ,get_base,      arg(void *, obj))  /* base address? */ \
fun(unsigned long      ,get_size,      arg(void *, obj))  /* size? */ \
fun(const void *       ,get_site,      arg(void *, obj))  /* where allocated?   optional   */ \
/* fun(liballocs_err_t    ,get_info,      arg(void *, obj), arg(struct big_allocation *, maybe_alloc), arg(struct uniqtype **,out_type), arg(void **,out_base), arg(unsigned long*,out_size), arg(const void**, out_site)) \ */ \
/* fun(struct big_allocation *,ensure_big,arg(void *, obj)) \ */ \
/* fun(Dl_info            ,dladdr,        arg(void *, obj))  /1* dladdr-like -- only for static *1/ \ */ \
/* fun(lifetime_policy_t *,get_lifetime,  arg(void *, obj)) \ */ \
/* fun(addr_discipl_t     ,get_discipl,   arg(void *, site)) /1* what will the code (if any) assume it can do with the ptr? *1/ \ */ \
fun(_Bool              ,can_issue,     arg(void *, obj), arg(off_t, off)) \
/* fun(size_t             ,raw_metadata,  arg(struct allocated_chunk *,start),arg(struct alloc_metadata **, buf)) \ */ \
fun(void               ,set_type,      arg(struct uniqtype *,new_t)) /* optional (stack) */\
fun(void               ,set_site,      arg(struct uniqtype *,new_t)) /* optional (stack) */

#define __allocmeta_fun_arg(argt, name) argt
#define __allocmeta_fun_ptr(rett, name, ...) rett (*name)( __VA_ARGS__ );

struct allocator
{
	const char *name;
	_Bool is_cacheable; /* HACK. FIXME: check libcrunch / is_a_cache really gets invalidated by all allocators. */
	ALLOC_REFLECTIVE_API(__allocmeta_fun_ptr, __allocmeta_fun_arg)
	/* Put the base API last, because it's least likely to take non-NULL values. */
	ALLOC_BASE_API(__allocmeta_fun_ptr, __allocmeta_fun_arg)
};

struct allocator __generic_malloc_allocator = {
	.name = "generic malloc",
	/* .get_info = NULL,  // __generic_heap_get_info, TODO ? */
	.is_cacheable = 1,
	/* .ensure_big = NULL,  // ensure_big, TODO ? */
};



/*
 * libcrunch.c
*/

/* Heap storage sized using a "loose" data type, like void*,
 * is marked as loose, and becomes non-loose when a cast to a non-loose type.
 * Annoyingly, liballocs eagerly replaces alloc site info with uniqtype
 * info. So making queries on an object will erase its looseness.
 * FIXME: separate this out, so that non-libcrunch clients don't have to
 * explicitly preserve looseness. */
#define STORAGE_CONTRACT_IS_LOOSE(ins, site) \
(((site) != NULL) /* i.e. liballocs only just erased the alloc site */ || \
!(ins)->alloc_site_flag /* or it still hasn't */ || \
((ins)->alloc_site & 0x1ul) /* or it's marked as loose explicitly */)

/* suppressions */
struct suppression
{
	const char *test_type_pat;
	const char *testing_function_pat;
	const char *alloc_type_pat;
};
const struct suppression suppressions[] = {
	{.test_type_pat="foo", .testing_function_pat="bar", .alloc_type_pat="foobar"}
};

static _Bool is_lazy_uniqtype(const void *u)
{
	// TODO: For now I'm ignoring lazy types
	return 0;
}

static _Bool is_suppressed(
	const char *test_typestr,
	const void *test_site,
	const char *alloc_typestr
) {
	return 0;
}

/* This filter is used to avoid repeated warnings, unless
 * the user has requested them (verbose mode). */
static _Bool should_report_failure_at(void *site)
{
	// TODO just going to assume verbose
	return 1;
}

static void cache_is_a(
	const void *obj_base,
	const void *obj_limit,
	const struct uniqtype *t,
	_Bool result,
	unsigned short period,
	const void *alloc_base
) {
	// TODO nopped because not using until reenabled in libcrunch_cil_inlines.h
	return;
}

static unsigned long repeat_suppression_count;
static const void *last_failed_site;
static const struct uniqtype *last_failed_deepest_subobject_type;

/*
 * misc
*/
void __assert_fail(
	const char *__assertion,
	const char *__file,
	unsigned int __line,
	const char *__function
) {
	//TODO
	return;
}

#define assert(e) ((e) ? (void)0 : \
	__assert_fail(__func__, __FILE__, __LINE__, #e))



/*
 * stubs.c
*/

/* tentative cache entry redesign to integrate bounds and types:
 *
 * - lower
 * - upper     (one-past)
 * - t         (may be null, i.e. bounds only)
 * - sz        (size of t)
 * - period    (need not be same as period, i.e. if T is int, alloc is array of stat, say)
 *                 ** ptr arithmetic is only valid if sz == period
 *                 ** entries with sz != period are still useful for checking types 
 * - results   (__is_a, __like_a, __locally_like_a, __is_function_refining, ... others?)
 */
struct __libcrunch_cache_entry_s
{
	const void *obj_base;
	const void *obj_limit;
	struct uniqtype *uniqtype;
	unsigned period;
	unsigned short result;
	unsigned char prev_mru;
	unsigned char next_mru;
	/* TODO: do inline uniqtype cache word check? */
} __attribute__((aligned(64)));
#define LIBCRUNCH_MAX_IS_A_CACHE_SIZE 8
struct __libcrunch_cache
{
	unsigned int validity; /* does *not* include the null entry */
	const unsigned short size_plus_one; /* i.e. including the null entry */
	unsigned short next_victim;
	unsigned char head_mru;
	unsigned char tail_mru;
	/* We use index 0 to mean "unused" / "null". */
	struct __libcrunch_cache_entry_s entries[1 + LIBCRUNCH_MAX_IS_A_CACHE_SIZE];
};


_Bool __libcrunch_is_initialized = 0;
extern _Bool our_init_flag __attribute__(
	(visibility("hidden"), alias("__libcrunch_is_initialized"))
);

struct __libcrunch_cache __libcrunch_is_a_cache; // all zeroes
/* Counters, macro will define the unsigned long to store the value and then
 * define a sysctl MIB under the debug.libcrunch parent node */
static SYSCTL_NODE(
	_debug, OID_AUTO, libcrunch, CTLFLAG_RD, 0, "libcrunch stats"
);
#define LIBCRUNCH_COUNTER(name) \
	unsigned long int __libcrunch_ ## name = 0; \
	SYSCTL_ULONG( \
		_debug_libcrunch, OID_AUTO, name, CTLFLAG_RD, \
		&__libcrunch_ ## name, \
		sizeof(__libcrunch_ ## name), \
		"__libcrunch_" #name \
	)
LIBCRUNCH_COUNTER(begun);
LIBCRUNCH_COUNTER(failed);
LIBCRUNCH_COUNTER(succeeded);
LIBCRUNCH_COUNTER(aborted_typestr);
LIBCRUNCH_COUNTER(is_a_hit_cache);
LIBCRUNCH_COUNTER(created_invalid_pointer);
LIBCRUNCH_COUNTER(checked_pointer_adjustments);
LIBCRUNCH_COUNTER(primary_secondary_transitions);
LIBCRUNCH_COUNTER(fault_handler_fixups);
/* added from libcrunch.c */
LIBCRUNCH_COUNTER(lazy_heap_type_assignment);
LIBCRUNCH_COUNTER(failed_in_alloc);

void __liballocs_systrap_init(void)
{
	PRINTD("__liballocs_systrap_init");
	return;
}
static void init(void) __attribute__((constructor));
static void init(void)
{
	PRINTD("init");
	/* It's critical that we detect whether we're being overridden,
	 * and skip this init if so. The preload code in liballocs wants to
	 * initialise systrap *only* after it has scanned /proc/self/maps,
	 * so that it can accurately track mmappings. To test for overriddenness,
	 * we use a hidden alias for something that will be overridden by our
	 * overrider, here the init flag. */
	if (&__libcrunch_is_initialized == &our_init_flag) __liballocs_systrap_init();
}

void __libcrunch_scan_lazy_typenames(void *blah)
{
	PRINTD("__libcrunch_scan_lazy_typenames");
}

int __libcrunch_global_init(void)
{
	PRINTD("__libcrunch_global_init");

	if (__libcrunch_is_initialized) return 0; // we are okay

	// don't try more than once to initialize
	static _Bool tried_to_initialize;
	if (tried_to_initialize) return -1;
	tried_to_initialize = 1;
	
	// we must have initialized liballocs
	__liballocs_ensure_init();
	
	/* We always include "signed char" in the lazy heap types. (FIXME: this is
	 * a C-specificity we'd rather not have here, but live with it for now.
	 * Perhaps the best way is to have "uninterpreted_sbyte" and make
	 * signed_char an alias for it.) We count the other ones. */
	// TODO ignoring lazy heap types for now
	/* const char *lazy_heap_types_str = LIBCRUNCH_LAZY_HEAP_TYPES */
	
	/* Load the suppression list from LIBCRUNCH_SUPPRESS. It's a space-separated
	 * list of triples <test-type-pat, testing-function-pat, alloc-type-pat>
	 * where patterns can end in "*" to indicate prefixing. */
	/* I've moved this to be hardcoded in suppressions to avoid porting all
	 * that string manipulation */

	// we need a segv handler to handle uses of trapped pointers
	// Don't think this is necessary in the kernel
	/* install_segv_handler(); */
	
	// for sane memory usage measurement, consider referencedness to start now
	// Don't know how this would apply
	/* clear_mem_refbits(); */
	
	__libcrunch_is_initialized = 1;
	PRINTD("libcrunch successfully initialized"); // lvl=1
	return 0;
}

int __libcrunch_check_init(void)
{
	if (unlikely(! &__libcrunch_is_initialized))
	{
		/* This means that we're not linked with libcrunch. 
		 * There's nothing we can do! */
		return -1;
	}
	if (unlikely(!__libcrunch_is_initialized))
	{
		/* This means we haven't initialized.
		 * Try that now (it won't try more than once). */
		int ret = __libcrunch_global_init ();
		return ret;
	}
	return 0;
}

int __is_a_internal(const void *obj, const void *arg)
{
	PRINTD("__is_a_internal");
	PRINTD1(
		"__is_a_internal, address pointed to by obj: %u",
		obj
	);

	/* We might not be initialized yet (recall that __libcrunch_global_init is 
	 * not a constructor, because it's not safe to call super-early). */
	__libcrunch_check_init();
	
	const struct uniqtype *test_uniqtype = (const struct uniqtype *) arg;
	struct allocator *a = NULL;
	const void *alloc_start;
	unsigned long alloc_size_bytes;
	struct uniqtype *alloc_uniqtype = (struct uniqtype *)0;
	const void *alloc_site;
	
	struct liballocs_err *err = __liballocs_get_alloc_info(obj, 
		&a,
		&alloc_start,
		&alloc_size_bytes,
		&alloc_uniqtype,
		&alloc_site);
	
	if (__builtin_expect(err != NULL, 0)) return 1; // liballocs has already counted this abort

	signed target_offset_within_uniqtype = (char*) obj - (char*) alloc_start;
	void *range_base;
	void *range_limit;
	unsigned short period = (alloc_uniqtype->pos_maxoff > 0) ? alloc_uniqtype->pos_maxoff : 0;
	/* If we're searching in a heap array, we need to take the offset modulo the
	 * element size. Otherwise just take the whole-block offset. */
	if (ALLOC_IS_DYNAMICALLY_SIZED(alloc_start, alloc_site)
			&& alloc_uniqtype
			&& alloc_uniqtype->pos_maxoff != UNIQTYPE_POS_MAXOFF_UNBOUNDED)
	{
		// HACK: for now, assume that the repetition continues to the end
		range_limit = (char*) alloc_start + alloc_size_bytes;
		target_offset_within_uniqtype %= alloc_uniqtype->pos_maxoff;
		range_base = (char*) alloc_start + target_offset_within_uniqtype; // FIXME: please check
	}
	else
	{
		/* HMM. Is this right? What about regularity *not* at top-level? */
		range_limit = (char*) obj + (test_uniqtype ? test_uniqtype->pos_maxoff : 1);
		range_base = (char*) obj;
	}
	
	/* FIXME: static allocations are cacheable *only* so long as we
	 * purge the cache on dynamic unloading (which we currently don't).
	 * FIXME: we need to distinguish alloca'd from stack'd allocations, somehow. */
	_Bool is_cacheable = a->is_cacheable /*|| ALLOC_IS_DYNAMICALLY_SIZED(alloc_start, alloc_site)*/;
	
	struct uniqtype *cur_obj_uniqtype = alloc_uniqtype;
	struct uniqtype *cur_containing_uniqtype = NULL;
	struct uniqtype_rel_info *cur_contained_pos = NULL;

	signed cumulative_offset_searched = 0;
	_Bool success = __liballocs_find_matching_subobject(
		target_offset_within_uniqtype,
		cur_obj_uniqtype,
		(struct uniqtype *) test_uniqtype,
		&cur_obj_uniqtype,
		&target_offset_within_uniqtype,
		&cumulative_offset_searched,
		&cur_containing_uniqtype,
		&cur_contained_pos
	);
	
	if (__builtin_expect(success, 1))
	{
		/* populate cache */
		if (is_cacheable) cache_is_a(range_base, range_limit, test_uniqtype, 1,
			period, alloc_start);

		++__libcrunch_succeeded;
		return 1;
	}
	
	// if we got here, we might still need to apply lazy heap typing
	if (__builtin_expect(a == &__generic_malloc_allocator /* FIXME */
			&& is_lazy_uniqtype(alloc_uniqtype)
			&& !__currently_allocating, 0))
	{
		struct insert *ins = __liballocs_get_insert(obj);
		assert(ins);
		if (STORAGE_CONTRACT_IS_LOOSE(ins, alloc_site))
		{
			++__libcrunch_lazy_heap_type_assignment;
			
			// update the heap chunk's info to say that its type is (strictly) our test_uniqtype
			ins->alloc_site_flag = 1;
			ins->alloc_site = (uintptr_t) test_uniqtype;
			if (is_cacheable) cache_is_a(range_base, range_limit, test_uniqtype, 1, period, alloc_start);
		
			return 1;
		}
	}
	
	// if we got here, the check failed
	if (is_cacheable) cache_is_a(range_base, range_limit, test_uniqtype, 0, period, alloc_start);
	if (__currently_allocating || __currently_freeing)
	{
		++__libcrunch_failed_in_alloc;
		// suppress warning
	}
	else
	{
		++__libcrunch_failed;
		
		if (!is_suppressed(
				UNIQTYPE_NAME(test_uniqtype),
				__builtin_return_address(0),
				alloc_uniqtype ? UNIQTYPE_NAME(alloc_uniqtype) : NULL
			))
		{
			if (should_report_failure_at(__builtin_return_address(0)))
			{
				if (last_failed_site == __builtin_return_address(0)
						&& last_failed_deepest_subobject_type == cur_obj_uniqtype)
				{
					++repeat_suppression_count;
				}
				else
				{
					if (repeat_suppression_count > 0)
					{
						PRINTD1(
							"Suppressed %ld further occurrences of the previous error",
							repeat_suppression_count
						); // lvl=0
					}

					// lvl=0
					PRINTD2("Failed check __is_a(%p, %p)", obj, test_uniqtype);
					PRINTD1("a.k.a. \"%s\"", UNIQTYPE_NAME(test_uniqtype));
					PRINTD2(
						"obj is %ld bytes into a %s",
						(long)((char*) obj - (char*) alloc_start),
						a ? a->name : "(no allocator)"
					);
					PRINTD2(
						"%s%s",
						(ALLOC_IS_DYNAMICALLY_SIZED(alloc_start, alloc_site) &&
						 alloc_uniqtype &&
						 alloc_size_bytes > alloc_uniqtype->pos_maxoff)
						 ? " allocation of " : " ",
						NAME_FOR_UNIQTYPE(alloc_uniqtype)
					);
					PRINTD2(
						"(deepest subobject: %s at offset %d) ",
						(cur_obj_uniqtype
						 ? ((cur_obj_uniqtype == alloc_uniqtype)
							? "(the same)" : UNIQTYPE_NAME(cur_obj_uniqtype))
						 : "(none)"),
						cumulative_offset_searched
					);
					PRINTD1(
						"originating at %p",
						alloc_site
					);

					last_failed_site = __builtin_return_address(0);
					last_failed_deepest_subobject_type = cur_obj_uniqtype;

					repeat_suppression_count = 0;
				}
			}
		}
	}
	return 1; // HACK: so that the program will continue
}

int __like_a_internal(const void *obj, const void *r)
{
	PRINTD("__like_a_internal");
	return 1;
}

int __loosely_like_a_internal(const void *obj, const void *r)
{
	PRINTD("__loosely_like_a_internal");
	return 1;
}

int __named_a_internal(const void *obj, const void *r)
{
	PRINTD("__named_a_internal");
	return 1;
}

int __check_args_internal(const void *obj, int nargs, ...)
{
	PRINTD("__check_args_internal");
	return 1;
}

int __is_a_function_refining_internal(const void *obj, const void *r)
{
	PRINTD("__is_a_function_refining_internal");
	return 1;
}

int __is_a_pointer_of_degree_internal(const void *obj, int d)
{
	PRINTD("__is_a_pointer_of_degree_internal");
	return 1;
}
int __can_hold_pointer_internal(const void *obj, const void *target)
{
	PRINTD("__can_hold_pointer_internal");
	return 1;
}
