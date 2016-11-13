## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@

module_SCRIPTS = FvwmPerl

EXTRA_DIST = $(man_MANS)
man_MANS = FvwmPerl.1

_FvwmPerl.1: FvwmPerl
	pod2man --center "Fvwm Modules" --release "@VERSION@@VERSIONINFO@" \
		--name FvwmPerl FvwmPerl >FvwmPerl.1

regenerate: _FvwmPerl.1
