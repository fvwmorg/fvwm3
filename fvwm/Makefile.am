## Process this file with automake to create Makefile.in

configdir = @FVWM_CONFIGDIR@
bin_PROGRAMS = fvwm2

EXTRA_DIST = $(man_MANS) $(config_DATA)
man_MANS = fvwm2.1

config_DATA = ConfigFvwmDefaults ConfigFvwmSetup

fvwm2_SOURCES = add_window.c bindings.h bindings.c borders.c \
	builtins.c \
	colormaps.c colors.c decorations.c defaults.h events.c events.h \
	focus.c focus.h \
	functions.c functions.h fvwm.c fvwm.h fvwmdebug.c fvwmdebug.h \
	gnome.c gsfr.h icccm2.c icons.c icons.h menus.c menus.h misc.c \
	misc.h modconf.c module_interface.c module_interface.h \
	move_resize.c move_resize.h \
	parse.h placement.c read.c read.h repeat.h repeat.c screen.h \
	session.c session.h stack.c stack.h \
	style.c	style.h virtual.c windowlist.c

fvwm2_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)

AM_CFLAGS = -DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\"     \
	-DFVWM_CONFIGDIR=\"$(FVWM_CONFIGDIR)\"


