/* -*-c-*- */
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

#ifndef EVENTS_H
#define EVENTS_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

void dispatch_event(XEvent *e, Bool preserve_Fw);
int GetContext(FvwmWindow *, const XEvent *, Window *dummy);
int My_XNextEvent(Display *dpy, XEvent *event);
int flush_expose(Window w);
int flush_accumulate_expose(Window w, XEvent *e);
void handle_all_expose(void);
Bool StashEventTime(const XEvent *ev);
void CoerceEnterNotifyOnCurrentWindow(void);
void InitEventHandlerJumpTable(void);
void SendConfigureNotify(
	FvwmWindow *fw, int x, int y, unsigned int w, unsigned int h, int bw,
	Bool send_for_frame_too);
void WaitForButtonsUp(Bool do_handle_expose);
int discard_events(long event_mask);
int discard_window_events(Window w, long event_mask);
int flush_property_notify(Atom atom, Window w);
void sync_server(int toggle);
Bool is_resizing_event_pending(FvwmWindow *fw);

#endif /* EVENTS_H */
