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

/* access to the common flags of a window */
#define DO_GRAB_FOCUS(fw) \
          ((fw)->flags.common.do_grab_focus_when_created)
#define DO_GRAB_FOCUS_TRANSIENT(fw) \
          ((fw)->flags.common.do_grab_focus_when_transient_created)
#define DO_SKIP_CIRCULATE(fw)  \
          ((fw)->flags.common.do_circulate_skip)
#define SET_DO_SKIP_CIRCULATE(fw,x) \
          (fw)->flags.common.do_circulate_skip = !!(x)
#define SETM_DO_SKIP_CIRCULATE(fw,x) \
          (fw)->flag_mask.common.do_circulate_skip = !!(x)
#define DO_SKIP_ICON_CIRCULATE(fw) \
          ((fw)->flags.common.do_circulate_skip_icon)
#define SET_DO_SKIP_ICON_CIRCULATE(fw,x) \
          (fw)->flags.common.do_circulate_skip_icon = !!(x)
#define SETM_DO_SKIP_ICON_CIRCULATE(fw,x) \
          (fw)->flag_mask.common.do_circulate_skip_icon = !!(x)
#define DO_NOT_SHOW_ON_MAP(fw)  \
          ((fw)->flags.common.do_not_show_on_map)
#define DO_SKIP_WINDOW_LIST(fw) \
          ((fw)->flags.common.do_window_list_skip)
#define SET_DO_SKIP_WINDOW_LIST(fw,x) \
          (fw)->flags.common.do_window_list_skip = !!(x)
#define SETM_DO_SKIP_WINDOW_LIST(fw,x) \
          (fw)->flag_mask.common.do_window_list_skip = !!(x)
#define DO_START_ICONIC(fw)    ((fw)->flags.common.do_start_iconic)
#define SET_DO_START_ICONIC(fw,x) \
          (fw)->flags.common.do_start_iconic = !!(x)
#define SETM_DO_START_ICONIC(fw,x) \
          (fw)->flag_mask.common.do_start_iconic = !!(x)
#define IS_ICON_STICKY(fw)     ((fw)->flags.common.is_icon_sticky)
#define SET_ICON_STICKY(fw,x)  (fw)->flags.common.is_icon_sticky = !!(x)
#define SETM_ICON_STICKY(fw,x) \
          (fw)->flag_mask.common.is_icon_sticky = !!(x)
#define IS_ICON_SUPPRESSED(fw) \
          ((fw)->flags.common.is_icon_suppressed)
#define SET_ICON_SUPPRESSED(fw,x)  \
          (fw)->flags.common.is_icon_suppressed = !!(x)
#define SETM_ICON_SUPPRESSED(fw,x) \
          (fw)->flag_mask.common.is_icon_suppressed = !!(x)
#define IS_LENIENT(fw)         ((fw)->flags.common.is_lenient)
#define SET_LENIENT(fw,x)      (fw)->flags.common.is_lenient = !!(x)
#define SETM_LENIENT(fw,x)     (fw)->flag_mask.common.is_lenient = !!(x)
#define IS_STICKY(fw)          ((fw)->flags.common.is_sticky)
#define SET_STICKY(fw,x)       (fw)->flags.common.is_sticky = !!(x)
#define SETM_STICKY(fw,x)      (fw)->flag_mask.common.is_sticky = !!(x)
#define GET_FOCUS_MODE(fw)     ((fw)->flags.common.focus_mode)
#define SET_FOCUS_MODE(fw,x)   (fw)->flags.common.focus_mode = (x)
#define SETM_FOCUS_MODE(fw,x)  (fw)->flag_mask.common.focus_mode =(x)
#define HAS_CLICK_FOCUS(fw)    \
          ((fw)->flags.common.focus_mode == FOCUS_CLICK)
#define HAS_MOUSE_FOCUS(fw)    \
          ((fw)->flags.common.focus_mode == FOCUS_MOUSE)
#define HAS_SLOPPY_FOCUS(fw)   \
          ((fw)->flags.common.focus_mode == FOCUS_SLOPPY)
#define HAS_NEVER_FOCUS(fw)   \
          ((fw)->flags.common.focus_mode == FOCUS_NEVER)
#define HAS_NO_ICON_TITLE(fw)  \
          ((fw)->flags.common.has_no_icon_title)
#define SET_HAS_NO_ICON_TITLE(fw,x) \
          (fw)->flags.common.has_no_icon_title = !!(x)
#define SETM_HAS_NO_ICON_TITLE(fw,x) \
          (fw)->flag_mask.common.has_no_icon_title = !!(x)
#define HAS_MWM_BORDER(fw)     ((fw)->flags.common.has_mwm_border)
#define HAS_MWM_BUTTONS(fw)    ((fw)->flags.common.has_mwm_buttons)
#define HAS_MWM_OVERRIDE_HINTS(fw)  \
                               ((fw)->flags.common.has_mwm_override)
#define HAS_OVERRIDE_SIZE_HINTS(fw)  \
                               ((fw)->flags.common.has_override_size)
#define IGNORE_RESTACK(fw)  \
                               ((fw)->flags.common.ignore_restack)


/* access to the special flags of a window */
#define DO_REUSE_DESTROYED(fw) ((fw)->flags.do_reuse_destroyed)
#define SET_DO_REUSE_DESTROYED(fw,x) \
          (fw)->flags.do_reuse_destroyed = !!(x)
#define SETM_DO_REUSE_DESTROYED(fw,x) \
          (fw)->flag_mask.do_reuse_destroyed = !!(x)
#define HAS_BORDER(fw)         ((fw)->flags.has_border)
#define SET_HAS_BORDER(fw,x)   (fw)->flags.has_border = !!(x)
#define SETM_HAS_BORDER(fw,x)  (fw)->flag_mask.has_border = !!(x)
#define HAS_TITLE(fw)          ((fw)->flags.has_title)
#define SET_HAS_TITLE(fw,x)    (fw)->flags.has_title = !!(x)
#define SETM_HAS_TITLE(fw,x)   (fw)->flag_mask.has_title = !!(x)
#define IS_MAPPED(fw)          ((fw)->flags.is_mapped)
#define SET_MAPPED(fw,x)       (fw)->flags.is_mapped = !!(x)
#define SETM_MAPPED(fw,x)      (fw)->flag_mask.is_mapped = !!(x)
#define IS_ICONIFIED(fw)       ((fw)->flags.is_iconified)
#define SET_ICONIFIED(fw,x)    (fw)->flags.is_iconified = !!(x)
#define SETM_ICONIFIED(fw,x)   (fw)->flag_mask.is_iconified = !!(x)
#define IS_ICONIFIED_BY_PARENT(fw) \
                               ((fw)->flags.is_iconified_by_parent)
#define SET_ICONIFIED_BY_PARENT(fw,x) \
          (fw)->flags.is_iconified_by_parent = !!(x)
#define SETM_ICONIFIED_BY_PARENT(fw,x) \
          (fw)->flag_mask.is_iconified_by_parent = !!(x)
#define IS_ICON_ENTERED(fw)     ((fw)->flags.is_icon_entered)
#define SET_ICON_ENTERED(fw,x)  (fw)->flags.is_icon_entered = !!(x)
#define SETM_ICON_ENTERED(fw,x) (fw)->flag_mask.is_icon_entered = !!(x)
#define IS_ICON_OURS(fw)       ((fw)->flags.is_icon_ours)
#define SET_ICON_OURS(fw,x)    (fw)->flags.is_icon_ours = !!(x)
#define SETM_ICON_OURS(fw,x)   (fw)->flag_mask.is_icon_ours = !!(x)
#define IS_ICON_SHAPED(fw)     ((fw)->flags.is_icon_shaped)
#define SET_ICON_SHAPED(fw,x)  (fw)->flags.is_icon_shaped = !!(x)
#define SETM_ICON_SHAPED(fw,x) (fw)->flag_mask.is_icon_shaped = !!(x)
#define IS_ICON_MOVED(fw)      ((fw)->flags.is_icon_moved)
#define SET_ICON_MOVED(fw,x)   (fw)->flags.is_icon_moved = !!(x)
#define SETM_ICON_MOVED(fw,x)  (fw)->flag_mask.is_icon_moved = !!(x)
#define IS_ICON_UNMAPPED(fw)   ((fw)->flags.is_icon_unmapped)
#define SET_ICON_UNMAPPED(fw,x) \
                               (fw)->flags.is_icon_unmapped = !!(x)
#define SETM_ICON_UNMAPPED(fw,x) \
                               (fw)->flag_mask.is_icon_unmapped = !!(x)
#define IS_MAP_PENDING(fw)     ((fw)->flags.is_map_pending)
#define SET_MAP_PENDING(fw,x)  (fw)->flags.is_map_pending = !!(x)
#define SETM_MAP_PENDING(fw,x) (fw)->flag_mask.is_map_pending = !!(x)
#define IS_MAXIMIZED(fw)       ((fw)->flags.is_maximized)
#define SET_MAXIMIZED(fw,x)    (fw)->flags.is_maximized = !!(x)
#define SETM_MAXIMIZED(fw,x)   (fw)->flag_mask.is_maximized = !!(x)
#define IS_NAME_CHANGED(fw)    ((fw)->flags.is_name_changed)
#define SET_NAME_CHANGED(fw,x) (fw)->flags.is_name_changed = !!(x)
#define SETM_NAME_CHANGED(fw,x) (fw)->flag_mask.is_name_changed = !!(x)
#define IS_PIXMAP_OURS(fw)     ((fw)->flags.is_pixmap_ours)
#define SET_PIXMAP_OURS(fw,x)  (fw)->flags.is_pixmap_ours = !!(x)
#define SETM_PIXMAP_OURS(fw,x) (fw)->flag_mask.is_pixmap_ours = !!(x)
#define IS_SHADED(fw)          ((fw)->flags.is_window_shaded)
#define SET_SHADED(fw,x)       (fw)->flags.is_window_shaded = !!(x)
#define SETM_SHADED(fw,x)      (fw)->flag_mask.is_window_shaded = !!(x)
#define IS_TRANSIENT(fw)       ((fw)->flags.is_transient)
#define SET_TRANSIENT(fw,x)    (fw)->flags.is_transient = !!(x)
#define SETM_TRANSIENT(fw,x)   (fw)->flag_mask.is_transient = !!(x)
#define IS_VIEWPORT_MOVED(fw)  ((fw)->flags.is_viewport_moved)
#define SET_VIEWPORT_MOVED(fw,x) \
          (fw)->flags.is_viewport_moved = !!(x)
#define SETM_VIEWPORT_MOVED(fw,x) \
          (fw)->flag_mask.is_viewport_moved = !!(x)
#define IS_VIEWPORT_MOVED(fw)  ((fw)->flags.is_viewport_moved)
#define IS_VISIBLE(fw)         ((fw)->flags.is_visible)
#define SET_VISIBLE(fw,x)      (fw)->flags.is_visible = !!(x)
#define SETM_VISIBLE(fw,x)     (fw)->flag_mask.is_visible = !!(x)
#define IS_WINDOW_BEING_MOVED_OPAQUE(fw) \
                               ((fw)->flags.is_window_being_moved_opaque)
#define SET_WINDOW_BEING_MOVED_OPAQUE(fw,x) \
          (fw)->flags.is_window_being_moved_opaque = !!(x)
#define SETM_WINDOW_BEING_MOVED_OPAQUE(fw,x) \
          (fw)->flag_mask.is_window_being_moved_opaque = !!(x)
#define WM_DELETES_WINDOW(fw)   ((fw)->flags.does_wm_delete_window)
#define SET_WM_DELETES_WINDOW(fw,x) \
          (fw)->flags.does_wm_delete_window = !!(x)
#define SETM_WM_DELETES_WINDOW(fw,x) \
          (fw)->flag_mask.does_wm_delete_window = !!(x)
#define WM_TAKES_FOCUS(fw)     ((fw)->flags.does_wm_take_focus)
#define SET_WM_TAKES_FOCUS(fw,x) \
          (fw)->flags.does_wm_take_focus = !!(x)
#define SETM_WM_TAKES_FOCUS(fw,x) \
          (fw)->flag_mask.does_wm_take_focus = !!(x)



#endif /* _GSFR_ */
