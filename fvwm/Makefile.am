## Process this file with automake to create Makefile.in

configdir = @FVWM_DATADIR@
bin_PROGRAMS = fvwm2

EXTRA_DIST = $(man_MANS) $(config_DATA)
man_MANS = fvwm2.1

config_DATA = ConfigFvwmDefaults ConfigFvwmSetup

## please keep .c file in largest first order to speed up parallel makes
fvwm2_SOURCES = add_window.h bindings.h borders.h builtins.h colormaps.h \
		colors.h commands.h conditional.h cursor.h decorations.h \
		events.h ewmh.h ewmh_intern.h \
		externs.h focus.h functions.h fvwm.h fvwmdebug.h geometry.h \
		gnome.h icccm2.h icons.h menus.h misc.h modconf.h \
		module_interface.h move_resize.h placement.h read.h \
		repeat.h screen.h session.h stack.h style.h update.h \
		virtual.h window_flags.h windowlist.h \
		menus.c borders.c placement.c focus.c read.c \
		style.c module_interface.c stack.c misc.c fvwmdebug.c \
		events.c icons.c conditional.c colormaps.c repeat.c \
		move_resize.c functions.c decorations.c bindings.c colors.c \
		builtins.c virtual.c cursor.c update.c \
		add_window.c session.c windowlist.c modconf.c \
		fvwm.c gnome.c geometry.c icccm2.c ewmh.c ewmh_events.c \
		ewmh_icons.c ewmh_names.c ewmh_conf.c schedule.c schedule.h

fvwm2_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) $(stroke_LIBS)\
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) -lm \
	$(iconv_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libs $(xpm_CFLAGS) $(stroke_CFLAGS)\
	$(X_CFLAGS) $(iconv_CFLAGS)

AM_CFLAGS = \
	-DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\" \
	-DFVWM_DATADIR=\"$(FVWM_DATADIR)\" \
	-DFVWM_CONFDIR=\"$(FVWM_CONFDIR)\"

