#define _GNU_SOURCE

/* Debug printouts */
#define DEBUG_STUBS
#ifdef DEBUG_STUBS
#include <sys/ktr.h>
#define PRINTD(x) CTR0(KTR_PTRACE, x)
#define PRINTD1(s, x) CTR1(KTR_PTRACE, s,x)
#else
#define PRINTD(x) (void)
#define PRINTD1(s, x) (void)
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
 * liballocs.c
*/



/*
 * liballocs.h
*/



/*
 * libcrunch.c
*/



/*
 * ???
*/



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


_Bool __libcrunch_is_initialized = 1;
extern _Bool our_init_flag __attribute__(
	(visibility("hidden"), alias("__libcrunch_is_initialized"))
);

struct __libcrunch_cache __libcrunch_is_a_cache; // all zeroes
unsigned long int __libcrunch_begun = 0;
unsigned long int __libcrunch_failed = 0;
unsigned long int __libcrunch_succeeded = 0;
unsigned long int __libcrunch_aborted_typestr = 0;
unsigned long int __libcrunch_is_a_hit_cache = 0;
unsigned long int __libcrunch_created_invalid_pointer = 0;
unsigned long int __libcrunch_checked_pointer_adjustments = 0;
unsigned long int __libcrunch_primary_secondary_transitions = 0;
unsigned long int __libcrunch_fault_handler_fixups = 0;

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
	return 0;
}

int __libcrunch_check_init(void)
{
	PRINTD("__libcrunch_check_init");
	return 0;
}

int __is_a_internal(const void *obj, const void *arg)
{
	PRINTD("__is_a_internal");
	PRINTD1(
		"__is_a_internal, address pointed to by obj: %u",
		obj
	);
	return 1;
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
