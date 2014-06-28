## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmScroll
man_MANS = FvwmScroll.1
EXTRA_DIST = $(man_MANS) ConfigFvwmScroll

config_DATA = \
  ConfigFvwmScroll

FvwmScroll_SOURCES = FvwmScroll.c FvwmScroll.h GrabWindow.c
FvwmScroll_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS) \
	$(Xrender_LIBS) $(rsvg_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(X_CFLAGS) $(Xrender_CFLAGS)
