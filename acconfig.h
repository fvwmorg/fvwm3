/* Name of config filenames in FVWM_USERDIR and FVWM_DATADIR */
#define FVWM_CONFIG "config"

/* Suffix for old (to be deprecated) config filenames */
#define FVWM2RC ".fvwm2rc"

/* Additional version information, like date */
#define VERSIONINFO ""

/* Define if gdk-imlib is used */
#undef GDK_IMLIB

/* Where to search for images. */
#undef FVWM_IMAGEPATH

/* Define if Xpm library is used. */
#undef XPM

/* Define if ppm library is used. */
#undef HAVE_PNG

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX). */
#undef WORDS_BIGENDIAN

/* Define if rplay library is used. */
#undef HAVE_RPLAY

/* Define if Xinerama should be emulated on a single screen. */
#undef USE_XINERAMA_EMULATION

/* Define if Xinerama library is used. */
#undef HAVE_XINERAMA

/* Define if Solaris' Xinerama calls are being used. */
/* (Solaris 7 11/99 and later) */
#undef HAVE_SOLARIS_XINERAMA

/* Define if Solaris' X11/extensions/xinerama.h header is provided. */
/* (Solaris 9 and later) */
#undef HAVE_SOLARIS_XINERAMA_H

/* Define if Xrender library is used. */
#undef HAVE_XRENDER

/* Define if Xft library is used. */
#undef HAVE_XFT

/* Define if Xft 2 library is used. */
#undef HAVE_XFT2

/* Define if Xft library can handle utf8 encoding */
#undef HAVE_XFT_UTF8

/* Define if stroke library is used. */
#undef HAVE_STROKE

#ifdef HAVE_STROKE
#    define STROKE_ARG(x) x,
#    define STROKE_CODE(x) x
#else
#    define STROKE_ARG(x)
#    define STROKE_CODE(x)
#endif

/* Define if readline is available. */
#undef HAVE_READLINE

/* Define if iconv (in the libc) or libiconv is available */
#undef HAVE_ICONV

/* define if we use libiconv (not needed in general: for example iconv is
* native with recent glibc) */
#undef USE_LIBICONV

/* define if second arg of iconv use const */
#undef ICONV_ARG_CONST

#ifdef USE_LIBICONV
/* define to use locale_charset in the place of nl_langinfog if libiconv
 * is used */
#undef HAVE_LIBCHARSET
#endif

/* Define if nl_langinfo is available */
#undef HAVE_CODESET

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#undef HAVE_NLS

/* Define if you want the Shaped window extensions.
 * Shaped window extensions seem to increase the window managers RSS
 * by about 60 Kbytes. They provide for leaving a title-bar on the window
 * without a border.
 * If you don't use shaped window extension, you can either make your
 * shaped windows undecorated, or live with a border and backdrop around
 * all your shaped windows (oclock, xeyes)
 *
 * If you normally use a shaped window (xeyes or oclock), you might as
 * well compile this extension in, since the memory cost is  minimal in
 * this case (The shaped window shared libs will be loaded anyway). If you
 * don't normally use a shaped window, you have to decide for yourself.
 *
 * Note: if it is compiled in, run time detection is used to make sure that
 * the currently running X server supports it. */
#undef SHAPE

/* Define if  MIT Shared Memory extension is used. */
#undef HAVE_XSHM

/* Define if fribidi library is used. */
#undef HAVE_BIDI

/* Enables the "MiniIcon" Style option to specify a small pixmap which
 * can be used as one of the title-bar buttons, shown in window list,
 * utilized by modules, etc.  Requires PIXMAP_BUTTONS to be defined
 * (see below). */
/* #undef MINI_ICONS */
/* NOTE: hard coded to 1 */
#if 1
#define FMiniIconsSupported 1
#else
#define FMiniIconsSupported 0
#endif

/* Enables tagged general decoration styles which can be assigned to
 * windows using the UseDecor Style option, or dynamically updated
 * with ChangeDecor.  To create and destroy "decor" definitions, see
 * the man page entries for AddToDecor and DestroyDecor.  There is a
 * slight memory penalty for each additionally defined decor. */
/* #undef USEDECOR */
/* NOTE: hard coded to 1 */
#define USEDECOR 1

/* Enables session management functionality. */
#undef SESSION

/* Enables to use setlocale() provided by X */
#undef X_LOCALE

/* Enable X output method */
#undef HAVE_XOUTPUT_METHOD

/* Enable the use of mkstemp */
#undef HAVE_SAFTY_MKSTEMP

/* Specify a type for sig_atomic_t if it's not available. */
#undef sig_atomic_t

/* Define to the type used in argument 1 `select'.  Usually this is an `int'. */
#undef fd_set_size_t

/* Define to the type used in arguments 2-4 of `select', if not set by system
   headers. */
#undef fd_set

/* Define a suitable cast for arguments 2-4 of `select'.  On most systems,
   this will be the empty string, as select usually takes pointers to fd_set. */
#undef SELECT_FD_SET_CAST


/*
** if you would like to see lots of debug messages from fvwm, for debugging
** purposes, uncomment the next line
*/
#undef FVWM_DEBUG_MSGS

#ifdef FVWM_DEBUG_MSGS
#   define DBUG(x,y) fvwm_msg(DBG,x,y)
#else
#   define DBUG(x,y) /* no messages */
#endif


/* Produces a log of all executed commands and their times on stderr. */
#undef FVWM_COMMAND_LOG

#ifdef FVWM_COMMAND_LOG
#   define FVWM_DEBUG_TIME 1
#endif


/* Old AIX systems (3.2.5) don't define some common keysyms. */
#undef COMPAT_OLD_KEYSYMDEF

#ifdef COMPAT_OLD_KEYSYMDEF
#  define XK_Page_Up   XK_Prior
#  define XK_Page_Down XK_Next
#endif


/* Old libstroke <= 0.4 does not use STROKE_ prefix for constants. */
#undef COMPAT_OLD_LIBSTROKE

#ifdef COMPAT_OLD_LIBSTROKE
/* currently we only use one constant */
#  define STROKE_MAX_SEQUENCE MAX_SEQUENCE
#endif


/**
 * The next few defines are options that are only changed from their values
 * shown here on systems that _don't_ use the configure script.
 **/

/* Enable tests for missing too many XEvents.  Usually you want this. */
#define WORRY_ABOUT_MISSED_XEVENTS 1

/* Define if the X11 ConnectionNumber is actually a file descriptor. */
#define HAVE_X11_FD 1

/* Define if fork() has unix semantics.  On VMS, no child process is created
   until after a successful exec(). */
#define FORK_CREATES_CHILD 1

/* Suffix for executable filenames; NULL if no extension needed. */
#define EXECUTABLE_EXTENSION NULL

/* Define to remove the extension from executable pathnames before calling
   exec(). */
#undef REMOVE_EXECUTABLE_EXTENSION

@TOP@

/*
The stuff above TOP goes to the top of config.h.in
What appears below BOTTOM goes to the bottom
This text should not appear anywhere (but it does with autoconf-2.50+).
*/

@BOTTOM@


#if RETSIGTYPE != void
#define SIGNAL_RETURN return 0
#else
#define SIGNAL_RETURN return
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

#if HAVE_ALLOCA_H
#  include <alloca.h>
#else
#  ifdef _AIX
       #pragma alloca
#  else
#    ifndef alloca /* predefined by HP cc +Olibcalls */
	 char *alloca ();
#    endif
#  endif
#endif

#ifdef STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#else
#  ifdef HAVE_STRING_H
#    include <string.h>
#  else
#    include <strings.h>
#  endif
#  ifdef HAVE_MEMORY_H
#    include <memory.h>
#  endif
#  ifdef HAVE_STDLIB_H
#    include <stdlib.h>
#  endif
#  ifdef HAVE_MALLOC_H
#    include <malloc.h>
#  endif
#  ifndef HAVE_STRCHR
#    define strchr(_s,_c)   index((_s),(_c))
#    define strrchr(_s,_c)  rindex((_s),(_c))
#  endif
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

#ifndef FEVENT_C
#include <X11/Xlib.h>
#include "libs/FEvent.h"
#endif
