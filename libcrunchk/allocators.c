#include <libcrunchk/libcrunch.h>
#include <sys/malloc.h>


/*
 * malloc wrappers
*/

int __currently_freeing;  // TODO
int __currently_allocating;

/* The real malloc function, sig taken from malloc(9) man page */
void *__real_malloc(unsigned long size, struct malloc_type *type, int flags);

/* The hook */
void *__wrap_malloc(unsigned long size, struct malloc_type *type, int flags)
{
	PRINTD1("malloc called, size: %u", size);
	if (!type) PRINTD("malloc, no type!");
	else PRINTD1("malloc called, type: %s", type->ks_shortdesc);

	// Keep track of if we're first to set currently_allocating...
	_Bool set_currently_allocating = !__currently_allocating;
	__currently_allocating = 1;

	void *ret;
	ret = __real_malloc(size, type, flags);

	// ...and only unset it if that's the case
	if (set_currently_allocating) __currently_allocating = 0;
	return ret;
}


/*
 * allocator structs
*/

struct allocator __generic_malloc_allocator = {
	.name = "generic malloc",
	/* .get_info = NULL,  // __generic_heap_get_info, TODO ? */
	.is_cacheable = 1,
	/* .ensure_big = NULL,  // ensure_big, TODO ? */
};
