/* This program is free software; you can redistribute it and/or modify
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

#ifndef Picture_Base_H
#define Picture_Base_H

#include "config.h"

#ifdef HAVE_XRENDER
#define XRenderSupport 1
#else
#define XRenderSupport 0
#endif

#ifdef XPM
#define XpmSupport 1
#else
#define XpmSupport 0
#endif

#ifdef HAVE_PNG
#define PngSupport 1
#else
#define PngSupport 0
#endif

extern Bool Pdefault;
extern Visual *Pvisual;
extern Colormap Pcmap;
extern unsigned int Pdepth;
extern Display *Pdpy;	  /* Save area for display pointer */


/* This routine called during fvwm and some modules initialization */
void PictureInitCMap(Display *dpy);

/* these can be used to switch visuals before calling GetPicture */
/* do NOT use with CachePicture */
void PictureUseDefaultVisual(void);
void PictureUseFvwmVisual(void);
void PictureSaveFvwmVisual(void);

/** Returns current setting of the image path **/
char* PictureGetImagePath( void );


/** Sets image path to newpath.	 Environment variables are expanded, and '+'
    is expanded to previous value of imagepath.	 The new path is in
    newly-allocated memory, so newpath may be freed or re-used.	 **/
void PictureSetImagePath( const char* newpath );


/** Search for file along pathlist.  If pathlist is NULL, will use the current
    imagepath setting.	If filename is not found, but filename.gz is found,
    will return the latter.  Mode is typically R_OK.  See searchPath() for
    more details.  **/
char* PictureFindImageFile(const char* filename, const char* pathlist, int mode);

typedef struct FvwmPictureThing
{
	struct FvwmPictureThing *next;
	char *name;
	unsigned long stamp;  /* should be FileStamp */
	Pixmap picture;
	Pixmap mask;
	Pixmap alpha;
	unsigned int depth;
	unsigned int width;
	unsigned int height;
	unsigned int count;
	Pixel *alloc_pixels;
	int nalloc_pixels;
} FvwmPicture;

typedef struct
{
	unsigned alpha : 1;
	unsigned alloc_pixels : 1;
} FvwmPictureFlags;

#endif /* Picture_Base_H */
