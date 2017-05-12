#ifndef INDEX_TREE_H
#define INDEX_TREE_H

#include <sys/param.h>
#include <sys/malloc.h>

#ifdef _KERNEL
MALLOC_DECLARE(M_ITREE_NODE);
MALLOC_DECLARE(M_ITREE_DATA);
#else
#define M_ITREE_NODE NULL
#define M_ITREE_DATA NULL
#define M_TEMP NULL  // TODO investigate above types causing page faults
#endif

struct itree_node {
	void *data;
	struct itree_node *parent, *left, *right;
};

/* function which returns -1 if a < b, 1 if b > a and 0 if a == b */
typedef int (*itree_compare_func)(const void *a, const void *b);

/* function which returns unsigned integer of the distance between two
 * elements, only used when looking for closest node */
typedef unsigned long (*itree_distance_func)(const void *a, const void *b);

/* function to be applied to the data field in a traversal */
typedef void (*itree_traverse_func)(void *a, int depth);

/* Note: the data to_insert points to must have an appropriate allocation
 * lifetime, e.g. heap, only the pointer is copied by insert */
void itree_insert(
	struct itree_node **proot,
	void *to_insert,
	itree_compare_func compare
);

/* Note: caller is responsible for freeing to_remove, so this returns the data
 * pointer from the removed node.
 * Can return NULL if to_remove not present. */
void *itree_remove(
	struct itree_node **proot,
	void *to_remove,
	itree_compare_func compare
);

extern struct itree_node *itree_find(
	struct itree_node **proot,
	struct itree_node *root,
	const void *to_find,
	itree_compare_func compare
);

/* Finds the node which is equal according to compare, or if that's not
 * present, the node which is less than the one to find and closest to it (kind
 * of like the predecessor) */
extern struct itree_node *itree_find_closest_under(
	struct itree_node **proot,
	struct itree_node *root,
	const void *to_find,
	itree_compare_func compare,
	itree_distance_func distance
);

void itree_inorder_traverse(
	struct itree_node *root,
	itree_traverse_func func,
	int depth
);

#endif /* INDEX_TREE_H */
