#ifndef LIBALLOCS_H
#define LIBALLOCS_H

#include <libcrunchk/include/uniqtype.h>
#include <libcrunchk/include/allocmeta.h>
#include <libcrunchk/include/libcrunch.h>  // for debug prints

/*
 * misc
*/
extern void __assert_fail(
	const char *__assertion,
	const char *__file,
	unsigned int __line,
	const char *__function
);
#define __liballocs_private_assert(cond, reason, f, l, fn) \
	(cond ? (void)0 : __assert_fail(reason, f, l, fn))
#define assert(e) (e ? (void)0 : \
	__assert_fail(#e, __FILE__, __LINE__, __func__))  // in libcrunch.c


/*
 * liballocs structs
*/
#define ALLOC_IS_DYNAMICALLY_SIZED(all, as) ((all) != (as))

/* err structs */
struct liballocs_err
{
	const char *message;
};
typedef struct liballocs_err *liballocs_err_t;
extern struct liballocs_err __liballocs_err_not_impl;
extern struct liballocs_err __liballocs_err_abort;
extern struct liballocs_err __liballocs_err_stack_walk_step_failure;
extern struct liballocs_err __liballocs_err_stack_walk_reached_higher_frame ;
extern struct liballocs_err __liballocs_err_stack_walk_reached_top_of_stack ;
extern struct liballocs_err __liballocs_err_unknown_stack_walk_problem ;
extern struct liballocs_err __liballocs_err_unindexed_heap_object;
extern struct liballocs_err __liballocs_err_unrecognised_alloc_site;
extern struct liballocs_err __liballocs_err_unrecognised_static_object;
extern struct liballocs_err __liballocs_err_object_of_unknown_storage;

/* counters */
extern unsigned long __liballocs_aborted_stack;
extern unsigned long __liballocs_aborted_static;
extern unsigned long __liballocs_aborted_unknown_storage;
extern unsigned long __liballocs_hit_heap_case;
extern unsigned long __liballocs_hit_stack_case;
extern unsigned long __liballocs_hit_static_case;
extern unsigned long __liballocs_aborted_unindexed_heap;
extern unsigned long __liballocs_aborted_unrecognised_allocsite;

/* special uniqtype to indicate that this type was never encountered before and
 * so should be set on first is_aU() check. hacky, yes. */
extern struct uniqtype unset__uniqtype__;


/*
 * liballocs - functions
*/

/*inline*/ void __liballocs_ensure_init(void);

/*inline*/ struct liballocs_err *__liballocs_get_alloc_info(
	const void *obj,
	struct allocator **out_allocator,
	const void **out_alloc_start,
	unsigned long *out_alloc_size_bytes,
	struct uniqtype **out_alloc_uniqtype,
	const void **out_alloc_site
);

struct insert *lookup_object_info(
	const void *mem,
	void **out_object_start,
	size_t *out_object_size,
	void **ignored
);

const char *(__attribute__((pure)) __liballocs_uniqtype_name)(
	const struct uniqtype *u
);

/*inline*/ _Bool __liballocs_find_matching_subobject(
	signed target_offset_within_uniqtype,
	struct uniqtype *cur_obj_uniqtype,
	struct uniqtype *test_uniqtype,
	struct uniqtype **last_attempted_uniqtype,
	signed *last_uniqtype_offset,
	signed *p_cumulative_offset_searched,
	struct uniqtype **p_cur_containing_uniqtype,
	struct uniqtype_rel_info **p_cur_contained_pos
);

extern /*inline*/ _Bool
/* __attribute__((always_inline,gnu_inline)) */
__liballocs_first_subobject_spanning(
	signed *p_target_offset_within_uniqtype,
	struct uniqtype **p_cur_obj_uniqtype,
	struct uniqtype **p_cur_containing_uniqtype,
	struct uniqtype_rel_info **p_cur_contained_pos
);

const char *format_symbolic_address(const void *addr);

/* added - sets the type in the uniqtype_index for a given alloc_site */
void __liballocs_notify_unset_type(
	const void *alloc_site,
	const void *test_uniqtype
);

extern /*inline*/
struct uniqtype * /*__attribute__((gnu_inline))*/
allocsite_to_uniqtype(const void *allocsite);

liballocs_err_t extract_and_output_alloc_site_and_type(
	struct insert *p_ins,
	struct uniqtype **out_type,
	void **out_site
);


/*
 * Not from liballocs
*/
struct insert *__liballocs_get_insert(const void *mem);

#endif /* LIBALLOCS_H */
