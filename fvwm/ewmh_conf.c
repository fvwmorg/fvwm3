/* Copyright (C) 2001  Olivier Chapuis
 *
 * This program is free software; you can redistribute it and/or modify
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

#include "config.h"

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "cursor.h"
#include "functions.h"
#include "misc.h"
#include "screen.h"
#include "update.h"
#include "stack.h"
#include "style.h"
#include "externs.h"
#include "ewmh.h"
#include "ewmh_intern.h"

#ifdef HAVE_EWMH
/* ************************************************************************* *
 * CMDS
 * ************************************************************************* */

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

void CMD_EwmhBaseStrut(F_CMD_ARGS)
{
  int val[4];

  if (GetIntegerArguments(action, NULL, val, 4) != 4 ||
      val[0] < 0 || val[1] < 0 || val[2] < 0 || val[3] < 0)
  {
    fvwm_msg(ERR,"CMD_EwmhBaseStrut",
	     "EwmhBaseStrut needs four positive arguments");
    return;
  }

  if (ewmhc.BaseStrut.left != val[0] ||  ewmhc.BaseStrut.right != val[1] ||
      ewmhc.BaseStrut.top != val[2] ||  ewmhc.BaseStrut.bottom != val[3])
  {
    ewmhc.BaseStrut.left   = val[0];
    ewmhc.BaseStrut.right  = val[1];
    ewmhc.BaseStrut.top    = val[2];
    ewmhc.BaseStrut.bottom = val[3];
    ewmh_ComputeAndSetWorkArea();
    ewmh_HandleDynamicWorkArea();
  }
}
/* ************************************************************************* *
 * Styles
 * ************************************************************************* */

Bool EWMH_CMD_Style(char *token, window_style *ptmpstyle)
{
  int found = False;

  if (StrEquals(token, "EWMHDonateIcon"))
  {
    found = True;
    SFSET_DO_EWMH_DONATE_ICON(*ptmpstyle, 1);
    SMSET_DO_EWMH_DONATE_ICON(*ptmpstyle, 1);
    SCSET_DO_EWMH_DONATE_ICON(*ptmpstyle, 1);
  }
  else if (StrEquals(token, "EWMHDonateMiniIcon"))
  {
    found = True;
    SFSET_DO_EWMH_DONATE_MINI_ICON(*ptmpstyle, 1);
    SMSET_DO_EWMH_DONATE_MINI_ICON(*ptmpstyle, 1);
    SCSET_DO_EWMH_DONATE_MINI_ICON(*ptmpstyle, 1);
  }
  else if (StrEquals(token, "EWMHDontDonateIcon"))
  {
    found = True;
    SFSET_DO_EWMH_DONATE_ICON(*ptmpstyle, 0);
    SMSET_DO_EWMH_DONATE_ICON(*ptmpstyle, 1);
    SCSET_DO_EWMH_DONATE_ICON(*ptmpstyle, 1);
  }
  else if (StrEquals(token, "EWMHDontDonateMiniIcon"))
  {
    found = True;
    SFSET_DO_EWMH_DONATE_MINI_ICON(*ptmpstyle, 0);
    SMSET_DO_EWMH_DONATE_MINI_ICON(*ptmpstyle, 1);
    SCSET_DO_EWMH_DONATE_MINI_ICON(*ptmpstyle, 1);
  }
  else if (StrEquals(token, "EWMHMaximizeIgnoreWorkingArea"))
  {
    found = True;
    SFSET_EWMH_MAXIMIZE_MODE(*ptmpstyle, EWMH_IGNORE_WORKING_AREA);
    SMSET_EWMH_MAXIMIZE_MODE(*ptmpstyle, EWMH_WORKING_AREA_MASK);
    SCSET_EWMH_MAXIMIZE_MODE(*ptmpstyle, EWMH_WORKING_AREA_MASK);
  }
  else if (StrEquals(token, "EWMHMaximizeUseWorkingArea"))
  {
    found = True;
    SFSET_EWMH_MAXIMIZE_MODE(*ptmpstyle, EWMH_USE_WORKING_AREA);
    SMSET_EWMH_MAXIMIZE_MODE(*ptmpstyle, EWMH_WORKING_AREA_MASK);
    SCSET_EWMH_MAXIMIZE_MODE(*ptmpstyle, EWMH_WORKING_AREA_MASK);
  }
  else if (StrEquals(token, "EWMHMaximizeUseDynamicWorkingArea"))
  {
    found = True;
    SFSET_EWMH_MAXIMIZE_MODE(*ptmpstyle, EWMH_USE_DYNAMIC_WORKING_AREA);
    SMSET_EWMH_MAXIMIZE_MODE(*ptmpstyle, EWMH_WORKING_AREA_MASK);
    SCSET_EWMH_MAXIMIZE_MODE(*ptmpstyle, EWMH_WORKING_AREA_MASK);
  }
  else if (StrEquals(token, "EWMHMiniIconOverride"))
  {
    found = True;
    SFSET_DO_EWMH_MINI_ICON_OVERRIDE(*ptmpstyle, 1);
    SMSET_DO_EWMH_MINI_ICON_OVERRIDE(*ptmpstyle, 1);
    SCSET_DO_EWMH_MINI_ICON_OVERRIDE(*ptmpstyle, 1);
  }
  else if (StrEquals(token, "EWMHNoMiniIconOverride"))
  {
    found = True;
    SFSET_DO_EWMH_MINI_ICON_OVERRIDE(*ptmpstyle, 0);
    SMSET_DO_EWMH_MINI_ICON_OVERRIDE(*ptmpstyle, 1);
    SCSET_DO_EWMH_MINI_ICON_OVERRIDE(*ptmpstyle, 1);
  }
  else if (StrEquals(token, "EWMHPlacementIgnoreWorkingArea"))
  {
    found = True;
    ptmpstyle->flags.ewmh_placement_mode = EWMH_IGNORE_WORKING_AREA;
    ptmpstyle->flag_mask.ewmh_placement_mode = EWMH_WORKING_AREA_MASK;
    ptmpstyle->change_mask.ewmh_placement_mode = EWMH_WORKING_AREA_MASK;
  }
  else if (StrEquals(token, "EWMHPlacementUseWorkingArea"))
  {
    found = True;
    ptmpstyle->flags.ewmh_placement_mode = EWMH_USE_WORKING_AREA;
    ptmpstyle->flag_mask.ewmh_placement_mode = EWMH_WORKING_AREA_MASK;
    ptmpstyle->change_mask.ewmh_placement_mode = EWMH_WORKING_AREA_MASK;
  }
  else if (StrEquals(token, "EWMHPlacementUseDynamicWorkingArea"))
  {
    found = True;
    ptmpstyle->flags.ewmh_placement_mode = EWMH_USE_DYNAMIC_WORKING_AREA;
    ptmpstyle->flag_mask.ewmh_placement_mode = EWMH_WORKING_AREA_MASK;
    ptmpstyle->change_mask.ewmh_placement_mode = EWMH_WORKING_AREA_MASK;
  }
  else if (StrEquals(token, "EWMHUseStackingOrderHints"))
  {
    found = True;
    SFSET_DO_EWMH_USE_STACKING_HINTS(*ptmpstyle, 1);
    SMSET_DO_EWMH_USE_STACKING_HINTS(*ptmpstyle, 1);
    SCSET_DO_EWMH_USE_STACKING_HINTS(*ptmpstyle, 1);
  }
  else if (StrEquals(token, "EWMHIgnoreStackingOrderHints"))
  {
    found = True;
    SFSET_DO_EWMH_USE_STACKING_HINTS(*ptmpstyle, 0);
    SMSET_DO_EWMH_USE_STACKING_HINTS(*ptmpstyle, 1);
    SCSET_DO_EWMH_USE_STACKING_HINTS(*ptmpstyle, 1);
  }
  else if (StrEquals(token, "EWMHUseStateHints"))
  {
    found = True;
    SFSET_DO_EWMH_IGNORE_STATE_HINTS(*ptmpstyle, 0);
    SMSET_DO_EWMH_IGNORE_STATE_HINTS(*ptmpstyle, 1);
    SCSET_DO_EWMH_IGNORE_STATE_HINTS(*ptmpstyle, 1);
  }
  else if (StrEquals(token, "EWMHIgnoreStateHints"))
  {
    found = True;
    SFSET_DO_EWMH_IGNORE_STATE_HINTS(*ptmpstyle, 1);
    SMSET_DO_EWMH_IGNORE_STATE_HINTS(*ptmpstyle, 1);
    SCSET_DO_EWMH_IGNORE_STATE_HINTS(*ptmpstyle, 1);
  }
  else if (StrEquals(token, "EWMHUseStrutHints"))
  {
    found = True;
    SFSET_DO_EWMH_IGNORE_STRUT_HINTS(*ptmpstyle, 0);
    SMSET_DO_EWMH_IGNORE_STRUT_HINTS(*ptmpstyle, 1);
    SCSET_DO_EWMH_IGNORE_STRUT_HINTS(*ptmpstyle, 1);
  }
  else if (StrEquals(token, "EWMHIgnoreStrutHints"))
  {
    found = True;
    SFSET_DO_EWMH_IGNORE_STRUT_HINTS(*ptmpstyle, 1);
    SMSET_DO_EWMH_IGNORE_STRUT_HINTS(*ptmpstyle, 1);
    SCSET_DO_EWMH_IGNORE_STRUT_HINTS(*ptmpstyle, 1);
  }
  return found;
}

#else /* HAVE_EWMH */

void CMD_EwmhNumberOfDesktops(F_CMD_ARGS)
{
}

void CMD_EwmhBaseStrut(F_CMD_ARGS)
{
}

Bool EWMH_CMD_Style(char *token, window_style *ptmpstyle)
{
  int found = False;

  if (StrEquals(token, "EWMHDonateIcon"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHDonateMiniIcon"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHDontDonateIcon"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHDontDonateMiniIcon"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHMaximizeIgnoreWorkingArea"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHMaximizeUseWorkingArea"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHMaximizeUseDynamicWorkingArea"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHMiniIconOverride"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHNoMiniIconOverride"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHPlacementIgnoreWorkingArea"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHPlacementUseWorkingArea"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHPlacementUseDynamicWorkingArea"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHUseStackingOrderHints"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHIgnoreStackingOrderHints"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHUseStateHints"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHIgnoreStateHints"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHUseStrutHints"))
  {
    found = True;
  }
  else if (StrEquals(token, "EWMHIgnoreStrutHints"))
  {
    found = True;
  }
  return found;
}

#endif /* HAVE_EWMH */
