## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmRearrange 

## We use _SCRIPTS because it is the easiest way to get them installed with the
## executable bit set.  Using _PROGRAMS ran into trouble because automake
## insisted on creating compile & link rules for them.
module_SCRIPTS = FvwmCascade FvwmTile
FvwmCascade FvwmTile: FvwmRearrange
	$(LN_S) $< $@

man_MANS = FvwmRearrange.1 FvwmTile.1 FvwmCascade.1
FvwmTile.1 FvwmCascade.1: FvwmRearrange.1
	$(LN_S) $< $@

EXTRA_DIST = FvwmRearrange.1

FvwmRearrange_SOURCES = FvwmRearrange.c
#FvwmCascade_SOURCES =
#FvwmTile_SOURCES =

FvwmRearrange_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a
#FvwmCascade_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a
#FvwmTile_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS)  \
    $(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir)/libs $(X_CFLAGS) $(xpm_CFLAGS)

