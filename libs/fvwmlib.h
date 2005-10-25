/* -*-c-*- */

#ifndef FVWMLIB_H
#define FVWMLIB_H

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>              /* needed for xpm.h and Pixel defn */
#include <ctype.h>

#include "fvwmrect.h"

/*
 * Generic debugging
 */

#ifdef NDEBUG
/* Make everything evaluates to empty.  */
# define DB_IS_LEVEL(_l) 0
# define DBL(_l,_x)
# define DB(_x)
#else
/* Define it all!  */
# define FVWM_DB_FILE   "FVWM_DB_FILE"
# define FVWM_DB_LEVEL  "FVWM_DB_LEVEL"

# ifndef __FILE__
#  define __FILE__ 0
# endif
# ifndef __LINE__
#  define __LINE__ -1
# endif
# if __GNUC__ < 2 || __STRICT_ANSI__
#  define __PRETTY_FUNCTION__ 0
# endif
# ifndef DB_MODULE
#  define DB_MODULE 0
# endif

# define DB_IS_LEVEL(_l)    ((_l)<=f_db_level)
# define DBL(_l,_x) \
	do if (DB_IS_LEVEL(_l)) { \
		f_db_info.filenm=__FILE__;\
		f_db_info.funcnm=__PRETTY_FUNCTION__;\
		f_db_info.module=DB_MODULE;\
		f_db_info.lineno=__LINE__;\
		f_db_print _x; \
	} while(0)
# define DB(_x) DBL(2,_x)

struct f_db_info
{
	const char *filenm;
	const char *funcnm;
	const char *module;
	long lineno;
};

extern struct f_db_info f_db_info;
extern int f_db_level;

extern void f_db_print(const char *fmt, ...)
	__attribute__ ((__format__ (__printf__, 1, 2)));
#endif



/*
 * typedefs
 */


/*
 * Replacements for missing system calls.
 */

#ifndef HAVE_ATEXIT
int atexit( void(*func)() );
#endif

#ifndef HAVE_GETHOSTNAME
int gethostname( char* name, int len );
#endif

#ifndef HAVE_STRCASECMP
int strcasecmp( char* s1, char* s2 );
#endif

#ifndef HAVE_STRNCASECMP
int strncasecmp( char* s1, char* s2, int len );
#endif

#ifndef HAVE_STRERROR
char* strerror( int errNum );
#endif

#ifndef HAVE_USLEEP
int usleep( unsigned long usec );
#endif


#include "ClientMsg.h"
#include "Grab.h"
#include "Parse.h"
#include "Strings.h"
#include "envvar.h"
#include "safemalloc.h"

extern int matchWildcards(const char *pattern, const char *string);

#define MAX_MODULE_INPUT_TEXT_LEN 1000


/*
 * Various system related utils
 */

int GetFdWidth(void);
int getostype(char *buf, int max);

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
void setPath( char** p_path, const char* newpath, int free_old_path );

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
char* searchPath( const char* path, const char* filename,
		  const char* suffix, int type );

/* An interface for verifying cached files. */

#define FileStamp unsigned long
FileStamp getFileStamp(const char *name);
void setFileStamp(FileStamp *stamp, const char *name);
Bool isFileStampChanged(const FileStamp *stamp, const char *name);


/* mkstemp */
int fvwm_mkstemp (char *TEMPLATE);

/*
 * Stuff for dealing w/ bitmaps & pixmaps:
 */

XColor *GetShadowColor(Pixel);
XColor *GetHiliteColor(Pixel);
Pixel GetShadow(Pixel);
Pixel GetHilite(Pixel);
XColor *GetForeShadowColor(Pixel foreground, Pixel background);
Pixel GetForeShadow(Pixel foreground, Pixel background);
XColor *GetTintedColor(Pixel in, Pixel tint, int percent);
Pixel GetTintedPixel(Pixel in, Pixel tint, int percent);

/* This function converts the colour stored in a colorcell (pixel) into the
 * string representation of a colour.  The output is printed at the
 * address 'output'.  It is either in rgb format ("rgb:rrrr/gggg/bbbb") if
 * use_hash is False or in hash notation ("#rrrrggggbbbb") if use_hash is true.
 * The return value is the number of characters used by the string.  The
 * rgb values of the output are undefined if the colorcell is invalid.  The
 * memory area pointed at by 'output' must be at least 64 bytes (in case of
 * future extensions and multibyte characters).*/
int pixel_to_color_string(
	Display *dpy, Colormap cmap, Pixel pixel, char *output, Bool use_hash);

Pixel GetSimpleColor(char *name);
/* handles colorset color names too */
Pixel GetColor(char *name);

/* duplicate an already allocated color */
Pixel fvwmlib_clone_color(Pixel p);
void fvwmlib_free_colors(Display *dpy, Pixel *pixels, int n, Bool no_limit);
void fvwmlib_copy_color(
	Display *dpy, Pixel *dst_color, Pixel *src_color, Bool do_free_dest,
	Bool do_copy_src);

/*
 * Wrappers around various X11 routines
 */

typedef struct FvwmFont
{
	XFontStruct *font;            /* font structure */
	XFontSet fontset;             /* font set */
	int height;                   /* height of the font */
	int y;                        /* Y coordinate to draw characters */
} FvwmFont;

XFontStruct *GetFontOrFixed(Display *disp, char *fontname);
XFontSet GetFontSetOrFixed(Display *disp, char *fontname);
Bool LoadFvwmFont(Display *dpy, char *fontname, FvwmFont *ret_font);
void FreeFvwmFont(Display *dpy, FvwmFont *font);

void MyXGrabServer(Display *disp);
void MyXUngrabServer(Display *disp);

void send_clientmessage (Display *disp, Window w, Atom a, Time timestamp);

/* not really a wrapper, but useful and X related */
void PrintXErrorAndCoredump(Display *dpy, XErrorEvent *error, char *MyName);

/*
 * Return the subwindow member of an event if the event type has one.
 */
Window GetSubwindowFromEvent(Display *dpy, const XEvent *eventp);

/*
 * Wrappers around Xrm routines (XResources.c)
 */
void MergeXResources(Display *dpy, XrmDatabase *pdb, Bool override);
void MergeCmdLineResources(XrmDatabase *pdb, XrmOptionDescList opts,
			   int num_opts, char *name, int *pargc, char **argv,
			   Bool fNoDefaults);
Bool MergeConfigLineResource(XrmDatabase *pdb, char *line, char *prefix,
			     char *bindstr);
Bool GetResourceString(
	XrmDatabase db, const char *resource, const char *prefix, XrmValue *xval);


/*
 * Stuff for Graphics.c and ModGraph.c
 */

void do_relieve_rectangle(
	Display *dpy, Drawable d, int x, int y, int w, int h,
	GC ReliefGC, GC ShadowGC, int line_width, Bool use_alternate_shading);
void do_relieve_rectangle_with_rotation(
	Display *dpy, Drawable d, int x, int y, int w, int h,
	GC ReliefGC, GC ShadowGC, int line_width, Bool use_alternate_shading,
	int rotation);

#define RelieveRectangle(dpy, d, x, y, w, h, ReliefGC, ShadowGC, line_width) \
	do_relieve_rectangle( \
		dpy, d, x, y, w, h, ReliefGC, ShadowGC, line_width, False)
#define RelieveRectangle2(dpy, d, x, y, w, h, ReliefGC, ShadowGC, line_width) \
	do_relieve_rectangle( \
		dpy, d, x, y, w, h, ReliefGC, ShadowGC, line_width, True)

Pixmap CreateStretchXPixmap(Display *dpy, Pixmap src, int src_width,
			    int src_height, int src_depth,
			    int dest_width, GC gc);

Pixmap CreateStretchYPixmap(Display *dpy, Pixmap src, int src_width,
			    int src_height, int src_depth,
			    int dest_height, GC gc);

Pixmap CreateStretchPixmap(Display *dpy, Pixmap src, int src_width,
			   int src_height, int src_depth,
			   int dest_width, int dest_height,
			   GC gc);

Pixmap CreateTiledPixmap(Display *dpy, Pixmap src, int src_width,
			 int src_height, int dest_width,
			 int dest_height, int depth, GC gc);

Pixmap CreateRotatedPixmap(
	Display *dpy, Pixmap src, int src_width, int src_height, int depth,
	GC gc, int rotation);

GC fvwmlib_XCreateGC(
	Display *display, Drawable drawable, unsigned long valuemask,
	XGCValues *values);

/**** gradient stuff ****/

/* gradient types */
#define H_GRADIENT 'H'
#define V_GRADIENT 'V'
#define D_GRADIENT 'D'
#define B_GRADIENT 'B'
#define S_GRADIENT 'S'
#define C_GRADIENT 'C'
#define R_GRADIENT 'R'
#define Y_GRADIENT 'Y'

Bool IsGradientTypeSupported(char type);

/* Convenience function. Calls AllocNonLinearGradient to fetch all colors and
 * then frees the color names and the perc and color_name arrays. */
XColor *AllocAllGradientColors(
	char *color_names[], int perc[], int nsegs, int ncolors, int dither);

unsigned int ParseGradient(char *gradient, char **rest, char ***colors_return,
			   int **perc_return, int *nsegs_return);

Bool CalculateGradientDimensions(Display *dpy, Drawable d, int ncolors,
				 char type, int dither, unsigned int *width_ret,
				 unsigned int *height_ret);
Drawable CreateGradientPixmap(
	Display *dpy, Drawable d, GC gc, int type, int g_width, int g_height,
	int ncolors, XColor *xcs, int dither, Pixel **d_pixels, int *d_npixels,
	Drawable in_drawable, int d_x, int d_y, int d_width, int d_height,
	XRectangle *rclip);
Pixmap CreateGradientPixmapFromString(
	Display *dpy, Drawable d, GC gc, int type, char *action,
	unsigned int *width_return, unsigned int *height_return,
	Pixel **alloc_pixels, int *nalloc_pixels, int dither);

void DrawTrianglePattern(
	Display *dpy, Drawable d, GC ReliefGC, GC ShadowGC, GC FillGC,
	int x, int y, int width, int height, int bw, char orientation,
	Bool draw_relief, Bool do_fill, Bool is_pressed);


/*
 * WinMagic.c
 */

void SlideWindow(
	Display *dpy, Window win,
	int s_x, int s_y, unsigned int s_w, unsigned int s_h,
	int e_x, int e_y, unsigned int e_w, unsigned int e_h,
	int steps, int delay_ms, float *ppctMovement,
	Bool do_sync, Bool use_hints);

Window GetTopAncestorWindow(Display *dpy, Window child);

int GetEqualSizeChildren(
	Display *dpy, Window parent, int depth, VisualID visualid, Colormap colormap,
	Window **ret_children);

/*
 * Target.c
 */

void fvwmlib_keyboard_shortcuts(
	Display *dpy, int screen, XEvent *Event, int x_move_size, int y_move_size,
	int *x_defect, int *y_defect, int ReturnEvent);

void fvwmlib_get_target_window(
	Display *dpy, int screen, char *MyName, Window *app_win,
	Bool return_subwindow);

Window fvwmlib_client_window(Display *dpy, Window input);

/*
 * Cursor.c
 */

int fvwmCursorNameToIndex (char *cursor_name);

/* Set up heap debugging library dmalloc.  */

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

/* Set up mtrace from glibc 2.1.x for x > ?  */
#ifdef MTRACE_DEBUGGING
#include <mcheck.h>
#endif

#endif
