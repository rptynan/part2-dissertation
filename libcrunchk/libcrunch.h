#include <sys/types.h>
#include <libcrunchk/uniqtype.h>

/* Debug printouts */
#if DEBUG_STUBS == 1
  #include <sys/ktr.h>
  #define PRINTD(x) CTR0(KTR_PTRACE, x)
  #define PRINTD1(f, x) CTR1(KTR_PTRACE, f, x)
  #define PRINTD2(f, x1, x2) CTR2(KTR_PTRACE, f, x1, x2)
#elif DEBUG_STUBS == 2
  extern int printf(const char* format, ... );
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
 * misc
*/
#define assert(e) ( e ? (void)0 : \
	__assert_fail(__func__, __FILE__, __LINE__, #e))  // in libcrunch.c


#define NULL 0


/*
 * allocators
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

extern int __currently_allocating;
extern int __currently_freeing;


/*
 * liballocs
*/
#define ALLOC_IS_DYNAMICALLY_SIZED(all, as) ((all) != (as))

/* structs */
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

/* Functions */
inline struct liballocs_err *__liballocs_get_alloc_info(
	const void *obj,
	struct allocator **out_allocator,
	const void **out_alloc_start,
	unsigned long *out_alloc_size_bytes,
	struct uniqtype **out_alloc_uniqtype,
	const void **out_alloc_site
);

struct insert *__liballocs_get_insert(const void *mem);

const char *(__attribute__((pure)) __liballocs_uniqtype_name)(
	const struct uniqtype *u
);

inline _Bool __liballocs_find_matching_subobject(
	signed target_offset_within_uniqtype,
	struct uniqtype *cur_obj_uniqtype,
	struct uniqtype *test_uniqtype,
	struct uniqtype **last_attempted_uniqtype,
	signed *last_uniqtype_offset,
	signed *p_cumulative_offset_searched,
	struct uniqtype **p_cur_containing_uniqtype,
	struct uniqtype_rel_info **p_cur_contained_pos
);

inline void __liballocs_ensure_init(void);