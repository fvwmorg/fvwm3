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

extern char* MyName;
extern Display* dpy;
extern Window Root;
extern int screen;
extern GC hiliteGC, shadowGC;
extern fd_set_size_t fd_width;
extern int fd[2];
extern int x_fd;


char *safemalloc(int length);
void DeadPipe(int nonsense);
void GetTargetWindow(Window *app_win);
void nocolor(char *a, char *b);
Window ClientWindow(Window input);
void get_graphics(char *line);

void CreateWindow(int x, int y,int w, int h);
Pixel GetShadow(Pixel background);
Pixel GetHilite(Pixel background);
Pixel GetColor(char *name);
void Loop(Window target);
void RedrawWindow(Window target);
void change_window_name(char *str);
extern void send_clientmessage (Display *disp, Window w, Atom a, Time timestamp);
void GrabWindow(Window target);
void change_icon_name(char *str);
void RedrawLeftButton(GC rgc, GC sgc,int x1,int y1);
void RedrawRightButton(GC rgc, GC sgc,int x1,int y1);
void RedrawTopButton(GC rgc, GC sgc,int x1,int y1);
void RedrawBottomButton(GC rgc, GC sgc,int x1,int y1);

