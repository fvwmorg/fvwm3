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

#ifndef Init_Picture_H
#define Init_Picture_H

extern Bool Pdefault;
extern Visual *Pvisual;
extern Colormap Pcmap;
extern unsigned int Pdepth;
extern Display *Pdpy;     /* Save area for display pointer */


/* This routine called during fvwm and some modules initialization */
void InitPictureCMap(Display *dpy);

/* these can be used to switch visuals before calling GetPicture */
/* do NOT use with CachePicture */
void UseDefaultVisual(void);
void UseFvwmVisual(void);
void SaveFvwmVisual(void);

/** Returns current setting of the image path **/
char* GetImagePath( void );


/** Sets image path to newpath.  Environment variables are expanded, and '+'
    is expanded to previous value of imagepath.  The new path is in
    newly-allocated memory, so newpath may be freed or re-used.  **/
void SetImagePath( const char* newpath );


/** Search for file along pathlist.  If pathlist is NULL, will use the current
    imagepath setting.  If filename is not found, but filename.gz is found,
    will return the latter.  Mode is typically R_OK.  See searchPath() for
    more details.  **/
char* findImageFile( const char* filename, const char* pathlist, int mode );

typedef struct PictureThing
{
    struct PictureThing *next;
    char *name;
    unsigned long stamp;  /* should be FileStamp */
    Pixmap picture;
    Pixmap mask;
    unsigned int depth;
    unsigned int width;
    unsigned int height;
    unsigned int count;
    Pixel *alloc_pixels;
    int nalloc_pixels;
} Picture;

#endif /* Init_Picture_H */
