## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libfvwm.a

libfvwm_a_SOURCES = Bindings.c ClientMsg.c ColorUtils.c GetFont.c \
	Grab.c Graphics.c ModGraph.c ModGraph.h Module.c Parse.c Parse.h \
	Picture.c Picture.h Strings.c Strings.h \
	System.c XResource.c debug.c envvar.c envvar.h \
	fvwmlib.h lang-strings.h safemalloc.c wild.c

libfvwm_a_LIBADD = @LIBOBJS@ @ALLOCA@

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

