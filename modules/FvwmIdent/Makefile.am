## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_CONFIGDIR@

module_PROGRAMS = FvwmIdent
man_MANS = FvwmIdent.1
EXTRA_DIST = $(man_MANS) ConfigFvwmIdent

config_DATA = \
  ConfigFvwmIdent

FvwmIdent_SOURCES = FvwmIdent.c FvwmIdent.h
FvwmIdent_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS)  \
	$(X_PRE_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

