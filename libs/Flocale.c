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
#include "libs/fvwmlib.h"
#include "safemalloc.h"
#include "Strings.h"
#include "Parse.h"
#include "Picture.h"
#include "Flocale.h"
#include "FBidi.h"

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
static char *Fmodifiers = NULL;
static FlocaleCharset UnsetCharset = {"Unset", NULL, -2, NULL};


/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */


static
FlocaleFont *FlocaleGetFftFont(Display *dpy, char *fontname)
{
	FftFontType *fftf = NULL;
	FlocaleFont *flf = NULL;

	fftf = FftGetFont(dpy, fontname);
	if (fftf == NULL)
	{
		return NULL;
	}
	flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
	flf->count = 1;
	flf->fftf = *fftf;
	flf->font = NULL;
	flf->fontset = None;
	flf->fc =  &UnsetCharset;
	FftGetFontHeights(
		&flf->fftf, &flf->height, &flf->ascent, &flf->descent);
	FftGetFontWidths(
		&flf->fftf, &flf->max_char_width, &flf->min_char_offset);
	free(fftf);

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

	if (!FlocaleMultibyteSupport)
	{
		return NULL;
	}
	if (!(fontset = XCreateFontSet(dpy, fontname, &ml, &mc, &ds)))
	{
		return NULL;
	}

	if (mc > 0)
	{
		if (mc_errors <= FLOCALE_NUMBER_MISS_CSET_ERR_MSG)
		{
			mc_errors++;
			fprintf(stderr,
				"[%s][FlocaleGetFontSet][%s]:"
				"The following charsets are missing:\n",
				fontname, (module)? module:"FVWM");
			if (mc_errors >= FLOCALE_NUMBER_MISS_CSET_ERR_MSG)
			{
				fprintf(stderr, "\tNo more such error will be"
					" reported");
			}
		}
		for (i=0; i < mc; i++)
		{
			fprintf(stderr, " %s", ml[i]);
		}
		fprintf(stderr, "\n");
		if (mc_errors >= FLOCALE_NUMBER_MISS_CSET_ERR_MSG)
		{
			fprintf(stderr,
				"\tNo more such error will be reported");
		}
		XFreeStringList(ml);
	}

	flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
	flf->count = 1;
	flf->fontset = fontset;
	flf->font = NULL;
	flf->fftf.fftfont = NULL;
	flf->fc = &UnsetCharset;
	fset_extents = XExtentsOfFontSet(fontset);
	flf->height = fset_extents->max_logical_extent.height;
	flf->ascent = - fset_extents->max_logical_extent.y;
	flf->descent = fset_extents->max_logical_extent.height +
		fset_extents->max_logical_extent.y;
	flf->max_char_width = fset_extents->max_logical_extent.width;
	flf->min_char_offset = fset_extents->max_ink_extent.x;

	return flf;
}

static
FlocaleFont *FlocaleGetFont(Display *dpy, char *fontname)
{
	XFontStruct *font = NULL;
	FlocaleFont *flf;
	char *str,*fn;

	str = GetQuotedString(fontname, &fn, ",", NULL, NULL, NULL);
	while (!font && (fn && *fn))
	{
		font = XLoadQueryFont(dpy, fn);
		if (fn != NULL)
		{
			free(fn);
			fn = NULL;
		}
		if (!font)
		{
			str = GetQuotedString(str, &fn, ",", NULL, NULL, NULL);
		}
	}
	if (fn != NULL)
	{
		free(fn);
	}
	if (font == NULL)
	{
		return NULL;
	}

	flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
	flf->count = 1;
	if (FlocaleMultibyteSupport)
	{
		flf->fontset = None;
	}
	if (FftSupport)
	{
		flf->fftf.fftfont = NULL;
	}
	flf->font = font;
	flf->fc = &UnsetCharset;
	flf->height = flf->font->ascent + flf->font->descent;
	flf->ascent = flf->font->ascent;
	flf->descent = flf->font->descent;
	flf->max_char_width = flf->font->max_bounds.width;
	flf->min_char_offset = flf->font->min_bounds.lbearing;

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
void FlocaleRotateDrawString(Display *dpy, FlocaleFont *flf,
			     FlocaleWinString *fws, int len)
{
	static GC my_gc, font_gc;
	int j, i, xp, yp;
	unsigned char *vertdata, *bitdata;
	int vert_w, vert_h, vert_len, bit_w, bit_h, bit_len;
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
	width = FlocaleTextWidth(flf, fws->str, len);
	height = flf->height;

	if (width < 1) width = 1;
	if (height < 1) height = 1;

	/* glyph width and height when vertical */
	vert_w = width;
	vert_h = height;

	/* width in bytes */
	vert_len = (vert_w-1)/8+1;

	/* create and clear the canvas */
	canvas_pix = XCreatePixmap(dpy, fws->win, width, height, 1);
	font_gc = fvwmlib_XCreateGC(dpy, canvas_pix, 0, NULL);
	XSetBackground(dpy, font_gc, 0);
	XSetForeground(dpy, font_gc, 0);
	XFillRectangle(dpy, canvas_pix, font_gc, 0, 0, width, height);

	/* draw the character centre top right on canvas .*/
	XSetForeground(dpy, font_gc, 1);
	if (flf->font != NULL)
	{
		XSetFont(dpy, font_gc, flf->font->fid);
		XDrawImageString(dpy, canvas_pix, font_gc,
				 0, flf->height - flf->descent,
				 fws->str, len);
	}
	else if (FlocaleMultibyteSupport && flf->fontset != None)
	{
		XmbDrawString(dpy, canvas_pix, flf->fontset, font_gc,
			      0, flf->height - flf->descent,
			      fws->str, len);
	}

	/* reserve memory for first XImage */
	vertdata = (unsigned char *) safemalloc((unsigned)(vert_len*vert_h));

	/* create depth 1 XImage */
	if ((image = XCreateImage(dpy, Pvisual, 1, XYBitmap,
		    0, (char *)vertdata, vert_w, vert_h, 8, 0)) == NULL)
	{
		return;
	}
	image->byte_order = image->bitmap_bit_order = MSBFirst;

	/* extract character from canvas ... */
	XGetSubImage(dpy, canvas_pix, 0, 0, vert_w, vert_h,
		     1, XYPixmap, image, 0, 0);
	image->format = XYBitmap;

	/* width, height of rotated character ... */
	if (fws->flags.text_rotation == TEXT_ROTATED_180)
	{
		bit_w = vert_w;
		bit_h = vert_h;
	}
	else /* vertical text */
	{
		bit_w = vert_h;
		bit_h = vert_w;
	}

	/* width in bytes ... */
	bit_len = (bit_w-1)/8 + 1;

	/* reserve memory for the rotated image ... */
	bitdata = (unsigned char *)safecalloc((unsigned)(bit_h*bit_len), 1);

	/* create the rotated X image ... */
	if ((rotated_image = XCreateImage(dpy, Pvisual, 1, XYBitmap, 0,
					  (char *)bitdata,
					  bit_w, bit_h, 8, 0)) == NULL)
	{
		return;
	}

	rotated_image->byte_order = rotated_image->bitmap_bit_order = MSBFirst;

	/* map vertical data to rotated character ... */
	for (j = 0; j < bit_h; j++)
	{
		for (i = 0; i < bit_w; i++)
		{
			/* map bits ... */
			if (fws->flags.text_rotation == TEXT_ROTATED_270)
				val = vertdata[i*vert_len + (vert_w-j-1)/8] &
					(128>>((vert_w-j-1)%8));

			else if (fws->flags.text_rotation == TEXT_ROTATED_180)
				val = vertdata[
					(vert_h-j-1)*vert_len +
					(vert_w-i-1)/8] &
					(128>>((vert_w-i-1)%8));

			else /* TEXT_ROTATED_90 */
				val = vertdata[(vert_h-i-1)*vert_len + j/8] &
					(128>>(j%8));

			if (val)
				bitdata[j*bit_len + i/8] =
					bitdata[j*bit_len + i/8] |
					(128>>(i%8));
		}
	}

	/* create the character's bitmap  and put the image on it */
	rotated_pix = XCreatePixmap(dpy, fws->win, bit_w, bit_h, 1);
	XPutImage(dpy, rotated_pix, font_gc, rotated_image,
		  0, 0, 0, 0, bit_w, bit_h);

	/* free the image and data  */
	XDestroyImage(image);
	XDestroyImage(rotated_image);

	/* free pixmap and GC ... */
	XFreePixmap(dpy, canvas_pix);
	XFreeGC(dpy, font_gc);

	/* suitable offset: FIXME */
	if (fws->flags.text_rotation == TEXT_ROTATED_270)
	{
		xp = 1;
		yp = fws->y;
	}
	else if (fws->flags.text_rotation == TEXT_ROTATED_180)
	{
		/* not used and not tested */
		xp = 0;
		yp = fws->y;
	}
	else /* fws->flags.text_rotation == TEXT_ROTATED_90 */
	{
		xp = 1;
		yp = fws->y;
	}

	/* write the image on the window */
	XSetFillStyle(dpy, my_gc, FillStippled);
	XSetStipple(dpy, my_gc, rotated_pix);
	XSetTSOrigin(dpy, my_gc, xp, yp);
	XFillRectangle(dpy, fws->win, my_gc, xp, yp, bit_w, bit_h);

	XFreePixmap(dpy, rotated_pix);
	XFreeGC(dpy, my_gc);
}

static
void FlocaleSetlocaleForX(int category,
			  const char *locale,
			  const char *module
			  )
{
	FlocaleSeted = True;

	if ((Flocale = setlocale(category, locale)) == NULL)
	{
		fprintf(stderr,
			"[%s][%s]: ERROR -- Cannot set locale. Please check"
			" your $LC_CTYPE or $LANG.\n",
			(module == NULL)? "" : module, "FInitLocale");
		return;
	}
	if (!XSupportsLocale())
	{
		fprintf(stderr,
			"[%s][%s]: WARNING -- X does not support locale %s\n",
			(module == NULL)? "":module, "FInitLocale",Flocale);
		Flocale = NULL;
	}
}

/* ---------------------------- interface functions ------------------------- */

/* ***************************************************************************
 * locale initialisation
 * ***************************************************************************/

void FlocaleInit(int category, const char *locale, const char *modifiers,
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
			(module == NULL)? "":module, "FInitLocale");
	}
#if FLOCALE_DEBUG_SETLOCALE
	fprintf(stderr,"[%s][FlocaleInit] locale: %s, modifier: %s\n",
		module, Flocale, Fmodifiers);
#endif
}

/* ***************************************************************************
 * fonts loading
 * ***************************************************************************/

FlocaleFont *FlocaleLoadFont(Display *dpy, char *fontname, char *module)
{
	FlocaleFont *flf = FlocaleFontList;
	Bool ask_default = False;
	char *t;
	char *str,*fn = NULL;

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

	/* not cached load the font as a ; sperated list */
	str = GetQuotedString(fontname, &fn, ";", NULL, NULL, NULL);
	while (!flf && (fn && *fn))
	{
		flf = FlocaleGetFontOrFontSet(dpy, fn, fontname, module);
		if (fn != NULL)
		{
			free(fn);
			fn = NULL;
		}
		if (!flf)
		{
			str = GetQuotedString(str, &fn, ";", NULL, NULL, NULL);
		}
	}
	if (fn != NULL)
	{
		free(fn);
	}

	if (flf == NULL)
	{
		/* loading fail try default font */
		if (!ask_default)
		{
			fprintf(stderr,"[%s][FlocaleGetFont]: "
				"WARNING -- can't get font '%s',"
				" try to load default font:\n",
				(module)? module:"FVWM", fontname);
		}
		else
		{
			/* we already try default fonts: try again? yes */
		}
		if (FlocaleMultibyteSupport && Flocale != NULL)
		{
			if (!ask_default)
			{
				fprintf(stderr, "\t %s\n",
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
				fprintf(stderr,"\t %s\n",
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
					" ERROR -- can't get font\n",
					(module)? module:"FVWM");
			}
			else
			{
				fprintf(stderr,
					"[%s][FlocaleLoadFont]: ERROR"
					" -- can't get default font:\n",
					(module)? module:"FVWM");
				if (FlocaleMultibyteSupport)
				{
					fprintf(stderr,"\t %s\n",
						FLOCALE_MB_FALLBACK_FONT);
				}
				fprintf(stderr,"\t %s\n",
					FLOCALE_FALLBACK_FONT);
			}
		}
	}
	if (flf != NULL)
	{
		flf->next = FlocaleFontList;
		FlocaleFontList = flf;
	}

	return flf;
}

void FlocaleUnloadFont(Display *dpy, FlocaleFont *flf)
{
	FlocaleFont *list = FlocaleFontList;

	if (!flf)
	{
		return;
	}
	if (--(flf->count)>0) /* Remove a weight, still too heavy? */
	{
		return;
	}

	if (flf->name != NULL &&
	   !StrEquals(flf->name,FLOCALE_MB_FALLBACK_FONT) &&
	   !StrEquals(flf->name,FLOCALE_FALLBACK_FONT))
	{
		free(flf->name);
	}
	if (FftSupport && flf->fftf.fftfont != NULL)
	{
		FftFontClose(dpy, flf->fftf.fftfont);
		if (flf->fftf.tb_fftfont != NULL)
			FftFontClose(dpy, flf->fftf.tb_fftfont);
		if (flf->fftf.bt_fftfont != NULL)
			FftFontClose(dpy, flf->fftf.bt_fftfont);
	}
	if (FlocaleMultibyteSupport && flf->fontset != NULL)
	{
		XFreeFontSet(dpy, flf->fontset);
	}
	if (flf->font != NULL)
	{
		XFreeFont(dpy, flf->font);
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
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fstring,
	unsigned long flags)
{
	int len;
	char *str1 = NULL, *str2;
	Bool is_rtl;

	is_rtl = False;
	if (!fstring || !fstring->str)
	{
		return;
	}

	if (flags & FWS_HAVE_LENGTH)
	{
		len = fstring->len;
	}
	else
	{
		len = strlen(fstring->str);
	}

	/* TO FIX: should use charset from font structure */
	str2 = FBidiConvert(dpy, fstring->str, flf, &is_rtl);
	if (str2)
	{
		str1 = fstring->str;
		fstring->str = str2;
	}

	if (fstring->flags.text_rotation != TEXT_ROTATED_0 &&
	    flf->fftf.fftfont == NULL)
	{
		FlocaleRotateDrawString(dpy, flf, fstring, len);
	}
	else if (FftSupport && flf->fftf.fftfont != NULL)
	{
		int x,y;
		switch(fstring->flags.text_rotation)
		{
		case TEXT_ROTATED_270:
			y = fstring->y +
				FftTextWidth(&flf->fftf, fstring->str, len);
			x = fstring->x;
			break;
		case TEXT_ROTATED_90:
		default:
			y = fstring->y;
			x = fstring->x;
			break;
		}
		FftDrawString(
			dpy, fstring->win, &flf->fftf, fstring->gc, x, y,
			fstring->str, len, fstring->flags.text_rotation);
	}
	else if (FlocaleMultibyteSupport && flf->fontset != None)
	{
		XmbDrawString(dpy, fstring->win, flf->fontset,
			      fstring->gc, fstring->x, fstring->y,
			      fstring->str, len);
	}
	else if (flf->font != None)
	{
		XDrawString(dpy, fstring->win, fstring->gc, fstring->x,
			    fstring->y, fstring->str, len);
	}

	if (str2)
	{
		fstring->str = str1;
		free(str2);
	}

	return;
}

int FlocaleTextWidth(FlocaleFont *flf, char *str, int sl)
{
	if (!str || sl == 0)
		return 0;

	if (sl < 0)
	{
		/* a vertical string: nothing to do! */
		sl = -sl;
	}
	if (FftSupport && flf->fftf.fftfont != NULL)
	{
		return FftTextWidth(&flf->fftf, str, sl);
	}
	if (FlocaleMultibyteSupport && flf->fontset != None)
	{
		return XmbTextEscapement(flf->fontset, str, sl);
	}
	if (flf->font != None)
	{
		return XTextWidth(flf->font, str, sl);
	}

	return 0;
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

/* ***************************************************************************
 * misc
 * ***************************************************************************/
FlocaleCharset *FlocaleGetUnsetCharset(void)
{
	return &UnsetCharset;
}
