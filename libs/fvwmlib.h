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

#ifndef FVWMLIB_H
#define FVWMLIB_H

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>              /* needed for xpm.h and Pixel defn */
#include <ctype.h>

#include "fvwmrect.h"

/* Allow GCC extensions to work, if you have GCC */

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__
#  define __attribute__(x)
# endif
/* The __-protected variants of `format' and `printf' attributes
   are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __format__ format
#  define __printf__ printf
# endif
#endif

/***********************************************************************
 * Generic debugging
 ***********************************************************************/

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
# define DBL(_l,_x) do if (DB_IS_LEVEL(_l)) { \
                      f_db_info.filenm=__FILE__;\
                      f_db_info.funcnm=__PRETTY_FUNCTION__;\
                      f_db_info.module=DB_MODULE;\
                      f_db_info.lineno=__LINE__;\
                      f_db_print _x; }while(0)
# define DB(_x)     DBL(2,_x)

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



/***********************************************************************
 * typedefs
 ***********************************************************************/


/***********************************************************************
 * Replacements for missing system calls.
 ***********************************************************************/

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

int matchWildcards(char *pattern, char *string);

#define MAX_MODULE_INPUT_TEXT_LEN 1000


/***********************************************************************
 * Various system related utils
 ***********************************************************************/

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
void setFileStamp(FileStamp *stamp, const char *name);
Bool isFileStampChanged(const FileStamp *stamp, const char *name);



/***********************************************************************
 * Stuff for dealing w/ bitmaps & pixmaps:
 ***********************************************************************/

XColor *GetShadowColor(Pixel);
XColor *GetHiliteColor(Pixel);
Pixel GetShadow(Pixel);
Pixel GetHilite(Pixel);

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


/***********************************************************************
 * Wrappers around various X11 routines
 ***********************************************************************/

typedef struct FvwmFont
{
  XFontStruct *font;		/* font structure */
#ifdef I18N_MB
  XFontSet fontset;		/* font set */
#endif
  int height;			/* height of the font */
  int y;			/* Y coordinate to draw characters */
} FvwmFont;

XFontStruct *GetFontOrFixed(Display *disp, char *fontname);
#ifdef I18N_MB
XFontSet GetFontSetOrFixed(Display *disp, char *fontname);
#endif
Bool LoadFvwmFont(Display *dpy, char *fontname, FvwmFont *ret_font);
void FreeFvwmFont(Display *dpy, FvwmFont *font);

void MyXGrabServer(Display *disp);
void MyXUngrabServer(Display *disp);

void send_clientmessage (Display *disp, Window w, Atom a, Time timestamp);

/* not really a wrapper, but useful and X related */
void PrintXErrorAndCoredump(Display *dpy, XErrorEvent *error, char *MyName);

/*
 * This function determines the location of the mouse pointer from the event
 * if possible, if not it queries the X server. Returns False if it had to
 * query the server and the call failed.
 */
Bool GetLocationFromEventOrQuery(Display *dpy, Window w, XEvent *eventp,
				 int *ret_x, int *ret_y);
/*
 * Return the subwindow member of an event if the event type has one.
 */
Window GetSubwindowFromEvent(Display *dpy, XEvent *eventp);

/***********************************************************************
 * Wrappers around Xrm routines (XResources.c)
 ***********************************************************************/
void MergeXResources(Display *dpy, XrmDatabase *pdb, Bool override);
void MergeCmdLineResources(XrmDatabase *pdb, XrmOptionDescList opts,
			   int num_opts, char *name, int *pargc, char **argv,
			   Bool fNoDefaults);
Bool MergeConfigLineResource(XrmDatabase *pdb, char *line, char *prefix,
			     char *bindstr);
Bool GetResourceString(
  XrmDatabase db, const char *resource, const char *prefix, XrmValue *xval);


/***********************************************************************
 * Stuff for Graphics.c and ModGraph.c
 ***********************************************************************/

void RelieveRectangle(Display *dpy, Drawable d, int x,int y,int w,int h,
		      GC ReliefGC, GC ShadowGC, int line_width);
void RelieveRectangle2(Display *dpy, Drawable d, int x,int y,int w,int h,
		      GC ReliefGC, GC ShadowGC, int line_width);

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

Pixel *AllocLinearGradient(char *s_from, char *s_to, int npixels,
			   int skip_first_color);

Pixel *AllocNonlinearGradient(char *s_colors[], int clen[],
			      int nsegs, int npixels);

/* Convenience function. Calls AllocNonLinearGradient to fetch all colors and
 * then frees the color names and the perc and color_name arrays. */
Pixel *AllocAllGradientColors(char *color_names[], int perc[],
			      int nsegs, int ncolors);

unsigned int ParseGradient(char *gradient, char **rest, char ***colors_return,
			   int **perc_return, int *nsegs_return);

Bool CalculateGradientDimensions(Display *dpy, Drawable d, int ncolors,
				 char type, unsigned int *width_ret,
				 unsigned int *height_ret);

Drawable CreateGradientPixmap(
  Display *dpy, Drawable d, GC gc, int type, int g_width, int g_height,
  int ncolors, Pixel *pixels, Drawable in_drawable, int d_x, int d_y,
  int d_width, int d_height, XRectangle *rclip);

Pixmap CreateGradientPixmapFromString(Display *dpy, Drawable d, GC gc,
				      int type, char *action,
				      unsigned int *width_return,
				      unsigned int *height_return,
				      Pixel **alloc_pixels, int *nalloc_pixels);

void DrawTrianglePattern(
  Display *dpy, Drawable d, GC ReliefGC, GC ShadowGC, GC FillGC,
  int x, int y, int width, int height, int bw, char orientation,
  Bool draw_relief, Bool do_fill, Bool is_pressed);


/***********************************************************************
 * WinMagic.c
 ***********************************************************************/

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

/***********************************************************************
 * Key and mouse bindings
 ***********************************************************************/

/* contexts for button presses */
#define C_NO_CONTEXT	0x00
#define C_WINDOW	0x01
#define C_TITLE		0x02
#define C_ICON		0x04
#define C_ROOT		0x08
#define C_FRAME		0x10
#define C_SIDEBAR       0x20
#define C_L1            0x40
#define C_R1            0x80
#define C_L2           0x100
#define C_R2           0x200
#define C_L3           0x400
#define C_R3           0x800
#define C_L4          0x1000
#define C_R4          0x2000
#define C_L5          0x4000
#define C_R5          0x8000
#define C_UNMANAGED  0x10000
#define C_RALL       (C_R1|C_R2|C_R3|C_R4|C_R5)
#define C_LALL       (C_L1|C_L2|C_L3|C_L4|C_L5)
#define C_ALL   (C_WINDOW|C_TITLE|C_ICON|C_ROOT|C_FRAME|C_SIDEBAR|\
                 C_LALL|C_RALL)

#define ALL_MODIFIERS (ShiftMask|LockMask|ControlMask|Mod1Mask|Mod2Mask|\
                       Mod3Mask|Mod4Mask|Mod5Mask)

/* Possible binding types */
#define BindingType     char
#define MOUSE_BINDING   0
#define KEY_BINDING     1
#ifdef HAVE_STROKE
#define STROKE_BINDING  2
#endif /* HAVE_STROKE */

typedef struct Binding
{
  BindingType type;       /* Is it a mouse, key, or stroke binding */
  STROKE_CODE(void *Stroke_Seq;) /* stroke sequence */
  int Button_Key;         /* Mouse Button number of Keycode */
  char *key_name;         /* In case of keycode, give the key_name too */
  int Context;            /* Contex is Fvwm context, ie titlebar, frame, etc */
  int Modifier;           /* Modifiers for keyboard state */
  void *Action;           /* What to do? */
  void *Action2;          /* This one can be used too */
  struct Binding *NextBinding;
} Binding;

Bool ParseContext(char *in_context, int *out_context_mask);
Bool ParseModifiers(char *in_modifiers, int *out_modifier_mask);
void CollectBindingList(
  Display *dpy, Binding **pblist_src, Binding **pblist_dest, BindingType type,
  STROKE_ARG(void *stroke)
  int button, KeySym keysym, int modifiers, int contexts);
int AddBinding(
  Display *dpy, Binding **pblist, BindingType type, STROKE_ARG(void *stroke)
  int button, KeySym keysym, char *key_name, int modifiers, int contexts,
  void *action, void *action2);
void FreeBindingStruct(Binding *b);
void FreeBindingList(Binding *b);
void RemoveBinding(Binding **pblist, Binding *b, Binding *prev);
Bool RemoveMatchingBinding(
  Display *dpy, Binding **pblist, BindingType type, STROKE_ARG(char *stroke)
  int button, KeySym keysym, int modifiers, int contexts);
void *CheckBinding(Binding *blist,
		   STROKE_ARG(char *stroke)
		   int button_keycode,
		   unsigned int modifier, unsigned int dead_modifiers,
		   int Context, BindingType type);
Bool MatchBinding(Display *dpy, Binding *b,
		  STROKE_ARG(void *stroke)
		  int button, KeySym keysym,
		  unsigned int modifier,unsigned int dead_modifiers,
		  int Context, BindingType type);
Bool MatchBindingExactly(
  Binding *b, STROKE_ARG(void *stroke)
  int button, KeyCode keycode, unsigned int modifier, int Context,
  BindingType type);
void GrabWindowKey(Display *dpy, Window w, Binding *binding,
		   unsigned int contexts, unsigned int dead_modifiers,
		   Bool fGrab);
void GrabAllWindowKeys(Display *dpy, Window w, Binding *blist,
		       unsigned int contexts, unsigned int dead_modifiers,
		       Bool fGrab);
void GrabWindowButton(Display *dpy, Window w, Binding *binding,
		      unsigned int contexts, unsigned int dead_modifiers,
		      Cursor cursor, Bool fGrab);
void GrabAllWindowButtons(Display *dpy, Window w, Binding *blist,
                          unsigned int contexts, unsigned int dead_modifiers,
                          Cursor cursor, Bool fGrab);
void GrabAllWindowKeysAndButtons(Display *dpy, Window w, Binding *blist,
				 unsigned int contexts,
				 unsigned int dead_modifiers,
				 Cursor cursor, Bool fGrab);
void GrabWindowKeyOrButton(
  Display *dpy, Window w, Binding *binding, unsigned int contexts,
  unsigned int dead_modifiers, Cursor cursor, Bool fGrab);
KeySym FvwmStringToKeysym(Display *dpy, char *key);

/***********************************************************************
 * Target.c
 ***********************************************************************/

void fvwmlib_keyboard_shortcuts(
    Display *dpy, int screen, XEvent *Event, int x_move_size, int y_move_size,
    int *x_defect, int *y_defect, int ReturnEvent);

void fvwmlib_get_target_window(
    Display *dpy, int screen, char *MyName, Window *app_win,
    Bool return_subwindow);

Window fvwmlib_client_window(Display *dpy, Window input);

/***********************************************************************
 * Cursor.c
 ***********************************************************************/

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
