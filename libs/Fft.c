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

#include "fvwmlib.h"
#include "safemalloc.h"
#include "Strings.h"
#include "Flocale.h"
#include "Fft.h"
#include "FftInterface.h"
#include "PictureBase.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

#define FFT_SET_ROTATED_90_MATRIX(m) \
	((m)->xx = (m)->yy = 0, (m)->xy = 1, (m)->yx = -1)

#define FFT_SET_ROTATED_270_MATRIX(m) \
	((m)->xx = (m)->yy = 0, (m)->xy = -1, (m)->yx = 1)

#define FFT_SET_ROTATED_180_MATRIX(m) \
	((m)->xx = (m)->yy = -1, (m)->xy = (m)->yx = 0)

/* ---------------------------- imports ------------------------------------- */

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
FftChar16 *FftUtf8ToFftString16(unsigned char *str, int len, int *nl)
{
	FftChar16 *new;
	int i = 0, j= 0;

	new = (FftChar16 *)safemalloc((len+1)*sizeof(FftChar16));
	while(i < len && str[i] != 0)
	{
		if (str[i] <= 0x7f)
		{
			new[j] = str[i];
		}
		else if (str[i] <= 0xdf && i+1 < len)
		{
		    new[j] = ((str[i] & 0x1f) << 6) + (str[i+1] & 0x3f);
		    i++;
		}
		else if (i+2 < len)
		{
			new[j] = ((str[i] & 0x0f) << 12) +
				((str[i+1] & 0x3f) << 6) +
				(str[i+2] & 0x3f);
			i += 2;
		}
		i++; j++;
	}
	*nl = j;
	return new;
}

static
void FftSetupEncoding(FftFontType *fftf)
{
	int i = 0;
	FftPatternElt *e;
	FftFont *f;

	if (fftf == NULL)
		return;
	
	f =  fftf->fftfont;
	fftf->utf8 = False;
	fftf->encoding = NULL;

	while(i < f->pattern->num)
	{
		e = &f->pattern->elts[i];
		if (StrEquals(e->object, FFT_ENCODING) &&
		    e->values->value.u.s != NULL)
		{
			fftf->encoding = e->values->value.u.s;
			if (StrEquals(fftf->encoding, "ISO10646-1"))
			{
				fftf->utf8 = True;
			}
			return;
		}
		i++;
	}
}

static
FftFont *FftGetRotatedFont(
	Display *dpy, FftFont *f, text_rotation_type text_rotation)
{
	FftPattern *rotated_pat;
	FftMatrix m;
	int dummy;

	if (f == NULL)
		return NULL;

	rotated_pat = FftPatternDuplicate(f->pattern);
	if (rotated_pat == NULL)
		return NULL;

	dummy = FftPatternDel(rotated_pat, XFT_MATRIX);

	if (text_rotation == TEXT_ROTATED_90)
	{
		FFT_SET_ROTATED_90_MATRIX(&m);
	}
	else if (text_rotation == TEXT_ROTATED_180)
	{
		FFT_SET_ROTATED_180_MATRIX(&m);
	}
	else if (text_rotation == TEXT_ROTATED_270)
	{
		FFT_SET_ROTATED_270_MATRIX(&m);
	}
	else
	{
		return NULL;
	}

	if (!FftPatternAddMatrix(rotated_pat, FFT_MATRIX, &m))
		return NULL;
	return FftFontOpenPattern(dpy, rotated_pat);
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
	FftFontType *fftf, int *max_char_width)
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
	fftf->fftfont_rotated_90 = NULL;
	fftf->fftfont_rotated_180 = NULL;
	fftf->fftfont_rotated_270 = NULL;
	FftSetupEncoding(fftf);

	return fftf;
}

void FftDrawString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws,
	Pixel fg, Pixel fgsh, Bool has_fg_pixels, int len, unsigned long flags)
{
	FftDraw *fftdraw = NULL;
	XGCValues vr;
	XColor xfg, xfgsh;
	FftColor fft_fg, fft_fgsh;
	FftFontType *fftf;
	FftFont *uf;
	int x,y,xsh_sign,ysh_sign,i;

	if (!FftSupport)
	{
		return;
	}

	fftf = &flf->fftf;
	if (fws->flags.text_rotation == TEXT_ROTATED_90)
	{
		if (fftf->fftfont_rotated_90 == NULL)
		{
			fftf->fftfont_rotated_90 =
				FftGetRotatedFont(dpy, fftf->fftfont,
						  fws->flags.text_rotation);
		}
		uf = fftf->fftfont_rotated_90;
		y = fws->y;
		x = fws->x;
		xsh_sign = -1;
		ysh_sign = 1;
	}
	else if (fws->flags.text_rotation == TEXT_ROTATED_180)
	{
		if (fftf->fftfont_rotated_180 == NULL)
		{
			fftf->fftfont_rotated_180 =
				FftGetRotatedFont(dpy, fftf->fftfont,
						  fws->flags.text_rotation);
		}
		uf = fftf->fftfont_rotated_180;
		y = fws->y;
		x = fws->x + FftTextWidth(&flf->fftf, fws->str, len);
		xsh_sign = 1;
		ysh_sign = 1;
	}
	else if (fws->flags.text_rotation == TEXT_ROTATED_270)
	{
		if (fftf->fftfont_rotated_270 == NULL)
		{
			fftf->fftfont_rotated_270 =
				FftGetRotatedFont(dpy, fftf->fftfont,
						  fws->flags.text_rotation);
		}
		uf = fftf->fftfont_rotated_270;
		y = fws->y + FftTextWidth(&flf->fftf, fws->str, len)
			+ flf->shadow_size;
		x = fws->x;
		xsh_sign = 1;
		ysh_sign = -1;
	}
	else
	{
		uf = fftf->fftfont;
		y = fws->y;
		x = fws->x;
		xsh_sign = 1;
		ysh_sign = 1;
	}

	if (uf == NULL)
		return;

	fftdraw = FftDrawCreate(dpy, (Drawable)fws->win, Pvisual, Pcmap);
	if (has_fg_pixels)
	{
		xfg.pixel = fg;
		xfgsh.pixel = fgsh;
	}
	else if (fws->gc &&
		 XGetGCValues(dpy, fws->gc, GCForeground, &vr))
	{
		xfg.pixel = vr.foreground;
	}
	else
	{
#if 0
		fprintf(stderr, "[fvwmlibs][FftDrawString]: ERROR --"
			" cannot found color\n");
#endif
		xfg.pixel = WhitePixel(dpy, fftscreen);
	}

	XQueryColor(dpy, Pcmap, &xfg);
	fft_fg.color.red = xfg.red;
	fft_fg.color.green = xfg.green;
	fft_fg.color.blue = xfg.blue;
	fft_fg.color.alpha = 0xffff;
	fft_fg.pixel = xfg.pixel;
	if (flf->shadow_size > 0 && has_fg_pixels)
	{
		XQueryColor(dpy, Pcmap, &xfgsh);
		fft_fgsh.color.red = xfgsh.red;
		fft_fgsh.color.green = xfgsh.green;
		fft_fgsh.color.blue = xfgsh.blue;
		fft_fgsh.color.alpha = 0xffff;
		fft_fgsh.pixel = xfgsh.pixel;
	}

	if (FftUtf8Support && fftf->utf8)
	{
		if (flf->shadow_size > 0 && has_fg_pixels)
		{
			for(i=1; i <= flf->shadow_size; i++)
			{
				FftDrawStringUtf8(
					  fftdraw, &fft_fgsh, uf,
					  x + (xsh_sign*i), y + (ysh_sign*i),
					  (FftChar8 *)fws->str, len);
			}
		}
		FftDrawStringUtf8(fftdraw, &fft_fg, uf, x, y,
				  (FftChar8 *)fws->str, len);
	}
	else if (fftf->utf8)
	{
		FftChar16 *new;
		int nl;

		new = FftUtf8ToFftString16((unsigned char *)fws->str, len, &nl);
		if (new != NULL)
		{
			if (flf->shadow_size > 0 && has_fg_pixels)
			{
				for(i=1; i <= flf->shadow_size; i++)
				{	
					FftDrawString16(
						fftdraw, &fft_fgsh, uf,
						x + (xsh_sign*i),
						y + (ysh_sign*i),
						new, nl);
				}
			}
			FftDrawString16(fftdraw, &fft_fg, uf, x, y, new, nl);
			free(new);
		}
	}
	else
	{
		if (flf->shadow_size > 0 && has_fg_pixels)
		{
			for(i=1; i <= flf->shadow_size; i++)
			{
				FftDrawString8(
				       fftdraw, &fft_fgsh, uf,
				       x + (xsh_sign*i), y + (ysh_sign*i),
				       (unsigned char *)fws->str, len);
			}
		}
		FftDrawString8(
			       fftdraw, &fft_fg, uf, x, y, 
			       (unsigned char *)fws->str, len);
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
	else if (fftf->utf8)
	{
		FftChar16 *new;
		int nl;

		new = FftUtf8ToFftString16((unsigned char *)str, len, &nl);
		if (new != NULL)
		{
			FftTextExtents16(
				fftdpy, fftf->fftfont, new, nl, &extents);
			free(new);
		}
	}
	else
	{
		FftTextExtents8(fftdpy, fftf->fftfont, str, len, &extents);
	}

	return extents.xOff;
}
