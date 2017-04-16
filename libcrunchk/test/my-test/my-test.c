#include <stdio.h>


struct bar {
	char a;
};

struct foo {
	int x;
	int y;
	struct bar b;
};


int main() {

	// Test 1
	// Should pass first is_a()
	struct bar *p = malloc(sizeof(struct bar));
	// And then fail the second
	struct foo *p1 = (struct foo *) p;

	printf("Done!\n");
	return 0;
}
