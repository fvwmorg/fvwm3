/*----------------------------------------------------------------------------------------------------------------------------------
    File     : config.h for Fvwm2
    Author   : Fabien Villard (Villard_F@Decus.Fr)
    Date     : 20-JAN-1999
    Descrip  : adapted for VMS from the config.h.in generated automatically from configure.in by autoheader.
               Compatibility stuff for Fvwm.
               Tested with Dec 5.7 on Alpha.
    Note     : this file can be edited to change some options.
    Modifs   :
      25-JAN-1999  Fab  Version 2.1.10 de Fvwm. Autoconfig gives args types for select().
----------------------------------------------------------------------------------------------------------------------------------*/
/* --- Those defines were intialy in makefiles and command lines --- */
#define FVWMRC "FvwmRc.Dat"
#define FVWM_MODULEDIR "Fvwm$BaseDir:[Modules.Bin]"
#define FVWM_CONFIGDIR "/Sys$Login"

#include "VMS/vms.h"

/* - Replacement functions - */
#define fork vfork
#define execvp execv
#define execl VMS_ExecL
#define fvwm_msg VMS_msg
#define select VMS_select_pipes

#define CoerceEnterNotifyOnCurrentWindow \
        CoerceEnterNotifyOnCurrentWind          /* - Nname too long, exceeds 31 characters. - */

#define __attribute__(toto) 

#undef DEBUG_EVENTS                             /* --- Define to trace My_XNextEvent and DispatchEvent in events.c --- */
#define DEBUG_MODULES                           /* --- Define to trace modules launching --- */

#define PACKAGE "VMS port (25-JAN-1999)"
#define VERSION "2.2"

/* Where to search for the fvwm icons.  */
#define FVWM_IMAGEPATH "Sys$Login:"

/* Define if Xpm library is used.  */
#define XPM

/* Define if rplay library is used.  */
#undef HAVE_RPLAY

/* Define if readline is available.  */
#undef HAVE_READLINE

/* Define to disable motif applications ability to have modal dialogs. Use with care.  */
#undef MODALITY_IS_EVIL

/* Tells the WM not to request save unders for pop-up menus. A quick test using monochrome X11 shows that save
 * unders cost about 4Kbytes RAM, but saves a lot of window redraws if you have windows that take a while
 * to refresh. For xcolor, I assume the cost is more like 4Kbytesx8 = 32kbytes (256 color).  */
#define NO_SAVEUNDERS

/* Define if you want the Shaped window extensions.
 * Shaped window extensions seem to increase the window managers RSS by about 60 Kbytes. They provide for leaving a title-bar
 * on the window without a border. If you don't use shaped window extension, you can either make your shaped windows undecorated,
 * or live with a border and backdrop around all your shaped windows (oclock, xeyes)
 *
 * If you normally use a shaped window (xeyes or oclock), you might as well compile this extension in, since the memory cost is
 * minimal in this case (The shaped window shared libs will be loaded anyway). If you don't normally use a shaped window, you have
 * to decide for yourself.
 *
 * Note: if it is compiled in, run time detection is used to make sure that the currently running X server supports it.  */
#undef SHAPE

/* Enables the ActiveDown button state.  This allows different button styles for pressed down buttons on active windows (also
 * for the title-bar if EXTENDED_TITLESTYLE is enabled below).  The man page refers to this button state as "ActiveDown."  If
 * not defined, the "ActiveUp" state is used instead.  Disabling this reduces memory usage.  */
#define ACTIVEDOWN_BTNS

/* Enables the Inactive button state.  This allows different button styles for inactive windows (also for the title-bar if
 * EXTENDED_TITLESTYLE is enabled below).  The man page refers to this button state as "Inactive."  If not defined, the "ActiveUp"
 * state is used instead.  Disabling this reduces memory usage.  */
#define INACTIVE_BTNS

/* Enables the "MiniIcon" Style option to specify a small pixmap which can be used as one of the title-bar buttons, shown in
 * window list, utilized by modules, etc.  Requires PIXMAP_BUTTONS to be defined (see below).  */
#define MINI_ICONS

/* Enables the vector button style.  This button type is considered "standard," so it is recommended that you leave it in.  */
#define VECTOR_BUTTONS

/* Enables the pixmap button style.  You must have Xpm support to use color pixmaps.  See the man page button style entries for
 * "Pixmap" and "TiledPixmap" for usage information.  */
#define PIXMAP_BUTTONS

/* Enables the gradient button style.  See the man page button style entries for "HGradient" and "VGradient" for usage
 * information.  */
#define GRADIENT_BUTTONS

/* Enables stacked button styles (also for the title-bar if EXTENDED_TITLESTYLE is enabled below).  There is a slight memory
 * penalty for each additional style. See the man page entries for AddButtonStyle and AddTitleStyle for usage information.  */
#define MULTISTYLE

/* Enables styled title-bars (specified with the TitleStyle command in a similar fashion to the ButtonStyle command).
 * It also compiles in support to change the title-bar height.  */
#define EXTENDED_TITLESTYLE

/* Enables the BorderStyle command.  Not all button styles are available.  See the man page entry for BorderStyle for usage
 * information.  If you are also using PIXMAP_BUTTONS, you can also texture your borders with tiled pixmaps. The BorderStyle
 * command has Active and Inactive states, regardless of the -DACTIVEDOWN_BTNS and -DINACTIVE_BTNS defines.  */
#define BORDERSTYLE

/* Enables tagged general decoration styles which can be assigned to windows using the UseDecor Style option, or dynamically
 * updated with ChangeDecor.  To create and destroy "decor" definitions, see the man page entries for AddToDecor and DestroyDecor.
 * There is a slight memory penalty for each additionally defined decor.  */
#define USEDECOR

/* Enables the WindowShade function.  This function "rolls" the window up so only the title-bar remains.  See the man page entry
 * for "WindowShade" for more information.  */
#define WINDOWSHADE

/* Enables session management functionality. */
#undef SESSION

/* Specify a type for sig_atomic_t if it's not available.  */
#undef sig_atomic_t


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
#undef WORRY_ABOUT_MISSED_XEVENTS

/* Define if the X11 ConnectionNumber is actually a file descriptor. */
#undef HAVE_X11_FD 

/* Define if fork() has unix semantics.  On VMS, no child process is created
   until after a successful exec(). */
#undef FORK_CREATES_CHILD

/* Suffix for executable filenames; NULL if no extension needed. */
#define EXECUTABLE_EXTENSION ".exe"

/* Define to remove the extension from executable pathnames before calling
   exec(). */
#define REMOVE_EXECUTABLE_EXTENSION 1


/* Define to empty if the keyword does not work.  */
#undef const

/* Define if you have the strftime function.  */
#define HAVE_STRFTIME 1

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* ### Define as __inline if that's what the C compiler calls it.  */
/*
#define inline __inline
*/
#define inline 

/* Define if on MINIX.  */
#undef _MINIX

/* Define to `long' if <sys/types.h> doesn't define.  */
#undef off_t

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef pid_t

/* ### Define if the system does not provide POSIX.1 features except with this defined.  */
#undef _POSIX_1_SOURCE

/* ### Define if you need to in order for stat and other things to work.  */
#undef _POSIX_SOURCE

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to the type of arg1 for select(). */
#define SELECT_TYPE_ARG1 int

/* Define to the type of args 2, 3 and 4 for select(). */
#define SELECT_TYPE_ARG234 (fd_set *)

/* Define to the type of arg5 for select(). */
#define SELECT_TYPE_ARG5 (struct timeval *)

/* Define if the setvbuf function takes the buffering type as its second argument and the buffer pointer as the third, as on
 * System V before release 3.  */
#undef SETVBUF_REVERSED

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
#undef size_t

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS

/* Define if the X Window System is missing or not being used.  */
#undef X_DISPLAY_MISSING

/* Define if lex declares yytext as a char * by default, not a char[].  */
#undef YYTEXT_POINTER

/* Define if you have the atexit function.  */
#undef HAVE_ATEXIT

/* Define if you have the div function.  */
#define HAVE_DIV 1

/* Define if you have the gethostname function.  */
#define HAVE_GETHOSTNAME 1

/* Define if you have the gettimeofday function.  */
/* Fab 8-DEC-1998, VMS : from time.h :
** Note that gettimeofday() is UTC based, so it is not visible if
** compiling in a local time based compatibility mode.
*/
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the memcpy function.  */
#define HAVE_MEMCPY 1

/* Define if you have the memmove function.  */
#define HAVE_MEMMOVE 1

/* Define if you have the mkfifo function.  */
#undef HAVE_MKFIFO

/* Define if you have the on_exit function.  */
#undef HAVE_ON_EXIT

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the setvbuf function.  */
#define HAVE_SETVBUF 1

/* Define if you have the sigaction function.  */
#undef HAVE_SIGACTION

/* Define if you have the siginterrupt function.  */
#undef HAVE_SIGINTERRUPT

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strchr function.  */
#define HAVE_STRCHR 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strncasecmp function.  */
#define HAVE_STRNCASECMP 1

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the strtol function.  */
#define HAVE_STRTOL 1

/* Define if you have the sysconf function.  */
#undef HAVE_SYSCONF

/* Define if you have the uname function.  */
#define HAVE_UNAME 1

/* Define if you have the usleep function.  */
#define HAVE_USLEEP 1

/* Define if you have the vfprintf function.  */
#define HAVE_VFPRINTF 1

/* Define if you have the wait3 function.  */
#define HAVE_WAIT3 1

/* Define if you have the wait4 function.  */
#define HAVE_WAIT4 1

/* Define if you have the waitpid function.  */
#define HAVE_WAITPID 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <getopt.h> header file.  */
#undef HAVE_GETOPT_H

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <malloc.h> header file.  */
#define HAVE_MALLOC_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/select.h> header file.  */
#undef HAVE_SYS_SELECT_H

/* Define if you have the <sys/socket.h> header file.  */
#undef HAVE_SYS_SOCKET_H

/* Define if you have the <sys/systeminfo.h> header file.  */
#undef HAVE_SYS_SYSTEMINFO_H

/* Define if you have the <sys/time.h> header file.  */
#undef HAVE_SYS_TIME_H

/* Define if you have the <sys/types.h> header file.  */
#undef HAVE_SYS_TYPES_H

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

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

