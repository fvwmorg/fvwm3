## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libfvwm.a

## .c files in largest first order to speed up parallel makes
libfvwm_a_SOURCES = Colorset.h ClientMsg.h Grab.h Module.h Parse.h Picture.h \
	Strings.h envvar.h fvwmlib.h fvwmsignal.h lang-strings.h safemalloc.h \
	vpacket.h wild.c wild.h Parse.c Picture.c Bindings.c envvar.c \
	XResource.c ColorUtils.c Module.c Graphics.c fvwmsignal.c System.c \
	Strings.c wild.c GetFontSet.c debug.c GetFont.c safemalloc.c Grab.c \
	ClientMsg.c Colorset.c defaults.h XError.c Pointer.c WinMagic.c

libfvwm_a_LIBADD = @LIBOBJS@ @ALLOCA@

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

