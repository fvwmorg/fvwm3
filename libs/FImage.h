/* -*-c-*- */

#ifndef FIMAGE_H
#define FIMAGE_H

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include "FShm.h"

/* ---------------------------- type definitions --------------------------- */

typedef struct
{
	XImage *im;
	FShmSegmentInfo *shminfo;
} FImage;

/* ---------------------------- interface functions ------------------------ */

FImage *FCreateFImage (
	Display *dpy, Visual *visual, unsigned int depth, int format,
	unsigned int width, unsigned int height);

FImage *FGetFImage(
	Display *dpy, Drawable d, Visual *visual,
	unsigned int depth, int x, int y, unsigned int width,
	unsigned int height, unsigned long plane_mask, int format);

void FPutFImage(
	Display *dpy, Drawable d, GC gc, FImage *fim, int src_x, int src_y,
	int dest_x, int dest_y, unsigned int width, unsigned int height);

void FDestroyFImage(Display *dpy, FImage *fim);

#endif /* FIMAGE_H */
