## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmIdent
man_MANS = FvwmIdent.1
EXTRA_DIST = $(man_MANS) ConfigFvwmIdent

config_DATA = \
  ConfigFvwmIdent

FvwmIdent_SOURCES = FvwmIdent.c FvwmIdent.h
FvwmIdent_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs -lfvwm3 $(Xft_LIBS) $(X_LIBS) $(xpm_LIBS) \
	$(png_LIBS) $(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 -lm \
	$(X_EXTRA_LIBS) $(Xrender_LIBS) $(rsvg_LIBS) $(iconv_LIBS) $(Bidi_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(Xft_CFLAGS) $(xpm_CFLAGS) $(png_CFLAGS) \
	$(X_CFLAGS) $(Xrender_CFLAGS) $(iconv_CFLAGS) $(Bidi_CFLAGS)

