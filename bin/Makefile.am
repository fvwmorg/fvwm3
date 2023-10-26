## Process this file with automake to create Makefile.in

SUBDIRS = FvwmPrompt

bin_PROGRAMS = fvwm-root
fvwm_root_SOURCE= fvwm-root.c
fvwm_root_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

bin_SCRIPTS = fvwm-perllib \
	fvwm-convert-2.6 \
	fvwm-menu-xlock fvwm-menu-directory \
	fvwm-menu-desktop \
	FvwmCommand

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm3 $(xpm_LIBS) $(Xcursor_LIBS) \
	$(X_PRE_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS) $(Xrender_LIBS) \
	$(Xcursor_LIBS) $(png_LIBS) $(rsvg_LIBS) $(XRandR_LIBS) \
	$(XFT_LIBS)
AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS) $(png_CFLAGS) \
	$(rsvg_CFLAGS) $(XRandR_CFLAGS) $(XFT_CFLAGS)

configdir = @FVWM_DATADIR@
config_DATA = fvwm-menu-desktop-config.fpl

EXTRA_DIST = fvwm-menu-desktop-config.fpl FvwmPrompt

dist-hook:
	rm -f $(distdir)/FvwmPrompt/FvwmPrompt

## Create compatibility symlinks if available

fvwm_root_name =`echo "fvwm-root" | "$(SED)" -e "${transform}"`
FvwmCommand_name =`echo "FvwmCommand" | "$(SED)" -e "${transform}"`

uninstall-local:
	@rm -f $(DESTDIR)$(bindir)/$(FvwmCommand_name)$(EXEEXT)

CLEANFILES = $(bin_SCRIPTS) $(bin_PROGRAMS)
