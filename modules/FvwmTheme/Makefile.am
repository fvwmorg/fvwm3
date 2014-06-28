## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmTheme
man_MANS = FvwmTheme.1
EXTRA_DIST = $(man_MANS)

FvwmTheme_SOURCES = FvwmTheme.c
FvwmTheme_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm  \
	$(X_PRE_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS)

AM_CPPFLAGS = -I$(top_srcdir)  $(X_CFLAGS)

