## Process this file with automake to create Makefile.in

configdir = @FVWM_DATADIR@/default-config
inst_location = $(DESTDIR)@FVWM_DATADIR@/default-config
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
	cp -r $(srcdir)/images $(inst_location)
	ln -sf default-config/FvwmScript-DateTime $(inst_location)/..
	ln -sf default-config/FvwmScript-ConfirmQuit $(inst_location)/..
	ln -sf default-config/FvwmScript-ConfirmCopyConfig $(inst_location)/..

uninstall-hook:
	rm -fr $(DESTDIR)/$(configdir)
