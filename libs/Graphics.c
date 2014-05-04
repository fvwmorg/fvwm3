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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/* Graphics.c: misc convenience functions for drawing stuff  */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <X11/Xlib.h>
#include <stdio.h>
#include <math.h>

#include "defaults.h"
#include "libs/fvwmlib.h"
#include "libs/Parse.h"
#include "libs/PictureBase.h"
#include "libs/PictureUtils.h"
#include "libs/PictureGraphics.h"
#include "libs/gravity.h"
#include "libs/FImage.h"
#include "libs/Graphics.h"

/* ---------------------------- local definitions -------------------------- */

/* Define some standard constants that are not included in the C89 standard */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#ifndef M_1_PI
#define M_1_PI 0.31830988618379067154
#endif

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

/* Draws the relief pattern around a window
 * Draws a line_width wide rectangle from (x,y) to (x+w,y+h) i.e w+1 wide,
 * h+1 high
 * Draws end points assuming CAP_NOT_LAST style in GC
 * Draws anti-clockwise in case CAP_BUTT is the style and the end points overlap
 * Top and bottom lines come out full length, the sides come out 1 pixel less
 * This is so FvwmBorder windows have a correct bottom edge and the sticky lines
 * look like just lines
 * rotation rotate the relief and shadow part
 */
void do_relieve_rectangle_with_rotation(
	Display *dpy, Drawable d, int x, int y, int w, int h,
	GC ReliefGC, GC ShadowGC, int line_width, Bool use_alternate_shading,
	int rotation)
{
	XSegment* seg;
	GC shadow_gc, relief_gc;
	int i,i2;
	int a;
	int l;
	int max_w;
	int max_h;

	a = (use_alternate_shading) ? 1 : 0;
	l = 1 - a;
	if (w <= 0 || h <= 0)
	{
		return;
	}
	/* If line_width is negative, reverse the rotation, which will */
	/* have the effect of inverting the relief. */
	if (line_width < 0)
	{
		line_width = -line_width;
		rotation = gravity_add_rotations(rotation, ROTATION_180);
	}
	switch (rotation)
	{
	case ROTATION_180:
	case ROTATION_270:
		rotation = gravity_add_rotations(rotation, ROTATION_180);
		shadow_gc = ReliefGC;
		relief_gc = ShadowGC;
		break;
	default:
		shadow_gc = ShadowGC;
		relief_gc = ReliefGC;
		break;
	}
	max_w = min((w + 1) / 2, line_width);
	max_h = min((h + 1) / 2, line_width);
	seg = (XSegment*)alloca((sizeof(XSegment) * line_width) * 2);
	/* from 0 to the lesser of line_width & just over half w */
	for (i = 0; i < max_w; i++)
	{
		if (rotation == ROTATION_0)
		{
			/* left */
			seg[i].x1 = x+i; seg[i].y1 = y+i+a;
			seg[i].x2 = x+i; seg[i].y2 = y+h-i+a;
		}
		else /* ROTATION_90 */
		{
			/* right */
			seg[i].x1 = x+w-i; seg[i].y1 = y+h-i-a;
			seg[i].x2 = x+w-i; seg[i].y2 = y+i+1-a;
		}
	}
	i2 = i;
	/* draw top segments */
	for (i = 0; i < max_h; i++,i2++)
	{
		seg[i2].x1 = x+w-i-a; seg[i2].y1 = y+i;
		seg[i2].x2 = x+i+1-a; seg[i2].y2 = y+i;
	}
	XDrawSegments(dpy, d, relief_gc, seg, i2);
	/* bottom */
	for (i = 0; i < max_h; i++)
	{
		seg[i].x1 = x+i+a+l;   seg[i].y1 = y+h-i;
		seg[i].x2 = x+w-i-1+a; seg[i].y2 = y+h-i;
	}
	i2 = i;
	for (i = 0; i < max_w; i++,i2++)
	{
		if (rotation == ROTATION_0)
		{
			/* right */
			seg[i2].x1 = x+w-i; seg[i2].y1 = y+h-i-a;
			seg[i2].x2 = x+w-i; seg[i2].y2 = y+i+1-a;
		}
		else /* ROTATION_90 */
		{
			/* left */
			seg[i2].x1 = x+i; seg[i2].y1 = y+i+a;
			seg[i2].x2 = x+i; seg[i2].y2 = y+h-i+a;
		}
	}
	XDrawSegments(dpy, d, shadow_gc, seg, i2);

	return;
}

void do_relieve_rectangle(
	Display *dpy, Drawable d, int x, int y, int w, int h,
	GC ReliefGC, GC ShadowGC, int line_width, Bool use_alternate_shading)
{
	do_relieve_rectangle_with_rotation(
		dpy, d, x, y, w, h, ReliefGC, ShadowGC, line_width,
		use_alternate_shading, ROTATION_0);
	return;
}

/* Creates a pixmap that is a horizontally stretched version of the input
 * pixmap
 */
Pixmap CreateStretchXPixmap(
	Display *dpy, Pixmap src, int src_width, int src_height, int src_depth,
	int dest_width, GC gc)
{
	int i;
	Pixmap pixmap;
	GC my_gc = None;

	if (src_width < 0 || src_height < 0 || dest_width < 0)
	{
		return None;
	}
	pixmap = XCreatePixmap(dpy, src, dest_width, src_height, src_depth);
	if (pixmap == None)
	{
		return None;
	}
	if (gc == None)
	{
		my_gc = fvwmlib_XCreateGC(dpy, pixmap, 0, 0);
	}
	for (i = 0; i < dest_width; i++)
	{
		XCopyArea(
			dpy, src, pixmap, (gc == None)? my_gc:gc,
			(i * src_width) / dest_width, 0, 1, src_height, i, 0);
	}
	if (my_gc)
	{
		XFreeGC(dpy, my_gc);
	}
	return pixmap;
}


/* Creates a pixmap that is a vertically stretched version of the input
 * pixmap
 */
Pixmap CreateStretchYPixmap(
	Display *dpy, Pixmap src, int src_width, int src_height, int src_depth,
	int dest_height, GC gc)
{
	int i;
	Pixmap pixmap;
	GC my_gc = None;

	if (src_height < 0 || src_depth < 0 || dest_height < 0)
	{
		return None;
	}
	pixmap = XCreatePixmap(dpy, src, src_width, dest_height, src_depth);
	if (pixmap == None)
	{
		return None;
	}
	if (gc == None)
	{
		my_gc = fvwmlib_XCreateGC(dpy, pixmap, 0, 0);
	}
	for (i = 0; i < dest_height; i++)
	{
		XCopyArea(
			dpy, src, pixmap, (gc == None)? my_gc:gc,
			0, (i * src_height) / dest_height, src_width, 1, 0, i);
	}
	if (my_gc)
	{
		XFreeGC(dpy, my_gc);
	}
	return pixmap;
}


/* Creates a pixmap that is a stretched version of the input
 * pixmap
 */
Pixmap CreateStretchPixmap(
	Display *dpy, Pixmap src, int src_width, int src_height, int src_depth,
	int dest_width, int dest_height, GC gc)
{
	Pixmap pixmap = None;
	Pixmap temp_pixmap;
	GC my_gc = None;

	if (src_width < 0 || src_height < 0 || src_depth < 0 || dest_width < 0)
	{
		return None;
	}
	if (gc == None)
	{
		my_gc = fvwmlib_XCreateGC(dpy, src, 0, 0);
	}
	temp_pixmap = CreateStretchXPixmap(
		dpy, src, src_width, src_height, src_depth, dest_width,
		(gc == None)? my_gc:gc);
	if (temp_pixmap == None)
	{
		if (my_gc)
		{
			XFreeGC(dpy, my_gc);
		}
		return None;
	}
	pixmap = CreateStretchYPixmap(
		dpy, temp_pixmap, dest_width, src_height, src_depth,
		dest_height, (gc == None)? my_gc:gc);
	XFreePixmap(dpy, temp_pixmap);
	if (my_gc)
	{
		XFreeGC(dpy, my_gc);
	}
	return pixmap;
}

/* Creates a pixmap that is a tiled version of the input pixmap.  Modifies the
 * sets the fill_style of the GC to FillSolid and the tile to None. */
Pixmap CreateTiledPixmap(
	Display *dpy, Pixmap src, int src_width, int src_height,
	int dest_width, int dest_height, int depth, GC gc)
{
	XGCValues xgcv;
	Pixmap pixmap;

	if (src_width < 0 || src_height < 0 ||
	    dest_width < 0 || dest_height < 0)
	{
		return None;
	}
	pixmap = XCreatePixmap(dpy, src, dest_width, dest_height, depth);
	if (pixmap == None)
	{
		return None;
	}
	xgcv.fill_style = FillTiled;
	xgcv.tile = src;
	xgcv.ts_x_origin = 0;
	xgcv.ts_y_origin = 0;
	XChangeGC(
		dpy, gc, GCFillStyle | GCTile | GCTileStipXOrigin |
		GCTileStipYOrigin, &xgcv);
	XFillRectangle(dpy, pixmap, gc, 0, 0, dest_width, dest_height);
	xgcv.fill_style = FillSolid;
	XChangeGC(dpy, gc, GCFillStyle, &xgcv);

	return pixmap;
}

Pixmap CreateRotatedPixmap(
	Display *dpy, Pixmap src, int src_width, int src_height, int depth,
	GC gc, int rotation)
{
	GC my_gc = None;
	Pixmap pixmap = None;
	int dest_width, dest_height, i, j;
	Bool error = False;
	FImage *fim = NULL;
	FImage *src_fim = NULL;

	if (src_width <= 0 || src_height <= 0)
	{
		return None;
	}
	switch(rotation)
	{
	case ROTATION_90:
	case ROTATION_270:
		dest_width = src_height;
		dest_height = src_width;
		break;
	case ROTATION_0:
	case ROTATION_180:
		dest_width = src_width;
		dest_height = src_height;
		break;
	default:
		return None;
		break;
	}
	pixmap = XCreatePixmap(dpy, src, dest_width, dest_height, depth);
	if (pixmap == None)
	{
		return None;
	}
	if (gc == None)
	{
		my_gc = fvwmlib_XCreateGC(dpy, src, 0, 0);
	}
	if (rotation == ROTATION_0)
	{
		XCopyArea(
			dpy, src, pixmap, (gc == None)? my_gc:gc,
			0, 0, src_width, src_height, 0, 0);
		goto bail;
	}

	if (!(src_fim = FGetFImage(
		dpy, src, Pvisual, depth, 0, 0, src_width, src_height,
		AllPlanes, ZPixmap)))
	{
		error = True;
		goto bail;
	}
	if (!(fim = FCreateFImage(
		dpy, Pvisual, depth, ZPixmap, dest_width, dest_height)))
	{
		error = True;
		goto bail;
	}

	for (j = 0; j < src_height; j++)
	{
		for (i = 0; i < src_width; i++)
		{
			switch(rotation)
			{
			case ROTATION_270:
				XPutPixel(
					fim->im, j, src_width - i - 1,
					XGetPixel(src_fim->im, i, j));
				break;
			case ROTATION_90:
				XPutPixel(
					fim->im, src_height - j - 1, i,
					XGetPixel(src_fim->im, i, j));
				break;
			case ROTATION_180:
				XPutPixel(
					fim->im,
					src_width - i - 1, src_height - j - 1,
					XGetPixel(src_fim->im, i, j));
				break;
			default:
				break;
			}
		}
	}
	FPutFImage(dpy, pixmap, gc, fim, 0, 0, 0, 0, dest_width, dest_height);
 bail:
	if (error && pixmap)
	{
		XFreePixmap(dpy,pixmap);
		pixmap = None;
	}
	if (fim)
	{
		FDestroyFImage(dpy, fim);
	}
	if (src_fim)
	{
		FDestroyFImage(dpy, src_fim);
	}
	if (my_gc)
	{
		XFreeGC(dpy, my_gc);
	}
	return pixmap;
}

/*
 *
 * Returns True if the given type of gradient is supported.
 *
 */
Bool IsGradientTypeSupported(char type)
{
	switch (toupper(type))
	{
	case V_GRADIENT:
	case H_GRADIENT:
	case B_GRADIENT:
	case D_GRADIENT:
	case R_GRADIENT:
	case Y_GRADIENT:
	case S_GRADIENT:
	case C_GRADIENT:
		return True;
	default:
		fprintf(stderr, "%cGradient type is not supported\n",
			toupper(type));
		return False;
	}
}

/*
 *
 * Allocates a linear color gradient (veliaa@rpi.edu)
 *
 */
static
XColor *AllocLinearGradient(
	char *s_from, char *s_to, int npixels, int skip_first_color, int dither)
{
	XColor *xcs;
	XColor from, to, c;
	float r;
	float dr;
	float g;
	float dg;
	float b;
	float db;
	int i;
	int got_all = 1;
	int div;

	if (npixels < 1)
	{
		fprintf(stderr,
			"AllocLinearGradient: Invalid number of pixels: %d\n",
			npixels);
		return NULL;
	}
	if (!s_from || !XParseColor(Pdpy, Pcmap, s_from, &from))
	{
		fprintf(stderr, "Cannot parse color \"%s\"\n",
			s_from ? s_from : "<blank>");
		return NULL;
	}
	if (!s_to || !XParseColor(Pdpy, Pcmap, s_to, &to))
	{
		fprintf(stderr, "Cannot parse color \"%s\"\n",
			s_to ? s_to : "<blank>");
		return NULL;
	}

	/* divisor must not be zero, hence this calculation */
	div = (npixels == 1) ? 1 : npixels - 1;
	c = from;
	/* red part and step width */
	r = from.red;
	dr = (float)(to.red - from.red);
	/* green part and step width */
	g = from.green;
	dg = (float)(to.green - from.green);
	/* blue part and step width */
	b = from.blue;
	db = (float)(to.blue - from.blue);
	xcs = xcalloc(1, sizeof(XColor) * npixels);
	c.flags = DoRed | DoGreen | DoBlue;
	for (i = (skip_first_color) ? 1 : 0; i < npixels && div > 0; ++i)
	{
		c.red   = (unsigned short)
			((int)(r + dr / (float)div * (float)i + 0.5));
		c.green = (unsigned short)
			((int)(g + dg / (float)div * (float)i + 0.5));
		c.blue  = (unsigned short)
			((int)(b + db / (float)div * (float)i + 0.5));
		if (dither == 0 && !PictureAllocColor(Pdpy, Pcmap, &c, False))
		{
			got_all = 0;
		}
		xcs[i] = c;
	}
	if (!got_all && dither == 0)
	{
		fprintf(stderr, "Cannot alloc color gradient %s to %s\n",
			s_from, s_to);
	}

	return xcs;
}

/*
 *
 * Allocates a nonlinear color gradient (veliaa@rpi.edu)
 *
 */
static XColor *AllocNonlinearGradient(
	char *s_colors[], int clen[], int nsegs, int npixels, int dither)
{
	XColor *xcs = xmalloc(sizeof(XColor) * npixels);
	int i;
	int curpixel = 0;
	int *seg_end_colors;
	int seg_sum = 0;
	float color_sum = 0.0;

	if (nsegs < 1 || npixels < 2)
	{
		fprintf(stderr,
			"Gradients must specify at least one segment and"
			" two colors\n");
		free(xcs);
		return NULL;
	}
	for (i = 0; i < npixels; i++)
	{
		xcs[i].pixel = 0;
	}

	/* get total length of all segments */
	for (i = 0; i < nsegs; i++)
	{
		seg_sum += clen[i];
	}

	/* calculate the index of a segment's las color */
	seg_end_colors = alloca(nsegs * sizeof(int));
	if (nsegs == 1)
	{
		seg_end_colors[0] = npixels - 1;
	}
	else
	{
		for (i = 0; i < nsegs; i++)
		{
			color_sum += (float)(clen[i] * (npixels - 1)) /
				(float)(seg_sum);
			seg_end_colors[i] = (int)(color_sum + 0.5);
		}
		if (seg_end_colors[nsegs - 1] > npixels - 1)
		{
			fprintf(stderr,
				"BUG: (AllocNonlinearGradient): "
				"seg_end_colors[nsegs - 1] (%d)"
				" > npixels - 1 (%d)."
				" Gradient drawing aborted\n",
				seg_end_colors[nsegs - 1], npixels - 1);
			return NULL;
		}
		/* take care of rounding errors */
		seg_end_colors[nsegs - 1] = npixels - 1;
	}

	for (i = 0; i < nsegs; ++i)
	{
		XColor *c = NULL;
		int j;
		int n;
		int skip_first_color = (curpixel != 0);

		if (i == 0)
		{
			n = seg_end_colors[0] + 1;
		}
		else
		{
			n = seg_end_colors[i] - seg_end_colors[i - 1] + 1;
		}

		if (n > 1)
		{
			c = AllocLinearGradient(
				s_colors[i], s_colors[i + 1], n,
				skip_first_color, dither);
			if (!c && (n - skip_first_color) != 0)
			{
				free(xcs);
				return NULL;
			}
			for (j = skip_first_color; j < n; ++j)
			{
				xcs[curpixel + j] = c[j];
			}
			curpixel += n - 1;
		}
		if (c)
		{
			free(c);
			c = NULL;
		}
		if (curpixel != seg_end_colors[i])
		{
			fprintf(stderr,
				"BUG: (AllocNonlinearGradient): "
				"nsegs %d, i %d, curpixel %d,"
				" seg_end_colors[i] = %d,"
				" npixels %d, n %d\n",
				nsegs, i, curpixel,
				seg_end_colors[i],npixels,n);
			return NULL;
		}
	}
	return xcs;
}

/* Convenience function. Calls AllocNonLinearGradient to fetch all colors and
 * then frees the color names and the perc and color_name arrays. */
XColor *AllocAllGradientColors(
	char *color_names[], int perc[], int nsegs, int ncolors, int dither)
{
	XColor *xcs = NULL;
	int i;

	/* grab the colors */
	xcs = AllocNonlinearGradient(
		color_names, perc, nsegs, ncolors, dither);
	for (i = 0; i <= nsegs; i++)
	{
		if (color_names[i])
		{
			free(color_names[i]);
		}
	}
	free(color_names);
	free(perc);
	if (!xcs)
	{
		fprintf(stderr, "couldn't create gradient\n");
		return NULL;
	}

	return xcs;
}

/* groks a gradient string and creates arrays of colors and percentages
 * returns the number of colors asked for (No. allocated may be less due
 * to the ColorLimit command).  A return of 0 indicates an error
 */
int ParseGradient(
	char *gradient, char **rest, char ***colors_return, int **perc_return,
	int *nsegs_return)
{
	char *item;
	char *orig;
	int npixels;
	char **s_colors;
	int *perc;
	int nsegs, i, sum;
	Bool is_syntax_error = False;

	/* get the number of colors specified */
	if (rest)
	{
		*rest = gradient;
	}
	orig = gradient;

	if (GetIntegerArguments(gradient, &gradient, (int *)&npixels, 1) != 1 ||
	    npixels < 2)
	{
		fprintf(
			stderr, "ParseGradient: illegal number of colors in"
			" gradient: '%s'\n", orig);
		return 0;
	}

	/* get the starting color or number of segments */
	gradient = GetNextToken(gradient, &item);
	if (gradient)
	{
		gradient = SkipSpaces(gradient, NULL, 0);
	}
	if (!gradient || !*gradient || !item)
	{
		fprintf(stderr, "Incomplete gradient style: '%s'\n", orig);
		if (item)
		{
			free(item);
		}
		if (rest)
		{
			*rest = gradient;
		}
		return 0;
	}

	if (GetIntegerArguments(item, NULL, &nsegs, 1) != 1)
	{
		/* get the end color of a simple gradient */
		s_colors = xmalloc(sizeof(char *) * 2);
		perc = xmalloc(sizeof(int));
		nsegs = 1;
		s_colors[0] = item;
		gradient = GetNextToken(gradient, &item);
		s_colors[1] = item;
		perc[0] = 100;
	}
	else
	{
		free(item);
		/* get a list of colors and percentages */
		if (nsegs < 1)
			nsegs = 1;
		if (nsegs > MAX_GRADIENT_SEGMENTS)
			nsegs = MAX_GRADIENT_SEGMENTS;
		s_colors = xmalloc(sizeof(char *) * (nsegs + 1));
		perc = xmalloc(sizeof(int) * nsegs);
		for (i = 0; !is_syntax_error && i <= nsegs; i++)
		{
			s_colors[i] = 0;
			gradient = GetNextToken(gradient, &s_colors[i]);
			if (i < nsegs)
			{
				if (GetIntegerArguments(
					    gradient, &gradient, &perc[i], 1)
				    != 1 || perc[i] <= 0)
				{
					/* illegal size */
					perc[i] = 0;
				}
			}
		}
		if (s_colors[nsegs] == NULL)
		{
			fprintf(
				stderr, "ParseGradient: too few gradient"
				" segments: '%s'\n", orig);
			is_syntax_error = True;
		}
	}

	/* sanity check */
	for (i = 0, sum = 0; !is_syntax_error && i < nsegs; ++i)
	{
		int old_sum = sum;

		sum += perc[i];
		if (sum < old_sum)
		{
			/* integer overflow */
			fprintf(
				stderr, "ParseGradient: multi gradient"
				" overflow: '%s'", orig);
			is_syntax_error = 1;
			break;
		}
	}
	if (is_syntax_error)
	{
		for (i = 0; i <= nsegs; ++i)
		{
			if (s_colors[i])
			{
				free(s_colors[i]);
			}
		}
		free(s_colors);
		free(perc);
		if (rest)
		{
			*rest = gradient;
		}
		return 0;
	}


	/* sensible limits */
	if (npixels < 2)
		npixels = 2;
	if (npixels > MAX_GRADIENT_COLORS)
		npixels = MAX_GRADIENT_COLORS;

	/* send data back */
	*colors_return = s_colors;
	*perc_return = perc;
	*nsegs_return = nsegs;
	if (rest)
		*rest = gradient;

	return npixels;
}

/* Calculate the prefered dimensions of a gradient, based on the number of
 * colors and the gradient type. Returns False if the gradient type is not
 * supported. */
Bool CalculateGradientDimensions(
	Display *dpy, Drawable d, int ncolors, char type, int dither,
	int *width_ret, int *height_ret)
{
	static int best_width = 0, best_height = 0;
	int dither_factor = (dither > 0)? 128:1;

	/* get the best tile size (once) */
	if (!best_width)
	{
		if (!XQueryBestTile(
			    dpy, d, 1, 1, (unsigned int*)&best_width,
			    (unsigned int*)&best_height))
		{
			best_width = 0;
			best_height = 0;
		}
		/* this is needed for buggy X servers like XFree 3.3.3.1 */
		if (!best_width)
			best_width = 1;
		if (!best_height)
			best_height = 1;
	}

	switch (type) {
	case H_GRADIENT:
		*width_ret = ncolors;
		*height_ret = best_height * dither_factor;
		break;
	case V_GRADIENT:
		*width_ret = best_width * dither_factor;
		*height_ret = ncolors;
		break;
	case D_GRADIENT:
	case B_GRADIENT:
		/* diagonal gradients are rendered into a rectangle for which
		 * the width plus the height is equal to ncolors + 1. The
		 * rectangle is square when ncolors is odd and one pixel
		 * taller than wide with even numbers */
		*width_ret = (ncolors + 1) / 2;
		*height_ret = ncolors + 1 - *width_ret;
		break;
	case S_GRADIENT:
		/* square gradients have the last color as a single pixel in
		 * the centre */
		*width_ret = *height_ret = 2 * ncolors - 1;
		break;
	case C_GRADIENT:
		/* circular gradients have the first color as a pixel in each
		 * corner */
		*width_ret = *height_ret = 2 * ncolors - 1;
		break;
	case R_GRADIENT:
	case Y_GRADIENT:
		/* swept types need each color to occupy at least one pixel at
		 * the edge. Get the smallest odd number that will provide
		 * enough */
		for (*width_ret = 1;
		     (double)(*width_ret - 1) * M_PI < (double)ncolors;
		     *width_ret += 2)
		{
			/* nothing to do here */
		}
		*height_ret = *width_ret;
		break;
	default:
		fprintf(stderr, "%cGradient not supported\n", type);
		return False;
	}
	return True;
}

/* Does the actual drawing of the pixmap. If the in_drawable argument is None,
 * a new pixmap of the given depth, width and height is created. If it is not
 * None the gradient is drawn into it. The d_width, d_height, d_x and d_y
 * describe the traget rectangle within the drawable. */
Drawable CreateGradientPixmap(
	Display *dpy, Drawable d, GC gc, int type, int g_width,
	int g_height, int ncolors, XColor *xcs, int dither, Pixel **d_pixels,
	int *d_npixels, Drawable in_drawable, int d_x, int d_y,
	int d_width, int d_height, XRectangle *rclip)
{
	Pixmap pixmap = None;
	PictureImageColorAllocator *pica = NULL;
	XColor c;
	FImage *fim;
	register int i, j;
	XGCValues xgcv;
	Drawable target;
	int t_x;
	int t_y;
	int t_width;
	int t_height;

	if (d_pixels != NULL && *d_pixels != NULL)
	{
		if (d_npixels != NULL && *d_npixels > 0)
		{
			PictureFreeColors(
				dpy, Pcmap, *d_pixels, *d_npixels, 0, False);
		}
		free(*d_pixels);
		*d_pixels = NULL;
	}
	if (d_npixels != NULL)
	{
		*d_npixels = 0;
	}
	if (g_height < 0 || g_width < 0 || d_width < 0 || d_height < 0)
		return None;

	if (in_drawable == None)
	{
		/* create a pixmap to use */
		pixmap = XCreatePixmap(dpy, d, g_width, g_height, Pdepth);
		if (pixmap == None)
			return None;
		target = pixmap;
		t_x = 0;
		t_y = 0;
		t_width = g_width;
		t_height = g_height;
	}
	else
	{
		target = in_drawable;
		t_x = d_x;
		t_y = d_y;
		t_width = d_width;
		t_height = d_height;
	}

	fim = FCreateFImage(
		dpy, Pvisual, Pdepth, ZPixmap, t_width, t_height);
	if (!fim)
	{
		fprintf(stderr, "%cGradient couldn't get image\n", type);
		if (pixmap != None)
			XFreePixmap(dpy, pixmap);
		return None;
	}
	if (dither)
	{
		pica = PictureOpenImageColorAllocator(
			dpy, Pcmap, t_width, t_height,
			False, False, dither, False);
	}
	/* now do the fancy drawing */
	switch (type)
	{
	case H_GRADIENT:
	{
		for (i = 0; i < t_width; i++)
		{
			int d = i * ncolors / t_width;
			c = xcs[d];
			for (j = 0; j < t_height; j++)
			{
				if (dither)
				{
					c = xcs[d];
					PictureAllocColorImage(
						dpy, pica, &c, i, j);
				}
				XPutPixel(fim->im, i, j, c.pixel);
			}
		}
	}
	break;
	case V_GRADIENT:
	{
		for (j = 0; j < t_height; j++)
		{
			int d = j * ncolors / t_height;
			c = xcs[d];
			for (i = 0; i < t_width; i++)
			{
				if (dither)
				{
					c = xcs[d];
					PictureAllocColorImage(
						dpy, pica, &c, i, j);
				}
				XPutPixel(fim->im, i, j, c.pixel);
			}
		}
		break;
	}
	case D_GRADIENT:
	{
		register int t_scale = t_width + t_height - 1;
		for (i = 0; i < t_width; i++)
		{
			for (j = 0; j < t_height; j++)
			{
				c = xcs[(i+j) * ncolors / t_scale];
				if (dither)
				{
					PictureAllocColorImage(
						dpy, pica, &c, i, j);
				}
				XPutPixel(fim->im, i, j, c.pixel);
			}
		}
		break;
	}
	case B_GRADIENT:
	{
		register int t_scale = t_width + t_height - 1;
		for (i = 0; i < t_width; i++)
		{
			for (j = 0; j < t_height; j++)
			{
				c = xcs[(i + (t_height - j - 1)) * ncolors /
				       t_scale];
				if (dither)
				{
					PictureAllocColorImage(
						dpy, pica, &c, i, j);
				}
				XPutPixel(fim->im, i, j, c.pixel);
			}
		}
		break;
	}
	case S_GRADIENT:
	{
		register int t_scale = t_width * t_height;
		register int myncolors = ncolors * 2;
		for (i = 0; i < t_width; i++) {
			register int pi = min(i, t_width - 1 - i) * t_height;
			for (j = 0; j < t_height; j++) {
				register int pj =
					min(j, t_height - 1 - j) * t_width;
				c = xcs[(min(pi, pj) * myncolors - 1) /
				       t_scale];
				if (dither)
				{
					PictureAllocColorImage(
						dpy, pica, &c, i, j);
				}
				XPutPixel(fim->im, i, j, c.pixel);
			}
		}
	}
	break;
	case C_GRADIENT:
	{
		register double t_scale =
			(double)(t_width * t_height) / sqrt(8);
		for (i = 0; i < t_width; i++)
		{
			for (j = 0; j < t_height; j++)
			{
				register double x =
					(double)((2 * i - t_width) * t_height) /
					4.0;
				register double y =
					(double)((t_height - 2 * j) * t_width) /
					4.0;
				register double rad = sqrt(x * x + y * y);
				c = xcs[(int)((rad * ncolors - 0.5) / t_scale)];
				if (dither)
				{
					PictureAllocColorImage(
						dpy, pica, &c, i, j);
				}
				XPutPixel(fim->im, i, j, c.pixel);
			}
		}
		break;
	}
	case R_GRADIENT:
	{
		register int w = t_width - 1;
		register int h = t_height - 1;
		/* g_width == g_height, both are odd, therefore x can be 0.0 */
		for (i = 0; i <= w; i++) {
			for (j = 0; j <= h; j++) {
				register double x =
					(double)((2 * i - w) * h) / 4.0;
				register double y =
					(double)((h - 2 * j) * w) / 4.0;
				/* angle ranges from -pi/2 to +pi/2 */
				register double angle;
				if (x != 0.0) {
					angle = atan(y / x);
				} else {
					angle = (y < 0) ? - M_PI_2 : M_PI_2;
				}
				/* extend to -pi/2 to 3pi/2 */
				if (x < 0)
					angle += M_PI;
				/* move range from -pi/2:3*pi/2 to 0:2*pi */
				if (angle < 0.0)
					angle += M_PI * 2.0;
				/* normalize to gradient */
				c = xcs[(int)(angle * M_1_PI * 0.5 * ncolors)];
				if (dither)
				{
					PictureAllocColorImage(
						dpy, pica, &c, i, j);
				}
				XPutPixel(fim->im, i, j, c.pixel);
			}
		}
	}
	break;
	/*
	 * The Yin Yang gradient style and the following code are:
	 * Copyright 1999 Sir Boris. (email to sir_boris@bigfoot.com may be
	 * read by his groom but is not guaranteed to elicit a response)
	 * No restrictions are placed on this code, as long as the copyright
	 * notice is preserved.
	 */
	case Y_GRADIENT:
	{
		register int r = t_width * t_height / 4;
		for (i = 0; i < t_width; i++) {
			for (j = 0; j < t_height; j++) {
				register double x =
					(double)((2 * i - t_width) * t_height) /
					4.0;
				register double y =
					(double)((t_height - 2 * j) * t_width) /
					4.0;
				register double rad = sqrt(x * x + y * y);
				/* angle ranges from -pi/2 to +pi/2 */
				register double angle;

				if (x != 0.0) {
					angle = atan(y / x);
				} else {
					angle = (y < 0) ? - M_PI_2 : M_PI_2;
				}
				/* extend to -pi/2 to 3pi/2 */
				if (x < 0)
					angle += M_PI;
				/* warp the angle within the yinyang circle */
				if (rad <= r) {
					angle -= acos(rad / r);
				}
				/* move range from -pi/2:3*pi/2 to 0:2*pi */
				if (angle < 0.0)
					angle += M_PI * 2.0;
				/* normalize to gradient */
				c = xcs[(int)(angle * M_1_PI * 0.5 * ncolors)];
				if (dither)
				{
					PictureAllocColorImage(
						dpy, pica, &c, i, j);
				}
				XPutPixel(fim->im, i, j, c.pixel);
			}
		}
	}
	break;
	default:
		/* placeholder function, just fills the pixmap with the first
		 * color */
		memset(fim->im->data, 0, fim->im->bytes_per_line * t_height);
		XAddPixel(fim->im, xcs[0].pixel);
		break;
	}

	if (dither)
	{
		if (d_pixels != NULL && d_npixels != NULL)
		{
			PictureCloseImageColorAllocator(
				dpy, pica, d_npixels, d_pixels, 0);
		}
		else
		{
			/* possible color leak */
		}
	}

	/* set the gc style */
	xgcv.function = GXcopy;
	xgcv.plane_mask = AllPlanes;
	xgcv.fill_style = FillSolid;
	xgcv.clip_mask = None;
	XChangeGC(dpy, gc, GCFunction|GCPlaneMask|GCFillStyle|GCClipMask,
		  &xgcv);
	if (rclip)
	{
		XSetClipRectangles(dpy, gc, 0, 0, rclip, 1, Unsorted);
	}
	/* copy the image to the server */
	FPutFImage(dpy, target, gc, fim, 0, 0, t_x, t_y, t_width, t_height);
	if (rclip)
	{
		XSetClipMask(dpy, gc, None);
	}
	FDestroyFImage(dpy, fim);
	return target;
}


/* Create a pixmap from a gradient specifier, width and height are hints
 * that are only used for gradients that can be tiled e.g. H or V types
 * types are HVDBSCRY for Horizontal, Vertical, Diagonal, Back-diagonal, Square,
 * Circular, Radar and Yin/Yang respectively (in order of bloatiness)
 */
Pixmap CreateGradientPixmapFromString(
	Display *dpy, Drawable d, GC gc, int type, char *action,
	int *width_return, int *height_return,
	Pixel **pixels_return, int *nalloc_pixels, int dither)
{
	Pixel *d_pixels = NULL;
	int d_npixels = 0;
	XColor *xcs = NULL;
	int ncolors = 0;
	char **colors;
	int *perc, nsegs;
	Pixmap pixmap = None;

	/* set return pixels to NULL in case of premature return */
	if (pixels_return)
		*pixels_return = NULL;
	if (nalloc_pixels)
		*nalloc_pixels = 0;

	/* translate the gradient string into an array of colors etc */
	if (!(ncolors = ParseGradient(action, NULL, &colors, &perc, &nsegs))) {
		fprintf(stderr, "Can't parse gradient: '%s'\n", action);
		return None;
	}
	/* grab the colors */
	xcs = AllocAllGradientColors(
		colors, perc, nsegs, ncolors, dither);
	if (xcs == NULL)
	{
		return None;
	}

	/* grok the size to create from the type */
	type = toupper(type);

	if (CalculateGradientDimensions(
		    dpy, d, ncolors, type, dither, width_return, height_return))
	{
		pixmap = CreateGradientPixmap(
			dpy, d, gc, type, *width_return, *height_return,
			ncolors, xcs, dither, &d_pixels, &d_npixels,
			None, 0, 0, 0, 0, NULL);
	}

	/* if the caller has not asked for the pixels there is probably a leak
	 */
	if (PUseDynamicColors)
	{
		if (!(pixels_return && nalloc_pixels))
		{
			/* if the caller has not asked for the pixels there is
			 * probably a leak */
			fprintf(stderr,
			"CreateGradient: potential color leak, losing track"
			" of pixels\n");
			if (d_pixels != NULL)
			{
				free(d_pixels);
			}
		}
		else
		{
			if (!dither)
			{
				Pixel *pixels;
				int i;

				pixels = xmalloc(ncolors * sizeof(Pixel));
				for(i=0; i<ncolors; i++)
				{
					pixels[i] = xcs[i].pixel;
				}
				*pixels_return = pixels;
				*nalloc_pixels = ncolors;
			}
			else
			{
				*pixels_return = d_pixels;
				*nalloc_pixels = d_npixels;
			}
		}
	}
	else if (d_pixels != NULL)
	{
		/* should not happen */
		free(d_pixels);
	}

	free(xcs);

	return pixmap;
}

/*
 *
 *  Draws a little Triangle pattern within a window
 *
 */
void DrawTrianglePattern(
	Display *dpy, Drawable d, GC ReliefGC, GC ShadowGC, GC FillGC,
	int x, int y, int width, int height, int bw, char orientation,
	Bool draw_relief, Bool do_fill, Bool is_pressed)
{
	const struct
	{
		const char line[3];
		const char point[3];
	} hi[4] =
		{
			{ { 1, 0, 0 }, { 1, 1, 0 } }, /* up */
			{ { 1, 0, 1 }, { 1, 0, 0 } }, /* down */
			{ { 1, 0, 0 }, { 1, 1, 0 } }, /* left */
			{ { 1, 0, 1 }, { 1, 1, 0 } }  /* right */
		};
	XPoint points[4];
	GC temp_gc;
	int short_side;
	int long_side;
	int t_width;
	int t_height;
	int i;
	int type;

	/* remove border width from target area */
	width -= 2 * bw;
	height -= 2 * bw;
	x += bw;
	y += bw;
	if (width < 1 || height < 1)
		/* nothing to do */
		return;

	orientation = tolower(orientation);
	switch (orientation)
	{
	case 'u':
	case 'd':
		long_side = width;
		short_side = height;
		type = (orientation == 'd');
		break;
	case 'l':
	case 'r':
		long_side = height;
		short_side = width;
		type = (orientation == 'r') + 2;
		break;
	default:
		/* unknowm orientation */
		return;
	}

	/* assure the base side has an odd length */
	if ((long_side & 0x1) == 0)
		long_side--;
	/* reduce base length if short sides don't fit */
	if (short_side < long_side / 2 + 1)
		long_side = 2 * short_side - 1;
	else
		short_side = long_side / 2 + 1;

	if (orientation == 'u' || orientation == 'd')
	{
		t_width = long_side;
		t_height = short_side;
	}
	else
	{
		t_width = short_side;
		t_height = long_side;
	}
	/* find proper x/y coordinate */
	x += (width - t_width) / 2;
	y += (height - t_height) / 2;
	/* decrement width and height for convenience of calculation */
	t_width--;
	t_height--;

	/* get the list of points to draw */
	switch (orientation)
	{
	case 'u':
		y += t_height;
		t_height = -t_height;
	case 'd':
		points[1].x = x + t_width / 2;
		points[1].y = y + t_height;
		points[2].x = x + t_width;
		points[2].y = y;
		break;
	case 'l':
		x += t_width;
		t_width = -t_width;
	case 'r':
		points[1].x = x + t_width;
		points[1].y = y + t_height / 2;
		points[2].x = x;
		points[2].y = y + t_height;
		break;
	}
	points[0].x = x;
	points[0].y = y;
	points[3].x = x;
	points[3].y = y;

	if (do_fill)
	{
		/* solid triangle */
		XFillPolygon(
			dpy, d, FillGC, points, 3, Convex, CoordModeOrigin);
	}
	if (draw_relief)
	{
		/* relief triangle */
		for (i = 0; i < 3; i++)
		{
			temp_gc = (is_pressed ^ hi[type].line[i]) ?
				ReliefGC : ShadowGC;
			XDrawLine(
				dpy, d, temp_gc, points[i].x, points[i].y,
				points[i+1].x, points[i+1].y);
		}
		for (i = 0; i < 3; i++)
		{
			temp_gc = (is_pressed ^ hi[type].point[i]) ?
				ReliefGC : ShadowGC;
			XDrawPoint(dpy, d, temp_gc, points[i].x, points[i].y);
		}
	}

	return;
}

GC fvwmlib_XCreateGC(
	Display *display, Drawable drawable, unsigned long valuemask,
	XGCValues *values)
{
	GC gc;
	Bool f;
	XGCValues gcv;

	if (!values)
	{
		values = &gcv;
	}
	f = values->graphics_exposures;
	if (!(valuemask & GCGraphicsExposures))
	{
		valuemask |= GCGraphicsExposures;
		values->graphics_exposures = 0;
	}
	gc = XCreateGC(display, drawable, valuemask, values);
	values->graphics_exposures = f;

	return gc;
}
