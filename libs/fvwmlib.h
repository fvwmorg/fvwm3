/* -*-c-*- */

#ifndef FVWMLIB_H
#define FVWMLIB_H

#include <ctype.h>

#include "fvwm_x11.h"
#include "fvwmrect.h"
#include "safemalloc.h"

/* Convenience function ti init all the graphics subsystems */
void
flib_init_graphics(Display *dpy);

/* Set up heap debugging library dmalloc.  */
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

/* Set up mtrace from glibc 2.1.x for x > ?  */
#ifdef MTRACE_DEBUGGING
#include <mcheck.h>
#endif

#endif
