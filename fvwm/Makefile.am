## Process this file with automake to create Makefile.in

configdir = @FVWM_DATADIR@
bin_PROGRAMS = fvwm

EXTRA_DIST = $(man_MANS) $(config_DATA)
man_MANS = fvwm.1

config_DATA = ConfigFvwmDefaults ConfigFvwmSetup

## please keep .c file in largest first order to speed up parallel makes
fvwm_SOURCES = \
	add_window.h bindings.h borders.h builtins.h colormaps.h colors.h \
	commands.h conditional.h cursor.h decorations.h events.h ewmh.h \
	ewmh_intern.h externs.h focus.h functions.h fvwm.h geometry.h \
	gnome.h icccm2.h icons.h menuitem.h menus.h menustyle.h menudim.h \
	misc.h modconf.h module_interface.h move_resize.h placement.h \
	read.h repeat.h schedule.h screen.h session.h stack.h style.h \
	update.h virtual.h window_flags.h windowlist.h frame.h colorset.h \
	menus.c colorset.c style.c borders.c move_resize.c builtins.c \
	events.c add_window.c module_interface.c icons.c stack.c functions.c \
	virtual.c session.c fvwm.c placement.c focus.c read.c misc.c \
	conditional.c colormaps.c repeat.c decorations.c bindings.c colors.c \
	cursor.c update.c windowlist.c modconf.c gnome.c geometry.c icccm2.c \
	ewmh.c ewmh_events.c ewmh_icons.c ewmh_names.c ewmh_conf.c schedule.c \
	menuitem.c menustyle.c menudim.c menucmd.c windowshade.c frame.c

fvwm_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) $(stroke_LIBS) \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) -lm \
	$(iconv_LIBS) $(Xft_LIBS) $(FT2_LIBS) $(Bidi_LIBS) $(png_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libs $(xpm_CFLAGS) $(stroke_CFLAGS) \
	$(X_CFLAGS) $(iconv_CFLAGS) $(Xft_CFLAGS) $(FT2_CFLAGS) \
	$(Bidi_CFLAGS) $(png_CFLAGS)

AM_CFLAGS = \
	-DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\" \
	-DFVWM_DATADIR=\"$(FVWM_DATADIR)\" \
	-DFVWM_CONFDIR=\"$(FVWM_CONFDIR)\"

## Create compatibility symlinks if available

install-exec-local:
	@rm -f $(bindir)/fvwm2
	@$(LN_S) fvwm $(bindir)/fvwm2 || \
		echo "Minor warning: $(bindir)/fvwm2 symlink was not created"

install-data-local:
	@rm -f $(mandir)/man1/fvwm2.1
	@$(LN_S) fvwm.1 $(mandir)/man1/fvwm2.1 || \
		echo "Minor warning: $(mandir)/man1/fvwm2.1 symlink was not created"
