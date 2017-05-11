static void swap_ptrs(void **p1, void **p2)
{
	void *temp = *p1;
	*p1 = *p2;
	*p2 = temp;
}

	int c = 42;
	int d = 69105;
	int *blah = &c;
	int *foo = &d;
int main(void)
{
	swap_ptrs(&blah, &foo);

	return 0;
}
