## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmSave
man_MANS = FvwmSave.1
EXTRA_DIST = $(man_MANS)

FvwmSave_SOURCES = FvwmSave.c FvwmSave.h
FvwmSave_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm  \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir) $(Xft_CFLAGS) $(X_CFLAGS)

