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
#define SDO_SKIP_CIRCULATE(s)        ((s)->common.do_circulate_skip)
#define SDO_SKIP_CIRCULATE_ICON(s)   ((s)->common.circulate_skip_icon)
#define SDO_SHOW_ON_MAP(s)           ((s)->common.do_show_on_map)
#define SDO_SKIP_WINDOW_LIST(s)      ((s)->common.do_window_list_skip)
#define SDO_START_ICONIC(s)          ((s)->common.do_start_iconic)
#define SIS_ICON_STICKY(s)           ((s)->common.is_icon_sticky)
#define SIS_ICON_SUPPRESSED(s)       ((s)->common.is_icon_suppressed)
#define SIS_LENIENT(s)               ((s)->common.is_lenient)
#define SIS_STICKY(s)                ((s)->common.is_sticky)
#define SHAS_CLICK_FOCUS(s)          \
          ((s)->common.focus_mode == FOCUS_CLICK)
#define SHAS_MOUSE_FOCUS(s)          \
          ((s)->common.focus_mode == FOCUS_MOUSE)
#define SHAS_SLOPPY_FOCUS(s)         \
          ((s)->common.focus_mode == FOCUS_SLOPPY)
#define SHAS_NO_ICON_TITLE(s)        ((s)->common.has_no_icon_title)
#define SHAS_MWM_BORDER(s)           ((s)->common.has_mwm_border)
#define SHAS_MWM_BUTTONS(s)          ((s)->common.has_mwm_buttons)
#define SHAS_MWM_OVERRIDE_HINTS(s)   ((s)->common.has_mwm_override)

/* access to the special flags of a style */
/* call these with a pointer to a style_flags struct */
#define SDO_DECORATE_TRANSIENT(s)    ((s)->do_decorate_transient)
#define SDO_PLACE_RANDOM(s)          ((s)->do_place_random)
#define SDO_PLACE_SMART(s)           ((s)->do_place_smart)
#define SDO_START_LOWERED(s)         ((s)->do_start_lowered)
#define SHAS_BORDER_WIDTH(s)         ((s)->has_border_width)
#define SHAS_COLOR_BACK(s)           ((s)->has_color_back)
#define SHAS_COLOR_FORE(s)           ((s)->has_color_fore)
#define SHAS_HANDLE_WIDTH(s)         ((s)->has_handle_width)
#define SHAS_ICON(s)                 ((s)->has_icon)
#ifdef MINI_ICONS
#define SHAS_MINI_ICON(s)            ((s)->has_mini_icon)
#endif
#define SHAS_MWM_DECOR(s)            ((s)->has_mwm_decor)
#define SHAS_MWM_FUNCTIONS(s)        ((s)->has_mwm_functions)
#define SHAS_NO_BORDER(s)            ((s)->has_no_border)
#define SHAS_NO_TITLE(s)             ((s)->has_no_title)
#define SHAS_OL_DECOR(s)             ((s)->has_ol_decor)
#define SUSE_LAYER(s)                ((s)->use_layer)
#define SUSE_NO_PPOSITION(s)         ((s)->use_no_pposition)
#define SUSE_START_RAISED_LOWERED(s) ((s)->use_start_raised_lowered)
#define SUSE_START_ON_DESK(s)        ((s)->use_start_on_desk)

/* function prototypes */
void ProcessNewStyle(F_CMD_ARGS);
void lookup_style(FvwmWindow *tmp_win, window_style *styles);
void merge_styles(window_style *, window_style *);
int cmp_masked_flags(void *flags1, void *flags2, void *mask, int len);

#endif /* _STYLE_ */
