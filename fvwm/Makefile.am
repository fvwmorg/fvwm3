## Process this file with automake to create Makefile.in

configdir = @FVWM_CONFIGDIR@
bin_PROGRAMS = fvwm2

EXTRA_DIST = $(man_MANS) $(config_DATA)
man_MANS = fvwm2.1

config_DATA = ConfigFvwmDefaults ConfigFvwmSetup

fvwm2_SOURCES = add_window.c bindings.h bindings.c borders.c builtins.c	     \
	colormaps.c colors.c decorations.c defaults.h events.c focus.c	     \
	functions.c functions.h fvwm.c fvwm.h fvwmdebug.c fvwmdebug.h	     \
	gnome.c gsfr.h icccm2.c icons.c icons.h menus.c menus.h misc.c	     \
	misc.h modconf.c module.c module.h move.c parse.h placement.c read.c \
	repeat.h repeat.c resize.c screen.h session.c session.h style.c	     \
	style.h virtual.c vpacket.h windowlist.c

fvwm2_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)

AM_CFLAGS = -DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\"     \
	-DFVWM_CONFIGDIR=\"$(FVWM_CONFIGDIR)\"


