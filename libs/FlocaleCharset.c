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

/* ---------------------------- included header files ----------------------- */

#include <config.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "safemalloc.h"
#include "Strings.h"
#include "Parse.h"
#include "FlocaleCharset.h"
#include "Ficonv.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static FlocaleCharset *FLCXOMCharset = NULL;	    /* X locale charset */
/* list of XOM locale charset */
static FlocaleCharset **FLCXOMCharsetList = NULL;
static int FLCXOMCharsetList_num = 0;

static char *nullsl[]		 = {NULL};

#if FiconvSupport
/* from hpux iconv man page: The fromcode and tocode names can be any length,
 * but only the first four and the last letter are used to identify the code
 * set. */
/* the first name is the gnu canonical name */
char *armscii_8[]	  = {"ARMSCII-8", NULL};
char *big5_0[]		  = {"BIG5",
			     "big5",	   /* aix, hpux, osf */
			     "zh_TW-big5", /* solaris 9? */
			     NULL};
char *cns11643_1986_1[]	  = {NULL};
char *cns11643_1986_2[]	  = {NULL};
char *cns11643_1992_3[]	  = {NULL};
char *cns11643_1992_4[]	  = {NULL};
char *cns11643_1992_5[]	  = {NULL};
char *cns11643_1992_6[]	  = {NULL};
char *cns11643_1992_7[]	  = {NULL};
char *dosencoding_cp437[] = {"CP437", NULL};
char *dosencoding_cp850[] = {"CP850",
			     "cp850",	/* osf */
			     "IBM-850", /* aix */
			     NULL};
char *gb2312_1980_0[]	  = {"GB2312",
			     "gb2312",	  /* solaris */
			     "dechanzi",  /* osf */
			     "hp15CN",	  /* hpux */
			     "IBM-eucCN", /* aix */
			     "eucCN",	  /* irix */
			     NULL};
char *georgian_academy[]  = {"GEORGIAN-ACADEMY", NULL};
char *georgian_ps[]	  = {"GEORGIAN-PS", NULL};
char *ibm_cp1133[]	  = {"CP850" , NULL};
char *isiri_3342[]	  = {"ISIRI-3342", NULL};
char *iso10646_1[]	  = {"UTF-8", /* gnu, aix, osf, solaris */
			     "utf8"   /* hpux */,
			     "UTF8",
			     "utf_8",
			     NULL};
/* embed ascii into ISO-8859-1*/
char *iso8859_1[]	  = {"ISO-8859-1",  /* gnu */
			     "ISO8859-1",   /* aix, irix, osf, solaris */
			     "iso8859-1",   /* aix, irix, osf */
			     "iso88591",    /* hpux */
			     "8859-1",	    /* solaris ? */
			     "iso81",	    /* hpux */
			     "ascii",
			     "ASCII",
			     "646", /* solaris */
			     "ANSI_X3.4-1968",
			     "ISO_646.IRV:1983", /* old glibc */
			     "american_e",
			     NULL};
char *iso8859_2[]	  = {"ISO-8859-2",
			     "ISO8859-2",
			     "iso8859-2",
			     "iso88592",
			     "8859-2",
			     "iso82",
			     NULL};
char *iso8859_3[]	  = {"ISO-8859-3",
			     "ISO8859-3",
			     "iso8859-3",
			     "iso88593",
			     "8859-3",
			     "iso83",
			     NULL};
char *iso8859_4[]	  = {"ISO-8859-4",
			     "ISO8859-4",
			     "iso8859-4",
			     "iso88594",
			     "8859-4",
			     "iso84",
			     NULL};
char *iso8859_5[]	  = {"ISO-8859-5",
			     "ISO8859-5",
			     "iso8859-5",
			     "iso88595",
			     "8859-5",
			     "iso85",
			     NULL};
char *iso8859_6[]	  = {"ISO-8859-6",
			     "ISO8859-6",
			     "iso8859-6",
			     "iso88596",
			     "8859-6",
			     "iso86",
			     NULL};
char *iso8859_7[]	  = {"ISO-8859-7",
			     "ISO8859-7",
			     "iso8859-7",
			     "iso88597",
			     "8859-7",
			     "iso87",
			     NULL};
char *iso8859_8[]	  = {"ISO-8859-8",
			     "ISO8859-8",
			     "iso88859-8",
			     "iso88598",
			     "8859-8",
			     "iso88",
			     NULL};
char *iso8859_9[]	  = {"ISO-8859-9",
			     "ISO8859-9",
			     "iso8859-9",
			     "iso88599",
			     "8859-9",
			     "iso89",
			     NULL};
char *iso8859_10[]	  = {"ISO-8859-10",
			     "ISO8859-10",
			     "iso8859-10",
			     "iso885910",
			     "8859-10",
			     "iso80", /*?*/
			     "iso10",
			     NULL};
char *iso8859_13[]	  = {"ISO-8859-13",
			     "ISO8859-13",
			     "iso8859-13",
			     "iso885913",
			     "8859-13",
			     "IBM-921", /* aix */
			     "iso813", /*?*/
			     "iso13",
			     NULL};
char *iso8859_14[]	  = {"ISO-8859-14",
			     "ISO8859-14",
			     "iso8859-14",
			     "iso885914",
			     "8859-14",
			     "iso814",
			     "iso14",
			     NULL};
char *iso8859_15[]	  = {"ISO-8859-15",
			     "ISO88859-15",
			     "iso88859-15",
			     "iso885915",
			     "8859-15",
			     "iso815",
			     "iso15",
			     NULL};
char *iso8859_16[]	  = {"ISO-8859-16",
			     "ISO8859-16",
			     "iso8859-16",
			     "iso885916",
			     "8859-16",
			     "iso80",
			     "iso16",
			     NULL};
char *utf_8[]		  =  {"UTF-8", /* gnu, solaris */
			      "UTF8",
			      "utf8",
			      "utf_8",
			      NULL};
char *jisx0201_1976_0[]	  = {"JIS_X0201",
			     "ISO-IR-14",
			     "jis", /* hpux */
			     NULL};
char *jisx0208_1983_0[]	  = {"JIS_X0208", NULL};
char *jisx0208_1990_0[]	  = {"JIS_X0208", NULL};
char *jisx0212_1990_0[]	  = {"JIS_X0212", NULL};
char *koi8_r[]		  = {"KOI8-R", /* gnu, solaris */
			     NULL};
char *koi8_u[]		  = {"KOI8-U",
			     NULL};
char *ksc5601_1987_0[]	  = {"KSC5636",
			     "ko_KR-johap", /* solaris */
			     "ko_KR-johap92",
			     "ko_KR-euc",
			     "CP949", /* osf */
			     NULL};
char *ksc5601_1992_0[]	  = {"KSC5636", /* ?? */
			     "ko_KR-johap92", /* solaris */
			     NULL};
char *microsoft_cp1251[]  = {"CP1251",
			     NULL};
char *microsoft_cp1255[]  = {"CP1255",
			     NULL};
char *microsoft_cp1256[]  = {"CP1256",
			     NULL};
char *mulelao_1[]	  = {"MULELAO-1",
			     "MULELAO",
			     NULL};
char *sjisx_1[]		  = {"SHIFT_JIS",
			     "SJIS", /* solaris, osf */
			     "sjis", /* hpux */
			     "PCK",  /* solaris */
			     NULL};
char *tatar_cyr[]	  = {NULL};
char *tcvn_5712[]	  = {"TCVN", NULL};
char *tis620_0[]	  = {"TIS-620",
			     "tis620", /* hpux */
			     "TACTIS", /* osf */
			     NULL};
char *viscii1_1_1[]	  = {"VISCII", NULL};
#endif

#if FiconvSupport

#ifdef HAVE_BIDI
#define	 CT_ENTRY(x,y,z) {x, y, FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED, z}
#else
#define	 CT_ENTRY(x,y,z) {x, y, FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED, NULL}
#endif

#else /* !FlocaleIconvSupport */

#ifdef HAVE_BIDI
#define	 CT_ENTRY(x,y,z) {x, nullsl, FLC_INDEX_ICONV_CHARSET_NOT_FOUND, z}
#else
#define	 CT_ENTRY(x,y,z) {x, nullsl, FLC_INDEX_ICONV_CHARSET_NOT_FOUND, NULL}
#endif

#endif

static
FlocaleCharset UnkownCharset =
	{"Unknown", nullsl, FLC_INDEX_ICONV_CHARSET_NOT_FOUND, NULL};

/* the table contains all Xorg "charset" plus some others */

FlocaleCharset FlocaleCharsetTable[] =
{
	CT_ENTRY("ARMSCII-8",		armscii_8,	     NULL),
	CT_ENTRY("BIG5-0",		big5_0,		     NULL),
	/* These CNS* are official for Xorg (traditional Chinese) but
	 * I do not know the iconv charset names */
	CT_ENTRY("CNS11643.1986-1",	cns11643_1986_1,     NULL),
	CT_ENTRY("CNS11643.1986-2",	cns11643_1986_2,     NULL),
	CT_ENTRY("CNS11643.1992-3",	cns11643_1992_3,     NULL),
	CT_ENTRY("CNS11643.1992-4",	cns11643_1992_4,     NULL),
	CT_ENTRY("CNS11643.1992-5",	cns11643_1992_5,     NULL),
	CT_ENTRY("CNS11643.1992-6",	cns11643_1992_6,     NULL),
	CT_ENTRY("CNS11643.1992-7",	cns11643_1992_7,     NULL),
	CT_ENTRY("DOSENCODING-CP437",	dosencoding_cp437,   NULL),
	CT_ENTRY("DOSENCODING-CP850",	dosencoding_cp850,   NULL),
	CT_ENTRY("GB2312.1980-0",	gb2312_1980_0,	     NULL),
	CT_ENTRY("GEORGIAN-ACADEMY",	georgian_academy,    NULL),
	CT_ENTRY("GEORGIAN-PS",		georgian_ps,	     NULL),
	CT_ENTRY("IBM-CP1133",		ibm_cp1133,	     NULL),
	CT_ENTRY("ISIRI-3342",		isiri_3342,	     "ISIRI-3342"),
	CT_ENTRY("ISO10646-1",		iso10646_1,	     "UTF-8"),
	CT_ENTRY("ISO8859-1",		iso8859_1,	     NULL),
	CT_ENTRY("ISO8859-2",		iso8859_2,	     NULL),
	CT_ENTRY("ISO8859-3",		iso8859_3,	     NULL),
	CT_ENTRY("ISO8859-4",		iso8859_4,	     NULL),
	CT_ENTRY("ISO8859-5",		iso8859_5,	     NULL),
	CT_ENTRY("ISO8859-6",		iso8859_6,	     "ISO8859-6"),
	CT_ENTRY("ISO8859-6.8X",	iso8859_6,	     "ISO8859-6"),
	CT_ENTRY("ISO8859-7",		iso8859_7,	     NULL),
	CT_ENTRY("ISO8859-8",		iso8859_8,	     "ISO8859-8"),
	CT_ENTRY("ISO8859-9",		iso8859_9,	     NULL),
	CT_ENTRY("ISO8859-9E",		iso8859_9,	    NULL),
	CT_ENTRY("ISO8859-10",		iso8859_10,	     NULL),
	CT_ENTRY("ISO8859-13",		iso8859_13,	     NULL),
	CT_ENTRY("ISO8859-14",		iso8859_14,	     NULL),
	CT_ENTRY("ISO8859-15",		iso8859_15,	     NULL),
	CT_ENTRY("ISO8859-16",		iso8859_16,	     NULL),
	CT_ENTRY("JISX.UDC-1",		jisx0201_1976_0,     NULL), /* ? */
	CT_ENTRY("JISX0201.1976-0",	jisx0201_1976_0,     NULL),
	CT_ENTRY("JISX0208.1983-0",	jisx0208_1983_0,     NULL),
	CT_ENTRY("JISX0208.1990-0",	jisx0208_1990_0,     NULL),
	CT_ENTRY("JISX0212.1990-0",	jisx0212_1990_0,     NULL),
	CT_ENTRY("KOI8-C",		koi8_r,		     NULL),
	CT_ENTRY("KOI8-R",		koi8_r,		     NULL),
	CT_ENTRY("KOI8-U",		koi8_u,		     NULL),
	CT_ENTRY("KSC5601.1987-0",	ksc5601_1987_0,	     NULL),
	CT_ENTRY("KSC5601.1992-0",	ksc5601_1992_0,	     NULL),
	CT_ENTRY("MICROSOFT-CP1251",	microsoft_cp1251,    NULL),
	CT_ENTRY("MICROSOFT-CP1255",	microsoft_cp1255,    "CP1255"),
	CT_ENTRY("MICROSOFT-CP1256",	microsoft_cp1256,    "CP1256"),
	CT_ENTRY("MULELAO-1",		mulelao_1,	     NULL),
	CT_ENTRY("SJISX-1",		sjisx_1,	     NULL),
	CT_ENTRY("TATAR-CYR",		tatar_cyr,	     NULL),
	CT_ENTRY("TCVN-5712",		tcvn_5712,	     NULL),
	CT_ENTRY("TIS620-0",		tis620_0,	     NULL),
	CT_ENTRY("VISCII1.1-1",		viscii1_1_1,	     NULL),
	/* aliases */
	CT_ENTRY("ADOBE-STANDARD",	iso8859_1,	     NULL), /* ? */
	CT_ENTRY("ASCII-0",		iso8859_1,	     NULL),
	CT_ENTRY("BIBIG5HKSCS",		big5_0,		     NULL), /* ? */
	CT_ENTRY("BIG5-E0",		big5_0,		     NULL), /* emacs */
	CT_ENTRY("BIG5-E1",		big5_0,		     NULL), /* emacs */
	CT_ENTRY("BIG5-1",		big5_0,		     NULL),
	CT_ENTRY("ISO646.1991-IRV",	iso8859_1,	     NULL),
	CT_ENTRY("TIS620.2533-1",	tis620_0,	     NULL),
	CT_ENTRY(NULL, nullsl, NULL)
};

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

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
	if (om && (XGetOMValues(om, XNRequiredCharSet, &cs, NULL)) == NULL)
	{
		if (cs.charset_count > 0)
		{
			FLCXOMCharsetList_num = cs.charset_count;
			FLCXOMCharsetList =
			    (FlocaleCharset **)safemalloc(
				   sizeof(FlocaleCharset)*FLCXOMCharsetList_num);
			for(i = 0; i <	FLCXOMCharsetList_num; i++)
			{
				FLCXOMCharsetList[i] =
				    FlocaleCharsetOfXCharset(cs.charset_list[i]);
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
		FLCXOMCharset = FLCXOMCharsetList[0];
#endif
}

/* ---------------------------- interface functions ------------------------- */

void FlocaleCharsetInit(Display *dpy, const char *module)
{
	static Bool initialized = False;

	if (initialized)
		return;

	initialized = True;

	/* set the defaults X locale charsets */
	FlocaleInit_X_Charset(dpy, module);
}

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

void FlocaleCharsetSetFlocaleCharset(Display *dpy, FlocaleFont *flf, char *hints)
{
	char *charset = NULL;
	char *iconv = NULL;
	Bool iconv_found = False;
	int i = 0;

	FlocaleCharsetInit(dpy, "fvwmlibs");

	if (hints && *hints)
	{
		iconv = GetQuotedString(hints, &charset, "/", NULL, NULL, NULL);
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
			flf->fc =  FlocaleCharsetOfXCharset(flf->fftf.encoding);
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
			if (strcmp(iconv,FLC_GET_LOCALE_CHARSET(flf->fc,i)) == 0)
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
		fc = (FlocaleCharset *)safemalloc(sizeof(FlocaleCharset));
		if (flf->fc != NULL)
		{
			CopyString(&fc->x, flf->fc->x);
			if (flf->fc->bidi)
				CopyString(&fc->bidi, flf->fc->bidi);
			else
				fc->bidi = NULL;
		}
		else
		{
			CopyString(&fc->x, "Unkown"); /* for simplicity */
			fc->bidi = NULL;
		}
		fc->locale = (char **)safemalloc(2*sizeof(char *));
		CopyString(&fc->locale[0], iconv);
		fc->locale[1] = NULL;
		fc->iconv_index =  FLC_INDEX_ICONV_CHARSET_NOT_INITIALIZED;
		flf->fc = fc;
	}
	if (flf->font != NULL && flf->fc != NULL &&
	    StrEquals(flf->fc->x, "ISO10646-1"))
	{
		flf->flags.is_utf8 = True;
	}

	if (charset != NULL)
	{
		free(charset);
	}
	if (flf->fc == NULL)
	{
		flf->fc = &UnkownCharset;
	}
}

FlocaleCharset *FlocaleCharsetGetFLCXOMCharset(void)
{
	return FLCXOMCharset;
}

FlocaleCharset *FlocaleCharsetGetUnknownCharset(void)
{
	return &UnkownCharset;
}

const char *FlocaleGetBidiCharset(Display *dpy, FlocaleFont *flf)
{
	FlocaleCharset *fc;
	if (flf == NULL)
	{
		return NULL;
	}
	fc = flf->fc;
	if (fc == FlocaleCharsetGetUnknownCharset() || fc->bidi == NULL)
	{
		return NULL;
	}
	return (const char *)fc->bidi;
}

