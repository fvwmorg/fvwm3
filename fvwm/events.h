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

void DispatchEvent(Bool preserve_Tmp_win);
int GetContext(FvwmWindow *, XEvent *, Window *dummy);
int My_XNextEvent(Display *dpy, XEvent *event);
int flush_expose(Window w);
int flush_accumulate_expose(Window w, XEvent *e);
void handle_all_expose(void);
Bool StashEventTime(XEvent *ev);
void CoerceEnterNotifyOnCurrentWindow(void);
void InitEventHandlerJumpTable(void);

/* event handlers */
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
void SendConfigureNotify(
  FvwmWindow *tmp_win, int x, int y, unsigned int w, unsigned int h, int bw,
  Bool send_for_frame_too);
void HandleClientMessage(void);
void HandlePropertyNotify(void);
void HandleKeyPress(void);
void HandleVisibilityNotify(void);
STROKE_CODE(void HandleButtonRelease(void);)
STROKE_CODE(void HandleMotionNotify(void);)
void WaitForButtonsUp(Bool do_handle_expose);
int discard_events(long event_mask);

#endif /* _EVENTS_ */
