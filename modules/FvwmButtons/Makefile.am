## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmButtons
man_MANS = FvwmButtons.1
EXTRA_DIST = $(man_MANS) CHANGES samplebuttonrc BUGS ConfigFvwmButtons

config_DATA = ConfigFvwmButtons

FvwmButtons_SOURCES = \
	FvwmButtons.c FvwmButtons.h button.c button.h draw.c draw.h \
	icons.c icons.h misc.c misc.h output.c parse.c parse.h \
	dynamic.c dynamic.h

FvwmButtons_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs -lfvwm3 $(Xft_LIBS) $(X_LIBS) $(xpm_LIBS)  \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS) \
	$(iconv_LIBS) $(Xrender_LIBS) $(Xcursor_LIBS) $(Bidi_LIBS) \
	$(png_LIBS) $(rsvg_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(Xft_CFLAGS) $(X_CFLAGS) \
	$(iconv_CFLAGS) $(Xrender_CFLAGS) $(fribidi_CFLAGS) $(png_CFLAGS)

