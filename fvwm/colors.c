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

/*
 * *************************************************************************
 * This module was all new
 * by Rob Nation
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 *
 * The highlight and shadow logic is now in libs/ColorUtils.c.
 * Its completely new.
 * *************************************************************************
 */


#include "config.h"

#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "fvwm.h"
#include "cursor.h"
#include "functions.h"
#include "libs/fvwmlib.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"

/***********************************************************************
 *
 *  Procedure:
 *	CreateGCs - open fonts and create all the needed GC's.  I only
 *		    want to do this once, hence the first_time flag.
 *
 ***********************************************************************/
void CreateGCs(void)
{
  XGCValues gcv;
  unsigned long gcm;

  /* create scratch GC's */
  gcm = GCFunction|GCLineWidth;
  gcv.function = GXcopy;
  gcv.line_width = 0;

  Scr.ScratchGC1 = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
  Scr.ScratchGC2 = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
  Scr.ScratchGC3 = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
  Scr.TransMaskGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

}


/****************************************************************************
 *
 * Free an array of colours (n colours), never free black
 *
 ****************************************************************************/
void FreeColors(Pixel *pixels, int n)
{
  int i;

  /* We don't ever free "black" - dirty hack to allow freeing colours at all */
  for (i = 0; i < n; i++)
  {
    if (pixels[i] != 0)
      XFreeColors(dpy, Pcmap, pixels + i, 1, 0);
  }
}

