/* -*-c-*- */

#ifndef FVWMLIB_H
#define FVWMLIB_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>              /* needed for xpm.h and Pixel defn */
#include <ctype.h>

#include "fvwmrect.h"
#include "safemalloc.h"

/* Convenience function ti init all the graphics subsystems */
void flib_init_graphics(Display *dpy);

/*
 * Replacements for missing system calls.
 */

#ifndef HAVE_ATEXIT
int atexit(void(*func)());
#endif

#ifndef HAVE_GETHOSTNAME
int gethostname(char* name, int len);
#endif

#ifndef HAVE_STRCASECMP
int strcasecmp(char* s1, char* s2);
#endif

#ifndef HAVE_STRNCASECMP
int strncasecmp(char* s1, char* s2, int len);
#endif

#ifndef HAVE_STRERROR
char* strerror(int errNum);
#endif

#ifndef HAVE_USLEEP
int usleep(unsigned long usec);
#endif

/* Set up heap debugging library dmalloc.  */
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

/* Set up mtrace from glibc 2.1.x for x > ?  */
#ifdef MTRACE_DEBUGGING
#include <mcheck.h>
#endif

#endif
