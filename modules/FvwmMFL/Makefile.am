## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmMFL

FvwmMFL_SOURCES = FvwmMFL.c
FvwmMFL_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm3  \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) $(XRandR_LIBS) \
	$(libevent_LIBS) $(libbson_LIBS) $(Xft_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(X_CFLAGS) $(libbson_CFLAGS) $(Xft_CFLAGS)

