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

#ifndef ADD_WINDOW_H
#define ADD_WINDOW_H

extern char NoName[];
extern char NoClass[];
extern char NoResource[];

void setup_focus_policy(FvwmWindow *tmp_win);
void setup_window_font(
  FvwmWindow *tmp_win, window_style *pstyle, Bool do_destroy);
void setup_icon_font(
  FvwmWindow *tmp_win, window_style *pstyle, Bool do_destroy);
void setup_style_and_decor(
  FvwmWindow *tmp_win, window_style *pstyle, short *buttons);
void setup_auxiliary_windows(
  FvwmWindow *tmp_win, Bool setup_frame_and_parent, short buttons);
void setup_frame_attributes(
  FvwmWindow *tmp_win, window_style *pstyle);
void destroy_auxiliary_windows(
  FvwmWindow *Tmp_win, Bool destroy_frame_and_parent);
void change_auxiliary_windows(FvwmWindow *tmp_win, short buttons);
void setup_key_and_button_grabs(FvwmWindow *tmp_win);
void setup_frame_geometry(FvwmWindow *tmp_win);
void setup_frame_size_limits(FvwmWindow *tmp_win, window_style *pstyle);
void change_icon(FvwmWindow *tmp_win, window_style *pstyle);
#ifdef MINI_ICONS
void change_mini_icon(FvwmWindow *tmp_win, window_style *pstyle);
#endif
void change_icon_boxes(FvwmWindow *tmp_win, window_style *pstyle);

void FetchWmProtocols(FvwmWindow *);
FvwmWindow *AddWindow(Window w, FvwmWindow *ReuseWin);
void GetWindowSizeHints(FvwmWindow *);
void free_window_names (FvwmWindow *tmp, Bool nukename, Bool nukeicon);
void destroy_window(FvwmWindow *);
void RestoreWithdrawnLocation (FvwmWindow *tmp, Bool restart, Window parent);

#endif /* ADD_WINDOW_H */
