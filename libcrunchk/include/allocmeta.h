#ifndef ALLOC_META_H
#define ALLOC_META_H

#include <sys/types.h>
#include <libcrunchk/include/pageindex.h>
#include <libcrunchk/include/uniqtype.h>

struct big_allocation;

/* The idealised base-level allocator protocol. These operations are mostly
   to be considered logically; some allocators (e.g. stack, GC) "inline" them
   rather than defining them as entry points. However, some allocators do define
   corresponding entry points, like malloc; here it would make sense to implement 
   these operations as an abstraction layer. I'm not yet sure how useful this is. */
#define ALLOC_BASE_API(fun, arg) \
fun(struct allocated_chunk *,new_uninit     ,arg(size_t, sz),arg(size_t,align),arg(struct uniqtype *,t)) \
fun(struct allocated_chunk *,new_zero       ,arg(size_t, sz),arg(size_t,align),arg(struct uniqtype *,t)) \
fun(void,                    delete         ,arg(struct allocated_chunk *,start)) \
fun(_Bool,                   resize_in_place,arg(struct allocated_chunk *,start),arg(size_t,new_sz)) \
fun(struct allocated_chunk *,safe_migrate,   arg(struct allocated_chunk *,start),arg(struct allocator *,recipient)) /* may fail */\
fun(struct allocated_chunk *,unsafe_migrate, arg(struct allocated_chunk *,start),arg(struct allocator *,recipient)) /* needn't free existing (stack) */\
fun(void,                    register_suballoc,arg(struct allocated_chunk *,start),arg(struct allocator *,suballoc))

#define ALLOC_REFLECTIVE_API(fun, arg) \
/* The *process-wide* reflective interface of liballocs */ \
fun(struct uniqtype *  ,get_type,      arg(void *, obj)) /* what type? */ \
fun(void *             ,get_base,      arg(void *, obj))  /* base address? */ \
fun(unsigned long      ,get_size,      arg(void *, obj))  /* size? */ \
fun(const void *       ,get_site,      arg(void *, obj))  /* where allocated?   optional   */ \
fun(struct liballocs_err *	,get_info, arg(void *, obj), \
									   arg(struct big_allocation *, maybe_alloc), \
									   arg(struct uniqtype **, out_type), \
									   arg(void **, out_base), \
									   arg(unsigned long*, out_size), \
									   arg(const void**, out_site)) \
fun(struct big_allocation *,ensure_big,arg(void *, obj)) \
/* fun(Dl_info            ,dladdr,        arg(void *, obj))  /1* dladdr-like -- only for static *1/ \ */ \
/* fun(lifetime_policy_t *,get_lifetime,  arg(void *, obj)) \ */ \
/* fun(addr_discipl_t     ,get_discipl,   arg(void *, site)) /1* what will the code (if any) assume it can do with the ptr? *1/ \ */ \
fun(_Bool              ,can_issue,     arg(void *, obj), arg(off_t, off)) \
/* fun(size_t             ,raw_metadata,  arg(struct allocated_chunk *,start),arg(struct alloc_metadata **, buf)) \ */ \
fun(void               ,set_type,      arg(struct uniqtype *,new_t)) /* optional (stack) */\
fun(void               ,set_site,      arg(struct uniqtype *,new_t)) /* optional (stack) */

#define __allocmeta_fun_arg(argt, name) argt
#define __allocmeta_fun_ptr(rett, name, ...) rett (*name)( __VA_ARGS__ );

struct allocator
{
	const char *name;
	_Bool is_cacheable; /* HACK. FIXME: check libcrunch / is_a_cache really gets invalidated by all allocators. */
	ALLOC_REFLECTIVE_API(__allocmeta_fun_ptr, __allocmeta_fun_arg)
	/* Put the base API last, because it's least likely to take non-NULL values. */
	ALLOC_BASE_API(__allocmeta_fun_ptr, __allocmeta_fun_arg)
};


extern int __currently_allocating;
extern int __currently_freeing;

extern struct allocator __generic_malloc_allocator;
extern struct allocator __static_allocator;

#endif /* ALLOC_META_H */
