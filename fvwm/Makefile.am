## Process this file with automake to create Makefile.in

configdir = @FVWM_CONFIGDIR@
bin_PROGRAMS = fvwm2

EXTRA_DIST = $(man_MANS) $(config_DATA)
man_MANS = fvwm2.1

config_DATA = ConfigFvwmDefaults ConfigFvwmSetup

## please keep .c file in largest first order to speed up parallel makes
fvwm2_SOURCES = add_window.h bindings.h borders.h builtins.h colormaps.h \
		colors.h conditional.h cursor.h decorations.h events.h \
		externs.h focus.h functions.h fvwm.h fvwmdebug.h geometry.h \
		gnome.h icccm2.h icons.h menus.h misc.h modconf.h \
		module_interface.h move_resize.h placement.h read.h \
		repeat.h screen.h session.h stack.h style.h update.h \
		virtual.h window_flags.h windowlist.h \
		menus.c move_resize.c builtins.c style.c events.c \
		add_window.c fvwm.c borders.c icons.c module_interface.c \
		functions.c virtual.c session.c gnome.c placement.c \
		stack.c conditional.c decorations.c cursor.c windowlist.c \
		geometry.c misc.c focus.c colormaps.c bindings.c update.c \
		icccm2.c modconf.c read.c fvwmdebug.c repeat.c colors.c

fvwm2_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) $(stroke_LIBS)\
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) -lm

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libs $(xpm_CFLAGS) $(stroke_CFLAGS)\
	   $(X_CFLAGS)

AM_CFLAGS = -DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\"     \
	-DFVWM_CONFIGDIR=\"$(FVWM_CONFIGDIR)\"


