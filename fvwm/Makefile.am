## Process this file with automake to create Makefile.in

bin_PROGRAMS = fvwm2

EXTRA_DIST = $(man_MANS)
man_MANS = fvwm2.1

fvwm2_SOURCES = add_window.c bindings.c borders.c \
builtins.c colormaps.c colors.c complex.c decorations.c defaults.h \
events.c focus.c functions.c fvwm.c fvwm.h fvwmdebug.c fvwmdebug.h icons.c \
menus.c menus.h misc.c misc.h modconf.c module.c module.h move.c \
parse.h placement.c read.c resize.c screen.h style.c \
virtual.c windows.c

fvwm2_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)

# These three files hardcode one or more of the following:
# FVWMRC         = filename suffix for read command
# FVWM_MODULEDIR = default location to find modules
# FVWM_CONFIGDIR = fallback location for reading config files (used
#                  only if file not found in home directory)
#
# this used to work, but no longer (?)
#builtins.o fvwm.o read.o:
#	$(COMPILE) -c $< \
#		-DFVWMRC=\".fvwm2rc\" \
#		-DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\" \
#		-DFVWM_CONFIGDIR=\"$(FVWM_CONFIGDIR)\"

builtins.o: builtins.c
	$(COMPILE) -c $< \
		-DFVWMRC=\".fvwm2rc\" \
		-DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\" \
		-DFVWM_CONFIGDIR=\"$(FVWM_CONFIGDIR)\"

fvwm.o: fvwm.c
	$(COMPILE) -c $< \
		-DFVWMRC=\".fvwm2rc\" \
		-DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\" \
		-DFVWM_CONFIGDIR=\"$(FVWM_CONFIGDIR)\"

read.o: read.c
	$(COMPILE) -c $< \
		-DFVWMRC=\".fvwm2rc\" \
		-DFVWM_MODULEDIR=\"$(FVWM_MODULEDIR)\" \
		-DFVWM_CONFIGDIR=\"$(FVWM_CONFIGDIR)\"
