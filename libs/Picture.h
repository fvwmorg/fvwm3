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

/** Access to the image path **/
char* GetImagePath( void );
void SetImagePath( char* newpath );

char* findImageFile( char* filename, char* pathlist, int mode );


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
