## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@

module_SCRIPTS = FvwmWindowLister

EXTRA_DIST = $(man_MANS)
man_MANS = FvwmWindowLister.1

_FvwmWindowLister.1: FvwmWindowLister.in
	pod2man --center "FVWM Modules" FvwmWindowLister >FvwmWindowLister.1
