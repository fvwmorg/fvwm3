/*
  Around 12/20/99 we did the 3rd rewrite of the shadow/hilite stuff.
  (That I know about (dje).
  The first stuff I saw just applied a percentage.
  Then we got some code from SCWM.
  This stuff comes from "Visual.c" which is part of Lesstif.
  Here's their copyright:

 * Copyright (C) 1995 Free Software Foundation, Inc.
 *
 * This file is part of the GNU LessTif Library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *

 The routine at the bottom "pixel_to_color_string" was not from Lesstif.

 Port by Dan Espen, no additional copyright
*/

#include "config.h"                     /* must be first */

#include <stdio.h>
#include <X11/Xproto.h>                 /* for X functions in general */
#include "fvwmlib.h"                    /* prototype GetShadow GetHilit */

#define	PCT_BRIGHTNESS			(6 * 0xffff / 100)

/* How much lighter/darker to make things in default routine */

#define	PCT_DARK_BOTTOM		70	/* lighter (less dark, actually) */
#define	PCT_DARK_TOP		50	/* lighter */
#define	PCT_LIGHT_BOTTOM	55	/* darker */
#define	PCT_LIGHT_TOP		80	/* darker */
#define	PCT_MEDIUM_BOTTOM_BASE	40	/* darker */
#define	PCT_MEDIUM_BOTTOM_RANGE	25
#define	PCT_MEDIUM_TOP_BASE	60	/* lighter */
#define	PCT_MEDIUM_TOP_RANGE	-30

/* The "brightness" of an RGB color.  The "right" way seems to use
 * empirical values like the default thresholds below, but it boils down
 * to red is twice as bright as blue and green is thrice blue.
 */

#define	BRIGHTNESS ((int)red + (int)red + (int)green + (int)green + (int)green + (int)blue)

/* From Xm.h on Solaris */
#define XmDEFAULT_DARK_THRESHOLD        15
#define XmDEFAULT_LIGHT_THRESHOLD       85
extern Colormap Pcmap;
extern Display *Pdpy;

static XColor color;

/**** This part is the olf fvwm way to calculate colours. Still used for
 **** 'medium' brigness colours. */
#define DARKNESS_FACTOR 0.5
#define BRIGHTNESS_FACTOR 1.4
#define SCALE 65535.0
#define HALF_SCALE (SCALE / 2)
typedef enum {
  R_MAX_G_MIN, R_MAX_B_MIN,
  G_MAX_B_MIN, G_MAX_R_MIN,
  B_MAX_R_MIN, B_MAX_G_MIN
} MinMaxState;
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
/**** End of original fvwm code. ****/

XColor *GetShadowColor(Pixel background)
{
  long brightness, brightadj;
  unsigned short red, green, blue;

  color.pixel = background;
  XQueryColor (Pdpy, Pcmap, &color);
  red = color.red;
  green = color.green;
  blue = color.blue;

  brightness = BRIGHTNESS;
  /* For "dark" backgrounds, make everything a fixed %age lighter */
  if (brightness < XmDEFAULT_DARK_THRESHOLD * PCT_BRIGHTNESS)
    {
      color.red = 0xffff - ((0xffff - red) * PCT_DARK_BOTTOM) / 100;
      color.green = 0xffff - ((0xffff - green) * PCT_DARK_BOTTOM + 50) / 100;
      color.blue = 0xffff - ((0xffff - blue) * PCT_DARK_BOTTOM + 50) / 100;
    }
  /* For "light" background, make everything a fixed %age darker */
  else if (brightness > XmDEFAULT_LIGHT_THRESHOLD * PCT_BRIGHTNESS)
    {
      color.red = (red * PCT_LIGHT_BOTTOM + 50) / 100;
      color.green = (green * PCT_LIGHT_BOTTOM + 50) / 100;
      color.blue = (blue * PCT_LIGHT_BOTTOM + 50) / 100;
    }
  /* For "medium" background, select is a fixed %age darker;
   * top (lighter) and bottom (darker) are a variable %age
   * based on the background's brightness
   */
  else
    {
#if 1
      color_mult(&color.red, &color.green, &color.blue, DARKNESS_FACTOR);
#else
      brightness = (brightness + (PCT_BRIGHTNESS >> 1)) / PCT_BRIGHTNESS;
      brightadj = PCT_MEDIUM_BOTTOM_BASE +
        (brightness * PCT_MEDIUM_BOTTOM_RANGE + 50) / 100;
      color.red = (red * brightadj + 50) / 100;
      color.green = (green * brightadj + 50) / 100;
      color.blue = (blue * brightadj + 50) / 100;
#endif
    }
  return &color;
}

Pixel GetShadow(Pixel background)
{
  XColor *colorp;

  colorp = GetShadowColor(background);
  XAllocColor (Pdpy, Pcmap, colorp);
  return colorp->pixel;
}

XColor *GetHiliteColor(Pixel background)
{
  long brightness, brightadj;
  unsigned short red, green, blue;

  color.pixel = background;
  XQueryColor (Pdpy, Pcmap, &color);
  red = color.red;
  green = color.green;
  blue = color.blue;

  brightness = BRIGHTNESS;
  /* For "dark" backgrounds, make everything a fixed %age lighter */
  if (brightness < XmDEFAULT_DARK_THRESHOLD * PCT_BRIGHTNESS)
    {
      color.red = 0xffff - ((0xffff - red) * PCT_DARK_TOP + 50) / 100;
      color.green = 0xffff - ((0xffff - green) * PCT_DARK_TOP + 50) / 100;
      color.blue = 0xffff - ((0xffff - blue) * PCT_DARK_TOP + 50) / 100;
    }
  /* For "light" background, make everything a fixed %age darker */
  else if (brightness > XmDEFAULT_LIGHT_THRESHOLD * PCT_BRIGHTNESS)
    {
      color.red = (red * PCT_LIGHT_TOP + 50) / 100;
      color.green = (green * PCT_LIGHT_TOP + 50) / 100;
      color.blue = (blue * PCT_LIGHT_TOP + 50) / 100;
    }
  /* For "medium" background, select is a fixed %age darker;
   * top (lighter) and bottom (darker) are a variable %age
   * based on the background's brightness
   */
  else
    {
#if 1
      color_mult(&color.red, &color.green, &color.blue, BRIGHTNESS_FACTOR);
#else
      brightness = (brightness + (PCT_BRIGHTNESS >> 1)) / PCT_BRIGHTNESS;
      brightadj = PCT_MEDIUM_TOP_BASE +
        (brightness * PCT_MEDIUM_TOP_RANGE + 50) / 100;
      color.red = 0xffff - ((0xffff - red) * brightadj + 50) / 100;
      color.green = 0xffff - ((0xffff - green) * brightadj + 50) / 100;
      color.blue = 0xffff - ((0xffff - blue) * brightadj + 50) / 100;
#endif
    }
  return &color;
}

Pixel GetHilite(Pixel background)
{
  XColor *colorp;

  colorp = GetHiliteColor(background);
  XAllocColor (Pdpy, Pcmap, colorp);
  return colorp->pixel;
}

/* This function converts the colour stored in a colorcell (pixel) into the
 * string representation of a colour.  The output is printed at the
 * address 'output'.  It is either in rgb format ("rgb:rrrr/gggg/bbbb") if
 * use_hash is False or in hash notation ("#rrrrggggbbbb") if use_hash is true.
 * The return value is the number of characters used by the string.  The
 * rgb values of the output are undefined if the colorcell is invalid.  The
 * memory area pointed at by 'output' must be at least 64 bytes (in case of
 * future extensions and multibyte characters).*/
int pixel_to_color_string(
  Display *dpy, Colormap cmap, Pixel pixel, char *output, Bool use_hash)
{
  XColor color;
  int n;

  color.pixel = pixel;
  color.red = 0;
  color.green = 0;
  color.blue = 0;

  XQueryColor(dpy, cmap, &color);
  if (!use_hash)
  {
    sprintf(output, "rgb:%04x/%04x/%04x%n", (int)color.red, (int)color.green,
	    (int)color.blue, &n);
  }
  else
  {
    sprintf(output, "#%04x%04x%04x%n", (int)color.red, (int)color.green,
	    (int)color.blue, &n);
  }

  return n;
}
