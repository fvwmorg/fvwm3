## Process this file with automake to create Makefile.in

bin_PROGRAMS = @XPMROOT@ xselection
bin_SCRIPTS = fvwmbug
noinst_SCRIPTS = fvwm-menu-xlock fvwm-menu-directory fvwm-menu-desktop

## The logic must be to add fvwm-menu-* to bin_SCRIPTS if "@PERL@" is not empty
##bin_SCRIPTS = $(bin_SCRIPTS) fvwm-menu-xlock fvwm-menu-directory fvwm-menu-desktop


EXTRA_PROGRAMS = xpmroot

man_MANS = xpmroot.1 fvwmbug.1 fvwmrc_convert.1 \
	fvwm-menu-xlock.1 fvwm-menu-directory.1 fvwm-menu-desktop.1
EXTRA_DIST = fvwmrc_convert quantize_pixmaps $(man_MANS) \
	fvwm-menu-xlock fvwm-menu-directory fvwm-menu-desktop

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)
INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)


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

pod2man: _fvwm-menu-xlock.1 _fvwm-menu-directory.1

fvwmbug: fvwmbug.sh
	@rm -f $@
	BUILDER=$${USER-$${LOGNAME-`whoami`}} && \
	sed -e 's,xCCx,$(CC),'  -e 's,xCFLAGSx,$(CFLAGS),' \
	    -e 's,xprefixx,$(prefix),' -e 's,xsysconfdirx,$(sysconfdir),' \
	    -e 's,xlibexecdirx,$(libexecdir),' -e "s,xBUILDERx,$$BUILDER," \
	    fvwmbug.sh > $@

CLEANFILES = $(bin_SCRIPTS)
