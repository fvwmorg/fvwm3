## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmWinList
man_MANS = FvwmWinList.1
EXTRA_DIST = $(man_MANS) config.sample ConfigFvwmWinList

config_DATA = \
  ConfigFvwmWinList

FvwmWinList_SOURCES = ButtonArray.c ButtonArray.h \
    FvwmWinList.c FvwmWinList.h List.c List.h \
		    Mallocs.c Mallocs.h
FvwmWinList_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS) \
	$(Xft_LIBS) $(FT2_LIBS) $(Bidi_LIBS)

INCLUDES = -I$(top_srcdir) $(X_CFLAGS) $(FT2_CFLAGS) $(Bidi_CFLAGS)

