/* -*-c-*- */
/* Copyright (C) 2003  Marcus Lundblad */
/*
 * FCombineChars.h
 *
 * Interface to character combining
 */

#ifndef FCOMBINECHARS_H
#define FCOMBINECHARS_H

#include "config.h"
#include <X11/Xutil.h>

typedef struct
{
	int position;  /* position in the string */
	XChar2b c;     /* UCS-2 character */
} superimpose_char_t;

int CombineChars(
	unsigned char *str_visual, int len, superimpose_char_t **comb_chars);

#endif /* FCOMBINECHARS_H */
