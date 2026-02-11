/* -*-c-*- */

#ifndef FVWM_WINDOW_FLAGS_H
#define FVWM_WINDOW_FLAGS_H

#include "focus_policy.h"

/* access to the common flags of a window */
#define FW_COMMON_FLAGS(fw) \
	((fw)->flags.common)
#define FW_COMMON_STATIC_FLAGS(fw) \
	((fw)->flags.common.s)
#define FW_FOCUS_POLICY(fw) \
	((fw)->flags.common.s.focus_policy)

#define DO_LOWER_TRANSIENT(fw) \
	((fw)->flags.common.s.do_lower_transient)
#define DO_NOT_SHOW_ON_MAP(fw) \
	((fw)->flags.common.s.do_not_show_on_map)
#define DO_RAISE_TRANSIENT(fw) \
	((fw)->flags.common.s.do_raise_transient)
#define DO_RESIZE_OPAQUE(fw) \
	((fw)->flags.common.s.do_resize_opaque)
#define DO_SHRINK_WINDOWSHADE(fw) \
	((fw)->flags.common.s.do_shrink_windowshade)
#define DO_SKIP_CIRCULATE(fw) \
	((fw)->flags.common.s.do_circulate_skip)
#define SET_DO_SKIP_CIRCULATE(fw,x) \
	(fw)->flags.common.s.do_circulate_skip = !!(x)
#define DO_SKIP_ICON_CIRCULATE(fw) \
	((fw)->flags.common.s.do_circulate_skip_icon)
#define SET_DO_SKIP_ICON_CIRCULATE(fw,x) \
	(fw)->flags.common.s.do_circulate_skip_icon = !!(x)
#define DO_SKIP_SHADED_CIRCULATE(fw) \
	((fw)->flags.common.s.do_circulate_skip_shaded)
#define SET_DO_SKIP_SHADED_CIRCULATE(fw,x) \
	(fw)->flags.common.s.do_circulate_skip_shaded = !!(x)
#define DO_SKIP_WINDOW_LIST(fw) \
	((fw)->flags.common.s.do_window_list_skip)
#define SET_DO_SKIP_WINDOW_LIST(fw,x) \
	(fw)->flags.common.s.do_window_list_skip = !!(x)
#define DO_STACK_TRANSIENT_PARENT(fw) \
	((fw)->flags.common.s.do_stack_transient_parent)
#define GET_TITLE_DIR(fw) \
	((fw)->flags.common.title_dir)
#define HAS_TITLE_DIR(fw,x) \
	((fw)->flags.common.title_dir == x)
#define SET_TITLE_DIR(fw,x) \
	((fw)->flags.common.title_dir = x)
#define SETM_TITLE_DIR(fw,x) \
	((fw)->flag_mask.common.title_dir = ((x) ? DIR_MAJOR_MASK : 0))
#define SET_USER_STATES(fw, mask) \
	((fw)->flags.common.user_states |= (mask))
#define CLEAR_USER_STATES(fw, mask) \
	((fw)->flags.common.user_states &= ~(mask))
#define TOGGLE_USER_STATES(fw, mask) \
	((fw)->flags.common.user_states ^= (mask))
#define SETM_USER_STATES(fw, mask) \
	((fw)->flag_mask.common.user_states |= (mask))
#define GET_USER_STATES(fw) \
	((fw)->flags.common.user_states)
#define HAS_VERTICAL_TITLE(fw) \
	((HAS_TITLE_DIR(fw,DIR_W) || HAS_TITLE_DIR(fw,DIR_E)))
#define HAS_STIPPLED_TITLE(fw) \
	((fw)->flags.common.s.has_stippled_title)
#define HAS_STICKY_STIPPLED_TITLE(fw) \
	!((fw)->flags.common.s.has_no_sticky_stippled_title)
#define HAS_STICKY_STIPPLED_ICON_TITLE(fw) \
	!((fw)->flags.common.s.has_no_sticky_stippled_icon_title)
#define HAS_STIPPLED_ICON_TITLE(fw) \
	((fw)->flags.common.s.has_stippled_icon_title)
#define ICON_OVERRIDE_MODE(fw) \
	((fw)->flags.common.s.icon_override)
#define IS_ICON_STICKY_ACROSS_PAGES(fw) \
	((fw)->flags.common.s.is_icon_sticky_across_pages)
#define SET_ICON_STICKY_ACROSS_PAGES(fw,x) \
	(fw)->flags.common.s.is_icon_sticky_across_pages = !!(x)
#define SETM_ICON_STICKY_ACROSS_PAGES(fw,x) \
	(fw)->flag_mask.common.s.is_icon_sticky_across_pages = !!(x)
#define IS_ICON_STICKY_ACROSS_DESKS(fw) \
	((fw)->flags.common.s.is_icon_sticky_across_desks)
#define SET_ICON_STICKY_ACROSS_DESKS(fw,x) \
	(fw)->flags.common.s.is_icon_sticky_across_desks = !!(x)
#define SETM_ICON_STICKY_ACROSS_DESKS(fw,x) \
	(fw)->flag_mask.common.s.is_icon_sticky_across_desks = !!(x)
#define USE_ICON_POSITION_HINT(fw) \
	((fw)->flags.common.s.use_icon_position_hint)
#define USE_INDEXED_WINDOW_NAME(fw) \
	((fw)->flags.common.s.use_indexed_window_name)
#define USE_INDEXED_ICON_NAME(fw) \
	((fw)->flags.common.s.use_indexed_icon_name)
#define WINDOWSHADE_LAZINESS(fw) \
	((fw)->flags.common.s.windowshade_laziness)
#define DO_EWMH_MINI_ICON_OVERRIDE(fw) \
	((fw)->flags.common.s.do_ewmh_mini_icon_override)
#define DO_EWMH_DONATE_ICON(fw) \
	((fw)->flags.common.s.do_ewmh_donate_icon)
#define DO_EWMH_DONATE_MINI_ICON(fw) \
	((fw)->flags.common.s.do_ewmh_donate_mini_icon)
#define DO_EWMH_USE_STACKING_HINTS(fw) \
	((fw)->flags.common.s.do_ewmh_use_stacking_hints)
#define DO_EWMH_IGNORE_STRUT_HINTS(fw) \
	((fw)->flags.common.s.do_ewmh_ignore_strut_hints)
#define DO_EWMH_IGNORE_STATE_HINTS(fw) \
	((fw)->flags.common.s.do_ewmh_ignore_state_hints)
#define DO_EWMH_IGNORE_WINDOW_TYPE(fw) \
	((fw)->flags.common.s.do_ewmh_ignore_window_type)
#define EWMH_MAXIMIZE_MODE(fw) \
	((fw)->flags.common.s.ewmh_maximize_mode)
#define IS_ICON_SUPPRESSED(fw) \
	((fw)->flags.common.s.is_icon_suppressed)
#define SET_ICON_SUPPRESSED(fw,x) \
	(fw)->flags.common.s.is_icon_suppressed = !!(x)
#define IS_STICKY_ACROSS_PAGES(fw) \
	((fw)->flags.common.is_sticky_across_pages)
#define SET_STICKY_ACROSS_PAGES(fw,x) \
	(fw)->flags.common.is_sticky_across_pages = !!(x)
#define SETM_STICKY_ACROSS_PAGES(fw,x) \
	(fw)->flag_mask.common.is_sticky_across_pages = !!(x)
#define IS_STICKY_ACROSS_DESKS(fw) \
	((fw)->flags.common.is_sticky_across_desks)
#define SET_STICKY_ACROSS_DESKS(fw,x) \
	(fw)->flags.common.is_sticky_across_desks = !!(x)
#define SETM_STICKY_ACROSS_DESKS(fw,x) \
	(fw)->flag_mask.common.is_sticky_across_desks = !!(x)
#define HAS_ICON_FONT(fw) \
	((fw)->flags.common.has_icon_font)
#define HAS_NO_ICON_TITLE(fw) \
	((fw)->flags.common.s.has_no_icon_title)
#define SET_HAS_NO_ICON_TITLE(fw,x) \
	(fw)->flags.common.s.has_no_icon_title = !!(x)
#define HAS_WINDOW_FONT(fw) \
	((fw)->flags.common.has_window_font)
#define HAS_MWM_BORDER(fw) \
	((fw)->flags.common.s.has_mwm_border)
#define HAS_MWM_BUTTONS(fw) \
	((fw)->flags.common.s.has_mwm_buttons)
#define HAS_MWM_OVERRIDE_HINTS(fw) \
	((fw)->flags.common.s.has_mwm_override)
#define HAS_OVERRIDE_SIZE_HINTS(fw) \
	((fw)->flags.common.s.has_override_size)
#define DO_ICONIFY_WINDOW_GROUPS(fw) \
	((fw)->flags.common.s.do_iconify_window_groups)
#define DO_IGNORE_RESTACK(fw) \
	((fw)->flags.common.s.do_ignore_restack)
#define DO_IGNORE_ICON_BOXES(fw) \
	((fw)->flags.common.s.do_ignore_icon_boxes)
#define DO_USE_WINDOW_GROUP_HINT(fw) \
	((fw)->flags.common.s.do_use_window_group_hint)
#define IS_FIXED(fw) \
	((fw)->flags.common.s.is_fixed)
#define SET_FIXED(fw,x) \
	(fw)->flags.common.s.is_fixed = !!(x)
#define SETM_FIXED(fw,x) \
	(fw)->flag_mask.common.s.is_fixed = !!(x)
#define IS_FIXED_PPOS(fw) \
	((fw)->flags.common.s.is_fixed_ppos)
#define IS_SIZE_FIXED(fw) \
	((fw)->flags.common.s.is_size_fixed)
#define SET_SIZE_FIXED(fw,x) \
	(fw)->flags.common.s.is_size_fixed = !!(x)
#define SETM_SIZE_FIXED(fw,x) \
	(fw)->flag_mask.common.s.is_size_fixed = !!(x)
#define IS_PSIZE_FIXED(fw) \
	((fw)->flags.common.s.is_psize_fixed)
#define IS_UNICONIFIABLE(fw) \
	((fw)->flags.common.s.is_uniconifiable)
#define SET_IS_UNICONIFIABLE(fw,x) \
	(fw)->flags.common.s.is_uniconifiable = !!(x)
#define SETM_IS_UNICONIFIABLE(fw,x) \
	(fw)->flag_mask.common.s.is_uniconifiable = !!(x)
#define IS_UNMAXIMIZABLE(fw) \
	((fw)->flags.common.s.is_unmaximizable)
#define SET_IS_UNMAXIMIZABLE(fw,x) \
	(fw)->flags.common.s.is_unmaximizable = !!(x)
#define SETM_IS_UNMAXIMIZABLE(fw,x) \
	(fw)->flag_mask.common.s.is_unmaximizable = !!(x)
#define IS_UNCLOSABLE(fw) \
	((fw)->flags.common.s.is_unclosable)
#define SET_IS_UNCLOSABLE(fw,x) \
	(fw)->flags.common.s.is_unclosable = !!(x)
#define SETM_IS_UNCLOSABLE(fw,x) \
	(fw)->flag_mask.common.s.is_unclosable = !!(x)
#define IS_MAXIMIZE_FIXED_SIZE_DISALLOWED(fw) \
	((fw)->flags.common.s.is_maximize_fixed_size_disallowed)
#define HAS_DEPRESSABLE_BORDER(fw) \
	((fw)->flags.common.s.has_depressable_border)
#define USE_TITLE_DECOR_ROTATION(fw) \
	((fw)->flags.common.s.use_title_decor_rotation)

/* access to the special flags of a window */
#define DO_REUSE_DESTROYED(fw) \
	((fw)->flags.do_reuse_destroyed)
#define HAS_NO_BORDER(fw) \
	((fw)->flags.common.has_no_border)
#define SET_HAS_NO_BORDER(fw,x) \
	(fw)->flags.common.has_no_border = !!(x)
#define SETM_HAS_NO_BORDER(fw,x) \
	(fw)->flag_mask.common.has_no_border = !!(x)
#define HAS_HANDLES(fw) \
	((fw)->flags.has_handles)
#define SET_HAS_HANDLES(fw,x) \
	(fw)->flags.has_handles = !!(x)
#define SETM_HAS_HANDLES(fw,x) \
	(fw)->flag_mask.has_handles = !!(x)
#define HAS_ICON_CHANGED(fw) \
	((fw)->flags.has_icon_changed)
#define SET_HAS_ICON_CHANGED(fw,x) \
	(fw)->flags.has_icon_changed = !!(x)
#define HAS_TITLE(fw) \
	((fw)->flags.has_title)
#define SET_HAS_TITLE(fw,x) \
	(fw)->flags.has_title = !!(x)
#define SETM_HAS_TITLE(fw,x) \
	(fw)->flag_mask.has_title = !!(x)
#define HAS_NEW_WM_NORMAL_HINTS(fw) \
	((fw)->flags.has_new_wm_normal_hints)
#define SET_HAS_NEW_WM_NORMAL_HINTS(fw,x) \
	(fw)->flags.has_new_wm_normal_hints = !!(x)
#define IS_MAPPED(fw) \
	((fw)->flags.is_mapped)
#define SET_MAPPED(fw,x) \
	(fw)->flags.is_mapped = !!(x)
#define IS_DECOR_CHANGED(fw) \
	((fw)->flags.is_decor_changed)
#define SET_DECOR_CHANGED(fw,x) \
	(fw)->flags.is_decor_changed = !!(x)
#define IS_ICON_FONT_LOADED(fw) \
	((fw)->flags.is_icon_font_loaded)
#define SET_ICON_FONT_LOADED(fw,x) \
	(fw)->flags.is_icon_font_loaded = !!(x)
#define IS_ICONIFIED(fw) \
	((fw)->flags.is_iconified)
#define SET_ICONIFIED(fw,x) \
	(fw)->flags.is_iconified = !!(x)
#define SETM_ICONIFIED(fw,x) \
	(fw)->flag_mask.is_iconified = !!(x)
#define IS_ICONIFIED_BY_PARENT(fw) \
	((fw)->flags.is_iconified_by_parent)
#define SET_ICONIFIED_BY_PARENT(fw,x) \
	(fw)->flags.is_iconified_by_parent = !!(x)
#define IS_ICON_ENTERED(fw) \
	((fw)->flags.is_icon_entered)
#define SET_ICON_ENTERED(fw,x) \
	(fw)->flags.is_icon_entered = !!(x)
#define IS_ICON_OURS(fw) \
	((fw)->flags.is_icon_ours)
#define SET_ICON_OURS(fw,x) \
	(fw)->flags.is_icon_ours = !!(x)
#define IS_ICON_SHAPED(fw) \
	((fw)->flags.is_icon_shaped)
#define SET_ICON_SHAPED(fw,x) \
	(fw)->flags.is_icon_shaped = !!(x)
#define IS_ICON_MOVED(fw) \
	((fw)->flags.is_icon_moved)
#define SET_ICON_MOVED(fw,x) \
	(fw)->flags.is_icon_moved = !!(x)
#define IS_ICON_UNMAPPED(fw) \
	((fw)->flags.is_icon_unmapped)
#define SET_ICON_UNMAPPED(fw,x) \
	(fw)->flags.is_icon_unmapped = !!(x)
#define IS_IN_TRANSIENT_SUBTREE(fw) \
	((fw)->flags.is_in_transient_subtree)
#define SET_IN_TRANSIENT_SUBTREE(fw,x) \
	(fw)->flags.is_in_transient_subtree = !!(x)
#define IS_MAP_PENDING(fw) \
	((fw)->flags.is_map_pending)
#define SET_MAP_PENDING(fw,x) \
	(fw)->flags.is_map_pending = !!(x)
#define IS_MAXIMIZED(fw) \
	((fw)->flags.is_maximized)
#define SET_MAXIMIZED(fw,x) \
	(fw)->flags.is_maximized = !!(x)
#define SETM_MAXIMIZED(fw,x) \
	(fw)->flag_mask.is_maximized = !!(x)
#define IS_NAME_CHANGED(fw) \
	((fw)->flags.is_name_changed)
#define SET_NAME_CHANGED(fw,x) \
	(fw)->flags.is_name_changed = !!(x)
#define IS_PIXMAP_OURS(fw) \
	((fw)->flags.is_pixmap_ours)
#define SET_PIXMAP_OURS(fw,x) \
	(fw)->flags.is_pixmap_ours = !!(x)
#define IS_PLACED_BY_FVWM(fw) \
	((fw)->flags.is_placed_by_fvwm)
#define SET_PLACED_BY_FVWM(fw,x) \
	(fw)->flags.is_placed_by_fvwm = (x)
#define SETM_PLACED_BY_FVWM(fw,x) \
	(fw)->flag_mask.is_placed_by_fvwm = (x)
#define IS_SCHEDULED_FOR_DESTROY(fw) \
	((fw)->flags.is_scheduled_for_destroy)
#define SET_SCHEDULED_FOR_DESTROY(fw,x) \
	(fw)->flags.is_scheduled_for_destroy = !!(x)
#define IS_SCHEDULED_FOR_RAISE(fw) \
	((fw)->flags.is_scheduled_for_raise)
#define SET_SCHEDULED_FOR_RAISE(fw,x) \
	(fw)->flags.is_scheduled_for_raise = !!(x)
#define IS_SHADED(fw) \
	((fw)->flags.is_window_shaded)
#define USED_TITLE_DIR_FOR_SHADING(fw) \
	((fw)->flags.used_title_dir_for_shading)
#define SET_USED_TITLE_DIR_FOR_SHADING(fw,x) \
	((fw)->flags.used_title_dir_for_shading = !!(x))
#define SHADED_DIR(fw) \
	((fw)->flags.shaded_dir)
#define SET_SHADED(fw,x) \
	(fw)->flags.is_window_shaded = !!(x)
#define SET_SHADED_DIR(fw,x) \
	(fw)->flags.shaded_dir = (x)
#define SETM_SHADED(fw,x) \
	(fw)->flag_mask.is_window_shaded = !!(x)
#define IS_TEAR_OFF_MENU(fw) \
	((fw)->flags.is_tear_off_menu)
#define SET_TEAR_OFF_MENU(fw,x) \
	(fw)->flags.is_tear_off_menu = !!(x)
#define IS_TRANSIENT(fw) \
	((fw)->flags.is_transient)
#define SET_TRANSIENT(fw,x) \
	(fw)->flags.is_transient = !!(x)
#define SETM_TRANSIENT(fw,x) \
	(fw)->flag_mask.is_transient = !!(x)
#define IS_ICONIFY_PENDING(fw) \
	((fw)->flags.is_iconify_pending)
#define SET_ICONIFY_PENDING(fw,x) \
	(fw)->flags.is_iconify_pending = !!(x)
#define DO_ICONIFY_AFTER_MAP(fw) \
	((fw)->flags.do_iconify_after_map)
#define SET_ICONIFY_AFTER_MAP(fw,x) \
	(fw)->flags.do_iconify_after_map = !!(x)
#define DO_DISABLE_CONSTRAIN_SIZE_FULLSCREEN(fw) \
	((fw)->flags.do_disable_constrain_size_fullscreen)
#define SET_DISABLE_CONSTRAIN_SIZE_FULLSCREEN(fw,x) \
	(fw)->flags.do_disable_constrain_size_fullscreen = !!(x)
#define SET_SIZE_INC_SET(fw,x) \
	(fw)->flags.is_size_inc_set = !!(x)
#define IS_STYLE_DELETED(fw) \
	((fw)->flags.is_style_deleted)
#define SET_STYLE_DELETED(fw,x) \
	(fw)->flags.is_style_deleted = !!(x)
#define IS_VIEWPORT_MOVED(fw) \
	((fw)->flags.is_viewport_moved)
#define SET_VIEWPORT_MOVED(fw,x) \
	(fw)->flags.is_viewport_moved = !!(x)
#define IS_VIEWPORT_MOVED(fw) \
	((fw)->flags.is_viewport_moved)
#define IS_FOCUS_CHANGE_BROADCAST_PENDING(fw) \
	((fw)->flags.is_focus_change_broadcast_pending)
#define SET_FOCUS_CHANGE_BROADCAST_PENDING(fw,x) \
	(fw)->flags.is_focus_change_broadcast_pending = !!(x)
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
#define IS_WINDOW_BEING_MOVED_OPAQUE(fw) \
	((fw)->flags.is_window_being_moved_opaque)
#define SET_WINDOW_BEING_MOVED_OPAQUE(fw,x) \
	(fw)->flags.is_window_being_moved_opaque = !!(x)
#define IS_WINDOW_FONT_LOADED(fw) \
	((fw)->flags.is_window_font_loaded)
#define SET_WINDOW_FONT_LOADED(fw,x) \
	(fw)->flags.is_window_font_loaded = !!(x)
#define CR_MOTION_METHOD(fw) \
	((fw)->flags.cr_motion_method)
#define SET_CR_MOTION_METHOD(fw,x) \
	(fw)->flags.cr_motion_method = ((x) & CR_MOTION_METHOD_MASK)
#define WAS_CR_MOTION_METHOD_DETECTED(fw) \
	((fw)->flags.was_cr_motion_method_detected)
#define SET_CR_MOTION_METHOD_DETECTED(fw,x) \
	(fw)->flags.was_cr_motion_method_detected = !!(x)
#define WM_DELETES_WINDOW(fw) \
	((fw)->flags.does_wm_delete_window)
#define SET_WM_DELETES_WINDOW(fw,x) \
	(fw)->flags.does_wm_delete_window = !!(x)
#define WM_TAKES_FOCUS(fw) \
	((fw)->flags.does_wm_take_focus)
#define SET_WM_TAKES_FOCUS(fw,x) \
	(fw)->flags.does_wm_take_focus = !!(x)
#define DO_FORCE_NEXT_CR(fw) \
	((fw)->flags.do_force_next_cr)
#define SET_FORCE_NEXT_CR(fw,x) \
	(fw)->flags.do_force_next_cr = !!(x)
#define SET_FORCE_NEXT_PN(fw,x) \
	(fw)->flags.do_force_next_pn = !!(x)
#define USING_DEFAULT_WINDOW_FONT(fw) \
	((fw)->flags.using_default_window_font)
#define SET_USING_DEFAULT_WINDOW_FONT(fw,x) \
	(fw)->flags.using_default_window_font = !!(x)
#define USING_DEFAULT_ICON_FONT(fw) \
	((fw)->flags.using_default_icon_font)
#define SET_USING_DEFAULT_ICON_FONT(fw,x) \
	(fw)->flags.using_default_icon_font = !!(x)
#define WAS_ICON_HINT_PROVIDED(fw) \
	((fw)->flags.was_icon_hint_provided)
#define SET_WAS_ICON_HINT_PROVIDED(fw,x) \
	(fw)->flags.was_icon_hint_provided = (x)
#define WAS_ICON_NAME_PROVIDED(fw) \
	((fw)->flags.was_icon_name_provided)
#define SET_WAS_ICON_NAME_PROVIDED(fw,x) \
	(fw)->flags.was_icon_name_provided = (x)
#define WAS_NEVER_DRAWN(fw) \
	((fw)->flags.was_never_drawn)
#define SET_WAS_NEVER_DRAWN(fw,x) \
	(fw)->flags.was_never_drawn = (x)
#define HAS_EWMH_WM_NAME(fw) \
	((fw)->flags.has_ewmh_wm_name)
#define SET_HAS_EWMH_WM_NAME(fw,x) \
	(fw)->flags.has_ewmh_wm_name = !!(x)
#define HAS_EWMH_WM_ICON_NAME(fw) \
	((fw)->flags.has_ewmh_wm_icon_name)
#define SET_HAS_EWMH_WM_ICON_NAME(fw,x) \
	(fw)->flags.has_ewmh_wm_icon_name = !!(x)
#define HAS_EWMH_WM_ICON_HINT(fw) \
	((fw)->flags.has_ewmh_wm_icon_hint)
#define SET_HAS_EWMH_WM_ICON_HINT(fw,x) \
	(fw)->flags.has_ewmh_wm_icon_hint = (x)
#define USE_EWMH_ICON(fw) \
	((fw)->flags.use_ewmh_icon)
#define SET_USE_EWMH_ICON(fw,x) \
	(fw)->flags.use_ewmh_icon = !!(x)
#define HAS_EWMH_MINI_ICON(fw) \
	((fw)->flags.has_ewmh_mini_icon)
#define SET_HAS_EWMH_MINI_ICON(fw,x) \
	(fw)->flags.has_ewmh_mini_icon = !!(x)
#define HAS_EWMH_WM_PID(fw) \
	((fw)->flags.has_ewmh_wm_pid)
#define SET_HAS_EWMH_WM_PID(fw,x) \
	(fw)->flags.has_ewmh_wm_pid = !!(x)
#define IS_EWMH_MODAL(fw) \
	((fw)->flags.is_ewmh_modal)
#define SET_EWMH_MODAL(fw,x) \
	(fw)->flags.is_ewmh_modal = !!(x)
#define IS_EWMH_FULLSCREEN(fw) \
	((fw)->flags.is_ewmh_fullscreen)
#define SET_EWMH_FULLSCREEN(fw,x) \
	(fw)->flags.is_ewmh_fullscreen = !!(x)
#define SET_HAS_EWMH_INIT_FULLSCREEN_STATE(fw,x) \
	(fw)->flags.has_ewmh_init_fullscreen_state = (x)
#define HAS_EWMH_INIT_FULLSCREEN_STATE(fw) \
	((fw)->flags.has_ewmh_init_fullscreen_state)
#define SET_HAS_EWMH_INIT_HIDDEN_STATE(fw,x) \
	(fw)->flags.has_ewmh_init_hidden_state = (x)
#define HAS_EWMH_INIT_HIDDEN_STATE(fw) \
	((fw)->flags.has_ewmh_init_hidden_state)
#define SET_HAS_EWMH_INIT_MAXHORIZ_STATE(fw,x) \
	(fw)->flags.has_ewmh_init_maxhoriz_state = (x)
#define HAS_EWMH_INIT_MAXHORIZ_STATE(fw) \
	((fw)->flags.has_ewmh_init_maxhoriz_state)
#define SET_HAS_EWMH_INIT_MAXVERT_STATE(fw,x) \
	(fw)->flags.has_ewmh_init_maxvert_state = (x)
#define HAS_EWMH_INIT_MAXVERT_STATE(fw) \
	((fw)->flags.has_ewmh_init_maxvert_state)
#define SET_HAS_EWMH_INIT_MODAL_STATE(fw,x) \
	(fw)->flags.has_ewmh_init_modal_state = (x)
#define HAS_EWMH_INIT_MODAL_STATE(fw) \
	((fw)->flags.has_ewmh_init_modal_state)
#define SET_HAS_EWMH_INIT_SHADED_STATE(fw,x) \
	(fw)->flags.has_ewmh_init_shaded_state = (x)
#define HAS_EWMH_INIT_SHADED_STATE(fw) \
	((fw)->flags.has_ewmh_init_shaded_state)
#define SET_HAS_EWMH_INIT_SKIP_PAGER_STATE(fw,x) \
	(fw)->flags.has_ewmh_init_skip_pager_state = (x)
#define HAS_EWMH_INIT_SKIP_PAGER_STATE(fw) \
	((fw)->flags.has_ewmh_init_skip_pager_state)
#define SET_HAS_EWMH_INIT_SKIP_TASKBAR_STATE(fw,x) \
	(fw)->flags.has_ewmh_init_skip_taskbar_state = (x)
#define HAS_EWMH_INIT_SKIP_TASKBAR_STATE(fw) \
	((fw)->flags.has_ewmh_init_skip_taskbar_state)
#define SET_HAS_EWMH_INIT_STICKY_STATE(fw,x) \
	(fw)->flags.has_ewmh_init_sticky_state = (x)
#define HAS_EWMH_INIT_STICKY_STATE(fw) \
	((fw)->flags.has_ewmh_init_sticky_state)
#define SET_HAS_EWMH_INIT_WM_DESKTOP(fw,x) \
	(fw)->flags.has_ewmh_init_wm_desktop = (x)
#define HAS_EWMH_INIT_WM_DESKTOP(fw) \
	((fw)->flags.has_ewmh_init_wm_desktop)

#endif /* FVWM_WINDOW_FLAGS_H */
