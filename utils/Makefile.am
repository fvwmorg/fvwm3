## Process this file with automake to create Makefile.in

## Don't build and install xselection, there is a better xselection package.
EXTRA_PROGRAMS = xselection

EXTRA_DIST = quantize_pixmaps build_dev.sh configure_dev.sh rebuild_dev.sh \
	fvwm_make_browse_menu.sh fvwm_make_directory_menu.sh changelog.vim

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) \
	$(X_PRE_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS) $(png_LIBS)
INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS) $(png_CFLAGS)
