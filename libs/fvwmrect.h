/* -*-c-*- */
/* Copyright (C) 2001  Dominik Vogt */

#ifndef FVWMRECT_H
#define FVWMRECT_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef struct
{
	int x;
	int y;
	int width;
	int height;
} signed_rectangle;

typedef struct
{
	int x;
	int y;
	unsigned int width;
	unsigned int height;
} rectangle;

typedef struct
{
	int x;
	int y;
} position;

typedef struct
{
	unsigned int width;
	unsigned int height;
} size_rect;

typedef struct
{
	size_rect top_left;
	size_rect bottom_right;
	size_rect total_size;
} size_borders;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

/* Returns 1 if the given rectangles intersect and 0 otherwise */
int fvwmrect_do_rectangles_intersect(rectangle *r, rectangle *s);
/* Subtracts the values in s2_ from the ones in s1_g and stores the result in
 * diff_g. */
void fvwmrect_subtract_rectangles(
	signed_rectangle *rdiff, rectangle *r1, rectangle *r2);
/* Returns 1 is the rectangles are identical and 0 if not */
int fvwmrect_rectangles_equal(
	rectangle *r1, rectangle *r2);
int fvwmrect_move_into_rectangle(
	rectangle *move_rec, rectangle *target_rec);
int fvwmrect_intersect_xrectangles(
	XRectangle *r1, XRectangle *r2);

#endif /* FVWMRECT_H */
