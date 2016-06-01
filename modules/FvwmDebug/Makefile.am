## Process this file with automake to create Makefile.in

## Notes: Imakefile also linked with -ldnet_stub, on AlphaArchitecture

moduledir = @FVWM_MODULEDIR@
module_SCRIPTS = FvwmDebug

man_MANS = FvwmDebug.1
EXTRA_DIST = $(man_MANS)

# all the following lines may be removed one day
EXTRA_PROGRAMS = FvwmOldDebug
FvwmOldDebug_SOURCES = FvwmOldDebug.c FvwmOldDebug.h
FvwmOldDebug_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs -lfvwm

# FIXME:
# Despite not using X functions explicitly, the code includes
# fvwmlib.h, which *does* include X headers and xpm.h!
AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(Xft_CFLAGS) $(X_CFLAGS)

_FvwmDebug.1: FvwmDebug
	pod2man --center "Fvwm Modules" --release "@VERSION@@VERSIONINFO@" \
		--name FvwmDebug FvwmDebug >FvwmDebug.1

regenerate: _FvwmDebug.1
