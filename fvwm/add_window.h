/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef ADD_WINDOW_H
#define ADD_WINDOW_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

void setup_visible_name(FvwmWindow *fw, Bool is_icon);
void setup_wm_hints(FvwmWindow *fw);
void setup_placement_penalty(FvwmWindow *fw, window_style *pstyle);
void setup_focus_policy(FvwmWindow *fw);
void setup_title_geometry(
	FvwmWindow *fw, window_style *pstyle);
void setup_window_font(
	FvwmWindow *fw, window_style *pstyle, Bool do_destroy);
void setup_icon_font(
	FvwmWindow *fw, window_style *pstyle, Bool do_destroy);
void setup_style_and_decor(
	FvwmWindow *fw, window_style *pstyle, short *buttons);
void setup_auxiliary_windows(
	FvwmWindow *fw, Bool setup_frame_and_parent, short buttons);
void setup_frame_attributes(
	FvwmWindow *fw, window_style *pstyle);
void destroy_auxiliary_windows(
	FvwmWindow *fw, Bool destroy_frame_and_parent);
void change_auxiliary_windows(
	FvwmWindow *fw, short buttons);
void setup_key_and_button_grabs(
	FvwmWindow *fw);
void setup_frame_geometry(
	FvwmWindow *fw);
void setup_frame_size_limits(
	FvwmWindow *fw, window_style *pstyle);
void setup_icon_size_limits(
	FvwmWindow *fw, window_style *pstyle);
void increase_icon_hint_count(
	FvwmWindow *fw);
void change_icon(
	FvwmWindow *fw, window_style *pstyle);
void change_mini_icon(
	FvwmWindow *fw, window_style *pstyle);
void change_icon_boxes(
	FvwmWindow *fw, window_style *pstyle);
void FetchWmProtocols(
	FvwmWindow *);
FvwmWindow *AddWindow(
	Window w, FvwmWindow *ReuseWin, initial_window_options_type * win_opts);
void GetWindowSizeHints(
	FvwmWindow *);
void free_window_names(
	FvwmWindow *tmp, Bool nukename, Bool nukeicon);
void destroy_window(
	FvwmWindow *);
void RestoreWithdrawnLocation(
	FvwmWindow *tmp, Bool is_restart_or_recapture, Window parent);
void Reborder(void);

#endif /* ADD_WINDOW_H */
