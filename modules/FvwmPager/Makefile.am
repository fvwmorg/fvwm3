## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmPager
man_MANS = FvwmPager.1
EXTRA_DIST = $(man_MANS) ConfigFvwmPager

config_DATA = \
  ConfigFvwmPager

FvwmPager_SOURCES = FvwmPager.c FvwmPager.h x_pager.c
FvwmPager_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) $(png_LIBS) \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) -lm \
	$(Xft_LIBS) $(FT2_LIBS) $(Bidi_LIBS)

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(png_CFLAGS) $(X_CFLAGS) \
	$(FT2_CFLAGS) $(Bidi_CFLAGS)

