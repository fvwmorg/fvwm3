## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmRearrange

FvwmRearrange_SOURCES = FvwmRearrange.c
FvwmRearrange_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lm -lfvwm3  \
	$(X_PRE_LIBS) $(XRandR_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(Xft_CFLAGS) $(X_CFLAGS) $(XRandR_CFLAGS)

