## Process this file with automake to create Makefile.in

configdir = @FVWM_CONFIGDIR@

EXTRA_DIST = ScriptFvwmBellSetup ScriptFvwmButtons ScriptFvwmColorset	\
	ScriptFvwmDate ScriptFvwmBaseConfig ScriptFvwmFileBrowser	\
	ScriptFvwmFind ScriptFvwmKeyboardSetup ScriptFvwmPointerSetup	\
	ScriptFvwmQuit ScriptFvwmScreenDump ScriptFvwmScreenSetup	\
	ScriptFvwmSetup95.in TerminalSettings.fss UpdateFvwm2TermMenu	\
	system.fvwmrc

config_DATA = ScriptFvwmBellSetup ScriptFvwmButtons ScriptFvwmColorset	\
	ScriptFvwmDate ScriptFvwmBaseConfig ScriptFvwmFileBrowser	\
	ScriptFvwmFind ScriptFvwmKeyboardSetup ScriptFvwmPointerSetup	\
	ScriptFvwmQuit ScriptFvwmScreenDump ScriptFvwmScreenSetup	\
	ScriptFvwmSetup95

ScriptFvwmSetup95: ScriptFvwmSetup95.in
	rm -f $@
	sed -e 's,xFVWM_CONFIGDIRx,$(FVWM_CONFIGDIR),' -e 's,xPERLx,$(PERL),' $(top_srcdir)/modules/FvwmScript/Scripts/ScriptFvwmSetup95.in > $@-t
	mv $@-t $@
