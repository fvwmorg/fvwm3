/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis
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

/* FlocaleRotateDrawString is strongly inspired by some part of xvertext
 * taken from wmx */
/* Here the copyright for this function: */
/* xvertext, Copyright (c) 1992 Alan Richardson (mppa3@uk.ac.sussex.syma)
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  All work developed as a consequence of the use of
 * this program should duly acknowledge such use. No representations are
 * made about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * Minor modifications by Chris Cannam for wm2/wmx
 * Major modifications by Kazushi (Jam) Marukawa for wm2/wmx i18n patch
 * Simplification by olicha for use with fvwm
 */

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "defaults.h"
#include "fvwmlib.h"
#include "safemalloc.h"
#include "Strings.h"
#include "Parse.h"
#include "PictureBase.h"
#include "Flocale.h"
#include "FlocaleCharset.h"
#include "FBidi.h"
#include "FftInterface.h"
#include "Colorset.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static FlocaleFont *FlocaleFontList = NULL;
static char *Flocale = NULL;
static Bool FlocaleSeted = False;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */


static
XChar2b *FlocaleUtf8ToUnicodeStr2b(unsigned char *str, int len, int *nl)
{
	XChar2b *str2b = NULL;
	int i = 0, j = 0, t;

	str2b = (XChar2b *)safemalloc((len+1)*sizeof(XChar2b));
	while(i < len && str[i] != 0)
	{
		if (str[i] <= 0x7f)
		{
			str2b[j].byte2 = str[i];
			str2b[j].byte1 = 0;	
		}
		else if (str[i] <= 0xdf && i+1 < len)
		{
		    t = ((str[i] & 0x1f) << 6) + (str[i+1] & 0x3f);
		    str2b[j].byte2 = (unsigned char)(t & 0xff);
		    str2b[j].byte1 = (unsigned char)(t >> 8);
		    i++;
		}
		else if (i+2 <len)
		{
			t = ((str[i] & 0x0f) << 12) + ((str[i+1] & 0x3f) << 6)+
				(str[i+2] & 0x3f);
			str2b[j].byte2 = (unsigned char)(t & 0xff);
			str2b[j].byte1 = (unsigned char)(t >> 8);
			i += 2;
		}
		i++; j++;
	}
	*nl = j;
	return str2b;
}

/* Note: this function is not expected to work perfectly; good mb rendering
 * should be (and is) done using Xmb functions and not XDrawString16.
 * This function is used when the locale does not correspond to the font */ 
static
XChar2b *FlocaleStringToString2b(unsigned char *str, int len, int *nl)
{
	XChar2b *str2b = NULL;
	int i = 0, j = 0;

	str2b = (XChar2b *)safemalloc((len+1)*sizeof(XChar2b));
	while(i < len && str[i] != 0)
	{
		if (str[i] <= 0x7f)
		{
			/* ascii: this is not perfect but ok for gb2312.1980-0,
			 * and ksc5601.1987-0, but for jisx0208.1983-0 this is
			 * ok only for 0-9, a-z and A-Z, char as (, ! ...etc
			 * are elsewhere  */
			str2b[j].byte1 = 0x23; /* magic number!! */
			str2b[j].byte2 = str[i++];
		}
		else
		{
			/* mb gl (for gr replace & 0x7f by | 0x80 ...)*/
			str2b[j].byte1 = str[i++] & 0x7f;
			str2b[j].byte2 = str[i++] & 0x7f;
		}
		j++;
	}
	*nl = j;
	return str2b;
}

static
void FlocaleFontStructDrawString(Display *dpy, FlocaleFont *flf, Drawable d,
				 GC gc, int x, int y, Pixel fg, Pixel fgsh,
				 Bool has_fg_pixels, FlocaleWinString *fws,
				 int len, Bool image)
{
	int i;

	if (flf->utf8 || flf->mb)
	{
		XChar2b *str2b;
		int nl;

		if (flf->utf8)
			str2b = FlocaleUtf8ToUnicodeStr2b(fws->str, len, &nl);
		else
			str2b = FlocaleStringToString2b(fws->str, len, &nl);
		if (str2b != NULL)
		{
			if (image)
			{
				/* rotated drawing */
				XDrawImageString16(dpy, d, gc, x, y, str2b, nl);
			}
			else
			{
				/* normal drawing */
				if (flf->shadow_size > 0 && has_fg_pixels)
				{
					XSetForeground(dpy, gc, fgsh);
					for(i=1; i <= flf->shadow_size; i++)
					{
						XDrawString16(dpy, d, gc,
							      x+i, y+i,
							      str2b, nl);
					}
					XSetForeground(dpy, gc, fg);
				}
				XDrawString16(dpy, d, gc, x, y, str2b, nl);
			}
			free(str2b);
		}
	}
	else
	{
		if (image)
		{
			/* rotated drawing */
			XDrawImageString(dpy, d, gc, x, y, fws->str, len);
		}
		else
		{
			if (flf->shadow_size > 0 && has_fg_pixels)
			{
				XSetForeground(dpy, gc, fgsh);
				for(i=1; i <= flf->shadow_size; i++)
				{
					XDrawString(dpy, d, gc, x+i, y+i,
						    fws->str, len);
				}
				XSetForeground(dpy, gc, fg);
			}
			XDrawString(dpy, d, gc, x, y, fws->str, len);
		}
	}
}

static
void FlocaleRotateDrawString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws, Pixel fg,
	Pixel fgsh, Bool has_fg_pixels, int len)
{
	static GC my_gc, font_gc;
	int j, i, xpfg, ypfg, xpsh, ypsh, xpsh_sign, ypsh_sign;
	unsigned char *normal_data, *rotated_data;
	unsigned int normal_w, normal_h, normal_len;
	unsigned int rotated_w, rotated_h, rotated_len;
	char val;
	int width, height;
	XImage *image, *rotated_image;
	Pixmap canvas_pix, rotated_pix;

	if (fws->str == NULL || len < 1)
		return;
	if (fws->flags.text_rotation == TEXT_ROTATED_0)
		return; /* should not happen */

	my_gc = fvwmlib_XCreateGC(dpy, fws->win, 0, NULL);
	XCopyGC(dpy, fws->gc, GCForeground|GCBackground, my_gc);

	/* width and height */
	width = FlocaleTextWidth(flf, fws->str, len) - flf->shadow_size;
	height = flf->height - flf->shadow_size;

	if (width < 1) width = 1;
	if (height < 1) height = 1;

	/* glyph width and height of the normal text */
	normal_w = width;
	normal_h = height;

	/* width in bytes */
	normal_len = (normal_w - 1) / 8 + 1;

	/* create and clear the canvas */
	canvas_pix = XCreatePixmap(dpy, fws->win, width, height, 1);
	font_gc = fvwmlib_XCreateGC(dpy, canvas_pix, 0, NULL);
	XSetBackground(dpy, font_gc, 0);
	XSetForeground(dpy, font_gc, 0);
	XFillRectangle(dpy, canvas_pix, font_gc, 0, 0, width, height);

	/* draw the character center top right on canvas */
	XSetForeground(dpy, font_gc, 1);
	if (flf->font != NULL)
	{
		XSetFont(dpy, font_gc, flf->font->fid);
		FlocaleFontStructDrawString(dpy, flf, canvas_pix, font_gc, 0,
					    flf->height - flf->descent,
					    fg, fgsh, has_fg_pixels,
					    fws, len, True);
	}
	else if (FlocaleMultibyteSupport && flf->fontset != None)
	{
		XmbDrawString(
			dpy, canvas_pix, flf->fontset, font_gc, 0,
			flf->height - flf->descent, fws->str, len);
	}

	/* reserve memory for the first XImage */
	normal_data = (unsigned char *)safemalloc(normal_len * normal_h);

	/* create depth 1 XImage */
	if ((image = XCreateImage(dpy, Pvisual, 1, XYBitmap,
		0, (char *)normal_data, normal_w, normal_h, 8, 0)) == NULL)
	{
		return;
	}
	image->byte_order = image->bitmap_bit_order = MSBFirst;

	/* extract character from canvas */
	XGetSubImage(
		dpy, canvas_pix, 0, 0, normal_w, normal_h,
		1, XYPixmap, image, 0, 0);
	image->format = XYBitmap;

	/* width, height of the rotated text */
	if (fws->flags.text_rotation == TEXT_ROTATED_180)
	{
		rotated_w = normal_w;
		rotated_h = normal_h;
	}
	else /* vertical text */
	{
		rotated_w = normal_h;
		rotated_h = normal_w;
	}

	/* width in bytes */
	rotated_len = (rotated_w - 1) / 8 + 1;

	/* reserve memory for the rotated image */
	rotated_data = (unsigned char *)safecalloc(rotated_h * rotated_len, 1);

	/* create the rotated X image */
	if ((rotated_image = XCreateImage(
		dpy, Pvisual, 1, XYBitmap, 0, (char *)rotated_data,
		rotated_w, rotated_h, 8, 0)) == NULL)
	{
		return;
	}

	rotated_image->byte_order = rotated_image->bitmap_bit_order = MSBFirst;

	/* map normal text data to rotated text data */
	for (j = 0; j < rotated_h; j++)
	{
		for (i = 0; i < rotated_w; i++)
		{
			/* map bits ... */
			if (fws->flags.text_rotation == TEXT_ROTATED_270)
				val = normal_data[
					i * normal_len +
					(normal_w - j - 1) / 8
				] & (128 >> ((normal_w - j - 1) % 8));

			else if (fws->flags.text_rotation == TEXT_ROTATED_180)
				val = normal_data[
					(normal_h - j - 1) * normal_len +
					(normal_w - i - 1) / 8
				] & (128 >> ((normal_w - i - 1) % 8));

			else /* TEXT_ROTATED_90 */
				val = normal_data[
					(normal_h - i - 1) * normal_len +
					j / 8
				] & (128 >> (j % 8));

			if (val)
				rotated_data[j * rotated_len + i / 8] |=
					(128 >> (i % 8));
		}
	}

	/* create the character's bitmap  and put the image on it */
	rotated_pix = XCreatePixmap(dpy, fws->win, rotated_w, rotated_h, 1);
	XPutImage(
		dpy, rotated_pix, font_gc, rotated_image, 0, 0, 0, 0,
		rotated_w, rotated_h);

	/* free the image and data  */
	XDestroyImage(image);
	XDestroyImage(rotated_image);

	/* free pixmap and GC */
	XFreePixmap(dpy, canvas_pix);
	XFreeGC(dpy, font_gc);

	/* suitable offset: FIXME the "1" should be fws->x but ... */
	if (fws->flags.text_rotation == TEXT_ROTATED_270) /* CCW */
	{
		xpfg = 1;
		ypfg = fws->y + flf->shadow_size;
		xpsh_sign = 1;
		ypsh_sign = -1;
	}
	else if (fws->flags.text_rotation == TEXT_ROTATED_180)
	{
		xpfg = 1 + flf->shadow_size;
		ypfg = fws->y + flf->shadow_size;
		xpsh_sign = -1;
		ypsh_sign = -1;
	}
	else /* fws->flags.text_rotation == TEXT_ROTATED_90 (CW) */
	{
		xpfg = 1 + flf->shadow_size;
		ypfg = fws->y;
		xpsh_sign = -1;
		ypsh_sign = 1;
	}

	/* write the image on the window */
	XSetFillStyle(dpy, my_gc, FillStippled);
	XSetStipple(dpy, my_gc, rotated_pix);
	if (flf->shadow_size > 0 && has_fg_pixels)
	{
		XSetForeground(dpy, my_gc, fgsh);
		for(i = 1; i <= flf->shadow_size; i++)
		{
			xpsh = xpfg + (xpsh_sign*i);
			ypsh = ypfg + (ypsh_sign*i);
			XSetTSOrigin(dpy, my_gc, xpsh, ypsh);
			XFillRectangle(dpy, fws->win, my_gc, xpsh, ypsh,
				       rotated_w, rotated_h);
		}
		XSetForeground(dpy, my_gc, fg);
	}
	XSetTSOrigin(dpy, my_gc, xpfg, ypfg);
	XFillRectangle(dpy, fws->win, my_gc, xpfg, ypfg, rotated_w, rotated_h);

	XFreePixmap(dpy, rotated_pix);
	XFreeGC(dpy, my_gc);
}

static
FlocaleFont *FlocaleGetFftFont(Display *dpy, char *fontname)
{
	FftFontType *fftf = NULL;
	FlocaleFont *flf = NULL;
	char *fn, *hints = NULL;

	hints = GetQuotedString(fontname, &fn, "/", NULL, NULL, NULL);
	fftf = FftGetFont(dpy, fn);
	if (fftf == NULL)
	{
		if (fn != NULL)
			free(fn);
		return NULL;
	}
	flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
	memset(flf, '\0', sizeof(FlocaleFont));
	flf->count = 1;
	flf->fftf = *fftf;
	FlocaleCharsetSetFlocaleCharset(dpy, flf, hints);
	FftGetFontHeights(
		&flf->fftf, &flf->height, &flf->ascent, &flf->descent);
	FftGetFontWidths(
		&flf->fftf, &flf->max_char_width);
	free(fftf);
	if (fn != NULL)
		free(fn);
	return flf;
}

static
FlocaleFont *FlocaleGetFontSet(Display *dpy, char *fontname, char *module)
{
	static int mc_errors = 0;
	FlocaleFont *flf = NULL;
	XFontSet fontset = NULL;
	char **ml;
	int mc,i;
	char *ds;
	XFontSetExtents *fset_extents;
	char *fn, *hints = NULL;

	if (!FlocaleMultibyteSupport)
	{
		return NULL;
	}

	hints = GetQuotedString(fontname, &fn, "/", NULL, NULL, NULL);
	if (!(fontset = XCreateFontSet(dpy, fn, &ml, &mc, &ds)))
	{
		if (fn != NULL)
			free(fn);
		return NULL;
	}

	if (mc > 0)
	{
		if (mc_errors <= FLOCALE_NUMBER_MISS_CSET_ERR_MSG)
		{
			mc_errors++;
			fprintf(stderr,
				"[%s][FlocaleGetFontSet]: (%s)"
				" Missing font charsets:\n",
				(module)? module: "FVWM", fontname);
			for (i = 0; i < mc; i++)
			{
				fprintf(stderr, "%s", ml[i]);
				if (i < mc - 1)
					fprintf(stderr, ", ");
			}
			fprintf(stderr, "\n");
			if (mc_errors == FLOCALE_NUMBER_MISS_CSET_ERR_MSG)
			{
				fprintf(stderr,
					"[%s][FlocaleGetFontSet]:"
					" No more missing charset reportings\n",
					(module)? module: "FVWM");
			}
		}
		XFreeStringList(ml);
	}

	flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
	memset(flf, '\0', sizeof(FlocaleFont));
	flf->count = 1;
	flf->fontset = fontset;
	FlocaleCharsetSetFlocaleCharset(dpy, flf, hints);
	fset_extents = XExtentsOfFontSet(fontset);
	flf->height = fset_extents->max_logical_extent.height;
	flf->ascent = - fset_extents->max_logical_extent.y;
	flf->descent = fset_extents->max_logical_extent.height +
		fset_extents->max_logical_extent.y;
	flf->max_char_width = fset_extents->max_logical_extent.width;
	if (fn != NULL)
		free(fn);

	return flf;
}

static
FlocaleFont *FlocaleGetFont(Display *dpy, char *fontname)
{
	XFontStruct *font = NULL;
	FlocaleFont *flf;
	char *str,*fn,*tmp;
	char *hints = NULL;

	hints = GetQuotedString(fontname, &tmp, "/", NULL, NULL, NULL);
	str = GetQuotedString(tmp, &fn, ",", NULL, NULL, NULL);
	while (!font && (fn && *fn))
	{
		font = XLoadQueryFont(dpy, fn);
		if (fn != NULL)
		{
			free(fn);
			fn = NULL;
		}
		if (!font && str && *str)
		{
			str = GetQuotedString(str, &fn, ",", NULL, NULL, NULL);
		}
	}
	if (font == NULL)
	{
		if (fn != NULL)
			free(fn);
		if (tmp != NULL)
			free(tmp);
		return NULL;
	}

	flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
	memset(flf, '\0', sizeof(FlocaleFont));
	flf->count = 1;
	flf->fontset = None;
	flf->fftf.fftfont = NULL;
	flf->font = font;
	FlocaleCharsetSetFlocaleCharset(dpy, flf, hints);
	flf->height = flf->font->ascent + flf->font->descent;
	flf->ascent = flf->font->ascent;
	flf->descent = flf->font->descent;
	flf->max_char_width = flf->font->max_bounds.width;
	if (flf->font->max_byte1 > 0)
		flf->mb = True;
	else
		flf->mb = False;
	if (fn != NULL)
		free(fn);
	if (tmp != NULL)
		free(tmp);

	return flf;
}

static
FlocaleFont *FlocaleGetFontOrFontSet(
	Display *dpy, char *fontname, char *fullname, char *module)
{
	FlocaleFont *flf = NULL;

	if (fontname && strlen(fontname) > 4 &&
	    strncasecmp("xft:", fontname, 4) == 0)
	{
		if (FftSupport)
		{
			flf = FlocaleGetFftFont(dpy, fontname+4);
		}
		if (flf)
		{
			CopyString(&flf->name, fullname);
		}
		return flf;
	}
	if (FlocaleMultibyteSupport && flf == NULL && Flocale != NULL &&
	    fontname)
	{
		flf = FlocaleGetFontSet(dpy, fontname, module);
	}
	if (flf == NULL && fontname)
	{
		flf = FlocaleGetFont(dpy, fontname);
	}
	if (flf && fontname)
	{
		if (StrEquals(fullname, FLOCALE_MB_FALLBACK_FONT))
		{
			flf->name = FLOCALE_MB_FALLBACK_FONT;
		}
		else if (StrEquals(fullname, FLOCALE_FALLBACK_FONT))
		{
			flf->name = FLOCALE_FALLBACK_FONT;
		}
		else
		{
			CopyString(&flf->name, fullname);
		}
		return flf;
	}
	return NULL;
}

static
void FlocaleSetlocaleForX(
	int category, const char *locale, const char *module)
{
	FlocaleSeted = True;

	if ((Flocale = setlocale(category, locale)) == NULL)
	{
		fprintf(stderr,
			"[%s][%s]: ERROR -- Cannot set locale. Please check"
			" your $LC_CTYPE or $LANG.\n",
			(module == NULL)? "" : module, "FlocaleSetlocaleForX");
		return;
	}
	if (!XSupportsLocale())
	{
		fprintf(stderr,
			"[%s][%s]: WARNING -- X does not support locale %s\n",
			(module == NULL)? "": module, "FlocaleSetlocaleForX",
			Flocale);
		Flocale = NULL;
	}
}

/* ---------------------------- interface functions ------------------------- */

/* ***************************************************************************
 * locale initialisation
 * ***************************************************************************/

#if FlocaleMultibyteSupport

static char *Fmodifiers = NULL;

void FlocaleInit(
	int category, const char *locale, const char *modifiers,
	const char *module)
{
	if (!FlocaleMultibyteSupport)
	{
		return;
	}

	FlocaleSetlocaleForX(category, locale, module);
	if (Flocale == NULL)
		return;

	if (modifiers != NULL &&
	    (Fmodifiers = XSetLocaleModifiers(modifiers)) == NULL)
	{
		fprintf(stderr,
			"[%s][%s]: WARNING -- Cannot set locale modifiers\n",
			(module == NULL)? "": module, "FlocaleInit");
	}
#if FLOCALE_DEBUG_SETLOCALE
	fprintf(stderr,"[%s][FlocaleInit] locale: %s, modifier: %s\n",
		module, Flocale, Fmodifiers);
#endif
}

#endif /* FlocaleMultibyteSupport */

/* ***************************************************************************
 * fonts loading
 * ***************************************************************************/

FlocaleFont *FlocaleLoadFont(Display *dpy, char *fontname, char *module)
{
	FlocaleFont *flf = FlocaleFontList;
	Bool ask_default = False;
	char *t;
	char *str,*fn = NULL;
	int shadow_size = 0;

	/* removing quoting for modules */
	if (fontname && (t = strchr("\"'`", *fontname)))
	{
		char c = *t;
		fontname++;
		if (fontname[strlen(fontname)-1] == c)
			fontname[strlen(fontname)-1] = 0;
	}

	if (fontname == NULL || *fontname == 0)
	{
		ask_default = True;
		fontname = (FlocaleMultibyteSupport) ?
			FLOCALE_MB_FALLBACK_FONT : FLOCALE_FALLBACK_FONT;
	}

	while (flf)
	{
		char *c1, *c2;

		for (c1 = fontname, c2 = flf->name; *c1 && *c2; ++c1, ++c2)
		{
			if (*c1 != *c2)
			{
				break;
			}
		}
		if (!*c1 && !*c2)
		{
			flf->count++;
			return flf;
		}
		flf = flf->next;
	}

	/* not cached load the font as a ";" separated list */

	/* first see if we have a shadow relief */
	if (strlen(fontname) > 12 &&
	    strncasecmp("shadowsize=", fontname, 11) == 0)
	{
		str = GetQuotedString(fontname+11, &fn, ":", NULL, NULL, NULL);
		if (!(fn && *fn) ||
		    !GetIntegerArguments(fn, NULL,&shadow_size, 1))
		{
			shadow_size = 0;
			fprintf(stderr,"[%s][FlocaleGetFont]: WARNING -- bad "
				"shadow size description in font:\n\t'%s'\n",
				(module)? module: "FVWM", fontname);
		}
		if (fn && *fn)
		{
			str = GetQuotedString(str, &fn, ";", NULL, NULL, NULL);
		}
		if (!fn || !*fn)
		{
			fn = (FlocaleMultibyteSupport) ?
				FLOCALE_MB_FALLBACK_FONT : FLOCALE_FALLBACK_FONT;
		}
		fprintf(stderr,"SHADOW: %i, fn: '%s', rest: '%s'\n",
			shadow_size, fn, str);
	}
	else
	{
		str = GetQuotedString(fontname, &fn, ";", NULL, NULL, NULL);
	}

	while (!flf && (fn && *fn))
	{
		flf = FlocaleGetFontOrFontSet(dpy, fn, fontname, module);
		if (fn != NULL && fn != FLOCALE_MB_FALLBACK_FONT &&
		    fn != FLOCALE_FALLBACK_FONT)
		{
			free(fn);
			fn = NULL;
		}
		if (!flf && str && *str)
		{
			str = GetQuotedString(str, &fn, ";", NULL, NULL, NULL);
		}
	}
	if (fn != NULL && fn != FLOCALE_MB_FALLBACK_FONT &&
	    fn != FLOCALE_FALLBACK_FONT)
	{
		free(fn);
	}

	if (flf == NULL)
	{
		/* loading failed, try default font */
		if (!ask_default)
		{
			fprintf(stderr,"[%s][FlocaleGetFont]: "
				"WARNING -- can't load font '%s',"
				" trying default:\n",
				(module)? module: "FVWM", fontname);
		}
		else
		{
			/* we already tried default fonts: try again? yes */
		}
		if (FlocaleMultibyteSupport && Flocale != NULL)
		{
			if (!ask_default)
			{
				fprintf(stderr, "\t%s\n",
					FLOCALE_MB_FALLBACK_FONT);
			}
			if ((flf = FlocaleGetFontSet(
				     dpy, FLOCALE_MB_FALLBACK_FONT, module)) !=
			    NULL)
			{
				flf->name = FLOCALE_MB_FALLBACK_FONT;
			}
		}
		if (flf == NULL)
		{
			if (!ask_default)
			{
				fprintf(stderr,"\t%s\n",
					FLOCALE_FALLBACK_FONT);
			}
			if ((flf = FlocaleGetFont(
				     dpy, FLOCALE_FALLBACK_FONT)) != NULL)
			{
				flf->name = FLOCALE_FALLBACK_FONT;
			}
			else if (!ask_default)
			{
				fprintf(stderr,
					"[%s][FlocaleLoadFont]:"
					" ERROR -- can't load font.\n",
					(module)? module: "FVWM");
			}
			else
			{
				fprintf(stderr,
					"[%s][FlocaleLoadFont]: ERROR"
					" -- can't load default font:\n",
					(module)? module: "FVWM");
				if (FlocaleMultibyteSupport)
				{
					fprintf(stderr, "\t%s\n",
						FLOCALE_MB_FALLBACK_FONT);
				}
				fprintf(stderr, "\t%s\n",
					FLOCALE_FALLBACK_FONT);
			}
		}
	}
	
	if (flf != NULL)
	{
		flf->shadow_size = shadow_size;
		flf->height += shadow_size;
		/* add the shadow to ascent or descent ? */
		flf->descent += shadow_size;
		flf->max_char_width += shadow_size;
		if (flf->fc == FlocaleCharsetGetUnknownCharset())
		{
			fprintf(stderr,"[%s][FlocaleLoadFont]: "
				"WARNING -- Unkown charset for font\n\t'%s'\n",
				(module)? module: "FVWM", flf->name);
		}
		flf->next = FlocaleFontList;
		FlocaleFontList = flf;
	}

	return flf;
}

void FlocaleUnloadFont(Display *dpy, FlocaleFont *flf)
{
	FlocaleFont *list = FlocaleFontList;
	int i = 0;

	if (!flf)
	{
		return;
	}
	/* Remove a weight, still too heavy? */
	if (--(flf->count) > 0)
	{
		return;
	}

	if (flf->name != NULL &&
		!StrEquals(flf->name, FLOCALE_MB_FALLBACK_FONT) &&
		!StrEquals(flf->name, FLOCALE_FALLBACK_FONT))
	{
		free(flf->name);
	}
	if (FftSupport && flf->fftf.fftfont != NULL)
	{
		FftFontClose(dpy, flf->fftf.fftfont);
		if (flf->fftf.fftfont_rotated_90 != NULL)
			FftFontClose(dpy, flf->fftf.fftfont_rotated_90);
		if (flf->fftf.fftfont_rotated_180 != NULL)
			FftFontClose(dpy, flf->fftf.fftfont_rotated_180);
		if (flf->fftf.fftfont_rotated_270 != NULL)
			FftFontClose(dpy, flf->fftf.fftfont_rotated_270);
	}
	if (FlocaleMultibyteSupport && flf->fontset != NULL)
	{
		XFreeFontSet(dpy, flf->fontset);
	}
	if (flf->font != NULL)
	{
		XFreeFont(dpy, flf->font);
	}
	if (flf->must_free_fc)
	{
		if (flf->fc->x)
			free(flf->fc->x);
		if (flf->fc->bidi)
			free(flf->fc->bidi);
		if (flf->fc->locale != NULL)
		{
			while(FLC_GET_LOCALE_CHARSET(flf->fc,i) != NULL)
			{
				free(FLC_GET_LOCALE_CHARSET(flf->fc,i));
				i++;
			}
			free(flf->fc->locale);
		}
		free(flf->fc);
	}

	/* Link it out of the list (it might not be there) */
	if (flf == list) /* in head? simple */
	{
		FlocaleFontList = flf->next;
	}
	else
	{
		while (list && list->next != flf)
		{
			/* fast forward until end or found */
			list = list->next;
		}
		/* not end? means we found it in there, possibly at end */
		if (list)
		{
			/* link around it */
			list->next = flf->next;
		}
	}
	free(flf);
}

/* ***************************************************************************
 * Width and Drawing Text
 * ***************************************************************************/
void FlocaleDrawString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws,
	unsigned long flags)
{
	int len, i;
	char *str1 = NULL;  /* if Bidi used: original string */
	char *str2 = NULL;  /* if Bidi used: converted string */
	Bool is_rtl;
	const char *bidi_charset;
	Pixel fg = 0, fgsh = 0;
	Bool has_fg_pixels = False;

	is_rtl = False;
	if (!fws || !fws->str)
	{
		return;
	}

	if (flags & FWS_HAVE_LENGTH)
	{
		len = fws->len;
	}
	else
	{
		len = strlen(fws->str);
	}

	/* check whether we should apply Bidi filter to text */
	bidi_charset = FlocaleGetBidiCharset(dpy, flf);
	if (bidi_charset)
	{
		str2 = FBidiConvert(fws->str, bidi_charset, &is_rtl);
	}
	if (str2)
	{
		str1 = fws->str;
		fws->str = str2;
	}

	/* get the pixels */
	if (fws->flags.has_colorset)
	{
		fg = fws->colorset->fg;
		fgsh = fws->colorset->fgsh;
		has_fg_pixels = True;
	}
	else if (fws->flags.has_fore_colors)
	{
		fg = fws->fg;
		fgsh = fws->fgsh;
		has_fg_pixels = True;
	}
	fprintf(stderr,"PIXEL %lx,%lx,%i,%i\n", fg,fgsh, has_fg_pixels,
		fws->flags.has_colorset);
	if (fws->flags.text_rotation != TEXT_ROTATED_0 &&
	    flf->fftf.fftfont == NULL)
	{
		FlocaleRotateDrawString(
			    dpy, flf, fws, fg, fgsh, has_fg_pixels, len);
	}
	else if (FftSupport && flf->fftf.fftfont != NULL)
	{
		FftDrawString(
			    dpy, flf, fws, fg, fgsh, has_fg_pixels, len, flags);
	}
	else if (FlocaleMultibyteSupport && flf->fontset != None)
	{
		if (flf->shadow_size > 0 && has_fg_pixels)
		{
			    XSetForeground(dpy, fws->gc, fgsh);
			    for(i=1; i <= flf->shadow_size; i++)
			    {
				    XmbDrawString(
					  dpy, fws->win, flf->fontset, fws->gc,
					  fws->x + i, fws->y + i, fws->str, len);
			    }
			    XSetForeground(dpy, fws->gc, fg);
		}
		XmbDrawString(
			dpy, fws->win, flf->fontset, fws->gc,
			fws->x, fws->y, fws->str, len);
	}
	else if (flf->font != None)
	{
		FlocaleFontStructDrawString(dpy, flf, fws->win, fws->gc,
					    fws->x, fws->y,
					    fg, fgsh, has_fg_pixels,
					    fws, len, False);
	}

	if (str2)
	{
		fws->str = str1;
		free(str2);
	}

	return;
}

int FlocaleTextWidth(FlocaleFont *flf, char *str, int sl)
{
	int result = 0;

	if (!str || sl == 0)
		return 0;

	if (sl < 0)
	{
		/* a vertical string: nothing to do! */
		sl = -sl;
	}
	if (FftSupport && flf->fftf.fftfont != NULL)
	{
		result = FftTextWidth(&flf->fftf, str, sl);
	}
	else if (FlocaleMultibyteSupport && flf->fontset != None)
	{
		result = XmbTextEscapement(flf->fontset, str, sl);
	}
	else if (flf->font != None)
	{
		if (flf->utf8 || flf->mb)
		{
			XChar2b *str2b;
			int nl;

			if (flf->utf8)
				str2b = FlocaleUtf8ToUnicodeStr2b(str, sl, &nl);
			else
				str2b = FlocaleStringToString2b(str, sl, &nl);
			if (str2b != NULL)
			{
				result = XTextWidth16(flf->font, str2b, nl);
				free(str2b);
			}
		}
		else
		{
			result = XTextWidth(flf->font, str, sl);
		}
	}

	return result + flf->shadow_size;
}

void FlocaleAllocateWinString(FlocaleWinString **pfws)
{
	*pfws = (FlocaleWinString *)safemalloc(sizeof(FlocaleWinString));
	memset(*pfws, '\0', sizeof(FlocaleWinString));
}

/* ***************************************************************************
 * Text properties
 * ***************************************************************************/
void FlocaleGetNameProperty(
	Status (func)(Display *, Window, XTextProperty *), Display *dpy,
	Window w, FlocaleNameString *ret_name)
{
	char **list;
	int num;
	XTextProperty text_prop;

	if (func(dpy, w, &text_prop) == 0 ||
	    (FlocaleCompoundText && !text_prop.value))
	{
		return;
	}

	if (FlocaleMultibyteSupport && text_prop.encoding == XA_STRING)
	{
		/* STRING encoding, use this as it is */
		ret_name->name = (char *)text_prop.value;
		ret_name->name_list = NULL;
		return;
	}
	else if (FlocaleCompoundText && text_prop.encoding == XA_STRING)
	{
		/* STRING encoding, use this as it is */
		CopyString(&(ret_name->name), (char *)text_prop.value);
		XFree(text_prop.value);
		return;
	}
	else if (!FlocaleMultibyteSupport && !FlocaleCompoundText)
	{
		ret_name->name = (char *)text_prop.value;
		return;
	}
	/* not STRING encoding, try to convert */
	if (FlocaleCompoundText && !FlocaleSeted)
	{
		FlocaleSetlocaleForX(LC_CTYPE, "", "fvwmlibs");
	}
	if (XmbTextPropertyToTextList(dpy, &text_prop, &list, &num) >= Success
	    && num > 0 && *list)
	{
		/* XXX: does not consider the conversion is REALLY succeeded */
		XFree(text_prop.value); /* return of XGetWM(Icon)Name() */
		if (FlocaleCompoundText)
		{
			CopyString(&(ret_name->name), *list);
			if (list)
			{
				XFreeStringList(list);
			}
		}
		else
		{
			ret_name->name = *list;
			ret_name->name_list = list;
		}
	}
	else
	{
		if (list)
		{
			XFreeStringList(list);
		}
		if (FlocaleCompoundText)
		{
			CopyString(&(ret_name->name), (char *)text_prop.value);
			XFree(text_prop.value);
		}
		else if (FlocaleMultibyteSupport)
		{
			ret_name->name = (char *)text_prop.value;
			ret_name->name_list = NULL;
		}
	}
}

void FlocaleFreeNameProperty(FlocaleNameString *ptext)
{
	if (FlocaleMultibyteSupport && ptext->name_list != NULL)
	{
		if (ptext->name != NULL && ptext->name != *ptext->name_list)
			XFree(ptext->name);
		XFreeStringList(ptext->name_list);
		ptext->name_list = NULL;
	}
	else if (FlocaleCompoundText && ptext->name != NULL)
	{
		free(ptext->name); /* this is not reall needed IMHO we
				    * can XFree I think (olicha) */
	}
	else if (ptext->name != NULL)
	{
		XFree(ptext->name);
	}
	ptext->name = NULL;

	return;
}

Bool FlocaleTextListToTextProperty(
	Display *dpy, char **list, int count, XICCEncodingStyle style,
	XTextProperty *text_prop_return)
{
	int ret = False;

	if (FlocaleMultibyteSupport && Flocale != NULL)
	{
		ret = XmbTextListToTextProperty(
			dpy, list, count, style, text_prop_return);
		if (ret == XNoMemory)
		{
			ret = False;
		}
		else
		{
			/* ret == Success or the number of unconvertible
			 * characters. ret should be != XLocaleNotSupported
			 * because in this case Flocale == NULL */
			ret = True;
		}
	}
	if (!ret)
	{
		if (XStringListToTextProperty(
			    list, count, text_prop_return) == 0)
		{
			ret = False;
		}
		else
		{
			ret = True;
		}
	}

	return ret;
}

