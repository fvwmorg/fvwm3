/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */

#ifndef FICONV_H
#define FICONV_H

/* ---------------------------- included header files ---------------------- */

#include "config.h"

/* ---------------------------- global definitions ------------------------- */

#ifdef HAVE_ICONV
#define FiconvSupport 1
#else
#define FiconvSupport 0
#endif

#if FiconvSupport

#define Ficonv_open(a,b)    iconv_open(a,b)
#define Ficonv_close(a)     iconv_close(a)
#define Ficonv(a,b,c,d,e)   iconv(a,b,c,d,e)

#else

#define Ficonv_open(a,b)    (Ficonv_t)-1
#define Ficonv_close(a)     -1
#define Ficonv(a,b,c,d,e)   -1

#endif

/* ---------------------------- global macros ------------------------------ */

#define FICONV_CONVERSION_MAX_NUMBER_OF_WARNING 10

/* ---------------------------- type definitions --------------------------- */

typedef void* Ficonv_t;

/* ---------------------------- interface functions ------------------------ */
char *FiconvUtf8ToCharset(
	Display *dpy, FlocaleCharset *fc, const char *in,
	unsigned int in_size);
char *FiconvCharsetToUtf8(
	Display *dpy, FlocaleCharset *fc, const char *in,
	unsigned int in_size);
char *FiconvCharsetToCharset(
	Display *dpy, FlocaleCharset *in_fc, FlocaleCharset *out_fc,
	const char *in, unsigned int in_size);
#endif /* FICONV_H */
