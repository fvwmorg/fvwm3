## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmIconMan
man_MANS = FvwmIconMan.1
EXTRA_DIST = $(man_MANS) ConfigFvwmIconMan

config_DATA = \
  ConfigFvwmIconMan

FvwmIconMan_SOURCES = FvwmIconMan.c FvwmIconMan.h debug.c debug.h \
	debuglevels.h functions.c fvwm.c globals.c readconfig.c \
	readconfig.h winlist.c x.c x.h xmanager.c xmanager.h

FvwmIconMan_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS)  \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) -lm

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

