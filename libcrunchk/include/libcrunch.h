#ifndef LIBCRUNCH_H
#define LIBCRUNCH_H

#include <sys/types.h>

#ifndef NULL
#define NULL 0
#endif

/* Debug printouts */
#if DEBUG_STUBS == 1
  #include <sys/ktr.h>
  #define PRINTD(x) CTR0(KTR_PTRACE, x)
  #define PRINTD1(f, x) CTR1(KTR_PTRACE, f, x)
  #define PRINTD2(f, x1, x2) CTR2(KTR_PTRACE, f, x1, x2)
#elif DEBUG_STUBS == 2
  extern int printf(const char* format, ... );
  #define PRINTD(x) printf(x "\n")
  #define PRINTD1(f, x) printf(f "\n", x)
  #define PRINTD2(f, x1, x2) printf(f "\n", x1, x2)
#else
  #define PRINTD(x) if(0 && x)
  #define PRINTD1(f, x) if(0 && f && x)
  #define PRINTD2(f, x1, x2) if(0 && f && x1 && x2)
#endif

/* Simply converting the existing debug calls for now, want to avoid calling
 * string operations in kernel */
#define __libcrunch_debug_level 999
#define debug_printf(lvl, fmt, ...) do { \
    if ((lvl) <= __libcrunch_debug_level) { \
      PRINTD(#lvl fmt #__VA_ARGS__); \
    } \
  } while (0)


/*
 * libcrunch_cil_inlines.h
*/
#ifndef unlikely
#define __libcrunch_defined_unlikely
#define unlikely(cond) (__builtin_expect( (cond), 0 ))
#endif
#ifndef likely
#define __libcrunch_defined_likely
#define likely(cond)   (__builtin_expect( (cond), 1 ))
#endif

#endif /* LIBCRUNCH_H */
