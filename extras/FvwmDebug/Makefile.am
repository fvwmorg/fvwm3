## Process this file with automake to create Makefile.in

## Notes: Imakefile also linked with -ldnet_stub, on AlphaArchitecture

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmDebug
FvwmDebug_SOURCES = FvwmDebug.c FvwmDebug.h
FvwmDebug_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmDebug.1
EXTRA_DIST = $(man_MANS)

LDADD = -L$(top_builddir)/libs -lfvwm

# FIXME:
# Despite not using X functions explicitly, the code includes
# fvwmlib.h, which *does* include X headers and xpm.h!
INCLUDES = -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)
