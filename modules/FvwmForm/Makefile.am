## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_DATADIR@

module_PROGRAMS = FvwmForm
man_MANS = FvwmForm.1

## FvwmTalk was made obsolete in March 1999.
## This provides compatibility.
module_SCRIPTS = FvwmTalk

FvwmTalk: ../../config.h Makefile
	echo "#!/bin/sh" > $@
	echo exec ${moduledir}/FvwmForm '$$@' FvwmForm-Talk>> $@

## NB: Neither _MANS nor _DATA are included by default in
## distributions!
EXTRA_DIST = $(man_MANS) Changes \
  FvwmForm-Setup.in   \
  FvwmForm-Form       \
  FvwmForm-Capture    \
  FvwmForm-QuitVerify \
  FvwmForm-Rlogin     \
  FvwmForm-RootCursor \
  FvwmForm-Talk       \
  FvwmForm-TalkHelp

FvwmForm_SOURCES = FvwmForm.c FvwmForm.h ReadXServer.c ParseCommand.c

FvwmForm_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

config_DATA =        \
  FvwmForm-Setup      \
  FvwmForm-Form       \
  FvwmForm-Capture    \
  FvwmForm-QuitVerify \
  FvwmForm-Rlogin     \
  FvwmForm-RootCursor \
  FvwmForm-Talk       \
  FvwmForm-TalkHelp

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS)  \
	$(X_PRE_LIBS) $(XINERAMA_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) -lm

INCLUDES = -I$(top_srcdir) $(X_CFLAGS) $(xpm_CFLAGS)

FvwmForm-Setup: FvwmForm-Setup.in Makefile
	rm -f $@
	sed -e 's,xFVWM_DATADIRx,$(FVWM_DATADIR),' $(top_srcdir)/modules/FvwmForm/FvwmForm-Setup.in > $@-t
	mv $@-t $@

CLEANFILES = $(module_SCRIPTS) FvwmForm-Setup
