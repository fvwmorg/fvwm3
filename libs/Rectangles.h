/* -*-c-*- */

/*
 *  Convinence functions for manipulating rectangles, segments, regions ...etc.
 */

#ifndef FRECTANGLES_H
#define FRECTANGLES_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

/* If the two rectangles [x1, y1, w1, h2] and [x2, x2, w2, h2] have a
 * non emtpy interstection this function return True and the
 * XRectangle *r is set to the intersection (if not NULL). If the two
 * rectangles have an emty intersection False is returned and r is not
 * modified.  */
Bool frect_get_intersection(
	int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2,
	XRectangle *r);

/* as above but the input are XRectangles */
Bool frect_get_rect_intersection(
	XRectangle a, XRectangle b, XRectangle *r);

/* as above for two segments */
Bool frect_get_seg_intersection(
	int x1, int w1, int x2, int w2, int *x, int *w);

#endif
