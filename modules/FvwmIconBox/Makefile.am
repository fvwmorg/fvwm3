## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmIconBox
man_MANS = FvwmIconBox.1
EXTRA_DIST = $(man_MANS) CHANGES COPYRIGHT TO-DO ConfigFvwmIconBox

config_DATA = \
  ConfigFvwmIconBox

FvwmIconBox_SOURCES = FvwmIconBox.c FvwmIconBox.h icons.c
FvwmIconBox_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs -lfvwm $(Xft_LIBS) $(X_LIBS) $(xpm_LIBS)  \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS) \
	$(iconv_LIBS) $(Xrender_LIBS) $(Bidi_LIBS) $(png_LIBS)

INCLUDES = -I$(top_srcdir) $(Xft_CFLAGS) $(xpm_CFLAGS) $(X_CFLAGS) \
	$(iconv_CFLAGS) $(Xrender_CFLAGS) $(Bidi_CFLAGS) $(png_CFLAGS)

