## Process this file with automake to create Makefile.in

# Controversy?  patchlevel 2.0.47 optionally installed the library...
noinst_LIBRARIES = libfvwm.a

libfvwm_a_SOURCES = ClientMsg.c GetFont.c Grab.c ModParse.c ModParse.h \
	Module.c Parse.c Picture.c Strings.c System.c System.h \
	envvar.c fvwmlib.h safemalloc.c sleep.c wild.c \
	lang-strings.h XResource.c

INCLUDES = -I$(top_srcdir) $(X_CFLAGS) $(xpm_CFLAGS)

