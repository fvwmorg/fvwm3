#ifndef Picture_H
#define Picture_H

typedef struct PictureThing
{
    struct PictureThing *next;
    char *name;
    Pixmap picture;
    Pixmap mask;
    unsigned int depth;
    unsigned int width;
    unsigned int height;
    unsigned int count;
} Picture;

extern Colormap PictureCMap;
extern Display *PictureSaveDisplay;     /* Save area for display pointer */

/* This routine called during fvwm and some modules initialization */
void InitPictureCMap(Display *dpy, Window Root);

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


/** Manipulating Pictures **/

Picture* LoadPicture( Display* dpy, Window Root, 
		      char *picturePath, int color_limit);

/**
 * For GetPicture() and CachePicture(), setting ImagePath to
 * NULL means "search the default image path".
 **/
Picture* GetPicture( Display* dpy, Window Root, 
		     char* ImagePath, char* pictureName, int color_limit);

Picture* CachePicture( Display *dpy, Window Root,
		       char* ImagePath, char* pictureName, int color_limit);

void DestroyPicture(Display* dpy, Picture* p);

#ifdef XPM
#include <X11/xpm.h>
void color_reduce_pixmap(XpmImage* image, int colourLimit);
#endif

#endif
