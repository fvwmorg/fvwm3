/* Suffix for config filenames */
#define FVWMRC ".fvwm2rc"

/* Define if gdk-imlib is used */
#undef IMLIB

/* define if GNOME WM hints are enabled */
#undef GNOME

/* Where to search for images.  */
#undef FVWM_IMAGEPATH

/* Define if Xpm library is used.  */
#undef XPM

/* Define if rplay library is used.  */
#undef HAVE_RPLAY

/* Define if stroke library is used. */
#undef HAVE_STROKE

/* Define if readline is available.  */
#undef HAVE_READLINE

/* Define to disable motif applications ability to have modal dialogs.
 * Use with care.  */
#undef MODALITY_IS_EVIL

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
 * the currently running X server supports it.  */
#undef SHAPE

/* Enables the ActiveDown button state.  This allows different button
 * styles for pressed down buttons on active windows (also for the
 * title-bar if EXTENDED_TITLESTYLE is enabled below).  The man page
 * refers to this button state as "ActiveDown."  If not defined, the
 * "ActiveUp" state is used instead.  Disabling this reduces memory
 * usage.  */
/* NOTE: this option will be removed in the near future and replaced with a
 * configuration command */
/* #undef ACTIVEDOWN_BTNS */
#define ACTIVEDOWN_BTNS 1

/* Enables the Inactive button state.  This allows different button
 * styles for inactive windows (also for the title-bar if
 * EXTENDED_TITLESTYLE is enabled below).  The man page refers to this
 * button state as "Inactive."  If not defined, the "ActiveUp" state
 * is used instead.  Disabling this reduces memory usage.  */
/* NOTE: this option will be removed in the near future and replaced with a
 * configuration command */
/* #undef INACTIVE_BTNS */
#define INACTIVE_BTNS 1

/* Enables the "MiniIcon" Style option to specify a small pixmap which
 * can be used as one of the title-bar buttons, shown in window list,
 * utilized by modules, etc.  Requires PIXMAP_BUTTONS to be defined
 * (see below).  */
/* #undef MINI_ICONS */
/* NOTE: this option will be removed in the near future (i.e. hardcoded on). */
#define MINI_ICONS 1

/* Enables the vector button style.  This button type is considered
 * "standard," so it is recommended that you leave it in.  */
/* #undef VECTOR_BUTTONS */
/* NOTE: this option will be removed in the near future (i.e. hardcoded on). */
#define VECTOR_BUTTONS 1

/* Enables the pixmap button style.  You must have Xpm support to use
 * color pixmaps.  See the man page button style entries for "Pixmap"
 * and "TiledPixmap" for usage information.  */
/* #undef PIXMAP_BUTTONS */
/* NOTE: this option will be removed in the near future (i.e. hardcoded on). */
#define PIXMAP_BUTTONS 1

/* Enables the gradient button style.  See the man page button style
 * entries for "HGradient" and "VGradient" for usage information.  */
/* #undef GRADIENT_BUTTONS */
/* NOTE: this option will be removed in the near future (i.e. hardcoded on). */
#define GRADIENT_BUTTONS 1

/* Enables stacked button styles (also for the title-bar if
 * EXTENDED_TITLESTYLE is enabled below).  There is a slight memory
 * penalty for each additional style. See the man page entries for
 * AddButtonStyle and AddTitleStyle for usage information.  */
/* #undef MULTISTYLE */
/* NOTE: this option will be removed in the near future (i.e. hardcoded on). */
#define MULTISTYLE 1

/* Enables styled title-bars (specified with the TitleStyle command in
 * a similar fashion to the ButtonStyle command).  It also compiles in
 * support to change the title-bar height.  */
/* #undef EXTENDED_TITLESTYLE */
/* NOTE: this option will be removed in the near future (i.e. hardcoded on). */
#define EXTENDED_TITLESTYLE 1

/* Enables the BorderStyle command.  Not all button styles are
 * available.  See the man page entry for BorderStyle for usage
 * information.  If you are also using PIXMAP_BUTTONS, you can also
 * texture your borders with tiled pixmaps.  The BorderStyle command
 * has Active and Inactive states, regardless of the -DACTIVEDOWN_BTNS
 * and -DINACTIVE_BTNS defines.  */
/* #undef BORDERSTYLE */
/* NOTE: this option will be removed in the near future (i.e. hardcoded on). */
#define BORDERSTYLE 1

/* Enables tagged general decoration styles which can be assigned to
 * windows using the UseDecor Style option, or dynamically updated
 * with ChangeDecor.  To create and destroy "decor" definitions, see
 * the man page entries for AddToDecor and DestroyDecor.  There is a
 * slight memory penalty for each additionally defined decor.  */
/* #undef USEDECOR */
/* NOTE: this option will be removed in the near future (i.e. hardcoded on). */
#define USEDECOR 1

/* Enables session management functionality. */
#undef SESSION

/* Enables X11 multibyte character support */
#undef I18N_MB

/* Enables to use setlocale() provided by X */
#undef X_LOCALE

/* Enables X11 multibyte character support */
#undef I18N_MB

/* Enables to use setlocale() provided by X */
#undef X_LOCALE

/* Specify a type for sig_atomic_t if it's not available.  */
#undef sig_atomic_t

/* Define to the type used in argument 1 `select'.  Usually this is an `int'.  */
#undef fd_set_size_t

/* Define to the type used in arguments 2-4 of `select', if not set by system
   headers.  */
#undef fd_set

/* Define a suitable cast for arguments 2-4 of `select'.  On most systems,
   this will be the empty string, as select usually takes pointers to fd_set.  */
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

#error The stuff above TOP goes to the top of config.h.in
#error What appears below BOTTOM goes to the bottom
#error This text should not appear anywhere!

@BOTTOM@


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
