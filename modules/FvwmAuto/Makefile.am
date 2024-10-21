## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmAuto

EXTRA_DIST = meson.build

FvwmAuto_SOURCES = FvwmAuto.c
FvwmAuto_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm3  \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) $(XRandR_LIBS) \
	$(freetype_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(X_CFLAGS) $(freetype_CFLAGS)

