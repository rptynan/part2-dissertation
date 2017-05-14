#include <libcrunchk/include/index_tree.h>
#include <libcrunchk/include/libcrunch.h>

#ifdef _KERNEL
MALLOC_DEFINE(M_ITREE_NODE, "itree_node", "Node for libcrunchk's index tree");
MALLOC_DEFINE(
	M_ITREE_DATA,
	"itree_data",
	"Data for a node in libcrunhk's index tree"
);
#endif

static inline void itree_splay(
	struct itree_node **proot,
	struct itree_node *x
);


void itree_insert(
	struct itree_node **rootp,
	void *to_insert,
	itree_compare_func compare
) {
	struct itree_node *parent = NULL;
	struct itree_node **parent_link = NULL;
	struct itree_node *root = *rootp;
	while (root) {
		if (compare(to_insert, root->data) < 0) {  // <
			parent = root;
			parent_link = &root->left;
			root = root->left;
		}
		else {  // >=
			parent = root;
			parent_link = &root->right;
			root = root->right;
		}
	}

	root = __real_malloc(
		sizeof(struct itree_node),
		M_TEMP,
		M_WAITOK  // We should only be called when the containing malloc is
				  // wait ok anyway
	);
	if (root) {
		root->data = to_insert;
		root->parent = parent;
		root->left = NULL;
		root->right = NULL;
		if (parent_link) *parent_link = root;
		if (!*rootp) *rootp = root;
		itree_splay(rootp, root);
	}
	else {
		PRINTD("Failed to allocate itree_node!");
	}
}


/* internal */
struct itree_node *itree_get_max(
	struct itree_node *root,
	itree_compare_func compare
) {
	if (!root) return NULL;
	while (root->right) {
		root = root->right;
	}
	return root;
}

struct itree_node *itree_get_min(
	struct itree_node *root,
	itree_compare_func compare
) {
	if (!root) return NULL;
	while (root->left) {
		root = root->left;
	}
	return root;
}

#define replace_parent_link(r_node, replacement) \
if (r_node->parent) {														\
	/* Link to replace in parent (if not root node) */						\
	struct itree_node **parent_link;										\
	parent_link = (r_node->parent->left == r_node)							\
		? &(r_node->parent->left)											\
		: &(r_node->parent->right);											\
	*parent_link = replacement;												\
}																			\
else {																		\
	*proot = replacement;													\
}
void *itree_remove(
	struct itree_node **proot,
	void *to_remove,
	itree_compare_func compare
) {
	struct itree_node *r_node = itree_find(proot, *proot, to_remove, compare);
	if (!r_node) return NULL;
	itree_splay(proot, r_node);

	// Easy case, only one child (or none)
	if (!r_node->left || !r_node->right) {
		struct itree_node *replacement = (r_node->left)
			? r_node->left
			: r_node->right;
		replace_parent_link(r_node, replacement);
		if (replacement) replacement->parent = r_node->parent;  // replacement could be NULL
	}
	// Difficult case, need to swap max from left subtree with r_node
	else {
		struct itree_node *replacement = itree_get_max(r_node->left, compare);
		if (replacement == r_node->left) {
			// Awkward case, left child is the max (and therefore has no right
			// child), so just splice it in
			replace_parent_link(r_node, replacement);
			replacement->parent = r_node->parent;
			replacement->right = r_node->right;
			if (r_node->right) r_node->right->parent = replacement;
		}
		else {
			// The max is a leaf and the right child of its parent and we stich
			// its left child to its parent
			replacement->parent->right = replacement->left;
			if (replacement->left) replacement->left->parent = replacement->parent;
			// link with parent
			replace_parent_link(r_node, replacement);
			replacement->parent = r_node->parent;
			// set replacement's childrens to r_node's
			replacement->left = r_node->left;
			replacement->right = r_node->right;
			// set r_node's children to have parent replacement
			if (r_node->left) r_node->left->parent = replacement;
			if (r_node->right) r_node->right->parent = replacement;
		}
	}

	// Don't forget to free the removed node and pass data pointer back so it
	// can be freed by caller
	void *res = r_node->data;
	__real_free(r_node, M_TEMP);
	return res;
}


struct itree_node *itree_find(
	struct itree_node **proot,
	struct itree_node *root,
	const void *to_find,
	itree_compare_func compare
) {
	if (!root) return NULL;
	while (root) {
		if (compare(to_find, root->data) == 0) {
			itree_splay(proot, root);
			return root;
		}
		else if (compare(to_find, root->data) < 0) {  // <
			if (!root->left) break;
			root = root->left;
		}
		else {  // >
			if (!root->right) break;
			root = root->right;
		}
	}
	if (root) itree_splay(proot, root);
	return NULL;
}


extern struct itree_node *itree_find_closest_under(
	struct itree_node **proot,
	struct itree_node *root,
	const void *to_find,
	itree_compare_func compare,
	itree_distance_func distance
) {
	if (!root) return NULL;

	// TODO we need to find a node to set out closest node to at the beginning
	// root doesn't work because it might be above but very close
	// so hack for now is to set to the minimum
	struct itree_node *closest_node = itree_get_min(root, compare);
	unsigned long min_distance = distance(closest_node->data, to_find);
	while (root) {
		if (compare(to_find, root->data) == 0) {
			itree_splay(proot, root);
			return root;
		}
		else if (compare(to_find, root->data) < 0) {  // <
			// to_find is less than, so try find something smaller
			root = root->left;
		}
		else {  // >
			// to_find is greater than, so check if closest so far
			unsigned long cur_distance = distance(root->data, to_find);
			if (cur_distance < min_distance) {
				closest_node = root;
				min_distance = cur_distance;
			}
			root = root->right;
		}
	}
	if (closest_node) itree_splay(proot, closest_node);
	return closest_node;
}


void itree_inorder_traverse(
	struct itree_node *root,
	itree_traverse_func func,
	int depth
) {
	if (!root) return;

	itree_inorder_traverse(root->left, func, depth + 1);
	func(root->data, depth);
	itree_inorder_traverse(root->right, func, depth + 1);
}

/* Below is from https://en.wikipedia.org/wiki/Splay_tree */

static inline void itree_left_rotate(
	struct itree_node **proot,
	struct itree_node *x
) {
	struct itree_node *y = x->right;
	if(y) {
		x->right = y->left;
		if( y->left ) y->left->parent = x;
		y->parent = x->parent;
	}

	if( !x->parent ) *proot = y;
	else if( x == x->parent->left ) x->parent->left = y;
	else x->parent->right = y;
	if(y) y->left = x;
	x->parent = y;
}

static inline void itree_right_rotate(
	struct itree_node **proot,
	struct itree_node *x
) {
	struct itree_node *y = x->left;
	if(y) {
		x->left = y->right;
		if( y->right ) y->right->parent = x;
		y->parent = x->parent;
	}
	if( !x->parent ) *proot = y;
	else if( x == x->parent->left ) x->parent->left = y;
	else x->parent->right = y;
	if(y) y->right = x;
	x->parent = y;
}

static inline void itree_splay(
	struct itree_node **proot,
	struct itree_node *x
) {
	if (!__libcrunch_splaying_on) return;
	while( x->parent ) {
		if( !x->parent->parent ) {
			if( x->parent->left == x ) itree_right_rotate(proot, x->parent);
			else itree_left_rotate(proot, x->parent);
		} else if( x->parent->left == x && x->parent->parent->left == x->parent ) {
			itree_right_rotate(proot, x->parent->parent);
			itree_right_rotate(proot, x->parent);
		} else if( x->parent->right == x && x->parent->parent->right == x->parent ) {
			itree_left_rotate(proot, x->parent->parent);
			itree_left_rotate(proot, x->parent);
		} else if( x->parent->left == x && x->parent->parent->right == x->parent ) {
			itree_right_rotate(proot, x->parent);
			itree_left_rotate(proot, x->parent);
		} else {
			itree_left_rotate(proot, x->parent);
			itree_right_rotate(proot, x->parent);
		}
	}
}
