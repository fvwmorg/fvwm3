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
#include <X11/Intrinsic.h>              /* needed for Pixel defn */


/* The hilight/shadow stuff should maybe be in a separate relief.c? */

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

void 
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
	  min_max_state = G_MAX_R_MIN;
	  a = r - b;
	} else {
	  min = r;
	  min_max_state = G_MAX_B_MIN;
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


Pixel
adjust_pixel_brightness(Pixel pixel, double factor)
{
  XColor c;
  extern Colormap PictureCMap;
  extern Display *PictureSaveDisplay;
  c.pixel = pixel;
  
  XQueryColor (PictureSaveDisplay, PictureCMap, &c);
  color_mult(&c.red, &c.green, &c.blue, factor);
  XAllocColor (PictureSaveDisplay, PictureCMap, &c);

  return c.pixel;
}
Pixel GetShadow(Pixel background) {
 return adjust_pixel_brightness(background, 0.7);
}
Pixel GetHilite(Pixel background) {
 return adjust_pixel_brightness(background, 1.3);
}
