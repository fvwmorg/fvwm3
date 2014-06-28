## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmCpp
man_MANS = FvwmCpp.1
EXTRA_DIST = $(man_MANS)

FvwmCpp_SOURCES = FvwmCpp.c FvwmCpp.h
FvwmCpp_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm -lm  \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(X_CFLAGS)
AM_CFLAGS = -DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\"	\
	-DFVWM_DATADIR=\"$(FVWM_DATADIR)\"		\
	-DFVWM_CPP='"$(FVWM_CPP)"'
