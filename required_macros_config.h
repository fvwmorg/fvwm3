//These macros are all used in the codebase.

//
// Config
//

/* Version number of package */
#define VERSION "1.0.9"

/* Additional version information, like date */
#define VERSIONINFO "1.0.8-21-gd47b2ba9"

//Define to the version of this package.
#define PACKAGE_VERSION "1.0.9"

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#define HAVE_NLS 1

//
// Paths
//

//FIX: These need to be dynamic based on the prefix
#define FVWM_MODULEDIR "/usr/local/libexec/fvwm3/1.0.9"
//FIX: What does this do?
#define FVWM_DATADIR "/usr/local/share"
/* Where to look for system wide config */
#define FVWM_CONFDIR "/usr/local/etc"
//FIX: What does this do?
#define FVWM_COLORSET_PRIVATE 1
/* Where to look for the locale files */
#define LOCALEDIR "/usr/local/share/fvwm3/locale"

/* Where to search for images. */
#define FVWM_IMAGEPATH "/usr/include/X11/bitmaps:/usr/include/X11/pixmaps"

//
// File names
//

/* Suffix for old (to be deprecated) config filenames */
#define FVWM2RC ".fvwm2rc"

/* Name of config filenames in FVWM_USERDIR and FVWM_DATADIR */
#define FVWM_CONFIG "config"

//Name of package
#define PACKAGE "fvwm3"

//
// Optional Libraries
//

/* Define if Xpm library is used. */
#define XPM 1

/* Define if fribidi library it to be used. */
#define HAVE_BIDI 1

/* Define if librsvg library it to be used. */
#define HAVE_RSVG 1

//Define if Xkb library it to be used.
#define HAVE_X11_XKBLIB_H 1

//Define if Xcursor library it to be used.
#define HAVE_XCURSOR 1

//Define if Xft library it to be used.
#define HAVE_XFT 1

//Define if MIT Shared Memory library it to be used.
#define HAVE_XSHM 1

/* Define if PNG library it to be used */
#define HAVE_PNG 1

//Define if Xrender library it to be used.
#define HAVE_XRENDER 1

/* Define if the readline library it to be used. */
#define HAVE_READLINE 1

/* Define if the readline library has the full GNU interface */
#define HAVE_GNU_READLINE 1

/* Define if the iconv library is to be used either from libc or libiconv */
#define HAVE_ICONV 1

/* define if we use libiconv itself. Generally this is not needed as iconv is
    native with recent glibc */
#define USE_LIBICONV 1

//
// Optional Headers
//

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif

//Define to 1 if you have the <stdint.h> header file.
#define HAVE_STDINT_H 1

//Define to 1 if you have the <sys/select.h> header file.
#define HAVE_SYS_SELECT_H 1

//Define to 1 if you have the <sys/stat.h> header file.
#define HAVE_SYS_STAT_H 1

//Define to 1 if you have the <sys/systeminfo.h> header file.
#define HAVE_SYS_SYSTEMINFO_H 1

//Define to 1 if you have the <sys/time.h> header file.
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. This
    macro is obsolete. */
#define TIME_WITH_SYS_TIME 1

//
// Optional functions
//

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `setpgid' function. */
#define HAVE_SETPGID 1

/* Define to 1 if you have the `setpgrp' function. */
#define HAVE_SETPGRP 1

//Define to 1 if you have the `sigaction' function.
#define HAVE_SIGACTION 1

//Define to 1 if you have the `siginterrupt' function.
#define HAVE_SIGINTERRUPT 1

//Define to 1 if you have the `siglongjmp' function.
#define HAVE_SIGLONGJMP 1

//Define to 1 if you have the `sigsetjmp' function.
#define HAVE_SIGSETJMP 1

/* Define to 1 if you have the `div' function. */
#define HAVE_DIV 1

/* Define to 1 if you have the `getpwuid' function. */
#define HAVE_GETPWUID 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/*Define to 1 if you have the `sysconf' function.*/
#define HAVE_SYSCONF 1

//Define to 1 if you have the `uname' function.
#define HAVE_UNAME 1

//Define to 1 if you have the `wait3' function.
#define HAVE_WAIT3 1

//Define to 1 if you have the `waitpid' function.
#define HAVE_WAITPID 1

/* Enable the use of the system `mkstemp' function, if not then use the fvwm version of it. Was called HAVE_SAFETY_MKSTEMP */
#define HAVE_WORKING_MKSTEMP 1

//
// Helper Macros
//

/* A macro that touches a variable in a compiler independent way to suppress
 * warnings. */
#define SUPPRESS_UNUSED_VAR_WARNING(x) \
do { void *p; p = (void *)&x; (void)p; } while (0);


//
// Static
//

/* Suffix for executable filenames; NULL if no extension needed. */
#define EXECUTABLE_EXTENSION NULL

/* Define to remove the extension from executable pathnames before calling
   exec(). */
#undef REMOVE_EXECUTABLE_EXTENSION

/* Define if fork() has unix semantics.  On VMS, no child process is created
   until after a successful exec(). */
#define FORK_CREATES_CHILD 1

/* Define if the X11 ConnectionNumber is actually a file descriptor. */
#define HAVE_X11_FD 1

#ifndef min
#  define min(a,b) (((a)<(b)) ? (a) : (b))
#endif
#ifndef max
#  define max(a,b) (((a)>(b)) ? (a) : (b))
#endif
#ifndef abs
#  define abs(a) (((a)>=0)?(a):-(a))
#endif

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

//
// Unclassified
//

//IDK Why we need this
#define HAVE_ASPRINTF

#ifndef HAVE_ASPRINTF
int	asprintf(char **, const char *, ...);
int     vasprintf(char **, const char *, va_list);
#endif

/* Have nl_langinfo (CODESET) */
#define HAVE_CODESET 1

//Enable X output method
#define HAVE_XOUTPUT_METHOD 1

//define if second arg of iconv use
#define ICONV_ARG_CONST

//Define as the return type of signal handlers (`int' or `void').
#define RETSIGTYPE void

/* Define a suitable cast for arguments 2-4 of `select'. On most systems, this
    will be the empty string, as select usually takes pointers to fd_set. */
#define SELECT_FD_SET_CAST 1

//Enables session management functionality.
#define SESSION 1

//Define to 1 if the `setpgrp' function requires zero arguments.
#define SETPGRP_VOID 1

/* Define if you want the Shaped window extensions. Shaped window extensions
    seem to increase the window managers RSS by about 60 Kbytes. They provide
    for leaving a title-bar on the window without a border. If you don't use
    shaped window extension, you can either make your shaped windows
    undecorated, or live with a border and backdrop around all your shaped
    windows (oclock, xeyes) If you normally use a shaped window (xeyes or
    oclock), you might as well compile this extension in, since the memory cost
    is minimal in this case (The shaped window shared libs will be loaded
    anyway). If you don't normally use a shaped window, you have to decide for
    yourself. Note: if it is compiled in, run time detection is used to make
    sure that the currently running X server supports it. */
#define SHAPE 1

/* Define to the type of a signed integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
#define int16_t 1

/* Define to `long int' if <sys/types.h> does not define. */
#define off_t 1

/* Define as a signed integer type capable of holding a process identifier. */
#define pid_t 1

/* Specify a type for sig_atomic_t if it's not available. */
#define sig_atomic_t 1

/* Define to `unsigned int' if <sys/types.h> does not define. */
#define size_t 1

/* Define to the type of an unsigned integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
#define uint16_t 1

/* Define to empty if `const' does not conform to ANSI C. */
#define const 1

/* Define to the type used in arguments 2-4 of `select', if not set by system
   headers. */
#define fd_set 1

/* Define to the type used in argument 1 `select'. Usually this is an `int'.
   */
#define fd_set_size_t int

/* Define to remove the extension from executable pathnames before calling
   exec(). */
#define REMOVE_EXECUTABLE_EXTENSION 1

//
// Includes
//

#ifdef HAVE_STRING_H
#  include <string.h>
#endif

#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif

//
// Includes
//

#ifndef HAVE_TAILQ
#	include <sys/queue.h>
#endif

#include "libs/defaults.h"

#ifndef HAVE_STRLCAT
#   include "libs/strlcat.h"
#endif

#ifndef HAVE_STRLCPY
#   include "libs/strlcpy.h"
#endif

#include "libs/log.h"
