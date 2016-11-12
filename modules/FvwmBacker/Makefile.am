## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmBacker
man_MANS = FvwmBacker.1
EXTRA_DIST = $(man_MANS) ConfigFvwmBacker

config_DATA = \
  ConfigFvwmBacker

FvwmBacker_SOURCES = FvwmBacker.c FvwmBacker.h root_bits.c
FvwmBacker_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm3 \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS) \
	$(Xrender_LIBS) $(rsvg_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(X_CFLAGS) $(Xrender_CFLAGS)

