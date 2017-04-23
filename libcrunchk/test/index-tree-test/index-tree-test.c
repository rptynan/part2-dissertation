#include "../../include/index_tree.h"
#include <assert.h>
#include <stdio.h>


struct my_data {
	int x;
};

int my_compare(const void *a, const void *b) {
	struct my_data *aa = (struct my_data *) a;
	struct my_data *bb = (struct my_data *) b;
	if (aa->x < bb->x) return -1;
	if (aa->x > bb->x) return 1;
	return 0;
}

void printstuff(void *a) {
	struct my_data *aa = (struct my_data *) a;
	printf("traverse: %d\n", aa->x);
}


int main() {
	struct itree_node *root = NULL;

	struct my_data *p;
	for (int i = 0; i < 100; i++) {
		// Insert numbers in range [200, 1200)
		p = malloc(sizeof(struct my_data), M_ITREE_DATA, NULL);
		p->x = rand() % 1000 + 200;
		itree_insert(&root, p, my_compare);
		// Insert numbers in range [1300, 2300)
		p = malloc(sizeof(struct my_data), M_ITREE_DATA, NULL);
		p->x = rand() % 1000 + 1300;
		itree_insert(&root, p, my_compare);
	}
	// We'll look for these (so they're definitely only in there once)
	p = malloc(sizeof(struct my_data), M_ITREE_DATA, NULL);
	p->x = 42;
	itree_insert(&root, p, my_compare);
	p = malloc(sizeof(struct my_data), M_ITREE_DATA, NULL);
	p->x = 1242;
	itree_insert(&root, p, my_compare);
	p = malloc(sizeof(struct my_data), M_ITREE_DATA, NULL);
	p->x = 2442;
	itree_insert(&root, p, my_compare);

	// Test find
	struct my_data q;
	q.x = 42;
	assert(((struct my_data *) itree_find(root, &q, my_compare)->data)->x == 42);
	q.x = 1242;
	assert(((struct my_data *) itree_find(root, &q, my_compare)->data)->x == 1242);
	q.x = 2442;
	assert(((struct my_data *) itree_find(root, &q, my_compare)->data)->x == 2442);

	return 0;
}
