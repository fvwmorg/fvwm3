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

#ifndef FBIDI_H
#define FBIDI_H

#include "config.h"
#include <X11/Xlib.h>  /* just for Bool */

#if HAVE_FRIBIDI

/*
 * Checks whether the string in the given charset should be BidiConvert'd.
 * Charset examples: "iso8859-8", "iso8859-6", "utf-8".
 */
Bool FBidiIsApplicable(const char *charset);

/*
 * Converts the given logical string to visual string for the given charset.
 */
char *FBidiConvert(const char *logical_str, const char *charset, Bool *rtl_dir);

#else /* !HAVE_BIDI */

#define FBidiIsApplicable(c) False

#define FBidiConvert(s,c,d) NULL

#endif /* HAVE_BIDI */

#endif /* FBIDI_H */
