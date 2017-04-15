#include <libcrunchk/include/pageindex.h>

// TODO
#define BIG_LOCK
#define BIG_UNLOCK


extern inline
struct allocator *(/*__attribute__((always_inline,gnu_inline))*/
__liballocs_leaf_allocator_for) (
	const void *obj,
	struct big_allocation **out_containing_bigalloc,
	struct big_allocation **out_maybe_the_allocation
) {
	struct big_allocation *deepest = NULL;
	for (struct big_allocation *cur = __liballocs_get_bigalloc_containing(obj);
			__builtin_expect(cur != NULL, 1);
			)
	{
		deepest = cur;

		/* Increment: does one of the children overlap? */
		for (struct big_allocation *child = cur->first_child;
				__builtin_expect(child != NULL, 0);
				child = child->next_sib)
		{
			if ((char*) child->begin <= (char*) obj && 
					(char*) child->end > (char*) obj)
			{
				cur = child;
			}
		}
		
		if (cur == deepest) cur = NULL;
	}
	/* Now cur is null, and deepest is the deepest overlapping.
	 * If the deepest is not suballocated, then it's definitely
	 * the leaf. If it is suballocated, then *either* the suballocator
	 * *or* the bigalloc allocator might be responsible for the
	 * memory under ptr. We assume that it's the suballocator.
	 * 
	 * ... but that's wrong. All we know is that if a deeper
	 * allocation exists, it's not big, and it's exactly one level
	 * down (there's no nesting in non-big allocations). 
	 * FIMXE: we should really *try* the suballocator and then,
	 * if ptr actually falls between the cracks, return the 
	 * bigalloc's allocator. But that makes things slower than
	 * we want. So we should add a slower call for this. */
	
	if (__builtin_expect(!deepest, 0)) return NULL;
	else if (__builtin_expect(deepest->suballocator != NULL, 1))
	{
		/* The allocator is the suballocator, and the containing bigalloc
		 * is deepest. */
		if (out_containing_bigalloc) *out_containing_bigalloc = deepest;
		if (out_maybe_the_allocation) *out_maybe_the_allocation = NULL;
		return deepest->suballocator;
	}
	else
	{
		if (out_containing_bigalloc) *out_containing_bigalloc = deepest->parent;
		if (out_maybe_the_allocation) *out_maybe_the_allocation = deepest;
		return deepest->allocated_by;
	}
}


void __liballocs_report_wild_address(const void *ptr)
{
	PRINTD1("__liballocs_report_wild_address: %p", ptr);
	// TODO, pageindex.c
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


extern inline
struct big_allocation *(__attribute__((always_inline,gnu_inline))
__liballocs_get_bigalloc_containing) (const void *obj)
{
	return NULL;
	// TODO
	// if (__builtin_expect(obj == 0, 0)) return NULL;
	// if (__builtin_expect(obj == (void*) -1, 0)) return NULL;
	/* More heuristics go here. */
	/* bigalloc_num_t bigalloc_num = pageindex[PAGENUM(obj)]; */
	/* if (bigalloc_num == 0) return NULL; */
	/* struct big_allocation *b = &big_allocations[bigalloc_num]; */
	/* return b; */
}


struct big_allocation *__lookup_bigalloc(
	const void *mem,
	struct allocator *a,
	void **out_object_start
) {
	return NULL;
	// TODO
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


struct big_allocation *find_bigalloc(
	const void *addr,
	struct allocator *a
) {
	return NULL;
	// TODO
	/* bigalloc_num_t start_idx = pageindex[PAGENUM(addr)]; */
	/* if (start_idx == 0) return NULL; */
	/* return find_bigalloc_recursive(&big_allocations[start_idx], addr, a); */
}


struct big_allocation *find_bigalloc_recursive(
	struct big_allocation *start, 
	const void *addr,
	struct allocator *a
) {
	return NULL;
	// TODO
	/* /1* Is it this one? *1/ */
	/* if (start->allocated_by == a) return start; */
	
	/* /1* Okay, it's not this one. Is it one of the children? *1/ */
	/* for (struct big_allocation *child = start->first_child; */
	/* 		child; */
	/* 		child = child->next_sib) */
	/* { */
	/* 	if ((char*) child->begin <= (char*) addr && */ 
	/* 			child->end > addr) */
	/* 	{ */
	/* 		/1* okay, tail-recurse down here *1/ */
	/* 		return find_bigalloc_recursive(child, addr, a); */
	/* 	} */
	/* } */
	
	/* /1* We didn't find an overlapping child, so we fail. *1/ */
	/* return NULL; */
}

