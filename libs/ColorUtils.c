/* -*-c-*- */
/*
 * Around 12/20/99 we did the 3rd rewrite of the shadow/hilite stuff.
 * (That I know about (dje).
 * The first stuff I saw just applied a percentage.
 * Then we got some code from SCWM.
 * This stuff comes from "Visual.c" which is part of Lesstif.
 * Here's their copyright:
 *
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
 * License along with this library; if not, see:
 * <http://www.gnu.org/licenses/>
 *
 * The routine at the bottom "pixel_to_color_string" was not from Lesstif.
 *
 * Port by Dan Espen, no additional copyright
 */

#include "config.h"                     /* must be first */

#include <stdio.h>
#include <X11/Xproto.h>                 /* for X functions in general */
#include "fvwmlib.h"                    /* prototype GetShadow GetHilit */
#include "Parse.h"
#include "Colorset.h"
#include "PictureBase.h"
#include "PictureUtils.h"
#include "ColorUtils.h"

#define PCT_BRIGHTNESS                  (6 * 0xffff / 100)

/* How much lighter/darker to make things in default routine */

#define PCT_DARK_BOTTOM         70      /* lighter (less dark, actually) */
#define PCT_DARK_TOP            50      /* lighter */
#define PCT_LIGHT_BOTTOM        55      /* darker */
#define PCT_LIGHT_TOP           80      /* darker */
#define PCT_MEDIUM_BOTTOM_BASE  40      /* darker */
#define PCT_MEDIUM_BOTTOM_RANGE 25
#define PCT_MEDIUM_TOP_BASE     60      /* lighter */
#define PCT_MEDIUM_TOP_RANGE    -30

/* The "brightness" of an RGB color.  The "right" way seems to use
 * empirical values like the default thresholds below, but it boils down
 * to red is twice as bright as blue and green is thrice blue.
 */

#define BRIGHTNESS(r,g,b) (2*(int)(r) + 3*(int)(g) + 1*(int)(b))

/* From Xm.h on Solaris */
#define XmDEFAULT_DARK_THRESHOLD        15
#define XmDEFAULT_LIGHT_THRESHOLD       85

static XColor color;

/**** This part is the old fvwm way to calculate colours. Still used for
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

static XColor *GetShadowOrHiliteColor(
	Pixel background, float light, float dark, float factor)
{
	long brightness;
	unsigned int red, green, blue;

	memset(&color, 0, sizeof(color));
	color.pixel = background;
	XQueryColor(Pdpy, Pcmap, &color);
	red = color.red;
	green = color.green;
	blue = color.blue;

	brightness = BRIGHTNESS(red, green, blue);
	/* For "dark" backgrounds, make everything a fixed %age lighter */
	if (brightness < XmDEFAULT_DARK_THRESHOLD * PCT_BRIGHTNESS)
	{
		color.red = (unsigned short)
			(0xffff - ((0xffff - red) * dark + 50) / 100);
		color.green = (unsigned short)
			(0xffff - ((0xffff - green) * dark + 50) / 100);
		color.blue = (unsigned short)
			(0xffff - ((0xffff - blue) * dark + 50) / 100);
	}
	/* For "light" background, make everything a fixed %age darker */
	else if (brightness > XmDEFAULT_LIGHT_THRESHOLD * PCT_BRIGHTNESS)
	{
		color.red =
			(unsigned short)((red * light + 50) / 100);
		color.green =
			(unsigned short)((green * light + 50) / 100);
		color.blue =
			(unsigned short)((blue * light + 50) / 100);
	}
	/* For "medium" background, select is a fixed %age darker;
	 * top (lighter) and bottom (darker) are a variable %age
	 * based on the background's brightness
	 */
	else
	{
		color_mult(&color.red, &color.green, &color.blue, factor);
	}
	return &color;
}


XColor *GetShadowColor(Pixel background)
{
	return GetShadowOrHiliteColor(
		background, PCT_LIGHT_BOTTOM, PCT_DARK_BOTTOM, DARKNESS_FACTOR);
}

Pixel GetShadow(Pixel background)
{
	XColor *colorp;

	colorp = GetShadowColor(background);
	PictureAllocColor(Pdpy, Pcmap, colorp, True);
	if (colorp->pixel == background)
	{
		colorp->pixel = PictureGetNextColor(colorp->pixel, 1);
	}
	return colorp->pixel;
}

XColor *GetHiliteColor(Pixel background)
{
	return GetShadowOrHiliteColor(
		background, PCT_LIGHT_TOP, PCT_DARK_TOP, BRIGHTNESS_FACTOR);
}

Pixel GetHilite(Pixel background)
{
	XColor *colorp;

	colorp = GetHiliteColor(background);
	PictureAllocColor(Pdpy, Pcmap, colorp, True);
	if (colorp->pixel == background)
	{
		colorp->pixel = PictureGetNextColor(colorp->pixel, -1);
	}
	return colorp->pixel;
}

XColor *GetForeShadowColor(Pixel foreground, Pixel background)
{
	XColor bg_color;
	float fg[3], bg[3];
	int result[3];
	int i;

	memset(&color, 0, sizeof(color));
	memset(&bg_color, 0, sizeof(bg_color));
	color.pixel = foreground;
	bg_color.pixel = background;
	XQueryColor(Pdpy, Pcmap, &color);
	XQueryColor(Pdpy, Pcmap, &bg_color);
	fg[0] = color.red;
	fg[1] = color.green;
	fg[2] = color.blue;
	bg[0] = bg_color.red;
	bg[1]=  bg_color.green;
	bg[2] = bg_color.blue;

	for (i=0; i<3; i++)
	{
		if (fg[i] - bg[i] < 8192 && fg[i] - bg[i] > -8192)
		{
			result[i] = 0;
		}
		else
		{
			result[i] = (int)((5 * bg[i] - fg[i]) / 4);
			if (fg[i] < bg[i] || result[i] < 0)
			{
				result[i] = (int)((3 * bg[i] + fg[i]) / 4);
			}
		}
	}
	color.red = result[0];
	color.green = result[1];
	color.blue = result[2];

	return &color;
}

Pixel GetForeShadow(Pixel foreground, Pixel background)
{
	XColor *colorp;

	colorp = GetForeShadowColor(foreground, background);
	PictureAllocColor(Pdpy, Pcmap, colorp, True);
	if (colorp->pixel == background)
	{
		colorp->pixel = PictureGetNextColor(colorp->pixel, 1);
	}
	return colorp->pixel;
}

XColor *GetTintedColor(Pixel in, Pixel tint, int percent)
{
	XColor tint_color;

	memset(&color, 0, sizeof(color));
	memset(&tint_color, 0, sizeof(tint_color));
	color.pixel = in;
	XQueryColor(Pdpy, Pcmap, &color);
	tint_color.pixel = tint;
	XQueryColor(Pdpy, Pcmap, &tint_color);

	color.red = (unsigned short)
		(((100-percent)*color.red + tint_color.red * percent) / 100);
	color.green = (unsigned short)
		(((100-percent)*color.green + tint_color.green * percent) /
		 100);
	color.blue = (unsigned short)
		(((100-percent)*color.blue + tint_color.blue * percent) / 100);
	return &color;
}

Pixel GetTintedPixel(Pixel in, Pixel tint, int percent)
{
	XColor *colorp;

	colorp = GetTintedColor(in, tint, percent);
	PictureAllocColor(Pdpy, Pcmap, colorp, True);
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
		sprintf(
			output, "rgb:%04x/%04x/%04x%n", (int)color.red,
			(int)color.green, (int)color.blue, &n);
	}
	else
	{
		sprintf(
			output, "#%04x%04x%04x%n", (int)color.red,
			(int)color.green, (int)color.blue, &n);
	}

	return n;
}

static char *colorset_names[] =
{
	"$[fg.cs",
	"$[bg.cs",
	"$[hilight.cs",
	"$[shadow.cs",
	NULL
};

Pixel GetSimpleColor(char *name)
{
	XColor color;
	Bool is_illegal_rgb = False;

	memset(&color, 0, sizeof(color));
	/* This is necessary because some X servers coredump when presented a
	 * malformed rgb colour name. */
	if (name && strncasecmp(name, "rgb:", 4) == 0)
	{
		int i;
		char *s;

		for (i = 0, s = name + 4; *s; s++)
		{
			if (*s == '/')
				i++;
		}
		if (i != 2)
			is_illegal_rgb = True;
	}

	if (is_illegal_rgb)
	{
		fprintf(stderr, "Illegal RGB format \"%s\"\n", name);
	}
	else if (!XParseColor (Pdpy, Pcmap, name, &color))
	{
		fprintf(stderr, "Cannot parse color \"%s\"\n",
			name ? name : "<blank>");
	}
	else if (!PictureAllocColor(Pdpy, Pcmap, &color, True))
	{
		fprintf(stderr, "Cannot allocate color \"%s\"\n", name);
	}
	return color.pixel;
}

Pixel GetColor(char *name)
{
	int i;
	int n;
	int cs;
	char *rest;
	XColor color;

	switch ((i = GetTokenIndex(name, colorset_names, -1, &rest)))
	{
	case 0:
	case 1:
	case 2:
	case 3:
		if (!isdigit(*rest) || (*rest == '0' && *(rest + 1) != 0))
		{
			/* not a non-negative integer without leading zeros */
			fprintf(stderr,
				"Invalid colorset number in color '%s'\n",
				name);
			return 0;
		}
		sscanf(rest, "%d%n", &cs, &n);
		if (*(rest + n) != ']')
		{
			fprintf(stderr,
				"No closing brace after '%d' in color '%s'\n",
				cs, name);
			return 0;
		}
		if (*(rest + n + 1) != 0)
		{
			fprintf(stderr, "Trailing characters after brace in"
				" color '%s'\n", name);
			return 0;
		}
		AllocColorset(cs);
		switch (i)
		{
		case 0:
			color.pixel = Colorset[cs].fg;
			break;
		case 1:
			color.pixel = Colorset[cs].bg;
			break;
		case 2:
			color.pixel = Colorset[cs].hilite;
			break;
		case 3:
			color.pixel = Colorset[cs].shadow;
			break;
		}
		if (!PictureAllocColor(Pdpy, Pcmap, &color, True))
		{
			fprintf(stderr, "Cannot allocate color %d from"
				" colorset %d\n", i, cs);
			return 0;
		}
		return color.pixel;

	default:
		break;
	}

	return GetSimpleColor(name);
}

/* Allocates the color from the input Pixel again */
Pixel fvwmlib_clone_color(Pixel p)
{
	XColor c;

	c.pixel = p;
	XQueryColor(Pdpy, Pcmap, &c);
	if (!PictureAllocColor(Pdpy, Pcmap, &c, True))
	{
		fprintf(stderr, "Cannot allocate clone Pixel %d\n", (int)p);
		return 0;
	}

	return c.pixel;
}

/* Free an array of colours (n colours), never free black */
void fvwmlib_free_colors(Display *dpy, Pixel *pixels, int n, Bool no_limit)
{
	int i;

	/* We don't ever free black - dirty hack to allow freeing colours at
	 * all */
	/* olicha: ???? */
	for (i = 0; i < n; i++)
	{
		if (pixels[i] != 0)
		{
			PictureFreeColors(
				dpy, Pcmap, pixels + i, 1, 0, no_limit);
		}
	}

	return;
}

/* Copy one color and reallocate it */
void fvwmlib_copy_color(
	Display *dpy, Pixel *dst_color, Pixel *src_color, Bool do_free_dest,
	Bool do_copy_src)
{
	if (do_free_dest)
	{
		fvwmlib_free_colors(dpy, dst_color, 1, True);
	}
	if (do_copy_src)
	{
		*dst_color = fvwmlib_clone_color(*src_color);
	}
}
