## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmWinList
man_MANS = FvwmWinList.1
EXTRA_DIST = $(man_MANS) CHANGELOG config.sample

FvwmWinList_SOURCES = ButtonArray.c ButtonArray.h Colors.c Colors.h \
    FvwmWinList.c FvwmWinList.h List.c List.h \
		    Mallocs.c Mallocs.h
FvwmWinList_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS)  \
    $(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)

