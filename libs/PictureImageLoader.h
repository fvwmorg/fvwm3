/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */

#ifndef PICTURE_IMAGE_LOADER_H
#define PICTURE_IMAGE_LOADER_H

/* ---------------------------- included header files ---------------------- */

#include <X11/Xmd.h>

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- interface functions ------------------------ */

/*
 * <pubfunc>PImageCreatePixmapFromArgbData
 * <description>
 * Create a pixmap with its mask and alpha channel from ARGB data.
 * </description>
 */
Bool PImageCreatePixmapFromArgbData(
	Display *dpy, Window win, CARD32 *data, int start, unsigned int width,
	unsigned int height, Pixmap pixmap, Pixmap mask, Pixmap alpha,
	int *have_alpha, unsigned int *nalloc_pixels, Pixel **alloc_pixels,
	int *no_limit, FvwmPictureAttributes fpa);
/*
 * <pubfunc>PImageLoadPixmapFromFile
 * <description>
 * Create a pixmap with its mask and alpha channel from a file.
 * </description>
 */
Bool PImageLoadPixmapFromFile(
	Display *dpy, Window win, char *file, Pixmap *pixmap, Pixmap *mask,
	Pixmap *alpha, unsigned int *width, unsigned int *height, int *depth,
	unsigned int *nalloc_pixels, Pixel **alloc_pixels, int *no_limit,
	FvwmPictureAttributes fpa);

/*
 * <pubfunc>PImageLoadPixmapFromFile
 * <description>
 * Create a FvwmPicture from a file.
 * </description>
 */
FvwmPicture *PImageLoadFvwmPictureFromFile(
	Display *dpy, Window win, char *path, FvwmPictureAttributes fpa);

/*
 * <pubfunc>PImageLoadPixmapFromFile
 * <description>
 * Create a cursor from a file.
 * </description>
 */
Bool PImageLoadCursorPixmapFromFile(
	Display *dpy, Window win, char *path, Pixmap *source, Pixmap *mask,
	unsigned int *x, unsigned int *y);

/*
 * <pubfunc>PImageLoadPixmapFromFile
 * <description>
 * Create a pixmap with its mask from xpm data.
 * </description>
 */
Bool PImageLoadPixmapFromXpmData(
	Display *dpy, Window win, int color_limit, char **data,
	Pixmap *pixmap, Pixmap *mask, unsigned int *width,
	unsigned int *height, int *depth);

#endif /* PICTURE_IMAGE_LOADER_H  */
