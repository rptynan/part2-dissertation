#include "../../include/index_tree.h"
#include <assert.h>
#include <stdio.h>

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

#define TEST_RANGE 1000000
#define TEST_ITEMS TEST_RANGE / 4


struct my_data {
	int x;
};
struct itree_node *root = NULL;

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
_Bool inserted_nums[TEST_RANGE * 2] = {};

void new_inserted(int x) {
	inserted_nums[x] = 1;
}

_Bool is_inserted(int x) {
	return inserted_nums[x];
}

void remove_inserted(int x) {
	inserted_nums[x] = 0;
}

void printstuff(void *a, int depth) {
	struct my_data *aa = (struct my_data *) a;
	printf("%dtraverse: %d\n", depth, aa->x);
}



void test_find_closest_under() {
	for (int i = 0; i < TEST_RANGE; i++) {
		if (is_inserted(i)) {
			int j = 0;
			for (; !is_inserted(i + j + 1) && i + j < TEST_RANGE; j++);
			struct my_data q;
			q.x = i + j;
			assert(((struct my_data *) itree_find_closest_under(root, &q, my_compare, my_distance)->data)->x == i);
		}
	}
	printf("test_find_closest_under passed\n");
}

void test_find() {
	for (int i = 0; i < TEST_RANGE; i++) {
		if (is_inserted(i)) {
			struct my_data q;
			q.x = i;
			assert(((struct my_data *) itree_find(root, &q, my_compare)->data)->x == i);
		}
	}
	printf("test_find passed\n");
}


int main() {
	// Insert distinct numbers in range [0, 2000)
	struct my_data *p;
	for (int i = 0; i < TEST_ITEMS; i++) {
		p = malloc(sizeof(struct my_data), M_ITREE_DATA, NULL);
		do {
			p->x = rand() % TEST_RANGE;
		} while (is_inserted(p->x));
		new_inserted(p->x);
		itree_insert(&root, p, my_compare);
	}

	test_find();
	test_find_closest_under();

	// do some removals
	struct itree_node *found;
	for (int i = 0; i < TEST_RANGE; i++) {
		if (is_inserted(i)) {
			int doit = rand() % 2;
			if (doit) {
				struct my_data q;
				q.x = i;
				free(itree_remove(&root, &q, my_compare), M_ITREE_DATA);
				remove_inserted(i);
			}
		}
	}

	test_find();
	test_find_closest_under();

	return 0;
}
