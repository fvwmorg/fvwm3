## Process this file with automake to create Makefile.in

PERL = @PERL@

bin_PROGRAMS = @XPMROOT@ xselection
bin_SCRIPTS = fvwmbug
noinst_SCRIPTS = BuildXLockMenu

EXTRA_PROGRAMS = xpmroot

man_MANS = xpmroot.1 BuildXLockMenu.1 fvwmbug.1
EXTRA_DIST = BuildXLockMenu.in fvwmrc_convert fvwm_make_browse_menu.sh \
	quantize_pixmaps $(man_MANS)

LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)
INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)


fvwmbug: fvwmbug.sh FORCE
	@rm -f $@
	BUILDER=$${USER-$${LOGNAME-`whoami`}} && \
	sed $< > $@ -e 's,xCCx,$(CC),'  -e 's,xCFLAGSx,$(CFLAGS),' \
	    -e 's,xprefixx,$(prefix),' -e 's,xsysconfdirx,$(sysconfdir),' \
	    -e 's,xlibexecdirx,$(libexecdir),' -e "s,xBUILDERx,$$BUILDER,"

BuildXLockMenu: BuildXLockMenu.in
	@rm -f $@
	sed -e 's,xPERLx,$(PERL),' $< > $@
	chmod +x $@

BuildXLockMenu.1: BuildXLockMenu.in
	pod2man --center "FVWM Utils" $< > $@

# do nothing, just force the above rules that depend on it...
FORCE:
