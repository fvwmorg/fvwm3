## Process this file with automake to create Makefile.in

configdir = @FVWM_DATADIR@
bin_PROGRAMS = fvwm3

EXTRA_DIST = $(config_DATA)

config_DATA = ConfigFvwmDefaults

## please keep .c file in largest first order to speed up parallel makes
fvwm3_SOURCES = \
	add_window.h bindings.h borders.h builtins.h colormaps.h colorset.h \
	commands.h conditional.h condrc.h cursor.h decorations.h events.h \
	eventhandler.h eventmask.h ewmh.h ewmh_intern.h expand.h externs.h \
	focus.h functable.h functions.h fvwm3.h geometry.h focus_policy.h \
	icccm2.h icons.h menubindings.h menudim.h menugeometry.h \
	menuitem.h menuroot.h menuparameters.h menus.h menustyle.h misc.h \
	modconf.h module_interface.h module_list.h move_resize.h \
	placement.h read.h repeat.h execcontext.h schedule.h screen.h \
	session.h stack.h style.h update.h virtual.h window_flags.h frame.h \
	infostore.h \
	\
	menus.c style.c borders.c events.c move_resize.c builtins.c \
	add_window.c icons.c fvwm3.c frame.c placement.c virtual.c \
	menustyle.c conditional.c ewmh.c stack.c session.c colorset.c \
	functions.c ewmh_events.c geometry.c module_list.c focus.c \
	windowlist.c functable.c menuitem.c expand.c module_interface.c \
	menubindings.c decorations.c ewmh_icons.c update.c bindings.c misc.c \
	cursor.c colormaps.c modconf.c  ewmh_conf.c read.c schedule.c \
	menucmd.c ewmh_names.c icccm2.c windowshade.c focus_policy.c repeat.c \
	execcontext.c menugeometry.c menudim.c condrc.c infostore.c

fvwm3_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

LDADD = \
	-L$(top_builddir)/libs -lfvwm3 $(Xft_LIBS) $(X_LIBS) $(xpm_LIBS) \
	$(stroke_LIBS) $(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 \
	$(X_EXTRA_LIBS) -lm $(iconv_LIBS) $(Xrender_LIBS) $(Xcursor_LIBS) \
	$(Bidi_LIBS) $(png_LIBS) $(rsvg_LIBS) $(intl_LIBS)

AM_CPPFLAGS = \
	-I$(top_srcdir) $(stroke_CFLAGS) $(Xft_CFLAGS) \
	$(xpm_CFLAGS) $(X_CFLAGS) $(iconv_CFLAGS) $(Xrender_CFLAGS) \
	$(Bidi_CFLAGS) $(png_CFLAGS) $(rsvg_CFLAGS) $(intl_CFLAGS)

AM_CFLAGS = \
	-DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\" \
	-DFVWM_DATADIR=\"$(FVWM_DATADIR)\" \
	-DFVWM_CONFDIR=\"$(FVWM_CONFDIR)\" \
	-DFVWM_COLORSET_PRIVATE=1 \
	-DLOCALEDIR=\"$(LOCALEDIR)\"
