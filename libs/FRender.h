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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef FRENDER_H
#define FRENDER_H

/* ---------------------------- included header files ----------------------- */

#include "config.h"
#include "PictureBase.h"

#if XRenderSupport
#define Picture XRenderPicture
#include <X11/extensions/Xrender.h>
#undef Picture
#endif

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

#if XRenderSupport
typedef XRenderDirectFormat FRenderDirectFormat;
typedef PictFormat FRPictFormat;
typedef XRenderPictFormat FRenderPictFormat;
typedef XRenderPicture FRenderPicture;
typedef XRenderVisual FRenderVisual;
typedef XRenderDepth FRenderDepth;
typedef XRenderInfo FRenderInfo;
typedef XRenderPictureAttributes FRenderPictureAttributes;
typedef XRenderColor FRenderColor;
typedef XGlyphInfo FGlyphInfo;

#define FRenderPictFormatID	   PictFormatID
#define FRenderPictFormatType	   PictFormatType
#define FRenderPictFormatDepth	   PictFormatDepth
#define FRenderPictFormatRed	   PictFormatRed
#define FRenderPictFormatRedMask   PictFormatRedMask
#define FRenderPictFormatGreen	   PictFormatGreen
#define FRenderPictFormatGreenMask PictFormatGreenMask
#define FRenderPictFormatBlue	   PictFormatBlue
#define FRenderPictFormatBlueMask  PictFormatBlueMask
#define FRenderPictFormatAlpha	   PictFormatAlpha
#define FRenderPictFormatAlphaMask PictFormatAlphaMask
#define FRenderPictFormatColormap  PictFormatColormap

#define FRenderBadPictFormat		    0
#define FRenderBadPicture		    1
#define FRenderBadPictOp		    2
#define FRenderBadGlyphSet		    3
#define FRenderBadGlyph			    4
#define FRenderRenderNumberErrors	    (FRenderBadGlyph+1)

#define FRenderPictTypeIndexed PictTypeIndexed
#define FRenderPictTypeDirect  PictTypeDirect

#define FRenderPictOpMinimum PictOpMinimum
#define FRenderPictOpClear PictOpClear
#define FRenderPictOpSrc PictOpSrc
#define FRenderPictOpDst PictOpDst
#define FRenderPictOpOver PictOpOver
#define FRenderPictOpOverReverse PictOpOverReverse
#define FRenderPictOpIn PictOpIn
#define FRenderPictOpInReverse PictOpInReverse
#define FRenderPictOpOut PictOpOut
#define FRenderPictOpOutReverse PictOpOutReverse
#define FRenderPictOpAtop PictOpAtop
#define FRenderPictOpAtopReverse PictOpAtopReverse
#define FRenderPictOpXor PictOpXor
#define FRenderPictOpAdd PictOpAdd
#define FRenderPictOpSaturate PictOpSaturate
#define FRenderPictOpMaximum PictOpMaximum
/*
 * Operators only available in version 0.2
 */
#if 0
#define FRenderPictOpDisjointMinimum	 PictOpDisjointMinimum
#define FRenderPictOpDisjointClear	 PictOpDisjointClear
#define FRenderPictOpDisjointSrc	 PictOpDisjointSrc
#define FRenderPictOpDisjointDst	 PictOpDisjointDst
#define FRenderPictOpDisjointOver	 PictOpDisjointOver
#define FRenderPictOpDisjointOverReverse PictOpDisjointOverReverse
#define FRenderPictOpDisjointIn		 PictOpDisjointIn
#define FRenderPictOpDisjointInReverse	 PictOpDisjointInReverse
#define FRenderPictOpDisjointOut	 PictOpDisjointOut
#define FRenderPictOpDisjointOutReverse	 PictOpDisjointOutReverse
#define FRenderPictOpDisjointAtop	 PictOpDisjointAtop
#define FRenderPictOpDisjointAtopReverse PictOpDisjointAtopReverse
#define FRenderPictOpDisjointXor	 PictOpDisjointXor
#define FRenderPictOpDisjointMaximum	 PictOpDisjointMaximum

#define FRenderPictOpConjointMinimum	 PictOpConjointMinimum
#define FRenderPictOpConjointClear	 PictOpConjointClear
#define FRenderPictOpConjointSrc	 PictOpConjointSrc
#define FRenderPictOpConjointDst	 PictOpConjointDst
#define FRenderPictOpConjointOver	 PictOpConjointOver
#define FRenderPictOpConjointOverReverse PictOpConjointOverReverse
#define FRenderPictOpConjointIn		 PictOpConjointIn
#define FRenderPictOpConjointInReverse	 PictOpConjointInReverse
#define FRenderPictOpConjointOut	 PictOpConjointOut
#define FRenderPictOpConjointOutReverse	 PictOpConjointOutReverse
#define FRenderPictOpConjointAtop	 PictOpConjointAtop
#define FRenderPictOpConjointAtopReverse PictOpConjointAtopReverse
#define FRenderPictOpConjointXor	 PictOpConjointXor
#define FRenderPictOpConjointMaximum	 PictOpConjointMaximum
#endif /* 0 */

#define FRenderPolyEdgeSharp  PolyEdgeSharp
#define FRenderPolyEdgeSmooth PolyEdgeSmooth

#define FRenderPolyModePrecise
#define FRenderPolyModeImprecise

#define FRenderCPRepeat		   CPRepeat
#define FRenderCPAlphaMap	   CPAlphaMap
#define FRenderCPAlphaXOrigin	   CPAlphaXOrigin
#define FRenderCPAlphaYOrigin	   CPAlphaYOrigin
#define FRenderCPClipXOrigin	   CPClipXOrigin
#define FRenderCPClipYOrigin	   CPClipYOrigin
#define FRenderCPClipMask	   CPClipMask
#define FRenderCPGraphicsExposure  CPGraphicsExposure
#define FRenderCPSubwindowMode	   CPSubwindowMode
#define FRenderCPPolyEdge	   CPPolyEdge
#define FRenderCPPolyMode	   CPPolyMode
#define FRenderCPDither		   CPDither
#define FRenderCPComponentAlpha	   CPComponentAlpha
#define FRenderCPLastBit	   CPLastBit

#define FRenderQueryExtension(a,b,c) XRenderQueryExtension(a,b,c)
#define FRenderQueryVersion(a,b,c) XRenderQueryVersion(a,b,c)
#define FRenderQueryFormats(a) XRenderQueryFormats(a)
#define FRenderFindVisualFormat(a,b) XRenderFindVisualFormat(a,b)
#define FRenderFindFormat(a,b,c,d) XRenderFindFormat(a,b,c,d)
#define FRenderCreatePicture(a,b,c,d,e) XRenderCreatePicture(a,b,c,d,e)
#define FRenderChangePicture(a,b,c,d) XRenderChangePicture(a,b,c,d)
#define FRenderSetPictureClipRectangles(a,b,c,d,e,f) \
	    XRenderSetPictureClipRectangles(a,b,c,d,e,f)
#define FRenderSetPictureClipRegion(a,b,c) XRenderSetPictureClipRegion(a,b,c)
#define FRenderFreePicture(a,b) XRenderFreePicture(a,b)
#define FRenderComposite(a,b,c,d,e,f,g,h,i,j,k,l,m) \
	    XRenderComposite(a,b,c,d,e,f,g,h,i,j,k,l,m)
#define FRenderFillRectangle(a,b,c,d,e,f,g,h) \
	    XRenderFillRectangle(a,b,c,d,e,f,g,h)
#define FRenderFillRectangles(a,b,c,d,e,f) XRenderFillRectangles(a,b,c,d,e,f)

#else /* !XRenderSupport */

typedef unsigned long	FRenderPicture;
typedef unsigned long	FRPictFormat;
typedef struct {
    short   red;
    short   redMask;
    short   green;
    short   greenMask;
    short   blue;
    short   blueMask;
    short   alpha;
    short   alphaMask;
} FRenderDirectFormat;
typedef struct {
    FRPictFormat	id;
    int			type;
    int			depth;
    FRenderDirectFormat	direct;
    Colormap		colormap;
} FRenderPictFormat;

typedef struct {
    Visual		*visual;
    FRenderPictFormat	*format;
} FRenderVisual;
typedef struct {
    int			depth;
    int			nvisuals;
    FRenderVisual	*visuals;
} FRenderDepth;

typedef struct {
    FRenderDepth	*depths;
    int			ndepths;
    FRenderPictFormat	*fallback;
} FRenderScreen;
typedef struct _FRenderInfo {
    FRenderPictFormat	*format;
    int			nformat;
    FRenderScreen	*screen;
    int			nscreen;
    FRenderDepth	*depth;
    int			ndepth;
    FRenderVisual	*visual;
    int			nvisual;
} FRenderInfo;

typedef struct _FRenderPictureAttributes {
    Bool		repeat;
    FRenderPicture	alpha_map;
    int			alpha_x_origin;
    int			alpha_y_origin;
    int			clip_x_origin;
    int			clip_y_origin;
    Pixmap		clip_mask;
    Bool		graphics_exposures;
    int			subwindow_mode;
    int			poly_edge;
    int			poly_mode;
    Atom		dither;
    Bool		component_alpha;
} FRenderPictureAttributes;
typedef struct {
    unsigned short   red;
    unsigned short   green;
    unsigned short   blue;
    unsigned short   alpha;
} FRenderColor;
typedef struct _FGlyphInfo {
    unsigned short  width;
    unsigned short  height;
    short	    x;
    short	    y;
    short	    xOff;
    short	    yOff;
} FGlyphInfo;

#define FRenderPictFormatID 0
#define FRenderPictFormatType  0
#define FRenderPictFormatDepth 0
#define FRenderPictFormatRed 0
#define FRenderPictFormatRedMask 0
#define FRenderPictFormatGreen 0
#define FRenderPictFormatGreenMask 0
#define FRenderPictFormatBlue 0
#define FRenderPictFormatBlueMask 0
#define FRenderPictFormatAlpha 0
#define FRenderPictFormatAlphaMask 0
#define FRenderPictFormatColormap 0

#define FRenderBadPictFormat		    0
#define FRenderBadPicture		    1
#define FRenderBadPictOp		    2
#define FRenderBadGlyphSet		    3
#define FRenderBadGlyph			    4
#define FRenderRenderNumberErrors	    (FRenderBadGlyph+1)

#define FRenderPictTypeIndexed 0
#define FRenderPictTypeDirect 0

#define FRenderPictOpMinimum 0
#define FRenderPictOpClear 0
#define FRenderPictOpSrc 0
#define FRenderPictOpDst 0
#define FRenderPictOpOver 0
#define FRenderPictOpOverReverse 0
#define FRenderPictOpIn 0
#define FRenderPictOpInReverse 0
#define FRenderPictOpOut 0
#define FRenderPictOpOutReverse 0
#define FRenderPictOpAtop 0
#define FRenderPictOpAtopReverse 0
#define FRenderPictOpXor 0
#define FRenderPictOpAdd 0
#define FRenderPictOpSaturate 0
#define FRenderPictOpMaximum 0
/*
 * Operators only available in version 0.2
 */
#if 0
#define FRenderPictOpDisjointMinimum 0
#define FRenderPictOpDisjointClear 0
#define FRenderPictOpDisjointSrc 0
#define FRenderPictOpDisjointDst 0
#define FRenderPictOpDisjointOver 0
#define FRenderPictOpDisjointOverReverse 0
#define FRenderPictOpDisjointIn 0
#define FRenderPictOpDisjointInReverse 0
#define FRenderPictOpDisjointOut 0
#define FRenderPictOpDisjointOutReverse 0
#define FRenderPictOpDisjointAtop 0
#define FRenderPictOpDisjointAtopReverse 0
#define FRenderPictOpDisjointXor 0
#define FRenderPictOpDisjointMaximum 0

#define FRenderPictOpConjointMinimum 0
#define FRenderPictOpConjointClear 0
#define FRenderPictOpConjointSrc 0
#define FRenderPictOpConjointDst 0
#define FRenderPictOpConjointOver 0
#define FRenderPictOpConjointOverReverse 0
#define FRenderPictOpConjointIn 0
#define FRenderPictOpConjointInReverse 0
#define FRenderPictOpConjointOut 0
#define FRenderPictOpConjointOutReverse 0
#define FRenderPictOpConjointAtop 0
#define FRenderPictOpConjointAtopReverse 0
#define FRenderPictOpConjointXor 0
#define FRenderPictOpConjointMaximum 0
#endif

#define FRenderPolyEdgeSharp 0
#define FRenderPolyEdgeSmooth 0

#define FRenderPolyModePrecise 0
#define FRenderPolyModeImprecise 0

#define FRenderCPRepeat 0
#define FRenderCPAlphaMap 0
#define FRenderCPAlphaXOrigin 0
#define FRenderCPAlphaYOrigin 0
#define FRenderCPClipXOrigin 0
#define FRenderCPClipYOrigin 0
#define FRenderCPClipMask 0
#define FRenderCPGraphicsExposure 0
#define FRenderCPSubwindowMode 0
#define FRenderCPPolyEdge 0
#define FRenderCPPolyMode 0
#define FRenderCPDither 0
#define FRenderCPComponentAlpha 0
#define FRenderCPLastBit 0

#define FRenderQueryExtension(a,b,c) 0
#define FRenderQueryVersion(a,b,c) 0
#define FRenderQueryFormats(a) 0
#define FRenderFindVisualFormat(a,b) NULL
#define FRenderFindFormat(a,b,c,d) NULL
#define FRenderCreatePicture(a,b,c,d,e) None
#define FRenderChangePicture(a,b,c,d)
#define FRenderSetPictureClipRectangles(a,b,c,d,e,f)
#define FRenderSetPictureClipRegion(a,b,c)
#define FRenderFreePicture(a,b)
#define FRenderComposite(a,b,c,d,e,f,g,h,i,j,k,l,m)
#define FRenderFillRectangle(a,b,c,d,e,f,g,h)
#define FRenderFillRectangles(a,b,c,d,e,f)
#endif

#endif /* FRENDER_H */
