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
#include <ctype.h>

#include "safemalloc.h"
#include "fvwmlib.h"
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

#if 0
#ifdef HAS_RANDR
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>
#endif
#endif

#ifndef DEBUG_PRINTS
#define DEBUG_PRINTS 1
#endif

enum
{
  env_override_policy = 1
};

enum
{
  GEOMETRY_SCREEN_GLOBAL  = -1,
  GEOMETRY_SCREEN_CURRENT = -2,
  GEOMETRY_SCREEN_PRIMARY = -3
};

enum
{
  /* Replace with -1 to switch off "primary screen" concept by default */
  DEFAULT_PRIMARY_SCREEN  = 0,
  /* Replace with GEOMETRY_SCREEN_GLOBAL to restore default behaviour */
  DEFAULT_GEOMETRY_SCREEN = GEOMETRY_SCREEN_PRIMARY
};

/* In fact, only corners matter -- there will never be GRAV_NONE */
enum {GRAV_POS = 0, GRAV_NONE = 1, GRAV_NEG = 2};
static int grav_matrix[3][3] =
{
  { NorthWestGravity, NorthGravity,  NorthEastGravity },
  { WestGravity,      CenterGravity, EastGravity },
  { SouthWestGravity, SouthGravity,  SouthEastGravity }
};
#define DEFAULT_GRAVITY NorthWestGravity


static Display            *disp              = NULL;
static Bool                is_xinerama_disabled = 0;
static XineramaScreenInfo *screens;
/* # of Xinerama screens, *not* counting the [0]global, 0 if disabled */
static int                 num_screens        = 0;
/* # of Xinerama screens, *not* counting the [0]global */
static int                 total_screens     = 0;
static int                 first_to_check    = 0;
static int                 last_to_check     = 0;
static int                 primary_scr       = DEFAULT_PRIMARY_SCREEN;

#if 0
#ifdef HAVE_RANDR
static Bool                randr_disabled    = 0;
static Bool                randr_active      = 0;
static int                 randr_event_base  = -1;
static int                 randr_error_base  = -1;
#endif
#endif

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
    blank_w = XCreateWindow(disp, root, 0, 7 * h / 8, ws, 2, 0, CopyFromParent,
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

#if 0
void XineramaSupportDisableRandR(void)
{
  if (disp != NULL)
  {
    fprintf(stderr, "XineramaSupport: WARNING: XineramaSupportDisableRandR()"
	    " called after XineramaSupportInit()!\n");
  }
  randr_disabled = 1;
}
#endif

void XineramaSupportSetPrimaryScreen(int scr)
{
  char  buf[100];

  primary_scr = scr;
  sprintf(buf, "%d", scr);
}

static int FindScreenOfXY(int x, int y)
{
  int i;

  for (i = first_to_check;  i <= last_to_check;  i++)
  {
    if (x >= screens[i].x_org  &&  x < screens[i].x_org + screens[i].width  &&
	y >= screens[i].y_org  &&  y < screens[i].y_org + screens[i].height)
    {
      return i;
    }
  }

  /* Ouch!  A "black hole" coords?  As for now, return global screen */
  return 0;
}

static void GetMouseXY(XEvent *eventp, int *x, int *y)
{
  if (is_xinerama_disabled || last_to_check == first_to_check)
  {
    /* We use .x_org,.y_org because nothing prevents a screen to be not at
     * (0,0) */
    *x = screens[first_to_check].x_org;
    *y = screens[first_to_check].y_org;
  }
  else
  {
    XEvent e;

    if (eventp == NULL)
    {
      eventp = &e;
      e.type = 0;
    }
    GetLocationFromEventOrQuery(disp, DefaultRootWindow(disp), eventp, x, y);
  }

  return;
}

#if 0
/* this function is useless. Rather force the caller to fill out all
 * parameters. */
int XineramaSupportClipToScreen(int *x, int *y, int w, int h)
{
  return XineramaSupportClipToScreenAt(*x, *y, x, y, w, h);
}
#endif

int XineramaSupportClipToScreen(
  int at_x, int at_y, int *x, int *y, int w, int h)
{
  int scr    = FindScreenOfXY(at_x, at_y);
  int x_grav = GRAV_POS, y_grav = GRAV_POS;

  if (*x + w > screens[scr].x_org + screens[scr].width)
  {
    *x = screens[scr].x_org + screens[scr].width  - w;
    x_grav = GRAV_NEG;
  }
  if (*y + h > screens[scr].y_org + screens[scr].height)
  {
    *y = screens[scr].y_org + screens[scr].height - h;
    y_grav = GRAV_NEG;
  }
  if (*x < screens[scr].x_org)
  {
    *x = screens[scr].x_org;
    x_grav = GRAV_POS;
  }
  if (*y < screens[scr].y_org)
  {
    *y = screens[scr].y_org;
    y_grav = GRAV_POS;
  }

  return grav_matrix[y_grav][x_grav];
}

/* Exists almost exclusively for FvwmForm's "Position" statement */
void XineramaSupportPositionCurrent(int *x, int *y, int px, int py, int w, int h)
{
  int  scr_x, scr_y, scr_w, scr_h;

  XineramaSupportGetCurrentScrRect(NULL, &scr_x, &scr_y, &scr_w, &scr_h);
  *x = scr_x + px;
  *y = scr_y + py;
  if (px < 0)
    *x += scr_w - w;
  if (py < 0)
    *y += scr_h - h;
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

void XineramaSupportCenterCurrent(XEvent *eventp, int *x, int *y, int w, int h)
{
  int ms_x;
  int ms_y;

  GetMouseXY(eventp, &ms_x, &ms_y);
  XineramaSupportCenter(ms_x, ms_y, x, y, w, h);
}

void XineramaSupportCenterPrimary(int *x, int *y, int w, int h)
{
  int  scr_x, scr_y, scr_w, scr_h;

  XineramaSupportGetPrimaryScrRect(&scr_x, &scr_y, &scr_w, &scr_h);
  XineramaSupportCenter(scr_x, scr_y, x, y, w, h);
}

void XineramaSupportGetCurrent00(XEvent *eventp, int *x, int *y)
{
  int  scr;
  int  ms_x, ms_y;

  GetMouseXY(eventp, &ms_x, &ms_y);
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

void XineramaSupportGetCurrentScrRect(
  XEvent *eventp, int *x, int *y, int *w, int *h)
{
  int ms_x, ms_y;

  GetMouseXY(eventp, &ms_x, &ms_y);
  XineramaSupportGetScrRect(ms_x, ms_y, x, y, w, h);
}

/* Note: <=-2:current, =-1:global, 0..?:screenN */
void XineramaSupportGetPrimaryScrRect(int *x, int *y, int *w, int *h)
{
  int  scr = primary_scr + 1;

  /* <0? (-MAXINT..-1)+1=(~-MAXINT..0) -- current mouse screen */
  if (scr < 0)
  {
    XineramaSupportGetCurrentScrRect(NULL, x, y, w, h);
    return;
  }

  if (scr == 0)
  {
    /* Treat out-of-range screen as 1st one */
  }
  else if (scr < first_to_check  ||  scr > last_to_check)
  {
    scr = first_to_check;
  }

  *x = screens[scr].x_org;
  *y = screens[scr].y_org;
  *w = screens[scr].width;
  *h = screens[scr].height;
}

void XineramaSupportGetGlobalScrRect(int *x, int *y, int *w, int *h)
{
  *x = screens[0].x_org;
  *y = screens[0].y_org;
  *w = screens[0].width;
  *h = screens[0].height;
}

void XineramaSupportGetResistanceRect(int wx, int wy, int ww, int wh,
                                      int *x0, int *y0, int *x1, int *y1)
{
  int  scr;
  int  sx0, sy0, sx1, sy1;

  /* Assign initial far-out-of-screen values */
  *x0 = 32767;
  *y0 = 32767;
  *x1 = -32768;
  *y1 = -32768;

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
      if (sx0 >= wx  &&  sx0 - wx < *x0 - wx)
	*x0 = sx0;
      if (sy0 >= wy  &&  sy0 - wy < *y0 - wy)
	*y0 = sy0;
      if (sx1 <= ww  &&  ww - sx1 < ww - *x1)
	*x1 = sx1;
      if (sy1 <= wh  &&  wh - sy1 < wh - *y1)
	*y1 = sy1;
    }
  }
}

#if 1
/*  XineramaSupportParseGeometry
 *      Parses the geometry in a form: XGeometry[@screen], i.e.
 *          [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>][@<screen>]
 *          where <screen> is either a number or "G" (global) "C" (current)
 *          or "P" (primary)
 *
 *  Args:
 *      parsestring, x_r, y_r, w_r, h_r  the same as in XParseGeometry
 *      hints  window hints structure, may be NULL
 *      flags  bitmask of allowed flags (XValue, WidthValue, XNegative...)
 *
 *  Note1:
 *      hints->width and hints->height will be used to calc negative geometry
 *      if width/height isn't specified in the geometry itself.
 *
 *  Note2:
 *      This function's behaviour is crafted to sutisfy/emulate the
 *      FvwmWinList::MakeMeWindow()'s behaviour.
 *
 *
 * dv 27-jul-2001: what is the "flags" good for? The caller is not forced to
 * use the returned values, so who cares?
 *
 *  Note3:
 *      A special value of `flags' when [XY]Value are there but [XY]Negative
 *      aren't, means that in case of negative geometry specification
 *      x_r/y_r values will be promoted to the screen border, but w/h
 *      wouldn't be subtracted, so that the program can do x-=w later
 *      ([XY]Negative *will* be returned, albeit absent in `flags').
 *          This option is supposed for proggies like FvwmButtons, which
 *      receive geometry specification long before they are able to actually
 *      use it (and which calculate w/h themselves).
 *          (The same effect can't be obtained with omitting {Width,Height}Value
 *      in the flags, since the app may wish to get the dimensions but apply
 *      some constraints later (as FvwmButtons do, BTW...).)
 *          This option can be also useful in cases where dimensions are
 *      specified not in pixels but in some other units (e.g., charcells).
 */
int XineramaSupportParseGeometry(
  char *parsestring, int *x_return, int *y_return, unsigned int *width_return,
  unsigned int *height_return)
{
  int   ret;
  int   saved;
  char *copy, *scr_p;
  int   s_size;
  int   x, y, w=0, h=0;
#if 0
  int   grav, x_grav, y_grav;
#endif
  int   scr = DEFAULT_GEOMETRY_SCREEN;
  int   scr_x, scr_y, scr_w, scr_h;

  /* Safety net */
  if (parsestring == NULL  ||  *parsestring == '\0')
    return 0;

  /* I. Do the string parsing */

  /* Make a local copy devoid of "@scr" */
  s_size = strlen(parsestring) + 1;
  copy = safemalloc(s_size);
  memcpy(copy, parsestring, s_size);
  scr_p = strchr(copy, '@');
  if (scr_p != NULL)
    *scr_p++ = '\0';

  /* Do the parsing and strip off extra bits */
  ret = XParseGeometry(copy, &x, &y, &w, &h);
  saved = ret & (XNegative | YNegative);

#if DEBUG_PRINTS
  fprintf(stderr,
	  "copy=%s, scr_p=%s, x=%d, y=%d, w=%d, h=%d, flags:%s%s%s%s%s%s\n",
	  copy, scr_p, x, y, w, h,
	  ret&XValue?      " XValue":"",
	  ret&YValue?      " YValue":"",
	  ret&WidthValue?  " WidthValue":"",
	  ret&HeightValue? " HeightValue":"",
	  ret&XNegative?   " XNegative":"",
	  ret&YNegative?   " YNegative":"");
#endif

  /* Parse the "@scr", if any */
  if (scr_p != NULL)
  {
    if (tolower(*scr_p) == 'g')
      scr = GEOMETRY_SCREEN_GLOBAL;
    else if (tolower(*scr_p) == 'c')
      scr = GEOMETRY_SCREEN_CURRENT;
    else if (tolower(*scr_p) == 'p')
      scr = GEOMETRY_SCREEN_PRIMARY;
    else if (*scr_p >= '0' && *scr_p <= '9')
      scr = atoi(scr_p);
  }

  /* We don't need the string any more */
  free(copy);

  /* II. Get the screen rectangle */
  switch (scr)
  {
  case GEOMETRY_SCREEN_GLOBAL:
    XineramaSupportGetGlobalScrRect(&scr_x, &scr_y, &scr_w, &scr_h);
    break;

  case GEOMETRY_SCREEN_CURRENT:
    XineramaSupportGetCurrentScrRect(NULL, &scr_x, &scr_y, &scr_w, &scr_h);
    break;

  case GEOMETRY_SCREEN_PRIMARY:
    XineramaSupportGetPrimaryScrRect(&scr_x, &scr_y, &scr_w, &scr_h);
    break;

  default:
    scr++;
    if (scr < first_to_check  ||  scr > last_to_check)
      scr = first_to_check;
    scr_x = screens[scr].x_org;
    scr_y = screens[scr].y_org;
    scr_w = screens[scr].width;
    scr_h = screens[scr].height;
  }

#if DEBUG_PRINTS
  fprintf(stderr, "scr=%d, (%d,%d,%d,%d)\n", scr, scr_x, scr_y, scr_w, scr_h);
#endif

  /* III. Interpret and fill in the values */

  /* Fill in dimensions for future negative calculations if
   * omitted/forbidden */
  /*!!! Maybe should use *x_return,*y_return if hints==NULL?  Unreliable... */
#if 0
  if (hints != NULL  &&  hints->flags & PSize)
  {
    if ((ret & WidthValue)  == 0)
      w = hints->width;
    if ((ret & HeightValue) == 0)
      h = hints->height;
  }
  else
#endif
  {
    /* This branch is required for case when size *is* specified, but masked
     * off */
    if ((ret & WidthValue)  == 0)
      w = 0;
    if ((ret & HeightValue) == 0)
      h = 0;
  }

#if DEBUG_PRINTS
  fprintf(stderr, "PRE: x=%d, y=%d, w=%d, h=%d, flags:%s%s%s%s%s%s\n",
	  x, y, w, h,
	  ret&XValue?      " XValue":"",
	  ret&YValue?      " YValue":"",
	  ret&WidthValue?  " WidthValue":"",
	  ret&HeightValue? " HeightValue":"",
	  ret&XNegative?   " XNegative":"",
	  ret&YNegative?   " YNegative":"");
#endif

  /* Advance coords to the screen... */
  x += scr_x;
  y += scr_y;

  /* ...and process negative geometries */
  if (saved & XNegative)
    x += scr_w;
  if (saved & YNegative)
    y += scr_h;
  if (ret & XNegative)
    x -= w;
  if (ret & YNegative)
    y -= h;

  /* Restore negative bits */
  ret |= saved;

#if 0
  /* Guess orientation */
  x_grav = (ret & XNegative)? GRAV_NEG : GRAV_POS;
  y_grav = (ret & YNegative)? GRAV_NEG : GRAV_POS;
  grav   = grav_matrix[y_grav][x_grav];
#endif

  /* Return the values */
  if (ret & XValue)
  {
    *x_return = x;
#if 0
    if (hints != NULL)
      hints->x = x;
#endif
  }
  if (ret & YValue)
  {
    *y_return = y;
#if 0
    if (hints != NULL)
      hints->y = y;
#endif
  }
  if (ret & WidthValue)
  {
    *width_return = w;
#if 0
    if (hints != NULL)
      hints->width = w;
#endif
  }
  if (ret & HeightValue)
  {
    *height_return = h;
#if 0
    if (hints != NULL)
      hints->height = h;
#endif
  }
#if 0
  if (1 /*flags & GravityValue*/  &&  grav != DEFAULT_GRAVITY)
  {
    if (hints != NULL  &&  hints->flags & PWinGravity)
      hints->win_gravity = grav;
  }
#endif
#if 0
  if (hints != NULL  &&  ret & XValue  &&  ret & YValue)
    hints->flags |= USPosition;
#endif

#if DEBUG_PRINTS
  fprintf(stderr, "x=%d, y=%d, w=%d, h=%d, flags:%s%s%s%s%s%s\n",
	  x, y, w, h,
	  ret&XValue?      " XValue":"",
	  ret&YValue?      " YValue":"",
	  ret&WidthValue?  " WidthValue":"",
	  ret&HeightValue? " HeightValue":"",
	  ret&XNegative?   " XNegative":"",
	  ret&YNegative?   " YNegative":"");
#endif

  return ret;
}
#endif


/* no rand_r for now */
# if 0
int  XineramaSupportGetRandrEventType(void)
{
#ifdef HAS_RANDR
  return randr_active? randr_event_base + RRScreenChangeNotify : 0;
#else
  return 0;
#endif
}

Bool XineramaSupportHandleRandrEvent(XEvent *event,
                                     int *old_w, int *old_h,
                                     int *new_w, int *new_h)
{
#ifndef HAS_RANDR
  return 0;
#else
  XRRScreenChangeNotifyEvent *ev = (XRRScreenChangeNotifyEvent *)event;
  int nw, nh, tmp;

  if (!randr_active  ||
      event->type != randr_event_base + RRScreenChangeNotify)
    return 0;

  nw = ev->width;
  nh = ev->height;

  /*
   * Note1: this check is not very good, since the right way is to
   *        obtain a list of possible rotations and ...
   *
   * Note2: as to WM's point of view, I'm unsure if rotation should be
   *        treated exactly as resizing (i.e. that it should reposition
   *        windows in the same fashion).
   */
  if (ev->rotation & (1<<1 | 1<<3))
  {
    tmp = nw;
    nw  = nh;
    nh  = tmp;
  }

  *old_w = screens[0].width;
  *old_h = screens[0].height;

  screens[0].width  = *new_w = nw;
  screens[0].height = *new_h = nh;

#ifdef DEBUG_PRINTS
  fprintf(stderr, "HandleRandrEvent(): rot=%d, old=%d*%d, new=%d*%d, d=%d*%d\n",
	  ev->rotation,
	  *old_w, *old_h, *new_w, *new_h,
	  DisplayWidth(disp, DefaultScreen(disp)),
	  DisplayHeight(disp, DefaultScreen(disp)));
#endif

  return (nw != *old_w  ||  nh != *old_h);
#endif
}
#endif
