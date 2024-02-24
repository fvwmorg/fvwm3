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
#include "FlocaleCharset.h"
#include "Ficonv.h"
#include "FftInterface.h"
#include "FRenderInit.h"
#include "PictureBase.h"
#include "log.h"

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
	FlocaleCharsetInit(dpy, NULL);
	fft_initialized = True;
}

static
XftFont *FftGetRotatedFont(Display *dpy, FftFontType *ft, rotation_t rotation)
{
	if (ft->fftfont[rotation]) {
		return ft->fftfont[rotation];
	}

	FcPattern *rotated_pat;
	FcMatrix r,b;
	FcMatrix *pm = NULL;
	XftFont *rf = NULL;

	memset(&b, 0, sizeof(FcMatrix));

	rotated_pat = FcPatternDuplicate(ft->fftfont[0]->pattern);

	if (rotated_pat == NULL)
		return NULL;

	if (rotation == ROTATION_90)
		FFT_SET_ROTATED_90_MATRIX(&r);
	else if (rotation == ROTATION_180)
		FFT_SET_ROTATED_180_MATRIX(&r);
	else if (rotation == ROTATION_270)
		FFT_SET_ROTATED_270_MATRIX(&r);
	else
		goto cleanup;

	if ((FcPatternGetMatrix(
		rotated_pat, FC_MATRIX, 0, &pm) == FcResultMatch) && pm)
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
	FcPatternDel(rotated_pat, FC_MATRIX);

	if (!FcPatternAddMatrix(rotated_pat, FC_MATRIX, &b))
		goto cleanup;

	rf = XftFontOpenPattern(dpy, rotated_pat);

 cleanup:
	if (!rf && rotated_pat)
		FcPatternDestroy(rotated_pat);

	ft->fftfont[rotation] = rf;
	return rf;
}

void FftGetFontHeights(
	FftFontType *fftf, int *height, int *ascent, int *descent)
{
	/* fft font height may be > fftfont->ascent + fftfont->descent, this
	 * depends on the minspace value */
	*height = fftf->fftfont[0]->height;
	*ascent = fftf->fftfont[0]->ascent;
	*descent = fftf->fftfont[0]->descent;

	return;
}

void FftCloseFont(Display *dpy, FftFontType *fftf)
{
	int i = 0;
	for (i = 0; i < 4; i++) {
		if (fftf->fftfont[i])
			XftFontClose(dpy, fftf->fftfont[i]);
	}
}

FftFontType *FftGetFont(Display *dpy, char *fontname, char *module)
{
	XftFont *fftfont = NULL;
	FftFontType *fftf = NULL;
	FcPattern *src_pat = NULL, *load_pat = NULL;
	FcMatrix *a = NULL;
	FcResult result = 0;

	if (!FftSupport || !fontname)
		return NULL;

	if (!fft_initialized)
		init_fft(dpy);

	if ((src_pat = XftNameParse(fontname)) == NULL ||
	    (load_pat = XftFontMatch(dpy, fftscreen, src_pat, &result)) == NULL)
		goto cleanup;


	/* safety check */
	if (FcPatternGetMatrix(load_pat, FC_MATRIX, 0, &a) == FcResultMatch && a != NULL)
	{
		FcMatrix b;
		Bool cm = False;
		memset(&b, 0, sizeof(FcMatrix));

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
			FcPatternDel(load_pat, FC_MATRIX);

			if (!FcPatternAddMatrix(load_pat, FC_MATRIX, &b))
				goto cleanup;
		}
	}
	/* FIXME: other safety checking ? */

	if ((fftfont = XftFontOpenPattern(dpy, load_pat)) == NULL)
		goto cleanup;

	fftf = fxcalloc(1, sizeof(FftFontType));
	fftf->fftfont[0] = fftfont;

	/*
	 * We could either use UCS4 or UTF-8. We choose UTF-8 because:
	 *
	 * 1. Almost everyone use UTF-8 locales now. Using UTF-8 avoids
	 * FlocaleEncodeString() converting the encoding.
	 *
	 * 2. FcUtf8toUcs4() is fast because it avoids a memory allocation. It's
	 * also a common path for other softwares, so it's likely to be well
	 * optimized.
	 *
	 */
	fftf->encoding = FlocaleCharsetGetUtf8Charset()->x;

 cleanup:
	if (src_pat)
		FcPatternDestroy(src_pat);
	if (!fftf && load_pat)
		FcPatternDestroy(load_pat);
	return fftf;
}

#define CACHE_ENTIRES 63

static struct font_cache {
	int fifo_start, fifo_end;

	/*
	 * Searching 64 shorts doesn't takes a full hash table. That's only two
	 * cache lines.
	 */
	unsigned short hashes[CACHE_ENTIRES];

	FcPattern *srcs[CACHE_ENTIRES];
	FcPattern *patterns[CACHE_ENTIRES];
} g_font_cache;

static FcPattern *MatchCacheFont(FcChar32 code, FcPattern *src, FcResult *result)
{
	unsigned short hash = FcPatternHash(src);
	int i = 0;
	hash ^= code; /* Because FcPatternHash() does not hash charset due to its cost. */

	for (i = g_font_cache.fifo_start; i < g_font_cache.fifo_end; i++) {
		int idx = i % CACHE_ENTIRES;
		if (g_font_cache.hashes[idx] != hash) continue;
		if (FcPatternEqual(g_font_cache.srcs[idx], src)) {
			*result = FcResultMatch;
			return FcPatternDuplicate(g_font_cache.patterns[idx]);
		}
	}
	*result = FcResultNoMatch;
	return NULL;
}

static void AddCacheFont(FcChar32 code, FcPattern *src, FcPattern *match)
{
	while (g_font_cache.fifo_end - g_font_cache.fifo_start >= CACHE_ENTIRES) {
		int idx = g_font_cache.fifo_start % CACHE_ENTIRES;
		FcPatternDestroy(g_font_cache.srcs[idx]);
		FcPatternDestroy(g_font_cache.patterns[idx]);
		g_font_cache.fifo_start++;
	}
	unsigned short hash = FcPatternHash(src);
	hash ^= code;

	int idx = g_font_cache.fifo_end % CACHE_ENTIRES;
	g_font_cache.hashes[idx] = hash;
	g_font_cache.srcs[idx] = FcPatternDuplicate(src);
	g_font_cache.patterns[idx] = FcPatternDuplicate(match);
	g_font_cache.fifo_end++;
}

static void MatchFont(Display *dpy,
		      XftFont *font,
		      int *xoff, int *yoff,
		      const char *p, const int len,
		      void (*render_char)(void *, XftCharFontSpec *),
		      void *user_data)
{
	FcPattern *template = NULL;
	int codelen = 0, off = 0;

	for (off = 0; off < len; off += codelen) {
		XftCharFontSpec sp;
		sp.x = *xoff;
		sp.y = *yoff;
		sp.font = font;
		sp.ucs4 = 0;

		codelen = FcUtf8ToUcs4((const FcChar8 *) p + off, &sp.ucs4, len - off);
                if (codelen <= 0) {
			fvwm_debug("XFT", "Malformed UTF8 char %x", (char) *(p + off));
			codelen = 1;
			continue;
		}

		if (!XftCharExists(dpy, font, sp.ucs4)) {
			/* Fallback */
			if (!template) {
				int slant = 0, weight = 0;
				double font_size = 0.0;
				FcPatternGetDouble(font->pattern, FC_SIZE, 0, &font_size);
				FcPatternGetInteger(font->pattern, FC_WEIGHT, 0, &weight);
				FcPatternGetInteger(font->pattern, FC_SLANT, 0, &slant);
				template = FcPatternBuild(
				    NULL,
				    FC_SIZE, FcTypeDouble, font_size,
				    FC_WEIGHT, FcTypeInteger, weight,
				    FC_SLANT, FcTypeInteger, slant,
				    NULL);
			} else {
				FcPatternDel(template, FC_CHARSET);
			}

			FcCharSet *fallback_charset = FcCharSetCreate();
			FcCharSetAddChar(fallback_charset, sp.ucs4);
			FcPatternAddCharSet(template, FC_CHARSET, fallback_charset);

			/* Lookup from the cache first! */
			FcResult result = FcResultMatch;
			FcPattern *font_pattern = MatchCacheFont(sp.ucs4, template, &result);
			if (!font_pattern) {
				font_pattern = XftFontMatch(dpy, XDefaultScreen(dpy), template, &result);
				if (font_pattern) AddCacheFont(sp.ucs4, template, font_pattern);
			}
			FcCharSetDestroy(fallback_charset);

			if (result == FcResultMatch) {
				XftFont *fallback_font = XftFontOpenPattern(dpy, font_pattern);
				if (fallback_font) sp.font = fallback_font;
				else FcPatternDestroy(font_pattern);
			}
		}
		XGlyphInfo info;
		XftTextExtents32(dpy, sp.font, &sp.ucs4, 1, &info);
		*xoff += info.xOff;
		*yoff += info.yOff;

		if (render_char) {
			render_char(user_data, &sp);
		} else if (sp.font != font) {
			XftFontClose(dpy, sp.font);
		}
	}
	if (template)
		FcPatternDestroy(template);
}

#define NR_CHARFONTSPEC 31

struct CharFontSpecBatch
{
	Display *dpy;
	XftFont *default_font;
	XftDraw *draw;
	XftColor *color;
	int size;
	XftCharFontSpec specs[NR_CHARFONTSPEC];
};

static void RenderCharFontSpec(void *p, XftCharFontSpec *sp)
{
	struct CharFontSpecBatch *batch = p;

	if (batch->size == NR_CHARFONTSPEC || sp == NULL)
	{
		XftDrawCharFontSpec(batch->draw, batch->color, batch->specs, batch->size);
		int i = 0;
		for (i = 0; i < batch->size; i++)
			if (batch->specs[i].font != batch->default_font)
				XftFontClose(batch->dpy, batch->specs[i].font);
		batch->size = 0;
	}

	if (sp != NULL)
		batch->specs[batch->size++] = *sp;
}

void FftDrawString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws,
	Pixel fg, Pixel fgsh, Bool has_fg_pixels, int len, unsigned long flags)
{
	XftDraw *draw = NULL;
	char *str;
	XGCValues vr;
	XColor xfg, xfgsh;
	XftColor fft_fg, fft_fgsh;
	FftFontType *fftf;
	XftFont *uf;
	int x,y, xt,yt;
	float alpha_factor;
	flocale_gstp_args gstp_args;

	if (!FftSupport)
		return;

	fftf = &flf->fftf;
	uf = FftGetRotatedFont(dpy, fftf, fws->flags.text_rotation);
	if (fws->flags.text_rotation == ROTATION_90) /* CW */
	{
		y = fws->y;
		x = fws->x - FLF_SHADOW_BOTTOM_SIZE(flf);
	}
	else if (fws->flags.text_rotation == ROTATION_180)
	{
		y = fws->y;
		x = fws->x + FftTextWidth(dpy, flf, fws->e_str, len);
	}
	else if (fws->flags.text_rotation == ROTATION_270)
	{
		y = fws->y + FftTextWidth(dpy, flf, fws->e_str, len);
		x = fws->x - FLF_SHADOW_UPPER_SIZE(flf);
	}
	else
	{
		y = fws->y;
		x = fws->x;
	}

	if (uf == NULL)
		return;

	draw = XftDrawCreate(dpy, (Drawable) fws->win, Pvisual, Pcmap);
	if (fws->flags.has_clip_region)
	{
		XftDrawSetClip(draw, fws->clip_region);
	}
	if (has_fg_pixels)
	{
		xfg.pixel = fg;
		xfgsh.pixel = fgsh;
	}
	else if (fws->gc && XGetGCValues(dpy, fws->gc, GCForeground, &vr))
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
	FlocaleInitGstpArgs(&gstp_args, flf, fws, x, y);

	struct CharFontSpecBatch batch;
	batch.size = 0;
	batch.dpy = dpy;
	batch.default_font = uf;
	batch.draw = draw;
	batch.color = &fft_fgsh;

	if (flf->shadow_size != 0 && has_fg_pixels)
	{
		while (FlocaleGetShadowTextPosition(&xt, &yt, &gstp_args)) {
			int mxt = xt, myt = yt;
			MatchFont(dpy, uf, &mxt, &myt, str, len, &RenderCharFontSpec, &batch);
			RenderCharFontSpec(&batch, NULL);
		}
	}

	batch.color = &fft_fg;
	xt = gstp_args.orig_x;
	yt = gstp_args.orig_y;
	MatchFont(dpy, uf, &xt, &yt, str, len, &RenderCharFontSpec, &batch);
	RenderCharFontSpec(&batch, NULL);

	XftDrawDestroy(draw);

	return;
}

int FftTextWidth(Display *dpy, FlocaleFont *flf, char *str, int len)
{
	int xoff = 0, yoff = 0;
	MatchFont(dpy, flf->fftf.fftfont[0], &xoff, &yoff, str, len, NULL, NULL);
	return xoff;
}


void FftPrintPatternInfo(FftFontType *ft)
{
	fvwm_debug("xft", "    Xft info:\n");

	int i = 0;
	XftFont *f = NULL;
	FcMatrix *pm = NULL;

	for (i = 0; i < 4; i++) {
		f = ft->fftfont[i];
		if (f == NULL)
		{
			continue;
		}
		fvwm_debug("xft", "    Rotation %d\n", i * 90);
		fvwm_debug(
			"xft",
			"	   height: %i, ascent: %i,"
			" descent: %i, maw: %i\n",
			f->height, f->ascent, f->descent, f->max_advance_width);
		if (i == 0)
		{
			FcPatternPrint(f->pattern);
		}
		else
		{
			if (FcPatternGetMatrix(
				    f->pattern, FC_MATRIX, 0, &pm) ==
			    FcResultMatch && pm)
			{
				fvwm_debug(
					"xft",
					"	   matrix: (%f %f %f %f)\n",
					pm->xx, pm->xy, pm->yx, pm->yy);
			}
		}
	}
	return;
}
