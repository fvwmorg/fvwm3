/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis
 *
 * This program is free software; you can redistribute it and/or modify
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

#ifndef FLOCALE_H
#define FLOCALE_H

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <X11/Xlocale.h>
#include <X11/Xutil.h>

#include "Fft.h"
/* FlocaleCharset.h and Ficonv.h should not be included */

/* ---------------------------- global definitions -------------------------- */

#ifdef COMPOUND_TEXT
#define FlocaleCompoundText 1
#else
#define FlocaleCompoundText 0
#endif

#ifdef MULTIBYTE
#define FlocaleMultibyteSupport 1
#undef FlocaleCompoundText
#define FlocaleCompoundText 0
#else
#define FlocaleMultibyteSupport 0
#endif

#define FWS_HAVE_LENGTH (1)

/* ---------------------------- global macros ------------------------------- */

#define IS_TEXT_DRAWN_VERTICALLY(x) \
	(x == TEXT_ROTATED_90 || x == TEXT_ROTATED_270)


#define FLC_INDEX_ICONV_CHARSET_NOT_FOUND         -1
#define FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED   -2

#define FLC_GET_X_CHARSET(fc)         (fc != NULL)? fc->x:NULL
#define FLC_SET_ICONV_INDEX(fc, i)    fc->iconv_index = i
#define FLC_GET_LOCALE_CHARSET(fc, i) fc->locale[i]
#define FLC_GET_ICONV_CHARSET(fc) \
      (fc != NULL && fc->iconv_index >= 0)? fc->locale[fc->iconv_index]:NULL
#define FLC_DO_ICONV_CHARSET_INITIALIZED(fc) \
      (fc != NULL && fc->iconv_index != FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED)
#define FLC_HAVE_ICONV_CHARSET(fc) (fc != NULL && fc->iconv_index >= 0)
#define FLC_GET_BIDI_CHARSET(fc) (fc != NULL)? fc->bidi : NULL

#define FLC_DEBUG_GET_X_CHARSET(fc)  \
         (fc == NULL || fc->x == NULL)? "None":fc->x
#define FLC_DEBUG_GET_ICONV_CHARSET(fc) \
         (fc != NULL && fc->iconv_index >= 0)? fc->locale[fc->iconv_index]:"None"
#define FLC_DEBUG_GET_BIDI_CHARSET(fc) \
	 (fc == NULL || fc->bidi == NULL)? "None":fc->bidi

#define FLOCALE_FALLBACK_XCHARSET "ISO8859-1"
#define FLOCALE_UTF8_XCHARSET     "ISO10646-1"
#define FLOCALE_ICONV_CONVERSION_MAX_NUMBER_OF_WARNING 10

#define FLOCALE_DEBUG_SETLOCALE 0
#define FLOCALE_DEBUG_CHARSET   0
#define FLOCALE_DEBUG_ICONV     0

/* ---------------------------- type definitions ---------------------------- */

typedef enum
{
	TEXT_ROTATED_0    = 0,
	TEXT_ROTATED_90   = 1,
	TEXT_ROTATED_180  = 2,
	TEXT_ROTATED_270  = 3,
	TEXT_ROTATED_MASK = 3,
} text_rotation_type;

typedef struct FlocaleCharset
{
	char *x;          /* X font charset */
	char **locale;    /* list of possible charset names */
	int iconv_index;  /* defines the iconv charset name */
	char *bidi;       /* if not null a fribidi charset */
} FlocaleCharset;

typedef struct _FlocaleFont
{
	struct _FlocaleFont *next;
	char *name;
	int count;
	XFontStruct *font;	/* font structure */
	XFontSet fontset;	/* font set */
	FftFontType fftf;
	FlocaleCharset *fc;
	int height;		/* height of the font: ascent + descent */
	int ascent;
	int descent;
	int max_char_width;
	int min_char_offset;
} FlocaleFont;

typedef struct
{
	char *str;
	GC gc;
	Window win;
	int x;
	int y;
	int len;
	struct
	{
		unsigned text_rotation : 2;
	} flags;
} FlocaleWinString;

typedef struct
{
	char *name;
	char **name_list;
} FlocaleNameString;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

#if FlocaleMultibyteSupport

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
 * The function should be called as FlocaleInit(LC_CTYPE, "", "", "myname");
 */
void FlocaleInit(
	int category, const char *local, const char *modifier,
	const char *module);

#else /* !FlocaleMultibyteSupport */

#define FlocaleInit(x,y,z,t)

#endif /* FlocaleMultibyteSupport */

/* ***************************************************************************
 * font loading
 * ***************************************************************************/

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
 *    a - if fn begin with "xft:", then if FftSupport fn is loaded as an xft
 *        font; if !FftSupport fn is skipped (ignored)
 *    b - If FlocaleMultibyteSupport and the locale is supported fn is loaded
 *        using XCreateFontSet. If this fail fallback into 1-c)
 *    c - If !FlocaleMultibyteSupport (or the locale is not supported or 1-b)
 *        fail) fn is loaded using XLoadQueryFont (the first loadable font in
 *        the fn "," separated list is load)
 * 2) If 0) and 1) fail:
 *    - If FlocaleMultibyteSupport try to load MB_FALLBACK_FONT with
 *      XCreateFontSet
 *    - If !FlocaleMultibyteSupport try to load FALLBACK_FONT with
 *      XLoadQueryFont
 * 3) If FlocaleMultibyteSupport and 0), 1) and 2) fail fall back to 2)
 *    !FlocaleMultibyteSupport
 * 4) If everything fail the function return NULL.
 *
 * If font loading succed. Only one of the font, fontset, fftfont member of the
 * FlocaleFont structure is not NULL/None. The caller should use this to
 * set appropriately the gc member of the FlocaleWinString struct (the fid
 * gc member should be set only if font is not NULL).
 *
 * In the FlocaleMultibyteSupport case, if the locale is not supported by the
 * Xlib, font loading is done as if !FlocaleMultibyteSupport
 */
FlocaleFont *FlocaleLoadFont(Display *dpy, char *fontname, char *module);

/*
 * unload the flf FlocaleFont
 */
void FlocaleUnloadFont(Display *dpy, FlocaleFont *flf);

/* ***************************************************************************
 * Width and Drawing
 * ***************************************************************************/

/*
 * Draw the text specified in fstring->str using fstring->gc as GC on the
 * fstring->win window at position fstring->{x,y} using the ff FlocaleFont.
 * If flags & FWS_HAVE_LENGTH, the fstring->len first characters of the
 * string is drawn. If !(flags & FWS_HAVE_LENGTH), the function draw the
 * the all string (fstring->len is ignored). Note that if ff->font is NULL
 * the gc should not conatins a GCFont, as if ff->font != NULL the GCFont
 * value should be ff->font->fid
 */
void FlocaleDrawString(
	Display *dpy, FlocaleFont *ff, FlocaleWinString *fstring,
	unsigned long flags);
/*
 * Call XmbTextEscapement(ff->fontset, str, sl) if ff->fontset is not None.
 * Call XTextWith(ff->font, str, sl) if ff->font is not NULL.
 * If sl is negative, the string is considered to be a vertival string and
 * the function returns the height of the text.
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
 * ret_name_list: used only if FlocaleMultibyteSupport
 * ret_name: the icon or the window name of the window
 */
void FlocaleGetNameProperty(
	Status (func)(Display *, Window, XTextProperty *), Display *dpy,
	Window w, FlocaleNameString *ret_name);

/*
 * Free the name property allocated with FlocaleGetNameProperty
 */
void FlocaleFreeNameProperty(
	FlocaleNameString *ptext);

/*
 * Simple warper to XmbTextListToTextProperty (FlocaleMultibyteSupport and the
 * locale is supported by the xlib) or XStringListToTextProperty
 */
Bool FlocaleTextListToTextProperty(
	Display *dpy, char **list, int count, XICCEncodingStyle style,
	XTextProperty *text_prop_return);

/* ***************************************************************************
 * Misc
 * ***************************************************************************/
FlocaleCharset *FlocaleGetUnsetCharset(void);
const char *FlocaleGetBidiCharset(Display *dpy, FlocaleFont *flf);

#endif /* FLOCALE_H */

