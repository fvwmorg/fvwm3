## Process this file with automake to create Makefile.in

bin_PROGRAMS = @XPMROOT@ xselection
bin_SCRIPTS = fvwmbug
noinst_SCRIPTS = fvwm-menu-xlock fvwm-menu-directory fvwm-menu-desktop

# The logic must be to add fvwm-menu-* to bin_SCRIPTS if @PERL@ is not empty
#bin_SCRIPTS = $(bin_SCRIPTS) fvwm-menu-xlock fvwm-menu-directory fvwm-menu-desktop

EXTRA_PROGRAMS = xpmroot

man_MANS = xpmroot.1 fvwmbug.1 fvwmrc_convert.1 \
	fvwm-menu-xlock.1 fvwm-menu-directory.1 fvwm-menu-desktop.1
EXTRA_DIST = fvwmrc_convert quantize_pixmaps $(man_MANS) \
	fvwm-menu-xlock fvwm-menu-directory fvwm-menu-desktop

LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)
INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)


fvwm-menu-xlock.1: fvwm-menu-xlock.in
	pod2man --center "Fvwm Utility" fvwm-menu-xlock     > $@ 2>/dev/null
fvwm-menu-directory.1: fvwm-menu-directory.in
	pod2man --center "Fvwm Utility" fvwm-menu-directory > $@ 2>/dev/null

fvwmbug: fvwmbug.sh FORCE
	@rm -f $@
	BUILDER=$${USER-$${LOGNAME-`whoami`}} && \
	sed -e 's,xCCx,$(CC),'  -e 's,xCFLAGSx,$(CFLAGS),' \
	    -e 's,xprefixx,$(prefix),' -e 's,xsysconfdirx,$(sysconfdir),' \
	    -e 's,xlibexecdirx,$(libexecdir),' -e "s,xBUILDERx,$$BUILDER," \
	    $< > $@

# do nothing, just force the above rules that depend on it...
FORCE:
