/* Allow GCC extensions to work, if you have GCC. */
#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later. */
#  if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__
#    define __attribute__(x)
#  endif
/* The __-protected variants of `format' and `printf' attributes
 * are accepted by gcc versions 2.6.4 (effectively 2.7) and later. */
#  if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#    define __format__ format
#    define __printf__ printf
#  endif
#endif

#ifdef HAVE_STRING_H
#  include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif
#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif

#if defined (HAVE_MALLOC_H) && !defined (__FreeBSD__) && !defined (__OpenBSD__) && !defined(__NetBSD__)
#  include <malloc.h>
#endif
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif
#ifndef HAVE_STRCHR
#  define strchr(_s,_c)   index((_s),(_c))
#  define strrchr(_s,_c)  rindex((_s),(_c))
#endif

#ifndef HAVE_MEMCPY
#  define memcpy(_d,_s,_l)  bcopy((_s),(_d),(_l))
#endif
#ifndef HAVE_MEMMOVE
#  define memmove(_d,_s,_l) bcopy((_s),(_d),(_l))
#endif

#if HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifndef min
#  define min(a,b) (((a)<(b)) ? (a) : (b))
#endif
#ifndef max
#  define max(a,b) (((a)>(b)) ? (a) : (b))
#endif
#ifndef abs
#  define abs(a) (((a)>=0)?(a):-(a))
#endif

#include "libs/defaults.h"

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

#ifdef HAVE_LSTAT
#define DO_USE_LSTAT 1
#define fvwm_lstat(x,y) lstat(x,y)
#else
#define DO_USE_LSTAT 0
#define fvwm_lstat(x,y) -1
#endif

/* A macro that touches a variable in a compiler independent way to suppress
 * warnings. */
#define SUPPRESS_UNUSED_VAR_WARNING(x) \
do { void *p; p = (void *)&x; (void)p; } while (0);

#ifdef HOST_MACOS
#undef HAVE_STRLCAT
#undef HAVE_STRLCPY
#else
#ifndef HAVE_STRLCAT
#   include "libs/strlcat.h"
#endif

#ifndef HAVE_STRLCPY
#   include "libs/strlcpy.h"
#endif
#endif

#ifndef HAVE_ASPRINTF
int	asprintf(char **, const char *, ...);
int     vasprintf(char **, const char *, va_list);
#endif

#include "libs/log.h"


/* Define to empty if 'const' does not conform to ANSI C. */
#undef const

/* Define to the type used in arguments 2-4 of `select', if not set by system
   headers. */
#undef fd_set

/* Define to the type used in argument 1 `select'. Usually this is an `int'.
   */
#undef fd_set_size_t

/* Define to '__inline__' or '__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#undef inline
#endif

/* Define to the type of a signed integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
#undef int16_t

/* Define to 'long int' if <sys/types.h> does not define. */
#undef off_t

/* Define as a signed integer type capable of holding a process identifier. */
#undef pid_t

/* Specify a type for sig_atomic_t if it's not available. */
#undef sig_atomic_t

/* Define as 'unsigned int' if <stddef.h> doesn't define. */
#undef size_t

/* Define to the type of an unsigned integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
#undef uint16_t