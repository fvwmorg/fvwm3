## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@

module_PROGRAMS = @FVWMGTK@
EXTRA_PROGRAMS = FvwmGtk

# DV: For some reason, using man_MANS does not work here: automake does not
# generate an install rule for man pages unless one of them is an existing file
# name.  Use a hand made rule to install the man page instead.
#man_MANS = @MANFVWMGTK@
man_onedir = $(mandir)/man1
man_one_DATA = @MANFVWMGTK@
EXTRA_DIST = FvwmGtk.1

GTK_CFLAGS = @GTK_CFLAGS@
GTK_LIBS = @GTK_LIBS@
GDK_IMLIB_CFLAGS = @GDK_IMLIB_CFLAGS@
GDK_IMLIB_LIBS = @GDK_IMLIB_LIBS@
GNOME_INCLUDEDIR = @GNOME_INCLUDEDIR@
GNOME_LIBDIR = @GNOME_LIBDIR@
GNOMEUI_LIBS = @GNOMEUI_LIBS@

FvwmGtk_SOURCES = FvwmGtk.c gtkpixmapmenuitem.c gtkpixmapmenuitem.h \
	dialog.c dialog.h expand.c expand.h windowlist.c windowlist.h \
	menu.c menu.h
FvwmGtk_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) -lm \
	$(GTK_LIBS) $(GDK_IMLIB_LIBS) \
	$(GNOME_LIBDIR) $(GNOMEUI_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(Xft_CFLAGS) $(X_CFLAGS) \
	$(GTK_CFLAGS) $(GDK_IMLIB_CFLAGS) $(GNOME_INCLUDEDIR)

clean-local:
	rm -f FvwmGtk
