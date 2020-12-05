## Process this file with automake to create Makefile.in

program_transform_name =

moduleexecdir = @FVWM_MODULEDIR@
moduleexec_PROGRAMS = FvwmEvent

FvwmEvent_SOURCES = FvwmEvent.c
FvwmEvent_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm3 \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(X_CFLAGS)
