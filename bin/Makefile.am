## Process this file with automake to create Makefile.in

bin_PROGRAMS = fvwm-root
bin_SCRIPTS = \
	fvwm-config fvwm-bug \
	fvwm-convert-2.4 fvwm-convert-2.6 \
	fvwm-menu-xlock fvwm-menu-directory \
	fvwm-menu-desktop fvwm-menu-headlines

man_MANS = \
	fvwm-root.1 fvwm-config.1 fvwm-bug.1 \
	fvwm-convert-2.2.1 fvwm-convert-2.4.1 fvwm-convert-2.6.1 \
	fvwm-menu-xlock.1 fvwm-menu-directory.1 \
	fvwm-menu-desktop.1 fvwm-menu-headlines.1
EXTRA_DIST = fvwm-convert-2.2 $(man_MANS)

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) \
	$(X_PRE_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS) $(png_LIBS)
INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS) $(png_CFLAGS)


## The long generation for *.1 is to make both pod2man and 'make -j' happy.

_fvwm-menu-xlock.1: fvwm-menu-xlock.in
	@TMP=tmp-$$$$ && (\
	mkdir $$TMP && \
	cp fvwm-menu-xlock.in $$TMP/fvwm-menu-xlock && \
	pod2man --center "Fvwm Utility" $$TMP/fvwm-menu-xlock >$$TMP/fvwm-menu-xlock.1 2>/dev/null && \
	mv $$TMP/fvwm-menu-xlock.1 .; \
	rm -rf $$TMP)

_fvwm-menu-directory.1: fvwm-menu-directory.in
	@TMP=tmp-$$$$ && (\
	mkdir $$TMP && \
	cp fvwm-menu-directory.in $$TMP/fvwm-menu-directory && \
	pod2man --center "Fvwm Utility" $$TMP/fvwm-menu-directory >$$TMP/fvwm-menu-directory.1 2>/dev/null && \
	mv $$TMP/fvwm-menu-directory.1 .; \
	rm -rf $$TMP)

_fvwm-menu-headlines.1: fvwm-menu-headlines.in
	@TMP=tmp-$$$$ && (\
	mkdir $$TMP && \
	cp fvwm-menu-headlines.in $$TMP/fvwm-menu-headlines && \
	pod2man --center "Fvwm Utility" $$TMP/fvwm-menu-headlines >$$TMP/fvwm-menu-headlines.1 2>/dev/null && \
	mv $$TMP/fvwm-menu-headlines.1 .; \
	rm -rf $$TMP)

pod2man: _fvwm-menu-xlock.1 _fvwm-menu-directory.1 _fvwm-menu-headlines.1

## Create compatibility symlinks if available

install-exec-local:
	@rm -f $(bindir)/xpmroot
	@$(LN_S) fvwm-root $(bindir)/xpmroot || \
		echo "Minor warning: $(bindir)/xpmroot symlink was not created"

install-data-local:
	@rm -f $(mandir)/xpmroot.1
	@$(LN_S) fvwm-root.1 $(mandir)/xpmroot.1 || \
		echo "Minor warning: $(mandir)/xpmroot.1 symlink was not created"

CLEANFILES = $(bin_SCRIPTS) $(bin_PROGRAMS)
