/* -*-c-*- */
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/*
 *  FScreen.c
 *    Xinerama abstraction support for window manager.
 *
 *  This module is all original code
 *  by Dmitry Yu. Bolkhovityanov <bolkhov@inp.nsk.su>
 *  Copyright 2001, Dmitry Bolkhovityanov
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 */

/*
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
 */

#include "config.h"

#include <stdio.h>
#include <ctype.h>

#include "defaults.h"
#include "fvwmlib.h"
#include "Parse.h"
#include "FScreen.h"
#include "PictureBase.h"

#ifdef HAVE_XINERAMA
# undef FSCREEN_NEED_SCREENINFO
# define FScreenHaveXinerama 1
# ifdef HAVE_SOLARIS_XINERAMA
#  define FScreenHaveSolarisXinerama 1
#  ifdef HAVE_SOLARIS_XINERAMA_H
#   include <X11/extensions/xinerama.h>
#  else
#   define FSCREEN_NEED_SOLARIS_PROTOTYPES
#  endif
#  define FSCREEN_NEED_SCREENINFO
# else
#  define FScreenHaveSolarisXinerama 0
#  include <X11/extensions/Xinerama.h>
# endif
#else
# define FSCREEN_NEED_SCREENINFO
# define FScreenHaveXinerama 0
# define FScreenHaveSolarisXinerama 0
#endif

#ifdef FSCREEN_NEED_SCREENINFO
typedef struct
{
	int   screen_number;
	short x_org;
	short y_org;
	unsigned short width;
	unsigned short height;
} XineramaScreenInfo;
#endif

#ifdef FSCREEN_NEED_SOLARIS_PROTOTYPES
/* Copied from Solaris 9's X11/extensions/xinerama.h */
Bool XineramaGetState(Display*, int);
Status XineramaGetInfo(Display*, int, XRectangle*, unsigned char*, int*);
Status XineramaGetCenterHint(Display*, int, int*, int*);
#endif

#if FScreenHaveSolarisXinerama == 0
#define XineramaGetInfo(a,b,c,d,e) 0
#endif

#if FScreenHaveXinerama
# if FScreenHaveSolarisXinerama
#  define XineramaQueryExtension(d,b,c) 1 /* Lie, for now */
#  define XineramaIsActive(d) XineramaGetState((d),0)
#  define XineramaQueryScreens(d,s) (*(s)) = 0, NULL
# endif
#else
# define XineramaQueryExtension(da, b, c) 0
# define XineramaIsActive(a) 0
# define XineramaQueryScreens(d,s) (*(s)) = 0, NULL
#endif

#ifndef MAXFRAMEBUFFERS
#define	MAXFRAMEBUFFERS 16
#endif


#ifdef USE_XINERAMA_EMULATION
#define FScreenXineramaEmulation 1
#else
#define FScreenXineramaEmulation 0
#endif

#if 0
#ifdef HAVE_RANDR
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>
#endif
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
static Bool                is_xinerama_enabled = DEFAULT_XINERAMA_ENABLED;
static Bool                is_sls_enabled    = False;
static Bool                have_sls_screen_list = False;
static XineramaScreenInfo *screens;
static XineramaScreenInfo *screens_xi;
static XineramaScreenInfo *screens_sls       = NULL;
/* # of Xinerama screens, *not* counting the global, 0 if disabled */
static int                 num_screens       = 0;
/* # of Xinerama screens, *not* counting the global */
static int                 total_screens     = 0;
static int                 total_screens_xi  = 0;
static int                 total_screens_sls = 1;
static int                 total_sls_width   = 1;
static int                 total_sls_height  = 1;
static int                 first_to_check    = 0;
static int                 last_to_check     = 0;
static int                 default_geometry_scr = FSCREEN_PRIMARY;
/* only to be accessed vie the set/get functions! */
static int                 primary_scr       = DEFAULT_PRIMARY_SCREEN;

#if 0
#ifdef HAVE_RANDR
static Bool                randr_disabled    = 0;
static Bool                randr_active      = 0;
static int                 randr_event_base  = -1;
static int                 randr_error_base  = -1;
#endif
#endif

static Window blank_w, vert_w, blank2_w, blank3_w;

static int FScreenParseScreenBit(char *arg, char default_screen);
static int FindScreenOfXY(int x, int y);

static XineramaScreenInfo *
solaris_XineramaQueryScreens(Display *d, int *nscreens)
{
	if (FScreenHaveSolarisXinerama)
	{
		XineramaScreenInfo *screens = NULL;
		XRectangle monitors[MAXFRAMEBUFFERS];
		unsigned char hints[16];
		int result;

		/* dummy instructions to keep -Wall happy */
		hints[0] = hints[0];
		result = XineramaGetInfo(
			d, DefaultScreen(d), monitors, hints, nscreens);
		if (result)
		{
			int m;

			/* Note, malloced area later freed by XFree() */
			screens = (XineramaScreenInfo *)malloc(
				sizeof(XineramaScreenInfo) * (*nscreens));
			for (m = 0; m < *nscreens; ++m)
			{
				screens[m].screen_number = m;
				screens[m].x_org = monitors[m].x;
				screens[m].y_org = monitors[m].y;
				screens[m].width = monitors[m].width;
				screens[m].height = monitors[m].height;
			}
		}
		else
		{
			fprintf(
				stderr,
				"Error getting Xinerama information\n");
			*nscreens = 0;
		}

		return screens;
	}
	else
	{
		return NULL;
	}
}

static XineramaScreenInfo *FXineramaQueryScreens(Display *d, int *nscreens)
{
	if (FScreenHaveXinerama == 0)
	{
		*nscreens = 0;

		return NULL;
	}
	if (FScreenHaveSolarisXinerama == 1)
	{
		return solaris_XineramaQueryScreens(d, nscreens);
	}
	else
	{
		return XineramaQueryScreens(d, nscreens);
	}
}

static void GetMouseXY(XEvent *eventp, int *x, int *y)
{
	if (!is_xinerama_enabled || last_to_check == first_to_check)
	{
		/* We use .x_org,.y_org because nothing prevents a screen to be
		 * not at (0,0) */
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
		fev_get_evpos_or_query(
			disp, DefaultRootWindow(disp), eventp, x, y);
	}

	return;
}

Bool FScreenIsEnabled(void)
{
	return (!is_xinerama_enabled || num_screens == 0) ? False : True;
}

Bool FScreenIsSLSEnabled(void)
{
	return is_sls_enabled;
}

static void FScreenUpdateEmulationMapState(void)
{
	static Bool is_mapped = False;

	if (!FScreenXineramaEmulation)
	{
		return;
	}

	if (is_xinerama_enabled && !is_sls_enabled)
	{
		if (!is_mapped)
		{
			XMapRaised(disp, blank_w);
			XMapRaised(disp, blank2_w);
			XMapRaised(disp, blank3_w);
			XMapRaised(disp, vert_w);
			is_mapped = True;
		}
	}
	else
	{
		if (is_mapped)
		{
			XUnmapWindow(disp, blank_w);
			XUnmapWindow(disp, blank2_w);
			XUnmapWindow(disp, blank3_w);
			XUnmapWindow(disp, vert_w);
			is_mapped = False;
		}
	}
}

static void FScreenSetState(Bool do_enable)
{
	is_xinerama_enabled = do_enable;
	if (do_enable && total_screens > 0)
	{
		num_screens = total_screens;
		first_to_check = 1;
		last_to_check  = total_screens;
	}
	else
	{
		num_screens = 0;
		first_to_check = 0;
		last_to_check = 0;
	}
	FScreenUpdateEmulationMapState();
}


void FScreenInit(Display *dpy)
{
	static Bool is_initialised = False;
	int dummy_rc;

	SUPPRESS_UNUSED_VAR_WARNING(dummy_rc);
	if (is_initialised)
	{
		return;
	}
	is_initialised = True;
	disp = dpy;
	if (FScreenXineramaEmulation)
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
		total_screens_xi = count;
		screens_xi = xmalloc(sizeof(XineramaScreenInfo) * (1 + count));
		/* calculate the faked sub screen dimensions */
		w = DisplayWidth(disp, scr);
		ws = 3 * w / 5;
		h = DisplayHeight(disp, scr);
		screens_xi[1].screen_number = 0;
		screens_xi[1].x_org         = 0;
		screens_xi[1].y_org         = h / 16;
		screens_xi[1].width         = ws;
		screens_xi[1].height        = 7 * h / 8;
		screens_xi[2].screen_number = 1;
		screens_xi[2].x_org         = ws;
		screens_xi[2].y_org         = 0;
		screens_xi[2].width         = w - ws;
		screens_xi[2].height        = 7 * h / 8;
		/* add delimiter */
		attributes.background_pixel = PictureWhitePixel();
		attributes.override_redirect = True;
		blank_w = XCreateWindow(
			disp, root, 0, screens_xi[1].y_org - 1,
			screens_xi[1].width, 2, 0, CopyFromParent,
			CopyFromParent, CopyFromParent,
			CWBackPixel|CWOverrideRedirect, &attributes);
		blank2_w = XCreateWindow(
			disp, root, 0,
			screens_xi[1].y_org + screens_xi[1].height - 1,
			screens_xi[1].width, 2, 0, CopyFromParent,
			CopyFromParent, CopyFromParent,
			CWBackPixel|CWOverrideRedirect, &attributes);
		blank3_w = XCreateWindow(
			disp, root, screens_xi[2].x_org,
			screens_xi[2].height - 1, w - screens_xi[2].x_org, 2,
			0, CopyFromParent, CopyFromParent, CopyFromParent,
			CWBackPixel|CWOverrideRedirect, &attributes);
		vert_w = XCreateWindow(
			disp, root, screens_xi[2].x_org - 1, 0, 2, h, 0,
			CopyFromParent, CopyFromParent, CopyFromParent,
			CWBackPixel|CWOverrideRedirect, &attributes);
	}
	else if (FScreenHaveXinerama &&
		 XineramaQueryExtension(disp, &dummy_rc, &dummy_rc) &&
		 XineramaIsActive(disp))
	{
		int count;
		XineramaScreenInfo *info;

		info = FXineramaQueryScreens(disp, &count);
		total_screens_xi = count;
		screens_xi = xmalloc(sizeof(XineramaScreenInfo) * (1 + count));
		memcpy(screens_xi + 1, info,
		       sizeof(XineramaScreenInfo) * count);
		XFree(info);
	}
	else
	{
		total_screens_xi = 0;
		screens_xi = xmalloc(sizeof(XineramaScreenInfo) * 1);
	}
	total_screens = total_screens_xi;
	screens = screens_xi;

	/* Now, fill screens[0] with global screen parameters */
	screens_xi[0].screen_number = -1;
	screens_xi[0].x_org         = 0;
	screens_xi[0].y_org         = 0;
	screens_xi[0].width         = DisplayWidth (disp, DefaultScreen(disp));
	screens_xi[0].height        = DisplayHeight(disp, DefaultScreen(disp));

	/* Fill in the screen range */
	FScreenSetState(is_xinerama_enabled);

	return;
}

void FScreenOnOff(Bool do_enable)
{
	FScreenSetState(do_enable);
}

void FScreenSLSOnOff(Bool do_enable)
{
	is_sls_enabled = do_enable;
	if (do_enable)
	{
		total_screens = total_screens_sls;
		if (!screens_sls)
		{
			/* Sls not configured yet, use whole screen by default
			 */
			FScreenConfigureSLSSize(1, 1);
		}
		screens = screens_sls;
	}
	else
	{
		total_screens = total_screens_xi;
		screens = screens_xi;
	}
	FScreenSetState(is_xinerama_enabled);
}

Bool FScreenConfigureSLSSize(int width, int height)
{
	unsigned long scr = DefaultScreen(disp);
	int w = DisplayWidth(disp, scr);
	int h = DisplayHeight(disp, scr);

	if (width <= 1)
	{
		width = 1;
	}
	else if (width > w)
	{
		width = w;
	}
	if (height <= 1)
	{
		height = 1;
	}
	else if (height > h)
	{
		height = h;
	}
	if (total_sls_width == width && total_sls_height == height &&
	    screens_sls)
	{
		return False;
	}
	if (screens_sls)
	{
		free(screens_sls);
		screens_sls = NULL;
	}
	/* calculate the screens */
	{
		int row, col, sn;
		int ws;
		int hs;

		total_screens_sls = width * height;
		total_sls_width = width;
		total_sls_height = height;
		ws = w / total_sls_width;
		hs = h / total_sls_height;
		screens_sls = xmalloc(sizeof(XineramaScreenInfo) *
				(1 + total_screens_sls));
		/* calculate the faked sub screen dimensions */
		screens_sls[0] = screens_xi[0];
		sn = 1;
		for (row = 0; row < total_sls_height; row++)
		{
			for (col = 0; col < total_sls_width; col++, sn++)
			{
				screens_sls[sn].screen_number = sn - 1;
				screens_sls[sn].x_org         = col * ws;
				screens_sls[sn].y_org         = row * hs;
				screens_sls[sn].width         = ws;
				screens_sls[sn].height        = hs;
			}
		}
	}
	have_sls_screen_list = False;
	FScreenSLSOnOff(is_sls_enabled);

	return True;
}

Bool FScreenConfigureSLSScreens(int nscreens, char *args)
{
	int sn;
	char *next;

	if (nscreens == 0 || args == NULL)
	{
		return FScreenConfigureSLSSize(1, 1);
	}
	if (nscreens == 1 && screens_sls)
	{
		return False;
	}
	if (screens_sls)
	{
		free(screens_sls);
		screens_sls = NULL;
	}
	screens_sls = xmalloc(sizeof(XineramaScreenInfo) * (nscreens + 1));
	screens_sls[0] = screens_xi[0];
	for (sn = 1; sn <= nscreens; sn++, args = next)
	{
		char *token;
		int val[4];

		/* read next screen spec */
		token = PeekToken(args, &next);
		if (!token)
		{
			break;
		}
		if (XParseGeometry(token, &val[0], &val[1],
				   (unsigned int *)&val[2],
				   (unsigned int *)&val[3]) ==
		    (XValue|YValue|WidthValue|HeightValue) ||
		    GetIntegerArguments(args, &next, val, 4) == 4)
		{
			if (val[0] < 0 || val[1] < 0 || val[2] < 1 ||
			    val[3] < 1)
			{
				/* illegal spec */
				break;
			}
			screens_sls[sn].screen_number = sn - 1;
			screens_sls[sn].x_org = val[0];
			screens_sls[sn].y_org = val[1];
			screens_sls[sn].width = val[2];
			screens_sls[sn].height = val[3];
		}
		else
		{
			/* illegal spec */
			break;
		}
	}
	total_screens_sls = sn - 1;
	have_sls_screen_list = True;
	FScreenSLSOnOff(is_sls_enabled);

	return True;
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

	return;
}
#endif

static int FScreenGetPrimaryScreen(XEvent *ev)
{
	if (!is_xinerama_enabled)
	{
		return 0;
	}
	if (primary_scr == FSCREEN_GLOBAL)
	{
		return 0;
	}
	else if (primary_scr == FSCREEN_CURRENT)
	{
		int mx;
		int my;

		/* use current screen as primary screen */
		GetMouseXY(ev, &mx, &my);
		return FindScreenOfXY(mx, my);
	}
	else if (primary_scr < 0 || primary_scr >= last_to_check)
	{
		/* out of range */
		return 0;
	}

	return primary_scr + 1;
}

void FScreenSetPrimaryScreen(int scr)
{
	primary_scr = scr;
}

/* Intended to be called by modules.  Simply pass in the parameter from the
 * config string sent by fvwm. */
void FScreenConfigureModule(char *args)
{
	int val[6];
	int n;
	char *next;

	n = GetIntegerArguments(args, &next, val, 4);
	if (n != 4)
	{
		/* ignore broken line */
		return;
	}
	FScreenSetPrimaryScreen(val[1]);

	if (val[3])
	{
		/* SLS screen coordinates follow */
		n = GetIntegerArguments(next, &next, val + 4, 1);
		if (n != 1)
		{
			/* ignore broken line */
			return;
		}
		FScreenConfigureSLSScreens(val[4], next);
	}
	else
	{
		/* simple SLS line */
		n = GetIntegerArguments(next, NULL, val + 4, 2);
		if (n != 2)
		{
			/* ignore broken line */
			return;
		}
		FScreenConfigureSLSSize(val[4], val[5]);
	}

	FScreenSLSOnOff(val[2]);
	FScreenOnOff(val[0]);

	return;
}

/* Here's the function used by fvwm to generate the string which
 * FScreenConfigureModule expects to receive back as its argument.
 */
const char *FScreenGetConfiguration(void)
{
	int i;
	int l;
	int l2;
	static char msg[MAX_MODULE_INPUT_TEXT_LEN];
	char buf[64];

	sprintf(
		msg, XINERAMA_CONFIG_STRING" %d %d %d %d",
		FScreenIsEnabled(), primary_scr,
		FScreenIsSLSEnabled(), have_sls_screen_list);
	l = strlen(msg);
	if (have_sls_screen_list)
	{
		sprintf(msg + l, " %d", total_screens_sls);
		for (i = 0; i < total_screens_sls; i++)
		{
			sprintf(buf, " %d %d %d %d", screens_sls[i].x_org,
				screens_sls[i].y_org, screens_sls[i].width,
				screens_sls[i].height);
			l2 = strlen(buf);
			if (l + l2 > MAX_MODULE_INPUT_TEXT_LEN)
			{
				break;
			}
			strcat(msg + l, buf);
			l += l2;
		}
	}
	else
	{
		sprintf(msg + l, " %d %d", total_sls_width, total_sls_height);
	}

	return msg;
}

/* Sets the default screen for ...ParseGeometry if no screen spec is given.
 * Usually this is FSCREEN_SPEC_PRIMARY, but this won't allow modules to appear
 * under the pointer. */
void FScreenSetDefaultModuleScreen(char *scr_spec)
{
	default_geometry_scr =
		FScreenParseScreenBit(scr_spec, FSCREEN_SPEC_PRIMARY);

	return;
}


static int FindScreenOfXY(int x, int y)
{
	int i;

	x = x % screens_xi[0].width;
	while (x < 0)
	{
		x += screens_xi[0].width;
	}
	y = y % screens_xi[0].height;
	while (y < 0)
	{
		y += screens_xi[0].height;
	}
	for (i = first_to_check;  i <= last_to_check;  i++)
	{
		if (x >= screens[i].x_org  &&
		    x < screens[i].x_org + screens[i].width  &&
		    y >= screens[i].y_org  &&
		    y < screens[i].y_org + screens[i].height)
		{
			return i;
		}
	}

	/* Ouch!  A "black hole" coords?  As for now, return global screen */
	return 0;
}

static int FindScreen(
	fscreen_scr_arg *arg, fscreen_scr_t screen)
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
		screen =
			FScreenGetPrimaryScreen(
				(arg && arg->mouse_ev) ? arg->mouse_ev : NULL);
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
		/* screen is given counting from 0; translate to counting from
		 * 1 */
		screen++;
		break;
	}

	return screen;
}

/* Given pointer coordinates, return the screen the pointer is on.
 *
 * Perhaps most useful with $[pointer.screen]
 */
int FScreenOfPointerXY(int x, int y)
{
	int pscreen;

	pscreen = FindScreenOfXY(x, y);

	return (pscreen > 0) ? --pscreen : pscreen;
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
	fscreen_scr_arg *arg, fscreen_scr_t screen, int *x, int *y,
	int *w, int *h)
{
	screen = FindScreen(arg, screen);
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

/* returns the screen id */
Bool FScreenGetScrId(
	fscreen_scr_arg *arg, fscreen_scr_t screen)
{
	screen = FindScreen(arg, screen);
	if (screen < 0)
	{
		screen = FSCREEN_GLOBAL;
	}

	return screen;
}

/* Translates the coodinates *x *y from the screen specified by arg_src and
 * screen_src to coordinates on the screen specified by arg_dest and
 * screen_dest. (see FScreenGetScrRect for more details). */
void FScreenTranslateCoordinates(
	fscreen_scr_arg *arg_src, fscreen_scr_t screen_src,
	fscreen_scr_arg *arg_dest, fscreen_scr_t screen_dest,
	int *x, int *y)
{
	int x_src;
	int y_src;
	int x_dest;
	int y_dest;

	FScreenGetScrRect(arg_src, screen_src, &x_src, &y_src, NULL, NULL);
	FScreenGetScrRect(arg_dest, screen_dest, &x_dest, &y_dest, NULL, NULL);

	if (x)
	{
		*x = *x + x_src - x_dest;
	}
	if (y)
	{
		*y = *y + y_src - y_dest;
	}

	return;
}

/* Arguments work exactly like for FScreenGetScrRect() */
int FScreenClipToScreen(
	fscreen_scr_arg *arg, fscreen_scr_t screen, int *x, int *y, int w,
	int h)
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
	fscreen_scr_arg *arg, fscreen_scr_t screen, int *x, int *y, int w,
	int h)
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

void FScreenGetResistanceRect(
	int wx, int wy, unsigned int ww, unsigned int wh, int *x0, int *y0,
	int *x1, int *y1)
{
	fscreen_scr_arg arg;

	arg.xypos.x = wx + ww / 2;
	arg.xypos.y = wy + wh / 2;
	FScreenGetScrRect(&arg, FSCREEN_XYPOS, x0, y0, x1, y1);
	*x1 += *x0;
	*y1 += *y0;

	return;
}

/* Arguments work exactly like for FScreenGetScrRect() */
Bool FScreenIsRectangleOnScreen(
	fscreen_scr_arg *arg, fscreen_scr_t screen, rectangle *rec)
{
	int sx;
	int sy;
	int sw;
	int sh;

	FScreenGetScrRect(arg, screen, &sx, &sy, &sw, &sh);

	return (rec->x + rec->width > sx && rec->x < sx + sw &&
		rec->y + rec->height > sy && rec->y < sy + sh) ? True : False;
}

void FScreenSpecToString(char *dest, int space, fscreen_scr_t screen)
{
	char s[32];

	if (space <= 0)
	{
		return;
	}
	switch (screen)
	{
	case FSCREEN_GLOBAL:
		strcpy(s, "global screen");
		break;
	case FSCREEN_CURRENT:
		strcpy(s, "current screen");
		break;
	case FSCREEN_PRIMARY:
		strcpy(s, "primary screen");
		break;
	case FSCREEN_XYPOS:
		strcpy(s, "screen specified by xy");
		break;
	default:
		sprintf(s, "%d", screen);
		break;
	}
	strncpy(dest, s, space);
	dest[space - 1] = 0;

	return;
}

static int FScreenParseScreenBit(char *scr_spec, char default_screen)
{
	int scr = default_geometry_scr;
	char c;

	c = (scr_spec) ? tolower(*scr_spec) : tolower(default_screen);
	if (c == FSCREEN_SPEC_GLOBAL)
	{
		scr = FSCREEN_GLOBAL;
	}
	else if (c == FSCREEN_SPEC_CURRENT)
	{
		scr = FSCREEN_CURRENT;
	}
	else if (c == FSCREEN_SPEC_PRIMARY)
	{
		scr = FSCREEN_PRIMARY;
	}
	else if (c == FSCREEN_SPEC_WINDOW)
	{
		scr = FSCREEN_XYPOS;
	}
	else if (isdigit(c))
	{
		scr = atoi(scr_spec);
	}
	else
	{
		c = tolower(default_screen);
		if (c == FSCREEN_SPEC_GLOBAL)
		{
			scr = FSCREEN_GLOBAL;
		}
		else if (c == FSCREEN_SPEC_CURRENT)
		{
			scr = FSCREEN_CURRENT;
		}
		else if (c == FSCREEN_SPEC_PRIMARY)
		{
			scr = FSCREEN_PRIMARY;
		}
		else if (c == FSCREEN_SPEC_WINDOW)
		{
			scr = FSCREEN_XYPOS;
		}
		else if (isdigit(c))
		{
			scr = atoi(scr_spec);
		}
	}

	return scr;
}

int FScreenGetScreenArgument(char *scr_spec, fscreen_scr_spec_t default_screen)
{
	while (scr_spec && isspace(*scr_spec))
	{
		scr_spec++;
	}

	return FScreenParseScreenBit(scr_spec, default_screen);
}

/*
 * FScreenParseGeometry
 *     Does the same as XParseGeometry, but handles additional "@scr".
 *     Since it isn't safe to define "ScreenValue" constant (actual values
 *     of other "XXXValue" are specified in Xutil.h, not by us, so there can
 *     be a clash), the screen value is always returned, even if it wasn't
 *     present in `parse_string' (set to default in that case).
 *
 */
int FScreenParseGeometryWithScreen(
	char *parsestring, int *x_return, int *y_return,
	unsigned int *width_return, unsigned int *height_return,
	int *screen_return)
{
	int   ret;
	char *copy, *scr_p;
	int   s_size;
	int   scr;

	/* Safety net */
	if (parsestring == NULL  ||  *parsestring == '\0')
	{
		return 0;
	}

	/* Make a local copy devoid of "@scr" */
	s_size = strlen(parsestring) + 1;
	copy = xmalloc(s_size);
	memcpy(copy, parsestring, s_size);
	scr_p = strchr(copy, '@');
	if (scr_p != NULL)
	{
		*scr_p++ = '\0';
	}

	/* Do the parsing */
	ret = XParseGeometry(
		copy, x_return, y_return, width_return, height_return);

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
	char *parsestring, int *x_return, int *y_return,
	unsigned int *width_return, unsigned int *height_return)
{
	int scr;
	int rc;
	int mx;
	int my;

	rc = FScreenParseGeometryWithScreen(
		parsestring, x_return, y_return, width_return, height_return,
		&scr);
	if (rc == 0)
	{
		return 0;
	}
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
		scr = FScreenGetPrimaryScreen(NULL);
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
					(screens[0].width -
					 screens[scr].width -
					 screens[scr].x_org);
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
					(screens[0].height -
					 screens[scr].height -
					 screens[scr].y_org);
			}
			else
			{
				*y_return += screens[scr].y_org;
			}
		}
	}

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
	int   x, y;
	unsigned int w = 0, h = 0;
	int   grav, x_grav, y_grav;
	int   scr = default_geometry_scr;
	int   scr_x, scr_y;
	int scr_w, scr_h;

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

	/* III. Interpret and fill in the values */

	/* Fill in dimensions for future negative calculations if
	 * omitted/forbidden */
	/* Maybe should use *x_return,*y_return if hints==NULL?
	 * Unreliable... */
	if (hints != NULL  &&  hints->flags & PSize)
	{
		if ((ret & WidthValue)  == 0)
		{
			w = hints->width;
		}
		if ((ret & HeightValue) == 0)
		{
			h = hints->height;
		}
	}
	else
	{
		/* This branch is required for case when size *is* specified,
		 * but masked off */
		if ((ret & WidthValue)  == 0)
		{
			w = 0;
		}
		if ((ret & HeightValue) == 0)
		{
			h = 0;
		}
	}

	/* Advance coords to the screen... */
	x += scr_x;
	y += scr_y;

	/* ...and process negative geometries */
	if (saved & XNegative)
	{
		x += scr_w;
	}
	if (saved & YNegative)
	{
		y += scr_h;
	}
	if (ret & XNegative)
	{
		x -= w;
	}
	if (ret & YNegative)
	{
		y -= h;
	}

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
		{
			hints->x = x;
		}
	}
	if (ret & YValue)
	{
		*y_return = y;
		if (hints != NULL)
		{
			hints->y = y;
		}
	}
	if (ret & WidthValue)
	{
		*width_return = w;
		if (hints != NULL)
		{
			hints->width = w;
		}
	}
	if (ret & HeightValue)
	{
		*height_return = h;
		if (hints != NULL)
		{
			hints->height = h;
		}
	}
	if (1 /*flags & GravityValue*/  &&  grav != DEFAULT_GRAVITY)
	{
		if (hints != NULL  &&  hints->flags & PWinGravity)
		{
			hints->win_gravity = grav;
		}
	}
	if (hints != NULL  &&  ret & XValue  &&  ret & YValue)
		hints->flags |= USPosition;

	return ret;
}

/*  FScreenMangleScreenIntoUSPosHints
 *      A hack to mangle the screen number into the XSizeHints structure.
 *      If the USPosition flag is set, hints->x is set to the magic number and
 *      hints->y is set to the screen number.  If the USPosition flag is clear,
 *      x and y are set to zero.
 *
 *  Note:  This is a *hack* to allow modules to specify the target screen for
 *  their windows and have the StartsOnScreen style set for them at the same
 *  time.  Do *not* rely on the mechanism described above.
 */
void FScreenMangleScreenIntoUSPosHints(fscreen_scr_t screen, XSizeHints *hints)
{
	if (hints->flags & USPosition)
	{
		hints->x = FSCREEN_MANGLE_USPOS_HINTS_MAGIC;
		hints->y = (short)screen;
	}
	else
	{
		hints->x = 0;
		hints->y = 0;
	}

	return;
}

/*  FScreenMangleScreenIntoUSPosHints
 *      A hack to mangle the screen number into the XSizeHints structure.
 *      If the USPosition flag is set, hints->x is set to the magic number and
 *      hints->y is set to the screen spec.  If the USPosition flag is clear,
 *      x and y are set to zero.
 *
 *  Note:  This is a *hack* to allow modules to specify the target screen for
 *  their windows and have the StartsOnScreen style set for them at the same
 *  time.  Do *not* rely on the mechanism described above.
 */
fscreen_scr_t FScreenFetchMangledScreenFromUSPosHints(XSizeHints *hints)
{
	fscreen_scr_t screen;

	if ((hints->flags & USPosition) &&
	    hints->x == FSCREEN_MANGLE_USPOS_HINTS_MAGIC)
	{
		screen = (fscreen_scr_t)(hints->y);
	}
	else
	{
		screen = FSCREEN_GLOBAL;
	}

	return screen;
}


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

	return (nw != *old_w  ||  nh != *old_h);
#endif
}
#endif
