## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmRearrange

## These modules were made obsolete in November, 1998.
## Remove from fvwm distribution no later than October, 1999.
module_SCRIPTS = FvwmCascade FvwmTile

FvwmCascade:
	echo "#!/bin/sh" > $@
	echo 'modargs="$$1 $$2 $$3 $$4 $$5"' >> $@
	echo 'shift;shift;shift;shift;shift' >> $@
	echo exec ${moduledir}'/FvwmRearrange $$modargs -cascade $$@' >> $@

FvwmTile:
	echo "#!/bin/sh" > $@
	echo 'modargs="$$1 $$2 $$3 $$4 $$5"' >> $@
	echo 'shift;shift;shift;shift;shift' >> $@
	echo exec ${moduledir}'/FvwmRearrange $$modargs -tile $$@' >> $@

EXTRA_DIST = $(man_MANS)
man_MANS = FvwmRearrange.1
CLEANFILES = $(module_SCRIPTS)

FvwmRearrange_SOURCES = FvwmRearrange.c
FvwmRearrange_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS)  \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

