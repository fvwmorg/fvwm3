## Process this file with automake to create Makefile.in

PERL = @PERL@

bin_PROGRAMS = @XPMROOT@ xselection
bin_SCRIPTS = BuildXLockMenu fvwm_make_browse_menu.sh

BuildXLockMenu: BuildXLockMenu.in
	@rm -f $@
	sed -e 's,xPERLx,$(PERL),' $< > $@-t
	mv $@-t $@
	chmod +x $@

EXTRA_PROGRAMS = xpmroot

man_MANS = xpmroot.1
EXTRA_DIST = BuildXLockMenu.in fvwmrc_convert fvwm_make_browse_menu.sh \
	quantize_pixmaps $(man_MANS)

LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)
INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)
