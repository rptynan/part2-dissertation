struct uniqtype;

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

#define UNIQTYPE_NAME(u) __liballocs_uniqtype_name(u) /* helper in liballocs.c */
#define NAME_FOR_UNIQTYPE(u) UNIQTYPE_NAME(u)
#define UNIQTYPE_POS_MAXOFF_UNBOUNDED ((1ul << (8*sizeof(unsigned int)))-1) /* UINT_MAX */

extern struct allocator __generic_malloc_allocator;
