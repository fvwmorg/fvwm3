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

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "Flocale.h"

#ifdef HAVE_CODESET
#include <langinfo.h>
#endif

#include "defaults.h"
#include "safemalloc.h"
#include "Strings.h"
#include "Parse.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static FlocaleFont *FlocaleFontList = NULL;
static char *Flocale = NULL;
static char *Fcharset = NULL;
static char *Fmodifiers = NULL;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static
FlocaleFont *FlocaleGetFftFont(Display *dpy, char *fontname)
{
	FftFontType *fftf = NULL;
	FlocaleFont *flf = NULL;

	fftf = FftGetFont(dpy, fontname);
	if (fftf == NULL)
	{
		return NULL;
	}
	flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
	flf->count = 1;
	flf->fftf = *fftf;
	flf->font = NULL;
	flf->fontset = None;
	FftGetFontHeights(
		&flf->fftf, &flf->height, &flf->ascent, &flf->descent);
	FftGetFontWidths(
		&flf->fftf, &flf->max_char_width, &flf->min_char_offset);
	free(fftf);

	return flf;
}

static
FlocaleFont *FlocaleGetFontSet(Display *dpy, char *fontname, char *module)
{
	static int mc_errors = 0;
	FlocaleFont *flf = NULL;
	XFontSet fontset = NULL;
	char **ml;
	int mc,i;
	char *ds;
	XFontSetExtents *fset_extents;

	if (!FlocaleMultibyteSupport)
	{
		return NULL;
	}
	if (!(fontset = XCreateFontSet(dpy, fontname, &ml, &mc, &ds)))
	{
		return NULL;
	}

	if (mc > 0)
	{
		if (mc_errors <= FLOCALE_NUMBER_MISS_CSET_ERR_MSG)
		{
			mc_errors++;
			fprintf(stderr,
				"[%s][FlocaleGetFontSet][%s]:"
				"The following charsets are missing:\n",
				fontname, (module)? module:"FVWM");
			if (mc_errors >= FLOCALE_NUMBER_MISS_CSET_ERR_MSG)
			{
				fprintf(stderr, "\tNo more such error will be"
					" reported");
			}
		}
		for (i=0; i < mc; i++)
		{
			fprintf(stderr, " %s", ml[i]);
		}
		fprintf(stderr, "\n");
		if (mc_errors >= FLOCALE_NUMBER_MISS_CSET_ERR_MSG)
		{
			fprintf(stderr,
				"\tNo more such error will be reported");
		}
		XFreeStringList(ml);
	}

	flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
	flf->count = 1;
	flf->fontset = fontset;
	flf->font = NULL;
	flf->fftf.fftfont = NULL;
	fset_extents = XExtentsOfFontSet(fontset);
	flf->height = fset_extents->max_logical_extent.height;
	flf->ascent = - fset_extents->max_logical_extent.y;
	flf->descent = fset_extents->max_logical_extent.height +
		fset_extents->max_logical_extent.y;
	flf->max_char_width = fset_extents->max_logical_extent.width;
	flf->min_char_offset = fset_extents->max_ink_extent.x;

	return flf;
}

static
FlocaleFont *FlocaleGetFont(Display *dpy, char *fontname)
{
	XFontStruct *font = NULL;
	FlocaleFont *flf;
	char *str,*fn;

	str = GetQuotedString(fontname, &fn, ",", NULL, NULL, NULL);
	while (!font && (fn && *fn))
	{
		font = XLoadQueryFont(dpy, fn);
		if (fn != NULL)
		{
			free(fn);
			fn = NULL;
		}
		if (!font)
		{
			str = GetQuotedString(str, &fn, ",", NULL, NULL, NULL);
		}
	}
	if (fn != NULL)
	{
		free(fn);
	}
	if (font == NULL)
	{
		return NULL;
	}

	flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
	flf->count = 1;
	if (FlocaleMultibyteSupport)
	{
		flf->fontset = None;
	}
	if (FftSupport)
	{
		flf->fftf.fftfont = NULL;
	}
	flf->font = font;
	flf->height = flf->font->ascent + flf->font->descent;
	flf->ascent = flf->font->ascent;
	flf->descent = flf->font->descent;
	flf->max_char_width = flf->font->max_bounds.width;
	flf->min_char_offset = flf->font->min_bounds.lbearing;

	return flf;
}

static
FlocaleFont *FlocaleGetFontOrFontSet(
	Display *dpy, char *fontname, char *fullname, char *module)
{
	FlocaleFont *flf = NULL;

	if (fontname && strlen(fontname) > 4 &&
	    strncasecmp("xft:", fontname, 4) == 0)
	{
		if (FftSupport)
		{
			flf = FlocaleGetFftFont(dpy, fontname+4);
		}
		if (flf)
		{
			CopyString(&flf->name, fullname);
		}
		return flf;
	}
	if (FlocaleMultibyteSupport && flf == NULL && Flocale != NULL &&
	    fontname)
	{
		flf = FlocaleGetFontSet(dpy, fontname, module);
	}
	if (flf == NULL && fontname)
	{
		flf = FlocaleGetFont(dpy, fontname);
	}
	if (flf && fontname)
	{
		if (StrEquals(fullname, FLOCALE_MB_FALLBACK_FONT))
		{
			flf->name = FLOCALE_MB_FALLBACK_FONT;
		}
		else if (StrEquals(fullname, FLOCALE_FALLBACK_FONT))
		{
			flf->name = FLOCALE_FALLBACK_FONT;
		}
		else
		{
			CopyString(&flf->name, fullname);
		}
		return flf;
	}
	return NULL;
}

/* ---------------------------- interface functions ------------------------- */

/* ***************************************************************************
 * locale initialisation
 * ***************************************************************************/
void FlocaleInit(
	int category,
	const char *locale,
	const char *modifiers,
	const char *module  /* name of the module for errors */
	)
{
	if (!FlocaleMultibyteSupport)
	{
		return;
	}
	if ((Flocale = setlocale(category, locale)) == NULL)
	{
		fprintf(stderr,
			"[%s][%s]: ERROR -- Cannot set locale. Please check"
			" your $LC_CTYPE or $LANG.\n",
			(module == NULL)? "" : module, "FInitLocale");
		return;
	}
	if (!XSupportsLocale())
	{
		fprintf(stderr,
			"[%s][%s]: WARNING -- X does not support locale %s\n",
			(module == NULL)? "":module, "FInitLocale",Flocale);
		Flocale = NULL;
	}
	if (modifiers != NULL &&
	    (Fmodifiers = XSetLocaleModifiers(modifiers)) == NULL)
	{
		fprintf(stderr,
			"[%s][%s]: WARNING -- Cannot set locale modifiers\n",
			(module == NULL)? "":module, "FInitLocale");
	}
#if 0
	fprintf(stderr,"LOCALES: %s, %s\n", Flocale, Fmodifiers);
#endif
}

void FlocaleInitCharset(const char *module)
{
	Fcharset = getenv("CHARSET");

	if (Flocale == NULL)
	{
		if (setlocale(LC_CTYPE, "") == NULL)
		{
			fprintf(stderr,
				"[%s][%s]: WARNNING -- Cannot set locale\n",
				module, "FInitCharset");
		}
	}
#ifdef HAVE_CODESET
	if (!Fcharset)
	{
		Fcharset = nl_langinfo(CODESET);
	}
#endif
#if 0
	fprintf(stderr,"CHARSET: %s\n", (Fcharset == NULL)? "":Fcharset);
#endif
	if (Fcharset == NULL)
	{
#ifdef HAVE_CODESET
		fprintf(stderr,
			"[%s][%s]: WARN -- Cannot get charset with"
			" nl_langinfo or CHARSET env\n",
			module, "FInitCharset");
#else
		fprintf(stderr,
			"[%s][%s]: WARN -- Cannot get charset with"
			" CHARSET env variable\n",
			module, "FInitCharset");
#endif
	}
	else if (strlen(Fcharset) < 3)
	{
#ifdef HAVE_CODESET
		fprintf(stderr,
			"[%s][%s]: WARN -- Illegal charset"
			" (nl_langinfo and CHARSET env): %s\n",
			module, "FInitCharset", Fcharset);
#else
		fprintf(stderr,
			"[%s][%s]: WARN -- Illegal charset"
			" (CHARSET env): %s\n",
			module, "FInitCharset", Fcharset);
#endif
		Fcharset = NULL;
	}
}

/* ***************************************************************************
 * fonts loading
 * ***************************************************************************/

FlocaleFont *FlocaleLoadFont(Display *dpy, char *fontname, char *module)
{
	FlocaleFont *flf = FlocaleFontList;
	Bool ask_default = False;
	char *t;
	char *str,*fn = NULL;

	/* removing quoting for modules */
	if (fontname && (t = strchr("\"'`", *fontname)))
	{
		char c = *t;
		fontname++;
		if (fontname[strlen(fontname)-1] == c)
			fontname[strlen(fontname)-1] = 0;
	}

	if (fontname == NULL || *fontname == 0)
	{
		ask_default = True;
		fontname = (FlocaleMultibyteSupport) ?
			FLOCALE_MB_FALLBACK_FONT : FLOCALE_FALLBACK_FONT;
	}

	while (flf)
	{
		char *c1, *c2;

		for (c1 = fontname, c2 = flf->name; *c1 && *c2; ++c1, ++c2)
		{
			if (*c1 != *c2)
			{
				break;
			}
		}
		if (!*c1 && !*c2)
		{
			flf->count++;
			return flf;
		}
		flf = flf->next;
	}

	/* not cached load the font as a ; sperated list */
	str = GetQuotedString(fontname, &fn, ";", NULL, NULL, NULL);
	while (!flf && (fn && *fn))
	{
		flf = FlocaleGetFontOrFontSet(dpy, fn, fontname, module);
		if (fn != NULL)
		{
			free(fn);
			fn = NULL;
		}
		if (!flf)
		{
			str = GetQuotedString(str, &fn, ";", NULL, NULL, NULL);
		}
	}
	if (fn != NULL)
	{
		free(fn);
	}

	if (flf == NULL)
	{
		/* loading fail try default font */
		if (!ask_default)
		{
			fprintf(stderr,"[%s][FlocaleGetFont]: "
				"WARNING -- can't get font '%s',"
				" try to load default font:\n",
				(module)? module:"FVWM", fontname);
		}
		else
		{
			/* we already try default fonts: try again? yes */
		}
		if (FlocaleMultibyteSupport && Flocale != NULL)
		{
			if (!ask_default)
			{
				fprintf(stderr, "\t %s\n",
					FLOCALE_MB_FALLBACK_FONT);
			}
			if ((flf = FlocaleGetFontSet(
				     dpy, FLOCALE_MB_FALLBACK_FONT, module)) !=
			    NULL)
			{
				flf->name = FLOCALE_MB_FALLBACK_FONT;
			}
		}
		if (flf == NULL)
		{
			if (!ask_default)
			{
				fprintf(stderr,"\t %s\n",
					FLOCALE_FALLBACK_FONT);
			}
			if ((flf = FlocaleGetFont(
				     dpy, FLOCALE_FALLBACK_FONT)) != NULL)
			{
				flf->name = FLOCALE_FALLBACK_FONT;
			}
			else if (!ask_default)
			{
				fprintf(stderr,
					"[%s][FlocaleLoadFont]:"
					" ERROR -- can't get font\n",
					(module)? module:"FVWM");
			}
			else
			{
				fprintf(stderr,
					"[%s][FlocaleLoadFont]: ERROR"
					" -- can't get default font:\n",
					(module)? module:"FVWM");
				if (FlocaleMultibyteSupport)
				{
					fprintf(stderr,"\t %s\n",
						FLOCALE_MB_FALLBACK_FONT);
				}
				fprintf(stderr,"\t %s\n",
					FLOCALE_FALLBACK_FONT);
			}
		}
	}
	if (flf != NULL)
	{
		flf->next = FlocaleFontList;
		FlocaleFontList = flf;
	}

	return flf;
}

void FlocaleUnloadFont(Display *dpy, FlocaleFont *flf)
{
	FlocaleFont *list = FlocaleFontList;

	if (!flf)
	{
		return;
	}
	if (--(flf->count)>0) /* Remove a weight, still too heavy? */
	{
		return;
	}

	if (flf->name != NULL &&
	   !StrEquals(flf->name,FLOCALE_MB_FALLBACK_FONT) &&
	   !StrEquals(flf->name,FLOCALE_FALLBACK_FONT))
	{
		free(flf->name);
	}
	if (FftSupport && flf->fftf.fftfont != NULL)
	{
		FftFontClose(dpy, flf->fftfont);
	}
	if (FlocaleMultibyteSupport && flf->fontset != NULL)
	{
		XFreeFontSet(dpy, flf->fontset);
	}
	if (flf->font != NULL)
	{
		XFreeFont(dpy, flf->font);
	}

	/* Link it out of the list (it might not be there) */
	if (flf == list) /* in head? simple */
	{
		FlocaleFontList = flf->next;
	}
	else
	{
		while (list && list->next != flf)
		{
			/* fast forward until end or found */
			list = list->next;
		}
		/* not end? means we found it in there, possibly at end */
		if (list)
		{
			/* link around it */
			list->next = flf->next;
		}
	}
	free(flf);
}

/* ***************************************************************************
 * Width and Drawing Text
 * ***************************************************************************/
void FlocaleDrawString(
	Display *dpy, FlocaleFont *flf, FlocaleWinString *fstring,
	unsigned long flags)
{
	int len;

	if (!fstring || !fstring->str)
		return;

	if (flags & FWS_HAVE_LENGTH)
	{
		len = fstring->len;
	}
	else
	{
		len = strlen(fstring->str);
	}
	if (fstring->flags.is_vertical_string)
	{
		/* FIXME: still have to write vertical text drawing.  Use
		 * normal code for now. */
	}
	if (FftSupport && flf->fftf.fftfont != NULL)
	{
		FftDrawString(
			dpy, fstring->win, &flf->fftf, fstring->gc, fstring->x,
			fstring->y, fstring->str, len);
		return;
	}
	if (FlocaleMultibyteSupport && flf->fontset != None)
	{
		XmbDrawString(dpy, fstring->win, flf->fontset,
			      fstring->gc, fstring->x, fstring->y,
			      fstring->str, len);
		return;
	}
	if (flf->font != None)
	{
		XDrawString(dpy, fstring->win, fstring->gc, fstring->x,
			    fstring->y, fstring->str, len);
		return;
	}

	return;
}

int FlocaleTextWidth(FlocaleFont *flf, char *str, int sl)
{
	if (!str || sl == 0)
		return 0;

	if (sl < 0)
	{
		/* a vertical string */
		/* FIXME: still have to write vertical text handling, just use
		 * the normal calculations for now. */
		sl = -sl;
		return sl * flf->height;
	}
	if (FftSupport && flf->fftf.fftfont != NULL)
	{
		return FftTextWidth(&flf->fftf, str, sl);
	}
	if (FlocaleMultibyteSupport && flf->fontset != None)
	{
		return XmbTextEscapement(flf->fontset, str, sl);
	}
	if (flf->font != None)
	{
		return XTextWidth(flf->font, str, sl);
	}

	return 0;
}

void FlocaleAllocateWinString(FlocaleWinString **pfws)
{
	*pfws = (FlocaleWinString *)safemalloc(sizeof(FlocaleWinString));
	memset(*pfws, '\0', sizeof(FlocaleWinString));
}

/* ***************************************************************************
 * Text properties
 * ***************************************************************************/
void FlocaleGetNameProperty(
	Status (func)(Display *, Window, XTextProperty *), Display *dpy,
	Window w, FlocaleNameString *ret_name)
{
	char **list;
	int num;
	XTextProperty text_prop;

	if (func(dpy, w, &text_prop) == 0 ||
	    (FlocaleCompoundText && !text_prop.value))
	{
		return;
	}

	if (FlocaleMultibyteSupport && text_prop.encoding == XA_STRING)
	{
		/* STRING encoding, use this as it is */
		ret_name->name = (char *)text_prop.value;
		ret_name->name_list = NULL;
		return;
	}
	else if (FlocaleCompoundText && text_prop.encoding == XA_STRING)
	{
		/* STRING encoding, use this as it is */
		CopyString(&(ret_name->name), (char *)text_prop.value);
		XFree(text_prop.value);
		return;
	}
	else if (!FlocaleMultibyteSupport && !FlocaleCompoundText)
	{
		return;
	}
	/* not STRING encoding, try to convert */
	if (XmbTextPropertyToTextList(dpy, &text_prop, &list, &num) >= Success
	    && num > 0 && *list)
	{
		/* XXX: does not consider the conversion is REALLY
		 * succeeded */
		XFree(text_prop.value); /* return of XGetWM(Icon)Name() */
		if (FlocaleCompoundText)
		{
			CopyString(&(ret_name->name), *list);
			if (list)
			{
				XFreeStringList(list);
			}
		}
		else
		{
			ret_name->name = *list;
			ret_name->name_list = list;
		}
	}
	else
	{
		if (list)
		{
			XFreeStringList(list);
		}
		XFree(text_prop.value); /* return of XGet(Icon)WMName() */
		if (func(dpy, w, &text_prop))
		{
			if (FlocaleCompoundText)
			{
				CopyString(
					&(ret_name->name),
					(char *)text_prop.value);
				XFree(text_prop.value);
			}
			else if (FlocaleMultibyteSupport)
			{
				ret_name->name = (char *)text_prop.value;
				ret_name->name_list = NULL;
			}
		}
	}
}


void FlocaleFreeNameProperty(FlocaleNameString *ptext)
{
	if (FlocaleMultibyteSupport && ptext->name_list != NULL)
	{
		XFreeStringList(ptext->name_list);
		*ptext->name_list = NULL;
	}
	else if (FlocaleCompoundText)
	{
		free(ptext->name);
	}
	else
	{
		XFree(ptext->name);
	}
	ptext->name = NULL;

	return;
}

Bool FlocaleTextListToTextProperty(
	Display *dpy, char **list, int count, XICCEncodingStyle style,
	XTextProperty *text_prop_return)
{
	int ret = False;

	if (FlocaleMultibyteSupport && Flocale != NULL)
	{
		ret = XmbTextListToTextProperty(
			dpy, list, count, style, text_prop_return);
		if (ret == XNoMemory)
		{
			ret = False;
		}
		ret = True;
	}
	if (!ret)
	{
		if (XStringListToTextProperty(
			    list, count, text_prop_return) == 0)
		{
			ret = False;
		}
		else
		{
			ret = True;
		}
	}

	return ret;
}

const char *FlocaleGetCharset(void)
{
	return (const char *)Fcharset;
}
