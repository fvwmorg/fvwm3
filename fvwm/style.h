/* -*-c-*- */

#ifndef _STYLE_
#define _STYLE_

/* access to the special flags of a style */
/* call these with a pointer to a style_flags struct */
#define SDO_DECORATE_TRANSIENT(sf) \
	((sf)->do_decorate_transient)
#define SDO_SAVE_UNDER(sf) \
	((sf)->do_save_under)
#define SDO_START_LOWERED(sf) \
	((sf)->do_start_lowered)
#define SDO_START_SHADED(sf) \
	((sf)->do_start_shaded)
#define SHAS_BORDER_WIDTH(sf) \
	((sf)->has_border_width)
#define SHAS_COLOR_BACK(sf) \
	((sf)->has_color_back)
#define SHAS_COLOR_FORE(sf) \
	((sf)->has_color_fore)
#define SHAS_HANDLE_WIDTH(sf) \
	((sf)->has_handle_width)
#define SHAS_ICON(sf) \
	((sf)->has_icon)
#define SHAS_ICON_BOXES(sf) \
	((sf)->has_icon_boxes)
#define SHAS_ICON_SIZE_LIMITS(sf) \
	((sf)->has_icon_size_limits)
#define SHAS_ICON_BACKGROUND_PADDING(sf) \
	((sf)->has_icon_background_padding)
#define SHAS_ICON_BACKGROUND_RELIEF(sf) \
	((sf)->has_icon_background_relief)
#define SHAS_ICON_TITLE_RELIEF(sf) \
	((sf)->has_icon_title_relief)
#define SHAS_MIN_WINDOW_SIZE(sf) \
	((sf)->has_min_window_size)
#define SHAS_MAX_WINDOW_SIZE(sf) \
	((sf)->has_max_window_size)
#define SHAS_WINDOW_SHADE_STEPS(sf) \
	((sf)->has_window_shade_steps)
#define SHAS_MINI_ICON(sf) \
	((sf)->has_mini_icon)
#define SHAS_MWM_DECOR(sf) \
	((sf)->has_mwm_decor)
#define SHAS_MWM_FUNCTIONS(sf) \
	((sf)->has_mwm_functions)
#define SHAS_NO_HANDLES(sf) \
	((sf)->has_no_handles)
#define SHAS_NO_TITLE(sf) \
	((sf)->has_no_title)
#define SHAS_OL_DECOR(sf) \
	((sf)->has_ol_decor)
#define SIS_BUTTON_DISABLED(sf) \
	((sf)->is_button_disabled)
#define SIS_UNMANAGED(sf) \
	((sf)->is_unmanaged)
#define SPLACEMENT_MODE(sf) \
	((sf)->placement_mode)
#define SEWMH_PLACEMENT_MODE(sf) \
	((sf)->ewmh_placement_mode)
#define SUSE_BACKING_STORE(sf) \
	((sf)->use_backing_store)
#define SUSE_PARENT_RELATIVE(sf) \
	((sf)->use_parent_relative)
#define SUSE_COLORSET(sf) \
	((sf)->use_colorset)
#define SUSE_COLORSET_HI(sf) \
	((sf)->use_colorset_hi)
#define SUSE_BORDER_COLORSET(sf) \
	((sf)->use_border_colorset)
#define SUSE_BORDER_COLORSET_HI(sf) \
	((sf)->use_border_colorset_hi)
#define SUSE_ICON_TITLE_COLORSET(sf) \
	((sf)->use_icon_title_colorset)
#define SUSE_ICON_TITLE_COLORSET_HI(sf) \
	((sf)->use_icon_title_colorset_hi)
#define SUSE_ICON_BACKGROUND_COLORSET(sf) \
	((sf)->use_icon_background_colorset)
#define SUSE_LAYER(sf) \
	((sf)->use_layer)
#define SUSE_NO_PPOSITION(sf) \
	((sf)->use_no_pposition)
#define SUSE_NO_USPOSITION(sf) \
	((sf)->use_no_usposition)
#define SUSE_NO_TRANSIENT_PPOSITION(sf) \
	((sf)->use_no_transient_pposition)
#define SUSE_NO_TRANSIENT_USPOSITION(sf) \
	((sf)->use_no_transient_usposition)
#define SUSE_START_ON_DESK(sf) \
	((sf)->use_start_on_desk)
#define SUSE_START_ON_PAGE_FOR_TRANSIENT(sf) \
	((sf)->use_start_on_page_for_transient)
#define SUSE_START_ON_SCREEN(sf) \
	((sf)->use_start_on_screen)
#define SMANUAL_PLACEMENT_HONORS_STARTS_ON_PAGE(sf) \
	((sf)->manual_placement_honors_starts_on_page)
#define SCAPTURE_HONORS_STARTS_ON_PAGE(sf) \
	((sf)->capture_honors_starts_on_page)
#define SRECAPTURE_HONORS_STARTS_ON_PAGE(sf) \
	((sf)->recapture_honors_starts_on_page)
#define SHAS_PLACEMENT_PENALTY(sf) \
	((sf)->has_placement_penalty)
#define SHAS_PLACEMENT_PERCENTAGE_PENALTY(sf) \
	((sf)->has_placement_percentage_penalty)
#define SHAS_PLACEMENT_POSITION_STRING(sf) \
	((sf)->has_placement_position_string
#define SCR_MOTION_METHOD(sf) \
	((sf)->ws_cr_motion_method)

/* access the various copies of the common flags structure. */
#define SCF(st) \
	((st).flags.common)
#define SCFS(st) \
	((st).flags.common.s)
#define SCM(st) \
	((st).flag_mask.common)
#define SCMS(st) \
	((st).flag_mask.common.s)
#define SCD(st) \
	((st).flag_default.common)
#define SCDS(st) \
	((st).flag_default.common.s)
#define SCC(st) \
	((st).change_mask.common)
#define SCCS(st) \
	((st).change_mask.common.s)
#define SFC(sf) \
	((sf).common)

/* access to common flags */
#define S_FOCUS_POLICY(c) \
	((c).s.focus_policy)
#define S_TITLE_DIR(c) \
	((c).title_dir)
#define S_SET_TITLE_DIR(c,x) \
	((c).title_dir = (x))
#define S_USER_STATES(c) \
	((c).user_states)
#define S_SET_USER_STATES(c,x) \
	((c).user_states = (x))
#define S_ADD_USER_STATES(c,x) \
	((c).user_states = ((c).user_states | (x)))
#define S_IS_STICKY_ACROSS_PAGES(c) \
	((c).is_sticky_across_pages)
#define S_SET_IS_STICKY_ACROSS_PAGES(c,x) \
	((c).is_sticky_across_pages = !!(x))
#define S_IS_STICKY_ACROSS_DESKS(c) \
	((c).is_sticky_across_desks)
#define S_SET_IS_STICKY_ACROSS_DESKS(c,x) \
	((c).is_sticky_across_desks = !!(x))
#define S_DO_CIRCULATE_SKIP(c) \
	((c).s.do_circulate_skip)
#define S_SET_DO_CIRCULATE_SKIP(c,x) \
	((c).s.do_circulate_skip = !!(x))
#define S_DO_CIRCULATE_SKIP_ICON(c) \
	((c).s.do_circulate_skip_icon)
#define S_SET_DO_CIRCULATE_SKIP_ICON(c,x) \
	((c).s.do_circulate_skip_icon = !!(x))
#define S_DO_CIRCULATE_SKIP_SHADED(c) \
	((c).s.do_circulate_skip_shaded)
#define S_SET_DO_CIRCULATE_SKIP_SHADED(c,x) \
	((c).s.do_circulate_skip_shaded = !!(x))
#define S_DO_ICONIFY_WINDOW_GROUPS(c) \
	((c).s.do_iconify_window_groups)
#define S_SET_DO_ICONIFY_WINDOW_GROUPS(c,x) \
	((c).s.do_iconify_window_groups = !!(x))
#define S_DO_IGNORE_ICON_BOXES(c) \
	((c).s.do_ignore_icon_boxes)
#define S_SET_DO_IGNORE_ICON_BOXES(c,x) \
	((c).s.do_ignore_icon_boxes = !!(x))
#define S_DO_IGNORE_RESTACK(c) \
	((c).s.do_ignore_restack)
#define S_SET_DO_IGNORE_RESTACK(c,x) \
	((c).s.do_ignore_restack = !!(x))
#define S_DO_USE_WINDOW_GROUP_HINT(c) \
	((c).s.do_use_window_group_hint)
#define S_SET_DO_USE_WINDOW_GROUP_HINT(c,x) \
	((c).s.do_use_window_group_hint = !!(x))
#define S_DO_LOWER_TRANSIENT(c) \
	((c).s.do_lower_transient)
#define S_SET_DO_LOWER_TRANSIENT(c,x) \
	((c).s.do_lower_transient = !!(x))
#define S_DO_NOT_SHOW_ON_MAP(c) \
	((c).s.do_not_show_on_map)
#define S_SET_DO_NOT_SHOW_ON_MAP(c,x) \
	((c).s.do_not_show_on_map = !!(x))
#define S_DO_RAISE_TRANSIENT(c) \
	((c).s.do_raise_transient)
#define S_SET_DO_RAISE_TRANSIENT(c,x) \
	((c).s.do_raise_transient = !!(x))
#define S_DO_RESIZE_OPAQUE(c) \
	((c).s.do_resize_opaque)
#define S_SET_DO_RESIZE_OPAQUE(c,x) \
	((c).s.do_resize_opaque = !!(x))
#define S_DO_SHRINK_WINDOWSHADE(c) \
	((c).s.do_shrink_windowshade)
#define S_SET_DO_SHRINK_WINDOWSHADE(c,x) \
	((c).s.do_shrink_windowshade = !!(x))
#define S_DO_STACK_TRANSIENT_PARENT(c) \
	((c).s.do_stack_transient_parent)
#define S_SET_DO_STACK_TRANSIENT_PARENT(c,x) \
	((c).s.do_stack_transient_parent = !!(x))
#define S_DO_WINDOW_LIST_SKIP(c) \
	((c).s.do_window_list_skip)
#define S_SET_DO_WINDOW_LIST_SKIP(c,x) \
	((c).s.do_window_list_skip = !!(x))
#define S_HAS_NO_BORDER(c) \
	((c).has_no_border)
#define S_SET_HAS_NO_BORDER(c,x) \
	((c).has_no_border = !!(x))
#define S_HAS_DEPRESSABLE_BORDER(c) \
	((c).s.has_depressable_border)
#define S_SET_HAS_DEPRESSABLE_BORDER(c,x) \
	((c).s.has_depressable_border = !!(x))
#define S_HAS_ICON_FONT(c) \
	((c).has_icon_font)
#define S_SET_HAS_ICON_FONT(c,x) \
	((c).has_icon_font = !!(x))
#define S_HAS_MWM_BORDER(c) \
	((c).s.has_mwm_border)
#define S_SET_HAS_MWM_BORDER(c,x) \
	((c).s.has_mwm_border = !!(x))
#define S_HAS_MWM_BUTTONS(c) \
	((c).s.has_mwm_buttons)
#define S_SET_HAS_MWM_BUTTONS(c,x) \
	((c).s.has_mwm_buttons = !!(x))
#define S_HAS_MWM_OVERRIDE(c) \
	((c).s.has_mwm_override)
#define S_SET_HAS_MWM_OVERRIDE(c,x) \
	((c).s.has_mwm_override = !!(x))
#define S_HAS_NO_STICKY_STIPPLED_ICON_TITLE(c) \
	((c).s.has_no_sticky_stippled_icon_title)
#define S_SET_HAS_NO_STICKY_STIPPLED_ICON_TITLE(c,x) \
	((c).s.has_no_sticky_stippled_icon_title = !!(x))
#define S_HAS_NO_ICON_TITLE(c) \
	((c).s.has_no_icon_title)
#define S_SET_HAS_NO_ICON_TITLE(c,x) \
	((c).s.has_no_icon_title = !!(x))
#define S_HAS_OVERRIDE_SIZE(c) \
	((c).s.has_override_size)
#define S_SET_HAS_OVERRIDE_SIZE(c,x) \
	((c).s.has_override_size = !!(x))
#define S_HAS_STIPPLED_TITLE(c) \
	((c).s.has_stippled_title)
#define S_SET_HAS_STIPPLED_TITLE(c,x) \
	((c).s.has_stippled_title = !!(x))
#define S_HAS_NO_STICKY_STIPPLED_TITLE(c) \
	((c).s.has_no_sticky_stippled_title)
#define S_SET_HAS_NO_STICKY_STIPPLED_TITLE(c,x) \
	((c).s.has_no_sticky_stippled_title = !!(x))
#define S_HAS_STIPPLED_ICON_TITLE(c) \
	((c).s.has_stippled_icon_title)
#define S_SET_HAS_STIPPLED_ICON_TITLE(c,x) \
	((c).s.has_stippled_icon_title = !!(x))
#define S_HAS_WINDOW_FONT(c) \
	((c).has_window_font)
#define S_SET_HAS_WINDOW_FONT(c,x) \
	((c).has_window_font = !!(x))
#define S_ICON_OVERRIDE(c) \
	((c).s.icon_override)
#define S_SET_ICON_OVERRIDE(c,x) \
	((c).s.icon_override = (x))
#define S_IS_BOTTOM_TITLE_ROTATED(c) \
	((c).s.is_bottom_title_rotated)
#define S_SET_IS_BOTTOM_TITLE_ROTATED(c,x) \
	((c).s.is_bottom_title_rotated = !!(x))
#define S_IS_FIXED(c) \
	((c).s.is_fixed)
#define S_SET_IS_FIXED(c,x) \
	((c).s.is_fixed = !!(x))
#define S_IS_FIXED_PPOS(c) \
	((c).s.is_fixed_ppos)
#define S_SET_IS_FIXED_PPOS(c,x) \
	((c).s.is_fixed_ppos = !!(x))
#define S_SET_IS_UNICONIFIABLE(c,x) \
	((c).s.is_uniconifiable = !!(x))
#define S_SET_IS_UNMAXIMIZABLE(c,x) \
	((c).s.is_unmaximizable = !!(x))
#define S_SET_IS_UNCLOSABLE(c,x) \
	((c).s.is_unclosable = !!(x))
#define S_SET_MAXIMIZE_FIXED_SIZE_DISALLOWED(c,x) \
	((c).s.is_maximize_fixed_size_disallowed = !!(x))
#define S_IS_ICON_STICKY_ACROSS_PAGES(c) \
	((c).s.is_icon_sticky_across_pages)
#define S_SET_IS_ICON_STICKY_ACROSS_PAGES(c,x) \
	((c).s.is_icon_sticky_across_pages = !!(x))
#define S_IS_ICON_STICKY_ACROSS_DESKS(c) \
	((c).s.is_icon_sticky_across_desks)
#define S_SET_IS_ICON_STICKY_ACROSS_DESKS(c,x) \
	((c).s.is_icon_sticky_across_desks = !!(x))
#define S_IS_ICON_SUPPRESSED(c) \
	((c).s.is_icon_suppressed)
#define S_SET_IS_ICON_SUPPRESSED(c,x) \
	((c).s.is_icon_suppressed = !!(x))
#define S_IS_LEFT_TITLE_ROTATED_CW(c) \
	((c).s.is_left_title_rotated_cw)
#define S_SET_IS_LEFT_TITLE_ROTATED_CW(c,x) \
	((c).s.is_left_title_rotated_cw = !!(x))
#define S_IS_SIZE_FIXED(c) \
	((c).s.is_size_fixed)
#define S_SET_IS_SIZE_FIXED(c,x) \
	((c).s.is_size_fixed = !!(x))
#define S_IS_PSIZE_FIXED(c) \
	((c).s.is_psize_fixed)
#define S_SET_IS_PSIZE_FIXED(c,x) \
	((c).s.is_psize_fixed = !!(x))
#define S_IS_RIGHT_TITLE_ROTATED_CW(c) \
	((c).s.is_right_title_rotated_cw)
#define S_SET_IS_RIGHT_TITLE_ROTATED_CW(c,x) \
	((c).s.is_right_title_rotated_cw = !!(x))
#define S_IS_TOP_TITLE_ROTATED(c) \
	((c).s.is_top_title_rotated)
#define S_SET_IS_TOP_TITLE_ROTATED(c,x) \
	((c).s.is_top_title_rotated = !!(x))
#define S_USE_ICON_POSITION_HINT(c) \
	((c).s.use_icon_position_hint)
#define S_SET_USE_ICON_POSITION_HINT(c,x) \
	((c).s.use_icon_position_hint = !!(x))
#define S_USE_INDEXED_WINDOW_NAME(c) \
	((c).s.use_indexed_window_name)
#define S_SET_USE_INDEXED_WINDOW_NAME(c,x) \
	((c).s.use_indexed_window_name = !!(x))
#define S_USE_INDEXED_ICON_NAME(c) \
	((c).s.use_indexed_icon_name)
#define S_SET_USE_INDEXED_ICON_NAME(c,x) \
	((c).s.use_indexed_icon_name = !!(x))
#define S_WINDOWSHADE_LAZINESS(c) \
	((c).s.windowshade_laziness)
#define S_SET_WINDOWSHADE_LAZINESS(c,x) \
	((c).s.windowshade_laziness = (x))
#define S_USE_TITLE_DECOR_ROTATION(c) \
	((c).s.use_title_decor_rotation)
#define S_SET_USE_TITLE_DECOR_ROTATION(c,x) \
	((c).s.use_title_decor_rotation = !!(x))
#define S_DO_EWMH_MINI_ICON_OVERRIDE(c) \
	((c).s.do_ewmh_mini_icon_override)
#define S_SET_DO_EWMH_MINI_ICON_OVERRIDE(c,x) \
	((c).s.do_ewmh_mini_icon_override = !!(x))
#define S_DO_EWMH_DONATE_ICON(c) \
	((c).s.do_ewmh_donate_icon)
#define S_SET_DO_EWMH_DONATE_ICON(c,x) \
	((c).s.do_ewmh_donate_icon = !!(x))
#define S_DO_EWMH_DONATE_MINI_ICON(c) \
	((c).s.do_ewmh_donate_mini_icon)
#define S_SET_DO_EWMH_DONATE_MINI_ICON(c,x) \
	((c).s.do_ewmh_donate_mini_icon = !!(x))
#define S_DO_EWMH_USE_STACKING_HINTS(c) \
	((c).s.do_ewmh_use_stacking_hints)
#define S_SET_DO_EWMH_USE_STACKING_HINTS(c,x) \
	((c).s.do_ewmh_use_stacking_hints = !!(x))
#define S_DO_EWMH_IGNORE_STRUT_HINTS(c) \
	((c).s.do_ewmh_ignore_strut_hints)
#define S_SET_DO_EWMH_IGNORE_STRUT_HINTS(c,x) \
	((c).s.do_ewmh_ignore_strut_hints = !!(x))
#define S_DO_EWMH_IGNORE_STATE_HINTS(c) \
	((c).s.do_ewmh_ignore_state_hints)
#define S_SET_DO_EWMH_IGNORE_STATE_HINTS(c,x) \
	((c).s.do_ewmh_ignore_state_hints = !!(x))
#define S_DO_EWMH_IGNORE_WINDOW_TYPE(c) \
	((c).s.do_ewmh_ignore_window_type)
#define S_SET_DO_EWMH_IGNORE_WINDOW_TYPE(c,x) \
	((c).s.do_ewmh_ignore_window_type = !!(x))
#define S_EWMH_MAXIMIZE_MODE(c) \
	((c).s.ewmh_maximize_mode)
#define S_SET_EWMH_MAXIMIZE_MODE(c,x) \
	((c).s.ewmh_maximize_mode = (x))

/* access to style_id */
#define SID_GET_NAME(id) \
	((id).name)
#define SID_SET_NAME(id,x) \
	((id).name = (x))
#define SID_GET_WINDOW_ID(id) \
	((id).window_id)
#define SID_SET_WINDOW_ID(id,x) \
	((id).window_id = (x))
#define SID_SET_HAS_NAME(id,x) \
	((id).flags.has_name = !!(x))
#define SID_GET_HAS_NAME(id) \
	((id).flags.has_name)
#define SID_SET_HAS_WINDOW_ID(id,x) \
	((id).flags.has_window_id = !!(x))
#define SID_GET_HAS_WINDOW_ID(id) \
	((id).flags.has_window_id)

/* access to other parts of a style (call with the style itself) */
#define SGET_NEXT_STYLE(s) \
	((s).next)
#define SSET_NEXT_STYLE(s,x) \
	((s).next = (x))
#define SGET_PREV_STYLE(s) \
	((s).prev)
#define SSET_PREV_STYLE(s,x) \
	((s).prev = (x))
#define SGET_ID(s) \
	((s).id)
#define SGET_NAME(s) \
	SID_GET_NAME(SGET_ID(s))
#define SSET_NAME(s,x) \
	SID_SET_NAME(SGET_ID(s),x)
#define SGET_WINDOW_ID(s) \
	SID_GET_WINDOW_ID(SGET_ID(s))
#define SSET_WINDOW_ID(s,x) \
	SID_SET_WINDOW_ID(SGET_ID(s),x)
#define SSET_ID_HAS_NAME(s,x) \
        SID_SET_HAS_NAME(SGET_ID(s), x)
#define SGET_ID_HAS_NAME(s) \
	SID_GET_HAS_NAME(SGET_ID(s))
#define SSET_ID_HAS_WINDOW_ID(s,x) \
	SID_SET_HAS_WINDOW_ID(SGET_ID(s),x)
#define SGET_ID_HAS_WINDOW_ID(s) \
	SID_GET_HAS_WINDOW_ID(SGET_ID(s))
#define SGET_ICON_NAME(s) \
	((s).icon_name)
#define SSET_ICON_NAME(s,x) \
	((s).icon_name = (x))
#define SGET_MINI_ICON_NAME(s) \
	((s).mini_icon_name)
#define SSET_MINI_ICON_NAME(s,x) \
	((s).mini_icon_name = (x))
#define SGET_DECOR_NAME(s) \
	((s).decor_name)
#define SSET_DECOR_NAME(s,x) \
	((s).decor_name = (x))
#define SGET_FORE_COLOR_NAME(s) \
	((s).fore_color_name)
#define SSET_FORE_COLOR_NAME(s,x) \
	((s).fore_color_name = (x))
#define SGET_BACK_COLOR_NAME(s) \
	((s).back_color_name)
#define SSET_BACK_COLOR_NAME(s,x) \
	((s).back_color_name = (x))
#define SGET_FORE_COLOR_NAME_HI(s) \
	((s).fore_color_name_hi)
#define SSET_FORE_COLOR_NAME_HI(s,x) \
	((s).fore_color_name_hi = (x))
#define SGET_BACK_COLOR_NAME_HI(s) \
	((s).back_color_name_hi)
#define SSET_BACK_COLOR_NAME_HI(s,x) \
	((s).back_color_name_hi = (x))
#define SGET_ICON_FONT(s) \
	((s).icon_font)
#define SSET_ICON_FONT(s,x) \
	((s).icon_font = (x))
#define SGET_WINDOW_FONT(s) \
	((s).window_font)
#define SSET_WINDOW_FONT(s,x) \
	((s).window_font = (x))
#define SGET_COLORSET(s) \
	((s).colorset)
#define SSET_COLORSET(s,x) \
	((s).colorset = (x))
#define SSET_BORDER_COLORSET(s,x) \
	((s).border_colorset = (x))
#define SGET_BORDER_COLORSET(s) \
	((s).border_colorset)
#define SGET_COLORSET_HI(s) \
	((s).colorset_hi)
#define SSET_COLORSET_HI(s,x) \
	((s).colorset_hi = (x))
#define SGET_BORDER_COLORSET_HI(s) \
	((s).border_colorset_hi)
#define SSET_BORDER_COLORSET_HI(s,x) \
	((s).border_colorset_hi = (x))
#define SSET_ICON_TITLE_COLORSET(s,x) \
	((s).icon_title_colorset = (x))
#define SGET_ICON_TITLE_COLORSET(s) \
	((s).icon_title_colorset)
#define SSET_ICON_TITLE_COLORSET_HI(s,x) \
	((s).icon_title_colorset_hi = (x))
#define SGET_ICON_TITLE_COLORSET_HI(s) \
	((s).icon_title_colorset_hi)
#define SSET_ICON_BACKGROUND_COLORSET(s,x) \
	((s).icon_background_colorset = (x))
#define SGET_ICON_BACKGROUND_COLORSET(s) \
	((s).icon_background_colorset)
#define SGET_FLAGS_POINTER(s) \
	(&((s).flags))
#define SGET_BORDER_WIDTH(s) \
	((s).border_width)
#define SSET_BORDER_WIDTH(s,x) \
	((s).border_width = (x))
#define SGET_HANDLE_WIDTH(s) \
	((s).handle_width)
#define SSET_HANDLE_WIDTH(s,x) \
	((s).handle_width = (x))
#define SGET_LAYER(s) \
	((s).layer)
#define SSET_LAYER(s,x) \
	((s).layer = (x))
#define SGET_START_DESK(s) \
	((s).start_desk)
#define SSET_START_DESK(s,x) \
	((s).start_desk = (x))
#define SGET_START_PAGE_X(s) \
	((s).start_page_x)
#define SSET_START_PAGE_X(s,x) \
	((s).start_page_x = (x))
#define SGET_START_PAGE_Y(s) \
	((s).start_page_y)
#define SSET_START_PAGE_Y(s,x) \
	((s).start_page_y = (x))
#define SGET_START_SCREEN(s) \
	((s).start_screen)
#define SSET_START_SCREEN(s,x) \
	((s).start_screen = (x))
#define SSET_STARTS_SHADED_DIR(s,x) \
	((s).flags.start_shaded_dir = (x))
#define SGET_STARTS_SHADED_DIR(s)		\
	((s).flags.start_shaded_dir)
#define SGET_MIN_ICON_WIDTH(s) \
	((s).min_icon_width)
#define SSET_MIN_ICON_WIDTH(s,x) \
	((s).min_icon_width = (x))
#define SGET_MIN_ICON_HEIGHT(s) \
	((s).min_icon_height)
#define SSET_MIN_ICON_HEIGHT(s,x) \
	((s).min_icon_height = (x))
#define SGET_MAX_ICON_WIDTH(s) \
	((s).max_icon_width)
#define SSET_MAX_ICON_WIDTH(s,x) \
	((s).max_icon_width = (x))
#define SGET_MAX_ICON_HEIGHT(s) \
	((s).max_icon_height)
#define SSET_MAX_ICON_HEIGHT(s,x) \
	((s).max_icon_height = (x))
#define SGET_ICON_RESIZE_TYPE(s) \
	((s).icon_resize_type)
#define SSET_ICON_RESIZE_TYPE(s,x) \
	((s).icon_resize_type = (x))
#define SGET_ICON_BACKGROUND_RELIEF(s) \
	((s).icon_background_relief)
#define SSET_ICON_BACKGROUND_RELIEF(s,x) \
	((s).icon_background_relief = (x))
#define SGET_ICON_BACKGROUND_PADDING(s) \
	((s).icon_background_padding)
#define SSET_ICON_BACKGROUND_PADDING(s,x) \
	((s).icon_background_padding = (x))
#define SGET_ICON_TITLE_RELIEF(s) \
	((s).icon_title_relief)
#define SSET_ICON_TITLE_RELIEF(s,x) \
	((s).icon_title_relief = (x))
#define SGET_MIN_WINDOW_WIDTH(s) \
	((s).min_window_width)
#define SSET_MIN_WINDOW_WIDTH(s,x) \
	((s).min_window_width = (x))
#define SGET_MAX_WINDOW_WIDTH(s) \
	((s).max_window_width)
#define SSET_MAX_WINDOW_WIDTH(s,x) \
	((s).max_window_width = (x))
#define SGET_MIN_WINDOW_HEIGHT(s) \
	((s).min_window_height)
#define SSET_MIN_WINDOW_HEIGHT(s,x) \
	((s).min_window_height = (x))
#define SGET_MAX_WINDOW_HEIGHT(s) \
	((s).max_window_height)
#define SSET_MAX_WINDOW_HEIGHT(s,x) \
	((s).max_window_height = (x))
#define SGET_WINDOW_SHADE_STEPS(s) \
	((s).shade_anim_steps)
#define SSET_WINDOW_SHADE_STEPS(s,x) \
	((s).shade_anim_steps = (x))
#define SGET_SNAP_PROXIMITY(s) \
	((s).snap_attraction.proximity)
#define SSET_SNAP_PROXIMITY(s,x) \
	((s).snap_attraction.proximity = (x))
#define SGET_SNAP_MODE(s) \
	((s).snap_attraction.mode)
#define SSET_SNAP_MODE(s,x) \
	((s).snap_attraction.mode = (x))
#define SGET_SNAP_GRID_X(s) \
	((s).snap_grid_x)
#define SSET_SNAP_GRID_X(s,x) \
	((s).snap_grid_x = (x))
#define SGET_SNAP_GRID_Y(s) \
	((s).snap_grid_y)
#define SSET_SNAP_GRID_Y(s,x) \
	((s).snap_grid_y = (x))
#define SGET_EDGE_DELAY_MS_MOVE(s) \
	((s).edge_delay_ms_move)
#define SSET_EDGE_DELAY_MS_MOVE(s,x) \
	((s).edge_delay_ms_move = (x))
#define SGET_EDGE_DELAY_MS_RESIZE(s) \
	((s).edge_delay_ms_resize)
#define SSET_EDGE_DELAY_MS_RESIZE(s,x) \
	((s).edge_delay_ms_resize = (x))
#define SGET_EDGE_RESISTANCE_MOVE(s) \
	((s).edge_resistance_move)
#define SSET_EDGE_RESISTANCE_MOVE(s,x) \
	((s).edge_resistance_move = (x))
#define SGET_EDGE_RESISTANCE_XINERAMA_MOVE(s) \
	((s).edge_resistance_xinerama_move)
#define SSET_EDGE_RESISTANCE_XINERAMA_MOVE(s,x) \
	((s).edge_resistance_xinerama_move = (x))
#define SGET_ICON_BOXES(s) \
	((s).icon_boxes)
#define SSET_ICON_BOXES(s,x) \
	((s).icon_boxes = (x))
#define SGET_PLACEMENT_PENALTY_PTR(s) \
	(&(s).pl_penalty)
#define SGET_NORMAL_PLACEMENT_PENALTY(s) \
	((s).pl_penalty.normal)
#define SGET_ONTOP_PLACEMENT_PENALTY(s) \
	((s).pl_penalty.ontop)
#define SGET_ICON_PLACEMENT_PENALTY(s) \
	((s).pl_penalty.icon)
#define SGET_STICKY_PLACEMENT_PENALTY(s) \
	((s).pl_penalty.sticky)
#define SGET_BELOW_PLACEMENT_PENALTY(s) \
	((s).pl_penalty.below)
#define SGET_EWMH_STRUT_PLACEMENT_PENALTY(s) \
	((s).pl_penalty.strut)
#define SSET_NORMAL_PLACEMENT_PENALTY(s,x) \
	((s).pl_penalty.normal = (x))
#define SSET_ONTOP_PLACEMENT_PENALTY(s,x) \
	((s).pl_penalty.ontop = (x))
#define SSET_ICON_PLACEMENT_PENALTY(s,x) \
	((s).pl_penalty.icon = (x))
#define SSET_STICKY_PLACEMENT_PENALTY(s,x) \
	((s).pl_penalty.sticky = (x))
#define SSET_BELOW_PLACEMENT_PENALTY(s,x) \
	((s).pl_penalty.below = (x))
#define SSET_EWMH_STRUT_PLACEMENT_PENALTY(s,x) \
	((s).pl_penalty.strut = (x))
#define SGET_PLACEMENT_PERCENTAGE_PENALTY_PTR(s) \
	(&(s).pl_percent_penalty)
#define SGET_99_PLACEMENT_PERCENTAGE_PENALTY(s) \
	((s).pl_percent_penalty.p99)
#define SGET_95_PLACEMENT_PERCENTAGE_PENALTY(s) \
	((s).pl_percent_penalty.p95)
#define SGET_85_PLACEMENT_PERCENTAGE_PENALTY(s) \
	((s).pl_percent_penalty.p85)
#define SGET_75_PLACEMENT_PERCENTAGE_PENALTY(s) \
	((s).pl_percent_penalty.p75)
#define SSET_99_PLACEMENT_PERCENTAGE_PENALTY(s,x) \
	((s).pl_percent_penalty.p99 = (x))
#define SSET_95_PLACEMENT_PERCENTAGE_PENALTY(s,x) \
	((s).pl_percent_penalty.p95 = (x))
#define SSET_85_PLACEMENT_PERCENTAGE_PENALTY(s,x) \
	((s).pl_percent_penalty.p85 = (x))
#define SSET_75_PLACEMENT_PERCENTAGE_PENALTY(s,x) \
	((s).pl_percent_penalty.p75 = (x))
#define SGET_PLACEMENT_POSITION_STRING(s) \
	((s).pl_position_string)
#define SSET_PLACEMENT_POSITION_STRING(s,x)	\
	((s).pl_position_string = (x))
#define SGET_INITIAL_MAP_COMMAND_STRING(s) \
	((s).initial_map_command_string)
#define SSET_INITIAL_MAP_COMMAND_STRING(s,x)	\
	((s).initial_map_command_string = (x))
#define SGET_TITLE_FORMAT_STRING(s) \
	((s).title_format_string)
#define SSET_TITLE_FORMAT_STRING(s,x) \
	((s).title_format_string = (x))
#define SGET_ICON_TITLE_FORMAT_STRING(s) \
	((s).icon_title_format_string)
#define SSET_ICON_TITLE_FORMAT_STRING(s,x) \
	((s).icon_title_format_string = (x))

/* function prototypes */
void lookup_style(FvwmWindow *fw, window_style *styles);
Bool blockcmpmask(char *blk1, char *blk2, char *mask, int length);
void check_window_style_change(
	FvwmWindow *t, update_win *flags, window_style *ret_style);
void reset_style_changes(void);
void update_style_colorset(int colorset);
void update_window_color_style(FvwmWindow *fw, window_style *style);
void update_window_color_hi_style(FvwmWindow *fw, window_style *style);
void update_icon_title_cs_style(FvwmWindow *fw, window_style *pstyle);
void update_icon_title_cs_hi_style(FvwmWindow *fw, window_style *pstyle);
void update_icon_background_cs_style(FvwmWindow *fw, window_style *pstyle);
void free_icon_boxes(icon_boxes *ib);
void style_destroy_style(style_id_t s_id);
void print_styles(int verbose);

#endif /* _STYLE_ */
