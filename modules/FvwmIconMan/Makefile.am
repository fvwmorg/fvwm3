## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmIconMan
EXTRA_DIST = ConfigFvwmIconMan meson.build

config_DATA = \
  ConfigFvwmIconMan

FvwmIconMan_SOURCES = FvwmIconMan.c FvwmIconMan.h debug.c debug.h \
	functions.c fvwm.c globals.c readconfig.c readconfig.h \
	winlist.c x.c x.h xmanager.c xmanager.h

FvwmIconMan_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs -lfvwm3 $(Xft_LIBS) $(X_LIBS) \
	$(X_PRE_LIBS) $(XRandR_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) \
	$(iconv_LIBS) $(Xrender_LIBS) $(rsvg_LIBS) -lm $(Bidi_LIBS)

AM_CPPFLAGS = -I$(top_srcdir)  $(Xft_CFLAGS) $(X_CFLAGS) \
	$(iconv_CFLAGS) $(Xrender_CFLAGS) $(Bidi_CFLAGS)

