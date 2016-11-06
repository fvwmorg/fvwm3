## Process this file with automake to create Makefile.in

configdir = @FVWM_DATADIR@/default-config
config_DATA = config \
	      .stalonetrayrc \
	      FvwmScript-DateTime \
	      FvwmScript-ConfirmQuit \
	      FvwmScript-ConfirmCopyConfig

EXTRA_DIST  = images \
	      config \
	      .stalonetrayrc \
	      FvwmScript-DateTime \
	      FvwmScript-ConfirmQuit \
	      FvwmScript-ConfirmCopyConfig

install-data-hook:
	cp -r images @FVWM_DATADIR@/default-config
	ln -sf @FVWM_DATADIR@/default-config/FvwmScript-DateTime @FVWM_DATADIR@
	ln -sf @FVWM_DATADIR@/default-config/FvwmScript-ConfirmQuit @FVWM_DATADIR@
	ln -sf @FVWM_DATADIR@/default-config/FvwmScript-ConfirmCopyConfig @FVWM_DATADIR@

uninstall-hook:
	rm -fr $(configdir)
	rm @FVWM_DATADIR@/FvwmScript-*
