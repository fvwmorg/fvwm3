## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@

module_SCRIPTS = FvwmPerl

EXTRA_DIST = $(man_MANS)
man_MANS = FvwmPerl.1

_FvwmPerl.1: FvwmPerl.in
	pod2man --center "FVWM Modules" FvwmPerl >FvwmPerl.1
