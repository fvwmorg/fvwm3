/* -*-c-*- */
/* Copyright (C) 1993, Robert Nation
 * Copyright (C) 1999 Carsten Haitzler and various contributors (imlib2)
 * Copyright (C) 2002  Olivier Chapuis
 *
 * This program is free software; you can redistribute it and/or modify
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
 *
 * The PictureRGBtoPixel are from imlib2. The code is from raster
 * (Carsten Haitzler) <raster@rasterman.com> <raster@valinux.com>
 *
 */

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include <fvwmlib.h>
#include "PictureBase.h"
#include "PictureUtils.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

#ifdef USE_OLD_COLOR_LIMIT_METHODE
/* This structure is used to quickly access the RGB values of the colors */
/* without repeatedly having to transform them.   */
typedef struct
{
	char * c_color;           /* Pointer to the name of the color */
	XColor rgb_space;         /* rgb color info */
#define ALLOC_COLOR_NO     0
#define ALLOC_COLOR_DONE   1
#define ALLOC_COLOR_FAILED 2
	unsigned int  allocated : 2; /* True if the pixel of the rgb_space
				      * is allocated */
} Color_Info;

#else

#endif

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

#ifdef USE_OLD_COLOR_LIMIT_METHODE
static char color_limit_base_table_init = 'n';

/* First thing in base array are colors probably already in the color map
   because they have familiar names.
   I pasted them into a xpm and spread them out so that similar colors are
   spread out.
   Toward the end are some colors to fill in the gaps.
   Currently 61 colors in this list.
*/
static Color_Info base_array[] =
{
	{"white"},
	{"black"},
	{"grey"},
	{"green"},
	{"blue"},
	{"red"},
	{"cyan"},
	{"yellow"},
	{"magenta"},
	{"DodgerBlue"},
	{"SteelBlue"},
	{"chartreuse"},
	{"wheat"},
	{"turquoise"},
	{"CadetBlue"},
	{"gray87"},
	{"CornflowerBlue"},
	{"YellowGreen"},
	{"NavyBlue"},
	{"MediumBlue"},
	{"plum"},
	{"aquamarine"},
	{"orchid"},
	{"ForestGreen"},
	{"lightyellow"},
	{"brown"},
	{"orange"},
	{"red3"},
	{"HotPink"},
	{"LightBlue"},
	{"gray47"},
	{"pink"},
	{"red4"},
	{"violet"},
	{"purple"},
	{"gray63"},
	{"gray94"},
	{"plum1"},
	{"PeachPuff"},
	{"maroon"},
	{"lavender"},
	{"salmon"},           /* for peachpuff, orange gap */
	{"blue4"},            /* for navyblue/mediumblue gap */
	{"PaleGreen4"},       /* for forestgreen, yellowgreen gap */
	{"#AA7700"},          /* brick, no close named color */
	{"#11EE88"},          /* light green, no close named color */
	{"#884466"},          /* dark brown, no close named color */
	{"#CC8888"},          /* light brick, no close named color */
	{"#EECC44"},          /* gold, no close named color */
	{"#AAAA44"},          /* dull green, no close named color */
	{"#FF1188"},          /* pinkish red */
	{"#992299"},          /* purple */
	{"#CCFFAA"},          /* light green */
	{"#664400"},          /* dark brown*/
	{"#AADD99"},          /* light green */
	{"#66CCFF"},          /* light blue */
	{"#CC2299"},          /* dark red */
	{"#FF11CC"},          /* bright pink */
	{"#11CC99"},          /* grey/green */
	{"#AA77AA"},          /* purple/red */
	{"#EEBB77"}           /* orange/yellow */
};

#define NColors (sizeof(base_array) / sizeof(Color_Info))

#else

static int PpaletteType = 0;
static int PpaletteColorLimit = 0;
static int PpaletteStrictColorLimit = 0;
Pixel *Pct = NULL;

#endif

/* ---------------------------- exported variables (globals) ---------------- */

/* ***************************************************************************
 *
 * colors reduction
 *
 * ***************************************************************************/
#ifdef USE_OLD_COLOR_LIMIT_METHODE
static
void c300_color_to_rgb(char *c_color, XColor *rgb_space)
{
	int rc;

	rc=XParseColor(Pdpy, Pcmap, c_color, rgb_space);
	if (rc==0) {
		fprintf(stderr,"color_to_rgb: can't parse color %s, rc %d\n",
			c_color, rc);
		return;
	}

	return;
}

/* A macro for squaring things */
#define SQUARE(X) ((X)*(X))
/* RGB Color distance sum of square of differences */
static
double c400_distance(XColor *target_ptr, XColor *base_ptr)
{
	register double dst;

	dst = SQUARE((double)(base_ptr->red   - target_ptr->red  )/655.35)
		+   SQUARE((double)(base_ptr->green - target_ptr->green)/655.35)
		+   SQUARE((double)(base_ptr->blue  - target_ptr->blue )/655.35);
	return dst;
}

/* from the color names in the base table, calc rgbs */
static
void c100_init_base_table ()
{
	int i;

	for (i=0; i<NColors; i++)
	{
		/* change all base colors to numbers */
		c300_color_to_rgb(
			base_array[i].c_color, &base_array[i].rgb_space);
		base_array[i].allocated = ALLOC_COLOR_NO;
	}

	return;
}

/* Replace the color in my_color by the closest matching color
   from base_table */
void PictureReduceColor(char **my_color, int color_limit)
{
	int i, limit, minind;
	double mindst=1e20;
	double dst;
	XColor rgb;          /* place to calc rgb for each color in xpm */


	if (!XpmSupport)
		return;

	if (color_limit_base_table_init == 'n')
	{
		/* if base table not created yet */
		c100_init_base_table();  /* init the base table */
		/* remember that its set now. */
		color_limit_base_table_init = 'y';
	} 
	if (!strcasecmp(*my_color,"none")) {
		return; /* do not substitute the "none" color */
	}

	c300_color_to_rgb(*my_color, &rgb); /* get rgb for a color in xpm */
	/* Loop over all base_array colors; find out which one is closest
	   to my_color */
	minind = 0;                         /* Its going to find something... */
	limit = NColors;                    /* init to max */
	if (color_limit < NColors) {        /* can't do more than I have */
		limit = color_limit;        /* Do reduction using subset */
	}                                   /* end reducing limit */
	for(i=0; i < limit; i++) {          /* loop over base array */
		dst = c400_distance (&rgb, &base_array[i].rgb_space);
		/* distance */
		if (dst < mindst ) {        /* less than min and better than
					     * last */
			mindst=dst;         /* new minimum */
			minind=i;           /* save loc of new winner */
			if (dst <= 100) {   /* if close enough */
				break;      /* done */
			}                   /* end close enough */
		}                           /* end new low distance */
	}                                   /* end all base colors */
	/* Finally: replace the color string by the newly determined color
	 * string */
	free(*my_color);                    /* free old color */
	/* area for new color */
	*my_color = safemalloc(strlen(base_array[minind].c_color) + 1);
	strcpy(*my_color,base_array[minind].c_color); /* put it there */

	return;
}
#endif

void PictureReduceRGBColor(XColor *c, int color_limit)
{
#ifdef USE_OLD_COLOR_LIMIT_METHODE
	int i, limit, minind;
	double mindst=1e20000;
	double dst;

	if (color_limit <= 0)
	{
		XAllocColor(Pdpy, Pcmap, c);
		return;
	}
	if (color_limit_base_table_init == 'n') {
		c100_init_base_table();
		color_limit_base_table_init = 'y';
	}
	minind = 0;
	limit = NColors;
	if (color_limit < NColors) {
		limit = color_limit;
	}
	for(i=0; i < limit; i++) {
		if (base_array[i].allocated == ALLOC_COLOR_FAILED)
			continue;
		dst = c400_distance(c, &base_array[i].rgb_space);
		if (dst < mindst)
		{
			if (base_array[i].allocated == ALLOC_COLOR_NO)
			{
				if (XAllocColor(
					Pdpy, Pcmap,
					&base_array[i].rgb_space))
				{
					base_array[i].allocated =
						ALLOC_COLOR_DONE;
				}
				else
				{
					base_array[i].allocated =
						ALLOC_COLOR_FAILED;
					continue;
				}
			}
			mindst=dst;
			minind=i;
			if (dst <= 100) {
				break;
			}
		}
	}

	c->pixel = base_array[minind].rgb_space.pixel;
	return;
#else
	c->pixel = PictureRGBtoPixel(c->red/257, c->green/257, c->blue/257);
#endif
}

/* ***************************************************************************
 *
 * rgb to pixel, from imilib2
 *
 * ***************************************************************************/
static
void decompose_mask(unsigned long mask, int *shift, int *prec)
{
	*shift = 0;
	*prec = 0;

	while (!(mask & 0x1))
	{
		(*shift)++;
		mask >>= 1;
	}

	while (mask & 0x1)
	{
		(*prec)++;
		mask >>= 1;
	}
}

Pixel PictureRGBtoPixel(int r, int g, int b)
{
	static int red_shift = 0;
	static int green_shift = 0;
	static int blue_shift = 0;
	static int red_prec = 0;
	static int green_prec = 0;
	static int blue_prec = 0;
	static Bool init = False;

#ifndef USE_OLD_COLOR_LIMIT_METHODE
	if (PpaletteColorLimit > 0 && Pct)
	{
		switch (PpaletteType)
		{
		case 0: /* 332 */
			return Pct[((b >> 6) & 0x03) |
				  ((g >> 3) & 0x1c) |
				  (r & 0xe0)];
			break;
		case 1: /* 232 */
			return Pct[((b >> 6) & 0x03) |
				  ((g >> 3) & 0x1c) |
				  ((r >> 1) & 0x60)];
			break;
		case 2: /* 222 */
			return Pct[((b >> 6) & 0x03) |
				  ((g >> 4) & 0x0c) |
				  ((r >> 2) & 0x30)];
			break;
		case 3: /* 221 */
			return Pct[((b >> 7) & 0x01) |
				  ((g >> 5) & 0x06) |
				  ((r >> 3) & 0x18)];
			break;
		case 4: /* 121 */
			return Pct[((b >> 7) & 0x01) |
				  ((g >> 5) & 0x06) |
				  ((r >> 4) & 0x08)];
			break;
		case 5: /* 111 */
			return Pct[((b >> 7) & 0x01) |
				  ((g >> 6) & 0x02) |
				  ((r >> 5) & 0x04)];
			break;
		case 6: /* 1 */
			return Pct[(((r & 0xff) + (g & 0xff) + (b & 0xff)) / 3) 
				  >> 7];
			break;
		case 7: /* 666 */
			return Pct[
				(((r*6) >> 8) * 36) + 
				(((g*6) >> 8) * 6) +
				((b*6) >> 8)];
			break;
		default:
			return 0;
		}
	}
	if (Pdepth <= 8)
	{
		XColor c;
		
		c.red = r*257;
		c.green = g*257;
		c.blue = b*257;
		XAllocColor(Pdpy, Pcmap, &c);
		return c.pixel;
	}
#endif

	if (!init)
	{
		init = True;
		decompose_mask(Pvisual->red_mask, &red_shift, &red_prec);
		decompose_mask(Pvisual->green_mask, &green_shift, &green_prec);
		decompose_mask(Pvisual->blue_mask, &blue_shift, &blue_prec);
	}
	return (Pixel)((((r >> (8 - red_prec)) << red_shift) +
	       ((g >> (8 - green_prec)) << green_shift) +
	       ((b >> (8 - blue_prec)) << blue_shift)));
}

int PictureAllocColor(Display *dpy, Colormap cmap, XColor *c, int no_limit)
{
#ifndef USE_OLD_COLOR_LIMIT_METHODE
	if (PpaletteColorLimit > 0 && no_limit &&
	    !PpaletteStrictColorLimit && XAllocColor(dpy, cmap, c))
	{
		return 1;
	}
	c->pixel = PictureRGBtoPixel(
		c->red/257, c->green/257, c->blue/257);
	return 1;
#else
	return XAllocColor(dpy, cmap, c);
#endif
}

void PictureFreeColors(
	Display *dpy, Colormap cmap, Pixel *pixels, int n, unsigned long planes)
{
#ifndef USE_OLD_COLOR_LIMIT_METHODE
	if (PpaletteColorLimit > 0)
	{
		Pixel *p;
		int i,j,do_free;
		int m = 0;

		p = (Pixel *)safemalloc(n*sizeof(Pixel));
		for(i= 0; i < n; i++)
		{
			do_free = 1;
			for(j=0; j<PpaletteColorLimit; j++)
			{
				if (pixels[i] == Pct[j])
				{
					do_free = 0;
					break;
				}
			}
			if (do_free)
			{
				p[m++] = pixels[i];
			}
		}
		if (m > 0)
		{
			XFreeColors(dpy, cmap, p, m, planes);
		}
		return;
	}
	if (Pct == NULL && Pdepth <= 8)
	{
		XFreeColors(dpy, cmap, pixels, n, planes);
	}
	return;
#else
	XFreeColors(dpy, cmap, pixels, n, planes);
#endif
}

Pixel PictureGetNextColor(Pixel p, int n)
{
#ifndef USE_OLD_COLOR_LIMIT_METHODE
	int i;

	if (n >= 0)
		n = 1;
	else
		n = -1;

	if (Pct == NULL)
		return p;
	for(i=0; i<PpaletteColorLimit; i++)
	{
		if (Pct[i] == p)
		{
			if (i == 0 && n < 0)
			{
				return Pct[PpaletteColorLimit-1];
			}
			else if (i == PpaletteColorLimit-1 && n > 0)
			{
				return Pct[0];
			}
			else
			{
				return Pct[i+n];
			}
		}
	}
	return p;
#else
	return p;
#endif
}

/* ***************************************************************************
 *
 * palette from imilib2
 *
 * ***************************************************************************/
#ifndef USE_OLD_COLOR_LIMIT_METHODE

static
void colors_alloc_fail(
	Display *d, Colormap cmap, int i, Pixel *color_lut, Bool read_write)
{
	unsigned long pixels[256];
	int j;

	if (i > 0)
	{
		for(j = 0; j < i; j++)
			pixels[j] = (unsigned long) color_lut[j];
		XFreeColors(d, cmap, pixels, i, 0);
	}
}

static
Bool my_alloc_color(
	Display *d, Colormap cmap, XColor *xc, int sig_mask, Bool read_write)
{
	XColor *xc_in;
	
	xc_in = xc;
	if ((!XAllocColor(d, cmap, xc)) ||
	    ((xc_in->red & sig_mask) != (xc->red & sig_mask)) ||
	    ((xc_in->green & sig_mask) != (xc->green & sig_mask)) ||
	    ((xc_in->blue & sig_mask) != (xc->blue & sig_mask)))
	{
		return 0;
	}
	return 1;
}


static
Pixel *AllocColors332(Display *d, Colormap cmap, Visual *v)
{
	int r, g, b, i, val;
	Pixel *color_lut;
	int sig_mask = 0;
	XColor xcl, xcl_in;

	for (i = 0; i < v->bits_per_rgb; i++) sig_mask |= (0x1 << i);
	sig_mask <<= (16 - v->bits_per_rgb);
	i = 0;
	color_lut = malloc(256 * sizeof(Pixel));
	for (r = 0; r < 8; r++)
	{
		for (g = 0; g < 8; g++)
		{
			for (b = 0; b < 4; b++)
			{
				val = (r << 6) | (r << 3) | (r);
				xcl.red = (unsigned short)
					((val << 7) | (val >> 2));
				val = (g << 6) | (g << 3) | (g);
				xcl.green = (unsigned short)
					((val << 7) | (val >> 2));
				val = (b << 6) | (b << 4) | (b << 2) | (b);
				xcl.blue = (unsigned short)((val << 8) | (val));
				xcl_in = xcl;
				if (!my_alloc_color(
					d, cmap, &xcl, sig_mask, False))
				{
					colors_alloc_fail(
						d, cmap, i, color_lut, False);
					free(color_lut);
					return NULL;
				}
				color_lut[i] = xcl.pixel;
				i++;
			}
		}
	}
	PpaletteType = 0;
	PpaletteColorLimit = 256;
	return color_lut;
}

static
Pixel *AllocColors666(Display *d, Colormap cmap, Visual *v)
{
	int r, g, b, i, val;
	Pixel *color_lut;
	int sig_mask = 0;
	XColor xcl, xcl_in;
   
	for (i = 0; i < v->bits_per_rgb; i++) sig_mask |= (0x1 << i);
	sig_mask <<= (16 - v->bits_per_rgb);
	i = 0;
	color_lut = malloc(216 * sizeof(Pixel));
	for (r = 0; r < 6; r++)
	{
		for (g = 0; g < 6; g++)
		{
			for (b = 0; b < 6; b++)
			{
				val = (int)((((double)r) / 5.0) * 65535);
				xcl.red = (unsigned short)(val);
				val = (int)((((double)g) / 5.0) * 65535);
				xcl.green = (unsigned short)(val);
				val = (int)((((double)b) / 5.0) * 65535);
				xcl.blue = (unsigned short)(val);
				xcl_in = xcl;
				if (!my_alloc_color(
					d, cmap, &xcl, sig_mask, False))
				{
					colors_alloc_fail(
						d, cmap, i, color_lut, False);
					free(color_lut);
					return NULL;
				}
				color_lut[i] = xcl.pixel;
				i++;
			}
		}
	}
	PpaletteType = 7;
	PpaletteColorLimit = 216;
	return color_lut;
}

static
Pixel *AllocColors232(Display *d, Colormap cmap, Visual *v)
{
	int r, g, b, i, val;
	Pixel *color_lut;
	int sig_mask = 0;
	XColor xcl, xcl_in;
   
	for (i = 0; i < v->bits_per_rgb; i++) sig_mask |= (0x1 << i);
	sig_mask <<= (16 - v->bits_per_rgb);
	i = 0;
	color_lut = malloc(128 * sizeof(Pixel));   
	for (r = 0; r < 4; r++)
	{
		for (g = 0; g < 8; g++)
		{
			for (b = 0; b < 4; b++)
			{
				val = (r << 6) | (r << 4) | (r << 2) | (r);
				xcl.red = (unsigned short)((val << 8) | (val));
				val = (g << 6) | (g << 3) | (g);
				xcl.green = (unsigned short)
					((val << 7) | (val >> 2));
				val = (b << 6) | (b << 4) | (b << 2) | (b);
				xcl.blue = (unsigned short)
					((val << 8) | (val));
				xcl_in = xcl;
				if (!my_alloc_color(
					d, cmap, &xcl, sig_mask, False))
				{
					colors_alloc_fail(
						d, cmap, i, color_lut, False);
					free(color_lut);
					return NULL;
				}
				color_lut[i] = xcl.pixel;
				i++;
			}
		}
	}
	PpaletteType = 1;
	PpaletteColorLimit = 128;
	return color_lut;
}

static
Pixel *AllocColors222(Display *d, Colormap cmap, Visual *v)
{
	int r, g, b, i, val;
	Pixel *color_lut;
	int sig_mask = 0;
	XColor xcl;
   
	for (i = 0; i < v->bits_per_rgb; i++) sig_mask |= (0x1 << i);
	sig_mask <<= (16 - v->bits_per_rgb);
	i = 0;
	color_lut = malloc(64 * sizeof(Pixel));   
	for (r = 0; r < 4; r++)
	{
		for (g = 0; g < 4; g++)
		{
			for (b = 0; b < 4; b++)
			{
				val = (r << 6) | (r << 4) | (r << 2) | (r);
				xcl.red = (unsigned short)((val << 8) | (val));
				val = (g << 6) | (g << 4) | (g << 2) | (g);
				xcl.green = (unsigned short)((val << 8) | (val));
				val = (b << 6) | (b << 4) | (b << 2) | (b);
				xcl.blue = (unsigned short)((val << 8) | (val));
				if (!my_alloc_color(
					d, cmap, &xcl, sig_mask, False))
				{
					colors_alloc_fail(
						d, cmap, i, color_lut, False);
					free(color_lut);
					return NULL;
				}
				color_lut[i] = xcl.pixel;
				i++;
			}
		}
	}
	PpaletteType = 2;
	PpaletteColorLimit = 64;
	return color_lut;
}

static
Pixel *AllocColors221(Display *d, Colormap cmap, Visual *v)
{
	int r, g, b, i, val;
	Pixel *color_lut;
	int sig_mask = 0;
	XColor xcl;
   
	for (i = 0; i < v->bits_per_rgb; i++) sig_mask |= (0x1 << i);
	sig_mask <<= (16 - v->bits_per_rgb);
	i = 0;
	color_lut = malloc(32 * sizeof(Pixel));   
	for (r = 0; r < 4; r++)
	{
		for (g = 0; g < 4; g++)
		{
			for (b = 0; b < 2; b++)
			{
				val = (r << 6) | (r << 4) | (r << 2) | (r);
				xcl.red = (unsigned short)((val << 8) | (val));
				val = (g << 6) | (g << 4) | (g << 2) | (g);
				xcl.green = (unsigned short)((val << 8) | (val));
				val = (b << 7) | (b << 6) | (b << 5) |
					(b << 4) | (b << 3) | (b << 2) |
					(b << 1) | (b);
				xcl.blue = (unsigned short)((val << 8) | (val));
				if (!my_alloc_color(
					d, cmap, &xcl, sig_mask, False))
				{
					colors_alloc_fail(
						d, cmap, i, color_lut, False);
					free(color_lut);
					return NULL;
				}
				color_lut[i] = xcl.pixel;
				i++;
			}
		}
	}
	PpaletteType = 3;
	PpaletteColorLimit = 32;
	return color_lut;
}

static
Pixel *AllocColors121(Display *d, Colormap cmap, Visual *v)
{
	int r, g, b, i, val;
	Pixel *color_lut;
	int sig_mask = 0;
	XColor xcl;
   
	for (i = 0; i < v->bits_per_rgb; i++) sig_mask |= (0x1 << i);
	sig_mask <<= (16 - v->bits_per_rgb);
	i = 0;
	color_lut = malloc(16 * sizeof(Pixel));   
	for (r = 0; r < 2; r++)
	{
		for (g = 0; g < 4; g++)
		{
			for (b = 0; b < 2; b++)
			{
				val = (r << 7) | (r << 6) | (r << 5) |
					(r << 4) | (r << 3) | (r << 2) |
					(r << 1) | (r);
				xcl.red = (unsigned short)((val << 8) | (val));
				val = (g << 6) | (g << 4) | (g << 2) | (g);
				xcl.green = (unsigned short)((val << 8) | (val));
				val = (b << 7) | (b << 6) | (b << 5) |
					(b << 4) | (b << 3) | (b << 2) |
					(b << 1) | (b);
				xcl.blue = (unsigned short)((val << 8) | (val));
				if (!my_alloc_color(
					d, cmap, &xcl, sig_mask, False))
				{
					colors_alloc_fail(
						d, cmap, i, color_lut, False);
					free(color_lut);
					return NULL;
				}
				color_lut[i] = xcl.pixel;
				i++;
			}
		}
	}
	PpaletteType = 4;
	PpaletteColorLimit = 16;
	return color_lut;
}

static
Pixel *AllocColors111(Display *d, Colormap cmap, Visual *v)
{
	int r, g, b, i, val;
	Pixel *color_lut;
	int sig_mask = 0;
	XColor xcl;

	for (i = 0; i < v->bits_per_rgb; i++) sig_mask |= (0x1 << i);
	sig_mask <<= (16 - v->bits_per_rgb);
	i = 0;
	color_lut = malloc(8 * sizeof(Pixel));   
	for (r = 0; r < 2; r++)
	{
		for (g = 0; g < 2; g++)
		{
			for (b = 0; b < 2; b++)
			{
				val = (r << 7) | (r << 6) | (r << 5) | (r << 4)
					| (r << 3) | (r << 2) | (r << 1) | (r);
				xcl.red = (unsigned short)((val << 8) | (val));
				val = (g << 7) | (g << 6) | (g << 5) |
					(g << 4) | (g << 3) | (g << 2) |
					(g << 1) | (g);
				xcl.green = (unsigned short)((val << 8) | (val));
				val = (b << 7) | (b << 6) | (b << 5) |
					(b << 4) | (b << 3) | (b << 2) |
					(b << 1) | (b);
				xcl.blue = (unsigned short)((val << 8) | (val));
				if (!my_alloc_color(
					d, cmap, &xcl, sig_mask, False))
				{
					colors_alloc_fail(
						d, cmap, i, color_lut, False);
					free(color_lut);
					return NULL;
				}
				color_lut[i] = xcl.pixel;
				i++;
			}
		}
	}
	PpaletteType = 5;
	PpaletteColorLimit = 8;
	return color_lut;
}

static
Pixel *AllocColors1(Display *d, Colormap cmap, Visual *v)
{
	XColor xcl;
	Pixel *color_lut;

	color_lut = malloc(2 * sizeof(Pixel));   
	xcl.red = (unsigned short)(0x0000);
	xcl.green = (unsigned short)(0x0000);
	xcl.blue = (unsigned short)(0x0000);
	XAllocColor(d, cmap, &xcl);
	color_lut[0] = xcl.pixel;
	xcl.red = (unsigned short)(0xffff);
	xcl.green = (unsigned short)(0xffff);
	xcl.blue = (unsigned short)(0xffff);
	XAllocColor(d, cmap, &xcl);
	color_lut[1] = xcl.pixel;
	PpaletteType = 6;
	PpaletteColorLimit = 2;
	return color_lut;
}

static
Pixel *AllocColorTable(int color_limit)
{
	Pixel *color_lut = NULL;

	if (Pvisual->bits_per_rgb > 1)
	{
		if ((color_limit >= 256) &&
		    (color_lut = AllocColors332(Pdpy, Pcmap, Pvisual)))
		{
			return color_lut;
		}
		if ((color_limit >= 216) &&
		    (color_lut = AllocColors666(Pdpy, Pcmap, Pvisual)))
		{
			return color_lut;
		}
		if ((color_limit >= 128) &&
		    (color_lut = AllocColors232(Pdpy, Pcmap, Pvisual)))
		{
			return color_lut;
		}
		if ((color_limit >= 64) &&
		    (color_lut = AllocColors222(Pdpy, Pcmap, Pvisual)))
		{
			return color_lut;
		}
		if ((color_limit >= 32) &&
		    (color_lut = AllocColors221(Pdpy, Pcmap, Pvisual)))
		{
			return color_lut;
		}
		if ((color_limit >= 16) &&
		    (color_lut = AllocColors121(Pdpy, Pcmap, Pvisual)))
		{
			return color_lut;
		}
	}
	if ((color_limit >= 8) &&
	    (color_lut = AllocColors111(Pdpy, Pcmap, Pvisual)))
	{
		return color_lut;
	}
	color_lut = AllocColors1(Pdpy, Pcmap, Pvisual);

	return color_lut;
}

void PictureAllocColorTable(int color_limit, char *module)
{
	char *envp;

	if (color_limit <= 0 || Pct != NULL)
		return;
	if ((envp = getenv("FVWM_USESTRICT_COLORLIMIT")) != NULL)
	{
		if (*envp == '1')
			PpaletteStrictColorLimit = 1;
	}
	
	Pct = AllocColorTable(color_limit);
	if (module && StrEquals("FVWM",module))
	    fprintf(stderr,"[%s][PictureAllocColorTable]: Info -- "
		    "use %i colours (%i asked)\n",
		    module, PpaletteColorLimit, color_limit);
}

#endif  /* ! USE_OLD_COLOR_LIMIT_METHODE */
