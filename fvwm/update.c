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

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
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
#include "module_interface.h"
#include "focus.h"

static void init_style(
  FvwmWindow *old_t, FvwmWindow *t, window_style *pstyle, short *pbuttons)
{
  /* copy the window structure because we still need some old values. */
  memcpy(old_t, t, sizeof(FvwmWindow));
  /* determine level of decoration */
  setup_style_and_decor(t, pstyle, pbuttons);
}

static void apply_window_updates(
  FvwmWindow *t, update_win *flags, window_style *pstyle, FvwmWindow *focus_w)
{
  FvwmWindow old_t;
  short buttons;
  Bool is_style_initialised = False;

  t->title_top_height = (HAS_BOTTOM_TITLE(t)) ? 0 : t->title_g.height;
  if (flags->do_update_window_grabs)
  {
    focus_grab_buttons(t, False);
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
   * These are a bit complicated because they can move windows to a different
   * page or desk. */
  if (flags->do_update_stick_icon && IS_ICONIFIED(t) && !IS_STICKY(t))
  {
    if (IS_ICON_STICKY(pstyle))
    {
      /* stick and unstick the window to force the icon on the current page */
      handle_stick(&Event, t->frame, t, C_FRAME, "", 0, 1);
      handle_stick(&Event, t->frame, t, C_FRAME, "", 0, 0);
    }
    else
    {
      flags->do_update_icon_title = True;
    }
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

  if (flags->do_update_window_font || flags->do_update_window_font_height)
  {
    if (!is_style_initialised)
    {
      init_style(&old_t, t, pstyle, &buttons);
      is_style_initialised = True;
    }
    setup_window_font(t, pstyle, flags->do_update_window_font);
    flags->do_redecorate = True;
  }
  if (flags->do_redecorate ||
      (IS_TRANSIENT(t) && flags->do_redecorate_transient))
  {
    rectangle naked_g;
    rectangle *new_g;

    if (!is_style_initialised)
    {
      init_style(&old_t, t, pstyle, &buttons);
      is_style_initialised = True;
    }

    /* redecorate */
    change_auxiliary_windows(t, buttons);

    /* calculate the new offsets */
    gravity_get_naked_geometry(
      old_t.hints.win_gravity, &old_t, &naked_g, &t->normal_g);
    gravity_translate_to_northwest_geometry_no_bw(
      old_t.hints.win_gravity, &old_t, &naked_g, &naked_g);
    gravity_add_decoration(
      old_t.hints.win_gravity, t, &t->normal_g, &naked_g);
    if (IS_MAXIMIZED(t))
    {
      int off_x = old_t.normal_g.x - old_t.max_g.x;
      int off_y = old_t.normal_g.y - old_t.max_g.y;
      int new_off_x;
      int new_off_y;

      /* maximized windows are always considered to have NorthWestGravity */
      gravity_get_naked_geometry(
	NorthWestGravity, &old_t, &naked_g, &t->max_g);
      gravity_translate_to_northwest_geometry_no_bw(
	NorthWestGravity, &old_t, &naked_g, &naked_g);
      gravity_add_decoration(
	NorthWestGravity, t, &t->max_g, &naked_g);
      new_g = &t->max_g;
      /* prevent random paging when unmaximizing after e.g. the border width has
       * changed */
      new_off_x = t->normal_g.x - t->max_g.x;
      new_off_y = t->normal_g.y - t->max_g.y;
      t->max_offset.x += new_off_x - off_x;
      t->max_offset.y += new_off_y - off_y;
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
    rectangle old_g;

    setup_frame_size_limits(t, pstyle);
    old_g = t->frame_g;
    t->frame_g = t->normal_g;
    gravity_constrain_size(t->hints.win_gravity, t, &(t->frame_g));
    t->normal_g = t->frame_g;
    if (IS_MAXIMIZED(t))
    {
      t->frame_g = t->max_g;
      gravity_constrain_size(t->hints.win_gravity, t, &(t->frame_g));
      t->max_g = t->frame_g;
    }
    t->frame_g = old_g;
    gravity_constrain_size(t->hints.win_gravity, t, &(t->frame_g));

    flags->do_setup_frame = True;
    flags->do_redraw_decoration = True;
  }
  if (flags->do_setup_frame)
  {
    if (IS_SHADED(t))
    {
      get_unshaded_geometry(t, &t->frame_g);
      get_shaded_geometry(t, &t->frame_g, &t->frame_g);
    }
    ForceSetupFrame(t, t->frame_g.x, t->frame_g.y, t->frame_g.width,
		    t->frame_g.height, True);
    GNOME_SetWinArea(t);
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
    /* domivogt (6-Jun-2000): Don't check if the window is visible here.  If we
     * do, some updates are not applied and when the window becomes visible
     * again, the X Server may not redraw the window. */
    DrawDecorations(t, DRAW_ALL, (Scr.Hilite == t), 2, None);
    Scr.Hilite = u;
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
    change_icon(t, pstyle);
    if (IS_ICONIFIED(t))
    {
      SET_ICONIFIED(t, 0);
      Iconify(t, 0, 0);
    }
    flags->do_update_icon_title = False;
  }
  if (flags->do_update_icon_title)
  {
    RedoIconName(t);
    if (IS_ICONIFIED(t))
      DrawIconWindow(t);
  }
  if (flags->do_setup_focus_policy)
  {
    setup_focus_policy(t);
  }
  if (flags->do_update_frame_attributes)
  {
    setup_frame_attributes(t, pstyle);
  }
  if (flags->do_update_modules_flags)
  {
    BroadcastConfig(M_CONFIGURE_WINDOW,t);
  }
  t->shade_anim_steps = pstyle->shade_anim_steps;

  return;
}

/* similar, but takes only care of destroying windows that have to go away. */
void destroy_scheduled_windows(void)
{
  FvwmWindow *t;
  FvwmWindow *next;
  Bool do_need_ungrab = False;

  if (Scr.flags.is_executing_complex_function != 0 ||
      Scr.flags.is_window_scheduled_for_destroy == 0)
    return;
  /* Grab the server during the style update! */
  if (GrabEm(CRS_WAIT, GRAB_BUSY))
    do_need_ungrab = True;
  MyXGrabServer(dpy);
  Scr.flags.is_window_scheduled_for_destroy = 0;
  /* need to destroy one or more windows before looking at the window list */
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
    UngrabEm(GRAB_BUSY);

  return;
}

/* similar to the flush_window_updates() function, but does only the updates
 * for a single window whose decor has been changed. */
void apply_decor_change(FvwmWindow *tmp_win)
{
  window_style style;
  update_win flags;

  lookup_style(tmp_win, &style);
  memset(&flags, 0, sizeof(flags));
  flags.do_redecorate = True;
  flags.do_update_window_font_height = True;
  apply_window_updates(tmp_win, &flags, &style, get_current_focus_window());
  Scr.flags.do_need_window_update = 1;
}

/* Check and apply new style to each window if the style has changed. */
void flush_window_updates(void)
{
  FvwmWindow *t;
  window_style style;
  FvwmWindow *focus_fw;
  Bool do_need_ungrab = False;
  update_win flags;

  memset(&flags, 0, sizeof(update_win));
  /* Grab the server during the style update! */
  if (GrabEm(CRS_WAIT, GRAB_BUSY))
    do_need_ungrab = True;
  MyXGrabServer(dpy);

  /* This is necessary in case the focus policy changes. With ClickToFocus some
   * buttons have to be grabbed/ungrabbed. */
  focus_fw = get_current_focus_window();
  DeleteFocus(1);

  /* Apply the new default font and colours first */
  if (Scr.flags.has_default_color_changed || Scr.flags.has_default_font_changed)
    ApplyDefaultFontAndColors();

  /* update styles for all windows */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    check_window_style_change(t, &flags, &style);
    if (Scr.flags.has_nr_buttons_changed)
      flags.do_redecorate = True;
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
    SetFocusWindow(focus_fw, 1);
    if (Scr.flags.has_mouse_binding_changed)
    {
      focus_grab_buttons(focus_fw, True);
    }
  }
  else
    DeleteFocus(1);

  /* finally clean up the change flags */
  reset_style_changes();
  reset_decor_changes();
  Scr.flags.do_need_window_update = 0;
  Scr.flags.has_default_font_changed = 0;
  Scr.flags.has_default_color_changed = 0;
  Scr.flags.has_mouse_binding_changed = 0;
  Scr.flags.has_nr_buttons_changed = 0;

  MyXUngrabServer(dpy);
  if (do_need_ungrab)
    UngrabEm(GRAB_BUSY);

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
