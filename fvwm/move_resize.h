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

#ifndef _MOVE_RESIZE_
#define _MOVE_RESIZE_

void switch_move_resize_grid(Bool state);
void AnimatedMoveOfWindow(
  Window w,int startX,int startY,int endX, int endY,Bool fWarpPointerToo,
  int cusDelay, float *ppctMovement, Bool ParentalMenu);
void AnimatedMoveFvwmWindow(
  FvwmWindow *tmp_win, Window w, int startX, int startY, int endX, int endY,
  Bool fWarpPointerToo, int cmsDelay, float *ppctMovement);
Bool moveLoop(FvwmWindow *tmp_win, int XOffset, int YOffset, int Width,
	      int Height, int *FinalX, int *FinalY,Bool do_move_opaque);
void handle_stick(F_CMD_ARGS, int toggle);
void resize_geometry_window(void);

#endif /* _MOVE_RESIZE_ */
