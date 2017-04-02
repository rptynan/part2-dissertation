#include <libcrunchk/libcrunch.h>


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
