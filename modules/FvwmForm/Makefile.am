## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
configdir = @FVWM_CONFIGDIR@

module_PROGRAMS = FvwmForm
man_MANS = FvwmForm.1

## FvwmTalk was made obsolete in March 1999.
## This provides compatibility.
module_SCRIPTS = FvwmTalk
FvwmTalk:
	echo "#!/bin/sh" > $@
	echo exec ${moduledir}/FvwmTalk '$$@' FormFvwmTalk.>> $@

## NB: Neither _MANS nor _DATA are included by default in
## distributions! 
EXTRA_DIST = $(man_MANS) $(config_DATA) Changes

FvwmForm_SOURCES = FvwmForm.c FvwmForm.h ReadXServer.c \
 ParseCommand.c
FvwmForm_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

config_DATA =        \
 FormFvwmForm.       \
 FormFvwmCapture.    \
 FormFvwmQuitVerify. \
 FormFvwmRlogin.     \
 FormFvwmTalk.       \
 FormFvwmTalkHelp.

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS)  \
    $(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir) $(X_CFLAGS) $(xpm_CFLAGS)
