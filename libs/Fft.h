/* -*-c-*- */
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

#ifndef FFT_H
#define FFT_H

/* ---------------------------- included header files ----------------------- */

#ifdef HAVE_XFT
#define Picture XRenderPicture
#include <X11/Xft/Xft.h>
#undef Picture
#define Picture Picture
#endif

/* ---------------------------- global definitions -------------------------- */

#ifdef HAVE_XFT

#define FftSupport 1
#ifdef HAVE_XFT_UTF8
#define FftUtf8Support 1
#else
#define FftUtf8Support 0
#endif

#else

#define FftSupport 0
#define FftUtf8Support 0

#endif

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

#if FftSupport
typedef XGlyphInfo FGlyphInfo;
typedef XftChar8 FftChar8;
typedef XftChar16 FftChar16;
typedef XftChar32 FftChar32;
typedef XftType FftType;
typedef XftMatrix FftMatrix;
typedef XftResult FftResult;
typedef XftValue FftValue;
typedef XftValueList FftValueList;
typedef XftPatternElt FftPatternElt;
typedef XftPattern FftPattern;
typedef XftFontSet FftFontSet;
typedef XftFontStruct FftFontStruct;
typedef XftDraw FftDraw;
typedef XftFont FftFont;
typedef XftColor FftColor;
typedef XftObjectSet FftObjectSet;
#else
typedef struct
{
	unsigned short width;
	unsigned short height;
	short x;
	short y;
	short xOff;
	short yOff;
} XGlyphInfo;
typedef XGlyphInfo FGlyphInfo;
typedef unsigned char FftChar8;
typedef unsigned short FftChar16;
typedef unsigned int FftChar32;
typedef int FftType;
typedef struct
{
    double xx, xy, yx, yy;
} FftMatrix;
typedef int FftResult;
typedef struct
{
	FftType	type;
	union
	{
		char *s;
		int i;
		Bool b;
		double d;
	} u;
} XftValue;
typedef XftValue FftValue;
typedef struct _XftValueList
{
	struct _XftValueList *next;
	FftValue value;
} XftValueList;
typedef XftValueList FftValueList;
typedef struct
{
	const char *object;
	FftValueList *values;
} XftPatternElt;
typedef XftPatternElt FftPatternElt;
typedef struct
{
	int num;
	int size;
	FftPatternElt *elts;
} XftPattern;
typedef XftPattern FftPattern;
typedef void FftFontSet;
typedef void FftFontVoid;
typedef void FftDraw;
typedef struct
{
	int ascent;
	int descent;
	int height;
	int max_advance_width;
	Bool core;
	FftPattern *pattern;
	union
	{
		struct
		{
			XFontStruct *font;
		} core;
		struct
		{
			void *font;
		} ft;
	} u;
} XftFont;
typedef XftFont FftFont;
typedef struct
{
	unsigned short red;
	unsigned short green;
	unsigned short blue;
	unsigned short alpha;
} XRenderColor;
typedef XRenderColor FRenderColor;
typedef struct
{
	unsigned long pixel;
	FRenderColor color;
} XftColor;
typedef XftColor FftColor;
typedef void FftObjectSet;
#endif

typedef struct
{
	FftFont *fftfont;
	FftFont *tb_fftfont;
	FftFont *bt_fftfont;
	Bool utf8;
} FftFontType;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

void FftGetFontHeights(
	FftFontType *fftf, int *height, int *ascent, int *descent);
void FftGetFontWidths(
	FftFontType *fftf, int *max_char_width, int *min_char_offset);
FftFontType *FftGetFont(Display *dpy, char *fontname);
void FftDrawString(
	Display *dpy, Window win, FftFontType *fftf, GC gc, int x,
	int y, char *str, int len, int is_vertical_string);
int FftTextWidth(FftFontType *fftf, char *str, int len);

/* ---------------------------- global definitions -------------------------- */

#if FftSupport

#define FFT_FAMILY XFT_FAMILY
#define FFT_STYLE XFT_STYLE
#define FFT_SLANT XFT_SLANT
#define FFT_WEIGHT XFT_WEIGHT
#define FFT_SIZE XFT_SIZE
#define FFT_PIXEL_SIZE XFT_PIXEL_SIZE
#define FFT_ENCODING XFT_ENCODING
#define FFT_SPACING XFT_SPACING
#define FFT_FOUNDRY XFT_FOUNDRY
#define FFT_CORE XFT_CORE
#define FFT_ANTIALIAS XFT_ANTIALIAS
#define FFT_XLFD XFT_XLFD
#define FFT_FILE XFT_FILE
#define FFT_INDEX XFT_INDEX
#define FFT_RASTERIZER XFT_RASTERIZER
#define FFT_OUTLINE XFT_OUTLINE
#define FFT_SCALABLE XFT_SCALABLE
#define FFT_RGBA XFT_RGBA
#define FFT_SCALE XFT_SCALE
#define FFT_RENDER XFT_RENDER
#define FFT_CHAR_WIDTH XFT_CHAR_WIDTH
#define FFT_CHAR_HEIGHT XFT_CHAR_HEIGHT
#define FFT_MATRIX XFT_MATRIX
#define FFT_WEIGHT_LIGHT XFT_WEIGHT_LIGHT
#define FFT_WEIGHT_MEDIUM XFT_WEIGHT_MEDIUM
#define FFT_WEIGHT_DEMIBOLD XFT_WEIGHT_DEMIBOLD
#define FFT_WEIGHT_BOLD XFT_WEIGHT_BOLD
#define FFT_WEIGHT_BLACK XFT_WEIGHT_BLACK
#define FFT_SLANT_ROMAN XFT_SLANT_ROMAN
#define FFT_SLANT_ITALIC XFT_SLANT_ITALIC
#define FFT_SLANT_OBLIQUE XFT_SLANT_OBLIQUE
#define FFT_PROPORTIONAL XFT_PROPORTIONAL
#define FFT_MONO XFT_MONO
#define FFT_CHARCELL XFT_CHARCELL
#define FFT_RGBA_NONE XFT_RGBA_NONE
#define FFT_RGBA_RGB XFT_RGBA_RGB
#define FFT_RGBA_BGR XFT_RGBA_BGR

#define FftConfigSubstitute(a) XftConfigSubstitute(a)
#define FftColorAllocName(a,b,c,d,e) XftColorAllocName(a,b,c,d,e)
#define FftColorAllocValue(a,b,c,d,e) XftColorAllocValue(a,b,c,d,e)
#define FftColorFree(a,b,c,d) XftColorFree(a,b,c,d)
#define FftValuePrint(a) XftValuePrint(a)
#define FftValueListPrint(a) XftValueListPrint(a)
#define FftPatternPrint(a) XftPatternPrint(a)
#define FftFontSetPrint(a) XftFontSetPrint(a)
#define FftDefaultHasRender(a) XftDefaultHasRender(a)
#define FftDefaultSet(a,b) XftDefaultSet(a,b)
#define FftDefaultSubstitute(a,b,c) XftDefaultSubstitute(a,b,c)
#define FftDrawCreate(a,b,c,d) XftDrawCreate(a,b,c,d)
#define FftDrawCreateBitmap(a,b) XftDrawCreateBitmap(a,b)
#define FftDrawChange(a,b) XftDrawChange(a,b)
#define FftDrawDestroy(a) XftDrawDestroy(a)
#define FftDrawString8(a,b,c,d,e,f,g) XftDrawString8(a,b,c,d,e,f,g)
#define FftDrawString16(a,b,c,d,e,f,g) XftDrawString16(a,b,c,d,e,f,g)
#define FftDrawString32(a,b,c,d,e,f,g) XftDrawString32(a,b,c,d,e,f,g)
#define FftDrawRect(a,b,c,d,e,f) XftDrawRect(a,b,c,d,e,f)
#define FftDrawSetClip(a,b) XftDrawSetClip(a,b)
#define FftTextExtents8(a,b,c,d,e) XftTextExtents8(a,b,c,d,e)
#define FftTextExtents16(a,b,c,d,e) XftTextExtents16(a,b,c,d,e)
#define FftTextExtents32(a,b,c,d,e) XftTextExtents32(a,b,c,d,e)
#define FftFontMatch(a,b,c,d) XftFontMatch(a,b,c,d)
#define FftFontOpenPattern(a,b) XftFontOpenPattern(a,b)
#define FftFontOpenName(a,b,c) XftFontOpenName(a,b,c)
#define FftFontOpenXlfd(a,b,c) XftFontOpenXlfd(a,b,c)
#define FftFontClose(a,b) XftFontClose(a,b)
#define FftGlyphExists(a,b,c) XftGlyphExists(a,b,c)
#define FftFontSetCreate() XftFontSetCreate()
#define FftFontSetDestroy(a) XftFontSetDestroy(a)
#define FftFontSetAdd(a,b) XftFontSetAdd(a,b)
#define FftInit(a) XftInit(a)
#define FftObjectSetCreate() XftObjectSetCreate()
#define FftObjectSetAdd(a,b) XftObjectSetAdd(a,b)
#define FftObjectSetDestroy(a) XftObjectSetDestroy(a)
#define FftObjectSetVaBuild(a,b) XftObjectSetVaBuild(a,b)
#define FftListFontSets(a,b,c,d) XftListFontSets(a,b,c,d)
#define FftListFontsPatternObjects(a,b,c,d) XftListFontsPatternObjects(a,b,c,d)
#define FftFontSetMatch(a,b,c,d) XftFontSetMatch(a,b,c,d)
#define FftNameParse(a) XftNameParse(a)
#define FftPatternCreate() XftPatternCreate()
#define FftPatternDuplicate(a) XftPatternDuplicate(a)
#define FftValueDestroy(a) XftValueDestroy(a)
#define FftValueListDestroy(a) XftValueListDestroy(a)
#define FftPatternDestroy(a) XftPatternDestroy(a)
#define FftPatternFind(a,b,c) XftPatternFind(a,b,c)
#define FftPatternAdd(a,b,c,d) XftPatternAdd(a,b,c,d)
#define FftPatternGet(a,b,c,d) XftPatternGet(a,b,c,d)
#define FftPatternDel(a,b) XftPatternDel(a,b)
#define FftPatternAddInteger(a,b,c) XftPatternAddInteger(a,b,c)
#define FftPatternAddDouble(a,b,c) XftPatternAddDouble(a,b,c)
#define FftPatternAddString(a,b,c) XftPatternAddString(a,b,c)
#define FftPatternAddBool(a,b,c) XftPatternAddBool(a,b,c)
#define FftPatternAddMatrix(a,b,c) XftPatternAddMatrix(a,b,c)
#define FftPatternGetInteger(a,b,c,d) XftPatternGetInteger(a,b,c,d)
#define FftPatternGetDouble(a,b,c,d) XftPatternGetDouble(a,b,c,d)
#define FftPatternGetString(a,b,c,d) XftPatternGetString(a,b,c,d)
#define FftPatternGetBool(a,b,c,d) XftPatternGetBool(a,b,c,d)
#define FftPatternVaBuild(a,b) XftPatternVaBuild(a,b)
#define FftXlfdParse(a,b,c) XftXlfdParse(a,b,c)
#define FftCoreOpen(a,b) XftCoreOpen(a,b)

/* utf8 functions */
#if FftUtf8Support
#define FftDrawStringUtf8(a,b,c,d,e,f,g) XftDrawStringUtf8(a,b,c,d,e,f,g)
#define FftTextExtentsUtf8(a,b,c,d,e) XftTextExtentsUtf8(a,b,c,d,e)
#else
#define FftDrawStringUtf8(a,b,c,d,e,f,g)
#define FftTextExtentsUtf8(a,b,c,d,e)
#endif

#else

#define FFT_FAMILY ""
#define FFT_STYLE ""
#define FFT_SLANT ""
#define FFT_WEIGHT ""
#define FFT_SIZE ""
#define FFT_PIXEL_SIZE ""
#define FFT_ENCODING ""
#define FFT_SPACING ""
#define FFT_FOUNDRY ""
#define FFT_CORE ""
#define FFT_ANTIALIAS ""
#define FFT_XLFD ""
#define FFT_FILE ""
#define FFT_INDEX ""
#define FFT_RASTERIZER ""
#define FFT_OUTLINE ""
#define FFT_SCALABLE ""
#define FFT_RGBA ""
#define FFT_SCALE ""
#define FFT_RENDER ""
#define FFT_CHAR_WIDTH ""
#define FFT_CHAR_HEIGHT ""
#define FFT_MATRIX ""

#define FFT_WEIGHT_LIGHT 0
#define FFT_WEIGHT_MEDIUM 0
#define FFT_WEIGHT_DEMIBOLD 0
#define FFT_WEIGHT_BOLD 0
#define FFT_WEIGHT_BLACK 0
#define FFT_SLANT_ROMAN 0
#define FFT_SLANT_ITALIC 0
#define FFT_SLANT_OBLIQUE 0
#define FFT_PROPORTIONAL 0
#define FFT_MONO 0
#define FFT_CHARCELL 0
#define FFT_RGBA_NONE 0
#define FFT_RGBA_RGB 0
#define FFT_RGBA_BGR 0

#define FftConfigSubstitute(a) False
#define FftColorAllocName(a,b,c,d,e) False
#define FftColorAllocValue(a,b,c,d,e) False
#define FftColorFree(a,b,c,d)
#define FftValuePrint(a)
#define FftValueListPrint(a)
#define FftPatternPrint(a)
#define FftFontSetPrint(a)
#define FftDefaultHasRender(a) False
#define FftDefaultSet(a,b) False
#define FftDefaultSubstitute(a,b,c)
#define FftDrawCreate(a,b,c,d) NULL
#define FftDrawCreateBitmap(a,b) NULL
#define FftDrawChange(a,b)
#define FftDrawDestroy(a)
#define FftDrawString8(a,b,c,d,e,f,g)
#define FftDrawString16(a,b,c,d,e,f,g)
#define FftDrawString32(a,b,c,d,e,f,g)
#define FftDrawRect(a,b,c,d,e,f)
#define FftDrawSetClip(a,b) False
#define FftTextExtents8(a,b,c,d,e)
#define FftTextExtents16(a,b,c,d,e)
#define FftTextExtents32(a,b,c,d,e)
#define FftFontMatch(a,b,c,d) NULL
#define FftFontOpenPattern(a,b) NULL
#define FftFontOpenName(a,b,c) NULL
#define FftFontOpenXlfd(a,b,c) NULL
#define FftFontClose(a,b)
#define FftGlyphExists(a,b,c) False
#define FftFontSetCreate() NULL
#define FftFontSetDestroy(a)
#define FftFontSetAdd(a,b) False
#define FftInit(a) False
#define FftObjectSetCreate() NULL
#define FftObjectSetAdd(a,b) False
#define FftObjectSetDestroy(a)
#define FftObjectSetVaBuild(a,b) NULL
#define FftListFontSets(a,b,c,d) NULL
#define FftListFontsPatternObjects(a,b,c,d) NULL
#define FftFontSetMatch(a,b,c,d) NULL
#define FftNameParse(a) NULL
#define FftPatternCreate() NULL
#define FftPatternDuplicate(a) NULL
#define FftValueDestroy(a)
#define FftValueListDestroy(a)
#define FftPatternDestroy(a)
#define FftPatternFind(a,b,c) NULL
#define FftPatternAdd(a,b,c,d) False
#define FftPatternGet(a,b,c,d)
#define FftPatternDel(a,b) False
#define FftPatternAddInteger(a,b,c) False
#define FftPatternAddDouble(a,b,c) False
#define FftPatternAddString(a,b,c) False
#define FftPatternAddBool(a,b,c) False
#define FftPatternAddMatrix(a,b,c) False
#define FftPatternGetInteger(a,b,c,d) 0
#define FftPatternGetDouble(a,b,c,d) 0
#define FftPatternGetString(a,b,c,d) 0
#define FftPatternGetBool(a,b,c,d) 0
#define FftPatternVaBuild(a,b) NULL
#define FftXlfdParse(a,b,c) NULL
#define FftCoreOpen(a,b) NULL

/* utf8 functions */
#define FftDrawStringUtf8(a,b,c,d,e,f,g)
#define FftTextExtentsUtf8(a,b,c,d,e)

#endif

/* unsupported functions: */
#if 0
#define FftFontOpen(a,b,...)
#define FftPatternBuild(a, ...)
#define FftObjectSetBuild(a, ...)
#define FftListFonts(a,b,...)
#endif

#endif /* FFT_H */
