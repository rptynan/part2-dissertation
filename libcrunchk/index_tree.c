#include <sys/malloc.h>
#include <libcrunchk/include/index_tree.h>
#include <libcrunchk/include/libcrunch.h>

#ifdef _KERNEL
MALLOC_DECLARE(M_ITREE_NODE);
MALLOC_DEFINE(M_ITREE_NODE, "itree_node", "Node for libcrunchk's index tree");
#else
#define M_ITREE_NODE NULL
#endif


struct itree_node *itree_insert(struct itree_node *root, unsigned long value) {
	if (!root) {
		root = __real_malloc(
			sizeof(struct itree_node),
			M_ITREE_NODE,
			M_NOWAIT  // TODO revise this
		);
		root->value = value;
		root->left = NULL;
		root->right = NULL;
	}
	else if (value < root->value) {
		root->left = itree_insert(root->left, value);
	}
	else {
		root->right = itree_insert(root->right, value);
	}
	return root;
}


struct itree_node *itree_find(struct itree_node *root, unsigned long value) {
	if (!root) return NULL;
	if (root->value == value) return root;

	if (value < root->value) {
		return itree_find(root->left, value);
	}
	return itree_find(root->right, value);
}
