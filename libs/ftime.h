/* -*-c-*- */

#ifndef FVWMLIB_FTIME_H
#define FVWMLIB_FTIME_H

#include "config.h"

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#endif /* FVWMLIB_FTIME_H */
