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

#ifdef HAVE_EWMH

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
  int val[3];

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

#else /* HAVE_EWMH */

void CMD_EwmhNumberOfDesktops(F_CMD_ARGS)
{
}

void CMD_EwmhBaseStrut(F_CMD_ARGS)
{
}

#endif /* HAVE_EWMH */
