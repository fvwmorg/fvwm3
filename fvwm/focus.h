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

#ifndef _FOCUS_
#define _FOCUS_

void SetFocus(Window, FvwmWindow *, Bool FocusByMouse);
void FocusOn(FvwmWindow *t, Bool FocusByMouse);
void WarpOn(FvwmWindow *t,int warp_x, int x_unit, int warp_y, int y_unit);
void flip_focus_func(F_CMD_ARGS);
void focus_func(F_CMD_ARGS);
void warp_func(F_CMD_ARGS);

#endif /* _FOCUS_ */
