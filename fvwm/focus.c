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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/* ---------------------------- included header files ----------------------- */

#include <config.h>
#include <stdio.h>

#include "libs/fvwmlib.h"
#include <libs/gravity.h>
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "focus.h"
#include "icons.h"
#include "borders.h"
#include "frame.h"
#include "virtual.h"
#include "stack.h"
#include "geometry.h"
#include "colormaps.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static Bool lastFocusType;
/* Last window which Fvwm gave the focus to NOT the window that really has the
 * focus */
static FvwmWindow *ScreenFocus = NULL;
/* Window which had focus before the pointer moved to a different screen. */
static FvwmWindow *LastScreenFocus = NULL;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static void SetPointerEventPosition(XEvent *eventp, int x, int y)
{
	switch (eventp->type)
	{
	case ButtonPress:
	case ButtonRelease:
	case KeyPress:
	case KeyRelease:
	case MotionNotify:
		eventp->xbutton.x_root = x;
		eventp->xbutton.y_root = y;
		eventp->xmotion.same_screen = True;
		break;
	default:
		break;
	}

	return;
}

/********************************************************************
 *
 * Sets the input focus to the indicated window.
 *
 **********************************************************************/
static void DoSetFocus(
	Window w, FvwmWindow *fw, Bool FocusByMouse, Bool NoWarp,
	Bool do_allow_force_broadcast)
{
	extern Time lastTimestamp;
	FvwmWindow *sf;

	if (fw && WM_TAKES_FOCUS(fw))
	{
		send_clientmessage(dpy, w, _XA_WM_TAKE_FOCUS, lastTimestamp);
	}
	if (fw && HAS_NEVER_FOCUS(fw))
	{
		if (WM_TAKES_FOCUS(fw))
		{
			/* give it a chance to take the focus itself */
			XFlush(dpy);
		}
		else
		{
			/* make sure the window is not hilighted */
			border_draw_decorations(
				fw, PART_ALL, False, False, CLEAR_ALL, NULL,
				NULL);
		}
		return;
	}
	if (fw && !FP_IS_LENIENT(FW_FOCUS_POLICY(fw)) &&
	    fw->wmhints && (fw->wmhints->flags & InputHint) &&
	    !fw->wmhints->input &&
	    (sf = get_focus_window()) && sf->Desk == Scr.CurrentDesk)
	{
		return;
	}
	/* ClickToFocus focus queue manipulation - only performed for
	 * Focus-by-mouse type focus events */
	/* Watch out: fw may not be on the windowlist and the windowlist may be
	 * empty */
	if (fw && fw != get_focus_window() && fw != &Scr.FvwmRoot)
	{
		/* pluck window from list and deposit at top */
		if (FocusByMouse)
		{
			/* remove fw from list */
			if (fw->prev)
			{
				fw->prev->next = fw->next;
			}
			if (fw->next)
			{
				fw->next->prev = fw->prev;
			}

			/* insert fw at start */
			fw->next = Scr.FvwmRoot.next;
			if (Scr.FvwmRoot.next)
			{
				Scr.FvwmRoot.next->prev = fw;
			}
			Scr.FvwmRoot.next = fw;
			fw->prev = &Scr.FvwmRoot;
		}
		else
		{
			/* move the windowlist around so that fw is at the top
			 */
			FvwmWindow *fw2;

			/* find the window on the windowlist */
			fw2 = &Scr.FvwmRoot;
			while (fw2 && fw2 != fw)
			{
				fw2 = fw2->next;
			}

			if (fw2)
			{
				/* the window is on the (non-zero length)
				 * windowlist */
				/* make fw2 point to the last window on the
				 * list */
				while (fw2->next)
				{
					fw2 = fw2->next;
				}

				/* close the ends of the windowlist */
				fw2->next = Scr.FvwmRoot.next;
				Scr.FvwmRoot.next->prev = fw2;

				/* make fw the new start of the list */
				Scr.FvwmRoot.next = fw;
				/* open the closed loop windowlist */
				fw->prev->next = NULL;
				fw->prev = &Scr.FvwmRoot;
			}
		}
	}
	lastFocusType = FocusByMouse;

	if (!fw && !Scr.flags.is_pointer_on_this_screen)
	{
		focus_grab_buttons(Scr.Ungrabbed, False);
		set_focus_window(NULL);
		/* DV (25-Nov-2000): Don't give the Scr.NoFocusWin the focus
		 * here. This would steal the focus from the other screen's
		 * root window again. */
		/* FOCUS_SET(Scr.NoFocusWin); */
		return;
	}

	if (fw && !NoWarp)
	{
		if (IS_ICONIFIED(fw))
		{
			rectangle r;
			Bool rc;

			rc = get_visible_icon_geometry(fw, &r);
			if (!rc || !IsRectangleOnThisPage(&r, fw->Desk))
			{
				fw = NULL;
				w = Scr.NoFocusWin;
			}
		}
		else if (!IsRectangleOnThisPage(&(fw->frame_g),fw->Desk))
		{
			fw = NULL;
			w = Scr.NoFocusWin;
		}
	}

	/*
	 * RBW - 1999/12/08 - we have to re-grab the unfocused window here for
	 * the MouseFocusClickRaises case also, but we can't ungrab the newly
	 * focused window here, or we'll never catch the raise click. For this
	 * special case, the newly-focused window is ungrabbed in events.c
	 * (HandleButtonPress). */
	if (Scr.Ungrabbed != fw)
	{
		/* need to grab all buttons for window that we are about to
		 * unfocus */
		focus_grab_buttons(Scr.Ungrabbed, False);
	}
	/* if we do click to focus, remove the grab on mouse events that
	 * was made to detect the focus change */
	if (fw && HAS_CLICK_FOCUS(fw))
	{
		focus_grab_buttons(fw, True);
	}
	/* RBW - allow focus to go to a NoIconTitle icon window so
	 * auto-raise will work on it... */
	if (fw && IS_ICONIFIED(fw))
	{
		Bool is_window_selected = False;

		if (FW_W_ICON_TITLE(fw))
		{
			w = FW_W_ICON_TITLE(fw);
			is_window_selected = True;
		}
		if ((!is_window_selected || WAS_ICON_HINT_PROVIDED(fw)) &&
		    FW_W_ICON_PIXMAP(fw))
		{
			w = FW_W_ICON_PIXMAP(fw);
		}
	}

	if (fw && FP_IS_LENIENT(FW_FOCUS_POLICY(fw)))
	{
		FOCUS_SET(w);
		set_focus_window(fw);
		if (do_allow_force_broadcast)
		{
			SET_FOCUS_CHANGE_BROADCAST_PENDING(fw, 1);
		}
		Scr.UnknownWinFocused = None;
	}
	else if (!fw || !(fw->wmhints) || !(fw->wmhints->flags & InputHint) ||
		 fw->wmhints->input != False)
	{
		/* Window will accept input focus */
		FOCUS_SET(w);
		set_focus_window(fw);
		if (fw)
		{
			if (do_allow_force_broadcast)
			{
				SET_FOCUS_CHANGE_BROADCAST_PENDING(fw, 1);
			}
		}
		Scr.UnknownWinFocused = None;
	}
	else if ((sf = get_focus_window()) && sf->Desk == Scr.CurrentDesk)
	{
		/* Window doesn't want focus. Leave focus alone */
		/* FOCUS_SET(FW_W(Scr.Hilite));*/
	}
	else
	{
		FOCUS_SET(Scr.NoFocusWin);
		set_focus_window(NULL);
	}
	XFlush(dpy);

	return;
}

static void MoveFocus(
	Window w, FvwmWindow *fw, Bool FocusByMouse, Bool NoWarp, Bool do_force,
	Bool do_allow_force_broadcast)
{
	FvwmWindow *ffw_old = get_focus_window();
	Bool accepts_input_focus = do_accept_input_focus(fw);
#if 0
	FvwmWindow *ffw_new;
#endif

	if (!do_force && fw == ffw_old)
	{
		if (ffw_old)
		{
#if 0
			/*
			  RBW - 2001/08/17 - For a MouseFocusClickRaises win,
			  * we must not drop the grab, since the (potential)
			  * click hasn't happened yet. */
			if ((!HAS_SLOPPY_FOCUS(ffw_old) &&
			     !HAS_MOUSE_FOCUS(ffw_old)) ||
			    !DO_RAISE_MOUSE_FOCUS_CLICK(ffw_old))
			{
				focus_grab_buttons(ffw_old, True);
			}
#else
			focus_grab_buttons(ffw_old, True);
#endif
		}
		return;
	}
	DoSetFocus(w, fw, FocusByMouse, NoWarp, do_allow_force_broadcast);
	if (get_focus_window() != ffw_old)
	{
		if (accepts_input_focus)
		{
#if 0
			/*  RBW - Ibid.  */
			ffw_new = get_focus_window();
			if (ffw_new)
			{
				if ((!HAS_SLOPPY_FOCUS(ffw_new) &&
				     !HAS_MOUSE_FOCUS(ffw_new)) ||
				    !DO_RAISE_MOUSE_FOCUS_CLICK(ffw_new))
				{
					focus_grab_buttons(ffw_new, True);
				}
			}
#else
			focus_grab_buttons(get_focus_window(), True);
#endif
		}
		focus_grab_buttons(ffw_old, False);
	}
}

/**************************************************************************
 *
 * Moves pointer to specified window
 *
 *************************************************************************/
static void warp_to_fvwm_window(
	XEvent *eventp, FvwmWindow *t, int warp_x, int x_unit, int warp_y,
	int y_unit)
{
	int dx,dy;
	int cx,cy;
	int x,y;

	if (t == (FvwmWindow *)0 ||
	    (IS_ICONIFIED(t) && FW_W_ICON_TITLE(t) == None))
	{
		return;
	}

	if (t->Desk != Scr.CurrentDesk)
	{
		goto_desk(t->Desk);
	}

	if (IS_ICONIFIED(t))
	{
		rectangle g;
		Bool rc;

		rc = get_visible_icon_title_geometry(t, &g);
		if (rc == False)
		{
			get_visible_icon_picture_geometry(t, &g);
		}
		cx = g.x + g.width / 2;
		cy = g.y + g.height / 2;
	}
	else
	{
		cx = t->frame_g.x + t->frame_g.width/2;
		cy = t->frame_g.y + t->frame_g.height/2;
	}

	dx = (cx + Scr.Vx) / Scr.MyDisplayWidth * Scr.MyDisplayWidth;
	dy = (cy + Scr.Vy) / Scr.MyDisplayHeight * Scr.MyDisplayHeight;

	MoveViewport(dx,dy,True);

	if (IS_ICONIFIED(t))
	{
		rectangle g;
		Bool rc;

		rc = get_visible_icon_title_geometry(t, &g);
		if (rc == False)
		{
			get_visible_icon_picture_geometry(t, &g);
		}
		x = g.x + g.width / 2;
		y = g.y + g.height / 2;
	}
	else
	{
		if (x_unit != Scr.MyDisplayWidth && warp_x >= 0)
		{
			x = t->frame_g.x + warp_x;
		}
		else if (x_unit != Scr.MyDisplayWidth)
		{
			x = t->frame_g.x + t->frame_g.width + warp_x;
		}
		else if (warp_x >= 0)
		{
			x = t->frame_g.x +
				(t->frame_g.width - 1) * warp_x / 100;
		}
		else
		{
			x = t->frame_g.x +
				(t->frame_g.width - 1) * (100 + warp_x) / 100;
		}

		if (y_unit != Scr.MyDisplayHeight && warp_y >= 0)
		{
			y = t->frame_g.y + warp_y;
		}
		else if (y_unit != Scr.MyDisplayHeight)
		{
			y = t->frame_g.y + t->frame_g.height + warp_y;
		}
		else if (warp_y >= 0)
		{
			y = t->frame_g.y +
				(t->frame_g.height - 1) * warp_y / 100;
		}
		else
		{
			y = t->frame_g.y +
				(t->frame_g.height - 1) * (100 + warp_y) / 100;
		}
	}
	XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x, y);
	SetPointerEventPosition(eventp, x, y);
	RaiseWindow(t);

	/* If the window is still not visible, make it visible! */
	if (t->frame_g.x + t->frame_g.width  < 0 ||
	    t->frame_g.y + t->frame_g.height < 0 ||
	    t->frame_g.x >= Scr.MyDisplayWidth   ||
	    t->frame_g.y >= Scr.MyDisplayHeight)
	{
		frame_setup_window(
			t, 0, 0, t->frame_g.width, t->frame_g.height, False);
		XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
		SetPointerEventPosition(eventp, 2, 2);
	}
}

static Bool focus_query_grab_buttons(FvwmWindow *fw, Bool is_focused)
{
	Bool flag;

	if (!is_focused && FP_DO_FOCUS_CLICK_CLIENT(FW_FOCUS_POLICY(fw)))
	{
		return True;
	}
	if (is_on_top_of_layer(fw))
	{
		return False;
	}
	if (is_focused)
	{
		flag = FP_DO_RAISE_FOCUSED_CLIENT_CLICK(FW_FOCUS_POLICY(fw));
	}
	else
	{
		flag = FP_DO_RAISE_UNFOCUSED_CLIENT_CLICK(FW_FOCUS_POLICY(fw));
	}

	return (flag) ? True : False;
}

/* ---------------------------- interface functions ------------------------- */

Bool do_accept_input_focus(FvwmWindow *fw)
{
	return (!fw || !fw->wmhints ||
		!(fw->wmhints->flags & InputHint) || fw->wmhints->input);
}

Bool focus_is_focused(FvwmWindow *fw)
{
	return (fw && fw == ScreenFocus);
}

Bool focus_query_click_to_raise(
	FvwmWindow *fw, Bool is_focused, Bool is_client_click)
{
	Bool flag;

	if (is_focused && is_client_click)
	{
		flag = FP_DO_RAISE_FOCUSED_CLIENT_CLICK(FW_FOCUS_POLICY(fw));
	}
	else if (is_focused && !is_client_click)
	{
		flag = FP_DO_RAISE_FOCUSED_DECOR_CLICK(FW_FOCUS_POLICY(fw));
	}
	else if (!is_focused && is_client_click)
	{
		flag = FP_DO_RAISE_UNFOCUSED_CLIENT_CLICK(FW_FOCUS_POLICY(fw));
	}
	else if (!is_focused && !is_client_click)
	{
		flag = FP_DO_RAISE_UNFOCUSED_DECOR_CLICK(FW_FOCUS_POLICY(fw));
	}

	return (flag) ? True : False;
}

void SetFocusWindow(
	FvwmWindow *fw, Bool FocusByMouse, Bool do_allow_force_broadcast)
{
	MoveFocus(
		FW_W(fw), fw, FocusByMouse, False, True,
		do_allow_force_broadcast);
}

void ReturnFocusWindow(FvwmWindow *fw, Bool FocusByMouse)
{
	MoveFocus(
		FW_W(fw), fw, FocusByMouse, True, False, True);
}

void DeleteFocus(Bool FocusByMouse, Bool do_allow_force_broadcast)
{
	MoveFocus(
		Scr.NoFocusWin, NULL, FocusByMouse, False, False,
		do_allow_force_broadcast);
}

void ForceDeleteFocus(Bool FocusByMouse)
{
	MoveFocus(Scr.NoFocusWin, NULL, FocusByMouse, False, True, True);
}

/* When a window is unmapped (or destroyed) this function takes care of
 * adjusting the focus window appropriately. */
void restore_focus_after_unmap(
	FvwmWindow *fw, Bool do_skip_marked_transients)
{
	extern FvwmWindow *colormap_win;
	FvwmWindow *t = NULL;
	FvwmWindow *set_focus_to = NULL;

	if (fw == get_focus_window())
	{
		if (FW_W_TRANSIENTFOR(fw) != None &&
		    FW_W_TRANSIENTFOR(fw) != Scr.Root)
		{
			for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
			{
				if (FW_W(t) == FW_W_TRANSIENTFOR(fw) &&
				    t->Desk == fw->Desk &&
				    (!do_skip_marked_transients ||
				     !IS_IN_TRANSIENT_SUBTREE(t)))
				{
					set_focus_to = t;
					break;
				}
			}
		}
		if (!set_focus_to &&
		    (HAS_CLICK_FOCUS(fw) || HAS_SLOPPY_FOCUS(fw)))
		{
			for (t = fw->next; t != NULL; t = t->next)
			{
				if (t->Desk == fw->Desk &&
				    !DO_SKIP_CIRCULATE(t) &&
				    !(DO_SKIP_ICON_CIRCULATE(t) &&
				      IS_ICONIFIED(t)) &&
				    (!do_skip_marked_transients ||
				     !IS_IN_TRANSIENT_SUBTREE(t)))
				{
					/* If it is on a different desk we have
					 * to look for another window */
					set_focus_to = t;
					break;
				}
			}
		}
		if (set_focus_to && set_focus_to != fw &&
		    set_focus_to->Desk == fw->Desk)
		{
			/* Don't transfer focus to windows on other desks */
			SetFocusWindow(set_focus_to, True, True);
		}
		if (fw == get_focus_window())
		{
			DeleteFocus(True, True);
		}
	}
	if (fw == Scr.pushed_window)
	{
		Scr.pushed_window = NULL;
	}
	if (fw == colormap_win)
	{
		InstallWindowColormaps(set_focus_to);
	}

	return;
}

/**************************************************************************
 *
 * Moves focus to specified window; only to be called bay Focus and
 * FlipFocus
 *
 *************************************************************************/
void FocusOn(FvwmWindow *t, Bool FocusByMouse, char *action)
{
	int dx,dy;
	int cx,cy;
	Bool do_not_warp;

	if (t == NULL || HAS_NEVER_FOCUS(t))
	{
		UngrabEm(GRAB_NORMAL);
		if (t)
		{
			/* give the window a chance to take the focus itself */
			MoveFocus(FW_W(t), t, FocusByMouse, True, False, True);
		}
		return;
	}

	if (!(do_not_warp = StrEquals(PeekToken(action, NULL), "NoWarp")))
	{
		if (t->Desk != Scr.CurrentDesk)
		{
			goto_desk(t->Desk);
		}

		if (IS_ICONIFIED(t))
		{
			rectangle g;
			Bool rc;

			rc = get_visible_icon_title_geometry(t, &g);
			if (rc == False)
			{
				get_visible_icon_picture_geometry(t, &g);
			}
			cx = g.x + g.width / 2;
			cy = g.y + g.height / 2;
		}
		else
		{
			cx = t->frame_g.x + t->frame_g.width/2;
			cy = t->frame_g.y + t->frame_g.height/2;
		}
		dx = (cx + Scr.Vx)/Scr.MyDisplayWidth*Scr.MyDisplayWidth;
		dy = (cy +Scr.Vy)/Scr.MyDisplayHeight*Scr.MyDisplayHeight;
		MoveViewport(dx,dy,True);

		/* If the window is still not visible, make it visible! */
		if (((t->frame_g.x + t->frame_g.height)< 0)||
		    (t->frame_g.y + t->frame_g.width < 0)||
		    (t->frame_g.x >Scr.MyDisplayWidth)||
		    (t->frame_g.y>Scr.MyDisplayHeight))
		{
			frame_setup_window(
				t, 0, 0, t->frame_g.width, t->frame_g.height,
				False);
			if (HAS_MOUSE_FOCUS(t) || HAS_SLOPPY_FOCUS(t))
			{
				XWarpPointer(
					dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
			}
		}
	}

	UngrabEm(GRAB_NORMAL);
	if (t->Desk == Scr.CurrentDesk)
	{
		FvwmWindow *sf;

		sf = get_focus_window();
		MoveFocus(FW_W(t), t, FocusByMouse, do_not_warp, False, True);
		if (sf != get_focus_window())
		{
			/* Ignore EnterNotify event while we are waiting for
			 * this window to be focused. */
			Scr.focus_in_pending_window = sf;
		}
	}

	return;
}

Bool IsLastFocusSetByMouse(void)
{
	return lastFocusType;
}

void focus_grab_buttons(FvwmWindow *fw, Bool is_focused)
{
	int i;
	Bool do_grab_window = False;
	unsigned char grab_buttons = Scr.buttons2grab;

	if (!fw)
	{
		return;
	}
	do_grab_window = focus_query_grab_buttons(fw, is_focused);
	if (do_grab_window == True)
	{
		grab_buttons = ((1 << NUMBER_OF_MOUSE_BUTTONS) - 1);
	}

#if 0
	/*
	  RBW - If we've come here to grab and all buttons are already grabbed,
	  or to ungrab and none is grabbed, then we've nothing to do.
	*/
	if ((!is_focused && grab_buttons == fw->grabbed_buttons) ||
	    (is_focused && ((fw->grabbed_buttons & grab_buttons) == 0)))
	{
		return;
	}
#else
	if (grab_buttons != fw->grabbed_buttons)
#endif
	{
		Bool do_grab;

		MyXGrabServer(dpy);
		Scr.Ungrabbed = (do_grab_window) ? NULL : fw;
		for (i = 0; i < NUMBER_OF_MOUSE_BUTTONS; i++)
		{
#if 0
			/*  RBW - Set flag for grab or ungrab according to how
			 * we were called. */
			if (!is_focused||1)
			{
				do_grab = !!(grab_buttons & (1 << i));
			}
			else
			{
				do_grab = !(grab_buttons & (1 << i));
			}
#else
			if ((grab_buttons & (1 << i)) == (fw->grabbed_buttons & (1 << i)))
				continue;
			do_grab = !!(grab_buttons & (1 << i));
#endif

			{
				register unsigned int mods;
				register unsigned int max =
					GetUnusedModifiers();
				register unsigned int living_modifiers = ~max;

				/* handle all bindings for the dead modifiers */
				for (mods = 0; mods <= max; mods++)
				{
					/* Since mods starts with 1 we don't
					 * need to test if mods contains a dead
					 * modifier. Otherwise both, dead and
					 * living * modifiers would be zero ==>
					 * mods == 0 */
					if (!(mods & living_modifiers))
					{
						if (do_grab)
						{
							XGrabButton(
								dpy, i + 1,
								mods,
								FW_W_PARENT(fw),
								True,
								ButtonPressMask,
								GrabModeSync,
								GrabModeAsync,
								None, None);
							/*  Set each FvwmWindow
							 * flag accordingly, as
							 * we grab or ungrab. */
							fw->grabbed_buttons |=
								(1<<i);
						}
						else
						{
							XUngrabButton(
								dpy, (i+1),
								mods,
								FW_W_PARENT(fw)
								);
							fw->grabbed_buttons &=
								!(1<<i);
						}
					}
				} /* for */
			}
		} /* for */
		MyXUngrabServer (dpy);
	}

	return;
}

/* same as above, but forces to regrab buttons on the window under the pointer
 * if necessary */
void focus_grab_buttons_on_pointer_window(void)
{
	Window w;
	FvwmWindow *fw;

	if (!XQueryPointer(
		    dpy, Scr.Root, &JunkRoot, &w, &JunkX, &JunkY, &JunkX,
		    &JunkY, &JunkMask))
	{
		/* pointer is not on this screen */
		return;
	}
	if (XFindContext(dpy, w, FvwmContext, (caddr_t *) &fw) == XCNOENT)
	{
		/* pointer is not over a window */
		return;
	}
	focus_grab_buttons(fw, (fw == get_focus_window()));

	return;
}

/* functions to access ScreenFocus, LastScreenFocus and PreviousFocus */
FvwmWindow *get_focus_window(void)
{
	return ScreenFocus;
}

void set_focus_window(FvwmWindow *fw)
{
	ScreenFocus = fw;

	return;
}

FvwmWindow *get_last_screen_focus_window(void)
{
	return LastScreenFocus;
}

void set_last_screen_focus_window(FvwmWindow *fw)
{
	LastScreenFocus = fw;

	return;
}

void update_last_screen_focus_window(FvwmWindow *fw)
{
	if (fw == LastScreenFocus)
	{
		LastScreenFocus = NULL;
	}

	return;
}

void set_focus_model(FvwmWindow *fw)
{
	if (fw->wmhints && (fw->wmhints->flags & InputHint) &&
	    fw->wmhints->input)
	{
		if (WM_TAKES_FOCUS(fw))
		{
			fw->focus_model = FM_LOCALLY_ACTIVE;
		}
		else
		{
			fw->focus_model = FM_PASSIVE;
		}
	}
	else
	{
		if (WM_TAKES_FOCUS(fw))
		{
			fw->focus_model = FM_GLOBALLY_ACTIVE;
		}
		else
		{
			fw->focus_model = FM_NO_INPUT;
		}
	}

	return;
}

/* This function is part of a hack to make focus handling work better with
 * applications that use the passive focus model but manage focus in their own
 * sub windows and should thus use the locally active focus model instead.
 * There are many examples like netscape or ddd. */
void refresh_focus(FvwmWindow *fw)
{
	Bool do_refresh = False;

	if (fw == NULL || fw != get_focus_window() || HAS_NEVER_FOCUS(fw))
	{
		/* only refresh the focus on the currently focused window */
		return;
	}

	/* only refresh the focus for windows with the passive focus model so
	 * that we don't disturb focus handling more than necessary */
	switch (fw->focus_model)
	{
	case FM_PASSIVE:
		do_refresh = True;
		break;
	case FM_NO_INPUT:
	case FM_GLOBALLY_ACTIVE:
	case FM_LOCALLY_ACTIVE:
	default:
		do_refresh = False;
		break;
	}
	if (do_refresh)
	{
		XWindowAttributes winattrs;

		MyXGrabServer(dpy);
		if (XGetWindowAttributes(dpy, FW_W(fw), &winattrs))
		{
			XSelectInput(
				dpy, FW_W(fw),
				winattrs.your_event_mask & ~FocusChangeMask);
			FOCUS_SET(FW_W(fw));
			XSelectInput(dpy, FW_W(fw), winattrs.your_event_mask);
		}
		MyXUngrabServer(dpy);
	}

	return;
}

/* Takes as input the window that wants the focus and the one that currently
 * has the focus and returns if the new window should get it. */
Bool focus_query_grab_focus(FvwmWindow *fw, FvwmWindow *focus_win)
{
	if (fw == NULL)
	{
		return False;
	}
	focus_win = get_focus_window();
	if (focus_win != NULL &&
	    FP_DO_OVERRIDE_GRAB_FOCUS(FW_FOCUS_POLICY(focus_win)))
	{
		/* Don't steal the focus from the current window */
		return False;
	}
	if (IS_TRANSIENT(fw) && !XGetGeometry(
		    dpy, FW_W_TRANSIENTFOR(fw), &JunkRoot, &JunkX, &JunkY,
		    &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth))
	{
		/* Gee, the transientfor does not exist! These evil application
		 * programmers must hate us a lot ;-) */
		FW_W_TRANSIENTFOR(fw) = Scr.Root;
	}
	if (IS_TRANSIENT(fw) && FW_W_TRANSIENTFOR(fw) != Scr.Root)
	{
		if (focus_win != NULL &&
		    FP_DO_GRAB_FOCUS_TRANSIENT(FW_FOCUS_POLICY(fw)) &&
		    FW_W(focus_win) == FW_W_TRANSIENTFOR(fw))
		{
			/* it's a transient and its transientfor currently has
			 * focus. */
			return True;
		}
		else
		{
			return False;
		}
	}
	else
	{
		if (FP_DO_GRAB_FOCUS(FW_FOCUS_POLICY(fw)) &&
		    (focus_win == NULL ||
		     !FP_DO_OVERRIDE_GRAB_FOCUS(FW_FOCUS_POLICY(focus_win))))
		{
			return True;
		}
		else
		{
			return False;
		}
	}

	return False;
}

/* Returns true if the focus has to be restored to a different window after
 * unmapping. */
Bool focus_query_restore_focus(FvwmWindow *fw)
{
	if (fw == NULL || fw != get_focus_window())
	{
		return False;
	}
	if (!IS_TRANSIENT(fw) &&
	    (FW_W_TRANSIENTFOR(fw) == Scr.Root ||
	     FP_DO_GRAB_FOCUS(FW_FOCUS_POLICY(fw))))
	{
		return True;
	}
	else if (IS_TRANSIENT(fw) &&
		 FP_DO_GRAB_FOCUS_TRANSIENT(FW_FOCUS_POLICY(fw)))
	{
		return True;
	}

	return False;
}

/* ---------------------------- builtin commands ---------------------------- */

void CMD_FlipFocus(F_CMD_ARGS)
{
	if (DeferExecution(eventp,&w,&fw,&context,CRS_SELECT,ButtonRelease))
		return;

	/* Reorder the window list */
	FocusOn(fw, TRUE, action);
}

void CMD_Focus(F_CMD_ARGS)
{
	if (DeferExecution(eventp,&w,&fw,&context,CRS_SELECT,ButtonRelease))
		return;

	FocusOn(fw, FALSE, action);
}

void CMD_WarpToWindow(F_CMD_ARGS)
{
	int val1_unit, val2_unit, n;
	int val1, val2;

	n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
	if (context != C_UNMANAGED)
	{
		if (DeferExecution(
			    eventp, &w, &fw, &context, CRS_SELECT,
			    ButtonRelease))
		{
			return;
		}
		if (n == 2)
		{
			warp_to_fvwm_window(
				eventp, fw, val1, val1_unit, val2, val2_unit);
		}
		else
		{
			warp_to_fvwm_window(eventp, fw, 0, 0, 0, 0);
		}
	}
	else
	{
		int x = 0;
		int y = 0;

		if (n == 2)
		{
			int wx;
			int wy;
			unsigned int ww;
			unsigned int wh;

			if (!XGetGeometry(
				    dpy, w, &JunkRoot, &wx, &wy, &ww, &wh,
				    &JunkBW, &JunkDepth))
			{
				return;
			}
			if (val1_unit != Scr.MyDisplayWidth)
			{
				x = val1;
			}
			else
			{
				x = (ww - 1) * val1 / 100;
			}
			if (val2_unit != Scr.MyDisplayHeight)
			{
				y = val2;
			}
			else
			{
				y = (wh - 1) * val2 / 100;
			}
			if (x < 0)
			{
				x += ww;
			}
			if (y < 0)
			{
				y += wh;
			}
		}
		XWarpPointer(dpy, None, w, 0, 0, 0, 0, x, y);
	}

	return;
}
