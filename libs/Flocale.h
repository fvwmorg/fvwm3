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

#ifdef X_LOCALE
#include <X11/Xlocale.h>
#else
#include <locale.h>
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
  int height;			/* height of the font: ascent + descent */
  int ascent;
  int descent;
} FlocaleFont;


/*
 * load a FlocaleFont (create it or load it from a cache)
 * fontname: a "," separated list of XFLD font names
 * module: name of the fvwm module for errors msg
 * If fontname is NULL the "default font" is returned. The following logic
 * is used:
 * 0) If fontname has been has been already loaded the cache is used
 * 1) - If MULTIBYTE and the locale is supported (and fontname is not NULL)
 *      fontname is loaded using XCreateFontSet (or the cache)
 *    - If !MULTIBYTE fontname is loaded using XLoadQueryFont (the first
 *      loadable font in the fontname "," separated list is load)
 * 2) If 1) fail with MULTIBYTE fallback into 1) !MULTIBYTE
 * 3) If 1) and 2) fail:
 *    - If MULTIBYTE try to load MB_FALLBACK_FONT
 *    - If !MULTIBYTE try to load FALLBACK_FONT
 * 4) If MULTIBYTE and 1), 2) and 3) fail fall back to 3) !MULTIBYTE
 * 5) If everything fail the function return NULL.
 *
 * If font loading succed. Either the returned FlocaleFont struct has its
 * font member set to NULL and its fontset menber != None or  its font member
 * !=NULL and its fontset menber set to None. The caller should use this to
 * set appropriately the gc member of the FlocaleWinString struct
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
#endif /* Locale_H */
