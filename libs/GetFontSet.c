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

/* This file brings from GetFont.c */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "fvwmlib.h"

/*
** loads fontset or "fixed" on failure
*/
XFontSet GetFontSetOrFixed(Display *disp, char *fontname)
{
  XFontSet fontset;
  char **ml;
  int mc;
  char *ds;

  if ((fontset = XCreateFontSet(disp,fontname,&ml,&mc,&ds))==NULL)
  {
    fprintf(stderr,
            "[FVWM][GetFontSetOrFixed]: WARNING -- can't get fontset %s, trying 'fixed'\n",
            fontname);
    /* fixed should always be avail, so try that */
#ifdef STRICTLY_FIXED
    if ((fontset = XCreateFontSet(disp,"fixed",&ml,&mc,&ds))==NULL)
#else
    /* Yes, you say it's not a *FIXED* font, but it helps you. */
    if ((fontset = XCreateFontSet(disp,"-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*",&ml,&mc,&ds))==NULL)
#endif
    {
      fprintf(stderr,"[FVWM][GetFontSetOrFixed]: ERROR -- can't get fontset 'fixed'\n");
    }
  }
  return fontset;
}

