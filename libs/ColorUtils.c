/*
 * The following GPL code from scwm  implements a good, fast 3D-shadowing
 * algorithm. It   converts  the color  from  RGB   to  HLS  space,  then
 * multiplies both the  luminosity   and the saturation by  a   specified
 * factor (clipping at the  extremes). Then it  converts back to  RGB and
 * creates a color. The guts of it, i.e.  the `color_mult' routine, looks
 * a bit longish, but this  is only because  there are 6-way conditionals
 * at the begining and end; it actually runs quite fast. The algorithm is
 * the same  as Gtk's,  but  the  implemenation  is independent and  more
 * streamlined.
 *
 * Calling `adjust_pixel_brightness' with  a `factor' of 1.3 for hilights
 * and 0.7 for  shadows exactly emulates  Gtk's shadowing, which is,  IMO
 * the most visually pleasing shadowing of any widget  set; using 1.2 and
 * 0.5  respectively gives something closer to  the "classic" fvwm effect
 * with deeper shadows and more subtle hilights, but still (IMO) smoother
 * and more attractive than fvwm.
 *
 * The only color these  routines do not  usefully handle is black; black
 * will be returned even  for a factor  greater than 1.0, when  optimally
 * one  would like  to  see a  very dark  gray.  This could   possibly be
 * addressed by  adding   a  small   additive factor  when    brightening
 * colors. If anyone adds that feature, please feed it upstream to me.
 *
 * Feel free to use this code in fvwm2, of course.
 *
 * - Maciej Stachowiak
 *
 * And, of course, history shows, we took him up on the offer.
 * Integrated into fvwm2 by Dan Espen, 11/13/98.
 */


/*
 * Copyright (C) 1997, 1998, Maciej Stachowiak and Greg J. Badros
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.GPL.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 *
 */

#include "config.h"                     /* must be first */

#include <stdio.h>
#include <X11/Xproto.h>                 /* for X functions in general */
#include "fvwmlib.h"                    /* prototype GetShadow GetHilit */

#define SCALE 65535.0
#define HALF_SCALE (SCALE / 2)
typedef enum {
  R_MAX_G_MIN, R_MAX_B_MIN, 
  G_MAX_B_MIN, G_MAX_R_MIN, 
  B_MAX_R_MIN, B_MAX_G_MIN 
} MinMaxState;


/* Multiply the HLS-space lightness and saturation of the color by the
   given multiple, k - based on the way gtk does shading, but independently
   coded. Should give better relief colors for many cases than the old
   fvwm algorithm. */

/* FIXMS: This can probably be optimized more, examine later. */

static void 
color_mult (unsigned short *red, 
	    unsigned short *green, 
	    unsigned short *blue, double k)
{
  if (*red == *green && *red == *blue) {
    double temp;
    /* A shade of gray */
    temp = k * (double) (*red);
    if (temp > SCALE) {
      temp = SCALE;
    }
    *red = (unsigned short)(temp);
    *green = *red;
    *blue = *red;
  } else {
    /* Non-zero saturation */
    double r, g, b;
    double min, max;
    double a, l, s;
    double delta;
    double middle;
    MinMaxState min_max_state;

    r = (double) *red;
    g = (double) *green;
    b = (double) *blue;

    if (r > g) {
      if (r > b) {
	max = r;
	if (g < b) {
	  min = g;
	  min_max_state = R_MAX_G_MIN;
	  a = b - g;
	} else {
	  min = b;
	  min_max_state = R_MAX_B_MIN;
	  a = g - b;
	}
      } else {
	max = b;
	min = g;
	min_max_state = B_MAX_G_MIN;
	a = r - g;
      }
    } else {
      if (g > b) {
	max = g;
	if (b < r) {
	  min = b;
	  min_max_state = G_MAX_B_MIN;
	  a = r - b;
	} else {
	  min = r;
	  min_max_state = G_MAX_R_MIN;
	  a = b - r; 
	}
      } else {
	max = b;
	min = r;
	min_max_state = B_MAX_R_MIN;
	a = g - r;
      }
    }
    
    delta = max - min;
    a = a / delta;

    l = (max + min) / 2;
    if (l <= HALF_SCALE) {
      s = max + min;
    } else {
      s = 2.0 * SCALE - (max + min);
    }
    s = delta/s;
    
    l *= k;
    if (l > SCALE) {
      l = SCALE;
    }
    s *= k;
    if (s > 1.0) {
      s = 1.0;
    }

    if (l <= HALF_SCALE) {
      max = l * (1 + s);
    } else {
      max = s * SCALE + l - s * l; 
    }

    min = 2 * l - max;
    delta = max - min;
    middle = min + delta * a;

    switch (min_max_state) {
    case R_MAX_G_MIN:
      r = max;
      g = min;
      b = middle;
      break;
    case R_MAX_B_MIN:
      r = max;
      g = middle;
      b = min;
      break;
    case G_MAX_B_MIN:
      r = middle;
      g = max;
      b = min;
      break;
    case G_MAX_R_MIN:
      r = min;
      g = max;
      b = middle;
      break;
    case B_MAX_G_MIN:
      r = middle;
      g = min;
      b = max;
      break;
    case B_MAX_R_MIN:
      r = min;
      g = middle;
      b = max;
      break;
    }

    *red = (unsigned short) r;
    *green = (unsigned short) g;
    *blue = (unsigned short) b;    
  }
}

/*
 * This  routine  uses PictureSaveDisplay  and PictureCMap  which must be
 * created by InitPictureCMAP in Picture.c.
 *
 * If you  attempt to use GetShadow  and GetHilit, make  sure your module
 * calls InitPictureCMAP first.
 */
static Pixel
adjust_pixel_brightness(Pixel pixel, double factor)
{
  extern Colormap PictureCMap;
  extern Display *PictureSaveDisplay;
  XColor c;
  c.pixel = pixel;
  XQueryColor (PictureSaveDisplay, PictureCMap, &c);
  color_mult(&c.red, &c.green, &c.blue, factor);
  XAllocColor (PictureSaveDisplay, PictureCMap, &c);

  return c.pixel;
}

/*
 * These  are  the original fvwm2  APIs, one  for highlights  and one for
 * shadows.   Together, if  used  in a frame    around a rectangle,  they
 * produce a 3d appearance.
 *
 * The  input  pixel,   is normally  the  background   color used in  the
 * rectangle.  One would  hope, when the  user selects to color something
 * with a multi-color pixmap, they will have the insight to also assign a
 * background color to the pixmaped  area  that approximates the  average
 * color of the pixmap.
 *
 * Currently callers handle monochrome before calling  this routine.  The
 * next logical enhancement is for that logic to be moved here.  Probably
 * a  new   API   that  deals   with  foreground/background/hilite/shadow
 * allocation all in 1 call is the next logical extenstion.
 *
 * Color  allocation  is also a  good   candidate for becoming  a library
 * routine.  The color   allocation  logic in FvwmButtons using   the XPM
 * library closeness stuff may be the ideal model.
 * (dje 11/15/98)
 */
#define DARKNESS_FACTOR 0.5
Pixel GetShadow(Pixel background) {
  return adjust_pixel_brightness(background, DARKNESS_FACTOR);
}

#define BRIGHTNESS_FACTOR 1.4
Pixel GetHilite(Pixel background) {
  return adjust_pixel_brightness(background, BRIGHTNESS_FACTOR);
}
