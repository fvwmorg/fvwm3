/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */
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
 * Simplification and complications by olicha for use with fvwm
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "defaults.h"
#include "fvwmlib.h"
#include "Graphics.h"
#include "ColorUtils.h"
#include "Strings.h"
#include "Parse.h"
#include "PictureBase.h"
#include "Flocale.h"
#include "FlocaleCharset.h"
#include "FBidi.h"
#include "FftInterface.h"
#include "Colorset.h"
#include "Ficonv.h"
#include "CombineChars.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

#define FSwitchDrawString(use_16, dpy, d, gc, x, y, s8, s16, l) \
	(use_16) ? \
	XDrawString16(dpy, d, gc, x, y, s16, l) : \
	XDrawString(dpy, d, gc, x, y, s8, l)
#define FSwitchDrawImageString(use_16, dpy, d, gc, x, y, s8, s16, l) \
	(use_16) ? \
	XDrawImageString16(dpy, d, gc, x, y, s16, l) : \
	XDrawImageString(dpy, d, gc, x, y, s8, l)

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static FlocaleFont *FlocaleFontList = NULL;
static char *Flocale = NULL;
static char *Fmodifiers = NULL;

/* TODO: make these (static const char *) */
static char *fft_fallback_font = FLOCALE_FFT_FALLBACK_FONT;
static char *mb_fallback_font = FLOCALE_MB_FALLBACK_FONT;
static char *fallback_font = FLOCALE_FALLBACK_FONT;


/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/*
 * shadow local functions
 */

static
void FlocaleParseShadow(char *str, int *shadow_size, int *shadow_offset,
			int *direction, char * fontname, char *module)
{
	char *dir_str;
	char *token;
	multi_direction_t dir;

	*direction = MULTI_DIR_NONE;
	token = PeekToken(str, &dir_str);
	if (token == NULL || *token == 0 ||
	    (GetIntegerArguments(token, NULL, shadow_size, 1) != 1) ||
	    *shadow_size < 0)
	{
		*shadow_size = 0;
		fprintf(stderr,"[%s][FlocaleParseShadow]: WARNING -- bad "
			"shadow size in font name:\n\t'%s'\n",
			(module)? module: "fvwm", fontname);
		return;
	}
	if (*shadow_size == 0)
	{
		return;
	}
	/* some offset ? */
	if (dir_str && *dir_str &&
	    (GetIntegerArguments(dir_str, NULL, shadow_offset, 1) == 1))
	{
		if (*shadow_offset < 0)
		{
			*shadow_offset = 0;
			fprintf(stderr,"[%s][FlocaleParseShadow]: WARNING -- "
				"bad shadow offset in font name:\n\t'%s'\n",
				(module)? module: "fvwmlibs", fontname);
		}
		PeekToken(dir_str, &dir_str);
	}
	while (dir_str && *dir_str && *dir_str != '\n')
	{
		dir = gravity_parse_multi_dir_argument(dir_str, &dir_str);
		if (dir == MULTI_DIR_NONE)
		{
			fprintf(stderr,"[%s][FlocaleParseShadow]: WARNING -- "
				"bad shadow direction in font description:\n"
				"\t%s\n",
				(module)? module: "fvwmlibs", fontname);
			PeekToken(dir_str, &dir_str); /* skip it */
		}
		else
		{
			*direction |= dir;
		}
	}
	if (*direction == MULTI_DIR_NONE)
		*direction = MULTI_DIR_SE;
}

/*
 * some simple converters
 */

static
int FlocaleChar2bOneCharToUtf8(XChar2b c, char *buf)
{
        int len;
	char byte1 = c.byte1;
	char byte2 = c.byte2;
	unsigned short ucs2 = ((unsigned short)byte1 << 8) + byte2;

        if(ucs2 <= 0x7f)
	{
	        len = 1;
	        buf[0] = (char)ucs2;
		buf[1] = 0;
	}
	else if(ucs2 <= 0x7ff)
	{
	        len = 2;
	        buf[0] = (ucs2 >> 6) | 0xc0;
		buf[1] = (ucs2 & 0x3f) | 0x80;
		buf[2] = 0;
	}
	else
	{
	        len = 3;
		buf[0] = (ucs2 >> 12) | 0xe0;
		buf[1] = ((ucs2 & 0xfff) >> 6) | 0x80;
		buf[2] = (ucs2 & 0x3f) | 0x80;
		buf[3] = 0;
	}
	return len;
}

/* return number of bytes of character at current position
   (pointed to by str) */
int FlocaleStringNumberOfBytes(FlocaleFont *flf, const char *str)
{
        int bytes = 0;
        if(FLC_ENCODING_TYPE_IS_UTF_8(flf->fc))
	{
	        /* handle UTF-8 */
	        if ((str[0] & 0x80) == 0)
	        {
		        bytes = 1;
	        }
		else if((str[0] & ~0xdf) == 0)
		{
		        bytes = 2;
		}
		else
		{
		        /* this handles only 16-bit Unicode */
		        bytes = 3;
		}
	}
	else if(flf->flags.is_mb)
	{
	        /* non-UTF-8 multibyte encoding */
	        if ((str[0] & 0x80) == 0)
		{
			bytes = 1;
		}
		else
		{
			bytes = 2;
		}
	}
	else
	{
	        /* we must be using an "ordinary" 8-bit encoding */
	        bytes = 1;
	}
	return bytes;
}

/* given a string, font specifying its locale and a byte offset gives
   character offset */
int FlocaleStringByteToCharOffset(FlocaleFont *flf, const char *str,
				  int offset)
{
	const char *curr_ptr = str;
	int i = 0;
	int len = strlen(str);
	int coffset = 0;
	int curr_len;

	for(i = 0 ;
	    i < offset && i < len ;
	    i += curr_len, curr_ptr += curr_len, coffset++)
	{
		curr_len = FlocaleStringNumberOfBytes(flf, curr_ptr);
	}
	return coffset;
}

/* like above but reversed, ie. return byte offset corresponding to given
   charater offset */
int FlocaleStringCharToByteOffset(FlocaleFont *flf, const char *str,
				  int coffset)
{
	const char *curr_ptr = str;
	int i;
	int len = strlen(str);
	int offset = 0;
	int curr_len;

	for(i = 0 ;
	    i < coffset && i < len ;
	    offset += curr_len, curr_ptr += curr_len, i++)
	{
		curr_len = FlocaleStringNumberOfBytes(flf, curr_ptr);
	}
	return offset;
}

/* return length of string in characters */
int FlocaleStringCharLength(FlocaleFont *flf, const char *str)
{
	int i, len;
	int str_len = strlen(str);
	for(i = 0, len = 0 ; i < str_len ;
	    i += FlocaleStringNumberOfBytes(flf, str+i), len++);
	return len;
}

static
XChar2b *FlocaleUtf8ToUnicodeStr2b(char *str, int len, int *nl)
{
	XChar2b *str2b = NULL;
	int i = 0, j = 0, t;

	str2b = xmalloc((len + 1) * sizeof(XChar2b));
	while (i < len && str[i] != 0)
	{
		if ((str[i] & 0x80) == 0)
		{
			str2b[j].byte2 = str[i];
			str2b[j].byte1 = 0;
		}
		else if ((str[i] & ~0xdf) == 0 && i+1 < len)
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

/* Note: this function is not expected to work; good mb rendering
 * should be (and is) done using Xmb functions and not XDrawString16
 * (or with iso10646-1 fonts and setting the encoding).
 * This function is used when the locale does not correspond to the font.
 * It works with  "EUC fonts": ksc5601.1987-0, gb2312 and maybe also
 * cns11643-*.  It works patially with jisx* and big5-0. Should try gbk-0,
 * big5hkscs-0, and cns-11643- */
static
XChar2b *FlocaleStringToString2b(
	Display *dpy, FlocaleFont *flf, char *str, int len, int *nl)
{
	XChar2b *str2b = NULL;
	char *tmp = NULL;
	Bool free_str = False;
	int i = 0, j = 0;
	Bool euc = True; /* KSC5601 (EUC-KR), GB2312 (EUC-CN), CNS11643-1986-1
			  * (EUC-TW) and converted  jisx (EUC-JP) */

	if (flf->fc && StrEquals(flf->fc->x,"jisx0208.1983-0"))
	{
		tmp = FiconvCharsetToCharset(
			dpy, flf->fc, FlocaleCharsetGetEUCJPCharset(), str,
			len);
		if (tmp != NULL)
		{
			free_str = True;
			str = tmp;
			len = strlen(tmp);
		}
	}
	else if (flf->fc && StrEquals(flf->fc->x,"big5-0"))
	{
		euc = False;
	}
	str2b = xmalloc((len + 1) * sizeof(XChar2b));
	if (euc)
	{
		while (i < len && str[i] != 0)
		{
			if ((str[i] & 0x80) == 0)
			{
				/* seems ok with KSC5601 and GB2312 as we get
				 * almost the ascii. I do no try
				 * CNS11643-1986-1.  Should convert to ascii
				 * with jisx */
				str2b[j].byte1 = 0x23; /* magic number! */
				str2b[j].byte2 = str[i++];
			}
			else if (i+1 < len)
			{
				/* mb gl (for gr replace & 0x7f by | 0x80 ...)
				 */
				str2b[j].byte1 = str[i++] & 0x7f;
				str2b[j].byte2 = str[i++] & 0x7f;
			}
			else
			{
				str2b[j].byte1 = 0;
				str2b[j].byte2 = 0;
				i++;
			}
			j++;
		}
	}
	else /* big5 and others not yet tested */
	{
		while (i < len && str[i] != 0)
		{
			if ((str[i] & 0x80) == 0)
			{
				/* we should convert to ascii */
#if 0
				str2b[j].byte1 = 0xa2; /* magic number! */
				str2b[j].byte2 = str[i++];
#endif
				/* a blanck char ... */
				str2b[j].byte1 = 0x21;
				str2b[j].byte2 = 0x21;
			}
			else if (i+1 < len)
			{
				str2b[j].byte1 = str[i++];
				str2b[j].byte2 = str[i++];
			}
			else
			{
				str2b[j].byte1 = 0;
				str2b[j].byte2 = 0;
				i++;
			}
			j++;
		}
	}
	*nl = j;
	if (free_str)
		free(str);
	return str2b;
}

static
char *FlocaleEncodeString(
	Display *dpy, FlocaleFont *flf, char *str, int *do_free, int len,
	int *nl, int *is_rtl, superimpose_char_t **comb_chars,
	int **l_to_v)
{
	char *str1, *str2, *str3;
	int len1;
	int len2;
	int i;
	Bool do_iconv = True;
	const char *bidi_charset;

	len1 = len;
	len2 = 0;

	if (is_rtl != NULL)
		*is_rtl = False;
	*do_free = False;
	*nl = len;

	if (flf->str_fc == NULL || flf->fc == NULL ||
	    flf->fc == flf->str_fc)
	{
		do_iconv = False;
	}

	str1 = str;
	if (FiconvSupport)
	{
		char *tmp_str;

	        /* first process combining characters */
	        tmp_str = FiconvCharsetToUtf8(
			dpy, flf->str_fc, (const char *)str,len);
		/* if conversion to UTF-8 failed str1 will be NULL */
		if(tmp_str != NULL)
		{
			/* do combining */
			len = CombineChars((unsigned char *)tmp_str,
					   strlen(tmp_str), comb_chars,
					   l_to_v);
			/* returns the length of the resulting UTF-8 string */
			/* convert back to current charset */
			str1 = FiconvUtf8ToCharset(
				dpy, flf->str_fc, (const char *)tmp_str,len);
			if (tmp_str != str1)
			{
				free(tmp_str);
			}
			if (str1)
			{
				*nl = len = strlen(str1);
				*do_free = True;
			}
			else
			{
				/* convert back to current charset fail */
				len = strlen(str);
				str1 = str;
			}
		}
	}

	if (FiconvSupport && do_iconv)
	{
		str2 = FiconvCharsetToCharset(
			dpy, flf->str_fc, flf->fc, (const char *)str1, len);
		if (str2 == NULL)
		{
			/* fail to convert */
			return str1;
		}
		if (str2 != str1)
		{
			if (*do_free && str1)
			{
				free(str1);
				str1 = str2;
			}
			*do_free = True;
			len1 = strlen(str2);
		}
	}
	else
	{
	        str2 = str1;
		len1 = len;
		/* initialise array with composing characters (empty) */
		if(comb_chars != NULL && *comb_chars == NULL)
		{
			*comb_chars = xmalloc(sizeof *comb_chars);
			(*comb_chars)[0].position = -1;
 			(*comb_chars)[0].c.byte1 = 0;
 			(*comb_chars)[0].c.byte2 = 0;
		}

		/* initialise logic to visual mapping here if that is demanded
		   (this is default when no combining has been done (1-to-1))
		*/
		if(l_to_v != NULL && *l_to_v == NULL)
		{
			*l_to_v = xmalloc((len + 1) * sizeof(int));
			for(i = 0 ; i < len ; i++)
				(*l_to_v)[i] = i;
			(*l_to_v)[len] = -1;
		}
	}

	if (FlocaleGetBidiCharset(dpy, flf->str_fc) != NULL &&
	    (bidi_charset = FlocaleGetBidiCharset(dpy, flf->fc)) != NULL)
	{
		str3 = FBidiConvert(str2, bidi_charset, len1,
				    is_rtl, &len2,
				    comb_chars != NULL ? *comb_chars : NULL,
				    l_to_v != NULL ? *l_to_v : NULL);
		if (str3 != NULL && str3  != str2)
		{
			if (*do_free)
			{
				free(str2);
			}
			*do_free = True;
			len1 = len2;
			str1 = str3;
		}
		/* if we failed to do BIDI convert, return string string from
		   combining phase */
		else
		{
		        str1 = str2;
			/* we already have the logical to visual mapping
			   from combining phase */
		}
	}

	*nl = len1;
	return str1;
}

static
void FlocaleEncodeWinString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws, int *do_free,
	int *len, superimpose_char_t **comb_chars, int **l_to_v)
{
	int len2b;
	fws->e_str = FlocaleEncodeString(
		dpy, flf, fws->str, do_free, *len, len, NULL, comb_chars,
		l_to_v);
	fws->str2b = NULL;

	if (flf->font != None)
	{
		if (FLC_ENCODING_TYPE_IS_UTF_8(flf->fc))
		{
			fws->str2b = FlocaleUtf8ToUnicodeStr2b(
				fws->e_str, *len, &len2b);
		}
		else if (flf->flags.is_mb)
		{
			fws->str2b = FlocaleStringToString2b(
				dpy, flf, fws->e_str, *len, &len2b);
		}
	}
}

/*
 * Text Drawing with a FontStruct
 */

static
void FlocaleFontStructDrawString(
	Display *dpy, FlocaleFont *flf, Drawable d, GC gc, int x, int y,
	Pixel fg, Pixel fgsh, Bool has_fg_pixels, FlocaleWinString *fws,
	int len, Bool image)
{
	int xt = x;
	int yt = y;
	int is_string16;
	flocale_gstp_args gstp_args;

	is_string16 = (FLC_ENCODING_TYPE_IS_UTF_8(flf->fc) || flf->flags.is_mb);
	if (is_string16 && fws->str2b == NULL)
	{
		return;
	}
	if (image)
	{
		/* for rotated drawing */
		FSwitchDrawImageString(
			is_string16, dpy, d, gc, x, y, fws->e_str, fws->str2b,
			len);
	}
	else
	{
		FlocaleInitGstpArgs(&gstp_args, flf, fws, x, y);
		/* normal drawing */
		if (flf->shadow_size != 0 && has_fg_pixels == True)
		{
			XSetForeground(dpy, fws->gc, fgsh);
			while (FlocaleGetShadowTextPosition(
				       &xt, &yt, &gstp_args))
			{
				FSwitchDrawString(
					is_string16, dpy, d, gc, xt, yt,
					fws->e_str, fws->str2b, len);
			}
		}
		if (has_fg_pixels == True)
		{
			XSetForeground(dpy, gc, fg);
		}
		xt = gstp_args.orig_x;
		yt = gstp_args.orig_y;
		FSwitchDrawString(
			is_string16, dpy, d, gc, xt,yt, fws->e_str, fws->str2b,
			len);
	}

	return;
}

/*
 * Rotated Text Drawing with a FontStruct or a FontSet
 */
static
void FlocaleRotateDrawString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws, Pixel fg,
	Pixel fgsh, Bool has_fg_pixels, int len,
	superimpose_char_t *comb_chars, int *pixel_pos)
{
	static GC my_gc = None;
	static GC font_gc = None;
	int j, i, xpfg, ypfg, xpsh, ypsh;
	unsigned char *normal_data, *rotated_data;
	unsigned int normal_w, normal_h, normal_len;
	unsigned int rotated_w, rotated_h, rotated_len;
	char val;
	int width, height, descent, min_offset;
	XImage *image, *rotated_image;
	Pixmap canvas_pix, rotated_pix;
	flocale_gstp_args gstp_args;
	char buf[4];

	if (fws->str == NULL || len < 1)
	{
		return;
	}
	if (fws->flags.text_rotation == ROTATION_0)
	{
		return; /* should not happen */
	}

	if (my_gc == None)
	{
		my_gc = fvwmlib_XCreateGC(dpy, fws->win, 0, NULL);
	}
	XCopyGC(dpy, fws->gc, GCForeground|GCBackground, my_gc);

	/* width and height (no shadow!) */
	width = FlocaleTextWidth(flf, fws->str, len) - FLF_SHADOW_WIDTH(flf);
	height = flf->height - FLF_SHADOW_HEIGHT(flf);
	descent = flf->descent - FLF_SHADOW_DESCENT(flf);;

	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;

	/* glyph width and height of the normal text */
	normal_w = width;
	normal_h = height;

	/* width in bytes */
	normal_len = (normal_w - 1) / 8 + 1;

	/* create and clear the canvas */
	canvas_pix = XCreatePixmap(dpy, fws->win, width, height, 1);
	if (font_gc == None)
	{
		font_gc = fvwmlib_XCreateGC(dpy, canvas_pix, 0, NULL);
	}
	XSetBackground(dpy, font_gc, 0);
	XSetForeground(dpy, font_gc, 0);
	XFillRectangle(dpy, canvas_pix, font_gc, 0, 0, width, height);

	/* draw the character center top right on canvas */
	XSetForeground(dpy, font_gc, 1);
	if (flf->font != NULL)
	{
		XSetFont(dpy, font_gc, flf->font->fid);
		FlocaleFontStructDrawString(dpy, flf, canvas_pix, font_gc, 0,
					    height - descent,
					    fg, fgsh, has_fg_pixels,
					    fws, len, True);
	}
	else if (flf->fontset != None)
	{
		XmbDrawString(
			dpy, canvas_pix, flf->fontset, font_gc, 0,
			height - descent, fws->e_str, len);
	}

	/* here take care of superimposing chars */
	i = 0;
	if(comb_chars != NULL)
	{
		while(comb_chars[i].c.byte1 != 0 && comb_chars[i].c.byte2 != 0)
		{
			/* draw composing character on top of corresponding
			   "real" character */
			FlocaleWinString tmp_fws = *fws;
			int offset = pixel_pos[comb_chars[i].position];
			int curr_len = FlocaleChar2bOneCharToUtf8(
							    comb_chars[i].c,
							    buf);
			int out_len;
			char *buf2 = FiconvUtf8ToCharset(
				dpy,
				flf->str_fc,
				(const char *)buf,curr_len);
			if(buf2 == NULL)
			{
				/* if conversion failed, combinational char
				   is not representable in current charset */
				/* just replace with empty string */
				buf2 = xmalloc(sizeof(char));
				*buf2 = '\0';
			}
			tmp_fws.e_str = buf2;
			tmp_fws.str2b = NULL;
			if(flf->fontset != None)
			{
				XmbDrawString(dpy, canvas_pix, flf->fontset,
					      fws->gc, offset,
					      height - descent, buf2,
					      strlen(buf2));
			}
			else if(flf->font != None)
			{
				if (FLC_ENCODING_TYPE_IS_UTF_8(flf->fc))
				{
					tmp_fws.str2b = 
						xmalloc(2 * sizeof(XChar2b));
					tmp_fws.str2b[0] = comb_chars[i].c;
					tmp_fws.str2b[1].byte1 = 0;
					tmp_fws.str2b[1].byte2 = 0;
					out_len = 1;
				}
				else if (flf->flags.is_mb)
				{
					tmp_fws.str2b =
					  FlocaleStringToString2b(
						    dpy, flf, tmp_fws.e_str,
						    curr_len, &out_len);
				}
				else
				{
					out_len = strlen(buf2);
				}
				XSetFont(dpy, font_gc, flf->font->fid);
				FlocaleFontStructDrawString(
					dpy, flf, canvas_pix, font_gc,
					offset, height - descent,
					fg, fgsh, has_fg_pixels, &tmp_fws,
					out_len, True);
			}

			free(buf2);
			if(tmp_fws.str2b != NULL)
			{
				free(tmp_fws.str2b);
			}
			i++;
		}
	}

	/* reserve memory for the first XImage */
	normal_data = xmalloc(normal_len * normal_h);

	/* create depth 1 XImage */
	if ((image = XCreateImage(
		     dpy, Pvisual, 1, XYBitmap, 0, (char *)normal_data,
		     normal_w, normal_h, 8, 0)) == NULL)
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
	if (fws->flags.text_rotation == ROTATION_180)
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
	rotated_data = xcalloc(rotated_h * rotated_len, 1);

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
			if (fws->flags.text_rotation == ROTATION_270)
				val = normal_data[
					i * normal_len +
					(normal_w - j - 1) / 8
					] & (128 >> ((normal_w - j - 1) % 8));

			else if (fws->flags.text_rotation == ROTATION_180)
				val = normal_data[
					(normal_h - j - 1) * normal_len +
					(normal_w - i - 1) / 8
					] & (128 >> ((normal_w - i - 1) % 8));

			else /* ROTATION_90 */
				val = normal_data[
					(normal_h - i - 1) * normal_len +
					j / 8] & (128 >> (j % 8));

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

	/* x and y corrections: we fill a rectangle! */
	min_offset = FlocaleGetMinOffset(flf, fws->flags.text_rotation);
	switch (fws->flags.text_rotation)
	{
	case ROTATION_90:
		/* CW */
		xpfg = fws->x - min_offset;
		ypfg = fws->y;
		break;
	case ROTATION_180:
		xpfg = fws->x;
		ypfg = fws->y - min_offset +
			FLF_SHADOW_BOTTOM_SIZE(flf);
		break;
	case ROTATION_270:
		/* CCW */
		xpfg = fws->x - min_offset;
		ypfg = fws->y;
		break;
	case ROTATION_0:
	default:
		xpfg = fws->x;
		ypfg = fws->y - min_offset;
		break;
	}
	xpsh = xpfg;
	ypsh = ypfg;
	/* write the image on the window */
	XSetFillStyle(dpy, my_gc, FillStippled);
	XSetStipple(dpy, my_gc, rotated_pix);
	FlocaleInitGstpArgs(&gstp_args, flf, fws, xpfg, ypfg);
	if (flf->shadow_size != 0 && has_fg_pixels == True)
	{
		XSetForeground(dpy, my_gc, fgsh);
		while (FlocaleGetShadowTextPosition(&xpsh, &ypsh, &gstp_args))
		{
			XSetTSOrigin(dpy, my_gc, xpsh, ypsh);
			XFillRectangle(
				dpy, fws->win, my_gc, xpsh, ypsh, rotated_w,
				rotated_h);
		}
	}
	xpsh = gstp_args.orig_x;
	ypsh = gstp_args.orig_y;
	XSetTSOrigin(dpy, my_gc, xpsh, ypsh);
	XFillRectangle(dpy, fws->win, my_gc, xpsh, ypsh, rotated_w, rotated_h);
	XFreePixmap(dpy, rotated_pix);

	return;
}

/*
 * Fonts info and checking
 */

static
char *FlocaleGetFullNameOfFontStruct(Display *dpy, XFontStruct *font)
{
	char *full_name = NULL;
	unsigned long value;

	if (XGetFontProperty(font, XA_FONT, &value))
	{
		full_name = XGetAtomName(dpy, value);
	}
	return full_name;
}

static
char *FlocaleGetCharsetOfFontStruct(Display *dpy, XFontStruct *font)
{
	int i = 0;
	int count = 0;
	char *charset = NULL;
	char *full_name;

	full_name = FlocaleGetFullNameOfFontStruct(dpy, font);
	if (full_name == NULL)
	{
		return NULL;
	}
	while(full_name[i] != '\0' && count < 13)
	{
		if (full_name[i] == '-')
		{
			count++;
		}
		i++;
	}

	if (count != 13)
	{
		return NULL;
	}
	CopyString(&charset, full_name+i);
	XFree(full_name);
	return charset;
}

static
char *FlocaleGetCharsetFromName(char *name)
{
	int l,i,e;
	char *charset;

	l = strlen(name);
	i = l-1;
	while(i >= 0 && name[i] != '-')
	{
		i--;
	}
	if (i == 0 || i == l-1)
	{
		return NULL;
	}
	i--;
	e = i;
	while(i >= 0 && name[i] != '-')
	{
		i--;
	}
	if (i <= 0 || e == i)
	{
		return NULL;
	}
	CopyString(&charset, name + i + 1);
	return charset;
}

/* return NULL if it is not reasonable to load a FontSet.
 * Currently return name if it is reasonable to load a FontSet, but in the
 * future we may want to transform name for faster FontSet loading */
static
char *FlocaleFixNameForFontSet(Display *dpy, char *name, char *module)
{
	char *new_name;
	char *charset;
	XFontStruct *test_font = NULL;

	if (!name)
	{
		return NULL;
	}

	new_name = name;

	if (strchr(name, ','))
	{
		/* tmp, do not handle "," separated list */
		return name;
	}
	charset = FlocaleGetCharsetFromName(name);
	if (charset == NULL && !strchr(name, '*') && !strchr(name, '?'))
	{
		/* probably a font alias! */
		if ((test_font = XLoadQueryFont(dpy, name)))
		{
			charset = FlocaleGetCharsetOfFontStruct(dpy, test_font);
			XFreeFont(dpy, test_font);
		}
	}
	if (charset != NULL)
	{
		if (!strchr(charset, '*') && !strchr(charset, '?') &&
		    !FlocaleCharsetIsCharsetXLocale(dpy, charset, module))
		{
			/* if the charset is fully specified and do not match
			 * one of the X locale charset */
			new_name = NULL;
#if 0
			fprintf(stderr,"[%s][FlocaleGetFontSet]: WARNING -- "
				"Use of a non X locale charset '%s' when "
				"loading font: %s\n",
				(module)? module:"fvwmlibs", charset, name);
#endif
		}
		free(charset);
	}
	return new_name;
}

/*
 * Fonts loading
 */

static
FlocaleFont *FlocaleGetFftFont(
	Display *dpy, char *fontname, char *encoding, char *module)
{
	FftFontType *fftf = NULL;
	FlocaleFont *flf = NULL;
	char *fn, *hints = NULL;

	hints = GetQuotedString(fontname, &fn, "/", NULL, NULL, NULL);
	if (fn == NULL)
	{
		fn = fft_fallback_font;
	}
	else if (*fn == '\0')
	{
		free(fn);
		fn = fft_fallback_font;
	}
	fftf = FftGetFont(dpy, fn, module);
	if (fftf == NULL)
	{
		if (fn != NULL && fn != fft_fallback_font)
		{
			free(fn);
		}
		return NULL;
	}
	flf = xcalloc(1, sizeof(FlocaleFont));
	memset(flf, '\0', sizeof(FlocaleFont));
	flf->count = 1;
	flf->fftf = *fftf;
	FlocaleCharsetSetFlocaleCharset(dpy, flf, hints, encoding, module);
	FftGetFontHeights(
		&flf->fftf, &flf->height, &flf->ascent, &flf->descent);
	FftGetFontWidths(flf, &flf->max_char_width);
	free(fftf);
	if (fn != NULL && fn != fft_fallback_font)
	{
		free(fn);
	}

	return flf;
}

static
FlocaleFont *FlocaleGetFontSet(
	Display *dpy, char *fontname, char *encoding, char *module)
{
	static int mc_errors = 0;
	FlocaleFont *flf = NULL;
	XFontSet fontset = NULL;
	char **ml;
	int mc,i;
	char *ds;
	XFontSetExtents *fset_extents;
	char *fn, *hints = NULL, *fn_fixed = NULL;

	hints = GetQuotedString(fontname, &fn, "/", NULL, NULL, NULL);
	if (*fn == '\0')
	{
		free(fn);
		fn = fn_fixed = mb_fallback_font;
	}
	else if (!(fn_fixed = FlocaleFixNameForFontSet(dpy, fn, module)))
	{
		if (fn != NULL && fn != mb_fallback_font)
		{
			free(fn);
		}
		return NULL;
	}
	if (!(fontset = XCreateFontSet(dpy, fn_fixed, &ml, &mc, &ds)))
	{
		if (fn_fixed && fn_fixed != fn)
		{
			free(fn_fixed);
		}
		if (fn != NULL && fn != mb_fallback_font)
		{
			free(fn);
		}
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
				(module)? module: "fvwmlibs", fontname);
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
					"[%s][FlocaleGetFontSet]: No more"
					" missing charset reportings\n",
					(module)? module: "fvwmlibs");
			}
		}
		XFreeStringList(ml);
	}

	flf = xcalloc(1, sizeof(FlocaleFont));
	flf->count = 1;
	flf->fontset = fontset;
	FlocaleCharsetSetFlocaleCharset(dpy, flf, hints, encoding, module);
	fset_extents = XExtentsOfFontSet(fontset);
	flf->height = fset_extents->max_ink_extent.height;
	flf->ascent = - fset_extents->max_ink_extent.y;
	flf->descent = fset_extents->max_ink_extent.height +
		fset_extents->max_ink_extent.y;
	flf->max_char_width = fset_extents->max_ink_extent.width;
	if (fn_fixed && fn_fixed != fn)
	{
		free(fn_fixed);
	}
	if (fn != NULL && fn != mb_fallback_font)
	{
		free(fn);
	}

	return flf;
}

static
FlocaleFont *FlocaleGetFont(
	Display *dpy, char *fontname, char *encoding, char *module)
{
	XFontStruct *font = NULL;
	FlocaleFont *flf;
	char *str,*fn,*tmp;
	char *hints = NULL;

	hints = GetQuotedString(fontname, &tmp, "/", NULL, NULL, NULL);
	str = GetQuotedString(tmp, &fn, ",", NULL, NULL, NULL);
	while (!font && fn)
	{
		if (*fn == '\0')
		{
			free(fn);
			fn = fallback_font;
		}
		font = XLoadQueryFont(dpy, fn);
		if (fn != NULL && fn != fallback_font)
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
		if (fn != NULL && fn != fallback_font)
		{
			free(fn);
		}
		if (tmp != NULL)
		{
			free(tmp);
		}
		return NULL;
	}

	flf = xcalloc(1, sizeof(FlocaleFont));
	flf->count = 1;
	flf->fontset = None;
	flf->fftf.fftfont = NULL;
	flf->font = font;
	FlocaleCharsetSetFlocaleCharset(dpy, flf, hints, encoding, module);
	flf->height = font->max_bounds.ascent + font->max_bounds.descent;
	flf->ascent = font->max_bounds.ascent;
	flf->descent = font->max_bounds.descent;
	flf->max_char_width = font->max_bounds.width;
	if (flf->font->max_byte1 > 0)
		flf->flags.is_mb = True;
	if (fn != NULL && fn != fallback_font)
	{
		free(fn);
	}
	if (tmp != NULL)
	{
		free(tmp);
	}

	return flf;
}

static
FlocaleFont *FlocaleGetFontOrFontSet(
	Display *dpy, char *fontname, char *encoding, char *fullname,
	char *module)
{
	FlocaleFont *flf = NULL;

	if (fontname && strlen(fontname) > 3 &&
	    strncasecmp("xft:", fontname, 4) == 0)
	{
		if (FftSupport)
		{
			flf = FlocaleGetFftFont(
				dpy, fontname+4, encoding, module);
		}
		if (flf)
		{
			CopyString(&flf->name, fullname);
		}
		return flf;
	}
	if (flf == NULL && Flocale != NULL && fontname)
	{
		flf = FlocaleGetFontSet(dpy, fontname, encoding, module);
	}
	if (flf == NULL && fontname)
	{
		flf = FlocaleGetFont(dpy, fontname, encoding, module);
	}
	if (flf && fontname)
	{
		if (StrEquals(fullname, mb_fallback_font))
		{
			flf->name = mb_fallback_font;
		}
		else if (StrEquals(fullname, fallback_font))
		{
			flf->name = fallback_font;
		}
		else
		{
			CopyString(&flf->name, fullname);
		}
		return flf;
	}
	return NULL;
}

/*
 * locale local functions
 */

static
void FlocaleSetlocaleForX(
	int category, const char *locale, const char *module)
{
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

/* ---------------------------- interface functions ------------------------ */

/*
 * locale initialisation
 */

void FlocaleInit(
	int category, const char *locale, const char *modifiers,
	const char *module)
{

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

/*
 * fonts loading
 */
char *prefix_list[] =
{
	"Shadow=",
	"StringEncoding=",
	NULL
};

FlocaleFont *FlocaleLoadFont(Display *dpy, char *fontname, char *module)
{
	FlocaleFont *flf = FlocaleFontList;
	Bool ask_default = False;
	char *t;
	char *str, *opt_str, *encoding= NULL, *fn = NULL;
	int shadow_size = 0;
	int shadow_offset = 0;
	int shadow_dir = MULTI_DIR_SE;
	int i;

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
		fontname = mb_fallback_font;
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

	/* But first see if we have a shadow relief and/or an encoding */
	str = fontname;
	while ((i = GetTokenIndex(str, prefix_list, -1, &str)) > -1)
	{
		str = GetQuotedString(str, &opt_str, ":", NULL, NULL, NULL);
		switch(i)
		{
		case 0: /* shadow= */
			FlocaleParseShadow(
				opt_str, &shadow_size, &shadow_offset,
				&shadow_dir, fontname, module);
			break;
		case 1: /* encoding= */
			if (encoding != NULL)
			{
				free(encoding);
				encoding = NULL;
			}
			if (opt_str && *opt_str)
			{
				CopyString(&encoding, opt_str);
			}
			break;
		default:
			break;
		}
		if (opt_str != NULL)
			free(opt_str);
	}
	if (str && *str)
	{
		str = GetQuotedString(str, &fn, ";", NULL, NULL, NULL);
	}
	else
	{
		fn = mb_fallback_font;
	}
	while (!flf && (fn && *fn))
	{
		flf = FlocaleGetFontOrFontSet(
			dpy, fn, encoding, fontname, module);
		if (fn != NULL && fn != mb_fallback_font &&
		    fn != fallback_font)
		{
			free(fn);
			fn = NULL;
		}
		if (!flf && str && *str)
		{
			str = GetQuotedString(str, &fn, ";", NULL, NULL, NULL);
		}
	}
	if (fn != NULL && fn != mb_fallback_font &&
	    fn != fallback_font)
	{
		free(fn);
	}

	if (flf == NULL)
	{
		/* loading failed, try default font */
		if (!ask_default)
		{
			fprintf(stderr,"[%s][FlocaleLoadFont]: "
				"WARNING -- can't load font '%s',"
				" trying default:\n",
				(module)? module: "fvwmlibs", fontname);
		}
		else
		{
			/* we already tried default fonts: try again? yes */
		}
		if (Flocale != NULL)
		{
			if (!ask_default)
			{
				fprintf(stderr, "\t%s\n",
					mb_fallback_font);
			}
			if ((flf = FlocaleGetFontSet(
				     dpy, mb_fallback_font, NULL,
				     module)) != NULL)
			{
				flf->name = mb_fallback_font;
			}
		}
		if (flf == NULL)
		{
			if (!ask_default)
			{
				fprintf(stderr,"\t%s\n",
					fallback_font);
			}
			if ((flf =
			     FlocaleGetFont(
				     dpy, fallback_font, NULL,
				     module)) != NULL)
			{
				flf->name = fallback_font;
			}
			else if (!ask_default)
			{
				fprintf(stderr,
					"[%s][FlocaleLoadFont]:"
					" ERROR -- can't load font.\n",
					(module)? module: "fvwmlibs");
			}
			else
			{
				fprintf(stderr,
					"[%s][FlocaleLoadFont]: ERROR"
					" -- can't load default font:\n",
					(module)? module: "fvwmlibs");
				fprintf(stderr, "\t%s\n",
					mb_fallback_font);
				fprintf(stderr, "\t%s\n",
					fallback_font);
			}
		}
	}

	if (flf != NULL)
	{
		if (shadow_size > 0)
		{
			flf->shadow_size = shadow_size;
			flf->flags.shadow_dir = shadow_dir;
			flf->shadow_offset = shadow_offset;
			flf->descent += FLF_SHADOW_DESCENT(flf);
			flf->ascent += FLF_SHADOW_ASCENT(flf);
			flf->height += FLF_SHADOW_HEIGHT(flf);
			flf->max_char_width += FLF_SHADOW_WIDTH(flf);
		}
		if (flf->fc == FlocaleCharsetGetUnknownCharset())
		{
			fprintf(stderr,"[%s][FlocaleLoadFont]: "
				"WARNING -- Unknown charset for font\n\t'%s'\n",
				(module)? module: "fvwmlibs", flf->name);
			flf->fc = FlocaleCharsetGetDefaultCharset(dpy, module);
		}
		else if (flf->str_fc == FlocaleCharsetGetUnknownCharset() &&
			 (encoding != NULL ||
			  (FftSupport && flf->fftf.fftfont != NULL &&
			   flf->fftf.str_encoding != NULL)))
		{
			fprintf(stderr,"[%s][FlocaleLoadFont]: "
				"WARNING -- Unknown string encoding for font\n"
				"\t'%s'\n",
				(module)? module: "fvwmlibs", flf->name);
		}
		if (flf->str_fc == FlocaleCharsetGetUnknownCharset())
		{
			flf->str_fc =
				FlocaleCharsetGetDefaultCharset(dpy, module);
		}
		flf->next = FlocaleFontList;
		FlocaleFontList = flf;
	}
	if (encoding != NULL)
	{
		free(encoding);
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
	    !StrEquals(flf->name, mb_fallback_font) &&
	    !StrEquals(flf->name, fallback_font))
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
	if (flf->fontset != NULL)
	{
		XFreeFontSet(dpy, flf->fontset);
	}
	if (flf->font != NULL)
	{
		XFreeFont(dpy, flf->font);
	}
	if (flf->flags.must_free_fc)
	{
		if (flf->fc->x)
			free(flf->fc->x);
		if (flf->fc->bidi)
			free(flf->fc->bidi);
		if (flf->fc->locale != NULL)
		{
			while (FLC_GET_LOCALE_CHARSET(flf->fc,i) != NULL)
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

/*
 * Width and Drawing Text
 */

void FlocaleInitGstpArgs(
	flocale_gstp_args *args, FlocaleFont *flf, FlocaleWinString *fws,
	int start_x, int start_y)
{
	args->step = 0;
	args->offset = flf->shadow_offset + 1;
	args->outer_offset = flf->shadow_offset + flf->shadow_size;
	args->size = flf->shadow_size;
	args->sdir = flf->flags.shadow_dir;
	switch (fws->flags.text_rotation)
	{
	case ROTATION_270: /* CCW */
		args->orig_x = start_x + FLF_SHADOW_UPPER_SIZE(flf);
		args->orig_y = start_y + FLF_SHADOW_RIGHT_SIZE(flf);
		break;
	case ROTATION_180:
		args->orig_x = start_x + FLF_SHADOW_RIGHT_SIZE(flf);
		args->orig_y = start_y;
		break;
	case ROTATION_90: /* CW */
		args->orig_x = start_x + FLF_SHADOW_BOTTOM_SIZE(flf);
		args->orig_y = start_y + FLF_SHADOW_LEFT_SIZE(flf);
		break;
	case ROTATION_0:
	default:
		args->orig_x = start_x + FLF_SHADOW_LEFT_SIZE(flf);
		args->orig_y = start_y;
		break;
	}
	args->rot = fws->flags.text_rotation;

	return;
}

Bool FlocaleGetShadowTextPosition(
	int *x, int *y, flocale_gstp_args *args)
{
	if (args->step == 0)
	{
		args->direction = MULTI_DIR_NONE;
		args->inter_step = 0;
	}
	if ((args->step == 0 || args->inter_step >= args->num_inter_steps) &&
	    args->size != 0)
	{
		/* setup a new direction */
		args->inter_step = 0;
		gravity_get_next_multi_dir(args->sdir, &args->direction);
		if (args->direction == MULTI_DIR_C)
		{
			int size;

			size = 2 * (args->outer_offset) + 1;
			args->num_inter_steps = size * size;
		}
		else
		{
			args->num_inter_steps = args->size;
		}
	}
	if (args->direction == MULTI_DIR_NONE || args->size == 0)
	{
		*x = args->orig_x;
		*y = args->orig_y;

		return False;
	}
	if (args->direction == MULTI_DIR_C)
	{
		int tx;
		int ty;
		int size;
		int is_finished;

		size = 2 * (args->outer_offset) + 1;
		tx = args->inter_step % size - args->outer_offset;
		ty = args->inter_step / size - args->outer_offset;
		for (is_finished = 0; ty <= args->outer_offset;
		     ty++, tx = -args->outer_offset)
		{
			for (; tx <= args->outer_offset; tx++)
			{
				if (tx <= -args->offset ||
				    tx >= args->offset ||
				    ty <= -args->offset || ty >= args->offset)
				{
					is_finished = 1;
					break;
				}
			}
			if (is_finished)
			{
				break;
			}
		}
		args->inter_step =
			(tx + args->outer_offset) +
			(ty + args->outer_offset) * size;
		if (!is_finished)
		{
			tx = 0;
			ty = 0;
		}
		*x = args->orig_x + tx;
		*y = args->orig_y + ty;
	}
	else if (args->inter_step > 0)
	{
		/* into a directional drawing */
		(*x) += args->x_sign;
		(*y) += args->y_sign;
	}
	else
	{
		direction_t dir;
		direction_t dir_x;
		direction_t dir_y;

		dir = gravity_multi_dir_to_dir(args->direction);
		gravity_split_xy_dir(&dir_x, &dir_y, dir);
		args->x_sign = gravity_dir_to_sign_one_axis(dir_x);
		args->y_sign = gravity_dir_to_sign_one_axis(dir_y);
		gravity_rotate_xy(
			args->rot, args->x_sign, args->y_sign,
			&args->x_sign, &args->y_sign);
		*x = args->orig_x + args->x_sign * args->offset;
		*y = args->orig_y + args->y_sign * args->offset;
	}
	args->inter_step++;
	args->step++;

	return True;
}

void FlocaleDrawString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws,
	unsigned long flags)
{
	int len;
	Bool do_free = False;
	Pixel fg = 0, fgsh = 0;
	Bool has_fg_pixels = False;
	flocale_gstp_args gstp_args;
	superimpose_char_t *comb_chars = NULL;
	char *curr_str;
	int char_len; /* length in number of chars */
	int *pixel_pos = NULL;
	int i;
	int j;
	char buf[4];
	int curr_pixel_pos;
	int curr_len;

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

	/* encode the string */
	FlocaleEncodeWinString(
		dpy, flf, fws, &do_free, &len, &comb_chars, NULL);
	curr_str = fws->e_str;
	for(char_len = 0, i = 0 ;
	    i < len && curr_str[i] != 0 ;
	    char_len++, i += curr_len)
	{
		curr_len = FlocaleStringNumberOfBytes(flf, curr_str + i);
	}

	/* for superimposition calculate the character positions in pixels */
	if (comb_chars != NULL && (
		   comb_chars[0].c.byte1 != 0 || comb_chars[0].c.byte2 != 0))
	{
		/* the second condition is actually redundant,
		   but there for clarity,
		   ending at 0 is what's expected in a correct
		   string */
		pixel_pos = xmalloc(
			(char_len != 0 ? char_len : 1) * sizeof(int));

		/* if there is 0 bytes in the encoded string, there might
		   still be combining character to draw (at position 0) */
		if(char_len == 0)
		{
			pixel_pos[0] = 0;
		}

		for(
			i = 0, curr_pixel_pos = 0 ;
			i < char_len ;
			i++, curr_str += curr_len)
		{
		        curr_len = FlocaleStringNumberOfBytes(flf, curr_str);
			for (j = 0 ; j < curr_len ; j++)
			{
			        buf[j] = curr_str[j];
			}
			buf[j] = 0;
			pixel_pos[i] = curr_pixel_pos;
			/* need to compensate for shadow width (if any) */
			curr_pixel_pos +=
				FlocaleTextWidth(flf, buf, curr_len) -
				FLF_SHADOW_WIDTH(flf);
		}
	}

	/* get the pixels */
	if (fws->flags.has_colorset)
	{
		fg = fws->colorset->fg;
		fgsh = fws->colorset->fgsh;
		has_fg_pixels = True;
	}
	else if (flf->shadow_size != 0)
	{
		XGCValues xgcv;

		if (XGetGCValues(dpy, fws->gc, GCForeground, &xgcv) != 0)
		{
			fg = xgcv.foreground;
		}
		else
		{
			fg = PictureBlackPixel();
		}
		fgsh = GetShadow(fg);
		has_fg_pixels = True;
	}

	if(flf->font != None &&
	   (FLC_ENCODING_TYPE_IS_UTF_8(flf->fc) || flf->flags.is_mb))
	{
		/* in this case, length is number of 2-byte chars */
		len = char_len;
	}

	if (fws->flags.text_rotation != ROTATION_0 &&
	    flf->fftf.fftfont == NULL)
	{
	        /* pass in information to perform superimposition */
		FlocaleRotateDrawString(
			dpy, flf, fws, fg, fgsh, has_fg_pixels, len,
			comb_chars, pixel_pos);
	}
	else if (FftSupport && flf->fftf.fftfont != NULL)
	{
		FftDrawString(
			dpy, flf, fws, fg, fgsh, has_fg_pixels, len, flags);
	}
	else if (flf->fontset != None)
	{
		int xt = fws->x;
		int yt = fws->y;

		FlocaleInitGstpArgs(&gstp_args, flf, fws, fws->x, fws->y);
		if (flf->shadow_size != 0)
		{
			XSetForeground(dpy, fws->gc, fgsh);
			while (FlocaleGetShadowTextPosition(
				       &xt, &yt, &gstp_args))
			{
				XmbDrawString(
					dpy, fws->win, flf->fontset, fws->gc,
					xt, yt, fws->e_str, len);
			}
		}
		if (has_fg_pixels == True)
		{
			XSetForeground(dpy, fws->gc, fg);
		}
		xt = gstp_args.orig_x;
		yt = gstp_args.orig_y;
		XmbDrawString(
			dpy, fws->win, flf->fontset, fws->gc,
			xt, yt, fws->e_str, len);
	}
	else if (flf->font != None)
	{
		FlocaleFontStructDrawString(
			dpy, flf, fws->win, fws->gc, fws->x, fws->y,
			fg, fgsh, has_fg_pixels, fws, len, False);
	}

	/* here take care of superimposing chars */
	i = 0;
	if (comb_chars != NULL)
	{
		while(comb_chars[i].c.byte1 != 0 && comb_chars[i].c.byte2 != 0)
		{
		        /* draw composing character on top of corresponding
			   "real" character */
		        FlocaleWinString tmp_fws = *fws;
			int offset = pixel_pos[comb_chars[i].position];
			char *buf2;
			int out_len;
		        curr_len = FlocaleChar2bOneCharToUtf8(comb_chars[i].c,
							      buf);
			buf2 = FiconvUtf8ToCharset(
				dpy, flf->str_fc, (const char *)buf, curr_len);
			if(buf2 == NULL)
			{
				/* if conversion failed, combinational char
				   is not representable in current charset */
				/* just replace with empty string */
				buf2 = xmalloc(sizeof(char));
				*buf2 = '\0';
			}
			tmp_fws.e_str = buf2;
			tmp_fws.str2b = NULL;
			if(FftSupport && flf->fftf.fftfont != NULL)
			{
			        tmp_fws.x = fws->x + offset;
				FftDrawString(
					dpy, flf, &tmp_fws, fg, fgsh,
					has_fg_pixels, strlen(buf2),
					flags);
			}
			else if(flf->fontset != None)
			{
			        int xt = fws->x;
				int yt = fws->y;

				FlocaleInitGstpArgs(
					&gstp_args, flf, fws, fws->x, fws->y);
				if (flf->shadow_size != 0)
				{
					XSetForeground(dpy, fws->gc, fgsh);
					while (FlocaleGetShadowTextPosition(
					       &xt, &yt, &gstp_args))
					{
						XmbDrawString(
							      dpy, fws->win,
							      flf->fontset,
							      fws->gc,
							      xt, yt,
							      buf2,
							      strlen(buf2));
					}
				}
				XSetForeground(dpy, fws->gc, fg);
				xt = gstp_args.orig_x;
				yt = gstp_args.orig_y;
			        XmbDrawString(
					dpy, fws->win, flf->fontset, fws->gc,
					xt + offset, yt, buf2, strlen(buf2));
			}
			else if (flf->font != None)
			{
				if (FLC_ENCODING_TYPE_IS_UTF_8(flf->fc))
				{
					tmp_fws.str2b = xmalloc(
							2 * sizeof(XChar2b));
					tmp_fws.str2b[0] = comb_chars[i].c;
					tmp_fws.str2b[1].byte1 = 0;
					tmp_fws.str2b[1].byte2 = 0;
					out_len = 1;
				        /*tmp_fws.str2b =
						FlocaleUtf8ToUnicodeStr2b(
							tmp_fws.e_str,
							curr_len, &out_len);*/
				}
				else if (flf->flags.is_mb)
				{
					tmp_fws.str2b =
					  FlocaleStringToString2b(
					  dpy, flf,
					  tmp_fws.e_str,
					  curr_len, &out_len);
				}
				else
				{
				        out_len = strlen(buf2);
				}
			        FlocaleFontStructDrawString(
					dpy, flf, fws->win, fws->gc,
					fws->x + offset, fws->y, fg, fgsh,
					has_fg_pixels, &tmp_fws, out_len,
					False);
			}

			free(buf2);
			if(tmp_fws.str2b != NULL)
			{
			        free(tmp_fws.str2b);
			}
			i++;
		}
	}

	if (do_free)
	{
		if (fws->e_str != NULL)
		{
			free(fws->e_str);
			fws->e_str = NULL;
		}
	}

	if (fws->str2b != NULL)
	{
		free(fws->str2b);
		fws->str2b = NULL;
	}

	if(comb_chars != NULL)
	{
	        free(comb_chars);
		if(pixel_pos)
			free(pixel_pos);
	}

	return;
}

void FlocaleDrawUnderline(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fws, int offset)
{
	int off1, off2, y, x_s, x_e;
	superimpose_char_t *comb_chars = NULL;
	int *l_to_v = NULL;
	Bool do_free = True;
	int len = strlen(fws->str);
	int l_coffset;
	int v_coffset;
	int voffset;

	if (fws == NULL || fws->str == NULL)
	{
		return;
	}

	/* need to encode the string first to get BIDI and combining chars */
	FlocaleEncodeWinString(dpy, flf, fws, &do_free, &len, &comb_chars,
			       &l_to_v);
	/* we don't need this, only interested in char mapping */
	free(comb_chars);

	/* now calculate char offset (in bytes) in visual string corresponding
	   to coffset */
	/* calculate absolute position in string (in characters) */
	l_coffset = FlocaleStringByteToCharOffset(flf, fws->str, offset);
	/* map to an offset in the visual string */
	v_coffset = l_to_v[l_coffset];
	/* calculate byte offset into visual string */
	voffset = FlocaleStringCharToByteOffset(flf, fws->e_str, v_coffset);

	off1 = FlocaleTextWidth(flf, fws->e_str, voffset) +
		((voffset == 0)?
		 FLF_SHADOW_LEFT_SIZE(flf) : - FLF_SHADOW_RIGHT_SIZE(flf) );
	off2 = FlocaleTextWidth(flf, fws->e_str + voffset,
		      FlocaleStringNumberOfBytes(flf, fws->e_str + voffset)) -
		FLF_SHADOW_WIDTH(flf) - 1 + off1;
	y = fws->y + 2;
	x_s = fws->x + off1;
	x_e = fws->x + off2;

	/* No shadow */
	XDrawLine(dpy, fws->win, fws->gc, x_s, y, x_e, y);

	/* free encoded string if it isn't the same as input string */
	if(fws->e_str != fws->str)
	{
		free(fws->e_str);
		fws->e_str = NULL;
	}

	if(fws->str2b != NULL)
	{
		free(fws->str2b);
		fws->str2b = NULL;
	}
	free(l_to_v);

	return;
}

int FlocaleTextWidth(FlocaleFont *flf, char *str, int sl)
{
	int result = 0;
	char *tmp_str;
	int new_l,do_free;
	superimpose_char_t *comb_chars = NULL;

	if (!str || sl == 0)
		return 0;

	if (sl < 0)
	{
		/* a vertical string: nothing to do! */
		sl = -sl;
	}

	/* FIXME */
	/* to avoid eccesive calls iconv (slow in Solaris 8)
	   don't bother to encode if string is one byte
	   when drawing a string this function is used to calculate
	   position of each character (for superimposition) */
	if(sl == 1)
	{
		tmp_str = str;
		new_l = sl;
		do_free = False;
	}
	else
	{
		tmp_str = FlocaleEncodeString(
			   Pdpy, flf, str, &do_free, sl, &new_l, NULL,
			   &comb_chars, NULL);
	}
	/* if we get zero-length, check to to see if there if there's any
	   combining chars, if so use an imagninary space as a
	   "base character" */
	if (strlen(tmp_str) == 0 && comb_chars &&
	    (comb_chars[0].c.byte1 != 0 || comb_chars[0].c.byte2 != 0))
	{
		if(do_free)
		{
			free(tmp_str);
		}
		if(comb_chars)
		{
			free(comb_chars);
		}
		return FlocaleTextWidth(flf, " ", 1);
	}
	else if (FftSupport && flf->fftf.fftfont != NULL)
		     {
		result = FftTextWidth(flf, tmp_str, new_l);
	}
	else if (flf->fontset != None)
	{
		result = XmbTextEscapement(flf->fontset, tmp_str, new_l);
	}
	else if (flf->font != None)
	{
		if (FLC_ENCODING_TYPE_IS_UTF_8(flf->fc) || flf->flags.is_mb)
		{
			XChar2b *str2b;
			int nl;

			if (FLC_ENCODING_TYPE_IS_UTF_8(flf->fc))
				str2b = FlocaleUtf8ToUnicodeStr2b(
					tmp_str, new_l, &nl);
			else
				str2b = FlocaleStringToString2b(
					Pdpy, flf, tmp_str, new_l, &nl);
			if (str2b != NULL)
			{
				result = XTextWidth16(flf->font, str2b, nl);
				free(str2b);
			}
		}
		else
		{
			result = XTextWidth(flf->font, tmp_str, new_l);
		}
	}
	if (do_free)
	{
		free(tmp_str);
	}
	if (comb_chars)
	{
		free(comb_chars);
	}

	return result + ((result != 0)? FLF_SHADOW_WIDTH(flf):0);
}

int FlocaleGetMinOffset(
	FlocaleFont *flf, rotation_t rotation)
{
	int min_offset;

#ifdef FFT_BUGGY_FREETYPE
	switch(rotation)
	{
	case ROTATION_270:
	case ROTATION_180:
		/* better than descent */
		min_offset = (flf->descent + flf->height - flf->ascent)/2;
		break;
	case ROTATION_0:
	case ROTATION_90:
	default:
		/* better than ascent */
		min_offset = (flf->ascent + flf->height - flf->descent)/2;
		break;
	}
#else
	switch(rotation)
	{
	case ROTATION_180:
	case ROTATION_90:
		/* better than descent */
		min_offset = (flf->descent + flf->height - flf->ascent)/2;
		break;
	case ROTATION_270:
	case ROTATION_0:
	default:
		/* better than ascent */
		min_offset = (flf->ascent + flf->height - flf->descent)/2;
		break;
	}
#endif
	return min_offset;
}

void FlocaleAllocateWinString(FlocaleWinString **pfws)
{
	*pfws = xcalloc(1, sizeof(FlocaleWinString));
}

/*
 * Text properties
 */
void FlocaleGetNameProperty(
	Status (func)(Display *, Window, XTextProperty *), Display *dpy,
	Window w, FlocaleNameString *ret_name)
{
	char **list;
	int num;
	XTextProperty text_prop;

	list = NULL;
	if (func(dpy, w, &text_prop) == 0)
	{
		return;
	}
	if (text_prop.encoding == XA_STRING)
	{
		/* STRING encoding, use this as it is */
		ret_name->name = (char *)text_prop.value;
		ret_name->name_list = NULL;
		return;
	}
	/* not STRING encoding, try to convert XA_COMPOUND_TEXT */
	if (XmbTextPropertyToTextList(dpy, &text_prop, &list, &num) >= Success
	    && num > 0 && *list)
	{
		/* Does not consider the conversion is REALLY succeeded:
		 * XmbTextPropertyToTextList return 0 (== Success) on success,
		 * a negative int if it fails (and in this case we are not
		 * here), the number of unconvertible char on "partial"
		 * success*/
		XFree(text_prop.value); /* return of XGetWM(Icon)Name() */
		ret_name->name = *list;
		ret_name->name_list = list;
	}
	else
	{
		if (list)
		{
			XFreeStringList(list);
		}
		ret_name->name = (char *)text_prop.value;
		ret_name->name_list = NULL;
	}
}

void FlocaleFreeNameProperty(FlocaleNameString *ptext)
{
	if (ptext->name_list != NULL)
	{
		if (ptext->name != NULL && ptext->name != *ptext->name_list)
			XFree(ptext->name);
		XFreeStringList(ptext->name_list);
		ptext->name_list = NULL;
	}
	else if (ptext->name != NULL
                 /* Sorry, this is pretty ugly.
                    in fvwm/events.c we have:
                    FlocaleNameString new_name = { NoName, NULL };
                    NoName is a global extern I don't want to add to
                    to this libary module.
                    So, this check comes close enough: */
                 && strcmp("Untitled",ptext->name) != 0)
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

	if (Flocale != NULL)
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

/*
 * Info
 */
void FlocalePrintLocaleInfo(Display *dpy, int verbose)
{
	FlocaleFont *flf = FlocaleFontList;
	int count = 0;
	FlocaleCharset *cs;

	fflush(stderr);
	fflush(stdout);
	fprintf(stderr,"fvwm info on locale:\n");
	fprintf(stderr,"  locale: %s, Modifier: %s\n",
		(Flocale)? Flocale:"", (Fmodifiers)? Fmodifiers:"");
	cs = FlocaleCharsetGetDefaultCharset(dpy, NULL);
	fprintf(stderr,"  Default Charset:  X: %s, Iconv: %s, Bidi: %s\n",
		cs->x,
		(cs->iconv_index >= 0)?
		cs->locale[cs->iconv_index]:"Not defined",
		(cs->bidi)? "Yes":"No");
	FlocaleCharsetPrintXOMInfo();
	while (flf)
	{
		count++;
		flf = flf->next;
	}
	fprintf(stderr,"  Number of loaded font: %i\n", count);
	if (verbose)
	{
		count = 0;
		flf = FlocaleFontList;
		while(flf)
		{
			cs = flf->fc;
			fprintf(stderr,"  * Font number %i\n", count);
			fprintf(stderr,"    fvwm info:\n");
			fprintf(stderr,"      Name: %s\n",
				(flf->name)?  flf->name:"");
			fprintf(stderr,"      Cache count: %i\n", flf->count);
			fprintf(stderr,"      Type: ");
			if (flf->font)
			{
				fprintf(stderr,"FontStruct\n");
			}
			else if (flf->fontset)
			{
				fprintf(stderr,"FontSet\n");
			}
			else
			{
				fprintf(stderr,"XftFont\n");
			}
			fprintf(stderr, "      Charset:  X: %s, Iconv: %s, "
				"Bidi: %s\n",
				cs->x,
				(cs->iconv_index >= 0)?
				cs->locale[cs->iconv_index]:"Not defined",
				(cs->bidi)? "Yes":"No");
			fprintf(stderr,"      height: %i, ascent: %i, "
				"descent: %i\n", flf->height, flf->ascent,
				flf->descent);
			fprintf(stderr,"      shadow size: %i, "
				"shadow offset: %i, shadow direction:%i\n",
				flf->shadow_size, flf->shadow_offset,
				flf->flags.shadow_dir);
			if (verbose >= 2)
			{
				if (flf->fftf.fftfont != NULL)
				{
					FftFontType *fftf;

					fftf = &flf->fftf;
					fprintf(stderr, "    Xft info:\n"
						"      - Vertical font:");
					FftPrintPatternInfo(
						fftf->fftfont, False);
					fprintf(stderr, "      "
						"- Rotated font 90:");
					if (fftf->fftfont_rotated_90)
						FftPrintPatternInfo(
							fftf->
							fftfont_rotated_90,
							True);
					else
						fprintf(stderr, " None\n");
					fprintf(stderr, "      "
						"- Rotated font 270:");
					if (fftf->fftfont_rotated_270)
						FftPrintPatternInfo(
							fftf->
							fftfont_rotated_270,
							True);
					else
						fprintf(stderr, " None\n");
					fprintf(stderr, "      "
						"- Rotated font 180:");
					if (fftf->fftfont_rotated_180)
						FftPrintPatternInfo(
							fftf->
							fftfont_rotated_180,
							True);
					else
						fprintf(stderr, " None\n");
				}
				else if (flf->font != NULL)
				{
					char *full_name;

					full_name =
						FlocaleGetFullNameOfFontStruct(
							dpy, flf->font);
					fprintf(stderr, "    X info:\n"
						"      %s\n",
						(full_name)? full_name:"?");
					if (full_name != NULL)
					{
						XFree(full_name);
					}
				}
				else if (flf->fontset != NULL)
				{
					int n,i;
					XFontStruct **font_struct_list;
					char **font_name_list;

					fprintf(stderr, "    X info:\n");
					n = XFontsOfFontSet(
						flf->fontset,
						&font_struct_list,
						&font_name_list);
					for(i = 0; i < n; i++)
					{
						fprintf(stderr,
							"      %s\n",
							font_name_list[i]);
					}
				}
			}
			count++;
			flf = flf->next;
		}
	}
}
