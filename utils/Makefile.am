## Process this file with automake to create Makefile.in

EXTRA_DIST = fvwm_make_browse_menu.sh fvwm_make_directory_menu.sh \
	     fvwm-version-str.sh

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) \
	$(X_PRE_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS) $(png_LIBS)
AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS) $(png_CFLAGS)
