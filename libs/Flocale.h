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

#ifndef Locale_H
#define Locale_H

#include "config.h"

#include <X11/Xlocale.h>

#ifdef HAVE_XFT
#define Picture XRenderPicture
#include <X11/Xft/Xft.h>
#undef Picture
#define Picture Picture
#define XFT_CODE(x) x
#else
#define XFT_CODE(x)
#endif


#ifdef MULTIBYTE

/* 
 * i18n X initialization (if  MULTIBYTE).
 * category: the usual category LC_CTYPE, LC_CTIME, ...
 * modifier: "" or NULL if NULL XSetLocaleModifiers is not called
 * module: the name of the fvwm module that call the function for reporting
 * errors message
 * The locale and the modifiers is sotred in Flocale and Fmodifiers.
 * Flocale is set to NULL if the locale is not supported by the Xlib.
 * In this case the Flocale* functions below does not use the Xmb* functions
 * 
 */
void FInitLocale(
		int category,
		const char *local,
		const char *modifier,
		const char *module
		);

#else /* !MULTIBYTE */

#define FInitLocale(x,y,z,t)

#endif /* MULTIBYTE */

/* 
 * Initialize the locale for !MULTIBYTE and get the user Charset for iconv
 * conversion.
 * module: should be the name of the fvwm module that call the function
 * for error messages.
 * This is probably not appropriate from an X point of view
 *
 */
void FInitCharset(const char *module);

/* ***************************************************************************
 * font loading
 * ***************************************************************************/
#define MB_FALLBACK_FONT \
      "-*-fixed-medium-r-semicondensed-*-13-*,-*-fixed-medium-r-normal-*-14-*,-*-medium-r-normal-*-16-*"
/* rationale: -*-fixed-medium-r-semicondensed-*-13-* should gives "fixed" in
   most non multibyte charset (?).
   -*-fixed-medium-r-normal-*-14-* is ok for jsx
   -*-medium-r-normal-*-16-* is for gb
   Of course this is for XFree. Any help for other X server basic font set
   is welcome.
*/
#define FALLBACK_FONT "fixed"
#define NUMBER_OF_MISSING_CHARSET_ERR_MSG 5

typedef struct flocalefont
{
  struct flocalefont *next;
  char *name;
  int count;
  XFontStruct *font;		/* font structure */
#ifdef MULTIBYTE
  XFontSet fontset;		/* font set */
#endif
#ifdef HAVE_XFT
  XftFont *xftfont;
  Bool utf8;
#endif
  int height;			/* height of the font: ascent + descent */
  int ascent;
  int descent;
  int max_char_width;
} FlocaleFont;


/*
 * load a FlocaleFont (create it or load it from a cache)
 * fontname: a ";" sperated list of "," separated list of XFLD font names or
 * either "xft:" followed by a  Xft font name. Examples: 
 * "xft:Verdana:Bold:pixelsize=14:rgba=rgb"
 * "xft:Verdana:size=12;-adobe-courier-medium-r-normal--14-*,fixed"
 * module: name of the fvwm module for errors msg
 * If fontname is NULL the "default font" is loaded (2,3,4).
 * The following logic is used:
 * 0) If fontname has been has been already loaded the cache is used
 * 1) We try to load each element "fn" of the ";" seprated list until success
 *    as follows:
 *    a - if fn begin with "xft:", then if HAVE_XFT fn is loaded as an xft
 *        font; if !HAVE_XFT fn is skipped (ignored)
 *    b - If MULTIBYTE and the locale is supported fn is loaded using
 *        XCreateFontSet. If this fail fallback into 1-c)
 *    c - If !MULTIBYTE (or the locale is not supported or 1-b) fail) fn is
 *        loaded using XLoadQueryFont (the first loadable font in the fn ","
 *        separated list is load)
 * 2) If 0) and 1) fail:
 *    - If MULTIBYTE try to load MB_FALLBACK_FONT with XCreateFontSet
 *    - If !MULTIBYTE try to load FALLBACK_FONT with XLoadQueryFont
 * 3) If MULTIBYTE and 0), 1) and 2) fail fall back to 2) !MULTIBYTE
 * 4) If everything fail the function return NULL.
 *
 * If font loading succed. Only one of the font, fontset, xftfont member of the
 * FlocaleFont structure is not NULL/None. The caller should use this to
 * set appropriately the gc member of the FlocaleWinString struct (the fid
 * gc member should be set only if font is not NULL).
 *
 * In the MULTIBYTE case, if the locale is not supported by the Xlib,
 * font loading is done as if !MULTIBYTE 
 */
FlocaleFont *FlocaleLoadFont(Display *dpy, char *fontname, char *module);

/*
 * unload the flf FlocaleFont
 */
void FlocaleUnloadFont(Display *dpy, FlocaleFont *flf);

/* ***************************************************************************
 * Width and Drawing
 * ***************************************************************************/
typedef struct FlocaleWinString
{
  char *str;
  GC gc;
  Window win;
  int x;
  int y;
  int len;
} FlocaleWinString;

#define FWS_HAVE_LENGTH (1)

/*
 * Draw the text specified in fstring->str using fstring->gc as GC on the
 * fstring->win window at position fstring->{x,y} using the ff FlocaleFont.
 * If flags & FWS_HAVE_LENGTH, the fstring->len first characters of the
 * string is drawn. If !(flags & FWS_HAVE_LENGTH), the function draw the
 * the all string (fstring->len is ignored). Note that if ff->font is NULL
 * the gc should not conatins a GCFont, as if ff->font != NULL the GCFont
 * value should be ff->font->fid  
 */
void FlocaleDrawString(Display *dpy, FlocaleFont *ff, FlocaleWinString *fstring,
		       unsigned long flags);
/*
 * Call XmbTextEscapement(ff->fontset, str, sl) if ff->fontset is not None.
 * Call XTextWith(ff->font, str, sl) if ff->font is not NULL.
 */
int FlocaleTextWidth(FlocaleFont *ff, char *str, int sl);

/*
 * Allocate memory for a FlocaleWinString intialized to 0 
 */
void FlocaleAllocateWinString(FlocaleWinString **pfws);

/* ***************************************************************************
 * Text properties
 * ***************************************************************************/

/*
 * return the window or icon name of a window w
 * func: XGetWMName or XGetWMIconName
 * dpy: the display
 * w: the window for which we want the (icon) name
 * ret_name_list: used only if MULTIBYTE
 * ret_name: the icon or the window name of the window
 */
void FlocaleGetNameProperty(Status (func)(Display *, Window, XTextProperty *),
			    Display *dpy, Window w,
			    MULTIBYTE_ARG(char ***ret_name_list)
			    char **ret_name);

/*
 * Free the name property allocated with FlocaleGetNameProperty
 */
void FlocaleFreeNameProperty(MULTIBYTE_ARG(char ***ptext_list) char **ptext);

/*
 * Simple warper to XmbTextListToTextProperty (MULTIBYTE and the locale is
 * supported by the xlib) or XStringListToTextProperty
 */
Bool FlocaleTextListToTextProperty(Display *dpy, char **list, int count,
				   XICCEncodingStyle style,
				   XTextProperty *text_prop_return);


/* ***************************************************************************
 * Xft stuff
 * ***************************************************************************/

#ifdef HAVE_XFT
FlocaleFont *get_FlocaleXftFont(Display *dpy, char *fontname);
void FftDrawString(Display *dpy, Window win, FlocaleFont *flf, GC gc,
		   int x, int y, char *str, int len);
int FftTextWidth(FlocaleFont *flf, char *str, int len);
#endif

#endif /* Locale_H */
