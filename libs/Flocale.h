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

#include "fvwmlib.h"
#include "gravity.h"
#include "Fft.h"
#include "Colorset.h"

/* FlocaleCharset.h and Ficonv.h should not be included */


/* ---------------------------- global definitions -------------------------- */

#define FWS_HAVE_LENGTH (1)

#define FLC_ENCODING_TYPE_NONE     0
#define FLC_ENCODING_TYPE_FONT     1
#define FLC_ENCODING_TYPE_UTF_8    2
#define FLC_ENCODING_TYPE_USC_2    3
#define FLC_ENCODING_TYPE_USC_4    4
#define FLC_ENCODING_TYPE_UTF_16   5

#define FLC_FFT_ENCODING_ISO8859_1  "ISO8859-1"
#define FLC_FFT_ENCODING_ISO10646_1 "ISO10646-1"

#define FLOCALE_FALLBACK_XCHARSET "ISO8859-1"
#define FLOCALE_UTF8_XCHARSET     "ISO10646-1"
#define FLOCALE_ICONV_CONVERSION_MAX_NUMBER_OF_WARNING 10

#define FLC_INDEX_ICONV_CHARSET_NOT_FOUND         -1
#define FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED   -2

#define FLOCALE_DEBUG_SETLOCALE 0
#define FLOCALE_DEBUG_CHARSET   0
#define FLOCALE_DEBUG_ICONV     0

/* ---------------------------- global macros ------------------------------- */

#define IS_TEXT_DRAWN_VERTICALLY(x) \
	((x) == TEXT_ROTATED_90 || (x) == TEXT_ROTATED_270)

#define FLC_GET_X_CHARSET(fc)         ((fc) != NULL)? (fc)->x:NULL
#define FLC_SET_ICONV_INDEX(fc, i)    (fc)->iconv_index = i
#define FLC_GET_LOCALE_CHARSET(fc, i) (fc)->locale[i]
#define FLC_GET_ICONV_CHARSET(fc) \
	((fc) != NULL && (fc)->iconv_index >= 0)? (fc)->locale[(fc)->iconv_index]:NULL
#define FLC_DO_ICONV_CHARSET_INITIALIZED(fc) \
	((fc) != NULL && (fc)->iconv_index != FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED)
#define FLC_HAVE_ICONV_CHARSET(fc) ((fc) != NULL && (fc)->iconv_index >= 0)
#define FLC_GET_BIDI_CHARSET(fc) ((fc) != NULL)? (fc)->bidi : NULL
#define FLC_ENCODING_TYPE_IS_UTF_8(fc) \
	((fc) != NULL && (fc)->encoding_type == FLC_ENCODING_TYPE_UTF_8)
#define FLC_ENCODING_TYPE_IS_USC_2(fc) \
	((fc) != NULL && (fc)->encoding_type == FLC_ENCODING_TYPE_USC_2)
#define FLC_ENCODING_TYPE_IS_USC_4(fc) \
	((fc) != NULL && (fc)->encoding_type == FLC_ENCODING_TYPE_USC_4)

#define FLC_DEBUG_GET_X_CHARSET(fc)  \
	((fc) == NULL || (fc)->x == NULL)? "None":(fc)->x
#define FLC_DEBUG_GET_ICONV_CHARSET(fc) \
	((fc) != NULL && (fc)->iconv_index >= 0)? (fc)->locale[(fc)->iconv_index]:"None"
#define FLC_DEBUG_GET_BIDI_CHARSET(fc) \
	((fc) == NULL || (fc)->bidi == NULL)? "None":(fc)->bidi

#define FLF_MULTIDIR_HAS_UPPER(flf) \
	(((flf)->flags.shadow_dir & MULTI_DIR_NW) || \
	 ((flf)->flags.shadow_dir & MULTI_DIR_N) || \
	 ((flf)->flags.shadow_dir & MULTI_DIR_NE))
#define FLF_MULTIDIR_HAS_BOTTOM(flf) \
	(((flf)->flags.shadow_dir & MULTI_DIR_SW) || \
	 ((flf)->flags.shadow_dir & MULTI_DIR_S) || \
	 ((flf)->flags.shadow_dir & MULTI_DIR_SE))
#define FLF_MULTIDIR_HAS_LEFT(flf) \
	(((flf)->flags.shadow_dir & MULTI_DIR_SW) || \
	 ((flf)->flags.shadow_dir & MULTI_DIR_W) || \
	 ((flf)->flags.shadow_dir & MULTI_DIR_NW))
#define FLF_MULTIDIR_HAS_RIGHT(flf) \
	(((flf)->flags.shadow_dir & MULTI_DIR_SE) || \
	 ((flf)->flags.shadow_dir & MULTI_DIR_E) || \
	 ((flf)->flags.shadow_dir & MULTI_DIR_NE))

#define FLF_SHADOW_FULL_SIZE(flf) ((flf)->shadow_size + (flf)->shadow_offset)
#define FLF_SHADOW_HEIGHT(flf) \
	(FLF_SHADOW_FULL_SIZE((flf)) * \
	 (FLF_MULTIDIR_HAS_UPPER((flf))+FLF_MULTIDIR_HAS_BOTTOM((flf))))
#define FLF_SHADOW_WIDTH(flf) \
	(FLF_SHADOW_FULL_SIZE((flf)) * \
	 (FLF_MULTIDIR_HAS_LEFT((flf))+FLF_MULTIDIR_HAS_RIGHT((flf))))
#define FLF_SHADOW_ASCENT(flf) \
	(FLF_SHADOW_FULL_SIZE((flf)) * FLF_MULTIDIR_HAS_UPPER((flf)))
#define FLF_SHADOW_DESCENT(flf) \
	(FLF_SHADOW_FULL_SIZE((flf)) * FLF_MULTIDIR_HAS_BOTTOM((flf)))

#define FLF_SHADOW_LEFT_SIZE(flf) \
	(FLF_SHADOW_FULL_SIZE((flf)) * FLF_MULTIDIR_HAS_LEFT((flf)))
#define FLF_SHADOW_RIGHT_SIZE(flf) \
	(FLF_SHADOW_FULL_SIZE((flf)) * FLF_MULTIDIR_HAS_RIGHT((flf)))
#define FLF_SHADOW_UPPER_SIZE(flf) \
	(FLF_SHADOW_FULL_SIZE((flf)) * FLF_MULTIDIR_HAS_UPPER((flf)))
#define FLF_SHADOW_BOTTOM_SIZE(flf) \
	(FLF_SHADOW_FULL_SIZE((flf)) * FLF_MULTIDIR_HAS_BOTTOM((flf)))

#define FLF_FONT_HAS_ALPHA(flf,cset) \
     ((flf && flf->fftf.fftfont != None) ||                      \
      (0 && cset >= 0 && Colorset[cset].fg_alpha_percent < 100))
/* ---------------------------- type definitions ---------------------------- */

typedef struct FlocaleCharset
{
	char *x;               /* X font charset */
	char **locale;         /* list of possible charset names */
	int iconv_index;       /* defines the iconv charset name */
	char *bidi;            /* if not null a fribidi charset */
	short encoding_type;   /* encoding: font, utf8 or usc2 */
} FlocaleCharset;

typedef struct _FlocaleFont
{
	struct _FlocaleFont *next;
	char *name;
	int count;
	XFontStruct *font;      /* font structure */
	XFontSet fontset;       /* font set */
	FftFontType fftf;       /* fvwm xft font */
	FlocaleCharset *fc;     /* fvwm charset of the font */
	FlocaleCharset *str_fc; /* fvwm charset of the strings to be displayed */
	int height;             /* height of the font: ascent + descent */
	int ascent;
	int descent;
	int max_char_width;
	short shadow_size;
	short shadow_offset;
	struct
	{
		unsigned shadow_dir : (DIR_ALL_MASK + 1);
		unsigned must_free_fc : 1;
		/* is_mb are used only with a XFontStruct font, for XFontSet
		 * everything is done in the good way automatically and this
		 * parameters is not needed */
		unsigned is_mb  : 1; /* if true the font is a 2 bytes font */
	} flags;
} FlocaleFont;

typedef struct
{
	char *str;
	char *e_str;    /* tmp */
	XChar2b *str2b; /* tmp */
	GC gc;
	colorset_t *colorset;
	Window win;
	int x;
	int y;
	int len;
	Region clip_region;
	struct
	{
		unsigned text_rotation : 2;
		unsigned has_colorset : 1;
		unsigned has_clip_region : 1;
	} flags;
} FlocaleWinString;

typedef struct
{
	char *name;
	char **name_list;
} FlocaleNameString;

typedef struct
{
	int step;
	int orig_x;
	int orig_y;
	int offset;
	int outer_offset;
	multi_direction_type direction;
	int inter_step;
	int num_inter_steps;
	int x_sign;
	int y_sign;
	int size;
	unsigned sdir : (DIR_ALL_MASK + 1);
	rotation_type rot;
} flocale_gstp_args;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

/*
 * i18n X initialization
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
 *    b - If the locale is supported fn is loaded using XCreateFontSet. If this
 *        fail fallback into 1-c)
 *    c - If the locale is not supported or 1-b fail fn is loaded using
 *        XLoadQueryFont (the first loadable font in the fn "," separated list
 *        is load)
 * 2) If 0) and 1) fail:
 *    - try to load MB_FALLBACK_FONT with XCreateFontSet
 *    - If this fail try to load FALLBACK_FONT with XLoadQueryFont
 * 3) If everything fail the function return NULL.
 *
 * If font loading succed. Only one of the font, fontset, fftfont member of the
 * FlocaleFont structure is not NULL/None. The caller should use this to
 * set appropriately the gc member of the FlocaleWinString struct (the fid
 * gc member should be set only if font is not NULL).
 *
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
 * Underline a character in a string (pete@tecc.co.uk) at coffest position
 */
void FlocaleDrawUnderline(
	     Display *dpy, FlocaleFont *flf, FlocaleWinString *fws, int coffset);

/*
 * Get the position for shadow text
 */
void FlocaleInitGstpArgs(
	flocale_gstp_args *args, FlocaleFont *flf, FlocaleWinString *fws,
	int start_x, int start_y);

Bool FlocaleGetShadowTextPosition(
	int *x, int *y, flocale_gstp_args *args);

/*
 * Call XmbTextEscapement(ff->fontset, str, sl) if ff->fontset is not None.
 * Call XTextWith(ff->font, str, sl) if ff->font is not NULL.
 * If sl is negative, the string is considered to be a vertival string and
 * the function returns the height of the text.
 */
int FlocaleTextWidth(FlocaleFont *ff, char *str, int sl);

/*
 * "y" (or "x" position if rotated and Xft font) of the text relatively to 0
 */
int FlocaleGetMinOffset(FlocaleFont *flf, rotation_type rotation);

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
 * ret_name_list: for
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
 * Info
 * ***************************************************************************/
void FlocalePrintLocaleInfo(Display *dpy, int verbose);

/* ***************************************************************************
 * Misc
 * ***************************************************************************/

#endif /* FLOCALE_H */

