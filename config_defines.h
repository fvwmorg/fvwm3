#include <stdio.h>

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

//#include "libs/defaults.h"

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

//#include "libs/log.h"
