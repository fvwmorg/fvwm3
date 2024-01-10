# Declare the minimum required CMake version
# Name the project
project(libfvwm3)

#Remove header from SRC_FILES regex:
#' src/[^\n ]*\.h'
# Define the build target for the executable
add_library(${PROJECT_NAME} BidiJoin.c Flocale.c PictureUtils.c FScreen.c Graphics.c
	PictureGraphics.c Bindings.c FlocaleCharset.c Parse.c
	PictureImageLoader.c Colorset.c ColorUtils.c CombineChars.c Module.c
	FRender.c Ficonv.c envvar.c Fft.c  gravity.c log.c
	XResource.c FEvent.c FImage.c WinMagic.c Target.c Picture.c XError.c
	fqueue.c fvwmsignal.c System.c PictureBase.c Cursor.c Strings.c
	fvwmrect.c FRenderInit.c safemalloc.c  FBidi.c
	wild.c Grab.c Event.c ClientMsg.c setpgrp.c FShape.c
	FGettext.c Rectangles.c timeout.c flist.c charmap.c wcontext.c
	modifiers.c fsm.c FTips.c fio.c fvwmlib3.c strlcat.c strlcpy.c getpwuid.c cJSON.c
)

# To make it libfvwm3.a not liblibfvwm3.a
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        OUTPUT_NAME "fvwm3"
)
