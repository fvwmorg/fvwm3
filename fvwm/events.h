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

#define XEVMASK_FRAMEW  (SubstructureRedirectMask | VisibilityChangeMask)
#define XEVMASK_DECORW  (ExposureMask | ButtonPressMask | ButtonReleaseMask | \
			 KeyPressMask | EnterWindowMask | LeaveWindowMask)
#define XEVMASK_TITLEW  (ButtonPressMask | ButtonReleaseMask | \
			 ExposureMask | OwnerGrabButtonMask | \
			 ButtonMotionMask | PointerMotionMask)
#define XEVMASK_BUTTONW XEVMASK_TITLEW
#define XEVMASK_PARENTW (SubstructureRedirectMask)
#define XEVMASK_BORDERW (ButtonPressMask | ButtonReleaseMask)
#define XEVMASK_CLIENTW (StructureNotifyMask | PropertyChangeMask | \
			 EnterWindowMask | LeaveWindowMask | \
			 ColormapChangeMask | FocusChangeMask)
#define XEVMASK_ICONW   (ButtonPressMask | ButtonReleaseMask | \
			 VisibilityChangeMask | ExposureMask | KeyPressMask | \
			 EnterWindowMask | LeaveWindowMask | FocusChangeMask)
#define XEVMASK_ICONPW  XEVMASK_ICONW
#define XEVMASK_MENU    (ButtonPressMask | ButtonReleaseMask | ExposureMask | \
 		         KeyReleaseMask | KeyPressMask | \
                         VisibilityChangeMask | ButtonMotionMask)
#define XEVMASK_MENUW   (ExposureMask | EnterWindowMask | \
                         KeyPressMask | KeyReleaseMask)
#define XEVMASK_TEAR_OFF_MENU \
  (ExposureMask | KeyPressMask | KeyReleaseMask | \
   EnterWindowMask | LeaveWindowMask | StructureNotifyMask | FocusChangeMask)
#define XEVMASK_PANFW   (ButtonPressMask | ButtonReleaseMask | \
 		         KeyReleaseMask | KeyPressMask | \
                         EnterWindowMask | LeaveWindowMask | \
                         VisibilityChangeMask)
#define XEVMASK_NOFOCUSW (KeyPressMask|FocusChangeMask)
#define XEVMASK_MENUNFW  (KeyPressMask|KeyReleaseMask|FocusChangeMask)
#define XEVMASK_ORW     (FocusChangeMask)
#define XEVMASK_ROOTW   (LeaveWindowMask| EnterWindowMask | \
          PropertyChangeMask | SubstructureRedirectMask | KeyPressMask | \
          SubstructureNotifyMask | ColormapChangeMask | \
          STROKE_CODE(ButtonMotionMask | DEFAULT_ALL_BUTTONS_MOTION_MASK |) \
          ButtonPressMask | ButtonReleaseMask)

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
void HandleMapRequestKeepRaised(
	Window keepraised, FvwmWindow *ReuseWin, Bool is_menu);
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
int discard_window_events(Window w, long event_mask);
int flush_property_notify(Atom atom, Window w);
void sync_server(int toggle);

#endif /* _EVENTS_ */
