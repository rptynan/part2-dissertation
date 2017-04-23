#include <stdio.h>


struct foo {
	int x;
	int y;
};

struct bar {
	int a;
	struct foo f;
};


int main() {

	/* struct foo *my_foo = malloc(sizeof(struct foo)); */

	printf("========================== Test 1 =============================\n");
	// Test 1 - normal
	/* struct bar *p = malloc(sizeof(struct bar), NULL, M_WAITOK); */
	printf("======== should pass ========\n");
	struct bar *p = (struct bar *) malloc(sizeof(struct bar));
	printf("======== should fail ========\n");
	struct foo *p1 = (struct foo *) p;

	p->f.x = 1;
	p->f.y = 2;
	void *t = &(p->f);
	/* struct foo *f = malloc(sizeof(struct foo)); */
	printf("========================== Test 2 =============================\n");
	printf("p: %p\n", p);
	printf("&(p->f): %p\n", &(p->f));
	// Test 2 - interior pointers
	printf("======== should pass ========\n");
	struct foo *p2 = (struct foo *) t;
	printf("======== should fail ========\n");
	struct bar *p3 = (struct bar *) t;

	printf("Done!\n");
	return 0;
}
