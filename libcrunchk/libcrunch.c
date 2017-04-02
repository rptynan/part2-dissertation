#include <libcrunchk/libcrunch.h>
// CTASSERT
/* #include <sys/cdefs.h> */
/* #include <sys/param.h> */
/* #include <sys/systm.h> */
// sysctl
#include <sys/types.h>
#include <sys/sysctl.h>


/* This needs to be somewhere, so might as well go here */
void __assert_fail(
	const char *__assertion,
	const char *__file,
	unsigned int __line,
	const char *__function
) {
	PRINTD1("Assert failed: %s", __assertion);
	PRINTD1("In file: %s", __file);
	PRINTD2("%s:%d", __function, __line);
	return;
}


/* Counters, macro will define the unsigned long to store the value and then
 * define a sysctl MIB under the debug.libcrunch parent node */
#if KERNEL
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
#else
  /* In the (userspace) test case, when we don't have the kernel stuff, just
   * ignore syctl */
  #define LIBCRUNCH_COUNTER(name) \
	unsigned long int __libcrunch_ ## name = 0;
#endif

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
