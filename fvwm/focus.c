/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "libs/ClientMsg.h"
#include "libs/Grab.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "eventhandler.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "focus.h"
#include "borders.h"
#include "frame.h"
#include "virtual.h"
#include "stack.h"
#include "geometry.h"
#include "colormaps.h"
#include "add_window.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef struct
{
	unsigned do_allow_force_broadcast : 1;
	unsigned do_forbid_warp : 1;
	unsigned do_force : 1;
	unsigned is_focus_by_flip_focus_cmd : 1;
	unsigned client_entered : 1;
	fpol_set_focus_by_t set_by;
} sftfwin_args_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static Bool lastFocusType;
/* Last window which Fvwm gave the focus to NOT the window that really has the
 * focus */
static FvwmWindow *ScreenFocus = NULL;
/* Window which had focus before the pointer moved to a different screen. */
static FvwmWindow *LastScreenFocus = NULL;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void _focus_set(Window w, FvwmWindow *fw)
{
	Scr.focus_in_requested_window = fw;
	XSetInputFocus(dpy, w, RevertToParent, CurrentTime);

	return;
}

void _focus_reset(void)
{
	Scr.focus_in_requested_window = NULL;
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);

	return;
}

/* ---------------------------- local functions ---------------------------- */

static Bool focus_get_fpol_context_flag(
	fpol_context_t *fpol_context, int context)
{
	int flag;

	switch (context)
	{
	case C_WINDOW:
	case C_EWMH_DESKTOP:
		flag = fpol_context->client;
		break;
	case C_ICON:
		flag = fpol_context->icon;
		break;
	default:
		flag = fpol_context->decor;
		break;
	}

	return (flag) ? True : False;
}

/*
 * Helper functions for setting the focus
 */
static void __try_program_focus(Window w, const FvwmWindow *fw)
{
	if (fw && WM_TAKES_FOCUS(fw) &&
	    FP_DO_FOCUS_BY_PROGRAM(FW_FOCUS_POLICY(fw)))
	{
		send_clientmessage(dpy, w, _XA_WM_TAKE_FOCUS, fev_get_evtime());
	}

	return;
}

static Bool __try_forbid_user_focus(
	Window w, FvwmWindow *fw)
{
	if (fw == NULL ||
	    fpol_query_allow_user_focus(&FW_FOCUS_POLICY(fw)) == True)
	{
		return False;
	}
	if (WM_TAKES_FOCUS(fw))
	{
		/* give it a chance to take the focus itself */
		__try_program_focus(w, fw);
		XFlush(dpy);
	}
	else
	{
		/* make sure the window is not hilighted */
		border_draw_decorations(
			fw, PART_ALL, False, False, CLEAR_ALL, NULL, NULL);
	}

	return True;
}

static Bool __check_allow_focus(
	Window w, FvwmWindow *fw, fpol_set_focus_by_t set_by)
{
	FvwmWindow *sf;

	if (fw == NULL || set_by == FOCUS_SET_FORCE)
	{
		/* always allow to delete focus */
		return True;
	}
	sf = get_focus_window();
	if (!FP_IS_LENIENT(FW_FOCUS_POLICY(fw)) &&
	    !focus_does_accept_input_focus(fw) &&
	    sf != NULL && sf->Desk == Scr.CurrentDesk)
	{
		/* window does not want focus */
		return False;
	}
	if (fpol_query_allow_set_focus(&FW_FOCUS_POLICY(fw), set_by))
	{
		return True;
	}

	return False;
}

static void __update_windowlist(
	FvwmWindow *fw, fpol_set_focus_by_t set_by,
	int is_focus_by_flip_focus_cmd)
{
	lastFocusType = (is_focus_by_flip_focus_cmd) ? True : False;
	if (fw == NULL || focus_is_focused(fw) || fw == &Scr.FvwmRoot ||
	    IS_SCHEDULED_FOR_DESTROY(fw))
	{
		return;
	}
	/* Watch out: fw may not be on the windowlist and the windowlist may be
	 * empty */
	if (!is_focus_by_flip_focus_cmd &&
	    (FP_DO_SORT_WINDOWLIST_BY(FW_FOCUS_POLICY(fw)) ==
	     FPOL_SORT_WL_BY_OPEN ||
	     set_by == FOCUS_SET_BY_FUNCTION))
	{
		/* move the windowlist around so that fw is at the top */
		FvwmWindow *fw2;

		/* find the window on the windowlist */
		for (fw2 = &Scr.FvwmRoot; fw2 && fw2 != fw; fw2 = fw2->next)
		{
			/* nothing */
		}
		if (fw2)
		{
			/* the window is on the (non-zero length) windowlist */
			/* make fw2 point to the last window on the list */
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
	else
	{
		/* pluck window from list and deposit at top */
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

	return;
}

static Bool __try_other_screen_focus(const FvwmWindow *fw)
{
	if (fw == NULL && !Scr.flags.is_pointer_on_this_screen)
	{
		FvwmWindow *sf;

		sf = get_focus_window();
		set_focus_window(NULL);
		if (sf != NULL)
		{
			focus_grab_buttons(sf);
		}
		/* DV (25-Nov-2000): Don't give the Scr.NoFocusWin the focus
		 * here. This would steal the focus from the other screen's
		 * root window again. */
		return True;
	}

	return False;
}

/*
 * Sets the input focus to the indicated window.
 */
static void __set_focus_to_fwin(
	Window w, FvwmWindow *fw, sftfwin_args_t *args)
{
	FvwmWindow *sf;

	if (__try_forbid_user_focus(w, fw) == True)
	{
		return;
	}
	__try_program_focus(w, fw);
	if (__check_allow_focus(w, fw, args->set_by) == False)
	{
		return;
	}
	__update_windowlist(fw, args->set_by, args->is_focus_by_flip_focus_cmd);
	if (__try_other_screen_focus(fw) == True)
	{
		return;
	}

	if (fw && !args->do_forbid_warp)
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
		else if (!IsRectangleOnThisPage(&(fw->g.frame), fw->Desk))
		{
			fw = NULL;
			w = Scr.NoFocusWin;
		}
	}

	sf = get_focus_window();
	if (fw == NULL)
	{
		FOCUS_SET(Scr.NoFocusWin, NULL);
		set_focus_window(NULL);
		Scr.UnknownWinFocused = None;
		XFlush(dpy);
		return;
	}
	/* RBW - allow focus to go to a NoIconTitle icon window so
	 * auto-raise will work on it. */
	if (IS_ICONIFIED(fw))
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

	if (FP_IS_LENIENT(FW_FOCUS_POLICY(fw)))
	{
		FOCUS_SET(w, fw);
		set_focus_window(fw);
		if (args->do_allow_force_broadcast)
		{
			SET_FOCUS_CHANGE_BROADCAST_PENDING(fw, 1);
		}
		Scr.UnknownWinFocused = None;
	}
	else if (focus_does_accept_input_focus(fw))
	{
		/* Window will accept input focus */
		if (Scr.StolenFocusWin == w && Scr.UnknownWinFocused != None)
		{
			/* Without this FocusIn is not generated on the
			 * window if it was focuesed when the unmanaged
			 * window took focus. */
			FOCUS_SET(Scr.NoFocusWin, NULL);

		}
		FOCUS_SET(w, fw);
		set_focus_window(fw);
		if (fw)
		{
			if (args->do_allow_force_broadcast)
			{
				SET_FOCUS_CHANGE_BROADCAST_PENDING(fw, 1);
			}
		}
		Scr.UnknownWinFocused = None;
	}
	else if (sf && sf->Desk == Scr.CurrentDesk)
	{
		/* Window doesn't want focus. Leave focus alone */
	}
	else
	{
		FOCUS_SET(Scr.NoFocusWin, NULL);
		set_focus_window(NULL);
	}
	XFlush(dpy);

	return;
}

static void set_focus_to_fwin(
	Window w, FvwmWindow *fw, sftfwin_args_t *args)
{
	FvwmWindow *sf;

	sf = get_focus_window();
	if (!args->do_force && fw == sf)
	{
		focus_grab_buttons(sf);
		return;
	}
	__set_focus_to_fwin(w, fw, args);
	/* Make sure the button grabs on the new and the old focused windows
	 * are up to date. */
        if (args->client_entered)
	{
		focus_grab_buttons_client_entered(fw);
	}
	else
	{
		focus_grab_buttons(fw);
	}
        /* RBW -- don't call this twice for the same window!  */
        if (fw != get_focus_window())
        {
                if (args->client_entered)
                {
                        focus_grab_buttons_client_entered(get_focus_window());
                }
                else
                {
                        focus_grab_buttons(get_focus_window());
                }
        }

	return;
}

/*
 *
 * Moves pointer to specified window
 *
 */
static void warp_to_fvwm_window(
	const exec_context_t *exc, int warp_x, int x_unit, int warp_y,
	int y_unit, int do_raise)
{
	int dx,dy;
	int cx,cy;
	int x,y;
	FvwmWindow *t = exc->w.fw;

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
		cx = t->g.frame.x + t->g.frame.width/2;
		cy = t->g.frame.y + t->g.frame.height/2;
	}
	dx = (cx + Scr.Vx) / Scr.MyDisplayWidth * Scr.MyDisplayWidth;
	dy = (cy + Scr.Vy) / Scr.MyDisplayHeight * Scr.MyDisplayHeight;
	if (dx != Scr.Vx || dy != Scr.Vy)
	{
		MoveViewport(dx, dy, True);
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
		x = g.x + g.width / 2;
		y = g.y + g.height / 2;
	}
	else
	{
		if (x_unit != Scr.MyDisplayWidth && warp_x >= 0)
		{
			x = t->g.frame.x + warp_x;
		}
		else if (x_unit != Scr.MyDisplayWidth)
		{
			x = t->g.frame.x + t->g.frame.width + warp_x;
		}
		else if (warp_x >= 0)
		{
			x = t->g.frame.x +
				(t->g.frame.width - 1) * warp_x / 100;
		}
		else
		{
			x = t->g.frame.x +
				(t->g.frame.width - 1) * (100 + warp_x) / 100;
		}

		if (y_unit != Scr.MyDisplayHeight && warp_y >= 0)
		{
			y = t->g.frame.y + warp_y;
		}
		else if (y_unit != Scr.MyDisplayHeight)
		{
			y = t->g.frame.y + t->g.frame.height + warp_y;
		}
		else if (warp_y >= 0)
		{
			y = t->g.frame.y +
				(t->g.frame.height - 1) * warp_y / 100;
		}
		else
		{
			y = t->g.frame.y +
				(t->g.frame.height - 1) * (100 + warp_y) / 100;
		}
	}
	FWarpPointerUpdateEvpos(
		exc->x.elast, dpy, None, Scr.Root, 0, 0, 0, 0, x, y);
	if (do_raise)
	{
		RaiseWindow(t, False);
	}
	/* If the window is still not visible, make it visible! */
	if (t->g.frame.x + t->g.frame.width  < 0 ||
	    t->g.frame.y + t->g.frame.height < 0 ||
	    t->g.frame.x >= Scr.MyDisplayWidth ||
	    t->g.frame.y >= Scr.MyDisplayHeight)
	{
		frame_setup_window(
			t, 0, 0, t->g.frame.width, t->g.frame.height, False);
		FWarpPointerUpdateEvpos(
			exc->x.elast, dpy, None, Scr.Root, 0, 0, 0, 0, 2, 2);
	}

	return;
}

static Bool focus_query_grab_buttons(FvwmWindow *fw, Bool client_entered)
{
	Bool flag;
	Bool is_focused;

	if (fw->Desk != Scr.CurrentDesk || IS_ICONIFIED(fw))
	{
		return False;
	}
	is_focused = focus_is_focused(fw);
	if (!is_focused && FP_DO_FOCUS_CLICK_CLIENT(FW_FOCUS_POLICY(fw)))
	{
		return True;
	}
	if (is_on_top_of_layer_and_above_unmanaged(fw))
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

static FvwmWindow *__restore_focus_after_unmap(
	const FvwmWindow *fw, Bool do_skip_marked_transients)
{
	FvwmWindow *t = NULL;
	FvwmWindow *set_focus_to = NULL;

	t = get_transientfor_fvwmwindow(fw);
	if (t != NULL &&
	    FP_DO_RELEASE_FOCUS_TRANSIENT(FW_FOCUS_POLICY(fw)) &&
	    !FP_DO_OVERRIDE_RELEASE_FOCUS(FW_FOCUS_POLICY(t)) &&
	    t->Desk == fw->Desk &&
	    (!do_skip_marked_transients || !IS_IN_TRANSIENT_SUBTREE(t)))
	{
		set_focus_to = t;
	}
	else if (t == NULL && FP_DO_RELEASE_FOCUS(FW_FOCUS_POLICY(fw)))
	{
		for (t = fw->next; t != NULL && set_focus_to == NULL;
		     t = t->next)
		{
			if (!FP_DO_OVERRIDE_RELEASE_FOCUS(
				    FW_FOCUS_POLICY(t)) &&
			    t->Desk == fw->Desk && !DO_SKIP_CIRCULATE(t) &&
			    !(IS_ICONIFIED(t) && (DO_SKIP_ICON_CIRCULATE(t) ||
			      IS_ICON_SUPPRESSED(t))) &&
			    (!do_skip_marked_transients ||
			     !IS_IN_TRANSIENT_SUBTREE(t)))
			{
				/* If it is on a different desk we have to look
				 * for another window */
				set_focus_to = t;
			}
		}
	}
	if (set_focus_to && set_focus_to != fw &&
	    set_focus_to->Desk == fw->Desk)
	{
		/* Don't transfer focus to windows on other desks */
		SetFocusWindow(set_focus_to, True, FOCUS_SET_FORCE);
	}
	if (focus_is_focused(fw))
	{
		DeleteFocus(True);
	}

	return set_focus_to;
}

/*
 *
 * Moves focus to specified window; only to be called bay Focus and FlipFocus
 *
 */
static void __activate_window_by_command(
	F_CMD_ARGS, int is_focus_by_flip_focus_cmd)
{
	int cx;
	int cy;
	Bool do_not_warp;
	sftfwin_args_t sf_args;
	FvwmWindow * const fw = exc->w.fw;

	memset(&sf_args, 0, sizeof(sf_args));
	sf_args.do_allow_force_broadcast = 1;
	sf_args.is_focus_by_flip_focus_cmd = is_focus_by_flip_focus_cmd;
	sf_args.set_by = FOCUS_SET_BY_FUNCTION;
        sf_args.client_entered = 0;
	if (fw == NULL || !FP_DO_FOCUS_BY_FUNCTION(FW_FOCUS_POLICY(fw)))
	{
		UngrabEm(GRAB_NORMAL);
		if (fw)
		{
			/* give the window a chance to take the focus itself */
			sf_args.do_forbid_warp = 1;
			sf_args.do_force = 0;
			set_focus_to_fwin(FW_W(fw), fw, &sf_args);
		}
		return;
	}

	do_not_warp = StrEquals(PeekToken(action, NULL), "NoWarp");
	if (!do_not_warp)
	{
		if (fw->Desk != Scr.CurrentDesk)
		{
			goto_desk(fw->Desk);
		}
		if (IS_ICONIFIED(fw))
		{
			rectangle g;
			Bool rc;

			rc = get_visible_icon_title_geometry(fw, &g);
			if (rc == False)
			{
				get_visible_icon_picture_geometry(fw, &g);
			}
			cx = g.x + g.width / 2;
			cy = g.y + g.height / 2;
		}
		else
		{
			cx = fw->g.frame.x + fw->g.frame.width/2;
			cy = fw->g.frame.y + fw->g.frame.height/2;
		}
		if (
			cx < 0 || cx >= Scr.MyDisplayWidth ||
			cy < 0 || cy >= Scr.MyDisplayHeight)
		{
			int dx;
			int dy;

			dx = ((cx + Scr.Vx) / Scr.MyDisplayWidth) *
				Scr.MyDisplayWidth;
			dy = ((cy + Scr.Vy) / Scr.MyDisplayHeight) *
				Scr.MyDisplayHeight;
			MoveViewport(dx, dy, True);
		}
#if 0 /* can not happen */
		/* If the window is still not visible, make it visible! */
		if (fw->g.frame.x + fw->g.frame.width < 0 ||
		    fw->g.frame.y + fw->g.frame.height < 0 ||
		    fw->g.frame.x >= Scr.MyDisplayWidth ||
		    fw->g.frame.y >= Scr.MyDisplayHeight)
		{
			frame_setup_window(
				fw, 0, 0, fw->g.frame.width,
				fw->g.frame.height, False);
			if (
				FP_DO_WARP_POINTER_ON_FOCUS_FUNC(
					FW_FOCUS_POLICY(fw)))
			{
				FWarpPointerUpdateEvpos(
					exc->x.elast, dpy, None, Scr.Root, 0,
					0, 0, 0, 2, 2);
			}
		}
#endif
	}
	UngrabEm(GRAB_NORMAL);

	if (fw->Desk == Scr.CurrentDesk)
	{
		FvwmWindow *sf;

		sf = get_focus_window();
		sf_args.do_forbid_warp = !!do_not_warp;
		sf_args.do_force = 0;
                sf_args.client_entered = 0;
		set_focus_to_fwin(FW_W(fw), fw, &sf_args);
		if (sf != get_focus_window())
		{
			/* Ignore EnterNotify event while we are waiting for
			 * this window to be focused. */
			Scr.focus_in_pending_window = sf;
		}
	}

	return;
}

static void __focus_grab_one_button(
	FvwmWindow *fw, int button, int grab_buttons)
{
	Bool do_grab;

	do_grab = (grab_buttons & (1 << button));
	if ((do_grab & (1 << button)) == (fw->grabbed_buttons & (1 << button)))
	{
		return;
	}
	if (do_grab)
	{
		XGrabButton(
			dpy, button + 1, AnyModifier, FW_W_PARENT(fw), True,
			ButtonPressMask, GrabModeSync, GrabModeAsync, None,
			None);
		/* Set window flags accordingly as we grab or ungrab. */
		fw->grabbed_buttons |= (1 << button);
	}
	else
	{
		XUngrabButton(dpy, button + 1, AnyModifier, FW_W_PARENT(fw));
		fw->grabbed_buttons &= ~(1 << button);
	}

	return;
}

/* ---------------------------- interface functions ------------------------ */

Bool focus_does_accept_input_focus(const FvwmWindow *fw)
{
	return (!fw || !fw->wmhints ||
		!(fw->wmhints->flags & InputHint) || fw->wmhints->input);
}

Bool focus_is_focused(const FvwmWindow *fw)
{
	return (fw && fw == ScreenFocus);
}

Bool focus_query_click_to_raise(
	FvwmWindow *fw, Bool is_focused, int context)
{
	fpol_context_t *c;

	if (is_focused)
	{
		c = &FP_DO_RAISE_FOCUSED_CLICK(FW_FOCUS_POLICY(fw));
	}
	else
	{
		c = &FP_DO_RAISE_UNFOCUSED_CLICK(FW_FOCUS_POLICY(fw));
	}

	return focus_get_fpol_context_flag(c, context);
}

Bool focus_query_click_to_focus(
	FvwmWindow *fw, int context)
{
	fpol_context_t *c;

	c = &FP_DO_FOCUS_CLICK(FW_FOCUS_POLICY(fw));

	return focus_get_fpol_context_flag(c, context);
}

/* Takes as input the window that wants the focus and the one that currently
 * has the focus and returns if the new window should get it. */
Bool focus_query_open_grab_focus(FvwmWindow *fw, FvwmWindow *focus_win)
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
	validate_transientfor(fw);
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
	}
	else
	{
		if (FP_DO_GRAB_FOCUS(FW_FOCUS_POLICY(fw)) &&
		    (focus_win == NULL ||
		     !FP_DO_OVERRIDE_GRAB_FOCUS(FW_FOCUS_POLICY(focus_win))))
		{
			return True;
		}
	}

	return False;
}

/* Returns true if the focus has to be restored to a different window after
 * unmapping. */
Bool focus_query_close_release_focus(const FvwmWindow *fw)
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

static void __focus_grab_buttons(FvwmWindow *fw, Bool client_entered)
{
	int i;
	Bool do_grab_window = False;
	int grab_buttons;

	if (fw == NULL || IS_SCHEDULED_FOR_DESTROY(fw) || !IS_MAPPED(fw))
	{
		/* It is pointless to grab buttons on dieing windows.  Buttons
		 * can not be grabbed when the window is unmapped. */
		return;
	}
	grab_buttons = Scr.buttons2grab;
	do_grab_window = focus_query_grab_buttons(fw, client_entered);
	if (do_grab_window == True)
	{
		grab_buttons |= FP_USE_MOUSE_BUTTONS(FW_FOCUS_POLICY(fw));
	}
	if (grab_buttons != fw->grabbed_buttons)
	{
		MyXGrabServer(dpy);
		for (i = 0; i < NUMBER_OF_EXTENDED_MOUSE_BUTTONS; i++)
		{
			__focus_grab_one_button(fw, i, grab_buttons);
		}
		MyXUngrabServer (dpy);
	}

	return;
}

void focus_grab_buttons(FvwmWindow *fw)
{
        __focus_grab_buttons(fw, False);
}

void focus_grab_buttons_client_entered(FvwmWindow *fw)
{
        __focus_grab_buttons(fw, True);
}

void focus_grab_buttons_on_layer(int layer)
{
	FvwmWindow *fw;

	for (
		fw = Scr.FvwmRoot.stack_next;
		fw != &Scr.FvwmRoot && fw->layer >= layer;
		fw = fw->stack_next)
	{
		if (fw->layer == layer)
		{
			focus_grab_buttons(fw);
		}
	}

	return;
}

void focus_grab_buttons_all(void)
{
	FvwmWindow *fw;

	for (fw = Scr.FvwmRoot.next; fw != NULL; fw = fw->next)
	{
		focus_grab_buttons(fw);
	}

	return;
}

void _SetFocusWindow(
	FvwmWindow *fw, Bool do_allow_force_broadcast,
	fpol_set_focus_by_t set_by, Bool client_entered)
{
	sftfwin_args_t sf_args;

	memset(&sf_args, 0, sizeof(sf_args));
	sf_args.do_allow_force_broadcast = !!do_allow_force_broadcast;
	sf_args.is_focus_by_flip_focus_cmd = 0;
	sf_args.do_forbid_warp = 0;
	sf_args.do_force = 1;
	sf_args.set_by = set_by;
        if (client_entered)
	{
		sf_args.client_entered = 1;
	}
	else
	{
		sf_args.client_entered = 0;
	}
	set_focus_to_fwin(FW_W(fw), fw, &sf_args);

	return;
}

void _ReturnFocusWindow(FvwmWindow *fw)
{
	sftfwin_args_t sf_args;

	memset(&sf_args, 0, sizeof(sf_args));
	sf_args.do_allow_force_broadcast = 1;
	sf_args.is_focus_by_flip_focus_cmd = 0;
	sf_args.do_forbid_warp = 1;
        sf_args.client_entered = 0;
	sf_args.do_force = 0;
	sf_args.set_by = FOCUS_SET_FORCE;
	set_focus_to_fwin(FW_W(fw), fw, &sf_args);

	return;
}

void _DeleteFocus(Bool do_allow_force_broadcast)
{
	sftfwin_args_t sf_args;

	memset(&sf_args, 0, sizeof(sf_args));
	sf_args.do_allow_force_broadcast = !!do_allow_force_broadcast;
	sf_args.is_focus_by_flip_focus_cmd = 0;
	sf_args.do_forbid_warp = 0;
        sf_args.client_entered = 0;
	sf_args.do_force = 0;
	sf_args.set_by = FOCUS_SET_FORCE;
	set_focus_to_fwin(Scr.NoFocusWin, NULL, &sf_args);

	return;
}

void _ForceDeleteFocus(void)
{
	sftfwin_args_t sf_args;

	memset(&sf_args, 0, sizeof(sf_args));
	sf_args.do_allow_force_broadcast = 1;
	sf_args.is_focus_by_flip_focus_cmd = 0;
	sf_args.do_forbid_warp = 0;
        sf_args.client_entered = 0;
	sf_args.do_force = 1;
	sf_args.set_by = FOCUS_SET_FORCE;
	set_focus_to_fwin(Scr.NoFocusWin, NULL, &sf_args);

	return;
}

/* When a window is unmapped (or destroyed) this function takes care of
 * adjusting the focus window appropriately. */
void restore_focus_after_unmap(
	const FvwmWindow *fw, Bool do_skip_marked_transients)
{
	extern FvwmWindow *colormap_win;
	FvwmWindow *set_focus_to = NULL;

	if (focus_is_focused(fw))
	{
		set_focus_to = __restore_focus_after_unmap(
			fw, do_skip_marked_transients);
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

Bool IsLastFocusSetByMouse(void)
{
	return lastFocusType;
}


/* same as above, but forces to regrab buttons on the window under the pointer
 * if necessary */
void focus_grab_buttons_on_pointer_window(void)
{
	Window w;
	FvwmWindow *fw;

	if (!FQueryPointer(
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
	focus_grab_buttons(fw);

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
	if (!focus_does_accept_input_focus(fw))
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
	else
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

	return;
}

/* This function is part of a hack to make focus handling work better with
 * applications that use the passive focus model but manage focus in their own
 * sub windows and should thus use the locally active focus model instead.
 * There are many examples like netscape or ddd. */
void focus_force_refresh_focus(const FvwmWindow *fw)
{
	XWindowAttributes winattrs;

	MyXGrabServer(dpy);
	if (XGetWindowAttributes(dpy, FW_W(fw), &winattrs))
	{
		XSelectInput(
			dpy, FW_W(fw),
			winattrs.your_event_mask & ~FocusChangeMask);
		FOCUS_SET(FW_W(fw), NULL /* we don't expect an event */);
		XSelectInput(dpy, FW_W(fw), winattrs.your_event_mask);
	}
	MyXUngrabServer(dpy);

	return;
}

void refresh_focus(const FvwmWindow *fw)
{
	Bool do_refresh = False;

	if (fw == NULL || !focus_is_focused(fw))
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
		focus_force_refresh_focus(fw);
	}

	return;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_FlipFocus(F_CMD_ARGS)
{
	/* Reorder the window list */
	__activate_window_by_command(F_PASS_ARGS, 1);

	return;
}

void CMD_Focus(F_CMD_ARGS)
{
	__activate_window_by_command(F_PASS_ARGS, 0);

	return;
}

void CMD_WarpToWindow(F_CMD_ARGS)
{
	int val1_unit, val2_unit, n;
	int val1, val2;
	int do_raise;
	char *next;
	char *token;

	next = GetNextToken(action, &token);
	if (StrEquals(token, "!raise"))
	{
		do_raise = 0;
		action = next;
	}
	else if (StrEquals(token, "raise"))
	{
		do_raise = 1;
		action = next;
	}
	else
	{
		do_raise = 1;
	}
	n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
	if (exc->w.wcontext != C_UNMANAGED)
	{
		if (n == 2)
		{
			warp_to_fvwm_window(
				exc, val1, val1_unit, val2, val2_unit,
				do_raise);
		}
		else
		{
			warp_to_fvwm_window(exc, 0, 0, 0, 0, do_raise);
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
			int ww;
			int wh;

			if (!XGetGeometry(
				    dpy, exc->w.w, &JunkRoot, &wx, &wy,
				    (unsigned int*)&ww, (unsigned int*)&wh,
				    (unsigned int*)&JunkBW,
				    (unsigned int*)&JunkDepth))
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
		FWarpPointerUpdateEvpos(
			exc->x.elast, dpy, None, exc->w.w, 0, 0, 0, 0, x, y);
	}

	return;
}
