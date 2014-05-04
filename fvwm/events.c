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
/* This module is based on Twm, but has been siginificantly modified
 * by Rob Nation
 */
/*
 *       Copyright 1988 by Evans & Sutherland Computer Corporation,
 *                          Salt Lake City, Utah
 *  Portions Copyright 1989 by the Massachusetts Institute of Technology
 *                        Cambridge, Massachusetts
 *
 *                           All Rights Reserved
 *
 *    Permission to use, copy, modify, and distribute this software and
 *    its documentation  for  any  purpose  and  without  fee is hereby
 *    granted, provided that the above copyright notice appear  in  all
 *    copies and that both  that  copyright  notice  and  this  permis-
 *    sion  notice appear in supporting  documentation,  and  that  the
 *    names of Evans & Sutherland and M.I.T. not be used in advertising
 *    in publicity pertaining to distribution of the  software  without
 *    specific, written prior permission.
 *
 *    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD
 *    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-
 *    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR
 *    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-
 *    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA
 *    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE
 *    OR PERFORMANCE OF THIS SOFTWARE.
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <X11/Xatom.h>

#include "libs/ftime.h"
#include "libs/fvwmlib.h"
#include "libs/System.h"
#include "libs/Grab.h"
#include "libs/Parse.h"
#include "libs/ColorUtils.h"
#include "libs/FShape.h"
#include "libs/PictureBase.h"
#include "libs/Colorset.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "commands.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "events.h"
#include "eventhandler.h"
#include "eventmask.h"
#include "libs/fvwmsignal.h"
#include "module_list.h"
#include "module_interface.h"
#include "session.h"
#include "borders.h"
#include "frame.h"
#include "add_window.h"
#include "icccm2.h"
#include "icons.h"
#include "ewmh.h"
#include "update.h"
#include "style.h"
#include "stack.h"
#include "geometry.h"
#include "focus.h"
#include "virtual.h"
#include "decorations.h"
#include "schedule.h"
#include "menus.h"
#include "colormaps.h"
#include "colorset.h"
#ifdef HAVE_STROKE
#include "stroke.h"
#endif /* HAVE_STROKE */

/* ---------------------------- local definitions -------------------------- */

#ifndef XUrgencyHint
#define XUrgencyHint            (1L << 8)
#endif

#define CR_MOVERESIZE_MASK (CWX | CWY | CWWidth | CWHeight | CWBorderWidth)

#define DEBUG_GLOBALLY_ACTIVE 1

#define MAX_NUM_WEED_EVENT_TYPES 40

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

extern void StartupStuff(void);

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef void (*PFEH)(const evh_args_t *ea);

typedef struct
{
	Window w;
	Bool do_return_true;
	Bool do_return_true_cr;
	unsigned long cr_value_mask;
	Bool ret_does_match;
	unsigned long ret_type;
} check_if_event_args;

typedef struct
{
	unsigned do_forbid_function : 1;
	unsigned do_focus : 1;
	unsigned do_swallow_click : 1;
	unsigned do_raise : 1;
} hfrc_ret_t;

typedef struct event_group
{
	int base;
	int count;
	PFEH *jump_table;
	struct event_group *next;
} event_group_t;

typedef struct
{
	int num_event_types;
	int event_types[MAX_NUM_WEED_EVENT_TYPES];
} _weed_event_type_arg;

typedef struct
{
	long event_mask;
} _weed_window_mask_events_arg;

typedef struct
{
	Window w;
	XEvent *last_cr_event;
	int count;
} _merge_cr_args;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static int Button = 0;
static const FvwmWindow *xcrossing_last_grab_window = NULL;
STROKE_CODE(static int send_motion);
STROKE_CODE(static char sequence[STROKE_MAX_SEQUENCE + 1]);
static event_group_t *base_event_group = NULL;

/* ---------------------------- exported variables (globals) --------------- */

int last_event_type = 0;
Window PressedW = None;

/* ---------------------------- local functions ---------------------------- */

static void fake_map_unmap_notify(const FvwmWindow *fw, int event_type)
{
	XEvent client_event;
	XWindowAttributes winattrs = {0};

	if (!XGetWindowAttributes(dpy, FW_W(fw), &winattrs))
	{
		return;
	}
	XSelectInput(
		dpy, FW_W(fw),
		winattrs.your_event_mask & ~StructureNotifyMask);
	client_event.type = event_type;
	client_event.xmap.display = dpy;
	client_event.xmap.event = FW_W(fw);
	client_event.xmap.window = FW_W(fw);
	switch (event_type)
	{
	case MapNotify:
		client_event.xmap.override_redirect = False;
		break;
	case UnmapNotify:
		client_event.xunmap.from_configure = False;
		break;
	default:
		/* not possible if called correctly */
		break;
	}
	FSendEvent(
		dpy, FW_W(fw), False, StructureNotifyMask, &client_event);
	XSelectInput(dpy, FW_W(fw), winattrs.your_event_mask);
	XFlush(dpy);

	return;
}

static int _pred_weed_accumulate_expose(
	Display *display, XEvent *ev, XPointer arg)
{
	XEvent *em = (XEvent *)arg;

	if (ev->type != Expose)
	{
		return 0;
	}
	{
		int x0;
		int x1;

		x1 = max(
			ev->xexpose.x + ev->xexpose.width,
			em->xexpose.x + em->xexpose.width);
		x0 = min(em->xexpose.x, ev->xexpose.x);
		em->xexpose.x = x0;
		em->xexpose.width = x1 - x0;
	}
	{
		int y0;
		int y1;

		y1 = max(
			ev->xexpose.y + ev->xexpose.height,
			em->xexpose.y + em->xexpose.height);
		y0 = min(em->xexpose.y, ev->xexpose.y);
		em->xexpose.y = y0;
		em->xexpose.height = y1 - y0;
	}

	return 1;
}

static int _pred_weed_handle_expose(
	Display *display, XEvent *event, XPointer arg)
{
	if (event->type == Expose)
	{
		dispatch_event(event);
		return 1;
	}
	else
	{
		return 0;
	}
}

static int _pred_weed_event_type(
	Display *display, XEvent *event, XPointer arg)
{
	_weed_event_type_arg *args = (_weed_event_type_arg *)arg;
	int i;

	for (i = 0; i < args->num_event_types; i++)
	{
		if (event->type == args->event_types[i])
		{
			/* invalidate event and continue weeding */
			return 1;
		}
	}

	/* keep event and continue weeding */
	return 0;
}

static int _pred_flush_property_notify_weed(
	Display *display, XEvent *event, XPointer arg)
{
	flush_property_notify_args *args =
		(flush_property_notify_args *)arg;
	int does_match_window;

	does_match_window = (
		FEV_HAS_EVENT_WINDOW(event->type) &&
		event->xany.window == args->w) ? 1 : 0;
	if (
		does_match_window &&
		event->type == args->event_type &&
		event->xproperty.atom == args->atom)
	{
		/* invalidate event and continue weeding */
		return 1;
	}
	else if (
		args->do_stop_at_event_type &&
		event->type == args->stop_at_event_type && (
			!FEV_HAS_EVENT_WINDOW(args->stop_at_event_type) ||
			does_match_window))
	{
		/* keep event and stop weeding */
		return 2;
	}

	/* keep event and continue weeding */
	return 0;
}

static Bool test_resizing_event(
	Display *display, XEvent *event, XPointer arg)
{
	check_if_event_args *cie_args;
	Bool rc;

	cie_args = (check_if_event_args *)arg;
	cie_args->ret_does_match = False;
	if (event->xany.window != cie_args->w)
	{
		return False;
	}
	rc = False;
	switch (event->type)
	{
	case ConfigureRequest:
		if ((event->xconfigurerequest.value_mask &
		     cie_args->cr_value_mask) != 0)
		{
			cie_args->ret_type = ConfigureRequest;
			cie_args->ret_does_match = True;
			rc = cie_args->do_return_true_cr;
		}
		break;
	case PropertyNotify:
		if (event->xproperty.atom == XA_WM_NORMAL_HINTS)
		{
			cie_args->ret_type = PropertyNotify;
			cie_args->ret_does_match = True;
			rc = cie_args->do_return_true;
		}
	default:
		break;
	}

	/* Yes, it is correct that this function may always returns False. */
	return rc;
}

static inline void _handle_cr_on_unmanaged(XEvent *e)
{
	XConfigureRequestEvent *cre = &e->xconfigurerequest;
	XWindowChanges xwc;
	unsigned long xwcm;

	xwcm = (cre->value_mask & CR_MOVERESIZE_MASK);
	xwc.x = cre->x;
	xwc.y = cre->y;
	xwc.width = cre->width;
	xwc.height = cre->height;
	xwc.border_width = cre->border_width;
	XConfigureWindow(dpy, cre->window, xwcm, &xwc);

	return;
}

static inline void _handle_cr_on_icon(XEvent *e, FvwmWindow *fw)
{
	XConfigureRequestEvent *cre = &e->xconfigurerequest;
	XWindowChanges xwc;
	unsigned long xwcm;

	xwcm = (cre->value_mask & CR_MOVERESIZE_MASK);
	xwc.x = cre->x;
	xwc.y = cre->y;
	xwc.width = cre->width;
	xwc.height = cre->height;
	xwc.border_width = cre->border_width;
	if (FW_W_ICON_PIXMAP(fw) == cre->window)
	{
		int bw;

		if (cre->value_mask & CWBorderWidth)
		{
			fw->icon_border_width = cre->border_width;
		}
		bw = fw->icon_border_width;
		if ((cre->value_mask & (CWWidth | CWHeight)) ==
		    (CWWidth | CWHeight))
		{
			set_icon_picture_size(
				fw, cre->width + 2 * bw, cre->height + 2 * bw);
		}
	}
	set_icon_position(fw, cre->x, cre->y);
	broadcast_icon_geometry(fw, False);
	XConfigureWindow(dpy, cre->window, xwcm, &xwc);
	if (cre->window != FW_W_ICON_PIXMAP(fw) &&
	    FW_W_ICON_PIXMAP(fw) != None)
	{
		rectangle g;

		get_icon_picture_geometry(fw, &g);
		xwc.x = g.x;
		xwc.y = g.y;
		xwcm = cre->value_mask & (CWX | CWY);
		XConfigureWindow(
			dpy, FW_W_ICON_PIXMAP(fw), xwcm, &xwc);
	}
	if (FW_W_ICON_TITLE(fw) != None)
	{
		rectangle g;

		get_icon_title_geometry(fw, &g);
		xwc.x = g.x;
		xwc.y = g.y;
		xwcm = cre->value_mask & (CWX | CWY);
		XConfigureWindow(
			dpy, FW_W_ICON_TITLE(fw), xwcm, &xwc);
	}

	return;
}

static inline void _handle_cr_on_shaped(FvwmWindow *fw)
{
	/* suppress compiler warnings w/o shape extension */
	int i;
	unsigned int u;
	Bool b;
	int boundingShaped;

	SUPPRESS_UNUSED_VAR_WARNING(i);
	SUPPRESS_UNUSED_VAR_WARNING(u);
	SUPPRESS_UNUSED_VAR_WARNING(b);
	if (FShapeQueryExtents(
		    dpy, FW_W(fw), &boundingShaped, &i, &i, &u, &u, &b,
		    &i, &i, &u, &u))
	{
		fw->wShaped = boundingShaped;
	}
	else
	{
		fw->wShaped = 0;
	}

	return;
}

static inline void _handle_cr_restack(
	int *ret_do_send_event, XEvent *e, FvwmWindow *fw)
{
	XConfigureRequestEvent *cre = &e->xconfigurerequest;
	XWindowChanges xwc;
	unsigned long xwcm;
	FvwmWindow *fw2 = NULL;

	if (cre->value_mask & CWSibling)
	{
		if (XFindContext(
			    dpy, cre->above, FvwmContext,
			    (caddr_t *)&fw2) == XCNOENT)
		{
			fw2 = NULL;
		}
		if (fw2 == fw)
		{
			fw2 = NULL;
		}
	}
	if (cre->detail != Above && cre->detail != Below)
	{
		HandleUnusualStackmodes(
			cre->detail, fw, cre->window, fw2, cre->above);
	}
	/* only allow clients to restack windows within their layer */
	else if (fw2 == NULL || compare_window_layers(fw2, fw) != 0)
	{
		switch (cre->detail)
		{
		case Above:
			RaiseWindow(fw, True);
			break;
		case Below:
			LowerWindow(fw, True);
			break;
		}
	}
	else
	{
		xwc.sibling = FW_W_FRAME(fw2);
		xwc.stack_mode = cre->detail;
		xwcm = CWSibling | CWStackMode;
		XConfigureWindow(dpy, FW_W_FRAME(fw), xwcm, &xwc);

		/* Maintain the condition that icon windows are stacked
		 * immediately below their frame
		 * 1. for fw */
		xwc.sibling = FW_W_FRAME(fw);
		xwc.stack_mode = Below;
		xwcm = CWSibling | CWStackMode;
		if (FW_W_ICON_TITLE(fw) != None)
		{
			XConfigureWindow(
				dpy, FW_W_ICON_TITLE(fw), xwcm, &xwc);
		}
		if (FW_W_ICON_PIXMAP(fw) != None)
		{
			XConfigureWindow(
				dpy, FW_W_ICON_PIXMAP(fw), xwcm, &xwc);
		}
		/* 2. for fw2 */
		if (cre->detail == Below)
		{
			xwc.sibling = FW_W_FRAME(fw2);
			xwc.stack_mode = Below;
			xwcm = CWSibling | CWStackMode;
			if (FW_W_ICON_TITLE(fw2) != None)
			{
				XConfigureWindow(
					dpy, FW_W_ICON_TITLE(fw2), xwcm, &xwc);
			}
			if (FW_W_ICON_PIXMAP(fw2) != None)
			{
				XConfigureWindow(
					dpy, FW_W_ICON_PIXMAP(fw2), xwcm,
					&xwc);
			}
		}
		/* Maintain the stacking order ring */
		if (cre->detail == Above)
		{
			remove_window_from_stack_ring(fw);
			add_window_to_stack_ring_after(
				fw, get_prev_window_in_stack_ring(fw2));
		}
		else /* cre->detail == Below */
		{
			remove_window_from_stack_ring(fw);
			add_window_to_stack_ring_after(fw, fw2);
		}
	        BroadcastRestackThisWindow(fw);
	}
	/* srt (28-Apr-2001): Tk needs a ConfigureNotify event after a
	 * raise, otherwise it would hang for two seconds */
	*ret_do_send_event = 1;

	return;
}

static inline void _cr_get_static_position(
	rectangle *ret_g, FvwmWindow *fw, XEvent *e, size_borders *b)
{
	XConfigureRequestEvent *cre = &e->xconfigurerequest;

	if (cre->value_mask & CWX)
	{
		ret_g->x = cre->x - b->top_left.width;
	}
	else
	{
		ret_g->x = fw->g.frame.x;
	}
	if (cre->value_mask & CWY)
	{
		ret_g->y = cre->y - b->top_left.height;
	}
	else
	{
		ret_g->y = fw->g.frame.y;
	}

	return;
}

static inline void _cr_get_grav_position(
	rectangle *ret_g, FvwmWindow *fw, XEvent *e, size_borders *b)
{
	XConfigureRequestEvent *cre = &e->xconfigurerequest;
	int grav_x;
	int grav_y;

	gravity_get_offsets(fw->hints.win_gravity, &grav_x, &grav_y);
	if (cre->value_mask & CWX)
	{
		ret_g->x = cre->x - ((grav_x + 1) * b->total_size.width) / 2;
	}
	else
	{
		ret_g->x = fw->g.frame.x;
	}
	if (cre->value_mask & CWY)
	{
		ret_g->y = cre->y - ((grav_y + 1) * b->total_size.height) / 2;
	}
	else
	{
		ret_g->y = fw->g.frame.y;
	}

	return;
}

/* Try to detect whether the application uses the ICCCM way of moving its
 * window or the traditional way, always assuming StaticGravity. */
static inline void _cr_detect_icccm_move(
	FvwmWindow *fw, XEvent *e, size_borders *b)
{
	XConfigureRequestEvent *cre = &e->xconfigurerequest;
	rectangle grav_g;
	rectangle static_g;
	rectangle dg_g;
	rectangle ds_g;
	int mx;
	int my;
	int m;
	int w;
	int h;
	int has_x;
	int has_y;

	if (CR_MOTION_METHOD(fw) != CR_MOTION_METHOD_AUTO)
	{
		if (Scr.bo.do_debug_cr_motion_method == 1)
		{
			fprintf(
				stderr,
				"_cdim: --- already detected (pid %d) %p"
				" '%s'\n", HAS_EWMH_WM_PID(fw), fw,
				fw->visible_name);
		}
		return;
	}
	if (HAS_EWMH_WM_PID(fw))
	{
		if (Scr.bo.do_debug_cr_motion_method == 1)
		{
			fprintf(
				stderr,"_cdim: +++ has ewmh_wm_pid: icccm"
				" %p '%s'\n", fw, fw->visible_name);
		}
		SET_CR_MOTION_METHOD(fw, CR_MOTION_METHOD_USE_GRAV);
		SET_CR_MOTION_METHOD_DETECTED(fw, 1);
		return;
	}
	if (fw->ewmh_window_type != EWMH_WINDOW_TYPE_NONE_ID)
	{
		if (Scr.bo.do_debug_cr_motion_method == 1)
		{
			fprintf(
				stderr, "_cdim: +++ has ewmh_window_type:"
				" icccm %p '%s'\n", fw,
				fw->visible_name);
		}
		SET_CR_MOTION_METHOD(fw, CR_MOTION_METHOD_USE_GRAV);
		SET_CR_MOTION_METHOD_DETECTED(fw, 1);
		return;
	}
	if (FShapesSupported && fw->wShaped)
	{
		if (Scr.bo.do_debug_cr_motion_method == 1)
		{
			fprintf(
				stderr, "_cdim: --- shaped window %p "
				"'%s'\n", fw, fw->visible_name);
		}
		/* no detection for shaped windows */
		return;
	}
	if (fw->hints.win_gravity == StaticGravity)
	{
		if (Scr.bo.do_debug_cr_motion_method == 1)
		{
			fprintf(
				stderr, "_cdim: --- using StaticGravity"
				" %p '%s'\n", fw, fw->visible_name);
		}
		return;
	}
	has_x = (cre->value_mask & CWX);
	has_y = (cre->value_mask & CWY);
	if (!has_x && !has_y)
	{
		if (Scr.bo.do_debug_cr_motion_method == 1)
		{
			fprintf(
				stderr, "_cdim: --- not moved %p '%s'\n",
				fw, fw->visible_name);
		}
		return;
	}
	_cr_get_grav_position(&grav_g, fw, e, b);
	_cr_get_static_position(&static_g, fw, e, b);
	if (static_g.x == grav_g.x)
 	{
		/* both methods have the same result; ignore */
		has_x = 0;
	}
	if (static_g.y == grav_g.y)
	{
		/* both methods have the same result; ignore */
		has_y = 0;
	}
	if (!has_x && !has_y)
	{
		if (Scr.bo.do_debug_cr_motion_method == 1)
		{
			fprintf(
				stderr, "_cdim: --- not moved %p '%s'\n",
				fw, fw->visible_name);
		}
		return;
	}
	dg_g.x = grav_g.x - fw->g.frame.x;
	dg_g.y = grav_g.y - fw->g.frame.y;
	ds_g.x = static_g.x - fw->g.frame.x;
	ds_g.y = static_g.y - fw->g.frame.y;
	if (Scr.bo.do_debug_cr_motion_method == 1)
	{
		fprintf(
			stderr, "s %3d/%3d %2d/%2d, g %3d/%3d %2d/%2d: ",
			static_g.x, static_g.y, ds_g.x, ds_g.y, grav_g.x,
			grav_g.y, dg_g.x, dg_g.y);
	}
	/* check full screen */
	if ((cre->value_mask & (CWX | CWY)) == (CWX | CWY) &&
	    (has_x || has_y) &&
	    cre->width == Scr.MyDisplayWidth &&
	    cre->height == Scr.MyDisplayHeight)
	{
		if (grav_g.x == -b->top_left.width &&
		    grav_g.y == -b->top_left.height)
		{
			/* Window is fullscreen using the ICCCM way. */
			SET_CR_MOTION_METHOD(fw, CR_MOTION_METHOD_USE_GRAV);
			SET_CR_MOTION_METHOD_DETECTED(fw, 1);
			if (Scr.bo.do_debug_cr_motion_method == 1)
			{
				fprintf(
					stderr, "+++ fullscreen icccm %p"
					" '%s'\n", fw, fw->visible_name);
			}
			return;
		}
		else if (static_g.x == -b->top_left.width &&
			 static_g.y == -b->top_left.height)
		{
			/* Window is fullscreen using the traditional way. */
			SET_CR_MOTION_METHOD(fw, CR_MOTION_METHOD_STATIC_GRAV);
			SET_CR_MOTION_METHOD_DETECTED(fw, 1);
			if (Scr.bo.do_debug_cr_motion_method == 1)
			{
				fprintf(
					stderr, "+++ fullscreen traditional"
					" %p '%s'\n", fw,
					fw->visible_name);
			}
			return;
		}
	}
	/* check travelling across the screen */
	if (has_x && dg_g.x == 0 && ds_g.x != 0 &&
	    has_y && dg_g.y == 0 && ds_g.y != 0)
	{
		/* The traditional way causes a shift by the border width or
		 * height.  Use ICCCM way. */
		SET_CR_MOTION_METHOD(fw, CR_MOTION_METHOD_USE_GRAV);
		SET_CR_MOTION_METHOD_DETECTED(fw, 1);
		if (Scr.bo.do_debug_cr_motion_method == 1)
		{
			fprintf(
				stderr, "+++ travelling icccm %p '%s'\n",
				fw, fw->visible_name);
		}
		return;
	}
	if (has_x && dg_g.x != 0 && ds_g.x == 0 &&
	    has_y && dg_g.y != 0 && ds_g.y == 0)
	{
		/* The ICCCM way causes a shift by the border width or height.
		 * Use traditional way. */
		SET_CR_MOTION_METHOD(fw, CR_MOTION_METHOD_STATIC_GRAV);
		SET_CR_MOTION_METHOD_DETECTED(fw, 1);
		if (Scr.bo.do_debug_cr_motion_method == 1)
		{
			fprintf(
				stderr, "+++ travelling traditional %p"
				" '%s'\n", fw, fw->visible_name);
		}
		return;
	}
	/* check placement near border */
	w = (cre->value_mask & CWWidth) ?
		cre->width + b->total_size.width : fw->g.frame.width;
	h = (cre->value_mask & CWHeight) ?
		cre->height + b->total_size.height : fw->g.frame.height;
	if (!has_x)
	{
		mx = CR_MOTION_METHOD_AUTO;
	}
	else if (static_g.x == 0 || static_g.x + w == Scr.MyDisplayWidth)
	{
		mx = CR_MOTION_METHOD_STATIC_GRAV;
	}
	else if (grav_g.x == 0 || grav_g.x + w == Scr.MyDisplayWidth)
	{
		mx = CR_MOTION_METHOD_USE_GRAV;
	}
	else
	{
		mx = CR_MOTION_METHOD_AUTO;
	}
	if (!has_y)
	{
		my = CR_MOTION_METHOD_AUTO;
	}
	else if (static_g.y == 0 || static_g.y + h == Scr.MyDisplayHeight)
	{
		my = CR_MOTION_METHOD_STATIC_GRAV;
	}
	else if (grav_g.y == 0 || grav_g.y + h == Scr.MyDisplayHeight)
	{
		my = CR_MOTION_METHOD_USE_GRAV;
	}
	else
	{
		my = CR_MOTION_METHOD_AUTO;
	}
	m = (mx != CR_MOTION_METHOD_AUTO) ? mx : my;
	if (m != CR_MOTION_METHOD_AUTO)
	{
		/* Window was placed next to the display border. */
		if (m == my || my == CR_MOTION_METHOD_AUTO)
		{
			SET_CR_MOTION_METHOD(fw, m);
			SET_CR_MOTION_METHOD_DETECTED(fw, 1);
			if (Scr.bo.do_debug_cr_motion_method == 1)
			{
				fprintf(
					stderr, "+++ near border %s %p "
					"'%s'\n", (m ==
						   CR_MOTION_METHOD_USE_GRAV)
					? "icccm" : "traditional", fw,
					fw->visible_name);
			}
			return;
		}
	}
	if (Scr.bo.do_debug_cr_motion_method == 1)
	{
		fprintf(
			stderr, "--- not detected %p '%s'\n", fw,
			fw->visible_name);
	}

	return;
}

/* This is not a good idea because this interferes with changes in the size
 * hints of the window.  However, it is impossible to be completely safe here.
 * For example, if the client changes the size inc, then resizes the size of
 * its window and then changes the size inc again - all in one batch - then
 * the WM will read the *second* size inc upon the *first* event and use the
 * wrong one in the ConfigureRequest calculations. */
/* dv (31 Mar 2002): The code now handles these situations, so enable it
 * again. */
static int _pred_merge_cr(Display *display, XEvent *event, XPointer arg)
{
	_merge_cr_args *args = (_merge_cr_args *)arg;

	switch (event->type)
	{
	case ConfigureRequest:
	{
		XConfigureRequestEvent *ecr = &event->xconfigurerequest;
		XConfigureRequestEvent *lcr =
			&args->last_cr_event->xconfigurerequest;

		if (event->xconfigurerequest.window != args->w)
		{
			/* no match, keep looking */
			return 0;
		}
		/* collect the size/position changes */
		if (lcr->value_mask & CWX)
		{
			ecr->x = lcr->x;
		}
		if (lcr->value_mask & CWY)
		{
			ecr->y = lcr->y;
		}
		if (lcr->value_mask & CWWidth)
		{
			ecr->width = lcr->width;
		}
		if (lcr->value_mask & CWHeight)
		{
			ecr->height = lcr->height;
		}
		if (lcr->value_mask & CWBorderWidth)
		{
			ecr->border_width = lcr->border_width;
		}
		/* add to new event and remove from old event */
		ecr->value_mask |= (lcr->value_mask & CR_MOVERESIZE_MASK);
		lcr->value_mask &= ~CR_MOVERESIZE_MASK;
		if (lcr->value_mask == 0)
		{
			/* The event has no useful contents anymore. */
			FEV_INVALIDATE_EVENT(args->last_cr_event);
		}
		args->last_cr_event = event;
		args->count++;

		/* don't drop the current event and continue weeding */
		return 0;
	}
	case PropertyNotify:
		if (
			event->xproperty.window == args->w &&
			event->xproperty.atom != XA_WM_NORMAL_HINTS)
		{
			/* ConfigureRequest events cannot be merged past
			 * changes of the size hints. */
			/* don't merge and stop weeding */
			return 2;
		}
		else
		{
			/* PropertyNotify for another window, or the changed
			 * property does not interfere with merging. */
			/* keep looking */
			return 0;
		}
	default:
		/* Other events do not interfer with merging. */
		/* keep looking */
		return 0;
	}
}

static inline int _merge_cr_moveresize(
	const evh_args_t *ea, XEvent *ev, FvwmWindow *fw, size_borders *b)
{
	_merge_cr_args args;

	memset(&args, 0, sizeof(args));
	args.w = ev->xconfigurerequest.window;
	args.last_cr_event = ev;
	FWeedIfEvents(dpy, _pred_merge_cr, (XPointer)&args);

#if 1 /*!!!*/
	if (args.count > 0)
	{
		fprintf(stderr, "%s: merged %d cr events\n", __func__, args.count);
	}
#endif
	/* use the count from the structure, not the return value of
	 * FWeedIfEvents() because the predicate has a different way of weeding
	 * and the return value is always zero. */
	return args.count;
}

static inline int _handle_cr_on_client(
	int *ret_do_send_event, XEvent *e, const evh_args_t *ea,
	FvwmWindow *fw, Bool force, int force_gravity)
{
	XConfigureRequestEvent *cre = &e->xconfigurerequest;
	rectangle current_g;
	rectangle new_g;
	rectangle d_g;
	size_rect constr_dim;
	size_rect oldnew_dim;
	size_borders b;
	int gravity;

	if (ea)
	{
		cre = &ea->exc->x.etrigger->xconfigurerequest;
	}
	if (cre->value_mask & CWBorderWidth)
	{
		/* for restoring */
		fw->attr_backup.border_width = cre->border_width;
	}
	if ((cre->value_mask & (CWWidth | CWHeight | CWX | CWY)) == 0)
	{
		return 0;
	}

	get_window_borders(fw, &b);
	/* Merge all pending ConfigureRequests for the window into a single
	 * event.  However, we can not do this if the window uses the motion
	 * method autodetection because the merged event might confuse the
	 * detection code. */
	if (ea && CR_MOTION_METHOD(fw) != CR_MOTION_METHOD_AUTO)
	{
		int count;

		count = _merge_cr_moveresize(ea, e, fw, &b);
		if (count > 0)
		{
			/* the event has been merged into a later one, do
			 * nothing */
			return 0;
		}
	}
#if 0
	fprintf(stderr,
		"cre: %d(%d) %d(%d) %d(%d)x%d(%d) fw 0x%08x w 0x%08x "
		"ew 0x%08x  '%s'\n",
		cre->x, (int)(cre->value_mask & CWX),
		cre->y, (int)(cre->value_mask & CWY),
		cre->width, (int)(cre->value_mask & CWWidth),
		cre->height, (int)(cre->value_mask & CWHeight),
		(int)FW_W_FRAME(fw), (int)FW_W(fw), (int)cre->window,
		(fw->name.name) ? fw->name.name : "");
#endif
	/* Don't modify frame_g fields before calling SetupWindow! */
	memset(&d_g, 0, sizeof(d_g));

	if (HAS_NEW_WM_NORMAL_HINTS(fw))
	{
		/* get the latest size hints */
		XSync(dpy, 0);
		GetWindowSizeHints(fw);
		SET_HAS_NEW_WM_NORMAL_HINTS(fw, 0);
	}
	if (!HAS_OVERRIDE_SIZE_HINTS(fw) && (fw->hints.flags & PMaxSize))
	{
		/* Java workaround */
		if (cre->height > fw->hints.max_height &&
		    fw->hints.max_height <= BROKEN_MAXSIZE_LIMIT)
		{
			fw->hints.max_height = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
			cre->value_mask |= CWHeight;
		}
		if (cre->width > fw->hints.max_width &&
		    fw->hints.max_width <= BROKEN_MAXSIZE_LIMIT)
		{
			fw->hints.max_width = DEFAULT_MAX_MAX_WINDOW_WIDTH;
			cre->value_mask |= CWWidth;
		}
	}
	if (!HAS_OVERRIDE_SIZE_HINTS(fw) && (fw->hints.flags & PMinSize))
	{
		if (cre->width < fw->hints.min_width &&
		    fw->hints.min_width >= BROKEN_MINSIZE_LIMIT)
		{
			fw->hints.min_width = 1;
			cre->value_mask |= CWWidth;
		}
		if (cre->height < fw->hints.min_height &&
		    fw->hints.min_height >= BROKEN_MINSIZE_LIMIT)
		{
			fw->hints.min_height = 1;
			cre->value_mask |= CWHeight;
		}
	}
	if (IS_SHADED(fw) ||
	    !is_function_allowed(F_MOVE, NULL, fw, RQORIG_PROGRAM, False))
	{
		/* forbid shaded applications to move their windows */
		cre->value_mask &= ~(CWX | CWY);
		/* resend the old geometry */
		*ret_do_send_event = 1;
	}
	if (IS_MAXIMIZED(fw))
	{
		/* dont allow clients to resize maximized windows */
		cre->value_mask &= ~(CWWidth | CWHeight);
		/* resend the old geometry */
		*ret_do_send_event = 1;
		d_g.width = 0;
		d_g.height = 0;
	}
	else if (
		!is_function_allowed(
			F_RESIZE, NULL, fw, RQORIG_PROGRAM, False))
	{
		cre->value_mask &= ~(CWWidth | CWHeight);
		*ret_do_send_event = 1;
	}

	if (!force && CR_MOTION_METHOD(fw) == CR_MOTION_METHOD_AUTO)
	{
		_cr_detect_icccm_move(fw, e, &b);
	}
	if (force_gravity > ForgetGravity && force_gravity <= StaticGravity)
	{
		gravity = force_gravity;
	}
	else
	{
		gravity = fw->hints.win_gravity;
	}
	if (IS_SHADED(fw))
	{
		direction_t gravity_dir;

		get_unshaded_geometry(fw, &current_g);
		/* the shade direction overrides the window's gravity */
		gravity_dir = gravity_grav_to_dir(gravity);
		gravity_dir = gravity_override_dir(
			gravity_dir, SHADED_DIR(fw));
		gravity = gravity_dir_to_grav(gravity_dir);
	}
	else
	{
		current_g = fw->g.frame;
	}
	if (!(cre->value_mask & (CWX | CWY)))
	{
		/* nothing */
	}
	else if ((force ||
		  CR_MOTION_METHOD(fw) == CR_MOTION_METHOD_USE_GRAV) &&
		 gravity != StaticGravity)
	{
		int ref_x;
		int ref_y;
		int grav_x;
		int grav_y;

		gravity_get_offsets(gravity, &grav_x, &grav_y);
		if (cre->value_mask & CWX)
		{
			ref_x = cre->x -
				((grav_x + 1) * b.total_size.width) / 2;
			d_g.x = ref_x - current_g.x;
		}
		if (cre->value_mask & CWY)
		{
			ref_y = cre->y -
				((grav_y + 1) * b.total_size.height) / 2;
			d_g.y = ref_y - current_g.y;
		}
	}
	else /* ..._USE_GRAV or ..._AUTO */
	{
		/* default: traditional cr handling */
		if (cre->value_mask & CWX)
		{
			d_g.x = cre->x - current_g.x - b.top_left.width;
		}
		if (cre->value_mask & CWY)
		{
			d_g.y = cre->y - current_g.y - b.top_left.height;
		}
	}

	if (cre->value_mask & CWHeight)
	{
		if (cre->height <
		    (WINDOW_FREAKED_OUT_SIZE - b.total_size.height))
		{
			d_g.height = cre->height -
				(current_g.height - b.total_size.height);
		}
		else
		{
			/* Ignore height changes to astronomically large
			 * windows (needed for XEmacs 20.4); don't care if the
			 * window is shaded here - we won't use 'height' in
			 * this case anyway.
			 * Inform the buggy app about the size that *we* want
			 */
			d_g.height = 0;
			*ret_do_send_event = 1;
		}
	}
	if (cre->value_mask & CWWidth)
	{
		if (cre->width < (WINDOW_FREAKED_OUT_SIZE - b.total_size.width))
		{
			d_g.width = cre->width -
				(current_g.width - b.total_size.width);
		}
		else
		{
			d_g.width = 0;
			*ret_do_send_event = 1;
		}
	}

	/* SetupWindow (x,y) are the location of the upper-left outer corner
	 * and are passed directly to XMoveResizeWindow (frame).  The
	 * (width,height) are the inner size of the frame.  The inner width is
	 * the same as the requested client window width; the inner height is
	 * the same as the requested client window height plus any title bar
	 * slop. */
	new_g = current_g;
	oldnew_dim.width = new_g.width + d_g.width;
	oldnew_dim.height = new_g.height + d_g.height;
	constr_dim.width = oldnew_dim.width;
	constr_dim.height = oldnew_dim.height;
	constrain_size(
		fw, NULL, &constr_dim.width, &constr_dim.height, 0, 0,
		CS_UPDATE_MAX_DEFECT);
	d_g.width += (constr_dim.width - oldnew_dim.width);
	d_g.height += (constr_dim.height - oldnew_dim.height);
	if ((cre->value_mask & CWX) && d_g.width)
	{
		new_g.x = current_g.x + d_g.x;
		new_g.width = current_g.width + d_g.width;
	}
	else if ((cre->value_mask & CWX) && !d_g.width)
	{
		new_g.x = current_g.x + d_g.x;
	}
	else if (!(cre->value_mask & CWX) && d_g.width)
	{
		gravity_resize(gravity, &new_g, d_g.width, 0);
	}
	if ((cre->value_mask & CWY) && d_g.height)
	{
		new_g.y = current_g.y + d_g.y;
		new_g.height = current_g.height + d_g.height;
	}
	else if ((cre->value_mask & CWY) && !d_g.height)
	{
		new_g.y = current_g.y + d_g.y;
	}
	else if (!(cre->value_mask & CWY) && d_g.height)
	{
		gravity_resize(gravity, &new_g, 0, d_g.height);
	}

	if (new_g.x == current_g.x && new_g.y == current_g.y &&
	    new_g.width == current_g.width &&
	    new_g.height == current_g.height)
	{
		/* Window will not be moved or resized; send a synthetic
		 * ConfigureNotify. */
		*ret_do_send_event = 1;
	}
	else if ((cre->value_mask & CWX) || (cre->value_mask & CWY) ||
		 d_g.width || d_g.height)
	{
		if (IS_SHADED(fw))
		{
			fw->g.normal = new_g;
			get_shaded_geometry(fw, &new_g, &new_g);
		}
		frame_setup_window_app_request(
			fw, new_g.x, new_g.y, new_g.width, new_g.height,
			False);
		/* make sure the window structure has the new position */
		update_absolute_geometry(fw);
		maximize_adjust_offset(fw);
	}
	else if (DO_FORCE_NEXT_CR(fw))
	{
		*ret_do_send_event = 1;
	}
	SET_FORCE_NEXT_CR(fw, 0);
	SET_FORCE_NEXT_PN(fw, 0);

	return 1;
}

void _handle_configure_request(
	XEvent *e, const evh_args_t *ea, FvwmWindow *fw,
	Bool force, int force_gravity)
{
	XConfigureRequestEvent *cre = &e->xconfigurerequest;
	int do_send_event = 0;
	int cn_count = 0;

	fev_sanitise_configure_request(cre);
	/* According to the July 27, 1988 ICCCM draft, we should ignore size
	 * and position fields in the WM_NORMAL_HINTS property when we map a
	 * window. Instead, we'll read the current geometry.  Therefore, we
	 * should respond to configuration requests for windows which have
	 * never been mapped. */
	if (fw == NULL)
	{
		_handle_cr_on_unmanaged(e);
		return;
	}
	if (cre->window == FW_W_ICON_TITLE(fw) ||
	    cre->window == FW_W_ICON_PIXMAP(fw))
	{
		_handle_cr_on_icon(e, fw);
	}
	if (FShapesSupported)
	{
		_handle_cr_on_shaped(fw);
	}
	if (fw != NULL && cre->window == FW_W(fw))
	{
		cn_count = _handle_cr_on_client(
			&do_send_event, e, ea, fw, force, force_gravity);
	}
	/* Stacking order change requested.  Handle this *after* geometry
	 * changes, since we need the new geometry in occlusion calculations */
	if ((cre->value_mask & CWStackMode) &&
	    (!DO_IGNORE_RESTACK(fw) || force))
	{
		_handle_cr_restack(&do_send_event, e, fw);
	}
#if 1
	/* This causes some ddd windows not to be drawn properly. Reverted back
	 * to the old method in frame_setup_window. */
	/* domivogt (15-Oct-1999): enabled this to work around buggy apps that
	 * ask for a nonsense height and expect that they really get it. */
	if (cn_count == 0 && do_send_event)
	{
		cn_count = 1;
	}
	else if (cn_count > 0)
	{
		do_send_event = 1;
	}
	for ( ; cn_count > 0; cn_count--)
	{
		SendConfigureNotify(
			fw, fw->g.frame.x, fw->g.frame.y, fw->g.frame.width,
			fw->g.frame.height, 0, True);
	}
	if (do_send_event)
	{
		XFlush(dpy);
	}
#endif

	return;
}

static Bool _pred_button_click(
	Display *display, XEvent *event, XPointer arg)
{
	if (event->type == ButtonPress || event->type == ButtonRelease)
	{
		return True;
	}

	return False;
}

/* Helper function for __handle_focus_raise_click(). */
static Bool __test_for_motion(int x0, int y0)
{
	int x;
	int y;
	unsigned int mask;
	XEvent e;

	/* Query the pointer to do this. We can't check for events here since
	 * the events are still needed if the pointer moves. */

	/* However, some special mouse (e.g., a touchpad with the
	 * synaptic driver) may handle a double click in a special way
	 * (for dragging through short touching and holding down the
	 * finger on the touchpad). Bascially, when you execute a
	 * double click the first button release is queued after the
	 * second _physical_ mouse release happen. It seems that
	 * FQueryPointer may not work as expected: it does not see
	 * that the button is released on a double click.  So, we need
	 * to check for a button press in the future to avoid a fvwm
	 * lockup! (olicha 2004-01-31) */

	for (x = x0, y = y0; FQueryPointer(
		     dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX, &JunkY,
		     &x, &y, &mask) == True; usleep(20000))
	{
		if ((mask & DEFAULT_ALL_BUTTONS_MASK) == 0)
		{
			/* all buttons are released */
			return False;
		}
		else if (abs(x - x0) >= Scr.MoveThreshold ||
			 abs(y - y0) >= Scr.MoveThreshold)
		{
			/* the pointer has moved */
			return True;
		}
		if (FCheckPeekIfEvent(dpy, &e, _pred_button_click, NULL))
		{
			/* click in the future */
			return False;
		}
		else
		{
			/* The predicate procedure finds no match, no event
			 * has been removed from the queue and XFlush was
			 * called. Nothing to do */
		}
	}

	/* pointer has moved off screen */
	return True;
}

/* Helper function for __handle_focus_raise_click(). */
static void __check_click_to_focus_or_raise(
	hfrc_ret_t *ret_args, const exec_context_t *exc)
{
	FvwmWindow * const fw = exc->w.fw;
	const XEvent *te = exc->x.etrigger;
	struct
	{
		unsigned is_client_click : 1;
		unsigned is_focused : 1;
	} f;

	f.is_focused = !!focus_is_focused(fw);
	f.is_client_click = (exc->w.wcontext == C_WINDOW ||
			     exc->w.wcontext == C_EWMH_DESKTOP);
	/* check if we need to raise and/or focus the window */
	ret_args->do_focus = focus_query_click_to_focus(fw, exc->w.wcontext);
	if (f.is_client_click && !ret_args->do_focus &&
	    !f.is_focused && FP_DO_FOCUS_BY_PROGRAM(FW_FOCUS_POLICY(fw)) &&
	    !fpol_query_allow_user_focus(&FW_FOCUS_POLICY(fw)))
	{
		/* Give the window a chance to to take focus itself */
		ret_args->do_focus = 1;
	}
	if (ret_args->do_focus && focus_is_focused(fw))
	{
		ret_args->do_focus = 0;
	}
 	ret_args->do_raise =
		focus_query_click_to_raise(fw, f.is_focused, exc->w.wcontext);
#define EXPERIMENTAL_ROU_HANDLING_V2
#ifdef EXPERIMENTAL_ROU_HANDLING_V2
/*  RBW -- Dang! This works without the one in HandleEnterNotify!  */
	if (ret_args->do_raise && is_on_top_of_layer_and_above_unmanaged(fw))
#else
	if (ret_args->do_raise && is_on_top_of_layer(fw))
#endif
	{
		ret_args->do_raise = 0;
	}
	if ((ret_args->do_focus &&
	     FP_DO_IGNORE_FOCUS_CLICK_MOTION(FW_FOCUS_POLICY(fw))) ||
	    (ret_args->do_raise &&
	     FP_DO_IGNORE_RAISE_CLICK_MOTION(FW_FOCUS_POLICY(fw))))
	{
		/* Pass further events to the application and check if a button
		 * release or motion event occurs next.  If we don't do this
		 * here, the pointer will seem to be frozen in
		 * __test_for_motion(). */
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		if (__test_for_motion(te->xbutton.x_root, te->xbutton.y_root))
		{
			/* the pointer was moved, process event normally */
			ret_args->do_focus = 0;
			ret_args->do_raise = 0;
		}
	}
	if (ret_args->do_focus || ret_args->do_raise)
	{
		if (!((ret_args->do_focus &&
		       FP_DO_ALLOW_FUNC_FOCUS_CLICK(FW_FOCUS_POLICY(fw))) ||
		      (ret_args->do_raise &&
		       FP_DO_ALLOW_FUNC_RAISE_CLICK(FW_FOCUS_POLICY(fw)))))
		{
			ret_args->do_forbid_function = 1;
		}
		if (!((ret_args->do_focus &&
		       FP_DO_PASS_FOCUS_CLICK(FW_FOCUS_POLICY(fw))) ||
		      (ret_args->do_raise &&
		       FP_DO_PASS_RAISE_CLICK(FW_FOCUS_POLICY(fw)))))
		{
			ret_args->do_swallow_click = 1;
		}
	}

	return;
}

/* Finds out if the click on a window must be used to focus or raise it. */
static void __handle_focus_raise_click(
	hfrc_ret_t *ret_args, const exec_context_t *exc)
{
	memset(ret_args, 0, sizeof(*ret_args));
	if (exc->w.fw == NULL)
	{
		return;
	}
	/* check for proper click button and modifiers*/
	if (FP_USE_MOUSE_BUTTONS(FW_FOCUS_POLICY(exc->w.fw)) != 0 &&
	    !(FP_USE_MOUSE_BUTTONS(FW_FOCUS_POLICY(exc->w.fw)) &
	      (1 << (exc->x.etrigger->xbutton.button - 1))))
	{
		/* wrong button, handle click normally */
		return;
	}
	else if (FP_USE_MODIFIERS(FW_FOCUS_POLICY(exc->w.fw)) !=
		 FPOL_ANY_MODIFIER &&
		 MaskUsedModifiers(
			 FP_USE_MODIFIERS(FW_FOCUS_POLICY(exc->w.fw))) !=
		 MaskUsedModifiers(exc->x.etrigger->xbutton.state))
	{
		/* right button but wrong modifiers, handle click normally */
		return;
	}
	else
	{
		__check_click_to_focus_or_raise(ret_args, exc);
	}

	return;
}

/* Helper function for HandleButtonPress */
static Bool __is_bpress_window_handled(const exec_context_t *exc)
{
	Window eventw;
	const XEvent *te = exc->x.etrigger;

	if (exc->w.fw == NULL)
	{
		if ((te->xbutton.window != Scr.Root ||
		     te->xbutton.subwindow != None) &&
		    !is_pan_frame(te->xbutton.window))
		{
			/* Ignore events in unmanaged windows or subwindows of
			 * a client */
			return False;
		}
		else
		{
			return True;
		}
	}
	eventw = (te->xbutton.subwindow != None &&
		  te->xany.window != FW_W(exc->w.fw)) ?
		te->xbutton.subwindow : te->xany.window;
	if (is_frame_hide_window(eventw) || eventw == FW_W_FRAME(exc->w.fw))
	{
		return False;
	}
	if (!XGetGeometry(
		    dpy, eventw, &JunkRoot, &JunkX, &JunkY,
		    (unsigned int*)&JunkWidth, (unsigned int*)&JunkHeight,
		    (unsigned int*)&JunkBW, (unsigned int*)&JunkDepth))
	{
		/* The window has already died. */
		return False;
	}

	return True;
}

/* Helper function for __handle_bpress_on_managed */
static Bool __handle_click_to_focus(const exec_context_t *exc)
{
	fpol_set_focus_by_t set_by;

	switch (exc->w.wcontext)
	{
	case C_WINDOW:
	case C_EWMH_DESKTOP:
		set_by = FOCUS_SET_BY_CLICK_CLIENT;
		break;
	case C_ICON:
		set_by = FOCUS_SET_BY_CLICK_ICON;
		break;
	default:
		set_by = FOCUS_SET_BY_CLICK_DECOR;
		break;
	}
	SetFocusWindow(exc->w.fw, True, set_by);
	focus_grab_buttons(exc->w.fw);
	if (focus_is_focused(exc->w.fw) && !IS_ICONIFIED(exc->w.fw))
	{
		border_draw_decorations(
			exc->w.fw, PART_ALL, True, True, CLEAR_ALL, NULL,
			NULL);
	}

	return focus_is_focused(exc->w.fw);
}

/* Helper function for __handle_bpress_on_managed */
static Bool __handle_click_to_raise(const exec_context_t *exc)
{
	Bool rc = False;
	int is_focused;

	is_focused = focus_is_focused(exc->w.fw);
	if (focus_query_click_to_raise(exc->w.fw, is_focused, True))
	{
		rc = True;
	}

	return rc;
}

/* Helper function for HandleButtonPress */
static void __handle_bpress_stroke(void)
{
	STROKE_CODE(stroke_init());
	STROKE_CODE(send_motion = True);

	return;
}

/* Helper function for __handle_bpress_on_managed */
static Bool __handle_bpress_action(
	const exec_context_t *exc, char *action)
{
	window_parts part;
	Bool do_force;
	Bool rc = False;

	if (!action || *action == 0)
	{
		PressedW = None;
		return False;
	}
	/* draw pressed in decorations */
	part = border_context_to_parts(exc->w.wcontext);
	do_force = (part & PART_TITLEBAR) ? True : False;
	border_draw_decorations(
		exc->w.fw, part, (Scr.Hilite == exc->w.fw), do_force,
		CLEAR_ALL, NULL, NULL);
	/* execute the action */
	if (IS_ICONIFIED(exc->w.fw))
	{
		/* release the pointer since it can't do harm over an icon */
		XAllowEvents(dpy, AsyncPointer, CurrentTime);
	}
	execute_function(NULL, exc, action, 0);
	if (exc->w.wcontext != C_WINDOW && exc->w.wcontext != C_NO_CONTEXT)
	{
		WaitForButtonsUp(True);
		rc = True;
	}
	/* redraw decorations */
	PressedW = None;
	if (check_if_fvwm_window_exists(exc->w.fw))
	{
		part = border_context_to_parts(exc->w.wcontext);
		do_force = (part & PART_TITLEBAR) ? True : False;
		border_draw_decorations(
			exc->w.fw, part, (Scr.Hilite == exc->w.fw), do_force,
			CLEAR_ALL, NULL, NULL);
	}

	return rc;
}

/* Handles button presses on the root window. */
static void __handle_bpress_on_root(const exec_context_t *exc)
{
	char *action;

	PressedW = None;
	__handle_bpress_stroke();
	/* search for an appropriate mouse binding */
	action = CheckBinding(
		Scr.AllBindings, STROKE_ARG(0) exc->x.etrigger->xbutton.button,
		exc->x.etrigger->xbutton.state, GetUnusedModifiers(), C_ROOT,
		BIND_BUTTONPRESS, NULL, NULL);
	if (action && *action)
	{
		const exec_context_t *exc2;
		exec_context_changes_t ecc;

		ecc.w.wcontext = C_ROOT;
		exc2 = exc_clone_context(exc, &ecc, ECC_WCONTEXT);
		execute_function(NULL, exc2, action, 0);
		exc_destroy_context(exc2);
		WaitForButtonsUp(True);
	}

	return;
}

/* Handles button presses on unmanaged windows */
static void __handle_bpress_on_unmanaged(const exec_context_t *exc)
{
	/* Pass the event to the application. */
	XAllowEvents(dpy, ReplayPointer, CurrentTime);
	XFlush(dpy);

	return;
}

/* Handles button presses on managed windows */
static void __handle_bpress_on_managed(const exec_context_t *exc)
{
	char *action;
	hfrc_ret_t f;
	FvwmWindow * const fw = exc->w.fw;
	XEvent *e;

	e = exc->x.etrigger;
	/* Now handle click to focus and click to raise. */
	__handle_focus_raise_click(&f, exc);
	PressedW = (f.do_forbid_function) ? None : exc->w.w;
	if (f.do_focus)
	{
		if (!__handle_click_to_focus(exc))
		{
			/* Window didn't accept the focus; pass the click to
			 * the application. */
			f.do_swallow_click = 0;
		}
	}
	if (f.do_raise)
	{
		if (__handle_click_to_raise(exc) == True)
		{
			/* We can't raise the window immediately because the
			 * action bound to the click might be "Lower" or
			 * "RaiseLower". So mark the window as scheduled to be
			 * raised after the binding is executed. Functions that
			 * modify the stacking order will reset this flag. */
			SET_SCHEDULED_FOR_RAISE(fw, 1);
		}
	}
	/* handle bindings */
	if (!f.do_forbid_function)
	{
		/* stroke bindings */
		__handle_bpress_stroke();
		/* mouse bindings */
		action = CheckBinding(
			Scr.AllBindings, STROKE_ARG(0) e->xbutton.button,
			e->xbutton.state, GetUnusedModifiers(),
			exc->w.wcontext, BIND_BUTTONPRESS, &fw->class,
			fw->name.name);
		if (__handle_bpress_action(exc, action))
		{
			f.do_swallow_click = 1;
		}
	}
	/* raise the window */
	if (IS_SCHEDULED_FOR_RAISE(fw))
	{
		/* Now that we know the action did not restack the window we
		 * can raise it.
		 * dv (10-Aug-2002):  We can safely raise the window after
		 * redrawing it since all the decorations are drawn in the
		 * window background and no Expose event is generated. */
		RaiseWindow(fw, False);
		SET_SCHEDULED_FOR_RAISE(fw, 0);
	}
	/* clean up */
	if (!f.do_swallow_click)
	{
		/* pass the click to the application */
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		XFlush(dpy);
	}
	else if (f.do_focus || f.do_raise)
	{
		WaitForButtonsUp(True);
	}

	return;
}

/* restore focus stolen by unmanaged */
static void __refocus_stolen_focus_win(const evh_args_t *ea)
{
	FOCUS_SET(Scr.StolenFocusWin, Scr.StolenFocusFvwmWin);
	ea->exc->x.etrigger->xfocus.window = Scr.StolenFocusWin;
	ea->exc->x.etrigger->type = FocusIn;
	Scr.UnknownWinFocused = None;
	Scr.StolenFocusWin = None;
	Scr.StolenFocusFvwmWin = NULL;
	dispatch_event(ea->exc->x.etrigger);

	return;
}

/* ---------------------------- event handlers ----------------------------- */

void HandleButtonPress(const evh_args_t *ea)
{
	DBUG("HandleButtonPress", "Routine Entered");

	GrabEm(CRS_NONE, GRAB_PASSIVE);
	if (__is_bpress_window_handled(ea->exc) == False)
	{
		__handle_bpress_on_unmanaged(ea->exc);
	}
	else if (ea->exc->w.fw != NULL)
	{
		__handle_bpress_on_managed(ea->exc);
	}
	else
	{
		__handle_bpress_on_root(ea->exc);
	}
	UngrabEm(GRAB_PASSIVE);

	return;
}

#ifdef HAVE_STROKE
void HandleButtonRelease(const evh_args_t *ea)
{
	char *action;
	char *name;
	int real_modifier;
	const XEvent *te = ea->exc->x.etrigger;
	XClassHint *class;

	DBUG("HandleButtonRelease", "Routine Entered");
	send_motion = False;
	stroke_trans (sequence);
	DBUG("HandleButtonRelease",sequence);
	/*  Allows modifier to work (Only R context works here). */
	real_modifier = te->xbutton.state - (1 << (7 + te->xbutton.button));
	if (ea->exc->w.fw == NULL)
	{
		class = NULL;
		name = NULL;
	}
	else
	{
		class = &ea->exc->w.fw->class;
		name = ea->exc->w.fw->name.name;
	}
	/* need to search for an appropriate stroke binding */
	action = CheckBinding(
		Scr.AllBindings, sequence, te->xbutton.button, real_modifier,
		GetUnusedModifiers(), ea->exc->w.wcontext, BIND_STROKE,
		class, name);
	/* got a match, now process it */
	if (action != NULL && (action[0] != 0))
	{
		execute_function(NULL, ea->exc, action, 0);
		WaitForButtonsUp(True);
	}

	return;
}
#endif /* HAVE_STROKE */

void HandleClientMessage(const evh_args_t *ea)
{
	const XEvent *te = ea->exc->x.etrigger;
	FvwmWindow * const fw = ea->exc->w.fw;

	DBUG("HandleClientMessage", "Routine Entered");

	if (EWMH_ProcessClientMessage(ea->exc))
	{
		return;
	}

	/* handle deletion of tear out menus */
	if (fw && IS_TEAR_OFF_MENU(fw) && te->xclient.format == 32 &&
	    te->xclient.data.l[0] == _XA_WM_DELETE_WINDOW)
	{
		menu_close_tear_off_menu(fw);
		return;
	}

	if (te->xclient.message_type == _XA_WM_CHANGE_STATE &&
	    fw && te->xclient.data.l[0] == IconicState && !IS_ICONIFIED(fw))
	{
		const exec_context_t *exc;
		exec_context_changes_t ecc;

		ecc.w.wcontext = C_WINDOW;
		exc = exc_clone_context(ea->exc, &ecc, ECC_WCONTEXT);
		execute_function(NULL, exc, "Iconify", 0);
		exc_destroy_context(exc);
		return;
	}

	/* FIXME: Is this safe enough ? I guess if clients behave
	 * according to ICCCM and send these messages only if they
	 * grabbed the pointer, it is OK */
	{
		extern Atom _XA_WM_COLORMAP_NOTIFY;
		if (te->xclient.message_type == _XA_WM_COLORMAP_NOTIFY)
		{
			set_client_controls_colormaps(te->xclient.data.l[1]);
			return;
		}
	}

	/* CKH - if we get here, it was an unknown client message, so send
	 * it to the client if it was in a window we know about.  I'm not so
	 * sure this should be done or not, since every other window manager
	 * I've looked at doesn't.  But it might be handy for a free drag and
	 * drop setup being developed for Linux. */
	/* TA:  20091231 - But this confuses QT Drag and Drop since it handles
	 * processing XSendEvents in an odd order.  For now, workaround this
	 * by using a BugOpts option.
	 */
	if (fw)
	{
		if ((!Scr.bo.do_enable_qt_drag_n_drop_workaround) &&
				(te->xclient.window != FW_W(fw)))
		{
			XEvent e;

			e = *te;
			e.xclient.window = FW_W(fw);
			FSendEvent(dpy, FW_W(fw), False, NoEventMask, &e);
		}
	}
}

void HandleColormapNotify(const evh_args_t *ea)
{
	colormap_handle_colormap_notify(ea);

	return;
}

void HandleConfigureRequest(const evh_args_t *ea)
{
	XEvent *te = ea->exc->x.etrigger;
	XConfigureRequestEvent *cre;
	FvwmWindow *fw = ea->exc->w.fw;

	DBUG("HandleConfigureRequest", "Routine Entered");

	cre = &te->xconfigurerequest;
	/* te->xany.window is te->.xconfigurerequest.parent, so the context
	 * window may be wrong. */
	if (XFindContext(dpy, cre->window, FvwmContext, (caddr_t *)&fw) ==
	    XCNOENT)
	{
		fw = NULL;
	}
	_handle_configure_request(te, ea, fw, False, ForgetGravity);

	return;
}

void HandleDestroyNotify(const evh_args_t *ea)
{
	DBUG("HandleDestroyNotify", "Routine Entered");

	destroy_window(ea->exc->w.fw);
	EWMH_ManageKdeSysTray(
		ea->exc->x.etrigger->xdestroywindow.window,
		ea->exc->x.etrigger->type);
	EWMH_WindowDestroyed();

	return;
}

#define DEBUG_ENTERNOTIFY 0
#if DEBUG_ENTERNOTIFY
static int ecount=0;
#define ENTER_DBG(x) fprintf x;
#else
#define ENTER_DBG(x)
#endif
void HandleEnterNotify(const evh_args_t *ea)
{
	const XEnterWindowEvent *ewp;
	XEvent d;
	FvwmWindow *sf;
	static Bool is_initial_ungrab_pending = True;
	Bool is_tear_off_menu;
	const XEvent *te = ea->exc->x.etrigger;
	FvwmWindow * const fw = ea->exc->w.fw;

	DBUG("HandleEnterNotify", "Routine Entered");
	ewp = &te->xcrossing;
ENTER_DBG((stderr, "++++++++ en (%d): fw %p w 0x%08x sw 0x%08x mode 0x%x detail 0x%x '%s'\n", ++ecount, fw, (int)ewp->window, (int)ewp->subwindow, ewp->mode, ewp->detail, fw?fw->visible_name:"(none)"));

	if (
		ewp->window == Scr.Root &&
		ewp->detail == NotifyInferior && ewp->mode == NotifyNormal)
	{
		/* pointer left subwindow */
		BroadcastPacket(
			MX_ENTER_WINDOW, 3, (long)Scr.Root, (long)NULL,
			(long)NULL);
	}
	else if (
		ewp->window == Scr.Root &&
		ewp->detail == NotifyNonlinearVirtual)
	{
		/* pointer entered screen */
		BroadcastPacket(
			MX_ENTER_WINDOW, 3, (long)Scr.Root, (long)NULL,
			(long)NULL);
	}
	if (Scr.ColormapFocus == COLORMAP_FOLLOWS_MOUSE)
	{
		if (fw && !IS_ICONIFIED(fw) && ewp->window == FW_W(fw))
		{
			InstallWindowColormaps(fw);
		}
		else
		{
			/* make sure its for one of our windows */
			/* handle a subwindow cmap */
			InstallWindowColormaps(NULL);
		}
	}
	else if (!fw)
	{
		EnterSubWindowColormap(ewp->window);
	}
	if (Scr.flags.is_wire_frame_displayed)
	{
ENTER_DBG((stderr, "en: exit: iwfd\n"));
		/* Ignore EnterNotify events while a window is resized or moved
		 * as a wire frame; otherwise the window list may be screwed
		 * up. */
		return;
	}
	if (fw)
	{
		if (ewp->window != FW_W_FRAME(fw) &&
		    ewp->window != FW_W_PARENT(fw) &&
		    ewp->window != FW_W(fw) &&
		    ewp->window != FW_W_ICON_TITLE(fw) &&
		    ewp->window != FW_W_ICON_PIXMAP(fw))
		{
			/* Ignore EnterNotify that received by any of the sub
			 * windows that don't handle this event.  unclutter
			 * triggers these events sometimes, re focusing an
			 * unfocused window under the pointer */
ENTER_DBG((stderr, "en: exit: funny window\n"));
			return;
		}
	}
	if (Scr.focus_in_pending_window != NULL)
	{
ENTER_DBG((stderr, "en: exit: fipw\n"));
		/* Ignore EnterNotify event while we are waiting for a window to
		 * receive focus via Focus or FlipFocus commands. */
		focus_grab_buttons(fw);
		return;
	}
	if (ewp->mode == NotifyGrab)
	{
ENTER_DBG((stderr, "en: exit: NotifyGrab\n"));
		return;
	}
	else if (ewp->mode == NotifyNormal)
	{
ENTER_DBG((stderr, "en: NotifyNormal\n"));
		if (ewp->detail == NotifyNonlinearVirtual &&
		    ewp->focus == False && ewp->subwindow != None)
		{
			/* This takes care of some buggy apps that forget that
			 * one of their dialog subwindows has the focus after
			 * popping up a selection list several times (ddd,
			 * netscape). I'm not convinced that this does not
			 * break something else. */
ENTER_DBG((stderr, "en: NN: refreshing focus\n"));
			refresh_focus(fw);
		}
	}
	else if (ewp->mode == NotifyUngrab)
	{
ENTER_DBG((stderr, "en: NotifyUngrab\n"));
		/* Ignore events generated by grabbing or ungrabbing the
		 * pointer.  However, there is no way to prevent the client
		 * application from handling this event and, for example,
		 * grabbing the focus.  This will interfere with functions that
		 * transferred the focus to a different window. */
		if (is_initial_ungrab_pending)
		{
ENTER_DBG((stderr, "en: NU: initial ungrab pending (lgw = NULL)\n"));
			is_initial_ungrab_pending = False;
			xcrossing_last_grab_window = NULL;
		}
		else
		{
			if (ewp->detail == NotifyNonlinearVirtual &&
			    ewp->focus == False && ewp->subwindow != None)
			{
				/* see comment above */
ENTER_DBG((stderr, "en: NU: refreshing focus\n"));
				refresh_focus(fw);
			}
			if (fw && fw == xcrossing_last_grab_window)
			{
ENTER_DBG((stderr, "en: exit: NU: is last grab window\n"));
				if (ewp->window == FW_W_FRAME(fw) ||
				    ewp->window == FW_W_ICON_TITLE(fw) ||
				    ewp->window == FW_W_ICON_PIXMAP(fw))
				{
ENTER_DBG((stderr, "en: exit: NU: last grab window = NULL\n"));
					xcrossing_last_grab_window = NULL;
				}
				focus_grab_buttons(fw);

				return;
			}
			else if (fw)
			{
				if (ewp->window != FW_W_FRAME(fw) &&
				    ewp->window != FW_W_ICON_TITLE(fw) &&
				    ewp->window != FW_W_ICON_PIXMAP(fw))
				{
ENTER_DBG((stderr, "en: exit: NU: not frame window\n"));
					focus_grab_buttons(fw);
					return;
				}
			}
		}
	}
	if (fw)
	{
		is_initial_ungrab_pending = False;
	}

	/* look for a matching leaveNotify which would nullify this EnterNotify
	 */
	/*
	 * RBW - if we're in startup, this is a coerced focus, so we don't
	 * want to save the event time, or exit prematurely.
	 *
	 * Ignore LeaveNotify events for tear out menus - handled by menu code
	 */
	is_tear_off_menu =
		(fw && IS_TEAR_OFF_MENU(fw) && ewp->window == FW_W(fw));
	if (!fFvwmInStartup && !is_tear_off_menu &&
	    FCheckTypedWindowEvent(dpy, ewp->window, LeaveNotify, &d))
	{
		if (d.xcrossing.mode == NotifyNormal &&
		    d.xcrossing.detail != NotifyInferior)
		{
ENTER_DBG((stderr, "en: exit: found LeaveNotify\n"));
			return;
		}
	}

	if (ewp->window == Scr.Root)
	{
		FvwmWindow *lf = get_last_screen_focus_window();

		if (!Scr.flags.is_pointer_on_this_screen)
		{
			Scr.flags.is_pointer_on_this_screen = 1;
			if (lf && lf != &Scr.FvwmRoot &&
			    !FP_DO_UNFOCUS_LEAVE(FW_FOCUS_POLICY(lf)))
			{
				SetFocusWindow(lf, True, FOCUS_SET_FORCE);
			}
			else if (lf != &Scr.FvwmRoot)
			{
				ForceDeleteFocus();
			}
			else
			{
				/* This was the first EnterNotify event for the
				 * root window - ignore */
			}
			set_last_screen_focus_window(NULL);
		}
		else if (!(sf = get_focus_window()) ||
			 FP_DO_UNFOCUS_LEAVE(FW_FOCUS_POLICY(sf)))
		{
			DeleteFocus(True);
		}
		else if (
			Scr.UnknownWinFocused != None && sf != NULL &&
			sf == Scr.StolenFocusFvwmWin)
		{
			__refocus_stolen_focus_win(ea);
		}
		if (Scr.ColormapFocus == COLORMAP_FOLLOWS_MOUSE)
		{
			InstallWindowColormaps(NULL);
		}
		focus_grab_buttons(lf);
		return;
	}
	else
	{
		Scr.flags.is_pointer_on_this_screen = 1;
	}

	/* An EnterEvent in one of the PanFrameWindows activates the Paging or
	   an EdgeCommand. */
	if (is_pan_frame(ewp->window))
	{
		char *edge_command = NULL;

		if (
			Scr.UnknownWinFocused != None &&
			(sf = get_focus_window()) != NULL &&
			sf == Scr.StolenFocusFvwmWin)
		{
			__refocus_stolen_focus_win(ea);
		}
		/* check for edge commands */
		if (ewp->window == Scr.PanFrameTop.win)
		{
			edge_command = Scr.PanFrameTop.command;
		}
		else if (ewp->window == Scr.PanFrameBottom.win)
		{
			edge_command = Scr.PanFrameBottom.command;
		}
		else if (ewp->window == Scr.PanFrameLeft.win)
		{
			edge_command = Scr.PanFrameLeft.command;
		}
		else if (ewp->window == Scr.PanFrameRight.win)
		{
			edge_command = Scr.PanFrameRight.command;
		}
		if (edge_command && ewp->mode == NotifyUngrab &&
		    ewp->detail == NotifyAncestor)
		{
			/* nothing */
		}
		else if (edge_command)
		{
			execute_function(NULL, ea->exc, edge_command, 0);
		}
		else
		{
			/* no edge command for this pan frame - so we do
			 * HandlePaging */
			int delta_x = 0;
			int delta_y = 0;
			XEvent e;

			/* this was in the HandleMotionNotify before, HEDU */
			Scr.flags.is_pointer_on_this_screen = 1;
			e = *te;
			HandlePaging(
				&e, Scr.EdgeScrollX, Scr.EdgeScrollY, &JunkX,
				&JunkY, &delta_x, &delta_y, True, True, False,
				Scr.ScrollDelay);
			return;
		}
	}
	if (!fw)
	{
		return;
	}
	if (IS_EWMH_DESKTOP(FW_W(fw)))
	{
		BroadcastPacket(
			MX_ENTER_WINDOW, 3, (long)Scr.Root, (long)NULL,
			(long)NULL);
		return;
	}
	if (ewp->window == FW_W_FRAME(fw) ||
	    ewp->window == FW_W_ICON_TITLE(fw) ||
	    ewp->window == FW_W_ICON_PIXMAP(fw))
	{
		BroadcastPacket(
			MX_ENTER_WINDOW, 3, (long)FW_W(fw),
			(long)FW_W_FRAME(fw), (unsigned long)fw);
	}
	sf = get_focus_window();
	if (sf && fw != sf && FP_DO_UNFOCUS_LEAVE(FW_FOCUS_POLICY(sf)))
	{
ENTER_DBG((stderr, "en: delete focus\n"));
		DeleteFocus(True);
	}
	focus_grab_buttons(fw);
	if (FP_DO_FOCUS_ENTER(FW_FOCUS_POLICY(fw)))
	{
ENTER_DBG((stderr, "en: set mousey focus\n"));
                if (ewp->window == FW_W(fw))
                {
			/*  Event is for the client window...*/
#ifndef EXPERIMENTAL_ROU_HANDLING_V2
			/*  RBW --  This may still be needed at times, I'm not
			 *sure yet.  */
			SetFocusWindowClientEntered(
				fw, True, FOCUS_SET_BY_ENTER);
#else
			SetFocusWindow(fw, True, FOCUS_SET_BY_ENTER);
#endif
                }
                else
                {
			/*  Event is for the frame...*/
			SetFocusWindow(fw, True, FOCUS_SET_BY_ENTER);
                }
	}
	else if (focus_is_focused(fw) && focus_does_accept_input_focus(fw))
	{
		/* We have to refresh the focus window here in case we left the
		 * focused fvwm window.  Motif apps may lose the input focus
		 * otherwise.  But do not try to refresh the focus of
		 * applications that want to handle it themselves. */
		focus_force_refresh_focus(fw);
	}
	else if (sf != fw)
	{
		/* Give the window a chance to grab the buttons needed for
		 * raise-on-click */
		focus_grab_buttons(sf);
	}
	if (
		Scr.UnknownWinFocused != None && sf != NULL &&
		sf == Scr.StolenFocusFvwmWin)
	{
			__refocus_stolen_focus_win(ea);
	}
	/* We get an EnterNotify with mode == UnGrab when fvwm releases the
	 * grab held during iconification. We have to ignore this, or icon
	 * title will be initially raised. */
	if (IS_ICONIFIED(fw) && (ewp->mode == NotifyNormal) &&
	    (ewp->window == FW_W_ICON_PIXMAP(fw) ||
	     ewp->window == FW_W_ICON_TITLE(fw)) &&
	    FW_W_ICON_PIXMAP(fw) != None)
	{
		SET_ICON_ENTERED(fw, 1);
		DrawIconWindow(fw, True, False, False, False, NULL);
	}
	/* Check for tear off menus */
	if (is_tear_off_menu)
	{
		menu_enter_tear_off_menu(ea->exc);
	}

	return;
}

void HandleExpose(const evh_args_t *ea)
{
	XEvent e;
	FvwmWindow * const fw = ea->exc->w.fw;

	e = *ea->exc->x.etrigger;
#if 0
	/* This doesn't work well. Sometimes, the expose count is zero although
	 * dozens of expose events are pending.  This happens all the time
	 * during a shading animation.  Simply flush expose events
	 * unconditionally. */
	if (e.xexpose.count != 0)
	{
		flush_accumulate_expose(e.xexpose.window, &e);
	}
#else
	flush_accumulate_expose(e.xexpose.window, &e);
#endif
	if (fw == NULL)
	{
		return;
	}
	if (e.xany.window == FW_W_ICON_TITLE(fw) ||
	    e.xany.window == FW_W_ICON_PIXMAP(fw))
	{
		DrawIconWindow(fw, True, True, False, False, &e);
		return;
	}
	else if (IS_TEAR_OFF_MENU(fw) && e.xany.window == FW_W(fw))
	{
		/* refresh the contents of the torn out menu */
		menu_expose(&e, NULL);
	}

	return;
}

void HandleFocusIn(const evh_args_t *ea)
{
	XEvent d;
	Window w = None;
	Window focus_w = None;
	Window focus_fw = None;
	Pixel fc = 0;
	Pixel bc = 0;
	FvwmWindow *ffw_old = get_focus_window();
	FvwmWindow *sf;
	Bool do_force_broadcast = False;
	Bool is_unmanaged_focused = False;
	static Window last_focus_w = None;
	static Window last_focus_fw = None;
	static Bool was_nothing_ever_focused = True;
	FvwmWindow *fw = ea->exc->w.fw;

	DBUG("HandleFocusIn", "Routine Entered");

	Scr.focus_in_pending_window = NULL;
	/* This is a hack to make the PointerKey command work */
	if (ea->exc->x.etrigger->xfocus.detail != NotifyPointer)
	{
		/**/
		w = ea->exc->x.etrigger->xany.window;
	}
	while (FCheckTypedEvent(dpy, FocusIn, &d))
	{
		/* dito */
		if (d.xfocus.detail != NotifyPointer)
		{
			/**/
			w = d.xany.window;
		}
	}
	/* dito */
	if (w == None)
	{
		return;
	}
	/**/
	if (XFindContext(dpy, w, FvwmContext, (caddr_t *) &fw) == XCNOENT)
	{
		fw = NULL;
	}

	Scr.UnknownWinFocused = None;
	if (
		fw && Scr.focus_in_requested_window != fw &&
		!FP_DO_FOCUS_BY_PROGRAM(FW_FOCUS_POLICY(fw)) &&
		fw->focus_model == FM_GLOBALLY_ACTIVE)
	{
		if (DEBUG_GLOBALLY_ACTIVE)
		{
			fprintf(
				stderr, "prevented globally active fw %p (%s)"
				" from stealing the focus\n", fw,
				fw->name.name);
			fprintf(
				stderr, "window was %p (%s)\n",
				Scr.focus_in_pending_window,
				Scr.focus_in_pending_window ?
				Scr.focus_in_pending_window->name.name : "");
		}
		Scr.focus_in_requested_window = NULL;
		__refocus_stolen_focus_win(ea);

		return;
	}
	Scr.focus_in_pending_window = NULL;
	if (!fw)
	{
		if (w != Scr.NoFocusWin)
		{
			Scr.UnknownWinFocused = w;
			Scr.StolenFocusWin =
				(ffw_old != NULL) ? FW_W(ffw_old) : None;
			Scr.StolenFocusFvwmWin = ffw_old;
			focus_w = w;
			is_unmanaged_focused = True;
		}
		/* Only show a non-focused window as focused,
		 * if the focus is on unmanaged and flickering qt dialogs
		 * workaround is on. */
		if (!Scr.bo.do_enable_flickering_qt_dialogs_workaround ||
		    !is_unmanaged_focused)
		{
			border_draw_decorations(
				Scr.Hilite, PART_ALL, False, True, CLEAR_ALL,
				NULL, NULL);
			if (Scr.ColormapFocus == COLORMAP_FOLLOWS_FOCUS)
			{
				if ((Scr.Hilite)&&(!IS_ICONIFIED(Scr.Hilite)))
				{
					InstallWindowColormaps(Scr.Hilite);
				}
				else
				{
					InstallWindowColormaps(NULL);
				}
			}
		}
		/* Not very useful if no window that fvwm and its modules know
		 * about has the focus. */
		fc = GetColor(DEFAULT_FORE_COLOR);
		bc = GetColor(DEFAULT_BACK_COLOR);
	}
	else if (fw != Scr.Hilite ||
		 /* domivogt (16-May-2000): This check is necessary to force
		  * sending a M_FOCUS_CHANGE packet after an unmanaged window
		  * was focused. Otherwise fvwm would believe that Scr.Hilite
		  * was still focused and not send any info to the modules. */
		 last_focus_fw == None ||
		 IS_FOCUS_CHANGE_BROADCAST_PENDING(fw) ||
		 fpol_query_allow_user_focus(&FW_FOCUS_POLICY(fw)))
	{
		if (w != Scr.NoFocusWin)
		{
			Scr.StolenFocusWin = (ffw_old != NULL) ?
				FW_W(ffw_old) : None;
			Scr.StolenFocusFvwmWin = ffw_old;
		}
		do_force_broadcast = IS_FOCUS_CHANGE_BROADCAST_PENDING(fw);
		SET_FOCUS_CHANGE_BROADCAST_PENDING(fw, 0);
		if (fw != Scr.Hilite &&
			fpol_query_allow_user_focus(&FW_FOCUS_POLICY(fw)))
		{
			border_draw_decorations(
				fw, PART_ALL, True, True, CLEAR_ALL, NULL,
				NULL);
		}
		focus_w = FW_W(fw);
		focus_fw = FW_W_FRAME(fw);
		fc = fw->hicolors.fore;
		bc = fw->hicolors.back;
		set_focus_window(fw);
		if (Scr.ColormapFocus == COLORMAP_FOLLOWS_FOCUS)
		{
			if ((Scr.Hilite)&&(!IS_ICONIFIED(Scr.Hilite)))
			{
				InstallWindowColormaps(Scr.Hilite);
			}
			else
			{
				InstallWindowColormaps(NULL);
			}
		}
	}
	else
	{
		return;
	}
	if (was_nothing_ever_focused || last_focus_fw == None ||
	    focus_w != last_focus_w || focus_fw != last_focus_fw ||
	    do_force_broadcast)
	{
		if (!Scr.bo.do_enable_flickering_qt_dialogs_workaround ||
		    !is_unmanaged_focused)
		{
			BroadcastPacket(
				M_FOCUS_CHANGE, 5, (long)focus_w,
				(long)focus_fw,
				(unsigned long)IsLastFocusSetByMouse(),
				(long)fc, (long)bc);
			EWMH_SetActiveWindow(focus_w);
		}
		last_focus_w = focus_w;
		last_focus_fw = focus_fw;
		was_nothing_ever_focused = False;
	}
	if ((sf = get_focus_window()) != ffw_old)
	{
		focus_grab_buttons(sf);
		focus_grab_buttons(ffw_old);
	}

	return;
}

void HandleFocusOut(const evh_args_t *ea)
{
	if (Scr.UnknownWinFocused != None && Scr.StolenFocusWin != None &&
	    ea->exc->x.etrigger->xfocus.window == Scr.UnknownWinFocused)
	{
		__refocus_stolen_focus_win(ea);
	}

	return;
}

void __handle_key(const evh_args_t *ea, Bool is_press)
{
	char *action;
	FvwmWindow *sf;
	KeyCode kc;
	int kcontext;
	const XEvent *te = ea->exc->x.etrigger;
	const FvwmWindow * const fw = ea->exc->w.fw;
	Bool is_second_binding;
	const XClassHint *winClass1, *winClass2;
	XClassHint tmp;
	char *name1, *name2;
	const exec_context_t *exc;
	exec_context_changes_t ecc;

	PressedW = None;

	/* Here's a real hack - some systems have two keys with the
	 * same keysym and different keycodes. This converts all
	 * the cases to one keycode. */
	kc = XKeysymToKeycode(dpy, fvwm_KeycodeToKeysym(dpy, te->xkey.keycode, 0, 0));

	/* Check if there is something bound to the key */

	sf = get_focus_window();
	if (sf == NULL)
	{
		tmp.res_name = tmp.res_class = name1 = "root";
		winClass1 = &tmp;
		kcontext = C_ROOT;
	}
	else
	{
		winClass1 = &sf->class;
		name1 = sf->name.name;
		kcontext = (sf == fw ? ea->exc->w.wcontext : C_WINDOW);
	}

	if (fw == NULL)
	{
		tmp.res_name = tmp.res_class = name2 = "root";
		winClass2 = &tmp;
	}
	else
	{
		winClass2 = &fw->class;
		name2 = fw->name.name;
	}
	/* Searching the binding list with a different 'type' value
	 * (ie. BIND_KEYPRESS vs BIND_PKEYPRESS) doesn't make a difference.
	 * The different context value does though. */
	action = CheckTwoBindings(
		&is_second_binding, Scr.AllBindings, STROKE_ARG(0) kc,
		te->xkey.state, GetUnusedModifiers(), kcontext, BIND_KEYPRESS,
		winClass1, name1, ea->exc->w.wcontext, BIND_PKEYPRESS,
		winClass2, name2);

	if (action != NULL)
	{
		if (!is_press)
		{
			XAllowEvents(dpy, AsyncKeyboard, CurrentTime);
			return;
		}
		exc = ea->exc;
		if (is_second_binding == False)
		{
			ecc.w.fw = sf;
			ecc.w.wcontext = kcontext;
			exc = exc_clone_context(
				ea->exc, &ecc, ECC_FW | ECC_WCONTEXT);
		}
		execute_function(NULL, exc, action, 0);
		if (is_second_binding == False)
		{
			exc_destroy_context(exc);
		}
		XAllowEvents(dpy, AsyncKeyboard, CurrentTime);
		return;
	}

	/* if we get here, no function key was bound to the key.  Send it
	 * to the client if it was in a window we know about. */
	sf = get_focus_window();
	if (sf && te->xkey.window != FW_W(sf))
	{
		XEvent e;

		e = *te;
		e.xkey.window = FW_W(sf);
		FSendEvent(
			dpy, e.xkey.window, False,
			(is_press)? KeyPressMask:KeyReleaseMask, &e);
	}
	else if (fw && te->xkey.window != FW_W(fw))
	{
		XEvent e;

		e = *te;
		e.xkey.window = FW_W(fw);
		FSendEvent(
			dpy, e.xkey.window, False,
			(is_press)? KeyPressMask:KeyReleaseMask, &e);
	}
	XAllowEvents(dpy, AsyncKeyboard, CurrentTime);

	return;
}

void HandleKeyPress(const evh_args_t *ea)
{
	__handle_key(ea, True);
}

void HandleKeyRelease(const evh_args_t *ea)
{
	__handle_key(ea, False);
}

void HandleLeaveNotify(const evh_args_t *ea)
{
	const XLeaveWindowEvent *lwp;
	const XEvent *te = ea->exc->x.etrigger;
	FvwmWindow * const fw = ea->exc->w.fw;

	DBUG("HandleLeaveNotify", "Routine Entered");

ENTER_DBG((stderr, "-------- ln (%d): fw %p w 0x%08x sw 0x%08x mode 0x%x detail 0x%x '%s'\n", ++ecount, fw, (int)te->xcrossing.window, (int)te->xcrossing.subwindow, te->xcrossing.mode, te->xcrossing.detail, fw?fw->visible_name:"(none)"));
	lwp = &te->xcrossing;
	if (
		lwp->window == Scr.Root &&
		lwp->detail == NotifyInferior && lwp->mode == NotifyNormal)
	{
		/* pointer entered subwindow */
		BroadcastPacket(
			MX_LEAVE_WINDOW, 3, (long)Scr.Root, (long)NULL,
			(long)NULL);
	}
	else if (
		lwp->window == Scr.Root &&
		lwp->detail == NotifyNonlinearVirtual)
	{
		/* pointer left screen */
		BroadcastPacket(
			MX_LEAVE_WINDOW, 3, (long)Scr.Root, (long)NULL,
			(long)NULL);
	}
	/* Ignore LeaveNotify events while a window is resized or moved as a
	 * wire frame; otherwise the window list may be screwed up. */
	if (Scr.flags.is_wire_frame_displayed)
	{
		return;
	}
	if (lwp->mode != NotifyNormal)
	{
		/* Ignore events generated by grabbing or ungrabbing the
		 * pointer.  However, there is no way to prevent the client
		 * application from handling this event and, for example,
		 * grabbing the focus.  This will interfere with functions that
		 * transferred the focus to a different window.  It is
		 * necessary to check for LeaveNotify events on the client
		 * window too in case buttons are not grabbed on it. */
		if (lwp->mode == NotifyGrab && fw &&
		    (lwp->window == FW_W_FRAME(fw) ||
		     lwp->window == FW_W(fw) ||
		     lwp->window == FW_W_ICON_TITLE(fw) ||
		     lwp->window == FW_W_ICON_PIXMAP(fw)))
		{
ENTER_DBG((stderr, "ln: *** lgw = %p\n", fw));
			xcrossing_last_grab_window = fw;
		}
#ifdef FOCUS_EXPANDS_TITLE
		if (fw && IS_ICONIFIED(fw))
		{
			SET_ICON_ENTERED(fw, 0);
			DrawIconWindow(
				fw, True, False, False, False, NULL);
		}
#endif
		return;
	}
	/* CDE-like behaviour of raising the icon title if the icon
	   gets the focus (in particular if the cursor is over the icon) */
	if (fw && IS_ICONIFIED(fw))
	{
		SET_ICON_ENTERED(fw,0);
		DrawIconWindow(fw, True, False, False, False, NULL);
	}

	/* An LeaveEvent in one of the PanFrameWindows activates
	   an EdgeLeaveCommand. */
	if (is_pan_frame(lwp->window))
	{
		char *edge_command_leave = NULL;

		/* check for edge commands */
		if (lwp->window == Scr.PanFrameTop.win)
		{
			edge_command_leave = Scr.PanFrameTop.command_leave;
		}
		else if (lwp->window == Scr.PanFrameBottom.win)
		{
			edge_command_leave = Scr.PanFrameBottom.command_leave;
		}
		else if (lwp->window == Scr.PanFrameLeft.win)
		{
			edge_command_leave = Scr.PanFrameLeft.command_leave;
		}
		else if (lwp->window == Scr.PanFrameRight.win)
		{
			edge_command_leave = Scr.PanFrameRight.command_leave;
		}
		if (edge_command_leave && lwp->mode == NotifyUngrab &&
		    lwp->detail == NotifyAncestor)
		{
			/* nothing */
		}
		else if (edge_command_leave)
		{
			execute_function(NULL, ea->exc, edge_command_leave, 0);
		}
	}


	/* If we leave the root window, then we're really moving
	 * another screen on a multiple screen display, and we
	 * need to de-focus and unhighlight to make sure that we
	 * don't end up with more than one highlighted window at a time */
	if (lwp->window == Scr.Root &&
	   /* domivogt (16-May-2000): added this test because somehow fvwm
	    * sometimes gets a LeaveNotify on the root window although it is
	    * single screen. */
	    Scr.NumberOfScreens > 1)
	{
		if (lwp->mode == NotifyNormal)
		{
			if (lwp->detail != NotifyInferior)
			{
				FvwmWindow *sf = get_focus_window();

				Scr.flags.is_pointer_on_this_screen = 0;
				set_last_screen_focus_window(sf);
				if (sf != NULL)
				{
					DeleteFocus(True);
				}
				if (Scr.Hilite != NULL)
				{
					border_draw_decorations(
						Scr.Hilite, PART_ALL, False,
						True, CLEAR_ALL, NULL, NULL);
				}
			}
		}
	}
	else
	{
		/* handle a subwindow cmap */
		LeaveSubWindowColormap(te->xany.window);
	}
	if (fw != NULL &&
	    (lwp->window == FW_W_FRAME(fw) ||
	     lwp->window == FW_W_ICON_TITLE(fw) ||
	     lwp->window == FW_W_ICON_PIXMAP(fw)))
	{
		BroadcastPacket(
			MX_LEAVE_WINDOW, 3, (long)FW_W(fw),
			(long)FW_W_FRAME(fw), (unsigned long)fw);
	}

	return;
}

void HandleMapNotify(const evh_args_t *ea)
{
	Bool is_on_this_page = False;
	const XEvent *te = ea->exc->x.etrigger;
	FvwmWindow * const fw = ea->exc->w.fw;

	DBUG("HandleMapNotify", "Routine Entered");

	if (!fw)
	{
		if (te->xmap.override_redirect == True &&
		    te->xmap.window != Scr.NoFocusWin)
		{
			XSelectInput(dpy, te->xmap.window, XEVMASK_ORW);
			XFlush(dpy);
			Scr.UnknownWinFocused = te->xmap.window;
		}
		return;
	}
	if (te->xmap.window == FW_W_FRAME(fw))
	{
		/* Now that we know the frame is mapped after capturing the
		 * window we do not need StructureNotifyMask events anymore. */
		XSelectInput(dpy, FW_W_FRAME(fw), XEVMASK_FRAMEW);
		XFlush(dpy);
	}
	/* Except for identifying over-ride redirect window mappings, we
	 * don't need or want windows associated with the
	 * SubstructureNotifyMask */
	if (te->xmap.event != te->xmap.window)
	{
		return;
	}
	SET_MAP_PENDING(fw, 0);
	/* don't map if the event was caused by a de-iconify */
	if (IS_ICONIFY_PENDING(fw))
	{
		return;
	}

	/* Make sure at least part of window is on this page before giving it
	 * focus... */
	is_on_this_page = IsRectangleOnThisPage(&(fw->g.frame), fw->Desk);

	/*
	 * Need to do the grab to avoid race condition of having server send
	 * MapNotify to client before the frame gets mapped; this is bad because
	 * the client would think that the window has a chance of being viewable
	 * when it really isn't.
	 */
	MyXGrabServer (dpy);
	if (FW_W_ICON_TITLE(fw))
	{
		XUnmapWindow(dpy, FW_W_ICON_TITLE(fw));
	}
	if (FW_W_ICON_PIXMAP(fw) != None)
	{
		XUnmapWindow(dpy, FW_W_ICON_PIXMAP(fw));
	}
	XMapSubwindows(dpy, FW_W_FRAME(fw));
	if (fw->Desk == Scr.CurrentDesk)
	{
		XMapWindow(dpy, FW_W_FRAME(fw));
	}
	if (IS_ICONIFIED(fw))
	{
		BroadcastPacket(
			M_DEICONIFY, 3, (long)FW_W(fw), (long)FW_W_FRAME(fw),
			(unsigned long)fw);
	}
	else
	{
		BroadcastPacket(
			M_MAP, 3, (long)FW_W(fw), (long)FW_W_FRAME(fw),
			(unsigned long)fw);
	}

	if (is_on_this_page &&
	    focus_query_open_grab_focus(fw, get_focus_window()) == True)
	{
		SetFocusWindow(fw, True, FOCUS_SET_FORCE);
	}
	border_draw_decorations(
		fw, PART_ALL, (fw == get_focus_window()) ? True : False, True,
		CLEAR_ALL, NULL, NULL);
	MyXUngrabServer (dpy);
	SET_MAPPED(fw, 1);
	SET_ICONIFIED(fw, 0);
	SET_ICON_UNMAPPED(fw, 0);
	if (DO_ICONIFY_AFTER_MAP(fw))
	{
		initial_window_options_t win_opts;

		/* finally, if iconification was requested before the window
		 * was mapped, request it now. */
		memset(&win_opts, 0, sizeof(win_opts));
		Iconify(fw, &win_opts);
		SET_ICONIFY_AFTER_MAP(fw, 0);
	}
	focus_grab_buttons_on_layer(fw->layer);

	return;
}

void HandleMappingNotify(const evh_args_t *ea)
{
	XRefreshKeyboardMapping(&ea->exc->x.etrigger->xmapping);

	return;
}

void HandleMapRequest(const evh_args_t *ea)
{
	DBUG("HandleMapRequest", "Routine Entered");

	if (fFvwmInStartup)
	{
		/* Just map the damn thing, decorations are added later
		 * in CaptureAllWindows. */
		XMapWindow(dpy, ea->exc->x.etrigger->xmaprequest.window);
		return;
	}
	HandleMapRequestKeepRaised(ea, None, NULL, NULL);

	return;
}

void HandleMapRequestKeepRaised(
	const evh_args_t *ea, Window KeepRaised, FvwmWindow *ReuseWin,
	initial_window_options_t *win_opts)
{
	Bool is_on_this_page = False;
	Bool is_new_window = False;
	FvwmWindow *tmp;
	FvwmWindow *sf;
	initial_window_options_t win_opts_bak;
	Window ew;
	FvwmWindow *fw;
	extern Bool Restarting;
	const char *initial_map_command;

	initial_map_command = NULL;
	if (win_opts == NULL)
	{
		memset(&win_opts_bak, 0, sizeof(win_opts_bak));
		win_opts = &win_opts_bak;
	}
	ew = ea->exc->w.w;
	if (ReuseWin == NULL)
	{
		if (XFindContext(dpy, ew, FvwmContext, (caddr_t *)&fw) ==
		    XCNOENT)
		{
			fw = NULL;
		}
		if (fw != NULL && IS_MAP_PENDING(fw))
		{
			/* The window is already going to be mapped, no need to
			 * do that twice */
			return;
		}
	}
	else
	{
		fw = ReuseWin;
	}

	if (fw == NULL && EWMH_IsKdeSysTrayWindow(ew))
	{
		/* This means that the window is swallowed by kicker and that
		 * kicker restart or exit. As we should assume that kicker
		 * restart we should return here, if not we go into trouble
		 * ... */
		return;
	}
	if (!win_opts->flags.do_override_ppos)
	{
		XFlush(dpy);
	}

	/* If the window has never been mapped before ... */
	if (!fw || (fw && DO_REUSE_DESTROYED(fw)))
	{
		/* Add decorations. */
		fw = AddWindow(
			&initial_map_command, ea->exc, ReuseWin, win_opts);
		if (fw == AW_NO_WINDOW)
		{
			return;
		}
		else if (fw == AW_UNMANAGED)
		{
			XMapWindow(dpy, ew);
			return;
		}
		is_new_window = True;
	}
	/*
	 * Make sure at least part of window is on this page
	 * before giving it focus...
	 */
	is_on_this_page = IsRectangleOnThisPage(&(fw->g.frame), fw->Desk);
	if (KeepRaised != None)
	{
		XRaiseWindow(dpy, KeepRaised);
	}
	/* If it's not merely iconified, and we have hints, use them. */

	if (IS_ICONIFIED(fw))
	{
		/* If no hints, or currently an icon, just "deiconify" */
		DeIconify(fw);
	}
	else if (IS_MAPPED(fw))
	{
		/* the window is already mapped - fake a MapNotify event */
		fake_map_unmap_notify(fw, MapNotify);
	}
	else
	{
		int state;

		if (fw->wmhints && (fw->wmhints->flags & StateHint))
		{
			state = fw->wmhints->initial_state;
		}
		else
		{
			state = NormalState;
		}
		if (win_opts->initial_state != DontCareState)
		{
			state = win_opts->initial_state;
		}

		switch (state)
		{
		case DontCareState:
		case NormalState:
		case InactiveState:
		default:
			MyXGrabServer(dpy);
			if (fw->Desk == Scr.CurrentDesk)
			{
				Bool do_grab_focus;

				SET_MAP_PENDING(fw, 1);
				XMapWindow(dpy, FW_W_FRAME(fw));
				XMapWindow(dpy, FW_W(fw));
				SetMapStateProp(fw, NormalState);
				if (Scr.flags.is_map_desk_in_progress)
				{
					do_grab_focus = False;
				}
				else if (!is_on_this_page)
				{
					do_grab_focus = False;
				}
				else if (focus_query_open_grab_focus(
						 fw, get_focus_window()) ==
					 True)
				{
					do_grab_focus = True;
				}
				else
				{
					do_grab_focus = False;
				}
				if (do_grab_focus)
				{
					SetFocusWindow(
						fw, True, FOCUS_SET_FORCE);
				}
				else
				{
					/* make sure the old focused window
					 * still has grabbed all necessary
					 * buttons. */
					focus_grab_buttons(
						get_focus_window());
				}
			}
			else
			{
#ifndef ICCCM2_UNMAP_WINDOW_PATCH
				/* nope, this is forbidden by the ICCCM2 */
				XMapWindow(dpy, FW_W(fw));
				SetMapStateProp(fw, NormalState);
#else
				/* Since we will not get a MapNotify, set the
				 * IS_MAPPED flag manually. */
				SET_MAPPED(fw, 1);
				SetMapStateProp(fw, IconicState);
				/* fake that the window was mapped to allow
				 * modules to swallow it */
				BroadcastPacket(
					M_MAP, 3, (long)FW_W(fw),
					(long)FW_W_FRAME(fw),
					(unsigned long)fw);
#endif
			}
			/* TA:  20090125:  We *have* to handle
			 * InitialMapCommand here and not in AddWindow() to
			 * allow for correct timings when the window is truly
			 * mapped. (c.f. things like Iconify.)
			 */

			/* TA:  20091212:  But only do this when we're *not*
			 * restarting -- the window is still mapped, but gets
			 * recaptured -- we don't want to trigger this event
			 * again.  Otherwise we end up toggling the state of
			 * the window in situations where the
			 * InitialMapCommand is Iconify or Maximize, for
			 * instance.
			 */
			if ((initial_map_command != NULL) &&
			   (!Restarting && Scr.flags.are_windows_captured))
			{
				execute_function_override_window(
					NULL, ea->exc,
					(char *)initial_map_command, 0, fw);
			}
			MyXUngrabServer(dpy);
			break;

		case IconicState:
			if (is_new_window)
			{
				/* the window will not be mapped - fake a
				 * MapNotify and an UnmapNotify event.  Can't
				 * remember exactly why this is necessary, but
				 * probably something w/ (de)iconify state
				 * confusion. */
				fake_map_unmap_notify(fw, MapNotify);
				fake_map_unmap_notify(fw, UnmapNotify);
			}
			if (win_opts->flags.is_iconified_by_parent ||
			    ((tmp = get_transientfor_fvwmwindow(fw)) &&
			     IS_ICONIFIED(tmp)))
			{
				win_opts->flags.is_iconified_by_parent = 0;
				SET_ICONIFIED_BY_PARENT(fw, 1);
			}
			if (USE_ICON_POSITION_HINT(fw) && fw->wmhints &&
			    (fw->wmhints->flags & IconPositionHint))
			{
				win_opts->default_icon_x = fw->wmhints->icon_x;
				win_opts->default_icon_y = fw->wmhints->icon_y;
			}
			Iconify(fw, win_opts);
			break;
		}
	}
	if (IS_SHADED(fw))
	{
		BroadcastPacket(
			M_WINDOWSHADE, 3, (long)FW_W(fw), (long)FW_W_FRAME(fw),
			(unsigned long)fw);
	}
	/* If the newly mapped window overlaps the focused window, make sure
	 * ClickToFocusRaises and MouseFocusClickRaises work again. */
	sf = get_focus_window();
	if (sf != NULL)
	{
		focus_grab_buttons(sf);
	}
	if (win_opts->flags.is_menu)
	{
		SET_MAPPED(fw, 1);
		SET_MAP_PENDING(fw, 0);
	}
	EWMH_SetClientList();
	EWMH_SetClientListStacking();

	return;
}

#ifdef HAVE_STROKE
void HandleMotionNotify(const evh_args_t *ea)
{
	DBUG("HandleMotionNotify", "Routine Entered");

	if (send_motion == True)
	{
		stroke_record(
			ea->exc->x.etrigger->xmotion.x,
			ea->exc->x.etrigger->xmotion.y);
	}

	return;
}
#endif /* HAVE_STROKE */

void HandlePropertyNotify(const evh_args_t *ea)
{
	Bool has_icon_changed = False;
	Bool has_icon_pixmap_hint_changed = False;
	Bool has_icon_window_hint_changed = False;
        /* NoName is an extern pointer to a constant "Untitled".
           See lib/Flocale.c FlocaleFreeNameProperty
           to see how this initialization causes a problem: */
	FlocaleNameString new_name = { NoName, NULL };
	int old_wmhints_flags;
	const XEvent *te = ea->exc->x.etrigger;
	char *urgency_action = NULL;
	FvwmWindow * const fw = ea->exc->w.fw;

	DBUG("HandlePropertyNotify", "Routine Entered");

	if (te->xproperty.window == Scr.Root &&
	    te->xproperty.state == PropertyNewValue &&
	    (te->xproperty.atom == _XA_XSETROOT_ID ||
	     te->xproperty.atom == _XA_XROOTPMAP_ID))
	{
		/* background change */
		/* _XA_XSETROOT_ID is used by fvwm-root, xli and more (xv sends
		 * no property  notify?).  _XA_XROOTPMAP_ID is used by Esetroot
		 * compatible program: the problem here is that with some
		 * Esetroot compatible program we get the message _before_ the
		 * background change. This is fixed with Esetroot 9.2 (not yet
		 * released, 2002-01-14) */

		/* update icon window with some alpha and tear-off menu */
		FvwmWindow *t;

		for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
		{
			int cs;
			int t_cs = -1;
			int b_cs = t->icon_background_cs;
			Bool draw_picture = False;
			Bool draw_title = False;

			/* redraw ParentRelative tear-off menu */
			menu_redraw_transparent_tear_off_menu(t, True);

			if (!IS_ICONIFIED(t) || IS_ICON_SUPPRESSED(t))
			{
				continue;
			}
			if (Scr.Hilite == t)
			{
				if (t->icon_title_cs_hi >= 0)
				{
					t_cs = cs = t->icon_title_cs_hi;
				}
				else
				{
					cs = t->cs_hi;
				}
			}
			else
			{
				if (t->icon_title_cs >= 0)
				{
					t_cs = cs = t->icon_title_cs;
				}
				else
				{
					cs = t->cs;
				}
			}
			if (t->icon_alphaPixmap != None ||
			    (cs >= 0 &&
			     Colorset[cs].icon_alpha_percent < 100) ||
			    CSET_IS_TRANSPARENT_PR(b_cs) ||
			    (!IS_ICON_SHAPED(t) &&
			     t->icon_background_padding > 0))
			{
				draw_picture = True;
			}
			if (CSET_IS_TRANSPARENT_PR(t_cs))
			{
				draw_title = True;
			}
			if (draw_title || draw_picture)
			{
				DrawIconWindow(
					t, draw_title, draw_picture, False,
					draw_picture, NULL);
			}
		}
		if (te->xproperty.atom == _XA_XROOTPMAP_ID)
		{
			update_root_transparent_colorset(te->xproperty.atom);
		}
		BroadcastPropertyChange(
			MX_PROPERTY_CHANGE_BACKGROUND, 0, 0, "");
		return;
	}

	if (!fw)
	{
		return;
	}
	switch (te->xproperty.atom)
	{
	case XA_WM_TRANSIENT_FOR:
	{
		if (setup_transientfor(fw) == True)
		{
			RaiseWindow(fw, False);
		}
		break;
	}
	case XA_WM_NAME:
	{
		int changed_names;

		flush_property_notify_stop_at_event_type(
			te->xproperty.atom, FW_W(fw), 0, 0);
		if (XGetGeometry(
			    dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
			    (unsigned int*)&JunkWidth,
			    (unsigned int*)&JunkHeight,
			    (unsigned int*)&JunkBW,
			    (unsigned int*)&JunkDepth) == 0)
		{
			/* Window does not exist anymore. */
			return;
		}
		if (HAS_EWMH_WM_NAME(fw))
		{
			return;
		}
		FlocaleGetNameProperty(XGetWMName, dpy, FW_W(fw), &new_name);
		if (new_name.name == NULL)
		{
			FlocaleFreeNameProperty(&new_name);
			return;
		}
		if (strlen(new_name.name) > MAX_WINDOW_NAME_LEN)
		{
			/* limit to prevent hanging X server */
			(new_name.name)[MAX_WINDOW_NAME_LEN] = 0;
		}
		if (fw->name.name && strcmp(new_name.name, fw->name.name) == 0)
		{
			/* migo: some apps update their names every second */
			/* griph: make sure we don't free the property if it
			   is THE same name */
			if (new_name.name != fw->name.name)
			{
				FlocaleFreeNameProperty(&new_name);
			}
			return;
		}

		free_window_names(fw, True, False);
		fw->name = new_name;
		SET_NAME_CHANGED(fw, 1);
		if (fw->name.name == NULL)
		{
			fw->name.name = NoName; /* must not happen */
		}
		changed_names = 1;
		/*
		 * if the icon name is NoName, set the name of the icon to be
		 * the same as the window
		 */
		if (!WAS_ICON_NAME_PROVIDED(fw)
#if 0
		    /* dje, reported as causing various dumps.
		       I tried to debug, but so far haven't even figured out
		       how to exercise this logic. Mov 9, 2013. */
		    || (fw->icon_name.name &&
			(fw->icon_name.name != fw->name.name))
#endif
)
		{
			fw->icon_name = fw->name;
			changed_names |= 2;
		}
		update_window_names(fw, changed_names);
		break;
	}
	case XA_WM_ICON_NAME:
	{
		flush_property_notify_stop_at_event_type(
			te->xproperty.atom, FW_W(fw), 0, 0);
		if (XGetGeometry(
			    dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
			    (unsigned int*)&JunkWidth,
			    (unsigned int*)&JunkHeight,
			    (unsigned int*)&JunkBW,
			    (unsigned int*)&JunkDepth) == 0)
		{
			/* Window does not exist anymore. */
			return;
		}
		if (HAS_EWMH_WM_ICON_NAME(fw))
		{
			return;
		}
		FlocaleGetNameProperty(
			XGetWMIconName, dpy, FW_W(fw), &new_name);
		if (new_name.name == NULL)
		{
			FlocaleFreeNameProperty(&new_name);
			return;
		}
		if (new_name.name && strlen(new_name.name) > MAX_ICON_NAME_LEN)
		{
			/* limit to prevent hanging X server */
			(new_name.name)[MAX_ICON_NAME_LEN] = 0;
		}
		if (fw->icon_name.name &&
			strcmp(new_name.name, fw->icon_name.name) == 0)
		{
			/* migo: some apps update their names every second */
			/* griph: make sure we don't free the property if it
			   is THE same name */
			if (new_name.name != fw->icon_name.name)
			{
				FlocaleFreeNameProperty(&new_name);
			}
			return;
		}

		free_window_names(fw, False, True);
		fw->icon_name = new_name;
		SET_WAS_ICON_NAME_PROVIDED(fw, 1);
		if (fw->icon_name.name == NULL)
		{
			/* currently never happens */
			fw->icon_name.name = fw->name.name;
			SET_WAS_ICON_NAME_PROVIDED(fw, 0);
		}
		update_window_names(fw, 2);
		break;
	}
	case XA_WM_HINTS:
	{
		flush_property_notify_stop_at_event_type(
			te->xproperty.atom, FW_W(fw), 0, 0);
		if (XGetGeometry(
			    dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
			    (unsigned int*)&JunkWidth,
			    (unsigned int*)&JunkHeight,
			    (unsigned int*)&JunkBW,
			    (unsigned int*)&JunkDepth) == 0)
		{
			/* Window does not exist anymore. */
			return;
		}
		/* clasen@mathematik.uni-freiburg.de - 02/01/1998 - new -
		 * the urgency flag is an ICCCM 2.0 addition to the WM_HINTS.
		 */
		old_wmhints_flags = 0;
		if (fw->wmhints)
		{
			old_wmhints_flags = fw->wmhints->flags;
			XFree ((char *) fw->wmhints);
		}
		setup_wm_hints(fw);
		if (fw->wmhints == NULL)
		{
			return;
		}

		/*
		 * rebuild icon if the client either provides an icon
		 * pixmap or window or has reset the hints to `no icon'.
		 */
		if ((fw->wmhints->flags & IconPixmapHint) ||
		    (old_wmhints_flags & IconPixmapHint))
		{
ICON_DBG((stderr, "hpn: iph changed (%d) '%s'\n", !!(int)(fw->wmhints->flags & IconPixmapHint), fw->name.name));
			has_icon_pixmap_hint_changed = True;
		}
		if ((fw->wmhints->flags & IconWindowHint) ||
		    (old_wmhints_flags & IconWindowHint))
		{
ICON_DBG((stderr, "hpn: iwh changed (%d) '%s'\n", !!(int)(fw->wmhints->flags & IconWindowHint), fw->name.name));
			has_icon_window_hint_changed = True;
			SET_USE_EWMH_ICON(fw, False);
		}
		increase_icon_hint_count(fw);
		if (has_icon_window_hint_changed ||
		    has_icon_pixmap_hint_changed)
		{
			if (ICON_OVERRIDE_MODE(fw) == ICON_OVERRIDE)
			{
ICON_DBG((stderr, "hpn: icon override '%s'\n", fw->name.name));
				has_icon_changed = False;
			}
			else if (ICON_OVERRIDE_MODE(fw) ==
				 NO_ACTIVE_ICON_OVERRIDE)
			{
				if (has_icon_pixmap_hint_changed)
				{
					if (WAS_ICON_HINT_PROVIDED(fw) ==
					    ICON_HINT_MULTIPLE)
					{
ICON_DBG((stderr, "hpn: using further iph '%s'\n", fw->name.name));
						has_icon_changed = True;
					}
					else  if (fw->icon_bitmap_file ==
						  NULL ||
						  fw->icon_bitmap_file ==
						  Scr.DefaultIcon)
					{
ICON_DBG((stderr, "hpn: using first iph '%s'\n", fw->name.name));
						has_icon_changed = True;
					}
					else
					{
						/* ignore the first icon pixmap
						 * hint if the application did
						 * not provide it from the
						 * start */
ICON_DBG((stderr, "hpn: first iph ignored '%s'\n", fw->name.name));
						has_icon_changed = False;
					}
				}
				else if (has_icon_window_hint_changed)
				{
ICON_DBG((stderr, "hpn: using iwh '%s'\n", fw->name.name));
					has_icon_changed = True;
				}
				else
				{
ICON_DBG((stderr, "hpn: iwh not changed, hint ignored '%s'\n", fw->name.name));
					has_icon_changed = False;
				}
			}
			else /* NO_ICON_OVERRIDE */
			{
ICON_DBG((stderr, "hpn: using hint '%s'\n", fw->name.name));
				has_icon_changed = True;
			}

			if (USE_EWMH_ICON(fw))
			{
				has_icon_changed = False;
			}

			if (has_icon_changed)
			{
ICON_DBG((stderr, "hpn: icon changed '%s'\n", fw->name.name));
				/* Okay, the icon hint has changed and style
				 * options tell us to honour this change.  Now
				 * let's see if we have to use the application
				 * provided pixmap or window (if any), the icon
				 * file provided by the window's style or the
				 * default style's icon. */
				if (fw->icon_bitmap_file == Scr.DefaultIcon)
				{
					fw->icon_bitmap_file = NULL;
				}
				if (!fw->icon_bitmap_file &&
				    !(fw->wmhints->flags &
				      (IconPixmapHint|IconWindowHint)))
				{
					fw->icon_bitmap_file =
						(Scr.DefaultIcon) ?
						Scr.DefaultIcon : NULL;
				}
				fw->iconPixmap = (Window)NULL;
				ChangeIconPixmap(fw);
			}
		}

		/* clasen@mathematik.uni-freiburg.de - 02/01/1998 - new -
		 * the urgency flag is an ICCCM 2.0 addition to the WM_HINTS.
		 * Treat urgency changes by calling user-settable functions.
		 * These could e.g. deiconify and raise the window or
		 * temporarily change the decor. */
		if (!(old_wmhints_flags & XUrgencyHint) &&
		    (fw->wmhints->flags & XUrgencyHint))
		{
			urgency_action = "Function UrgencyFunc";
		}
		if ((old_wmhints_flags & XUrgencyHint) &&
		    !(fw->wmhints->flags & XUrgencyHint))
		{
			urgency_action = "Function UrgencyDoneFunc";
		}
		if (urgency_action)
		{
			const exec_context_t *exc;
			exec_context_changes_t ecc;

			ecc.w.fw = fw;
			ecc.w.wcontext = C_WINDOW;
			exc = exc_clone_context(
				ea->exc, &ecc, ECC_FW | ECC_WCONTEXT);
			execute_function(NULL, exc, urgency_action, 0);
			exc_destroy_context(exc);
		}
		break;
	}
	case XA_WM_NORMAL_HINTS:
		/* just mark wm normal hints as changed and look them up when
		 * the next ConfigureRequest w/ x, y, width or height set
		 * arrives. */
		SET_HAS_NEW_WM_NORMAL_HINTS(fw, 1);

		/* TA:  20120317:  Always set the size hints here, regardless
		 * of them possibly being modified by a ConfigureNotify
		 * request, due to XSizeHints disallowing resize -- FVWM would
		 * always use old values if the application decided to toggle
		 * such things, and FVWM would then never resize the window.
		 *
		 * Note that SET_HAS_NEW_WM_NORMAL_HINTS being set here to
		 * true is still valid.
		 */
		GetWindowSizeHintsWithCheck(fw, 1);
		break;
	default:
		if (te->xproperty.atom == _XA_WM_PROTOCOLS)
		{
			FetchWmProtocols (fw);
		}
		else if (te->xproperty.atom == _XA_WM_COLORMAP_WINDOWS)
		{
			FetchWmColormapWindows (fw);    /* frees old data */
			ReInstallActiveColormap();
		}
		else if (te->xproperty.atom == _XA_WM_STATE)
		{
			/*
			 * Make sure at least part of window is on this page
			 * before giving it focus...
			 */
			Bool is_on_this_page;

			is_on_this_page = IsRectangleOnThisPage(
				&(fw->g.frame), fw->Desk);
			if (fw && is_on_this_page == True &&
			    focus_is_focused(fw) &&
			    FP_DO_FOCUS_ENTER(FW_FOCUS_POLICY(fw)))
			{
				/* refresh the focus - why? */
				focus_force_refresh_focus(fw);
			}
		}
		else
		{
			EWMH_ProcessPropertyNotify(ea->exc);
		}
		break;
	}
}

void HandleReparentNotify(const evh_args_t *ea)
{
	const XEvent *te = ea->exc->x.etrigger;
	FvwmWindow * const fw = ea->exc->w.fw;

	if (!fw)
	{
		return;
	}
	if (te->xreparent.parent == Scr.Root)
	{
		/* Ignore reparenting to the root window.  In some cases these
		 * events are selected although the window is no longer
		 * managed. */
		return;
	}
	if (te->xreparent.parent != FW_W_FRAME(fw))
	{
		/* window was reparented by someone else, destroy the frame */
		SetMapStateProp(fw, WithdrawnState);
		EWMH_RestoreInitialStates(fw, te->type);
		if (!IS_TEAR_OFF_MENU(fw))
		{
			XRemoveFromSaveSet(dpy, te->xreparent.window);
			XSelectInput(dpy, te->xreparent.window, NoEventMask);
		}
		else
		{
			XSelectInput(dpy, te->xreparent.window, XEVMASK_MENUW);
		}
		XSync(dpy, 0);
		FWeedIfWindowEvents(dpy, FW_W_FRAME(fw), NULL, NULL);
		destroy_window(fw);
		EWMH_ManageKdeSysTray(te->xreparent.window, te->type);
		EWMH_WindowDestroyed();
	}

	return;
}

void HandleSelectionRequest(const evh_args_t *ea)
{
	icccm2_handle_selection_request(ea->exc->x.etrigger);

	return;
}

void HandleSelectionClear(const evh_args_t *ea)
{
	icccm2_handle_selection_clear();

	return;
}

void HandleShapeNotify(const evh_args_t *ea)
{
	FvwmWindow * const fw = ea->exc->w.fw;

	DBUG("HandleShapeNotify", "Routine Entered");

	if (FShapesSupported)
	{
		const FShapeEvent *sev =
			(const FShapeEvent *)(ea->exc->x.etrigger);

		if (!fw)
		{
			return;
		}
		if (sev->kind != FShapeBounding)
		{
			return;
		}
		frame_setup_shape(
			fw, fw->g.frame.width, fw->g.frame.height, sev->shaped);
		EWMH_SetFrameStrut(fw);
		if (!IS_ICONIFIED(fw))
		{
			border_redraw_decorations(fw);
		}
	}

	return;
}

void HandleUnmapNotify(const evh_args_t *ea)
{
	int dstx, dsty;
	Window dumwin;
	XEvent dummy;
	XEvent map_event;
	const XEvent *te = ea->exc->x.etrigger;
	Bool focus_grabbed;
	Bool must_return = False;
	Bool do_map = False;
	FvwmWindow * const fw = ea->exc->w.fw;
	Window pw;
	Window cw;

	DBUG("HandleUnmapNotify", "Routine Entered");

	if (te->xunmap.event != te->xunmap.window
	    && (te->xunmap.event != Scr.Root || !te->xunmap.send_event))
	{
		/* Nothing to do except updating some states. */
		must_return = True;
	}

	/*
	 * The July 27, 1988 ICCCM spec states that a client wishing to switch
	 * to WithdrawnState should send a synthetic UnmapNotify with the
	 * event field set to (pseudo-)root, in case the window is already
	 * unmapped (which is the case for fvwm for IconicState).
	 * Unfortunately, we looked for the FvwmContext using that field, so
	 * try the window field also. */
	if (!fw)
	{
		if (XFindContext(
			    dpy, te->xunmap.window, FvwmContext,
			    (caddr_t *)&fw) == XCNOENT)
		{
			return;
		}
	}
	cw = FW_W(fw);
	pw = FW_W_PARENT(fw);
	if (te->xunmap.window == FW_W_FRAME(fw))
	{
		SET_ICONIFY_PENDING(fw , 0);
		return;
	}
	if (must_return)
	{
		return;
	}

	if (fw ==  Scr.Hilite)
	{
		Scr.Hilite = NULL;
	}
	focus_grabbed = focus_query_close_release_focus(fw);
	restore_focus_after_unmap(fw, False);
	if (!IS_MAPPED(fw) && !IS_ICONIFIED(fw))
	{
		return;
	}

	/*
	 * The program may have unmapped the client window, from either
	 * NormalState or IconicState.  Handle the transition to WithdrawnState.
	 *
	 * We need to reparent the window back to the root (so that fvwm exiting
	 * won't cause it to get mapped) and then throw away all state (pretend
	 * that we've received a DestroyNotify).
	 */
	if (!FCheckTypedWindowEvent(
		    dpy, te->xunmap.window, DestroyNotify, &dummy) &&
	    XTranslateCoordinates(
		    dpy, te->xunmap.window, Scr.Root, 0, 0, &dstx, &dsty,
		    &dumwin))
	{
		MyXGrabServer(dpy);
		SetMapStateProp(fw, WithdrawnState);
		EWMH_RestoreInitialStates(fw, te->type);
		if (FCheckTypedWindowEvent(
			    dpy, te->xunmap.window, ReparentNotify, &dummy))
		{
			if (fw->attr_backup.border_width)
			{
				XSetWindowBorderWidth(
					dpy, te->xunmap.window,
					fw->attr_backup.border_width);
			}
			if ((!IS_ICON_SUPPRESSED(fw))&&
			   (fw->wmhints &&
			    (fw->wmhints->flags & IconWindowHint)))
			{
				XUnmapWindow(dpy, fw->wmhints->icon_window);
			}
		}
		else
		{
			RestoreWithdrawnLocation(fw, False, Scr.Root);
		}
		if (!IS_TEAR_OFF_MENU(fw))
		{
			XRemoveFromSaveSet(dpy, te->xunmap.window);
			XSelectInput(dpy, te->xunmap.window, NoEventMask);
		}
		XSync(dpy, 0);
		MyXUngrabServer(dpy);
		if (FCheckTypedWindowEvent(dpy, pw, MapRequest, &map_event))
		{
			/* the client tried to map the window again while it
			 * was still inside the decoration windows */
		        do_map = True;
		}
	}
	destroy_window(fw);
	if (focus_grabbed == True)
	{
		CoerceEnterNotifyOnCurrentWindow();
	}
	EWMH_ManageKdeSysTray(te->xunmap.window, te->type);
	EWMH_WindowDestroyed();
	if (do_map == True)
	{
		map_event.xmaprequest.window = cw;
		map_event.xmaprequest.parent = Scr.Root;
		dispatch_event(&map_event);
		/* note: we really should handle all map and unmap notify
		 * events for that window in a loop here */
	}

	return;
}

void HandleVisibilityNotify(const evh_args_t *ea)
{
	FvwmWindow * const fw = ea->exc->w.fw;

	DBUG("HandleVisibilityNotify", "Routine Entered");

	if (fw && ea->exc->x.etrigger->xvisibility.window == FW_W_FRAME(fw))
	{
		switch (ea->exc->x.etrigger->xvisibility.state)
		{
		case VisibilityUnobscured:
			SET_FULLY_VISIBLE(fw, 1);
			SET_PARTIALLY_VISIBLE(fw, 1);
			break;
		case VisibilityPartiallyObscured:
			SET_FULLY_VISIBLE(fw, 0);
			SET_PARTIALLY_VISIBLE(fw, 1);
			break;
		default:
			SET_FULLY_VISIBLE(fw, 0);
			SET_PARTIALLY_VISIBLE(fw, 0);
			break;
		}
		/* Make sure the button grabs are up to date */
		focus_grab_buttons(fw);
	}

	return;
}

/* ---------------------------- interface functions ------------------------ */

/* Inform a client window of its geometry.
 *
 *  The input (frame) geometry will be translated to client geometry
 *  before sending. */
void SendConfigureNotify(
	FvwmWindow *fw, int x, int y, int w, int h, int bw,
	Bool send_for_frame_too)
{
	XEvent client_event;
	size_borders b;

	if (!fw || IS_SHADED(fw))
	{
		return;
	}
	client_event.type = ConfigureNotify;
	client_event.xconfigure.display = dpy;
	client_event.xconfigure.event = FW_W(fw);
	client_event.xconfigure.window = FW_W(fw);
	get_window_borders(fw, &b);
	client_event.xconfigure.x = x + b.top_left.width;
	client_event.xconfigure.y = y + b.top_left.height;
	client_event.xconfigure.width = w - b.total_size.width;
	client_event.xconfigure.height = h - b.total_size.height;
	client_event.xconfigure.border_width = bw;
	client_event.xconfigure.above = FW_W_FRAME(fw);
	client_event.xconfigure.override_redirect = False;
#if 0
	fprintf(stderr,
		"send cn: %d %d %dx%d fw 0x%08x w 0x%08x ew 0x%08x  '%s'\n",
		client_event.xconfigure.x, client_event.xconfigure.y,
		client_event.xconfigure.width, client_event.xconfigure.height,
		(int)FW_W_FRAME(fw), (int)FW_W(fw),
		(int)client_event.xconfigure.window,
		(fw->name.name) ? fw->name.name : "");
#endif
	fev_sanitise_configure_notify(&client_event.xconfigure);
	FSendEvent(
		dpy, FW_W(fw), False, StructureNotifyMask, &client_event);
	if (send_for_frame_too)
	{
		/* This is for buggy tk, which waits for the real
		 * ConfigureNotify on frame instead of the synthetic one on w.
		 * The geometry data in the event will not be correct for the
		 * frame, but tk doesn't look at that data anyway. */
		client_event.xconfigure.event = FW_W_FRAME(fw);
		client_event.xconfigure.window = FW_W_FRAME(fw);
		FSendEvent(
			dpy, FW_W_FRAME(fw), False, StructureNotifyMask,
			&client_event);
	}

	return;
}

/* Add an event group to the event handler */
int register_event_group(int event_base, int event_count, PFEH *jump_table)
{
	/* insert into the list */
	event_group_t *group;
	event_group_t *position = base_event_group;
	event_group_t *prev_position = NULL;

	while (
		position != NULL &&
		position->base + position->count < event_base)
	{
		prev_position = position;
		position = position->next;
	}
	if ((position != NULL && position->base < event_base + event_count))
	{
		/* there is already an event group registered at the specified
		 * event range, or the base is before the base X events */

		return 1;
	}
	/* create the group structure (these are not freed until fvwm exits) */
	group = xmalloc(sizeof(event_group_t));
	group->base = event_base;
	group->count = event_count;
	group->jump_table = jump_table;
	group->next = position;
	if (prev_position != NULL)
	{
		prev_position->next = group;
	}
	else
	{
		base_event_group = group;
	}

	return 0;
}

/*
** Procedure:
**   InitEventHandlerJumpTable
*/
void InitEventHandlerJumpTable(void)
{
	static PFEH EventHandlerJumpTable[LASTEvent];
	int i;

	for (i=0; i<LASTEvent; i++)
	{
		EventHandlerJumpTable[i] = NULL;
	}
	EventHandlerJumpTable[Expose] =           HandleExpose;
	EventHandlerJumpTable[DestroyNotify] =    HandleDestroyNotify;
	EventHandlerJumpTable[MapRequest] =       HandleMapRequest;
	EventHandlerJumpTable[MapNotify] =        HandleMapNotify;
	EventHandlerJumpTable[UnmapNotify] =      HandleUnmapNotify;
	EventHandlerJumpTable[ButtonPress] =      HandleButtonPress;
	EventHandlerJumpTable[EnterNotify] =      HandleEnterNotify;
	EventHandlerJumpTable[LeaveNotify] =      HandleLeaveNotify;
	EventHandlerJumpTable[FocusIn] =          HandleFocusIn;
	EventHandlerJumpTable[FocusOut] =         HandleFocusOut;
	EventHandlerJumpTable[ConfigureRequest] = HandleConfigureRequest;
	EventHandlerJumpTable[ClientMessage] =    HandleClientMessage;
	EventHandlerJumpTable[PropertyNotify] =   HandlePropertyNotify;
	EventHandlerJumpTable[KeyPress] =         HandleKeyPress;
	EventHandlerJumpTable[KeyRelease] =       HandleKeyRelease;
	EventHandlerJumpTable[VisibilityNotify] = HandleVisibilityNotify;
	EventHandlerJumpTable[ColormapNotify] =   HandleColormapNotify;
	EventHandlerJumpTable[SelectionClear]   = HandleSelectionClear;
	EventHandlerJumpTable[SelectionRequest] = HandleSelectionRequest;
	EventHandlerJumpTable[ReparentNotify] =   HandleReparentNotify;
	EventHandlerJumpTable[MappingNotify] =    HandleMappingNotify;
	STROKE_CODE(EventHandlerJumpTable[ButtonRelease] = HandleButtonRelease);
	STROKE_CODE(EventHandlerJumpTable[MotionNotify] = HandleMotionNotify);
#ifdef MOUSE_DROPPINGS
	STROKE_CODE(stroke_init(dpy,DefaultRootWindow(dpy)));
#else /* no MOUSE_DROPPINGS */
	STROKE_CODE(stroke_init());
#endif /* MOUSE_DROPPINGS */
	if (register_event_group(0, LASTEvent, EventHandlerJumpTable))
	{
		/* should never happen */
		fvwm_msg(ERR, "InitEventHandlerJumpTable",
			 "Failed to initialize event handlers");
		exit(1);
	}
	if (FShapesSupported)
	{
		static PFEH shape_jump_table[FShapeNumberEvents];

		for (i = 0; i < FShapeNumberEvents; i++)
		{
			shape_jump_table[i] = NULL;
		}
		shape_jump_table[FShapeNotify] = HandleShapeNotify;
		if (
			register_event_group(
				FShapeEventBase, FShapeNumberEvents,
				shape_jump_table))
		{
			fvwm_msg(ERR, "InitEventHandlerJumpTable",
				 "Failed to init Shape event handler");
		}
	}


	return;
}

/* handle a single X event */
void dispatch_event(XEvent *e)
{
	Window w = e->xany.window;
	FvwmWindow *fw;
	event_group_t *event_group;

	DBUG("dispatch_event", "Routine Entered");

	XFlush(dpy);
	if (w == Scr.Root)
	{
		switch (e->type)
		{
		case ButtonPress:
		case ButtonRelease:
			if (e->xbutton.subwindow != None)
			{
				w = e->xbutton.subwindow;
			}
		case MapRequest:
			w = e->xmaprequest.window;
			break;
		default:
			break;
		}
	}
	if (w == Scr.Root ||
	    XFindContext(dpy, w, FvwmContext, (caddr_t *)&fw) == XCNOENT)
	{
		fw = NULL;
	}
	if (
		fw != NULL && IS_SCHEDULED_FOR_DESTROY(fw) &&
		e->type != DestroyNotify)
	{
		return;
	}
	last_event_type = e->type;
	event_group = base_event_group;
	while (
		event_group != NULL &&
		event_group->base + event_group->count < e->type)
	{
		event_group = event_group->next;
	}

	if (
		event_group != NULL &&
		e->type - event_group->base < event_group->count &&
		event_group->jump_table[e->type - event_group->base] != NULL)
	{
		evh_args_t ea;
		exec_context_changes_t ecc;
		Window dummyw;

		ecc.type = EXCT_EVENT;
		ecc.x.etrigger = e;
		ecc.w.wcontext = GetContext(&fw, fw, e, &dummyw);
		ecc.w.w = w;
		ecc.w.fw = fw;
		ea.exc = exc_create_context(
			&ecc, ECC_TYPE | ECC_ETRIGGER | ECC_FW | ECC_W |
			ECC_WCONTEXT);
		(*event_group->jump_table[e->type - event_group->base])(&ea);
		exc_destroy_context(ea.exc);
	}

#ifdef C_ALLOCA
	/* If we're using the C version of alloca, see if anything needs to be
	 * freed up.
	 */
	alloca(0);
#endif
	DBUG("dispatch_event", "Leaving Routine");

	return;
}

/* ewmh configure request */
void events_handle_configure_request(
	XEvent *cre, FvwmWindow *fw, Bool force, int force_gravity)
{
	_handle_configure_request(cre, NULL, fw, force, force_gravity);

	return;
}

void HandleEvents(void)
{
	XEvent ev;

	DBUG("HandleEvents", "Routine Entered");
	STROKE_CODE(send_motion = False);
	while (!isTerminated)
	{
		last_event_type = 0;
		if (Scr.flags.is_window_scheduled_for_destroy)
		{
			destroy_scheduled_windows();
		}
		if (Scr.flags.do_need_window_update)
		{
			flush_window_updates();
		}
		if (My_XNextEvent(dpy, &ev))
		{
			/* DV (19-Sep-2014): We mark events as invalid by
			 * setting the send_event field to a bogus value in a
			 * predicate procedure.  It's unsure whether this works
			 * reliably.  */
			if (FEV_IS_EVENT_INVALID(ev))
			{
				continue;
			}
			dispatch_event(&ev);
		}
		if (Scr.flags.do_need_style_list_update)
		{
			simplify_style_list();
		}
	}

	return;
}

/*
 *
 * Waits for next X or module event, fires off startup routines when startup
 * modules have finished or after a timeout if the user has specified a
 * command line module that doesn't quit or gets stuck.
 *
 */
int My_XNextEvent(Display *dpy, XEvent *event)
{
	fd_set in_fdset, out_fdset;
	int num_fd;
	fmodule_list_itr moditr;
	fmodule *module;
	fmodule_input *input;
	static struct timeval timeout;
	static struct timeval *timeoutP = &timeout;

	DBUG("My_XNextEvent", "Routine Entered");

	/* check for any X events already queued up.
	 * Side effect: this does an XFlush if no events are queued
	 * Make sure nothing between here and the select causes further
	 * X requests to be sent or the select may block even though
	 * there are events in the queue */
	if (FPending(dpy))
	{
		DBUG(
			"My_XNextEvent", "taking care of queued up events"
			" & returning (1)");
		FNextEvent(dpy, event);
		return 1;
	}

	/* check for termination of all startup modules */
	if (fFvwmInStartup)
	{
		module_list_itr_init(&moditr);
		module = module_list_itr_next(&moditr);
		for (; module != NULL; module = module_list_itr_next(&moditr))
		{
			if (MOD_IS_CMDLINE(module) == 1)
			{
				break;
			}
		}
		module_cleanup();
		if (module == NULL)
		{
			/* last module */
			DBUG(
				"My_XNextEvent",
				"Starting up after command lines modules");
			/* set an infinite timeout to stop ticking */
			timeoutP = NULL;
			/* This may cause X requests to be sent */
			StartupStuff();

			return 0; /* so return without select()ing */
		}
	}

	/* Some signals can interrupt us while we wait for any action
	 * on our descriptors. While some of these signals may be asking
	 * fvwm to die, some might be harmless. Harmless interruptions
	 * mean we have to start waiting all over again ... */
	do
	{
		int ms;
		Bool is_waiting_for_scheduled_command = False;
		static struct timeval *old_timeoutP = NULL;

		/* The timeouts become undefined whenever the select returns,
		 * and so we have to reinitialise them */
		ms = squeue_get_next_ms();
		if (ms == 0)
		{
			/* run scheduled commands */
			squeue_execute();
			ms = squeue_get_next_ms();
			/* should not happen anyway.
			 * get_next_schedule_queue_ms() can't return 0 after a
			 * call to execute_schedule_queue(). */
			if (ms == 0)
			{
				ms = 1;
			}
		}
		if (ms < 0)
		{
			timeout.tv_sec = 42;
			timeout.tv_usec = 0;
		}
		else
		{
			/* scheduled commands are pending - don't wait too
			 * long */
			timeout.tv_sec = ms / 1000;
			timeout.tv_usec = 1000 * (ms % 1000);
			old_timeoutP = timeoutP;
			timeoutP = &timeout;
			is_waiting_for_scheduled_command = True;
		}

		FD_ZERO(&in_fdset);
		FD_ZERO(&out_fdset);
		FD_SET(x_fd, &in_fdset);

		/* nothing is done here if fvwm was compiled without session
		 * support */
		if (sm_fd >= 0)
		{
			FD_SET(sm_fd, &in_fdset);
		}

		module_list_itr_init(&moditr);
		while ( (module = module_list_itr_next(&moditr)) != NULL)
		{
			FD_SET(MOD_READFD(module), &in_fdset);

			if (!FQUEUE_IS_EMPTY(&MOD_PIPEQUEUE(module)))
			{
				FD_SET(MOD_WRITEFD(module), &out_fdset);
			}
		}

		DBUG("My_XNextEvent", "waiting for module input/output");
		num_fd = fvwmSelect(
			fvwmlib_max_fd, &in_fdset, &out_fdset, 0, timeoutP);
		if (is_waiting_for_scheduled_command)
		{
			timeoutP = old_timeoutP;
		}

		/* Express route out of fvwm ... */
		if (isTerminated)
		{
			return 0;
		}
	} while (num_fd < 0);

	if (num_fd > 0)
	{
		/* Check for module input. */
		module_list_itr_init(&moditr);
		while ( (module = module_list_itr_next(&moditr)) != NULL)
		{
			if (FD_ISSET(MOD_READFD(module), &in_fdset))
			{
				input = module_receive(module);
				/* enqueue the received command */
				module_input_enqueue(input);
			}
			if (
				MOD_WRITEFD(module) >= 0 &&
				FD_ISSET(MOD_WRITEFD(module), &out_fdset))
			{
				DBUG("My_XNextEvent",
				     "calling FlushMessageQueue");
				FlushMessageQueue(module);
			}
		}

		/* execute any commands queued up */
		DBUG("My_XNextEvent", "executing module comand queue");
		ExecuteCommandQueue();

		/* cleanup dead modules */
		module_cleanup();

		/* nothing is done here if fvwm was compiled without session
		 * support */
		if ((sm_fd >= 0) && (FD_ISSET(sm_fd, &in_fdset)))
		{
			ProcessICEMsgs();
		}

	}
	else
	{
		/* select has timed out, things must have calmed down so let's
		 * decorate */
		if (fFvwmInStartup)
		{
			fvwm_msg(ERR, "My_XNextEvent",
				 "Some command line modules have not quit, "
				 "Starting up after timeout.\n");
			StartupStuff();
			timeoutP = NULL; /* set an infinite timeout to stop
					  * ticking */
			reset_style_changes();
			Scr.flags.do_need_window_update = 0;
		}
		/* run scheduled commands if necessary */
		squeue_execute();
	}

	/* check for X events again, rather than return 0 and get called again
	 */
	if (FPending(dpy))
	{
		DBUG("My_XNextEvent",
		     "taking care of queued up events & returning (2)");
		FNextEvent(dpy,event);
		return 1;
	}

	DBUG("My_XNextEvent", "leaving My_XNextEvent");
	return 0;
}

/*
 *
 *  Procedure:
 *      Find the Fvwm context for the event.
 *
 */
int GetContext(FvwmWindow **ret_fw, FvwmWindow *t, const XEvent *e, Window *w)
{
	int context;
	Window win;
	Window subw = None;
	int x = 0;
	int y = 0;
	Bool is_key_event = False;

	win = e->xany.window;
	context = C_NO_CONTEXT;
	switch (e->type)
	{
	case KeyPress:
	case KeyRelease:
		x = e->xkey.x;
		y = e->xkey.y;
		subw = e->xkey.subwindow;
		if (win == Scr.Root && subw != None)
		{
			/* Translate root coordinates into subwindow
			 * coordinates.  Necessary for key bindings that work
			 * over unfocused windows. */
			win = subw;
			XTranslateCoordinates(
				dpy, Scr.Root, subw, x, y, &x, &y, &subw);
			XFindContext(dpy, win, FvwmContext, (caddr_t *) &t);
		}
		is_key_event = True;
		/* fall through */
	case ButtonPress:
	case ButtonRelease:
		if (!is_key_event)
		{
			x = e->xbutton.x;
			y = e->xbutton.y;
			subw = e->xbutton.subwindow;
		}
		if (t && win == FW_W_FRAME(t) && subw != None)
		{
			/* Translate frame coordinates into subwindow
			 * coordinates. */
			win = subw;
			XTranslateCoordinates(
				dpy, FW_W_FRAME(t), subw, x, y, &x, &y, &subw);
			if (win == FW_W_PARENT(t))
			{
				win = subw;
				XTranslateCoordinates(
					dpy, FW_W_PARENT(t), subw, x, y, &x,
					&y, &subw);
			}
		}
		break;
	default:
		XFindContext(dpy, win, FvwmContext, (caddr_t *)&t);
		break;
	}
	if (ret_fw != NULL)
	{
		*ret_fw = t;
	}
	if (!t)
	{
		return C_ROOT;
	}
	*w = win;
	if (*w == Scr.NoFocusWin)
	{
		return C_ROOT;
	}
	if (subw != None)
	{
		if (win == FW_W_PARENT(t))
		{
			*w = subw;
		}
	}
	if (*w == Scr.Root)
	{
		return C_ROOT;
	}
	context = frame_window_id_to_context(t, *w, &Button);

	return context;
}

/* Drops all expose events for the given window from the input queue and merges
 * the expose rectangles into a single big one (*e). */
void flush_accumulate_expose(Window w, XEvent *e)
{
	FWeedIfWindowEvents(dpy, w, _pred_weed_accumulate_expose, (XPointer)e);

	return;
}

/*
 *
 * Removes all expose events from the queue and does the necessary redraws
 *
 */
void handle_all_expose(void)
{
	void *saved_event;

	saved_event = fev_save_event();
	FPending(dpy);
	FWeedIfEvents(dpy, _pred_weed_handle_expose, NULL);
	fev_restore_event(saved_event);

	return;
}

/* CoerceEnterNotifyOnCurrentWindow()
 * Pretends to get a HandleEnterNotify on the window that the pointer
 * currently is in so that the focus gets set correctly from the beginning.
 * Note that this presently only works if the current window is not
 * click_to_focus;  I think that that behaviour is correct and desirable.
 * --11/08/97 gjb */
void CoerceEnterNotifyOnCurrentWindow(void)
{
	Window child;
	Window root;
	Bool f;
	evh_args_t ea;
	exec_context_changes_t ecc;
	XEvent e;
	FvwmWindow *fw;

	f = FQueryPointer(
		dpy, Scr.Root, &root, &child, &e.xcrossing.x_root,
		&e.xcrossing.y_root, &e.xcrossing.x, &e.xcrossing.y,
		&JunkMask);
	if (f == False || child == None)
	{
		return;
	}
	e.xcrossing.type = EnterNotify;
	e.xcrossing.window = child;
	e.xcrossing.subwindow = None;
	e.xcrossing.mode = NotifyNormal;
	e.xcrossing.detail = NotifyAncestor;
	e.xcrossing.same_screen = True;
	if (XFindContext(dpy, child, FvwmContext, (caddr_t *)&fw) == XCNOENT)
	{
		fw = NULL;
	}
	else
	{
		XTranslateCoordinates(
			dpy, Scr.Root, child, e.xcrossing.x_root,
			e.xcrossing.y_root, &JunkX, &JunkY, &child);
		if (child == FW_W_PARENT(fw))
		{
			child = FW_W(fw);
		}
		if (child != None)
		{
			e.xany.window = child;
		}
	}
	e.xcrossing.focus = (fw == get_focus_window()) ? True : False;
	ecc.type = EXCT_NULL;
	ecc.x.etrigger = &e;
	ea.exc = exc_create_context(&ecc, ECC_TYPE | ECC_ETRIGGER);
	HandleEnterNotify(&ea);
	exc_destroy_context(ea.exc);

	return;
}

/* This function discards all queued up events selected by the mask. */
int discard_typed_events(int num_event_types, int *event_types)
{
	_weed_event_type_arg args;
	int count;
	int i;

	XSync(dpy, 0);
	assert(num_event_types <= MAX_NUM_WEED_EVENT_TYPES);
	args.num_event_types = num_event_types;
	for (i = 0; i < num_event_types; i++)
	{
		args.event_types[i] = event_types[i];
	}
	count = FWeedIfEvents(dpy, _pred_weed_event_type, (XPointer)&args);

	return count;
}

/* Similar function for certain types of PropertyNotify. */
int flush_property_notify_stop_at_event_type(
	Atom atom, Window w, char do_stop_at_event_type,
	int stop_at_event_type)
{
	flush_property_notify_args args;

	XSync(dpy, 0);
	args.w = w;
	args.atom = atom;
	args.event_type = PropertyNotify;
	args.stop_at_event_type = stop_at_event_type;
	args.do_stop_at_event_type = do_stop_at_event_type;
	FWeedIfEvents(
		dpy, _pred_flush_property_notify_weed, (XPointer)&args);

	return 0;
}

/* Wait for all mouse buttons to be released
 * This can ease some confusion on the part of the user sometimes
 *
 * Discard superflous button events during this wait period. */
void WaitForButtonsUp(Bool do_handle_expose)
{
	unsigned int mask;
	unsigned int bmask;
	long evmask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
		KeyPressMask|KeyReleaseMask;
	int count;
	int use_wait_cursor;
	XEvent e;

	if (FQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX, &JunkY,
			  &JunkX, &JunkY, &mask) == False)
	{
		/* pointer is on a different screen - that's okay here */
	}
	mask &= DEFAULT_ALL_BUTTONS_MASK;
	if (mask == 0)
	{
		return;
	}
	if (do_handle_expose)
	{
		evmask |= ExposureMask;
	}
	GrabEm(None, GRAB_NORMAL);
	for (count = 0, use_wait_cursor = 0; mask != 0; count++)
	{
		/* handle expose events */
		XAllowEvents(dpy, SyncPointer, CurrentTime);
		if (FCheckMaskEvent(dpy, evmask, &e))
		{
			switch (e.type)
			{
			case ButtonRelease:
				if (e.xbutton.button <=
				    NUMBER_OF_MOUSE_BUTTONS)
				{
					bmask = (Button1Mask <<
						 (e.xbutton.button - 1));
					mask = e.xbutton.state & ~bmask;
				}
				break;
			case Expose:
				dispatch_event(&e);
				break;
			default:
				break;
			}
		}
		else
		{
			if (FQueryPointer(
				    dpy, Scr.Root, &JunkRoot, &JunkChild,
				    &JunkX, &JunkY, &JunkX, &JunkY, &mask) ==
			    False)
			{
				/* pointer is on a different screen - that's
				 * okay here */
			}
			mask &= DEFAULT_ALL_BUTTONS_MASK;
			usleep(1);
		}
		if (use_wait_cursor == 0 && count == 20)
		{
			GrabEm(CRS_WAIT, GRAB_NORMAL);
			use_wait_cursor = 1;
		}
	}
	UngrabEm(GRAB_NORMAL);
	if (use_wait_cursor)
	{
		UngrabEm(GRAB_NORMAL);
		XFlush(dpy);
	}

	return;
}

void sync_server(int toggle)
{
	static Bool synced = False;

	if (toggle == -1)
	{
		toggle = (synced == False);
	}
	if (toggle == 1)
	{
		synced = True;
	}
	else
	{
		synced = False;
	}
	XSynchronize(dpy, synced);
	XFlush(dpy);

	return;
}

Bool is_resizing_event_pending(FvwmWindow *fw)
{
	XEvent e;
	check_if_event_args args;

	args.w = FW_W(fw);
	args.do_return_true = False;
	args.do_return_true_cr = False;
	args.cr_value_mask = 0;
	args.ret_does_match = False;
	args.ret_type = 0;
	FCheckIfEvent(dpy, &e, test_resizing_event, (XPointer)&args);

	return args.ret_does_match;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_XSynchronize(F_CMD_ARGS)
{
	int toggle;

	toggle = ParseToggleArgument(action, NULL, -1, 0);
	sync_server(toggle);

	return;
}

void CMD_XSync(F_CMD_ARGS)
{
	XSync(dpy, 0);

	return;
}
