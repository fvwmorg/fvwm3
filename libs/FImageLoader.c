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
 * The png loader and FImageRGBtoPixel are from imlib2. The code is from raster
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
#include "safemalloc.h"
#include "InitPicture.h"
#include "Picture.h"
#include "Fxpm.h"
#include "Fpng.h"
#include "FImageLoader.h"

/* ---------------------------- local definitions --------------------------- */
#define FIMAGE_CMD_ARGS Display *dpy, Window Root, char *path, \
                  int color_limit, \
		  Pixmap *pixmap, Pixmap *mask, \
		  int *width, int *height, int *depth, \
		  int *nalloc_pixels, Pixel *alloc_pixels

typedef struct FImageLoader
{
  char *extension;
#ifdef __STDC__
  int (*func)(FIMAGE_CMD_ARGS);
#else
  int (*func)();
#endif
} FImageLoader;

/* ---------------------------- local macros -------------------------------- */

#define FIMAGE_ALPHA_LIMIT 130

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* This structure is used to quickly access the RGB values of the colors */
/* without repeatedly having to transform them.   */
typedef struct
{
	char * c_color;	          /* Pointer to the name of the color */
	XColor rgb_space;         /* rgb color info */
	Bool allocated;           /* True if the pixel of the rgb_space 
				   * is allocated */
} Color_Info;

/* ---------------------------- forward declarations ------------------------ */

static Bool FImageLoadPng(FIMAGE_CMD_ARGS);
static Bool FImageLoadXpm(FIMAGE_CMD_ARGS);
static Bool FImageLoadBitmap(FIMAGE_CMD_ARGS);

/* ---------------------------- local variables ----------------------------- */

static char color_limit_base_table_init = 'n';

FImageLoader Loaders[] =
{
	{ "xpm", FImageLoadXpm },
	{ "png", FImageLoadPng },
	{ "bmp", FImageLoadBitmap },
	{NULL,0}
};

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

/* ---------------------------- local functions ----------------------------- */

/* ***************************************************************************
 *
 * xpm colors reduction
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
	
	if (!XpmSupport)
		return;

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
static
void c200_substitute_color(char **my_color, int color_limit)
{
	int i, limit, minind;
	double mindst=1e20;
	double dst;
	XColor rgb;          /* place to calc rgb for each color in xpm */

	
	if (!XpmSupport)
		return;

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

/* given an xpm, change colors to colors close to the subset above. */
static
void color_reduce_pixmap(FxpmImage *image,int color_limit)
{
	int i;
	FxpmColor *color_table_ptr;
	static char color_limit_base_table_init = 'n';

	if (!XpmSupport)
		return;

	if (color_limit > 0) {             /* If colors to be limited */
		if (color_limit_base_table_init == 'n') {
			/* if base table not created yet */
			c100_init_base_table();  /* init the base table */
			/* remember that its set now. */
			color_limit_base_table_init = 'y';
		}                          /* end base table init */
		color_table_ptr = image->colorTable; /* start of xpm color
						      * table */
		for(i=0; i<image->ncolors; i++) {/* all colors in the xpm */
			/* Theres an array for this in the xpm library, but it
			 * doesn't appear to be part of the API.  Too bad.
			 * dje 01/09/00 */
			char **visual_color = 0;
			if (color_table_ptr->c_color) {
				visual_color = &color_table_ptr->c_color;
			} else if (color_table_ptr->g_color) {
				visual_color = &color_table_ptr->g_color;
			} else if (color_table_ptr->g4_color) {
				visual_color = &color_table_ptr->g4_color;
			} else {              /* its got to be one of these */
				visual_color = &color_table_ptr->m_color;
			}
			c200_substitute_color(visual_color,color_limit);
			color_table_ptr +=1;  /* counter for loop */
		}                             /* end all colors in xpm */
	}                                     /* end colors limited */

	return;
}

static void FImageReduceRGBColor(XColor *c, int color_limit)
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
static CARD32 FImageRGBtoPixel(int r, int g, int b)
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
			((r      ) & 0x0000ff));
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
	return val;
}

/* ***************************************************************************
 *
 * png loader
 *
 * ***************************************************************************/
static
Bool FImageLoadPng(FIMAGE_CMD_ARGS)
{
	Fpng_uint_32 w32, h32;
	Fpng_structp Fpng_ptr = NULL;
	Fpng_infop Finfo_ptr = NULL;
	CARD32 *data;
	int w, h;
	char hasa = 0, hasg = 0;
	FILE *f;
	int bit_depth, color_type, interlace_type;
	unsigned char buf[FPNG_BYTES_TO_CHECK];
	unsigned char **lines;
	int i;

	if (!PngSupport)
		return False;

	if (!(f = fopen(path, "rb")))
	{
		return False;
	}

	fread(buf, 1, FPNG_BYTES_TO_CHECK, f);
	if (!Fpng_check_sig(buf, FPNG_BYTES_TO_CHECK))
	{
		fclose(f);
		return False;
	}
	rewind(f);
	Fpng_ptr = Fpng_create_read_struct(FPNG_LIBPNG_VER_STRING,
					   NULL, NULL, NULL);
	if (!Fpng_ptr)
	{
		fclose(f);
		return False;
	}
	Finfo_ptr = Fpng_create_info_struct(Fpng_ptr);
	if (!Finfo_ptr)
	{
		Fpng_destroy_read_struct(&Fpng_ptr, NULL, NULL);
		fclose(f);
		return False;
	}
#if 0
	if (setjmp(Fpng_ptr->jmpbuf))
	{
		Fpng_destroy_read_struct(&Fpng_ptr, &Finfo_ptr, NULL);
		fclose(f);
		return False;
	}
#endif
	Fpng_init_io(Fpng_ptr, f);
	Fpng_read_info(Fpng_ptr, Finfo_ptr);
	Fpng_get_IHDR(Fpng_ptr, Finfo_ptr, (Fpng_uint_32 *) (&w32),
		     (Fpng_uint_32 *) (&h32), &bit_depth, &color_type,
		     &interlace_type, NULL, NULL);
	interlace_type = 0; /* not used */
	*width = w = (int) w32;
	*height = h = (int) h32;
	*depth = bit_depth;
	if (color_type == FPNG_COLOR_TYPE_PALETTE)
	{
		Fpng_set_expand(Fpng_ptr);
	}
	if (Finfo_ptr->color_type == FPNG_COLOR_TYPE_RGB_ALPHA)
	{
		hasa = 1;
	}
	if (Finfo_ptr->color_type == FPNG_COLOR_TYPE_GRAY_ALPHA)
	{
		hasa = 1;
		hasg = 1;
	}
	if (Finfo_ptr->color_type == FPNG_COLOR_TYPE_GRAY)
	{
		hasg = 1;
	}
	if (hasa)
		Fpng_set_expand(Fpng_ptr);
	/* we want ARGB */
	/* note form raster:      
	* thanks to mustapha for helping debug this on PPC Linux remotely by
	* sending across screenshots all the tiem and me figuring out form them
	* what the hell was up with the colors
	* now png loading shoudl work on big endian machines nicely   */
#ifdef WORDS_BIGENDIAN
	Fpng_set_swap_alpha(Fpng_ptr);
	Fpng_set_filler(Fpng_ptr, 0xff, FPNG_FILLER_BEFORE);
#else
	Fpng_set_bgr(Fpng_ptr);
	Fpng_set_filler(Fpng_ptr, 0xff, FPNG_FILLER_AFTER);
#endif
	/* 16bit color -> 8bit color */
	Fpng_set_strip_16(Fpng_ptr);
	/* pack all pixels to byte boundaires */
	Fpng_set_packing(Fpng_ptr);
	if (Fpng_get_valid(Fpng_ptr, Finfo_ptr, FPNG_INFO_tRNS))
		Fpng_set_expand(Fpng_ptr);

	data = (CARD32 *)safemalloc(w * h * sizeof(CARD32));
	lines = (unsigned char **) alloca(h * sizeof(unsigned char *));

	if (hasg)
	{
		Fpng_set_gray_to_rgb(Fpng_ptr);
		if (Fpng_get_bit_depth(Fpng_ptr, Finfo_ptr) < 8)
		{
			Fpng_set_gray_1_2_4_to_8(Fpng_ptr);
		}
	}
	for (i = 0; i < h; i++)
	{
		lines[i] =((unsigned char *) (data)) + (i * w * sizeof(CARD32));
	}
	Fpng_read_image(Fpng_ptr, lines);
	Fpng_read_end(Fpng_ptr, Finfo_ptr);
	Fpng_destroy_read_struct(&Fpng_ptr, &Finfo_ptr, (png_infopp) NULL);
	fclose(f);
	
	*depth = Pdepth;
	*pixmap = XCreatePixmap(dpy, Root, w, h, Pdepth);
	*mask = XCreatePixmap(dpy, Root, w, h, 1);
	if (!FImageCreatePixmapFromArgbData(dpy, Root, color_limit,
					    (unsigned char *)data,
					    0, w, h,
					    *pixmap, *mask)
	    || *pixmap == None)
	{
		if (*pixmap != None)
			XFreePixmap(dpy, *pixmap);
		if (*mask != None)
			XFreePixmap(dpy, *mask);
		free(data);
		return False;
	}
	free(data);
	return True;
}

/* ***************************************************************************
 *
 * xpm loader
 *
 * ***************************************************************************/
static
Bool FImageLoadXpm(FIMAGE_CMD_ARGS)
{
	FxpmAttributes xpm_attributes;
	int rc;
	FxpmImage	my_image = {0};
#ifdef HAVE_SIGACTION
	struct sigaction defaultHandler;
	struct sigaction originalHandler;
#else
	void (*originalHandler)(int);
#endif

	if (!XpmSupport)
		return False;

	xpm_attributes.visual = Pvisual;
	xpm_attributes.colormap = Pcmap;
	xpm_attributes.depth = Pdepth;
	xpm_attributes.closeness=40000; /* Allow for "similar" colors */
	xpm_attributes.valuemask = FxpmSize | FxpmCloseness | FxpmVisual |
		FxpmColormap | FxpmDepth;
	if (*nalloc_pixels == 0)
	{
		xpm_attributes.valuemask |= FxpmReturnAllocPixels;
	}
#ifdef HAVE_SIGACTION
	sigemptyset(&defaultHandler.sa_mask);
	defaultHandler.sa_flags = 0;
	defaultHandler.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &defaultHandler, &originalHandler);
#else
	originalHandler = signal(SIGCHLD, SIG_DFL);
#endif

	rc = FxpmReadFileToXpmImage(path, &my_image, NULL);

#ifdef HAVE_SIGACTION
	sigaction(SIGCHLD, &originalHandler, NULL);
#else
	signal(SIGCHLD, originalHandler);
#endif

	if (rc == FxpmSuccess)
	{
		color_reduce_pixmap(&my_image, color_limit);
		rc = FxpmCreatePixmapFromXpmImage(
			dpy, Root, &my_image, pixmap, mask,
			&xpm_attributes);
		if (rc == FxpmSuccess)
		{
			*width = my_image.width;
			*height = my_image.height;
			*depth = Pdepth;
			FxpmFreeXpmImage(&my_image);
			alloc_pixels = xpm_attributes.alloc_pixels;
			*nalloc_pixels = xpm_attributes.nalloc_pixels;
			return True;
		}
		FxpmFreeXpmImage(&my_image);
	}
	return False;
}

/* ***************************************************************************
 *
 * bitmap loader
 *
 * ***************************************************************************/
static
Bool FImageLoadBitmap(FIMAGE_CMD_ARGS)
{
	int l;

	if (XReadBitmapFile(dpy, Root, path, width, height, pixmap, &l, &l)
	    == BitmapSuccess)
	{
		*mask = None;
		*depth = 1;
		*nalloc_pixels = 0;
		alloc_pixels = NULL;
		return True;
	}
	return False;
}

/* ---------------------------- interface functions ------------------------- */

/* ***************************************************************************
 *
 * argb data to pixmaps
 *
 * ***************************************************************************/
Bool FImageCreatePixmapFromArgbData(Display *dpy, Window Root, int color_limit,
				    unsigned char *data,
				    int start, int width, int height,
				    Pixmap pixmap, Pixmap mask)
{
	static GC my_gc = None;
	GC mono_gc = None;
	XGCValues xgcv;
	register int i,j,k;
	XImage *image, *m_image;
	XColor c;
	Pixel back = WhitePixel(dpy, DefaultScreen(dpy));
	Pixel fore = BlackPixel(dpy, DefaultScreen(dpy));
	int a,r,g,b;
	
	if (my_gc == None)
	{
		my_gc = fvwmlib_XCreateGC(dpy, Root, 0, NULL);
	}
	xgcv.foreground = fore;
	xgcv.background = back;
	mono_gc = fvwmlib_XCreateGC(dpy, mask,
				    GCForeground | GCBackground, &xgcv);

	/* create an XImage structure */
	image = XCreateImage(dpy, Pvisual, Pdepth, ZPixmap, 0, 0, width, height,
			     Pdepth > 16 ? 32 : (Pdepth > 8 ? 16 : 8), 0);
	if (!image)
	{
		fprintf(stderr, "[FVWM][FImageCreatePixmapFromArgbData] "
			"-- WARN cannot create an XImage\n");
		return False;
	}
	m_image = XCreateImage(dpy, Pvisual, 1, ZPixmap, 0, 0, width, height,
			       Pdepth > 16 ? 32 : (Pdepth > 8 ? 16 : 8), 0);

	/* create space for drawing the image locally */
	image->data = safemalloc(image->bytes_per_line * height);
	m_image->data = safemalloc(m_image->bytes_per_line * height);

	k = 4*start;
	c.flags = DoRed | DoGreen | DoBlue;
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			b = data[k++];
			g = data[k++];
			r = data[k++];
			a = data[k++];
			if ((a >= FIMAGE_ALPHA_LIMIT) && color_limit <= 0 &&
			    (Pvisual->class == DirectColor ||
			     Pvisual->class == TrueColor))
			{
				c.pixel = FImageRGBtoPixel(r,g,b);
			}
			else if (a >= FIMAGE_ALPHA_LIMIT)
			{
				c.blue = b * 257;
				c.green = g * 257;
				c.red = r * 257;
				FImageReduceRGBColor(&c,color_limit);
			}
			else
			{
				c.pixel = 0;
			}
			XPutPixel(image, i, j, c.pixel);
			XPutPixel(m_image, i, j,
				  (a >= FIMAGE_ALPHA_LIMIT)? back:fore);
		}
	}
	/* copy the image to the server */
	XPutImage(dpy, pixmap, my_gc, image, 0, 0, 0, 0, width, height);
	XPutImage(dpy, mask, mono_gc, m_image, 0, 0, 0, 0, width, height);
	XDestroyImage(image);
	if (m_image != None)
		XDestroyImage(m_image);
	return True;
}


/* ***************************************************************************
 *
 * the images loaders
 *
 * ***************************************************************************/

Bool FImageLoadPixmapFromFile(Display *dpy, Window Root, char *path,
			      int color_limit,
			      Pixmap *pixmap, Pixmap *mask,
			      int *width, int *height, int *depth,
			      int *nalloc_pixels, Pixel *alloc_pixels)
{
	int done = 0, i = 0, tried = -1;
	char *ext = NULL;

	if (path == NULL)
		return False;

	if (strlen(path) > 3)
	{
		ext = path + strlen(path) - 3;
	}
	/* first try to load by extension */
	while(!done && ext != NULL && Loaders[i].extension != NULL)
	{
		if (StrEquals(Loaders[i].extension, ext))
		{
			if (Loaders[i].func(dpy, Root, path, color_limit,
					    pixmap, mask,
					    width, height, depth,
					    nalloc_pixels, alloc_pixels))
			{
				return True;
			}
			tried = i;
			done = 1;
		}
		i++;
	}

	i = 0;
	while(Loaders[i].extension != NULL)
	{
		if (i != tried && Loaders[i].func(dpy, Root, path, color_limit,
						  pixmap, mask,
						  width, height, depth,
						  nalloc_pixels, alloc_pixels))
		{
			return True;
		}
		i++;
	}
	pixmap = None;
	mask = None;
	*width = *height = *depth = *nalloc_pixels = 0;
	alloc_pixels = NULL;
	return False;
}

Picture *FImageLoadPictureFromFile(Display *dpy, Window Root, char *path,
				   int color_limit)
{
	Picture *p;
	Pixmap pixmap = None;
	Pixmap mask = None;
	int width = 0, height = 0, depth = 0;
	int nalloc_pixels = 0;
	Pixel *alloc_pixels = NULL;

	if (!FImageLoadPixmapFromFile(dpy, Root, path, color_limit,
				       &pixmap, &mask,
				       &width, &height, &depth,
				       &nalloc_pixels, alloc_pixels))
	{
		return NULL;
	}
 
	p = (Picture*)safemalloc(sizeof(Picture));
	memset(p, 0, sizeof(Picture));
	p->count = 1;
	p->name = path;
	p->next = NULL;
	setFileStamp(&p->stamp, p->name);
	p->picture = pixmap;
	p->mask = mask;
	p->width = width;
	p->height = height;
	p->depth = depth;
	p->nalloc_pixels = nalloc_pixels;
	p->alloc_pixels = alloc_pixels;

	return p;
}

Bool FImageLoadCursorPixmapFromFile(Display *dpy, Window Root,
				    char *path, Pixmap *source, Pixmap *mask,
				    unsigned int *x,
				    unsigned int *y)
{
	
	FxpmAttributes xpm_attributes;

	if (!XpmSupport)
		return False;

	/* we need source to be a bitmap */
	xpm_attributes.depth = 1;
	xpm_attributes.valuemask = FxpmSize | FxpmDepth | FxpmHotspot;
	if (FxpmReadFileToPixmap(dpy, Root, path, source, mask,
				&xpm_attributes) != FxpmSuccess)
	{
		fprintf(stderr, "[FVWM][FImageLoadCursorPixmapFromFile]"
			" Error reading cursor xpm %s",
			path);
		return False;
	}

	*x = xpm_attributes.x_hotspot;
	if (*x >= xpm_attributes.width)
	{
		*x = xpm_attributes.width / 2;
	}
	*y = xpm_attributes.y_hotspot;
	if (*y >= xpm_attributes.height)
	{
		*y = xpm_attributes.height / 2;
	}
	return True;
}

Bool FImageLoadPixmapFromXpmData(Display *dpy, Window Root, int color_limit,
				 char **data,
				 Pixmap *pixmap, Pixmap *mask,
				 int *width, int *height, int *depth)
{
	FxpmAttributes xpm_attributes;

	xpm_attributes.valuemask = FxpmReturnPixels | FxpmCloseness |
		FxpmExtensions | FxpmVisual | FxpmColormap | FxpmDepth;
	xpm_attributes.closeness = 40000 /* Allow for "similar" colors */;
	xpm_attributes.visual = Pvisual;
	xpm_attributes.colormap = Pcmap;
	xpm_attributes.depth = Pdepth;
	if(FxpmCreatePixmapFromData(dpy, Root, data, pixmap, mask, 
				   &xpm_attributes)!=FxpmSuccess)
	{
		return False;
	}
	*width = xpm_attributes.width;
	*height = xpm_attributes.height;
	*depth = Pdepth;
	return True;
}
