/* -*-c-*- */
/* Copyright (C) 2002  Nadim Shaikli */
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
