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

/* Graphics.c: misc convenience functions for drawing stuff
 */

#include "fvwm/defaults.h"
#include "libs/fvwmlib.h"
#include "libs/Picture.h"

#include <X11/Xlib.h>
#include <stdio.h>
#include <math.h>


/* Draws the relief pattern around a window
 * Draws a line_width wide rectangle from (x,y) to (x+w,y+h) i.e w+1 wide, h+1 high
 * Draws end points assuming CAP_NOT_LAST style in GC
 * Draws anti-clockwise in case CAP_BUTT is the style and the end points overlap
 * Top and bottom lines come out full length, the sides come out 1 pixel less
 * This is so FvwmBorder windows have a correct bottom edge and the sticky lines
 * look like just lines
 */
void RelieveRectangle(Display *dpy, Drawable d, int x,int y,int w,int h,
		      GC ReliefGC, GC ShadowGC, int line_width)
{
  XSegment* seg = (XSegment*)safemalloc(sizeof(XSegment) * line_width);
  int i;

  /* left side, from 0 to the lesser of line_width & just over half w */
  for (i = 0; (i < line_width) && (i <= w / 2); i++) {
    seg[i].x1 = x+i; seg[i].y1 = y+i;
    seg[i].x2 = x+i; seg[i].y2 = y+h-i;
  }
  XDrawSegments(dpy, d, ReliefGC, seg, i);
  /* bottom */
  for (i = 0; (i < line_width) && (i <= h / 2); i++) {
    seg[i].x1 = x+i;   seg[i].y1 = y+h-i;
    seg[i].x2 = x+w-i; seg[i].y2 = y+h-i;
  }
  XDrawSegments(dpy, d, ShadowGC, seg, i);
  /* right */
  for (i = 0; (i < line_width) && (i <= w / 2); i++) {
    seg[i].x1 = x+w-i; seg[i].y1 = y+h-i;
    seg[i].x2 = x+w-i; seg[i].y2 = y+i;
  }
  XDrawSegments(dpy, d, ShadowGC, seg, i);
  /* draw top segments */
  for (i = 0; (i < line_width) && (i <= h / 2); i++) {
    seg[i].x1 = x+w-i; seg[i].y1 = y+i;
    seg[i].x2 = x+i;   seg[i].y2 = y+i;
  }
  XDrawSegments(dpy, d, ReliefGC, seg, i);
  free(seg);
}


/* Creates a pixmap that is a horizontally stretched version of the input
 * pixmap
 */
Pixmap CreateStretchXPixmap(Display *dpy, Pixmap src, unsigned int src_width,
			    unsigned int src_height, unsigned int src_depth,
			    unsigned int dest_width, GC gc)
{
  int i;
  Pixmap pixmap = XCreatePixmap(dpy, src, dest_width, src_height, src_depth);

  if (pixmap)
    for (i = 0; i < dest_width; i++)
      XCopyArea(dpy, src, pixmap, gc, (i * src_width) / dest_width, 0,
		1, src_height, i, 0);

  return pixmap;
}


/* Creates a pixmap that is a vertically stretched version of the input
 * pixmap
 */
Pixmap CreateStretchYPixmap(Display *dpy, Pixmap src, unsigned int src_width,
			    unsigned int src_height, unsigned int src_depth,
			    unsigned int dest_height, GC gc)
{
  int i;
  Pixmap pixmap = XCreatePixmap(dpy, src, src_width, dest_height, src_depth);

  if (pixmap)
    for (i = 0; i < dest_height; i++)
      XCopyArea(dpy, src, pixmap, gc, 0, (i * src_height) / dest_height,
		src_width, 1, 0, i);

  return pixmap;
}


/* Creates a pixmap that is a stretched version of the input
 * pixmap
 */
Pixmap CreateStretchPixmap(Display *dpy, Pixmap src, unsigned int src_width,
			    unsigned int src_height, unsigned int src_depth,
			    unsigned int dest_width, unsigned int dest_height,
			    GC gc)
{
  Pixmap pixmap = None;
  Pixmap temp_pixmap = CreateStretchXPixmap(dpy, src, src_width, src_height,
					    src_depth, dest_width, gc);
  if (temp_pixmap)
  {
    pixmap = CreateStretchYPixmap(dpy, temp_pixmap, dest_width, src_height,
				  src_depth, dest_height, gc);
    XFreePixmap(dpy, temp_pixmap);
  }

  return pixmap;
}


/* Creates a pixmap that is a tiled version of the input pixmap (input pixmap
 * must be depth 1).
 */
Pixmap CreateTiledMaskPixmap(Display *dpy, Pixmap src, unsigned int src_width,
			     unsigned int src_height, unsigned int dest_width,
			     unsigned int dest_height, GC gc)
{
  int x;
  int y;
  Pixmap pixmap = XCreatePixmap(dpy, src, dest_width, dest_height, 1);

  if (pixmap)
  {
    for (y = 0; y < dest_height; y += src_height)
    {
      for (x = 0; x < dest_width; x += src_width)
      {
	XCopyArea(dpy, src, pixmap, gc, 0, 0, src_width, src_height, x, y);
      }
    }
  }

  return pixmap;
}


/****************************************************************************
 *
 * Allocates a linear color gradient (veliaa@rpi.edu)
 *
 ****************************************************************************/
Pixel *AllocLinearGradient(char *s_from, char *s_to, int npixels)
{
  Pixel *pixels;
  XColor from, to, c;
  int r, dr, g, dg, b, db;
  int i;
  int got_all = 1;
  int div;

  if (npixels < 1)
  {
    fprintf(stderr, "AllocLinearGradient: Invalid number of pixels: %d\n",
	    npixels);
    return NULL;
  }
  if (!s_from || !XParseColor(Pdpy, Pcmap, s_from, &from))
  {
    fprintf(stderr, "Cannot parse color %s\n", s_from ? s_from : "<blank>");
    return NULL;
  }
  if (!s_to || !XParseColor(Pdpy, Pcmap, s_to, &to))
  {
    fprintf(stderr, "Cannot parse color %s\n", s_to ? s_to : "<blank>");
    return NULL;
  }
  /* divisor must not be zero, hence this calculation */
  div = (npixels == 1) ? 1 : npixels - 1;
  c = from;
  /* red part and step width */
  r = from.red;
  dr = (to.red - from.red) / div;
  /* green part and step width */
  g = from.green;
  dg = (to.green - from.green) / div;
  /* blue part and step width */
  b = from.blue;
  db = (to.blue - from.blue) / div;
  pixels = (Pixel *)safemalloc(sizeof(Pixel) * npixels);
  c.flags = DoRed | DoGreen | DoBlue;
  for (i = 0; i < npixels; ++i)
  {
    if (!XAllocColor(Pdpy, Pcmap, &c))
      got_all = 0;
    pixels[ i ] = c.pixel;
    c.red = (unsigned short) (r += dr);
    c.green = (unsigned short) (g += dg);
    c.blue = (unsigned short) (b += db);
  }
  if (!got_all)
  {
    fprintf(stderr, "Cannot alloc color gradient %s to %s\n", s_from, s_to);
  }

  return pixels;
}

/****************************************************************************
 *
 * Allocates a nonlinear color gradient (veliaa@rpi.edu)
 *
 ****************************************************************************/
Pixel *AllocNonlinearGradient(char *s_colors[], int clen[],
			      int nsegs, int npixels)
{
  Pixel *pixels = (Pixel *)safemalloc(sizeof(Pixel) * npixels);
  int i;
  int curpixel = 0, perc = 0;

  if (nsegs < 1)
  {
    fprintf(stderr, "Gradients must specify at least one segment\n");
    free(pixels);
    return NULL;
  }
  for (i = 0; i < npixels; i++)
    pixels[i] = 0;

  for (i = 0; (i < nsegs) && (curpixel < npixels) && (perc <= 100); ++i)
  {
    Pixel *p;
    int j, n = clen[i] * npixels / 100;

    if (i == nsegs - 1)
    {
      /* last segment, need to allocate the final colour too. */
      n++;
    }
    p = AllocLinearGradient(s_colors[i], s_colors[i + 1], n);
    if (!p)
    {
      free(pixels);
      return NULL;
    }
    for (j = 0; ((j < n) && ((curpixel + j) < npixels)); ++j)
      pixels[curpixel + j] = p[j];
    perc += clen[i];
    curpixel += n;
    free(p);
  }
  for (i = curpixel; i < npixels; ++i)
    pixels[i] = pixels[i - 1];

  return pixels;
}

/* groks a gradient string and creates arrays of colors and percentages
 * returns the number of colors asked for (No. allocated may be less due
 * to the ColorLimit command).  A return of 0 indicates an error
 */
unsigned int ParseGradient(char *gradient, char ***colors_return,
			   int **perc_return, int *nsegs_return)
{
  char *item;
  unsigned int npixels;
  char **s_colors;
  int *perc;
  int nsegs, i, sum;

  /* get the number of colors specified */
  if (!(gradient = GetNextToken(gradient, &item)) || (item == NULL)) {
    fprintf(stderr, "ParseGradient: expected number of colors to allocate\n");
    if (item)
      free(item);
    return 0;
  }
  npixels = atoi(item);
  free(item);

  /* get the starting color or number of segments */
  if (!(gradient = GetNextToken(gradient, &item)) || (item == NULL)) {
    fprintf(stderr, "Incomplete gradient style\n");
    if (item)
      free(item);
    return 0;
  }

  if (!(isdigit(*item))) {
    /* get the end color of a simple gradient */
    s_colors = (char **)safemalloc(sizeof(char *) * 2);
    perc = (int *)safemalloc(sizeof(int));
    nsegs = 1;
    s_colors[0] = item;
    gradient = GetNextToken(gradient, &s_colors[1]);
    perc[0] = 100;
  } else {
    /* get a list of colors and percentages */
    nsegs = atoi(item);
    free(item);
    if (nsegs < 1)
      nsegs = 1;
    if (nsegs > MAX_GRADIENT_SEGMENTS)
      nsegs = MAX_GRADIENT_SEGMENTS;
    s_colors = (char **)safemalloc(sizeof(char *) * (nsegs + 1));
    perc = (int *)safemalloc(sizeof(int) * nsegs);
    for (i = 0; i <= nsegs; i++) {
      gradient = GetNextToken(gradient, &s_colors[i]);
      if (i < nsegs) {
        gradient = GetNextToken(gradient, &item);
	if (item) {
	  perc[i] = atoi(item);
	  free(item);
	} else {
	  perc[i] = 0;
	}
      }
    }
  }

  /* sanity check */
  for (i = 0, sum = 0; i < nsegs; ++i)
    sum += perc[i];

  if (sum != 100) {
    fprintf(stderr, "ParseGradient: multi gradient lengths must sum to 100");
    for (i = 0; i <= nsegs; ++i)
    if (s_colors[i])
      free(s_colors[i]);
    free(s_colors);
    free(perc);
    return 0;
  }

  /* sensible limits */
  if (npixels < 2)
    npixels = 2;
  if (npixels > MAX_GRADIENT_SEGMENTS)
    npixels = MAX_GRADIENT_SEGMENTS;

  /* send data back */
  *colors_return = s_colors;
  *perc_return = perc;
  *nsegs_return = nsegs;
  return npixels;
}



/* Create a pixmap from a gradient specifier, width and height are hints
 * that are only used for gradients that can be tiled e.g. H or V types
 * types are HVDBSCRY for Horizontal, Vertical, Diagonal, Back-diagonal, Square,
 * Circular, Radar and Yin/Yang respectively (in order of bloatiness)
 */
Pixmap CreateGradientPixmap(Display *dpy, Drawable d, GC gc, int type,
			    char *action, unsigned int *width_return,
			    unsigned int *height_return) {
  static unsigned int best_width = 0, best_height = 0;
  unsigned int width, height;
  Pixmap pixmap;
  Pixel *pixels;
  unsigned int ncolors = 0;
  char **colors;
  int *perc, nsegs;
  XGCValues xgcv;
  XImage *image;
  register int i, j;

  /* get the best tile size (once) */
  if (!best_width)
  {
    XQueryBestTile(dpy, d, 1, 1, &best_width, &best_height);
    /* this is needed for buggy X servers like XFree 3.3.3.1 */
    if (!best_width)
      best_width = 1;
    if (!best_height)
      best_height = 1;
  }

  /* translate the gradient string into an array of colors etc */
  if (!(ncolors = ParseGradient(action, &colors, &perc, &nsegs))) {
    fprintf(stderr, "Can't parse gradient: %s\n", action);
    return None;
  }
  /* grab the colors */
  pixels = AllocNonlinearGradient(colors, perc, nsegs, ncolors);
  for (i = 0; i <= nsegs; i++)
    if (colors[i])
      free(colors[i]);
  free(colors);
  free(perc);
  if (!pixels) {
    fprintf(stderr, "couldn't create gradient\n");
    return None;
  }
  /* grok the size to create from the type */
  type = toupper(type);
  switch (type) {
    case 'H':
      width = ncolors;
      height = best_height;
      break;
    case 'V':
      width = best_width;
      height = ncolors;
      break;
    case 'D':
    case 'B':
      /* diagonal gradients are rendered into a rectangle for which the
       * width plus the height is equal to ncolors + 1. The rectangle is square
       * when ncolors is odd and one pixel taller than wide with even numbers */
      width = (ncolors + 1) / 2;
      height = ncolors + 1 - width;
      break;
    case 'S':
      /* square gradients have the last color as a single pixel in the centre */
      width = height = 2 * ncolors - 1;
      break;
    case 'C':
      /* circular gradients have the first color as a pixel in each corner */
      width = height = 2 * ncolors - 1;
      break;
    case 'R':
    case 'Y':
      /* swept types need each color to occupy at least one pixel at the edge */
      /* get the smallest odd number that will provide enough */
      for (width = 1; (double)(width - 1) * M_PI < (double)ncolors; width += 2)
	;
      height = width;
      break;
    default:
      fprintf(stderr, "%cGradient not supported\n", type);
      return None;
  }
  /* create a pixmap to use */
  pixmap = XCreatePixmap(dpy, d, width, height, Pdepth);
  if (pixmap == None)
    return None;
  /* create an XImage structure */
  image = XCreateImage(dpy, Pvisual, Pdepth, ZPixmap, 0, 0, width, height,
                       Pdepth > 16 ? 32 : (Pdepth > 8 ? 16 : 8), 0);
  if (!image){
    fprintf(stderr, "%cGradient couldn't get image\n", type);
    XFreePixmap(dpy, pixmap);
    return None;
  }
  /* create space for drawing the image locally */
  image->data = safemalloc(image->bytes_per_line * height);
  /* now do the fancy drawing */
  /* draw one pixel further than expected in case line style is CapNotLast */
  switch (type) {
    case 'H':
      for (i = 0; i < width; i++) {
        register Pixel p = pixels[i];
        for (j = 0; j < height; j++)
          XPutPixel(image, i, j, p);
      }
      break;
    case 'V':
      for (j = 0; j < height; j++) {
        register Pixel p = pixels[j];
        for (i = 0; i < width; i++)
          XPutPixel(image, i, j, p);
      }
      break;
    case 'D':
      for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++)
          XPutPixel(image, i, j, pixels[i + j]);
      }
      break;
    case 'B':
      for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++)
          XPutPixel(image, i, j, pixels[i + ncolors / 2 - j]);
      }
      break;
    case 'S':
      {
        /* width == height so only reference one */
        register int w = width - 1;
        for (i = 0; i <= w; i++) {
          register int pi = min(i, w - i);
          for (j = 0; j <= w; j++) {
            register int pj = min(j, w - j);
            XPutPixel(image, i, j, pixels[min(pi, pj)]);
          }
        }
      }
      break;
    case 'C':
      /* width == height */
      for (i = 0; i < width; i++)
        for (j = 0; j < width; j++) {
          register int x = (i - width / 2), y = (j - width / 2);
          register int pixel = ncolors - 1 - sqrt((x * x + y * y) / 2);
          XPutPixel(image, i, j, pixels[pixel]);
        }
      break;
    case 'R':
      {
        register int r = (width - 1) / 2;
        /* width == height, both are odd, therefore x can be 0.0 */
        for (i = 0; i < width; i++) {
          for (j = 0; j < width; j++) {
            register double x = (i - r), y = (r - j);
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
            XPutPixel(image,i,j, pixels[(int)(angle * M_1_PI * 0.5 * ncolors)]);
          }
        }
      }
      break;
/* *************************************************************************
 * The Yin Yang gradient style and the following code are:
 * Copyright 1999 Sir Boris. (email to sir_boris@bigfoot.com may be read by
 * his groom but is not guaranteed to elicit a response)
 * No restrictions are placed on this code,
 * as long as the copyright notice is preserved.
 * ************************************************************************/
    case 'Y':
      {
        register int r = (width - 1) / 2;
        /* width == height, both are odd, therefore x can be 0.0 */
        for (i = 0; i < width; i++) {
          for (j = 0; j < width; j++) {
            register double x = (i - r), y = (r - j);
            register double dist = sqrt(x * x + y * y);
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
            if (dist <= r) {
              angle -= acos(dist / r);
            }
            /* move range from -pi/2:3*pi/2 to 0:2*pi */
            if (angle < 0.0)
              angle += M_PI * 2.0;
            /* normalize to gradient */
            XPutPixel(image,i,j, pixels[(int)(angle * M_1_PI * 0.5 * ncolors)]);
          }
        }
      }
      break;
    default:
      /* placeholder function, just fills the pixmap with the first color */
      memset(image->data, 0, image->bytes_per_line * width);
      XAddPixel(image, pixels[0]);
      break;
  }

  /* set the gc style */
  xgcv.function = GXcopy;
  xgcv.plane_mask = AllPlanes;
  xgcv.fill_style = FillSolid;
  xgcv.clip_mask = None;
  XChangeGC(dpy, gc, GCFunction|GCPlaneMask|GCFillStyle|GCClipMask, &xgcv);
  /* copy the image to the server */
  XPutImage(dpy, pixmap, gc, image, 0, 0, 0, 0, width, height);
  XDestroyImage(image);
  /* pass back info */
  *width_return = width;
  *height_return = height;
  return pixmap;
}
