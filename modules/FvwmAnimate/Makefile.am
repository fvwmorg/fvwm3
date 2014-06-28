## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmAnimate
man_MANS = FvwmAnimate.1
EXTRA_DIST = $(man_MANS)

FvwmAnimate_SOURCES = FvwmAnimate.c FvwmAnimate.h
FvwmAnimate_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) $(rplay_LIBS) \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) -lm $(png_LIBS) \
	$(rsvg_LIBS) $(Xrender_LIBS) $(Xcursor_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(rplay_CFLAGS) $(X_CFLAGS) \
	$(png_CFLAGS) $(Xrender_CFLAGS)
