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
 *   from [1] to [num_screens].  screens[0] is always the "global" screen,
 *   so if Xinerama is unavailable or disabled, the module performs
 *   all checks on screens[0] instead of screens[1..num_screens].
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
 *
 *   Modules with Xinerama support should listen to the XineramaEnable
 *   and XineramaDisable strings coming over the module pipe as
 *   M_CONFIG_INFO packets and call XineramaSupportEnable() or
 *   XineramaSupportDisable in response.
 *
 *********************************************************************/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include "safemalloc.h"

#include "XineramaSupport.h"
#ifdef HAVE_XINERAMA
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

static Display            *disp              = NULL;
static Bool                is_xinerama_disabled = 0;
static XineramaScreenInfo *screens;
/* # of Xinerama screens, *not* counting the [0]global, 0 if disabled */
static int                 num_screens        = 0;
/* # of Xinerama screens, *not* counting the [0]global */
static int                 total_screens     = 0;
static int                 first_to_check    = 0;
static int                 last_to_check     = 0;
#ifdef USE_XINERAMA_EMULATION
static Window blank_w, vert_w;
#endif


static void XineramaSupportSetState(Bool onoff)
{
  if (onoff && total_screens > 0)
  {
    first_to_check = 1;
    last_to_check  = num_screens;
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
#ifdef HAVE_XINERAMA
  int dummy_rc;
#endif

  if (is_initialised)
    return;
  is_initialised = True;
  disp = dpy;
#ifdef USE_XINERAMA_EMULATION
  if (True)
  {
    int count;
    int w;
    int h;
    int ws;
    unsigned long scr;
    Window root; 
    XSetWindowAttributes attributes;

    scr = DefaultScreen(disp);
    root = RootWindow(disp, scr);

    /* xinerama emulation simulates xinerama on a single screen:
     *
     * actual screen
     * +---------------------+--------------+
     * |                     |              |
     * |                     |              |
     * |                     |              |
     * |                     |  simulated   |
     * |                     |  screen 2    |
     * |  simulated screen 1 |              |
     * |                     |              |
     * |                     |              |
     * |                     |              |
     * |                     |              |
     * +---------------------+              |
     * |   blank area        |              |
     * |                     |              |
     * +---------------------+--------------+
     */
    count = 2;
    total_screens = count;
    num_screens = (is_xinerama_disabled) ? 0 : count;
    screens = (XineramaScreenInfo *)
      safemalloc(sizeof(XineramaScreenInfo) * (1 + count));
    /* calculate the faked sub screen dimensions */
    w = DisplayWidth(disp, scr);
    ws = 3 * w / 5;
    h = DisplayHeight(disp, scr);
    screens[1].screen_number = 0;
    screens[1].x_org         = 0;
    screens[1].y_org         = 0;
    screens[1].width         = ws;
    screens[1].height        = 7 * h / 8;
    screens[2].screen_number = 1;
    screens[2].x_org         = ws;
    screens[2].y_org         = 0;
    screens[2].width         = w - ws;
    screens[2].height        = h;
    /* add delimiter */
    attributes.background_pixel = WhitePixel(disp, scr);
    attributes.override_redirect = True;
    blank_w = XCreateWindow(disp, root, 0, 7 * h / 8, ws, h/8, 0, CopyFromParent,
			    CopyFromParent, CopyFromParent,
			    CWBackPixel|CWOverrideRedirect,
			    &attributes);
    vert_w = XCreateWindow(disp, root, ws, 0, 2, h, 0, CopyFromParent,
			    CopyFromParent, CopyFromParent,
			    CWBackPixel|CWOverrideRedirect,
			    &attributes);
    XMapRaised(disp,blank_w);
    XMapRaised(disp,vert_w);
  }
  else
#endif
#ifdef HAVE_XINERAMA
  if (XineramaQueryExtension(disp, &dummy_rc, &dummy_rc) &&
      XineramaIsActive(disp))
  {
    int count;
    XineramaScreenInfo *info;

    info = XineramaQueryScreens(disp, &count);
    total_screens = count;
    num_screens = (is_xinerama_disabled) ? 0 : count;
    screens = (XineramaScreenInfo *)
      safemalloc(sizeof(XineramaScreenInfo) * (1 + count));
    memcpy(screens + 1, info, sizeof(XineramaScreenInfo) * count);
    XFree(info);
  }
  else
#endif
  {
    num_screens = 0;
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
  num_screens = 0;
  /* Fill in the screen range */
  XineramaSupportSetState(1);
#ifdef USE_XINERAMA_EMULATION
  XUnmapWindow(disp,blank_w);
  XUnmapWindow(disp,vert_w);
#endif
}

void XineramaSupportEnable(void)
{
  is_xinerama_disabled = 0;
  num_screens = total_screens;
  /* Fill in the screen range */
  XineramaSupportSetState(1);
#ifdef USE_XINERAMA_EMULATION
  XMapRaised(disp,blank_w);
  XMapRaised(disp,vert_w);
#endif
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

void XineramaSupportClipToScreen(
  int src_x, int src_y, int *dest_x, int *dest_y, int w, int h)
{
  int  scr = FindScreenOfXY(src_x, src_y);

  if (*dest_x + w > screens[scr].x_org + screens[scr].width)
    *dest_x = screens[scr].x_org + screens[scr].width - w;
  if (*dest_y + h > screens[scr].y_org + screens[scr].height)
    *dest_y = screens[scr].y_org + screens[scr].height - h;
  if (*dest_x < screens[scr].x_org)
    *dest_x = screens[scr].x_org;
  if (*dest_y < screens[scr].y_org)
    *dest_y = screens[scr].y_org;
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

Bool XineramaSupportGetScrRect(int l_x, int l_y, int *x, int *y, int *w, int *h)
{
  int scr = FindScreenOfXY(l_x, l_y);

  *x = screens[scr].x_org;
  *y = screens[scr].y_org;
  *w = screens[scr].width;
  *h = screens[scr].height;

  /* Return 0 if the point was on none of the defined screens */
  return !(scr == 0 && num_screens > 1);
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
