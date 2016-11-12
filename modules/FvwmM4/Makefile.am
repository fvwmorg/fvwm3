## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmM4
man_MANS = FvwmM4.1
EXTRA_DIST = $(man_MANS)

FvwmM4_SOURCES = FvwmM4.c FvwmM4.h
FvwmM4_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm3 -lm  \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(X_CFLAGS)
AM_CFLAGS = -DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\" \
	-DFVWM_DATADIR=\"$(FVWM_DATADIR)\"
