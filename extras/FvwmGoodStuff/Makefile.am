## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmGoodStuff
FvwmGoodStuff_SOURCES = FvwmGoodStuff.c FvwmGoodStuff.h icons.c
FvwmGoodStuff_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmGoodStuff.1
EXTRA_DIST = $(man_MANS)

LDADD = -L$(top_builddir)/libs -lfvwm \
	$(xpm_LIBS) $(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)
