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

#ifndef _GSFR_
#define _GSFR_

#include "fvwm.h"

typedef struct
{
  /* common flags (former flags in bits 0-12) */
  unsigned circulate_skip : 1; /* was circulateskip */
  unsigned circulate_skip_icon : 1;
#define FOCUS_MOUSE   0x0
#define FOCUS_CLICK   0x1
#define FOCUS_SLOPPY  0x2
  unsigned focus_mode : 2; /* was click_focus/sloppy_focus */
  unsigned lenience : 1;
  unsigned has_no_icon_title : 1; /* was noicon_title */
  unsigned show_on_map : 1; /* was show_mapping */
  unsigned start_iconic : 1;
  unsigned is_sticky : 1; /* was sticky */
  unsigned is_icon_sticky : 1; /* was sticky_icon */
  unsigned suppress_icon : 1; /* was suppressicon */
  unsigned window_list_skip : 1; /* was listskip */
} common_flags_type;

typedef struct
{
  common_flags_type common_flags;
  unsigned does_wm_delete_window : 1; /* DoesWmDeleteWindow */
  unsigned does_wm_take_focus : 1; /* DoesWmTakeFocus */
  unsigned has_border : 1; /* BORDER */ /* Is this decorated with border*/
  unsigned has_mwm_borders : 1; /* MWMBorders */
  unsigned has_mwm_buttons : 1; /* MWMButtons */
  unsigned has_title : 1; /* TITLE */ /* Is this decorated with title */
  unsigned hint_override : 1; /* HintOverride */ /**/
  unsigned is_mapped : 1; /* MAPPED */ /* is it mapped? */
  unsigned is_iconified : 1; /* ICONIFIED */ /* is it an icon now? */
  unsigned is_iconified_by_parent : 1; /* To prevent iconified transients in a parent
					* icon from counting for Next */
  unsigned is_icon_ours : 1; /* ICON_OURS */ /* is the icon window supplied by the app? */
  unsigned is_icon_shaped : 1; /* SHAPED_ICON */ /* is the icon shaped? */
  unsigned is_icon_moved : 1; /* ICON_MOVED */ /* has the icon been moved by the user? */
  unsigned is_icon_unmapped : 1; /* ICON_UNMAPPED */ /* was the icon unmapped, even though the window is still iconified (Transients) */
  unsigned is_map_pending : 1; /* MAP_PENDING */ /* Sent an XMapWindow, but didn't receive a MapNotify yet.*/
  unsigned is_maximized : 1; /* MAXIMIZED */ /* is the window maximized? */
  unsigned is_name_changed : 1; /* Set if the client changes its WM_NAME.
				 * The source of twm contains an explanation
				 * why we need this information. */
  unsigned is_pixmap_ours : 1; /* PIXMAP_OURS */ /* is the icon pixmap ours to free? */
  unsigned is_transient : 1; /* TRANSIENT */ /* is it a transient window? */
  unsigned is_viewport_moved : 1; /* To prevent double move in MoveViewport.*/
  unsigned is_visible : 1; /* VISIBLE */ /* is the window fully visible */
  unsigned is_window_being_moved_opaque : 1;
  unsigned is_window_shaded : 1;
  unsigned reuse_destroyed : 1;     /* Reuse this struct, don't free it,
				     * when destroying/recapturing window. */
} window_flags;


/* access to the common flags of a window */
#define DO_SKIP_CIRCULATE(fw)  ((fw)->gsfr_flags.common_flags.circulate_skip)
#define DO_SKIP_CIRCULATE_ICON(fw) \
          ((fw)->gsfr_flags.common_flags.circulate_skip_icon)
#define DO_SHOW_ON_MAP(fw)     ((fw)->gsfr_flags.common_flags.show_on_map)
#define DO_SKIP_WINDOWLIST(fw) ((fw)->gsfr_flags.common_flags.window_list_skip)
#define DO_START_ICONIC(fw)    ((fw)->gsfr_flags.common_flags.start_iconic)
#define IS_ICON_STICKY(fw)     ((fw)->gsfr_flags.common_flags.is_icon_sticky)
#define IS_ICON_SUPPRESSED(fw) ((fw)->gsfr_flags.common_flags.suppress_icon)
#define IS_LENIENT(fw)         ((fw)->gsfr_flags.common_flags.lenience)
#define IS_STICKY(fw)          ((fw)->gsfr_flags.common_flags.is_sticky)
#define HAS_CLICK_FOCUS(fw)    \
          ((fw)->gsfr_flags.common_flags.focus_mode == FOCUS_CLICK)
#define HAS_MOUSE_FOCUS(fw)    \
          ((fw)->gsfr_flags.common_flags.focus_mode == FOCUS_MOUSE)
#define HAS_SLOPPY_FOCUS(fw)   \
          ((fw)->gsfr_flags.common_flags.focus_mode == FOCUS_SLOPPY)
#define HAS_NO_ICON_TITLE(fw)  \
          ((fw)->gsfr_flags.common_flags.has_no_icon_title)

/* access to the special flags of a window */
#define DO_OVERRIDE_HINTS(fw)  ((fw)->gsfr_flags.hint_override)
#define DO_REUSE_DESTROYED(fw) ((fw)->gsfr_flags.reuse_destroyed)
#define HAS_BORDER(fw)         ((fw)->gsfr_flags.has_border)
#define HAS_MWM_BORDER(fw)     ((fw)->gsfr_flags.has_mwm_borders)
#define HAS_MWM_BUTTONS(fw)    ((fw)->gsfr_flags.has_mwm_buttons)
#define HAS_TITLE(fw)          ((fw)->gsfr_flags.has_title)
#define IS_MAPPED(fw)          ((fw)->gsfr_flags.is_mapped)
#define IS_ICONIFIED(fw)       ((fw)->gsfr_flags.is_iconified)
#define IS_ICONIFIED_BY_PARENT(fw) \
                               ((fw)->gsfr_flags.is_iconified_by_parent)
#define IS_ICON_OURS(fw)       ((fw)->gsfr_flags.is_icon_ours)
#define IS_ICON_SHAPED(fw)     ((fw)->gsfr_flags.is_icon_shaped)
#define IS_ICON_MOVED(fw)      ((fw)->gsfr_flags.is_icon_moved)
#define IS_ICON_UNMAPPED(fw)   ((fw)->gsfr_flags.is_icon_unmapped)
#define IS_MAP_PENDING(fw)     ((fw)->gsfr_flags.is_map_pending)
#define IS_MAXIMIZED(fw)       ((fw)->gsfr_flags.is_maximized)
#define IS_NAME_CHANGED(fw)    ((fw)->gsfr_flags.is_name_changed)
#define IS_PIXMAP_OURS(fw)     ((fw)->gsfr_flags.is_pixmap_ours)
#define IS_TRANSIENT(fw)       ((fw)->gsfr_flags.is_transient)
#define IS_VIEWPORT_MOVED(fw)  ((fw)->gsfr_flags.is_viewport_moved)
#define IS_VISIBLE(fw)         ((fw)->gsfr_flags.is_visible)
#define IS_WINDOW_BEING_MOVED_OPAQUE(fw) \
                               ((fw)->gsfr_flags.is_window_being_moved_opaque)
#define IS_WINDOW_SHADED(fw)   ((fw)->gsfr_flags.is_window_shaded)
#define WM_DELETES_FOCUS(fw)   ((fw)->gsfr_flags.does_wm_delete_window)
#define WM_TAKES_FOCUS(fw)     ((fw)->gsfr_flags.does_wm_take_focus)



#endif /* _GSFR_ */
