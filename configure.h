/***************************************************************************
 *                         fvwm/configure.h
 * This and Fvwm.tmpl are the only files you should have to edit to build
 * and install Fvwm2.
 ***************************************************************************/

/*************************************************************************
 * #define BROKEN_SUN_HEADERS          
 *
 * Really, no one but Rob should need this 
 ************************************************************************/
#if 0
#if defined __sun__ && !defined SYSV
#define BROKEN_SUN_HEADERS          
#endif
#endif /* 0 */

/***************************************************************************
 *
 * In theory, this stuff can be replaced with GNU Autoconf 
 *
 **************************************************************************/

#if defined _POSIX_SOURCE || defined SYSV || defined __sun__ || defined sgi

#define HAVE_WAITPID  1
#define HAVE_SYSCONF 1
#define HAVE_UNAME 1
#undef HAVE_GETHOSTNAME
#define HAVE_STRERROR 1

#else

/**************************************************************************
 *
 * Do it yourself here if you don't like the above!
 *
 **************************************************************************/
/***************************************************************************
 * Define if you have waitpid.  
 **************************************************************************/
#define HAVE_WAITPID  1

/***************************************************************************
 * Define if you have sysconf
 **************************************************************************/
#define HAVE_SYSCONF 1

/***************************************************************************
 * Define if you have uname. Otherwise, define gethostname
 ***************************************************************************/
#define HAVE_UNAME 1
/* #define HAVE_GETHOSTNAME 1 */

/***************************************************************************
 * Define if you have strerror (all but SunOS 4.1.x?)
 **************************************************************************/
#define HAVE_STRERROR 1

#endif

#if defined(__sun__) || defined(sun)
#undef HAVE_STRERROR
#endif

/* End of do-it-yourself OS support section */


/***************************************************************************
 * Please translate the strings into the language which you use for 
 * your pop-up menus.
 *
 * Some decisions about where a function is prohibited (based on 
 * mwm-function-hints) is based on a string comparison between the 
 * menu item and the strings below.
 ***************************************************************************/
#define MOVE_STRING "move"
#define RESIZE_STRING1 "size"
#define RESIZE_STRING2 "resize"
#define MINIMIZE_STRING "minimize"
#define MINIMIZE_STRING2 "iconify"
#define MAXIMIZE_STRING "maximize"
#define CLOSE_STRING1 "close"
#define CLOSE_STRING2 "delete"
#define CLOSE_STRING3 "destroy"
#define CLOSE_STRING4 "quit"

/* #ifdef __alpha */
#if defined(__alpha) && !defined(linux)
#define NEEDS_ALPHA_HEADER
#undef BROKEN_SUN_HEADERS
#endif /* (__alpha) */


/* Allows gcc users to use inline. This doesn't cause problems for others. */
#ifndef __GNUC__
#define FVWM_INLINE /*nothing*/
#else
#if defined(__GNUC__) && !defined(inline)
#define FVWM_INLINE __inline__
#else
#define FVWM_INLINE inline
#endif
#endif


/*
** if you would like to see lots of debug messages from fvwm, for debugging
** purposes, uncomment the next line
*/
/* #define FVWM_DEBUG_MSGS */
#ifdef FVWM_DEBUG_MSGS
#define DBUG(x,y) fvwm_msg(DBG,x,y)
#else
#define DBUG(x,y) /* no messages */
#endif

/* Some versions of HP-UX don't have usleep(). */
#ifndef __hpux
#define HAVE_USLEEP
#endif

/* end of configure.h */
