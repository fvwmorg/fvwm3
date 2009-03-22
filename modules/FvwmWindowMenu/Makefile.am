## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@

module_SCRIPTS = FvwmWindowMenu

EXTRA_DIST = $(man_MANS)
man_MANS = FvwmWindowMenu.1

_FvwmWindowMenu.1: FvwmWindowMenu
	pod2man --center "Fvwm Modules" --release "@VERSION@@VERSIONINFO@" \
		--name FvwmWindowMenu FvwmWindowMenu >FvwmWindowMenu.1

regenerate: _FvwmWindowMenu.1
