## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmBacker
man_MANS = FvwmBacker.1
EXTRA_DIST = $(man_MANS) ConfigFvwmBacker

config_DATA = \
  ConfigFvwmBacker

FvwmBacker_SOURCES = FvwmBacker.c FvwmBacker.h root_bits.c
FvwmBacker_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm \
	$(X_PRE_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir) $(X_CFLAGS)

