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

/* This structure is used to quickly access the RGB values of the colors */
/* without repeatedly having to transform them.   */
typedef struct
{
	char * c_color;           /* Pointer to the name of the color */
	XColor rgb_space;         /* rgb color info */
	Bool allocated;           /* True if the pixel of the rgb_space
				   * is allocated */
} Color_Info;

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

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

/* ---------------------------- exported variables (globals) ---------------- */

/* ***************************************************************************
 *
 * colors reduction
 *
 * ***************************************************************************/
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
		base_array[i].allocated = False;
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


	if (!XpmSupport) /* at present time used only for xpm loading */
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

void PictureReduceRGBColor(XColor *c, int color_limit)
{
	int i, limit, minind;
	double mindst=1e20;
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
		dst = c400_distance (c, &base_array[i].rgb_space);
		if (dst < mindst ) {
			mindst=dst;
			minind=i;
			if (dst <= 100) {
				break;
			}
		}
	}

	if (!base_array[minind].allocated)
	{
		if (XAllocColor(Pdpy, Pcmap, &base_array[minind].rgb_space))
			base_array[minind].allocated = True;
	}
	c->pixel = base_array[minind].rgb_space.pixel;
	return;
}

/* ***************************************************************************
 *
 * rgb to pixel, from imilib2
 *
 * ***************************************************************************/
Pixel PictureRGBtoPixel(int r, int g, int b)
{
	static int rshift = 0;
	static int gshift = 0;
	static int bshift = 0;
	static Bool init = False;
	int i;
	CARD32 val;
	int rm = Pvisual->red_mask;
	int gm = Pvisual->green_mask;
	int bm = Pvisual->blue_mask;

	if ((rm == 0xf800) && (gm == 0x7e0) && (bm == 0x1f)) /* 565 */
	{
		return (((r << 8) & 0xf800) |
			((g << 3) & 0x07e0) |
			((b >> 3) & 0x001f));
	}
	if ((rm == 0xff0000) && (gm == 0xff00) && (bm == 0xff)) /* 888 */
	{
		return (((r << 16) & 0xff0000) |
			((g << 8 ) & 0x00ff00) |
			((b      ) & 0x0000ff));
	}
	if ((rm == 0x7c00) && (gm == 0x3e0) && (bm == 0x1f)) /* 555 */
	{
		return (((r << 7) & 0x7c00) |
			((g << 2) & 0x03e0) |
			((b >> 3) & 0x001f));
	}
	if (!init)
	{
		init = True;
		for (i = 31; i >= 0; i--)
		{
			if (rm >= (1 << i))
			{
				rshift = i - 7;
				break;
			}
		}
		for (i = 31; i >= 0; i--)
		{
			if (gm >= (1 << i))
			{
				gshift = i - 7;
				break;
			}
		}

		for (i = 31; i >= 0; i--)
		{
			if (bm >= (1 << i))
			{
				bshift = i - 7;
				break;
			}
		}
	}

	if (rshift >= 0)
		val = ((r << rshift) & rm);
	else
		val = ((r >> (-rshift)) & rm);
	if (gshift >= 0)
		val |= ((g << gshift) & gm);
	else
		val |= ((g >> (-gshift)) & gm);
	if (bshift >= 0)
		val |= ((b << bshift) & bm);
	else
		val |= ((b >> (-bshift)) & bm);
	return (Pixel)val;
}
