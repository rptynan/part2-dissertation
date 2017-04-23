#ifndef UNIQTYPE_H
#define UNIQTYPE_H

struct uniqtype;

/* Special uniqtypes from liballocs_private.h */
// TODO, these need to come from somewhere
extern struct uniqtype *pointer_to___uniqtype__void __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype__signed_char __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype__unsigned_char __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype____PTR_signed_char __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype____PTR___PTR_signed_char __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype__Elf64_auxv_t __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype____ARR0_signed_char __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype__intptr_t __attribute__((visibility("hidden")));

/* uniqtypes-defs.h */
enum uniqtype_kind {
	VOID,
	ARRAY = 0x1,
	BASE = 0x2,
	ENUMERATION = 0x4,
	COMPOSITE = 0x6,
	ADDRESS = 0x8,
	SUBPROGRAM = 0xa,
	SUBRANGE = 0xc
};

/* For a struct, we have at least three fields: name, offset, type. 	 */
/* In fact we might want *more* than that: a flag to say whether it's an */
/* offset or an absolute address (to encode the mcontext case). HMM.	 */
/* To avoid storing loads of pointers to close-by strings, each needing  */
/* relocation, we point to a names *vector* from a separate related[] entry. */
struct uniqtype_rel_info
{
   union {
	   struct {
		   struct uniqtype *ptr;
	   } t;
	   struct {
		   unsigned long val;
		   /* const char *name; might as well? NO, to save on relocations */
	   } enumerator;
	   /* For struct members, the main complexity is modelling stack frames  */
	   /* in the same formalism. Do we want to model locals that are in fact */
	   /* not stored in a manifest (type-directed) rep but are recoverable   */
	   /* (this is DW_OP_stack_value)? What about not stored at all?         */
	   /* We could do "stored (in frame, stable rep) (i.e .the good case)",  */
	   /* "stored (in register or static storage, stable rep) (common)",     */
	   /* "recoverable" (getter/setter), "absent"?                           */
	   struct {
		   struct uniqtype *ptr;
		   unsigned long off:56;
		   unsigned long is_absolute_address:1;
		   unsigned long may_be_invalid:1;
	   } memb;
	   struct {
		   const char **n; /* names vector */
	   } memb_names;
   } un;
};

struct alloc_addr_info
{
	unsigned long addr:47;
	unsigned flag:1;
	unsigned bits:16;
};

struct mcontext;
/* "make_precise" func for encoding dynamic / data-dependent reps (e.g. stack frame, hash table) */
typedef struct uniqtype *make_precise_fn_t(
	struct uniqtype *in,
	struct uniqtype *out,
	unsigned long out_len,
	void *obj,
	void *alloc_base,
	unsigned long alloc_sz,
	void *ip,
	struct mcontext *ctxt
);

struct uniqtype
{
   struct alloc_addr_info cache_word;
   unsigned pos_maxoff; /* positive size in bytes, or UINT_MAX for unbounded/unrep'able */
   union {
       struct {
           unsigned kind:4;
           unsigned unused_:28;
       } info;
       struct {
           unsigned kind:4;
       } _void;
       struct {
           unsigned kind:4;
           unsigned enc:6; /* i.e. up to 64 distinct encodings */
           unsigned one_plus_log_bit_size_delta:4; /* i.e. up to 15 i.e. delta of up to 2^14 less than implied bit size (8 * byte size) */
             signed bit_size_delta_delta:8; /* i.e. vary the delta +/- 127 bits */
             signed bit_off:10; /* i.e. bit offsets up to 2^9 *from either end* */
       } base; /* related[0] is signedness complement; could also do same-twice-as-big, same-twice-as-small? */
       struct {
           unsigned kind:4;
           unsigned is_contiguous:1; /* idea */
           unsigned is_log_spaced:1; /* idea (inefficiency: implies not is_contiguous) */
           unsigned nenum:26; /* HMM */
       } enumeration; /* related[0] is base type, related[1..nenum] are enumerators -- HMM, t is the *value*? */
       struct {
           unsigned kind:4;
           unsigned nmemb:20; /* 1M members should be enough */
           unsigned not_simultaneous:1; /* i.e. whether any member may be invalid */
       } composite; /* related[nmemb] is names ptr could also do "refines" i.e. templatey relations? */
       struct {
           unsigned kind:4;
           unsigned indir_level:5; /* contractually, after how many valid derefs might we get a non-address */
           unsigned genericity:1; /* I wrote "we need this when deciding whether or not to overwrite" -- can't remember what this means*/
           unsigned log_min_align:6; /* useful? just an idea */
       } address; /* related[0] is immediate pointee type; could also do ultimate non-pointer type? */
       struct {
           unsigned kind:4;
           unsigned narg:10; /* 1023 arguments is quite a lot */
           unsigned nret:10; /* sim. return values */
           unsigned is_va:1; /* is variadic */
           unsigned cc:7;    /* calling convention */
       } subprogram; /* related[0..nret] are return types; contained[nret..nret+narg] are args */
       struct {
           unsigned kind:4;
           unsigned min:14;
           unsigned max:14;
       } subrange; /* related[0] is host type */
       struct {
           unsigned is_array:1; /* because ARRAY is 8, i.e. top bit set */
           unsigned nelems:31; /* for consistency with pos_maxoff, use -1 for unbounded/unknown */
       } array; /* related[0] is element type */
       unsigned as_word; /* for funky optimizations, e.g. "== -1" to test for unbounded array */
   } un;
   make_precise_fn_t *make_precise; /* NULL means identity function */
   struct uniqtype_rel_info related[];
};

#define UNIQTYPE_POS_MAXOFF_UNBOUNDED ((1ul << (8*sizeof(unsigned int)))-1) /* UINT_MAX */
#define UNIQTYPE_ARRAY_LENGTH_UNBOUNDED ((1u<<31)-1)

#define UNIQTYPE_IS_SUBPROGRAM_TYPE(u)   ((u)->un.info.kind == SUBPROGRAM)
#define UNIQTYPE_SUBPROGRAM_ARG_COUNT(u) ((u)->un.subprogram.narg)
#define UNIQTYPE_IS_POINTER_TYPE(u)      ((u)->un.info.kind == ADDRESS)
#define UNIQTYPE_POINTEE_TYPE(u)         (UNIQTYPE_IS_POINTER_TYPE(u) ? (u)->related[0].un.t.ptr : (void*)0)
#define UNIQTYPE_IS_ARRAY_TYPE(u)        ((u)->un.array.is_array)
#define UNIQTYPE_IS_COMPOSITE_TYPE(u)    ((u)->un.info.kind == COMPOSITE)
#define UNIQTYPE_HAS_SUBOBJECTS(u)       (UNIQTYPE_IS_COMPOSITE_TYPE(u) || UNIQTYPE_IS_ARRAY_TYPE(u))
#define UNIQTYPE_HAS_KNOWN_LENGTH(u)     ((u)->pos_maxoff != UINT_MAX)
#define UNIQTYPE_IS_BASE_TYPE(u)         ((u)->un.info.kind == BASE)
#define UNIQTYPE_IS_ENUM_TYPE(u)         ((u)->un.info.kind == ENUMERATION)
#define UNIQTYPE_IS_BASE_OR_ENUM_TYPE(u) (UNIQTYPE_IS_BASE_TYPE(u) || UNIQTYPE_IS_ENUM_TYPE(u))
#define UNIQTYPE_ARRAY_LENGTH(u)         (UNIQTYPE_IS_ARRAY_TYPE(u) ? (u)->un.array.nelems : -1)
#define UNIQTYPE_ARRAY_ELEMENT_TYPE(u)   (UNIQTYPE_IS_ARRAY_TYPE(u) ? (u)->related[0].un.t.ptr : (struct uniqtype*)0)
#define UNIQTYPE_COMPOSITE_MEMBER_COUNT(u) (UNIQTYPE_IS_COMPOSITE_TYPE(u) ? (u)->un.composite.nmemb : 0)
#define UNIQTYPE_IS_2S_COMPL_INTEGER_TYPE(u) \
   ((u)->un.info.kind == BASE && (u)->un.base.enc == 0x5 /*DW_ATE_signed */)
#define UNIQTYPE_BASE_TYPE_SIGNEDNESS_COMPLEMENT(u) \
   (((u)->un.info.kind == BASE && \
       ((u)->un.base.enc == 0x5 /* DW_ATE_signed */ || ((u)->un.base.enc == 0x7 /* DW_ATE_unsigned */))) ? \
	    (u)->related[0].un.t.ptr : (struct uniqtype *)0)
#define UNIQTYPE_NAME(u) __liballocs_uniqtype_name(u) /* helper in liballocs.c */
#define UNIQTYPE_SYMBOL_NAME(u) __liballocs_uniqtype_symbol_name(u) /* helper in liballocs.c */
#define UNIQTYPE_IS_SANE(u) ( \
	((u)->un.array.is_array && ((u)->un.array.nelems == 0 || (u)->pos_maxoff > 0)) \
	|| ((u)->un.info.kind == VOID && (u)->pos_maxoff == 0) \
	|| ((u)->un.info.kind == BASE && (u)->un.base.enc != 0) \
	|| ((u)->un.info.kind == ENUMERATION && 1 /* FIXME */) \
	|| ((u)->un.info.kind == COMPOSITE && ((u)->pos_maxoff <= 1 || (u)->un.composite.nmemb > 0)) \
	|| ((u)->un.info.kind == ADDRESS && 1 /* FIXME */) \
	|| ((u)->un.info.kind == SUBRANGE && 1 /* FIXME */) \
	|| ((u)->un.info.kind == SUBPROGRAM && (u)->related[0].un.t.ptr != NULL) \
	)
#define NAME_FOR_UNIQTYPE(u) UNIQTYPE_NAME(u)
#define UNIQTYPE_BASE_TYPE_BIT_SIZE(u)         (((u)->un.info.kind != BASE) ? 0 : \
                                                   8*(u)->pos_maxoff - ( \
                                                  ( (u)->un.base.one_plus_log_bit_size_delta ? \
                                                    1ul<<((u)->un.base.one_plus_log_bit_size_delta - 1) \
                                                    : 0 ) + (u)->un.base.bit_size_delta_delta \
                                                  ) )
#define UNIQTYPE_BASE_TYPE_BIT_OFFSET(u)         (((((u)->un.info.kind != BASE) ? 0 : \
                                           (((u)->un.base.bit_off) < 0) ? \
                                                    (8*((u)->pos_maxoff) - (-((u)->un.base.bit_off))) \
                                                       : (u)->un.base.bit_off)))

#endif /* UNIQTYPE_H */
