## Process this file with automake to create Makefile.in

bin_PROGRAMS = fvwm2

EXTRA_DIST = $(man_MANS)
man_MANS = fvwm2.1

fvwm2_SOURCES = add_window.c alpha_header.h bindings.c borders.c \
builtins.c colormaps.c colors.c complex.c decorations.c \
events.c focus.c functions.c fvwm.c fvwm.h icons.c \
menus.c menus.h misc.c misc.h modconf.c module.c module.h move.c \
parse.h placement.c read.c resize.c screen.h style.c \
sun_headers.h virtual.c windows.c 

LDADD = @LIBOBJS@ -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir)/libs $(X_CFLAGS) $(xpm_CFLAGS)

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
