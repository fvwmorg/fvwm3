## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libfvwm.a

## please keep .c files in largest first order to speed up parallel makes
libfvwm_a_SOURCES = ClientMsg.h Colorset.h Grab.h Module.h Parse.h Picture.h \
	Strings.h defaults.h envvar.h fvwmlib.h fvwmsignal.h lang-strings.h \
	XineramaSupport.h \
	safemalloc.h vpacket.h wild.h \
	Graphics.c Parse.c Picture.c Bindings.c Colorset.c Event.c envvar.c \
	ColorUtils.c XResource.c Module.c WinMagic.c Target.c System.c \
	XineramaSupport.c \
	XError.c fvwmsignal.c GetFont.c Strings.c wild.c safemalloc.c \
	Pointer.c debug.c Grab.c ClientMsg.c

libfvwm_a_LIBADD = @LIBOBJS@ @ALLOCA@ $(Xinerama_LIBS)

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)
