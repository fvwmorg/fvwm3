/* -*-c-*- */
/* Copyright (C) 2001  Olivier Chapuis */
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

#include "config.h"

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/Strings.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "functions.h"
#include "misc.h"
#include "screen.h"
#include "virtual.h"
#include "commands.h"
#include "update.h"
#include "style.h"
#include "stack.h"
#include "events.h"
#include "ewmh.h"
#include "ewmh_intern.h"
#include "decorations.h"
#include "geometry.h"
#include "borders.h"

extern ewmh_atom ewmh_atom_wm_state[];

#define DEBUG_EWMH_INIT_STATE 0
/*
 * root
 */
int ewmh_CurrentDesktop(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (ev->xclient.data.l[0] < 0 || ev->xclient.data.l[0] > 0x7fffffff)
	{
		fvwm_msg(
			WARN, "ewmh_CurrentDesktop",
			"The application window (id %#lx)\n"
			"  \"%s\" tried to switch to an invalid desktop (%ld)\n"
			"  using an EWMH client message.\n"
			"    fvwm is ignoring this request.\n",
			fw ? FW_W(fw) : 0, fw ? fw->name.name : "(none)",
			ev->xclient.data.l[0]);
		fvwm_msg_report_app_and_workers();

		return -1;
	}
	goto_desk(ev->xclient.data.l[0]);

	return -1;
}

int ewmh_DesktopGeometry(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	char action[256];
	long width = ev->xclient.data.l[0];
	long height = ev->xclient.data.l[1];

	width = width / Scr.MyDisplayWidth;
	height = height / Scr.MyDisplayHeight;

	if (width <= 0 || height <= 0)
	{
		fvwm_msg(
			WARN, "ewmh_DesktopGeometry",
			"The application window (id %#lx)\n"
			"  \"%s\" tried to set an invalid desktop geometry"
			" (%ldx%ld)\n"
			"  using an EWMH client message.\n"
			"    fvwm is ignoring this request.\n",
			fw ? FW_W(fw) : 0, fw ? fw->name.name : "(none)",
			ev->xclient.data.l[0], ev->xclient.data.l[1]);
		fvwm_msg_report_app_and_workers();

		return -1;
	}
	sprintf(action, "DesktopSize %ld %ld", width, height);
	execute_function_override_window(NULL, NULL, action, 0, NULL);

	return -1;
}

int ewmh_DesktopViewPort(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (
		ev->xclient.data.l[0] < 0 ||
		ev->xclient.data.l[0] > 0x7fffffff ||
		ev->xclient.data.l[1] < 0 ||
		ev->xclient.data.l[1] > 0x7fffffff)
	{
		fvwm_msg(
			WARN, "ewmh_DesktopViewPort",
			"The application window (id %#lx)\n"
			"  \"%s\" tried to switch to an invalid page"
			" (%ldx%ld)\n"
			"  using an EWMH client message.\n"
			"    fvwm is ignoring this request.\n",
			fw ? FW_W(fw) : 0, fw ? fw->name.name : "(none)",
			ev->xclient.data.l[0], ev->xclient.data.l[1]);
		fvwm_msg_report_app_and_workers();

		return -1;
	}
	MoveViewport(ev->xclient.data.l[0], ev->xclient.data.l[1], 1);
	return -1;
}

int ewmh_NumberOfDesktops(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	int d = ev->xclient.data.l[0];

	/* not a lot of sinification for fvwm */
	if (d > 0 && (d <= ewmhc.MaxDesktops || ewmhc.MaxDesktops == 0))
	{
		ewmhc.NumberOfDesktops = d;
		EWMH_SetNumberOfDesktops();
	}
	else
	{
		fvwm_msg(
			WARN, "ewmh_NumberOfDesktops",
			"The application window (id %#lx)\n"
			"  \"%s\" tried to set an invalid number of desktops"
			" (%ld)\n"
			"  using an EWMH client message.\n"
			"    fvwm is ignoring this request.\n",
			fw ? FW_W(fw) : 0, fw ? fw->name.name : "(none)",
			ev->xclient.data.l[0]);
		fvwm_msg_report_app_and_workers();
	}

	return -1;
}

/*
 * window
 */

int ewmh_ActiveWindow(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (ev == NULL)
	{
		return 0;
	}
	execute_function_override_window(
		NULL, NULL, "EWMHActivateWindowFunc", 0, fw);

	return 0;
}

int ewmh_CloseWindow(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (ev == NULL)
	{
		return 0;
	}
	if (!is_function_allowed(F_CLOSE, NULL, fw, RQORIG_PROGRAM_US, False))
	{
		return 0;
	}
	execute_function_override_window(NULL, NULL, "Close", 0, fw);

	return 0;
}

int ewmh_MoveResizeWindow(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	int do_reconfigure;
	int win_gravity;
	int value_mask;

	if (ev == NULL)
	{
		return 0;
	}
	win_gravity = ev->xclient.data.l[0] & 0xff;
	value_mask = (ev->xclient.data.l[0] >> 8) & 0xf;
	if (fw == NULL)
	{
		/* unmanaged window */
		do_reconfigure = 1;
	}
	else
	{
		int func;

		if (
			((value_mask & CWWidth) == 0 ||
			 ev->xclient.data.l[3] == fw->g.normal.width) &&
			((value_mask & CWHeight) == 0 ||
			 ev->xclient.data.l[4] == fw->g.normal.height))
		{
			func = F_MOVE;
		}
		else
		{
			func = F_RESIZE;
		}
		do_reconfigure = !!is_function_allowed(
			func, NULL, fw, RQORIG_PROGRAM, False);
	}
	if (do_reconfigure == 1)
	{
		XEvent e;
		XConfigureRequestEvent *cre = &e.xconfigurerequest;

		cre->value_mask = value_mask;
		cre->x = ev->xclient.data.l[1];
		cre->y = ev->xclient.data.l[2];
		cre->width = ev->xclient.data.l[3];
		cre->height = ev->xclient.data.l[4];
		cre->window = ev->xclient.window;
		events_handle_configure_request(&e, fw, True, win_gravity);
	}

	return 0;
}

int ewmh_RestackWindow(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	int do_restack;

	if (ev == NULL)
	{
		return 0;
	}
	if (fw == NULL)
	{
		/* unmanaged window */
		do_restack = 1;
	}
	else
	{
		do_restack = !!DO_EWMH_USE_STACKING_HINTS(fw);
	}
	if (do_restack == 1)
	{
		XEvent e;
		XConfigureRequestEvent *cre = &e.xconfigurerequest;

		cre->value_mask = CWSibling | CWStackMode;
		cre->above = ev->xclient.data.l[1];
		cre->detail = ev->xclient.data.l[2];
		cre->window = ev->xclient.window;
		events_handle_configure_request(&e, fw, True, ForgetGravity);
	}

	return 0;
}

int ewmh_WMDesktop(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (ev != NULL && style == NULL)
	{
		/* client message */
		unsigned long d = (unsigned long)ev->xclient.data.l[0];

		/* the spec says that if d = 0xFFFFFFFF then we have to Stick
		 * the window however KDE use 0xFFFFFFFE :o) */
		if (d == (unsigned long)-2 || d == (unsigned long)-1)
		{
			execute_function_override_window(
				NULL, NULL, "Stick on", 0, fw);
		}
		else if (d > 0)
		{
			if (IS_STICKY_ACROSS_PAGES(fw) ||
			    IS_STICKY_ACROSS_DESKS(fw))
			{
				execute_function_override_window(
					NULL, NULL, "Stick off", 0, fw);
			}
			if (fw->Desk != d)
			{
				do_move_window_to_desk(fw, (int)d);
			}
		}
		else
		{
			fvwm_msg(
				WARN, "ewmh_WMDesktop",
				"The application window (id %#lx)\n"
				"  \"%s\" tried to move to an invalid desk"
				" (%ld)\n"
				"  using an EWMH client message.\n"
				"    fvwm is ignoring this request.\n",
				fw ? FW_W(fw) : 0,
				fw ? fw->name.name : "(none)",
				ev->xclient.data.l[0]);
			fvwm_msg_report_app_and_workers();
		}

		return 0;
	}

	if (style != NULL && ev == NULL)
	{
		/* start on desk */
		CARD32 *val;
		int size = 0;

		if (DO_EWMH_IGNORE_STATE_HINTS(style))
		{
			SET_HAS_EWMH_INIT_WM_DESKTOP(
				fw, EWMH_STATE_UNDEFINED_HINT);

			return 0;
		}
		if (HAS_EWMH_INIT_WM_DESKTOP(fw) != EWMH_STATE_UNDEFINED_HINT)
		{
			return 0;
		}
		val = ewmh_AtomGetByName(
			FW_W(fw), "_NET_WM_DESKTOP", EWMH_ATOM_LIST_CLIENT_WIN,
			&size);
		if (val == NULL)
		{
			SET_HAS_EWMH_INIT_WM_DESKTOP(fw, EWMH_STATE_NO_HINT);

			return 0;
		}
#if DEBUG_EWMH_INIT_STATE
		fprintf(
			stderr, "ewmh WM_DESKTOP hint for window 0x%lx  "
			"(%i,%lu,%u)\n", FW_W(fw),
			HAS_EWMH_INIT_WM_DESKTOP(fw),
			fw->ewmh_hint_desktop, val[0]);
#endif
		if (val[0] == (CARD32)-2 || val[0] == (CARD32)-1)
		{
			S_SET_IS_STICKY_ACROSS_PAGES(SCF(*style), 1);
			S_SET_IS_STICKY_ACROSS_PAGES(SCM(*style), 1);
			S_SET_IS_STICKY_ACROSS_PAGES(SCC(*style), 1);
			S_SET_IS_STICKY_ACROSS_DESKS(SCF(*style), 1);
			S_SET_IS_STICKY_ACROSS_DESKS(SCM(*style), 1);
			S_SET_IS_STICKY_ACROSS_DESKS(SCC(*style), 1);
		}
		else if (val[0] < 256)
		{
			/* prevent crazy hints ?? */
			style->flags.use_start_on_desk = 1;
			style->flag_mask.use_start_on_desk = 1;
			style->change_mask.use_start_on_desk = 1;
			SSET_START_DESK(*style, val[0]);
		}
		SET_HAS_EWMH_INIT_WM_DESKTOP(fw, EWMH_STATE_HAS_HINT);
		fw->ewmh_hint_desktop = val[0];
		free(val);
	}
	return 0;
}

int ewmh_MoveResize(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	int dir = -1;
	int x_warp = 0;
	int y_warp = 0;
	Bool move = False;
	char cmd[256];

	if (ev == NULL)
	{
		return 0;
	}

	dir = ev->xclient.data.l[2];
	switch(dir)
	{
	case _NET_WM_MOVERESIZE_SIZE_TOPLEFT:
		break;
	case _NET_WM_MOVERESIZE_SIZE_TOP:
		x_warp = 50;
		break;
	case _NET_WM_MOVERESIZE_SIZE_TOPRIGHT:
		x_warp = 100;
		break;
	case _NET_WM_MOVERESIZE_SIZE_RIGHT:
		x_warp = 100; y_warp = 50;
		break;
	case _NET_WM_MOVERESIZE_SIZE_KEYBOARD:
	case _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT:
		x_warp = 100; y_warp = 100;
		break;
	case _NET_WM_MOVERESIZE_SIZE_BOTTOM:
		x_warp = 50; y_warp = 100;
		break;
	case _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT:
		y_warp = 100;
		break;
	case _NET_WM_MOVERESIZE_SIZE_LEFT:
		y_warp = 50;
		break;
	case _NET_WM_MOVERESIZE_MOVE_KEYBOARD:
	case _NET_WM_MOVERESIZE_MOVE:
		move = True;
		break;
	default:
		return 0;
	}

	if (move)
	{
		if (
			!is_function_allowed(
				F_MOVE, NULL, fw, RQORIG_PROGRAM_US, False))
		{
			return 0;
		}
	}
	else
	{
		if (
			!is_function_allowed(
				F_RESIZE, NULL, fw, RQORIG_PROGRAM_US, False))
		{
			return 0;
		}
	}

	if (!move)
	{
		sprintf(cmd, "WarpToWindow %i %i",x_warp,y_warp);
		execute_function_override_window(NULL, NULL, cmd, 0, fw);
	}

	if (move)
	{
		execute_function_override_window(
			NULL, NULL, "Move", 0, fw);
	}
	else
	{
		execute_function_override_window(
			NULL, NULL, "Resize", 0, fw);
	}

	return 0;
}

/*
 * WM_STATE*
 */
int ewmh_WMState(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	unsigned long maximize = 0;

	if (ev != NULL)
	{
		ewmh_atom *a1,*a2;

		a1 = ewmh_GetEwmhAtomByAtom(
			ev->xclient.data.l[1], EWMH_ATOM_LIST_WM_STATE);
		a2 = ewmh_GetEwmhAtomByAtom(
			ev->xclient.data.l[2], EWMH_ATOM_LIST_WM_STATE);

		if (a1 != NULL)
		{
			maximize |= a1->action(fw, ev, NULL, 0);
		}
		if (a2 != NULL)
		{
			maximize |= a2->action(fw, ev, NULL, 0);
		}
	}
	else if (style != NULL)
	{
		CARD32 *val;
		unsigned int nitems;
		int size = 0;
		int i;
		ewmh_atom *list = ewmh_atom_wm_state;
		int has_hint = 0;

		val = ewmh_AtomGetByName(
			FW_W(fw), "_NET_WM_STATE", EWMH_ATOM_LIST_CLIENT_WIN,
			&size);

		if (val == NULL)
		{
			size = 0;
		}

#if DEBUG_EWMH_INIT_STATE
		if (size != 0)
		{
			fprintf(
				stderr, "Window 0x%lx has an init"
				" _NET_WM_STATE hint\n",FW_W(fw));
		}
#endif
		nitems = size / sizeof(CARD32);
		while(list->name != NULL)
		{
			has_hint = 0;
			for(i = 0; i < nitems; i++)
			{
				if (list->atom == val[i])
				{
					has_hint = 1;
				}
			}
			list->action(fw, NULL, style, has_hint);
			list++;
		}
		if (val != NULL)
		{
			free(val);
		}
		return 0;
	}

	if (maximize != 0)
	{
		int max_vert = (maximize & EWMH_MAXIMIZE_VERT)? 100:0;
		int max_horiz = (maximize & EWMH_MAXIMIZE_HORIZ)? 100:0;
		char cmd[256];

		if (maximize & EWMH_MAXIMIZE_REMOVE)
		{
			sprintf(cmd,"Maximize off");
		}
		else
		{
			if (!is_function_allowed(
				    F_MAXIMIZE, NULL, fw, RQORIG_PROGRAM_US,
				    False))
			{
				return 0;
			}
			sprintf(cmd,"Maximize on %i %i", max_horiz, max_vert);
		}
		execute_function_override_window(NULL, NULL, cmd, 0, fw);
	}
	return 0;
}

int ewmh_WMStateFullScreen(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (ev == NULL && style == NULL)
	{
		return (IS_EWMH_FULLSCREEN(fw));
	}

	if (ev == NULL && style != NULL)
	{
		/* start full screen */
		unsigned long has_hint = any;

#if DEBUG_EWMH_INIT_STATE
		if (has_hint)
		{
			fprintf(stderr,"\tFullscreen\n");
		}
#endif
		if (DO_EWMH_IGNORE_STATE_HINTS(style))
		{
			SET_HAS_EWMH_INIT_FULLSCREEN_STATE(
				fw, EWMH_STATE_UNDEFINED_HINT);
			return 0;
		}
		if (HAS_EWMH_INIT_FULLSCREEN_STATE(fw) !=
		    EWMH_STATE_UNDEFINED_HINT)
		{
			return 0;
		}
		if (!has_hint)
		{
			SET_HAS_EWMH_INIT_FULLSCREEN_STATE(
				fw, EWMH_STATE_NO_HINT);
			return 0;
		}
		SET_EWMH_FULLSCREEN(fw,True);
		SET_HAS_EWMH_INIT_FULLSCREEN_STATE(fw, EWMH_STATE_HAS_HINT);
		return 0;
	}

	if (ev != NULL)
	{
		/* client message */
		int bool_arg = ev->xclient.data.l[0];
		int is_full_screen;

		is_full_screen = IS_EWMH_FULLSCREEN(fw);
		if ((bool_arg == NET_WM_STATE_TOGGLE && !is_full_screen) ||
		    bool_arg == NET_WM_STATE_ADD)
		{
			EWMH_fullscreen(fw);
		}
		else
		{
			if (HAS_EWMH_INIT_FULLSCREEN_STATE(fw) ==
			    EWMH_STATE_HAS_HINT)
			{
				/* the application started fullscreen */
				SET_HAS_EWMH_INIT_FULLSCREEN_STATE(
					fw, EWMH_STATE_NO_HINT);
			}
			/* unmaximize will restore is_ewmh_fullscreen,
			 * layer and apply_decor_change */
			execute_function_override_window(
				NULL, NULL, "Maximize off", 0, fw);
		}
		if ((IS_EWMH_FULLSCREEN(fw) &&
		     !DO_EWMH_USE_STACKING_HINTS(fw)) ||
		    (!IS_EWMH_FULLSCREEN(fw) &&
		     DO_EWMH_USE_STACKING_HINTS(fw)))
		{
			/* On: if not raised by a layer cmd raise
			 * Off: if lowered by a layer cmd raise */
			execute_function_override_window(
				NULL, NULL, "Raise", 0, fw);
		}
	}

	return 0;
}

int ewmh_WMStateHidden(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (ev == NULL && style == NULL)
	{
		unsigned long do_restore = any;

		if (do_restore)
		{
			if (HAS_EWMH_INIT_HIDDEN_STATE(fw) ==
			    EWMH_STATE_HAS_HINT)
			{
				return True;
			}
			return False;
		}
		return IS_ICONIFIED(fw);
	}

	if (ev == NULL && style != NULL)
	{
		/* start iconified */
		unsigned long has_hint = any;

#if DEBUG_EWMH_INIT_STATE
		if (has_hint)
			fprintf(stderr,"\tHidden\n");
#endif
		if (DO_EWMH_IGNORE_STATE_HINTS(style))
		{
			SET_HAS_EWMH_INIT_HIDDEN_STATE(
				fw, EWMH_STATE_UNDEFINED_HINT);
			return 0;
		}
		if (HAS_EWMH_INIT_HIDDEN_STATE(fw) !=
		    EWMH_STATE_UNDEFINED_HINT)
		{
			return 0;
		}
		if (!has_hint)
		{
			SET_HAS_EWMH_INIT_HIDDEN_STATE(fw, EWMH_STATE_NO_HINT);
			return 0;
		}
		style->flags.do_start_iconic = 1;
		style->flag_mask.do_start_iconic = 1;
		style->change_mask.do_start_iconic = 1;
		SET_HAS_EWMH_INIT_HIDDEN_STATE(fw, EWMH_STATE_HAS_HINT);
		return 0;
	}

	if (ev != NULL)
	{
		/* client message */
		char cmd[16];
		int bool_arg = ev->xclient.data.l[0];

		if ((bool_arg == NET_WM_STATE_TOGGLE && !IS_ICONIFIED(fw)) ||
		    bool_arg == NET_WM_STATE_ADD)
		{
			/* iconify */
			if (
				!is_function_allowed(
					F_ICONIFY, NULL, fw, RQORIG_PROGRAM_US,
					False))
			{
				return 0;
			}
			sprintf(cmd, "Iconify on");
		}
		else
		{
			/* deiconify */
			sprintf(cmd, "Iconify off");
		}
		execute_function_override_window(NULL, NULL, cmd, 0, fw);
	}
	return 0;
}

int ewmh_WMStateMaxHoriz(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{

	if (ev == NULL && style == NULL)
	{
#if 0
		return (IS_MAXIMIZED(fw) && !IS_EWMH_FULLSCREEN(fw));
#else
		/* DV: the notion of vertical/horizontal maximization does not
		 * make any sense in fvwm, so just claim we're never maximized
		 */
		return 0;
#endif
	}

	if (ev == NULL && style != NULL)
	{
		unsigned long has_hint = any;
#if DEBUG_EWMH_INIT_STATE
		if (has_hint)
		{
			fprintf(
				stderr, "\t Maxhoriz %i\n",
				HAS_EWMH_INIT_MAXHORIZ_STATE(fw));
		}
#endif
		if (DO_EWMH_IGNORE_STATE_HINTS(style))
		{
			SET_HAS_EWMH_INIT_MAXHORIZ_STATE(
				fw, EWMH_STATE_UNDEFINED_HINT);
			return 0;
		}

                /* If the initial state is STATE_NO_HINT we still want to
                 * override it, since having just one of MAXIMIZED_HORIZ or
                 * MAXIMIZED_HORZ is enough to make the window maximized.
                 */
		if (HAS_EWMH_INIT_MAXHORIZ_STATE(fw) ==
		    EWMH_STATE_HAS_HINT)
		{
			return 0;
		}
		if (!has_hint)
		{
			SET_HAS_EWMH_INIT_MAXHORIZ_STATE(
				fw, EWMH_STATE_NO_HINT);
			return 0;
		}
		SET_HAS_EWMH_INIT_MAXHORIZ_STATE(fw, EWMH_STATE_HAS_HINT);
		return 0;
	}

	if (ev != NULL)
	{
		/* client message */
		int cmd_arg = ev->xclient.data.l[0];
		if (
			!IS_MAXIMIZED(fw) &&
			(cmd_arg == NET_WM_STATE_TOGGLE ||
			 cmd_arg == NET_WM_STATE_ADD))
		{
			return EWMH_MAXIMIZE_HORIZ;
		}
		else if (
			IS_MAXIMIZED(fw) &&
			(cmd_arg == NET_WM_STATE_TOGGLE ||
			 cmd_arg == NET_WM_STATE_REMOVE))
		{
			return EWMH_MAXIMIZE_REMOVE;
		}
	}

	return 0;
}

int ewmh_WMStateMaxVert(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{

	if (ev == NULL && style == NULL)
	{
#if 0
		return (IS_MAXIMIZED(fw) && !IS_EWMH_FULLSCREEN(fw));
#else
		/* DV: the notion of vertical/horizontal maximization does not
		 * make any sense in fvwm, so just claim we're never maximized
		 */
		return 0;
#endif
	}

	if (ev == NULL && style != NULL)
	{
		unsigned long has_hint = any;
#if DEBUG_EWMH_INIT_STATE
		if (has_hint)
		{
			fprintf(
				stderr, "\t Maxvert %i\n",
				HAS_EWMH_INIT_MAXVERT_STATE(fw));
		}
#endif
		if (DO_EWMH_IGNORE_STATE_HINTS(style))
		{
			SET_HAS_EWMH_INIT_MAXVERT_STATE(
				fw, EWMH_STATE_UNDEFINED_HINT);
			return 0;
		}
		if (HAS_EWMH_INIT_MAXVERT_STATE(fw) !=
		    EWMH_STATE_UNDEFINED_HINT)
		{
			return 0;
		}
		if (!has_hint)
		{
			SET_HAS_EWMH_INIT_MAXVERT_STATE(
				fw, EWMH_STATE_NO_HINT);
			return 0;
		}
		SET_HAS_EWMH_INIT_MAXVERT_STATE(fw, EWMH_STATE_HAS_HINT);
		return 0;
	}

	if (ev != NULL)
	{
		/* client message */
		int cmd_arg = ev->xclient.data.l[0];
		if (
			!IS_MAXIMIZED(fw) &&
			(cmd_arg == NET_WM_STATE_TOGGLE ||
			 cmd_arg == NET_WM_STATE_ADD))
		{
			return EWMH_MAXIMIZE_VERT;
		}
		else if (
			IS_MAXIMIZED(fw) &&
			(cmd_arg == NET_WM_STATE_TOGGLE ||
			 cmd_arg == NET_WM_STATE_REMOVE))
		{
			return EWMH_MAXIMIZE_REMOVE;
		}
	}

	return 0;
}

int ewmh_WMStateModal(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (ev == NULL && style == NULL)
	{
		unsigned long do_restore = any;

		if (do_restore)
		{
			if (HAS_EWMH_INIT_MODAL_STATE(fw) ==
			    EWMH_STATE_HAS_HINT)
			{
				return True;
			}
			return False;
		}
		return IS_EWMH_MODAL(fw);
	}

	if (ev == NULL && style != NULL)
	{
		unsigned long has_hint = any;
#if DEBUG_EWMH_INIT_STATE
		if (has_hint)
		{
			fprintf(
				stderr, "\t Modal %i\n",
				HAS_EWMH_INIT_MODAL_STATE(fw));
		}
#endif
		if (DO_EWMH_IGNORE_STATE_HINTS(style))
		{
			SET_HAS_EWMH_INIT_MODAL_STATE(
				fw, EWMH_STATE_UNDEFINED_HINT);
			return 0;
		}
		if (HAS_EWMH_INIT_MODAL_STATE(fw) != EWMH_STATE_UNDEFINED_HINT)
		{
			return 0;
		}
		if (!has_hint)
		{
			SET_HAS_EWMH_INIT_MODAL_STATE(fw, EWMH_STATE_NO_HINT);
			return 0;
		}

		/* the window map or had mapped with a modal hint */
		if (IS_TRANSIENT(fw))
		{
			SET_EWMH_MODAL(fw, True);
			/* the window is a modal transient window so we grab
			 * the focus it will be good to raise it but ... */
			FPS_GRAB_FOCUS_TRANSIENT(
				S_FOCUS_POLICY(SCF(*style)), 1);
			FPS_GRAB_FOCUS_TRANSIENT(
				S_FOCUS_POLICY(SCM(*style)), 1);
			FPS_GRAB_FOCUS_TRANSIENT(
				S_FOCUS_POLICY(SCC(*style)), 1);
			SET_HAS_EWMH_INIT_MODAL_STATE(
				fw, EWMH_STATE_HAS_HINT);
		}
		else
		{
			SET_EWMH_MODAL(fw, False);
			if (!FP_DO_GRAB_FOCUS_TRANSIENT(
				    S_FOCUS_POLICY(SCF(*style))))
			{
				FPS_GRAB_FOCUS_TRANSIENT(
					S_FOCUS_POLICY(SCF(*style)), 0);
				FPS_GRAB_FOCUS_TRANSIENT(
					S_FOCUS_POLICY(SCM(*style)), 1);
				FPS_GRAB_FOCUS_TRANSIENT(
					S_FOCUS_POLICY(SCC(*style)), 1);
			}
		}
		return 0;
	}

	if (ev != NULL && fw != NULL)
	{
		/* client message: I do not think we can get such message */
	    	/* java sends this message */
		int cmd_arg = ev->xclient.data.l[0];
		if (
			!IS_EWMH_MODAL(fw) &&
			(cmd_arg == NET_WM_STATE_TOGGLE ||
			 cmd_arg == NET_WM_STATE_ADD))
		{
		    /* ON */
		}
		else if (
			IS_EWMH_MODAL(fw) &&
			(cmd_arg == NET_WM_STATE_TOGGLE ||
			 cmd_arg == NET_WM_STATE_REMOVE))
		{
		    /* OFF */
		}
	/* 			!MODAL			MODAL
	 * CMD
	 * STATE_ADD		  ON			do nothing
	 * STATE_TOGGLE		  ON			  OFF
	 * STATE_REMOVE		  do nothing		  OFF
	 */
	}
	return 0;
}

int ewmh_WMStateShaded(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{

	if (ev == NULL && style == NULL)
	{
		unsigned long do_restore = any;

		if (do_restore)
		{
			if (HAS_EWMH_INIT_SHADED_STATE(fw) ==
			    EWMH_STATE_HAS_HINT)
			{
				return True;
			}
			return False;
		}
		return IS_SHADED(fw);
	}

	if (ev == NULL && style != NULL)
	{
		/* start shaded */
		unsigned long has_hint = any;
#if DEBUG_EWMH_INIT_STATE
		if (has_hint)
		{
			fprintf(
				stderr, "\t Shaded %i\n",
				HAS_EWMH_INIT_SHADED_STATE(fw));
		}
#endif
		if (DO_EWMH_IGNORE_STATE_HINTS(style))
		{
			SET_HAS_EWMH_INIT_SHADED_STATE(
				fw, EWMH_STATE_UNDEFINED_HINT);
			return 0;
		}
		if (HAS_EWMH_INIT_SHADED_STATE(fw) !=
		    EWMH_STATE_UNDEFINED_HINT)
		{
			return 0;
		}
		if (!has_hint)
		{
			SET_HAS_EWMH_INIT_SHADED_STATE(
				fw, EWMH_STATE_NO_HINT);
			return 0;
		}

		SET_SHADED(fw, 1);
		SET_SHADED_DIR(fw, GET_TITLE_DIR(fw));
		SET_HAS_EWMH_INIT_SHADED_STATE(fw, EWMH_STATE_HAS_HINT);
		return 0;
	}

	if (ev != NULL)
	{
		/* client message */
		int cmd_arg = ev->xclient.data.l[0];
		if (
			!IS_SHADED(fw) &&
			(cmd_arg == NET_WM_STATE_TOGGLE ||
			 cmd_arg == NET_WM_STATE_ADD))
		{
			execute_function_override_window(
				NULL, NULL, "Windowshade on", 0, fw);
		}
		else if (
			IS_SHADED(fw) &&
			(cmd_arg == NET_WM_STATE_TOGGLE ||
			 cmd_arg == NET_WM_STATE_REMOVE))
		{
			execute_function_override_window(
				NULL, NULL, "Windowshade off", 0, fw);
		}
	}
	return 0;
}

int ewmh_WMStateSkipPager(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (ev == NULL && style == NULL)
	{
		unsigned long do_restore = any;

		if (do_restore)
		{
			if (HAS_EWMH_INIT_SKIP_PAGER_STATE(fw) ==
			    EWMH_STATE_HAS_HINT)
			{
				return True;
			}
			return False;
		}
		return DO_SKIP_WINDOW_LIST(fw);
	}

	if (ev == NULL && style != NULL)
	{
		unsigned long has_hint = any;
#if DEBUG_EWMH_INIT_STATE
		/*if (has_hint)*/
		fprintf(
			stderr, "\t Skip_Pager %lu, %i, %i\n", has_hint,
			HAS_EWMH_INIT_SKIP_PAGER_STATE(fw),
			DO_EWMH_IGNORE_STATE_HINTS(style));
#endif
		if (DO_EWMH_IGNORE_STATE_HINTS(style))
		{
			SET_HAS_EWMH_INIT_SKIP_PAGER_STATE(
				fw, EWMH_STATE_UNDEFINED_HINT);
			return 0;
		}
		if (HAS_EWMH_INIT_SKIP_PAGER_STATE(fw) !=
		    EWMH_STATE_UNDEFINED_HINT)
		{
			return 0;
		}
		if (!has_hint)
		{
			SET_HAS_EWMH_INIT_SKIP_PAGER_STATE(
				fw, EWMH_STATE_NO_HINT);
			return 0;
		}

		S_SET_DO_WINDOW_LIST_SKIP(SCF(*style), 1);
		S_SET_DO_WINDOW_LIST_SKIP(SCM(*style), 1);
		S_SET_DO_WINDOW_LIST_SKIP(SCC(*style), 1);
		SET_HAS_EWMH_INIT_SKIP_PAGER_STATE(fw, EWMH_STATE_HAS_HINT);
		return 0;
	}

	if (ev != NULL)
	{
		/* I do not think we can get such client message */
		int bool_arg = ev->xclient.data.l[0];

		if ((bool_arg == NET_WM_STATE_TOGGLE &&
		     !DO_SKIP_WINDOW_LIST(fw)) ||
		    bool_arg == NET_WM_STATE_ADD)
		{
		}
		else
		{
		}
	}
	return 0;
}

int ewmh_WMStateSkipTaskBar(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (ev == NULL && style == NULL)
	{
		unsigned long do_restore = any;

		if (do_restore)
		{
			if (HAS_EWMH_INIT_SKIP_TASKBAR_STATE(fw) ==
			    EWMH_STATE_HAS_HINT)
			{
				return True;
			}
			return False;
		}
		return DO_SKIP_WINDOW_LIST(fw);
	}

	if (ev == NULL && style != NULL)
	{
		unsigned long has_hint = any;
#if DEBUG_EWMH_INIT_STATE
		/*if (has_hint)*/
		fprintf(stderr,"\t Skip_Taskbar %lu, %i, %i\n",
			has_hint,
			HAS_EWMH_INIT_SKIP_TASKBAR_STATE(fw),
			DO_EWMH_IGNORE_STATE_HINTS(style));
#endif
		if (DO_EWMH_IGNORE_STATE_HINTS(style))
		{
			SET_HAS_EWMH_INIT_SKIP_TASKBAR_STATE(
				fw, EWMH_STATE_UNDEFINED_HINT);
			return 0;
		}
		if (HAS_EWMH_INIT_SKIP_TASKBAR_STATE(fw) !=
		    EWMH_STATE_UNDEFINED_HINT)
		{
			return 0;
		}
		if (!has_hint)
		{
			SET_HAS_EWMH_INIT_SKIP_TASKBAR_STATE(
				fw, EWMH_STATE_NO_HINT);
			return 0;
		}

		S_SET_DO_WINDOW_LIST_SKIP(SCF(*style), 1);
		S_SET_DO_WINDOW_LIST_SKIP(SCM(*style), 1);
		S_SET_DO_WINDOW_LIST_SKIP(SCC(*style), 1);
		SET_HAS_EWMH_INIT_SKIP_TASKBAR_STATE(
			fw, EWMH_STATE_HAS_HINT);
		return 0;
	}

	if (ev != NULL)
	{
		/* I do not think we can get such client message */
		int bool_arg = ev->xclient.data.l[0];

		if ((bool_arg == NET_WM_STATE_TOGGLE &&
		     !DO_SKIP_WINDOW_LIST(fw)) ||
		    bool_arg == NET_WM_STATE_ADD)
		{
		}
		else
		{
		}
	}
	return 0;
}

int ewmh_WMStateStaysOnTop(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (ev == NULL && style == NULL)
	{
		unsigned long do_restore = any;

		if (do_restore)
		{
			if (fw->ewmh_hint_layer == Scr.TopLayer)
			{
				return True;
			}
			return False;
		}
		if (fw->layer >= Scr.TopLayer)
		{
			return True;
		}
		return False;
	}

	if (ev == NULL && style != NULL)
	{
		unsigned long has_hint = any;
#if DEBUG_EWMH_INIT_STATE
		if (has_hint)
		{
			fprintf(stderr,"\tStaysOnTop\n");
		}
#endif
		if (!DO_EWMH_USE_STACKING_HINTS(style))
		{
			return 0;
		}
		if (!has_hint && fw->ewmh_hint_layer == 0)
		{
			fw->ewmh_hint_layer = -1;
			return 0;
		}
		if (fw->ewmh_hint_layer == -1)
		{
			return 0;
		}

		fw->ewmh_hint_layer = Scr.TopLayer;
		SSET_LAYER(*style, Scr.TopLayer);
		style->flags.use_layer = 1;
		style->flag_mask.use_layer = 1;
		style->change_mask.use_layer = 1;
		return 0;
	}

	if (ev != NULL)
	{
		/* client message */
		int cmd_arg = ev->xclient.data.l[0];

		if (!DO_EWMH_USE_STACKING_HINTS(fw))
		{
		    	/* if we don't pay attention to the hints,
			 * I don't think we should honor this request also
			 */
			return 0;
		}
		if (fw->layer < Scr.TopLayer &&
		    (cmd_arg == NET_WM_STATE_TOGGLE ||
		     cmd_arg == NET_WM_STATE_ADD))
		{
			new_layer(fw, Scr.TopLayer);
		}
		else if (
			fw->layer == Scr.TopLayer &&
			(cmd_arg == NET_WM_STATE_TOGGLE ||
			 cmd_arg == NET_WM_STATE_REMOVE))
		{
			new_layer(fw, Scr.DefaultLayer);
		}
	/* 			layer < TopLayer	layer == TopLayer
	 * CMD
	 * STATE_ADD		new_layer(TOP)		   do nothing
	 * STATE_TOGGLE		new_layer(TOP)		new_layer(DEFAULT)
	 * STATE_REMOVE		  do nothing		new_layer(DEFAULT)
	 */
	}
	return 0;
}

int ewmh_WMStateStaysOnBottom(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	if (ev == NULL && style == NULL)
	{
		unsigned long do_restore = any;

		if (do_restore)
		{
			if (fw->ewmh_hint_layer == Scr.BottomLayer)
			{
				return True;
			}
			return False;
		}
		if (fw->layer <= Scr.BottomLayer)
		{
			return True;
		}
		return False;
	}

	if (ev == NULL && style != NULL)
	{
		unsigned long has_hint = any;
#if DEBUG_EWMH_INIT_STATE
		if (has_hint)
			fprintf(stderr,"\tStaysOnBottom\n");
#endif
		if (!DO_EWMH_USE_STACKING_HINTS(style))
		{
			return 0;
		}
		if (!has_hint && fw->ewmh_hint_layer == 0)
		{
			fw->ewmh_hint_layer = -1;
			return 0;
		}
		if (fw->ewmh_hint_layer == -1)
		{
			return 0;
		}

		fw->ewmh_hint_layer = Scr.BottomLayer;
		SSET_LAYER(*style, Scr.BottomLayer);
		style->flags.use_layer = 1;
		style->flag_mask.use_layer = 1;
		style->change_mask.use_layer = 1;
		return 0;
	}

	if (ev != NULL)
	{
		/* client message */
		int cmd_arg = ev->xclient.data.l[0];

		if (!DO_EWMH_USE_STACKING_HINTS(fw))
		{
		    	/* if we don't pay attention to the hints,
			 * I don't think we should honor this request also
			 */
			return 0;
		}
		if (
			fw->layer > Scr.BottomLayer &&
			(cmd_arg == NET_WM_STATE_TOGGLE ||
			 cmd_arg == NET_WM_STATE_ADD))
		{
			new_layer(fw, Scr.BottomLayer);
		}
		else if (
			fw->layer == Scr.BottomLayer &&
			(cmd_arg == NET_WM_STATE_TOGGLE ||
			 cmd_arg == NET_WM_STATE_REMOVE))
		{
			new_layer(fw, Scr.DefaultLayer);
		}
	/* 			layer > BottomLayer	layer == BottomLayer
	 * CMD
	 * STATE_ADD		new_layer(BOTTOM)	   do nothing
	 * STATE_TOGGLE		new_layer(BOTTOM)	new_layer(DEFAULT)
	 * STATE_REMOVE		  do nothing		new_layer(DEFAULT)
	 */
	}
	return 0;
}

int ewmh_WMStateSticky(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{

	if (ev == NULL && style == NULL)
	{
		unsigned long do_restore = any;

		if (do_restore)
		{
			if (HAS_EWMH_INIT_STICKY_STATE(fw) ==
			    EWMH_STATE_HAS_HINT)
			{
				return True;
			}
			return False;
		}
		return (IS_STICKY_ACROSS_PAGES(fw) &&
			IS_STICKY_ACROSS_DESKS(fw));
	}

	if (ev == NULL && style != NULL)
	{
		/* start sticky */
		unsigned long has_hint = any;
#if DEBUG_EWMH_INIT_STATE
		if (has_hint)
		{
			fprintf(stderr,"\t Sticky\n");
		}
#endif
		if (DO_EWMH_IGNORE_STATE_HINTS(style))
		{
			SET_HAS_EWMH_INIT_STICKY_STATE(
				fw, EWMH_STATE_UNDEFINED_HINT);
			return 0;
		}
		if (HAS_EWMH_INIT_STICKY_STATE(fw) !=
		    EWMH_STATE_UNDEFINED_HINT)
		{
			return 0;
		}
		if (!has_hint)
		{
			SET_HAS_EWMH_INIT_STICKY_STATE(
				fw, EWMH_STATE_NO_HINT);
			return 0;
		}
		S_SET_IS_STICKY_ACROSS_PAGES(SCF(*style), 1);
		S_SET_IS_STICKY_ACROSS_PAGES(SCM(*style), 1);
		S_SET_IS_STICKY_ACROSS_PAGES(SCC(*style), 1);
		S_SET_IS_STICKY_ACROSS_DESKS(SCF(*style), 1);
		S_SET_IS_STICKY_ACROSS_DESKS(SCM(*style), 1);
		S_SET_IS_STICKY_ACROSS_DESKS(SCC(*style), 1);
		SET_HAS_EWMH_INIT_STICKY_STATE(fw, EWMH_STATE_HAS_HINT);
		return 0;
	}

	if (ev != NULL)
	{
		/* client message */
		int bool_arg = ev->xclient.data.l[0];
		if ((bool_arg == NET_WM_STATE_TOGGLE &&
		     (!IS_STICKY_ACROSS_PAGES(fw) ||
		      !IS_STICKY_ACROSS_DESKS(fw))) ||
		    bool_arg == NET_WM_STATE_ADD)
		{
			execute_function_override_window(
				NULL, NULL, "Stick on", 0, fw);
		}
		else if ((IS_STICKY_ACROSS_PAGES(fw) ||
			  IS_STICKY_ACROSS_DESKS(fw)) &&
			 (bool_arg == NET_WM_STATE_TOGGLE ||
			  bool_arg == NET_WM_STATE_REMOVE))
		{
			execute_function_override_window(
				NULL, NULL, "Stick off", 1, fw);
		}
	}
	return 0;
}

/*
 * Property Notify (_NET_WM_ICON is in ewmh_icon.c, _NET_WM_*NAME are in
 * ewmh_name)                        *
 */
int ewmh_WMIconGeometry(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	int size;
	CARD32 *val;

	/* FIXME: After a (un)slide of kicker the geometry are wrong (not
	 * because we set the geometry just after the property notify).  This
	 * does not happen with kwin */
	val = ewmh_AtomGetByName(
		FW_W(fw), "_NET_WM_ICON_GEOMETRY",
		EWMH_ATOM_LIST_PROPERTY_NOTIFY, &size);

	if (val != NULL && size < 4 * sizeof(CARD32))
	{
		fvwm_msg(
			WARN, "ewmh_WMIconGeometry",
			"The application window (id %#lx)\n"
			"  \"%s\" tried to set to an icon geometry via EWMH\n"
			"  but provided only %d of the 4 values required.\n"
			"    fvwm is ignoring this request.\n",
			fw ? FW_W(fw) : 0, fw ? fw->name.name : "(none)",
			(int)(size / sizeof(CARD32)));
		fvwm_msg_report_app_and_workers();
		free(val);
		val = NULL;
	}
	if (val == NULL)
	{
		fw->ewmh_icon_geometry.x = 0;
		fw->ewmh_icon_geometry.y = 0;
		fw->ewmh_icon_geometry.width = 0;
		fw->ewmh_icon_geometry.height = 0;

		return 0;
	}
	fw->ewmh_icon_geometry.x = val[0];
	fw->ewmh_icon_geometry.y = val[1];
	fw->ewmh_icon_geometry.width = val[2];
	fw->ewmh_icon_geometry.height = val[3];
	free(val);

	return 0;
}

/**** for animation ****/
void EWMH_GetIconGeometry(FvwmWindow *fw, rectangle *icon_rect)
{
	if (!IS_ICON_SUPPRESSED(fw) ||
	    (fw->ewmh_icon_geometry.x == 0 &&
	     fw->ewmh_icon_geometry.y == 0 &&
	     fw->ewmh_icon_geometry.width == 0 &&
	     fw->ewmh_icon_geometry.height == 0))
	{
		return;
	}
	icon_rect->x = fw->ewmh_icon_geometry.x;
	icon_rect->y = fw->ewmh_icon_geometry.y;
	icon_rect->width = fw->ewmh_icon_geometry.width;
	icon_rect->height = fw->ewmh_icon_geometry.height;

	return;
}

int ewmh_WMStrut(
	FvwmWindow *fw, XEvent *ev, window_style *style, unsigned long any)
{
	int size = 0;
	CARD32 *val;

	if (ev == NULL)
	{
		fw->dyn_strut.left = fw->strut.left = 0;
		fw->dyn_strut.right = fw->strut.right = 0;
		fw->dyn_strut.top = fw->strut.top = 0;
		fw->dyn_strut.bottom = fw->strut.bottom = 0;
	}

	val = ewmh_AtomGetByName(
		FW_W(fw), "_NET_WM_STRUT",
		EWMH_ATOM_LIST_PROPERTY_NOTIFY, &size);

	if (val == NULL)
	{
		return 0;
	}

	if ((val[0] > 0 || val[1] > 0 || val[2] > 0 || val[3] > 0)
	    &&
	    (val[0] !=  fw->strut.left || val[1] != fw->strut.right ||
	     val[2] != fw->strut.top   || val[3] !=  fw->strut.bottom))
	{
		fw->strut.left   = val[0];
		fw->strut.right  = val[1];
		fw->strut.top    = val[2];
		fw->strut.bottom = val[3];
		ewmh_ComputeAndSetWorkArea();
	}
	if (val[0] !=  fw->dyn_strut.left ||
	    val[1] != fw->dyn_strut.right ||
	    val[2] != fw->dyn_strut.top ||
	    val[3] !=  fw->dyn_strut.bottom)
	{
		fw->dyn_strut.left   = val[0];
		fw->dyn_strut.right  = val[1];
		fw->dyn_strut.top    = val[2];
		fw->dyn_strut.bottom = val[3];
		ewmh_HandleDynamicWorkArea();
	}
	free(val);

	return 0;
}

Bool EWMH_ProcessClientMessage(const exec_context_t *exc)
{
	ewmh_atom *ewmh_a = NULL;
	FvwmWindow *fw = exc->w.fw;
	XEvent *ev = exc->x.elast;

	if ((ewmh_a = (ewmh_atom *)ewmh_GetEwmhAtomByAtom(
		     ev->xclient.message_type, EWMH_ATOM_LIST_CLIENT_ROOT)) !=
	    NULL)
	{
		if (ewmh_a->action != None)
		{
			ewmh_a->action(fw, ev, NULL, 0);
		}
		return True;
	}

	if ((ewmh_a = (ewmh_atom *)ewmh_GetEwmhAtomByAtom(
		     ev->xclient.message_type, EWMH_ATOM_LIST_CLIENT_WIN))
	    == NULL)
	{
		return False;
	}

	if (ev->xclient.window == None)
	{
		return False;
	}

	/* these one are special: we can get it on an unamaged window */
	if (StrEquals(ewmh_a->name, "_NET_MOVERESIZE_WINDOW") ||
	    StrEquals(ewmh_a->name, "_NET_RESTACK_WINDOW"))
	{
		ewmh_a->action(fw, ev, NULL, 0);
		return True;
	}

	if (fw == NULL)
	{
		return False;
	}


	if ((ewmh_a = (ewmh_atom *)ewmh_GetEwmhAtomByAtom(
		     ev->xclient.message_type, EWMH_ATOM_LIST_CLIENT_WIN)) !=
	    NULL)
	{
		if (ewmh_a->action != None)
		{
			ewmh_a->action(fw, ev, NULL, 0);
		}
		return True;
	}

	return False;
}

void EWMH_ProcessPropertyNotify(const exec_context_t *exc)
{
	ewmh_atom *ewmh_a = NULL;
	FvwmWindow *fw = exc->w.fw;
	XEvent *ev = exc->x.elast;

	if ((ewmh_a = (ewmh_atom *)ewmh_GetEwmhAtomByAtom(
		     ev->xproperty.atom, EWMH_ATOM_LIST_PROPERTY_NOTIFY)) !=
	    NULL)
	{
		if (ewmh_a->action != None)
		{
			flush_property_notify_stop_at_event_type(
				ev->xproperty.atom, FW_W(fw), 0, 0);
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
			ewmh_a->action(fw, ev, NULL, 0);
		}
	}

}
