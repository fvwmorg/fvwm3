## Process this file with automake to create Makefile.in

configdir = @FVWM_CONFIGDIR@
bin_PROGRAMS = fvwm2

EXTRA_DIST = $(man_MANS) $(config_DATA)
man_MANS = fvwm2.1

config_DATA = ConfigFvwmDefaults ConfigFvwmSetup

## .c file in largest first order to speed up parallel makes
fvwm2_SOURCES = bindings.h focus.h menus.h screen.h borders.h functions.h \
		misc.h session.h colormaps.h fvwm.h module_interface.h \
		stack.h cursor.h fvwmdebug.h move_resize.h style.h \
		gnome.h read.h virtual.h events.h icons.h repeat.h \
		conditional.h placement.h windowlist.h modconf.h add_window.h \
		colors.h builtins.h decorations.h icccm2.h \
		window_flags.h menus.c builtins.c events.c fvwm.c \
		move_resize.c borders.c style.c add_window.c icons.c \
		module_interface.c virtual.c functions.c session.c misc.c \
		placement.c gnome.c decorations.c stack.c colors.c \
		windowlist.c focus.c bindings.c colormaps.c cursor.c icccm2.c \
		modconf.c read.c fvwmdebug.c repeat.c conditional.c \
		geometry.c geometry.h update.c update.h

fvwm2_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) $(stroke_LIBS)\
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) -lm

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libs $(xpm_CFLAGS) $(stroke_CFLAGS)\
	   $(X_CFLAGS)

AM_CFLAGS = -DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\"     \
	-DFVWM_CONFIGDIR=\"$(FVWM_CONFIGDIR)\"


