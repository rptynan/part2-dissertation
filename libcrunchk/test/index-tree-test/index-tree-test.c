#include "../../include/index_tree.h"
#include <assert.h>
#include <stdio.h>

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)


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

unsigned long my_distance(const void *a, const void *b) {
	int *aa = (int *) a;
	int *bb = (int *) b;
	return (unsigned long) (MAX(*aa, *bb) - MIN(*aa, *bb));
}

int inserted_index = 0;
int inserted_nums[10000];

void new_inserted(int x) {
	inserted_nums[inserted_index++] = x;
}

_Bool is_inserted(int x) {
	for (int i = 0; i < inserted_index; i++) {
		if (inserted_nums[inserted_index] == x) return 1;
	}
	return 0;
}

void printstuff(void *a) {
	struct my_data *aa = (struct my_data *) a;
	printf("traverse: %d\n", aa->x);
}


int main() {
	struct itree_node *root = NULL;

	struct my_data *p;
	// Insert distinct numbers in range [0, 1000)
	for (int i = 0; i < 100; i++) {
		p = malloc(sizeof(struct my_data), M_ITREE_DATA, NULL);
		do {
			p->x = rand() % 1000;
		} while (is_inserted(p->x));
		new_inserted(p->x);
		itree_insert(&root, p, my_compare);
	}
	// We'll look for these, so make sure they're in there
	if (!is_inserted(42)) {
		p = malloc(sizeof(struct my_data), M_ITREE_DATA, NULL);
		p->x = 42;
		itree_insert(&root, p, my_compare);
	}
	if (!is_inserted(1242)) {
		p = malloc(sizeof(struct my_data), M_ITREE_DATA, NULL);
		p->x = 1242;
		itree_insert(&root, p, my_compare);
	}
	if (!is_inserted(2442)) {
		p = malloc(sizeof(struct my_data), M_ITREE_DATA, NULL);
		p->x = 2442;
		itree_insert(&root, p, my_compare);
	}

	// Test find
	struct my_data q;
	q.x = 42;
	assert(((struct my_data *) itree_find(root, &q, my_compare)->data)->x == 42);
	q.x = 1242;
	assert(((struct my_data *) itree_find(root, &q, my_compare)->data)->x == 1242);
	q.x = 2442;
	assert(((struct my_data *) itree_find(root, &q, my_compare)->data)->x == 2442);

	// Test find closest under
	q.x = 43;
	assert(((struct my_data *) itree_find_closest_under(root, &q, my_compare, my_distance)->data)->x == 42);
	q.x = 42;
	assert(((struct my_data *) itree_find_closest_under(root, &q, my_compare, my_distance)->data)->x == 42);
	q.x = 1299;
	assert(((struct my_data *) itree_find_closest_under(root, &q, my_compare, my_distance)->data)->x == 1242);
	q.x = 9000;
	assert(((struct my_data *) itree_find_closest_under(root, &q, my_compare, my_distance)->data)->x == 2442);

	return 0;
}
