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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "fvwmlib.h"

/*
** loads font or "fixed" on failure
*/
XFontStruct *GetFontOrFixed(Display *disp, char *fontname)
{
  XFontStruct *fnt = NULL;

  if (fontname)
    fnt = XLoadQueryFont(disp,fontname);
  if (!fnt && fontname)
  {
    fprintf(stderr,
            "[GetFontOrFixed]: WARNING -- can't get font %s, trying 'fixed'\n",
            fontname);
  }
  if (!fnt)
  {
    /* fixed should always be available, so try that */
    if ((fnt = XLoadQueryFont(disp,"fixed"))==NULL)
    {
      fprintf(stderr,"[GetFontOrFixed]: ERROR -- can't get font 'fixed'\n");
    }
  }
  return fnt;
}

#ifdef I18N_MB
/*
** loads fontset or "fixed" on failure
** Note, STRICTLY_FIXED does not seem to be defined on a standard compile.
** The fallback is to a font that does not use a font alias.
** I'm not sure why this is done for I18N_MB only. dje Dec 2001.
*/

#ifdef STRICTLY_FIXED
#define FALLBACK_FONT "fixed"
#else
#define FALLBACK_FONT "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*"
#endif

XFontSet GetFontSetOrFixed(Display *disp, char *fontname)
{
  XFontSet fontset = NULL;
  char **ml;
  int mc;
  char *ds;

  if (fontname)
    fontset = XCreateFontSet(disp,fontname,&ml,&mc,&ds);
  if (!fontset && fontname)
  {
    fprintf(stderr,
            "[FVWM][GetFontSetOrFixed]: "
	    "WARNING -- can't get fontset %s, trying '%s'\n",
            fontname,FALLBACK_FONT);
  }
  if (!fontset)
  {
    /* fixed should always be avail, so try that */
    /* Yes, you say it's not a *FIXED* font, but it helps you. */
    if ((fontset =
	 XCreateFontSet(disp,
			FALLBACK_FONT,
			&ml, &mc, &ds)) == NULL)
    {
      fprintf(stderr,"[FVWM][GetFontSetOrFixed]: "
	      "ERROR -- can't get fontset '%s'\n",FALLBACK_FONT);
    }
  }
  return fontset;
}
#endif


Bool LoadFvwmFont(Display *dpy, char *fontname, FvwmFont *ret_font)
{
#ifdef I18N_MB
  XFontSet newfontset;

  if ((newfontset = GetFontSetOrFixed(dpy, fontname)))
  {
    XFontSetExtents *fset_extents;
    XFontStruct **fs_list;
    char **ml;

    ret_font->fontset = newfontset;
    /* backward compatiblity setup */
    XFontsOfFontSet(newfontset, &fs_list, &ml);
    ret_font->font = fs_list[0];
    fset_extents = XExtentsOfFontSet(newfontset);
    ret_font->height = fset_extents->max_logical_extent.height;
  }
  else
  {
    return False;
  }
#else
  XFontStruct *newfont;

  if ((newfont = GetFontOrFixed(dpy, fontname)))
  {
    ret_font->font = newfont;
    ret_font->height = ret_font->font->ascent + ret_font->font->descent;
  }
  else
  {
    return False;
  }
#endif
  ret_font->y = ret_font->font->ascent;

  return True;
}

void FreeFvwmFont(Display *dpy, FvwmFont *font)
{
#ifdef I18N_MB
  if (font->fontset)
    XFreeFontSet(dpy, font->fontset);
  font->fontset = None;
#else
  if (font->font)
    XFreeFont(dpy, font->font);
#endif
  font->font = None;

  return;
}
