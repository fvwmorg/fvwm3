/* -*-c-*- */
/* Copyright (C) 2002 Mikhael Goikhman
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
 * FBidi.h - interface to Bidi (bidirectionality for some asian languages).
 */

/***
 * The main function is
 *   FBidiConvert(const char *logical_str, const char *charset, Bool *is_rtl)
 *
 * input:
 *   logical string - it should be a normal 8-bit string for now
 *   string charset - examples: "iso8859-15", "iso8859-8", "iso8859-6", "utf-8"
 *
 * output:
 *   visual string is returned (should be free'd), or NULL if not applicable
 *   the last argument is set to True if the string has the base RTL direction
 *
 * The is_rtl aggument may be used for 2 purposes. The first is to change the
 * string alignment if there is a free space (but it probably worth not to
 * override the user specified string aligment). The second is to determine
 * where to put ellipses if the string is longer than there is a space.
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

#ifndef FBIDI_H
#define FBIDI_H

#include "config.h"
#include <X11/Xlib.h>

#if HAVE_BIDI

#include "FlocaleCharset.h"

/*
 * Checks whether the string in the given charset should be BidiConvert'd.
 */
Bool FBidiIsApplicable(const char *charset);

/*
 * Converts the given logical string to visual string for the given charset.
 */
char *FBidiConvert(Display *dpy, const char *logical_str, FlocaleFont *flf,
		   Bool *is_rtl);

#else /* !HAVE_BIDI */

#define FBidiIsApplicable(c) False

#define FBidiConvert(y,s,c,d) NULL

#endif /* HAVE_BIDI */

#endif /* FBIDI_H */
