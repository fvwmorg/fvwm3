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

/*********************************************************************
 *  XineramaSupport.c
 *    Xinerama abstraction support for window manager.
 *
 *  This module is all original code
 *  by Dmitry Yu. Bolkhovityanov <bolkhov@inp.nsk.su>
 *  Copyright 2001, Dmitry Bolkhovityanov
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ********************************************************************/

/*********************************************************************
 * Brief description of used concept:
 *
 *   This code is always used by client, regardless of if Xinerama is
 *   available or not (either because -lXinerama was missing or
 *   because Xinerama extension is missing on display).
 *
 *   If Xinerama is available, this module maintains a list of screens,
 *   from [1] to [numscreens].  screens[0] is always the "global" screen,
 *   so if Xinerama is unavailable or disabled, the module performs
 *   all checks on screens[0] instead of screens[1..numscreens].
 *
 *   The client should first call the XineramaSupportInit(), passing
 *   it the opened display descriptor.  During this call the list of
 *   Xinerama screens is retrieved and 'dpy' is saved for future
 *   reference.
 *
 *   If the client wishes to hard-disable Xinerama support (e.g. if
 *   Xinerama extension is present but is broken on display), it should
 *   call XineramaSupportDisable() *before* XineramaSupportInit().
 *
 *   Using real Xinerama screens info may be switched on/off "on the
 *   fly" by calling XineramaSupportSetState(0=off/else=on).
 *********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "XineramaSupport.h"
#include "safemalloc.h"

#ifdef HAS_XINERAMA
#include <X11/extensions/Xinerama.h>
#else
typedef struct
{
  int   screen_number;
  short x_org;
  short y_org;
  short width;
  short height;
} XineramaScreenInfo;
#endif

#ifndef DO_MALLOC
#define DO_MALLOC malloc
#endif

static Display            *disp              = NULL;
static Bool                is_xinerama_disabled = 0;
static XineramaScreenInfo *screens;
/* # of Xinerama screens, *not* counting the [0]global, 0 if disabled */
static int                 numscreens        = 0;
/* # of Xinerama screens, *not* counting the [0]global */
static int                 total_screens     = 0;
static int                 first_to_check    = 0;
static int                 last_to_check     = 0;


static void XineramaSupportSetState(Bool onoff)
{
  if (onoff && total_screens > 0)
  {
    first_to_check = 1;
    last_to_check  = numscreens;
  }
  else
  {
    first_to_check = 0;
    last_to_check = 0;
  }
}

void XineramaSupportInit(Display *dpy)
{
  static Bool is_initialised = False;
#ifdef HAS_XINERAMA
  XineramaScreenInfo *info;
  int count;
#endif

  if (is_initialised)
    return;
  is_initialised = True;
  disp = dpy;
#ifdef HAS_XINERAMA
  if (XineramaIsActive(disp))
  {
    info = XineramaQueryScreens(disp, &count);
    total_screens = count;
    numscreens = (is_xinerama_disabled) ? 0 : count;
    screens = (XineramaScreenInfo *)
      safemalloc(sizeof(XineramaScreenInfo) * (1 + count));
    memcpy(screens + 1, info, sizeof(XineramaScreenInfo) * count);
    XFree(info);
  }
  else
#endif
  {
    numscreens = 0;
    total_screens = 0;
    screens = (XineramaScreenInfo *)safemalloc(sizeof(XineramaScreenInfo)*1);
  }

  /* Now, fill screens[0] with global screen parameters */
  screens[0].screen_number = -1;
  screens[0].x_org         = 0;
  screens[0].y_org         = 0;
  screens[0].width         = DisplayWidth (disp, DefaultScreen(disp));
  screens[0].height        = DisplayHeight(disp, DefaultScreen(disp));

  /* Fill in the screen range */
  XineramaSupportSetState(1);

  return;
}

void XineramaSupportDisable(void)
{
  is_xinerama_disabled = 1;
  numscreens = 0;
  /* Fill in the screen range */
  XineramaSupportSetState(1);
}

void XineramaSupportEnable(void)
{
  is_xinerama_disabled = 0;
  numscreens = total_screens;
  /* Fill in the screen range */
  XineramaSupportSetState(1);
}

static int FindScreenOfXY(int x, int y)
{
  int i;

  for (i = first_to_check;  i <= last_to_check;  i++)
    if (x >= screens[i].x_org  &&  x < screens[i].x_org + screens[i].width  &&
	y >= screens[i].y_org  &&  y < screens[i].y_org + screens[i].height)
      return i;

  /* Ouch!  A "black hole" coords?  As for now, return global screen */
  return 0;
}

static void GetMouseXY(int *x, int *y)
{
  Window r_w;
  Window c_w;
  int int_v;
  unsigned int uint_v;

  if (is_xinerama_disabled || last_to_check == first_to_check)
  {
    *x = 0;
    *y = 0;
  }
  else
  {
    XQueryPointer(
      disp, RootWindow(disp, DefaultScreen(disp)), &r_w, &c_w, x, y, &int_v,
      &int_v, &uint_v);
  }

  return;
}

void XineramaSupportClipToScreen(int *x, int *y, int w, int h)
{
  int  scr = FindScreenOfXY(*x, *y);

  if (*x + w > screens[scr].x_org + screens[scr].width)
    *x = screens[scr].x_org + screens[scr].width - w;
  if (*y + h > screens[scr].y_org + screens[scr].height)
    *y = screens[scr].y_org + screens[scr].height - h;
  if (*x < screens[scr].x_org)
    *x = screens[scr].x_org;
  if (*y < screens[scr].y_org)
    *y = screens[scr].y_org;
}

void XineramaSupportCenter(int ms_x, int ms_y, int *x, int *y, int w, int h)
{
  int  scr = FindScreenOfXY(ms_x, ms_y);

  *x = (screens[scr].width  - w) / 2;
  *y = (screens[scr].height - h) / 2;
  if (*x < 0)
    *x = 0;
  if (*y < 0)
    *y = 0;
  *x += screens[scr].x_org;
  *y += screens[scr].y_org;
}

void XineramaSupportCenterCurrent(int *x, int *y, int w, int h)
{
  int ms_x;
  int ms_y;

  GetMouseXY(&ms_x, &ms_y);
  XineramaSupportCenter(ms_x, ms_y, x, y, w, h);
}

void XineramaSupportGetCurrent00(int *x, int *y)
{
  int  scr;
  int  ms_x, ms_y;

  GetMouseXY(&ms_x, &ms_y);
  scr = FindScreenOfXY(ms_x, ms_y);
  *x = screens[scr].x_org;
  *y = screens[scr].y_org;
}

void XineramaSupportGetScrRect(int l_x, int l_y, int *x, int *y, int *w, int *h)
{
  int scr = FindScreenOfXY(l_x, l_y);

  *x = screens[scr].x_org;
  *y = screens[scr].y_org;
  *w = screens[scr].width;
  *h = screens[scr].height;
}

void XineramaSupportGetCurrentScrRect(int *x, int *y, int *w, int *h)
{
  int ms_x, ms_y;

  GetMouseXY(&ms_x, &ms_y);
  XineramaSupportGetScrRect(ms_x, ms_y, x, y, w, h);
}

void XineramaSupportGetResistanceRect(int wx, int wy, int ww, int wh,
                                      int *x0, int *y0, int *x1, int *y1)
{
  int  scr;
  int  sx0, sy0, sx1, sy1;

  /* Assign initial far-out-of-screen values */
  *x0 = *y0 = 32767;
  *x1 = *y1 = -32768;

  /* Convert size to 2-nd edge coords */
  ww += wx;
  wh += wy;

  for (scr = first_to_check;  scr <= last_to_check;  scr++)
  {
    sx0 = screens[scr].x_org;
    sy0 = screens[scr].y_org;
    sx1 = screens[scr].width  + sx0;
    sy1 = screens[scr].height + sy0;

    /* Take this screen into account only if it intersects with the window.
       Otherwise this algorithm would behave badly in case of
       non-regularly-tiled screens (i.e. when they don't form a regular
       grid). */
    if (sx0 <= ww  &&  sx1 >= wx  &&  sy0 <= wh  &&  sy1 >= wy)
    {
      if (sx0 >= wx  &&  sx0 - wx < *x0 - wx)  *x0 = sx0;
      if (sy0 >= wy  &&  sy0 - wy < *y0 - wy)  *y0 = sy0;
      if (sx1 <= ww  &&  ww - sx1 < ww - *x1)  *x1 = sx1;
      if (sy1 <= wh  &&  wh - sy1 < wh - *y1)  *y1 = sy1;
    }
  }
}
