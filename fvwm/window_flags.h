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

#ifndef _WINDOW_FLAGS_
#define _WINDOW_FLAGS_


/* access to the common flags of a window */
#define FW_COMMON_FLAGS(fw)         ((fw)->flags.common)
#define FW_COMMON_STATIC_FLAGS(fw)  ((fw)->flags.common.s)

#define DO_GRAB_FOCUS(fw) \
          ((fw)->flags.common.s.do_grab_focus_when_created)
#define DO_GRAB_FOCUS_TRANSIENT(fw) \
          ((fw)->flags.common.s.do_grab_focus_when_transient_created)
#define DO_LOWER_TRANSIENT(fw) ((fw)->flags.common.s.do_lower_transient)
#define DO_NOT_SHOW_ON_MAP(fw)  \
          ((fw)->flags.common.s.do_not_show_on_map)
#define DO_NOT_PASS_CLICK_FOCUS_CLICK(fw)  \
          ((fw)->flags.common.s.do_not_pass_click_focus_click)
#define SET_DO_NOT_PASS_CLICK_FOCUS_CLICK(fw,x) \
          (fw)->flags.common.s.do_not_pass_click_focus_click = !!(x)
#define SETM_DO_NOT_PASS_CLICK_FOCUS_CLICK(fw,x) \
          (fw)->flag_mask.common.s.do_not_pass_click_focus_click = !!(x)
#define DO_NOT_RAISE_CLICK_FOCUS_CLICK(fw)  \
          ((fw)->flags.common.s.do_not_raise_click_focus_click)
#define SET_DO_NOT_RAISE_CLICK_FOCUS_CLICK(fw,x) \
          (fw)->flags.common.s.do_not_raise_click_focus_click = !!(x)
#define SETM_DO_NOT_RAISE_CLICK_FOCUS_CLICK(fw,x) \
          (fw)->flag_mask.common.s.do_not_raise_click_focus_click = !!(x)
#define DO_RAISE_MOUSE_FOCUS_CLICK(fw)  \
          ((fw)->flags.common.s.do_raise_mouse_focus_click)
#define SET_DO_RAISE_MOUSE_FOCUS_CLICK(fw,x) \
          (fw)->flags.common.s.do_raise_mouse_focus_click = !!(x)
#define SETM_DO_RAISE_MOUSE_FOCUS_CLICK(fw,x) \
          (fw)->flag_mask.common.s.do_raise_mouse_focus_click = !!(x)
#define DO_RAISE_TRANSIENT(fw) ((fw)->flags.common.s.do_raise_transient)
#define DO_RESIZE_OPAQUE(fw)   ((fw)->flags.common.s.do_resize_opaque)
#define DO_SHRINK_WINDOWSHADE(fw)  \
          ((fw)->flags.common.s.do_shrink_windowshade)
#define SET_DO_SHRINK_WINDOWSHADE(fw,x) \
          (fw)->flags.common.s.do_shrink_windowshade = !!(x)
#define SETM_DO_SHRINK_WINDOWSHADE(fw,x) \
          (fw)->flag_mask.common.s.do_shrink_windowshade = !!(x)
#define DO_SKIP_CIRCULATE(fw)  \
          ((fw)->flags.common.s.do_circulate_skip)
#define SET_DO_SKIP_CIRCULATE(fw,x) \
          (fw)->flags.common.s.do_circulate_skip = !!(x)
#define SETM_DO_SKIP_CIRCULATE(fw,x) \
          (fw)->flag_mask.common.s.do_circulate_skip = !!(x)
#define DO_SKIP_ICON_CIRCULATE(fw) \
          ((fw)->flags.common.s.do_circulate_skip_icon)
#define SET_DO_SKIP_ICON_CIRCULATE(fw,x) \
          (fw)->flags.common.s.do_circulate_skip_icon = !!(x)
#define SETM_DO_SKIP_ICON_CIRCULATE(fw,x) \
          (fw)->flag_mask.common.s.do_circulate_skip_icon = !!(x)
#define DO_SKIP_SHADED_CIRCULATE(fw) \
          ((fw)->flags.common.s.do_circulate_skip_shaded)
#define SET_DO_SKIP_SHADED_CIRCULATE(fw,x) \
          (fw)->flags.common.s.do_circulate_skip_shaded = !!(x)
#define SETM_DO_SKIP_SHADED_CIRCULATE(fw,x) \
          (fw)->flag_mask.common.s.do_circulate_skip_shaded = !!(x)
#define DO_SKIP_WINDOW_LIST(fw) \
          ((fw)->flags.common.s.do_window_list_skip)
#define SET_DO_SKIP_WINDOW_LIST(fw,x) \
          (fw)->flags.common.s.do_window_list_skip = !!(x)
#define SETM_DO_SKIP_WINDOW_LIST(fw,x) \
          (fw)->flag_mask.common.s.do_window_list_skip = !!(x)
#define DO_STACK_TRANSIENT_PARENT(fw) \
          ((fw)->flags.common.s.do_stack_transient_parent)
#define DO_START_ICONIC(fw)    ((fw)->flags.common.s.do_start_iconic)
#define SET_DO_START_ICONIC(fw,x) \
          (fw)->flags.common.s.do_start_iconic = !!(x)
#define SETM_DO_START_ICONIC(fw,x) \
          (fw)->flag_mask.common.s.do_start_iconic = !!(x)

#define HAS_BOTTOM_TITLE(fw)   ((fw)->flags.common.s.has_bottom_title)
#define SET_HAS_BOTTOM_TITLE(fw,x) \
          (fw)->flags.common.s.has_bottom_title = !!(x)
#define SETM_HAS_BOTTOM_TITLE(fw,x) \
          (fw)->flag_mask.common.s.has_bottom_title = !!(x)
#define HAS_STIPPLED_TITLE(fw)  \
          ((fw)->flags.common.s.has_stippled_title)
#define SET_HAS_STIPPLED_TITLE(fw,x) \
          (fw)->flags.common.s.has_stippled_title = !!(x)
#define SETM_HAS_STIPPLED_TITLE(fw,x) \
          (fw)->flag_mask.common.s.has_stippled_title = !!(x)
#define ICON_OVERRIDE_MODE(fw)  \
          ((fw)->flags.common.s.icon_override)
#define SET_ICON_OVERRIDE_MODE(fw,x)  \
          (fw)->flags.common.s.icon_override = !!(x)
#define SETM_ICON_OVERRIDE_MODE(fw,x) \
          (fw)->flag_mask.common.s.icon_override = !!(x)
#define IS_ICON_STICKY(fw)     ((fw)->flags.common.s.is_icon_sticky)
#define SET_ICON_STICKY(fw,x)  (fw)->flags.common.s.is_icon_sticky = !!(x)
#define SETM_ICON_STICKY(fw,x) \
          (fw)->flag_mask.common.s.is_icon_sticky = !!(x)
#define USE_ICON_POSITION_HINT(fw) \
          ((fw)->flags.common.s.use_icon_position_hint)
#define SET_USE_ICON_POSITION_HINT(fw,x) \
          (fw)->flags.common.s.use_icon_position_hint = !!(x)
#define SETM_USE_ICON_POSITION_HINT(fw,x) \
          (fw)->flag_mask.common.s.use_icon_position_hint = !!(x)
#define USE_INDEXED_WINDOW_NAME(fw) \
          ((fw)->flags.common.s.use_indexed_window_name)
#define SET_USE_INDEXED_WINDOW_NAME(fw,x) \
          (fw)->flags.common.s.use_indexed_window_name = !!(x)
#define SETM_USE_INDEXED_WINDOW_NAME(fw,x) \
          (fw)->flag_mask.common.s.use_indexed_window_name = !!(x)
#define USE_INDEXED_ICON_NAME(fw) \
          ((fw)->flags.common.s.use_indexed_icon_name)
#define SET_USE_INDEXED_ICON_NAME(fw,x) \
          (fw)->flags.common.s.use_indexed_icon_name = !!(x)
#define SETM_USE_INDEXED_ICON_NAME(fw,x) \
          (fw)->flag_mask.common.s.use_indexed_icon_name = !!(x)
#define DO_EWMH_MINI_ICON_OVERRIDE(fw) \
          ((fw)->flags.common.s.do_ewmh_mini_icon_override)
#define SET_DO_EWMH_MINI_ICON_OVERRIDE(fw,x) \
          (fw)->flags.common.s.do_ewmh_mini_icon_override = !!(x)
#define SETM_DO_EWMH_MINI_ICON_OVERRIDE(fw,x) \
          (fw)->flag_mask.common.s.do_ewmh_mini_icon_override = !!(x)
#define DO_EWMH_DONATE_ICON(fw) \
          ((fw)->flags.common.s.do_ewmh_donate_icon)
#define SET_DO_EWMH_DONATE_ICON(fw,x) \
          (fw)->flags.common.s.do_ewmh_donate_icon = !!(x)
#define SETM_DO_EWMH_DONATE_ICON(fw,x) \
          (fw)->flag_mask.common.s.do_ewmh_donate_icon = !!(x)
#define DO_EWMH_DONATE_MINI_ICON(fw) \
          ((fw)->flags.common.s.do_ewmh_donate_mini_icon)
#define SET_DO_EWMH_DONATE_MINI_ICON(fw,x) \
          (fw)->flags.common.s.do_ewmh_donate_mini_icon = !!(x)
#define SETM_DO_EWMH_DONATE_MINI_ICON(fw,x) \
          (fw)->flag_mask.common.s.do_ewmh_donate_mini_icon = !!(x)
#define DO_EWMH_USE_STACKING_HINTS(fw) \
          ((fw)->flags.common.s.do_ewmh_use_stacking_hints)
#define SET_DO_EWMH_USE_STACKING_HINTS(fw,x) \
          (fw)->flags.common.s.do_ewmh_use_stacking_hints = !!(x)
#define SETM_DO_EWMH_USE_STACKING_HINTS(fw,x) \
          (fw)->flag_mask.common.s.do_ewmh_use_stacking_hints = !!(x)
#define DO_EWMH_IGNORE_STRUT_HINTS(fw) \
          ((fw)->flags.common.s.do_ewmh_ignore_strut_hints)
#define SET_DO_EWMH_IGNORE_STRUT_HINTS(fw,x) \
          (fw)->flags.common.s.do_ewmh_ignore_strut_hints = !!(x)
#define SETM_DO_EWMH_IGNORE_STRUT_HINTS(fw,x) \
          (fw)->flag_mask.common.s.do_ewmh_ignore_strut_hints = !!(x)
#define DO_EWMH_IGNORE_STATE_HINTS(fw) \
          ((fw)->flags.common.s.do_ewmh_ignore_state_hints)
#define SET_DO_EWMH_IGNORE_STATE_HINTS(fw,x) \
          (fw)->flags.common.s.do_ewmh_ignore_state_hints = !!(x)
#define SETM_DO_EWMH_IGNORE_STATE_HINTS(fw,x) \
          (fw)->flag_mask.common.s.do_ewmh_ignore_state_hints = !!(x)
#define EWMH_MAXIMIZE_MODE(fw) \
          ((fw)->flags.common.s.ewmh_maximize_mode)
#define SET_EWMH_MAXIMIZE_MODE(fw,x) \
          (fw)->flags.common.s.ewmh_maximize_mode = (x)
#define SETM_EWMH_MAXIMIZE_MODE(fw,x) \
          (fw)->flag_mask.common.s.ewmh_maximize_mode = (x)
#define IS_ICON_SUPPRESSED(fw) \
          ((fw)->flags.common.s.is_icon_suppressed)
#define SET_ICON_SUPPRESSED(fw,x)  \
          (fw)->flags.common.s.is_icon_suppressed = !!(x)
#define SETM_ICON_SUPPRESSED(fw,x) \
          (fw)->flag_mask.common.s.is_icon_suppressed = !!(x)
#define IS_LENIENT(fw)         ((fw)->flags.common.s.is_lenient)
#define SET_LENIENT(fw,x)      (fw)->flags.common.s.is_lenient = !!(x)
#define SETM_LENIENT(fw,x)     (fw)->flag_mask.common.s.is_lenient = !!(x)
#define IS_STICKY(fw)          ((fw)->flags.common.is_sticky)
#define SET_STICKY(fw,x)       (fw)->flags.common.is_sticky = !!(x)
#define SETM_STICKY(fw,x)      (fw)->flag_mask.common.is_sticky = !!(x)
#define GET_FOCUS_MODE(fw)     ((fw)->flags.common.s.focus_mode)
#define SET_FOCUS_MODE(fw,x)   (fw)->flags.common.s.focus_mode = (x)
#define SETM_FOCUS_MODE(fw,x)  (fw)->flag_mask.common.s.focus_mode =(x)
#define HAS_CLICK_FOCUS(fw)    \
          ((fw)->flags.common.s.focus_mode == FOCUS_CLICK)
#define HAS_MOUSE_FOCUS(fw)    \
          ((fw)->flags.common.s.focus_mode == FOCUS_MOUSE)
#define HAS_ICON_FONT(fw)  \
          ((fw)->flags.common.has_icon_font)
#define SET_HAS_ICON_FONT(fw,x) \
          (fw)->flags.common.has_icon_font = !!(x)
#define SETM_HAS_ICON_FONT(fw,x) \
          (fw)->flag_mask.common.has_icon_font = !!(x)
#define HAS_SLOPPY_FOCUS(fw)   \
          ((fw)->flags.common.s.focus_mode == FOCUS_SLOPPY)
#define HAS_NEVER_FOCUS(fw)   \
          ((fw)->flags.common.s.focus_mode == FOCUS_NEVER)
#define HAS_NO_ICON_TITLE(fw)  \
          ((fw)->flags.common.s.has_no_icon_title)
#define SET_HAS_NO_ICON_TITLE(fw,x) \
          (fw)->flags.common.s.has_no_icon_title = !!(x)
#define SETM_HAS_NO_ICON_TITLE(fw,x) \
          (fw)->flag_mask.common.s.has_no_icon_title = !!(x)
#define HAS_WINDOW_FONT(fw)  \
          ((fw)->flags.common.has_window_font)
#define SET_HAS_WINDOW_FONT(fw,x) \
          (fw)->flags.common.has_window_font = !!(x)
#define SETM_HAS_WINDOW_FONT(fw,x) \
          (fw)->flag_mask.common.has_window_font = !!(x)
#define HAS_MWM_BORDER(fw)     ((fw)->flags.common.s.has_mwm_border)
#define HAS_MWM_BUTTONS(fw)    ((fw)->flags.common.s.has_mwm_buttons)
#define HAS_MWM_OVERRIDE_HINTS(fw)  \
                               ((fw)->flags.common.s.has_mwm_override)
#define HAS_OVERRIDE_SIZE_HINTS(fw)  \
                               ((fw)->flags.common.s.has_override_size)
#define DO_ICONIFY_WINDOW_GROUPS(fw)  \
                               ((fw)->flags.common.s.do_iconify_window_groups)
#define DO_IGNORE_GNOME_HINTS(fw)  \
                               ((fw)->flags.common.s.do_ignore_gnome_hints)
#define DO_IGNORE_RESTACK(fw)  \
                               ((fw)->flags.common.s.do_ignore_restack)
#define DO_IGNORE_ICON_BOXES(fw)  \
                               ((fw)->flags.common.s.do_ignore_icon_boxes)
#define DO_USE_WINDOW_GROUP_HINT(fw)  \
                             ((fw)->flags.common.s.do_use_window_group_hint)
#define IS_FIXED(fw)           ((fw)->flags.common.s.is_fixed)
#define SET_FIXED(fw,x)        (fw)->flags.common.s.is_fixed = !!(x)
#define SETM_FIXED(fw,x)       (fw)->flag_mask.common.s.is_fixed = !!(x)
#define IS_FIXED_PPOS(fw)      ((fw)->flags.common.s.is_fixed_ppos)
#define SET_FIXED_PPOS(fw,x)   (fw)->flags.common.s.is_fixed_ppos = !!(x)
#define SETM_FIXED_PPOS(fw,x)  (fw)->flag_mask.common.s.is_fixed_ppos = !!(x)
#define IS_SIZE_FIXED(fw)      ((fw)->flags.common.s.is_size_fixed)
#define SET_SIZE_FIXED(fw,x)   (fw)->flags.common.s.is_size_fixed = !!(x)
#define SETM_SIZE_FIXED(fw,x)  (fw)->flag_mask.common.s.is_size_fixed = !!(x)
#define IS_PSIZE_FIXED(fw) ((fw)->flags.common.s.is_psize_fixed)
#define SET_PSIZE_FIXED(fw,x) \
                               (fw)->flags.common.s.is_psize_fixed = !!(x)
#define SETM_PSIZE_FIXED(fw,x) \
                            (fw)->flag_mask.common.s.is_psize_fixed = !!(x)
#define HAS_DEPRESSABLE_BORDER(fw) ((fw)->flags.common.s.has_depressable_border)


/* access to the special flags of a window */
#define DO_DELETE_ICON_MOVED(fw) \
                               ((fw)->flags.do_delete_icon_moved)
#define SET_DELETE_ICON_MOVED(fw,x) \
                               (fw)->flags.do_delete_icon_moved = !!(x)
#define SETM_DELETE_ICON_MOVED(fw,x) \
                               (fw)->flag_mask.do_delete_icon_moved = !!(x)
#define DO_REUSE_DESTROYED(fw) ((fw)->flags.do_reuse_destroyed)
#define SET_DO_REUSE_DESTROYED(fw,x) \
          (fw)->flags.do_reuse_destroyed = !!(x)
#define SETM_DO_REUSE_DESTROYED(fw,x) \
          (fw)->flag_mask.do_reuse_destroyed = !!(x)
#define HAS_BORDER(fw)         ((fw)->flags.has_border)
#define SET_HAS_BORDER(fw,x)   (fw)->flags.has_border = !!(x)
#define SETM_HAS_BORDER(fw,x)  (fw)->flag_mask.has_border = !!(x)
#define HAS_ICON_CHANGED(fw)         ((fw)->flags.has_icon_changed)
#define SET_HAS_ICON_CHANGED(fw,x)   (fw)->flags.has_icon_changed = !!(x)
#define SETM_HAS_ICON_CHANGED(fw,x)  (fw)->flag_mask.has_icon_changed = !!(x)
#define HAS_TITLE(fw)          ((fw)->flags.has_title)
#define SET_HAS_TITLE(fw,x)    (fw)->flags.has_title = !!(x)
#define SETM_HAS_TITLE(fw,x)   (fw)->flag_mask.has_title = !!(x)
#define IS_MAPPED(fw)          ((fw)->flags.is_mapped)
#define SET_MAPPED(fw,x)       (fw)->flags.is_mapped = !!(x)
#define SETM_MAPPED(fw,x)      (fw)->flag_mask.is_mapped = !!(x)
#define IS_DECOR_CHANGED(fw)     ((fw)->flags.is_decor_changed)
#define SET_DECOR_CHANGED(fw,x)  (fw)->flags.is_decor_changed = !!(x)
#define SETM_DECOR_CHANGED(fw,x) (fw)->flag_mask.is_decor_changed = !!(x)
#define IS_ICON_FONT_LOADED(fw)     ((fw)->flags.is_icon_font_loaded)
#define SET_ICON_FONT_LOADED(fw,x)  (fw)->flags.is_icon_font_loaded = !!(x)
#define SETM_ICON_FONT_LOADED(fw,x) (fw)->flag_mask.is_icon_font_loaded = !!(x)
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
#define IS_IN_TRANSIENT_SUBTREE(fw) ((fw)->flags.is_in_transient_subtree)
#define SET_IN_TRANSIENT_SUBTREE(fw,x) \
          (fw)->flags.is_in_transient_subtree = !!(x)
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
#define IS_PLACED_WB3(fw)      ((fw)->flags.is_placed_wb3)
#define SET_PLACED_WB3(fw,x)   (fw)->flags.is_placed_wb3 = !!(x)
#define SETM_PLACED_WB3(fw,x)  (fw)->flag_mask.is_placed_wb3 = !!(x)
#define IS_PLACED_BY_FVWM(fw)      ((fw)->flags.is_placed_by_fvwm)
#define SET_PLACED_BY_FVWM(fw,x)   (fw)->flags.is_placed_by_fvwm = (x)
#define SETM_PLACED_BY_FVWM(fw,x)  (fw)->flag_mask.is_placed_by_fvwm = (x)
#define IS_SCHEDULED_FOR_DESTROY(fw)    ((fw)->flags.is_scheduled_for_destroy)
#define SET_SCHEDULED_FOR_DESTROY(fw,x) \
          (fw)->flags.is_scheduled_for_destroy = !!(x)
#define IS_SCHEDULED_FOR_RAISE(fw)    ((fw)->flags.is_scheduled_for_raise)
#define SET_SCHEDULED_FOR_RAISE(fw,x) (fw)->flags.is_scheduled_for_raise = !!(x)
#define IS_SHADED(fw)          ((fw)->flags.is_window_shaded)
#define SET_SHADED(fw,x)       (fw)->flags.is_window_shaded = !!(x)
#define SETM_SHADED(fw,x)      (fw)->flag_mask.is_window_shaded = !!(x)
#define IS_TRANSIENT(fw)       ((fw)->flags.is_transient)
#define SET_TRANSIENT(fw,x)    (fw)->flags.is_transient = !!(x)
#define SETM_TRANSIENT(fw,x)   (fw)->flag_mask.is_transient = !!(x)
#define IS_DEICONIFY_PENDING(fw)     ((fw)->flags.is_deiconify_pending)
#define SET_DEICONIFY_PENDING(fw,x)  (fw)->flags.is_deiconify_pending = !!(x)
#define SETM_DEICONIFY_PENDING(fw,x) \
          (fw)->flag_mask.is_deiconify_pending = !!(x)
#define DO_ICONIFY_AFTER_MAP(fw)     ((fw)->flags.do_iconify_after_map)
#define SET_ICONIFY_AFTER_MAP(fw,x)  (fw)->flags.do_iconify_after_map = !!(x)
#define SETM_ICONIFY_AFTER_MAP(fw,x) \
          (fw)->flag_mask.do_iconify_after_map = !!(x)
#define IS_SIZE_INC_SET(fw)     ((fw)->flags.is_size_inc_set)
#define SET_SIZE_INC_SET(fw,x)  (fw)->flags.is_size_inc_set = !!(x)
#define SETM_SIZE_INC_SET(fw,x) (fw)->flag_mask.is_size_inc_set = !!(x)
#define IS_STYLE_DELETED(fw)     ((fw)->flags.is_style_deleted)
#define SET_STYLE_DELETED(fw,x)  (fw)->flags.is_style_deleted = !!(x)
#define SETM_STYLE_DELETED(fw,x) (fw)->flag_mask.is_style_deleted = !!(x)
#define IS_VIEWPORT_MOVED(fw)  ((fw)->flags.is_viewport_moved)
#define SET_VIEWPORT_MOVED(fw,x) \
          (fw)->flags.is_viewport_moved = !!(x)
#define SETM_VIEWPORT_MOVED(fw,x) \
          (fw)->flag_mask.is_viewport_moved = !!(x)
#define IS_VIEWPORT_MOVED(fw)  ((fw)->flags.is_viewport_moved)
#define IS_FOCUS_CHANGE_BROADCAST_PENDING(fw) \
          ((fw)->flags.is_focus_change_broadcast_pending)
#define SET_FOCUS_CHANGE_BROADCAST_PENDING(fw,x) \
          (fw)->flags.is_focus_change_broadcast_pending = !!(x)
#define SETM_FOCUS_CHANGE_BROADCAST_PENDING(fw,x) \
          (fw)->flag_mask.is_focus_change_broadcast_pending = !!(x)
#define IS_FULLY_VISIBLE(fw)   ((fw)->flags.is_fully_visible)
#define SET_FULLY_VISIBLE(fw,x) \
          (fw)->flags.is_fully_visible = !!(x)
#define SETM_FULLY_VISIBLE(fw,x) \
          (fw)->flag_mask.is_fully_visible = !!(x)
#define IS_PARTIALLY_VISIBLE(fw) \
          ((fw)->flags.is_partially_visible)
#define SET_PARTIALLY_VISIBLE(fw,x) \
          (fw)->flags.is_partially_visible = !!(x)
#define SETM_PARTIALLY_VISIBLE(fw,x) \
          (fw)->flag_mask.is_partially_visible = !!(x)
#define IS_WINDOW_DRAWN_ONCE(fw) \
          ((fw)->flags.is_window_drawn_once)
#define SET_WINDOW_DRAWN_ONCE(fw,x) \
	  (fw)->flags.is_window_drawn_once = !!(x)
#define SETM_WINDOW_DRAWN_ONCE(fw,x) \
	  (fw)->flag_mask.is_window_drawn_once = !!(x)
#define IS_WINDOW_BEING_MOVED_OPAQUE(fw) \
          ((fw)->flags.is_window_being_moved_opaque)
#define SET_WINDOW_BEING_MOVED_OPAQUE(fw,x) \
          (fw)->flags.is_window_being_moved_opaque = !!(x)
#define SETM_WINDOW_BEING_MOVED_OPAQUE(fw,x) \
          (fw)->flag_mask.is_window_being_moved_opaque = !!(x)
#define IS_WINDOW_BORDER_DRAWN(fw)     ((fw)->flags.is_window_border_drawn)
#define SET_WINDOW_BORDER_DRAWN(fw,x)  \
          (fw)->flags.is_window_border_drawn = !!(x)
#define SETM_WINDOW_BORDER_DRAWN(fw,x) \
          (fw)->flag_mask.is_window_border_drawn = !!(x)
#define IS_WINDOW_FONT_LOADED(fw)     ((fw)->flags.is_window_font_loaded)
#define SET_WINDOW_FONT_LOADED(fw,x)  (fw)->flags.is_window_font_loaded = !!(x)
#define SETM_WINDOW_FONT_LOADED(fw,x) \
          (fw)->flag_mask.is_window_font_loaded = !!(x)
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
#define USING_DEFAULT_WINDOW_FONT(fw) \
          ((fw)->flags.using_default_window_font)
#define SET_USING_DEFAULT_WINDOW_FONT(fw,x) \
          (fw)->flags.using_default_window_font = !!(x)
#define SETM_USING_DEFAULT_WINDOW_FONT(fw,x) \
          (fw)->flag_mask.using_default_window_font = !!(x)
#define USING_DEFAULT_ICON_FONT(fw) \
          ((fw)->flags.using_default_icon_font)
#define SET_USING_DEFAULT_ICON_FONT(fw,x) \
          (fw)->flags.using_default_icon_font = !!(x)
#define SETM_USING_DEFAULT_ICON_FONT(fw,x) \
          (fw)->flag_mask.using_default_icon_font = !!(x)
#define WAS_ICON_HINT_PROVIDED(fw) \
          ((fw)->flags.was_icon_hint_provided)
#define SET_WAS_ICON_HINT_PROVIDED(fw,x) \
          (fw)->flags.was_icon_hint_provided = (x)
#define SETM_WAS_ICON_HINT_PROVIDED(fw,x) \
          (fw)->flag_mask.was_icon_hint_provided = (x)
#define WAS_ICON_NAME_PROVIDED(fw) \
          ((fw)->flags.was_icon_name_provided)
#define SET_WAS_ICON_NAME_PROVIDED(fw,x) \
          (fw)->flags.was_icon_name_provided = (x)
#define SETM_WAS_ICON_NAME_PROVIDED(fw,x) \
          (fw)->flag_mask.was_icon_name_provided = (x)
#define HAS_EWMH_WM_NAME(fw)        ((fw)->flags.has_ewmh_wm_name)
#define SET_HAS_EWMH_WM_NAME(fw,x)  (fw)->flags.has_ewmh_wm_name = !!(x)
#define SETM_HAS_EWMH_WM_NAME(fw,x) (fw)->flag_mask.has_ewmh_wm_name = !!(x)
#define HAS_EWMH_WM_ICON_NAME(fw) \
          ((fw)->flags.has_ewmh_wm_icon_name)
#define SET_HAS_EWMH_WM_ICON_NAME(fw,x) \
          (fw)->flags.has_ewmh_wm_icon_name = !!(x)
#define SETM_HAS_EWMH_WM_ICON_NAME(fw,x) \
          (fw)->flag_mask.has_ewmh_wm_icon_name = !!(x)
#define HAS_EWMH_WM_ICON_HINT(fw) ((fw)->flags.has_ewmh_wm_icon_hint)
#define SET_HAS_EWMH_WM_ICON_HINT(fw,x)   (fw)->flags.has_ewmh_wm_icon_hint = (x)
#define SETM_HAS_EWMH_WM_ICON_HINT(fw,x)  (fw)->flag_mask.has_ewmh_wm_icon_hint = (x)
#define USE_EWMH_ICON(fw) ((fw)->flags.use_ewmh_icon)
#define SET_USE_EWMH_ICON(fw,x) (fw)->flags.use_ewmh_icon = !!(x)
#define SETM_USE_EWMH_ICON(fw,x) (fw)->flag_mask.use_ewmh_icon = !!(x)
#define HAS_EWMH_MINI_ICON(fw) ((fw)->flags.has_ewmh_mini_icon)
#define SET_HAS_EWMH_MINI_ICON(fw,x) (fw)->flags.has_ewmh_mini_icon = !!(x)
#define SETM_HAS_EWMH_MINI_ICON(fw,x) (fw)->flag_mask.has_ewmh_mini_icon = !!(x)
#endif /* _WINDOW_FLAGS_ */
