/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */

#ifndef FXPM_H
#define FXPM_H

#include "PictureBase.h"

#if XpmSupport
#include <X11/xpm.h>
#endif

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

#if XpmSupport
typedef XpmColorSymbol FxpmColorSymbol;
typedef XpmExtension   FxpmExtension;
typedef XpmColor       FxpmColor;
typedef XpmAttributes  FxpmAttributes;
typedef XpmImage       FxpmImage;
typedef XpmInfo        FxpmInfo;
#else
typedef struct {
	char *name;
	char *value;
	Pixel pixel;
} FxpmColorSymbol;
typedef struct {
	char *name;
	unsigned int nlines;
	char **lines;
} FxpmExtension;
typedef struct {
	char *string;
	char *symbolic;
	char *m_color;
	char *g4_color;
	char *g_color;
	char *c_color;
} FxpmColor;
typedef int (*FxpmAllocColorFunc)(
#ifdef __STDC__
	Display*,Colormap,char*,XColor*,void*
#endif
	);
typedef int (*FxpmFreeColorsFunc)(
#ifdef __STDC__
	Display*,Colormap,Pixel*,int,void*
#endif
	);
typedef struct {
	unsigned long valuemask;
	Visual *visual;
	Colormap colormap;
	unsigned int depth;
	unsigned int width;
	unsigned int height;
	unsigned int x_hotspot;
	unsigned int y_hotspot;
	unsigned int cpp;
	Pixel *pixels;
	unsigned int npixels;
	FxpmColorSymbol *colorsymbols;
	unsigned int numsymbols;
	char *rgb_fname;
	unsigned int nextensions;
	FxpmExtension *extensions;
	unsigned int ncolors;
	FxpmColor *colorTable;
	char *hints_cmt;
	char *colors_cmt;
	char *pixels_cmt;
	unsigned int mask_pixel;
	Bool exactColors;
	unsigned int closeness;
	unsigned int red_closeness;
	unsigned int green_closeness;
	unsigned int blue_closeness;
	int color_key;
	Pixel *alloc_pixels;
	int nalloc_pixels;
	Bool alloc_close_colors;
	int bitmap_format;
	FxpmAllocColorFunc alloc_color;
	FxpmFreeColorsFunc free_colors;
	void *color_closure;
} FxpmAttributes;
typedef struct {
	unsigned int width;
	unsigned int height;
	unsigned int cpp;
	unsigned int ncolors;
	FxpmColor *colorTable;
	unsigned int *data;
} FxpmImage;
typedef struct {
	unsigned long valuemask;
	unsigned int x_hotspot;
	unsigned int y_hotspot;
} FxpmInfo;
#endif

/* ---------------------------- global definitions ------------------------- */

#if XpmSupport

#define FxpmReturnPixels XpmReturnPixels
#define FxpmSize XpmSize
#define FxpmReturnAllocPixels XpmReturnAllocPixels
#define FxpmCloseness XpmCloseness
#define FxpmVisual XpmVisual
#define FxpmColormap XpmColormap
#define FxpmDepth XpmDepth
#define FxpmHotspot XpmHotspot
#define FxpmExtensions XpmExtensions
#define FxpmCharsPerPixel XpmCharsPerPixel
#define FxpmColorSymbols XpmColorSymbols
#define FxpmRgbFilename XpmRgbFilename
#define FxpmInfos XpmInfos
#define FxpmReturnInfos XpmReturnInfos
#define FxpmReturnExtensions XpmReturnExtensions
#define FxpmRGBCloseness XpmRGBCloseness
#define FxpmColorKey XpmColorKey
#define FxpmAllocCloseColors XpmAllocCloseColors
#define FxpmBitmapFormat XpmBitmapFormat
#define FxpmAllocColor XpmAllocColor
#define FxpmFreeColors XpmFreeColors
#define FxpmColorClosure XpmColorClosure
#define FxpmComments XpmComments
#define FxpmReturnComments XpmReturnComments

#define FxpmUndefPixel XpmUndefPixel

#define FxpmColorError XpmColorError
#define FxpmSuccess XpmSuccess
#define FxpmOpenFailed XpmOpenFailed
#define FxpmFileInvalid XpmFileInvalid
#define FxpmNoMemory XpmNoMemory
#define FxpmColorFailed XpmColorFailed

#define FXPM_MONO XPM_MONO
#define FXPM_GREY4 XPM_GREY4
#define FXPM_GRAY4 XPM_GRAY4
#define FXPM_GREY XPM_GREY
#define FXPM_GRAY XPM_GRAY
#define FXPM_COLOR XPM_COLOR

#define FxpmReadFileToXpmImage(a,b,c) XpmReadFileToXpmImage(a,b,c)
#define FxpmCreatePixmapFromXpmImage(a,b,c,d,e,f) \
	    XpmCreatePixmapFromXpmImage(a,b,c,d,e,f)
#define FxpmFreeXpmImage(a) XpmFreeXpmImage(a)
#define FxpmFreeXpmInfo(a) XpmFreeXpmInfo(a)
#define FxpmReadFileToPixmap(a,b,c,d,e,f) XpmReadFileToPixmap(a,b,c,d,e,f)
#define FxpmReadFileToImage(a,b,c,d,e) XpmReadFileToImage(a,b,c,d,e)
#define FxpmCreatePixmapFromData(a,b,c,d,e,f) \
	    XpmCreatePixmapFromData(a,b,c,d,e,f)

#else /* !XpmSupport */
#define FxpmReturnPixels 0
#define FxpmSize 0
#define FxpmReturnAllocPixels 0
#define FxpmCloseness 0
#define FxpmVisual 0
#define FxpmColormap 0
#define FxpmDepth 0
#define FxpmHotspot 0
#define FxpmExtensions 0
#define FxpmCharsPerPixel 0
#define FxpmColorSymbols 0
#define FxpmRgbFilename 0
#define FxpmInfos 0
#define FxpmReturnInfos 0
#define FxpmReturnExtensions 0
#define FxpmRGBCloseness 0
#define FxpmColorKey 0
#define FxpmAllocCloseColors 0
#define FxpmBitmapFormat 0
#define FxpmAllocColor 0
#define FxpmFreeColors 0
#define FxpmColorClosure 0
#define FxpmComments 0
#define FxpmReturnComments 0

#define FxpmUndefPixel 0

#define FxpmColorError    1
#define FxpmSuccess       0
#define FxpmOpenFailed   -1
#define FxpmFileInvalid  -2
#define FxpmNoMemory     -3
#define FxpmColorFailed  -4

#define FXPM_MONO       2
#define FXPM_GREY4      3
#define FXPM_GRAY4      3
#define FXPM_GREY       4
#define FXPM_GRAY       4
#define FXPM_COLOR      5

#define FxpmReadFileToXpmImage(a,b,c) 0
#define FxpmCreatePixmapFromXpmImage(a,b,c,d,e,f) 0
#define FxpmFreeXpmImage(a)
#define FxpmFreeXpmInfo(a)
#define FxpmReadFileToPixmap(a,b,c,d,e,f) 0
#define FxpmCreatePixmapFromData(a,b,c,d,e,f) 0

#endif /* XpmSupport */

#endif /* FXPM_H */
