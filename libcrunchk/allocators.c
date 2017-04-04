#include <libcrunchk/libcrunch.h>
#include <sys/malloc.h>


/*
 * allocator structs
*/

liballocs_err_t extract_and_output_alloc_site_and_type(
	struct insert *p_ins,
	struct uniqtype **out_type,
	void **out_site
) {
	PRINTD2(
		"extract_and_..: flag %u, site %p",
		p_ins->alloc_site_flag,
		p_ins->alloc_site
	);
	if (!p_ins) {
		++__liballocs_aborted_unindexed_heap;
		return &__liballocs_err_unindexed_heap_object;
	}
	/* void *alloc_site_addr = (void *) ((uintptr_t) p_ins->alloc_site); */

	/* Now we have a uniqtype or an allocsite. For long-lived objects
	 * the uniqtype will have been installed in the heap header already.
	 * This is the expected case.
	 */
	struct uniqtype *alloc_uniqtype;
	if (__builtin_expect(p_ins->alloc_site_flag, 1)) {
		if (out_site) *out_site = NULL;
		/* Clear the low-order bit, which is available as an extra flag
		 * bit. libcrunch uses this to track whether an object is "loose"
		 * or not. Loose objects have approximate type info that might be
		 * "refined" later, typically e.g. from __PTR_void to __PTR_T. */
		alloc_uniqtype = (struct uniqtype *)((uintptr_t)(p_ins->alloc_site) & ~0x1ul);
	}
	else {
		/* Look up the allocsite's uniqtype, and install it in the heap info
		 * (on NDEBUG builds only, because it reduces debuggability a bit). */
		uintptr_t alloc_site_addr = p_ins->alloc_site;
		void *alloc_site = (void*) alloc_site_addr;
		if (out_site) *out_site = alloc_site;
		alloc_uniqtype = allocsite_to_uniqtype(alloc_site/*, p_ins*/);

		/* Remember the unrecog'd alloc sites we see. */
		// TODO, this is caching?
		/* if (!alloc_uniqtype && alloc_site && */
		/* 	!__liballocs_addrlist_contains( */
		/* 		&__liballocs_unrecognised_heap_alloc_sites, */
		/* 		alloc_site */
		/* 	) */
		/* ) { */
		/* 	__liballocs_addrlist_add(&__liballocs_unrecognised_heap_alloc_sites, alloc_site); */
		/* } */
#ifdef NDEBUG
		// install it for future lookups
		// FIXME: make this atomic using a union
		// Is this in a loose state? NO. We always make it strict.
		// The client might override us by noticing that we return
		// it a dynamically-sized alloc with a uniqtype.
		// This means we're the first query to rewrite the alloc site,
		// and is the client's queue to go poking in the insert.
		if (alloc_uniqtype) {
			// Only doing this if not null because of inserting type from first
			// check in libcrunch
			p_ins->alloc_site_flag = 1;
			p_ins->alloc_site = (uintptr_t) alloc_uniqtype /* | 0x0ul */;
		}
#endif
	}

	// if we didn't get an alloc uniqtype, we abort
	if (!alloc_uniqtype)
	{
		++__liballocs_aborted_unrecognised_allocsite;

		/* We used to do this in clear_alloc_site_metadata in libcrunch...
		 * In cases where heap classification failed, we null out the allocsite
		 * to avoid repeated searching. We only do this for non-debug
		 * builds because it makes debugging a bit harder.
		 * NOTE that we don't want the insert to look like a deep-index
		 * terminator, so we set the flag.
		 */
		if (p_ins)
		{
	#ifdef NDEBUG
			/* TODO this breaks insert type of first check in is_a_internal */
			/* p_ins->alloc_site_flag = 1; */
			/* p_ins->alloc_site = 0; */
	#endif
			// Not sure if this makes sense in kernel
			/* assert(INSERT_DESCRIBES_OBJECT(p_ins)); */
			// The definition of below is that both of alloc_site and
			// alloc_site_flag must be null, which can't be true given the
			// above lines, right?
			/* assert(!INSERT_IS_NULL(p_ins)); */
		}
			
		return &__liballocs_err_unrecognised_alloc_site;;
	}
	// else output it
	if (out_type) *out_type = alloc_uniqtype;
	
	/* return success */
	return NULL;
}

struct tagged_insert {
	const void *addr;
	struct insert ins;  // FIXME storing the insert in the big array could lead
						// to an entry being evicted and replaced while a point
						// to its insert is passed around above..
	void *start;  // logically this is the same as addr?
	size_t size;
};
#define ADDR_ARRAY_SPACES (1 << 16)
#define ADDR_ARRAY_INDEX(addr) \
	((unsigned long) addr ) & (ADDR_ARRAY_SPACES - 1);
struct tagged_insert tagged_insert_array[ADDR_ARRAY_SPACES];
// TODO proper storage
// given memory address, get insert
struct insert *lookup_object_info(
		const void *mem,
		void **out_object_start,
		size_t *out_object_size
		/* void **ignored */
) {
	// TODO caching?
	if (!mem) return NULL;
	unsigned long i = ADDR_ARRAY_INDEX(mem);
	if (tagged_insert_array[i].addr == mem) {
		if (out_object_size) *out_object_size = tagged_insert_array[i].size;
		if (out_object_start) out_object_start = tagged_insert_array[i].start;
		return &tagged_insert_array[i].ins;  // can be null
	}
	return NULL;
}

#define EXTRA_INSERT_SPACE 0  // ?
liballocs_err_t __generic_heap_get_info(
	void * obj,
	/* struct big_allocation *maybe_bigalloc, */
	struct uniqtype **out_type,
	void **out_base,
	unsigned long *out_size,
	const void **out_site
) {
	++__liballocs_hit_heap_case; // FIXME: needn't be heap -- could be alloca
	/* For heap allocations, we look up the allocation site.
	 * (This also yields an offset within a toplevel object.)
	 * Then we translate the allocation site to a uniqtypes rec location.
	 * (For direct calls in eagerly-loaded code, we can cache this information
	 * within uniqtypes itself. How? Make uniqtypes include a hash table with
	 * initial contents mapping allocsites to uniqtype recs. This hash table
	 * is initialized during load, but can be extended as new allocsites
	 * are discovered, e.g. indirect ones.)
	 */
	struct insert *heap_info = NULL;
	
	size_t alloc_chunksize;
	heap_info = lookup_object_info(obj, *out_base, &alloc_chunksize/*, NULL*/);
	if (heap_info) {
		if (out_size) *out_size =
			alloc_chunksize - sizeof (struct insert) - EXTRA_INSERT_SPACE;
	}
	
	if (!heap_info)
	{
		++__liballocs_aborted_unindexed_heap;
		return &__liballocs_err_unindexed_heap_object;
	}
	
	return extract_and_output_alloc_site_and_type(
		heap_info, out_type, (void**) out_site
	);
}

struct allocator __generic_malloc_allocator = {
	.name = "generic malloc",
	.get_info = __generic_heap_get_info,
	.is_cacheable = 1,
	/* .ensure_big = NULL,  // ensure_big, TODO ? */
};


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
	if (ret) {
		void *caller = __builtin_return_address(1);
		// Insert for addr -> insert
		unsigned long i = ADDR_ARRAY_INDEX(ret);
		tagged_insert_array[i].addr = ret; // TODO ?? difference to start?
		tagged_insert_array[i].ins.alloc_site_flag = 0;
		tagged_insert_array[i].ins.alloc_site = (unsigned long) caller;
		tagged_insert_array[i].start = ret;
		tagged_insert_array[i].size = (size_t) size;
		// Insert for allocsite -> uniqtype
		i = ADDR_ARRAY_INDEX(caller);
		tagged_uniqtype_array[i].allocsite = caller;
		tagged_uniqtype_array[i].type = NULL;  // is_aU() first call sets this
	}
	// ...and only unset it if that's the case
	if (set_currently_allocating) __currently_allocating = 0;
	PRINTD1("malloc returning: %p", ret);
	return ret;
}
