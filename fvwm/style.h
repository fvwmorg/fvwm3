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

#include "fvwm.h"

/* access to the common flags of a window */
/* call these with a pointer to a style_flags struct */
#define SDO_GRAB_FOCUS(sf)            ((sf)->common.do_grab_focus_when_created)
#define SDO_GRAB_FOCUS_TRANSIENT(sf)  \
          ((sf)->common.do_grab_focus_when_transient_created)
#define SDO_SKIP_CIRCULATE(sf)        ((sf)->common.do_circulate_skip)
#define SDO_SKIP_CIRCULATE_ICON(sf)   ((sf)->common.circulate_skip_icon)
#define SDO_NOT_SHOW_ON_MAP(sf)       ((sf)->common.do_not_show_on_map)
#define SDO_SKIP_WINDOW_LIST(sf)      ((sf)->common.do_window_list_skip)
#define SDO_START_ICONIC(sf)          ((sf)->common.do_start_iconic)
#define SIS_ICON_STICKY(sf)           ((sf)->common.is_icon_sticky)
#define SIS_ICON_SUPPRESSED(sf)       ((sf)->common.is_icon_suppressed)
#define SIS_LENIENT(sf)               ((sf)->common.is_lenient)
#define SIS_STICKY(sf)                ((sf)->common.is_sticky)
#define SHAS_CLICK_FOCUS(sf)          \
          ((sf)->common.focus_mode == FOCUS_CLICK)
#define SHAS_MOUSE_FOCUS(sf)          \
          ((sf)->common.focus_mode == FOCUS_MOUSE)
#define SHAS_SLOPPY_FOCUS(sf)         \
          ((sf)->common.focus_mode == FOCUS_SLOPPY)
#define SHAS_NO_ICON_TITLE(sf)        ((sf)->common.has_no_icon_title)
#define SHAS_MWM_BORDER(sf)           ((sf)->common.has_mwm_border)
#define SHAS_MWM_BUTTONS(sf)          ((sf)->common.has_mwm_buttons)
#define SHAS_MWM_OVERRIDE_HINTS(sf)   ((sf)->common.has_mwm_override)
#define SHAS_OVERRIDE_SIZE_HINTS(sf)  ((sf)->common.has_override_size)

/* access to the special flags of a style */
/* call these with a pointer to a style_flags struct */
#define SDO_DECORATE_TRANSIENT(sf)    ((sf)->do_decorate_transient)
#define SDO_PLACE_RANDOM(sf)          ((sf)->do_place_random)
#define SDO_PLACE_SMART(sf)           ((sf)->do_place_smart)
#define SDO_START_LOWERED(sf)         ((sf)->do_start_lowered)
#define SHAS_BORDER_WIDTH(sf)         ((sf)->has_border_width)
#define SHAS_COLOR_BACK(sf)           ((sf)->has_color_back)
#define SHAS_COLOR_FORE(sf)           ((sf)->has_color_fore)
#define SHAS_HANDLE_WIDTH(sf)         ((sf)->has_handle_width)
#define SHAS_ICON(sf)                 ((sf)->has_icon)
#ifdef MINI_ICONS
#define SHAS_MINI_ICON(sf)            ((sf)->has_mini_icon)
#endif
#define SHAS_MWM_DECOR(sf)            ((sf)->has_mwm_decor)
#define SHAS_MWM_FUNCTIONS(sf)        ((sf)->has_mwm_functions)
#define SHAS_NO_BORDER(sf)            ((sf)->has_no_border)
#define SHAS_NO_TITLE(sf)             ((sf)->has_no_title)
#define SHAS_OL_DECOR(sf)             ((sf)->has_ol_decor)
#define SUSE_NO_PPOSITION(sf)         ((sf)->use_no_pposition)
#define SUSE_START_ON_DESK(sf)        ((sf)->use_start_on_desk)

/* access to other parts of a style (call with the style itself) */
#define SGET_NEXT_STYLE(style)        ((style).next)
#define SGET_NAME(style)              ((style).name)
#define SGET_ICON_NAME(style)         ((style).icon_name)
#ifdef MINI_ICONS
#define SGET_MINI_ICON_NAME(style)    ((style).mini_icon_name)
#endif
#ifdef USEDECOR
#define SGET_DECOR_NAME(style)        ((style).decor_name)
#endif
#define SGET_FORE_COLOR_NAME(style)   ((style).fore_color_name)
#define SGET_BACK_COLOR_NAME(style)   ((style).back_color_name)
#define SGET_FLAGS_POINTER(style)     (&((style).flags))
#define SGET_BUTTONS(style)           ((style).on_buttons)
#define SGET_BORDER_WIDTH(style)      ((style).border_width)
#define SGET_HANDLE_WIDTH(style)      ((style).handle_width)
#define SGET_LAYER(style)             ((style).layer)
#define SGET_START_DESK(style)        ((style).start_desk)
#define SGET_START_PAGE_X(style)      ((style).start_page_x)
#define SGET_START_PAGE_Y(style)      ((style).start_page_y)
#define SGET_ICON_BOXES(style)        ((style).IconBoxes)


/* function prototypes */
void ProcessNewStyle(F_CMD_ARGS);
void lookup_style(FvwmWindow *tmp_win, window_style *styles);
void merge_styles(window_style *, window_style *);
int cmp_masked_flags(void *flags1, void *flags2, void *mask, int len);

#endif /* _STYLE_ */
