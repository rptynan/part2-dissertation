#include <libcrunchk/include/libcrunch.h>
#include <libcrunchk/include/liballocs.h>
#include <libcrunchk/include/allocmeta.h>
#include <libcrunchk/include/pageindex.h>
#ifdef _KERNEL
  // CTASSERT, for sysctl
  #include <sys/cdefs.h>
  #include <sys/param.h>
  #include <sys/systm.h>
  // SYSINIT
  #include <sys/kernel.h>
#endif
// sysctl
#include <sys/types.h>
#include <sys/sysctl.h>
// variadic
#include <x86/stdarg.h>
// Need this to call is_aU() etc
#include </usr/local/src/libcrunch/include/libcrunch_cil_inlines.h>
#define unlikely(cond) (__builtin_expect( (cond), 0 ))
#define likely(cond)   (__builtin_expect( (cond), 1 ))


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
void __alloca_allocator_notify(void *new_userchunkaddr, unsigned long modified_size, 
		unsigned long *frame_counter, const void *caller, 
		const void *caller_sp, const void *caller_bp) { };
void __liballocs_unindex_stack_objects_counted_by(unsigned long *thing, void *frame_addr) {};
void __liballocs_alloca_caller_frame_cleanup(void *counter) {};


/* Counters, macro will define the unsigned long to store the value and then
 * define a sysctl MIB under the debug.libcrunch parent node */
#ifdef _KERNEL
  static SYSCTL_NODE(
	_debug, OID_AUTO, libcrunch, CTLFLAG_RD, 0, "libcrunch stats"
  );
  #define LIBCRUNCH_COUNTER(name) \
	unsigned long int __libcrunch_ ## name = 0; \
	SYSCTL_ULONG( \
		_debug_libcrunch, OID_AUTO, name, CTLFLAG_RW, \
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
LIBCRUNCH_COUNTER(lazy_heap_type_assignment);
LIBCRUNCH_COUNTER(failed_in_alloc);
// custom
LIBCRUNCH_COUNTER(failed_liballocs_err);
LIBCRUNCH_COUNTER(called_before_init);
LIBCRUNCH_COUNTER(splaying_on);


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


_Bool __libcrunch_is_initialized = 0;
extern _Bool our_init_flag __attribute__(
	(visibility("hidden"), alias("__libcrunch_is_initialized"))
); // necessary?

/* struct __libcrunch_cache __libcrunch_is_a_cache; // all zeroes */

/* Only run when userspace testing */
#ifndef _KERNEL
static void print_exit_summary(void)
{
	fprintf(stderr, "====================================================\n");
	fprintf(stderr, "libcrunch summary: \n");
	fprintf(stderr, "----------------------------------------------------\n");
	fprintf(stderr, "type checks begun:                         % 9ld\n", __libcrunch_begun);
	fprintf(stderr, "----------------------------------------------------\n");
/* #ifdef LIBCRUNCH_EXTENDED_COUNTS */
/* 	fprintf(stderr, "       aborted due to init failure:        % 9ld\n", __libcrunch_aborted_init); */
/* #endif */
	fprintf(stderr, "       aborted for bad typename:           % 9ld\n", __libcrunch_aborted_typestr);
/* #ifdef LIBCRUNCH_EXTENDED_COUNTS */
/* 	fprintf(stderr, "       trivially passed:                   % 9ld\n", __libcrunch_trivially_succeeded); */
/* #endif */
/* #ifdef LIBCRUNCH_EXTENDED_COUNTS */
/* 	fprintf(stderr, "       remaining                           % 9ld\n", __libcrunch_begun - (__libcrunch_trivially_succeeded + __liballocs_aborted_unknown_storage + __libcrunch_aborted_typestr + __libcrunch_aborted_init)); */
/* #else */
	fprintf(stderr, "       remaining                           % 9ld\n", __libcrunch_begun - (__liballocs_aborted_unknown_storage + __libcrunch_aborted_typestr));
/* #endif */	
	fprintf(stderr, "----------------------------------------------------\n");
	fprintf(stderr, "   of which did lazy heap type assignment: % 9ld\n", __libcrunch_lazy_heap_type_assignment);
	fprintf(stderr, "----------------------------------------------------\n");
	fprintf(stderr, "       failed inside allocation functions: % 9ld\n", __libcrunch_failed_in_alloc);
	fprintf(stderr, "       failed otherwise:                   % 9ld\n", __libcrunch_failed);
	/* fprintf(stderr, "                 of which user suppressed: % 9ld\n", __libcrunch_failed_and_suppressed); */
	fprintf(stderr, "       nontrivially passed:                % 9ld\n", __libcrunch_succeeded);
	fprintf(stderr, "----------------------------------------------------\n");
	fprintf(stderr, "   of which hit __is_a cache:              % 9ld\n", __libcrunch_is_a_hit_cache);
	fprintf(stderr, "----------------------------------------------------\n");
	fprintf(stderr, "out-of-bounds pointers created:            % 9ld\n", __libcrunch_created_invalid_pointer);
	/* fprintf(stderr, "accesses trapped and emulated:             % 9ld\n", 0ul /1* FIXME *1/); */
	/* fprintf(stderr, "calls to __fetch_bounds:                   % 9ld\n", __libcrunch_fetch_bounds_called /1* FIXME: remove *1/); */
	/* fprintf(stderr, "   of which missed cache:                  % 9ld\n", __libcrunch_fetch_bounds_missed_cache); */
	fprintf(stderr, "calls requiring secondary checks           % 9ld\n", __libcrunch_primary_secondary_transitions);
	fprintf(stderr, "trap-pointer fixups in fault handler       % 9ld\n", __libcrunch_fault_handler_fixups);
	fprintf(stderr, "====================================================\n");
	fprintf(stderr, "failed because of liballocs error:         % 9ld\n", __libcrunch_failed_liballocs_err);
}
#endif

void __libcrunch_scan_lazy_typenames(void *blah)
{
	PRINTD("__libcrunch_scan_lazy_typenames");
}

struct rwlock pageindex_rwlock;
struct rwlock heapindex_rwlock;
struct rwlock typesindex_rwlock;
int __libcrunch_global_sysinit(void *unused)
{
	PRINTD("__libcrunch_global_sysinit");
	// to output on boot screen
	printf("====================================================\n");
	printf("======         libcrunch initialising           ====\n");
	printf("====================================================\n");

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

	// For testing in userspace
	#ifndef _KERNEL
		atexit(print_exit_summary);
	#endif
	__libcrunch_is_initialized = 1;
	printf("libcrunch successfully initialized\n");
	PRINTD("libcrunch successfully initialized"); // lvl=1
	return 0;
}

#ifdef _KERNEL
// This will initialise libcrunch late in boot process. All type check
// functions should abort if called before this is initialised
SYSINIT(
	libcrunch_init, SI_SUB_LAST, SI_ORDER_ANY, __libcrunch_global_sysinit, NULL
);
#else
__attribute__((constructor)) static void userspace_init() {
	__libcrunch_global_sysinit(NULL);
}
#endif

int __libcrunch_global_init(void) {
	return 1;
}
int __libcrunch_check_init(void)
{
	return __libcrunch_is_initialized;
}


/* Helpers */

/* This helper is short-circuiting: it doesn't tell you the precise degree 
 * of the pointer, only whether it's at least d. */
static _Bool pointer_has_degree(struct uniqtype *t, int d)
{
	while (d > 0)
	{
		if (!UNIQTYPE_IS_POINTER_TYPE(t)) return 0;
		t = UNIQTYPE_POINTEE_TYPE(t);
		assert(t);
		--d;
	}
	return 1;
}

static _Bool pointer_degree_and_ultimate_pointee_type(struct uniqtype *t, int *out_d, 
		struct uniqtype **out_ultimate_pointee_type)
{
	int d = 0;
	while (UNIQTYPE_IS_POINTER_TYPE(t))
	{
		++d;
		t = UNIQTYPE_POINTEE_TYPE(t);
	}
	*out_d = d;
	*out_ultimate_pointee_type = t;
	return 1;
}

static _Bool is_generic_ultimate_pointee(struct uniqtype *ultimate_pointee_type)
{
	return ultimate_pointee_type == pointer_to___uniqtype__void 
		|| ultimate_pointee_type == pointer_to___uniqtype__signed_char
		|| ultimate_pointee_type == pointer_to___uniqtype__unsigned_char;
}

static _Bool holds_pointer_of_degree(struct uniqtype *cur_obj_uniqtype, int d, signed target_offset)
{
	struct uniqtype *cur_containing_uniqtype = NULL;
	struct uniqtype_rel_info *cur_contained_pos = NULL;
	signed target_offset_within_uniqtype = target_offset;

	/* Descend the subobject hierarchy until we can't go any further (since pointers
	 * are atomic. */
	_Bool success = 1;
	while (success)
	{
		success = __liballocs_first_subobject_spanning(
			&target_offset_within_uniqtype, &cur_obj_uniqtype, &cur_containing_uniqtype,
			&cur_contained_pos);
	}

	if (target_offset_within_uniqtype == 0 && UNIQTYPE_IS_POINTER_TYPE(cur_obj_uniqtype))
	{
		_Bool depth_okay = pointer_has_degree(cur_obj_uniqtype, d);
		if (depth_okay)
		{
			return 1;
		} else return 0;
	}
	
	return 0;
}

static int pointer_degree(struct uniqtype *t)
{
	_Bool success;
	int d;
	struct uniqtype *dontcare;
	success = pointer_degree_and_ultimate_pointee_type(t, &d, &dontcare);
	return success ? d : -1;
}

static int is_generic_pointer_type_of_degree_at_least(struct uniqtype *t, int must_have_d)
{
	_Bool success;
	int d;
	struct uniqtype *ultimate;
	success = pointer_degree_and_ultimate_pointee_type(t, &d, &ultimate);
	if (success && d >= must_have_d && is_generic_ultimate_pointee(ultimate)) return d;
	else return 0;
}

static _Bool is_generic_pointer_type(struct uniqtype *t)
{
	return is_generic_pointer_type_of_degree_at_least(t, 1);
}

static void
reinstate_looseness_if_necessary(
    const void *alloc_start, const void *alloc_site,
    struct uniqtype *alloc_uniqtype
)
{
	/* Unlike other checks, we want to preserve looseness of the target block's 
	 * type, if it's a pointer type. So set the loose flag if necessary. */
	if (ALLOC_IS_DYNAMICALLY_SIZED(alloc_start, alloc_site) 
			&& alloc_site != NULL
			&& UNIQTYPE_IS_POINTER_TYPE(alloc_uniqtype))
	{
		struct insert *ins = __liballocs_get_insert(alloc_start);
		//	(void*) alloc_start, malloc_usable_size((void*) alloc_start)
		//);
		if (ins->alloc_site_flag)
		{
			assert(0 == ins->alloc_site & 0x1ul);
			ins->alloc_site |= 0x1ul;
		}
	}
}



int __is_a_internal(const void *obj, const void *arg)
{
	PRINTD("__is_a_internal");
	PRINTD1(
		"__is_a_internal, address pointed to by obj: %p",
		obj
	);

	/* We might not be initialized yet (recall that __libcrunch_global_init is 
	 * not a constructor, because it's not safe to call super-early). */
	if (!__libcrunch_check_init()) {
		__libcrunch_called_before_init++;
		return 1;
	}
	
	const struct uniqtype *test_uniqtype = (const struct uniqtype *) arg;
	struct allocator *a = NULL;
	const void *alloc_start;
	unsigned long alloc_size_bytes;
	struct uniqtype *alloc_uniqtype = (struct uniqtype *)0;
	const void *alloc_site;

	struct liballocs_err *err = __liballocs_get_alloc_info(
		obj,
		&a,
		&alloc_start,
		&alloc_size_bytes,
		&alloc_uniqtype,
		&alloc_site
	);

	// Hack for ease, if the type hasn't been set by the allocation,
	// we assume it's this type, and try again.
	if (unlikely(alloc_uniqtype == &unset__uniqtype__)) {
		PRINTD1(
			"__is_a_internal, setting type to first check: allocsite %p",
			alloc_site
		);
		__liballocs_notify_unset_type(alloc_site, test_uniqtype);
		err = __liballocs_get_alloc_info(
			obj,
			&a,
			&alloc_start,
			&alloc_size_bytes,
			&alloc_uniqtype,
			&alloc_site
		);
	}

	if (__builtin_expect(err != NULL, 0)) {
		PRINTD1(
			"__is_a_internal, liballocs_get_alloc_info returned error: %s",
			err->message
		);
		__libcrunch_failed_liballocs_err++;
		return 1; // liballocs has already counted this abort
	}

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
		if (alloc_uniqtype->pos_maxoff) {
			target_offset_within_uniqtype %= alloc_uniqtype->pos_maxoff;
		}
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

		PRINTD("__is_a_internal succeeded");
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
			PRINTD("__is_a_internal lazy_heap_type_assignment");
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
		PRINTD("__is_a_internal failed in alloc");
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

					/* debug_printf(0, "Failed check __is_a(%p, %p a.k.a. \"%s\") at %p (%s); " */
					/* 		"obj is %ld bytes into a %s%s%s " */
					/* 		"(deepest subobject: %s at offset %d) " */
					/* 		"originating at %p\n", */ 
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
					PRINTD1(
						"alloc type of %p",
						alloc_uniqtype
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

/* Basically __like_a() does a structural match -- it unwraps the test type one
 * level and checks the fields against each other. If you set
 * LIBCRUNCH_USE_LIKE_A_FOR_TYPES="T S" it will use this function instead for
 * casts to S* and T*. It's only needed for sloppy C code that expects to do
 * structural subtyping-like tricks.
*/
int __like_a_internal(const void *obj, const void *arg)
{
	PRINTD("__like_a_internal");
	return 1; // TODO ignoring for now as may not be strictly necessary?
	// FIXME: use our recursive subobject search here? HMM -- semantics are non-obvious.
	
	/* We might not be initialized yet (recall that __libcrunch_global_init is 
	 * not a constructor, because it's not safe to call super-early). */
	if (!__libcrunch_check_init()) {
		__libcrunch_called_before_init++;
		return 1;
	}
	
	const struct uniqtype *test_uniqtype = (const struct uniqtype *) arg;
	struct allocator *a;
	const void *alloc_start;
	unsigned long alloc_size_bytes;
	struct uniqtype *alloc_uniqtype = (struct uniqtype *)0;
	const void *alloc_site;
	/* void *caller_address = __builtin_return_address(0); */
	
	struct liballocs_err *err = __liballocs_get_alloc_info(
		obj,
		&a,
		&alloc_start,
		&alloc_size_bytes,
		&alloc_uniqtype,
		&alloc_site
	);
	
	if (__builtin_expect(err != NULL, 0)) {
		__libcrunch_failed_liballocs_err++;
		return 1; // liballocs has already counted this abort
	}
	
	signed target_offset_within_uniqtype = (char*) obj - (char*) alloc_start;
	/* If we're searching in a heap array, we need to take the offset modulo the
	 * element size. Otherwise just take the whole-block offset. */
	if (ALLOC_IS_DYNAMICALLY_SIZED(alloc_start, alloc_site) &&
		alloc_uniqtype &&
		alloc_uniqtype->pos_maxoff != 0)
	{
		target_offset_within_uniqtype %= alloc_uniqtype->pos_maxoff;
	}
	
	struct uniqtype *cur_obj_uniqtype = alloc_uniqtype;
	struct uniqtype *cur_containing_uniqtype = NULL;
	struct uniqtype_rel_info *cur_contained_pos = NULL;
	
	/* Descend the subobject hierarchy until our target offset is zero, i.e. we
	 * find the outermost thing in the subobject tree that starts at the address
	 * we were passed (obj). */
	while (target_offset_within_uniqtype != 0)
	{
		_Bool success = __liballocs_first_subobject_spanning(
			&target_offset_within_uniqtype,
			&cur_obj_uniqtype,
			&cur_containing_uniqtype,
			&cur_contained_pos
		);
		if (!success) goto like_a_failed;
	}
	
	// trivially, identical types are like one another
	if (test_uniqtype == cur_obj_uniqtype) goto like_a_succeeded;
	
	// arrays are special
	_Bool matches;
	if (__builtin_expect(
		(UNIQTYPE_IS_ARRAY_TYPE(cur_obj_uniqtype) ||
		 UNIQTYPE_IS_ARRAY_TYPE(test_uniqtype)), 0))
	{
		matches =
			test_uniqtype == cur_obj_uniqtype
		||  (UNIQTYPE_IS_ARRAY_TYPE(test_uniqtype) && UNIQTYPE_ARRAY_LENGTH(test_uniqtype) == 1
				&& UNIQTYPE_ARRAY_ELEMENT_TYPE(test_uniqtype) == cur_obj_uniqtype)
		||  (UNIQTYPE_IS_ARRAY_TYPE(cur_obj_uniqtype) && UNIQTYPE_ARRAY_LENGTH(cur_obj_uniqtype) == 1
				&& UNIQTYPE_ARRAY_ELEMENT_TYPE(cur_obj_uniqtype) == test_uniqtype);
		/* We don't need to allow an array of one blah to be like a different
		 * array of one blah, because they should be the same type. 
		 * FIXME: there's a difficult case: an array of statically unknown length, 
		 * which happens to have length 1. */
		if (matches) goto like_a_succeeded; else goto like_a_failed;
	}
	
	/* We might have base types with signedness complements. */
	if (!UNIQTYPE_IS_BASE_TYPE(cur_obj_uniqtype) && !UNIQTYPE_IS_BASE_TYPE(test_uniqtype))
	{
		/* Does the cur obj type have a signedness complement matching the test type? */
		if (UNIQTYPE_BASE_TYPE_SIGNEDNESS_COMPLEMENT(cur_obj_uniqtype) == test_uniqtype) goto like_a_succeeded;
		/* Does the test type have a signedness complement matching the cur obj type? */
		if (UNIQTYPE_BASE_TYPE_SIGNEDNESS_COMPLEMENT(test_uniqtype) == cur_obj_uniqtype) goto like_a_succeeded;
	}
	
	/* Okay, we can start the like-a test: for each element in the test type,
	 * do we have a type-equivalent in the object type?
	 *
	 * We make an exception for arrays of char (signed or unsigned): if an
	 * element in the test type is such an array, we skip over any number of
	 * fields in the object type, until we reach the offset of the end element.
	*/
	unsigned i_obj_subobj = 0, i_test_subobj = 0;
	for (;
		i_obj_subobj < UNIQTYPE_COMPOSITE_MEMBER_COUNT(cur_obj_uniqtype)
			 && i_test_subobj < UNIQTYPE_COMPOSITE_MEMBER_COUNT(test_uniqtype);
		++i_test_subobj, ++i_obj_subobj)
	{
		if (__builtin_expect(UNIQTYPE_IS_ARRAY_TYPE(test_uniqtype->related[i_test_subobj].un.memb.ptr)
			&& (UNIQTYPE_ARRAY_ELEMENT_TYPE(test_uniqtype->related[i_test_subobj].un.memb.ptr)
					== pointer_to___uniqtype__signed_char
			|| UNIQTYPE_ARRAY_ELEMENT_TYPE(test_uniqtype->related[i_test_subobj].un.memb.ptr)
					== pointer_to___uniqtype__unsigned_char), 0))
		{
			// we will skip this field in the test type
			signed target_off =
				UNIQTYPE_COMPOSITE_MEMBER_COUNT(test_uniqtype) > i_test_subobj + 1
			 ?  test_uniqtype->related[i_test_subobj + 1].un.memb.off
			 :  test_uniqtype->related[i_test_subobj].un.memb.off
			      + test_uniqtype->related[i_test_subobj].un.memb.ptr->pos_maxoff;
			
			// ... if there's more in the test type, advance i_obj_subobj
			while (i_obj_subobj + 1 < UNIQTYPE_COMPOSITE_MEMBER_COUNT(cur_obj_uniqtype) &&
				cur_obj_uniqtype->related[i_obj_subobj + 1].un.memb.off
					< target_off) ++i_obj_subobj;
			/* We fail if we ran out of stuff in the target object type
			 * AND there is more to go in the test type. */
			if (i_obj_subobj + 1 >= UNIQTYPE_COMPOSITE_MEMBER_COUNT(cur_obj_uniqtype)
			 && UNIQTYPE_COMPOSITE_MEMBER_COUNT(test_uniqtype) > i_test_subobj + 1) goto like_a_failed;
				
			continue;
		}
		matches =
				test_uniqtype->related[i_test_subobj].un.memb.off == cur_obj_uniqtype->related[i_obj_subobj].un.memb.off
		 && 	test_uniqtype->related[i_test_subobj].un.memb.ptr == cur_obj_uniqtype->related[i_obj_subobj].un.memb.ptr;
		if (!matches) goto like_a_failed;
	}
	// if we terminated because we ran out of fields in the target type, fail
	if (i_test_subobj < UNIQTYPE_COMPOSITE_MEMBER_COUNT(test_uniqtype)) goto like_a_failed;
	
like_a_succeeded:
	++__libcrunch_succeeded;
	return 1;
	
	// if we got here, we've failed
	// if we got here, the check failed
like_a_failed:
	if (__currently_allocating || __currently_freeing)
	{
		PRINTD("__is_a_internal, failed in alloc");
		++__libcrunch_failed_in_alloc;
		// suppress warning
	}
	else
	{
		++__libcrunch_failed;
		if (!is_suppressed(UNIQTYPE_NAME(test_uniqtype), __builtin_return_address(0), alloc_uniqtype ? UNIQTYPE_NAME(alloc_uniqtype) : NULL))
		{
			if (should_report_failure_at(__builtin_return_address(0)))
			{
				/* debug_printf(0, "Failed check __like_a(%p, %p a.k.a. \"%s\")
				 * at %p (%s), allocation was a %s%s%s originating at %p\n", */ 
				PRINTD2("Failed check __like_a(%p, %p)", obj, test_uniqtype);
				PRINTD1("a.k.a \"%s\"", UNIQTYPE_NAME(test_uniqtype));
				PRINTD2(
					"at %p (%s)",
					__builtin_return_address(0),
					format_symbolic_address(__builtin_return_address(0))
				);
				PRINTD2(
					"allocation was a %s%s",
					a ? a->name : "(no allocator)",
					(ALLOC_IS_DYNAMICALLY_SIZED(alloc_start, alloc_site) && alloc_uniqtype && alloc_size_bytes > alloc_uniqtype->pos_maxoff) ? " allocation of " : " "
				);
				PRINTD2(
					"%s originating at %p",
					NAME_FOR_UNIQTYPE(alloc_uniqtype),
					alloc_site
				);
			}
		}
	}
	return 1; // HACK: so that the program will continue
}

int __loosely_like_a_internal(const void *obj, const void *arg)
{
	PRINTD("__loosely_like_a_internal");
	/* We might not be initialized yet (recall that __libcrunch_global_init is 
	 * not a constructor, because it's not safe to call super-early). */
	if (!__libcrunch_check_init()) {
		__libcrunch_called_before_init++;
		return 1;
	}
	
	struct uniqtype *test_uniqtype = (struct uniqtype *) arg;
	struct allocator *a;
	const void *alloc_start;
	unsigned long alloc_size_bytes;
	struct uniqtype *alloc_uniqtype = (struct uniqtype *)0;
	const void *alloc_site;
	void *caller_address = __builtin_return_address(0);
	
	struct liballocs_err *err = __liballocs_get_alloc_info(obj, 
		&a,
		&alloc_start,
		&alloc_size_bytes,
		&alloc_uniqtype, 
		&alloc_site);
	
	if (__builtin_expect(err != NULL, 0)) {
		__libcrunch_failed_liballocs_err++;
		return 1; // liballocs has already counted this abort
	}
	
	/* HACK */
	reinstate_looseness_if_necessary(alloc_start, alloc_site, alloc_uniqtype);
	
	signed target_offset_within_alloc_uniqtype = (char*) obj - (char*) alloc_start;
	/* If we're searching in a heap array, we need to take the offset modulo the 
	 * element size. Otherwise just take the whole-block offset. */
	if (ALLOC_IS_DYNAMICALLY_SIZED(alloc_start, alloc_site)
			&& alloc_uniqtype
			&& alloc_uniqtype->pos_maxoff != 0)
	{
		target_offset_within_alloc_uniqtype %= alloc_uniqtype->pos_maxoff;
	}
	
	struct uniqtype *cur_alloc_subobj_uniqtype = alloc_uniqtype;
	struct uniqtype *cur_containing_uniqtype = NULL;
	struct uniqtype_rel_info *cur_contained_pos = NULL;
	
	/* Descend the subobject hierarchy until our target offset is zero, i.e. we 
	 * find the outermost thing in the subobject tree that starts at the address
	 * we were passed (obj). */
	while (target_offset_within_alloc_uniqtype != 0)
	{
		_Bool success = __liballocs_first_subobject_spanning(
				&target_offset_within_alloc_uniqtype, &cur_alloc_subobj_uniqtype, &cur_containing_uniqtype,
				&cur_contained_pos);
		if (!success) goto loosely_like_a_failed;
	}
	
	do
	{
		
	// trivially, identical types are like one another
	if (test_uniqtype == cur_alloc_subobj_uniqtype) goto loosely_like_a_succeeded;
	
	// if our check type is a pointer type
	int real_degree;
	if (UNIQTYPE_IS_POINTER_TYPE(test_uniqtype)
			&& 0 != (real_degree = is_generic_pointer_type_of_degree_at_least(test_uniqtype, 1)))
	{
		// the pointed-to object must have at least the same degree
		if (holds_pointer_of_degree(cur_alloc_subobj_uniqtype, real_degree, 0))
		{
			++__libcrunch_succeeded;
			return 1;
		}
		
		// nothing is like a non-generic pointer
		goto try_deeper;
	}
	
	// arrays are special
	_Bool matches;
	if (__builtin_expect(UNIQTYPE_IS_ARRAY_TYPE(cur_alloc_subobj_uniqtype)
			|| UNIQTYPE_IS_ARRAY_TYPE(test_uniqtype), 0))
	{
		matches = 
			test_uniqtype == cur_alloc_subobj_uniqtype
		||  (UNIQTYPE_IS_ARRAY_TYPE(test_uniqtype) && UNIQTYPE_ARRAY_LENGTH(test_uniqtype) == 1 
				&& UNIQTYPE_ARRAY_ELEMENT_TYPE(test_uniqtype) == cur_alloc_subobj_uniqtype)
		||  (UNIQTYPE_IS_ARRAY_TYPE(cur_alloc_subobj_uniqtype) && UNIQTYPE_ARRAY_LENGTH(cur_alloc_subobj_uniqtype) == 1
				&& UNIQTYPE_ARRAY_ELEMENT_TYPE(cur_alloc_subobj_uniqtype) == test_uniqtype);
		/* We don't need to allow an array of one blah to be like a different
		 * array of one blah, because they should be the same type. 
		 * FIXME: there's a difficult case: an array of statically unknown length, 
		 * which happens to have length 1. */
		if (matches) goto loosely_like_a_succeeded; else goto try_deeper;
	}
	
	/* We might have base types with signedness complements. */
	if (__builtin_expect(
			!UNIQTYPE_IS_BASE_TYPE(cur_alloc_subobj_uniqtype) 
			|| UNIQTYPE_IS_BASE_TYPE(test_uniqtype), 0))
	{
		/* Does the cur obj type have a signedness complement matching the test type? */
		if (UNIQTYPE_BASE_TYPE_SIGNEDNESS_COMPLEMENT(cur_alloc_subobj_uniqtype)
				== test_uniqtype) goto loosely_like_a_succeeded;
		/* Does the test type have a signedness complement matching the cur obj type? */
		if (UNIQTYPE_BASE_TYPE_SIGNEDNESS_COMPLEMENT(test_uniqtype)
				== cur_alloc_subobj_uniqtype) goto loosely_like_a_succeeded;
	}
	
	/* Okay, we can start the like-a test: for each element in the test type, 
	 * do we have a type-equivalent in the object type?
	 * 
	 * We make an exception for arrays of char (signed or unsigned): if an
	 * element in the test type is such an array, we skip over any number of
	 * fields in the object type, until we reach the offset of the end element.  */
	unsigned i_obj_subobj = 0, i_test_subobj = 0;
	if (test_uniqtype != cur_alloc_subobj_uniqtype)
		debug_printf(0, "__loosely_like_a proceeding on subobjects of (test) %s and (object) %s\n",
			NAME_FOR_UNIQTYPE(test_uniqtype), NAME_FOR_UNIQTYPE(cur_alloc_subobj_uniqtype));
	for (; 
		i_obj_subobj < UNIQTYPE_COMPOSITE_MEMBER_COUNT(cur_alloc_subobj_uniqtype)
			&& i_test_subobj < UNIQTYPE_COMPOSITE_MEMBER_COUNT(test_uniqtype); 
		++i_test_subobj, ++i_obj_subobj)
	{
		debug_printf(0, "Subobject types are (test) %s and (object) %s\n",
			NAME_FOR_UNIQTYPE((struct uniqtype *) test_uniqtype->related[i_test_subobj].un.memb.ptr), 
			NAME_FOR_UNIQTYPE((struct uniqtype *) cur_alloc_subobj_uniqtype->related[i_obj_subobj].un.memb.ptr));
		
		if (__builtin_expect(UNIQTYPE_IS_ARRAY_TYPE(test_uniqtype->related[i_test_subobj].un.memb.ptr)
			&& (UNIQTYPE_ARRAY_ELEMENT_TYPE(test_uniqtype->related[i_test_subobj].un.memb.ptr)
					== pointer_to___uniqtype__signed_char
			|| UNIQTYPE_ARRAY_ELEMENT_TYPE(test_uniqtype->related[i_test_subobj].un.memb.ptr)
					== pointer_to___uniqtype__unsigned_char), 0))
		{
			// we will skip this field in the test type
			signed target_off =
				UNIQTYPE_COMPOSITE_MEMBER_COUNT(test_uniqtype) > i_test_subobj + 1
			 ?  test_uniqtype->related[i_test_subobj + 1].un.memb.off
			 :  test_uniqtype->related[i_test_subobj].un.memb.off
			      + test_uniqtype->related[i_test_subobj].un.memb.ptr->pos_maxoff;
			
			// ... if there's more in the test type, advance i_obj_subobj
			while (i_obj_subobj + 1 < UNIQTYPE_COMPOSITE_MEMBER_COUNT(cur_alloc_subobj_uniqtype) &&
				cur_alloc_subobj_uniqtype->related[i_obj_subobj + 1].un.memb.off < target_off) ++i_obj_subobj;
			/* We fail if we ran out of stuff in the actual object type
			 * AND there is more to go in the test (cast-to) type. */
			if (i_obj_subobj + 1 >= UNIQTYPE_COMPOSITE_MEMBER_COUNT(cur_alloc_subobj_uniqtype)
			 && UNIQTYPE_COMPOSITE_MEMBER_COUNT(test_uniqtype) > i_test_subobj + 1) goto try_deeper;
				
			continue;
		}
		
		int generic_ptr_degree = 0;
		matches = 
				(test_uniqtype->related[i_test_subobj].un.memb.off
				== cur_alloc_subobj_uniqtype->related[i_obj_subobj].un.memb.off)
		 && (
				// exact match
				(test_uniqtype->related[i_test_subobj].un.memb.ptr
				 == cur_alloc_subobj_uniqtype->related[i_obj_subobj].un.memb.ptr)
				|| // loose match: if the test type has a generic ptr...
				(
					0 != (
						generic_ptr_degree = is_generic_pointer_type_of_degree_at_least(
							test_uniqtype->related[i_test_subobj].un.memb.ptr, 1)
					)
					&& pointer_has_degree(
						(struct uniqtype *) cur_alloc_subobj_uniqtype->related[i_obj_subobj].un.memb.ptr,
						generic_ptr_degree
					)
				)
				|| // loose match: signed/unsigned
				(UNIQTYPE_IS_BASE_TYPE(test_uniqtype->related[i_test_subobj].un.memb.ptr)
				 && UNIQTYPE_IS_BASE_TYPE(cur_alloc_subobj_uniqtype->related[i_obj_subobj].un.memb.ptr)
				 && 
				 (UNIQTYPE_BASE_TYPE_SIGNEDNESS_COMPLEMENT((struct uniqtype *) test_uniqtype->related[i_test_subobj].un.memb.ptr)
					== ((struct uniqtype *) cur_alloc_subobj_uniqtype->related[i_obj_subobj].un.memb.ptr))
				)
		);
		if (!matches) goto try_deeper;
	}
	// if we terminated because we ran out of fields in the target type, fail
	if (i_test_subobj < UNIQTYPE_COMPOSITE_MEMBER_COUNT(test_uniqtype)) goto try_deeper;
	
	// if we got here, we succeeded
	goto loosely_like_a_succeeded;
	
	_Bool success;
	try_deeper:
		debug_printf(0, "No dice; will try the object type one level down if there is one...\n");
		success = __liballocs_first_subobject_spanning(
				&target_offset_within_alloc_uniqtype, &cur_alloc_subobj_uniqtype, &cur_containing_uniqtype,
				&cur_contained_pos);
		if (!success) goto loosely_like_a_failed;
		debug_printf(0, "... got %s\n", NAME_FOR_UNIQTYPE(cur_alloc_subobj_uniqtype));

	} while (1);
	
loosely_like_a_succeeded:
	if (test_uniqtype != alloc_uniqtype) debug_printf(0, "__loosely_like_a succeeded! test type %s, allocation type %s\n",
		NAME_FOR_UNIQTYPE(test_uniqtype), NAME_FOR_UNIQTYPE(alloc_uniqtype));
	++__libcrunch_succeeded;
	return 1;
	
	// if we got here, we've failed
	// if we got here, the check failed
loosely_like_a_failed:
	if (__currently_allocating || __currently_freeing) 
	{
		++__libcrunch_failed_in_alloc;
		// suppress warning
	}
	else
	{
		++__libcrunch_failed;
		if (!is_suppressed(UNIQTYPE_NAME(test_uniqtype), __builtin_return_address(0), alloc_uniqtype ? UNIQTYPE_NAME(alloc_uniqtype) : NULL))
		{
			if (should_report_failure_at(__builtin_return_address(0)))
			{
				debug_printf(0, "Failed check __loosely_like_a(%p, %p a.k.a. \"%s\") at %p (%s), allocation was a %s%s%s originating at %p\n", 
					obj, test_uniqtype, UNIQTYPE_NAME(test_uniqtype),
					__builtin_return_address(0), format_symbolic_address(__builtin_return_address(0)),
					a ? a->name : "(no allocator)",
					(ALLOC_IS_DYNAMICALLY_SIZED(alloc_start, alloc_site) && alloc_uniqtype && alloc_size_bytes > alloc_uniqtype->pos_maxoff) ? " allocation of " : " ", 
					NAME_FOR_UNIQTYPE(alloc_uniqtype), 
					alloc_site);
			}
		}
	}
	return 1; // HACK: so that the program will continue
}

/* This is called to check casts to a user-defined type that has
 * been declared, but not defined, in that compilation unit. I.e. The type is
 * opaque, so it just checks that the pointer is pointing to an instance of
 * that type name.
*/
int __named_a_internal(const void *obj, const void *r)
{
	PRINTD("__named_a_internal");
	/* This relies on getting uniqtype names which needs
	 * __liballocs_uniqtype_symbol_name, which is not implemented, so pass.
	*/
	return 1;
}

/* Checking function arguments' types? */
int __check_args_internal(const void *obj, int nargs, ...)
{
	PRINTD("__check_args_internal");
	if (!__libcrunch_check_init()) {
		__libcrunch_called_before_init++;
		return 1;
	}

	struct allocator *a;
	const void *alloc_start;
	unsigned long alloc_size_bytes;
	struct uniqtype *alloc_uniqtype = (struct uniqtype *)0;
	const void *alloc_site;
	signed target_offset_within_uniqtype;

	struct uniqtype *fun_uniqtype = NULL;
	
	struct liballocs_err *err = __liballocs_get_alloc_info(obj, 
		&a,
		&alloc_start,
		&alloc_size_bytes,
		&fun_uniqtype,
		&alloc_site);
	
	if (err != NULL) {
		__libcrunch_failed_liballocs_err++;
		return 1;
	}
	
	assert(fun_uniqtype);
	assert(alloc_start == obj);
	assert(UNIQTYPE_IS_SUBPROGRAM_TYPE(fun_uniqtype));
	
	/* Walk the arguments that the function expects. Simultaneously, 
	 * walk our arguments. */
	va_list ap;
	va_start(ap, nargs);
	
	// FIXME: this function screws with the __libcrunch_begun count somehow
	// -- try hello-funptr
	
	_Bool success = 1;
	int i;
	for (i = 0; i < nargs && i < fun_uniqtype->un.subprogram.narg; ++i)
	{
		void *argval = va_arg(ap, void*);
		/* related[0] is the return type */
		struct uniqtype *expected_arg = fun_uniqtype->related[i+MIN(1,fun_uniqtype->un.subprogram.nret)].un.t.ptr;
		/* We only check casts that are to pointer targets types.
		 * How to test this? */
		if (UNIQTYPE_IS_POINTER_TYPE(expected_arg))
		{
			struct uniqtype *expected_arg_pointee_type = UNIQTYPE_POINTEE_TYPE(expected_arg);
			success &= __is_aU(argval, expected_arg_pointee_type);
		}
		if (!success) break;
	}
	if (i == nargs && i < fun_uniqtype->un.subprogram.narg)
	{
		/* This means we exhausted nargs before we got to the end of the array.
		 * In other words, the function takes more arguments than we were passed
		 * for checking, i.e. more arguments than the call site passes. 
		 * Not good! */
		success = 0;
	}
	if (i < nargs && i == fun_uniqtype->un.subprogram.narg)
	{
		/* This means we were passed more args than the uniqtype told us about. 
		 * FIXME: check for its varargs-ness. If it's varargs, we're allowed to
		 * pass more. For now, fail. */
		success = 0;
	}
	
	va_end(ap);
	
	/* NOTE that __check_args is not just one "test"; it's many. 
	 * So we don't maintain separate counts here; our use of __is_aU above
	 * will create many counts. */
	
	return success ? 0 : i; // 0 means success here
	return 1;
}

/* is_a_internal but for function pointer casts */
int __is_a_function_refining_internal(const void *obj, const void *arg)
{
	PRINTD("__is_a_function_refining_internal");
	/* We might not be initialized yet (recall that __libcrunch_global_init is 
	 * not a constructor, because it's not safe to call super-early). */
	if (!__libcrunch_check_init()) {
		__libcrunch_called_before_init++;
		return 1;
	}
	
	const struct uniqtype *test_uniqtype = (const struct uniqtype *) arg;
	struct allocator *a = NULL;
	const void *alloc_start;
	unsigned long alloc_size_bytes;
	struct uniqtype *alloc_uniqtype = (struct uniqtype *)0;
	const void *alloc_site;
	signed target_offset_within_uniqtype;
	
	struct liballocs_err *err = __liballocs_get_alloc_info(obj, 
		&a,
		&alloc_start,
		&alloc_size_bytes,
		&alloc_uniqtype,
		&alloc_site);
	
	if (__builtin_expect(err != NULL, 0))
	{
		__libcrunch_failed_liballocs_err++;
		return 1;
	}
	
	/* If we're offset-zero, that's good... */
	if (alloc_start == obj)
	{
		/* If we're an exact match, that's good.... */
		if (alloc_uniqtype == arg)
		{
			++__libcrunch_succeeded;
			return 1;
		}
		else
		{
			/* If we're not a function, that's bad. */
			if (UNIQTYPE_IS_SUBPROGRAM_TYPE(alloc_uniqtype))
			{
				/* If our argument counts don't match, that's bad. */
				if (alloc_uniqtype->un.subprogram.narg == test_uniqtype->un.subprogram.narg)
				{
					/* For each argument, we want to make sure that 
					 * the "implicit" cast done on the argument, from
					 * the cast-from type to the cast-to type, i.e. that 
					 * the passed argument *is_a* received argument, i.e. that
					 * the cast-to argument *is_a* cast-from argument. */
					_Bool success = 1;
					/* Recall: return type is in [0] and arguments are in 1..array_len. */
					
					/* Would the cast from the return value to the post-cast return value
					 * always succeed? If so, this cast is okay. */
					struct uniqtype *alloc_return_type = alloc_uniqtype->related[0].un.t.ptr;
					struct uniqtype *cast_return_type = test_uniqtype->related[0].un.t.ptr;
					
					/* HACK: a little bit of C-specifity is creeping in here.
					 * FIXME: adjust this to reflect sloppy generic-pointer-pointer matches! 
					      (only if LIBCRUNCH_STRICT_GENERIC_POINTERS not set) */
					/* HACK: referencing uniqtypes directly from libcrunch is problematic
					 * for COMDAT / section group / uniqing reasons. Ideally we wouldn't
					 * do this. To prevent non-uniquing, we need to avoid linking
					 * uniqtypes into the preload .so. But we can't rely on any particular
					 * uniqtypes being in the executable; and if they're in a library
					 * won't let us bind to them from the preload library (whether the
					 * library is linked at startup or later, as it happens).  One workaround:
					 * use the _nonshared.a hack for -lcrunch_stubs too, so that all 
					 * libcrunch-enabled executables necessarily have __uniqtype__void
					 * and __uniqtype__signed_char in the executable.
					 * ARGH, but we still can't bind to these from the preload lib.
					 * (That's slightly surprising semantics, but it's what I observe.)
					 * We have to use dynamic linking. */
					#define would_always_succeed(from, to) \
						( \
							!UNIQTYPE_IS_POINTER_TYPE((to)) \
						||  (UNIQTYPE_POINTEE_TYPE((to)) == pointer_to___uniqtype__void) \
						||  (UNIQTYPE_POINTEE_TYPE((to)) == pointer_to___uniqtype__signed_char) \
						||  (UNIQTYPE_IS_POINTER_TYPE((from)) && \
							__liballocs_find_matching_subobject( \
							/* target_offset_within_uniqtype */ 0, \
							/* cur_obj_uniqtype */ UNIQTYPE_POINTEE_TYPE((from)), \
							/* test_uniqtype */ UNIQTYPE_POINTEE_TYPE((to)), \
							/* last_attempted_uniqtype */ NULL, \
							/* last_uniqtype_offset */ NULL, \
							/* p_cumulative_offset_searched */ NULL, \
							/* p_cur_containing_uniqtype */ NULL, \
							/* p_cur_contained_pos */ NULL)) \
						)
						
					/* ARGH. Are these the right way round?  
					 * The "implicit cast" is from the alloc'd return type to the 
					 * cast-to return type. */
					success &= would_always_succeed(alloc_return_type, cast_return_type);
					
					if (success) for (int i_rel = MIN(1, alloc_uniqtype->un.subprogram.nret);
						i_rel < MIN(1, alloc_uniqtype->un.subprogram.nret) + 
								alloc_uniqtype->un.subprogram.narg; ++i_rel)
					{
						/* ARGH. Are these the right way round?  
						 * The "implicit cast" is from the cast-to arg type to the 
						 * alloc'd arg type. */
						success &= would_always_succeed(
							test_uniqtype->related[i_rel].un.t.ptr,
							alloc_uniqtype->related[i_rel].un.t.ptr
						);

						if (!success) break;
					}
					
					if (success)
					{
						++__libcrunch_succeeded;
						return 1;
					}
				}
			}
		}
	}
	
	// if we got here, the check failed
	if (__currently_allocating || __currently_freeing)
	{
		++__libcrunch_failed_in_alloc;
		// suppress warning
	}
	else
	{
		++__libcrunch_failed;
		if (!is_suppressed(UNIQTYPE_NAME(test_uniqtype), __builtin_return_address(0), alloc_uniqtype ? UNIQTYPE_NAME(alloc_uniqtype) : NULL))
		{
			if (should_report_failure_at(__builtin_return_address(0)))
			{
				if (last_failed_site == __builtin_return_address(0)
						&& last_failed_deepest_subobject_type == alloc_uniqtype)
				{
					++repeat_suppression_count;
				}
				else
				{
					if (repeat_suppression_count > 0)
					{
						/* debug_printf(0, "Suppressed %ld further occurrences of the previous error\n", */ 
						/* 		repeat_suppression_count); */
						PRINTD1(
							"Suppressed %ld further occurrences of the previous error",
							repeat_suppression_count
						);
					}

					/* debug_printf(0, "Failed check __is_a_function_refining(%p, %p a.k.a. \"%s\") at %p (%s), " */
					/* 		"found an allocation of a %s%s%s " */
					/* 		"originating at %p\n", */ 
					/* 	obj, test_uniqtype, UNIQTYPE_NAME(test_uniqtype), */
					/* 	__builtin_return_address(0), format_symbolic_address(__builtin_return_address(0)), */ 
					/* 	a ? a->name : "(no allocator)", */
					/* 	(ALLOC_IS_DYNAMICALLY_SIZED(alloc_start, alloc_site) && alloc_uniqtype && alloc_size_bytes > alloc_uniqtype->pos_maxoff) ? " allocation of " : " ", */ 
					/* 	NAME_FOR_UNIQTYPE(alloc_uniqtype), */
					/* 	alloc_site); */
					PRINTD2("Failed check __is_a_function_refining(%p, %p)", obj, test_uniqtype);
					PRINTD1("a.k.a. \"%s\"", UNIQTYPE_NAME(test_uniqtype));
					PRINTD2(
						"at %p offset (%s) ",
						__builtin_return_address(0),
						format_symbolic_address(__builtin_return_address(0))
					);
					PRINTD2(
						"found an allocation of a %s%s",
						a ? a->name : "(no allocator)",
						(ALLOC_IS_DYNAMICALLY_SIZED(alloc_start, alloc_site) && alloc_uniqtype && alloc_size_bytes > alloc_uniqtype->pos_maxoff) ? " allocation of " : " "
					);
					PRINTD1(
						"%s",
						NAME_FOR_UNIQTYPE(alloc_uniqtype)
					);
					PRINTD1(
						"originating at %p",
						alloc_site
					);

					last_failed_site = __builtin_return_address(0);
					last_failed_deepest_subobject_type = alloc_uniqtype;
					repeat_suppression_count = 0;
				}
			}
		}
	}
	return 1; // HACK: so that the program will continue
}

int __is_a_pointer_of_degree_internal(const void *obj, int d)
{
	PRINTD("__is_a_pointer_of_degree_internal");
	// TODO
	return 1;
}

/* If we're writing into a non-generic pointer, 
 * __is_a(value, target's pointee type) must hold. It could hold at
 * any level in the stack of subobjects that "value" points into, so
 * we need the full __is_a check.
 * 
 * If we're writing into a generic pointer, we're more relaxed, but 
 * if target has degree 3, "value" must be the address of a degree2 pointer.
 */
struct match_cb_args
{
	struct uniqtype *type_of_pointer_being_stored_to;
	signed target_offset;
};
static int match_pointer_subobj_strict_cb(struct uniqtype *spans, signed span_start_offset, 
		unsigned depth, struct uniqtype *containing, struct uniqtype_rel_info *contained_pos, 
		signed containing_span_start_offset, void *arg)
{
	/* We're storing a pointer that is legitimately a pointer to t (among others) */
	struct uniqtype *t = spans;
	struct match_cb_args *args = (struct match_cb_args *) arg;
	struct uniqtype *type_we_can_store = UNIQTYPE_POINTEE_TYPE(args->type_of_pointer_being_stored_to);
	
	if (span_start_offset == args->target_offset && type_we_can_store == t)
	{
		return 1;
	}
	return 0;
}
static int match_pointer_subobj_generic_cb(struct uniqtype *spans, signed span_start_offset, 
		unsigned depth, struct uniqtype *containing, struct uniqtype_rel_info *contained_pos, 
		signed containing_span_start_offset, void *arg)
{
	/* We're storing a pointer that is legitimately a pointer to t (among others) */
	struct uniqtype *t = spans;
	struct match_cb_args *args = (struct match_cb_args *) arg;
	
	int degree_of_pointer_stored_to = pointer_degree(args->type_of_pointer_being_stored_to);

	if (span_start_offset == 0 && pointer_has_degree(t, degree_of_pointer_stored_to - 1))
	{
		return 1;
	}
	else return 0;
}
int __can_hold_pointer_internal(const void *obj, const void *value)
{
	PRINTD2("__can_hold_pointer_internal: obj: %p, value: %p", obj, value);
	/* We might not be initialized yet (recall that __libcrunch_global_init is 
	 * not a constructor, because it's not safe to call super-early). */
	if (!__libcrunch_check_init()) {
		__libcrunch_called_before_init++;
		return 1;
	}

	/* To hold a pointer, we must be a pointer. Find the pointer subobject at `obj'. */
	struct allocator *obj_a = NULL;
	const void *obj_alloc_start;
	unsigned long obj_alloc_size_bytes;
	struct uniqtype *obj_alloc_uniqtype = (struct uniqtype *)0;
	const void *obj_alloc_site;
	
	struct liballocs_err *obj_err = __liballocs_get_alloc_info(obj, 
		&obj_a,
		&obj_alloc_start,
		&obj_alloc_size_bytes,
		&obj_alloc_uniqtype,
		&obj_alloc_site);
	
	if (__builtin_expect(obj_err != NULL, 0))
	{
		return 1;
	}
	
	signed obj_target_offset_within_uniqtype = (char*) obj - (char*) obj_alloc_start;
	/* If we're searching in a heap array, we need to take the offset modulo the 
	 * element size. Otherwise just take the whole-block offset. */
	if (ALLOC_IS_DYNAMICALLY_SIZED(obj_alloc_start, obj_alloc_site)
			&& obj_alloc_uniqtype
			&& obj_alloc_uniqtype->pos_maxoff != 0)
	{
		obj_target_offset_within_uniqtype %= obj_alloc_uniqtype->pos_maxoff;
	}
	
	struct uniqtype *cur_obj_uniqtype = obj_alloc_uniqtype;
	struct uniqtype *cur_containing_uniqtype = NULL;
	struct uniqtype_rel_info *cur_contained_pos = NULL;
	
	/* Descend the subobject hierarchy until we can't go any further (since pointers
	 * are atomic. */
	_Bool success = 1;
	while (success)
	{
		success = __liballocs_first_subobject_spanning(
			&obj_target_offset_within_uniqtype, &cur_obj_uniqtype, &cur_containing_uniqtype,
			&cur_contained_pos);
	}
	struct uniqtype *type_of_pointer_being_stored_to = cur_obj_uniqtype;
	
	struct allocator *value_a = NULL;
	const void *value_alloc_start = NULL;
	unsigned long value_alloc_size_bytes = (unsigned long) -1;
	struct uniqtype *value_alloc_uniqtype = (struct uniqtype *)0;
	const void *value_alloc_site = NULL;
	_Bool value_contract_is_specialisable = 0;
	
	/* Might we have a pointer? */
	if (obj_target_offset_within_uniqtype == 0 && UNIQTYPE_IS_POINTER_TYPE(cur_obj_uniqtype))
	{
		int d;
		struct uniqtype *ultimate_pointee_type;
		pointer_degree_and_ultimate_pointee_type(type_of_pointer_being_stored_to, &d, &ultimate_pointee_type);
		assert(d > 0);
		assert(ultimate_pointee_type);
		
		/* Is this a generic pointer, of zero degree? */
		_Bool is_generic = is_generic_ultimate_pointee(ultimate_pointee_type);
		if (d == 1 && is_generic)
		{
			/* We pass if the value as (at least) equal degree.
			 * Note that the value is "off-by-one" in degree: 
			 * if target has degree 1, any address will do. */
			++__libcrunch_succeeded;
			return 1;
		}
		
		/* If we got here, we're going to have to understand `value',
		 * whether we're generic or not. */
		
// 		if (is_generic_ultimate_pointee(ultimate_pointee_type))
// 		{
// 			/* if target has degree 2, "value" must be the address of a degree1 pointer.
// 			 * if target has degree 3, "value" must be the address of a degree2 pointer.
// 			 * Etc. */
// 			struct uniqtype *value_pointee_uniqtype
// 			 = __liballocs_get_alloc_type_innermost(value);
// 			assert(value_pointee_uniqtype);
// 			if (pointer_has_degree(value_pointee_uniqtype, d - 1))
// 			{
// 				++__libcrunch_succeeded;
// 				return 1;
// 			}
// 		}

		struct liballocs_err *value_err = __liballocs_get_alloc_info(value, 
			&value_a,
			&value_alloc_start,
			&value_alloc_size_bytes,
			&value_alloc_uniqtype,
			&value_alloc_site);

		if (__builtin_expect(value_err != NULL, 0)) return 1; // liballocs has already counted this abort
		
		signed value_target_offset_within_uniqtype = (char*) value - (char*) value_alloc_start;
		/* If we're searching in a heap array, we need to take the offset modulo the 
		 * element size. Otherwise just take the whole-block offset. */
		if (ALLOC_IS_DYNAMICALLY_SIZED(value_alloc_start, value_alloc_site)
				&& value_alloc_uniqtype
				&& value_alloc_uniqtype->pos_maxoff != 0)
		{
			value_target_offset_within_uniqtype %= value_alloc_uniqtype->pos_maxoff;
		}
		/* HACK: preserve looseness of value. */
		reinstate_looseness_if_necessary(value_alloc_start, value_alloc_site, value_alloc_uniqtype);

		/* See if the top-level object matches */
		struct match_cb_args args = {
			.type_of_pointer_being_stored_to = type_of_pointer_being_stored_to,
			.target_offset = value_target_offset_within_uniqtype
		};
		int ret = (is_generic ? match_pointer_subobj_generic_cb : match_pointer_subobj_strict_cb)(
			value_alloc_uniqtype,
			0,
			0,
			NULL, NULL,
			0, 
			&args
		);
		/* Here we walk the subobject hierarchy until we hit 
		 * one that is at the right offset and equals test_uniqtype.
		 
		 __liballocs_walk_subobjects_starting(
		 
		 ) ... with a cb that tests equality with test_uniqtype and returns 
		 
		 */
		if (!ret) ret = __liballocs_walk_subobjects_spanning(value_target_offset_within_uniqtype, 
			value_alloc_uniqtype, 
			is_generic ? match_pointer_subobj_generic_cb : match_pointer_subobj_strict_cb, 
			&args);
		
		if (ret)
		{
			++__libcrunch_succeeded;
			return 1;
		}
	}
	/* Can we specialise the contract of
	 * 
	 *  either the written-to pointer
	 * or
	 *  the object pointed to 
	 *
	 * so that the check would succeed?
	 * 
	 * We can only specialise the contract of as-yet-"unused" objects.
	 * Might the written-to pointer be as-yet-"unused"?
	 * We know the check failed, so currently it can't point to the
	 * value we want it to, either because it's generic but has too-high degree
	 * or because it's non-generic and doesn't match "value".
	 * These don't seem like cases we want to specialise. The only one
	 * that makes sense is replacing it with a lower degree, and I can't see
	 * any practical case where that would arise (e.g. allocating sizeof void***
	 * when you actually want void** -- possible but weird).
	 * 
	 * Might the "value" object be as-yet-unused?
	 * Yes, certainly.
	 * The check failed, so it's the wrong type.
	 * If a refinement of its type yields a "right" type,
	 * we might be in business.
	 * What's a "right" type?
	 * If the written-to pointer is not generic, then it's that target type.
	 */
	// FIXME: use value_alloc_start to avoid another heap lookup
	struct insert *value_object_info = __liballocs_get_insert(value);
	/* HACK: until we have a "loose" bit */
	struct uniqtype *pointee = UNIQTYPE_POINTEE_TYPE(type_of_pointer_being_stored_to);

	if (!is_generic_pointer_type(type_of_pointer_being_stored_to)
		&& value_alloc_uniqtype
		&& UNIQTYPE_IS_POINTER_TYPE(value_alloc_uniqtype)
		&& is_generic_pointer_type(value_alloc_uniqtype)
		&& value_object_info
		&& STORAGE_CONTRACT_IS_LOOSE(value_object_info, value_alloc_site))
	{
		value_object_info->alloc_site_flag = 1;
		value_object_info->alloc_site = (uintptr_t) pointee; // i.e. *not* loose!
		debug_printf(0, "libcrunch: specialised allocation at %p from %s to %s\n", 
			value, NAME_FOR_UNIQTYPE(value_alloc_uniqtype), NAME_FOR_UNIQTYPE(pointee));
		++__libcrunch_lazy_heap_type_assignment;
		return 1;
	}

can_hold_pointer_failed:
	if (__currently_allocating || __currently_freeing)
	{
		++__libcrunch_failed_in_alloc;
		// suppress warning
	}
	else
	{
		++__libcrunch_failed;
		// split up debug statements are because printf was segfaulting on long
		// strings no idea why
		debug_printf(0,
			"Failed check __can_hold_pointer(%p, %p) at %p (%s), ",
			obj, value,
			__builtin_return_address(0),
			format_symbolic_address(__builtin_return_address(0))
		);
		debug_printf(0,
			"target pointer is a %s, %ld ",
			NAME_FOR_UNIQTYPE(type_of_pointer_being_stored_to),
			(long)((char*) obj - (char*) obj_alloc_start)
		);
		debug_printf(0,
			"bytes into a %s%s%s originating at %p, ",
			obj_a ? obj_a->name : "(no allocator)",
			(ALLOC_IS_DYNAMICALLY_SIZED(obj_alloc_start, obj_alloc_site) && obj_alloc_uniqtype && obj_alloc_size_bytes > obj_alloc_uniqtype->pos_maxoff) ? " allocation of " : " ", 
			NAME_FOR_UNIQTYPE(obj_alloc_uniqtype),
			obj_alloc_site
		);
		debug_printf(0,
			"value points %ld bytes into a ",
			(long)((char*) value - (char*) value_alloc_start),
		);
		debug_printf(0,
			"%s%s%s originating at %p\n",
			value_a ? value_a->name : "(no allocator)",
			(ALLOC_IS_DYNAMICALLY_SIZED(value_alloc_start, value_alloc_site) && value_alloc_uniqtype && value_alloc_size_bytes > value_alloc_uniqtype->pos_maxoff) ? " allocation of " : " ",
			NAME_FOR_UNIQTYPE(value_alloc_uniqtype)
		);
	}
	return 1; // fail, but program continues
	
}
