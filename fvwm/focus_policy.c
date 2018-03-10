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

/* Note: focus_policy.[ch] is meant to manage structures of type focus_policy_t
 * only.  No code dealing with *any* external data types belongs in here!  Put
 * it in focus.[ch] instead. */

/* ---------------------------- included header files ---------------------- */

#include "config.h"
#include <stdio.h>

#include "focus_policy.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

/* Initialise focus policy to the system defaults */
void fpol_init_default_fp(focus_policy_t *fp)
{
	memset(fp, 0, sizeof(focus_policy_t));
	FPS_FOCUS_ENTER(*fp, DEF_FP_FOCUS_ENTER);
	FPS_UNFOCUS_LEAVE(*fp, DEF_FP_UNFOCUS_LEAVE);
	FPS_FOCUS_CLICK_CLIENT(*fp, DEF_FP_FOCUS_CLICK_CLIENT);
	FPS_FOCUS_CLICK_DECOR(*fp, DEF_FP_FOCUS_CLICK_DECOR);
	FPS_FOCUS_BY_PROGRAM(*fp, DEF_FP_FOCUS_BY_PROGRAM);
	FPS_FOCUS_BY_FUNCTION(*fp, DEF_FP_FOCUS_BY_FUNCTION);
	FPS_WARP_POINTER_ON_FOCUS_FUNC(*fp, DEF_FP_WARP_POINTER_ON_FOCUS_FUNC);
	FPS_LENIENT(*fp, DEF_FP_LENIENT);
	FPS_RAISE_FOCUSED_CLIENT_CLICK(*fp, DEF_FP_RAISE_FOCUSED_CLIENT_CLICK);
	FPS_RAISE_UNFOCUSED_CLIENT_CLICK(
		*fp, DEF_FP_RAISE_UNFOCUSED_CLIENT_CLICK);
	FPS_RAISE_FOCUSED_DECOR_CLICK(
		*fp, DEF_FP_RAISE_FOCUSED_DECOR_CLICK);
	FPS_RAISE_UNFOCUSED_DECOR_CLICK(
		*fp, DEF_FP_RAISE_UNFOCUSED_DECOR_CLICK);
	FPS_RAISE_FOCUSED_ICON_CLICK(
		*fp, DEF_FP_RAISE_FOCUSED_ICON_CLICK);
	FPS_RAISE_UNFOCUSED_ICON_CLICK(
		*fp, DEF_FP_RAISE_UNFOCUSED_ICON_CLICK);
	FPS_MOUSE_BUTTONS(*fp, DEF_FP_MOUSE_BUTTONS);
	FPS_MODIFIERS(*fp, DEF_FP_MODIFIERS);
	FPS_PASS_FOCUS_CLICK(*fp, DEF_FP_PASS_FOCUS_CLICK);
	FPS_PASS_RAISE_CLICK(*fp, DEF_FP_PASS_RAISE_CLICK);
	FPS_IGNORE_FOCUS_CLICK_MOTION(*fp, DEF_FP_IGNORE_FOCUS_CLICK_MOTION);
	FPS_IGNORE_RAISE_CLICK_MOTION(*fp, DEF_FP_IGNORE_RAISE_CLICK_MOTION);
	FPS_ALLOW_FUNC_FOCUS_CLICK(*fp, DEF_FP_ALLOW_FUNC_FOCUS_CLICK);
	FPS_ALLOW_FUNC_RAISE_CLICK(*fp, DEF_FP_ALLOW_FUNC_RAISE_CLICK);
	FPS_GRAB_FOCUS(*fp, DEF_FP_GRAB_FOCUS);
	FPS_GRAB_FOCUS_TRANSIENT(*fp, DEF_FP_GRAB_FOCUS_TRANSIENT);
	FPS_OVERRIDE_GRAB_FOCUS(*fp, DEF_FP_OVERRIDE_GRAB_FOCUS);
	FPS_RELEASE_FOCUS(*fp, DEF_FP_RELEASE_FOCUS);
	FPS_RELEASE_FOCUS_TRANSIENT(*fp, DEF_FP_RELEASE_FOCUS_TRANSIENT);
	FPS_OVERRIDE_RELEASE_FOCUS(*fp, DEF_FP_OVERRIDE_RELEASE_FOCUS);
	FPS_SORT_WINDOWLIST_BY(*fp, DEF_FP_SORT_WINDOWLIST_BY);

	return;
}

int fpol_query_allow_set_focus(
	focus_policy_t *fpol, fpol_set_focus_by_t set_by_mode)
{
	switch (set_by_mode)
	{
	case FOCUS_SET_BY_ENTER:
		return FP_DO_FOCUS_ENTER(*fpol);
	case FOCUS_SET_BY_CLICK_CLIENT:
		return FP_DO_FOCUS_CLICK_CLIENT(*fpol);
	case FOCUS_SET_BY_CLICK_DECOR:
		return FP_DO_FOCUS_CLICK_DECOR(*fpol);
	case FOCUS_SET_BY_CLICK_ICON:
		return FP_DO_FOCUS_CLICK_ICON(*fpol);
	case FOCUS_SET_BY_PROGRAM:
		return FP_DO_FOCUS_BY_PROGRAM(*fpol);
	case FOCUS_SET_BY_FUNCTION:
		return FP_DO_FOCUS_BY_FUNCTION(*fpol);
	case FOCUS_SET_FORCE:
		return 1;
	}

	return 0;
}

int fpol_query_allow_user_focus(
	focus_policy_t *fpol)
{
	int flag = 0;

	flag |= FP_DO_FOCUS_ENTER(*fpol);
	flag |= FP_DO_FOCUS_CLICK_CLIENT(*fpol);
	flag |= FP_DO_FOCUS_CLICK_DECOR(*fpol);
	flag |= FP_DO_FOCUS_BY_FUNCTION(*fpol);

	return !!flag;
}

int fpol_is_policy_changed(
	focus_policy_t *fpol)
{
	if (FP_DO_FOCUS_ENTER(*fpol))
	{
		return 1;
	}
	if (FP_DO_UNFOCUS_LEAVE(*fpol))
	{
		return 1;
	}
	if (FP_DO_FOCUS_CLICK_CLIENT(*fpol))
	{
		return 1;
	}
	if (FP_DO_FOCUS_CLICK_DECOR(*fpol))
	{
		return 1;
	}
	if (FP_DO_FOCUS_BY_PROGRAM(*fpol))
	{
		return 1;
	}
	if (FP_DO_FOCUS_BY_FUNCTION(*fpol))
	{
		return 1;
	}
	if (FP_IS_LENIENT(*fpol))
	{
		return 1;
	}
	if (FP_DO_RAISE_FOCUSED_CLIENT_CLICK(*fpol))
	{
		return 1;
	}
	if (FP_DO_RAISE_UNFOCUSED_CLIENT_CLICK(*fpol))
	{
		return 1;
	}
	if (FP_DO_RAISE_FOCUSED_DECOR_CLICK(*fpol))
	{
		return 1;
	}
	if (FP_DO_RAISE_UNFOCUSED_DECOR_CLICK(*fpol))
	{
		return 1;
	}
	if (FP_USE_MOUSE_BUTTONS(*fpol))
	{
		return 1;
	}
	if (FP_USE_MODIFIERS(*fpol))
	{
		return 1;
	}

	return 0;
}
