## Process this file with automake to create Makefile.in

bin_PROGRAMS = @XPMROOT@
EXTRA_PROGRAMS = xpmroot

# automake apparently doesn't consider manpages as 'source', so it
# must also be listed in EXTRA_DIST
man_MANS = xpmroot.1
EXTRA_DIST = xpmroot.1

LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)
INCLUDES = -I$(top_srcdir)/libs $(X_CFLAGS) $(xpm_CFLAGS)
