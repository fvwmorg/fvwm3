/* -*-c-*- */
/* Copyright (C) 2002  fvwm workers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef FIMAGE_H
#define FIMAGE_H

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include "FShm.h"

/* ---------------------------- type definitions ---------------------------- */

typedef struct
{
	XImage *im;
	FShmSegmentInfo *shminfo;
} FImage;

/* ---------------------------- interface functions ------------------------- */

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
