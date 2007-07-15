/* -*-c-*- */

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
extern Display *Pdpy;     /* Save area for display pointer */
extern Bool PUseDynamicColors;

typedef struct
{
	int color_limit;
	int strict;
	int allocate;
	int not_dynamic;
	int use_named_table;
} PictureColorLimitOption;

/**
 * Set a colon-separated path, with environment variable expansions.
 * Expand '+' to be the value of the previous path.
 *
 * Parameters:
 * p_path          pointer to the path variable
 * newpath         new value for *p_path
 * free_old_path   set true if we should free the memory pointed to
 *                 by p_path on entry
 *
 * The value of newpath is copied into a newly-allocated place, to which
 * '*p_path' will point to upon return.  The memory region pointed to by
 * '*p_path' upon entry will be freed if 'free_old_path' is true.
 *
 **/
void setPath(char** p_path, const char* newpath, int free_old_path);

/**
 * Search along colon-separated path for filename, with optional suffix.
 *
 * Parameters:
 * path          colon-separated path of directory names
 * filename      basename of file to search for
 * suffix        if non-NULL, filename may have this suffix
 * type          mode sought for file
 *
 * For each DIR in the path, search for DIR/filename then
 * DIR/<filename><suffix> (if suffix is non-NULL).  Return the full path of
 * the first found.
 *
 * The parameter type is a mask consisting of one or more of R_OK, W_OK, X_OK
 * and F_OK.  R_OK, W_OK and X_OK request checking whether the file exists and
 * has read, write and execute permissions, respectively.  F_OK just requests
 * checking for the existence of the file.
 *
 * Returns: full pathname of sought-after file, or NULL.  The return value
 *          points to allocated memory that the caller must free.
 *
 **/
char* searchPath(
	const char* path, const char* filename, const char* suffix, int type);

/* This routine called during modules initialization. Fvwm has its own code
 * in fvwm.c */
void PictureInitCMap(Display *dpy);

/* as above but force to use the default visual. If use_my_color_limit is True
 * also enable color limitation (independent than the fvwm one). */
void PictureInitCMapRoot(
	Display *dpy, Bool init_color_limit, PictureColorLimitOption *opt,
	Bool use_my_color_limit, Bool init_dither);

/* Analogue of the Xlib WhitePixel and BlackPixel functions but use the
   Pvisual */
Pixel PictureWhitePixel(void);
Pixel PictureBlackPixel(void);

/* for initialization of the white and black pixel (for fvwm as PictureInitCMap*
 * do this) */
void PictureSetupWhiteAndBlack(void);

/* Analogue of the Xlib DefaultGC function but take care of the Pdepth:
   - If Pdepth == DefaultDepth return the DefaultGC
   - If Pdepth != DefaultDepth and first call create a static gc with the win
   and return the gc
   -  If Pdepth != DefaultDepth and already called return the static gc */
GC PictureDefaultGC(Display *dpy, Window win);

/* these can be used to switch visuals before calling GetPicture */
/* do NOT use with CachePicture */
void PictureUseDefaultVisual(void);
void PictureUseFvwmVisual(void);
void PictureSaveFvwmVisual(void);

/** Returns current setting of the image path **/
char* PictureGetImagePath(void);


/** Sets image path to newpath.  Environment variables are expanded, and '+'
    is expanded to previous value of imagepath.  The new path is in
    newly-allocated memory, so newpath may be freed or re-used.  **/
void PictureSetImagePath(const char* newpath);


/** Search for file along pathlist.  If pathlist is NULL, will use the current
    imagepath setting.  If filename is not found, but filename.gz is found,
    will return the latter.  Mode is typically R_OK.  See searchPath() for
    more details.  **/
char* PictureFindImageFile(
	const char* filename, const char* pathlist, int mode);

typedef struct FvwmPictureThing
{
	struct FvwmPictureThing *next;
	char *name;
	unsigned long stamp;  /* should be FileStamp */
	unsigned long fpa_mask;
	Pixmap picture;
	Pixmap mask;
	Pixmap alpha;
	unsigned int depth;
	unsigned int width;
	unsigned int height;
	unsigned int count;
	Pixel *alloc_pixels;
	int nalloc_pixels;
	Bool no_limit;
} FvwmPicture;

typedef struct
{
	unsigned alpha : 1;
	unsigned alloc_pixels : 1;
} FvwmPictureFlags;

#define FPAM_NO_ALLOC_PIXELS (1)       /* do not return the allocated pixels
					* this is used only if PUseDynamicColors,
					* if not the allocated pixels are never
					* returned */
#define FPAM_NO_COLOR_LIMIT  (1 << 1)  /* do not use color limitation */
#define FPAM_NO_ALPHA        (1 << 2)  /* do not return the alpha channel */
#define FPAM_DITHER          (1 << 3)  /* dither the image */
#define FPAM_TINT            (1 << 4)  /* tint the image */
#define FPAM_MONOCHROME      (1 << 5)  /* reduce the color depth to 1-bit */

typedef struct
{
	unsigned mask : 6;
	XColor tint;
        int tint_percent;
} FvwmPictureAttributes;

/* tint no yet implemented */
#define PICTURE_FPA_AGREE(p,fpa) (p->fpa_mask == fpa.mask)

#define FRAM_HAVE_ADDED_ALPHA       (1)
#define FRAM_HAVE_TINT              (1 << 1)
#define FRAM_HAVE_UNIFORM_COLOR     (1 << 2)
#define FRAM_DEST_IS_A_WINDOW       (1 << 3)
#define FRAM_HAVE_ICON_CSET         (1 << 4)

#include "Colorset.h"

typedef struct
{
	unsigned mask : 5;
	int added_alpha_percent;
	Pixel tint;
	int tint_percent;
	Pixel uniform_pixel;
	colorset_t *colorset;
} FvwmRenderAttributes;

#define PICTURE_HAS_ALPHA(picture,cset) \
    ((picture && picture->alpha != None) ||                   \
     (cset >= 0 && Colorset[cset].icon_alpha_percent < 100))
/* alpha limit if we cannot use the alpha channel */
#define PICTURE_ALPHA_LIMIT 130

typedef struct
{
	Colormap cmap;
	int dither;
	int no_limit;
	Bool is_8;
	unsigned long *pixels_table;
	int pixels_table_size;
} PictureImageColorAllocator;

#endif /* Picture_Base_H */
