## Process this file with automake to create Makefile.in

# Controversy?  patchlevel 2.0.47 optionally installed the library...
noinst_LIBRARIES = libfvwm.a

libfvwm_a_SOURCES = ClientMsg.c GetFont.c Grab.c ModParse.c ModParse.h \
	Module.c Parse.c Picture.c Strings.c System.c \
	envvar.c fvwmlib.h safemalloc.c wild.c \
	lang-strings.h XResource.c debug.c ColorUtils.c

libfvwm_a_LIBADD = @LIBOBJS@

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

