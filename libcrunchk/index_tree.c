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

