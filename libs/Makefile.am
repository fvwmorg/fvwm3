## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libfvwm.a

libfvwm_a_SOURCES = Bindings.c ClientMsg.c ColorUtils.c GetFont.c Grab.c      \
	Graphics.c Module.c Parse.c Picture.c Picture.h			      \
	Strings.c System.c XResource.c envvar.c fvwmlib.h safemalloc.c wild.c \
	lang-strings.h debug.c

libfvwm_a_LIBADD = @LIBOBJS@ @ALLOCA@

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

