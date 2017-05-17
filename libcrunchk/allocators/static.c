#include <libcrunchk/include/index_tree.h>
#include <libcrunchk/include/liballocs.h>
#include <sys/stddef.h>

static _Bool trying_to_initialize = 0;
static _Bool initialized = 0;

struct itree_node *staticindex_root = NULL;

extern int heapindex_compare(const void *a, const void *b);
extern unsigned long heapindex_distance(const void *a, const void *b);

struct insert *staticindex_lookup(const void *addr) {
	const struct insert ins = {.addr = (void *)addr};
	HEAPINDEX_RLOCK;
	struct itree_node *res = itree_find_closest_under(
		&staticindex_root, staticindex_root, &ins, heapindex_compare, heapindex_distance
	);
	HEAPINDEX_UNLOCK;
	// What if res->data is freed here?
	if (res) return (struct insert *) res->data;
	return NULL;
}

void staticindex_insert(
	void *alloc_site,
	void *addr,
	_Bool alloc_site_is_actually_uniqtype
) {
	struct insert *ins =
		__real_malloc(sizeof(struct insert), M_TEMP, M_WAITOK);
	ins->alloc_site_flag = alloc_site_is_actually_uniqtype;
	ins->alloc_site = (unsigned long) alloc_site;
	ins->addr = addr;
	__libcrunch_static_entries++;
	HEAPINDEX_WLOCK;
	itree_insert(&staticindex_root, (void *)ins, heapindex_compare);
	HEAPINDEX_UNLOCK;
}

/* To make sure .meta exists even if types*.c not included in build */
int __attribute__((section (".meta"))) make_sure_dot_meta_exists = 42;
/* To have a static variable to see how much to offset for static alloc,
 * because linking in the types objects causes everything to shift.
 * There are three places a static allocator can point to, .text, .data and
 * .bss
 * Hopefully .data and .bss are beside each other and have the same offset, so
 * we use the var_symbol to find this offset and shift them if the allocsite is
 * not in .text range (i.e. <= etext ). Otherwise we shift it using the
 * func_symbol offset.
 * I hope this works... the .text could be foiled by code from the types files,
 * but hopefully nothing else is put in the .data?
 * This is done in offset_all_statics().
*/
extern char etext;
extern char edata;
int magic_static_var_symbol = 42;
int magic_static_func_symbol() {return 42;}

/* structs that are same as in types source */
struct allocsite_entry
{
	void *next;
	void *prev;
	void *allocsite;
	struct uniqtype *uniqtype;
};
struct frame_allocsite_entry
{
	unsigned offset_from_frame_base;
	struct allocsite_entry entry;
};
struct static_allocsite_entry
{
	const char *name;
	struct allocsite_entry entry;
};

extern struct static_allocsite_entry __attribute__((weak)) statics[];



/* stolen from libkern, to avoid outside calls */
static int strcmp_private(s1, s2)
        register const char *s1, *s2;
{
        while (*s1 == *s2++)
                if (*s1++ == 0)
                        return (0);
        return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}

#ifndef _KERNEL
	int mtxpool_sleep = 0;
#endif
static void offset_all_statics ()
{
	// abort if statics aren't linked in
	if (! &statics) return;

	PRINTD1("magic_static_var_symbol: %p", &magic_static_var_symbol);
	PRINTD1("magic_static_func_symbol: %p", &magic_static_func_symbol);

	// find offsets
	ptrdiff_t dot_text_offset = 0;
	ptrdiff_t dot_data_offset = 0;
	ptrdiff_t dot_bss_offset = 0;
	struct static_allocsite_entry *s = &statics[0];
	do {
		if (!s->name) continue;
		/* These should be positive offsets as we're adding to stuff to the
		 * object */
		if (strcmp_private(s->name, "magic_static_func_symbol") == 0) {
			dot_text_offset = (void *)&magic_static_func_symbol - s->entry.allocsite;
		}
		if (strcmp_private(s->name, "magic_static_var_symbol") == 0) {
			dot_data_offset = (void *)&magic_static_var_symbol - s->entry.allocsite;
		}
		/* FIXME this is hack for bss, should do properly like two above */
		if (strcmp_private(s->name, "mtxpool_sleep") == 0) {
			dot_bss_offset = (void *)&mtxpool_sleep - s->entry.allocsite;
		}
	} while(s++
		&& (!dot_text_offset || !dot_data_offset || !dot_bss_offset)
		&& s->entry.allocsite && s->entry.uniqtype
	);
	printf("dot_text_offset: %p\n", dot_text_offset);
	printf("dot_data_offset: %p\n", dot_data_offset);
	printf("dot_bss_offset: %p\n", dot_bss_offset);

	// apply offsets and insert into appropriate indices
	s = &statics[0];
	do {
		// FIXME comparing apparent allocsites with actual addresses
		ptrdiff_t offset =
			(s->entry.allocsite + dot_text_offset < &etext)
			? dot_text_offset
			: (s->entry.allocsite + dot_data_offset < &edata)
				? dot_data_offset
				: dot_bss_offset;
		/* if (0xffffffff82ab0000 <= s->entry.allocsite + offset */
		/* 	&& s->entry.allocsite + offset <= 0xffffffff82abffff */
		/* ) { */
		/* 	printf("-----------\n"); */
		/* 	printf("name: %s\n", s->name); */
		/* 	printf("apparent addr: %p\n", s->entry.allocsite); */
		/* 	printf("in .text? %s\n", (s->entry.allocsite < &etext) ? "yes" : "no"); */
		/* 	printf("offset appr addr: %p\n", s->entry.allocsite + offset); */
		/* 	printf("&mtxpool_sleep: %p\n", &mtxpool_sleep); */
		/* } */

		pageindex_insert(
			/* s->entry.allocsite + offset, */
			(s->entry.uniqtype->pos_maxoff < 4294967295 )  // < UINT_MAX
				? s->entry.allocsite + s->entry.uniqtype->pos_maxoff
				: s->entry.allocsite,
			(s+1)->entry.allocsite + offset, // should be ok to do as last static is void
			&__static_allocator
		);

		staticindex_insert(
			s->entry.uniqtype,
			s->entry.allocsite + offset,
			1
		);
	} while(s++ && (s->name || s->entry.allocsite || s->entry.uniqtype));
}


void __static_allocator_init(void)
{
	if (!initialized && !trying_to_initialize)
	{
		PRINTD("Initialising static allocator");
		trying_to_initialize = 1;
		offset_all_statics();
		initialized = 1;
		trying_to_initialize = 0;
	}
}


struct uniqtype *
static_addr_to_uniqtype(const void *static_addr, void **out_object_start) {
	struct insert *ins = staticindex_lookup(static_addr);
	if (!ins) return NULL;
	if (!ins->alloc_site_flag) {
		// something went wrong, we should only get straight uniqtypes back
		PRINTD("static_addr_to_uniqtype got a non-static address, returning");
		return NULL;
	}
	if (out_object_start) *out_object_start = ins->addr;  // correct?
	return ins->alloc_site;
}

/* extern struct tcphdr libcrunch_magic_test_tcphdr; // debug */
static liballocs_err_t get_info(void * obj, struct big_allocation *maybe_bigalloc, 
	struct uniqtype **out_type, void **out_base, 
	unsigned long *out_size, const void **out_site)
{
	/* Debugging */
	/* if (!obj) { */
	/* 	PRINTD("static get_info debugging"); */
	/* 	struct static_allocsite_entry *s = &statics[0]; */
	/* 	do { */
	/* 		if (!s->name) continue; */
	/* 		if (strcmp_private(s->name, "libcrunch_magic_test_tcphdr") == 0) { */
	/* 			ptrdiff_t offset = (s->entry.allocsite < &etext) */
	/* 				? dot_text_offset : dot_data_offset; */
	/* 			PRINTD1("found magic var, offset: %p", offset); */
	/* 			/1* PRINTD1("var actual loc: %p", &libcrunch_magic_test_tcphdr); *1/ */
	/* 			PRINTD1("var apparent loc: %p", s->entry.allocsite); */
	/* 		} */
	/* 	} while(s++ && (s->name || s->entry.allocsite || s->entry.uniqtype)); */
	/* 	return NULL; */
	/* } */
	++__liballocs_hit_static_case;
	void *object_start;
	struct uniqtype *alloc_uniqtype = static_addr_to_uniqtype(obj, &object_start);
	if (out_type) *out_type = alloc_uniqtype;
	if (!alloc_uniqtype)
	{
		++__liballocs_aborted_static;
		return &__liballocs_err_unrecognised_static_object;
	}

	// else we can go ahead
	if (out_base) *out_base = object_start;
	if (out_site) *out_site = object_start;
	if (out_size) *out_size = alloc_uniqtype->pos_maxoff;
	return NULL;
}


struct allocator __static_allocator = {
	.name = "static",
	.is_cacheable = 1,
	.get_info = get_info
};
