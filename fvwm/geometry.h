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

#ifndef GEOMETRY_H
#define GEOMETRY_H

#define CS_ROUND_UP          0x01
#define CS_UPDATE_MAX_DEFECT 0x02

void gravity_get_offsets(int grav, int *xp,int *yp);
void gravity_move(int gravity, rectangle *rect, int xdiff, int ydiff);
void gravity_resize(int gravity, rectangle *rect, int wdiff, int hdiff);
void gravity_get_naked_geometry(
  int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g);
void gravity_add_decoration(
  int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g);
void get_relative_geometry(rectangle *rel_g, rectangle *abs_g);
void gravity_translate_to_northwest_geometry(
  int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g);
void gravity_translate_to_northwest_geometry_no_bw(
  int gravity, FvwmWindow *t, rectangle *dest_g, rectangle *orig_g);
void get_unshaded_geometry(FvwmWindow *tmp_win, rectangle *ret_g);
void get_shaded_geometry(
  FvwmWindow *tmp_win, rectangle *small_g, rectangle *big_g);
void update_relative_geometry(FvwmWindow *tmp_win);
void update_absolute_geometry(FvwmWindow *tmp_win);
void maximize_adjust_offset(FvwmWindow *tmp_win);
void constrain_size(
  FvwmWindow *tmp_win, unsigned int *widthp, unsigned int *heightp,
  int xmotion, int ymotion, int flags);
void gravity_constrain_size(
  int gravity, FvwmWindow *t, rectangle *rect, int flags);

#endif /* PLACEMENT_H */
