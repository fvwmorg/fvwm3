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
 * i18n X initialization 
 * category: the usual category LC_CTYPE, LC_CTIME, ...
 * modifier: "" or NULL if NULL XSetLocaleModifiers is not called
 * module: the name of the fvwm module that call the function for reporting
 * errors message
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
      "-*-fixed-medium-r-normal-*-13-*,-*-fixed-medium-r-normal-*-14-*"
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
 */
FlocaleFont *FlocaleLoadFont(Display *dpy, char *fontname, char *module);

/*
 * unload a FlocaleFont
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

void FlocaleDrawString(Display *dpy, FlocaleFont *ff, FlocaleWinString *fstring,
		       unsigned long flags);
int FlocaleTextWidth(FlocaleFont *ff, char *str, int sl);
void FlocaleAllocateWinString(FlocaleWinString **pfws);

/* ***************************************************************************
 * Text properties
 * ***************************************************************************/

void FlocaleGetNameProperty(Status (func)(Display *, Window, XTextProperty *),
			    Display *dpy, Window w,
			    MULTIBYTE_ARG(char ***ret_name_list)
			    char **ret_name);
void FlocaleFreeNameProperty(MULTIBYTE_ARG(char ***ptext_list) char **ptext);

Bool FlocaleTextListToTextProperty(Display *dpy, char **list, int count,
				   XICCEncodingStyle style,
				   XTextProperty *text_prop_return);
#endif /* Locale_H */
