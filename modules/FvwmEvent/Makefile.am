## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmEvent

## This module was made obsolete in May, 1999.
## Remove from fvwm distribution no later than May, 2000.
module_SCRIPTS = FvwmAudio

FvwmAudio: FORCE
	echo "#!/bin/sh" > $@
	echo echo Obsolete module '$$0', replace with FvwmEvent. >> $@
	echo exec ${moduledir}/FvwmRearrange FvwmAudio '$$@' >> $@

# do nothing, just force the above rules that depend on it...
FORCE:

CLEANFILES = $(module_SCRIPTS)
man_MANS = FvwmEvent.1
EXTRA_DIST = $(man_MANS)

FvwmEvent_SOURCES = FvwmEvent.c
FvwmEvent_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) $(rplay_LIBS) \
    $(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(rplay_CFLAGS) $(X_CFLAGS)
