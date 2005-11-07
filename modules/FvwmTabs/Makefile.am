## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_SCRIPTS = FvwmTabs

EXTRA_DIST = $(man_MANS) ConfigFvwmTabs FvwmTabs-DefaultSetup
man_MANS = FvwmTabs.1

config_DATA = ConfigFvwmTabs FvwmTabs-DefaultSetup

FvwmTabs.1: FvwmTabs.pod
	pod2man -c "FVWM Modules" -r "@VERSION@@VERSIONINFO@" $(srcdir)/FvwmTabs.pod > FvwmTabs.1
