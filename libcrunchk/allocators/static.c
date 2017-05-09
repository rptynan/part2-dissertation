#include <libcrunchk/include/pageindex.h>
#include <libcrunchk/include/liballocs.h>
#include <sys/stddef.h>

static _Bool trying_to_initialize = 0;
static _Bool initialized = 0;

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


static void offset_all_statics ()
{
	// abort if statics aren't linked in
	if (! &statics) return;

	PRINTD1("magic_static_var_symbol: %p", &magic_static_var_symbol);
	PRINTD1("magic_static_func_symbol: %p", &magic_static_func_symbol);

	// find offsets
	ptrdiff_t dot_text_offset = 0;
	ptrdiff_t dot_data_offset = 0;
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
	} while(s++ && (s->name || s->entry.allocsite || s->entry.uniqtype));

	// apply offsets and insert into appropriate indices
	s = &statics[0];
	do {
		ptrdiff_t offset = (s->entry.allocsite < &etext)
			? dot_text_offset : dot_data_offset;
		s->entry.allocsite += offset;
		/* PRINTD("-----------"); */
		/* PRINTD1("name: %s", s->name); */
		/* PRINTD1("addr: %p", s->entry.allocsite); */
		/* PRINTD1("in .text? %s", (s->entry.allocsite < &etext) ? "yes" : "no"); */
		/* PRINTD1("actual addr: %p", s->entry.allocsite + offset); */

		pageindex_insert(
			s->entry.allocsite,
			(s+1)->entry.allocsite + offset, // should be ok to do as last static is void
			&__static_allocator
		);

		// We piggyback on heapindex, set flag because we have uniqtype
		heapindex_insert(
			s->entry.uniqtype,
			s->entry.allocsite,
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
	struct insert *ins = heapindex_lookup(static_addr);
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
