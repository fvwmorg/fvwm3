/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef FIMAGE_LOADER_H
#define FIMAGE_LOADER_H 

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

/* ---------------------------- interface functions ------------------------- */
Bool FImageCreatePixmapFromArgbData(Display *dpy, Window Root, int color_limit,
				    unsigned char *data,
				    int start, int width, int height,
				    Pixmap pixmap, Pixmap mask);
Bool FImageLoadPixmapFromFile(Display *dpy, Window Root, char *path,
			      int color_limit,
			      Pixmap *pixmap, Pixmap *mask,
			      int *width, int *height, int *depth,
			      int *nalloc_pixels, Pixel *alloc_pixels);
Picture *FImageLoadPictureFromFile(Display *dpy, Window Root, char *path,
				   int color_limit);
Bool FImageLoadCursorPixmapFromFile(Display *dpy, Window Root, 
				    char *path,
				    Pixmap *source, Pixmap *mask,
				    unsigned int *x,
				    unsigned int *y);
Bool FImageLoadPixmapFromXpmData(Display *dpy, Window Root, int color_limit,
				 char **data,
				 Pixmap *pixmap, Pixmap *mask,
				 int *width, int *height, int *depth);

#endif /* FIMAGE_LOADER_H */
