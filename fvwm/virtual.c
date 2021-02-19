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

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/FGettext.h"
#include "libs/Grab.h"
#include "libs/Parse.h"
#include "libs/FEvent.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "cursor.h"
#include "events.h"
#include "eventmask.h"
#include "misc.h"
#include "screen.h"
#include "virtual.h"
#include "module_interface.h"
#include "module_list.h"
#include "focus.h"
#include "ewmh.h"
#include "move_resize.h"
#include "borders.h"
#include "frame.h"
#include "geometry.h"
#include "icons.h"
#include "stack.h"
#include "functions.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */
#ifndef XUrgencyHint
#define XUrgencyHint            (1L << 8)
#endif
/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/*
 * dje 12/19/98
 *
 * Testing with edge_thickness of 1 showed that some XServers don't
 * care for 1 pixel windows, causing EdgeScrolling  not to work with some
 * servers.  One  bug report was for SUNOS  4.1.3_U1.  We didn't find out
 * the exact  cause of the problem.  Perhaps  no enter/leave  events were
 * generated.
 *
 * Allowed: 0,1,or 2 pixel pan frames.
 *
 * 0   completely disables mouse  edge scrolling,  even  while dragging a
 * window.
 *
 * 1 gives the  smallest pan frames,  which seem to  work best  except on
 * some servers.
 *
 * 2 is the default.
 */
static int edge_thickness = 2;
static int last_edge_thickness = 2;

static void store_desktop_cmd(int, char *);
static int number_of_desktops(struct monitor *);
static bool should_free_panframe(PanFrame *);

struct desktop_cmds	 desktop_cmd_q;
struct desktop_fws	 desktop_fvwm_q;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */


static Bool _pred_button_event(Display *display, XEvent *event, XPointer arg)
{
	return (event->type == ButtonPress || event->type == ButtonRelease) ?
		True : False;
}

static void __drag_viewport(const exec_context_t *exc, int scroll_speed)
{
	XEvent e;
	int x;
	int y;
	unsigned int button_mask = 0;
	Bool is_finished = False;
	struct monitor	*m = monitor_get_current();

	if (!GrabEm(CRS_MOVE, GRAB_NORMAL))
	{
		XBell(dpy, 0);
		return;
	}

	if (FQueryPointer(
		    dpy, Scr.Root, &JunkRoot, &JunkChild, &x, &y,
		    &JunkX, &JunkY, &button_mask) == False)
	{
		/* pointer is on a different screen */
		/* Is this the best thing to do? */
		UngrabEm(GRAB_NORMAL);
		return;
	}
	MyXGrabKeyboard(dpy);
	button_mask &= DEFAULT_ALL_BUTTONS_MASK;
	memset(&e, 0, sizeof(e));
	while (!is_finished)
	{
		int old_x;
		int old_y;

		old_x = x;
		old_y = y;
		FMaskEvent(
			dpy, ButtonPressMask | ButtonReleaseMask |
			KeyPressMask | PointerMotionMask |
			ButtonMotionMask | ExposureMask |
			EnterWindowMask | LeaveWindowMask, &e);
		/* discard extra events before a logical release */
		if (e.type == MotionNotify ||
		    e.type == EnterNotify || e.type == LeaveNotify)
		{
			while (FPending(dpy) > 0 &&
			       FCheckMaskEvent(
				       dpy, ButtonMotionMask |
				       PointerMotionMask | ButtonPressMask |
				       ButtonRelease | KeyPressMask |
				       EnterWindowMask | LeaveWindowMask, &e))
			{
				if (e.type == ButtonPress ||
				    e.type == ButtonRelease ||
				    e.type == KeyPress)
				{
					break;
				}
			}
		}
		if (e.type == EnterNotify || e.type == LeaveNotify)
		{
			XEvent e2;
			int px;
			int py;

			/* Query the pointer to catch the latest information.
			 * This *is* necessary. */
			if (FQueryPointer(
				    dpy, Scr.Root, &JunkRoot, &JunkChild, &px,
				    &py, &JunkX, &JunkY, &JunkMask) == True)
			{
				fev_make_null_event(&e2, dpy);
				e2.type = MotionNotify;
				e2.xmotion.time = fev_get_evtime();
				e2.xmotion.x_root = px;
				e2.xmotion.y_root = py;
				e2.xmotion.state = JunkMask;
				e2.xmotion.same_screen = True;
				e = e2;
				fev_fake_event(&e);
			}
			else
			{
				/* pointer is on a different screen,
				 * ignore event */
			}
		}
		/* Handle a limited number of key press events to allow
		 * mouseless operation */
		if (e.type == KeyPress)
		{
			Keyboard_shortcuts(
				&e, NULL, NULL, NULL, ButtonRelease);
		}
		switch (e.type)
		{
		case KeyPress:
			/* simple code to bag out of move - CKH */
			if (XLookupKeysym(&(e.xkey), 0) == XK_Escape)
			{
				is_finished = True;
			}
			break;
		case ButtonPress:
			if (e.xbutton.button <= NUMBER_OF_MOUSE_BUTTONS &&
			    ((Button1Mask << (e.xbutton.button - 1)) &
			     button_mask))
			{
				/* No new button was pressed, just a delayed
				 * event */
				break;
			}
			/* fall through */
		case ButtonRelease:
			x = e.xbutton.x_root;
			y = e.xbutton.y_root;
			is_finished = True;
			break;
		case MotionNotify:
			if (e.xmotion.same_screen == False)
			{
				continue;
			}
			x = e.xmotion.x_root;
			y = e.xmotion.y_root;
			break;
		case Expose:
			dispatch_event(&e);
			break;
		default:
			/* cannot happen */
			break;
		} /* switch */
		if (x != old_x || y != old_y)
		{
			MoveViewport(m,
				m->virtual_scr.Vx + scroll_speed * (x - old_x),
				m->virtual_scr.Vy + scroll_speed * (y - old_y), False);
			FlushAllMessageQueues();
		}
	} /* while*/
	UngrabEm(GRAB_NORMAL);
	MyXUngrabKeyboard(dpy);
	WaitForButtonsUp(True);
}

/*
 *
 * Parse arguments for "Desk" and "MoveToDesk" (formerly "WindowsDesk"):
 *
 * (nil)       : desk number = current desk
 * n           : desk number = current desk + n
 * 0 n         : desk number = n
 * n x         : desk number = current desk + n
 * 0 n min max : desk number = n, but limit to min/max
 * n min max   : desk number = current desk + n, but wrap around at desk #min
 *               or desk #max
 * n x min max : desk number = current desk + n, but wrap around at desk #min
 *               or desk #max
 *
 * The current desk number is returned if not enough parameters could be
 * read (or if action is empty).
 *
 */
static int GetDeskNumber(struct monitor *mon, char *action, int current_desk)
{
	int n;
	int m;
	int is_relative;
	int desk;
	int val[4];
	int min, max;

	if (MatchToken(action, "prev"))
	{
		return mon->virtual_scr.prev_desk;
	}
	if (MatchToken(action, "next"))
	{
		if (current_desk + 1 >= number_of_desktops(mon))
			return (0);
		else
			return (current_desk + 1);
	}
	n = GetIntegerArguments(action, NULL, &(val[0]), 4);
	if (n <= 0)
	{
		return mon->virtual_scr.CurrentDesk;
	}
	if (n == 1)
	{
		return current_desk + val[0];
	}
	desk = current_desk;
	m = 0;
	if (val[0] == 0)
	{
		/* absolute desk number */
		desk = val[1];
		is_relative = 0;
	}
	else
	{
		/* relative desk number */
		desk += val[0];
		is_relative = 1;
	}
	if (n == 3)
	{
		m = 1;
	}
	if (n == 4)
	{
		m = 2;
	}
	if (n > 2)
	{
		/* handle limits */
		if (val[m] <= val[m+1])
		{
			min = val[m];
			max = val[m+1];
		}
		else
		{
			/* min > max is nonsense, so swap 'em. */
			min = val[m+1];
			max = val[m];
		}
		if (is_relative)
		{
			/* Relative moves wrap around.	*/
			if (desk < min)
			{
				desk += (max - min + 1);
			}
			else if (desk > max)
			{
				desk -= (max - min + 1);
			}
		}
		else if (desk < min)
		{
			/* Relative move outside of range, wrap around. */
			if (val[0] < 0)
			{
				desk = max;
			}
			else
			{
				desk = min;
			}
		}
		else if (desk > max)
		{
			/* Move outside of range, truncate. */
			if (val[0] > 0)
			{
				desk = min;
			}
			else
			{
				desk = max;
			}
		}
	}

	return desk;
}

/*
 *
 * Unmaps a window on transition to a new desktop
 *
 */
static void unmap_window(FvwmWindow *t)
{
	XWindowAttributes winattrs;
	unsigned long eventMask = 0;
	Status ret;

	/*
	 * Prevent the receipt of an UnmapNotify, since that would
	 * cause a transition to the Withdrawn state.
	 */
	ret = XGetWindowAttributes(dpy, FW_W(t), &winattrs);
	if (ret)
	{
		eventMask = winattrs.your_event_mask;
		/* suppress UnmapRequest event */
		XSelectInput(dpy, FW_W(t), eventMask & ~StructureNotifyMask);
	}
	if (IS_ICONIFIED(t))
	{
		if (FW_W_ICON_PIXMAP(t) != None)
		{
			XUnmapWindow(dpy,FW_W_ICON_PIXMAP(t));
		}
		if (FW_W_ICON_TITLE(t) != None)
		{
			XUnmapWindow(dpy,FW_W_ICON_TITLE(t));
		}
	}
	else
	{
		XUnmapWindow(dpy,FW_W_FRAME(t));
		border_undraw_decorations(t);
#ifdef ICCCM2_UNMAP_WINDOW_PATCH
		/* this is required by the ICCCM2 */
		XUnmapWindow(dpy, FW_W(t));
#endif
		if (!Scr.bo.do_enable_ewmh_iconic_state_workaround)
		{
			SetMapStateProp(t, IconicState);
		}
	}
	if (ret)
	{
		XSelectInput(dpy, FW_W(t), eventMask);
		XFlush(dpy);
	}

	return;
}

/*
 *
 * Maps a window on transition to a new desktop
 *
 */
static void map_window(FvwmWindow *t)
{
	XWindowAttributes winattrs;
	unsigned long eventMask = 0;
	Status ret;

	if (IS_SCHEDULED_FOR_DESTROY(t))
	{
		return;
	}
	/*
	 * Prevent the receipt of an UnmapNotify, since that would
	 * cause a transition to the Withdrawn state.
	 */
	ret = XGetWindowAttributes(dpy, FW_W(t), &winattrs);
	if (ret)
	{
		eventMask = winattrs.your_event_mask;
		/* suppress MapRequest event */
		XSelectInput(dpy, FW_W(t), eventMask & ~StructureNotifyMask);
	}
	if (IS_ICONIFIED(t))
	{
		if (FW_W_ICON_PIXMAP(t) != None)
		{
			XMapWindow(dpy,FW_W_ICON_PIXMAP(t));
		}
		if (FW_W_ICON_TITLE(t) != None)
		{
			XMapWindow(dpy,FW_W_ICON_TITLE(t));
		}
	}
	else if (IS_MAPPED(t))
	{
		border_draw_decorations(
			t, PART_ALL, (t == get_focus_window()) ? True : False,
			False, CLEAR_ALL, NULL, NULL);
		XMapWindow(dpy, FW_W_FRAME(t));
		XMapWindow(dpy, FW_W_PARENT(t));
		XMapSubwindows(dpy, FW_W_FRAME(t));
#ifdef ICCCM2_UNMAP_WINDOW_PATCH
		/* this is required by the ICCCM2 */
		XMapWindow(dpy, FW_W(t));
#endif
		if (!Scr.bo.do_enable_ewmh_iconic_state_workaround)
		{
			SetMapStateProp(t, NormalState);
		}
	}
	if (ret)
	{
		XSelectInput(dpy, FW_W(t), eventMask);
		XFlush(dpy);
	}

	return;
}

/*
 *
 * Unmap all windows on a desk -
 *   - Part 1 of a desktop switch
 *   - must eventually be followed by a call to MapDesk
 *   - unmaps from the bottom of the stack up
 *
 */
static void UnmapDesk(struct monitor *m, int desk, Bool grab)
{
	FvwmWindow *t;
	FvwmWindow *sf = get_focus_window();

	if (grab)
	{
		MyXGrabServer(dpy);
	}
	for (t = get_prev_window_in_stack_ring(&Scr.FvwmRoot);
	     t != &Scr.FvwmRoot; t = get_prev_window_in_stack_ring(t))
	{
		if ((monitor_mode == MONITOR_TRACKING_M) && t->m != m)
			continue;

		/* Only change mapping for non-sticky windows */
		if (!is_window_sticky_across_desks(t) && !IS_ICON_UNMAPPED(t))
		{
			if (t->Desk == desk)
			{
				if (sf == t)
				{
					t->flags.is_focused_on_other_desk = 1;
					t->FocusDesk = desk;
					DeleteFocus(True);
				}
				else
				{
					t->flags.is_focused_on_other_desk = 0;
				}
				unmap_window(t);
				SET_FULLY_VISIBLE(t, 0);
				SET_PARTIALLY_VISIBLE(t, 0);
			}
		}
		else
		{
			t->flags.is_focused_on_other_desk = 0;
		}
	}
	if (grab)
	{
		MyXUngrabServer(dpy);
	}

	return;
}

/*
 *
 * Map all windows on a desk -
 *   - Part 2 of a desktop switch
 *   - only use if UnmapDesk has previously been called
 *   - maps from the top of the stack down
 *
 */
static void MapDesk(struct monitor *m, int desk, Bool grab)
{
	FvwmWindow *t;
	FvwmWindow *FocusWin = NULL;
	FvwmWindow *StickyWin = NULL;
	FvwmWindow *sf = get_focus_window();

	Scr.flags.is_map_desk_in_progress = 1;
	if (grab)
	{
		MyXGrabServer(dpy);
	}
	for (t = get_next_window_in_stack_ring(&Scr.FvwmRoot);
	     t != &Scr.FvwmRoot; t = get_next_window_in_stack_ring(t))
	{
		if ((monitor_mode == MONITOR_TRACKING_M) && t->m != m)
			continue;

		/* Only change mapping for non-sticky windows */
		if (!is_window_sticky_across_desks(t) && !IS_ICON_UNMAPPED(t))
		{
			if (t->Desk == desk)
			{
				map_window(t);
			}
		}
		else
		{
			/*  If window is sticky, just update its desk (it's
			 * still mapped).  */
			t->Desk = desk;
			if (sf == t)
			{
				StickyWin = t;
			}
		}
	}
	if (grab)
	{
		MyXUngrabServer(dpy);
	}

	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if ((monitor_mode == MONITOR_TRACKING_M) && t->m != m)
			continue;
		/*
		  Autoplace any sticky icons, so that sticky icons from the old
		  desk don't land on top of stationary ones on the new desk.
		*/
		if (is_window_sticky_across_desks(t) && IS_ICONIFIED(t) &&
		    !IS_ICON_MOVED(t) && !IS_ICON_UNMAPPED(t))
		{
			AutoPlaceIcon(t, NULL, True);
		}
		/* Keep track of the last-focused window on the new desk. */
		if (t->flags.is_focused_on_other_desk && t->FocusDesk == desk)
		{
			t->flags.is_focused_on_other_desk = 0;
			FocusWin = t;
		}
	}

	/*  If a sticky window has focus, don't disturb it.  */
	if (!StickyWin && FocusWin)
	{
		/* Otherwise, handle remembering the last-focused clicky
		 * window.  */
		if (!FP_DO_UNFOCUS_LEAVE(FW_FOCUS_POLICY(FocusWin)))
		{
			ReturnFocusWindow(FocusWin);
		}
		else
		{
			DeleteFocus(True);
		}
	}
	Scr.flags.is_map_desk_in_progress = 0;

	return;
}

/* ---------------------------- interface functions ------------------------ */

/*
 *
 * Check to see if the pointer is on the edge of the screen, and scroll/page
 * if needed
 *
 * Returns
 *  0: no paging
 *  1: paging occured
 * -1: no need to call the function again before a new event arrives
 */
int HandlePaging(
	XEvent *pev, int HorWarpSize, int VertWarpSize, int *xl, int *yt,
	int *delta_x, int *delta_y, Bool Grab, Bool fLoop,
	Bool do_continue_previous, int delay)
{
	static int add_time = 0;
	int x,y;
	XEvent e;
	static Time my_timestamp = 0;
	static Time my_last_timestamp = 0;
	static Bool is_timestamp_valid = False;
	static int last_x = 0;
	static int last_y = 0;
	static Bool is_last_position_valid = False;
	struct monitor	*m = monitor_get_current();
	int mwidth, mheight;

	mwidth = m->virtual_scr.MyDisplayWidth;
	mheight = m->virtual_scr.MyDisplayHeight;


	*delta_x = 0;
	*delta_y = 0;
	if (!is_timestamp_valid && do_continue_previous)
	{
		/* don't call me again until something has happened */
		return -1;
	}
	if (!is_timestamp_valid && pev->type == MotionNotify)
	{
		x = pev->xmotion.x_root;
		y = pev->xmotion.y_root;
		if ((m->virtual_scr.VxMax == 0 ||
		     (x >= (m->si->x + edge_thickness) &&
		      x < (m->si->x + m->si->w)  - edge_thickness)) &&
		    (m->virtual_scr.VyMax == 0 ||
		     (y >= (m->si->y + edge_thickness) &&
		      y < (m->si->y + m->si->h) - edge_thickness)))
		{
			return -1;
		}
	}
	if (delay < 0 || (HorWarpSize == 0 && VertWarpSize==0))
	{
		is_timestamp_valid = False;
		add_time = 0;
		return 0;
	}
	if (m->virtual_scr.VxMax == 0 && m->virtual_scr.VyMax == 0)
	{
		is_timestamp_valid = False;
		add_time = 0;
		return 0;
	}
	if (!is_timestamp_valid)
	{
		is_timestamp_valid = True;
		my_timestamp = fev_get_evtime();
		is_last_position_valid = False;
		add_time = 0;
		last_x = -1;
		last_y = -1;
	}
	else if (my_last_timestamp != fev_get_evtime())
	{
		add_time = 0;
	}
	my_last_timestamp = fev_get_evtime();

	do
	{
		int rc;
		Window JunkW;
		int JunkC;
		unsigned int JunkM;

		if (FCheckPeekIfEvent(dpy, &e, _pred_button_event, NULL))
		{
			is_timestamp_valid = False;
			add_time = 0;
			return 0;
		}
		/* get pointer location */
		rc = FQueryPointer(
			dpy, Scr.Root, &JunkW, &JunkW, &x, &y, &JunkC, &JunkC,
			&JunkM);
		if (rc == False)
		{
			/* pointer is on a different screen */
			x = 0;
			y = 0;
		}

#if 0
		/* check actual pointer location since PanFrames can get buried
		 * under window being moved or resized - mab */
		if (x >= edge_thickness &&
		    x < mwidth - edge_thickness &&
		    x <= (m->si->w) - edge_thickness &&
		    y >= edge_thickness &&
		    y < mheight - edge_thickness &&
		    y >= (m->si->h) - edge_thickness)
		{
			is_timestamp_valid = False;
			add_time = 0;
			return 0;
		}
#endif
		if (!fLoop && is_last_position_valid &&
		    (x - last_x > MAX_PAGING_MOVE_DISTANCE ||
		     x - last_x < -MAX_PAGING_MOVE_DISTANCE ||
		     y - last_y > MAX_PAGING_MOVE_DISTANCE ||
		     y - last_y < -MAX_PAGING_MOVE_DISTANCE))
		{
			/* The pointer is moving too fast, prevent paging until
			 * it slows down. Don't prevent paging when fLoop is
			 * set since we can't be sure that HandlePaging will be
			 * called again. */
			is_timestamp_valid = True;
			my_timestamp = fev_get_evtime();
			add_time = 0;
			last_x = x;
			last_y = y;
			return 0;
		}
		last_x = x;
		last_y = y;
		is_last_position_valid = True;
		usleep(10000);
		add_time += 10;
	} while (fLoop && fev_get_evtime() - my_timestamp + add_time < delay);

	if (fev_get_evtime() - my_timestamp + add_time < delay)
	{
		return 0;
	}

	/* Get the latest pointer position.  This is necessary as XFree 4.1.0.1
	 * sometimes does not report mouse movement when it should. */
	if (FQueryPointer(
		    dpy, Scr.Root, &JunkRoot, &JunkChild, &x, &y, &JunkX,
		    &JunkY, &JunkMask) == False)
	{
		/* pointer is on a different screen - ignore */
	}
	/* Move the viewport */
	/* and/or move the cursor back to the approximate correct location */
	/* that is, the same place on the virtual desktop that it */
	/* started at */

	if (x <= (m->si->x + edge_thickness))
	{
		*delta_x = -HorWarpSize;
	}
	else if (x >= (m->si->x + m->si->w) - edge_thickness)
	{
		*delta_x = HorWarpSize;
	}
	else
	{
		*delta_x = 0;
	}
	if (m->virtual_scr.VxMax == 0)
	{
		*delta_x = 0;
	}
	if (y <= (m->si->y + edge_thickness))
	{
		*delta_y = -VertWarpSize;
	}
	else if (y >= (m->si->y + m->si->h) - edge_thickness)
	{
		*delta_y = VertWarpSize;
	}
	else
	{
		*delta_y = 0;
	}
	if (m->virtual_scr.VyMax == 0)
	{
		*delta_y = 0;
	}

	/* Ouch! lots of bounds checking */
	if (m->virtual_scr.Vx + *delta_x < 0)
	{
		if (!(Scr.flags.do_edge_wrap_x))
		{
			*delta_x = -m->virtual_scr.Vx;
			*xl = x - *delta_x;
		}
		else
		{
			*delta_x += m->virtual_scr.VxMax + mwidth;
			*xl = x + *delta_x % mwidth + HorWarpSize;
		}
	}
	else if (m->virtual_scr.Vx + *delta_x > m->virtual_scr.VxMax)
	{
		if (!(Scr.flags.do_edge_wrap_x))
		{
			*delta_x = m->virtual_scr.VxMax - m->virtual_scr.Vx;
			*xl = x - *delta_x;
		}
		else
		{
			*delta_x -= m->virtual_scr.VxMax + mwidth;
			*xl = x + *delta_x % mwidth - HorWarpSize;
		}
	}
	else
	{
		*xl = x - *delta_x;
	}

	if (m->virtual_scr.Vy + *delta_y < 0)
	{
		if (!(Scr.flags.do_edge_wrap_y))
		{
			*delta_y = -m->virtual_scr.Vy;
			*yt = y - *delta_y;
		}
		else
		{
			*delta_y += m->virtual_scr.VyMax + mheight;
			*yt = y + *delta_y % mheight + VertWarpSize;
		}
	}
	else if (m->virtual_scr.Vy + *delta_y > m->virtual_scr.VyMax)
	{
		if (!(Scr.flags.do_edge_wrap_y))
		{
			*delta_y = m->virtual_scr.VyMax - m->virtual_scr.Vy;
			*yt = y - *delta_y;
		}
		else
		{
			*delta_y -= m->virtual_scr.VyMax + mheight;
			*yt = y + *delta_y % mheight - VertWarpSize;
		}
	}
	else
	{
		*yt = y - *delta_y;
	}

	/* Check for paging -- and don't warp the pointer. */
	is_timestamp_valid = False;
	add_time = 0;
	if (*delta_x == 0 && *delta_y == 0)
	{
		return 0;
	}

	/* make sure the pointer isn't warped into the panframes */
	if (*xl <= (m->si->x + edge_thickness))
	{
		*xl = m->si->x + edge_thickness;
	}
	if (*yt <= (m->si->y + edge_thickness))
	{
		*yt = edge_thickness;
	}
	if (*xl >= (m->si->x + m->si->w) - edge_thickness)
	{
		*xl = (m->si->x + m->si->w) - edge_thickness -1;
	}
	if (*yt >= (m->si->y + m->si->h) - edge_thickness)
	{
		*yt = (m->si->y + m->si->h) - edge_thickness -1;
	}

	if (Grab)
	{
		MyXGrabServer(dpy);
	}
	/* Turn off the rubberband if its on */
	switch_move_resize_grid(False);
	FWarpPointer(dpy,None,Scr.Root,0,0,0,0,*xl,*yt);
	MoveViewport(m, m->virtual_scr.Vx + *delta_x,
			m->virtual_scr.Vy + *delta_y,False);
	if (FQueryPointer(
		    dpy, Scr.Root, &JunkRoot, &JunkChild, xl, yt, &JunkX,
		    &JunkY, &JunkMask) == False)
	{
		/* pointer is on a different screen */
		*xl = 0;
		*yt = 0;
	}
	if (Grab)
	{
		MyXUngrabServer(dpy);
	}

	return 1;
}

/* the root window is surrounded by four window slices, which are InputOnly.
 * So you can see 'through' them, but they eat the input. An EnterEvent in
 * one of these windows causes a Paging. The windows have the according cursor
 * pointing in the pan direction or are hidden if there is no more panning
 * in that direction. This is mostly intended to get a panning even atop
 * of Motif applictions, which does not work yet. It seems Motif windows
 * eat all mouse events.
 *
 * Hermann Dunkel, HEDU, dunkel@cul-ipn.uni-kiel.de 1/94
 */

static bool
should_free_panframe(PanFrame *pf)
{
	if (pf->command != NULL || pf->command_leave != NULL)
		return (false);

	return (false);
}

/*
 * checkPanFrames hides PanFrames if they are on the very border of the
 * VIRTUAL screen and EdgeWrap for that direction is off.
 * (A special cursor for the EdgeWrap border could be nice) HEDU
 */
void checkPanFrames(void)
{
	Bool do_unmap_l = False;
	Bool do_unmap_r = False;
	Bool do_unmap_t = False;
	Bool do_unmap_b = False;
	struct monitor	*m;

	if (!Scr.flags.are_windows_captured)
		return;

	/* thickness of 0 means remove the pan frames */
	if (edge_thickness == 0)
	{
		do_unmap_l = True;
		do_unmap_r = True;
		do_unmap_t = True;
		do_unmap_b = True;
	}

	TAILQ_FOREACH(m, &monitor_q, entry) {
		/* Remove Pan frames if paging by edge-scroll is permanently or
		 * temporarily disabled */
		if (m->virtual_scr.EdgeScrollX == 0 || m->virtual_scr.VxMax == 0)
		{
			do_unmap_l = True;
			do_unmap_r = True;
		}
		if (m->virtual_scr.EdgeScrollY == 0 || m->virtual_scr.VyMax == 0)
		{
			do_unmap_t = True;
			do_unmap_b = True;
		}
		if (m->virtual_scr.Vx == 0 && !Scr.flags.do_edge_wrap_x)
		{
			do_unmap_l = True;
		}
		if (m->virtual_scr.Vx == m->virtual_scr.VxMax &&
		    !Scr.flags.do_edge_wrap_x)
		{
			do_unmap_r = True;
		}
		if (m->virtual_scr.Vy == 0 && !Scr.flags.do_edge_wrap_y)
		{
			do_unmap_t = True;
		}
		if (m->virtual_scr.Vy == m->virtual_scr.VyMax &&
		    !Scr.flags.do_edge_wrap_y)
		{
			do_unmap_b = True;
		}

		/* correct the unmap variables if pan frame commands are set */
		if (edge_thickness != 0) {
			if (!should_free_panframe(&m->PanFrameLeft))
				do_unmap_l = False;
			if (!should_free_panframe(&m->PanFrameRight))
				do_unmap_r = False;
			if (!should_free_panframe(&m->PanFrameBottom))
				do_unmap_b = False;
			if (!should_free_panframe(&m->PanFrameTop))
				do_unmap_t = False;
		}
		/*
		 * hide or show the windows
		 */

		fvwm_debug(__func__,
			"1 %s: {left: %d, right; %d, top: %d, bottom: %d}\n",
			m->si->name,
			m->PanFrameLeft.isMapped, m->PanFrameRight.isMapped,
			m->PanFrameTop.isMapped,  m->PanFrameBottom.isMapped);
		fvwm_debug(__func__,
			"%s: unmap_t: %d, unmap_b: %d, unmap_l: %d, unmap_r: %d",
			m->si->name, do_unmap_t, do_unmap_b, do_unmap_l, do_unmap_r);

		/* left */
		if (do_unmap_l)
		{
			if (m->PanFrameLeft.isMapped)
			{
				XUnmapWindow(dpy, m->PanFrameLeft.win);
				m->PanFrameLeft.isMapped = False;
			}
		}
		else
		{
			if (edge_thickness != last_edge_thickness)
			{
				XResizeWindow(
					dpy, m->PanFrameLeft.win, edge_thickness,
					m->si->y + m->si->h);
			}
			if (!m->PanFrameLeft.isMapped)
			{
				XMapRaised(dpy, m->PanFrameLeft.win);
				m->PanFrameLeft.isMapped = True;
			}
		}
		/* right */
		if (do_unmap_r)
		{
			if (m->PanFrameRight.isMapped)
			{
				XUnmapWindow(dpy, m->PanFrameRight.win);
				m->PanFrameRight.isMapped = False;
			}
		}
		else
		{
			if (edge_thickness != last_edge_thickness)
			{
				XMoveResizeWindow(
					dpy, m->PanFrameRight.win,
					(m->si->x + m->si->w) - edge_thickness,
					m->si->y + m->si->h,
					edge_thickness, (m->si->y + m->si->h));
			}
			if (!m->PanFrameRight.isMapped)
			{
				XMapRaised(dpy, m->PanFrameRight.win);
				m->PanFrameRight.isMapped = True;
			}
		}
		/* top */
		if (do_unmap_t)
		{
			if (m->PanFrameTop.isMapped)
			{
				XUnmapWindow(dpy, m->PanFrameTop.win);
				m->PanFrameTop.isMapped = False;
			}
		}
		else
		{
			if (edge_thickness != last_edge_thickness)
			{
				XResizeWindow(
					dpy, m->PanFrameTop.win,
					(m->si->x + m->si->w),
					edge_thickness);
			}
			if (!m->PanFrameTop.isMapped)
			{
				XMapRaised(dpy, m->PanFrameTop.win);
				m->PanFrameTop.isMapped = True;
			}
		}
		/* bottom */
		if (do_unmap_b)
		{
			if (m->PanFrameBottom.isMapped)
			{
				XUnmapWindow(dpy, m->PanFrameBottom.win);
				m->PanFrameBottom.isMapped = False;
			}
		}
		else
		{
			if (edge_thickness != last_edge_thickness)
			{
				XMoveResizeWindow(
					dpy, m->PanFrameBottom.win, m->si->x,
					(m->si->y + m->si->h) - edge_thickness,
					(m->si->x + m->si->w), edge_thickness);
			}
			if (!m->PanFrameBottom.isMapped)
			{
				XMapRaised(dpy, m->PanFrameBottom.win);
				m->PanFrameBottom.isMapped = True;
			}
		}
		fvwm_debug(__func__,
			"2 %s: {left: %d, right; %d, top: %d, bottom: %d}\n",
			m->si->name,
			m->PanFrameLeft.isMapped, m->PanFrameRight.isMapped,
			m->PanFrameTop.isMapped,  m->PanFrameBottom.isMapped);
	}
	last_edge_thickness = edge_thickness;
}

/*
 *
 * Gotta make sure these things are on top of everything else, or they
 * don't work!
 *
 * For some reason, this seems to be unneeded.
 *
 */
void raisePanFrames(void)
{
	Window windows[1024];
	int n;
	struct monitor	*m;

	/* Note: make sure the stacking order of the pan frames is not changed
	 * every time they are raised by using XRestackWindows. */

	n = 0;
	TAILQ_FOREACH(m, &monitor_q, entry) {
		if (m->PanFrameTop.isMapped)
		{
			windows[n++] = m->PanFrameTop.win;
		}
		if (m->PanFrameLeft.isMapped)
		{
			windows[n++] = m->PanFrameLeft.win;
		}
		if (m->PanFrameRight.isMapped)
		{
			windows[n++] = m->PanFrameRight.win;
		}
		if (m->PanFrameBottom.isMapped)
		{
			windows[n++] = m->PanFrameBottom.win;
		}
	}

	if (n > 0)
	{
		XRaiseWindow(dpy, windows[0]);
		if (n > 1)
		{
			XRestackWindows(dpy, windows, n);
		}
	}
}

/*
 *
 * Creates the windows for edge-scrolling
 *
 */
void initPanFrames(void)
{
	XSetWindowAttributes attributes;
	unsigned long valuemask;
	int saved_thickness;
	struct monitor	*m;

	/* Not creating the frames disables all subsequent behavior */
	/* TKP. This is bad, it will cause an XMap request on a null window
	 * later*/
	/* if (edge_thickness == 0) return; */
	saved_thickness = edge_thickness;
	if (edge_thickness == 0)
	{
		edge_thickness = 2;
	}

	attributes.event_mask = XEVMASK_PANFW;
	valuemask=  (CWEventMask | CWCursor);

	attributes.cursor = Scr.FvwmCursors[CRS_TOP_EDGE];
	/* I know these overlap, it's useful when at (0,0) and the top one is
	 * unmapped */

	/* Free panframes here for all monitors. */
	fvwm_debug(__func__, "freeing panframes");
	TAILQ_FOREACH(m, &monitor_q, entry) {
		if (m->PanFrameLeft.isMapped)
		{
			XUnmapWindow(dpy, m->PanFrameLeft.win);
			m->PanFrameLeft.isMapped = False;
		}
		if (m->PanFrameRight.isMapped)
		{
			XUnmapWindow(dpy, m->PanFrameRight.win);
			m->PanFrameRight.isMapped = False;
		}
		if (m->PanFrameTop.isMapped)
		{
			XUnmapWindow(dpy, m->PanFrameTop.win);
			m->PanFrameTop.isMapped = False;
		}
		if (m->PanFrameBottom.isMapped)
		{
			XUnmapWindow(dpy, m->PanFrameBottom.win);
			m->PanFrameBottom.isMapped = False;
		}
	}

	if (monitor_mode == MONITOR_TRACKING_G) {
		/* Treat the global panframes separately -- the logic for
		 * handling these along with per-monitor ones was getting too
		 * cumbersome.
		 */
		struct monitor	*m, *mloop;
		int		 gw = monitor_get_all_widths();
		int		 gh = monitor_get_all_heights();

		m = TAILQ_FIRST(&monitor_q);

		m->PanFrameTop.win = XCreateWindow(dpy, Scr.Root, 0, 0, gw,
		    edge_thickness, 0, CopyFromParent, InputOnly,
		    CopyFromParent, valuemask, &attributes);

		m->PanFrameLeft.win = XCreateWindow(dpy, Scr.Root, 0, 0,
		    edge_thickness, gh, 0, CopyFromParent, InputOnly,
		    CopyFromParent, valuemask, &attributes);

		m->PanFrameRight.win = XCreateWindow(dpy, Scr.Root,
		    gw - edge_thickness, 0, edge_thickness, gh, 0,
		    CopyFromParent, InputOnly, CopyFromParent, valuemask,
		    &attributes);

		m->PanFrameBottom.win = XCreateWindow(dpy, Scr.Root, 0,
		    gh - edge_thickness, gw, edge_thickness, 0,
		    CopyFromParent, InputOnly, CopyFromParent, valuemask,
		    &attributes);

		TAILQ_FOREACH(mloop, &monitor_q, entry) {
			if (mloop == m)
				continue;
			memcpy(&mloop->PanFrameTop, &m->PanFrameTop, sizeof(m->PanFrameTop));
			memcpy(&mloop->PanFrameLeft, &m->PanFrameLeft, sizeof(m->PanFrameLeft));
			memcpy(&mloop->PanFrameRight, &m->PanFrameRight, sizeof(m->PanFrameRight));
			memcpy(&mloop->PanFrameBottom, &m->PanFrameBottom, sizeof(m->PanFrameBottom));
		}
		checkPanFrames();
		fvwm_debug(__func__, "finished setting up global panframes");
		return;
	}

	TAILQ_FOREACH(m, &monitor_q, entry) {
		m->PanFrameTop.win = XCreateWindow(
			dpy, Scr.Root, m->si->x, m->si->y,
			m->si->w, edge_thickness,
			0, CopyFromParent, InputOnly, CopyFromParent, valuemask,
			&attributes);
		m->PanFrameTop.command = NULL;
		m->PanFrameTop.command_leave = NULL;
		attributes.cursor = Scr.FvwmCursors[CRS_LEFT_EDGE];
		m->PanFrameLeft.win = XCreateWindow(
			dpy, Scr.Root, m->si->x, m->si->y,
			edge_thickness, m->si->h,
			0, CopyFromParent, InputOnly, CopyFromParent, valuemask,
			&attributes);
		m->PanFrameLeft.command = NULL;
		m->PanFrameLeft.command_leave = NULL;
		attributes.cursor = Scr.FvwmCursors[CRS_RIGHT_EDGE];
		m->PanFrameRight.win = XCreateWindow(
			dpy, Scr.Root,
			(m->si->x + m->si->w) - edge_thickness, (m->si->y + m->si->h) - m->si->h,
			edge_thickness, m->si->h, 0,
			CopyFromParent, InputOnly, CopyFromParent, valuemask,
			&attributes);
		m->PanFrameRight.command = NULL;
		m->PanFrameRight.command_leave = NULL;
		attributes.cursor = Scr.FvwmCursors[CRS_BOTTOM_EDGE];
		m->PanFrameBottom.win = XCreateWindow(
			dpy, Scr.Root, m->si->x,
			m->si->h - edge_thickness,
			m->si->w, edge_thickness, 0,
			CopyFromParent, InputOnly, CopyFromParent, valuemask,
			&attributes);
		m->PanFrameBottom.command = NULL;
		m->PanFrameBottom.command_leave = NULL;
		m->PanFrameTop.isMapped=m->PanFrameLeft.isMapped=
			m->PanFrameRight.isMapped= m->PanFrameBottom.isMapped=False;
	}
	edge_thickness = saved_thickness;
	checkPanFrames();
	fvwm_debug(__func__, "finished setting up per-monitor panframes");
}

Bool is_pan_frame(Window w)
{
	struct monitor *m;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		bool is_pf_top = (w == m->PanFrameTop.win);
		bool is_pf_bottom = (w == m->PanFrameBottom.win);
		bool is_pf_left = (w == m->PanFrameLeft.win);
		bool is_pf_right = (w == m->PanFrameRight.win);

		if (is_pf_top) {
			fvwm_debug(__func__, "Window is PanFrame top\n");
			return True;
		}

		if (is_pf_bottom) {
			fvwm_debug(__func__, "Window is PanFrame bottom\n");
			return True;
		}

		if (is_pf_left) {
			fvwm_debug(__func__, "Window is PanFrame left\n");
			return True;
		}

		if (is_pf_right) {
			fvwm_debug(__func__, "Window is PanFrame right\n");
			return True;
		}
	}

	return False;
}

/*
 *
 *  Moves the viewport within the virtual desktop
 *
 */
void MoveViewport(struct monitor *m, int newx, int newy, Bool grab)
{
	FvwmWindow *t, *t1;
	int deltax,deltay;
	int PageTop, PageLeft;
	int PageBottom, PageRight;
	int txl, txr, tyt, tyb;

	if (grab)
	{
		MyXGrabServer(dpy);
	}
	if (newx > m->virtual_scr.VxMax)
	{
		newx = m->virtual_scr.VxMax;
	}
	if (newy > m->virtual_scr.VyMax)
	{
		newy = m->virtual_scr.VyMax;
	}
	if (newx < 0)
	{
		newx = 0;
	}
	if (newy < 0)
	{
		newy = 0;
	}
	deltay = m->virtual_scr.Vy - newy;
	deltax = m->virtual_scr.Vx - newx;
	/*
	  Identify the bounding rectangle that will be moved into
	  the viewport.
	*/
	PageBottom    =  m->virtual_scr.MyDisplayHeight - deltay - 1;
	PageRight     =  m->virtual_scr.MyDisplayWidth  - deltax - 1;
	PageTop       =  0 - deltay;
	PageLeft      =  0 - deltax;

	if (deltax || deltay)
	{
		m->virtual_scr.prev_page_x = m->virtual_scr.Vx;
		m->virtual_scr.prev_page_y = m->virtual_scr.Vy;
		m->virtual_scr.prev_desk_and_page_page_x = m->virtual_scr.Vx;
		m->virtual_scr.prev_desk_and_page_page_y = m->virtual_scr.Vy;
		m->virtual_scr.prev_desk_and_page_desk = m->virtual_scr.CurrentDesk;
	}
	m->virtual_scr.Vx = newx;
	m->virtual_scr.Vy = newy;

	/* If we're in global mode, store the virtual_scr across all monitors.
	 * This will allow for per-monitor tracking to be toggled, as well as
	 * ensuring commands such as Goto{Page,Desk} operate correctly.
	 */
	monitor_assign_virtual(m);

	if (deltax || deltay)
	{
		struct monitor	*mloop;

		TAILQ_FOREACH(mloop, &monitor_q, entry) {
			BroadcastPacket(
				M_NEW_PAGE, 8,
				(long)mloop->virtual_scr.Vx,
				(long)mloop->virtual_scr.Vy,
				(long)mloop->virtual_scr.CurrentDesk,
				(long)mloop->virtual_scr.MyDisplayWidth,
				(long)mloop->virtual_scr.MyDisplayHeight,
				(long)((mloop->virtual_scr.VxMax / mloop->virtual_scr.MyDisplayWidth ) + 1),
				(long)((mloop->virtual_scr.VyMax / mloop->virtual_scr.MyDisplayHeight) + 1),
				(long)mloop->si->rr_output);
		}
		/*
		 * RBW - 11/13/1998      - new:  chase the chain
		 * bidirectionally, all at once! The idea is to move the
		 * windows that are moving out of the viewport from the bottom
		 * of the stacking order up, to minimize the expose-redraw
		 * overhead. Windows that will be moving into view will be
		 * moved top down, for the same reason. Use the new
		 * stacking-order chain, rather than the old last-focused
		 * chain.
		 *
		 * domivogt (29-Nov-1999): It's faster to first map windows
		 * top to bottom and then unmap windows bottom up.
		 */
		/* TA: 2020-01-21:  This change of skipping monitors will
		 * break using 'Scroll' and __drag_viewport().  We need to
		 * ensure we handle this case properly.
		 */
		t = get_next_window_in_stack_ring(&Scr.FvwmRoot);
		while (t != &Scr.FvwmRoot)
		{
			if ((monitor_mode == MONITOR_TRACKING_M) && t->m != m) {
				/*  Bump to next win...  */
				t = get_next_window_in_stack_ring(t);
				continue;
			}
			/*
			 * If the window is moving into the viewport...
			 */
			txl = t->g.frame.x;
			tyt = t->g.frame.y;
			txr = t->g.frame.x + t->g.frame.width - 1;
			tyb = t->g.frame.y + t->g.frame.height - 1;
			if (is_window_sticky_across_pages(t) &&
			    !IS_VIEWPORT_MOVED(t))
			{
				/* the absolute position has changed */
				t->g.normal.x -= deltax;
				t->g.normal.y -= deltay;
				t->g.max.x -= deltax;
				t->g.max.y -= deltay;
				/*  Block double move.  */
				SET_VIEWPORT_MOVED(t, 1);
			}
			if ((txr >= PageLeft && txl <= PageRight
			     && tyb >= PageTop && tyt <= PageBottom)
			    && !IS_VIEWPORT_MOVED(t)
			    && !IS_WINDOW_BEING_MOVED_OPAQUE(t))
			{
				/*  Block double move.  */
				SET_VIEWPORT_MOVED(t, 1);
				/* If the window is iconified, and sticky
				 * Icons is set, then the window should
				 * essentially be sticky */
				if (!is_window_sticky_across_pages(t))
				{
					if (IS_ICONIFIED(t))
					{
						modify_icon_position(
							t, deltax, deltay);
						move_icon_to_position(t);
						broadcast_icon_geometry(
							t, False);
					}
					frame_setup_window(
						t, t->g.frame.x + deltax,
						t->g.frame.y + deltay,
						t->g.frame.width,
						t->g.frame.height, False);
				}
			}
			/*  Bump to next win...  */
			t = get_next_window_in_stack_ring(t);
		}
		t1 = get_prev_window_in_stack_ring(&Scr.FvwmRoot);
		while (t1 != &Scr.FvwmRoot)
		{
			if ((monitor_mode == MONITOR_TRACKING_M) && t1->m != m) {
				/*  Bump to next win...  */
				t1 = get_prev_window_in_stack_ring(t1);
				continue;
			}
			/*
			 *If the window is not moving into the viewport...
			 */
			SET_VIEWPORT_MOVED(t, 1);
			txl = t1->g.frame.x;
			tyt = t1->g.frame.y;
			txr = t1->g.frame.x + t1->g.frame.width - 1;
			tyb = t1->g.frame.y + t1->g.frame.height - 1;
			if (! (txr >= PageLeft && txl <= PageRight
			       && tyb >= PageTop && tyt <= PageBottom)
			    && !IS_VIEWPORT_MOVED(t1)
			    && !IS_WINDOW_BEING_MOVED_OPAQUE(t1))
			{
				/* If the window is iconified, and sticky
				 * Icons is set, then the window should
				 * essentially be sticky */
				if (!is_window_sticky_across_pages(t1))
				{
					if (IS_ICONIFIED(t1))
					{
						modify_icon_position(
							t1, deltax, deltay);
						move_icon_to_position(t1);
						broadcast_icon_geometry(
							t1, False);
					}
					frame_setup_window(
						t1, t1->g.frame.x + deltax,
						t1->g.frame.y + deltay,
						t1->g.frame.width,
						t1->g.frame.height, False);
				}
			}
			/*  Bump to next win...  */
			t1 = get_prev_window_in_stack_ring(t1);
		}
		for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
		{
			if ((monitor_mode == MONITOR_TRACKING_M) && t->m != m)
				continue;

			if (IS_VIEWPORT_MOVED(t))
			{
				/* Clear double move blocker. */
				SET_VIEWPORT_MOVED(t, 0);
			}
			/* If its an icon, and its sticking, autoplace it so
			 * that it doesn't wind up on top a a stationary
			 * icon */
			if (is_window_sticky_across_pages(t) &&
			    IS_ICONIFIED(t) && !IS_ICON_MOVED(t) &&
			    !IS_ICON_UNMAPPED(t))
			{
				AutoPlaceIcon(t, NULL, True);
			}
		}
	}
	checkPanFrames();
	/* regrab buttons in case something got obscured or unobscured */
	focus_grab_buttons_all();

#if 0
	/* dv (2004-07-01): I don't think that's a good idea.  We could eat too
	 * many events. */
	/* do this with PanFrames too ??? HEDU */
	{
		XEvent e;

		while (FCheckTypedEvent(dpy, MotionNotify, &e))
		{
			/* nothing */
		}
	}
#endif
	if (grab)
	{
		MyXUngrabServer(dpy);
	}
	EWMH_SetDesktopViewPort(m);

	return;
}

void goto_desk(int desk, struct monitor *m)
{
	struct monitor	*m2 = NULL;
	/* RBW - the unmapping operations are now removed to their own
	 * functions so they can also be used by the new GoToDeskAndPage
	 * command. */

	/* FIXME: needs to broadcast monitors if global. */

	if (m == NULL)
		return;

	if (m->virtual_scr.CurrentDesk != desk)
	{
		m->virtual_scr.prev_desk = m->virtual_scr.CurrentDesk;
		m->virtual_scr.prev_desk_and_page_desk = m->virtual_scr.CurrentDesk;
		m->virtual_scr.prev_desk_and_page_page_x = m->virtual_scr.Vx;
		m->virtual_scr.prev_desk_and_page_page_y = m->virtual_scr.Vy;

		monitor_assign_virtual(m);

		UnmapDesk(m, m->virtual_scr.CurrentDesk, True);
		m->virtual_scr.CurrentDesk = desk;
		MapDesk(m, desk, True);

		monitor_assign_virtual(m);

		focus_grab_buttons_all();

		if (monitor_mode == MONITOR_TRACKING_M) {
			BroadcastPacket(M_NEW_DESK, 2, (long)m->virtual_scr.CurrentDesk,
					(long)m->si->rr_output);
			/* FIXME: domivogt (22-Apr-2000): Fake a 'restack' for sticky
			 * window upon desk change.  This is a workaround for a
			 * problem in FvwmPager: The pager has a separate 'root'
			 * window for each desk.  If the active desk changes, the
			 * pager destroys sticky mini windows and creates new ones in
			 * the other desktop 'root'.  But the pager can't know where to
			 * stack them.  So we have to tell it explicitly where they
			 * go :-( This should be fixed in the pager, but right now the
			 * pager doesn't maintain the stacking order. */
			BroadcastRestackAllWindows();
			EWMH_SetCurrentDesktop(m);

			return;
		}

		TAILQ_FOREACH(m2, &monitor_q, entry) {
			if (m != m2) {
				m2->Desktops = m->Desktops;
				m2->virtual_scr.CurrentDesk = m->virtual_scr.CurrentDesk;
                        }

			BroadcastPacket(M_NEW_DESK, 2, (long)m2->virtual_scr.CurrentDesk,
					(long)m2->si->rr_output);

			/* FIXME: domivogt (22-Apr-2000): Fake a 'restack' for sticky
			 * window upon desk change.  This is a workaround for a
			 * problem in FvwmPager: The pager has a separate 'root'
			 * window for each desk.  If the active desk changes, the
			 * pager destroys sticky mini windows and creates new ones in
			 * the other desktop 'root'.  But the pager can't know where to
			 * stack them.  So we have to tell it explicitly where they
			 * go :-( This should be fixed in the pager, but right now the
			 * pager doesn't maintain the stacking order. */
			BroadcastRestackAllWindows();
			EWMH_SetCurrentDesktop(m2);

		}
	}
}

/*
 *
 * Move a window to a new desktop
 *
 */
void do_move_window_to_desk(FvwmWindow *fw, int desk)
{
	struct monitor	*m;

	if (fw == NULL)
		return;

	m = fw->m;

	/*
	 * Set the window's desktop, and map or unmap it as needed.
	 */
	{
		if (fw->Desk == m->virtual_scr.CurrentDesk)
		{
			fw->Desk = desk;
			if (fw == get_focus_window())
			{
				DeleteFocus(True);
			}
			unmap_window(fw);
			SET_FULLY_VISIBLE(fw, 0);
			SET_PARTIALLY_VISIBLE(fw, 0);
		}
		else if (desk == m->virtual_scr.CurrentDesk)
		{
			fw->Desk = desk;
			/* If its an icon, auto-place it */
			if (IS_ICONIFIED(fw))
			{
				AutoPlaceIcon(fw, NULL, True);
			}
			map_window(fw);
		}
		else
		{
			fw->Desk = desk;
		}
		BroadcastConfig(M_CONFIGURE_WINDOW,fw);
	}
	focus_grab_buttons_on_layer(fw->layer);
	EWMH_SetWMDesktop(fw);

	return;
}

Bool get_page_arguments(FvwmWindow *fw, char *action, int *page_x, int *page_y, struct monitor **mret)
{
	int val[2];
	int suffix[2];
	int mw, mh;
	char *token;
	char *taction, *action_cpy, *action_cpy_start;
	int wrapx;
	int wrapy;
	int limitdeskx;
	int limitdesky;
	struct monitor	*m, *m_use;

	wrapx = 0;
	wrapy = 0;
	limitdeskx = 1;
	limitdesky = 1;

	m_use = (fw && fw->m) ? fw->m : monitor_get_current();
	action_cpy = strdup(action);
	action_cpy_start = action_cpy;
	token = PeekToken(action_cpy, &action_cpy);
	free(action_cpy_start);

	if (token == NULL)
		return (False);

	m = monitor_resolve_name(token);
	if (strcmp(m->si->name, token) != 0)
		m = m_use;
	else
		PeekToken(action, &action);

	if (mret != NULL)
		*mret = m;

	mw = m->virtual_scr.MyDisplayWidth;
	mh = m->virtual_scr.MyDisplayHeight;

	for (; ; action = taction)
	{
		int do_reverse;

		token = PeekToken(action, &taction);
		if (token == NULL)
		{
			*page_x = m->virtual_scr.Vx;
			*page_y = m->virtual_scr.Vy;
			return True;
		}

		if (StrEquals(token, "prev"))
		{
			/* last page selected */
			*page_x = m->virtual_scr.prev_page_x;
			*page_y = m->virtual_scr.prev_page_y;
			return True;
		}
		do_reverse = 0;
		for ( ; *token == '!'; token++)
		{
			do_reverse = !do_reverse;
		}
		if (StrEquals(token, "wrapx"))
		{
			wrapx = (1 ^ do_reverse);
		}
		else if (StrEquals(token, "wrapy"))
		{
			wrapy = (1 ^ do_reverse);
		}
		else if (StrEquals(token, "nodesklimitx"))
		{
			limitdeskx = (0 ^ do_reverse);
		}
		else if (StrEquals(token, "nodesklimity"))
		{
			limitdesky = (0 ^ do_reverse);
		}
		else
		{
			/* no more options */
			break;
		}
	}

	if (GetSuffixedIntegerArguments(action, NULL, val, 2, "pw", suffix) !=
	    2)
	{
		return 0;
	}

	if (suffix[0] == 1)
	{
		*page_x = val[0] * mw + m->virtual_scr.Vx;
	}
	else if (suffix[0] == 2)
	{
		*page_x += val[0] * mw;
	}
	else if (val[0] >= 0)
	{
		*page_x = val[0] * mw;
	}
	else
	{
		*page_x = (val[0] + 1) * mw + m->virtual_scr.VxMax;
	}
	if (suffix[1] == 1)
	{
		*page_y = val[1] * mh + m->virtual_scr.Vy;
	}
	else if (suffix[1] == 2)
	{
		*page_y += val[1] * mh;
	}
	else if (val[1] >= 0)
	{
		*page_y = val[1] * mh;
	}
	else
	{
		*page_y = (val[1] + 1) * mh + m->virtual_scr.VyMax;
	}

	/* limit to desktop size */
	if (limitdeskx && !wrapx)
	{
		if (*page_x < 0)
		{
			*page_x = 0;
		}
		else if (*page_x > m->virtual_scr.VxMax)
		{
			*page_x = m->virtual_scr.VxMax;
		}
	}
	else if (limitdeskx && wrapx)
	{
		while (*page_x < 0)
		{
			*page_x += m->virtual_scr.VxMax +
				m->virtual_scr.MyDisplayWidth;
		}
		while (*page_x > m->virtual_scr.VxMax)
		{
			*page_x -= m->virtual_scr.VxMax +
				m->virtual_scr.MyDisplayWidth;
		}
	}
	if (limitdesky && !wrapy)
	{
		if (*page_y < 0)
		{
			*page_y = 0;
		}
		else if (*page_y > m->virtual_scr.VyMax)
		{
			*page_y = m->virtual_scr.VyMax;
		}
	}
	else if (limitdesky && wrapy)
	{
		while (*page_y < 0)
		{
			*page_y += m->virtual_scr.VyMax +
				m->virtual_scr.MyDisplayHeight;
		}
		while (*page_y > m->virtual_scr.VyMax)
		{
			*page_y -= m->virtual_scr.VyMax +
				m->virtual_scr.MyDisplayHeight;
		}
	}

	return True;
}

char *GetDesktopName(struct monitor *m, int desk)
{
	DesktopsInfo *d;

	d = m->Desktops->next;
	while (d != NULL && d->desk != desk)
	{
		d = d->next;
	}
	if (d != NULL)
	{
		return d->name;
	}

	return NULL;
}

/* ---------------------------- builtin commands --------------------------- */

/* EdgeCommand - binds a function to a pan frame enter event */
void CMD_EdgeCommand(F_CMD_ARGS)
{
	direction_t direction;
	char *command = NULL, *actdup = NULL, *rest;
	struct monitor	*m;

	if (action != NULL)
		actdup = fxstrdup(action);

	/* get the direction */
	direction = gravity_parse_dir_argument(action, &action, DIR_NONE);

	if (direction >= 0 && direction <= DIR_MAJOR_MASK)
	{
		/* check if the command does contain at least one token */
		rest = GetNextToken(actdup , &action);
		if (rest == NULL || *rest == '\0')
		{
			/* the command does not contain a token so
			   the command of this edge is removed */
			free(command);
			command = NULL;
		} else
			command = fxstrdup(rest);

		TAILQ_FOREACH(m, &monitor_q, entry) {
			/* assign command to the edge(s) */
			if (direction == DIR_N)
			{
				free(m->PanFrameTop.command);
				m->PanFrameTop.command = NULL;
				if (command != NULL)
					m->PanFrameTop.command = fxstrdup(command);
			}
			else if (direction == DIR_S)
			{
				free(m->PanFrameBottom.command);
				m->PanFrameBottom.command = NULL;
				if (command != NULL)
					m->PanFrameBottom.command = fxstrdup(command);
			}
			else if (direction == DIR_W)
			{
				free(m->PanFrameLeft.command);
				m->PanFrameLeft.command = NULL;
				if (command != NULL)
					m->PanFrameLeft.command = fxstrdup(command);
			}
			else if (direction == DIR_E)
			{
				free(m->PanFrameRight.command);
				m->PanFrameRight.command = NULL;
				if (command != NULL)
					m->PanFrameRight.command = fxstrdup(command);

				fvwm_debug(__func__, "RIGHT IS: %s",
					m->PanFrameRight.command);
			}
			else
			{
				/* this should never happen */
				fvwm_debug(__func__,
					   "Internal error in CMD_EdgeCommand");
			}
		}
		free(command);
		free(actdup);
	} else {
		fvwm_debug(__func__, "EdgeCommand needs a direction\n");
		free(actdup);
		return;
	}
	/* maybe something has changed so we adapt the pan frames */
	checkPanFrames();
}

/* EdgeLeaveCommand - binds a function to a pan frame Leave event */
void CMD_EdgeLeaveCommand(F_CMD_ARGS)
{
	direction_t direction;
	char *command = NULL, *actdup = NULL, *rest;
	struct monitor *m;

	if (action != NULL)
		actdup = fxstrdup(action);

	/* get the direction */
	direction = gravity_parse_dir_argument(action, &action, DIR_NONE);

	if (direction >= 0 && direction <= DIR_MAJOR_MASK)
	{
		/* check if the command does contain at least one token */
		rest = GetNextToken(actdup , &action);
		if (rest == NULL || *rest == '\0')
		{
			/* the command does not contain a token so
			   the command of this edge is removed */
			free(command);
			command = NULL;
		} else
			command = fxstrdup(rest);

		TAILQ_FOREACH(m, &monitor_q, entry) {
			/* assign command to the edge(s) */
			if (direction == DIR_N)
			{
				free(m->PanFrameTop.command_leave);
				m->PanFrameTop.command_leave = NULL;
				if (command != NULL)
					m->PanFrameTop.command_leave = fxstrdup(command);
			}
			else if (direction == DIR_S)
			{
				free(m->PanFrameBottom.command_leave);
				m->PanFrameBottom.command_leave = NULL;
				if (command != NULL)
					m->PanFrameBottom.command_leave = fxstrdup(command);
			}
			else if (direction == DIR_W)
			{
				free(m->PanFrameLeft.command_leave);
				m->PanFrameLeft.command_leave = NULL;
				if (command != NULL)
					m->PanFrameLeft.command_leave = fxstrdup(command);
			}
			else if (direction == DIR_E)
			{
				free(m->PanFrameRight.command_leave);
				m->PanFrameRight.command_leave = NULL;
				if (command != NULL)
					m->PanFrameRight.command_leave = fxstrdup(command);
			}
			else
			{
				/* this should never happen */
				fvwm_debug(__func__, "Internal error");
			}
		}
		free(command);
		free(actdup);
	} else {
		fvwm_debug(__func__, "EdgeLeaveCommand needs a direction");
		free(actdup);
		return;
	}

	/* maybe something has changed so we adapt the pan frames */
	checkPanFrames();
}

void CMD_EdgeThickness(F_CMD_ARGS)
{
	int val, n;

	n = GetIntegerArguments(action, NULL, &val, 1);
	if (n != 1)
	{
		fvwm_debug(__func__,
			   "EdgeThickness requires 1 numeric argument,"
			   " found %d args",n);
		return;
	}
		/* check range */
	if (val < 0 || val > 2)
	{
		fvwm_debug(__func__,
			   "EdgeThickness arg must be between 0 and 2,"
			   " found %d",val);
		return;
	}
	edge_thickness = val;
	checkPanFrames();

	return;
}

void CMD_EdgeScroll(F_CMD_ARGS)
{
	int val1, val2, val1_unit, val2_unit, n;
	char *token;
	struct monitor	*m;

	n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
	if (n != 2)
	{
		fvwm_debug(__func__,
			   "EdgeScroll requires two arguments");
		return;
	}

	/*
	 * if edgescroll >1000 and <100000
	 * wrap at edges of desktop (a "spherical" desktop)
	 */
	if (val1 >= 1000 && val1_unit != 100)
	{
		val1 /= 1000;
		Scr.flags.do_edge_wrap_x = 1;
	}
	else
	{
		Scr.flags.do_edge_wrap_x = 0;
	}
	if (val2 >= 1000 && val2_unit != 100)
	{
		val2 /= 1000;
		Scr.flags.do_edge_wrap_y = 1;
	}
	else
	{
		Scr.flags.do_edge_wrap_y = 0;
	}

	action=SkipNTokens(action,2);
	token = PeekToken(action, NULL);

	if (token)
	{
		if (StrEquals(token, "wrap"))
		{
			Scr.flags.do_edge_wrap_x = 1;
			Scr.flags.do_edge_wrap_y = 1;
		}
		else if (StrEquals(token, "wrapx"))
		{
			Scr.flags.do_edge_wrap_x = 1;
		}
		else if (StrEquals(token, "wrapy"))
		{
			Scr.flags.do_edge_wrap_y = 1;
		}
	}

	TAILQ_FOREACH(m, &monitor_q, entry) {
		m->virtual_scr.EdgeScrollX = val1 * val1_unit / 100;
		m->virtual_scr.EdgeScrollY = val2 * val2_unit / 100;
	}

	checkPanFrames();

	return;
}

void CMD_EdgeResistance(F_CMD_ARGS)
{
	int val[3];
	int n;

	val[0] = 0;
	n = GetIntegerArguments(action, NULL, val, 3);
       	if (n > 1 && val[0] >= 10000)
	{
		/* map val[0] >= 10000 in old syntax to -1 in new syntax */
		val[0] = -1;
	}
	if (n == 1)
	{
		Scr.ScrollDelay = val[0];
	}
	else if (n >= 2 && n <= 3)
	{
		char cmd[99];
		char stylecmd[99];
		char stylecmd2[99];

		Scr.ScrollDelay = val[0];
		sprintf(cmd, "EdgeResistance %d", val[0]);
		sprintf(stylecmd, "Style * EdgeMoveDelay %d", val[0]);
		if (n == 2)
		{
			sprintf(
				stylecmd2, "Style * EdgeMoveResistance %d",
				val[1]);
		}
		else
		{
			sprintf(
				stylecmd2, "Style * EdgeMoveResistance %d %d",
				val[1], val[2]);
		}
		fvwm_debug(__func__,
			   "The command EdgeResistance with three arguments is"
			   " obsolete. Please use the following commands"
			   " instead:\n%s\n%s\n%s\n", cmd, stylecmd,
			   stylecmd2);
		execute_function(
			cond_rc, exc, cmd,
			FUNC_DONT_REPEAT | FUNC_DONT_EXPAND_COMMAND);
		execute_function(
			cond_rc, exc, stylecmd,
			FUNC_DONT_REPEAT | FUNC_DONT_EXPAND_COMMAND);
		execute_function(
			cond_rc, exc, stylecmd2,
			FUNC_DONT_REPEAT | FUNC_DONT_EXPAND_COMMAND);
	}
	else
	{
		fvwm_debug(__func__,
			   "EdgeResistance requires two or three arguments");
		return;
	}

	return;
}

void CMD_DesktopConfiguration(F_CMD_ARGS)
{
	FvwmWindow	*t;
	struct monitor	*m_loop, *m = monitor_get_current();

	if (action == NULL) {
		fvwm_debug(__func__, "action is required");
		return;
	}

	if (strcmp(action, "global") == 0) {
		/* If we're switching to global mode after coming out of per-monitor
		 * mode, the desks won't be the same.  Fix this by switching
		 * other monitor desks to be the same as the current monitor.
		 */

		char *cmd = NULL;
		TAILQ_FOREACH(m_loop, &monitor_q, entry) {
			if (m_loop == m)
				continue;
			xasprintf(&cmd, "GotoDeskAndPage %s %d %d %d",
				m_loop->si->name,
				m->virtual_scr.CurrentDesk,
				m->virtual_scr.Vx / m->virtual_scr.MyDisplayWidth,
				m->virtual_scr.Vy / m->virtual_scr.MyDisplayHeight);

			execute_function_override_window(NULL, NULL, cmd, 0, NULL);
			free(cmd);
		}
		monitor_mode = MONITOR_TRACKING_G;
	} else if (strcmp(action, "per-monitor") == 0)
		monitor_mode = MONITOR_TRACKING_M;
	else {
		fvwm_debug(__func__, "action not recognised");
		return;
	}

	initPanFrames();
	checkPanFrames();
	raisePanFrames();

	for (t = Scr.FvwmRoot.next; t; t = t->next)
		UPDATE_FVWM_SCREEN(t);

	BroadcastMonitorList(NULL);
}

void
calculate_page_sizes(struct monitor *m, int dx, int dy)
{
	m->virtual_scr.VxMax = dx *
		m->virtual_scr.MyDisplayWidth - m->virtual_scr.MyDisplayWidth;
	m->virtual_scr.VyMax = dy *
		m->virtual_scr.MyDisplayHeight - m->virtual_scr.MyDisplayHeight;
}

void CMD_DesktopSize(F_CMD_ARGS)
{
	int val[2];
	struct monitor	*m;

	if (GetIntegerArguments(action, NULL, val, 2) != 2 &&
	    GetRectangleArguments(action, &val[0], &val[1]) != 2)
	{
		fvwm_debug(__func__,
			   "DesktopSize requires two arguments");
		return;
	}

	TAILQ_FOREACH(m, &monitor_q, entry) {
		m->dx = val[0] <= 0 ? 1 : val[0];
		m->dy = val[1] <= 0 ? 1 : val[1];

		calculate_page_sizes(m, m->dx, m->dy);

		BroadcastPacket(
			M_NEW_PAGE, 8,
			(long)m->virtual_scr.Vx,
			(long)m->virtual_scr.Vy,
			(long)m->virtual_scr.CurrentDesk,
			(long)m->virtual_scr.MyDisplayWidth,
			(long)m->virtual_scr.MyDisplayHeight,
			(long)((m->virtual_scr.VxMax / m->virtual_scr.MyDisplayWidth) + 1),
			(long)((m->virtual_scr.VyMax / m->virtual_scr.MyDisplayHeight) + 1),
			(long)m->si->rr_output);

		EWMH_SetDesktopGeometry(m);
	}
	checkPanFrames();
}

/*
 *
 * Move to a new desktop
 *
 */
void CMD_GotoDesk(F_CMD_ARGS)
{
	struct monitor  *m_use = monitor_get_current(), *m;
	char		*action_cpy, *action_cpy_start, *token;

	action_cpy = strdup(action);
	action_cpy_start = action_cpy;
	token = PeekToken(action_cpy, &action_cpy);

	m = monitor_resolve_name(token);
	if (strcmp(m->si->name, token) != 0)
		m = m_use;
	else
		PeekToken(action, &action);

	fvwm_debug(__func__, "%s: using monitor: %s\n", __func__, m->si->name);

	goto_desk(GetDeskNumber(m, action, m->virtual_scr.CurrentDesk), m);

	free(action_cpy_start);
	return;
}

void CMD_Desk(F_CMD_ARGS)
{
	CMD_GotoDesk(F_PASS_ARGS);

	return;
}

/*
 *
 * Move to a new desktop and page at the same time.
 *   This function is designed for use by the Pager, and replaces the old
 *   GoToDesk 0 10000 hack.
 *   - unmap all windows on the current desk so they don't flash when the
 *     viewport is moved, then switch the viewport, then the desk.
 *
 */
void CMD_GotoDeskAndPage(F_CMD_ARGS)
{
	int val[3];
	Bool is_new_desk;
	char *action_cpy, *action_cpy_start;
	char *token;
	struct monitor  *m_use = monitor_get_current(), *m;

	action_cpy = strdup(action);
	action_cpy_start = action_cpy;
	token = PeekToken(action_cpy, &action_cpy);

	m = monitor_resolve_name(token);
	if (strcmp(m->si->name, token) != 0)
		m = m_use;
	else
		PeekToken(action, &action);

	free(action_cpy_start);

	/* FIXME: monitor needs broadcast when global. */

	if (MatchToken(action, "prev"))
	{
		val[0] = m->virtual_scr.prev_desk_and_page_desk;
		val[1] = m->virtual_scr.prev_desk_and_page_page_x;
		val[2] = m->virtual_scr.prev_desk_and_page_page_y;
	}
	else if (GetIntegerArguments(action, NULL, val, 3) == 3)
	{
		val[1] *= m->virtual_scr.MyDisplayWidth;
		val[2] *= m->virtual_scr.MyDisplayHeight;
	}
	else
	{
		return;
	}

	is_new_desk = (m->virtual_scr.CurrentDesk != val[0]);
	if (is_new_desk)
	{
		UnmapDesk(m, m->virtual_scr.CurrentDesk, True);
	}
	m->virtual_scr.prev_desk_and_page_page_x = m->virtual_scr.Vx;
	m->virtual_scr.prev_desk_and_page_page_y = m->virtual_scr.Vy;
	MoveViewport(m, val[1], val[2], True);
	if (is_new_desk)
	{
		m->virtual_scr.prev_desk = m->virtual_scr.CurrentDesk;
		m->virtual_scr.prev_desk_and_page_desk = m->virtual_scr.CurrentDesk;
		m->virtual_scr.CurrentDesk = val[0];

		/* If we're in global mode, store the virtual_scr across all
		 * monitors.  This will allow for per-monitor tracking to be
		 * toggled, as well as ensuring commands such as
		 * Goto{Page,Desk} operate correctly.
		 */
		monitor_assign_virtual(m);

		MapDesk(m, val[0], True);
		focus_grab_buttons_all();
		BroadcastPacket(M_NEW_DESK, 2, (long)m->virtual_scr.CurrentDesk,
			(long)m->si->rr_output);
		/* FIXME: domivogt (22-Apr-2000): Fake a 'restack' for sticky
		 * window upon desk change.  This is a workaround for a
		 * problem in FvwmPager: The pager has a separate 'root'
		 * window for each desk.  If the active desk changes, the
		 * pager destroys sticky mini windows and creates new ones in
		 * the other desktop 'root'.  But the pager can't know where to
		 * stack them.  So we have to tell it ecplicitly where they
		 * go :-( This should be fixed in the pager, but right now the
		 * pager doesn't the stacking order. */
		BroadcastRestackAllWindows();
	}
	else
	{
		BroadcastPacket(M_NEW_DESK, 2, (long)m->virtual_scr.CurrentDesk,
				(long)m->si->rr_output);
	}
	BroadcastMonitorList(NULL);
	EWMH_SetCurrentDesktop(m);

	return;
}

void CMD_GotoPage(F_CMD_ARGS)
{
	FvwmWindow * const fw = exc->w.fw;
	struct monitor	*m = (fw && fw->m) ? fw->m : monitor_get_current();
	struct monitor	*mpa;
	int x;
	int y;

	x = m->virtual_scr.Vx;
	y = m->virtual_scr.Vy;
	if (!get_page_arguments(fw, action, &x, &y, &mpa))
	{
		fvwm_debug(__func__,
			   "GotoPage: invalid arguments: %s", action);
		return;
	}

	m = mpa;

	if (x < 0)
	{
		x = 0;
	}
	if (x > m->virtual_scr.VxMax)
	{
		x = m->virtual_scr.VxMax;
	}
	if (y < 0)
	{
		y = 0;
	}
	if (y > m->virtual_scr.VyMax)
	{
		y = m->virtual_scr.VyMax;
	}
	MoveViewport(m, x,y,True);

	return;
}

/* function with parsing of command line */
void CMD_MoveToDesk(F_CMD_ARGS)
{
	int desk;
	FvwmWindow * const fw = exc->w.fw;

	desk = GetDeskNumber(fw->m, action, fw->Desk);
	if (desk == fw->Desk)
	{
		return;
	}
	do_move_window_to_desk(fw, desk);

	return;
}

void CMD_Scroll(F_CMD_ARGS)
{
	int x,y;
	int val1, val2, val1_unit, val2_unit;
	struct monitor  *m = monitor_get_current();

	if (GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit) != 2)
	{
		/* less then two integer parameters implies interactive
		 * scroll check if we are scrolling in reverse direction */
		char *option;
		int scroll_speed = 1;

		option = PeekToken(action, NULL);
		if (option != NULL)
		{
			if (StrEquals(option, "Reverse"))
			{
				scroll_speed *= -1;
			}
		}
		__drag_viewport(exc, scroll_speed);

		return;
	}
	if ((val1 > -100000)&&(val1 < 100000))
	{
		x = m->virtual_scr.Vx + val1*val1_unit/100;
	}
	else
	{
		x = m->virtual_scr.Vx + (val1/1000)*val1_unit/100;
	}
	if ((val2 > -100000)&&(val2 < 100000))
	{
		y=m->virtual_scr.Vy + val2*val2_unit/100;
	}
	else
	{
		y = m->virtual_scr.Vy + (val2/1000)*val2_unit/100;
	}
	if (((val1 <= -100000)||(val1 >= 100000))&&(x>m->virtual_scr.VxMax))
	{
		int xpixels;

		xpixels = (m->virtual_scr.VxMax / m->virtual_scr.MyDisplayWidth + 1) *
			m->virtual_scr.MyDisplayWidth;
		x %= xpixels;
		y += m->virtual_scr.MyDisplayHeight * (1+((x-m->virtual_scr.VxMax-1)/xpixels));
		if (y > m->virtual_scr.VyMax)
		{
			y %= (m->virtual_scr.VyMax / m->virtual_scr.MyDisplayHeight + 1) *
				m->virtual_scr.MyDisplayHeight;
		}
	}
	if (((val1 <= -100000)||(val1 >= 100000))&&(x<0))
	{
		x = m->virtual_scr.VxMax;
		y -= m->virtual_scr.MyDisplayHeight;
		if (y < 0)
		{
			y=m->virtual_scr.VyMax;
		}
	}
	if (((val2 <= -100000)||(val2>= 100000))&&(y>m->virtual_scr.VyMax))
	{
		int ypixels = (m->virtual_scr.VyMax / m->virtual_scr.MyDisplayHeight + 1) *
			m->virtual_scr.MyDisplayHeight;
		y %= ypixels;
		x += m->virtual_scr.MyDisplayWidth * (1+((y-m->virtual_scr.VyMax-1)/ypixels));
		if (x > m->virtual_scr.VxMax)
		{
			x %= (m->virtual_scr.VxMax / m->virtual_scr.MyDisplayWidth + 1) *
				m->virtual_scr.MyDisplayWidth;
		}
	}
	if (((val2 <= -100000)||(val2>= 100000))&&(y<0))
	{
		y = m->virtual_scr.VyMax;
		x -= m->virtual_scr.MyDisplayWidth;
		if (x < 0)
		{
			x=m->virtual_scr.VxMax;
		}
	}
	MoveViewport(m, x,y,True);

	return;
}

static int
number_of_desktops(struct monitor *m)
{
	DesktopsInfo	*d;
	int		 count = 0;

	d = m->Desktops->next;
	while (d != NULL) {
		count++;
		d = d->next;
	}

	return (count);
}

static void
store_desktop_cmd(int desk, char *name)
{
	struct desktop_cmd	*dc, *dc_loop;

	if (name == NULL) {
		fvwm_debug(__func__, "%s: name cannot be NULL\n", __func__);
		return;
	}

	TAILQ_FOREACH(dc_loop, &desktop_cmd_q, entry) {
		/* Update the name for an existing desktop, only if it
		 * differs.
		 */
		if (dc_loop->desk == desk && (strcmp(dc_loop->name, name) != 0)) {
			free(dc_loop->name);
			dc_loop->name = fxstrdup(name);

			return;
		}
	}

	dc = fxcalloc(1, sizeof *dc);
	dc->name = fxstrdup(name);
	dc->desk = desk;
	TAILQ_INSERT_TAIL(&desktop_cmd_q, dc, entry);
}

static struct desktop_fw *
desktop_fw_new(void)
{
	struct desktop_fw	*new;

	new = fxcalloc(1, sizeof *new);
	TAILQ_INIT(&new->desk_fvwmwin_q);

	return (new);
}

bool
desk_get_fw_urgent(struct monitor *m, int desk)
{
	struct desktop_fw	*df;
	struct desk_fvwmwin	*dfws;

	TAILQ_FOREACH(df, &desktop_fvwm_q, entry) {
		if (df->desk == desk) {
			TAILQ_FOREACH(dfws, &df->desk_fvwmwin_q, entry) {
				if (dfws == NULL || dfws->fw == NULL)
					continue;
				if (dfws->fw->m != m)
					continue;
				if (dfws->fw->wmhints != NULL &&
				    dfws->fw->wmhints->flags & XUrgencyHint)
					return (true);
			}
		}
	}
	return (false);
}

int
desk_get_fw_count(struct monitor *m, int desk)
{
	struct desktop_fw	*df;
	struct desk_fvwmwin	*dfws;
	window_style		 style;
	style_flags		*sflags;
	int 			 count = 0;

	TAILQ_FOREACH(df, &desktop_fvwm_q, entry) {
		if (df->desk == desk) {
			TAILQ_FOREACH(dfws, &df->desk_fvwmwin_q, entry) {
				if (dfws && dfws->fw && (dfws->fw->m != m))
					continue;
				lookup_style(dfws->fw, &style);
				sflags = SGET_FLAGS_POINTER(style);

				if (DO_SKIP_WINDOW_LIST(dfws->fw))
					continue;
				if (SIS_UNMANAGED(sflags))
					continue;
				count++;
			}
		}
	}

	return (count);
}


void
desk_add_fw(FvwmWindow *fw)
{
	struct desktop_fw	*df, *df_loop;
	struct desk_fvwmwin	*dfws;
	int			 desk = monitor_get_current()->virtual_scr.CurrentDesk;

	if (fw == NULL)
		return;

	if (TAILQ_EMPTY(&desktop_fvwm_q))
		TAILQ_INIT(&desktop_fvwm_q);

	desk_del_fw(fw);

	TAILQ_FOREACH(df_loop, &desktop_fvwm_q, entry) {
		if (df_loop->desk == desk) {
			TAILQ_FOREACH(dfws, &df_loop->desk_fvwmwin_q, entry) {
				if (dfws->fw == fw &&
				    dfws->fw->m == fw->m &&
				    dfws->fw->Desk == desk)
					break;
			}
			dfws = fxcalloc(1, sizeof *dfws);
			dfws->fw = fw;
			TAILQ_INSERT_TAIL(&df_loop->desk_fvwmwin_q, dfws, entry);
			return;
		}
	}

	df = desktop_fw_new();
	df->desk = desk;

	dfws = fxcalloc(1, sizeof *dfws);
	dfws->fw = fw;

	TAILQ_INSERT_TAIL(&df->desk_fvwmwin_q, dfws, entry);
	TAILQ_INSERT_TAIL(&desktop_fvwm_q, df, entry);
}

void
desk_del_fw(FvwmWindow *fw)
{
	struct desktop_fw	*df;
	struct desk_fvwmwin	*dfws;

	TAILQ_FOREACH(df, &desktop_fvwm_q, entry) {
		TAILQ_FOREACH(dfws, &df->desk_fvwmwin_q, entry) {
			if (dfws->fw == fw) {
				TAILQ_REMOVE(&df->desk_fvwmwin_q, dfws, entry);
				free(dfws);
				break;
			}
		}
	}
}

void
apply_desktops_monitor(struct monitor *m)

{
	DesktopsInfo	*t, *d, *new, **prev;
	struct desktop_cmd	*dc;

	TAILQ_FOREACH(dc, &desktop_cmd_q, entry) {
		d = m->Desktops->next;
		while (d != NULL && d->desk != dc->desk)
		{
			d = d->next;
		}

		if (d != NULL)
		{
			if (d->name != NULL)
			{
				free(d->name);
				d->name = NULL;
			}
			CopyString(&d->name, dc->name);
		}
		else
		{
			/* new deskops entries: add it in order */
			d = m->Desktops->next;
			t = m->Desktops;
			prev = &(m->Desktops->next);
			while (d != NULL && d->desk < dc->desk)
			{
				t = t->next;
				prev = &(d->next);
				d = d->next;
			}
			if (d == NULL)
			{
				/* add it at the end */
				*prev = fxcalloc(1, sizeof(DesktopsInfo));
				(*prev)->desk = dc->desk;

				CopyString(&((*prev)->name), dc->name);
			}
			else
			{
				/* instert it */
				new = fxcalloc(1, sizeof(DesktopsInfo));
				new->desk = dc->desk;
				CopyString(&(new->name), dc->name);
				t->next = new;
				new->next = d;
			}
			/* should check/set the working areas */
		}

		if (!fFvwmInStartup)
		{
			char *msg;
			/*const char *default_desk_name = _("Desk");*/

			/* should send the info to the FvwmPager and set the EWMH
			 * desktop names */
			{
				asprintf(&msg, "DesktopName %d %s",
					dc->desk, dc->name);
			}
#if 0
			else
			{
				/* TA:  FIXME!  xasprintf() */
				msg = fxmalloc(strlen(default_desk_name)+44);
				sprintf(
						msg, "DesktopName %d %s %d", desk,
						default_desk_name, desk);
			}
#endif
			BroadcastConfigInfoString(msg);
			free(msg);
		}
	}
	EWMH_SetDesktopNames(m);
}

/*
 *
 * Defines the name of a desktop
 *
 */
void CMD_DesktopName(F_CMD_ARGS)
{
	struct monitor	*m;
	int		 desk;

	if (GetIntegerArguments(action, &action, &desk, 1) != 1)
	{
		fvwm_debug(__func__,
		           "First argument to DesktopName must be an integer: %s",
		           action);
		return;
	}

	if (TAILQ_EMPTY(&desktop_cmd_q))
		TAILQ_INIT(&desktop_cmd_q);

	store_desktop_cmd(desk, action);

	TAILQ_FOREACH(m, &monitor_q, entry)
		apply_desktops_monitor(m);
}
