/* -*-c-*- */
/* Copyright (C) 2002 Mikhael Goikhman */
/*
 * FBidi.h - interface to Bidi (bidirectionality for some asian languages).
 */
/***
 * The main function is
 *   char *FBidiConvert(
 *     const char *logical_str, const char *charset, int str_len, Bool *is_rtl,
 *     int *out_len);
 *
 * input:
 *   logical string - the original string
 *   string charset - examples: "iso8859-15", "iso8859-8", "iso8859-6", "utf-8"
 *
 * output:
 *   visual string is returned (should be free'd), or NULL if not applicable
 *   the last argument is set to True if the string has the base RTL direction
 *
 * The is_rtl aggument may be used for 2 purposes. The first is to change the
 * string alignment if there is a free space (but it probably worth not to
 * override the user specified string aligment). The second is to determine
 * where to put ellipses if the string is longer than the available space.
 *
 * There are several possible ways to solve the ellipses problem.
 * This is not automated, the caller should choose one or another way.
 *
 * 1) For a logical string evaluate a visual string (and a base direction)
 *    only once. If needed, cut the visual string and add ellipses at the
 *    correct string side.
 *
 * 2) Cut logical string to the size that will be drawn, add allipses to the
 *    end and call FBidiConvert on the resulting string.
 *
 * 3) Cut logical string to the size that will be drawn, call FBidiConvert
 *    and add ellipses at the correct string side.
 *
 * Probably 2) is the best, but there is one nuance, the ellipses may be at
 * the middle of the string. With 1) and 3) the ellipses are always on the
 * edge of the string. The 1) is good when speed is more important than memory
 * and the string is usually either pure LTR or RTL.
 *
 * Example 1:
 *
 * input:  she said "SHALOM" and then "BOKER TOV".
 * output: she said "MOLAHS" and then "VOT REKOB".
 * is_rtl: False
 *
 * 1) she said "MO...
 * 2) she said ...HS"
 * 3) she said HS"...
 *
 * Example 2:
 *
 * input:  SHALOM, world!
 * output: !world ,MOLAHS
 * is_rtl: True
 *
 * 1) ...ld ,MOLAHS
 * 2) wo... ,MOLAHS
 * 3) ...wo ,MOLAHS
 *
 **/

#ifndef FVWMLIB_FBIDI_H
#define FVWMLIB_FBIDI_H

#include "config.h"
#include <X11/Xlib.h>
#include "CombineChars.h"

#if HAVE_BIDI
/*
 * Checks whether the string in the given charset should be BidiConvert'd.
 */
Bool FBidiIsApplicable(const char *charset);

/*
 * Converts the given logical string to visual string for the given charset.
 */
char *FBidiConvert(
	const char *logical_str, const char *charset, int str_len,
	Bool *is_rtl, int *out_len, superimpose_char_t *comb_chars,
	int *pos_l_to_v);

#else /* !HAVE_BIDI */

#define FBidiIsApplicable(c) False

#define FBidiConvert(s, c, l, r, o, cc, lv) NULL

#endif /* HAVE_BIDI */

#endif /* FVWMLIB_FBIDI_H */
