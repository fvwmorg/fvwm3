## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmRearrange

## These modules were made obsolete in November, 1998.
## Remove from fvwm distribution no later than October, 1999.
module_SCRIPTS = FvwmCascade FvwmTile

FvwmCascade: ../../config.h Makefile
	echo "#!/bin/sh" > $@
	echo 'modargs="$$1 $$2 $$3 $$4 $$5"' >> $@
	echo 'shift;shift;shift;shift;shift' >> $@
	echo exec ${moduledir}'/FvwmRearrange $$modargs -cascade $$@' >> $@

FvwmTile: ../../config.h Makefile
	echo "#!/bin/sh" > $@
	echo 'modargs="$$1 $$2 $$3 $$4 $$5"' >> $@
	echo 'shift;shift;shift;shift;shift' >> $@
	echo exec ${moduledir}'/FvwmRearrange $$modargs -tile $$@' >> $@

EXTRA_DIST = $(man_MANS)
man_MANS = FvwmRearrange.1
CLEANFILES = $(module_SCRIPTS)

FvwmRearrange_SOURCES = FvwmRearrange.c
FvwmRearrange_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lm -lfvwm3  \
	$(X_PRE_LIBS) $(XRandR_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(Xft_CFLAGS) $(X_CFLAGS) $(XRandR_CFLAGS)

