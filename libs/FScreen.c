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
 *  FScreen.c
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
 *   The client should first call the FScreenInit(), passing
 *   it the opened display descriptor.  During this call the list of
 *   Xinerama screens is retrieved and 'dpy' is saved for future
 *   reference.
 *
 *   If the client wishes to hard-disable Xinerama support (e.g. if
 *   Xinerama extension is present but is broken on display), it should
 *   call FScreenDisable() *before* FScreenInit().
 *
 *   Using real Xinerama screens info may be switched on/off "on the
 *   fly" by calling FScreenSetState(0=off/else=on).
 *
 *   Modules with Xinerama support should listen to the XineramaEnable
 *   and XineramaDisable strings coming over the module pipe as
 *   M_CONFIG_INFO packets and call FScreenEnable() or
 *   FScreenDisable in response.
 *
 *********************************************************************/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "defaults.h"
#include "fvwmlib.h"
#include "safemalloc.h"
#include "FScreen.h"

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
#ifdef HAVE_RANDR
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>
#endif
#endif

#ifndef DEBUG_PRINTS
/* #define DEBUG_PRINTS 1 */
#endif

enum
{
  /* Replace with FSCREEN_GLOBAL to restore default behaviour */
  DEFAULT_GEOMETRY_SCREEN = FSCREEN_PRIMARY
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
static Bool                is_xinerama_disabled = DEFAULT_XINERAMA_DISABLED;
static XineramaScreenInfo *screens;
/* # of Xinerama screens, *not* counting the [0]global, 0 if disabled */
static int                 num_screens        = 0;
/* # of Xinerama screens, *not* counting the [0]global */
static int                 total_screens     = 0;
static int                 first_to_check    = 0;
static int                 last_to_check     = 0;
static int                 primary_scr       = DEFAULT_PRIMARY_SCREEN + 1;
static int                 default_geometry_scr = FSCREEN_PRIMARY;

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

static int FScreenParseScreenBit(char *arg, char default_screen);

Bool FScreenIsEnabled(void)
{
  return (is_xinerama_disabled || num_screens == 0) ? False : True;
}

static void FScreenSetState(Bool onoff)
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

void FScreenInit(Display *dpy)
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
  FScreenSetState(1);

  return;
}

void FScreenDisable(void)
{
  is_xinerama_disabled = 1;
  num_screens = 0;
  /* Fill in the screen range */
  FScreenSetState(1);
#ifdef USE_XINERAMA_EMULATION
  XUnmapWindow(disp,blank_w);
  XUnmapWindow(disp,vert_w);
#endif
}

void FScreenEnable(void)
{
  is_xinerama_disabled = 0;
  num_screens = total_screens;
  /* Fill in the screen range */
  FScreenSetState(1);
#ifdef USE_XINERAMA_EMULATION
  XMapRaised(disp,blank_w);
  XMapRaised(disp,vert_w);
#endif
}

#if 0
void FScreenDisableRandR(void)
{
  if (disp != NULL)
  {
    fprintf(stderr, "FScreen: WARNING: FScreenDisableRandR()"
	    " called after FScreenInit()!\n");
  }
  randr_disabled = 1;
}
#endif

int FScreenGetPrimaryScreen(void)
{
  return (is_xinerama_disabled) ? 0 : primary_scr;
}

void FScreenSetPrimaryScreen(int scr)
{
  if (scr >= 0 && scr < num_screens)
  {
    primary_scr = scr + 1;
  }
  else
  {
    primary_scr = 0;
  }

  return;
}

/* Intended to be called by modules.  Simply pass in the parameter from the
 * config string sent by fvwm. */
void FScreenConfigureModule(char *args)
{
  int screen = 0;

  GetIntegerArguments(args, NULL, &screen, 1);
  if (screen < 0)
  {
    screen = -1;
  }
  FScreenSetPrimaryScreen(screen - 1);
  if (screen == -1)
  {
    FScreenDisable();
  }
  else
  {
    FScreenEnable();
  }

  return;
}

/* Sets the default screen for ...ParseGeometry if no screen spec is given.
 * Usually this is FSCREEN_SPEC_PRIMARY, but this won't allow modules to appear
 * under the pointer. */
void FScreenSetDefaultModuleScreen(char *scr_spec)
{
  default_geometry_scr = FScreenParseScreenBit(scr_spec, FSCREEN_SPEC_PRIMARY);
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

/* Returns the specified screens geometry rectangle.  screen can be a screen
 * number or any of the values FSCREEN_GLOBAL, FSCREEN_CURRENT,
 * FSCREEN_PRIMARY or FSCREEN_XYPOS.  The arg union only has a meaning for
 * FSCREEN_CURRENT and FSCREEN_XYARG.  For FSCREEN_CURRENT its mouse_ev member
 * may be given.  It is tried to find out the pointer position from the event
 * first before querying the pointer.  For FSCREEN_XYPOS the xpos member is used
 * to fetch the x/y position of the point on the screen.  If arg is NULL, the
 * position 0 0 is assumed instead.
 *
 * Any of the arguments arg, x, y, w and h may be NULL.
 *
 * FSCREEN_GLOBAL:  return the global screen dimensions
 * FSCREEN_CURRENT: return dimensions of the screen with the pointer
 * FSCREEN_PRIMARY: return the primary screen dimensions
 * FSCREEN_XYPOS:   return dimensions of the screen with the given coordinates
 *
 * The function returns False if the global screen was returned and more than
 * one screen is configured.  Otherwise it returns True.
 */
Bool FScreenGetScrRect(
  fscreen_scr_arg *arg, int screen, int *x, int *y, int *w, int *h)
{
  fscreen_scr_arg tmp;

  if (num_screens == 0)
  {
    screen = FSCREEN_GLOBAL;
  }
  switch (screen)
  {
  case FSCREEN_GLOBAL:
    screen = 0;
    break;
  case FSCREEN_PRIMARY:
    screen = primary_scr;
    break;
  case FSCREEN_CURRENT:
    /* translate to xypos format */
    if (!arg)
    {
      tmp.mouse_ev = NULL;
      arg = &tmp;
    }
    GetMouseXY(arg->mouse_ev, &arg->xypos.x, &arg->xypos.y);
    /* fall through */
  case FSCREEN_XYPOS:
    /* translate to screen number */
    if (!arg)
    {
      tmp.xypos.x = 0;
      tmp.xypos.y = 0;
      arg = &tmp;
    }
    screen = FindScreenOfXY(arg->xypos.x, arg->xypos.y);
    break;
  default:
    /* screen is given counting from 0; translate to counting from 1 */
    screen++;
    break;
  }

  if (screen < first_to_check || screen > last_to_check)
  {
    screen = 0;
  }
  if (x)
  {
    *x = screens[screen].x_org;
  }
  if (y)
  {
    *y = screens[screen].y_org;
  }
  if (w)
  {
    *w = screens[screen].width;
  }
  if (h)
  {
    *h = screens[screen].height;
  }

  return !(screen == 0 && num_screens > 1);
}

/* Arguments work exactly like for FScreenGetScrRect() */
int FScreenClipToScreen(
  fscreen_scr_arg *arg, int screen, int *x, int *y, int w, int h)
{
  int sx;
  int sy;
  int sw;
  int sh;
  int lx = (x) ? *x : 0;
  int ly = (y) ? *y : 0;
  int x_grav = GRAV_POS;
  int y_grav = GRAV_POS;

  FScreenGetScrRect(arg, screen, &sx, &sy, &sw, &sh);
  if (lx + w > sx + sw)
  {
    lx = sx + sw  - w;
    x_grav = GRAV_NEG;
  }
  if (ly + h > sy + sh)
  {
    ly = sy + sh - h;
    y_grav = GRAV_NEG;
  }
  if (lx < sx)
  {
    lx = sx;
    x_grav = GRAV_POS;
  }
  if (ly < sy)
  {
    ly = sy;
    y_grav = GRAV_POS;
  }
  if (x)
  {
    *x = lx;
  }
  if (y)
  {
    *y = ly;
  }

  return grav_matrix[y_grav][x_grav];
}

/* Arguments work exactly like for FScreenGetScrRect() */
void FScreenCenterOnScreen(
  fscreen_scr_arg *arg, int screen, int *x, int *y, int w, int h)
{
  int sx;
  int sy;
  int sw;
  int sh;
  int lx;
  int ly;

  FScreenGetScrRect(arg, screen, &sx, &sy, &sw, &sh);
  lx = (sw  - w) / 2;
  ly = (sh - h) / 2;
  if (lx < 0)
    lx = 0;
  if (ly < 0)
    ly = 0;
  lx += sx;
  ly += sy;
  if (x)
  {
    *x = lx;
  }
  if (y)
  {
    *y = ly;
  }

  return;
}

void FScreenGetResistanceRect(int wx, int wy, int ww, int wh,
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
    if (sx0 < ww && sx1 > wx && sy0 < wh && sy1 > wy)
    {
      if (sx0 >= wx && sx0 - wx < *x0 - wx)
	*x0 = sx0;
      if (sy0 >= wy && sy0 - wy < *y0 - wy)
	*y0 = sy0;
      if (sx1 <= ww && ww - sx1 < ww - *x1)
	*x1 = sx1;
      if (sy1 <= wh && wh - sy1 < wh - *y1)
	*y1 = sy1;
    }
  }
}

/* Arguments work exactly like for FScreenGetScrRect() */
Bool FScreenIsRectangleOnScreen(
  fscreen_scr_arg *arg, int screen, rectangle *rec)
{
  int sx;
  int sy;
  int sw;
  int sh;

  FScreenGetScrRect(arg, screen, &sx, &sy, &sw, &sh);
  return (rec->x + rec->width > sx && rec->x < sx + sw &&
	  rec->y + rec->height > sy && rec->y < sy + sh) ? True : False;
}


static int FScreenParseScreenBit(char *scr_spec, char default_screen)
{
  int scr = default_geometry_scr;
  char c;

  c = (scr_spec) ? tolower(*scr_spec) : tolower(default_screen);
  if (c == FSCREEN_SPEC_GLOBAL)
    scr = FSCREEN_GLOBAL;
  else if (c == FSCREEN_SPEC_CURRENT)
    scr = FSCREEN_CURRENT;
  else if (c == FSCREEN_SPEC_PRIMARY)
    scr = FSCREEN_PRIMARY;
  else if (isdigit(c))
    scr = atoi(scr_spec);
  else
  {
    c = tolower(default_screen);
    if (c == FSCREEN_SPEC_GLOBAL)
      scr = FSCREEN_GLOBAL;
    else if (c == FSCREEN_SPEC_CURRENT)
      scr = FSCREEN_CURRENT;
    else if (c == FSCREEN_SPEC_PRIMARY)
      scr = FSCREEN_PRIMARY;
    else if (isdigit(c))
      scr = atoi(scr_spec);
  }

  return scr;
}

int FScreenGetScreenArgument(char *scr_spec, char default_screen)
{
  while (scr_spec && isspace(*scr_spec))
    scr_spec++;

  return FScreenParseScreenBit(scr_spec, default_screen);
}

#if 1
/*
 * FScreenParseGeometry
 *     Does the same as XParseGeometry, but handles additional "@scr".
 *     Since it isn't safe to define "ScreenValue" constant (actual values
 *     of other "XXXValue" are specified in Xutil.h, not by us, so there can
 *     be a clash), the screen value is always returned, event if it wasn't
 *     present in `parse_string' (set to default in that case).
 *
 */
int FScreenParseGeometryWithScreen(
  char *parsestring, int *x_return, int *y_return, unsigned int *width_return,
  unsigned int *height_return, int *screen_return)
{
  int   ret;
  char *copy, *scr_p;
  int   s_size;
  int   scr;

  /* Safety net */
  if (parsestring == NULL  ||  *parsestring == '\0')
    return 0;

  /* Make a local copy devoid of "@scr" */
  s_size = strlen(parsestring) + 1;
  copy = safemalloc(s_size);
  memcpy(copy, parsestring, s_size);
  scr_p = strchr(copy, '@');
  if (scr_p != NULL)
    *scr_p++ = '\0';

  /* Do the parsing */
  ret = XParseGeometry(copy, x_return, y_return, width_return, height_return);

#if DEBUG_PRINTS
  fprintf(stderr,
	  "copy=%s, scr_p=%s, x=%d, y=%d, w=%d, h=%d, flags:%s%s%s%s%s%s\n",
	  copy, (scr_p)?scr_p:"(null)", *x_return, *y_return, *width_return,
	  *height_return,
	  ret&XValue?      " XValue":"",
	  ret&YValue?      " YValue":"",
	  ret&WidthValue?  " WidthValue":"",
	  ret&HeightValue? " HeightValue":"",
	  ret&XNegative?   " XNegative":"",
	  ret&YNegative?   " YNegative":"");
#endif

  /* Parse the "@scr", if any */
  scr = FScreenParseScreenBit(scr_p, FSCREEN_SPEC_PRIMARY);
  *screen_return = scr;

  /* We don't need the string any more */
  free(copy);

  return ret;
}

/* Same as above, but dump screen return value to keep compatible with the X
 * function. */
int FScreenParseGeometry(
  char *parsestring, int *x_return, int *y_return, unsigned int *width_return,
  unsigned int *height_return)
{
  int scr;
  int rc;
  int mx;
  int my;

  rc = FScreenParseGeometryWithScreen(
    parsestring, x_return, y_return, width_return, height_return, &scr);
  switch (scr)
  {
  case FSCREEN_GLOBAL:
    scr = 0;
    break;
  case FSCREEN_CURRENT:
    GetMouseXY(NULL, &mx, &my);
    scr = FindScreenOfXY(mx, my);
    break;
  case FSCREEN_PRIMARY:
    scr = primary_scr;
    break;
  default:
    scr++;
    break;
  }
  if (scr <= 0 || scr > last_to_check)
  {
    scr = 0;
  }
  else
  {
    /* adapt geometry to selected screen */
    if (rc & XValue)
    {
      if (rc & XNegative)
      {
	*x_return -=
	  (screens[0].width - screens[scr].width - screens[scr].x_org);
      }
      else
      {
	*x_return += screens[scr].x_org;
      }
    }
    if (rc & YValue)
    {
      if (rc & YNegative)
      {
	*y_return -=
	  (screens[0].height - screens[scr].height - screens[scr].y_org);
      }
      else
      {
	*y_return += screens[scr].y_org;
      }
    }
  }
#if DEBUG_PRINTS
  fprintf(stderr, "*** xpg: x=%d, y=%d, w=%d, h=%d, flags:%s%s%s%s%s%s\n",
	  *x_return, *y_return, *width_return, *height_return,
	  rc&XValue?      " XValue":"",
	  rc&YValue?      " YValue":"",
	  rc&WidthValue?  " WidthValue":"",
	  rc&HeightValue? " HeightValue":"",
	  rc&XNegative?   " XNegative":"",
	  rc&YNegative?   " YNegative":"");
#endif

  return rc;
}


/*  FScreenGetGeometry
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
int FScreenGetGeometry(
  char *parsestring, int *x_return, int *y_return,
  int *width_return, int *height_return, XSizeHints *hints, int flags)
{
  int   ret;
  int   saved;
  int   x, y, w=0, h=0;
  int   grav, x_grav, y_grav;
  int   scr = default_geometry_scr;
  int   scr_x, scr_y, scr_w, scr_h;

  /* I. Do the parsing and strip off extra bits */
  ret = FScreenParseGeometryWithScreen(parsestring, &x, &y, &w, &h, &scr);
  saved = ret & (XNegative | YNegative);
  ret  &= flags;

  /* II. Get the screen rectangle */
  switch (scr)
  {
  case FSCREEN_GLOBAL:
  case FSCREEN_CURRENT:
  case FSCREEN_PRIMARY:
  case FSCREEN_XYPOS:
    FScreenGetScrRect(NULL, scr, &scr_x, &scr_y, &scr_w, &scr_h);
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
  if (hints != NULL  &&  hints->flags & PSize)
  {
    if ((ret & WidthValue)  == 0)
      w = hints->width;
    if ((ret & HeightValue) == 0)
      h = hints->height;
  }
  else
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

  /* Guess orientation */
  x_grav = (ret & XNegative)? GRAV_NEG : GRAV_POS;
  y_grav = (ret & YNegative)? GRAV_NEG : GRAV_POS;
  grav   = grav_matrix[y_grav][x_grav];

  /* Return the values */
  if (ret & XValue)
  {
    *x_return = x;
    if (hints != NULL)
      hints->x = x;
  }
  if (ret & YValue)
  {
    *y_return = y;
    if (hints != NULL)
      hints->y = y;
  }
  if (ret & WidthValue)
  {
    *width_return = w;
    if (hints != NULL)
      hints->width = w;
  }
  if (ret & HeightValue)
  {
    *height_return = h;
    if (hints != NULL)
      hints->height = h;
  }
  if (1 /*flags & GravityValue*/  &&  grav != DEFAULT_GRAVITY)
  {
    if (hints != NULL  &&  hints->flags & PWinGravity)
      hints->win_gravity = grav;
  }
  if (hints != NULL  &&  ret & XValue  &&  ret & YValue)
    hints->flags |= USPosition;

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
int  FScreenGetRandrEventType(void)
{
#ifdef HAVE_RANDR
  return randr_active? randr_event_base + RRScreenChangeNotify : 0;
#else
  return 0;
#endif
}

Bool FScreenHandleRandrEvent(
  XEvent *event, int *old_w, int *old_h, int *new_w, int *new_h)
{
#ifndef HAVE_RANDR
  return 0;
#else
  XRRScreenChangeNotifyEvent *ev = (XRRScreenChangeNotifyEvent *)event;
  int nw, nh, tmp;

  if (!randr_active  ||
      event->type != randr_event_base + RRScreenChangeNotify)
  {
    return 0;
  }

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

  screens[0].width  = nw;
  *new_w = nw;
  screens[0].height = nh;
  *new_h = nh;

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
