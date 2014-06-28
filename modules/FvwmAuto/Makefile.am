## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmAuto
man_MANS = FvwmAuto.1
EXTRA_DIST = $(man_MANS)

FvwmAuto_SOURCES = FvwmAuto.c
FvwmAuto_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm  \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(X_CFLAGS)

