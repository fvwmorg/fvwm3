#ifndef FCURSOR_H
#define FCURSOR_H

#ifdef HAVE_XCURSOR
#	include <X11/Xcursor/Xcursor.h>

	typedef XcursorImage			FcursorImage;
	typedef XcursorImages			FcursorImages;
	typedef XcursorPixel			FcursorPixel;

#	define FcursorFilenameLoadImages(a,b)	XcursorFilenameLoadImages(a,b)
#	define FcursorGetDefaultSize(a)		XcursorGetDefaultSize(a)
#	define FcursorImageCreate(a,b)		XcursorImageCreate(a,b)
#	define FcursorImageDestroy(a)		XcursorImageDestroy(a)
#	define FcursorImagesDestroy(a)		XcursorImagesDestroy(a)
#	define FcursorImageLoadCursor(a,b)	XcursorImageLoadCursor(a,b)
#	define FcursorImagesLoadCursor(a,b)	XcursorImagesLoadCursor(a,b)
#else
	typedef struct {
		int width;
		int height;
		int xhot;
		int yhot;
		int delay;
		void *pixels;
	} FcursorImage;
	typedef struct {
		int nimage;
		FcursorImage **images;
	} FcursorImages;
	typedef void FcursorPixel;

#	define FcursorFilenameLoadImages(a,b) 0
#	define FcursorGetDefaultSize(a) 0
#	define FcursorImageCreate(a,b) 0
#	define FcursorImageDestroy(a)
#	define FcursorImagesDestroy(a)
#	define FcursorImageLoadCursor(a,b) 0
#	define FcursorImagesLoadCursor(a,b) 0
#endif

#endif /* FCURSOR_H */
