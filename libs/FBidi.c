/* -*-c-*- */
/* Copyright (C) 2002  Mikhael Goikhman
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
 * FBidi.c - interface to Bidi, we use fribidi implementation here.
 */

#include "FBidi.h"

#if HAVE_FRIBIDI

#include <fribidi/fribidi.h>

Bool FBidiIsApplicable(const char *charset)
{
	if (fribidi_parse_charset((char *)charset) == FRIBIDI_CHARSET_NOT_FOUND)
	{
		return False;
	}
	return True;
}

char *FBidiConvert(const char *logical_str, const char *charset, Bool *rtl_dir)
{
	int str_len = strlen(logical_str);
	char *visual_str;

	FriBidiCharSet fribidi_charset;
	FriBidiChar *logical_unicode_str;
	FriBidiChar *visual_unicode_str;
	FriBidiCharType pbase_dir = FRIBIDI_TYPE_ON;

	*rtl_dir = False;

	fribidi_charset = fribidi_parse_charset((char *)charset);
	if (fribidi_charset == FRIBIDI_CHARSET_NOT_FOUND)
	{
		return NULL;
	}

	logical_unicode_str =
		(FriBidiChar *)malloc((str_len + 1) * sizeof(FriBidiChar));

	/* convert to unicode first */
	str_len = fribidi_charset_to_unicode(
		fribidi_charset, (char *)logical_str, str_len,
		logical_unicode_str);

	visual_unicode_str =
		(FriBidiChar *)malloc(str_len * sizeof(FriBidiChar) + 1);

	/* apply bidi algorithm, convert logical string to visual string */
	fribidi_log2vis(
		logical_unicode_str, str_len, &pbase_dir,
		visual_unicode_str, NULL, NULL, NULL);

	visual_str = (char *)malloc(str_len * sizeof(char) + 1);

	/* convert from unicode finally */
	str_len = fribidi_unicode_to_charset(
		fribidi_charset, visual_unicode_str, str_len,
		visual_str);

	if (pbase_dir == FRIBIDI_TYPE_RTL)
	{
		*rtl_dir = True;
	}

	free(logical_unicode_str);
	free(visual_unicode_str);

	return visual_str;
}

#endif /* HAVE_BIDI */
