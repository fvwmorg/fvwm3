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

#ifndef _EVENTS_
#define _EVENTS_

void DispatchEvent(void);
int GetContext(FvwmWindow *, XEvent *, Window *dummy);
int My_XNextEvent(Display *dpy, XEvent *event);
void InitEventHandlerJumpTable(void);
void HandleEvents(void);
void HandleExpose(void);
void HandleFocusIn(void);
void HandleFocusOut(void);
void HandleDestroyNotify(void);
void HandleMapRequest(void);
void HandleMapRequestKeepRaised(Window keepraised, FvwmWindow *ReuseWin);
void HandleMapNotify(void);
void HandleUnmapNotify(void);
void HandleMotionNotify(void);
void HandleButtonRelease(void);
void HandleButtonPress(void);
void HandleEnterNotify(void);
void HandleLeaveNotify(void);
void HandleConfigureRequest(void);
void HandleClientMessage(void);
void HandlePropertyNotify(void);
void HandleKeyPress(void);
void HandleVisibilityNotify(void);
void HandleColormapNotify(void);

#endif /* _EVENTS_ */
