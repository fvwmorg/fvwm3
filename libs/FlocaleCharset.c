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

/* ---------------------------- included header files ---------------------- */

#include "config.h"
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "Strings.h"
#include "Parse.h"
#include "Flocale.h"
#include "FlocaleCharset.h"
#include "Ficonv.h"

#if FlocaleLibcharsetSupport
#include <libcharset.h>
#endif
#if FlocaleCodesetSupport
#include <langinfo.h>
#endif

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* X locale charset */
static FlocaleCharset *FLCXOMCharset = NULL;
/* list of XOM locale charset */
static FlocaleCharset **FLCXOMCharsetList = NULL;
static int FLCXOMCharsetList_num = 0;
/* UTF-8 charset */
static FlocaleCharset *FLCUtf8Charset = NULL;
/* locale charset from the locale not X */
static FlocaleCharset *FLCLocaleCharset = NULL;

static char *nullsl[]            = {NULL};

#if FiconvSupport
/* from hpux iconv man page: The fromcode and tocode names can be any length,
 * but only the first four and the last letter are used to identify the code
 * set. */
/* the first name is the gnu canonical name */
char *armscii_8[]         = {"ARMSCII-8", NULL};
char *big5_0[]            = {"BIG5",
			     "big5",       /* aix, hpux, osf */
			     "zh_TW-big5", /* solaris 9? */
			     NULL};
char *big5hkscs_0[]          = {"BIG5-HKSCS",
				"BIG5",
				"big5",       /* aix, hpux, osf */
				"zh_TW-big5", /* solaris 9? */
				NULL};
char *cns11643[]          = {"EUCTW",
			     "EUC-TW",
			     "eucTW",
			     "euc-tw",
			     "euctw",
			     NULL};
char *dosencoding_cp437[] = {"CP437", NULL};
char *dosencoding_cp850[] = {"CP850",
			     "cp850",   /* osf */
			     "IBM-850", /* aix */
			     NULL};
char *gb2312_1980_0[]     = {"EUC-CN",
			     "GB2312",
			     "gb2312",    /* solaris */
			     "dechanzi",  /* osf */
			     "hp15CN",    /* hpux */
			     "IBM-eucCN", /* aix */
			     "eucCN",     /* irix */
			     NULL};
char *gbk_0[]             = {"GBK", NULL};
char *georgian_academy[]  = {"GEORGIAN-ACADEMY", NULL};
char *georgian_ps[]       = {"GEORGIAN-PS", NULL};
char *ibm_cp1133[]        = {"CP850" , NULL};
char *isiri_3342[]        = {"ISIRI-3342", NULL};
/* embed ascii into ISO-8859-1*/
char *iso8859_1[]         = {"ISO-8859-1",  /* gnu */
			     "ISO8859-1",   /* aix, irix, osf, solaris */
			     "iso8859-1",   /* aix, irix, osf */
			     "iso88591",    /* hpux */
			     "8859-1",      /* solaris ? */
			     "iso81",       /* hpux */
			     "ascii",
			     "ASCII",
			     "646", /* solaris */
			     "ANSI_X3.4-1968",
			     "ISO_646.IRV:1983", /* old glibc */
			     "american_e",
			     NULL};
char *iso8859_2[]         = {"ISO-8859-2",
			     "ISO8859-2",
			     "iso8859-2",
			     "iso88592",
			     "8859-2",
			     "iso82",
			     NULL};
char *iso8859_3[]         = {"ISO-8859-3",
			     "ISO8859-3",
			     "iso8859-3",
			     "iso88593",
			     "8859-3",
			     "iso83",
			     NULL};
char *iso8859_4[]         = {"ISO-8859-4",
			     "ISO8859-4",
			     "iso8859-4",
			     "iso88594",
			     "8859-4",
			     "iso84",
			     NULL};
char *iso8859_5[]         = {"ISO-8859-5",
			     "ISO8859-5",
			     "iso8859-5",
			     "iso88595",
			     "8859-5",
			     "iso85",
			     NULL};
char *iso8859_6[]         = {"ISO-8859-6",
			     "ISO8859-6",
			     "iso8859-6",
			     "iso88596",
			     "8859-6",
			     "iso86",
			     NULL};
char *iso8859_7[]         = {"ISO-8859-7",
			     "ISO8859-7",
			     "iso8859-7",
			     "iso88597",
			     "8859-7",
			     "iso87",
			     NULL};
char *iso8859_8[]         = {"ISO-8859-8",
			     "ISO8859-8",
			     "iso88859-8",
			     "iso88598",
			     "8859-8",
			     "iso88",
			     NULL};
char *iso8859_9[]         = {"ISO-8859-9",
			     "ISO8859-9",
			     "iso8859-9",
			     "iso88599",
			     "8859-9",
			     "iso89",
			     NULL};
char *iso8859_10[]        = {"ISO-8859-10",
			     "ISO8859-10",
			     "iso8859-10",
			     "iso885910",
			     "8859-10",
			     "iso80", /*?*/
			     "iso10",
			     NULL};
char *iso8859_13[]        = {"ISO-8859-13",
			     "ISO8859-13",
			     "iso8859-13",
			     "iso885913",
			     "8859-13",
			     "IBM-921", /* aix */
			     "iso813", /*?*/
			     "iso13",
			     NULL};
char *iso8859_14[]        = {"ISO-8859-14",
			     "ISO8859-14",
			     "iso8859-14",
			     "iso885914",
			     "8859-14",
			     "iso814",
			     "iso14",
			     NULL};
char *iso8859_15[]        = {"ISO-8859-15",
			     "ISO88859-15",
			     "iso88859-15",
			     "iso885915",
			     "8859-15",
			     "iso815",
			     "iso15",
			     NULL};
char *iso8859_16[]        = {"ISO-8859-16",
			     "ISO8859-16",
			     "iso8859-16",
			     "iso885916",
			     "8859-16",
			     "iso80",
			     "iso16",
			     NULL};
char *jisx0201_1976_0[]   = {"JIS_X0201",
			     "ISO-IR-14",
			     "jis", /* hpux */
			     NULL};
char *jisx0208_1983_0[]   = {"JIS_X0208", NULL};
char *jisx0208_1990_0[]   = {"JIS_X0208", NULL};
char *jisx0212_1990_0[]   = {"JIS_X0212", NULL};
char *koi8_r[]            = {"KOI8-R", /* gnu, solaris */
			     NULL};
char *koi8_u[]            = {"KOI8-U",
			     NULL};
char *ksc5601[]           = {"KSC5636",
			     "ko_KR-johap", /* solaris */
			     "ko_KR-johap92",
			     "ko_KR-euc",
			     "eucKR",
			     "EUC-KR",
			     "EUCKR",
			     "euc-kr",
			     "CP949", /* osf */
			     NULL};
char *microsoft_cp1251[]  = {"CP1251",
			     NULL};
char *microsoft_cp1255[]  = {"CP1255",
			     NULL};
char *microsoft_cp1256[]  = {"CP1256",
			     NULL};
char *mulelao_1[]         = {"MULELAO-1",
			     "MULELAO",
			     NULL};
char *sjisx_1[]           = {"SHIFT_JIS",
			     "SJIS", /* solaris, osf */
			     "sjis", /* hpux */
			     "PCK",  /* solaris */
			     NULL};
char *tatar_cyr[]         = {NULL};
char *tcvn_5712[]         = {"TCVN", NULL};
char *tis620_0[]          = {"TIS-620",
			     "tis620", /* hpux */
			     "TACTIS", /* osf */
			     NULL};
char *viscii1_1_1[]       = {"VISCII", NULL};
/* encofing only ...*/
char *utf_8[]             =  {"UTF-8", /* gnu, solaris */
			      "UTF8",
			      "utf8",
			      "utf_8",
			      NULL};
char *usc_2[]             = {"USC-2",
			     "USC2",
			     "usc2",
			     "usc-2",
			     "usc_2",
			     NULL};
char *usc_4[]             = {"USC-4",
			     "USC4",
			     "usc4",
			     "usc-4",
			     "usc_4",
			     NULL};
char *utf_16[]             =  {"UTF-16",
			      "UTF16",
			      "utf16",
			      "utf_16",
			      NULL};
char *euc_jp[]            = {"EUC-JP",
			     "EUCJP",
			     "euc_jp",
			     "euc-jp",
			     "eucjp",
			     NULL};
#endif

#if FiconvSupport

#ifdef HAVE_BIDI
#define  CT_ENTRY(x,y,z) \
  {x, y, FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED, z, FLC_ENCODING_TYPE_FONT}
#define  CT_ENTRY_WET(x,y,z,t) \
  {x, y, FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED, z, t}
#else
#define  CT_ENTRY(x,y,z) \
  {x, y, FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED, NULL, FLC_ENCODING_TYPE_FONT}
#define  CT_ENTRY_WET(x,y,z,t) \
  {x, y, FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED, NULL, t}
#endif

#else /* !FlocaleIconvSupport */

#ifdef HAVE_BIDI
#define  CT_ENTRY(x,y,z) \
  {x, nullsl, FLC_INDEX_ICONV_CHARSET_NOT_FOUND, z, FLC_ENCODING_TYPE_FONT}
#define  CT_ENTRY_WET(x,y,z,t) \
  {x, nullsl, FLC_INDEX_ICONV_CHARSET_NOT_FOUND, z, t}
#else
#define  CT_ENTRY(x,y,z) \
  {x, nullsl, FLC_INDEX_ICONV_CHARSET_NOT_FOUND, NULL, FLC_ENCODING_TYPE_FONT}
#define  CT_ENTRY_WET(x,y,z,t) \
  {x, nullsl, FLC_INDEX_ICONV_CHARSET_NOT_FOUND, NULL, t}
#endif

#endif

static
FlocaleCharset UnknownCharset =
	{"Unknown", nullsl, FLC_INDEX_ICONV_CHARSET_NOT_FOUND, NULL};

/* the table contains all Xorg "charset" plus some others */

FlocaleCharset FlocaleCharsetTable[] =
{
	CT_ENTRY("ARMSCII-8",           armscii_8,           NULL),
	CT_ENTRY("BIG5-0",              big5_0,              NULL),
	CT_ENTRY("BIG5HKSCS-0",         big5hkscs_0,         NULL),
	CT_ENTRY("CNS11643.1986-1",     cns11643,     NULL),
	CT_ENTRY("CNS11643.1986-2",     cns11643,     NULL),
	CT_ENTRY("CNS11643.1992-3",     cns11643,     NULL),
	CT_ENTRY("CNS11643.1992-4",     cns11643,     NULL),
	CT_ENTRY("CNS11643.1992-5",     cns11643,     NULL),
	CT_ENTRY("CNS11643.1992-6",     cns11643,     NULL),
	CT_ENTRY("CNS11643.1992-7",     cns11643,     NULL),
	CT_ENTRY("DOSENCODING-CP437",   dosencoding_cp437,   NULL),
	CT_ENTRY("DOSENCODING-CP850",   dosencoding_cp850,   NULL),
	CT_ENTRY("EUC-JP",              euc_jp,              NULL),
	CT_ENTRY("GB2312.1980-0",       gb2312_1980_0,       NULL),
	CT_ENTRY("GBK-0",               gbk_0,               NULL),
	CT_ENTRY("GEORGIAN-ACADEMY",    georgian_academy,    NULL),
	CT_ENTRY("GEORGIAN-PS",         georgian_ps,         NULL),
	CT_ENTRY("IBM-CP1133",          ibm_cp1133,          NULL),
	CT_ENTRY("ISIRI-3342",          isiri_3342,          "ISIRI-3342"),
	/* exception ISO10646-1 implies UTF-8 and not USC-2 ! */
	CT_ENTRY_WET("ISO10646-1",  utf_8,  "UTF-8",  FLC_ENCODING_TYPE_UTF_8),
	CT_ENTRY("ISO8859-1",           iso8859_1,           NULL),
	CT_ENTRY("ISO8859-2",           iso8859_2,           NULL),
	CT_ENTRY("ISO8859-3",           iso8859_3,           NULL),
	CT_ENTRY("ISO8859-4",           iso8859_4,           NULL),
	CT_ENTRY("ISO8859-5",           iso8859_5,           NULL),
	CT_ENTRY("ISO8859-6",           iso8859_6,           "ISO8859-6"),
	CT_ENTRY("ISO8859-6.8X",        iso8859_6,           "ISO8859-6"),
	CT_ENTRY("ISO8859-7",           iso8859_7,           NULL),
	CT_ENTRY("ISO8859-8",           iso8859_8,           "ISO8859-8"),
	CT_ENTRY("ISO8859-9",           iso8859_9,           NULL),
	CT_ENTRY("ISO8859-9E",          iso8859_9,           NULL),
	CT_ENTRY("ISO8859-10",          iso8859_10,          NULL),
	CT_ENTRY("ISO8859-13",          iso8859_13,          NULL),
	CT_ENTRY("ISO8859-14",          iso8859_14,          NULL),
	CT_ENTRY("ISO8859-15",          iso8859_15,          NULL),
	CT_ENTRY("ISO8859-16",          iso8859_16,          NULL),
	CT_ENTRY("JISX.UDC-1",          jisx0201_1976_0,     NULL), /* ? */
	CT_ENTRY("JISX0201.1976-0",     jisx0201_1976_0,     NULL),
	CT_ENTRY("JISX0208.1983-0",     jisx0208_1983_0,     NULL),
	CT_ENTRY("JISX0208.1990-0",     jisx0208_1990_0,     NULL),
	CT_ENTRY("JISX0212.1990-0",     jisx0212_1990_0,     NULL),
	CT_ENTRY("KOI8-C",              koi8_r,              NULL),
	CT_ENTRY("KOI8-R",              koi8_r,              NULL),
	CT_ENTRY("KOI8-U",              koi8_u,              NULL),
	CT_ENTRY("KSC5601.1987-0",      ksc5601,             NULL),
	CT_ENTRY("KSC5601.1992-0",      ksc5601,             NULL),
	CT_ENTRY("MICROSOFT-CP1251",    microsoft_cp1251,    NULL),
	CT_ENTRY("MICROSOFT-CP1255",    microsoft_cp1255,    "CP1255"),
	CT_ENTRY("MICROSOFT-CP1256",    microsoft_cp1256,    "CP1256"),
	CT_ENTRY("MULELAO-1",           mulelao_1,           NULL),
	CT_ENTRY("SJISX-1",             sjisx_1,             NULL),
	CT_ENTRY("TATAR-CYR",           tatar_cyr,           NULL),
	CT_ENTRY("TCVN-5712",           tcvn_5712,           NULL),
	CT_ENTRY("TIS620-0",            tis620_0,            NULL),
	CT_ENTRY("VISCII1.1-1",         viscii1_1_1,         NULL),
	/* aliases */
	/* CT_ENTRY("ADOBE-STANDARD",   iso8859_1,           NULL), no! */
	CT_ENTRY("ASCII-0",             iso8859_1,           NULL),
	CT_ENTRY("BIBIG5HKSCS",         big5_0,              NULL), /* ? */
	CT_ENTRY("BIG5-E0",             big5_0,              NULL), /* emacs */
	CT_ENTRY("BIG5-E1",             big5_0,              NULL), /* emacs */
	CT_ENTRY("BIG5-1",              big5_0,              NULL),
	CT_ENTRY("ISO646.1991-IRV",     iso8859_1,           NULL),
	CT_ENTRY("TIS620.2533-1",       tis620_0,            NULL),
	CT_ENTRY_WET("UTF-8",  utf_8,  "UTF-8",  FLC_ENCODING_TYPE_UTF_8),
	CT_ENTRY_WET("USC-2",  usc_2,  "USC-2",  FLC_ENCODING_TYPE_USC_2),
	CT_ENTRY_WET("USC-4",  usc_4,  NULL,     FLC_ENCODING_TYPE_USC_4),
	CT_ENTRY_WET("UTF-16", utf_16, NULL,     FLC_ENCODING_TYPE_UTF_16),
	CT_ENTRY(NULL, nullsl, NULL)
};

/* to be supported: ADOBE-STANDARD, some Xft1 charset  */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static
FlocaleCharset *FlocaleCharsetOfXCharset(char *x)
{
	int j = 0;

	while(FlocaleCharsetTable[j].x != NULL)
	{
		if (StrEquals(x,FlocaleCharsetTable[j].x))
		{
			return &FlocaleCharsetTable[j];
		}
		j++;
	}
	return NULL;
}

static
FlocaleCharset *FlocaleCharsetOfLocaleCharset(char *l)
{
	int j = 0, i = 0;

	while(FlocaleCharsetTable[j].x != NULL)
	{
		if (StrEquals(l, FlocaleCharsetTable[j].x))
		{
			return &FlocaleCharsetTable[j];
		}
		i = 0;
		while(FlocaleCharsetTable[j].locale[i] != NULL)
		{
			if (StrEquals(l, FlocaleCharsetTable[j].locale[i]))
			{
				return &FlocaleCharsetTable[j];
			}
			i++;
		}
		j++;
	}
	return NULL;
}

static
FlocaleCharset *FlocaleCharsetOfFontStruct(Display *dpy, XFontStruct *fs)
{
	unsigned long value = 0;
	char *name,*tmp;
	FlocaleCharset *fc;
	int count = 0;

	if (fs == NULL)
		return NULL;

	if (!XGetFontProperty(fs, XA_FONT, &value))
	{
		return NULL;
	}
	if ((name = XGetAtomName(dpy, value)) == NULL)
	{
		return NULL;
	}

	tmp = name;
	while (*tmp != '\0' && count < 13)
	{
		if (*tmp == '-')
		{
			count++;
		}
		tmp++;
		if (count == 13)
		{
			fc = FlocaleCharsetOfXCharset(tmp);
			XFree(name);
			return fc;
		}
	}
	XFree(name);
	return NULL;
}

static
void FlocaleInit_X_Charset(Display *dpy, const char *module)
{
#ifdef HAVE_XOUTPUT_METHOD
	XOM om;
	XOMCharSetList cs;
	int i;

	om = XOpenOM(dpy, NULL, NULL, NULL);
	if (om && XGetOMValues(om, XNRequiredCharSet, &cs, NULL) == NULL)
	{
		if (cs.charset_count > 0)
		{
			if (FLCXOMCharsetList != NULL)
			{
				free(FLCXOMCharsetList);
			}
			FLCXOMCharsetList_num = cs.charset_count;
			FLCXOMCharsetList = xmalloc(
				sizeof(FlocaleCharset) * cs.charset_count);
			for (i = 0; i <  FLCXOMCharsetList_num; i++)
			{
				FLCXOMCharsetList[i] =
					FlocaleCharsetOfXCharset(
						cs.charset_list[i]);
#if FLOCALE_DEBUG_CHARSET
				fprintf(stderr,
					"[FlocaleInitCharset] XOM charset "
					"%i: %s, bidi:%s\n",
					i,
					FLC_DEBUG_GET_X_CHARSET(
						FLCXOMCharsetList[i]),
					FLC_DEBUG_GET_BIDI_CHARSET (
						FLCXOMCharsetList[i]));
#endif
			}
		}
	}
	if (om)
	{
		XCloseOM(om);
	}
	if (FLCXOMCharsetList_num > 0 && FLCXOMCharsetList[0])
	{
		char *best_charset;

		if (FLCLocaleCharset != NULL)
		{
			best_charset = FLCLocaleCharset->x;
#if FLOCALE_DEBUG_CHARSET
			fprintf(stderr,
				"[FlocaleInitCharset] FLCLocaleCharset: %s\n",
				best_charset);
#endif
		}
		else
		{
			best_charset = FLOCALE_FALLBACK_XCHARSET;
#if FLOCALE_DEBUG_CHARSET
			fprintf(stderr,
				"[FlocaleInitCharset] FALLBACK: %s\n",
				best_charset);
#endif
		}

		FLCXOMCharset = FLCXOMCharsetList[0];
		if (best_charset == NULL)
		{
			/* should not happen */
		}
		else
		{
			for(i = 0; i <  FLCXOMCharsetList_num; i++)
			{
				if (StrEquals(
					best_charset,
					FLC_DEBUG_GET_X_CHARSET(
						FLCXOMCharsetList[i])))
				{
					FLCXOMCharset = FLCXOMCharsetList[i];
					break;
				}
			}
		}
#if FLOCALE_DEBUG_CHARSET
		fprintf(stderr,
			"[FlocaleInitCharset] XOM charset "
			"%i: %s\n",
			i,
			FLC_DEBUG_GET_X_CHARSET(FLCXOMCharset));
#endif
	}
#endif
}

/* ---------------------------- interface functions ------------------------ */

void FlocaleCharsetInit(Display *dpy, const char *module)
{
	static Bool initialized = False;
	char *charset;

	if (initialized == True)
	{
		return;
	}
	initialized = True;

	/* try to find the regular charset */
	charset = getenv("CHARSET");
#if FLOCALE_DEBUG_CHARSET
	fprintf(stderr,
		"[FlocaleInitCharset] CHARSET: %s\n", (!charset)? "null":charset);
#endif
	if ((!charset || strlen(charset) < 3) && FlocaleLibcharsetSupport)
	{
		charset = (char *)Flocale_charset();
#if FLOCALE_DEBUG_CHARSET
		fprintf(
			stderr,
			"[FlocaleInitCharset] FlocaleLibcharsetSupport: %s\n",
			(!charset)? "null":charset);
#endif
	}
	if ((!charset || strlen(charset) < 3) && FlocaleCodesetSupport)
	{
		charset = Fnl_langinfo(FCODESET);
#if FLOCALE_DEBUG_CHARSET
		fprintf(
			stderr,
			"[FlocaleInitCharset] Fnl_langinfo: %s\n",
			(!charset)? "null":charset);
#endif
	}
	if (charset != NULL && strlen(charset) > 2)
	{
		FLCLocaleCharset =
			FlocaleCharsetOfLocaleCharset(charset);
#if FLOCALE_DEBUG_CHARSET
		fprintf(
			stderr,
			"[FlocaleInitCharset] FLCLocaleCharset: %s\n", charset);
#endif
	}

	/* set the defaults X locale charsets */
	FlocaleInit_X_Charset(dpy, module);

	/* never null */
	FLCUtf8Charset = FlocaleCharsetOfXCharset(FLOCALE_UTF8_XCHARSET);

#if FLOCALE_DEBUG_CHARSET
	fprintf(stderr,"[FlocaleCharsetInit] locale charset: x:%s, bidi:%s\n",
		FLC_DEBUG_GET_X_CHARSET(FLCXOMCharset),
		FLC_DEBUG_GET_BIDI_CHARSET (FLCXOMCharset));
	fprintf(stderr,"[FlocaleCharsetInit] locale charset: x:%s, bidi:%s\n",
		FLC_DEBUG_GET_X_CHARSET(FLCLocaleCharset),
		FLC_DEBUG_GET_BIDI_CHARSET (FLCLocaleCharset));
#endif

	return;
}

void FlocaleCharsetSetFlocaleCharset(
	Display *dpy, FlocaleFont *flf, char *hints, char *encoding,
	char *module)
{
	char *charset = NULL;
	char *iconv = NULL;
	Bool iconv_found = False;
	int i = 0;

	FlocaleCharsetInit(dpy, module);

	if (hints && *hints)
	{
		iconv = GetQuotedString(
			hints, &charset, "/", NULL, NULL, NULL);
		if (charset && *charset && *charset != '*' )
		{
			flf->fc = FlocaleCharsetOfXCharset(charset);
		}
		if (flf->fc == NULL && charset && *charset && *charset != '*')
		{
			flf->fc = FlocaleCharsetOfLocaleCharset(charset);
		}
		if (flf->fc == NULL && iconv && *iconv)
		{
			flf->fc = FlocaleCharsetOfLocaleCharset(iconv);
		}
	}
	if (flf->fc == NULL)
	{
		if (FftSupport && flf->fftf.fftfont != NULL)
		{
			flf->fc = FlocaleCharsetOfXCharset(flf->fftf.encoding);
		}
		else if (flf->fontset != None)
		{
			if (FLCXOMCharset != NULL)
			{
				flf->fc = FLCXOMCharset;
			}
			else
			{
				/* we are here if !HAVE_XOUTPUT_METHOD */
				XFontStruct **fs_list;
				char **ml;

				if (XFontsOfFontSet(
					    flf->fontset, &fs_list, &ml) > 0)
				{
					flf->fc = FLCXOMCharset =
						FlocaleCharsetOfFontStruct(
							dpy, fs_list[0]);
				}
			}
		}
		else if (flf->font != NULL)
		{
			flf->fc = FlocaleCharsetOfFontStruct(dpy, flf->font);
		}
	}
	if (flf->fc != NULL && iconv && *iconv)
	{
		/* the user has specified an iconv converter name:
		 * check if we have it and force user choice */
		while(!iconv_found &&
		      FLC_GET_LOCALE_CHARSET(flf->fc,i) != NULL)
		{
			if (
				strcmp(
					iconv,
					FLC_GET_LOCALE_CHARSET(flf->fc,i)) ==
				0)
			{
				iconv_found = True;
				/* Trust the user? yes ... */
				FLC_SET_ICONV_INDEX(flf->fc,i);
			}
			i++;
		}
	}
	if (iconv && *iconv && !iconv_found)
	{
		FlocaleCharset *fc;

		/* the user has specified an iconv converter name and we do not
		 * have it: must create a FlocaleCharset */
		flf->flags.must_free_fc = True;
		fc = xmalloc(sizeof(FlocaleCharset));
		if (flf->fc != NULL)
		{
			CopyString(&fc->x, flf->fc->x);
			fc->encoding_type = flf->fc->encoding_type;
			if (flf->fc->bidi)
				CopyString(&fc->bidi, flf->fc->bidi);
			else
				fc->bidi = NULL;
		}
		else
		{
			CopyString(&fc->x, "Unknown"); /* for simplicity */
			fc->bidi = NULL;
			fc->encoding_type = FLC_ENCODING_TYPE_FONT;
		}
		fc->locale = xmalloc(2*sizeof(char *));
		CopyString(&fc->locale[0], iconv);
		fc->locale[1] = NULL;
		fc->iconv_index =  FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED;
		flf->fc = fc;
	}
	if (charset != NULL)
	{
		free(charset);
	}
	if (flf->fc == NULL)
	{
		flf->fc = &UnknownCharset;
	}

	/* now the string charset */
	if (encoding != NULL)
	{
		flf->str_fc = FlocaleCharsetOfXCharset(encoding);
		if (flf->str_fc == NULL)
		{
			flf->str_fc = FlocaleCharsetOfLocaleCharset(encoding);
		}
		if (flf->str_fc == NULL)
		{
			flf->str_fc = &UnknownCharset;
		}
	}
	else if (FftSupport && flf->fftf.fftfont != NULL)
	{
		if (flf->fftf.str_encoding != NULL)
		{
			flf->str_fc = FlocaleCharsetOfXCharset(
				flf->fftf.str_encoding);
			if (flf->str_fc == NULL)
			{
				flf->str_fc = FlocaleCharsetOfLocaleCharset(
					flf->fftf.str_encoding);
			}
			if (flf->str_fc == NULL)
			{
				flf->str_fc = &UnknownCharset;
			}
		}
		else
		{
			flf->str_fc =
				FlocaleCharsetGetDefaultCharset(dpy, module);
		}
	}
	if (flf->str_fc == NULL)
	{
		if (flf->fc != &UnknownCharset)
		{
			flf->str_fc = flf->fc;
		}
		else
		{
			flf->str_fc =
				FlocaleCharsetGetDefaultCharset(dpy, module);
		}
	}
}

FlocaleCharset *FlocaleCharsetGetDefaultCharset(Display *dpy, char *module)
{
	static int warn = True;

	FlocaleCharsetInit(dpy, module);

	if (FLCXOMCharset != NULL)
		return FLCXOMCharset;
	if (FLCLocaleCharset != NULL)
		return FLCLocaleCharset;

	if (warn)
	{
		warn = False;
		fprintf(stderr,
			"[%s][%s]: WARN -- Cannot find default locale "
			"charset with:\n\t",
			(module != NULL)? module:"FVWMlibs",
			"FlocaleGetDefaultCharset");
		fprintf(stderr,"X Output Method ");
		fprintf(stderr,", CHARSET env variable");
		if (FlocaleLibcharsetSupport)
			fprintf(stderr,", locale_charset");
		if (FlocaleCodesetSupport)
			fprintf(stderr,", nl_langinfo");
		fprintf(stderr,"\n");
		/* never null */
		FLCLocaleCharset =
			FlocaleCharsetOfXCharset(FLOCALE_FALLBACK_XCHARSET);
		fprintf(stderr,"\tUse default charset: %s\n",
			FLOCALE_FALLBACK_XCHARSET);

	}
	return FLCLocaleCharset;
}

FlocaleCharset *FlocaleCharsetGetFLCXOMCharset(void)
{
	return FLCXOMCharset;
}

FlocaleCharset *FlocaleCharsetGetUtf8Charset(void)
{
	return FLCUtf8Charset;
}

FlocaleCharset *FlocaleCharsetGetLocaleCharset(void)
{
	return FLCLocaleCharset;
}

FlocaleCharset *FlocaleCharsetGetUnknownCharset(void)
{
	return &UnknownCharset;
}

const char *FlocaleGetBidiCharset(Display *dpy, FlocaleCharset *fc)
{
	if (fc == NULL || fc == FlocaleCharsetGetUnknownCharset() ||
	    fc->bidi == NULL)
	{
		return NULL;
	}
	return (const char *)fc->bidi;
}


FlocaleCharset *FlocaleCharsetGetEUCJPCharset(void)
{
	static FlocaleCharset *fc = NULL;

	if (fc != NULL)
		return fc;

	/* never null */
	fc = FlocaleCharsetOfXCharset("EUC-JP");
	return fc;
}

Bool FlocaleCharsetIsCharsetXLocale(Display *dpy, char *charset, char *module)
{
#ifdef HAVE_XOUTPUT_METHOD
	int i;

	FlocaleCharsetInit(dpy, module);
	if (FLCXOMCharsetList_num > 0)
	{
		for(i = 0; i <  FLCXOMCharsetList_num; i++)
		{
			if (StrEquals(
				    FLC_DEBUG_GET_X_CHARSET(
					    FLCXOMCharsetList[i]),
				    charset))
			{
				return True;
			}
		}
	}
	return False;
#else
	return True; /* Hum */
#endif
}

void FlocaleCharsetPrintXOMInfo(void)
{
#ifdef HAVE_XOUTPUT_METHOD
	int i;

	fprintf(stderr,"  XOM Charsets: ");
	if (FLCXOMCharsetList_num > 0)
	{
		for(i = 0; i <  FLCXOMCharsetList_num; i++)
		{
			fprintf(
				stderr, "%s ",
				FLC_DEBUG_GET_X_CHARSET(FLCXOMCharsetList[i]));
		}
	}
	fprintf(stderr,"\n");
#endif
}
