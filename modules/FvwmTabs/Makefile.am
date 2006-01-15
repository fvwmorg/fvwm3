## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_SCRIPTS = FvwmTabs
#DIST_SOURCES = FvwmTabs.pod

EXTRA_DIST = $(man_MANS) ConfigFvwmTabs FvwmTabs-DefaultSetup
man_MANS = FvwmTabs.1

config_DATA = ConfigFvwmTabs FvwmTabs-DefaultSetup

_FvwmTabs.1: $(srcdir)/FvwmTabs.pod
	pod2man -c "FVWM Modules" -r "@VERSION@@VERSIONINFO@" $(srcdir)/FvwmTabs.pod > FvwmTabs.1
