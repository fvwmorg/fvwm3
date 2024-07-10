## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmMFL
module_SCRIPTS = FvwmCommandS

FvwmMFL_SOURCES = FvwmMFL.c
FvwmMFL_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

## FvwmCommand/FvwmCommandS was made obsolete via
##
## 7b8684385 (Deprecation of modules, 2019-04-14)
##
## However, to maintain backwards compatibility with version 2, provide a
## wrapper for FvwmCommandS to point to FvwmMFL so that there's better parity.
FvwmCommandS: Makefile
	echo "#!/bin/sh" > $@
	echo 'modargs="$$1 $$2 $$3 $$4 $$5"' >> $@
	echo 'shift; shift; shift; shift; shift' >> $@
	echo exec $(moduledir)'/FvwmMFL $$modargs $$@' >> $@

CLEANFILES = $(module_SCRIPTS)

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm3  \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) $(XRandR_LIBS) \
	$(libevent_LIBS) $(Xft_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(X_CFLAGS) $(Xft_CFLAGS) $(libevent_CFLAGS)
