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

#include "cursor.h"
#include <X11/cursorfont.h>

static Cursor cursors[CRS_MAX];
static const unsigned int default_cursors[CRS_MAX] =
{
  None,
  XC_top_left_corner,      /* CRS_POSITION */
  XC_top_left_arrow,       /* CRS_TITLE */
  XC_top_left_arrow,       /* CRS_DEFAULT */
  XC_hand2,                /* CRS_SYS */
  XC_fleur,                /* CRS_MOVE */
  XC_sizing,               /* CRS_RESIZE */
  XC_watch,                /* CRS_WAIT */
  XC_top_left_arrow,       /* CRS_MENU */
  XC_crosshair,            /* CRS_SELECT */
  XC_pirate,               /* CRS_DESTROY */
  XC_top_side,             /* CRS_TOP */
  XC_right_side,           /* CRS_RIGHT */
  XC_bottom_side,          /* CRS_BOTTOM */
  XC_left_side,            /* CRS_LEFT */
  XC_top_left_corner,      /* CRS_TOP_LEFT */
  XC_top_right_corner,     /* CRS_TOP_RIGHT */
  XC_bottom_left_corner,   /* CRS_BOTTOM_LEFT */
  XC_bottom_right_corner,  /* CRS_BOTTOM_RIGHT */
  XC_top_side,             /* CRS_TOP_EDGE */
  XC_right_side,           /* CRS_RIGHT_EDGE */
  XC_bottom_side,          /* CRS_BOTTOM_EDGE */
  XC_left_side             /* CRS_LEFT_EDGE */
};

/***********************************************************************
 *
 *  Procedure:
 *	CreateCursors - Loads fvwm cursors
 *
 ***********************************************************************/
Cursor *CreateCursors(Display *dpy)
{
  int i;
  /* define cursors */
  cursors[0] = None;
  for (i = 1; i < CRS_MAX; i++)
  {
    cursors[i] = XCreateFontCursor(dpy, default_cursors[i]);
  }
  return cursors;
}
