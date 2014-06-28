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
LDADD = -L$(top_builddir)/libs -lfvwm $(Xft_LIBS) $(X_LIBS) $(xpm_LIBS) \
	$(png_LIBS) $(rsvg_LIBS) $(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 \
	$(X_EXTRA_LIBS) -lm $(Xrender_LIBS) $(Xcursor_LIBS) $(iconv_LIBS) \
	$(Bidi_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(png_CFLAGS) $(Xft_CFLAGS) \
	$(X_CFLAGS) $(Xrender_CFLAGS) $(iconv_CFLAGS) $(Bidi_CFLAGS)

