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

/* ---------------------------- included header files ----------------------- */

#include "config.h"
#include <stdio.h>

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "cursor.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "update.h"
#include "style.h"
#include "builtins.h"
#include "borders.h"
#include "frame.h"
#include "gnome.h"
#include "ewmh.h"
#include "icons.h"
#include "geometry.h"
#include "move_resize.h"
#include "add_window.h"
#include "module_interface.h"
#include "focus.h"
#include "stack.h"
#include "icons.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

/* ---------------------------- interface functions ------------------------- */

static void init_style(
	FvwmWindow *old_t, FvwmWindow *t, window_style *pstyle,
	short *pbuttons)
{
	/* copy the window structure because we still need some old values. */
	memcpy(old_t, t, sizeof(FvwmWindow));
	/* determine level of decoration */
	setup_style_and_decor(t, pstyle, pbuttons);
}

static void apply_window_updates(
	FvwmWindow *t, update_win *flags, window_style *pstyle,
	FvwmWindow *focus_w)
{
	FvwmWindow old_t;
	short buttons;
	Bool is_style_initialised = False;
	rectangle frame_g;
	const exec_context_t *exc;
	exec_context_changes_t ecc;

	frame_g.x = t->frame_g.x;
	frame_g.y = t->frame_g.y;
	frame_g.width = t->frame_g.width;
	frame_g.height = t->frame_g.height;
	if (flags->do_setup_focus_policy)
	{
		setup_focus_policy(t);
		if (t == focus_w &&
		    fpol_query_allow_user_focus(&FW_FOCUS_POLICY(t)))
		{
			focus_w = NULL;
			if (Scr.Hilite == t)
			{
				Scr.Hilite = NULL;
			}
			flags->do_redraw_decoration = True;
		}
	}
	if (flags->do_update_gnome_styles)
	{
		if (!S_DO_IGNORE_GNOME_HINTS(SCF(*pstyle)))
		{
			GNOME_GetStyle(t, pstyle);
		}
	}
	if (flags->do_update_window_grabs)
	{
		focus_grab_buttons(t);
	}
	if (IS_TRANSIENT(t) && flags->do_redecorate_transient)
	{
		flags->do_redecorate = True;
		flags->do_update_window_font = True;
	}

	/*
	 * is_sticky
	 * is_icon_sticky
	 *
	 * These are a bit complicated because they can move windows to a
	 * different page or desk. */
	ecc.type = EXCT_NULL;
	ecc.w.fw = t;
	ecc.w.w = FW_W_FRAME(t);
	ecc.w.wcontext = C_FRAME;
	exc = exc_create_context(
		&ecc, ECC_TYPE | ECC_FW | ECC_W | ECC_WCONTEXT);
	if (flags->do_update_stick_icon && IS_ICONIFIED(t) &&
	    !(IS_STICKY_ACROSS_PAGES(t) || IS_STICKY_ACROSS_DESKS(t)))
	{
		if (IS_ICON_STICKY_ACROSS_PAGES(pstyle) ||
		    IS_ICON_STICKY_ACROSS_DESKS(pstyle))
		{
			/* stick and unstick the window to force the icon on
			 * the current page */
			handle_stick(
				NULL, exc, "",
				S_IS_STICKY_ACROSS_PAGES(SCF(*pstyle)),
				S_IS_STICKY_ACROSS_DESKS(SCF(*pstyle)), 1, 1);
			handle_stick(NULL, exc, "", 0, 0, 1, 0);
		}
		else
		{
			flags->do_update_icon_title = True;
		}
	}
	else if (flags->do_update_stick)
	{
		handle_stick(
			NULL, exc, "", S_IS_STICKY_ACROSS_PAGES(SCF(*pstyle)),
			S_IS_STICKY_ACROSS_DESKS(SCF(*pstyle)), 0, 0);
	}
	exc_destroy_context(exc);
	if (FMiniIconsSupported && flags->do_update_mini_icon)
	{
		if (!HAS_EWMH_MINI_ICON(t) || DO_EWMH_MINI_ICON_OVERRIDE(t))
		{
			change_mini_icon(t, pstyle);
		}
		else
		{
			if (EWMH_SetIconFromWMIcon(t, NULL, 0, True))
			{
				SET_HAS_EWMH_MINI_ICON(t, True);
			}
			else
			{
				/* "should" not happen */
				SET_HAS_EWMH_MINI_ICON(t, False);
				change_mini_icon(t, pstyle);
			}
		}
	}
	if (flags->do_update_visible_window_name)
	{
		setup_visible_name(t, False);
		BroadcastName(M_VISIBLE_NAME,FW_W(t),FW_W_FRAME(t),
			      (unsigned long)t,t->visible_name);
		EWMH_SetVisibleName(t, False);
	}

	if (flags->do_update_visible_icon_name)
	{
		setup_visible_name(t, True);
		BroadcastName(MX_VISIBLE_ICON_NAME,FW_W(t),FW_W_FRAME(t),
			      (unsigned long)t,t->visible_icon_name);
		EWMH_SetVisibleName(t, True);
	}

	if (flags->do_update_window_font ||
	    flags->do_update_window_font_height)
	{
		if (!is_style_initialised)
		{
			init_style(&old_t, t, pstyle, &buttons);
			is_style_initialised = True;
		}
		setup_window_font(t, pstyle, flags->do_update_window_font);
		flags->do_redecorate = True;
	}
	if (flags->do_update_title_text_dir)
	{
		flags->do_redecorate = True;
	}
	if (flags->do_redecorate || flags->do_update_title_dir)
	{
		size_borders b_old;
		size_borders b_new;
		int dw = 0;
		int dh = 0;
		rectangle naked_g;
		rectangle *new_g;

		if (flags->do_redecorate)
		{
			if (!is_style_initialised)
			{
				init_style(&old_t, t, pstyle, &buttons);
				is_style_initialised = True;
			}

			/* redecorate */
			change_auxiliary_windows(t, buttons);

			/* calculate the new offsets */
			/* naked_g: geometry without decor */
			gravity_get_naked_geometry(
				old_t.hints.win_gravity, &old_t, &naked_g,
				&t->normal_g);
			/* gravity without decor */
			gravity_translate_to_northwest_geometry_no_bw(
				old_t.hints.win_gravity, &old_t, &naked_g,
				&naked_g);
			/* set normal_g with the decor */
			gravity_add_decoration(
				old_t.hints.win_gravity, t, &t->normal_g,
				&naked_g);
		}
		if (flags->do_update_title_dir)
		{
			/* new border sizes */
			get_window_borders(t, &b_old);
			SET_TITLE_DIR(t, S_TITLE_DIR(SCF(*pstyle)));
			setup_title_geometry(t, pstyle);
			get_window_borders(t, &b_new);

			/* resizing */
			dw = b_new.total_size.width - b_old.total_size.width;
			dh = b_new.total_size.height - b_old.total_size.height;
			gravity_resize(
				t->hints.win_gravity, &t->normal_g, dw, dh);
			gravity_constrain_size(
				t->hints.win_gravity, t, &t->normal_g, 0);
		}

		if (IS_MAXIMIZED(t))
		{
			if (flags->do_redecorate)
			{
				int off_x = old_t.normal_g.x - old_t.max_g.x;
				int off_y = old_t.normal_g.y - old_t.max_g.y;
				int new_off_x;
				int new_off_y;

				/* maximized windows are always considered to
				 * have NorthWestGravity */
				gravity_get_naked_geometry(
					NorthWestGravity, &old_t, &naked_g,
					&t->max_g);
				gravity_translate_to_northwest_geometry_no_bw(
					NorthWestGravity, &old_t, &naked_g,
					&naked_g);
				gravity_add_decoration(
					NorthWestGravity, t, &t->max_g,
					&naked_g);
				/* prevent random paging when unmaximizing after
				 * e.g.the border width has changed */
				new_off_x = t->normal_g.x - t->max_g.x;
				new_off_y = t->normal_g.y - t->max_g.y;
				t->max_offset.x += new_off_x - off_x;
				t->max_offset.y += new_off_y - off_y;
			}
			if (flags->do_update_title_dir)
			{
				frame_g = t->max_g;
				gravity_resize(
					t->hints.win_gravity, &t->max_g, dw,
					dh);
				gravity_constrain_size(
					t->hints.win_gravity, t, &t->max_g,
					CS_UPDATE_MAX_DEFECT);
			}
			new_g = &t->max_g;
		}
		else
		{
			new_g = &t->normal_g;
		}
		if (IS_SHADED(t))
		{
			get_unshaded_geometry(t, new_g);
			if (USED_TITLE_DIR_FOR_SHADING(t))
			{
				SET_SHADED_DIR(t, GET_TITLE_DIR(t));
			}
			get_shaded_geometry(t, &frame_g, new_g);
		}
		else
		{
			get_relative_geometry(&frame_g, new_g);
		}
		flags->do_setup_frame = True;
		flags->do_redraw_decoration = True;
	}
	if (flags->do_update_rotated_title)
	{
		if (t->title_text_rotation != ROTATION_0)
		{
			flags->do_setup_frame = True;
			flags->do_redraw_decoration = True;
		} 
	}
	if (0 && flags->do_update_title_dir)
	{
		size_borders b_old;
		size_borders b_new;
		rectangle unshaded_g;
		int dw;
		int dh;

		get_unshaded_geometry(t, &unshaded_g);
		get_window_borders(t, &b_old);
		SET_TITLE_DIR(t, S_TITLE_DIR(SCF(*pstyle)));
		setup_title_geometry(t, pstyle);
		get_window_borders(t, &b_new);
		dw = b_new.total_size.width - b_old.total_size.width;
		dh = b_new.total_size.height - b_old.total_size.height;

		frame_g = t->normal_g;
		gravity_resize(t->hints.win_gravity, &frame_g, dw, dh);
		gravity_constrain_size(t->hints.win_gravity, t, &frame_g, 0);
		t->normal_g = frame_g;
		if (IS_MAXIMIZED(t))
		{
			frame_g = t->max_g;
			gravity_resize(
				t->hints.win_gravity, &frame_g, dw, dh);
			gravity_constrain_size(
				t->hints.win_gravity, t, &frame_g,
				CS_UPDATE_MAX_DEFECT);
			t->max_g = frame_g;
		}
		if (IS_SHADED(t))
		{
			get_shaded_geometry(t, &frame_g, &unshaded_g);
		}
		else if (IS_MAXIMIZED(t))
		{
			get_relative_geometry(&frame_g, &t->max_g);
		}
		else
		{
			get_relative_geometry(&frame_g, &t->normal_g);
		}

		flags->do_setup_frame = True;
		flags->do_redraw_decoration = True;
	}
	if (flags->do_resize_window)
	{
		rectangle old_g;

		setup_frame_size_limits(t, pstyle);
		old_g = frame_g;
		frame_g = t->normal_g;
		gravity_constrain_size(t->hints.win_gravity, t, &frame_g, 0);
		t->normal_g = frame_g;
		if (IS_MAXIMIZED(t))
		{
			frame_g = t->max_g;
			gravity_constrain_size(
				t->hints.win_gravity, t, &frame_g,
				CS_UPDATE_MAX_DEFECT);
			t->max_g = frame_g;
		}
		frame_g = old_g;
		gravity_constrain_size(
			t->hints.win_gravity, t, &frame_g, 0);

		flags->do_setup_frame = True;
		flags->do_redraw_decoration = True;
	}
	if (flags->do_setup_frame)
	{
		FvwmWindow *tmp;

		setup_title_geometry(t, pstyle);
		/* frame_force_setup_window needs to know if the window is
		 * hilighted */
		tmp = get_focus_window();
		set_focus_window(focus_w);
		frame_force_setup_window(
			t, frame_g.x, frame_g.y, frame_g.width, frame_g.height,
			True);
		set_focus_window(tmp);
		GNOME_SetWinArea(t);
		EWMH_SetFrameStrut(t);
	}
	if (flags->do_update_window_color)
	{
		if (t != focus_w)
		{
			flags->do_redraw_decoration = True;
		}
		update_window_color_style(t, pstyle);
		if (t != Scr.Hilite)
		{
			flags->do_broadcast_focus = True;
		}
	}
	if (flags->do_update_window_color_hi)
	{
		if (t == focus_w)
		{
			flags->do_redraw_decoration = True;
		}
		update_window_color_hi_style(t, pstyle);
		flags->do_broadcast_focus = True;
		if (t == Scr.Hilite)
		{
			flags->do_broadcast_focus = True;
		}
	}
	if (flags->do_update_icon_title_cs_hi)
	{
		if (t == focus_w)
		{
			flags->do_redraw_icon = True;
		}
		update_icon_title_cs_hi_style(t, pstyle);
	}
	if (flags->do_update_icon_title_cs)
	{
		if (t != focus_w)
		{
			flags->do_redraw_icon = True;
		}
		update_icon_title_cs_style(t, pstyle);
	}
	if (flags->do_update_icon_background_cs)
	{
		int old_cs = t->icon_background_cs;
		
		update_icon_background_cs_style(t, pstyle);
		if ((old_cs < 0 && t->icon_background_cs >= 0) ||
		    (old_cs >= 0 && t->icon_background_cs < 0))
		{
			flags->do_update_icon = True;
		}
		else
		{
			flags->do_redraw_icon = True;
		}
	}
	if (flags->do_update_icon_size_limits)
	{
		setup_icon_size_limits(t, pstyle);
		flags->do_update_icon = True;
	}
	if (flags->do_update_icon_font)
	{
		if (!is_style_initialised)
		{
			init_style(&old_t, t, pstyle, &buttons);
			is_style_initialised = True;
		}
		setup_icon_font(t, pstyle, flags->do_update_icon_font);
		flags->do_update_icon_title = True;
	}
	if (flags->do_update_icon_boxes)
	{
		change_icon_boxes(t, pstyle);
	}
	if (flags->do_update_icon)
	{
		setup_icon_background_parameters(t, pstyle);
		setup_icon_title_parameters(t, pstyle);
		change_icon(t, pstyle);
		flags->do_update_icon_placement = True;
		flags->do_update_icon_title = False;
		flags->do_redraw_icon = False;
		flags->do_update_ewmh_icon = True;
	}
	if (flags->do_redraw_icon)
	{
		if (IS_ICONIFIED(t))
		{
			DrawIconWindow(t, True, True, False, True, NULL);;
			flags->do_redraw_decoration = False;
		}
		flags->do_update_icon_title = False;
	}
	if (flags->do_update_icon_title)
	{
		RedoIconName(t);
	}
	if (flags->do_update_icon_placement)
	{
		if (IS_ICONIFIED(t))
		{
			initial_window_options_type win_opts;

			memset(&win_opts, 0, sizeof(win_opts));
			SET_ICONIFIED(t, 0);
			Iconify(t, &win_opts);
			flags->do_redraw_decoration = False;
		}
	}
	if (flags->do_redraw_decoration)
	{
		FvwmWindow *tmp;

		/* frame_redraw_decorations needs to know if the window is
		 * hilighted */
		tmp = get_focus_window();
		set_focus_window(focus_w);
		if (IS_ICONIFIED(t))
		{
			DrawIconWindow(t, True, True, False, False, NULL);
		}
		else
		{
			border_redraw_decorations(t);
		}
		set_focus_window(tmp);
	}
	if (flags->do_update_frame_attributes)
	{
		setup_frame_attributes(t, pstyle);
	}
	if (flags->do_update_ewmh_state_hints)
	{
		EWMH_SetWMState(t, False);
	}
	if (flags->do_update_modules_flags)
	{
		BroadcastConfig(M_CONFIGURE_WINDOW,t);
	}
	if (flags->do_update_ewmh_mini_icon || flags->do_update_ewmh_icon)
	{
		EWMH_DoUpdateWmIcon(
			t, flags->do_update_ewmh_mini_icon,
			flags->do_update_ewmh_icon);
	}
	if (flags->do_update_placement_penalty)
	{
		setup_placement_penalty(t, pstyle);
	}
	if (flags->do_update_working_area)
	{
		EWMH_UpdateWorkArea();
	}
	if (flags->do_update_ewmh_stacking_hints)
	{
		if (DO_EWMH_USE_STACKING_HINTS(t))
		{
			if (t->ewmh_hint_layer > 0 &&
			    t->layer != t->ewmh_hint_layer)
			{
				new_layer(t, t->ewmh_hint_layer);
			}
		}
		else
		{
			if (t->ewmh_hint_layer > 0)
			{
				new_layer(t, Scr.DefaultLayer);
			}
		}
	}
	if (flags->do_update_ewmh_allowed_actions)
	{
		EWMH_SetAllowedActions(t);
	}
	if (flags->do_broadcast_focus)
	{
		if (Scr.Hilite != NULL && t == Scr.Hilite)
		{
			BroadcastPacket(
				M_FOCUS_CHANGE, 5, FW_W(Scr.Hilite),
				FW_W_FRAME(Scr.Hilite), 0,
				Scr.Hilite->hicolors.fore,
				Scr.Hilite->hicolors.back);
		}
	}
	if (flags->do_refresh)
	{
		if (!IS_ICONIFIED(t))
		{
			refresh_window(FW_W_FRAME(t), False);
		}
	}
	t->shade_anim_steps = pstyle->shade_anim_steps;

	return;
}

/* ---------------------------- builtin commands ---------------------------- */

/* takes only care of destroying windows that have to go away. */
void destroy_scheduled_windows(void)
{
	FvwmWindow *t;
	FvwmWindow *next;
	Bool do_need_ungrab = False;

	if (Scr.flags.is_executing_complex_function ||
	    Scr.flags.is_executing_menu_function ||
	    !Scr.flags.is_window_scheduled_for_destroy)
	{
		return;
	}
	/* Grab the server during the style update! */
	if (GrabEm(CRS_WAIT, GRAB_BUSY))
	{
		do_need_ungrab = True;
	}
	MyXGrabServer(dpy);
	Scr.flags.is_window_scheduled_for_destroy = 0;
	/* need to destroy one or more windows before looking at the window
	 * list */
	for (t = Scr.FvwmRoot.next, next = NULL; t != NULL; t = next)
	{
		next = t->next;
		if (IS_SCHEDULED_FOR_DESTROY(t))
		{
			SET_SCHEDULED_FOR_DESTROY(t, 0);
			destroy_window(t);
		}
	}
	MyXUngrabServer(dpy);
	if (do_need_ungrab)
	{
		UngrabEm(GRAB_BUSY);
	}

	return;
}

/* similar to the flush_window_updates() function, but does only the updates
 * for a single window whose decor has been changed. */
void apply_decor_change(FvwmWindow *fw)
{
	window_style style;
	update_win flags;

	lookup_style(fw, &style);
	memset(&flags, 0, sizeof(flags));
	flags.do_redecorate = True;
	flags.do_update_window_font_height = True;
	apply_window_updates(fw, &flags, &style, get_focus_window());

	return;
}

/* Check and apply new style to each window if the style has changed. */
void flush_window_updates(void)
{
	FvwmWindow *t;
	window_style style;
	FvwmWindow *focus_fw;
	Bool do_need_ungrab = False;
	update_win flags;

	/* Grab the server during the style update! */
	if (GrabEm(CRS_WAIT, GRAB_BUSY))
	{
		do_need_ungrab = True;
	}
	MyXGrabServer(dpy);

	/* This is necessary in case the focus policy changes. With
	 * ClickToFocus some buttons have to be grabbed/ungrabbed. */
	focus_fw = get_focus_window();
	DeleteFocus(False);

	/* Apply the new default font and colours first */
	if (Scr.flags.has_default_color_changed ||
	    Scr.flags.has_default_font_changed)
	{
		ApplyDefaultFontAndColors();
	}

	/* update styles for all windows */
	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		memset(&flags, 0, sizeof(update_win));
		check_window_style_change(t, &flags, &style);
		if (Scr.flags.has_xinerama_state_changed)
		{
			flags.do_update_icon_boxes = True;
			flags.do_update_icon_placement = True;
		}
		if (Scr.flags.has_nr_buttons_changed)
		{
			flags.do_redecorate = True;
		}
		/* TODO: this is not optimised for minimal redrawing yet*/
		if (t->decor->flags.has_changed)
		{
			flags.do_redecorate = True;
			flags.do_update_window_font_height = True;
		}
		if (Scr.flags.has_default_font_changed && !HAS_ICON_FONT(t))
		{
			flags.do_update_icon_font = True;
		}
		if (Scr.flags.has_default_font_changed && !HAS_WINDOW_FONT(t))
		{
			flags.do_update_window_font = True;
		}
		if (t->decor->flags.has_title_height_changed)
		{
			flags.do_update_window_font_height = True;
		}
		if (Scr.flags.has_mouse_binding_changed)
		{
			flags.do_update_window_grabs = True;
		}
		/* now apply the changes */
		apply_window_updates(t, &flags, &style, focus_fw);
	}

	/* restore the focus; also handles the case that the previously focused
	 * window is now NeverFocus */
	if (focus_fw)
	{
		SetFocusWindow(focus_fw, False, FOCUS_SET_FORCE);
		if (Scr.flags.has_mouse_binding_changed)
		{
			focus_grab_buttons(focus_fw);
		}
	}
	else
	{
		DeleteFocus(True);
	}

	/* finally clean up the change flags */
	reset_style_changes();
	reset_decor_changes();
	Scr.flags.do_need_window_update = 0;
	Scr.flags.has_default_font_changed = 0;
	Scr.flags.has_default_color_changed = 0;
	Scr.flags.has_mouse_binding_changed = 0;
	Scr.flags.has_nr_buttons_changed = 0;
	Scr.flags.has_xinerama_state_changed = 0;

	MyXUngrabServer(dpy);
	if (do_need_ungrab)
	{
		UngrabEm(GRAB_BUSY);
	}

	return;
}

void CMD_UpdateStyles(F_CMD_ARGS)
{
	if (Scr.flags.do_need_window_update)
	{
		flush_window_updates();
	}

	return;
}
