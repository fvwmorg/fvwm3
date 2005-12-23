/* -*-c-*- */

#ifndef FFT_H
#define FFT_H

/*
 * Note: This "warper" is far from being complete! if you need something
 * in Xft or Xft + fontconfig (Fc for short) (aka Xft2) not already defined
 * you must add the corresponding Fft* functions/types (2 or 3 times).
 *
 */

/* ---------------------------- included header files ---------------------- */

/* no compat to avoid problems in the future */
#define _XFT_NO_COMPAT_ 1

#ifdef HAVE_XFT
#define Picture XRenderPicture
#include <X11/Xft/Xft.h>
#undef Picture
#endif

#include "FRender.h"

/* ---------------------------- global definitions ------------------------- */

#ifdef HAVE_XFT

#define FftSupport 1

#ifdef HAVE_XFT_UTF8
#define FftUtf8Support 1
#else
#define FftUtf8Support 0
#endif

#ifdef HAVE_XFT2
#define FftSupportUseXft2 1
#else
#define FftSupportUseXft2 0
#endif

#else

#define FftSupport 0
#define FftUtf8Support 0
#define FftSupportUseXft2 0

#endif

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

#if FftSupport

/* Fc (fontconfig) stuff */
#if FftSupportUseXft2

typedef FcChar8     FftChar8;
typedef FcChar16    FftChar16;
typedef FcChar32    FftChar32;
typedef FcBool      FftBool;
typedef FcType      FftType;
typedef FcMatrix    FftMatrix;
typedef FcValue     FftValue;
typedef FcPattern   FftPattern;
typedef FcFontSet   FftFontSet;
typedef FcObjectSet FftObjectSet;
#if 1
/* FftResult must be FcResult for gcc 3.4 not to warn of incompatible
 * pointer type
 */
typedef FcResult    FftResult;

#define FftResultMatch        FcResultMatch
#define FftResultNoMatch      FcResultNoMatch
#define FftResultTypeMismatch FcResultTypeMismatch
#define FftFftResultNoId      FcResultNoId

#else
typedef enum _FftResult
{
	FftResultMatch        = FcResultMatch,
	FftResultNoMatch      = FcResultNoMatch,
	FftResultTypeMismatch = FcResultTypeMismatch,
	FftFftResultNoId	    = FcResultNoId
} FftResult;
#endif
/* XftValue and are different in Xft+Fc and Xft 1 */
typedef struct _Xft1Value
{
	FftType     type;
	union {
		char    *s;
		int     i;
		Bool    b;
		double  d;
		FftMatrix *m;
	} u;
} Xft1Value;
typedef Xft1Value Fft1Value;
/* no value list in Xft+Fc */
typedef struct _XftValueList
{
	struct _XftValueList *next;
	Fft1Value value;
} XftValueList;
typedef XftValueList FftValueList;
/* no  XftPatternElt in Xft+Fc */
typedef struct
{
	const char *object;
	FftValueList *values;
} XftPatternElt;
typedef XftPatternElt FftPatternElt;
/* XftPattern and FcPattern are different */
typedef struct _Xft1Pattern
{
	int             num;
	int             size;
	FftPatternElt   *elts;
} Fft1Pattern;

#else /* !FftSupportUseXft2 */

typedef XftChar8     FftChar8;
typedef XftChar16    FftChar16;
typedef XftChar32    FftChar32;
typedef XftType      FftType;
typedef XftMatrix    FftMatrix;
typedef XftPatternElt FftPatternElt;
typedef XftFontSet   FftFontSet;
typedef XftObjectSet FftObjectSet;
typedef XftValue     FftValue;
typedef XftPattern   FftPattern;
typedef XftValue     Fft1Value;
typedef XftPattern   Fft1Pattern;

typedef enum _FftResult
{
	FftResultMatch        = XftResultMatch,
	FftResultNoMatch      = XftResultNoMatch,
	FftResultTypeMismatch = XftResultTypeMismatch,
	FftFftResultNoId	    = XftResultNoId
} FftResult;

#endif /* FftSupportUseXft2 */

/* Xft stuff (common) */
typedef XftDraw FftDraw;
typedef XftFont FftFont;
typedef XftColor FftColor;

#if FftSupportUseXft2
/* XftFont are != in Xft+Fc and Xft */
typedef struct _Fft1Font
{
	int         ascent;
	int         descent;
	int         height;
	int         max_advance_width;
	Bool        core;
	Fft1Pattern *pattern;
	union {
		struct {
			XFontStruct     *font;
		} core;
		struct {
			void *font;
		} ft;
	} u;
} Fft1Font;
#else
typedef XftFont Fft1Font;
#endif

#else

typedef unsigned char FftChar8;
typedef unsigned short FftChar16;
typedef unsigned int FftChar32;
typedef int FftType;
typedef struct
{
	double xx, xy, yx, yy;
} FftMatrix;
typedef enum _FftResult {
	FftResultMatch        = 0,
	FftResultNoMatch      = 1,
	FftResultTypeMismatch = 2,
	FftFftResultNoId	    = 3
} FftResult;
typedef struct
{
	FftType type;
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
	unsigned long pixel;
	FRenderColor color;
} XftColor;
typedef XftColor FftColor;
typedef void FftObjectSet;
/* XftValue are different in Xft+Fc and Xft 1 */
typedef struct _Xft1Value
{
	FftType     type;
	union
	{
		char    *s;
		int     i;
		Bool    b;
		double  d;
		FftMatrix *m;
	} u;
} Xft1Value;
typedef Xft1Value Fft1Value;
/* XftPattern and XftFont are different in Xft+Fc and Xft 1 */
typedef struct _Xft1Pattern
{
	int             num;
	int             size;
	FftPatternElt   *elts;
} Fft1Pattern;

typedef struct _Fft1Font
{
	int         ascent;
	int         descent;
	int         height;
	int         max_advance_width;
	Bool        core;
	Fft1Pattern *pattern;
	union
	{
		struct
		{
			XFontStruct     *font;
		} core;
		struct
		{
			void *font;
		} ft;
	} u;
} Fft1Font;
#endif


/*
 * Fvwm Xft font structure
 */

typedef struct
{
	FftFont *fftfont;
	FftFont *fftfont_rotated_90;
	FftFont *fftfont_rotated_180;
	FftFont *fftfont_rotated_270;
	char *encoding;
	char *str_encoding;
} FftFontType;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void FftPDumyFunc(void);

/* ---------------------------- global definitions ------------------------- */

#if FftSupport

/* Fc stuff */
#if FftSupportUseXft2
#define FFT_FAMILY          FC_FAMILY
#define FFT_STYLE           FC_STYLE
#define FFT_SLANT           FC_SLANT
#define FFT_WEIGHT          FC_WEIGHT
#define FFT_SIZE            FC_SIZE
#define FFT_PIXEL_SIZE      FC_PIXEL_SIZE
#define FFT_SPACING         FC_SPACING
#define FFT_FOUNDRY         FC_FOUNDRY
#define FFT_CORE            FC_CORE
#define FFT_ANTIALIAS       FC_ANTIALIAS
#define FFT_XLFD            FC_XLFD
#define FFT_FILE            FC_FILE
#define FFT_INDEX           FC_INDEX
#define FFT_RASTERIZER      FC_RASTERIZER
#define FFT_OUTLINE         FC_OUTLINE
#define FFT_SCALABLE        FC_SCALABLE
#define FFT_RGBA            FC_RGBA
#define FFT_SCALE           FC_SCALE
#define FFT_RENDER          FC_RENDER
#define FFT_CHAR_WIDTH      FC_CHAR_WIDTH
#define FFT_CHAR_HEIGHT     FC_CHAR_HEIGHT
#define FFT_MATRIX          FC_MATRIX
#define FFT_WEIGHT_LIGHT    FC_WEIGHT_LIGHT
#define FFT_WEIGHT_MEDIUM   FC_WEIGHT_MEDIUM
#define FFT_WEIGHT_DEMIBOLD FC_WEIGHT_DEMIBOLD
#define FFT_WEIGHT_BOLD     FC_WEIGHT_BOLD
#define FFT_WEIGHT_BLACK    FC_WEIGHT_BLACK
#define FFT_SLANT_ROMAN     FC_SLANT_ROMAN
#define FFT_SLANT_ITALIC    FC_SLANT_ITALIC
#define FFT_SLANT_OBLIQUE   FC_SLANT_OBLIQUE
#define FFT_PROPORTIONAL    FC_PROPORTIONAL
#define FFT_MONO            FC_MONO
#define FFT_CHARCELL        FC_CHARCELL
#define FFT_RGBA_NONE       FC_RGBA_NONE
#define FFT_RGBA_RGB        FC_RGBA_RGB
#define FFT_RGBA_BGR        FC_RGBA_BGR

/* new in Fc */
#define FFT_HINTING         FC_HINTING
#define FFT_VERTICAL_LAYOUT FC_VERTICAL_LAYOUT
#define FFT_AUTOHINT        FC_AUTOHINT
#define FFT_GLOBAL_ADVANCE  FC_GLOBAL_ADVANCE
#define FFT_SOURCE          FC_SOURCE
#define FFT_CHARSET         FC_CHARSET
#define FFT_LANG            FC_LANG
#define FFT_DIR_CACHE_FILE  FC_DIR_CACHE_FILE
#define FFT_USER_CACHE_FILE FC_USER_CACHE_FILE
/* skip all the FC_LANG_* OS/2 CodePageRange bits */

/* not in Fc */
#define FFT_ENCODING        "encoding"

#else /* !FftSupportUseXft2 */

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
#define FFT_RGBA_VRGB XFT_RGBA_VRGB
#define FFT_RGBA_VBGR XFT_RGBA_VBGR

/* new in Fc */
#define FFT_HINTING         ""
#define FFT_VERTICAL_LAYOUT ""
#define FFT_AUTOHINT        ""
#define FFT_GLOBAL_ADVANCE  ""
#define FFT_SOURCE          ""
#define FFT_CHARSET         ""
#define FFT_LANG            ""
#define FFT_DIR_CACHE_FILE  ""
#define FFT_USER_CACHE_FILE ""
/* skip all the FC_LANG_* OS/2 CodePageRange bits */

/* not in Fc */
#define FFT_ENCODING        XFT_ENCODING

#endif /* !FftSupportUseXft2 */

/* Fc stuff */
#if FftSupportUseXft2
#define FftConfigSubstitute(a) FcConfigSubstitute(a)
#define FftValuePrint(a) FcValuePrint(a)
#define FftPatternPrint(a) FcPatternPrint(a)
#define FftFontSetPrint(a) FcFontSetPrint(a)
#define FftGlyphExists(a,b,c) FcGlyphExists(a,b,c)
#define FftObjectSetCreate() FcObjectSetCreate()
#define FftObjectSetAdd(a,b) FcObjectSetAdd(a,b)
#define FftObjectSetDestroy(a) FcObjectSetDestroy(a)
#define FftObjectSetVaBuild(a,b) FcObjectSetVaBuild(a,b)
#define FftListFontsPatternObjects(a,b,c,d) FcListFontsPatternObjects(a,b,c,d)
#define FftFontSetMatch(a,b,c,d) FcFontSetMatch(a,b,c,d)
#define FftPatternCreate() FcPatternCreate()
#define FftPatternDuplicate(a) FcPatternDuplicate(a)
#define FftValueDestroy(a) FcValueDestroy(a)
#define FftValueListDestroy(a) FcValueListDestroy(a)
#define FftPatternDestroy(a) FcPatternDestroy(a)
#define FftPatternFind(a,b,c) FcPatternFind(a,b,c)
#define FftPatternAdd(a,b,c,d) FcPatternAdd(a,b,c,d)
#define FftPatternGet(a,b,c,d) FcPatternGet(a,b,c,d)
#define FftPatternDel(a,b) FcPatternDel(a,b)
#define FftPatternAddInteger(a,b,c) FcPatternAddInteger(a,b,c)
#define FftPatternAddDouble(a,b,c) FcPatternAddDouble(a,b,c)
#define FftPatternAddString(a,b,c) FcPatternAddString(a,b,c)
#define FftPatternAddBool(a,b,c) FcPatternAddBool(a,b,c)
#define FftPatternAddMatrix(a,b,c) FcPatternAddMatrix(a,b,c)
#define FftPatternGetInteger(a,b,c,d) FcPatternGetInteger(a,b,c,d)
#define FftPatternGetDouble(a,b,c,d) FcPatternGetDouble(a,b,c,d)
#define FftPatternGetString(a,b,c,d) FcPatternGetString(a,b,c,d)
#define FftPatternGetBool(a,b,c,d) FcPatternGetBool(a,b,c,d)
#define FftPatternGetMatrix(a,b,c,d) FcPatternGetMatrix(a,b,c,d)
#define FftPatternVaBuild(a,b) FcPatternVaBuild(a,b)
#else /* !FftSupportUseXft2 */
#define FftConfigSubstitute(a) XftConfigSubstitute(a)
#define FftValuePrint(a) XftValuePrint(a)
#define FftPatternPrint(a) XftPatternPrint(a)
#define FftFontSetPrint(a) XftFontSetPrint(a)
#define FftGlyphExists(a,b,c) XftGlyphExists(a,b,c)
#define FftObjectSetCreate() XftObjectSetCreate()
#define FftObjectSetAdd(a,b) XftObjectSetAdd(a,b)
#define FftObjectSetDestroy(a) XftObjectSetDestroy(a)
#define FftObjectSetVaBuild(a,b) XftObjectSetVaBuild(a,b)
#define FftListFontsPatternObjects(a,b,c,d) XftListFontsPatternObjects(a,b,c,d)
#define FftFontSetMatch(a,b,c,d) XftFontSetMatch(a,b,c,d)
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
#define FftPatternGetMatrix(a,b,c,d) XftPatternGetMatrix(a,b,c,d)
#define FftPatternVaBuild(a,b) XftPatternVaBuild(a,b)
#endif /* !XftSupportUseXft2 */

/* Xft stuff */
#define FftColorAllocName(a,b,c,d,e) XftColorAllocName(a,b,c,d,e)
#define FftColorAllocValue(a,b,c,d,e) XftColorAllocValue(a,b,c,d,e)
#define FftColorFree(a,b,c,d) XftColorFree(a,b,c,d)
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
#define FftPDrawString8  XftDrawString8
#define FftPDrawString16 XftDrawString16
#define FftPDrawString32 XftDrawString32
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
#define FftFontSetCreate() XftFontSetCreate()
#define FftFontSetAdd(a,b) XftFontSetAdd(a,b)
#define FftInit(a) XftInit(a)
#define FftListFontSets(a,b,c,d) XftListFontSets(a,b,c,d)
#define FftNameParse(a) XftNameParse(a)
#define FftXlfdParse(a,b,c) XftXlfdParse(a,b,c)
#define FftCoreOpen(a,b) XftCoreOpen(a,b)

/* utf8 functions */
#if FftUtf8Support
#define FftDrawStringUtf8(a,b,c,d,e,f,g) XftDrawStringUtf8(a,b,c,d,e,f,g)
#define FftPDrawStringUtf8 XftDrawStringUtf8
#define FftTextExtentsUtf8(a,b,c,d,e) XftTextExtentsUtf8(a,b,c,d,e)
#else
#define FftDrawStringUtf8(a,b,c,d,e,f,g)
#define FftPDrawStringUtf8 FftPDumyFunc
#define FftTextExtentsUtf8(a,b,c,d,e)
#endif

#else /* !FftSupport */

#define FFT_FAMILY ""
#define FFT_STYLE ""
#define FFT_SLANT ""
#define FFT_WEIGHT ""
#define FFT_SIZE ""
#define FFT_PIXEL_SIZE ""
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
#define FFT_RGBA_VRGB 0
#define FFT_RGBA_VBGR 0
/* new in Fc */
#define FFT_HINTING         ""
#define FFT_VERTICAL_LAYOUT ""
#define FFT_AUTOHINT        ""
#define FFT_GLOBAL_ADVANCE  ""
#define FFT_SOURCE          ""
#define FFT_CHARSET         ""
#define FFT_LANG            ""
#define FFT_DIR_CACHE_FILE  ""
#define FFT_USER_CACHE_FILE ""
/* skip all the FC_LANG_* OS/2 CodePageRange bits */
/* not in Fc */
#define FFT_ENCODING        ""

/* Fc stuff */
#define FftConfigSubstitute(a) False
#define FftValuePrint(a)
#define FftPatternPrint(a)
#define FftFontSetPrint(a)
#define FftGlyphExists(a,b,c) False
#define FftObjectSetCreate() NULL
#define FftObjectSetAdd(a,b) False
#define FftObjectSetDestroy(a)
#define FftObjectSetVaBuild(a,b) NULL
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
#define FftPatternGetMatrix(a,b,c,d) 0
#define FftPatternVaBuild(a,b) NULL

/* Xft stuff */
#define FftColorAllocName(a,b,c,d,e) False
#define FftColorAllocValue(a,b,c,d,e) False
#define FftColorFree(a,b,c,d)
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
#define FftPDrawString8 FftPDumyFunc
#define FftPDrawString16 FftPDumyFunc
#define FftPDrawString32 FftPDumyFunc
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
#define FftFontSetCreate() NULL
#define FftFontSetDestroy(a)
#define FftFontSetAdd(a,b) False
#define FftInit(a) False
#define FftListFontSets(a,b,c,d) NULL
#define FftNameParse(a) NULL
#define FftXlfdParse(a,b,c) NULL
#define FftCoreOpen(a,b) NULL

/* utf8 functions */
#define FftDrawStringUtf8(a,b,c,d,e,f,g)
#define FftPDrawStringUtf8 FftPDumyFunc
#define FftTextExtentsUtf8(a,b,c,d,e)

#endif

#endif /* FFT_H */
