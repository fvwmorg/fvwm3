## Process this file with automake to create Makefile.in

bin_PROGRAMS = @XPMROOT@
EXTRA_PROGRAMS = xpmroot

man_MANS = xpmroot.1
EXTRA_DIST = BuildXLockMenu fvwmrc_convert quantize_pixmaps $(man_MANS)

LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)
INCLUDES = -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)
