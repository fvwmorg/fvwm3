## Process this file with automake to create Makefile.in

configdir = @FVWM_DATADIR@

config_DATA = \
	FvwmScript-BaseConfig FvwmScript-BellSetup FvwmScript-Buttons \
	FvwmScript-Colorset FvwmScript-Date FvwmScript-FileBrowser \
	FvwmScript-Find FvwmScript-KeyboardSetup FvwmScript-PointerSetup \
	FvwmScript-Quit FvwmScript-ScreenDump FvwmScript-ScreenSetup \
	FvwmScript-Setup95 FvwmScript-WidgetDemo fvwm-script-setup95.pl

EXTRA_DIST = $(config_DATA) \
	TerminalSettings.fss UpdateFvwm2TermMenu system.fvwmrc \
	FvwmScript-Setup95.in

FvwmScript-Setup95: FvwmScript-Setup95.in
	rm -f $@
	sed -e 's,xFVWM_DATADIRx,$(FVWM_DATADIR),' -e 's,xPERLx,$(PERL),' -e 's,xFVWM_IMAGEPATHx,$(FVWM_IMAGEPATH),' $(top_srcdir)/modules/FvwmScript/Scripts/FvwmScript-Setup95.in > $@-t
	mv $@-t $@

CLEANFILES = FvwmScript-Setup95
