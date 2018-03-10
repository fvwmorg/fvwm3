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
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "fvwm.h"
#include "cursor.h"
#include "functions.h"
#include "misc.h"
#include "screen.h"
#include "update.h"
#include "stack.h"
#include "style.h"
#include "externs.h"
#include "icons.h"
#include "ewmh.h"
#include "ewmh_intern.h"
#include "move_resize.h"

/*
 * CMDS
 */

static
void set_state_workaround(void)
{
	FvwmWindow *t;

	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if ((t->Desk != Scr.CurrentDesk) &&
		    (!is_window_sticky_across_desks(t) &&
		     !IS_ICON_UNMAPPED(t)))
		{
			if (Scr.bo.do_enable_ewmh_iconic_state_workaround)
			{
				SetMapStateProp(t, NormalState);
			}
			else
			{
				SetMapStateProp(t, IconicState);
			}
		}
	}
}

Bool EWMH_BugOpts(char *opt, Bool toggle)
{
	Bool save_isw = Scr.bo.do_enable_ewmh_iconic_state_workaround;

	if (StrEquals(opt,"EWMHIconicStateWorkaround"))
	{
		switch (toggle)
		{
		case -1:
			Scr.bo.do_enable_ewmh_iconic_state_workaround ^= 1;
			break;
		case 0:
		case 1:
			Scr.bo.do_enable_ewmh_iconic_state_workaround = toggle;
			break;
		default:
			Scr.bo.do_enable_ewmh_iconic_state_workaround = 0;
			break;
		}
		if (save_isw != Scr.bo.do_enable_ewmh_iconic_state_workaround)
		{
			set_state_workaround();
		}
		return True;
	}

	return False;
}

void CMD_EwmhNumberOfDesktops(F_CMD_ARGS)
{
	int val[2];
	int num;

	num = GetIntegerArguments(action, NULL, val, 2);
	if ((num != 1 && num != 2) || val[0] < 1 ||
	    (num == 2 && val[1] < val[0] && val[1] != 0))
	{
		fvwm_msg(ERR,"EwmhNumberOfDesktops",
			 "Bad arguments to EwmhNumberOfDesktops");
		return;
	}

	if (num == 2 && ewmhc.MaxDesktops != val[1])
	{
		ewmhc.MaxDesktops = val[1];
		num = 3;
	}
	else if (num == 1 && ewmhc.MaxDesktops != 0)
	{
		ewmhc.MaxDesktops = 0;
		num = 3;
	}

	if (ewmhc.NumberOfDesktops != val[0])
	{
		ewmhc.NumberOfDesktops = val[0];
		num = 3;
	}

	if (num == 3)
	{
		ewmhc.NeedsToCheckDesk = True;
		EWMH_SetNumberOfDesktops();
	}
}

void CMD_EwmhBaseStruts(F_CMD_ARGS)
{
	int val[4];

	if (GetIntegerArguments(action, NULL, val, 4) != 4 ||
	    val[0] < 0 || val[1] < 0 || val[2] < 0 || val[3] < 0)
	{
		fvwm_msg(ERR,"CMD_EwmhBaseStruts",
			 "EwmhBaseStruts needs four positive arguments");
		return;
	}

	if (ewmhc.BaseStrut.left != val[0] ||
	    ewmhc.BaseStrut.right != val[1] ||
	    ewmhc.BaseStrut.top != val[2] ||
	    ewmhc.BaseStrut.bottom != val[3])
	{
		ewmhc.BaseStrut.left   = val[0];
		ewmhc.BaseStrut.right  = val[1];
		ewmhc.BaseStrut.top    = val[2];
		ewmhc.BaseStrut.bottom = val[3];

		EWMH_UpdateWorkArea();
	}
}
/*
 * Styles
 */

Bool EWMH_CMD_Style(char *token, window_style *ptmpstyle, int on)
{
	int found = False;

	if (StrEquals(token, "EWMHDonateIcon"))
	{
		found = True;
		S_SET_DO_EWMH_DONATE_ICON(SCF(*ptmpstyle), on);
		S_SET_DO_EWMH_DONATE_ICON(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_DONATE_ICON(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHDonateMiniIcon"))
	{
		found = True;
		S_SET_DO_EWMH_DONATE_MINI_ICON(SCF(*ptmpstyle), on);
		S_SET_DO_EWMH_DONATE_MINI_ICON(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_DONATE_MINI_ICON(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHDontDonateIcon"))
	{
		found = True;
		S_SET_DO_EWMH_DONATE_ICON(SCF(*ptmpstyle), !on);
		S_SET_DO_EWMH_DONATE_ICON(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_DONATE_ICON(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHDontDonateMiniIcon"))
	{
		found = True;
		S_SET_DO_EWMH_DONATE_MINI_ICON(SCF(*ptmpstyle), !on);
		S_SET_DO_EWMH_DONATE_MINI_ICON(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_DONATE_MINI_ICON(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHMaximizeIgnoreWorkingArea"))
	{
		found = True;
		S_SET_EWMH_MAXIMIZE_MODE(
			SCF(*ptmpstyle), EWMH_IGNORE_WORKING_AREA);
		S_SET_EWMH_MAXIMIZE_MODE(
			SCM(*ptmpstyle), EWMH_WORKING_AREA_MASK);
		S_SET_EWMH_MAXIMIZE_MODE(
			SCC(*ptmpstyle), EWMH_WORKING_AREA_MASK);
	}
	else if (StrEquals(token, "EWMHMaximizeUseWorkingArea"))
	{
		found = True;
		S_SET_EWMH_MAXIMIZE_MODE(
			SCF(*ptmpstyle), EWMH_USE_WORKING_AREA);
		S_SET_EWMH_MAXIMIZE_MODE(
			SCM(*ptmpstyle), EWMH_WORKING_AREA_MASK);
		S_SET_EWMH_MAXIMIZE_MODE(
			SCC(*ptmpstyle), EWMH_WORKING_AREA_MASK);
	}
	else if (StrEquals(token, "EWMHMaximizeUseDynamicWorkingArea"))
	{
		found = True;
		S_SET_EWMH_MAXIMIZE_MODE(
			SCF(*ptmpstyle), EWMH_USE_DYNAMIC_WORKING_AREA);
		S_SET_EWMH_MAXIMIZE_MODE(
			SCM(*ptmpstyle), EWMH_WORKING_AREA_MASK);
		S_SET_EWMH_MAXIMIZE_MODE(
			SCC(*ptmpstyle), EWMH_WORKING_AREA_MASK);
	}
	else if (StrEquals(token, "EWMHMiniIconOverride"))
	{
		found = True;
		S_SET_DO_EWMH_MINI_ICON_OVERRIDE(SCF(*ptmpstyle), on);
		S_SET_DO_EWMH_MINI_ICON_OVERRIDE(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_MINI_ICON_OVERRIDE(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHNoMiniIconOverride"))
	{
		found = True;
		S_SET_DO_EWMH_MINI_ICON_OVERRIDE(SCF(*ptmpstyle), !on);
		S_SET_DO_EWMH_MINI_ICON_OVERRIDE(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_MINI_ICON_OVERRIDE(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHPlacementIgnoreWorkingArea"))
	{
		found = True;
		ptmpstyle->flags.ewmh_placement_mode =
			EWMH_IGNORE_WORKING_AREA;
		ptmpstyle->flag_mask.ewmh_placement_mode =
			EWMH_WORKING_AREA_MASK;
		ptmpstyle->change_mask.ewmh_placement_mode =
			EWMH_WORKING_AREA_MASK;
	}
	else if (StrEquals(token, "EWMHPlacementUseWorkingArea"))
	{
		found = True;
		ptmpstyle->flags.ewmh_placement_mode = EWMH_USE_WORKING_AREA;
		ptmpstyle->flag_mask.ewmh_placement_mode =
			EWMH_WORKING_AREA_MASK;
		ptmpstyle->change_mask.ewmh_placement_mode =
			EWMH_WORKING_AREA_MASK;
	}
	else if (StrEquals(token, "EWMHPlacementUseDynamicWorkingArea"))
	{
		found = True;
		ptmpstyle->flags.ewmh_placement_mode =
			EWMH_USE_DYNAMIC_WORKING_AREA;
		ptmpstyle->flag_mask.ewmh_placement_mode =
			EWMH_WORKING_AREA_MASK;
		ptmpstyle->change_mask.ewmh_placement_mode =
			EWMH_WORKING_AREA_MASK;
	}
	else if (StrEquals(token, "EWMHUseStackingOrderHints"))
	{
		found = True;
		S_SET_DO_EWMH_USE_STACKING_HINTS(SCF(*ptmpstyle), on);
		S_SET_DO_EWMH_USE_STACKING_HINTS(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_USE_STACKING_HINTS(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHIgnoreStackingOrderHints"))
	{
		found = True;
		S_SET_DO_EWMH_USE_STACKING_HINTS(SCF(*ptmpstyle), !on);
		S_SET_DO_EWMH_USE_STACKING_HINTS(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_USE_STACKING_HINTS(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHUseStateHints"))
	{
		found = True;
		S_SET_DO_EWMH_IGNORE_STATE_HINTS(SCF(*ptmpstyle), !on);
		S_SET_DO_EWMH_IGNORE_STATE_HINTS(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_IGNORE_STATE_HINTS(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHIgnoreStateHints"))
	{
		found = True;
		S_SET_DO_EWMH_IGNORE_STATE_HINTS(SCF(*ptmpstyle), on);
		S_SET_DO_EWMH_IGNORE_STATE_HINTS(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_IGNORE_STATE_HINTS(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHUseStrutHints"))
	{
		found = True;
		S_SET_DO_EWMH_IGNORE_STRUT_HINTS(SCF(*ptmpstyle), !on);
		S_SET_DO_EWMH_IGNORE_STRUT_HINTS(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_IGNORE_STRUT_HINTS(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHIgnoreStrutHints"))
	{
		found = True;
		S_SET_DO_EWMH_IGNORE_STRUT_HINTS(SCF(*ptmpstyle), on);
		S_SET_DO_EWMH_IGNORE_STRUT_HINTS(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_IGNORE_STRUT_HINTS(SCC(*ptmpstyle), 1);
	}
	else if (StrEquals(token, "EWMHIgnoreWindowType"))
	{
		found = True;
		S_SET_DO_EWMH_IGNORE_WINDOW_TYPE(SCF(*ptmpstyle), on);
		S_SET_DO_EWMH_IGNORE_WINDOW_TYPE(SCM(*ptmpstyle), 1);
		S_SET_DO_EWMH_IGNORE_WINDOW_TYPE(SCC(*ptmpstyle), 1);
	}
	return found;
}
