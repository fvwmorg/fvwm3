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
 * The png loader and PImageRGBtoPixel are from imlib2. The code is from raster
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
#include "Picture.h"
#include "PictureUtils.h"
#include "Fxpm.h"
#include "Fpng.h"
#include "FRenderInit.h"

/* ---------------------------- local definitions --------------------------- */
#define FIMAGE_CMD_ARGS Display *dpy, Window Root, char *path, \
		  int color_limit, \
		  Pixmap *pixmap, Pixmap *mask, Pixmap *alpha, \
		  int *width, int *height, int *depth, \
		  int *nalloc_pixels, Pixel **alloc_pixels, \
		  FvwmPictureFlags fpf

typedef struct PImageLoader
{
  char *extension;
#ifdef __STDC__
  int (*func)(FIMAGE_CMD_ARGS);
#else
  int (*func)();
#endif
} PImageLoader;

/* ---------------------------- local macros -------------------------------- */

#if XRenderSupport
#define FIMAGE_ALPHA_LIMIT 0
#else
#define FIMAGE_ALPHA_LIMIT 130
#endif

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

static Bool PImageLoadPng(FIMAGE_CMD_ARGS);
static Bool PImageLoadXpm(FIMAGE_CMD_ARGS);
static Bool PImageLoadBitmap(FIMAGE_CMD_ARGS);

/* ---------------------------- local variables ----------------------------- */

PImageLoader Loaders[] =
{
	{ "xpm", PImageLoadXpm },
	{ "png", PImageLoadPng },
	{ "bmp", PImageLoadBitmap },
	{NULL,0}
};

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

/* ***************************************************************************
 *
 * xpm colors reduction
 *
 * ***************************************************************************/

#ifdef USE_OLD_COLOR_LIMIT_METHODE
/* given an xpm, change colors to colors close to the subset above. */
static
void color_reduce_xpm(FxpmImage *image,int color_limit)
{
	int i;
	FxpmColor *color_table_ptr;

	if (!XpmSupport)
		return;

	if (color_limit > 0) {             /* If colors to be limited */
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
			PictureReduceColor(visual_color,color_limit);
			color_table_ptr +=1;  /* counter for loop */
		}                             /* end all colors in xpm */
	}                                     /* end colors limited */

	return;
}
#endif

/* ***************************************************************************
 *
 * png loader
 *
 * ***************************************************************************/
static
Bool PImageLoadPng(FIMAGE_CMD_ARGS)
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
	int have_alpha;

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
	if (XRenderSupport && fpf.alpha && FRenderGetExtensionSupported())
	{
		*alpha = XCreatePixmap(dpy, Root, w, h, 8);
	}
	if (!PImageCreatePixmapFromArgbData(dpy, Root, color_limit,
					    (unsigned char *)data,
					    0, w, h,
					    *pixmap, *mask, *alpha, &have_alpha)
	    || *pixmap == None)
	{
		if (*pixmap != None)
			XFreePixmap(dpy, *pixmap);
		if (*alpha != None)
		{
			XFreePixmap(dpy, *alpha);
		}
		if (*mask != None)
			XFreePixmap(dpy, *mask);
		free(data);
		return False;
	}
	if (!have_alpha && *alpha != None)
	{
		XFreePixmap(dpy, *alpha);
		*alpha = None;
	}
	free(data);
	return True;
}

/* ***************************************************************************
 *
 * xpm loader
 *
 * ***************************************************************************/
int PImageXpmAllocColor(
	Display *dpy, Colormap colormap, char *colorname, XColor *color,
	void *closure);
int PImageXpmFreeColor(
	Display *dpy, Colormap colormap, Pixel *pixel, int n, void *closure);

int PImageXpmAllocColor(
	Display *dpy, Colormap colormap, char *colorname, XColor *xcolor,
	void *closure)
{
	if (colorname)
	{
		if (!XParseColor(dpy, colormap, colorname, xcolor))
			return -1;
	}
	return PictureAllocColor(dpy, colormap, xcolor, False);
}

int PImageXpmFreeColor(
	Display *dpy, Colormap colormap, Pixel *pixel, int n, void *closure)
{
	PictureFreeColors(dpy, colormap, pixel, n, 0);
	return 1;
}

static
Bool PImageLoadXpm(FIMAGE_CMD_ARGS)
{
	FxpmAttributes xpm_attributes;
	int rc;
	FxpmImage       my_image = {0};
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
#ifndef USE_OLD_COLOR_LIMIT_METHODE
	xpm_attributes.valuemask |= FxpmAllocColor | FxpmFreeColors |
		FxpmColorClosure;
	xpm_attributes.alloc_color = PImageXpmAllocColor;
	xpm_attributes.free_colors = PImageXpmFreeColor;
	xpm_attributes.color_closure = NULL;
	xpm_attributes.closeness = 0;
#else
	if (fpf.alloc_pixels)
	{
		xpm_attributes.valuemask |= FxpmReturnAllocPixels;
	}
#endif
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
#ifdef USE_OLD_COLOR_LIMIT_METHODE
		color_reduce_xpm(&my_image, color_limit);
#endif
		rc = FxpmCreatePixmapFromXpmImage(
			dpy, Root, &my_image, pixmap, mask,
			&xpm_attributes);
		if (rc == FxpmSuccess)
		{
			*width = my_image.width;
			*height = my_image.height;
			*depth = Pdepth;
			FxpmFreeXpmImage(&my_image);
			if (fpf.alloc_pixels && nalloc_pixels != NULL
#ifndef USE_OLD_COLOR_LIMIT_METHODE
			    && 0 
#endif
			   )
			{
				*alloc_pixels = xpm_attributes.alloc_pixels;
				*nalloc_pixels = xpm_attributes.nalloc_pixels;
			}
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
Bool PImageLoadBitmap(FIMAGE_CMD_ARGS)
{
	int l;

	if (XReadBitmapFile(dpy, Root, path, width, height, pixmap, &l, &l)
	    == BitmapSuccess)
	{
		*mask = None;
		*depth = 1;
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
Bool PImageCreatePixmapFromArgbData(Display *dpy, Window Root, int color_limit,
				    unsigned char *data,
				    int start, int width, int height,
				    Pixmap pixmap, Pixmap mask,
				    Pixmap alpha, int *have_alpha)
{
	static GC my_gc = None;
	GC mono_gc = None;
	GC a_gc = None;
	XGCValues xgcv;
	register int i,j,k;
	XImage *image, *m_image = NULL;
	XImage *a_image = NULL;
	XColor c;
	Pixel back = WhitePixel(dpy, DefaultScreen(dpy));
	Pixel fore = BlackPixel(dpy, DefaultScreen(dpy));
	int a,r,g,b;
	Bool use_alpha_pix = (XRenderSupport && alpha != None &&
			      FRenderGetExtensionSupported());

	*have_alpha = False;
	if (my_gc == None)
	{
		my_gc = fvwmlib_XCreateGC(dpy,
					  RootWindow(dpy, DefaultScreen(dpy)),
					  0, NULL);
		if (my_gc == None)
			return False;
	}
	if (use_alpha_pix)
	{
		a_gc = fvwmlib_XCreateGC(dpy, alpha, 0, NULL);
	}
	if (mask)
	{
		xgcv.foreground = fore;
		xgcv.background = back;
		mono_gc = fvwmlib_XCreateGC(dpy, mask,
					    GCForeground | GCBackground, &xgcv);
	}

	/* create an XImage structure */
	image = XCreateImage(
		dpy, Pvisual, Pdepth, ZPixmap, 0, 0, width, height,
		Pdepth > 16 ? 32 : (Pdepth > 8 ? 16 : 8), 0);
	if (!image)
	{
		if (mono_gc != None)
			XFreeGC(dpy, mono_gc);
		if (a_gc != None)
			XFreeGC(dpy, a_gc);
		fprintf(stderr, "[FVWM][PImageCreatePixmapFromArgbData] "
			"-- WARN cannot create an XImage\n");
		return False;
	}
	if (mask)
	{
		m_image = XCreateImage(
			dpy, Pvisual, 1, ZPixmap, 0, 0, width, height,
			Pdepth > 16 ? 32 : (Pdepth > 8 ? 16 : 8), 0);
	}
	/* create space for drawing the image locally */
	image->data = safemalloc(image->bytes_per_line * height);
	if (m_image)
	{
		m_image->data = safemalloc(m_image->bytes_per_line * height);
	}
	if (use_alpha_pix)
	{
		a_image = XCreateImage(
			dpy, Pvisual, 8, ZPixmap, 0, 0, width, height,
			Pdepth > 16 ? 32 : (Pdepth > 8 ? 16 : 8), 0);
		a_image->data = safemalloc(a_image->bytes_per_line * height);
	}

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

			if ((a > FIMAGE_ALPHA_LIMIT) &&
			    color_limit <= 0 &&
			    (Pvisual->class == DirectColor ||
			     Pvisual->class == TrueColor))
			{
				c.pixel = PictureRGBtoPixel(r,g,b);
			}
			else if (a > FIMAGE_ALPHA_LIMIT)
			{
				c.blue = b * 257;
				c.green = g * 257;
				c.red = r * 257;
				PictureReduceRGBColor(&c,color_limit);
			}
			else
			{
				c.pixel = 0;
			}
			XPutPixel(image, i, j, c.pixel);
			if (m_image)
			{
				XPutPixel(
					m_image, i, j,
					(a > FIMAGE_ALPHA_LIMIT)? back:fore);
			}
			if (use_alpha_pix && a_image)
			{
				XPutPixel(a_image, i, j, a);
				*have_alpha |= (a < 255 && a > 0);
			}
		}
	}
	/* copy the image to the server */
	XPutImage(dpy, pixmap, my_gc, image, 0, 0, 0, 0, width, height);
	if (m_image)
	{
		XPutImage(
			dpy, mask, mono_gc, m_image, 0, 0, 0, 0, width, height);
	}
	if (*have_alpha)
	{
		XPutImage(dpy, alpha, a_gc, a_image, 0, 0, 0, 0, width, height);
	}
	XDestroyImage(image);
	if (m_image)
		XDestroyImage(m_image);
	if (use_alpha_pix && a_image)
		XDestroyImage(a_image);
	if (mono_gc != None)
		XFreeGC(dpy, mono_gc);
	if (a_gc != None)
		XFreeGC(dpy, a_gc);
	return True;
}


/* ***************************************************************************
 *
 * the images loaders
 *
 * ***************************************************************************/

Bool PImageLoadPixmapFromFile(Display *dpy, Window Root, char *path,
			      int color_limit,
			      Pixmap *pixmap, Pixmap *mask, Pixmap *alpha,
			      int *width, int *height, int *depth,
			      int *nalloc_pixels, Pixel **alloc_pixels,
			      FvwmPictureFlags fpf)
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
					    pixmap, mask, alpha,
					    width, height, depth,
					    nalloc_pixels, alloc_pixels, fpf))
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
						  pixmap, mask, alpha,
						  width, height, depth,
						  nalloc_pixels, alloc_pixels,
						  fpf))
		{
			return True;
		}
		i++;
	}
	pixmap = None;
	mask = None;
	alpha = None;
	*width = *height = *depth = 0;
	if (nalloc_pixels != NULL)
	{
		*nalloc_pixels = 0;
	}
	alloc_pixels = NULL;
	return False;
}

FvwmPicture *PImageLoadFvwmPictureFromFile(Display *dpy, Window Root, char *path,
					   int color_limit)
{
	FvwmPicture *p;
	Pixmap pixmap = None;
	Pixmap mask = None;
	Pixmap alpha = None;
	int width = 0, height = 0, depth = 0;
	int nalloc_pixels = 0;
	Pixel *alloc_pixels = NULL;
	FvwmPictureFlags fpf;

	fpf.alloc_pixels = 1;
	fpf.alpha = 1;
	if (!PImageLoadPixmapFromFile(dpy, Root, path, color_limit,
				      &pixmap, &mask, &alpha,
				      &width, &height, &depth,
				      &nalloc_pixels, &alloc_pixels, fpf))
	{
		return NULL;
	}

	p = (FvwmPicture*)safemalloc(sizeof(FvwmPicture));
	memset(p, 0, sizeof(FvwmPicture));
	p->count = 1;
	p->name = path;
	p->next = NULL;
	setFileStamp(&p->stamp, p->name);
	p->picture = pixmap;
	p->mask = mask;
	p->alpha = alpha;
	p->width = width;
	p->height = height;
	p->depth = depth;
	p->nalloc_pixels = nalloc_pixels;
	p->alloc_pixels = alloc_pixels;

	return p;
}

Bool PImageLoadCursorPixmapFromFile(Display *dpy, Window Root,
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
		fprintf(stderr, "[FVWM][PImageLoadCursorPixmapFromFile]"
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

Bool PImageLoadPixmapFromXpmData(Display *dpy, Window Root, int color_limit,
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
