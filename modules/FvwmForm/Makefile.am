## Process this file with automake to create Makefile.in

program_transform_name =

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
  FvwmForm-Form           \
  FvwmForm-Capture        \
  FvwmForm-Desktop        \
  FvwmForm-QuitVerify     \
  FvwmForm-Rlogin         \
  FvwmForm-RootCursor     \
  FvwmForm-Talk           \
  FvwmForm-TalkHelp       \
  FvwmForm-XDGOptionsHelp \
  FvwmForm-XDGMenuHelp

FvwmForm_SOURCES = FvwmForm.c FvwmForm.h ReadXServer.c ParseCommand.c

FvwmForm_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

config_DATA =             \
  FvwmForm-Form           \
  FvwmForm-Capture        \
  FvwmForm-Desktop        \
  FvwmForm-QuitVerify     \
  FvwmForm-Rlogin         \
  FvwmForm-RootCursor     \
  FvwmForm-Talk           \
  FvwmForm-TalkHelp       \
  FvwmForm-XDGOptionsHelp \
  FvwmForm-XDGMenuHelp

## Xpm note: while this module may not depend on Xpm explicitly,
## there are sometimes dependencies through functions in libfvwm
## so we might as well link against libXpm, if present.
LDADD = -L$(top_builddir)/libs -lfvwm3 $(Xft_LIBS) $(X_LIBS)  \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) \
	-lm $(Xrender_LIBS) $(rsvg_LIBS) $(iconv_LIBS) $(Bidi_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(Xft_CFLAGS) $(X_CFLAGS) $(iconv_CFLAGS) \
	$(Bidi_CFLAGS) $(Xrender_CFLAGS)

CLEANFILES = $(module_SCRIPTS)
