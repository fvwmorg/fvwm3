## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libfvwm.a

## please keep .c files in largest first order to speed up parallel makes
libfvwm_a_SOURCES = \
	BidiJoin.h Bindings.h ClientMsg.h Colorset.h CombineChars.h FBidi.h   \
	FEvent.h FImage.h FRender.h FRenderInit.h FRenderInterface.h FSMlib.h \
	FScreen.h FShape.h FShm.h FTips.h Fft.h FftInterface.h FGettext.h \
	Ficonv.h Flocale.h FlocaleCharset.h Fpng.h Fxpm.h Grab.h Module.h \
	Parse.h Picture.h PictureBase.h PictureDitherMatrice.h \
	PictureGraphics.h PictureImageLoader.h PictureUtils.h Strings.h \
	defaults.h envvar.h flist.h fsm.h ftime.h fvwmlib.h fvwmrect.h \
	fvwmsignal.h gravity.c gravity.h lang-strings.h queue.h safemalloc.h \
	setpgrp.h vpacket.h wild.h Rectangles.h timeout.h charmap.h wcontext.h \
	modifiers.h \
	\
	BidiJoin.c Flocale.c PictureUtils.c FScreen.c Graphics.c \
	PictureGraphics.c Bindings.c FlocaleCharset.c Parse.c \
	PictureImageLoader.c Colorset.c ColorUtils.c CombineChars.c Module.c \
	FRender.c Ficonv.c envvar.c Fft.c  gravity.c \
	XResource.c FEvent.c FImage.c WinMagic.c Target.c Picture.c XError.c \
	queue.c fvwmsignal.c System.c PictureBase.c Cursor.c Strings.c \
	fvwmrect.c FRenderInit.c debug.c safemalloc.c  FBidi.c \
	wild.c Grab.c Event.c ClientMsg.c setpgrp.c FShape.c \
	FGettext.c Rectangles.c timeout.c flist.c charmap.c wcontext.c \
	modifiers.c fsm.c FTips.c

libfvwm_a_LIBADD = @LIBOBJS@ @ALLOCA@

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(Xft_CFLAGS) $(X_CFLAGS) \
	$(iconv_CFLAGS) $(Xrender_CFLAGS) $(Bidi_CFLAGS) $(png_CFLAGS) \
	$(intl_CFLAGS)
