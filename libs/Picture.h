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

#include "PictureBase.h"
#include "PictureImageLoader.h"

/** Manipulating FvwmPictures **/

/**
 * For PGetFvwmPicture() and PCacheFvwmPicture(), setting
 * ImagePath to NULL means "search the default image path".
 **/

/* <pubfunc>PGetFvwmPicture
 * <description>
 * Return an FvwmPicture loaded from the file pictureName found in the 
 * ImagePath.. If ImagePath is NULL the default image path is used.
 * </description>
 */
FvwmPicture* PGetFvwmPicture(Display* dpy, Window Root,
			     char* ImagePath, char* pictureName,
			     int color_limit);
/* <pubfunc>PCacheFvwmPicture
 * <description>
 * Return the FvwmPicture loaded from the file pictureName found in the 
 * ImagePath. Fisrt the picture is searched in the FvwmPicture cache (so 
 * if this picture has been already loaded it is not loaded again and a
 * weight is added to the found picture). If the picture is not in the cache 
 * it is loaded from the file and added to the FvwmPicture cache. 
 * If ImagePath is NULL the default image path is used.
 * </description>
 */
FvwmPicture* PCacheFvwmPicture(Display *dpy, Window Root,
			       char* ImagePath, char* pictureName,
			       int color_limit);

/* <pubfunc>PLoadFvwmPictureFromPixmap
 * <description>
 * Return a FvwmPicture from the given data.
 * </description>
 */
FvwmPicture *PLoadFvwmPictureFromPixmap(Display *dpy, Window Root,
					char *name, Pixmap pixmap,
					Pixmap mask,
					Pixmap alpha,
					int width, int height);

/* <pubfunc>PDestroyFvwmPicture
 * <description>
 * Return a FvwmPicture from the given data. The picture is added to the 
 * FvwmPicture cache. This is not really usefull as it is not possible
 * to really cache a picture from the given data.
 * </description>
 */
FvwmPicture *PCacheFvwmPictureFromPixmap(Display *dpy, Window Root,
					 char *name, Pixmap pixmap,
					 Pixmap mask,Pixmap alpha,
					 int width, int height);
/* <pubfunc>PDestroyFvwmPicture
 * <description>
 * Remove a weight to the FvwmPicture p from the FvwmPicture cache.
 * If the weight is zero the allocated datas from p are freed
 * </description>
 */
void PDestroyFvwmPicture(Display* dpy, FvwmPicture* p);

/* <pubfunc>PCloneFvwmPicture
 * <description>
 * Duplicate an already allocated FvwmPicture in the FvwmPicture cache
 * (a weight is added to the picture).
 * </description>
 */
FvwmPicture *PCloneFvwmPicture(FvwmPicture *pic);

#endif
