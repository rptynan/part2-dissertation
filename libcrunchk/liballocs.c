#include <libcrunchk/libcrunch.h>


/* special uniqtypes */
struct uniqtype *pointer_to___uniqtype__void;
struct uniqtype *pointer_to___uniqtype__signed_char;
struct uniqtype *pointer_to___uniqtype__unsigned_char;
struct uniqtype *pointer_to___uniqtype____PTR_signed_char;
struct uniqtype *pointer_to___uniqtype____PTR___PTR_signed_char;
struct uniqtype *pointer_to___uniqtype__Elf64_auxv_t;
struct uniqtype *pointer_to___uniqtype____ARR0_signed_char;
struct uniqtype *pointer_to___uniqtype__intptr_t;


/* counters TODO sysctl? */
unsigned long __liballocs_aborted_stack = 0;
unsigned long __liballocs_aborted_static = 0;
unsigned long __liballocs_aborted_unknown_storage = 0;
unsigned long __liballocs_hit_heap_case = 0;
unsigned long __liballocs_hit_stack_case = 0;
unsigned long __liballocs_hit_static_case = 0;
unsigned long __liballocs_aborted_unindexed_heap = 0;
unsigned long __liballocs_aborted_unrecognised_allocsite = 0;

/* built-in errors */
struct liballocs_err __liballocs_err_not_impl
 = {"Not implemented"};
struct liballocs_err __liballocs_err_abort
 = {"Aborted"};
struct liballocs_err __liballocs_err_stack_walk_step_failure
 = { "stack walk reached higher frame" };
struct liballocs_err __liballocs_err_stack_walk_reached_higher_frame 
 = { "stack walk reached higher frame" };
struct liballocs_err __liballocs_err_stack_walk_reached_top_of_stack 
 = { "stack walk reached top-of-stack" };
struct liballocs_err __liballocs_err_unknown_stack_walk_problem 
 = { "unknown stack walk problem" };
struct liballocs_err __liballocs_err_unindexed_heap_object
 = { "unindexed heap object" };
struct liballocs_err __liballocs_err_unrecognised_alloc_site
 = { "unrecognised alloc site" };
struct liballocs_err __liballocs_err_unrecognised_static_object
 = { "unrecognised static object" };
struct liballocs_err __liballocs_err_object_of_unknown_storage
 = { "object of unknown storage" };


extern inline void __liballocs_ensure_init(void)
{
	PRINTD("__liballocs_ensure_init");
	// TODO liballocs.h
	return;
}

extern inline struct liballocs_err *__liballocs_get_alloc_info(
	const void *obj,
	struct allocator **out_allocator,
	const void **out_alloc_start,
	unsigned long *out_alloc_size_bytes,
	struct uniqtype **out_alloc_uniqtype,
	const void **out_alloc_site
) {
	PRINTD1("__liballocs_get_alloc_info: %p", obj);
	/* struct liballocs_err *err = 0; */

	struct allocator *a = __liballocs_leaf_allocator_for(
		obj
	);
	if (__builtin_expect(!a, 0))
	{
		_Bool fixed = __liballocs_notify_unindexed_address(obj);
		if (fixed)
		{
			a = __liballocs_leaf_allocator_for(
				obj
			);
			return &__liballocs_err_abort;
		}
		else
		{
			__liballocs_report_wild_address(obj);
			++__liballocs_aborted_unknown_storage;
			return &__liballocs_err_object_of_unknown_storage;
		}
	}
	if (out_allocator) *out_allocator = a;
	return a->get_info(
		(void*) obj,
		/* maybe_the_allocation, */
		out_alloc_uniqtype,
		(void**) out_alloc_start,
		out_alloc_size_bytes,
		out_alloc_site
	);
}

/* A client-friendly lookup function that knows about bigallocs.
 * FIXME: this needs to go away! Clients shouldn't have to know about inserts,
 * and not all allocators maintain them. */
struct insert *__liballocs_get_insert(const void *mem)
{
	PRINTD1("__liballocs_get_insert: %p", mem);
	// TODO
	return lookup_object_info(mem, NULL, NULL/*, NULL*/);
}

extern inline _Bool __liballocs_find_matching_subobject(
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
	if (target_offset_within_uniqtype == 0 
		&& (!test_uniqtype || cur_obj_uniqtype == test_uniqtype))
	{
		if (p_cur_containing_uniqtype) *p_cur_containing_uniqtype = NULL;
		if (p_cur_contained_pos) *p_cur_contained_pos = NULL;
		return 1;
	}
	else
	{
		/* We might have *multiple* subobjects spanning the offset.
		 * Test all of them. */
		struct uniqtype *containing_uniqtype = NULL;
		struct uniqtype_rel_info *contained_pos = NULL;
		
		signed sub_target_offset = target_offset_within_uniqtype;
		struct uniqtype *contained_uniqtype = cur_obj_uniqtype;
		
		_Bool success = __liballocs_first_subobject_spanning(
			&sub_target_offset, &contained_uniqtype,
			&containing_uniqtype, &contained_pos);
		// now we have a *new* sub_target_offset and contained_uniqtype
		
		if (!success) return 0;
		
		if (p_cumulative_offset_searched) *p_cumulative_offset_searched += contained_pos->un.memb.off;
		
		if (last_attempted_uniqtype) *last_attempted_uniqtype = contained_uniqtype;
		if (last_uniqtype_offset) *last_uniqtype_offset = sub_target_offset;
		do {
			assert(containing_uniqtype == cur_obj_uniqtype);
			_Bool recursive_test = __liballocs_find_matching_subobject(
					sub_target_offset,
					contained_uniqtype, test_uniqtype,
					last_attempted_uniqtype, last_uniqtype_offset, p_cumulative_offset_searched,
					p_cur_containing_uniqtype,
					p_cur_contained_pos);
			if (__builtin_expect(recursive_test, 1))
			{
				if (p_cur_containing_uniqtype) *p_cur_containing_uniqtype = containing_uniqtype;
				if (p_cur_contained_pos) *p_cur_contained_pos = contained_pos;

				return 1;
			}
			// else look for a later contained subobject at the same offset
			signed subobj_ind = contained_pos - &containing_uniqtype->related[0];
			assert(subobj_ind >= 0);
			assert(subobj_ind == 0 || subobj_ind < UNIQTYPE_COMPOSITE_MEMBER_COUNT(containing_uniqtype));
			if (__builtin_expect(
					UNIQTYPE_COMPOSITE_MEMBER_COUNT(containing_uniqtype) <= subobj_ind + 1
					|| containing_uniqtype->related[subobj_ind + 1].un.memb.off != 
						containing_uniqtype->related[subobj_ind].un.memb.off,
				1))
			{
				// no more subobjects at the same offset, so fail
				return 0;
			} 
			else
			{
				contained_pos = &containing_uniqtype->related[subobj_ind + 1];
				contained_uniqtype = contained_pos->un.memb.ptr;
			}
		} while (1);
		
		assert(0);
	}
	return 0;
}

/* stolen from libkern, not sure if necessary to avoid outside calls? */
static int strncmp(const char *s1, const char *s2, size_t n)
{
	if (n == 0)
		return (0);
	do {
		if (*s1 != *s2++)
			return (*(const unsigned char *)s1 -
					*(const unsigned char *)(s2 - 1));
		if (*s1++ == '\0')
			break;
	} while (--n != 0);
	return (0);
}

const char *(__attribute__((pure)) __liballocs_uniqtype_name)(
	const struct uniqtype *u
) {
	return "(no type)";
	if (!u) return "(no type)";
	const char *symbol_name = __liballocs_uniqtype_symbol_name(u);
	if (symbol_name)
	{
		if (0 == strncmp(symbol_name, "__uniqtype__", sizeof "__uniqtype__" - 1))
		{
			/* Codeless. */
			return symbol_name + sizeof "__uniqtype__" - 1;
		}
		else if (0 == strncmp(symbol_name, "__uniqtype_", sizeof "__uniqtype_" - 1))
		{
			/* With code. */
			return symbol_name + sizeof "__uniqtype_" - 1 + /* code + underscore */ 9;
		}
		return symbol_name;
	}
	return "(unnamed type)";
}

inline _Bool
/* __attribute__((always_inline,gnu_inline)) */
__liballocs_first_subobject_spanning(
	signed *p_target_offset_within_uniqtype,
	struct uniqtype **p_cur_obj_uniqtype,
	struct uniqtype **p_cur_containing_uniqtype,
	struct uniqtype_rel_info **p_cur_contained_pos
) {
	struct uniqtype *cur_obj_uniqtype = *p_cur_obj_uniqtype;
	signed target_offset_within_uniqtype = *p_target_offset_within_uniqtype;
	/* Calculate the offset to descend to, if any. This is different for 
	 * structs versus arrays. */
	if (UNIQTYPE_IS_ARRAY_TYPE(cur_obj_uniqtype))
	{
		signed num_contained = UNIQTYPE_ARRAY_LENGTH(cur_obj_uniqtype);
		struct uniqtype *element_uniqtype = UNIQTYPE_ARRAY_ELEMENT_TYPE(cur_obj_uniqtype);
		if (element_uniqtype->pos_maxoff != 0 && 
				num_contained > target_offset_within_uniqtype / element_uniqtype->pos_maxoff)
		{
			*p_cur_containing_uniqtype = cur_obj_uniqtype;
			*p_cur_contained_pos = &cur_obj_uniqtype->related[0];
			*p_cur_obj_uniqtype = element_uniqtype;
			*p_target_offset_within_uniqtype = target_offset_within_uniqtype % element_uniqtype->pos_maxoff;
			return 1;
		} else return 0;
	}
	else // struct/union case
	{
		signed num_contained = UNIQTYPE_COMPOSITE_MEMBER_COUNT(cur_obj_uniqtype);

		int lower_ind = 0;
		int upper_ind = num_contained;
		while (lower_ind + 1 < upper_ind) // difference of >= 2
		{
			/* Bisect the interval */
			int bisect_ind = (upper_ind + lower_ind) / 2;
			__liballocs_private_assert(bisect_ind > lower_ind, "bisection progress",
				__FILE__, __LINE__, __func__);
			if (cur_obj_uniqtype->related[bisect_ind].un.memb.off > target_offset_within_uniqtype)
			{
				/* Our solution lies in the lower half of the interval */
				upper_ind = bisect_ind;
			} else lower_ind = bisect_ind;
		}

		if (lower_ind + 1 == upper_ind)
		{
			/* We found one offset; we may still have overshot, in the case of a 
			 * stack frame where offset zero might not be used. */
			if (cur_obj_uniqtype->related[lower_ind].un.memb.off > target_offset_within_uniqtype)
			{
				return 0;
			}
			/* We found one offset */
			__liballocs_private_assert(cur_obj_uniqtype->related[lower_ind].un.memb.off <= target_offset_within_uniqtype,
				"offset underapproximates", __FILE__, __LINE__, __func__);

			/* ... but we might not have found the *lowest* index, in the 
			 * case of a union. Scan backwards so that we have the lowest. 
			 * FIXME: need to account for the element size? Or here are we
			 * ignoring padding anyway? */
			while (lower_ind > 0 
				&& cur_obj_uniqtype->related[lower_ind-1].un.memb.off
					 == cur_obj_uniqtype->related[lower_ind].un.memb.off)
			{
				--lower_ind;
			}
			*p_cur_contained_pos = &cur_obj_uniqtype->related[lower_ind];
			*p_cur_containing_uniqtype = cur_obj_uniqtype;
			*p_cur_obj_uniqtype
			 = cur_obj_uniqtype->related[lower_ind].un.memb.ptr;
			/* p_cur_obj_uniqtype now points to the subobject's uniqtype. 
			 * We still have to adjust the offset. */
			*p_target_offset_within_uniqtype
			 = target_offset_within_uniqtype - cur_obj_uniqtype->related[lower_ind].un.memb.off;

			return 1;
		}
		else /* lower_ind >= upper_ind */
		{
			// this should mean num_contained == 0
			__liballocs_private_assert(num_contained == 0,
				"no contained objects", __FILE__, __LINE__, __func__);
			return 0;
		}
	}
 return 1;
}

const char *format_symbolic_address(const void *addr)
{
	return "format_symbolic_address not implemented";
}

inline
struct allocator *(__attribute__((always_inline,gnu_inline))
__liballocs_leaf_allocator_for) (
	const void *obj
	/* struct big_allocation **out_containing_bigalloc, */
	/* struct big_allocation **out_maybe_the_allocation */
) {
	// TODO, add the two other kernel allocators, zone and kmap
	// TODO, add the stack allocator
	return &__generic_malloc_allocator;
}

void __liballocs_report_wild_address(const void *ptr)
{
	PRINTD1("__liballocs_report_wild_address: %p", ptr);
	// TODO, pageindex.c
	return;
}

struct tagged_uniqtype tagged_uniqtype_array[ALLOCSITE_ARRAY_SPACES];
inline
struct uniqtype * __attribute__((gnu_inline))
allocsite_to_uniqtype(const void *allocsite)
{
	PRINTD1("allocsite_to_uniqtype: %p", allocsite);
	if (!allocsite) return NULL;
	unsigned long i = ALLOCSITE_ARRAY_INDEX(allocsite);
	if (tagged_uniqtype_array[i].allocsite == allocsite)
		return tagged_uniqtype_array[i].type;  // can be null
	return NULL;
}

_Bool __liballocs_notify_unindexed_address(const void *ptr)
{
	/* We get called if the caller finds an address that's not indexed
	 * anywhere.  It's a way of asking us to check.
	 * We ask all our allocators in turn whether they own this address.
	 * Only stack is expected to reply positively. */
	// Well not true for my version..
	/* _Bool ret = __stack_allocator_notify_unindexed_address(ptr); */
	/* if (ret) return 1; */
	/* // FIXME: loop through the others */
	// TODO, not sure if this will play larger role than just stack
	return 0;
}
