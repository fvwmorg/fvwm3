## Process this file with automake to create Makefile.in

SUBDIRS = Module

configdir = @FVWM_PERLLIBDIR@/FVWM

config_DATA = Module.pm Constants.pm

EXTRA_DIST = create-constants

Constants.pm: Makefile.am $(top_srcdir)/libs/Module.h
	cd $(top_builddir)/perllib/FVWM && \
	(fvwm_libdir=$(top_builddir)/libs ./create-constants)
