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

#include "safemalloc.h"
#include "Strings.h"
#include "Parse.h"

char *Flocale = NULL;
char *Fcharset = NULL;
char *Fmodifiers = NULL;

static FlocaleFont *FlocaleFontList = NULL;

/* ***************************************************************************
 * locale initialisation
 * ***************************************************************************/
#ifdef MULTIBYTE
void FInitLocale(
		 int category,
		 const char *locale,
		 const char *modifiers,
		 const char *module  /* name of the module for errors */
		 )
{

  if ((Flocale = setlocale(category, locale)) == NULL)
  {
    fprintf(stderr,
      "[%s][%s]: ERROR -- Cannot set locale. Check your $LC_CTYPE or $LANG.\n",
	    (module == NULL)? "":module, "FInitLocale");
    return;
  }
  if (!XSupportsLocale())
  {
    fprintf(stderr,
      "[%s][%s]: WARNING -- X does not support locale %s\n",
      (module == NULL)? "":module, "FInitLocale",Flocale);
    Flocale = NULL;
  }
  if (modifiers != NULL && (Fmodifiers = XSetLocaleModifiers(modifiers)) == NULL)
  {
    fprintf(stderr,
      "[%s][%s]: WARNING -- Cannot set locale modifiers\n",
	    (module == NULL)? "":module, "FInitLocale");
  }
#if 0
  fprintf(stderr,"LOCALES: %s, %s\n", Flocale, Fmodifiers);
#endif
}

#endif /* MULTIBYTE */

void FInitCharset(const char *module)
{
  Fcharset = getenv("CHARSET");

  if (Flocale == NULL)
  {
    if (setlocale(LC_CTYPE, "") == NULL)
      fprintf(stderr,
	"[%s][%s]: WARNNING -- Cannot set locale\n", module, "FInitCharset");
  }
#ifdef HAVE_CODESET
  if (!Fcharset)
    Fcharset = nl_langinfo(CODESET);
#endif
#if 0
  fprintf(stderr,"CHARSET: %s\n", (Fcharset == NULL)? "":Fcharset);
#endif
  if (Fcharset == NULL)
  {
#ifdef HAVE_CODESET
    fprintf(stderr,
      "[%s][%s]: WARN -- Cannot get charset with nl_langinfo or CHARSET env\n",
      module, "FInitCharset");
#else
    fprintf(stderr,
      "[%s][%s]: WARN -- Cannot get charset with CHARSET env variable\n",
      module, "FInitCharset");
#endif
  }
  else if (strlen(Fcharset) < 3)
  {
#ifdef HAVE_CODESET
    fprintf(stderr,
      "[%s][%s]: WARN -- Illegal charset (nl_langinfo and CHARSET env): %s\n",
      module, "FInitCharset", Fcharset);
#else
    fprintf(stderr,
      "[%s][%s]: WARN -- Illegal charset (CHARSET env): %s\n",
      module, "FInitCharset", Fcharset);
#endif
    Fcharset = NULL;
  }
}

/* ***************************************************************************
 * fonts loading
 * ***************************************************************************/

#ifdef MULTIBYTE
static
FlocaleFont *get_FlocaleFontSet(Display *dpy, char *fontname, char *module)
{
  static int mc_errors = 0;
  FlocaleFont *flf = NULL;
  XFontSet fontset = NULL;
  char **ml;
  int mc,i;
  char *ds;
  XFontSetExtents *fset_extents;

  if (!(fontset = XCreateFontSet(dpy, fontname, &ml, &mc, &ds)))
    return NULL;
 
  if (mc > 0)
  {
    if (mc_errors <= NUMBER_OF_MISSING_CHARSET_ERR_MSG)
    {
      mc_errors++;
      fprintf(stderr, 
	      "[%s][get_FlocaleFontSet][%s]:"
	      "The following charsets are missing:\n",
	      fontname, (module)? module:"FVWM");
      if (mc_errors == NUMBER_OF_MISSING_CHARSET_ERR_MSG)
	fprintf(stderr, "\tNo more such error will be reported");
    }
    for(i=0; i < mc; i++)
      fprintf(stderr, " %s", ml[i]);
    fprintf(stderr, "\n");
    if (mc_errors == NUMBER_OF_MISSING_CHARSET_ERR_MSG)
      fprintf(stderr, "\tNo more such error will be reported");
    XFreeStringList(ml);
  }

  flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
  flf->count = 1;
  flf->fontset = fontset;
  flf->font = NULL;
  XFT_CODE(flf->xftfont = NULL);
  fset_extents = XExtentsOfFontSet(fontset);
  flf->height = fset_extents->max_logical_extent.height;
  flf->ascent = - fset_extents->max_logical_extent.y;
  flf->descent = fset_extents->max_logical_extent.height +
    fset_extents->max_logical_extent.y;
  flf->max_char_width = fset_extents->max_logical_extent.width;
  return flf;
}
#endif

static
FlocaleFont *get_FlocaleFont(Display *dpy, char *fontname)
{
  XFontStruct *font = NULL;
  FlocaleFont *flf;
  char *str,*fn;

  str = GetQuotedString(fontname, &fn, ",", NULL, NULL, NULL);
  while(!font && (fn && *fn))
  {
    font = XLoadQueryFont(dpy, fn);
    if (fn != NULL)
    {
      free(fn);
      fn = NULL;
    }
    if (!font)
      str = GetQuotedString(str, &fn, ",", NULL, NULL, NULL);
  }
  if (fn != NULL)
    free(fn);
  if (font == NULL)
    return NULL;

  flf = (FlocaleFont *)safemalloc(sizeof(FlocaleFont));
  flf->count = 1;
  MULTIBYTE_CODE(flf->fontset = None);
  XFT_CODE(flf->xftfont = NULL);
  flf->font = font;
  flf->height = flf->font->ascent + flf->font->descent;
  flf->ascent = flf->font->ascent;
  flf->descent = flf->font->descent;
  flf->max_char_width = flf->font->max_bounds.width;
  return flf;
}

static
FlocaleFont *get_FlocaleFontOrFontSet(
	       Display *dpy, char *fontname, char *fullname, char *module)
{
  FlocaleFont *flf = NULL;

  if (fontname && strlen(fontname) > 4 && strncasecmp("xft:", fontname, 4) == 0)
  {
#ifdef HAVE_XFT
    flf = get_FlocaleXftFont(dpy, fontname+4);
#endif
    if (flf)
      CopyString(&flf->name, fullname);
    return flf;
  }
#ifdef MULTIBYTE
  if (flf == NULL && Flocale != NULL && fontname)
  {
    flf = get_FlocaleFontSet(dpy, fontname, module);
  }
#endif
  if (flf == NULL && fontname)
  {
    flf = get_FlocaleFont(dpy, fontname);
  }
  if (flf && fontname)
  {
    if (StrEquals(fullname,MB_FALLBACK_FONT))
      flf->name = MB_FALLBACK_FONT;
    else if (StrEquals(fullname,FALLBACK_FONT))
      flf->name = FALLBACK_FONT;
    else
      CopyString(&flf->name, fullname);
    return flf;
  }
  return NULL;
}

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
#ifdef MULTIBYTE
    fontname = MB_FALLBACK_FONT;
#else
    fontname = FALLBACK_FONT;
#endif
  }

  while(flf)
  {
    char *c1, *c2;

    for (c1 = fontname, c2 = flf->name; *c1 && *c2; ++c1, ++c2)
      if (*c1 != *c2)
	break;

    if (!*c1 && !*c2)
    {
      flf->count++;
      return flf;
    }
    flf = flf->next;
  }

  /* not cached load the font as a ; sperated list */
  str = GetQuotedString(fontname, &fn, ";", NULL, NULL, NULL);
  while(!flf && (fn && *fn))
  {
    flf = get_FlocaleFontOrFontSet(dpy, fn, fontname, module);
    if (fn != NULL)
    {
      free(fn);
      fn = NULL;
    }
    if (!flf)
      str = GetQuotedString(str, &fn, ";", NULL, NULL, NULL);
  }
  if (fn != NULL)
    free(fn);

  if (flf == NULL)
  {
    /* loading fail try default font */
    if (!ask_default)
    {
      fprintf(stderr,"[%s][FlocaleGetFont]: "
	      "WARNING -- can't get font '%s', try to load default font:\n",
	      (module)? module:"FVWM", fontname);
    }
    else
    {
      /* we already try default fonts: try again? yes */
    }
#ifdef MULTIBYTE
    if (Flocale != NULL)
    {
      if (!ask_default)
	fprintf(stderr,"\t %s\n", MB_FALLBACK_FONT);
      if ((flf = get_FlocaleFontSet(dpy, MB_FALLBACK_FONT, module)) != NULL)
      {
	flf->name = MB_FALLBACK_FONT;
      }
    }
#endif
    if (flf == NULL)
    {
      if (!ask_default)
	fprintf(stderr,"\t %s\n", FALLBACK_FONT);
      if ((flf = get_FlocaleFont(dpy, FALLBACK_FONT)) != NULL)
      {
	flf->name = FALLBACK_FONT;
      }
      else
      {
	if (!ask_default)
	{
	  fprintf(stderr,"[%s][FlocaleLoadFont]: ERROR -- can't get font\n",
		  (module)? module:"FVWM");
	}
	else
	{
	  fprintf(stderr,
		  "[%s][FlocaleLoadFont]: ERROR -- can't get default font:\n",
		  (module)? module:"FVWM");
#ifdef MULTIBYTE
	  fprintf(stderr,"\t %s\n", MB_FALLBACK_FONT);
#endif
	  fprintf(stderr,"\t %s\n", FALLBACK_FONT);
	}
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
    return;

  if(--(flf->count)>0) /* Remove a weight, still too heavy? */
    return;

  
  if(flf->name != NULL &&
     !StrEquals(flf->name,MB_FALLBACK_FONT) &&
     !StrEquals(flf->name,FALLBACK_FONT))
  {
    free(flf->name);
  }
#ifdef HAVE_XFT
  if (flf->xftfont != NULL)
  {
    XftFontClose (dpy, flf->xftfont);
  }
#endif
#ifdef MULTIBYTE
  if (flf->fontset != NULL)
  {
    XFreeFontSet(dpy, flf->fontset);
  }
#endif
  if (flf->font != NULL)
  {
    XFreeFont(dpy, flf->font);
  }

  /* Link it out of the list (it might not be there) */
  if (flf == list) /* in head? simple */
    FlocaleFontList = flf->next;
  else
  {
    while(list && list->next != flf) /* fast forward until end or found */
      list = list->next;
    if(list) /* not end? means we found it in there, possibly at end */
      list->next = flf->next; /* link around it */
  }
  free(flf);
}

/* ***************************************************************************
 * Width and Drawing Text
 * ***************************************************************************/
void FlocaleDrawString(Display *dpy, FlocaleFont *flf, FlocaleWinString *fstring,
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

#ifdef HAVE_XFT
  if (flf->xftfont != NULL)
    FftDrawString(dpy, fstring->win, flf, fstring->gc,
		  fstring->x, fstring->y, fstring->str, len);
  else
#endif
#ifdef MULTIBYTE
  if (flf->fontset != None)
    XmbDrawString(dpy, fstring->win, flf->fontset,
                  fstring->gc, fstring->x, fstring->y,
		  fstring->str, len);
  else
#endif
    if (flf->font != None)
      XDrawString(dpy, fstring->win, fstring->gc, fstring->x, 
		  fstring->y, fstring->str, len);
}

int FlocaleTextWidth(FlocaleFont *flf, char *str, int sl)
{
  if (!str || sl < 1)
    return 0;

#ifdef HAVE_XFT
  if (flf->xftfont != NULL)
    return FftTextWidth(flf, str, sl);
  else
#endif
#ifdef MULTIBYTE
  if (flf->fontset != None)
    return XmbTextEscapement(flf->fontset, str, sl);
#endif
  if (flf->font != None)
    return XTextWidth(flf->font, str, sl);
  return 0;
}

void FlocaleAllocateWinString(FlocaleWinString **pfws)
{
  *pfws = (FlocaleWinString *)safemalloc(sizeof(FlocaleWinString));
  memset(*pfws, '\0', sizeof(FlocaleWinString));
  (*pfws)->str = NULL;
}

/* ***************************************************************************
 * Text properties
 * ***************************************************************************/
/* configure does that but never knows */
#ifdef MULTIBYTE
#undef COMPOUND_TEXT
#endif

#if defined(MULTIBYTE) || defined(COMPOUND_TEXT)
#define MULTIBYTE_OR_COMPOUND_TEXT_CODE(x) x
#define MULTIBYTE_OR_COMPOUND_TEXT 1
#else
#define MULTIBYTE_OR_COMPOUND_TEXT_CODE(x)
#endif

void FlocaleGetNameProperty(Status (func)(Display *, Window, XTextProperty *),
			   Display *dpy, Window w,
			   MULTIBYTE_ARG(char ***ret_name_list)
			   char **ret_name)
{
  XTextProperty text_prop;

  if (func(dpy, w, &text_prop) != 0
      MULTIBYTE_OR_COMPOUND_TEXT_CODE(&& text_prop.value))
  {
    MULTIBYTE_OR_COMPOUND_TEXT_CODE(if (text_prop.encoding == XA_STRING))
    {
      /* STRING encoding, use this as it is */
#ifdef COMPOUND_TEXT
      CopyString(&*ret_name, (char *)text_prop.value);
      XFree(text_prop.value);
#else
      *ret_name = (char *)text_prop.value;
      MULTIBYTE_CODE(*ret_name_list = NULL);
#endif
    }
#ifdef MULTIBYTE_OR_COMPOUND_TEXT
    else
    {
      char **list;
      int num;

      /* not STRING encoding, try to convert */
      if (XmbTextPropertyToTextList(dpy, &text_prop, &list, &num) >= Success
	  && num > 0 && *list)
      {
	/* XXX: does not consider the conversion is REALLY succeeded */
	XFree(text_prop.value); /* return of XGetWM(Icon)Name() */
#ifdef COMPOUND_TEXT
	CopyString(&*ret_name, *list);
	if (list)
	  XFreeStringList(list);
#else
	*ret_name = *list;
	MULTIBYTE_CODE(*ret_name_list = list);
#endif
      }
      else
      {
	if (list)
	  XFreeStringList(list);
	XFree(text_prop.value); /* return of XGet(Icon)WMName() */
	if (func(dpy, w, &text_prop))
	{
#ifdef COMPOUND_TEXT
	  CopyString(&*ret_name, (char *)text_prop.value);
	  XFree(text_prop.value);
#else
	  MULTIBYTE_CODE(*ret_name = (char *)text_prop.value);
	  MULTIBYTE_CODE(*ret_name_list = NULL);
#endif
	}
      }
    }
#endif /* MULTIBYTE_OR_COMPOUND_TEXT */
  }
}


void FlocaleFreeNameProperty(MULTIBYTE_ARG(char ***ptext_list) char **ptext)
{
#ifdef MULTIBYTE
  if (*ptext_list != NULL)
  {
    XFreeStringList(*ptext_list);
    *ptext_list = NULL;
  }
  else
#endif
  {
#ifdef COMPOUND_TEXT
    free(*ptext);
#else
    XFree(*ptext);
#endif
  }
  *ptext = NULL;

  return;
}

Bool FlocaleTextListToTextProperty(Display *dpy, char **list, int count,
				   XICCEncodingStyle style,
				   XTextProperty *text_prop_return)
{
  int ret = False;

#ifdef MULTIBYTE
  if (Flocale != NULL)
  {
    ret = XmbTextListToTextProperty(dpy, list, count, style, text_prop_return);
    if (ret == XNoMemory)
    {
      ret = False;
    }
    ret = True;
  }
#endif
  if (!ret)
  {
    if (XStringListToTextProperty(list, count, text_prop_return) == 0)
      ret = False;
    else
      ret = True;
  }

  return ret;
}
