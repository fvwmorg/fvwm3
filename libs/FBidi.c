/* -*-c-*- */
/* Copyright (C) 2002  Mikhael Goikhman */
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

/*
 * FBidi.c - interface to Bidi, we use fribidi implementation here.
 * See FBidi.h for some comments on this interface.
 */

#include "config.h"

#include "FBidi.h"

#if HAVE_BIDI

#include "safemalloc.h"
#include "BidiJoin.h"
#include <fribidi/fribidi.h>
#include <stdio.h>

Bool FBidiIsApplicable(const char *charset)
{
	if (fribidi_parse_charset((char *)charset) ==
	    FRIBIDI_CHAR_SET_NOT_FOUND)
	{
		return False;
	}
	return True;
}

char *FBidiConvert(
	const char *logical_str, const char *charset, int str_len,
	Bool *is_rtl, int *out_len, superimpose_char_t *comb_chars,
	int *l_to_v)
{
	char *visual_str;
	FriBidiCharSet fribidi_charset;
	FriBidiChar *logical_unicode_str;
	FriBidiChar *visual_unicode_str;
	FriBidiParType pbase_dir = FRIBIDI_TYPE_ON;
	FriBidiStrIndex *pos_l_to_v;
	int i;

	if (logical_str == NULL || charset == NULL)
	{
		return NULL;
	}
	if (str_len < 0)
	{
		str_len = strlen(logical_str);
	}
	if (is_rtl != NULL)
	{
		*is_rtl = False;
	}

	fribidi_charset = fribidi_parse_charset((char *)charset);
	if (fribidi_charset == FRIBIDI_CHAR_SET_NOT_FOUND)
	{
		return NULL;
	}

	/* it is possible that we allocate a bit more here, if utf-8 */
	logical_unicode_str = xmalloc((str_len + 1) * sizeof(FriBidiChar));

	/* convert to unicode first */
	str_len = fribidi_charset_to_unicode(
		fribidi_charset, (char *)logical_str, str_len,
		logical_unicode_str);

	visual_unicode_str = xmalloc((str_len + 1) * sizeof(FriBidiChar));

	/* apply bidi algorithm, convert logical string to visual string */
	/* also keep track of how characters are reordered here, to reorder
	   combing characters accordingly */
	pos_l_to_v = xmalloc((str_len + 1) * sizeof(FriBidiStrIndex));
	fribidi_log2vis(
		logical_unicode_str, str_len, &pbase_dir,
		visual_unicode_str, pos_l_to_v, NULL, NULL);

	/* remap mapping from logical to visual to "compensate" for BIDI */
	if (comb_chars != NULL)
	{
		for (i = 0;
		    comb_chars[i].c.byte1 != 0 ||
		    comb_chars[i].c.byte2 != 0;
		    i++)
		{
			/* if input string is zero characters => only
			   combining chars, set position to zero */
			comb_chars[i].position =
				str_len != 0 ?
				pos_l_to_v[comb_chars[i].position] : 0;
		}
	}

	if (l_to_v != NULL)
	{
		/* values in the previuos mapping gives the position of
		   input characters after combining step */
		/* mapping from BIDI conversion maps from the positions in
		   the output from combining */
		int orig_len;
		int *l_to_v_temp;
		for (i = 0; l_to_v[i] != -1; i++)
		{
		}
		orig_len = i;
		l_to_v_temp = xmalloc(orig_len * sizeof(int));
		for (i = 0; i < orig_len; i++)
		{
			l_to_v_temp[i] = pos_l_to_v[l_to_v[i]];
		}
		for (i = 0; i < orig_len; i++)
		{
			l_to_v[i] = l_to_v_temp[i];
		}
		free(l_to_v_temp);
	}
	free(pos_l_to_v);


	/* character shape/join - will get pulled into fribidi with time */
	str_len = shape_n_join(visual_unicode_str, str_len);

	visual_str = xmalloc((4 * str_len + 1) * sizeof(char));

	/* convert from unicode finally */
	*out_len = fribidi_unicode_to_charset(
		fribidi_charset, visual_unicode_str, str_len, visual_str);

	if (is_rtl != NULL &&
		fribidi_get_bidi_type(*visual_unicode_str) == FRIBIDI_TYPE_RTL)
	{
		*is_rtl = True;
	}

	free(logical_unicode_str);
	free(visual_unicode_str);
	return visual_str;
}

#endif /* HAVE_BIDI */
