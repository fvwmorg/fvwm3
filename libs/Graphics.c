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

#include "defaults.h"
#include "libs/fvwmlib.h"
#include "libs/Picture.h"

#include <X11/Xlib.h>
#include <stdio.h>
#include <math.h>


/* Draws the relief pattern around a window
 * Draws a line_width wide rectangle from (x,y) to (x+w,y+h) i.e w+1 wide,
 * h+1 high
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
    seg[i].x2 = x+i; seg[i].y2 = y+h-i-1;
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
    seg[i].x1 = x+w-i; seg[i].y1 = y+h-i-1;
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
Pixmap CreateStretchXPixmap(Display *dpy, Pixmap src, int src_width,
			    int src_height, int src_depth,
			    int dest_width, GC gc)
{
  int i;
  Pixmap pixmap;

  if (src_width < 0 || src_height < 0 || dest_width < 0)
  	return None;

  pixmap = XCreatePixmap(dpy, src, dest_width, src_height, src_depth);

  if (pixmap)
    for (i = 0; i < dest_width; i++)
      XCopyArea(dpy, src, pixmap, gc, (i * src_width) / dest_width, 0,
		1, src_height, i, 0);

  return pixmap;
}


/* Creates a pixmap that is a vertically stretched version of the input
 * pixmap
 */
Pixmap CreateStretchYPixmap(Display *dpy, Pixmap src, int src_width,
			    int src_height, int src_depth,
			    int dest_height, GC gc)
{
  int i;
  Pixmap pixmap;

  if (src_height < 0 || src_depth < 0 || dest_height < 0)
  	return None;

  pixmap = XCreatePixmap(dpy, src, src_width, dest_height, src_depth);

  if (pixmap)
    for (i = 0; i < dest_height; i++)
      XCopyArea(dpy, src, pixmap, gc, 0, (i * src_height) / dest_height,
		src_width, 1, 0, i);

  return pixmap;
}


/* Creates a pixmap that is a stretched version of the input
 * pixmap
 */
Pixmap CreateStretchPixmap(Display *dpy, Pixmap src, int src_width,
			    int src_height, int src_depth,
			    int dest_width, int dest_height,
			    GC gc)
{
  Pixmap pixmap = None;
  Pixmap temp_pixmap;

  if (src_width < 0 || src_height < 0 || src_depth < 0 || dest_width < 0)
    return None;

  temp_pixmap = CreateStretchXPixmap(dpy, src, src_width, src_height,
					    src_depth, dest_width, gc);
  if (temp_pixmap)
  {
    pixmap = CreateStretchYPixmap(dpy, temp_pixmap, dest_width, src_height,
				  src_depth, dest_height, gc);
    XFreePixmap(dpy, temp_pixmap);
  }

  return pixmap;
}


/* Creates a pixmap that is a tiled version of the input pixmap. */
Pixmap CreateTiledPixmap(Display *dpy, Pixmap src, int src_width,
			 int src_height, int dest_width,
			 int dest_height, int depth, GC gc)
{
  int x;
  int y;
  Pixmap pixmap;

  if (src_width < 0 || src_height < 0 || dest_width < 0 || dest_height < 0)
  	return None;

  pixmap = XCreatePixmap(dpy, src, dest_width, dest_height, depth);

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
 * Returns True if the given type of gradient is supported.
 *
 ****************************************************************************/
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
    fprintf(stderr, "%cGradient type is not supported\n", toupper(type));
    return False;
  }

}

/****************************************************************************
 *
 * Allocates a linear color gradient (veliaa@rpi.edu)
 *
 ****************************************************************************/
Pixel *AllocLinearGradient(
  char *s_from, char *s_to, int npixels, int skip_first_color)
{
  Pixel *pixels;
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
    fprintf(stderr, "AllocLinearGradient: Invalid number of pixels: %d\n",
	    npixels);
    return NULL;
  }
  if (!s_from || !XParseColor(Pdpy, Pcmap, s_from, &from))
  {
    fprintf(stderr, "Cannot parse color \"%s\"\n", s_from ? s_from : "<blank>");
    return NULL;
  }
  if (!s_to || !XParseColor(Pdpy, Pcmap, s_to, &to))
  {
    fprintf(stderr, "Cannot parse color \"%s\"\n", s_to ? s_to : "<blank>");
    return NULL;
  }

#define DEBUG_ALG 0
#if DEBUG_ALG
fprintf(stderr,"\n*** from '%s to '%s' skip %d\n", s_from?s_from:"", s_to?s_to:"", skip_first_color);
fprintf(stderr,"*** from.red %d, from.green %d, from.red %d\n", (int)from.red, (int)from.green, (int)from.blue);
fprintf(stderr,"*** to.red %d, to.green %d, to.red %d\n", (int)to.red, (int)to.green, (int)to.blue);
#endif
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
#if DEBUG_ALG
fprintf(stderr,"*** div %d,, c.red %d, c.green %d, c.blue %d, t %f, g %f, b %f, dr %f, dg %f, db %f\n", div, (int)c.red, (int)c.green, (int)c.blue, r, g, b, dr, dg, db);
#endif
  pixels = (Pixel *)safemalloc(sizeof(Pixel) * npixels);
  memset(pixels, 0, sizeof(Pixel) * npixels);
  c.flags = DoRed | DoGreen | DoBlue;
  for (i = (skip_first_color) ? 1 : 0; i < npixels && div > 0; ++i)
  {
    c.red   = (unsigned short)((int)(r + dr / (float)div * (float)i + 0.5));
    c.green = (unsigned short)((int)(g + dg / (float)div * (float)i + 0.5));
    c.blue  = (unsigned short)((int)(b + db / (float)div * (float)i + 0.5));
#if DEBUG_ALG
fprintf(stderr, "*** %i: c.red %d, c.green %d, c.blue %d\n", i, (int)c.red, (int)c.green, (int)c.blue);
#endif
    if (!XAllocColor(Pdpy, Pcmap, &c))
    {
      got_all = 0;
#if DEBUG_ALG
fprintf(stderr, "*** could not get colour %d\n", i);
#endif
    }
#if DEBUG_ALG
else
{
fprintf(stderr, "*** final color: c.p %d, dc.red %d, c.green %d, c.blue %d\n", (int)c.pixel, (int)c.red, (int)c.green, (int)c.blue);
}
#endif
    pixels[i] = c.pixel;
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
Pixel *AllocNonlinearGradient(
  char *s_colors[], int clen[], int nsegs, int npixels)
{
  Pixel *pixels = (Pixel *)safemalloc(sizeof(Pixel) * npixels);
  int i;
  int curpixel = 0;
  int *seg_end_colors;
  int seg_sum = 0;
  float color_sum = 0.0;

  if (nsegs < 1 || npixels < 2)
  {
    fprintf(stderr,
	    "Gradients must specify at least one segment and two colors\n");
    free(pixels);
    return NULL;
  }
  for (i = 0; i < npixels; i++)
    pixels[i] = 0;

  /* get total length of all segments */
  for (i = 0; i < nsegs; i++)
    seg_sum += clen[i];

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
      color_sum += (float)(clen[i] * (npixels - 1)) / (float)(seg_sum);
      seg_end_colors[i] = (int)(color_sum + 0.5);
    }
    if (seg_end_colors[nsegs - 1] > npixels - 1)
    {
      fprintf(stderr,
	      "BUG: (AllocNonlinearGradient): "
	      "seg_end_colors[nsegs - 1] > npixels - 1\n");
      abort();
      exit(1);
    }
    /* take care of rounding errors */
    seg_end_colors[nsegs - 1] = npixels - 1;
  }

  for (i = 0; i < nsegs; ++i)
  {
    Pixel *p = NULL;
    int j;
    int n;
    int skip_first_color = (curpixel != 0);

    if (i == 0)
      n = seg_end_colors[0] + 1;
    else
      n = seg_end_colors[i] - seg_end_colors[i - 1] + 1;

    if (n > 1)
    {
      p = AllocLinearGradient(
	s_colors[i], s_colors[i + 1], n, skip_first_color);
      if (!p && (n - skip_first_color) != 0)
      {
	free(pixels);
	return NULL;
      }
      for (j = skip_first_color; j < n; ++j)
	pixels[curpixel + j] = p[j];
      curpixel += n - 1;
    }
    if (curpixel != seg_end_colors[i])
    {
      fprintf(stderr,
	      "BUG: (AllocNonlinearGradient): "
	      "i = %d, curpixel = %d, seg_end_colors[i] = %d\n", i, curpixel,
	      seg_end_colors[i]);
      abort();
      exit(1);
    }
    if (p)
    {
      free(p);
      p = NULL;
    }
  }

  return pixels;
}

/* Convenience function. Calls AllocNonLinearGradient to fetch all colors and
 * then frees the color names and the perc and color_name arrays. */
Pixel *AllocAllGradientColors(char *color_names[], int perc[],
			      int nsegs, int ncolors)
{
  Pixel *pixels = None;
  int i;

  /* grab the colors */
  pixels = AllocNonlinearGradient(color_names, perc, nsegs, ncolors);
  for (i = 0; i <= nsegs; i++)
  {
    if (color_names[i])
      free(color_names[i]);
  }
  free(color_names);
  free(perc);
  if (!pixels)
  {
    fprintf(stderr, "couldn't create gradient\n");
    return None;
  }

  return pixels;
}

/* groks a gradient string and creates arrays of colors and percentages
 * returns the number of colors asked for (No. allocated may be less due
 * to the ColorLimit command).  A return of 0 indicates an error
 */
unsigned int ParseGradient(char *gradient, char **rest, char ***colors_return,
			   int **perc_return, int *nsegs_return)
{
  char *item;
  char *orig;
  unsigned int npixels;
  char **s_colors;
  int *perc;
  int nsegs, i, sum;
  Bool is_syntax_error = False;

  /* get the number of colors specified */
  if (rest)
    *rest = gradient;
  orig = gradient;

  if (GetIntegerArguments(gradient, &gradient, (int *)&npixels, 1) != 1 ||
      npixels < 2)
  {
    fprintf(
      stderr, "ParseGradient: illegal number of colors in gradient: '%s'\n",
      orig);
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
      free(item);
    if (rest)
      *rest = gradient;
    return 0;
  }

  if (GetIntegerArguments(item, NULL, &nsegs, 1) != 1)
  {
    /* get the end color of a simple gradient */
    s_colors = (char **)safemalloc(sizeof(char *) * 2);
    perc = (int *)safemalloc(sizeof(int));
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
    s_colors = (char **)safemalloc(sizeof(char *) * (nsegs + 1));
    perc = (int *)safemalloc(sizeof(int) * nsegs);
    for (i = 0; !is_syntax_error && i <= nsegs; i++)
    {
      s_colors[i] = 0;
      gradient = GetNextToken(gradient, &s_colors[i]);
      if (i < nsegs)
      {
	if (GetIntegerArguments(gradient, &gradient, &perc[i], 1) != 1 ||
	    perc[i] <= 0)
	{
	  /* illegal size */
	  perc[i] = 0;
	}
      }
    }
    if (s_colors[nsegs] == NULL)
    {
      fprintf(
	stderr, "ParseGradient: too few gradient segments: '%s'\n", orig);
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
	stderr, "ParseGradient: multi gradient overflow: '%s'", orig);
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
  Display *dpy, Drawable d, int ncolors, char type,
  unsigned int *width_ret, unsigned int *height_ret)
{
  static unsigned int best_width = 0, best_height = 0;

  /* get the best tile size (once) */
  if (!best_width)
  {
    if (!XQueryBestTile(dpy, d, 1, 1, &best_width, &best_height))
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
      *height_ret = best_height;
      break;
    case V_GRADIENT:
      *width_ret = best_width;
      *height_ret = ncolors;
      break;
    case D_GRADIENT:
    case B_GRADIENT:
      /* diagonal gradients are rendered into a rectangle for which the
       * width plus the height is equal to ncolors + 1. The rectangle is square
       * when ncolors is odd and one pixel taller than wide with even numbers */
      *width_ret = (ncolors + 1) / 2;
      *height_ret = ncolors + 1 - *width_ret;
      break;
    case S_GRADIENT:
      /* square gradients have the last color as a single pixel in the centre */
      *width_ret = *height_ret = 2 * ncolors - 1;
      break;
    case C_GRADIENT:
      /* circular gradients have the first color as a pixel in each corner */
      *width_ret = *height_ret = 2 * ncolors - 1;
      break;
    case R_GRADIENT:
    case Y_GRADIENT:
      /* swept types need each color to occupy at least one pixel at the edge */
      /* get the smallest odd number that will provide enough */
      for (*width_ret = 1;
	   (double)(*width_ret - 1) * M_PI < (double)ncolors;
	   *width_ret += 2)
	;
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
  Display *dpy, Drawable d, GC gc, int type, int g_width, int g_height,
  int ncolors, Pixel *pixels, Drawable in_drawable, int d_x, int d_y,
  int d_width, int d_height, XRectangle *rclip)
{
  Pixmap pixmap = None;
  XImage *image;
  register int i, j;
  XGCValues xgcv;
  Drawable target;
  int t_x;
  int t_y;
  unsigned int t_width;
  unsigned int t_height;

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

  /* create an XImage structure */
  image = XCreateImage(dpy, Pvisual, Pdepth, ZPixmap, 0, 0, t_width, t_height,
                       Pdepth > 16 ? 32 : (Pdepth > 8 ? 16 : 8), 0);
  if (!image)
  {
    fprintf(stderr, "%cGradient couldn't get image\n", type);
    if (pixmap != None)
      XFreePixmap(dpy, pixmap);
    return None;
  }
  /* create space for drawing the image locally */
  image->data = safemalloc(image->bytes_per_line * t_height);
  /* now do the fancy drawing */
  /* draw one pixel further than expected in case line style is CapNotLast */
  switch (type)
  {
    case H_GRADIENT:
      {
	for (i = 0; i < t_width; i++) {
	  register Pixel p = pixels[i * ncolors / t_width];
	  for (j = 0; j < t_height; j++)
	    XPutPixel(image, i, j, p);
	}
      }
      break;
    case V_GRADIENT:
      {
	for (j = 0; j < t_height; j++) {
	  register Pixel p = pixels[j * ncolors / t_height];
	  for (i = 0; i < t_width; i++)
	    XPutPixel(image, i, j, p);
	}
	break;
      }
    case D_GRADIENT:
      {
	register int t_scale = t_width + t_height - 1;
	for (i = 0; i < t_width; i++) {
	  for (j = 0; j < t_height; j++)
	    XPutPixel(image, i, j, pixels[(i + j) * ncolors / t_scale]);
	}
	break;
      }
    case B_GRADIENT:
      {
	register int t_scale = t_width + t_height - 1;
	for (i = 0; i < t_width; i++) {
	  for (j = 0; j < t_height; j++)
	    XPutPixel(image, i, j,
		      pixels[(i + (t_height - j - 1)) * ncolors / t_scale]);
	}
	break;
      }
    case S_GRADIENT:
      {
        register int w = t_width - 1;
        register int h = t_height - 1;
	register int t_scale = w * h;
	register int myncolors = ncolors * 2;
        for (i = 0; i <= w; i++) {
          register int pi = min(i, w - i) * h;
          for (j = 0; j <= h; j++) {
            register int pj = min(j, h - j) * w;
            XPutPixel(image, i, j,
		      pixels[(min(pi, pj) * myncolors - 1) / t_scale]);
          }
        }
      }
      break;
    case C_GRADIENT:
      {
	register int w = t_width - 1;
	register int h = t_height - 1;
	register double t_scale = (double)(w * h) / sqrt(8);

	for (i = 0; i <= w; i++)
	  for (j = 0; j <= h; j++) {
            register double x = (double)((2 * i - w) * h) / 4.0;
            register double y = (double)((h - 2 * j) * w) / 4.0;
            register double rad = sqrt(x * x + y * y);
	    XPutPixel(image, i, j,
		      pixels[(int)((rad * ncolors - 0.5) / t_scale)]);
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
            register double x = (double)((2 * i - w) * h) / 4.0;
            register double y = (double)((h - 2 * j) * w) / 4.0;
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
    case Y_GRADIENT:
      {
	register int w = t_width - 1;
	register int h = t_height - 1;
	register int r = w * h / 4;

        for (i = 0; i <= w; i++) {
          for (j = 0; j <= h; j++) {
            register double x = (double)((2 * i - w) * h) / 4.0;
            register double y = (double)((h - 2 * j) * w) / 4.0;
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
            XPutPixel(image,i,j, pixels[(int)(angle * M_1_PI * 0.5 * ncolors)]);
          }
        }
      }
      break;
    default:
      /* placeholder function, just fills the pixmap with the first color */
      memset(image->data, 0, image->bytes_per_line * g_width);
      XAddPixel(image, pixels[0]);
      break;
  }

  /* set the gc style */
  xgcv.function = GXcopy;
  xgcv.plane_mask = AllPlanes;
  xgcv.fill_style = FillSolid;
  xgcv.clip_mask = None;
  XChangeGC(dpy, gc, GCFunction|GCPlaneMask|GCFillStyle|GCClipMask, &xgcv);
  if (rclip)
  {
    XSetClipRectangles(dpy, gc, 0, 0, rclip, 1, Unsorted);
  }
  /* copy the image to the server */
  XPutImage(dpy, target, gc, image, 0, 0, t_x, t_y, t_width, t_height);
  if (rclip)
  {
    XSetClipMask(dpy, gc, None);
  }
  XDestroyImage(image);
  return target;
}


/* Create a pixmap from a gradient specifier, width and height are hints
 * that are only used for gradients that can be tiled e.g. H or V types
 * types are HVDBSCRY for Horizontal, Vertical, Diagonal, Back-diagonal, Square,
 * Circular, Radar and Yin/Yang respectively (in order of bloatiness)
 */
Pixmap CreateGradientPixmapFromString(Display *dpy, Drawable d, GC gc,
				      int type, char *action,
				      unsigned int *width_return,
				      unsigned int *height_return,
				      Pixel **pixels_return, int *nalloc_pixels)
{
  Pixel *pixels;
  unsigned int ncolors = 0;
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
  pixels = AllocAllGradientColors(colors, perc, nsegs, ncolors);
  if (pixels == None)
    return None;

  /* grok the size to create from the type */
  type = toupper(type);

  if (CalculateGradientDimensions(
	dpy, d, ncolors, type, width_return, height_return))
  {
    pixmap = CreateGradientPixmap(
      dpy, d, gc, type, *width_return, *height_return, ncolors, pixels, None,
      0, 0, 0, 0, NULL);
  }

  /* if the caller has not asked for the pixels there is probably a leak */
  if (!pixels_return)
  {
    fprintf(stderr,
	    "CreateGradient: potential color leak, losing track of pixels\n");
    free(pixels);
  }
  else
  {
    *pixels_return = pixels;
  }

  if (nalloc_pixels)
    *nalloc_pixels = ncolors;

  return pixmap;
}

/****************************************************************************
 *
 *  Draws a little Triangle pattern within a window
 *
 ****************************************************************************/
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
    XFillPolygon(dpy, d, FillGC, points, 3, Convex, CoordModeOrigin);
  }
  if (draw_relief)
  {
    /* relief triangle */
    for (i = 0; i < 3; i++)
    {
      temp_gc = (is_pressed ^ hi[type].line[i]) ? ReliefGC : ShadowGC;
      XDrawLine(dpy, d, temp_gc, points[i].x, points[i].y,
		points[i+1].x, points[i+1].y);
    }
    for (i = 0; i < 3; i++)
    {
      temp_gc = (is_pressed ^ hi[type].point[i]) ? ReliefGC : ShadowGC;
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
    values = &gcv;
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

