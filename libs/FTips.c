/* -*-c-*- */
/* Copyright (C) 2004  Olivier Chapuis */
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

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include "libs/ftime.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "defaults.h"
#include "fvwmlib.h"
#include "Strings.h"
#include "PictureBase.h"
#include "Flocale.h"
#include "Graphics.h"
#include "Colorset.h"
#include "libs/FScreen.h"
#include "FTips.h"

/* ---------------------------- local definitions -------------------------- */

#define FVWM_TIPS_NOTHING 0
#define FVWM_TIPS_WAITING 1
#define FVWM_TIPS_MAPPED  2

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static Window win = None;
static GC gc = None;
static Window win_for;

static ftips_config *current_config, *default_config;

static char *label;
static int state = FVWM_TIPS_NOTHING;
static void *boxID = NULL;

static FlocaleWinString fwin_string;
static rectangle box;

static unsigned long timeOut;
static unsigned long onTime;

static Atom _net_um_for = None;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static
unsigned long __get_time(void)
{
	struct timeval t;

	gettimeofday(&t, NULL);
	return (1000*t.tv_sec + t.tv_usec/1000);
}

static
void __initialize_window(Display *dpy)
{
	XGCValues xgcv;
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	Atom _net_um_window_type;
	long _net_um_window_type_tooltips;

	valuemask = CWOverrideRedirect | CWEventMask | CWColormap;
	attributes.override_redirect = True;
	attributes.event_mask = ExposureMask;
	attributes.colormap = Pcmap;
	win = XCreateWindow(
		dpy, DefaultRootWindow(dpy), 0, 0, 1, 1,
		current_config->border_width, Pdepth, InputOutput, Pvisual,
		valuemask, &attributes);

	_net_um_window_type = XInternAtom(
		dpy, "_NET_UM_WINDOW_TYPE", False);
	_net_um_window_type_tooltips = XInternAtom(
		dpy, "_NET_UM_WINDOW_TYPE_TOOLTIPS", False);

	XChangeProperty(
		dpy, win, _net_um_window_type, XA_ATOM, 32, PropModeReplace,
		(unsigned char *) &_net_um_window_type_tooltips, 1);

	_net_um_for = XInternAtom(dpy, "_NET_UM_FOR", False);

	gc = fvwmlib_XCreateGC(dpy, win, 0, &xgcv);
	return;
}

static
void __setup_cs(Display *dpy)
{
	XSetWindowAttributes xswa;
	unsigned long valuemask = 0;


	if (current_config->colorset > -1)
	{
		xswa.border_pixel = Colorset[current_config->colorset].fg;
		xswa.background_pixel = Colorset[current_config->colorset].bg;
		if (Colorset[current_config->colorset].pixmap)
		{
			/* set later */
			xswa.background_pixmap = None;
			valuemask = CWBackPixmap | CWBorderPixel;;
		}
		else
		{
			valuemask = CWBackPixel | CWBorderPixel;
		}
	}
	else
	{
		xswa.border_pixel = current_config->border_pixel;
		xswa.background_pixel = current_config->bg;
		valuemask = CWBackPixel | CWBorderPixel;
	}
	XChangeWindowAttributes(dpy, win, valuemask, &xswa);
}

static
void __setup_gc(Display *dpy)
{
	XGCValues xgcv;
	unsigned long valuemask;

	valuemask = GCForeground;
	if (current_config->colorset > -1)
	{
		xgcv.foreground = Colorset[current_config->colorset].fg;
	}
	else
	{
		xgcv.foreground = current_config->fg;
	}
	if (current_config->Ffont && current_config->Ffont->font != NULL)
	{
		xgcv.font = current_config->Ffont->font->fid;
		valuemask |= GCFont;
	}
	XChangeGC(dpy, gc, valuemask, &xgcv);

	return;
}

static
void __draw(Display *dpy)
{
	if (!current_config->Ffont)
	{
		return;
	}

	fwin_string.str = label;
	fwin_string.win = win;
	fwin_string.gc = gc;
	if (current_config->colorset > -1)
	{
		fwin_string.colorset = &Colorset[current_config->colorset];
		fwin_string.flags.has_colorset = True;
	}
	else
	{
		fwin_string.flags.has_colorset = False;
	}
	fwin_string.x = 2;
	fwin_string.y = current_config->Ffont->ascent;
	FlocaleDrawString(dpy, current_config->Ffont, &fwin_string, 0);

	return;
}

static
void __map_window(Display *dpy)
{
	rectangle new_g;
	rectangle screen_g;
	Window dummy;
	fscreen_scr_arg *fsarg = NULL; /* for now no xinerama support */
	ftips_placement_t placement;
	int x,y;
	static int border_width = 1;
	static Window win_f = None;

	if (border_width != current_config->border_width)
	{
		XSetWindowBorderWidth(dpy, win, current_config->border_width);
		border_width = current_config->border_width;
	}

	FScreenGetScrRect(
		fsarg,  FSCREEN_GLOBAL, &screen_g.x, &screen_g.y,
		&screen_g.width, &screen_g.height);

	new_g.height = ((current_config->Ffont)? current_config->Ffont->height:0)
		+ 1;
	new_g.width = 4;

	if (label && current_config->Ffont)
	{
		new_g.width += FlocaleTextWidth(
			current_config->Ffont, label, strlen(label));
	}

	if (current_config->placement != FTIPS_PLACEMENT_AUTO_UPDOWN &&
	    current_config->placement != FTIPS_PLACEMENT_AUTO_LEFTRIGHT)
	{
		placement = current_config->placement;
	}
	else
	{
		XTranslateCoordinates(
			dpy, win_for, DefaultRootWindow(dpy), box.x, box.y,
			&x, &y, &dummy);
		if (current_config->placement == FTIPS_PLACEMENT_AUTO_UPDOWN)
		{
			if (y + box.height/2 >= screen_g.height/2)
			{
				placement = FTIPS_PLACEMENT_UP;
			}
			else
			{
				placement = FTIPS_PLACEMENT_DOWN;
			}
		}
		else
		{
			if (x + box.width/2 >= screen_g.width/2)
			{
				placement = FTIPS_PLACEMENT_LEFT;
			}
			else
			{
				placement = FTIPS_PLACEMENT_RIGHT;
			}
		}
	}

	if (placement == FTIPS_PLACEMENT_RIGHT ||
	    placement == FTIPS_PLACEMENT_LEFT)
	{
		if (current_config->justification == FTIPS_JUSTIFICATION_CENTER)
		{
			y = box.y + (box.height / 2) - (new_g.height / 2) -
				current_config->border_width;
		}
		else if (current_config->justification ==
			 FTIPS_JUSTIFICATION_RIGHT_DOWN)
		{
			y = box.y + box.height  - new_g.height -
				(2 * current_config->border_width) -
				current_config->justification_offset;
		}
		else /* LEFT_UP */
		{
			y = box.y + current_config->justification_offset;

		}
	}
	else /* placement == FTIPS_PLACEMENT_DOWN ||
	       placement == FTIPS_PLACEMENT_UP */
	{
		if (current_config->justification == FTIPS_JUSTIFICATION_CENTER)
		{
			x = box.x + (box.width / 2) - (new_g.width / 2) -
				current_config->border_width;
		}
		else if (current_config->justification ==
			 FTIPS_JUSTIFICATION_RIGHT_DOWN)
		{
			x = box.x + box.width  - new_g.width -
				(2 * current_config->border_width) -
				current_config->justification_offset;
		}
		else /* LEFT_UP */
		{
			x = box.x + current_config->justification_offset;
		}
	}

	if (placement == FTIPS_PLACEMENT_RIGHT)
	{
		x = box.x + box.width + current_config->placement_offset + 1;
	}
	else if (placement == FTIPS_PLACEMENT_LEFT)
	{
		x = box.x - current_config->placement_offset - new_g.width
			- (2 * current_config->border_width) - 1;
	}
	else if (placement == FTIPS_PLACEMENT_DOWN)
	{
		y = box.y + box.height + current_config->placement_offset - 0;
	}
	else  /* UP */
	{
		y = box.y - current_config->placement_offset - new_g.height
			+ 0 - (2 * current_config->border_width);
	}

	XTranslateCoordinates(
		dpy, win_for, DefaultRootWindow(dpy), x, y,
		&new_g.x, &new_g.y, &dummy);

	if (placement == FTIPS_PLACEMENT_RIGHT ||
	    placement == FTIPS_PLACEMENT_LEFT)
	{
		int x1,y1,l1,l2;

		if (new_g.x < 2)
		{
			x = box.x + box.width + current_config->placement_offset
				+ 1;
			XTranslateCoordinates(
				dpy, win_for, DefaultRootWindow(dpy), x, y,
				&x1, &y1, &dummy);
			/* */
			l1 = new_g.width + new_g.x - 2;
			l2 = screen_g.width - (x1 + new_g.width) -
				current_config->border_width - 2;
			if (l2 > l1)
			{
				new_g.x = x1;
			}
		}
		else if (new_g.x + new_g.width >
			 screen_g.width - (2 * current_config->border_width) - 2)
		{
			x = box.x - current_config->placement_offset - new_g.width
				- (2 * current_config->border_width) - 1;
			XTranslateCoordinates(
				dpy, win_for, DefaultRootWindow(dpy), x, y,
				&x1, &y1, &dummy);
			/* */
			l1 = new_g.width + x1 - 2;
			l2 = screen_g.width - (new_g.x + new_g.width) -
				(2 * current_config->border_width) - 2;
			if (l1 > l2)
			{
				new_g.x = x1;
			}
		}
		if ( new_g.y < 2 )
		{
			new_g.y = 2;
		}
		else if (new_g.y + new_g.height >
			 screen_g.height - (2 * current_config->border_width) - 2)
		{
			new_g.y = screen_g.height - new_g.height -
				(2 * current_config->border_width) - 2;
		}
	}
	else /* placement == FTIPS_PLACEMENT_DOWN ||
		placement == FTIPS_PLACEMENT_UP */
	{
		if (new_g.y < 2)
		{
			y = box.y + box.height +
				current_config->placement_offset - 0;
			XTranslateCoordinates(
				dpy, win_for, DefaultRootWindow(dpy),
				x, y, &new_g.x, &new_g.y, &dummy);
		}
		else if (new_g.y + new_g.height >
			 screen_g.height - (2 * current_config->border_width) - 2)
		{
			y = box.y - current_config->placement_offset -
				new_g.height + 0
				- (2 * current_config->border_width);
			XTranslateCoordinates(
				dpy, win_for, DefaultRootWindow(dpy),
				x, y, &new_g.x, &new_g.y, &dummy);
		}
		if ( new_g.x < 2 )
		{
			new_g.x = 2;
		}
		else if (new_g.x + new_g.width >
			 screen_g.width - (2 * current_config->border_width) - 2)
		{
			new_g.x = screen_g.width - new_g.width -
				(2 * current_config->border_width) - 2;
		}
	}

	/* make changes to window */
	XMoveResizeWindow(
		dpy, win, new_g.x, new_g.y, new_g.width, new_g.height);
	__setup_gc(dpy);
	if (current_config->colorset > -1)
	{
		SetWindowBackground(
			dpy, win, new_g.width, new_g.height,
			&Colorset[current_config->colorset], Pdepth, gc, True);
	}
	else
	{
		XSetWindowBackground (dpy, win, current_config->bg);
	}
	if (current_config->border_width > 0)
	{
		XSetWindowBorder(
			dpy, win, Colorset[current_config->colorset].fg);
	}

	if (state != FVWM_TIPS_MAPPED && win_f != win_for)
	{
		long l_win_for;

		l_win_for = win_for;
		XChangeProperty(
			dpy, win, _net_um_for, XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &l_win_for, 1);
		win_f = win_for;
	}
	XMapRaised(dpy, win);
	state = FVWM_TIPS_MAPPED;

	return;
}

/* ---------------------------- interface functions ------------------------ */

Bool FTipsInit(Display *dpy)
{

	current_config = default_config = FTipsNewConfig();
	__initialize_window(dpy);

	if (gc == None || win == None)
	{
		return False;
	}

	__setup_cs(dpy);
	__setup_gc(dpy);

	memset(&fwin_string, 0, sizeof(fwin_string));

	return True;
}

ftips_config *FTipsNewConfig(void)
{
	ftips_config *fc;

	fc = xcalloc(1, sizeof(ftips_config));

	/* use colorset 0 as default */
	fc->border_width = FTIPS_DEFAULT_BORDER_WIDTH;
	fc->placement = FTIPS_DEFAULT_PLACEMENT;
	fc->justification = FTIPS_DEFAULT_JUSTIFICATION;
	fc->placement_offset = FTIPS_DEFAULT_PLACEMENT_OFFSET;
	fc->justification_offset = FTIPS_DEFAULT_JUSTIFICATION_OFFSET;
	fc->delay = 1000;
	fc->mapped_delay = 300;

	return fc;
}

void FTipsOn(
	Display *dpy, Window win_f, ftips_config *fc, void *id, char *str,
	int x, int y, int w, int h)
{
	unsigned long delay;

	box.x = x;
	box.y = y;
	box.width = w;
	box.height = h;

	if (fc == NULL)
	{
		fc = default_config;
	}
	current_config = fc;

	if (label != NULL)
	{
		free(label);
	}
	CopyString(&label, str);
	win_for = win_f;

	if (id == boxID)
	{
		if (id && state == FVWM_TIPS_WAITING)
		{
			FTipsCheck(dpy);
		}
		return;
	}
	onTime = __get_time();
	if (state == FVWM_TIPS_MAPPED)
	{
		FTipsCancel(dpy);
		delay = fc->mapped_delay;
	}
	else
	{
		delay = fc->delay;
	}
	boxID = id;
	timeOut = onTime + delay;
	state = FVWM_TIPS_WAITING;
	if (delay == 0)
	{
		FTipsCheck(dpy);
	}

	return;
}

void FTipsCancel(Display *dpy)
{
	if (state == FVWM_TIPS_MAPPED && win != None)
	{
		XUnmapWindow(dpy, win);
	}
	boxID = 0;
	state = FVWM_TIPS_NOTHING;
}

unsigned long FTipsCheck(Display *dpy)
{
	unsigned long ct;

	if (state != FVWM_TIPS_WAITING || win == None)
	{
		return 0;
	}

	ct = __get_time();

	if (ct >= timeOut)
	{
		__map_window(dpy);
		XFlush(dpy);
		state = FVWM_TIPS_MAPPED;
		return 0;
	}
	else
	{
		XFlush(dpy);
		return timeOut - ct;
	}
}

Bool FTipsExpose(Display *dpy, XEvent *ev)
{
	int ex,ey,ex2,ey2;

	if (win == None || ev->xany.window != win)
	{
		return False;
	}
	ex = ev->xexpose.x;
	ey = ev->xexpose.y;
	ex2 = ev->xexpose.x + ev->xexpose.width;
	ey2= ev->xexpose.y + ev->xexpose.height;

	while (FCheckTypedWindowEvent(dpy, ev->xany.window, Expose, ev))
	{
		ex = min(ex, ev->xexpose.x);
		ey = min(ey, ev->xexpose.y);
		ex2 = max(ex2, ev->xexpose.x + ev->xexpose.width);
		ey2= max(ey2 , ev->xexpose.y + ev->xexpose.height);
	}
#if 0
	fprintf (
		stderr, "\tExpose: %i,%i,%i,%i %i\n",
		ex, ey, ex2-ex, ey2-ey, ev->xexpose.count);
#endif
	__draw(dpy);

	return True;
}

Bool FTipsHandleEvents(Display *dpy, XEvent *ev)
{
	if (ev->xany.window != win)
	{
		return False;
	}

	switch(ev->type)
	{
	case Expose:
		FTipsExpose(dpy, ev);
		break;
	default:
		break;
	}
	return True;
}

void FTipsUpdateLabel(Display *dpy, char *str)
{
	if (state != FVWM_TIPS_MAPPED && state != FVWM_TIPS_WAITING)
	{
		return;
	}

	if (label)
	{
		free(label);
	}
	CopyString(&label, str);
	if (state == FVWM_TIPS_MAPPED)
	{
		__map_window(dpy);
		__draw(dpy);
	}
}

void FTipsColorsetChanged(Display *dpy, int cs)
{
	if (state != FVWM_TIPS_MAPPED || cs != current_config->colorset)
	{
		return;
	}
	__map_window(dpy);
	__draw(dpy);
}
