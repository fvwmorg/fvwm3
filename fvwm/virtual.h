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

#ifndef _VIRTUAL_
#define _VIRTUAL_

int HandlePaging(int, int, int *, int *, int *, int *, Bool, Bool, Bool);
void checkPanFrames(void);
void raisePanFrames(void);
void initPanFrames(void);
void MoveViewport(int newx, int newy,Bool);
void goto_desk(int desk);
void do_move_window_to_desk(FvwmWindow *fw, int desk);
Bool get_page_arguments(char *action, int *page_x, int *page_y);
char *GetDesktopName(int desk);

#endif /* _VIRTUAL_ */
