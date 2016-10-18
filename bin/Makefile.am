## Process this file with automake to create Makefile.in

bin_PROGRAMS = fvwm-root
fvwm_root_SOURCE= fvwm-root.c
fvwm_root_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

bin_SCRIPTS = fvwm-config fvwm-bug fvwm-perllib \
	fvwm-convert-2.6 \
	fvwm-menu-xlock fvwm-menu-directory \
	fvwm-menu-desktop fvwm-menu-headlines

man_MANS = \
	fvwm-root.1 fvwm-config.1 fvwm-bug.1 fvwm-perllib.1 \
	fvwm-convert-2.6.1 \
	fvwm-menu-xlock.1 fvwm-menu-directory.1 \
	fvwm-menu-desktop.1 fvwm-menu-headlines.1

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) $(Xcursor_LIBS) \
	$(X_PRE_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS) $(Xrender_LIBS) \
	$(Xcursor_LIBS) $(png_LIBS) $(rsvg_LIBS) $(Xinerama_LIBS)
AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS) $(png_CFLAGS) \
	$(rsvg_CFLAGS) $(Xinerama_CFLAGS)

configdir = @FVWM_DATADIR@
config_DATA = fvwm-menu-desktop-config.fpl

EXTRA_DIST = fvwm-menu-desktop-config.fpl $(man_MANS)

## The long generation for *.1 is to make both pod2man and 'make -j' happy.

_fvwm-menu-xlock.1: fvwm-menu-xlock
	@TMP=tmp-$$$$ NAME=fvwm-menu-xlock && (\
	mkdir $$TMP && \
	cp $$NAME.in $$TMP/$$NAME && \
	pod2man --center "Fvwm Utilities" --release "@VERSION@@VERSIONINFO@" \
		--name $$NAME $$TMP/$$NAME >$$TMP/$$NAME.1 && \
	mv $$TMP/$$NAME.1 .; \
	rm -rf $$TMP)

_fvwm-menu-directory.1: fvwm-menu-directory
	@TMP=tmp-$$$$ NAME=fvwm-menu-directory && (\
	mkdir $$TMP && \
	cp $$NAME.in $$TMP/$$NAME && \
	pod2man --center "Fvwm Utilities" --release "@VERSION@@VERSIONINFO@" \
		--name $$NAME $$TMP/$$NAME >$$TMP/$$NAME.1 && \
	mv $$TMP/$$NAME.1 .; \
	rm -rf $$TMP)

_fvwm-menu-headlines.1: fvwm-menu-headlines
	@TMP=tmp-$$$$ NAME=fvwm-menu-headlines && (\
	mkdir $$TMP && \
	cp $$NAME.in $$TMP/$$NAME && \
	pod2man --center "Fvwm Utilities" --release "@VERSION@@VERSIONINFO@" \
		--name $$NAME $$TMP/$$NAME >$$TMP/$$NAME.1 && \
	mv $$TMP/$$NAME.1 .; \
	rm -rf $$TMP)

_fvwm-perllib.1: fvwm-perllib
	@TMP=tmp-$$$$ NAME=fvwm-perllib && (\
	mkdir $$TMP && \
	cp $$NAME.in $$TMP/$$NAME && \
	pod2man --center "Fvwm Utilities" --release "@VERSION@@VERSIONINFO@" \
		--name $$NAME $$TMP/$$NAME >$$TMP/$$NAME.1 && \
	mv $$TMP/$$NAME.1 .; \
	rm -rf $$TMP)

regenerate: \
	_fvwm-menu-xlock.1 _fvwm-menu-directory.1 _fvwm-menu-headlines.1 \
	_fvwm-perllib.1

## Create compatibility symlinks if available

install-exec-local:
	@rm -f $(DESTDIR)$(bindir)/xpmroot
	@$(LN_S) fvwm-root $(DESTDIR)$(bindir)/xpmroot || \
		echo "Minor warning: $(bindir)/xpmroot symlink was not created"

install-data-local:
	@rm -f $(DESTDIR)$(mandir)/man1/xpmroot.1
	@$(LN_S) fvwm-root.1 $(DESTDIR)$(mandir)/man1/xpmroot.1 || \
		echo "Minor warning: $(mandir)/man1/xpmroot.1 symlink was not created"

CLEANFILES = $(bin_SCRIPTS) $(bin_PROGRAMS)
