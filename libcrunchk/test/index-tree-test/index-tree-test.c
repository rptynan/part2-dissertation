#include "../../include/index_tree.h"
/* #include <stdlib.h> */
#include <assert.h>
#include <stdio.h>


int main() {
	struct itree_node *root = NULL;

	for (int i = 0; i < 1000; i++) {
		// Insert numbers in range [200, 1200)
		root = itree_insert(root, rand() % 1000 + 200);
		// Insert numbers in range [1300, 2300)
		root = itree_insert(root, rand() % 1000 + 1300);
	}
	// We'll look for these (so they're definitely only in there once)
	root = itree_insert(root, 42);
	root = itree_insert(root, 1242);
	root = itree_insert(root, 2442);

	// Test find
	assert(itree_find(root, 42)->value == 42);
	assert(itree_find(root, 1242)->value == 1242);
	assert(itree_find(root, 2442)->value == 2442);

	return 0;
}
