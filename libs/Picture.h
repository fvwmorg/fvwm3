/* This program is free software; you can redistribute it and/or modify
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

#ifndef Picture_H
#define Picture_H

#include "InitPicture.h"

/** Manipulating Pictures **/

/**
 * Load the given picture file.  If XPM is enabled, will try first to load
 * as pixmap file.  If no XPM file found (or XPM not enabled) will try to
 * load as bitmap.
 **/
Picture* LoadPicture( Display* dpy, Window Root,
		      char *pictureFileName, int color_limit);

/**
 * For GetPicture() and CachePicture(), setting ImagePath to
 * NULL means "search the default image path".
 **/
Picture* GetPicture( Display* dpy, Window Root,
		     char* ImagePath, char* pictureName, int color_limit);
Picture* CachePicture( Display *dpy, Window Root,
		       char* ImagePath, char* pictureName, int color_limit);

Picture *LoadPictureFromPixmap(Display *dpy, Window Root, char *name,
			       Pixmap pixmap, Pixmap mask,
			       int width, int height);
Picture *CachePictureFromPixmap(Display *dpy, Window Root,char *name,
				Pixmap pixmap, Pixmap mask,
				int width, int height);

void DestroyPicture(Display* dpy, Picture* p);

/* duplicate an already allocated picture */
Picture *fvwmlib_clone_picture(Picture *pic);

#endif
