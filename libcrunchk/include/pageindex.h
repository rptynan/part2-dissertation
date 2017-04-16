#ifndef PAGEINDEX_H
#define PAGEINDEX_H

#include <libcrunchk/include/allocmeta.h>
#include <libcrunchk/include/libcrunch.h>  // for debug prints

#define ADDR_BITSIZE 48

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
	unsigned alloc_site_flag:1;  // If true, alloc_site is the uniqtype
	/* unsigned long *alloc_site:ADDR_BITSIZE; */
	unsigned long alloc_site;
	union  __attribute__((packed))
	{
		struct ptrs ptrs;
		unsigned bits:16;
	} un;
} ;//__attribute__((packed));



/* We maintain two structures:
 *
 * - a list of "big allocations";
 * - an index mapping from page numbers to
 *      the deepest big allocation that completely spans that page.
 *   (this was formerly called the "level 0 index", and only mapped to
 *    first-level allocations a.k.a. memory mappings).
 * 
 * Note that a suballocated chunk may still be small enough that it
 * doesn't span any whole pages. It will still have a bigalloc number.
 * Indeed, one of the points of "big allocations" is to centralise the
 * complex business of allocation nesting. Since all nested allocations
 * are made out of a bigalloc, we can handle all that stuff here once
 * for every possible leaf allocator.
 */

/* Each big allocation has some metadata attached. The meaning of 
 * "insert" is down to the individual allocator. */
/* struct meta_info */
/* { */
/* 	enum meta_info_kind { DATA_PTR, INS_AND_BITS } what; */
/* 	union */
/* 	{ */
/* 		struct */
/* 		{ */
/* 			void *data_ptr; */
/* 			void (*free_func)(void*); */
/* 		} opaque_data; */
/* 		struct */ 
/* 		{ */
/* 			struct insert ins; */
			/* FIXME: document what these fields are for. I think it's when we 
			 * push malloc chunks' metadata down into the bigalloc metadata. */
			/*unsigned is_object_start:1;
			unsigned npages:20;
			unsigned obj_offset:7;*/
		/* } ins_and_bits; */
	/* } un; */
/* }; */

/* A "big allocation" is one that 
 * is suballocated from, or
 * spans at least BIG_ALLOC_THRESHOLD bytes of page-aligned memory. */
/* #define BIG_ALLOC_THRESHOLD (16*PAGE_SIZE) */

struct allocator;
struct big_allocation
{
	void *begin;
	void *end;
	/* struct big_allocation *parent; */
	/* struct big_allocation *next_sib; */
	/* struct big_allocation *prev_sib; */
	/* struct big_allocation *first_child; */
	struct allocator *allocated_by; // should always be parent->suballocator
	/* struct allocator *suballocator; // ... suballocated bigallocs may have only small children */
	/* struct meta_info meta; */
	/* void *suballocator_meta; */
	/* void (*suballocator_free_func)(void*); */
};
/* #define BIGALLOC_IN_USE(b) ((b)->begin && (b)->end) */
/* #define NBIGALLOCS 1024 */
/* extern struct big_allocation big_allocations[]; */

/* typedef uint16_t bigalloc_num_t; */

/* extern bigalloc_num_t *pageindex __attribute__((weak,visibility("protected"))); */


extern void pageindex_insert(
	void *begin,
	void *end,
	struct allocator *allocated_by
);

extern /*inline*/
struct allocator *(/*__attribute__((always_inline,gnu_inline))*/
__liballocs_leaf_allocator_for) (
	const void *obj,
	struct big_allocation **out_containing_bigalloc,
	struct big_allocation **out_maybe_the_allocation
);

void __liballocs_report_wild_address(const void *ptr);

_Bool __liballocs_notify_unindexed_address(const void *ptr);

struct big_allocation *__lookup_bigalloc(
	const void *mem,
	struct allocator *a,
	void **out_object_start
);

#endif /* PAGEINDEX_H */
