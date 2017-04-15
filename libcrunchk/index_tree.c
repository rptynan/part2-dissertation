#include <libcrunchk/include/index_tree.h>
#include <libcrunchk/include/libcrunch.h>


struct itree_node *itree_insert(
	struct itree_node *root,
	void *to_insert,
	itree_compare_func compare
) {
	if (!root) {
		root = __real_malloc(
			sizeof(struct itree_node),
			M_ITREE_NODE,
			M_NOWAIT  // TODO revise this
		);
		root->data = to_insert;
		root->left = NULL;
		root->right = NULL;
	}
	else if (compare(to_insert, root->data) < 0) {  // <
		root->left = itree_insert(root->left, to_insert, compare);
	}
	else {  // >=
		root->right = itree_insert(root->right, to_insert, compare);
	}
	return root;
}


struct itree_node *itree_find(
	struct itree_node *root,
	void *to_find,
	itree_compare_func compare
) {
	if (!root) return NULL;
	if (compare(to_find, root->data) == 0) return root;

	if (compare(to_find, root->data) < 0) {  // <
		return itree_find(root->left, to_find, compare);
	}
	return itree_find(root->right, to_find, compare);
}


void *itree_inorder_traverse(
	struct itree_node *root,
	itree_traverse_func func
) {
	if (!root) return;

	itree_inorder_traverse(root->left, func);
	func(root->data);
	itree_inorder_traverse(root->right, func);
}

