/* -*-c-*- */
/* Copyright (C) 1993, Robert Nation
 * Copyright (C) 2002  Olivier Chapuis */
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

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include "fvwmlib.h"
#include "envvar.h"
#include "Grab.h"
#include "Parse.h"
#include "ftime.h"
#include "PictureBase.h"
#include "PictureUtils.h"
#include "PictureDitherMatrice.h"

/* ---------------------------- local definitions and macro ----------------- */

#if 0
/* dv: unused */
/* form alloc_in_cmap from the xpm lib */
#define XPM_DIST(r1,g1,b1,r2,g2,b2) (long)\
                          (3*(abs((long)r1-(long)r2) + \
			      abs((long)g1-(long)g2) + \
			      abs((long)b1-(long)b2)) + \
			   abs((long)r1 + (long)g1 + (long)b1 - \
			       ((long)r2 +  (long)g2 + (long)b2)))
#define XPM_COLOR_CLOSENESS 40000
#endif

#define SQUARE(X) ((X)*(X))

#define TRUE_DIST(r1,g1,b1,r2,g2,b2) (long)\
                   (SQUARE((long)((r1 - r2)>>8)) \
		+   SQUARE((long)((g1 - g2)>>8)) \
                +   SQUARE((long)((b1 - b2)>>8)))

#define FAST_DIST(r1,g1,b1,r2,g2,b2) (long)\
                   (abs((long)(r1 - r2)) \
		+   abs((long)(g1 - g2)) \
                +   abs((long)(b1 - b2)))

#define FVWM_DIST(r1,g1,b1,r2,g2,b2) \
                   (abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2) \
                    + 2*abs(abs(r1-g1) + abs(g1-b1) + abs(r1-b1) \
                            - abs(r2-g2) - abs(g2-b2) - abs(r2-b2)))

#define USED_DIST(r1,g1,b1,r2,g2,b2) FVWM_DIST(r1,g1,b1,r2,g2,b2)

#define PICTURE_COLOR_CLOSENESS USED_DIST(3,3,3,0,0,0)

#define PICTURE_PAllocTable         1000000
#define PICTURE_PUseDynamicColors   100000
#define PICTURE_PStrictColorLimit   10000
#define PICTURE_use_named           1000
#define PICTURE_TABLETYPE_LENGHT    7

/* humm ... dither is probably borken with gamma correction. Anyway I do
 * do think that using gamma correction for the colors cubes is a good
 * idea */
#define USE_GAMMA_CORECTION 0
/* 2.2 is recommanded by the Poynon colors FAQ, some others suggest 1.5 and 2
 * Use float constants!*/
#define COLOR_GAMMA 1.5
#define GREY_GAMMA  2.0

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef struct
{
	XColor color;               /* rgb color info */
	unsigned long alloc_count;  /* nbr of allocation */
} PColor;

typedef struct
{
	/*
	 * info for colors table (depth <= 8)
	 */
	/* color cube used */
	short nr;
	short ng;
	short nb;
	short ngrey;
	/* grey palette def, nbr of grey = 2^grey_bits */
	short grey_bits;
	/* color cube used for dithering with the named table */
	short d_nr;
	short d_ng;
	short d_nb;
	short d_ngrey_bits;
	/* do we found a pre-allocated pallet ? */
	Bool pre_allocated_pallet;
	/* info for depth > 8 */
	int red_shift;
	int green_shift;
	int blue_shift;
	int red_prec;
	int green_prec;
	int blue_prec;
	/* for dithering in depth 15 and 16 */
	unsigned short *red_dither;
	unsigned short *green_dither;
	unsigned short *blue_dither;
	/* colors allocation function */
	int (*alloc_color)(Display *dpy, Colormap cmap, XColor *c);
	int (*alloc_color_no_limit)(Display *dpy, Colormap cmap, XColor *c);
	int (*alloc_color_dither)(
		Display *dpy, Colormap cmap, XColor *c, int x, int y);
	void (*free_colors)(
		Display *dpy, Colormap cmap, Pixel *pixels, int n,
		unsigned long planes);
	void (*free_colors_no_limit)(
		Display *dpy, Colormap cmap, Pixel *pixels, int n,
		unsigned long planes);
} PColorsInfo;

typedef struct {
	int cols_index;
	long closeness;
} CloseColor;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static int PColorLimit = 0;
static PColor *Pct = NULL;
static PColor *Pac = NULL;
static short *PMappingTable = NULL;
static short *PDitherMappingTable = NULL;
static Bool PStrictColorLimit = 0;
static Bool PAllocTable = 0;
static PColorsInfo Pcsi = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL};

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/*
 * get shift and prec from a mask
 */
static
void decompose_mask(
	unsigned long mask, int *shift, int *prec)
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

/*
 * color allocation in the colormap. strongly inspired by SetCloseColor from
 * the Xpm library (depth <= 8)
 */

static int
closeness_cmp(const void *a, const void *b)
{
    CloseColor *x = (CloseColor *) a, *y = (CloseColor *) b;

    /* cast to int as qsort requires */
    return (int) (x->closeness - y->closeness);
}

static
int alloc_color_in_cmap(XColor *c, Bool force)
{
	static XColor colors[256];
	CloseColor closenesses[256];
	XColor tmp;
	int i,j;
	int map_entries = (Pvisual->class == DirectColor)?
		(1 << Pdepth) : Pvisual->map_entries;
	time_t current_time;
	time_t last_time = 0;

	map_entries = (map_entries > 256)? 256:map_entries;
	current_time = time(NULL);
	if (current_time - last_time >= 2 || force)
	{
		last_time = current_time;
		for (i = 0; i < map_entries; i++)
		{
			colors[i].pixel = i;
		}
		XQueryColors(Pdpy, Pcmap, colors, map_entries);
	}
	for(i = 0; i < map_entries; i++)
	{
		closenesses[i].cols_index = i;
		closenesses[i].closeness = USED_DIST(
			(int)(c->red),
			(int)(c->green),
			(int)(c->blue),
			(int)(colors[i].red),
			(int)(colors[i].green),
			(int)(colors[i].blue));
	}
	qsort(closenesses, map_entries, sizeof(CloseColor), closeness_cmp);

	i = 0;
	j = closenesses[i].cols_index;
	while (force ||
	       (abs((long)c->red - (long)colors[j].red) <=
		PICTURE_COLOR_CLOSENESS &&
		abs((long)c->green - (long)colors[j].green) <=
		PICTURE_COLOR_CLOSENESS &&
		abs((long)c->blue - (long)colors[j].blue) <=
		PICTURE_COLOR_CLOSENESS))
	{
			tmp.red = colors[j].red;
			tmp.green = colors[j].green;
			tmp.blue = colors[j].blue;
			if (XAllocColor(Pdpy, Pcmap, &tmp))
			{
				c->red = tmp.red;
				c->green = tmp.green;
				c->blue = tmp.blue;
				c->pixel = tmp.pixel;
				return 1;
			}
			else
			{
				i++;
				if (i == map_entries)
					break;
				j = closenesses[i].cols_index;
			}
	}
	return 0;
}

/*
 * dithering
 */

static
int my_dither(int x, int y, XColor *c)
{
        /* the dither matrice */
	static const char DM[128][128] = DITHER_MATRICE;
	int index;
	const char *dmp;

	if (Pcsi.grey_bits != 0)
	{
		/* Grey Scale */
		int prec = Pcsi.grey_bits;

		if (Pcsi.grey_bits == 1)
		{
			/* FIXME, can we do a better dithering */
			prec = 2;
		}
		dmp = DM[(0 + y) & (DM_HEIGHT - 1)];
		index = (c->green + ((c->blue + c->red) >> 1)) >> 1;
		index += (dmp[(0 + x) & (DM_WIDTH - 1)] << 2) >> prec;
		index = (index - (index >> prec));
		index = index >> (8 - Pcsi.grey_bits);
	}
	else
	{
		/* color cube */
		int dith, rs, gs, bs, gb, b;
		int tr,tb,tg;

		rs = Pcsi.d_nr - 1;
		gs = Pcsi.d_ng - 1;
		bs = Pcsi.d_nb - 1;
		gb = Pcsi.d_ng*Pcsi.d_nb;
		b = Pcsi.d_nb;

		dmp = DM[(0 + y) & (DM_HEIGHT - 1)];
		dith = (dmp[(0 + x) & (DM_WIDTH - 1)] << 2) | 7;
		tr = ((c->red * rs) + dith) >> 8;
		tg = ((c->green * gs) + (262 - dith)) >> 8;
		tb = ((c->blue * bs) + dith) >> 8;
		index = tr * gb + tg * b + tb;
#if 0
		/* try to use the additonal grey. Not easy, good for
		 * certain image/gradient bad for others */
		if (Pcsi.d_ngrey_bits)
		{
			int g_index;

			/* dither in the Pcsi.ngrey^3 cc */
			tr = ((c->red * (Pcsi.ngrey-1)) + dith) >> 8;
			tg = ((c->green * (Pcsi.ngrey-1)) + (262 - dith)) >> 8;
			tb = ((c->blue * (Pcsi.ngrey-1)) + dith) >> 8;
			/* get the grey */
			fprintf(stderr, "%i,%i,%i(%i/%i) ", tr,tg,tb,
				abs(tr-tg) + abs(tb-tg) + abs(tb-tr),Pcsi.ngrey);
			g_index = ((tr + tg + tb)/3);
			if (g_index != 0 && g_index != Pcsi.ngrey-1 &&
			    abs(tr-tg) + abs(tb-tg) + abs(tb-tr) <=
			    Pcsi.d_ngrey_bits)
			{
				g_index =  g_index + Pcsi.ng*Pcsi.nb*Pcsi.ng -1;
				index = g_index;
			}
		}
#endif
		if (PDitherMappingTable != NULL)
		{
			index = PDitherMappingTable[index];
		}
	}
	return index;
}

static
int my_dither_depth_15_16_init(void)
{
	const unsigned char _dither_44[4][4] =
	{
		{0, 4, 1, 5},
		{6, 2, 7, 3},
		{1, 5, 0, 4},
		{7, 3, 6, 2}
	};
	int y,x,i;
	int rm = 0xf8, re = 0x7, gm = 0xfc, ge = 0x3, bm = 0xf8, be = 0x7;

	if (Pdepth == 16 && (Pvisual->red_mask == 0xf800) &&
	    (Pvisual->green_mask == 0x7e0) &&
	    (Pvisual->blue_mask == 0x1f))
	{
		/* ok */
	}
	else if (Pdepth == 15 && (Pvisual->red_mask == 0x7c00) &&
		 (Pvisual->green_mask == 0x3e0) &&
		 (Pvisual->blue_mask == 0x1f))
	{
		gm = 0xf8; ge = 0x7;
	}
	else
	{
		return 0; /* fail */
	}

	Pcsi.red_dither = xmalloc(4*4*256*sizeof(unsigned short));
	Pcsi.green_dither = xmalloc(4*4*256*sizeof(unsigned short));
	Pcsi.blue_dither = xmalloc(4*4*256*sizeof(unsigned short));

	for (y = 0; y < 4; y++)
	{
		for (x = 0; x < 4; x++)
		{
			for (i = 0; i < 256; i++)
			{
				if ((_dither_44[x][y] < (i & re)) &&
				    (i < (256 - 8)))
				{
					Pcsi.red_dither[
						(x << 10) | (y << 8) | i] =
						((i + 8) & rm) << 8;
				}
				else
				{
					Pcsi.red_dither[
						(x << 10) | (y << 8) | i] =
						(i & rm) << 8;
				}
				if ((_dither_44[x][y] < ((i & ge) << 1))
				    && (i < (256 - 4)))
				{
					Pcsi.green_dither[
						(x << 10) | (y << 8) | i] =
						((i + 4) & gm) << 8;
				}
				else
				{
					Pcsi.green_dither[
						(x << 10) | (y << 8) | i] =
						(i & gm) << 8;
				}
				if ((_dither_44[x][y] < (i & be)) &&
				    (i < (256 - 8)))
				{
					Pcsi.blue_dither[
						(x << 10) | (y << 8) | i] =
						((i + 8) & bm) << 8;
				}
				else
				{
					Pcsi.blue_dither[
						(x << 10) | (y << 8) | i] =
						(i & bm) << 8;
				}
			}
		}
	}
	return 1;
}

/*
 * Color allocation in the "palette"
 */

static
int alloc_color_in_pct(XColor *c, int index)
{
	if (Pct[index].alloc_count == 0)
	{
		int s = PStrictColorLimit;

		PStrictColorLimit = 0;
		c->red = Pct[index].color.red;
		c->green = Pct[index].color.green;
		c->blue = Pct[index].color.blue;
		PictureAllocColor(Pdpy, Pcmap, c, True); /* WARN (rec) */
		Pct[index].color.pixel = c->pixel;
		Pct[index].alloc_count = 1;
		PStrictColorLimit = s;
	}
	else
	{
		c->red = Pct[index].color.red;
		c->green = Pct[index].color.green;
		c->blue = Pct[index].color.blue;
		c->pixel = Pct[index].color.pixel;
		if (Pct[index].alloc_count < 0xffffffff)
			(Pct[index].alloc_count)++;
	}
	return 1;
}

static
int get_color_index(int r, int g, int b, int is_8)
{
	int index;

	if (!is_8)
	{
		r= r >> 8;
		g= g >> 8;
		b= b >> 8;
	}
	if (Pcsi.grey_bits > 0)
	{
		/* FIXME: Use other proporition ? */
		index = ((r+g+b)/3) >> (8 - Pcsi.grey_bits);
	}
	else
	{
#if 1
		/* "exact" computation (corrected linear dist) */
		float fr,fg,fb;
		int ir, ig, ib;

		/* map to the cube */
		fr = ((float)r * (Pcsi.nr-1))/255;
		fg = ((float)g * (Pcsi.ng-1))/255;
		fb = ((float)b * (Pcsi.nb-1))/255;

		if (PMappingTable != NULL)
		{
			ir = (int)fr + (fr - (int)fr > 0.5);
			ig = (int)fg + (fg - (int)fg > 0.5);
			ib = (int)fb + (fb - (int)fb > 0.5);

			index = ir * Pcsi.ng*Pcsi.nb + ig * Pcsi.nb + ib;
		}
		else
		{
			/* found the best of the 8 linear closest points */
			int lr,lg,lb,tr,tg,tb,best_dist = -1,i,d;

			index = 0;
			lr = min((int)fr+1,Pcsi.nr-1);
			lg = min((int)fg+1,Pcsi.ng-1);
			lb = min((int)fb+1,Pcsi.nb-1);
			for(tr =(int)fr; tr<=lr; tr++)
			{
				for(tg =(int)fg; tg<=lg; tg++)
				{
					for(tb =(int)fb; tb<=lb; tb++)
					{
						i = tr * Pcsi.ng*Pcsi.nb +
							tg * Pcsi.nb +
							tb;
						d = USED_DIST(
							r,g,b,
							(Pct[i].color.red>>8),
							(Pct[i].color.green>>8),
							(Pct[i].color.blue>>8));
						if (best_dist == -1 ||
						    d < best_dist)
						{
							index = i;
							best_dist = d;
						}
					}
				}
			}

			/* now found the best grey */
			if (Pcsi.ngrey - 2 > 0)
			{
				/* FIXME: speedup this with more than 8 grey */
				int start = Pcsi.nr*Pcsi.ng*Pcsi.nb;
				for(i=start; i < start+Pcsi.ngrey-2; i++)
				{
					d = USED_DIST(
						r,g,b,
						(Pct[i].color.red>>8),
						(Pct[i].color.green>>8),
						(Pct[i].color.blue>>8));
					if (d < best_dist)
					{
						index = i;
						best_dist = d;
					}
				}
			}
			return index;
		}
#else
		/* approximation; faster */
		index = ((r * Pcsi.nr)>>8) * Pcsi.ng*Pcsi.nb +
			((g * Pcsi.ng)>>8) * Pcsi.nb +
			((b * Pcsi.nb)>>8);
#endif
		if (PMappingTable != NULL)
		{
			index = PMappingTable[index];
		}
	}
	return index;
}

/*
 * Main colors allocator
 */
static
int alloc_color_proportion(Display *dpy, Colormap cmap, XColor *c)
{
	c->pixel = (Pixel)(
		((c->red   >> (16 - Pcsi.red_prec))<< Pcsi.red_shift) +
		((c->green >> (16 - Pcsi.green_prec))<< Pcsi.green_shift) +
		((c->blue  >> (16 - Pcsi.blue_prec))<< Pcsi.blue_shift)
		);
	return 1;
}

static
int alloc_color_proportion_dither(
	Display *dpy, Colormap cmap, XColor *c, int x, int y)
{
	/* 8 bit colors !! */
	c->red = Pcsi.red_dither[
		(((x + 0) & 0x3) << 10) | ((y & 0x3) << 8) |
		((c->red) & 0xff)] * 257;
	c->green = Pcsi.green_dither[
		(((x + 0) & 0x3) << 10) | ((y & 0x3) << 8) |
		((c->green) & 0xff)] * 257;
	c->blue = Pcsi.blue_dither[
		(((x + 0) & 0x3) << 10) | ((y & 0x3) << 8) |
		((c->blue) & 0xff)] * 257;
	c->pixel = (Pixel)(
		((c->red >> (16 - Pcsi.red_prec)) << Pcsi.red_shift) +
		((c->green >> (16 - Pcsi.green_prec))
		 << Pcsi.green_shift) +
		((c->blue >> (16 - Pcsi.blue_prec)) << Pcsi.blue_shift)
		);
	return 1;
}

static
int alloc_color_proportion_grey(
	Display *dpy, Colormap cmap, XColor *c)
{
	/* FIXME: is this ok in general? */
	c->pixel = ((c->red + c->green + c->blue)/3);
	if (Pdepth < 16)
	{
		c->pixel = c->pixel >> (16 - Pdepth);
	}
	return 1;
}

static
int alloc_color_in_table(Display *dpy, Colormap cmap, XColor *c)
{

	int index = get_color_index(c->red,c->green,c->blue, False);
	return alloc_color_in_pct(c, index);
}

static
int alloc_color_in_table_dither(
	Display *dpy, Colormap cmap, XColor *c, int x, int y)
{
	int index;
	/* 8 bit colors !! */
	index = my_dither(x, y, c);
	return alloc_color_in_pct(c, index);
}

static
int alloc_color_dynamic_no_limit(
	Display *dpy, Colormap cmap, XColor *c)
{
	int r = 0;

	if (XAllocColor(dpy, cmap, c))
	{
		r = 1;
	}
	else if (!alloc_color_in_cmap(c, False))
	{
		MyXGrabServer(dpy);
		r = alloc_color_in_cmap(c, True);
		MyXUngrabServer(dpy);
	}
	else
	{
		r = 1;
	}
	if (r && Pac != NULL && (c->pixel <= (1 << Pdepth) /* always true*/))
	{
		Pac[c->pixel].alloc_count++;
		Pac[c->pixel].color.red = c->red;
		Pac[c->pixel].color.green = c->green;
		Pac[c->pixel].color.blue = c->blue;
		Pac[c->pixel].color.pixel = c->pixel;
	}
	return r;
}

static
int alloc_color_x(
	Display *dpy, Colormap cmap, XColor *c)
{
	return XAllocColor(dpy, cmap, c);
}

static
void free_colors_in_table(
	Display *dpy, Colormap cmap, Pixel *pixels, int n,
	unsigned long planes)
{
	Pixel *p;
	int i,j,do_free;
	int m = 0;

	if (!Pct || !PUseDynamicColors)
	{
		return;
	}

	p = xmalloc(n * sizeof(Pixel));
	for(i= 0; i < n; i++)
	{
		do_free = 1;
		for(j=0; j<PColorLimit; j++)
		{
			if (Pct[j].alloc_count &&
			    Pct[j].alloc_count < 0xffffffff &&
			    pixels[i] == Pct[j].color.pixel)
			{
				(Pct[j].alloc_count)--;
				if (Pct[j].alloc_count)
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
	free(p);

	return;
}

static
void free_colors_x(
	Display *dpy, Colormap cmap, Pixel *pixels, int n,
	unsigned long planes)
{
	XFreeColors(dpy, cmap, pixels, n, planes);
	if (Pac != NULL)
	{
		int nbr_colors = (1 << Pdepth);
		int i;

		for(i= 0; i < n; i++)
		{
			if (pixels[i] <= nbr_colors)
			{
				Pac[pixels[i]].alloc_count--;
			}
		}
	}
}

/*
 * local function for building pallet (dynamic colors, private DirectColor
 * cmap)
 */
static
XColor *build_mapping_colors(int nr, int ng, int nb)
{
	int r, g, b, i;
	XColor *colors;

	colors = xmalloc(nr*ng*nb * sizeof(XColor));
	i = 0;
	for (r = 0; r < nr; r++)
	{
		for (g = 0; g < ng; g++)
		{
			for (b = 0; b < nb; b++)
			{
				colors[i].red =
					r * 65535 / (nr - 1);
				colors[i].green =
					g * 65535 / (ng - 1);
				colors[i].blue =
					b * 65535 / (nb - 1);
				i++;
			}
		}
	}
	return colors;
}

static short *build_mapping_table(int nr, int ng, int nb, Bool use_named)
{
	int size = nr*ng*nb;
	XColor *colors_map;
	short *Table;
	int i,j, minind;
	double mindst = 40000;
	double dst;

	colors_map = build_mapping_colors(nr, ng, nb);
	Table = xmalloc((size+1) * sizeof(short));
	for(i=0; i<size; i++)
	{
		minind = 0;
		for(j=0; j<PColorLimit; j++)
		{
			if (use_named)
			{
				/* for back ward compatibility */
				dst = TRUE_DIST(colors_map[i].red,
						colors_map[i].green,
						colors_map[i].blue,
						Pct[j].color.red,
						Pct[j].color.green,
						Pct[j].color.blue);
			}
			else
			{
				dst = USED_DIST(colors_map[i].red,
						colors_map[i].green,
						colors_map[i].blue,
						Pct[j].color.red,
						Pct[j].color.green,
						Pct[j].color.blue);
			}
			if (j == 0 || dst < mindst)
			{
				mindst=dst;
				minind=j;
			}
		}
		Table[i] = minind;
	}
	Table[size] = Table[size-1];
	free(colors_map);
	return Table;
}

static
void free_table_colors(PColor *color_table, int npixels)
{
	Pixel pixels[256];
	int i,n=0;

	if (npixels > 0)
	{
		for(i = 0; i < npixels; i++)
		{
			if (color_table[i].alloc_count)
			{
				pixels[n++] = color_table[i].color.pixel;
			}
			color_table[i].alloc_count = 0;
		}
		if (n > 0)
		{
			XFreeColors(Pdpy, Pcmap, pixels, n, 0);
		}
	}
}

/* FIXME: the DirectColor case */
static
int get_nbr_of_free_colors(int max_check)
{
	int check = 1;
	Pixel Pixels[256];
	int map_entries = (Pvisual->class == DirectColor)?
		(1 << Pdepth):Pvisual->map_entries;
	if (max_check < 1)
		return 0;
	if (map_entries > 256)
	{
		max_check = 256;
	}
	max_check = (max_check > map_entries) ? map_entries:max_check;
	while(1)
	{
		if (XAllocColorCells(
			Pdpy, Pcmap, False, NULL, 0, Pixels, check))
		{
			XFreeColors(Pdpy, Pcmap, Pixels, check, 0);
			check++;
		}
		else
		{
			return check-1;
		}
		if (check > max_check)
		{
			return check-1;
		}
	}
	return check-1;
}

static
PColor *alloc_color_cube(
	int nr, int ng, int nb, int ngrey, int grey_bits, Bool do_allocate)
{
	int r, g, b, grey, i, start_grey, end_grey;
	PColor *color_table;
	XColor color;
	int size;

	size = nr*ng*nb + ngrey + (1 << grey_bits)*(grey_bits != 0);
	if (grey_bits)
	{
		ngrey = (1 << grey_bits);
	}
	if (nr > 0 && ngrey > 0)
	{
		start_grey = 1;
		end_grey = ngrey - 1;
		size = size - 2;
	}
	else
	{
		start_grey = 0;
		end_grey = ngrey;
	}

	color_table = xmalloc((size+1) * sizeof(PColor));

	i = 0;

#if USE_GAMMA_CORECTION
#define CG(x) 65535.0 * pow((x)/65535.0,1/COLOR_GAMMA)
#define GG(x) 65535.0 * pow((x)/65535.0,1/GREY_GAMMA)
#else
#define CG(x) x
#define GG(x) x
#endif

	if (nr > 0)
	{
		for (r = 0; r < nr; r++)
		{
			for (g = 0; g < ng; g++)
			{
				for (b = 0; b < nb; b++)
				{
					color.red = CG(r * 65535 / (nr - 1));
					color.green = CG(g * 65535 / (ng - 1));
					color.blue = CG(b * 65535 / (nb - 1));
					if (do_allocate)
					{
						if (!XAllocColor(Pdpy, Pcmap,
								 &color))
						{
							free_table_colors(
								color_table, i);
							free(color_table);
							return NULL;
						}
						color_table[i].color.pixel =
							color.pixel;
						color_table[i].alloc_count = 1;
					}
					else
					{
						color_table[i].alloc_count = 0;
					}
					color_table[i].color.red = color.red;
					color_table[i].color.green = color.green;
					color_table[i].color.blue = color.blue;
					i++;
				}
			}
		}
	}

	if (ngrey > 0)
	{
		for (grey = start_grey; grey < end_grey; grey++)
		{
			color.red = color.green = color.blue =
				GG(grey * 65535 / (ngrey - 1));
			if (do_allocate)
			{
				if (!XAllocColor(Pdpy, Pcmap, &color))
				{
					free_table_colors(color_table, i);
					free(color_table);
					return NULL;
				}
				color_table[i].color.pixel = color.pixel;
				color_table[i].alloc_count = 1;
			}
			else
			{
				color_table[i].alloc_count = 0;
			}
			color_table[i].color.red = color.red;
			color_table[i].color.green = color.green;
			color_table[i].color.blue = color.blue;
			i++;
		}
	}
	color_table[size].color.red = color_table[size-1].color.red;
	color_table[size].color.green = color_table[size-1].color.green;
	color_table[size].color.blue = color_table[size-1].color.blue;
	color_table[size].color.pixel = color_table[size-1].color.pixel;
	color_table[size].alloc_count = 0;
	PColorLimit = size;
	return color_table;
}


static
PColor *alloc_named_ct(int *limit, Bool do_allocate)
{

/* First thing in base array are colors probably already in the color map
   because they have familiar names.
   I pasted them into a xpm and spread them out so that similar colors are
   spread out.
   Toward the end are some colors to fill in the gaps.
   Currently 61 colors in this list.
*/
	char *color_names[] =
	{
		"black",
		"white",
		"grey",
		"green",
		"blue",
		"red",
		"cyan",
		"yellow",
		"magenta",
		"DodgerBlue",
		"SteelBlue",
		"chartreuse",
		"wheat",
		"turquoise",
		"CadetBlue",
		"gray87",
		"CornflowerBlue",
		"YellowGreen",
		"NavyBlue",
		"MediumBlue",
		"plum",
		"aquamarine",
		"orchid",
		"ForestGreen",
		"lightyellow",
		"brown",
		"orange",
		"red3",
		"HotPink",
		"LightBlue",
		"gray47",
		"pink",
		"red4",
		"violet",
		"purple",
		"gray63",
		"gray94",
		"plum1",
		"PeachPuff",
		"maroon",
		"lavender",
		"salmon",           /* for peachpuff, orange gap */
		"blue4",            /* for navyblue/mediumblue gap */
		"PaleGreen4",       /* for forestgreen, yellowgreen gap */
		"#AA7700",          /* brick, no close named color */
		"#11EE88",          /* light green, no close named color */
		"#884466",          /* dark brown, no close named color */
		"#CC8888",          /* light brick, no close named color */
		"#EECC44",          /* gold, no close named color */
		"#AAAA44",          /* dull green, no close named color */
		"#FF1188",          /* pinkish red */
		"#992299",          /* purple */
		"#CCFFAA",          /* light green */
		"#664400",          /* dark brown*/
		"#AADD99",          /* light green */
		"#66CCFF",          /* light blue */
		"#CC2299",          /* dark red */
		"#FF11CC",          /* bright pink */
		"#11CC99",          /* grey/green */
		"#AA77AA",          /* purple/red */
		"#EEBB77"           /* orange/yellow */
	};
	int NColors = sizeof(color_names)/sizeof(char *);
	int i,rc;
	PColor *color_table;
	XColor color;

	*limit = (*limit > NColors)? NColors: *limit;
	color_table = xmalloc((*limit+1) * sizeof(PColor));
	for(i=0; i<*limit; i++)
	{
		rc=XParseColor(Pdpy, Pcmap, color_names[i], &color);
		if (rc==0) {
			fprintf(stderr,"color_to_rgb: can't parse color %s,"
				" rc %d\n", color_names[i], rc);
			free_table_colors(color_table, i);
			free(color_table);
			return NULL;
		}
		if (do_allocate)
		{
			if (!XAllocColor(Pdpy, Pcmap, &color))
			{
				free_table_colors(color_table, i);
				free(color_table);
				return NULL;
			}
			color_table[i].color.pixel = color.pixel;
			color_table[i].alloc_count = 1;
		}
		else
		{
			color_table[i].alloc_count = 0;
		}
		color_table[i].color.red = color.red;
		color_table[i].color.green = color.green;
		color_table[i].color.blue = color.blue;
	}
	color_table[*limit].color.red = color_table[*limit-1].color.red;
	color_table[*limit].color.green = color_table[*limit-1].color.green;
	color_table[*limit].color.blue = color_table[*limit-1].color.blue;
	color_table[*limit].color.pixel = color_table[*limit-1].color.pixel;
	color_table[*limit].alloc_count = 0;
	PColorLimit = *limit;
	return color_table;
}

static
void create_mapping_table(
	int nr, int ng, int nb, int ngrey, int grey_bits,
	Bool non_regular_pallet)
{

	Pcsi.grey_bits = 0;

	/* initialize dithering colors numbers */
	if (!non_regular_pallet)
	{
		/* */
		Pcsi.d_nr = nr;
		Pcsi.d_ng = ng;
		Pcsi.d_nb = nb;
		Pcsi.d_ngrey_bits = 2;
		while((1<<Pcsi.d_ngrey_bits) < ngrey)
		{
			Pcsi.d_ngrey_bits++;
		}
		if (1<<Pcsi.d_ngrey_bits != ngrey)
		{
			Pcsi.d_ngrey_bits = 0;
		}
		Pcsi.grey_bits = grey_bits;
	}
	else
	{
		/* dither table should be small */
		Pcsi.grey_bits = 0;
		if (PColorLimit <= 9)
		{
			Pcsi.d_nr = 3;
			Pcsi.d_ng = 3;
			Pcsi.d_nb = 3;
			Pcsi.d_ngrey_bits = 0;
		}
		else if (PColorLimit <= 64)
		{
			Pcsi.d_nr = 4;
			Pcsi.d_ng = 4;
			Pcsi.d_nb = 4;
			Pcsi.d_ngrey_bits = 0;
		}
		else
		{
			Pcsi.d_nr = 8;
			Pcsi.d_ng = 8;
			Pcsi.d_nb = 8;
			Pcsi.d_ngrey_bits = 0;
		}
		PDitherMappingTable = build_mapping_table(
			Pcsi.d_nr, Pcsi.d_ng, Pcsi.d_nb, non_regular_pallet);
	}

	/* initialize colors number fo index computation */
	if (PColorLimit == 2)
	{
		/* ok */
		Pcsi.nr = 0;
		Pcsi.ng = 0;
		Pcsi.nb = 0;
		Pcsi.ngrey = 0;
		Pcsi.grey_bits = 1;
	}
	else if (grey_bits > 0)
	{
		Pcsi.nr = 0;
		Pcsi.ng = 0;
		Pcsi.nb = 0;
		Pcsi.ngrey = 0;
		Pcsi.grey_bits = grey_bits;
	}
	else if (non_regular_pallet || (0&&ngrey>0))
	{
		/* note: using these table with !used_named && ngrey>0 will
		 * probably leads to faster image loading. But I see nothing
		 * of significative. On the others hands not using it gives
		 * maybe better colors approximation. */
		if (PColorLimit <= 9)
		{
			Pcsi.nr = 8;
			Pcsi.ng = 8;
			Pcsi.nb = 8;
			Pcsi.ngrey = 0;
		}
		else
		{
			Pcsi.nr = 16;
			Pcsi.ng = 16;
			Pcsi.nb = 16;
			Pcsi.ngrey = 0;
		}
		PMappingTable = build_mapping_table(
			Pcsi.nr, Pcsi.ng, Pcsi.nb, non_regular_pallet);
	}
	else
	{
		Pcsi.nr = nr;
		Pcsi.ng = ng;
		Pcsi.nb = nb;
		Pcsi.ngrey = ngrey;
		Pcsi.grey_bits = 0;
	}
}

static void finish_ct_init(
	int call_type, int ctt, int nr, int ng, int nb, int ngrey,
	int grey_bits, Bool use_named)
{
	if (call_type == PICTURE_CALLED_BY_FVWM)
	{
		char *env;

		if (PAllocTable)
		{
			ctt = PICTURE_PAllocTable + ctt;
		}
		if (PUseDynamicColors)
		{
			ctt = PICTURE_PUseDynamicColors + ctt;
		}
		if (PStrictColorLimit)
		{
			ctt = PICTURE_PStrictColorLimit + ctt;
		}
		if (use_named)
		{
			ctt = PICTURE_use_named + ctt;
		}
		else
		{
			ctt++;
		}
		/* TA:  FIXME!  Spelling!!! */
		/* TA:  FIXME!  Use xasprintf() */
		env = xmalloc(PICTURE_TABLETYPE_LENGHT + 1);
		sprintf(env, "%i", ctt);
		flib_putenv("FVWM_COLORTABLE_TYPE", env);
		free(env);
		if (Pdepth <= 8)
		{
			Pac = xcalloc((1 << Pdepth), sizeof(PColor));
		}
	}

	if (Pct)
	{
		if (!PAllocTable && call_type == PICTURE_CALLED_BY_FVWM)
		{
			free_table_colors(Pct, PColorLimit);
		}
		create_mapping_table(nr,ng,nb,ngrey,grey_bits,use_named);
	}
}

#define PA_COLOR_CUBE (1 << 1)
#define FVWM_COLOR_CUBE (1 << 2)
#define PA_GRAY_SCALE (1 << 3)
#define FVWM_GRAY_SCALE (1 << 4)
#define ANY_COLOR_CUBE (PA_COLOR_CUBE|FVWM_COLOR_CUBE)
#define ANY_GRAY_SCALE (PA_GRAY_SCALE|FVWM_GRAY_SCALE)

static
int PictureAllocColorTable(
	PictureColorLimitOption *opt, int call_type, Bool use_my_color_limit)
{
	char *envp;
	int free_colors, nbr_of_color, limit, cc_nbr, i, size;
	int use_named_table = 0;
	int do_allocate = 0;
	int use_default = 1;
	int private_cmap = !(Pdefault);
	int color_limit;
	int pa_type = (Pvisual->class != GrayScale) ?
		PA_COLOR_CUBE : PA_GRAY_SCALE;
	int fvwm_type = (Pvisual->class != GrayScale) ?
		FVWM_COLOR_CUBE : FVWM_GRAY_SCALE;
	int cc[][6] =
	{
		/* {nr,ng,nb,ngrey,grey_bits,logic} */
		/* 5 first for direct colors and Pdepth > 8*/
		/* 8192 colors depth 13, a reasonable max for a color table */
		{16, 32, 16, 0, 0, FVWM_COLOR_CUBE},
		/* 4096 colors depth 12 */
		{16, 16, 16, 0, 0, FVWM_COLOR_CUBE},
		/* 1024 colors depth 10 */
		{8, 16, 8, 0, 0, FVWM_COLOR_CUBE},
		/* 512 colors depth 9 */
		{8, 8, 8, 0, 0, FVWM_COLOR_CUBE},
		/* 256 colors 3/3/2 standard colormap */
		{8, 8, 4, 0, 0, FVWM_COLOR_CUBE},
		/* 256 grey scale */
		{0, 0, 0, 0, 8, ANY_GRAY_SCALE},
		/* 244 Xrender XFree-4.2 */
		{6, 6, 6, 30, 0, ANY_COLOR_CUBE},
		/* 216 Xrender XFree-4.2,GTK/QT "default cc" */
		{6, 6, 6, 0, 0, ANY_COLOR_CUBE},
		/* 180 (GTK) */
		{6, 6, 5, 0, 0, ANY_COLOR_CUBE},
		/* 144 (GTK) */
		{6, 6, 4, 0, 0, ANY_COLOR_CUBE},
		/* 128 grey scale */
		{0, 0, 0, 0, 7, ANY_GRAY_SCALE},
		/* 125 GTK mini default cc (may change? 444) */
		{5, 5, 5, 0, 0, ANY_COLOR_CUBE},
		/* 100 (GTK with color limit) */
		{5, 5, 4, 0, 0, ANY_COLOR_CUBE},
		/* 85 Xrender XFree-4.3 */
		{4, 4, 4, 23, 0, ANY_COLOR_CUBE},
		/* 78 (in fact 76)  a good default ??*/
		{4, 4, 4, 16, 0, FVWM_COLOR_CUBE},
		/* 70  a good default ?? */
		{4, 4, 4, 8, 0, ANY_COLOR_CUBE},
		/* 68  a good default ?? */
		{4, 4, 4, 6, 0, ANY_COLOR_CUBE},
		/* 64 Xrender XFree-4.3 (GTK wcl) */
		{4, 4, 4, 0, 0, ANY_COLOR_CUBE},
		/* 64 grey scale */
		{0, 0, 0, 0, 6, ANY_GRAY_SCALE},
		/* 54, maybe a good default? */
		{4, 4, 3, 8, 0, FVWM_COLOR_CUBE},
		/* 48, (GTK wcl) no grey but ok */
		{4, 4, 3, 0, 0, FVWM_COLOR_CUBE},
		/* 32, 2/2/1 standard colormap */
		{4, 4, 2, 0, 0, FVWM_COLOR_CUBE},
		/* 32 xrender xfree-4.2 */
		{0, 0, 0, 0, 6, ANY_GRAY_SCALE},
		/* 29 */
		{3, 3, 3, 4, 0, FVWM_COLOR_CUBE},
		/* 27 (xrender in depth 6&7(hypo) GTK wcl) */
		{3, 3, 3, 0, 0, FVWM_COLOR_CUBE|PA_COLOR_CUBE*(Pdepth<8)},
		/* 16 grey scale */
		{0, 0, 0, 0, 4, FVWM_GRAY_SCALE},
		/* 10 */
		{2, 2, 2, 4, 0, FVWM_COLOR_CUBE},
                /* 8 (xrender/qt/gtk wcl) */
		{2, 2, 2, 0, 0, FVWM_COLOR_CUBE},
		/* 8 grey scale Xrender depth 4 and XFree-4.3 */
		{0, 0, 0, 0, 3, FVWM_GRAY_SCALE|PA_GRAY_SCALE*(Pdepth<5)},
		/* 4 grey scale*/
		{0, 0, 0, 0, 2,
		 FVWM_GRAY_SCALE|FVWM_COLOR_CUBE|PA_COLOR_CUBE*(Pdepth<4)},
		/* 2 */
		{0, 0, 0, 0, 1, FVWM_COLOR_CUBE|FVWM_GRAY_SCALE}
	};

	cc_nbr = sizeof(cc)/(sizeof(cc[0]));

	/* set up  default */
	PStrictColorLimit = 0;
	PUseDynamicColors = 1;
	PAllocTable = 0;
	use_named_table = False;
	color_limit = 0;
	use_default = True;

	/* use fvwm color limit */
	if (!use_my_color_limit &&
	     (envp = getenv("FVWM_COLORTABLE_TYPE")) != NULL)
	{
		int nr = 0, ng = 0, nb = 0, grey_bits = 0, ngrey = 0;
		int ctt = atoi(envp);

		if (ctt >= PICTURE_PAllocTable)
		{
			ctt -= PICTURE_PAllocTable;
			PAllocTable = 1; /* not useful for a module !*/
		}
		if (ctt >= PICTURE_PUseDynamicColors)
		{
			PUseDynamicColors = 1;
			ctt -= PICTURE_PUseDynamicColors;
		}
		if (ctt >= PICTURE_PStrictColorLimit)
		{
			PStrictColorLimit = 1;
			ctt -= PICTURE_PStrictColorLimit;
		}
		if (ctt >= PICTURE_use_named)
		{
			ctt -= PICTURE_use_named;
			Pct = alloc_named_ct(&ctt, False);
			use_named_table = True;
		}
		else if (ctt == 0)
		{
			/* depth <= 8 and no colors limit ! */
			PColorLimit = 0;
			return 0;
		}
		else if (ctt <= cc_nbr)
		{
			ctt--;
			Pct = alloc_color_cube(
				cc[ctt][0], cc[ctt][1], cc[ctt][2], cc[ctt][3],
				cc[ctt][4],
				False);
			nr = cc[ctt][0];
			ng = cc[ctt][1];
			nb = cc[ctt][2];
			ngrey = cc[ctt][3];
			grey_bits = cc[ctt][4];
		}
		if (Pct != NULL)
		{
			/* should always happen */
			finish_ct_init(
				call_type, ctt, nr, ng, nb, ngrey, grey_bits,
				use_named_table);
			return PColorLimit;
		}
	}

	nbr_of_color = (1 << Pdepth);
	color_limit = 0;

	/* parse the color limit env variable */
	if ((envp = getenv("FVWM_COLORLIMIT")) != NULL)
	{
		char *rest, *l;

		rest = GetQuotedString(envp, &l, ":", NULL, NULL, NULL);
		if (l && *l != '\0' && (color_limit = atoi(l)) >= 0)
		{
			use_default = 0;
		}
		if (l != NULL)
		{
			free(l);
		}
		if (color_limit == 9 || color_limit == 61)
		{
			use_named_table = 1;
		}
		if (rest && *rest != '\0')
		{
			if (rest[0] == '1')
			{
				PStrictColorLimit = 1;
			}
			else
			{
				PStrictColorLimit = 0;
			}
			if (strlen(rest) > 1 && rest[1] == '1')
			{
				use_named_table = 1;
			}
			else
			{
				use_named_table = 0;
			}
			if (strlen(rest) > 2 && rest[2] == '1')
			{
				PUseDynamicColors = 1;
			}
			else
			{
				PUseDynamicColors = 0;
			}
			if (strlen(rest) > 3 && rest[3] == '1')
			{
				PAllocTable = 1;
			}
			else
			{
				PAllocTable = 0;
			}
		}
	}
	else if (opt != NULL) /* use the option */
	{
		if (opt->color_limit > 0)
		{
			use_default = 0;
			color_limit = opt->color_limit;
		}
		if (color_limit == 9 || color_limit == 61)
		{
			use_named_table = 1;
		}
		if (opt->strict > 0)
		{
			PStrictColorLimit = 1;
		}
		else if (opt->strict == 0)
		{
			PStrictColorLimit = 0;
		}
		if (opt->use_named_table > 0)
		{
			use_named_table = 1;
		}
		else if (opt->use_named_table == 0)
		{
			use_named_table = 0;
		}
		if (opt->not_dynamic > 0)
		{
			PUseDynamicColors = 0;
		}
		else if (opt->not_dynamic == 0)
		{
			PUseDynamicColors = 0;
		}
		if (opt->allocate > 0)
		{
			PAllocTable = 1;
		}
		else if (opt->allocate == 0)
		{
			PAllocTable = 0;
		}
	}

	if (color_limit <= 0)
	{
		use_default = 1;
		color_limit = nbr_of_color;
	}

	/* first try to see if we have a "pre-allocated" color cube.
	 * The bultin RENDER X extension pre-allocate a color cube plus
	 * some grey's (xc/programs/Xserver/render/miindex)
	 * See gdk/gdkrgb.c for the cubes used by gtk+-2, 666 is the default,
	 * 555 is the minimal cc (this may change): if gtk cannot allocate
	 * the 555 cc (or better) a private cmap is used.
	 * for qt-3: see src/kernel/{qapplication.cpp,qimage.cpp,qcolor_x11.c}
	 * the 666 cube is used by default (with approx in the cmap if some
	 * color allocation fail), and some qt app may accept an
	 * --ncols option to limit the nbr of colors, then some "2:3:1"
	 * proportions color cube are used (222, 232, ..., 252, 342, ..., 362,
	 * 452, ...,693, ...)
	 * imlib2 try to allocate the 666 cube if this fail it try more
	 * exotic table (see rend.c and rgba.c) */
	i = 0;
	free_colors = 0;
	if (Pdepth <= 8 && !private_cmap && use_default &&
	    i < cc_nbr && Pct == NULL && (Pvisual->class & 1))
	{
		free_colors = get_nbr_of_free_colors(nbr_of_color);
	}
	while(Pdepth <= 8 && !private_cmap && use_default &&
	      i < cc_nbr && Pct == NULL && (Pvisual->class & 1))
	{
		size = cc[i][0]*cc[i][1]*cc[i][2] + cc[i][3] -
			2*(cc[i][3] > 0) + (1 << cc[i][4])*(cc[i][4] != 0);
		if (size > nbr_of_color || !(cc[i][5] & pa_type))
		{
			i++;
			continue;
		}
		if (free_colors <= nbr_of_color - size)
		{
			Pct = alloc_color_cube(
				cc[i][0], cc[i][1], cc[i][2], cc[i][3],
				cc[i][4], True);
		}
		if (Pct != NULL)
		{
			if (free_colors <=
			    get_nbr_of_free_colors(nbr_of_color))
			{
				/* done */
			}
			else
			{
				free_table_colors(Pct, PColorLimit);
				free(Pct);
				Pct = NULL;
			}
		}
		i++;
	}
	if (Pct != NULL)
	{
		PUseDynamicColors = 0;
		PAllocTable = 1;
		Pcsi.pre_allocated_pallet = 1;
		i = i - 1;
		finish_ct_init(
			call_type, i, cc[i][0], cc[i][1], cc[i][2], cc[i][3],
			cc[i][4], 0);
		return PColorLimit;
	}

	/*
	 * now use "our" table
	 */

	limit = (color_limit >= nbr_of_color)? nbr_of_color:color_limit;

	if (use_default && !private_cmap)
	{
		/* XRender cvs default: */
#if 0
		if (limit > 100)
			limit = nbr_of_color/3;
		else
			limit = nbr_of_color/2;
		/* depth 8: 85 */
		/* depth 4: 8 */
#endif
		if (limit > 256)
		{
			/* direct colors & Pdepth > 8 */
			if (Pdepth >= 16)
			{
				limit = 8192;
			}
			else if (Pdepth >= 15)
			{
				limit = 4096;
			}
			else
			{
				limit = 512;
			}
		}
		else if (limit == 256)
		{
			if (Pvisual->class == GrayScale)
			{
				limit = 64;
			}
			else if (Pvisual->class == DirectColor)
			{
				limit = 32;
			}
			else
			{
				limit = 68;
				/* candidate:
				 * limit = 54;  4x4x3 + 6 grey
				 * limit = 61 (named table)
				 * limit = 85 current XRender default 4cc + 21
				 * limit = 76 future(?) XRender default 4cc + 16
				 * limit = 68 4x4x4 + 4
				 * limit = 64 4x4x4 + 0 */
			}

		}
		else if (limit == 128 || limit == 64)
		{
			if (Pvisual->class == GrayScale)
			{
				limit = 32;
			}
			else
			{
				limit = 31;
			}
		}
		else if (limit >= 16)
		{
			if (Pvisual->class == GrayScale)
			{
				limit = 8;
			}
			else
			{
				limit = 10;
			}
		}
		else if (limit >= 8)
		{
			limit = 4;
		}
		else
		{
			limit = 2;
		}
	}
	if (limit < 2)
	{
		limit = 2;
	}

	if (Pvisual->class == DirectColor)
	{
		/* humm ... Any way this case should never happen in real life:
		 * DirectColor default colormap! */
		PUseDynamicColors = 0;
		PAllocTable = 1;
		PStrictColorLimit = 1;
	}
	if (PAllocTable)
	{
		do_allocate = 1;
	}
	else
	{
		do_allocate = 0;
	}

	/* use the named table ? */
	if (use_named_table)
	{
		i = limit;
		while(Pct == NULL && i >= 2)
		{
			Pct = alloc_named_ct(&i, do_allocate);
			i--;
		}
	}
	if (Pct != NULL)
	{
		finish_ct_init(
			call_type, PColorLimit, 0, 0, 0, 0, 0, 1);
		return PColorLimit;
	}

	/* color cube or regular grey scale */
	i = 0;
	while(i < cc_nbr && Pct == NULL)
	{
		if ((cc[i][5] & fvwm_type) &&
		    cc[i][0]*cc[i][1]*cc[i][2] + cc[i][3] - 2*(cc[i][3] > 0) +
		    (1 << cc[i][4])*(cc[i][4] != 0) <= limit)
		{
			Pct = alloc_color_cube(
				cc[i][0], cc[i][1], cc[i][2], cc[i][3], cc[i][4],
				do_allocate);
		}
		i++;
	}
	if (Pct != NULL)
	{
		i = i-1;
		finish_ct_init(
			call_type, i, cc[i][0], cc[i][1], cc[i][2], cc[i][3],
			cc[i][4], 0);
		return PColorLimit;
	}

	/* I do not think we can be here */
	Pct = alloc_color_cube(0, 0, 0, 0, 1, False);
	finish_ct_init(call_type, cc_nbr-1, 0, 0, 0, 0, 1, 0);
	if (Pct == NULL)
	{
		fprintf(stderr,
			"[fvwm] ERR -- Cannot get Black and White. exiting!\n");
		exit(2);
	}
	return PColorLimit;
}

/*
 * Allocation of a private DirectColor cmap this is broken for depth > 16
 */
static
Bool alloc_direct_colors(int *limit, Bool use_my_color_limit)
{
	unsigned long nr,ng,nb,r,g,b,cr,cg,cf,pr,pg;
	unsigned long red_mask, green_mask, blue_mask;
	XColor *colors;

	if (Pdepth <= 16)
	{
		red_mask = Pvisual->red_mask;
		green_mask = Pvisual->green_mask;
		blue_mask = Pvisual->blue_mask;
	}
	else
	{
		/* Use a standard depth 16 colormap. This is broken FIXME! */
		red_mask = 0xf800;
		green_mask = 0x7e0;
		blue_mask = 0x1f;
	}

	decompose_mask(
		red_mask, &Pcsi.red_shift, &Pcsi.red_prec);
	decompose_mask(
		green_mask, &Pcsi.green_shift, &Pcsi.green_prec);
	decompose_mask(
		blue_mask, &Pcsi.blue_shift, &Pcsi.blue_prec);

	if (!use_my_color_limit)
	{
		/* colors allocated by fvwm we can return */
		return 1;
	}

	nr = 1 << Pcsi.red_prec;
	ng = 1 << Pcsi.green_prec;
	nb = 1 << Pcsi.blue_prec;

	colors = xmalloc(nb*sizeof(XColor));
	cf = DoRed|DoBlue|DoGreen;
	for (r=0; r<nr; r++)
	{
		cr = r * 65535 / (nr - 1);
		pr = (cr >> (16 - Pcsi.red_prec)) << Pcsi.red_shift;
		for (g = 0; g < ng; g++)
		{
			cg = g * 65535 / (ng - 1);
			pg = (cg >> (16 - Pcsi.green_prec)) << Pcsi.green_shift;
			for (b = 0; b < nb; b++)
			{
				colors[b].flags = cf;
				colors[b].red = cr;
				colors[b].green = cg;
				colors[b].blue = b * 65535 / (nb - 1);
				colors[b].pixel =
					(Pixel)(pr + pg +
						((colors[b].blue  >>
						  (16 - Pcsi.blue_prec)) <<
						 Pcsi.blue_shift));
			}
			XStoreColors(Pdpy, Pcmap, colors, nb);
		}
	}
	free(colors);
	return 1;
}

/*
 * Init the table for Static Colors
 */
static
void init_static_colors_table(void)
{
	XColor colors[256];
	int i;
	int nbr_of_colors = min(256, (1 << Pdepth));

	PColorLimit = nbr_of_colors;
	Pct = xmalloc((nbr_of_colors+1) * sizeof(PColor));
	for (i = 0; i < nbr_of_colors; i++)
	{
		colors[i].pixel = Pct[i].color.pixel = i;
	}
	XQueryColors(Pdpy, Pcmap, colors, nbr_of_colors);
	for (i = 0; i < nbr_of_colors; i++)
	{
		Pct[i].color.red = colors[i].red;
		Pct[i].color.green = colors[i].green;
		Pct[i].color.blue = colors[i].blue;
		Pct[i].alloc_count = 1;
	}
	Pct[PColorLimit].color.red = Pct[PColorLimit-1].color.red;
	Pct[PColorLimit].color.green = Pct[PColorLimit-1].color.green;
	Pct[PColorLimit].color.blue = Pct[PColorLimit-1].color.blue;
	Pct[PColorLimit].alloc_count = 1;
	create_mapping_table(0, 0, 0, 0, 0, True);
}


/*
 * misc local functions
 */
static
void print_colormap(Colormap cmap)
{
	XColor colors[256];
	int i;
	int nbr_of_colors = max(256, (1 << Pdepth));
	for (i = 0; i < nbr_of_colors; i++)
	{
		colors[i].pixel = i;
	}
	XQueryColors(Pdpy, cmap, colors, nbr_of_colors);
	for (i = 0; i < nbr_of_colors; i++)
	{
		fprintf(stderr,"    rgb(%.3i): %.3i/%.3i/%.3i\n", i,
			colors[i].red >> 8,
			colors[i].green >> 8,
			colors[i].blue >> 8);
	}
}

/* ---------------------------- interface functions ------------------------ */

int PictureAllocColor(Display *dpy, Colormap cmap, XColor *c, int no_limit)
{
	if (PStrictColorLimit && Pct != NULL)
	{
		no_limit = 0;
	}

	if (no_limit)
	{
		return Pcsi.alloc_color_no_limit(dpy, cmap, c);
	}
	else
	{
		return Pcsi.alloc_color(dpy, cmap, c);
	}
	return 0;
}


int PictureAllocColorAllProp(
	Display *dpy, Colormap cmap, XColor *c, int x, int y,
	Bool no_limit, Bool is_8, Bool do_dither)
{

	if (!no_limit && do_dither && Pcsi.alloc_color_dither != NULL)
	{
		if (!is_8)
		{
			c->red = c->red >> 8;
			c->green = c->green >> 8;
			c->blue = c->blue >> 8;
		}
		return Pcsi.alloc_color_dither(dpy, cmap, c, x, y);
	}
	else
	{
		if (is_8)
		{
			c->red = c->red << 8;
			c->green = c->green << 8;
			c->blue = c->blue << 8;
		}
		return PictureAllocColor(dpy, cmap, c, False);
	}
	return 0;
}

int PictureAllocColorImage(
	Display *dpy, PictureImageColorAllocator *pica, XColor *c, int x, int y)
{
	int r;

	r = PictureAllocColorAllProp(
		dpy, pica->cmap, c, x, y,
		pica->no_limit, pica->is_8, pica->dither);
	if (r && pica->pixels_table != NULL && pica->pixels_table_size &&
	    c->pixel < pica->pixels_table_size)
	{
		pica->pixels_table[c->pixel]++;
	}
	return r;
}

PictureImageColorAllocator *PictureOpenImageColorAllocator(
	Display *dpy, Colormap cmap, int x, int y, Bool no_limit,
	Bool do_not_save_pixels, int dither, Bool is_8)
{
	PictureImageColorAllocator *pica;
	Bool do_save_pixels = False;

	pica = xmalloc(sizeof(PictureImageColorAllocator));
	if (Pdepth <= 8 && !do_not_save_pixels && (Pvisual->class & 1) &&
	    ((PUseDynamicColors && Pct) || no_limit))
	{
		int s =  1 << Pdepth;
		pica->pixels_table = xcalloc(s, sizeof(unsigned long));
		pica->pixels_table_size = s;
		do_save_pixels = True;
	}
	if (!do_save_pixels)
	{
		pica->pixels_table = NULL;
		pica->pixels_table_size = 0;
	}
	pica->is_8 = is_8;
	if (dither && Pdepth <= 16)
	{
		pica->dither = dither;
	}
	else
	{
		pica->dither = dither;
	}
	pica->no_limit = no_limit;
	pica->cmap = cmap;
	return pica;
}

void PictureCloseImageColorAllocator(
	Display *dpy, PictureImageColorAllocator *pica,
	int *nalloc_pixels, Pixel **alloc_pixels, Bool *no_limit)
{
	if (nalloc_pixels)
	{
		*nalloc_pixels = 0;
	}
	if (alloc_pixels != NULL)
	{
		*alloc_pixels = NULL;
	}
	if (no_limit != NULL)
	{
		*no_limit = 0;
	}
	if (pica->pixels_table)
	{
		int i,j;
		int k = 0, l = 0;
		unsigned int np = 0;
		int free_num = 0;
		Pixel *free_pixels = NULL;
		Pixel *save_pixels = NULL;

		for(i = 0; i < pica->pixels_table_size; i++)
		{
			if (pica->pixels_table[i])
			{
				free_num += (pica->pixels_table[i]-1);
				np++;
			}
		}
		if (free_num)
		{
			free_pixels = xmalloc(free_num * sizeof(Pixel));
		}
		if (np && nalloc_pixels != NULL && alloc_pixels != NULL)
		{
			save_pixels = xmalloc(np * sizeof(Pixel));
		}
		for(i = 0; i < pica->pixels_table_size; i++)
		{
			if (pica->pixels_table[i])
			{
				if (save_pixels)
				{
					save_pixels[k++] = i;
				}
				for(j=1; j < pica->pixels_table[i]; j++)
				{
					free_pixels[l++] = i;
				}
			}
		}
		if (free_num)
		{
			PictureFreeColors(
				dpy, pica->cmap, free_pixels, free_num, 0,
				pica->no_limit);
			free(free_pixels);
		}
		if (nalloc_pixels != NULL && alloc_pixels != NULL)
		{
			*nalloc_pixels = np;
			*alloc_pixels = save_pixels;
			if (no_limit != NULL)
			{
				*no_limit = pica->no_limit;
			}
		}
		else if (save_pixels)
		{
			free(save_pixels);
		}
		free(pica->pixels_table);
	}
	free(pica);
	return;
}

void PictureFreeColors(
	Display *dpy, Colormap cmap, Pixel *pixels, int n,
	unsigned long planes, Bool no_limit)
{
	if (no_limit)
	{
		if (Pcsi.free_colors_no_limit != NULL)
		{
			Pcsi.free_colors_no_limit(
				dpy, cmap, pixels, n, planes);
		}
	}
	else
	{
		if (Pcsi.free_colors != NULL)
		{
			Pcsi.free_colors(dpy, cmap, pixels, n, planes);
		}
	}
	return;
}

Pixel PictureGetNextColor(Pixel p, int n)
{
	int i;
	XColor c;

	if (n >= 0)
		n = 1;
	else
		n = -1;

	if (Pct == NULL)
	{
		return p;
	}
	for(i=0; i<PColorLimit; i++)
	{
		if (Pct[i].color.pixel == p)
		{
			if (i == 0 && n < 0)
			{
				c = Pct[PColorLimit-1].color;
				alloc_color_in_pct(&c, PColorLimit-1);
				return Pct[PColorLimit-1].color.pixel;
			}
			else if (i == PColorLimit-1 && n > 0)
			{
				c = Pct[0].color;
				alloc_color_in_pct(&c, 0);
				return Pct[0].color.pixel;
			}
			else
			{
				c = Pct[i+n].color;
				alloc_color_in_pct(&c, i+n);
				return Pct[i+n].color.pixel;
			}
		}
	}
	return p;
}

/* Replace the color in my_color by the closest matching color
   from base_table */
void PictureReduceColorName(char **my_color)
{
	int index;
	XColor rgb;          /* place to calc rgb for each color in xpm */

	if (!XpmSupport)
		return;

	if (!strcasecmp(*my_color,"none")) {
		return; /* do not substitute the "none" color */
	}

	if (!XParseColor(Pdpy, Pcmap, *my_color, &rgb))
	{
		fprintf(stderr,"color_to_rgb: can't parse color %s\n",
			*my_color);
	}
	index = get_color_index(rgb.red,rgb.green,rgb.blue, False);
	/* Finally: replace the color string by the newly determined color
	 * string */
	free(*my_color);                    /* free old color */
	/* area for new color */
	/* TA:  FIXME!  xasprintf() */
	*my_color = xmalloc(8);
	sprintf(*my_color,"#%x%x%x",
		Pct[index].color.red >> 8,
		Pct[index].color.green >> 8,
		Pct[index].color.blue >> 8); /* put it there */
	return;
}

Bool PictureDitherByDefault(void)
{
	if (Pct != NULL)
	{
		return True;
	}
	return False;
}

Bool PictureUseBWOnly(void)
{
	if (Pdepth < 2 || (PStrictColorLimit && PColorLimit == 2))
	{
		return True;
	}
	return False;
}

int PictureInitColors(
	int call_type, Bool init_color_limit, PictureColorLimitOption *opt,
	Bool use_my_color_limit, Bool init_dither)
{
	Bool dither_ok = False;

	switch (Pvisual->class)
	{
	case DirectColor:
		/* direct colors is more or less broken */
		decompose_mask(
			Pvisual->red_mask, &Pcsi.red_shift,
			&Pcsi.red_prec);
		decompose_mask(
			Pvisual->green_mask, &Pcsi.green_shift,
			&Pcsi.green_prec);
		decompose_mask(
			Pvisual->blue_mask, &Pcsi.blue_shift,
			&Pcsi.blue_prec);
		Pcsi.alloc_color_no_limit = alloc_color_proportion;
		Pcsi.alloc_color = alloc_color_proportion;
		Pcsi.alloc_color_dither = alloc_color_proportion_dither;
		Pcsi.free_colors_no_limit = NULL;
		Pcsi.free_colors = NULL;
		PColorLimit = 0;
		break;
	case TrueColor:
		decompose_mask(
			Pvisual->red_mask, &Pcsi.red_shift,
			&Pcsi.red_prec);
		decompose_mask(
			Pvisual->green_mask, &Pcsi.green_shift,
			&Pcsi.green_prec);
		decompose_mask(
			Pvisual->blue_mask, &Pcsi.blue_shift,
			&Pcsi.blue_prec);
		Pcsi.alloc_color_no_limit = alloc_color_proportion;
		Pcsi.alloc_color = alloc_color_proportion;
		Pcsi.free_colors_no_limit = NULL;
		Pcsi.free_colors = NULL;
		PColorLimit = 0;
		if (init_dither && (Pdepth == 15 || Pdepth == 16))
		{
			dither_ok = my_dither_depth_15_16_init();
		}
		if (dither_ok)
		{
			Pcsi.alloc_color_dither = alloc_color_proportion_dither;
		}
		else
		{
			Pcsi.alloc_color_dither = NULL;
		}
		break;
	case StaticColor:
		if (0 && Pvisual->red_mask != 0 && Pvisual->green_mask != 0 &&
		    Pvisual->blue_mask != 0)
		{
			decompose_mask(
				Pvisual->red_mask, &Pcsi.red_shift,
				&Pcsi.red_prec);
			decompose_mask(
				Pvisual->green_mask, &Pcsi.green_shift,
				&Pcsi.green_prec);
			decompose_mask(
				Pvisual->blue_mask, &Pcsi.blue_shift,
				&Pcsi.blue_prec);
			Pcsi.alloc_color_no_limit = alloc_color_proportion;
			Pcsi.alloc_color = alloc_color_proportion;
			PColorLimit = 0;
		}
		else
		{
			if (init_color_limit)
			{
				Pcsi.alloc_color = alloc_color_in_table;
				Pcsi.alloc_color_dither =
					alloc_color_in_table_dither;
				Pcsi.alloc_color_no_limit = alloc_color_x;
				init_static_colors_table();
			}
			else
			{
				Pcsi.alloc_color = alloc_color_x;
				Pcsi.alloc_color_dither = NULL;
				Pcsi.alloc_color_no_limit = alloc_color_x;
			}
		}
		Pcsi.free_colors_no_limit = NULL;
		Pcsi.free_colors = NULL;
		break;
	case StaticGray:
		/* FIXME: we assume that we have a regular grey ramp */
		if (0)
		{
			Pcsi.alloc_color_no_limit = alloc_color_proportion_grey;
			Pcsi.alloc_color = alloc_color_proportion;
			PColorLimit = 0;
		}
		else
		{
			if (init_color_limit)
			{
				Pcsi.alloc_color = alloc_color_in_table;
				Pcsi.alloc_color_dither =
					alloc_color_in_table_dither;
				Pcsi.alloc_color_no_limit = alloc_color_x;
				init_static_colors_table();
			}
			else
			{
				Pcsi.alloc_color = alloc_color_x;
				Pcsi.alloc_color_dither = NULL;
				Pcsi.alloc_color_no_limit = alloc_color_x;
			}
		}
		Pcsi.free_colors_no_limit = NULL;
		Pcsi.free_colors = NULL;
		break;
	case PseudoColor:
	case GrayScale:
	default:
		Pcsi.alloc_color_no_limit = alloc_color_dynamic_no_limit;
		Pcsi.free_colors_no_limit = free_colors_x;
		break;
	}

	if (!(Pvisual->class & 1))
	{
		/* static classes */
		PUseDynamicColors = 0;
		if (call_type == PICTURE_CALLED_BY_FVWM &&
		    getenv("FVWM_COLORTABLE_TYPE") != NULL)
		{
			flib_putenv("FVWM_COLORTABLE_TYPE", "");
		}
		return PColorLimit;
	}

	/* dynamic classes */

	if (!Pdefault && Pvisual->class == DirectColor)
	{
		PColorLimit = 0;
		PUseDynamicColors = 0;
		alloc_direct_colors(0, use_my_color_limit);
		if (call_type == PICTURE_CALLED_BY_FVWM &&
		    getenv("FVWM_COLORTABLE_TYPE") != NULL)
		{
			flib_putenv("FVWM_COLORTABLE_TYPE", "");
		}
		return 0;
	}


	if (init_color_limit)
	{
		Pcsi.alloc_color = alloc_color_in_table;
		Pcsi.alloc_color_dither = alloc_color_in_table_dither;
		PictureAllocColorTable(opt, call_type, use_my_color_limit);
		if (PUseDynamicColors)
		{
			Pcsi.free_colors = free_colors_in_table;
		}
		else
		{
			Pcsi.free_colors = NULL;
		}
	}
	else
	{
		Pcsi.alloc_color = alloc_color_dynamic_no_limit;
		Pcsi.free_colors = free_colors_x;
		Pcsi.alloc_color_dither = NULL;
	}
	return PColorLimit;
}

void PicturePrintColorInfo(int verbose)
{
	unsigned long nbr_of_colors = 1 << Pdepth;

	fprintf(stderr, "fvwm info on colors\n");
	fprintf(stderr, "  Visual ID: 0x%x, Default?: %s, Class: ",
		(int)(Pvisual->visualid),
		(Pdefault)? "Yes":"No");
	if (Pvisual->class == TrueColor)
	{
		fprintf(stderr,"TrueColor");
	}
	else if (Pvisual->class == PseudoColor)
	{
		fprintf(stderr,"PseudoColor");
	}
	else if (Pvisual->class == DirectColor)
	{
		fprintf(stderr,"DirectColor");
	}
	else if (Pvisual->class == StaticColor)
	{
		fprintf(stderr,"StaticColor");
	}
	else if (Pvisual->class == GrayScale)
	{
		fprintf(stderr,"GrayScale");
	}
	else if (Pvisual->class == StaticGray)
	{
		fprintf(stderr,"StaticGray");
	}
	fprintf(stderr, "\n");
	fprintf(stderr, "  Depth: %i, Number of colors: %lu",
		Pdepth, (unsigned long)nbr_of_colors);
	if (Pct != NULL)
	{
		fprintf(stderr,"\n  Pallet with %i colors", PColorLimit);
		if (Pvisual->class & 1)
		{
			fprintf(stderr,", Number of free colors: %i\n",
				get_nbr_of_free_colors((1 << Pdepth)));
			fprintf(stderr,
				"  Auto Detected: %s, Strict: %s, Allocated: %s,"
				" Dynamic: %s\n",
				(Pcsi.pre_allocated_pallet)? "Yes":"No",
				(PStrictColorLimit)? "Yes":"No",
				(PAllocTable)? "Yes":"No",
				(PUseDynamicColors)? "Yes":"No");
		}
		else
		{
			fprintf(stderr," (default colormap)\n");
		}
		if (PColorLimit <= 256)
		{
			int i;
			int count = 0;
			int count_alloc = 0;

			if (verbose)
			{
				fprintf(stderr,"  The fvwm colors table:\n");
			}
			for (i = 0; i < PColorLimit; i++)
			{
				if (verbose)
				{
					fprintf(
						stderr,
						"    rgb:%.3i/%.3i/%.3i\t%lu\n",
						Pct[i].color.red >> 8,
						Pct[i].color.green >> 8,
						Pct[i].color.blue >> 8,
						Pct[i].alloc_count);
				}
				if (Pct[i].alloc_count)
				{
					count++;
				}
			}
			if ((Pvisual->class & 1) && Pac != NULL)
			{
				if (verbose)
				{
					fprintf(stderr,"  fvwm colors not in"
						" the table:\n");
				}
				for(i=0; i < nbr_of_colors; i++)
				{
					int j = 0;
					Bool found = False;

					if (!Pac[i].alloc_count)
						continue;
					while(j < PColorLimit && !found)
					{
						if (i == Pct[j].color.pixel)
						{
							found = True;
						}
						j++;
					}
					if (found)
						continue;
					count_alloc++;
					if (verbose)
					{
						fprintf(
							stderr,
							"    rgb:"
							"%.3i/%.3i/%.3i\t%lu\n",
							Pac[i].color.red >> 8,
							Pac[i].color.green >> 8,
							Pac[i].color.blue >> 8,
							Pac[i].alloc_count);
					}
				}
				if (verbose && count_alloc == 0)
				{
					if (verbose)
					{
						fprintf(stderr,"    None\n");
					}
				}
			}
			if (Pvisual->class & 1)
			{
				fprintf(stderr,
					"  Number of colours used by fvwm:\n");
				fprintf(stderr,
					"    In the table: %i\n", count);
				fprintf(
					stderr, "    Out of the table: %i\n",
					count_alloc);
				fprintf(stderr,
					"    Total: %i\n", count_alloc+count);
			}
		}
	}
	else
	{
		if (Pvisual->class == DirectColor)
		{
			fprintf(stderr, ", Pseudo Pallet with: %i colors\n",
				(1 << Pcsi.red_prec)*(1 << Pcsi.green_prec)*
				(1 << Pcsi.blue_prec));
		}
		else
		{
			fprintf(stderr, ", No Pallet (static colors)\n");
		}
		fprintf(stderr, "  red: %i, green: %i, blue %i\n",
			1 << Pcsi.red_prec, 1 << Pcsi.green_prec,
			1 << Pcsi.blue_prec);
		if (verbose && Pdepth <= 8)
		{
			if (Pvisual->class == DirectColor)
			{
				fprintf(stderr, "  Colormap:\n");
			}
			else
			{
				fprintf(stderr,
					"  Static Colormap used by fvwm:\n");
			}
			print_colormap(Pcmap);
		}
	}

	if (Pdepth <= 8 && verbose >= 2)
	{
		fprintf(stderr,"\n  Default Colormap:\n");
		print_colormap(DefaultColormap(Pdpy,DefaultScreen(Pdpy)));
	}
}
