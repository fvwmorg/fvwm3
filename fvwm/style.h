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

#ifndef _STYLE_
#define _STYLE_

/* access to the special flags of a style */
/* call these with a pointer to a style_flags struct */
#define SDO_DECORATE_TRANSIENT(sf)    ((sf)->do_decorate_transient)
#define SDO_PLACE_RANDOM(sf)          ((sf)->do_place_random)
#define SDO_PLACE_SMART(sf)           ((sf)->do_place_smart)
#define SDO_SAVE_UNDER(sf)            ((sf)->do_save_under)
#define SDO_START_LOWERED(sf)         ((sf)->do_start_lowered)
#define SHAS_BORDER_WIDTH(sf)         ((sf)->has_border_width)
#define SHAS_COLOR_BACK(sf)           ((sf)->has_color_back)
#define SHAS_COLOR_FORE(sf)           ((sf)->has_color_fore)
#define SHAS_HANDLE_WIDTH(sf)         ((sf)->has_handle_width)
#define SHAS_ICON(sf)                 ((sf)->has_icon)
#define SHAS_ICON_BOXES(sf)           ((sf)->has_icon_boxes)
#define SHAS_MAX_WINDOW_SIZE(sf)      ((sf)->has_max_window_size)
#ifdef MINI_ICONS
#define SHAS_MINI_ICON(sf)            ((sf)->has_mini_icon)
#endif
#define SHAS_MWM_DECOR(sf)            ((sf)->has_mwm_decor)
#define SHAS_MWM_FUNCTIONS(sf)        ((sf)->has_mwm_functions)
#define SHAS_NO_BORDER(sf)            ((sf)->has_no_border)
#define SHAS_NO_TITLE(sf)             ((sf)->has_no_title)
#define SHAS_OL_DECOR(sf)             ((sf)->has_ol_decor)
#define SIS_BUTTON_DISABLED(sf)       ((sf)->is_button_disabled)
#define SUSE_BACKING_STORE(sf)        ((sf)->use_backing_store)
#define SUSE_COLORSET(sf)             ((sf)->use_colorset)
#define SUSE_LAYER(sf)                ((sf)->use_layer)
#define SUSE_NO_PPOSITION(sf)         ((sf)->use_no_pposition)
#define SUSE_START_ON_DESK(sf)        ((sf)->use_start_on_desk)
#define SUSE_START_ON_PAGE_FOR_TRANSIENT(sf) \
  ((sf)->use_start_on_page_for_transient)
#define SACTIVE_PLACEMENT_HONORS_STARTS_ON_PAGE(sf) \
  ((sf)->active_placement_honors_starts_on_page)
#define SCAPTURE_HONORS_STARTS_ON_PAGE(sf) \
  ((sf)->capture_honors_starts_on_page)
#define SRECAPTURE_HONORS_STARTS_ON_PAGE(sf) \
  ((sf)->recapture_honors_starts_on_page)
#define SICON_OVERRIDE(sf)            ((sf)->icon_override)








/* access to common flags */
#define SFGET_COMMON_FLAGS(st) ((st).flags.common)
#define SFGET_COMMON_STATIC_FLAGS(st) ((st).flags.common.s)
#define SMGET_COMMON_FLAGS(st) ((st).flag_mask.common)
#define SMGET_COMMON_STATIC_FLAGS(st) ((st).flag_mask.common.s)
#define SCGET_COMMON_FLAGS(st) ((st).change_mask.common)
#define SCGET_COMMON_STATIC_FLAGS(st) ((st).change_mask.common.s)
#define SHAS_BOTTOM_TITLE(sf) ((sf).common.s.has_bottom_title)
#define SFHAS_BOTTOM_TITLE(st) ((st).flags.common.s.has_bottom_title)
#define SMHAS_BOTTOM_TITLE(st) ((st).flag_mask.common.s.has_bottom_title)
#define SCHAS_BOTTOM_TITLE(st) ((st).change_mask.common.s.has_bottom_title)
#define SFSET_HAS_BOTTOM_TITLE(st,x) \
          ((st).flags.common.s.has_bottom_title = !!(x))
#define SMSET_HAS_BOTTOM_TITLE(st,x) \
          ((st).flag_mask.common.s.has_bottom_title = !!(x))
#define SCSET_HAS_BOTTOM_TITLE(st,x) \
          ((st).change_mask.common.s.has_bottom_title = !!(x))
#define SIS_STICKY(sf) ((sf).common.is_sticky)
#define SFIS_STICKY(st) ((st).flags.common.is_sticky)
#define SMIS_STICKY(st) ((st).flag_mask.common.is_sticky)
#define SCIS_STICKY(st) ((st).change_mask.common.is_sticky)
#define SFSET_IS_STICKY(st,x) ((st).flags.common.is_sticky = !!(x))
#define SMSET_IS_STICKY(st,x) ((st).flag_mask.common.is_sticky = !!(x))
#define SCSET_IS_STICKY(st,x) ((st).change_mask.common.is_sticky = !!(x))
#define SDO_CIRCULATE_SKIP(sf) ((sf).common.s.do_circulate_skip)
#define SFDO_CIRCULATE_SKIP(st) ((st).flags.common.s.do_circulate_skip)
#define SMDO_CIRCULATE_SKIP(st) ((st).flag_mask.common.s.do_circulate_skip)
#define SCDO_CIRCULATE_SKIP(st) ((st).change_mask.common.s.do_circulate_skip)
#define SFSET_DO_CIRCULATE_SKIP(st,x) ((st).flags.common.s.do_circulate_skip = !!(x))
#define SMSET_DO_CIRCULATE_SKIP(st,x) ((st).flag_mask.common.s.do_circulate_skip = !!(x))
#define SCSET_DO_CIRCULATE_SKIP(st,x) ((st).change_mask.common.s.do_circulate_skip = !!(x))
#define SDO_CIRCULATE_SKIP_ICON(sf) ((sf).common.s.do_circulate_skip_icon)
#define SFDO_CIRCULATE_SKIP_ICON(st) ((st).flags.common.s.do_circulate_skip_icon)
#define SMDO_CIRCULATE_SKIP_ICON(st) ((st).flag_mask.common.s.do_circulate_skip_icon)
#define SCDO_CIRCULATE_SKIP_ICON(st) ((st).change_mask.common.s.do_circulate_skip_icon)
#define SFSET_DO_CIRCULATE_SKIP_ICON(st,x) ((st).flags.common.s.do_circulate_skip_icon = !!(x))
#define SMSET_DO_CIRCULATE_SKIP_ICON(st,x) ((st).flag_mask.common.s.do_circulate_skip_icon = !!(x))
#define SCSET_DO_CIRCULATE_SKIP_ICON(st,x) ((st).change_mask.common.s.do_circulate_skip_icon = !!(x))
#define SDO_CIRCULATE_SKIP_SHADED(sf) ((sf).common.s.do_circulate_skip_shaded)
#define SFDO_CIRCULATE_SKIP_SHADED(st) ((st).flags.common.s.do_circulate_skip_shaded)
#define SMDO_CIRCULATE_SKIP_SHADED(st) ((st).flag_mask.common.s.do_circulate_skip_shaded)
#define SCDO_CIRCULATE_SKIP_SHADED(st) ((st).change_mask.common.s.do_circulate_skip_shaded)
#define SFSET_DO_CIRCULATE_SKIP_SHADED(st,x) ((st).flags.common.s.do_circulate_skip_shaded = !!(x))
#define SMSET_DO_CIRCULATE_SKIP_SHADED(st,x) ((st).flag_mask.common.s.do_circulate_skip_shaded = !!(x))
#define SCSET_DO_CIRCULATE_SKIP_SHADED(st,x) ((st).change_mask.common.s.do_circulate_skip_shaded = !!(x))
#define SDO_FLIP_TRANSIENT(sf) ((sf).common.s.do_flip_transient)
#define SFDO_FLIP_TRANSIENT(st) ((st).flags.common.s.do_flip_transient)
#define SMDO_FLIP_TRANSIENT(st) ((st).flag_mask.common.s.do_flip_transient)
#define SCDO_FLIP_TRANSIENT(st) ((st).change_mask.common.s.do_flip_transient)
#define SFSET_DO_FLIP_TRANSIENT(st,x) ((st).flags.common.s.do_flip_transient = !!(x))
#define SMSET_DO_FLIP_TRANSIENT(st,x) ((st).flag_mask.common.s.do_flip_transient = !!(x))
#define SCSET_DO_FLIP_TRANSIENT(st,x) ((st).change_mask.common.s.do_flip_transient = !!(x))
#define SDO_GRAB_FOCUS_WHEN_CREATED(sf) ((sf).common.s.do_grab_focus_when_created)
#define SFDO_GRAB_FOCUS_WHEN_CREATED(st) ((st).flags.common.s.do_grab_focus_when_created)
#define SMDO_GRAB_FOCUS_WHEN_CREATED(st) ((st).flag_mask.common.s.do_grab_focus_when_created)
#define SCDO_GRAB_FOCUS_WHEN_CREATED(st) ((st).change_mask.common.s.do_grab_focus_when_created)
#define SFSET_DO_GRAB_FOCUS_WHEN_CREATED(st,x) ((st).flags.common.s.do_grab_focus_when_created = !!(x))
#define SMSET_DO_GRAB_FOCUS_WHEN_CREATED(st,x) ((st).flag_mask.common.s.do_grab_focus_when_created = !!(x))
#define SCSET_DO_GRAB_FOCUS_WHEN_CREATED(st,x) ((st).change_mask.common.s.do_grab_focus_when_created = !!(x))
#define SDO_GRAB_FOCUS_WHEN_TRANSIENT_CREATED(sf) ((sf).common.s.do_grab_focus_when_transient_created)
#define SFDO_GRAB_FOCUS_WHEN_TRANSIENT_CREATED(st) ((st).flags.common.s.do_grab_focus_when_transient_created)
#define SMDO_GRAB_FOCUS_WHEN_TRANSIENT_CREATED(st) ((st).flag_mask.common.s.do_grab_focus_when_transient_created)
#define SCDO_GRAB_FOCUS_WHEN_TRANSIENT_CREATED(st) ((st).change_mask.common.s.do_grab_focus_when_transient_created)
#define SFSET_DO_GRAB_FOCUS_WHEN_TRANSIENT_CREATED(st,x) ((st).flags.common.s.do_grab_focus_when_transient_created = !!(x))
#define SMSET_DO_GRAB_FOCUS_WHEN_TRANSIENT_CREATED(st,x) ((st).flag_mask.common.s.do_grab_focus_when_transient_created = !!(x))
#define SCSET_DO_GRAB_FOCUS_WHEN_TRANSIENT_CREATED(st,x) ((st).change_mask.common.s.do_grab_focus_when_transient_created = !!(x))
#define SDO_IGNORE_RESTACK(sf) ((sf).common.s.do_ignore_restack)
#define SFDO_IGNORE_RESTACK(st) ((st).flags.common.s.do_ignore_restack)
#define SMDO_IGNORE_RESTACK(st) ((st).flag_mask.common.s.do_ignore_restack)
#define SCDO_IGNORE_RESTACK(st) ((st).change_mask.common.s.do_ignore_restack)
#define SFSET_DO_IGNORE_RESTACK(st,x) ((st).flags.common.s.do_ignore_restack = !!(x))
#define SMSET_DO_IGNORE_RESTACK(st,x) ((st).flag_mask.common.s.do_ignore_restack = !!(x))
#define SCSET_DO_IGNORE_RESTACK(st,x) ((st).change_mask.common.s.do_ignore_restack = !!(x))
#define SDO_LOWER_TRANSIENT(sf) ((sf).common.s.do_lower_transient)
#define SFDO_LOWER_TRANSIENT(st) ((st).flags.common.s.do_lower_transient)
#define SMDO_LOWER_TRANSIENT(st) ((st).flag_mask.common.s.do_lower_transient)
#define SCDO_LOWER_TRANSIENT(st) ((st).change_mask.common.s.do_lower_transient)
#define SFSET_DO_LOWER_TRANSIENT(st,x) ((st).flags.common.s.do_lower_transient = !!(x))
#define SMSET_DO_LOWER_TRANSIENT(st,x) ((st).flag_mask.common.s.do_lower_transient = !!(x))
#define SCSET_DO_LOWER_TRANSIENT(st,x) ((st).change_mask.common.s.do_lower_transient = !!(x))
#define SDO_NOT_SHOW_ON_MAP(sf) ((sf).common.s.do_not_show_on_map)
#define SFDO_NOT_SHOW_ON_MAP(st) ((st).flags.common.s.do_not_show_on_map)
#define SMDO_NOT_SHOW_ON_MAP(st) ((st).flag_mask.common.s.do_not_show_on_map)
#define SCDO_NOT_SHOW_ON_MAP(st) ((st).change_mask.common.s.do_not_show_on_map)
#define SFSET_DO_NOT_SHOW_ON_MAP(st,x) ((st).flags.common.s.do_not_show_on_map = !!(x))
#define SMSET_DO_NOT_SHOW_ON_MAP(st,x) ((st).flag_mask.common.s.do_not_show_on_map = !!(x))
#define SCSET_DO_NOT_SHOW_ON_MAP(st,x) ((st).change_mask.common.s.do_not_show_on_map = !!(x))
#define SDO_RAISE_TRANSIENT(sf) ((sf).common.s.do_raise_transient)
#define SFDO_RAISE_TRANSIENT(st) ((st).flags.common.s.do_raise_transient)
#define SMDO_RAISE_TRANSIENT(st) ((st).flag_mask.common.s.do_raise_transient)
#define SCDO_RAISE_TRANSIENT(st) ((st).change_mask.common.s.do_raise_transient)
#define SFSET_DO_RAISE_TRANSIENT(st,x) ((st).flags.common.s.do_raise_transient = !!(x))
#define SMSET_DO_RAISE_TRANSIENT(st,x) ((st).flag_mask.common.s.do_raise_transient = !!(x))
#define SCSET_DO_RAISE_TRANSIENT(st,x) ((st).change_mask.common.s.do_raise_transient = !!(x))
#define SDO_RESIZE_OPAQUE(sf) ((sf).common.s.do_resize_opaque)
#define SFDO_RESIZE_OPAQUE(st) ((st).flags.common.s.do_resize_opaque)
#define SMDO_RESIZE_OPAQUE(st) ((st).flag_mask.common.s.do_resize_opaque)
#define SCDO_RESIZE_OPAQUE(st) ((st).change_mask.common.s.do_resize_opaque)
#define SFSET_DO_RESIZE_OPAQUE(st,x) ((st).flags.common.s.do_resize_opaque = !!(x))
#define SMSET_DO_RESIZE_OPAQUE(st,x) ((st).flag_mask.common.s.do_resize_opaque = !!(x))
#define SCSET_DO_RESIZE_OPAQUE(st,x) ((st).change_mask.common.s.do_resize_opaque = !!(x))
#define SDO_STACK_TRANSIENT_PARENT(sf) ((sf).common.s.do_stack_transient_parent)
#define SFDO_STACK_TRANSIENT_PARENT(st) ((st).flags.common.s.do_stack_transient_parent)
#define SMDO_STACK_TRANSIENT_PARENT(st) ((st).flag_mask.common.s.do_stack_transient_parent)
#define SCDO_STACK_TRANSIENT_PARENT(st) ((st).change_mask.common.s.do_stack_transient_parent)
#define SFSET_DO_STACK_TRANSIENT_PARENT(st,x) ((st).flags.common.s.do_stack_transient_parent = !!(x))
#define SMSET_DO_STACK_TRANSIENT_PARENT(st,x) ((st).flag_mask.common.s.do_stack_transient_parent = !!(x))
#define SCSET_DO_STACK_TRANSIENT_PARENT(st,x) ((st).change_mask.common.s.do_stack_transient_parent = !!(x))
#define SDO_START_ICONIC(sf) ((sf).common.s.do_start_iconic)
#define SFDO_START_ICONIC(st) ((st).flags.common.s.do_start_iconic)
#define SMDO_START_ICONIC(st) ((st).flag_mask.common.s.do_start_iconic)
#define SCDO_START_ICONIC(st) ((st).change_mask.common.s.do_start_iconic)
#define SFSET_DO_START_ICONIC(st,x) ((st).flags.common.s.do_start_iconic = !!(x))
#define SMSET_DO_START_ICONIC(st,x) ((st).flag_mask.common.s.do_start_iconic = !!(x))
#define SCSET_DO_START_ICONIC(st,x) ((st).change_mask.common.s.do_start_iconic = !!(x))
#define SDO_WINDOW_LIST_SKIP(sf) ((sf).common.s.do_window_list_skip)
#define SFDO_WINDOW_LIST_SKIP(st) ((st).flags.common.s.do_window_list_skip)
#define SMDO_WINDOW_LIST_SKIP(st) ((st).flag_mask.common.s.do_window_list_skip)
#define SCDO_WINDOW_LIST_SKIP(st) ((st).change_mask.common.s.do_window_list_skip)
#define SFSET_DO_WINDOW_LIST_SKIP(st,x) ((st).flags.common.s.do_window_list_skip = !!(x))
#define SMSET_DO_WINDOW_LIST_SKIP(st,x) ((st).flag_mask.common.s.do_window_list_skip = !!(x))
#define SCSET_DO_WINDOW_LIST_SKIP(st,x) ((st).change_mask.common.s.do_window_list_skip = !!(x))
#define SFOCUS_MODE(sf) ((sf).common.s.focus_mode)
#define SFFOCUS_MODE(st) ((st).flags.common.s.focus_mode)
#define SMFOCUS_MODE(st) ((st).flag_mask.common.s.focus_mode)
#define SCFOCUS_MODE(st) ((st).change_mask.common.s.focus_mode)
#define SFSET_FOCUS_MODE(st,x) ((st).flags.common.s.focus_mode = (x))
#define SMSET_FOCUS_MODE(st,x) ((st).flag_mask.common.s.focus_mode = (x))
#define SCSET_FOCUS_MODE(st,x) ((st).change_mask.common.s.focus_mode = (x))
#define SHAS_DEPRESSABLE_BORDER(sf) ((sf).common.s.has_depressable_border)
#define SFHAS_DEPRESSABLE_BORDER(st) ((st).flags.common.s.has_depressable_border)
#define SMHAS_DEPRESSABLE_BORDER(st) ((st).flag_mask.common.s.has_depressable_border)
#define SCHAS_DEPRESSABLE_BORDER(st) ((st).change_mask.common.s.has_depressable_border)
#define SFSET_HAS_DEPRESSABLE_BORDER(st,x) ((st).flags.common.s.has_depressable_border = !!(x))
#define SMSET_HAS_DEPRESSABLE_BORDER(st,x) ((st).flag_mask.common.s.has_depressable_border = !!(x))
#define SCSET_HAS_DEPRESSABLE_BORDER(st,x) ((st).change_mask.common.s.has_depressable_border = !!(x))
#define SHAS_MWM_BORDER(sf) ((sf).common.s.has_mwm_border)
#define SFHAS_MWM_BORDER(st) ((st).flags.common.s.has_mwm_border)
#define SMHAS_MWM_BORDER(st) ((st).flag_mask.common.s.has_mwm_border)
#define SCHAS_MWM_BORDER(st) ((st).change_mask.common.s.has_mwm_border)
#define SFSET_HAS_MWM_BORDER(st,x) ((st).flags.common.s.has_mwm_border = !!(x))
#define SMSET_HAS_MWM_BORDER(st,x) ((st).flag_mask.common.s.has_mwm_border = !!(x))
#define SCSET_HAS_MWM_BORDER(st,x) ((st).change_mask.common.s.has_mwm_border = !!(x))
#define SHAS_MWM_BUTTONS(sf) ((sf).common.s.has_mwm_buttons)
#define SFHAS_MWM_BUTTONS(st) ((st).flags.common.s.has_mwm_buttons)
#define SMHAS_MWM_BUTTONS(st) ((st).flag_mask.common.s.has_mwm_buttons)
#define SCHAS_MWM_BUTTONS(st) ((st).change_mask.common.s.has_mwm_buttons)
#define SFSET_HAS_MWM_BUTTONS(st,x) ((st).flags.common.s.has_mwm_buttons = !!(x))
#define SMSET_HAS_MWM_BUTTONS(st,x) ((st).flag_mask.common.s.has_mwm_buttons = !!(x))
#define SCSET_HAS_MWM_BUTTONS(st,x) ((st).change_mask.common.s.has_mwm_buttons = !!(x))
#define SHAS_MWM_OVERRIDE(sf) ((sf).common.s.has_mwm_override)
#define SFHAS_MWM_OVERRIDE(st) ((st).flags.common.s.has_mwm_override)
#define SMHAS_MWM_OVERRIDE(st) ((st).flag_mask.common.s.has_mwm_override)
#define SCHAS_MWM_OVERRIDE(st) ((st).change_mask.common.s.has_mwm_override)
#define SFSET_HAS_MWM_OVERRIDE(st,x) ((st).flags.common.s.has_mwm_override = !!(x))
#define SMSET_HAS_MWM_OVERRIDE(st,x) ((st).flag_mask.common.s.has_mwm_override = !!(x))
#define SCSET_HAS_MWM_OVERRIDE(st,x) ((st).change_mask.common.s.has_mwm_override = !!(x))
#define SHAS_NO_ICON_TITLE(sf) ((sf).common.s.has_no_icon_title)
#define SFHAS_NO_ICON_TITLE(st) ((st).flags.common.s.has_no_icon_title)
#define SMHAS_NO_ICON_TITLE(st) ((st).flag_mask.common.s.has_no_icon_title)
#define SCHAS_NO_ICON_TITLE(st) ((st).change_mask.common.s.has_no_icon_title)
#define SFSET_HAS_NO_ICON_TITLE(st,x) ((st).flags.common.s.has_no_icon_title = !!(x))
#define SMSET_HAS_NO_ICON_TITLE(st,x) ((st).flag_mask.common.s.has_no_icon_title = !!(x))
#define SCSET_HAS_NO_ICON_TITLE(st,x) ((st).change_mask.common.s.has_no_icon_title = !!(x))
#define SHAS_OVERRIDE_SIZE(sf) ((sf).common.s.has_override_size)
#define SFHAS_OVERRIDE_SIZE(st) ((st).flags.common.s.has_override_size)
#define SMHAS_OVERRIDE_SIZE(st) ((st).flag_mask.common.s.has_override_size)
#define SCHAS_OVERRIDE_SIZE(st) ((st).change_mask.common.s.has_override_size)
#define SFSET_HAS_OVERRIDE_SIZE(st,x) ((st).flags.common.s.has_override_size = !!(x))
#define SMSET_HAS_OVERRIDE_SIZE(st,x) ((st).flag_mask.common.s.has_override_size = !!(x))
#define SCSET_HAS_OVERRIDE_SIZE(st,x) ((st).change_mask.common.s.has_override_size = !!(x))
#define SIS_FIXED(sf) ((sf).common.s.is_fixed)
#define SFIS_FIXED(st) ((st).flags.common.s.is_fixed)
#define SMIS_FIXED(st) ((st).flag_mask.common.s.is_fixed)
#define SCIS_FIXED(st) ((st).change_mask.common.s.is_fixed)
#define SFSET_IS_FIXED(st,x) ((st).flags.common.s.is_fixed = !!(x))
#define SMSET_IS_FIXED(st,x) ((st).flag_mask.common.s.is_fixed = !!(x))
#define SCSET_IS_FIXED(st,x) ((st).change_mask.common.s.is_fixed = !!(x))
#define SIS_ICON_STICKY(sf) ((sf).common.s.is_icon_sticky)
#define SFIS_ICON_STICKY(st) ((st).flags.common.s.is_icon_sticky)
#define SMIS_ICON_STICKY(st) ((st).flag_mask.common.s.is_icon_sticky)
#define SCIS_ICON_STICKY(st) ((st).change_mask.common.s.is_icon_sticky)
#define SFSET_IS_ICON_STICKY(st,x) ((st).flags.common.s.is_icon_sticky = !!(x))
#define SMSET_IS_ICON_STICKY(st,x) ((st).flag_mask.common.s.is_icon_sticky = !!(x))
#define SCSET_IS_ICON_STICKY(st,x) ((st).change_mask.common.s.is_icon_sticky = !!(x))
#define SIS_ICON_SUPPRESSED(sf) ((sf).common.s.is_icon_suppressed)
#define SFIS_ICON_SUPPRESSED(st) ((st).flags.common.s.is_icon_suppressed)
#define SMIS_ICON_SUPPRESSED(st) ((st).flag_mask.common.s.is_icon_suppressed)
#define SCIS_ICON_SUPPRESSED(st) ((st).change_mask.common.s.is_icon_suppressed)
#define SFSET_IS_ICON_SUPPRESSED(st,x) ((st).flags.common.s.is_icon_suppressed = !!(x))
#define SMSET_IS_ICON_SUPPRESSED(st,x) ((st).flag_mask.common.s.is_icon_suppressed = !!(x))
#define SCSET_IS_ICON_SUPPRESSED(st,x) ((st).change_mask.common.s.is_icon_suppressed = !!(x))
#define SIS_LENIENT(sf) ((sf).common.s.is_lenient)
#define SFIS_LENIENT(st) ((st).flags.common.s.is_lenient)
#define SMIS_LENIENT(st) ((st).flag_mask.common.s.is_lenient)
#define SCIS_LENIENT(st) ((st).change_mask.common.s.is_lenient)
#define SFSET_IS_LENIENT(st,x) ((st).flags.common.s.is_lenient = !!(x))
#define SMSET_IS_LENIENT(st,x) ((st).flag_mask.common.s.is_lenient = !!(x))
#define SCSET_IS_LENIENT(st,x) ((st).change_mask.common.s.is_lenient = !!(x))













/* access to other parts of a style (call with the style itself) */
#define SGET_NEXT_STYLE(s)            ((s).next)
#define SSET_NEXT_STYLE(s,x)          ((s).next = (x))
#define SGET_NAME(s)                  ((s).name)
#define SSET_NAME(s,x)                ((s).name = (x))
#define SGET_ICON_NAME(s)             ((s).icon_name)
#define SSET_ICON_NAME(s,x)           ((s).icon_name = (x))
#ifdef MINI_ICONS
#define SGET_MINI_ICON_NAME(s)        ((s).mini_icon_name)
#define SSET_MINI_ICON_NAME(s,x)      ((s).mini_icon_name = (x))
#endif
#ifdef USEDECOR
#define SGET_DECOR_NAME(s)            ((s).decor_name)
#define SSET_DECOR_NAME(s,x)          ((s).decor_name = (x))
#endif
#define SGET_FORE_COLOR_NAME(s)       ((s).fore_color_name)
#define SSET_FORE_COLOR_NAME(s,x)     ((s).fore_color_name = (x))
#define SGET_BACK_COLOR_NAME(s)       ((s).back_color_name)
#define SSET_BACK_COLOR_NAME(s,x)     ((s).back_color_name = (x))
#define SGET_COLORSET(s)              ((s).colorset)
#define SSET_COLORSET(s,x)            ((s).colorset = (x))
#define SGET_FLAGS_POINTER(s)         (&((s).flags))
#define SGET_BORDER_WIDTH(s)          ((s).border_width)
#define SSET_BORDER_WIDTH(s,x)        ((s).border_width = (x))
#define SGET_HANDLE_WIDTH(s)          ((s).handle_width)
#define SSET_HANDLE_WIDTH(s,x)        ((s).handle_width = (x))
#define SGET_LAYER(s)                 ((s).layer)
#define SSET_LAYER(s,x)               ((s).layer = (x))
#define SGET_START_DESK(s)            ((s).start_desk)
#define SSET_START_DESK(s,x)          ((s).start_desk = (x))
#define SGET_START_PAGE_X(s)          ((s).start_page_x)
#define SSET_START_PAGE_X(s,x)        ((s).start_page_x = (x))
#define SGET_START_PAGE_Y(s)          ((s).start_page_y)
#define SSET_START_PAGE_Y(s,x)        ((s).start_page_y = (x))
#define SGET_MAX_WINDOW_WIDTH(s)      ((s).max_window_width)
#define SSET_MAX_WINDOW_WIDTH(s,x)    ((s).max_window_width = (x))
#define SGET_MAX_WINDOW_HEIGHT(s)     ((s).max_window_height)
#define SSET_MAX_WINDOW_HEIGHT(s,x)   ((s).max_window_height = (x))
#define SGET_ICON_BOXES(s)            ((s).icon_boxes)
#define SSET_ICON_BOXES(s,x)          ((s).icon_boxes = (x))


/* function prototypes */
void ProcessNewStyle(F_CMD_ARGS);
void ProcessDestroyStyle(F_CMD_ARGS);
void lookup_style(FvwmWindow *tmp_win, window_style *styles);
int cmp_masked_flags(void *flags1, void *flags2, void *mask, int len);
void check_window_style_change(
  FvwmWindow *t, update_win *flags, window_style *ret_style);
void reset_style_changes(void);
void update_style_colorset(int colorset);
void update_window_color_style(FvwmWindow *tmp_win, window_style *style);
void free_icon_boxes(icon_boxes *ib);

#endif /* _STYLE_ */
