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
#include <X11/Xlib.h>
#ifdef HAVE_CODESET
#include <locale.h>
#include <langinfo.h>
#endif
#include "Flocale.h"

char *Flocale = NULL;
char *Fcharset = NULL;
char *Fmodifiers = NULL;

#ifdef I18N_MB
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

#endif /* I18N_MB */

void FInitCharset(const char *module)
{
  Fcharset = getenv("CHARSET");

  if (Flocale == NULL)
  {
    if (setlocale(LC_CTYPE, getenv("LC_CTYPE")) == NULL)
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
