## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@

module_SCRIPTS = FvwmWindowMenu

EXTRA_DIST = $(man_MANS)
man_MANS = FvwmWindowMenu.1

_FvwmWindowMenu.1: FvwmWindowMenu.in
	pod2man --center "FVWM Modules" FvwmWindowMenu >FvwmWindowMenu.1
