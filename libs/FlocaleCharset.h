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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef FLOCALE_CHARSET_H
#define FLOCALE_CHARSET_H

/* ---------------------------- included header files ----------------------- */

#include "config.h"
#include "Flocale.h"

/* ---------------------------- global definitions -------------------------- */

#ifdef HAVE_LIBCHARSET
#define FlocaleLibcharsetSupport 1
#else
#define FlocaleLibcharsetSupport 0
#endif

#ifdef HAVE_CODESET
#define FlocaleCodesetSupport 1
#else
#define FlocaleCodesetSupport 0
#endif

#if FlocaleLibcharsetSupport
#define Flocale_charset()   locale_charset()
#else
#define Flocale_charset()   NULL
#endif

#if FlocaleCodesetSupport
#define Fnl_langinfo(a) nl_langinfo(a)
#define FCODESET CODESET
#else
#define Fnl_langinfo(a) NULL
#define FCODESET 0
#endif

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

/*
 *
 */
void FlocaleCharsetInit(Display *dpy, const char *module);

void FlocaleCharsetSetFlocaleCharset(
	Display *dpy, FlocaleFont *flf, char *hints, char *encoding,
	char *module);

FlocaleCharset *FlocaleCharsetGetDefaultCharset(Display *dpy, char *module);
FlocaleCharset *FlocaleCharsetGetFLCXOMCharset(void);
FlocaleCharset *FlocaleCharsetGetUtf8Charset(void);
FlocaleCharset *FlocaleCharsetGetLocaleCharset(void);
FlocaleCharset *FlocaleCharsetGetUnknownCharset(void);
const char *FlocaleGetBidiCharset(Display *dpy, FlocaleCharset *fc);
FlocaleCharset *FlocaleCharsetGetEUCJPCharset(void);

#endif /* FLOCALE_CHARSET_H */
