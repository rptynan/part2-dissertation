static void swap_ptrs(void **p1, void **p2)
{
	void *temp = *p1;
	*p1 = *p2;
	*p2 = temp;
}

int c = 42;
int d = 69105;
/* Here we do stuff that's genuinely invalid. Example: 1
 * use our swap function to swap two unsigned longs. */
unsigned long long blah = (unsigned long long) &c;
unsigned long long foo = (unsigned long long) &d;

int main(void)
{
	swap_ptrs(&blah, &foo);

	/* Any more? */

	return 0;
}
