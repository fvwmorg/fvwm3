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

#include <X11/keysym.h>
#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FGettext.h"
#include "libs/Grab.h"
#include "libs/Parse.h"
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
static int prev_page_x = 0;
static int prev_page_y = 0;
static int prev_desk = 0;
static int prev_desk_and_page_desk = 0;
static int prev_desk_and_page_page_x = 0;
static int prev_desk_and_page_page_y = 0;

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
			MoveViewport(
				Scr.Vx + scroll_speed * (x - old_x),
				Scr.Vy + scroll_speed * (y - old_y), False);
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
static int GetDeskNumber(char *action, int current_desk)
{
	int n;
	int m;
	int is_relative;
	int desk;
	int val[4];
	int min, max;

	if (MatchToken(action, "prev"))
	{
		return prev_desk;
	}
	n = GetIntegerArguments(action, NULL, &(val[0]), 4);
	if (n <= 0)
	{
		return Scr.CurrentDesk;
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
static void UnmapDesk(int desk, Bool grab)
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
static void MapDesk(int desk, Bool grab)
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
		if ((Scr.VxMax == 0 ||
		     (x >= edge_thickness &&
		      x < Scr.MyDisplayWidth  - edge_thickness)) &&
		    (Scr.VyMax == 0 ||
		     (y >= edge_thickness &&
		      y < Scr.MyDisplayHeight - edge_thickness)))
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
	if (Scr.VxMax == 0 && Scr.VyMax == 0)
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

		/* check actual pointer location since PanFrames can get buried
		 * under window being moved or resized - mab */
		if (x >= edge_thickness &&
		    x < Scr.MyDisplayWidth-edge_thickness &&
		    y >= edge_thickness &&
		    y < Scr.MyDisplayHeight-edge_thickness)
		{
			is_timestamp_valid = False;
			add_time = 0;
			return 0;
		}
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
	if (x < edge_thickness)
	{
		*delta_x = -HorWarpSize;
	}
	else if (x >= Scr.MyDisplayWidth-edge_thickness)
	{
		*delta_x = HorWarpSize;
	}
	else
	{
		*delta_x = 0;
	}
	if (Scr.VxMax == 0)
	{
		*delta_x = 0;
	}
	if (y < edge_thickness)
	{
		*delta_y = -VertWarpSize;
	}
	else if (y >= Scr.MyDisplayHeight-edge_thickness)
	{
		*delta_y = VertWarpSize;
	}
	else
	{
		*delta_y = 0;
	}
	if (Scr.VyMax == 0)
	{
		*delta_y = 0;
	}

	/* Ouch! lots of bounds checking */
	if (Scr.Vx + *delta_x < 0)
	{
		if (!(Scr.flags.do_edge_wrap_x))
		{
			*delta_x = -Scr.Vx;
			*xl = x - *delta_x;
		}
		else
		{
			*delta_x += Scr.VxMax + Scr.MyDisplayWidth;
			*xl = x + *delta_x % Scr.MyDisplayWidth + HorWarpSize;
		}
	}
	else if (Scr.Vx + *delta_x > Scr.VxMax)
	{
		if (!(Scr.flags.do_edge_wrap_x))
		{
			*delta_x = Scr.VxMax - Scr.Vx;
			*xl = x - *delta_x;
		}
		else
		{
			*delta_x -= Scr.VxMax +Scr.MyDisplayWidth;
			*xl = x + *delta_x % Scr.MyDisplayWidth - HorWarpSize;
		}
	}
	else
	{
		*xl = x - *delta_x;
	}

	if (Scr.Vy + *delta_y < 0)
	{
		if (!(Scr.flags.do_edge_wrap_y))
		{
			*delta_y = -Scr.Vy;
			*yt = y - *delta_y;
		}
		else
		{
			*delta_y += Scr.VyMax + Scr.MyDisplayHeight;
			*yt = y + *delta_y % Scr.MyDisplayHeight + VertWarpSize;
		}
	}
	else if (Scr.Vy + *delta_y > Scr.VyMax)
	{
		if (!(Scr.flags.do_edge_wrap_y))
		{
			*delta_y = Scr.VyMax - Scr.Vy;
			*yt = y - *delta_y;
		}
		else
		{
			*delta_y -= Scr.VyMax + Scr.MyDisplayHeight;
			*yt = y + *delta_y % Scr.MyDisplayHeight - VertWarpSize;
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
	if (*xl < edge_thickness)
	{
		*xl = edge_thickness;
	}
	if (*yt < edge_thickness)
	{
		*yt = edge_thickness;
	}
	if (*xl >= Scr.MyDisplayWidth - edge_thickness)
	{
		*xl = Scr.MyDisplayWidth - edge_thickness -1;
	}
	if (*yt >= Scr.MyDisplayHeight - edge_thickness)
	{
		*yt = Scr.MyDisplayHeight - edge_thickness -1;
	}

	if (Grab)
	{
		MyXGrabServer(dpy);
	}
	/* Turn off the rubberband if its on */
	switch_move_resize_grid(False);
	FWarpPointer(dpy,None,Scr.Root,0,0,0,0,*xl,*yt);
	MoveViewport(Scr.Vx + *delta_x,Scr.Vy + *delta_y,False);
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
	/* Remove Pan frames if paging by edge-scroll is permanently or
	 * temporarily disabled */
	if (Scr.EdgeScrollX == 0 || Scr.VxMax == 0)
	{
		do_unmap_l = True;
		do_unmap_r = True;
	}
	if (Scr.EdgeScrollY == 0 || Scr.VyMax == 0)
	{
		do_unmap_t = True;
		do_unmap_b = True;
	}
	if (Scr.Vx == 0 && !Scr.flags.do_edge_wrap_x)
	{
		do_unmap_l = True;
	}
	if (Scr.Vx == Scr.VxMax && !Scr.flags.do_edge_wrap_x)
	{
		do_unmap_r = True;
	}
	if (Scr.Vy == 0 && !Scr.flags.do_edge_wrap_y)
	{
		do_unmap_t = True;
	}
	if (Scr.Vy == Scr.VyMax && !Scr.flags.do_edge_wrap_y)
	{
		do_unmap_b = True;
	}

	/* correct the unmap variables if pan frame commands are set */
	if (edge_thickness != 0)
	{
		if (Scr.PanFrameLeft.command != NULL || Scr.PanFrameLeft.command_leave != NULL)
		{
			do_unmap_l = False;
		}
		if (Scr.PanFrameRight.command != NULL || Scr.PanFrameRight.command_leave != NULL)
		{
			do_unmap_r = False;
		}
		if (Scr.PanFrameBottom.command != NULL || Scr.PanFrameBottom.command_leave != NULL)
		{
			do_unmap_b = False;
		}
		if (Scr.PanFrameTop.command != NULL || Scr.PanFrameTop.command_leave != NULL)
		{
			do_unmap_t = False;
		}
	}

	/*
	 * hide or show the windows
	 */

	/* left */
	if (do_unmap_l)
	{
		if (Scr.PanFrameLeft.isMapped)
		{
			XUnmapWindow(dpy, Scr.PanFrameLeft.win);
			Scr.PanFrameLeft.isMapped = False;
		}
	}
	else
	{
		if (edge_thickness != last_edge_thickness)
		{
			XResizeWindow(
				dpy, Scr.PanFrameLeft.win, edge_thickness,
				Scr.MyDisplayHeight);
		}
		if (!Scr.PanFrameLeft.isMapped)
		{
			XMapRaised(dpy, Scr.PanFrameLeft.win);
			Scr.PanFrameLeft.isMapped = True;
		}
	}
	/* right */
	if (do_unmap_r)
	{
		if (Scr.PanFrameRight.isMapped)
		{
			XUnmapWindow(dpy, Scr.PanFrameRight.win);
			Scr.PanFrameRight.isMapped = False;
		}
	}
	else
	{
		if (edge_thickness != last_edge_thickness)
		{
			XMoveResizeWindow(
				dpy, Scr.PanFrameRight.win,
				Scr.MyDisplayWidth - edge_thickness, 0,
				edge_thickness, Scr.MyDisplayHeight);
		}
		if (!Scr.PanFrameRight.isMapped)
		{
			XMapRaised(dpy, Scr.PanFrameRight.win);
			Scr.PanFrameRight.isMapped = True;
		}
	}
	/* top */
	if (do_unmap_t)
	{
		if (Scr.PanFrameTop.isMapped)
		{
			XUnmapWindow(dpy, Scr.PanFrameTop.win);
			Scr.PanFrameTop.isMapped = False;
		}
	}
	else
	{
		if (edge_thickness != last_edge_thickness)
		{
			XResizeWindow(
				dpy, Scr.PanFrameTop.win, Scr.MyDisplayWidth,
				edge_thickness);
		}
		if (!Scr.PanFrameTop.isMapped)
		{
			XMapRaised(dpy, Scr.PanFrameTop.win);
			Scr.PanFrameTop.isMapped = True;
		}
	}
	/* bottom */
	if (do_unmap_b)
	{
		if (Scr.PanFrameBottom.isMapped)
		{
			XUnmapWindow(dpy, Scr.PanFrameBottom.win);
			Scr.PanFrameBottom.isMapped = False;
		}
	}
	else
	{
		if (edge_thickness != last_edge_thickness)
		{
			XMoveResizeWindow(
				dpy, Scr.PanFrameBottom.win, 0,
				Scr.MyDisplayHeight - edge_thickness,
				Scr.MyDisplayWidth, edge_thickness);
		}
		if (!Scr.PanFrameBottom.isMapped)
		{
			XMapRaised(dpy, Scr.PanFrameBottom.win);
			Scr.PanFrameBottom.isMapped = True;
		}
	}
	last_edge_thickness = edge_thickness;

	return;
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
	Window windows[4];
	int n;

	/* Note: make sure the stacking order of the pan frames is not changed
	 * every time they are raised by using XRestackWindows. */
	n = 0;
	if (Scr.PanFrameTop.isMapped)
	{
		windows[n++] = Scr.PanFrameTop.win;
	}
	if (Scr.PanFrameLeft.isMapped)
	{
		windows[n++] = Scr.PanFrameLeft.win;
	}
	if (Scr.PanFrameRight.isMapped)
	{
		windows[n++] = Scr.PanFrameRight.win;
	}
	if (Scr.PanFrameBottom.isMapped)
	{
		windows[n++] = Scr.PanFrameBottom.win;
	}
	if (n > 0)
	{
		XRaiseWindow(dpy, windows[0]);
		if (n > 1)
		{
			XRestackWindows(dpy, windows, n);
		}
	}

	return;
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
	Scr.PanFrameTop.win = XCreateWindow(
		dpy, Scr.Root, 0, 0, Scr.MyDisplayWidth, edge_thickness,
		0, CopyFromParent, InputOnly, CopyFromParent, valuemask,
		&attributes);
	attributes.cursor = Scr.FvwmCursors[CRS_LEFT_EDGE];
	Scr.PanFrameLeft.win = XCreateWindow(
		dpy, Scr.Root, 0, 0, edge_thickness, Scr.MyDisplayHeight,
		0, CopyFromParent, InputOnly, CopyFromParent, valuemask,
		&attributes);
	attributes.cursor = Scr.FvwmCursors[CRS_RIGHT_EDGE];
	Scr.PanFrameRight.win = XCreateWindow(
		dpy, Scr.Root, Scr.MyDisplayWidth - edge_thickness, 0,
		edge_thickness, Scr.MyDisplayHeight, 0, CopyFromParent,
		InputOnly, CopyFromParent, valuemask, &attributes);
	attributes.cursor = Scr.FvwmCursors[CRS_BOTTOM_EDGE];
	Scr.PanFrameBottom.win = XCreateWindow(
		dpy, Scr.Root, 0, Scr.MyDisplayHeight - edge_thickness,
		Scr.MyDisplayWidth, edge_thickness, 0, CopyFromParent,
		InputOnly, CopyFromParent, valuemask, &attributes);
	Scr.PanFrameTop.isMapped=Scr.PanFrameLeft.isMapped=
		Scr.PanFrameRight.isMapped= Scr.PanFrameBottom.isMapped=False;
	edge_thickness = saved_thickness;

	return;
}

Bool is_pan_frame(Window w)
{
	if (w == Scr.PanFrameTop.win || w == Scr.PanFrameBottom.win ||
	    w == Scr.PanFrameLeft.win || w == Scr.PanFrameRight.win)
	{
		return True;
	}
	else
	{
		return False;
	}
}

/*
 *
 *  Moves the viewport within the virtual desktop
 *
 */
void MoveViewport(int newx, int newy, Bool grab)
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
	if (newx > Scr.VxMax)
	{
		newx = Scr.VxMax;
	}
	if (newy > Scr.VyMax)
	{
		newy = Scr.VyMax;
	}
	if (newx < 0)
	{
		newx = 0;
	}
	if (newy < 0)
	{
		newy = 0;
	}
	deltay = Scr.Vy - newy;
	deltax = Scr.Vx - newx;
	/*
	  Identify the bounding rectangle that will be moved into
	  the viewport.
	*/
	PageBottom    =  Scr.MyDisplayHeight - deltay - 1;
	PageRight     =  Scr.MyDisplayWidth  - deltax - 1;
	PageTop       =  0 - deltay;
	PageLeft      =  0 - deltax;
	if (deltax || deltay)
	{
		prev_page_x = Scr.Vx;
		prev_page_y = Scr.Vy;
		prev_desk_and_page_page_x = Scr.Vx;
		prev_desk_and_page_page_y = Scr.Vy;
		prev_desk_and_page_desk = Scr.CurrentDesk;
	}
	Scr.Vx = newx;
	Scr.Vy = newy;

	if (deltax || deltay)
	{
		BroadcastPacket(
			M_NEW_PAGE, 7, (long)Scr.Vx, (long)Scr.Vy,
			(long)Scr.CurrentDesk, (long)Scr.MyDisplayWidth,
			(long)Scr.MyDisplayHeight,
			(long)((Scr.VxMax / Scr.MyDisplayWidth) + 1),
			(long)((Scr.VyMax / Scr.MyDisplayHeight) + 1));

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
		t = get_next_window_in_stack_ring(&Scr.FvwmRoot);
		while (t != &Scr.FvwmRoot)
		{
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
	EWMH_SetDesktopViewPort();

	return;
}

void goto_desk(int desk)
{
	/* RBW - the unmapping operations are now removed to their own
	 * functions so they can also be used by the new GoToDeskAndPage
	 * command. */
	if (Scr.CurrentDesk != desk)
	{
		prev_desk = Scr.CurrentDesk;
		prev_desk_and_page_desk = Scr.CurrentDesk;
		prev_desk_and_page_page_x = Scr.Vx;
		prev_desk_and_page_page_y = Scr.Vy;
		UnmapDesk(Scr.CurrentDesk, True);
		Scr.CurrentDesk = desk;
		MapDesk(desk, True);
		focus_grab_buttons_all();
		BroadcastPacket(M_NEW_DESK, 1, (long)Scr.CurrentDesk);
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
		EWMH_SetCurrentDesktop();
	}

	return;
}

/*
 *
 * Move a window to a new desktop
 *
 */
void do_move_window_to_desk(FvwmWindow *fw, int desk)
{
	if (fw == NULL)
	{
		return;
	}

	/*
	 * Set the window's desktop, and map or unmap it as needed.
	 */
	/* Only change mapping for non-sticky windows */
	if (!is_window_sticky_across_desks(fw) /*&& !IS_ICON_UNMAPPED(fw)*/)
	{
		if (fw->Desk == Scr.CurrentDesk)
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
		else if (desk == Scr.CurrentDesk)
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

Bool get_page_arguments(char *action, int *page_x, int *page_y)
{
	int val[2];
	int suffix[2];
	char *token;
	char *taction;
	int wrapx;
	int wrapy;
	int limitdeskx;
	int limitdesky;

	wrapx = 0;
	wrapy = 0;
	limitdeskx = 1;
	limitdesky = 1;
	for (; ; action = taction)
	{
		int do_reverse;

		token = PeekToken(action, &taction);
		if (token == NULL)
		{
			*page_x = Scr.Vx;
			*page_y = Scr.Vy;
			return True;
		}
		if (StrEquals(token, "prev"))
		{
			/* last page selected */
			*page_x = prev_page_x;
			*page_y = prev_page_y;
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
		*page_x = val[0] * Scr.MyDisplayWidth + Scr.Vx;
	}
	else if (suffix[0] == 2)
	{
		*page_x += val[0] * Scr.MyDisplayWidth;
	}
	else if (val[0] >= 0)
	{
		*page_x = val[0] * Scr.MyDisplayWidth;
	}
	else
	{
		*page_x = (val[0] + 1) * Scr.MyDisplayWidth + Scr.VxMax;
	}
	if (suffix[1] == 1)
	{
		*page_y = val[1] * Scr.MyDisplayHeight + Scr.Vy;
	}
	else if (suffix[1] == 2)
	{
		*page_y += val[1] * Scr.MyDisplayHeight;
	}
	else if (val[1] >= 0)
	{
		*page_y = val[1] * Scr.MyDisplayHeight;
	}
	else
	{
		*page_y = (val[1] + 1) * Scr.MyDisplayHeight + Scr.VyMax;
	}

	/* limit to desktop size */
	if (limitdeskx && !wrapx)
	{
		if (*page_x < 0)
		{
			*page_x = 0;
		}
		else if (*page_x > Scr.VxMax)
		{
			*page_x = Scr.VxMax;
		}
	}
	else if (limitdeskx && wrapx)
	{
		while (*page_x < 0)
		{
			*page_x += Scr.VxMax + Scr.MyDisplayWidth;
		}
		while (*page_x > Scr.VxMax)
		{
			*page_x -= Scr.VxMax + Scr.MyDisplayWidth;
		}
	}
	if (limitdesky && !wrapy)
	{
		if (*page_y < 0)
		{
			*page_y = 0;
		}
		else if (*page_y > Scr.VyMax)
		{
			*page_y = Scr.VyMax;
		}
	}
	else if (limitdesky && wrapy)
	{
		while (*page_y < 0)
		{
			*page_y += Scr.VyMax + Scr.MyDisplayHeight;
		}
		while (*page_y > Scr.VyMax)
		{
			*page_y -= Scr.VyMax + Scr.MyDisplayHeight;
		}
	}

	return True;
}

char *GetDesktopName(int desk)
{
	DesktopsInfo *d;

	d = Scr.Desktops->next;
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
	char * command;

	/* get the direction */
	direction = gravity_parse_dir_argument(action, &action, DIR_NONE);

	if (direction >= 0 && direction <= DIR_MAJOR_MASK)
	{

		/* check if the command does contain at least one token */
		command = xstrdup(action);
		if (PeekToken(action , &action) == NULL)
		{
			/* the command does not contain a token so
			   the command of this edge is removed */
			free(command);
			command = NULL;
		}
		/* assign command to the edge(s) */
		if (direction == DIR_N)
		{
			if (Scr.PanFrameTop.command != NULL)
			{
				free(Scr.PanFrameTop.command);
			}
			Scr.PanFrameTop.command = command;
		}
		else if (direction == DIR_S)
		{
			if (Scr.PanFrameBottom.command != NULL)
			{
				free(Scr.PanFrameBottom.command);
			}
			Scr.PanFrameBottom.command = command;
		}
		else if (direction == DIR_W)
		{
			if (Scr.PanFrameLeft.command != NULL)
			{
				free(Scr.PanFrameLeft.command);
			}
			Scr.PanFrameLeft.command = command;
		}
		else if (direction == DIR_E)
		{
			if (Scr.PanFrameRight.command != NULL)
			{
				free(Scr.PanFrameRight.command);
			}
			Scr.PanFrameRight.command = command;
		}
		else
		{
			/* this should never happen */
			fvwm_msg(ERR, "EdgeCommand",
				 "Internal error in CMD_EdgeCommand");
		}

	}
	else
	{

		/* check if the argument does contain at least one token */
		if (PeekToken(action , &action) == NULL)
		{
			/* Just plain EdgeCommand, so all edge commands are
			 * removed */
			if (Scr.PanFrameTop.command != NULL)
			{
				free(Scr.PanFrameTop.command);
				Scr.PanFrameTop.command = NULL;
			}
			if (Scr.PanFrameBottom.command != NULL)
			{
				free(Scr.PanFrameBottom.command);
				Scr.PanFrameBottom.command = NULL;
			}
			if (Scr.PanFrameLeft.command != NULL)
			{
				free(Scr.PanFrameLeft.command);
				Scr.PanFrameLeft.command = NULL;
			}
			if (Scr.PanFrameRight.command != NULL)
			{
				free(Scr.PanFrameRight.command);
				Scr.PanFrameRight.command = NULL;
			}
		}
		else
		{
			/* not a proper direction */
			fvwm_msg(ERR, "EdgeCommand",
				 "EdgeCommand [direction [function]]");
		}
	}

	/* maybe something has changed so we adapt the pan frames */
	checkPanFrames();

	return;
}

/* EdgeLeaveCommand - binds a function to a pan frame Leave event */
void CMD_EdgeLeaveCommand(F_CMD_ARGS)
{
	direction_t direction;
	char * command;

	/* get the direction */
	direction = gravity_parse_dir_argument(action, &action, DIR_NONE);

	if (direction >= 0 && direction <= DIR_MAJOR_MASK)
	{

		/* check if the command does contain at least one token */
		command = xstrdup(action);
		if (PeekToken(action , &action) == NULL)
		{
			/* the command does not contain a token so
			   the command of this edge is removed */
			free(command);
			command = NULL;
		}
		/* assign command to the edge(s) */
		if (direction == DIR_N)
		{
			if (Scr.PanFrameTop.command_leave != NULL)
			{
				free(Scr.PanFrameTop.command_leave);
			}
			Scr.PanFrameTop.command_leave = command;
		}
		else if (direction == DIR_S)
		{
			if (Scr.PanFrameBottom.command_leave != NULL)
			{
				free(Scr.PanFrameBottom.command_leave);
			}
			Scr.PanFrameBottom.command_leave = command;
		}
		else if (direction == DIR_W)
		{
			if (Scr.PanFrameLeft.command_leave != NULL)
			{
				free(Scr.PanFrameLeft.command_leave);
			}
			Scr.PanFrameLeft.command_leave = command;
		}
		else if (direction == DIR_E)
		{
			if (Scr.PanFrameRight.command_leave != NULL)
			{
				free(Scr.PanFrameRight.command_leave);
			}
			Scr.PanFrameRight.command_leave = command;
		}
		else
		{
			/* this should never happen */
			fvwm_msg(ERR, "EdgeLeaveCommand",
				 "Internal error in CMD_EdgeLeaveCommand");
		}

	}
	else
	{

		/* check if the argument does contain at least one token */
		if (PeekToken(action , &action) == NULL)
		{
			/* Just plain EdgeLeaveCommand, so all edge commands are
			 * removed */
			if (Scr.PanFrameTop.command_leave != NULL)
			{
				free(Scr.PanFrameTop.command_leave);
				Scr.PanFrameTop.command_leave = NULL;
			}
			if (Scr.PanFrameBottom.command_leave != NULL)
			{
				free(Scr.PanFrameBottom.command_leave);
				Scr.PanFrameBottom.command_leave = NULL;
			}
			if (Scr.PanFrameLeft.command_leave != NULL)
			{
				free(Scr.PanFrameLeft.command_leave);
				Scr.PanFrameLeft.command_leave = NULL;
			}
			if (Scr.PanFrameRight.command_leave != NULL)
			{
				free(Scr.PanFrameRight.command_leave);
				Scr.PanFrameRight.command_leave = NULL;
			}
		}
		else
		{
			/* not a proper direction */
			fvwm_msg(ERR, "EdgeLeaveCommand",
				 "EdgeLeaveCommand [direction [function]]");
		}
	}

	/* maybe something has changed so we adapt the pan frames */
	checkPanFrames();

	return;
}

void CMD_EdgeThickness(F_CMD_ARGS)
{
	int val, n;

	n = GetIntegerArguments(action, NULL, &val, 1);
	if (n != 1)
	{
		fvwm_msg(ERR,"setEdgeThickness",
			 "EdgeThickness requires 1 numeric argument,"
			 " found %d args",n);
		return;
	}
		/* check range */
	if (val < 0 || val > 2)
	{
		fvwm_msg(ERR,"setEdgeThickness",
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

	n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
	if (n != 2)
	{
		fvwm_msg(
			ERR, "SetEdgeScroll",
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

	Scr.EdgeScrollX = val1 * val1_unit / 100;
	Scr.EdgeScrollY = val2 * val2_unit / 100;

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
		fvwm_msg(
			OLD, "CMD_EdgeResistance",
			"The command EdgeResistance with three arguments is"
			" obsolete. Please use the following commands"
			" instead:\n%s\n%s\n%s\n", cmd, stylecmd, stylecmd2);
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
		fvwm_msg(
			ERR, "CMD_EdgeResistance",
			"EdgeResistance requires two or three arguments");
		return;
	}

	return;
}

void CMD_Xinerama(F_CMD_ARGS)
{
	int toggle;

	toggle = ParseToggleArgument(action, NULL, -1, 0);
	if (toggle == -1)
	{
		toggle = !FScreenIsEnabled();
	}
	if (!toggle != !FScreenIsEnabled())
	{
		Scr.flags.do_need_window_update = True;
		Scr.flags.has_xinerama_state_changed = True;
		FScreenOnOff(toggle);
		broadcast_xinerama_state();
	}

	return;
}

void CMD_XineramaPrimaryScreen(F_CMD_ARGS)
{
	if (FScreenIsEnabled())
	{
		Scr.flags.do_need_window_update = True;
		Scr.flags.has_xinerama_state_changed = True;
	}
	broadcast_xinerama_state();

	return;
}

void CMD_DesktopSize(F_CMD_ARGS)
{
	int val[2];

	if (GetIntegerArguments(action, NULL, val, 2) != 2 &&
	    GetRectangleArguments(action, &val[0], &val[1]) != 2)
	{
		fvwm_msg(ERR, "CMD_DesktopSize",
			 "DesktopSize requires two arguments");
		return;
	}

	Scr.VxMax = (val[0] <= 0) ?
		0: val[0]*Scr.MyDisplayWidth-Scr.MyDisplayWidth;
	Scr.VyMax = (val[1] <= 0) ?
		0: val[1]*Scr.MyDisplayHeight-Scr.MyDisplayHeight;
	BroadcastPacket(
		M_NEW_PAGE, 7, (long)Scr.Vx, (long)Scr.Vy,
		(long)Scr.CurrentDesk, (long)Scr.MyDisplayWidth,
		(long)Scr.MyDisplayHeight,
		(long)((Scr.VxMax / Scr.MyDisplayWidth) + 1),
		(long)((Scr.VyMax / Scr.MyDisplayHeight) + 1));

	checkPanFrames();
	EWMH_SetDesktopGeometry();

	return;
}

/*
 *
 * Move to a new desktop
 *
 */
void CMD_GotoDesk(F_CMD_ARGS)
{
	goto_desk(GetDeskNumber(action, Scr.CurrentDesk));

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

	if (MatchToken(action, "prev"))
	{
		val[0] = prev_desk_and_page_desk;
		val[1] = prev_desk_and_page_page_x;
		val[2] = prev_desk_and_page_page_y;
	}
	else if (GetIntegerArguments(action, NULL, val, 3) == 3)
	{
		val[1] *= Scr.MyDisplayWidth;
		val[2] *= Scr.MyDisplayHeight;
	}
	else
	{
		return;
	}

	is_new_desk = (Scr.CurrentDesk != val[0]);
	if (is_new_desk)
	{
		UnmapDesk(Scr.CurrentDesk, True);
	}
	prev_desk_and_page_page_x = Scr.Vx;
	prev_desk_and_page_page_y = Scr.Vy;
	MoveViewport(val[1], val[2], True);
	if (is_new_desk)
	{
		prev_desk = Scr.CurrentDesk;
		prev_desk_and_page_desk = Scr.CurrentDesk;
		Scr.CurrentDesk = val[0];
		MapDesk(val[0], True);
		focus_grab_buttons_all();
		BroadcastPacket(M_NEW_DESK, 1, (long)Scr.CurrentDesk);
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
		BroadcastPacket(M_NEW_DESK, 1, (long)Scr.CurrentDesk);
	}
	EWMH_SetCurrentDesktop();

	return;
}

void CMD_GotoPage(F_CMD_ARGS)
{
	int x;
	int y;

	x = Scr.Vx;
	y = Scr.Vy;
	if (!get_page_arguments(action, &x, &y))
	{
		fvwm_msg(
			ERR, "goto_page_func",
			"GotoPage: invalid arguments: %s", action);
		return;
	}
	if (x < 0)
	{
		x = 0;
	}
	if (x > Scr.VxMax)
	{
		x = Scr.VxMax;
	}
	if (y < 0)
	{
		y = 0;
	}
	if (y > Scr.VyMax)
	{
		y = Scr.VyMax;
	}
	MoveViewport(x,y,True);

	return;
}

/* function with parsing of command line */
void CMD_MoveToDesk(F_CMD_ARGS)
{
	int desk;
	FvwmWindow * const fw = exc->w.fw;

	desk = GetDeskNumber(action, fw->Desk);
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
		x = Scr.Vx + val1*val1_unit/100;
	}
	else
	{
		x = Scr.Vx + (val1/1000)*val1_unit/100;
	}
	if ((val2 > -100000)&&(val2 < 100000))
	{
		y=Scr.Vy + val2*val2_unit/100;
	}
	else
	{
		y = Scr.Vy + (val2/1000)*val2_unit/100;
	}
	if (((val1 <= -100000)||(val1 >= 100000))&&(x>Scr.VxMax))
	{
		int xpixels;

		xpixels = (Scr.VxMax / Scr.MyDisplayWidth + 1) *
			Scr.MyDisplayWidth;
		x %= xpixels;
		y += Scr.MyDisplayHeight * (1+((x-Scr.VxMax-1)/xpixels));
		if (y > Scr.VyMax)
		{
			y %= (Scr.VyMax / Scr.MyDisplayHeight + 1) *
				Scr.MyDisplayHeight;
		}
	}
	if (((val1 <= -100000)||(val1 >= 100000))&&(x<0))
	{
		x = Scr.VxMax;
		y -= Scr.MyDisplayHeight;
		if (y < 0)
		{
			y=Scr.VyMax;
		}
	}
	if (((val2 <= -100000)||(val2>= 100000))&&(y>Scr.VyMax))
	{
		int ypixels = (Scr.VyMax / Scr.MyDisplayHeight + 1) *
			Scr.MyDisplayHeight;
		y %= ypixels;
		x += Scr.MyDisplayWidth * (1+((y-Scr.VyMax-1)/ypixels));
		if (x > Scr.VxMax)
		{
			x %= (Scr.VxMax / Scr.MyDisplayWidth + 1) *
				Scr.MyDisplayWidth;
		}
	}
	if (((val2 <= -100000)||(val2>= 100000))&&(y<0))
	{
		y = Scr.VyMax;
		x -= Scr.MyDisplayWidth;
		if (x < 0)
		{
			x=Scr.VxMax;
		}
	}
	MoveViewport(x,y,True);

	return;
}

/*
 *
 * Defines the name of a desktop
 *
 */
void CMD_DesktopName(F_CMD_ARGS)
{
	int desk;
	DesktopsInfo *t, *d, *new, **prev;

	if (GetIntegerArguments(action, &action, &desk, 1) != 1)
	{
		fvwm_msg(
			ERR,"CMD_DesktopName",
			"First argument to DesktopName must be an integer: %s",
			action);
		return;
	}

	d = Scr.Desktops->next;
	while (d != NULL && d->desk != desk)
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
		if (action != NULL && *action && *action != '\n')
		{
			CopyString(&d->name, action);
		}
	}
	else
	{
		/* new deskops entries: add it in order */
		d = Scr.Desktops->next;
		t = Scr.Desktops;
		prev = &(Scr.Desktops->next);
		while (d != NULL && d->desk < desk)
		{
			t = t->next;
			prev = &(d->next);
			d = d->next;
		}
		if (d == NULL)
		{
			/* add it at the end */
			*prev = xcalloc(1, sizeof(DesktopsInfo));
			(*prev)->desk = desk;
			if (action != NULL && *action && *action != '\n')
			{
				CopyString(&((*prev)->name), action);
			}
		}
		else
		{
			/* instert it */
			new = xcalloc(1, sizeof(DesktopsInfo));
			new->desk = desk;
			if (action != NULL && *action && *action != '\n')
			{
				CopyString(&(new->name), action);
			}
			t->next = new;
			new->next = d;
		}
		/* should check/set the working areas */
	}

	if (!fFvwmInStartup)
	{
		char *msg;
		const char *default_desk_name = _("Desk");

		/* should send the info to the FvwmPager and set the EWMH
		 * desktop names */
		if (action != NULL && *action && *action != '\n')
		{
			/* TA:  FIXME!  xasprintf() */
			msg = xmalloc(strlen(action) + 44);
			sprintf(msg, "DesktopName %d %s", desk, action);
		}
		else
		{
			/* TA:  FIXME!  xasprintf() */
			msg = xmalloc(strlen(default_desk_name)+44);
			sprintf(
				msg, "DesktopName %d %s %d", desk,
				default_desk_name, desk);
		}
		BroadcastConfigInfoString(msg);
		free(msg);
	}
	EWMH_SetDesktopNames();

	return;
}
