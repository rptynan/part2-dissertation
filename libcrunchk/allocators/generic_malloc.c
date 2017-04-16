#include <libcrunchk/include/pageindex.h>
#include <libcrunchk/include/liballocs.h>
#include <sys/malloc.h>

#ifndef EXTRA_INSERT_SPACE
#define EXTRA_INSERT_SPACE 0
#endif

/* A client-friendly lookup function that knows about bigallocs.
 * FIXME: this needs to go away! Clients shouldn't have to know about inserts,
 * and not all allocators maintain them. */
struct insert *__liballocs_get_insert(const void *mem)
{
	struct big_allocation *b = __lookup_bigalloc(mem,
		&__generic_malloc_allocator, NULL);
	if (b)
	{
		// TODO see if this is used
		/* assert(b->meta.what == INS_AND_BITS); */
		/* return &b->meta.un.ins_and_bits.ins; */
	}
	else return lookup_object_info(mem, NULL, NULL, NULL);
}

liballocs_err_t __generic_heap_get_info(
	void * obj,
	struct big_allocation *maybe_bigalloc,
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
	
	/* NOTE: bigallocs already have the size adjusted by the insert. */
	if (maybe_bigalloc)
	{
		/* We already have the metadata. */
		// TODO replacing this with lookup for now, is meta updated anywhere?
		// If so, use here.
		/* heap_info = &maybe_bigalloc->meta.un.ins_and_bits.ins; */
		heap_info = lookup_object_info(obj, NULL, NULL, NULL);
		if (out_base) *out_base = maybe_bigalloc->begin;
		if (out_size) *out_size = (char*) maybe_bigalloc->end - (char*) maybe_bigalloc->begin;
	} 
	else
	{
		size_t alloc_chunksize;
		heap_info = lookup_object_info(obj, out_base, &alloc_chunksize, NULL);
		if (heap_info)
		{
			if (out_size) *out_size = alloc_chunksize - sizeof (struct insert) - EXTRA_INSERT_SPACE;
		}
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


/* A client-friendly lookup function with cache. */
struct insert *lookup_object_info(const void *mem, void **out_object_start, size_t *out_object_size,
		void **ignored)
{
	return NULL; // TODO

	/* Unlike our malloc hooks, we might get called before initialization,
	   e.g. if someone tries to do a lookup before the first malloc of the
	   program's execution. Rather than putting an initialization check
	   in the fast-path functions, we bail here.  */
	/* if (!index_region) return NULL; */
	
	/* Try matching in the cache. NOTE: how does this impact bigalloc and deep-indexed 
	 * entries? In all cases, we cache them here. We also keep a "is_deepest" flag
	 * which tells us (conservatively) whether it's known to be the deepest entry
	 * indexing that storage. In this function, we *only* return a cache hit if the 
	 * flag is set. (In lookup_l01_object_info, this logic is different.) */
	/* check_cache_sanity(); */
	/* void *l01_object_start = NULL; */
	/* struct insert *found_l01 = NULL; */
	/* for (unsigned i = 0; i < LOOKUP_CACHE_SIZE; ++i) */
	/* { */
	/* 	if (lookup_cache[i].object_start && */ 
	/* 			(char*) mem >= (char*) lookup_cache[i].object_start && */ 
	/* 			(char*) mem < (char*) lookup_cache[i].object_start + lookup_cache[i].usable_size) */
	/* 	{ */
	/* 		// possible hit */
	/* 		if (lookup_cache[i].depth == 1 || lookup_cache[i].depth == 0) */
	/* 		{ */
	/* 			l01_object_start = lookup_cache[i].object_start; */
	/* 			found_l01 = lookup_cache[i].insert; */
	/* 		} */
			
	/* 		if (lookup_cache[i].is_deepest) */
	/* 		{ */
	/* 			// HIT! */
	/* 			assert(lookup_cache[i].object_start); */
	/* #if defined(TRACE_DEEP_HEAP_INDEX) || defined(TRACE_HEAP_INDEX) */
	/* 			fprintf(stderr, "Cache hit at pos %d (%p) with alloc site %p\n", i, */ 
	/* 					lookup_cache[i].object_start, (void*) (uintptr_t) lookup_cache[i].insert->alloc_site); */
	/* 			fflush(stderr); */
	/* #endif */
	/* 			assert(INSERT_DESCRIBES_OBJECT(lookup_cache[i].insert)); */

	/* 			if (out_object_start) *out_object_start = lookup_cache[i].object_start; */
	/* 			if (out_object_size) *out_object_size = lookup_cache[i].usable_size; */
	/* 			// ... so ensure we're not about to evict this guy */
	/* 			if (next_to_evict - &lookup_cache[0] == i) */
	/* 			{ */
	/* 				next_to_evict = &lookup_cache[(i + 1) % LOOKUP_CACHE_SIZE]; */
	/* 				assert(next_to_evict - &lookup_cache[0] < LOOKUP_CACHE_SIZE); */
	/* 			} */
	/* 			assert(INSERT_DESCRIBES_OBJECT(lookup_cache[i].insert)); */
	/* 			return lookup_cache[i].insert; */
	/* 		} */
	/* 	} */
	/* } */
	
	/* // didn't hit cache, but we may have seen the l01 entry */
	/* struct insert *found; */
	/* void *object_start; */
	/* unsigned short depth = 1; */
	/* if (found_l01) */
	/* { */
	/* 	/1* CARE: the cache's p_ins points to the alloc's insert, even if it's been */
	/* 	 * moved (in the suballocated case). So we re-lookup the physical insert here. *1/ */
	/* 	found = insert_for_chunk(l01_object_start); */
	/* } */
	/* else */
	/* { */
	/* 	found = lookup_l01_object_info_nocache(mem, &l01_object_start); */
	/* } */
	/* size_t size; */

	/* if (found) */
	/* { */
	/* 	size = usersize(l01_object_start); */
	/* 	object_start = l01_object_start; */
	/* 	_Bool is_deepest = INSERT_DESCRIBES_OBJECT(found); */
		
	/* 	// cache the l01 entry */
	/* 	install_cache_entry(object_start, size, 1, is_deepest, object_insert(object_start, found)); */
		
	/* 	if (!is_deepest) */
	/* 	{ */
	/* 		assert(l01_object_start); */
	/* 		/1* deep case *1/ */
	/* 		void *deep_object_start; */
	/* 		size_t deep_object_size; */
	/* 		struct insert *found_deeper = NULL; */
	/* 		/1* lookup_deep_alloc(mem, 1, found, &deep_object_start, */
	/* 		 * &deep_object_size, &containing_chunk_rec); *1/ */
	/* 		if (found_deeper) */
	/* 		{ */
	/* 			assert(0); */
	/* 			// override the values we assigned just now */
	/* 			object_start = deep_object_start; */
	/* 			found = found_deeper; */
	/* 			size = deep_object_size; */
	/* 			// cache this too */
	/* 			//g_entry(object_start, size, 2 /1* FIXME *1/, 1 /1* FIXME *1/, */ 
	/* 			//	found); */
	/* 		} */
	/* 		else */
	/* 		{ */
	/* 			// we still have to point the metadata at the *sub*indexed copy */
	/* 			assert(!INSERT_DESCRIBES_OBJECT(found)); */
	/* 			found = object_insert(mem, found); */
	/* 		} */
	/* 	} */

	/* 	if (out_object_start) *out_object_start = object_start; */
	/* 	if (out_object_size) *out_object_size = size; */
	/* } */
	
	/* assert(!found || INSERT_DESCRIBES_OBJECT(found)); */
	/* return found; */
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

int __currently_freeing = 0;  // TODO
int __currently_allocating = 0;

/* The real malloc function, sig taken from malloc(9) man page */
void *__real_malloc(unsigned long size, struct malloc_type *type, int flags);

/* The hook */
void *__wrap_malloc(unsigned long size, struct malloc_type *type, int flags)
{
	PRINTD1("malloc called, size: %u", size);
	if (!type) PRINTD("malloc, no type!");
	else PRINTD1("malloc called, type: %s", type->ks_shortdesc);
	PRINTD1("malloc __currently_allocating = %u", __currently_allocating);

	__currently_allocating++;
	void *ret;
	ret = __real_malloc(size, type, flags);
	if (ret) {
		pageindex_insert(ret, ret + size, &__generic_malloc_allocator);
		void *caller = __builtin_return_address(1);
		/* heapindex_insert(ret, ret + size, caller); */
	}
	/* 	// Insert for addr -> insert */
	/* 	unsigned long i = ADDR_ARRAY_INDEX(ret); */
	/* 	tagged_insert_array[i].addr = ret; // TODO ?? difference to start? */
	/* 	tagged_insert_array[i].ins.alloc_site_flag = 0; */
	/* 	tagged_insert_array[i].ins.alloc_site = (unsigned long) caller; */
	/* 	tagged_insert_array[i].start = ret; */
	/* 	tagged_insert_array[i].size = (size_t) size; */
	/* 	// Insert for allocsite -> uniqtype */
	/* 	i = ADDR_ARRAY_INDEX(caller); */
	/* 	tagged_uniqtype_array[i].allocsite = caller; */
	/* 	tagged_uniqtype_array[i].type = NULL;  // is_aU() first call sets this */
	/* } */
	__currently_allocating--;
	PRINTD1("malloc returning: %p", ret);
	return ret;
}
