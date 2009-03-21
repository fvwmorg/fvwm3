## Process this file with automake to create Makefile.in

## Notes: Imakefile also linked with -ldnet_stub, on AlphaArchitecture

moduledir = @FVWM_MODULEDIR@
module_SCRIPTS = FvwmDebug FvwmGtkDebug

man_MANS = FvwmDebug.1 FvwmGtkDebug.1
EXTRA_DIST = $(man_MANS)

# all the following lines may be removed one day
EXTRA_PROGRAMS = FvwmOldDebug
FvwmOldDebug_SOURCES = FvwmOldDebug.c FvwmOldDebug.h
FvwmOldDebug_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs -lfvwm

# FIXME:
# Despite not using X functions explicitly, the code includes
# fvwmlib.h, which *does* include X headers and xpm.h!
INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(Xft_CFLAGS) $(X_CFLAGS)

_FvwmDebug.1: FvwmDebug.in
        pod2man --center "Fvwm Modules" --name FvwmDebug FvwmDebug >FvwmDebug.1

_FvwmGtkDebug.1: FvwmGtkDebug.in
        pod2man --center "Fvwm Modules" --name FvwmGtkDebug FvwmGtkDebug >FvwmGtkDebug.1

regenerate: _FvwmDebug.1 _FvwmGtkDebug.1
