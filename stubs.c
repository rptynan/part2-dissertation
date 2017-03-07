#define _GNU_SOURCE
/* #include <sys/stdint.h> */
/* #include <stdio.h> */
/* #include <sys/mman.h> */
#include <sys/ktr.h>


/* rpt */
void __assert_fail(const char *__assertion, const char *__file,
    unsigned int __line, const char *__function) { return; }
void __alloca_allocator_notify(void *new_userchunkaddr, unsigned long modified_size, 
        unsigned long *frame_counter, const void *caller, const void *sp_at_caller,
        const void *bp_at_caller) { return; }
void __liballocs_unindex_stack_objects_counted_by(unsigned long *bytes_counter, void *frame_addr) { return; }
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
extern _Bool our_init_flag __attribute__((visibility("hidden"),alias("__libcrunch_is_initialized")));

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
	CTR0(KTR_PTRACE, "__liballocs_systrap_init");
	return;
}
static void init(void) __attribute__((constructor));
static void init(void)
{
	CTR0(KTR_PTRACE, "init");
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
	CTR0(KTR_PTRACE, "__libcrunch_scan_lazy_typenames");
}

int __libcrunch_check_init(void)
{
	CTR0(KTR_PTRACE, "__libcrunch_check_init");
	return 0;
}

int __libcrunch_global_init(void)
{
	CTR0(KTR_PTRACE, "__libcrunch_global_init");
	return 0;
}

int __is_a_internal(const void *obj, const void *r)
{
	CTR0(KTR_PTRACE, "__is_a_internal");
	CTR1(
		KTR_PTRACE,
		"* __is_a_internal, address pointed to by obj: %d",
		obj
	);
	return 1;
}

int __like_a_internal(const void *obj, const void *r)
{
	CTR0(KTR_PTRACE, "__like_a_internal");
	return 1;
}

int __loosely_like_a_internal(const void *obj, const void *r)
{
	CTR0(KTR_PTRACE, "__loosely_like_a_internal");
	return 1;
}

int __named_a_internal(const void *obj, const void *r)
{
	CTR0(KTR_PTRACE, "__named_a_internal");
	return 1;
}

int __check_args_internal(const void *obj, int nargs, ...)
{
	CTR0(KTR_PTRACE, "__check_args_internal");
	return 1;
}

int __is_a_function_refining_internal(const void *obj, const void *r)
{
	CTR0(KTR_PTRACE, "__is_a_function_refining_internal");
	return 1;
}
int __is_a_pointer_of_degree_internal(const void *obj, int d)
{
	CTR0(KTR_PTRACE, "__is_a_pointer_of_degree_internal");
	return 1;
}
int __can_hold_pointer_internal(const void *obj, const void *target)
{
	CTR0(KTR_PTRACE, "__can_hold_pointer_internal");
	return 1;
}
