/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

/* ---------------------------- included header files ----------------------- */

#include <config.h>

#include <stdio.h>

#include <X11/Xlib.h>
#include "safemalloc.h"
#include "Strings.h"
#include "Flocale.h"
#include "Fft.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

#define FFT_SET_TOP_TO_BOTTOM_MATRIX(m) ((m)->xx = (m)->yy = 0, \
				 (m)->xy = 1,  (m)->yx = -1)

#define FFT_SET_BOTTOM_TO_TOP_MATRIX(m) ((m)->xx = (m)->yy = 0, \
				 (m)->xy = -1, (m)->yx = 1)

/* ---------------------------- imports ------------------------------------- */

/* we cannot include Picture.h */
extern Colormap Pcmap;
extern Visual *Pvisual;

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static Display *fftdpy = NULL;
static int fftscreen;
static int fft_initialized = False;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static
void init_fft(Display *dpy)
{
	fftdpy = dpy;
	fftscreen = DefaultScreen(dpy);
	fft_initialized = True;
}

static
Bool is_utf8_encoding(FftFont *f)
{
	int i = 0;
	FftPatternElt *e;

	if (!FftUtf8Support)
	{
		return False;
	}

	while(i < f->pattern->num)
	{
		e = &f->pattern->elts[i];
		if (StrEquals(e->object, FFT_ENCODING))
		{
			return (StrEquals(e->values->value.u.s,
					  "iso10646-1")) ? True : False;
		}
		i++;
	}

	return False;
}

static
FftFont *FftGetRotatedFont(
		Display *dpy, FftFont *f, text_direction_type text_dir)
{
	FftPattern *vert_pat;
	FftMatrix m;
	int dummy;

	if (f == NULL)
		return NULL;

	vert_pat = FftPatternDuplicate (f->pattern);
	if (vert_pat == NULL)
		return NULL;

	dummy = FftPatternDel(vert_pat, XFT_MATRIX);

	if (text_dir == TEXT_DIR_TOP_TO_BOTTOM)
	{
		FFT_SET_TOP_TO_BOTTOM_MATRIX(&m);
	}
	else if (text_dir == TEXT_DIR_BOTTOM_TO_TOP)
	{
		FFT_SET_BOTTOM_TO_TOP_MATRIX(&m);  
	}
	else
	{
		return NULL;
	}

	if (!FftPatternAddMatrix (vert_pat, FFT_MATRIX, &m))
		return NULL;
	return FftFontOpenPattern (dpy, vert_pat);
}

/* ---------------------------- interface functions ------------------------- */

void FftGetFontHeights(
	FftFontType *fftf, int *height, int *ascent, int *descent)
{
	/* fft font height may be > fftfont->ascent + fftfont->descent, this
	 * depends on the minspace value */
	*height = fftf->fftfont->height;
	*ascent = fftf->fftfont->ascent;
	*descent = fftf->fftfont->descent;

	return;
}

void FftGetFontWidths(
	FftFontType *fftf, int *max_char_width, int *min_char_offset)
{
	FGlyphInfo extents;

	/* FIXME */
	if (FftUtf8Support && fftf->utf8)
	{
		FftTextExtentsUtf8(fftdpy, fftf->fftfont, "W", 1, &extents);
	}
	else
	{
		FftTextExtents8(fftdpy, fftf->fftfont, "W", 1, &extents);
	}
	*max_char_width = extents.xOff;
	/* FIXME: how do you calculate this? */
	*min_char_offset = 0;

	return;
}

FftFontType *FftGetFont(Display *dpy, char *fontname)
{
	FftFont *fftfont = NULL;
	FftFontType *fftf = NULL;

	if (!FftSupport)
	{
		return NULL;
	}
	if (!fontname)
	{
		return NULL;
	}
	if (!fft_initialized)
	{
		init_fft(dpy);
	}
	fftfont = FftFontOpenName(dpy, fftscreen, fontname);
	if (!fftfont)
	{
		return NULL;
	}
	fftf = (FftFontType *)safemalloc(sizeof(FftFontType));
	fftf->fftfont = fftfont;
	fftf->tb_fftfont = NULL;
	fftf->bt_fftfont = NULL;
	fftf->utf8 = is_utf8_encoding(fftfont);

	return fftf;
}

void FftDrawString(
		   Display *dpy, Window win, FftFontType *fftf, GC gc, int x,
		   int y, char *str, int len, int direction)
{
	FftDraw *fftdraw = NULL;
	XGCValues vr;
	XColor xfg;
	FftColor fft_fg;
	FftFont *uf;

	if (!FftSupport)
	{
		return;
	}
	if (direction == TEXT_DIR_TOP_TO_BOTTOM)
	{
		if (fftf->tb_fftfont == NULL)
		{
			fftf->tb_fftfont =
				FftGetRotatedFont(dpy, fftf->fftfont, direction);
		}
		uf = fftf->tb_fftfont;
	}
	else if (direction == TEXT_DIR_BOTTOM_TO_TOP)
	{
		if (fftf->bt_fftfont == NULL)
		{
			fftf->bt_fftfont =
				FftGetRotatedFont(dpy, fftf->fftfont, direction);
		}
		uf = fftf->bt_fftfont;
	}
	else
	{
		uf = fftf->fftfont;
	}

	if (uf == NULL)
		return;

	fftdraw = FftDrawCreate(dpy, (Drawable)win, Pvisual, Pcmap);
	if (XGetGCValues(dpy, gc, GCForeground, &vr))
	{
		xfg.pixel = vr.foreground;
	}
	else
	{
		fprintf(stderr, "[fvwmlibs][FftDrawString]: ERROR --"
			" cannot found color\n");
		xfg.pixel = WhitePixel(dpy, fftscreen);
	}

	XQueryColor(dpy, Pcmap, &xfg);
	fft_fg.color.red = xfg.red;
	fft_fg.color.green = xfg.green;
	fft_fg.color.blue = xfg.blue;
	fft_fg.color.alpha = 0xffff;
	fft_fg.pixel = xfg.pixel;

	if (FftUtf8Support && fftf->utf8)
	{
		FftDrawStringUtf8(
			fftdraw, &fft_fg, uf, x, y, (unsigned char *) str, len);
	}
	else
	{
		FftDrawString8(
			fftdraw, &fft_fg, uf, x, y, (unsigned char *)str, len);
	}
	FftDrawDestroy (fftdraw);
}

int FftTextWidth(FftFontType *fftf, char *str, int len)
{
	FGlyphInfo extents;

	if (!FftSupport)
	{
		return 0;
	}
	if (FftUtf8Support && fftf->utf8)
	{
		FftTextExtentsUtf8(fftdpy, fftf->fftfont, str, len, &extents);
	}
	else
	{
		FftTextExtents8(fftdpy, fftf->fftfont, str, len, &extents);
	}

	return extents.xOff;
}
