## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@

module_PROGRAMS = @FVWMGTK@

EXTRA_PROGRAMS = FvwmGtk

man_MANS = FvwmGtk.1
EXTRA_DIST = $(man_MANS)

GTK_CFLAGS = @GTK_CFLAGS@
GTK_CONFIG = @GTK_CONFIG@
GTK_LIBS = @GTK_LIBS@

FvwmGtk_SOURCES = FvwmGtk.c gtkpixmapmenuitem.c gtkpixmapmenuitem.h
FvwmGtk_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
    $(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) -lm \
    $(GTK_LIBS)

INCLUDES = -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS) $(GTK_CFLAGS)
