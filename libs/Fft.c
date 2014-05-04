/* -*-c-*- */
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

#include <X11/Xlib.h>

#include "fvwmlib.h"
#include "wild.h"
#include "Strings.h"
#include "Flocale.h"
#include "Fft.h"
#include "FlocaleCharset.h"
#include "FftInterface.h"
#include "FRenderInit.h"
#include "PictureBase.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

#define FFT_SET_ROTATED_90_MATRIX(m) \
	((m)->xx = (m)->yy = 0, (m)->xy = 1, (m)->yx = -1)

#define FFT_SET_ROTATED_270_MATRIX(m) \
	((m)->xx = (m)->yy = 0, (m)->xy = -1, (m)->yx = 1)

#define FFT_SET_ROTATED_180_MATRIX(m) \
	((m)->xx = (m)->yy = -1, (m)->xy = (m)->yx = 0)

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static Display *fftdpy = NULL;
static int fftscreen;
static int fft_initialized = False;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

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

	new = xmalloc((len+1)*sizeof(FftChar16));
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
void FftSetupEncoding(
	Display *dpy, FftFontType *fftf, char *encoding, char *module)
{
	FlocaleCharset *fc;

	if (!FftSupport || fftf == NULL)
		return;

	fftf->encoding = NULL;
	fftf->str_encoding = NULL;

	if (encoding != NULL)
	{
		fftf->str_encoding = encoding;
	}
	if (FftSupportUseXft2)
	{
		if (encoding != NULL)
		{
			fftf->encoding = encoding;
		}
		else if ((fc =
			  FlocaleCharsetGetDefaultCharset(dpy,NULL)) != NULL &&
			 StrEquals(fc->x,FLC_FFT_ENCODING_ISO8859_1))
		{
			fftf->encoding = FLC_FFT_ENCODING_ISO8859_1;
		}
		else
		{
			fftf->encoding = FLC_FFT_ENCODING_ISO10646_1;
		}
	}
	else if (FftSupport)
	{
		int i = 0;
		FftPatternElt *e;
		Fft1Font *f;

		f =  (Fft1Font *)fftf->fftfont;
		while(i < f->pattern->num)
		{
			e = &f->pattern->elts[i];
			if (StrEquals(e->object, FFT_ENCODING) &&
			    e->values->value.u.s != NULL)
			{
				fftf->encoding = e->values->value.u.s;
				return;
			}
			i++;
		}
	}
}

static
FftFont *FftGetRotatedFont(
	Display *dpy, FftFont *f, rotation_t rotation)
{
	FftPattern *rotated_pat;
	FftMatrix r,b;
	FftMatrix *pm = NULL;
	FftFont *rf = NULL;

	if (f == NULL)
		return NULL;

	memset(&b, 0, sizeof(FftMatrix));

	rotated_pat = FftPatternDuplicate(f->pattern);

	if (rotated_pat == NULL)
	{
		return NULL;
	}

	if (rotation == ROTATION_90)
	{
		FFT_SET_ROTATED_90_MATRIX(&r);
	}
	else if (rotation == ROTATION_180)
	{
		FFT_SET_ROTATED_180_MATRIX(&r);
	}
	else if (rotation == ROTATION_270)
	{
		FFT_SET_ROTATED_270_MATRIX(&r);
	}
	else
	{
		goto bail;
	}

	if ((FftPatternGetMatrix(
		rotated_pat, FFT_MATRIX, 0, &pm) == FftResultMatch) && pm)
	{
		/* rotate the matrice */
		b.xx = r.xx * pm->xx + r.xy * pm->yx;
		b.xy = r.xx * pm->xy + r.xy * pm->yy;
		b.yx = r.yx * pm->xx + r.yy * pm->yx;
		b.yy = r.yx * pm->xy + r.yy * pm->yy;
	}
	else
	{
		b.xx = r.xx;
		b.xy = r.xy;
		b.yx = r.yx;
		b.yy = r.yy;
	}
	if (FftPatternDel(rotated_pat, FFT_MATRIX))
	{
		/* nothing */
	}
	if (!FftPatternAddMatrix(rotated_pat, FFT_MATRIX, &b))
	{
		goto bail;
	}
	rf = FftFontOpenPattern(dpy, rotated_pat);

 bail:
	if (!rf && rotated_pat)
	{
		FftPatternDestroy(rotated_pat);
	}
	return rf;
}

void FftPDumyFunc(void)
{
}

/* ---------------------------- interface functions ------------------------ */

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
	FlocaleFont *flf, int *max_char_width)
{
	FGlyphInfo extents;

	memset(&extents, 0, sizeof(FGlyphInfo));

	/* FIXME:  max_char_width should not be use in the all fvwm! */
	if (FftUtf8Support && FLC_ENCODING_TYPE_IS_UTF_8(flf->fc))
	{
		FftTextExtentsUtf8(fftdpy, flf->fftf.fftfont, (FftChar8*)"W",
				   1, &extents);
	}
	else
	{
		FftTextExtents8(fftdpy, flf->fftf.fftfont, (FftChar8*)"W", 1,
				&extents);
	}
	*max_char_width = extents.xOff;

	return;
}

FftFontType *FftGetFont(Display *dpy, char *fontname, char *module)
{
	FftFont *fftfont = NULL;
	FftFontType *fftf = NULL;
	char *fn = NULL, *str_enc = NULL;
	FlocaleCharset *fc;
	FftPattern *src_pat = NULL, *load_pat = NULL;
	FftMatrix *a = NULL;
	FftResult result = 0;

	if (!FftSupport || !(FRenderGetExtensionSupported() || 1))
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
	/* Xft2 always load an USC-4 fonts, that we consider as an USC-2 font
	 * (i.e., an ISO106464-1 font) or an ISO8859-1 if the user ask for this
	 * or the locale charset is ISO8859-1.
	 * Xft1 support ISO106464-1, ISO8859-1 and others we load an
	 * ISO8859-1 font by default if the locale charset is ISO8859-1 and
	 * an ISO106464-1 one if this not the case */
	if (matchWildcards("*?8859-1*", fontname))
	{
		str_enc = FLC_FFT_ENCODING_ISO8859_1;
	}
	else if (matchWildcards("*?10646-1*", fontname))
	{
		str_enc = FLC_FFT_ENCODING_ISO10646_1;
	}
	if (!FftSupportUseXft2 && str_enc == NULL)
	{
		if ((fc = FlocaleCharsetGetFLCXOMCharset()) != NULL &&
		    StrEquals(fc->x,FLC_FFT_ENCODING_ISO8859_1))
		{
			fn = CatString2(fontname,":encoding=ISO8859-1");
		}
		else
		{
			fn = CatString2(fontname,":encoding=ISO10646-1");
		}
	}
	else
	{
		fn = fontname;
	}
	SUPPRESS_UNUSED_VAR_WARNING(fn);
	if ((src_pat = FftNameParse(fn)) == NULL)
	{
		goto bail;
	}
	SUPPRESS_UNUSED_VAR_WARNING(result);
	if ((load_pat = FftFontMatch(dpy, fftscreen, src_pat, &result)) == NULL)
	{
		goto bail;
	}
	/* safety check */
	if (FftPatternGetMatrix(
		load_pat, FFT_MATRIX, 0, &a) == FftResultMatch && a)
	{
		FftMatrix b;
		Bool cm = False;

		memset(&b, 0, sizeof(FftMatrix));

		if (a->xx < 0)
		{
			a->xx = -a->xx;
			cm = True;
		}
		if (a->yx != 0)
		{
			a->yx = 0;
			cm = True;
		}
		if (cm)
		{
			b.xx = a->xx;
			b.xy = a->xy;
			b.yx = a->yx;
			b.yy = a->yy;
			if (FftPatternDel(load_pat, FFT_MATRIX))
			{
				/* nothing */
			}
			if (!FftPatternAddMatrix(load_pat, FFT_MATRIX, &b))
			{
				goto bail;
			}
		}
	}
	/* FIXME: other safety checking ? */
	fftfont = FftFontOpenPattern(dpy, load_pat);

	if (!fftfont)
	{
		goto bail;
	}
	fftf = xmalloc(sizeof(FftFontType));
	fftf->fftfont = fftfont;
	fftf->fftfont_rotated_90 = NULL;
	fftf->fftfont_rotated_180 = NULL;
	fftf->fftfont_rotated_270 = NULL;
	FftSetupEncoding(dpy, fftf, str_enc, module);

 bail:
	if (src_pat)
	{
		FftPatternDestroy(src_pat);
	}
	if (!fftf && load_pat)
	{
		FftPatternDestroy(load_pat);
	}
	return fftf;
}

void FftDrawString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws,
	Pixel fg, Pixel fgsh, Bool has_fg_pixels, int len, unsigned long flags)
{
	FftDraw *fftdraw = NULL;
	typedef void (*DrawStringFuncType)(
		FftDraw *fftdraw, FftColor *fft_fg, FftFont *uf, int xt,
		int yt, char *str, int len);
	DrawStringFuncType DrawStringFunc;
	char *str;
	Bool free_str = False;
	XGCValues vr;
	XColor xfg, xfgsh;
	FftColor fft_fg, fft_fgsh;
	FftFontType *fftf;
	FftFont *uf;
	int x,y, xt,yt;
	float alpha_factor;
	flocale_gstp_args gstp_args;

	if (!FftSupport)
	{
		return;
	}

	fftf = &flf->fftf;
	if (fws->flags.text_rotation == ROTATION_90) /* CW */
	{
		if (fftf->fftfont_rotated_90 == NULL)
		{
			fftf->fftfont_rotated_90 =
				FftGetRotatedFont(dpy, fftf->fftfont,
						  fws->flags.text_rotation);
		}
		uf = fftf->fftfont_rotated_90;
#ifdef FFT_BUGGY_FREETYPE
		y = fws->y +  FftTextWidth(flf, fws->e_str, len);
#else
		y = fws->y;
#endif
		x = fws->x - FLF_SHADOW_BOTTOM_SIZE(flf);
	}
	else if (fws->flags.text_rotation == ROTATION_180)
	{
		if (fftf->fftfont_rotated_180 == NULL)
		{
			fftf->fftfont_rotated_180 =
				FftGetRotatedFont(dpy, fftf->fftfont,
						  fws->flags.text_rotation);
		}
		uf = fftf->fftfont_rotated_180;
		y = fws->y;
		x = fws->x + FftTextWidth(flf, fws->e_str, len);
	}
	else if (fws->flags.text_rotation == ROTATION_270)
	{
		if (fftf->fftfont_rotated_270 == NULL)
		{
			fftf->fftfont_rotated_270 =
				FftGetRotatedFont(dpy, fftf->fftfont,
						  fws->flags.text_rotation);
		}
		uf = fftf->fftfont_rotated_270;
#ifdef FFT_BUGGY_FREETYPE
		y = fws->y;
#else
		y = fws->y + FftTextWidth(flf, fws->e_str, len);
#endif
		x = fws->x - FLF_SHADOW_UPPER_SIZE(flf);
	}
	else
	{
		uf = fftf->fftfont;
		y = fws->y;
		x = fws->x;
	}

	if (uf == NULL)
		return;

	fftdraw = FftDrawCreate(dpy, (Drawable)fws->win, Pvisual, Pcmap);
	if (fws->flags.has_clip_region)
	{
		if (FftDrawSetClip(fftdraw, fws->clip_region) == False)
		{
			/* just to suppress a compiler warning */
		}
	}
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
			" cannot find color\n");
#endif
		xfg.pixel = PictureBlackPixel();
	}

	XQueryColor(dpy, Pcmap, &xfg);
	alpha_factor = ((fws->flags.has_colorset)?
		 ((float)fws->colorset->fg_alpha_percent/100) : 1);
	/* Render uses premultiplied alpha */
	fft_fg.color.red = xfg.red * alpha_factor;
	fft_fg.color.green = xfg.green * alpha_factor;
	fft_fg.color.blue = xfg.blue * alpha_factor;
	fft_fg.color.alpha = 0xffff * alpha_factor;
	fft_fg.pixel = xfg.pixel;
	if (flf->shadow_size != 0 && has_fg_pixels)
	{
		XQueryColor(dpy, Pcmap, &xfgsh);
		fft_fgsh.color.red = xfgsh.red * alpha_factor;
		fft_fgsh.color.green = xfgsh.green * alpha_factor;
		fft_fgsh.color.blue = xfgsh.blue * alpha_factor;
		fft_fgsh.color.alpha = 0xffff * alpha_factor;
		fft_fgsh.pixel = xfgsh.pixel;
	}

	xt = x;
	yt = y;

	str = fws->e_str;
	if (FftUtf8Support && FLC_ENCODING_TYPE_IS_UTF_8(flf->fc))
	{
		DrawStringFunc = (DrawStringFuncType)FftPDrawStringUtf8;
	}
	else if (FLC_ENCODING_TYPE_IS_UTF_8(flf->fc))
	{
		DrawStringFunc = (DrawStringFuncType)FftPDrawString16;
		str = (char *)FftUtf8ToFftString16(
			  (unsigned char *)fws->e_str, len, &len);
		free_str = True;
	}
	else if (FLC_ENCODING_TYPE_IS_USC_2(flf->fc))
	{
		DrawStringFunc = (DrawStringFuncType)FftPDrawString16;
	}
	else if (FLC_ENCODING_TYPE_IS_USC_4(flf->fc))
	{
		DrawStringFunc = (DrawStringFuncType)FftPDrawString32;
	}
	else
	{
		DrawStringFunc = (DrawStringFuncType)FftPDrawString8;
	}

	FlocaleInitGstpArgs(&gstp_args, flf, fws, x, y);
	if (flf->shadow_size != 0 && has_fg_pixels)
	{
		while (FlocaleGetShadowTextPosition(&xt, &yt, &gstp_args))
		{
			DrawStringFunc(
				fftdraw, &fft_fgsh, uf, xt, yt, str, len);
		}
	}
	xt = gstp_args.orig_x;
	yt = gstp_args.orig_y;
	DrawStringFunc(fftdraw, &fft_fg, uf, xt, yt, str, len);

	if (free_str && str != NULL)
	{
		free(str);
	}
	FftDrawDestroy(fftdraw);

	return;
}

int FftTextWidth(FlocaleFont *flf, char *str, int len)
{
	FGlyphInfo extents;
	int result = 0;

	if (!FftSupport)
	{
		return 0;
	}
	if (FftUtf8Support && FLC_ENCODING_TYPE_IS_UTF_8(flf->fc))
	{
		FftTextExtentsUtf8(
				fftdpy, flf->fftf.fftfont, (FftChar8*)str, len,
				&extents);
		result = extents.xOff;
	}
	else if (FLC_ENCODING_TYPE_IS_UTF_8(flf->fc))
	{
		FftChar16 *new;
		int nl;

		new = FftUtf8ToFftString16((unsigned char *)str, len, &nl);
		if (new != NULL)
		{
			FftTextExtents16(
				fftdpy, flf->fftf.fftfont, new, nl, &extents);
			result = extents.xOff;
			free(new);
		}
	}
	else if (FLC_ENCODING_TYPE_IS_USC_2(flf->fc))
	{
		FftTextExtents16(
			fftdpy, flf->fftf.fftfont, (FftChar16 *)str, len,
			&extents);
		result = extents.xOff;
	}
	else if (FLC_ENCODING_TYPE_IS_USC_4(flf->fc))
	{
		FftTextExtents32(
			fftdpy, flf->fftf.fftfont, (FftChar32 *)str, len,
			&extents);
		result = extents.xOff;
	}
	else
	{
		FftTextExtents8(
			fftdpy, flf->fftf.fftfont, (FftChar8 *)str, len,
			&extents);
		result = extents.xOff;
	}

	return result;
}


void FftPrintPatternInfo(FftFont *f, Bool vertical)
{
	/* FftPatternPrint use stdout */
	fflush (stderr);
	printf("\n        height: %i, ascent: %i, descent: %i, maw: %i\n",
	       f->height, f->ascent, f->descent, f->max_advance_width);
	if (!vertical)
	{
		printf("        ");
		FftPatternPrint(f->pattern);
	}
	else
	{
		FftMatrix *pm = NULL;

		if (FftPatternGetMatrix(
			f->pattern, FFT_MATRIX, 0, &pm) == FftResultMatch && pm)
		{
			printf("         matrix: (%f %f %f %f)\n",
			       pm->xx, pm->xy, pm->yx, pm->yy);
		}
	}
	fflush (stdout);
	return;
}
