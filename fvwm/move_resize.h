#/* This program is free software; you can redistribute it and/or modify
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

void MoveOutline(int x, int  y, int  width, int height);
void AnimatedMoveOfWindow(
  Window w,int startX,int startY,int endX, int endY,Bool fWarpPointerToo,
  int cusDelay, float *ppctMovement);
void AnimatedMoveFvwmWindow(
  FvwmWindow *tmp_win, Window w, int startX, int startY, int endX, int endY,
  Bool fWarpPointerToo, int cmsDelay, float *ppctMovement);
int moveLoop(FvwmWindow *,int,int,int,int,int *,int *,Bool,Bool);
void move_window(F_CMD_ARGS);
void move_window_doit(
  XEvent *eventp,Window w,FvwmWindow *tmp_win, unsigned long context,
  char *action,int* Module, Bool do_animate, Bool do_move_to_page);
void animated_move_window(F_CMD_ARGS);
void move_window_to_page(F_CMD_ARGS);
void SetMoveSmoothness(F_CMD_ARGS);
void SetMoveThreshold(F_CMD_ARGS);
void SetOpaque(F_CMD_ARGS);
void HideGeometryWindow(F_CMD_ARGS);
void SetSnapAttraction(F_CMD_ARGS);
void SetSnapGrid(F_CMD_ARGS);
void SetXOR(F_CMD_ARGS);
void SetXORPixmap(F_CMD_ARGS);
void resize_window(F_CMD_ARGS);
void ConstrainSize(FvwmWindow *, unsigned int *width, unsigned int *height,
		   int xmotion, int ymotion, Bool roundUp);
void maximize_adjust_offset(FvwmWindow *tmp_win);
void Maximize(F_CMD_ARGS);
void handle_stick(F_CMD_ARGS, int toggle);
void stick_function(F_CMD_ARGS);

#endif /* _MOVE_RESIZE_ */
