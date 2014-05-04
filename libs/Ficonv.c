/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */
/* This program is free software; you can redistribute it and/or modify
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/* Some code (convert_charsets) inspired by the glib-2 (gutf8.c) copyrighted
 * by Tom Tromey & Red Hat, Inc. */

/* ---------------------------- included header files ---------------------- */

#include "config.h"
#include <stdio.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "FlocaleCharset.h"
#include "Ficonv.h"
#include "Strings.h"

#if FiconvSupport
#include <iconv.h>

#if defined(USE_LIBICONV) && !defined (_LIBICONV_H)
#error libiconv in use but included iconv.h not from libiconv
#endif
#if !defined(USE_LIBICONV) && defined (_LIBICONV_H) && defined (LIBICONV_PLUG)
#error libiconv not in use but included iconv.h is from libiconv
#endif
#endif /* FiconvSupport */

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static Bool FiconvInitialized = False;
static FlocaleCharset *FLCIconvUtf8Charset = NULL;       /* UTF-8 charset */
static FlocaleCharset *FLCIconvDefaultCharset = NULL;
static int do_transliterate_utf8 = 0;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static
Bool is_iconv_supported(char *c1, char *c2)
{
	Ficonv_t cd1,cd2;
	Bool r = False;

	if (!FiconvSupport || !c1 || !c2)
		return False;

	cd1 = Ficonv_open(c1, c2);
	cd2 = Ficonv_open(c2, c1);
	if (cd1 != (Ficonv_t) -1 && cd2 != (Ficonv_t) -1)
		r = True;
	if (cd1 != (Ficonv_t) -1)
		(void)Ficonv_close(cd1);
	if (cd2 != (Ficonv_t) -1)
		(void)Ficonv_close(cd2);
	return r;
}

#define TRANSLIT_SUFFIX "//TRANSLIT"
static
char *translit_csname(char *cs)
{
	return CatString2(cs, TRANSLIT_SUFFIX);
}

static
int is_translit_supported(char *c1, char *c2)
{
	Ficonv_t cd;

	if (!FiconvSupport || !c1 || !c2)
		return 0;

	cd = Ficonv_open(translit_csname(c1),c2);
	if (cd == (Ficonv_t) -1)
	{
		return 0;
	}
	(void)Ficonv_close(cd);
	cd = Ficonv_open(translit_csname(c2),c1);
	if (cd == (Ficonv_t) -1)
	{
		return 0;
	}
	(void)Ficonv_close(cd);

	return 1;
}

static
int set_default_iconv_charsets(FlocaleCharset *fc)
{
	int i=0,j=0;

	if (!FiconvSupport || FLCIconvUtf8Charset == NULL || fc == NULL)
		return False;

	while(FLC_GET_LOCALE_CHARSET(FLCIconvUtf8Charset,i) != NULL)
	{
		j = 0;
		while(FLC_GET_LOCALE_CHARSET(fc,j) != NULL)
		{
			if (is_iconv_supported(
				  FLC_GET_LOCALE_CHARSET(
					  FLCIconvUtf8Charset,i),
				  FLC_GET_LOCALE_CHARSET(fc,j)))
			{
				FLC_SET_ICONV_INDEX(FLCIconvUtf8Charset,i);
				FLC_SET_ICONV_INDEX(fc,j);

				if (is_translit_supported(
					    FLC_GET_LOCALE_CHARSET(
						    FLCIconvUtf8Charset,i),
					    FLC_GET_LOCALE_CHARSET(fc,j)))
				{
					FLC_SET_ICONV_TRANSLIT_CHARSET(
						fc, xstrdup(translit_csname(
							FLC_GET_LOCALE_CHARSET(
								fc,j))));
				} else {
					FLC_SET_ICONV_TRANSLIT_CHARSET(
						fc, FLC_TRANSLIT_NOT_SUPPORTED);
				}

				return 1;
			}
			j++;
		}
		i++;
	}
	FLC_SET_ICONV_INDEX(FLCIconvUtf8Charset,
			    FLC_INDEX_ICONV_CHARSET_NOT_FOUND);
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
	if (FLCIconvUtf8Charset == NULL ||
	    !FLC_DO_ICONV_CHARSET_INITIALIZED(FLCIconvUtf8Charset))
	{
		FLC_SET_ICONV_INDEX(fc, FLC_INDEX_ICONV_CHARSET_NOT_FOUND);
		return;
	}
	while(FLC_GET_LOCALE_CHARSET(fc,i) != NULL)
	{
		if (is_iconv_supported(
			FLC_GET_ICONV_CHARSET(FLCIconvUtf8Charset),
			FLC_GET_LOCALE_CHARSET(fc,i)))
		{
			FLC_SET_ICONV_INDEX(fc,i);
			if (is_translit_supported(
				    FLC_GET_ICONV_CHARSET(FLCIconvUtf8Charset),
				    FLC_GET_LOCALE_CHARSET(fc,i)))
			{
				FLC_SET_ICONV_TRANSLIT_CHARSET(
					fc, xstrdup(
						translit_csname(
							FLC_GET_LOCALE_CHARSET(
								fc,i))));
			} else {
				FLC_SET_ICONV_TRANSLIT_CHARSET(
					fc, FLC_TRANSLIT_NOT_SUPPORTED);
			}
			return;
		}
		i++;
	}
	FLC_SET_ICONV_INDEX(fc, FLC_INDEX_ICONV_CHARSET_NOT_FOUND);
}

static
char *convert_charsets(const char *in_charset, const char *out_charset,
		       const char *in, unsigned int in_size)
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
			fprintf(
				stderr,
				"[fvwm][convert_charsets]: WARNING -\n\t");
			fprintf(
				stderr,
				"conversion from `%s' to `%s' not available\n",
				in_charset,out_charset);
		}
		else
		{
			fprintf(
				stderr,
				"[fvwm][convert_charsets]: WARNING -\n\t");
			fprintf(
				stderr,
				"conversion from `%s' to `%s' fail (init)\n",
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
	outp = dest = xmalloc(outbuf_size);

	inptr = in;

	for (is_finished = 0; is_finished == 0; )
	{
		SUPPRESS_UNUSED_VAR_WARNING(inptr);
		SUPPRESS_UNUSED_VAR_WARNING(insize);
		SUPPRESS_UNUSED_VAR_WARNING(outbytes_remaining);
		nconv = Ficonv(
			cd, (ICONV_ARG_CONST char **)&inptr, &insize, &outp,
			&outbytes_remaining);
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
				/* -1 for nul */
				outbytes_remaining = outbuf_size - used - 1;
				is_finished = 0;
				break;
			}
#ifdef EILSEQ
			case EILSEQ:
				/* Something went wrong.  */
				if (error_count <=
				    FICONV_CONVERSION_MAX_NUMBER_OF_WARNING)
				{
					fprintf(
						stderr,
						"[fvwm][convert_charsets]:"
						" WARNING -\n\t");
					fprintf(
						stderr,
						"Invalid byte sequence during"
						" conversion from %s to %s\n",
						in_charset,out_charset);
				}
				have_error = 1;
				break;
#endif
			default:
				if (error_count <=
				    FICONV_CONVERSION_MAX_NUMBER_OF_WARNING)
				{
					fprintf(
						stderr,
						"[fvwm][convert_charsets]:"
						" WARNING -\n\t");
					fprintf(
						stderr,
						"Error during conversion from"
						" %s to %s\n", in_charset,
						out_charset);
				}
				have_error = 1;
				break;
			}
		}
	}

	/* Terminate the output string  */
	*outp = '\0';

	if (Ficonv_close (cd) != 0)
	{
		fprintf(
			stderr,
			"[fvwm][convert_charsets]: WARNING - iconv_close"
			" fail\n");
	}

	if (have_error)
	{
		error_count++;
		free (dest);
		return NULL;
	}
	else
	{
		return dest;
	}
}

static
void FiconvInit(Display *dpy, const char *module)
{
	int suc = False;

	if (!FiconvSupport || FiconvInitialized)
		return;

	FiconvInitialized = True;
	FlocaleCharsetInit(dpy, module);
	FLCIconvUtf8Charset = FlocaleCharsetGetUtf8Charset();
	FLCIconvDefaultCharset = FlocaleCharsetGetFLCXOMCharset();

	suc = set_default_iconv_charsets(FLCIconvDefaultCharset);
	if (!suc)
	{
		FLCIconvDefaultCharset = FlocaleCharsetGetLocaleCharset();
		suc = set_default_iconv_charsets(FLCIconvDefaultCharset);
	}
	if (!suc)
	{
		fprintf(stderr,
		       "[%s][FiconvInit]: WARN -- Cannot get default "
		       "iconv charset for default charsets '%s' and '%s'\n",
		       module,
		       FLC_DEBUG_GET_X_CHARSET(
			       FlocaleCharsetGetFLCXOMCharset()),
		       FLC_DEBUG_GET_X_CHARSET(FLCIconvDefaultCharset));
		FLCIconvUtf8Charset = NULL;
		FLCIconvDefaultCharset = NULL;
	}

#if FLOCALE_DEBUG_CHARSET
	fprintf(stderr,"[FiconvInit] iconv charset: x:%s, iconv:%s\n",
		FLC_DEBUG_GET_X_CHARSET(FLCIconvDefaultCharset),
		FLC_DEBUG_GET_ICONV_CHARSET(FLCIconvDefaultCharset));
	fprintf(stderr,"[FiconvInit] UTF-8 charset: x:%s, iconv:%s\n",
		FLC_DEBUG_GET_X_CHARSET(FLCIconvUtf8Charset),
		FLC_DEBUG_GET_ICONV_CHARSET(FLCIconvUtf8Charset));
#endif

}

static
FlocaleCharset *FiconvSetupConversion(Display *dpy, FlocaleCharset *fc)
{
	FlocaleCharset *my_fc = NULL;

	if (!FiconvSupport)
	{
		return NULL;
	}

	if (!FiconvInitialized)
	{
		FiconvInit(dpy, "fvwm");
	}

	if (FLCIconvUtf8Charset == NULL)
	{
		return NULL;
	}

	if (fc == NULL)
	{
		my_fc = FLCIconvDefaultCharset;
		if (my_fc == NULL)
		{
			return NULL;
		}
	}
	else
	{
		my_fc = fc;
	}

	if (!FLC_DO_ICONV_CHARSET_INITIALIZED(my_fc))
	{
		set_iconv_charset_index(my_fc);
#if FLOCALE_DEBUG_CHARSET
		fprintf(stderr, "[Flocale] set up iconv charset: "
			"x: %s, iconv: %s\n",
			FLC_DEBUG_GET_X_CHARSET(my_fc),
			FLC_DEBUG_GET_ICONV_CHARSET(my_fc));
#endif
		if (!FLC_HAVE_ICONV_CHARSET(my_fc))
		{
			fprintf(
				stderr,
				"[fvwmlibs] cannot get iconv converter "
				"for charset %s\n",
				FLC_DEBUG_GET_X_CHARSET(my_fc));
			return NULL;
		}
	}
	if (!FLC_HAVE_ICONV_CHARSET(my_fc))
	{
		return NULL;
	}

	return my_fc;
}

/* ---------------------------- interface functions ------------------------ */

/* set transliteration state */
void FiconvSetTransliterateUtf8(int toggle)
{
	switch (toggle)
	{
	case -1:
		do_transliterate_utf8 ^= 1;
		break;
	case 0:
	case 1:
		do_transliterate_utf8 = toggle;
		break;
	default:
		do_transliterate_utf8 = 0;
	}

}


/* conversion from UTF8 to the "current" charset */
char *FiconvUtf8ToCharset(Display *dpy, FlocaleCharset *fc,
			  const char *in, unsigned int in_size)
{
	char *out = NULL;
	FlocaleCharset *my_fc = NULL;

	if (!FiconvSupport)
	{
		return NULL;
	}

	my_fc = FiconvSetupConversion(dpy, fc);
	if (my_fc == NULL)
	{
		return NULL;
	}

#if FLOCALE_DEBUG_ICONV
	fprintf(stderr, "[FiconvUtf8ToCharset] conversion from %s to %s\n",
		FLC_DEBUG_GET_ICONV_CHARSET(FLCIconvUtf8Charset),
		FLC_DEBUG_GET_ICONV_CHARSET(my_fc));
#endif

	if (FLC_ENCODING_TYPE_IS_UTF_8(my_fc))
	{
		/* in can be a none terminate string so do not use CopyString
		 */
		/* TA:  FIXME!  xasprintf() */
		out = xmalloc(in_size+1);
		strncpy(out, in, in_size);
		out[in_size]=0;
	}
	else
	{
		char *to_cs;
		if (do_transliterate_utf8 && FLC_IS_TRANSLIT_SUPPORTED(my_fc))
		{
			to_cs = FLC_GET_ICONV_TRANSLIT_CHARSET(my_fc);
		}
		else
		{
			to_cs = FLC_GET_ICONV_CHARSET(my_fc);
		}
		out = convert_charsets(
				FLC_GET_ICONV_CHARSET(FLCIconvUtf8Charset),
				to_cs,
				in, in_size);
	}

	return out;
}

/* conversion from the current charset to UTF8 */
char *FiconvCharsetToUtf8(Display *dpy, FlocaleCharset *fc,
			  const char *in, unsigned int in_size)
{
	char *out = NULL;
	FlocaleCharset *my_fc = NULL;

	if (!FiconvSupport)
	{
		return NULL;
	}

	my_fc = FiconvSetupConversion(dpy, fc);
	if (my_fc == NULL)
	{
		return NULL;
	}

#if FLOCALE_DEBUG_ICONV
	fprintf(stderr, "[FiconvCharsetToUtf8] conversion from %s to %s\n",
		FLC_DEBUG_GET_ICONV_CHARSET(my_fc),
		FLC_DEBUG_GET_ICONV_CHARSET(FLCIconvUtf8Charset));
#endif

	if (FLC_ENCODING_TYPE_IS_UTF_8(my_fc))
	{
		/* in can be a non terminate string so do not use CopyString */
		/* TA:  FIXME!  xasprintf() */
		out = xmalloc(in_size+1);
		strncpy(out, in, in_size);
		out[in_size]=0;
	}
	else
	{
		out = convert_charsets(
				FLC_GET_ICONV_CHARSET(my_fc),
				FLC_GET_ICONV_CHARSET(FLCIconvUtf8Charset),
				in, in_size);
	}
	return out;
}

/* conversion from charset to charset */
char *FiconvCharsetToCharset(
	Display *dpy, FlocaleCharset *in_fc, FlocaleCharset *out_fc,
	const char *in, unsigned int in_size)
{
	char *out = NULL;
	char *tmp = NULL;
	int tmp_len;
	Bool free_tmp = False;
	FlocaleCharset *my_in_fc;
	FlocaleCharset *my_out_fc;

	if (!FiconvSupport ||
	    (my_in_fc = FiconvSetupConversion(dpy, in_fc)) == NULL)
	{
		return NULL;
	}
	if (!FiconvSupport ||
	    (my_out_fc = FiconvSetupConversion(dpy, out_fc)) == NULL)
	{
		return NULL;
	}

	tmp = (char *)in;
	tmp_len = in_size;
	if (!FLC_ENCODING_TYPE_IS_UTF_8(my_in_fc))
	{
		tmp = FiconvCharsetToUtf8(
			dpy, my_in_fc, (const char *)in, in_size);
		if (tmp != NULL)
		{
			free_tmp = True;
			tmp_len = strlen(tmp);
		}
		else
		{
			/* fail to convert */
			return NULL;
		}
	}

	out = tmp;
	if (!FLC_ENCODING_TYPE_IS_UTF_8(my_out_fc))
	{
		out = FiconvUtf8ToCharset(
			dpy, my_out_fc, (const char *)tmp, tmp_len);
		if (free_tmp)
		{
			free(tmp);
		}
	}

	return out;
}
