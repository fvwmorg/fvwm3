## Process this file with automake to create Makefile.in

configdir = @FVWM_DATADIR@
bin_PROGRAMS = fvwm

EXTRA_DIST = $(config_DATA)
man_MANS = fvwm.1

config_DATA = ConfigFvwmDefaults ConfigFvwmSetup

## please keep .c file in largest first order to speed up parallel makes
fvwm_SOURCES = \
	add_window.h bindings.h borders.h builtins.h colormaps.h \
	commands.h conditional.h condrc.h cursor.h decorations.h events.h \
	eventhandler.h eventmask.h ewmh.h \
	ewmh_intern.h expand.h externs.h focus.h functable.h functions.h \
	fvwm.h geometry.h focus_policy.h \
	gnome.h icccm2.h icons.h menuitem.h menus.h menustyle.h menudim.h \
	misc.h modconf.h module_interface.h move_resize.h placement.h \
	read.h repeat.h execcontext.h schedule.h screen.h session.h stack.h \
	style.h \
	update.h virtual.h window_flags.h windowlist.h frame.h colorset.h \
	\
	menus.c style.c events.c move_resize.c add_window.c builtins.c \
	borders.c icons.c fvwm.c module_interface.c frame.c virtual.c stack.c \
	ewmh.c menustyle.c session.c colorset.c placement.c functions.c \
	gnome.c focus.c conditional.c geometry.c ewmh_events.c windowlist.c \
	menuitem.c ewmh_icons.c decorations.c update.c expand.c misc.c \
	functable.c bindings.c cursor.c colormaps.c ewmh_conf.c modconf.c \
	read.c menucmd.c ewmh_names.c icccm2.c schedule.c windowshade.c \
	focus_policy.c repeat.c execcontext.c menudim.c condrc.c

fvwm_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = \
	-L$(top_builddir)/libs -lfvwm $(Xft_LIBS) $(X_LIBS) $(xpm_LIBS) \
	$(stroke_LIBS) $(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 \
	$(X_EXTRA_LIBS) -lm $(iconv_LIBS) $(Xrender_LIBS) $(Bidi_LIBS) \
	$(png_LIBS) $(intl_LIBS)

INCLUDES = \
	-I$(top_srcdir) -I$(top_srcdir)/libs $(stroke_CFLAGS) $(Xft_CFLAGS) \
	$(xpm_CFLAGS) $(X_CFLAGS) $(iconv_CFLAGS) $(Xrender_CFLAGS) \
	$(Bidi_CFLAGS) $(png_CFLAGS) $(intl_CFLAGS)

AM_CFLAGS = \
	-DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\" \
	-DFVWM_DATADIR=\"$(FVWM_DATADIR)\" \
	-DFVWM_CONFDIR=\"$(FVWM_CONFDIR)\" \
	-DFVWM_COLORSET_PRIVATE=1 \
	-DLOCALEDIR=\"$(LOCALEDIR)\"

## Create compatibility symlinks if available

install-exec-local:
	@rm -f $(DESTDIR)$(bindir)/fvwm2
	@$(LN_S) fvwm $(DESTDIR)$(bindir)/fvwm2 || \
		echo "Minor warning: $(bindir)/fvwm2 symlink was not created"

install-data-local:
	@rm -f $(DESTDIR)$(mandir)/man1/fvwm2.1
	@$(LN_S) fvwm.1 $(DESTDIR)$(mandir)/man1/fvwm2.1 || \
	echo "Minor warning: $(mandir)/man1/fvwm2.1 symlink was not created"
