## Process this file with automake to create Makefile.in

SUBDIRS = Module

configdir = @FVWM_PERLLIBDIR@/FVWM

config_DATA = Module.pm Commands.pm Constants.pm EventNames.pm Event.pm

EXTRA_DIST = \
	create-commands Commands.pm \
	create-constants Constants.pm \
	EventNames.pm Event.pm

_Constants.pm: Makefile.am $(top_srcdir)/libs/Module.h
	cd $(top_builddir)/perllib/FVWM && \
	(fvwm_libdir=$(top_builddir)/libs ./create-constants)

_Commands.pm: Makefile $(top_srcdir)/fvwm/functable.c
	cd $(top_builddir)/perllib/FVWM && \
	(fvwm_libdir=$(top_builddir)/libs ./create-commands)
