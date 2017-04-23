#include <libcrunchk/include/pageindex.h>
#include <libcrunchk/include/index_tree.h>

// TODO
#define BIG_LOCK
#define BIG_UNLOCK

/* index_tree wrapper functions */
struct itree_node *pageindex_root = NULL;

int pageindex_compare(const void *a, const void *b) {
	struct big_allocation *aa = (struct big_allocation *) a;
	struct big_allocation *bb = (struct big_allocation *) b;
	if (aa->begin < bb->begin) return -1;
	if (aa->begin > bb->begin) return +1;
	return 0;
}

static inline struct big_allocation* pageindex_lookup(const void *begin) {
	PRINTD1("pageindex_lookup: %p", begin);
	const struct big_allocation b = {.begin = (void *)begin};
	struct itree_node *res = itree_find(pageindex_root, &b, pageindex_compare);
	if (res) return (struct big_allocation *) res->data;
	return NULL;
}

void pageindex_insert(
	void *begin,
	void *end,
	struct allocator *allocated_by
) {
	PRINTD1("pageindex_insert: %p", begin);
	struct big_allocation *b =
		__real_malloc(sizeof(struct big_allocation), M_TEMP, M_WAITOK);
	b->begin = begin;
	b->end = end;
	b->allocated_by = allocated_by;
	itree_insert(&pageindex_root, (void *)b, pageindex_compare);
}


extern inline
struct allocator *(/*__attribute__((always_inline,gnu_inline))*/
__liballocs_leaf_allocator_for) (
	const void *obj,
	struct big_allocation **out_containing_bigalloc,  // unused?
	struct big_allocation **out_maybe_the_allocation
) {
	struct big_allocation *res = pageindex_lookup(obj);
	if (out_maybe_the_allocation) *out_maybe_the_allocation = res;
	if (res) return res->allocated_by;
	return NULL;
}


void __liballocs_report_wild_address(const void *ptr)
{
	PRINTD1("__liballocs_report_wild_address: %p", ptr);
	// TODO, this is just reporting a pointer we can't track
	return;
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

struct big_allocation *__lookup_bigalloc(
	const void *mem,
	struct allocator *a,
	void **out_object_start
) {
	// TODO is this necessary at all?
	return NULL;
	/* if (!pageindex) init(); */
	/* int lock_ret; */
	/* BIG_LOCK */
	
	/* struct big_allocation *b = find_bigalloc(mem, a); */
	/* if (b) */
	/* { */
	/* 	BIG_UNLOCK */
	/* 	return b; */
	/* } */
	/* else */
	/* { */
	/* 	BIG_UNLOCK */
	/* 	return NULL; */
	/* } */
}
