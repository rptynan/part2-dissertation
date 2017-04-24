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


void *itree_remove(
	struct itree_node **proot,
	void *to_remove,
	itree_compare_func compare
) {
	struct itree_node *r_node = itree_find(*proot, to_remove, compare);
	if (!r_node) return NULL;

	// Easy case, only one child (or none)
	if (!r_node->left || !r_node->right) {
		struct itree_node *subtree = (r_node->left)
			? r_node->left
			: r_node->right;
		if (r_node->parent) {
			// Link to replace in parent (if not root node)
			struct itree_node **parent_link;
			parent_link = (r_node->parent->left == r_node)
				? &(r_node->parent->left)
				: &(r_node->parent->right);
			*parent_link = subtree;
		}
		if (subtree) subtree->parent = r_node->parent;  // subtree could be NULL
	}
	// Difficult case, need to swap max from left subtree with r_node
	else {
		struct itree_node *replacement = itree_get_max(r_node->left, compare);
		if (replacement == r_node->left) {
			// Awkward case, left child is the max (and therefore has no right
			// child), so just splice it in
			if (r_node->parent) {
				// Link to replace in parent (if not root node)
				struct itree_node **parent_link;
				parent_link = (r_node->parent->left == r_node)
					? &(r_node->parent->left)
					: &(r_node->parent->right);
				*parent_link = replacement;
			}
			replacement->parent = r_node->parent;
			replacement->right = r_node->right;
			if (r_node->right) r_node->right->parent = replacement;
		}
		else {
			// The max is a leaf and the right child of its parent
			replacement->parent->right = NULL;
			if (r_node->parent) {
				// Link to replace in parent (if not root node)
				struct itree_node **parent_link;
				parent_link = (r_node->parent->left == r_node)
					? &(r_node->parent->left)
					: &(r_node->parent->right);
				*parent_link = replacement;
			}
			replacement->parent = r_node->parent;
			replacement->left = r_node->left;
			replacement->right = r_node->right;
			if (r_node->left) r_node->left->parent = replacement;
			if (r_node->right) r_node->right->parent = replacement;
		}
	}

	// Don't forget to free the removed node
	void *res = r_node->data;
	__real_free(r_node, M_TEMP);
	return res;
}


struct itree_node *itree_find(
	struct itree_node *root,
	const void *to_find,
	itree_compare_func compare
) {
	if (!root) return NULL;
	while (root) {
		if (compare(to_find, root->data) == 0) return root;
		else if (compare(to_find, root->data) < 0) {  // <
			root = root->left;
		}
		else {  // >
			root = root->right;
		}
	}
	return NULL;
}


extern struct itree_node *itree_find_closest_under(
	struct itree_node *root,
	const void *to_find,
	itree_compare_func compare,
	itree_distance_func distance
) {
	if (!root) return NULL;

	void *closest_node = root;
	unsigned long min_distance = distance(root->data, to_find);
	while (root) {
		if (compare(to_find, root->data) == 0) return root;
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
	return closest_node;
}


void itree_inorder_traverse(
	struct itree_node *root,
	itree_traverse_func func
) {
	if (!root) return;

	itree_inorder_traverse(root->left, func);
	func(root->data);
	itree_inorder_traverse(root->right, func);
}

