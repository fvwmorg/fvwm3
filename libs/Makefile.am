## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libfvwm.a

## please keep .c files in largest first order to speed up parallel makes
libfvwm_a_SOURCES = \
	ClientMsg.h Colorset.h FBidi.h FRender.h FRenderInit.h \
	FRenderInterface.h FScreen.h FShape.h Fft.h FftInterface.h Ficonv.h \
	Flocale.h FlocaleCharset.h Fpng.h Fxpm.h Grab.h Module.h Parse.h \
	Picture.h PictureBase.h PictureDitherMatrice.h PictureGraphics.h \
	PictureImageLoader.h \
	PictureUtils.h Strings.h defaults.h envvar.h ftime.h fvwmlib.h \
	fvwmrect.h fvwmsignal.h gravity.c gravity.h lang-strings.h queue.h \
	safemalloc.h setpgrp.h vpacket.h wild.h \
	\
	Graphics.c Parse.c PictureBase.c Picture.c Bindings.c Colorset.c \
	Event.c envvar.c ColorUtils.c XResource.c Module.c WinMagic.c \
	Target.c System.c FScreen.c Flocale.c Fft.c FlocaleCharset.c Ficonv.c \
	FBidi.c PictureImageLoader.c PictureGraphics.c PictureUtils.c \
	FRender.c FRenderInit.c XError.c fvwmsignal.c Strings.c wild.c \
	safemalloc.c Pointer.c debug.c Grab.c ClientMsg.c fvwmrect.c Cursor.c \
	setpgrp.c queue.c FShape.c

libfvwm_a_LIBADD = @LIBOBJS@ @ALLOCA@

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(Xft_CFLAGS) $(X_CFLAGS) $(Bidi_CFLAGS)
