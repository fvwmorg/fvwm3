## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_CONFIGDIR@

module_PROGRAMS = FvwmBacker
man_MANS = FvwmBacker.1
EXTRA_DIST = $(man_MANS) FvwmBacker.README ConfigFvwmBacker

config_DATA = \
  ConfigFvwmBacker

FvwmBacker_SOURCES = FvwmBacker.c FvwmBacker.h Mallocs.c Mallocs.h root_bits.c
FvwmBacker_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS)  \
    $(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

