/* This irogram is free software; you can redistribute it and/or modify
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
 * Changed 10/06/97 by dje:
 * Change single IconBox into chain of IconBoxes.
 * Allow IconBox to be specified using X Geometry string.
 * Parse optional IconGrid.
 * Parse optional IconFill.
 * Use macros to make parsing more uniform, and easier to read.
 * Rewrote AddToList without tons of arg passing and merging.
 * Added a few comments.
 *
 * This module was all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * code for parsing the fvwm style command
 *
 ***********************************************************************/
#include "config.h"
#include <stdio.h>

#include "fvwm.h"
#include "cursor.h"
#include "functions.h"
#include "libs/fvwmlib.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "defaults.h"
#include "update.h"
#include "style.h"
#include "builtins.h"
#include "libs/Colorset.h"
#include "borders.h"
#include "gnome.h"
#include "icons.h"
#include "focus.h"
#include "geometry.h"
#include "move_resize.h"
#include "add_window.h"

static void apply_window_updates(
  FvwmWindow *t, update_win *flags, window_style *pstyle, FvwmWindow *focus_w)
{
  /*
   * is_sticky
   * is_icon_sticky
   *
   * These are a bit complicated because they can move windows to a different
   * page or desk. */
  if (flags->do_update_stick_icon)
  {
    /* stick and unstick the window to force the icon on the current page */
    handle_stick(&Event, t->frame, t, C_FRAME, "", 0, 1);
    handle_stick(&Event, t->frame, t, C_FRAME, "", 0, 0);
  }
  else if (flags->do_update_stick)
  {
    handle_stick(&Event, t->frame, t, C_FRAME, "", 0, SFIS_STICKY(*pstyle));
  }
#ifdef MINI_ICONS
  if (flags->do_update_mini_icon)
  {
    change_mini_icon(t, pstyle);
  }
#endif
  if (flags->do_redecorate)
  {
    char left_buttons;
    char right_buttons;
    FvwmWindow old_t;
    rectangle naked_g;
    rectangle *new_g;

    /* copy window structure because we still need some old values */
    memcpy(&old_t, t, sizeof(FvwmWindow));

    /* determine level of decoration */
    setup_style_and_decor(t, pstyle, &left_buttons, &right_buttons);

    /* redecorate */
    change_auxiliary_windows(t, left_buttons, right_buttons);

    /* calculate the new offsets */
    gravity_get_naked_geometry(
      old_t.hints.win_gravity, &old_t, &naked_g, &t->normal_g);
    gravity_translate_to_northwest_geometry_no_bw(
      old_t.hints.win_gravity, &old_t, &naked_g, &naked_g);
    gravity_add_decoration(
      old_t.hints.win_gravity, t, &t->normal_g, &naked_g);
    if (IS_MAXIMIZED(t))
    {
      /* maximized windows are always considered to have NorthWestGravity */
      gravity_get_naked_geometry(
	NorthWestGravity, &old_t, &naked_g, &t->max_g);
      gravity_translate_to_northwest_geometry_no_bw(
	NorthWestGravity, &old_t, &naked_g, &naked_g);
      gravity_add_decoration(
	NorthWestGravity, t, &t->max_g, &naked_g);
      new_g = &t->max_g;
    }
    else
    {
      new_g = &t->normal_g;
    }
    get_relative_geometry(&t->frame_g, new_g);

    flags->do_setup_frame = True;
    flags->do_redraw_decoration = True;
  }
  if (flags->do_resize_window)
  {
    setup_frame_size_limits(t, pstyle);
    flags->do_setup_frame = True;
    flags->do_redraw_decoration = True;
  }
  if (flags->do_setup_frame)
  {
    if (IS_SHADED(t))
      get_shaded_geometry(t, &t->frame_g, &t->frame_g);
    ForceSetupFrame(t, t->frame_g.x, t->frame_g.y, t->frame_g.width,
		    t->frame_g.height, True);
#ifdef GNOME
    GNOME_SetWinArea(t);
#endif
  }
  if (flags->do_update_window_color)
  {
    if (focus_w != t)
      flags->do_redraw_decoration = True;
    update_window_color_style(t, pstyle);
  }
  if (flags->do_update_window_color_hi)
  {
    if (focus_w == t)
      flags->do_redraw_decoration = True;
    update_window_color_hi_style(t, pstyle);
  }
  if (flags->do_redraw_decoration)
  {
    FvwmWindow *u = Scr.Hilite;

    if (IS_SHADED(t))
      XRaiseWindow(dpy, t->decor_w);
    if (IsRectangleOnThisPage(&t->frame_g, Scr.CurrentDesk))
      DrawDecorations(t, DRAW_ALL, (Scr.Hilite == t), 2, None);
    Scr.Hilite = u;
  }
  if (flags->do_update_icon_boxes)
  {
    change_icon_boxes(t, pstyle);
  }
  if (flags->do_update_icon)
  {
    change_icon(t, pstyle);
    if (IS_ICONIFIED(t))
    {
      SET_ICONIFIED(t, 0);
      Iconify(t, 0, 0);
    }
  }
  if (flags->do_setup_focus_policy)
  {
    setup_focus_policy(t);
  }
  if (flags->do_update_frame_attributes)
  {
    setup_frame_attributes(t, pstyle);
  }

  return;
}

/* Check and apply new style to each window if the style has changed. */
void update_windows(void)
{
  FvwmWindow *t;
  window_style style;
  Window focus_w;
  FvwmWindow *focus_fw;
  Bool do_need_ungrab = False;
  update_win flags;

  memset(&flags, 0, sizeof(update_win));
  /* Grab the server during the style update! */
  MyXGrabServer(dpy);
  if (GrabEm(CRS_WAIT, GRAB_BUSY))
    do_need_ungrab = True;
  XSync(dpy,0);

  /* This is necessary in case the focus policy changes. With ClickToFocus some
   * buttons have to be grabbed/ungrabbed. */
  focus_fw = Scr.Focus;
  focus_w = (focus_fw) ? focus_fw->w : Scr.NoFocusWin;
  SetFocus(Scr.NoFocusWin, NULL, 1);

  if (Scr.flags.has_default_font_changed || Scr.flags.has_default_color_changed)
    ApplyDefaultFontAndColors();

  /* update styles for all windows */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    check_window_style_change(t, &flags, &style);
    flags.do_update_window_font =
      (Scr.flags.has_window_font) ?
      t->decor->flags.has_font_changed : Scr.flags.has_default_font_changed;
    flags.do_update_icon_font =
      (Scr.flags.has_icon_font) ?
      Scr.flags.has_icon_font_changed : Scr.flags.has_default_font_changed;
    /*!!!this is not optimised for minimal redrawing yet*/
    if (t->decor->flags.has_changed)
    {
      flags.do_redecorate = 1;
    }
    if (t->decor->flags.has_font_changed)
    {
    }
    /* now apply the changes */
    apply_window_updates(t, &flags, &style, focus_fw);
  }

  if (Scr.flags.has_default_font_changed || Scr.flags.has_default_color_changed)
    ApplyDefaultFontAndColors();

  /* restore the focus; also handles the case that the previously focused
   * window is now NeverFocus */
  SetFocus(focus_w, focus_fw, 1);
  /* finally clean up the change flags */
  reset_style_changes();
  reset_decor_changes();
  Scr.flags.do_need_window_update = 0;
  Scr.flags.has_default_font_changed = 0;
  Scr.flags.has_default_color_changed = 0;

  if (do_need_ungrab)
    UngrabEm(GRAB_BUSY);
  MyXUngrabServer(dpy);
  XSync(dpy, 0);

  return;
}
