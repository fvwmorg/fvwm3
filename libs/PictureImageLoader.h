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

#ifndef PICTURE_IMAGE_LOADER_H
#define PICTURE_IMAGE_LOADER_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

/* ---------------------------- interface functions ------------------------- */

/*
 * <pubfunc>PImageCreatePixmapFromArgbData
 * <description>
 * Create a pixmap with its mask and alpha channel from ARGB data.
 * </description>
 */
Bool PImageCreatePixmapFromArgbData(Display *dpy, Window Root, int color_limit,
				    unsigned char *data,
				    int start, int width, int height,
				    Pixmap pixmap, Pixmap mask, Pixmap alpha,
				    int *have_alpha);
/*
 * <pubfunc>PImageLoadPixmapFromFile
 * <description>
 * Create a pixmap with its mask and alpha channel from a file.
 * </description>
 */
Bool PImageLoadPixmapFromFile(Display *dpy, Window Root, char *file,
			      int color_limit,
			      Pixmap *pixmap, Pixmap *mask, Pixmap *alpha,
			      int *width, int *height, int *depth,
			      int *nalloc_pixels, Pixel *alloc_pixels,
			      FvwmPictureFlags fpf);

/*
 * <pubfunc>PImageLoadPixmapFromFile
 * <description>
 * Create a FvwmPicture from a file.
 * </description>
 */
FvwmPicture *PImageLoadFvwmPictureFromFile(Display *dpy, Window Root, char *path,
					   int color_limit);

/*
 * <pubfunc>PImageLoadPixmapFromFile
 * <description>
 * Create a cursor from a file.
 * </description>
 */
Bool PImageLoadCursorPixmapFromFile(Display *dpy, Window Root, 
				    char *path,
				    Pixmap *source, Pixmap *mask,
				    unsigned int *x,
				    unsigned int *y);

/*
 * <pubfunc>PImageLoadPixmapFromFile
 * <description>
 * Create a pixmap with its mask from xpm data.
 * </description>
 */
Bool PImageLoadPixmapFromXpmData(Display *dpy, Window Root, int color_limit,
				 char **data,
				 Pixmap *pixmap, Pixmap *mask,
				 int *width, int *height, int *depth);

#endif /* PICTURE_IMAGE_LOADER_H  */
