## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmTaskBar

FvwmTaskBar_SOURCES =  FvwmTaskBar.c List.c ButtonArray.c Mallocs.c	\
	Colors.c Start.c FvwmTaskBar.h ButtonArray.h List.h		\
	Colors.h Mallocs.h Start.h Goodies.c Goodies.h

FvwmTaskBar_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmTaskBar.1
EXTRA_DIST = $(man_MANS) minimail.xbm ConfigFvwmTaskBar

config_DATA = \
  ConfigFvwmTaskBar

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm -lm $(xpm_LIBS) \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) \
	$(Xft_LIBS) $(FT2_LIBS) $(Bidi_LIBS)

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS) \
	$(FT2_CFLAGS) $(Bidi_CFLAGS)

