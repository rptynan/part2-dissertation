#include <sys/malloc.h>

#ifdef _KERNEL
MALLOC_DECLARE(M_ITREE_NODE);
MALLOC_DEFINE(M_ITREE_NODE, "itree_node", "Node for libcrunchk's index tree");
MALLOC_DECLARE(M_ITREE_DATA);
MALLOC_DEFINE(
	M_ITREE_DATA,
	"itree_data",
	"Data for a node in libcrunhk's index tree"
);
#else
#define M_ITREE_NODE NULL
#define M_ITREE_DATA NULL
#endif

struct itree_node {
	void *data;
	struct itree_node *left, *right;
};

/* function which returns -1 if a < b, 1 if b > a and 0 if a == b */
typedef int (*itree_compare_func)(void *a, void *b);

/* function to be applied to the data field in a traversal */
typedef void (*itree_traverse_func)(void *a);

/* Note: the data to_insert points to must have an appropriate allocation
 * lifetime, e.g. heap, only the pointer is copied by insert */
extern struct itree_node *itree_insert(
	struct itree_node *root,
	void *to_insert,
	itree_compare_func compare
);

extern struct itree_node *itree_find(
	struct itree_node *root,
	void *data,
	itree_compare_func compare
);

void itree_inorder_traverse(
	struct itree_node *root,
	itree_traverse_func func
);
