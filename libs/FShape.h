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

/*
** FShape.h: drop in replacements for the X shape library encapsulation
*/
#ifndef FVWMLIB_FSHAPE_H
#define FVWMLIB_FSHAPE_H

#ifdef SHAPE
#include <X11/extensions/shape.h>
#define FShapeQueryExtension(dpy, evbase, errbase) \
	XShapeQueryExtension(dpy, evbase, errbase)
#define FShapeQueryVersion(dpy, vmajor, vminor) \
	XShapeQueryVersion(dpy, vmajor, vminor)
#define FShapeCombineRegion(dpy, dst, dst_kind, xoff, yoff, reg, op) \
	XShapeCombineRegion(dpy, dst, dst_kind, xoff, yoff, reg, op)
#define FShapeCombineRectangles( \
		dpy, dst, dst_kind, xoff, yoff, rect, n_rects, op, ordering) \
	XShapeCombineRectangles( \
		dpy, dst, dst_kind, xoff, yoff, rect, n_rects, op, ordering)
#define FShapeCombineMask(dpy, dst, dst_kind, xoff, yoff, src, op) \
	XShapeCombineMask(dpy, dst, dst_kind, xoff, yoff, src, op)
#define FShapeCombineShape(dpy, dst, dst_kind, xoff, yoff, src, src_kind, op) \
	XShapeCombineShape(dpy, dst, dst_kind, xoff, yoff, src, src_kind, op)
#define FShapeOffsetShape(dpy, dst, dst_kind, xoff, yoff) \
	XShapeOffsetShape(dpy, dst, dst_kind, xoff, yoff)
#define FShapeQueryExtents( \
		dpy, w, bounding_shaped, xb, yb, wb, hb, clip_shaped, \
		xclip, yclip, wclip, hclip) \
	XShapeQueryExtents( \
		dpy, w, bounding_shaped, xb, yb, wb, hb, clip_shaped, \
		xclip, yclip, wclip, hclip)
#define FShapeSelectInput(dpy, w, mask) \
	XShapeSelectInput(dpy, w, mask)
#define FShapeInputSelected(dpy, w) \
	XShapeInputSelected(dpy, w)
#define FShapeGetRectangles(dpy, w, kind, count, ordering) \
	XShapeGetRectangles(dpy, w, kind, count, ordering)

extern int FShapeEventBase;
extern int FShapeErrorBase;
/* Shapes compiled in? */
extern Bool FShapesSupported;
/* Shapes supported by server? */
#define FHaveShapeExtension 0
void FShapeInit(Display *dpy);

#else
/* drop in replacements if shape support is not compiled in */
#define X_ShapeQueryVersion		0
#define X_ShapeRectangles		1
#define X_ShapeMask			2
#define X_ShapeCombine			3
#define X_ShapeOffset			4
#define X_ShapeQueryExtents		5
#define X_ShapeSelectInput		6
#define X_ShapeInputSelected		7
#define X_ShapeGetRectangles		8
#define ShapeSet			0
#define ShapeUnion			1
#define ShapeIntersect			2
#define ShapeSubtract			3
#define ShapeInvert			4
#define ShapeBounding			0
#define ShapeClip			1
#define ShapeNotifyMask			(1L << 0)
#define ShapeNotify			0
#define ShapeNumberEvents		(FShapeNotify + 1)
typedef struct
{
  int	type;		    /* of event */
  unsigned long serial;   /* # of last request processed by server */
  Bool send_event;	    /* true if this came frome a SendEvent request */
  Display *display;	    /* Display the event was read from */
  Window window;	    /* window of event */
  int kind;		    /* ShapeBounding or ShapeClip */
  int x, y;		    /* extents of new region */
  unsigned width, height;
  Time time;		    /* server timestamp when region changed */
  Bool shaped;	    /* true if the region exists */
} XShapeEvent;
#define FShapeQueryExtension(dpy, evbase, errbase) ((Bool)False)
#define FShapeQueryVersion(dpy, vmajor, vminor) ((Status)0)
#define FShapeCombineRegion(dpy, dst, dst_kind, xoff, yoff, reg, op)
#define FShapeCombineRectangles( \
		dpy, dst, dst_kind, xoff, yoff, rect, n_rects, op, ordering)
#define FShapeCombineMask(dpy, dst, dst_kind, xoff, yoff, src, op)
#define FShapeCombineShape(dpy, dst, dst_kind, xoff, yoff, src, src_kind, op)
#define FShapeOffsetShape(dpy, dst, dst_kind, xoff, yoff)
#define FShapeQueryExtents( \
		dpy, w, bounding_shaped, xb, yb, wb, hb, clip_shaped, \
		xclip, yclip, wclip, hclip) ((Status)0)
#define FShapeSelectInput(dpy, w, mask)
#define FShapeInputSelected(dpy, w) ((unsinged long)0)
#define FShapeGetRectangles(dpy, w, kind, count, ordering) ((XRectangle *)0)
/* define empty dummies */
#define FShapeEventBase          0
#define FShapeErrorBase          0
/* Shapes supported by server? */
#define FShapesSupported         0
/* Shapes compiled in? */
#define FHaveShapeExtension      0
#define FShapeInit(dpy)
#endif

/* fvwm replacements for shape lib */
#define F_ShapeQueryVersion		X_ShapeQueryVersion
#define F_ShapeRectangles		X_ShapeRectangles
#define F_ShapeMask			X_ShapeMask
#define F_ShapeCombine			X_ShapeCombine
#define F_ShapeOffset			X_ShapeOffset
#define F_ShapeQueryExtents		X_ShapeQueryExtents
#define F_ShapeSelectInput		X_ShapeSelectInput
#define F_ShapeInputSelected		X_ShapeInputSelected
#define F_ShapeGetRectangles		X_ShapeGetRectangles
#define FShapeSet			ShapeSet
#define FShapeUnion			ShapeUnion
#define FShapeIntersect			ShapeIntersect
#define FShapeSubtract			ShapeSubtract
#define FShapeInvert			ShapeInvert
#define FShapeBounding			ShapeBounding
#define FShapeClip			ShapeClip
#define FShapeNotifyMask		ShapeNotifyMask
#define FShapeNotify			ShapeNotify
#define FShapeNumberEvents		ShapeNumberEvents
typedef XShapeEvent FShapeEvent;


#endif /* FVWMLIB_FSHAPE_H */
