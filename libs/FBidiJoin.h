/* -*-c-*- */
/* Copyright (C) 2002  Nadim Shaikli
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
 * FBidiJoin.h
 *
 * Interface to character shaping/joining that is required by most Bidi
 * (bidirectional) languages.
 */

#ifndef FBIDIJOIN_H
#define FBIDIJOIN_H

#include "config.h"

#if HAVE_BIDI

#include <fribidi/fribidi.h>

/*
 * Shape/Join a passed-in visual string
 */
int shape_n_join(FriBidiChar *str_visual, int str_len);

#else /* !HAVE_BIDI */

#define shape_n_join(a, b) 0

#endif /* HAVE_BIDI */

#endif /* FBIDIJOIN_H */
