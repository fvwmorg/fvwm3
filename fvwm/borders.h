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

#ifndef _BORDERS_H
#define _BORDERS_H

typedef enum
{
  DRAW_FRAME    = 0x1,
  DRAW_TITLE    = 0x2,
  DRAW_BUTTONS  = 0x4,
  DRAW_ALL      = 0x7
} draw_window_parts;

int ButtonPosition(int context, FvwmWindow * t);
void SetupTitleBar(FvwmWindow *tmp_win, int w, int h);
void SetupFrame(
  FvwmWindow *tmp_win, int x, int y, int w, int h, Bool sendEvent);
void ForceSetupFrame(
  FvwmWindow *tmp_win, int x, int y, int w, int h, Bool sendEvent);
void set_decor_gravity(
  FvwmWindow *tmp_win, int gravity, int parent_gravity, int client_gravity);
void SetShape(FvwmWindow *, int);
void draw_clipped_decorations(
  FvwmWindow *t, draw_window_parts draw_parts, Bool has_focus, int force,
  Window expose_win, XRectangle *rclip);
void DrawDecorations(
  FvwmWindow *t, draw_window_parts draw_parts, Bool has_focus, int force,
  Window expose_win);
void RedrawDecorations(FvwmWindow *tmp_win);
#endif /* _BORDERS_H */
