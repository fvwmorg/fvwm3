## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libfvwm.a

## please keep .c files in largest first order to speed up parallel makes
libfvwm_a_SOURCES = ClientMsg.h Colorset.h Grab.h Module.h Parse.h Picture.h \
	Strings.h defaults.h envvar.h fvwmlib.h fvwmsignal.h lang-strings.h \
	FScreen.h FShape.c FShape.h fvwmrect.h Flocale.h Fft.h FftInterface.h \
	FlocaleCharset.h Ficonv.h  FBidi.h PictureImageLoader.h PictureBase.h \
	Fxpm.h Fpng.h FRender.h PictureGraphics.h PictureUtils.h \
	FRenderInterface.h FRenderInit.h safemalloc.h vpacket.h wild.h \
	Graphics.c Parse.c PictureBase.c Picture.c Bindings.c Colorset.c \
	Event.c envvar.c ColorUtils.c XResource.c Module.c WinMagic.c \
	Target.c System.c \
	FScreen.c Flocale.c Fft.c FlocaleCharset.c Ficonv.c FBidi.c \
	PictureImageLoader.c PictureGraphics.c PictureUtils.c FRender.c \
	FRenderInit.c XError.c fvwmsignal.c Strings.c wild.c safemalloc.c \
	Pointer.c debug.c Grab.c ClientMsg.c fvwmrect.c Cursor.c \
	setpgrp.c setpgrp.h queue.c queue.h gravity.c gravity.h

libfvwm_a_LIBADD = @LIBOBJS@ @ALLOCA@

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(Xft_CFLAGS) $(X_CFLAGS) $(Bidi_CFLAGS)
