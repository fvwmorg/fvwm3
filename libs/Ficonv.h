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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef FICONV_H
#define FICONV_H

/* ---------------------------- included header files ----------------------- */

#include "config.h"

/* ---------------------------- global definitions -------------------------- */

#ifdef HAVE_ICONV
#define FiconvSupport 1
#else
#define FiconvSupport 0
#endif

#if FiconvSupport

#define Ficonv_open(a,b)    iconv_open(a,b)
#define Ficonv_close(a)	    iconv_close(a)
#define Ficonv(a,b,c,d,e)   iconv(a,b,c,d,e)

#else

#define Ficonv_open(a,b)    (Ficonv_t)-1
#define Ficonv_close(a)	    -1
#define Ficonv(a,b,c,d,e)   -1

#endif

/* ---------------------------- global macros ------------------------------- */

#define FICONV_CONVERSION_MAX_NUMBER_OF_WARNING 10

/* ---------------------------- type definitions ---------------------------- */

typedef void* Ficonv_t;

/* ---------------------------- interface functions ------------------------- */
char *FiconvUtf8ToCharset(Display *dpy, FlocaleCharset *fc,
			  const char *in, unsigned int in_size);
char *FiconvCharsetToUtf8(Display *dpy, FlocaleCharset *fc,
			  const char *in, unsigned int in_size);
char *FiconvCharsetToCharset(
	Display *dpy, FlocaleCharset *in_fc, FlocaleCharset *out_fc,
	const char *in, unsigned int in_size);
#endif /* FICONV_H */
