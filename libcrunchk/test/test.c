#include <stdio.h>
#include <stdlib.h>

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
	struct bar *p = malloc(sizeof(struct bar));
	struct foo *p1 = (struct foo *) p;

	printf("Done!\n");
	return 0;
}
