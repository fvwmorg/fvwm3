## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@

module_PROGRAMS = FvwmBanner

man_MANS = FvwmBanner.1
EXTRA_DIST = $(man_MANS) fvwm-logo-current.xpm fvwm-logo-current.xbm

FvwmBanner_SOURCES = FvwmBanner.c
FvwmBanner_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a fvwm-logo-current.xpm

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm3 -lm $(xpm_LIBS)  \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) \
	$(png_LIBS) $(rsvg_LIBS) $(Xrender_LIBS) $(Xcursor_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS) $(png_CFLAGS) \
	$(Xrender_CFLAGS)

