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

/* Some code (convert_charsets) inspired by the glib-2 (gutf8.c) copyrighted
 * by Tom Tromey & Red Hat, Inc. */

/* ---------------------------- included header files ----------------------- */

#include <config.h>
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "safemalloc.h"
#include "FlocaleCharset.h"
#include "Ficonv.h"

#if FiconvSupport
#include <iconv.h>

#if defined(USE_LIBICONV) && !defined (_LIBICONV_H)
#error libiconv in use but included iconv.h not from libiconv
#endif
#if !defined(USE_LIBICONV) && defined (_LIBICONV_H)
#error libiconv not in use but included iconv.h is from libiconv
#endif
#endif /* FiconvSupport */

#if FlocaleLibcharsetSupport
#include <libcharset.h>
#endif
#if FlocaleCodesetSupport
#include <langinfo.h>
#endif

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static Bool FiconvInitialized = False;
static FlocaleCharset *FLCUtf8Charset = NULL;       /* UTF-8 charset */
static FlocaleCharset *FLCDefaultIconvCharset = NULL; 

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static
Bool is_iconv_supported(char *c1, char *c2)
{
	Ficonv_t cd1,cd2;
	Bool r = False;
	int dummy;

	if (!FiconvSupport || !c1 || !c2)
		return False;

	cd1 = Ficonv_open(c1, c2);
	cd2 = Ficonv_open(c2, c1);
	if (cd1 != (Ficonv_t) -1 && cd2 != (Ficonv_t) -1)
		r = True;
	if (cd1 != (Ficonv_t) -1)
		dummy = Ficonv_close(cd1);
	if (cd2 != (Ficonv_t) -1)
		dummy = Ficonv_close(cd2);
	return r;
}

static
int set_default_iconv_charsets(FlocaleCharset *fc)
{
	int i=0,j=0;

	if (!FiconvSupport || FLCUtf8Charset == NULL || fc == NULL)
		return False;

	while(FLC_GET_LOCALE_CHARSET(FLCUtf8Charset,i) != NULL)
	{
		j = 0;
		while(FLC_GET_LOCALE_CHARSET(fc,j) != NULL)
		{
			if (is_iconv_supported(
				  FLC_GET_LOCALE_CHARSET(FLCUtf8Charset,i),
				  FLC_GET_LOCALE_CHARSET(fc,j)))
			{
				FLC_SET_ICONV_INDEX(FLCUtf8Charset,i);
				FLC_SET_ICONV_INDEX(fc,j);
				return 1;
			}
			j++;
		}
		i++;
	}
	FLC_SET_ICONV_INDEX(FLCUtf8Charset, FLC_INDEX_ICONV_CHARSET_NOT_FOUND);
	FLC_SET_ICONV_INDEX(fc, FLC_INDEX_ICONV_CHARSET_NOT_FOUND);
	return 0;
}

static
void set_iconv_charset_index(FlocaleCharset *fc)
{
	int i = 0;

	if (!FiconvSupport || fc == NULL)
		return;

	if (FLC_DO_ICONV_CHARSET_INITIALIZED(fc))
		return; /* already set */
	if (FLCUtf8Charset == NULL ||
	    !FLC_DO_ICONV_CHARSET_INITIALIZED(FLCUtf8Charset))
	{
		FLC_SET_ICONV_INDEX(fc, FLC_INDEX_ICONV_CHARSET_NOT_FOUND);
		return;
	}
	while(FLC_GET_LOCALE_CHARSET(fc,i) != NULL)
	{
		if (is_iconv_supported(
			FLC_GET_ICONV_CHARSET(FLCUtf8Charset),
			FLC_GET_LOCALE_CHARSET(fc,i)))
		{
			FLC_SET_ICONV_INDEX(fc,i);
			return;
		}
	}
	FLC_SET_ICONV_INDEX(fc, FLC_INDEX_ICONV_CHARSET_NOT_FOUND);
}

static
char *convert_charsets(const char *in_charset, const char *out_charset,
		       const unsigned char *in, unsigned int in_size)
{
  static int error_count = 0;
  Ficonv_t cd;
  int have_error = 0;
  int is_finished = 0;
  size_t nconv;
  size_t insize,outbuf_size,outbytes_remaining,len;
  const char *inptr;
  char *dest;
  char *outp;

  if (in == NULL || !FiconvSupport)
    return NULL;

  cd = Ficonv_open(out_charset, in_charset);
  if (cd == (Ficonv_t) -1)
  {
    /* Something went wrong.  */
    if (error_count > FICONV_CONVERSION_MAX_NUMBER_OF_WARNING)
      return NULL;
    error_count++;
    if (errno == EINVAL)
    {
      fprintf(stderr, "[FVWM][convert_charsets]: WARNING -\n\t");
      fprintf(stderr, "conversion from `%s' to `%s' not available\n",
	      in_charset,out_charset);
    }
    else
    {
      fprintf(stderr, "[FVWM][convert_charsets]: WARNING -\n\t");
      fprintf(stderr, "conversion from `%s' to `%s' fail (init)\n",
	      in_charset,out_charset);
    }
    /* Terminate the output string.  */
    return NULL;
  }

  /* in maybe a none terminate string */
  len = in_size;

  outbuf_size = len + 1;
  outbytes_remaining = outbuf_size - 1;
  insize = len;
  outp = dest = safemalloc(outbuf_size);

  inptr = in;

  for (is_finished = 0; is_finished == 0; )
  {
#ifdef ICONV_ARG_USE_CONST
    nconv =
      Ficonv(cd, (const char **)&inptr, &insize, &outp, &outbytes_remaining);
#else
    nconv =
      Ficonv(cd, (char **)&inptr,&insize, &outp, &outbytes_remaining);
#endif
    is_finished = 1;
    if (nconv == (size_t) - 1)
    {
      switch (errno)
      {
      case EINVAL:
        /* Incomplete text, do not report an error */
        break;
      case E2BIG:
      {
        size_t used = outp - dest;

        outbuf_size *= 2;
        dest = realloc (dest, outbuf_size);

        outp = dest + used;
        outbytes_remaining = outbuf_size - used - 1; /* -1 for nul */
	is_finished = 0;
        break;
      }
      case EILSEQ:
	    /* Something went wrong.  */
	if (error_count <= FICONV_CONVERSION_MAX_NUMBER_OF_WARNING)
	{
	  fprintf(stderr, "[FVWM][convert_charsets]: WARNING -\n\t");
	  fprintf(stderr,
		  "Invalid byte sequence during conversion from %s to %s\n",
		  in_charset,out_charset);
	}
        have_error = 1;
	break;
      default:
	if (error_count <= FICONV_CONVERSION_MAX_NUMBER_OF_WARNING)
	{
	  fprintf(stderr, "[FVWM][convert_charsets]: WARNING -\n\t");
	  fprintf(stderr, "Error during conversion from %s to %s\n",
		  in_charset,out_charset);
	}
        have_error = 1;
        break;
      }
    }
  }

  /* Terminate the output string  */
  *outp = '\0';

  if (Ficonv_close (cd) != 0)
    fprintf(stderr, "[FVWM][convert_charsets]: WARNING - iconv_close fail\n");

  if (have_error)
  {
    error_count++;
    free (dest);
    return NULL;
  }
  else
    return dest;
}

static
void FiconvInit(Display *dpy, const char *module)
{
	int suc = False;
	char *charset;
	FlocaleCharset *xom_charset;

	if (!FiconvSupport || FiconvInitialized)
		return;

	FiconvInitialized = True;
	FlocaleCharsetInit(dpy, module);
	xom_charset = FlocaleCharsetGetFLCXOMCharset();

	charset = getenv("CHARSET");
	if ((!charset || strlen(charset) < 3) && FlocaleLibcharsetSupport)
		charset = (char *)Flocale_charset();
	if ((!charset || strlen(charset) < 3) && FlocaleCodesetSupport)
		charset = Fnl_langinfo(FCODESET);

	if (charset != NULL && strlen(charset) > 2)
	{
		FLCDefaultIconvCharset = FlocaleCharsetOfLocaleCharset(charset);
	}
	if (FLCDefaultIconvCharset == NULL)
	{
		FLCDefaultIconvCharset = xom_charset;
	}
	if (FLCDefaultIconvCharset == NULL)
	{
		if (charset == NULL)
		{
			fprintf(stderr,
				"[%s][%s]: WARN -- Cannot get locale "
				"charset with:\n\t",
				module, "FiconvInit");
		}
		else
		{
			fprintf(stderr,
				"[%s][%s]: WARN -- Unsupported locale "
				"charset '%s' obtained with:\n",
				module, "FiconvInit", charset);
		}
		fprintf(stderr,"\tCHARSET env variable");
		if (FlocaleLibcharsetSupport)
			fprintf(stderr,", locale_charset");
		if (FlocaleCodesetSupport)
			fprintf(stderr,", nl_langinfo");
		if (FlocaleMultibyteSupport)
			fprintf(stderr,", XOM");
		fprintf(stderr,"\n");
	}

	/* get the fallback charset if the above fail: never null */
	if (FLCDefaultIconvCharset == NULL)
	{
		FLCDefaultIconvCharset =
			FlocaleCharsetOfXCharset(FLOCALE_FALLBACK_XCHARSET);
		fprintf(stderr,"\tUse Default Charset: %s\n",
			FLC_GET_X_CHARSET(FLCDefaultIconvCharset));
	}

#if FLOCALE_DEBUG_CHARSET
	fprintf(stderr,"[Ficonv] locale charset: x:%s, bidi:%s\n",
		FLC_DEBUG_GET_X_CHARSET(FLCDefaultIconvCharset),
		FLC_DEBUG_GET_BIDI_CHARSET (FLCDefaultIconvCharset));      
#endif

	/* never null */
	FLCUtf8Charset = FlocaleCharsetOfXCharset(FLOCALE_UTF8_XCHARSET);

	suc = set_default_iconv_charsets(FLCDefaultIconvCharset);
	if (!suc && FLCDefaultIconvCharset != xom_charset && xom_charset != NULL)
	{
		FLCDefaultIconvCharset = xom_charset;
		suc = set_default_iconv_charsets(FLCDefaultIconvCharset);
	}
	if (!suc)
	{
		fprintf(stderr,
			"[%s][FiconvInit]: WARN -- Cannot get default "
			"iconv charset with:\n", module);
		fprintf(stderr, "\tlocale charset '%s'\n",
			FLC_DEBUG_GET_X_CHARSET(FLCDefaultIconvCharset));
		if (FlocaleMultibyteSupport)
		{
			fprintf(stderr, "\tXOM charset '%s'\n",
				FLC_GET_X_CHARSET(xom_charset));
		}
		FLCUtf8Charset = NULL;
		FLCDefaultIconvCharset = NULL;
	}

#if FLOCALE_DEBUG_CHARSET
fprintf(stderr,"[FiconvInit] iconv charset: x:%s, iconv:%s\n",
		FLC_DEBUG_GET_X_CHARSET(FLCDefaultIconvCharset),
		FLC_DEBUG_GET_ICONV_CHARSET(FLCDefaultIconvCharset));
	fprintf(stderr,"[FiconvInit] UTF-8 charset: x:%s, iconv:%s\n",
		FLC_DEBUG_GET_X_CHARSET(FLCUtf8Charset),
		FLC_DEBUG_GET_ICONV_CHARSET(FLCUtf8Charset));
#endif

}

static
FlocaleCharset *FiconvSetupConversion(Display *dpy, FlocaleFont *flf)
{
	FlocaleCharset *fc;

	if (!FiconvSupport)
		return NULL;

	if (!FiconvInitialized)
		FiconvInit(dpy, "FVWM");

	if (FLCUtf8Charset == NULL)
		return NULL;

	if (flf == NULL)
	{
		fc = FLCDefaultIconvCharset;
	}
	else
	{
		fc = flf->fc;
	}
	if (fc == FlocaleGetUnsetCharset())
	{
		FlocaleCharsetSetFlocaleCharset(dpy, flf);
		fc = flf->fc;
	}
	if (fc == FlocaleCharsetGetUnknownCharset())
		return NULL;

	if (!FLC_DO_ICONV_CHARSET_INITIALIZED(fc))
	{
		set_iconv_charset_index(fc);
#if FLOCALE_DEBUG_CHARSET
		fprintf(stderr, "[Flocale] set up iconv charset: " 
			"x: %s, iconv: %s\n",
			FLC_DEBUG_GET_X_CHARSET(fc),
			FLC_DEBUG_GET_ICONV_CHARSET(fc));
#endif
	}
	if (!FLC_HAVE_ICONV_CHARSET(FLCUtf8Charset) ||
	    !FLC_HAVE_ICONV_CHARSET(fc))
	{
		return NULL;
	}

	return fc;
}

/* ---------------------------- interface functions ------------------------- */

/* conversion from UTF8 to the current charset */
char *FiconvUtf8ToCharset(Display *dpy, FlocaleFont *flf,
			  const char *in, unsigned int in_size)
{
	char *out = NULL;
	FlocaleCharset *fc;

	if (!FiconvSupport || (fc = FiconvSetupConversion(dpy, flf)) == NULL)
	{
		return NULL;
	}

#if FLOCALE_DEBUG_ICONV
	fprintf(stderr, "[FiconvUtf8ToCharset] conversion from %s to %s\n",
		FLC_DEBUG_GET_ICONV_CHARSET(FLCUtf8Charset),
		FLC_DEBUG_GET_ICONV_CHARSET(fc));
#endif

	out = convert_charsets(FLC_GET_ICONV_CHARSET(FLCUtf8Charset),
			       FLC_GET_ICONV_CHARSET(fc),
			       in, in_size);

	return out;
}

/* conversion from the current charset to UTF8 */
char *FiconvCharsetToUtf8(Display *dpy, FlocaleFont *flf,
			  const char *in, unsigned int in_size)
{
	char *out = NULL;
	FlocaleCharset *fc;

	if (!FiconvSupport || (fc = FiconvSetupConversion(dpy, flf)) == NULL)
	{
		return NULL;
	}

#if FLOCALE_DEBUG_ICONV
	fprintf(stderr, "[FiconvCharsetToUtf8] conversion from %s to %s\n",
		FLC_DEBUG_GET_ICONV_CHARSET(fc),
		FLC_DEBUG_GET_ICONV_CHARSET(FLCUtf8Charset));
#endif

	out = convert_charsets(FLC_GET_ICONV_CHARSET(fc),
			       FLC_GET_ICONV_CHARSET(FLCUtf8Charset),
			       in, in_size);

	return out;
}
