## Process this file with automake to create Makefile.in

configdir = @FVWM_DATADIR@

common_samples = \
	FvwmScript-BaseConfig FvwmScript-BellSetup FvwmScript-Buttons \
	FvwmScript-Colorset FvwmScript-Date FvwmScript-FileBrowser \
	FvwmScript-Find FvwmScript-KeyboardSetup FvwmScript-PointerSetup \
	FvwmScript-Quit FvwmScript-ScreenDump FvwmScript-ScreenSetup \
	FvwmScript-WidgetDemo fvwm-script-ComExample.pl

config_DATA = $(common_samples) FvwmScript-ComExample

EXTRA_DIST = $(common_samples) \
	TerminalSettings.fss UpdateFvwm2TermMenu FvwmScript-ComExample.in

FvwmScript-ComExample: FvwmScript-ComExample.in Makefile
	rm -f $@
	sed -e 's,xFVWM_DATADIRx,$(FVWM_DATADIR),' -e 's,xPERLx,$(PERL),' $(top_srcdir)/modules/FvwmScript/Scripts/FvwmScript-ComExample.in > $@-t
	mv $@-t $@

CLEANFILES = FvwmScript-ComExample
