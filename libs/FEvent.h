/* -*-c-*- */

#ifndef FEVENT_H
#define FEVENT_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions (privileged access) ----- */

#ifdef FEVENT_PRIVILEGED_ACCESS
void fev_copy_last_event(XEvent *dest);
XEvent *fev_get_last_event_address(void);
#endif

/* ---------------------------- interface functions (normal_access) -------- */

/* get the latest event time */
Time fev_get_evtime(void);

/* This function determines the location of the mouse pointer from the event
 * if possible, if not it queries the X server. Returns False if it had to
 * query the server and the call failed because the pointer was on a
 * different screen. */
Bool fev_get_evpos_or_query(
	Display *dpy, Window w, const XEvent *e, int *ret_x, int *ret_y);

/* Sets the x_root/y_root position in the given event if it's of the proper
 * type.  Returns True if the position was set. */
Bool fev_set_evpos(XEvent *e, int x, int y);

/* announce a faked event to the FEvent module */
void fev_fake_event(XEvent *ev);

/* temporarily store the cached event in allocated memory */
void *fev_save_event(void);

/* restore an event saved with fev_save_event and free the memory it uses */
void fev_restore_event(void *ev);

/* fill the event structure *ev with a dummy event of no particular type */
void fev_make_null_event(XEvent *ev, Display *dpy);

/* return a copy of the last XEVent in *ev */
void fev_get_last_event(XEvent *ev);

/* ---------------------------- X event replacements ----------------------- */

/* Replacements for X functions */
XTimeCoord *FGetMotionEvents(
	Display *display, Window w, Time start, Time stop, int *nevents_return);
int FAllowEvents(
	Display *display, int event_mode, Time time);
Bool FCheckIfEvent(
	Display *display, XEvent *event_return,
	Bool (*predicate) (Display *display, XEvent *event, XPointer arg),
	XPointer arg);
Bool FCheckMaskEvent(
	Display *display, long event_mask, XEvent *event_return);
Bool FCheckPeekIfEvent(
        Display *display, XEvent *event_return,
        Bool (*predicate) (Display *display, XEvent *event, XPointer arg),
        XPointer arg);
Bool FCheckTypedEvent(
	Display *display, int event_type, XEvent *event_return);
Bool FCheckTypedWindowEvent(
	Display *display, Window w, int event_type, XEvent *event_return);
Bool FCheckWindowEvent(
	Display *display, Window w, long event_mask, XEvent *event_return);
int FEventsQueued(
	Display *display, int mode);
int FIfEvent(
	Display *display, XEvent *event_return,
	Bool (*predicate) (Display *display, XEvent *event, XPointer arg),
	XPointer arg);
int FMaskEvent(
	Display *display, long event_mask, XEvent *event_return);
int FNextEvent(
	Display *display, XEvent *event_return);
int FPeekEvent(
	Display *display, XEvent *event_return);
int FPeekIfEvent(
	Display *display, XEvent *event_return,
	Bool (*predicate) (Display *display, XEvent *event, XPointer arg),
	XPointer arg);
int FPending(
	Display *display);
int FPutBackEvent(
	Display *display, XEvent *event);
int FQLength(
	Display *display);
Bool FQueryPointer(
	Display *display, Window w, Window *root_return, Window *child_return,
	int *root_x_return, int *root_y_return, int *win_x_return,
	int *win_y_return, unsigned int *mask_return);
int FSelectInput(
	Display *display, Window w, long event_mask);
Status FSendEvent(
	Display *display, Window w, Bool propagate, long event_mask,
	XEvent *event_send);
int FWarpPointer(
	Display *display, Window src_w, Window dest_w, int src_x, int src_y,
	unsigned int src_width, unsigned int src_height, int dest_x,
	int dest_y);
int FWindowEvent(
	Display *display, Window w, long event_mask, XEvent *event_return);

/* ---------------------------- disable X symbols -------------------------- */

/* FEVENT_C must only be defined in FEvent.c! */
#ifndef FEVENT_C
#define XGetMotionEvents(a, b, c, d, e) use_FGetMotionEvents
#define XCheckIfEvent(a, b, c, d) use_FCheckIfEvent
#define XCheckMaskEvent(a, b, c) use_FCheckMaskEvent
#define XCheckTypedEvent(a, b, c) use_FCheckTypedEvent
#define XCheckTypedWindowEvent(a, b, c, d) use_FCheckTypedWindowEvent
#define XCheckWindowEvent(a, b, c, d) use_FCheckWindowEvent
#define XEventsQueued(a, b) use_FEventsQueued
#define XIfEvent(a, b, c, d) use_FIfEvent
#define XMaskEvent(a, b, c) use_FMaskEvent
#define XNextEvent(a, b) use_FNextEvent
#define XPeekEvent(a, b) use_FPeekEvent
#define XPeekIfEvent(a, b, c, d) use_FPeekIfEvent
#define XPending(a) use_FPending
#define XPutBackEvent(a, b) use_FPutBackEvent
#define XQueryPointer(a, b, c, d, e, f, g, h, i) use_FQueryPointer
#define XQLength(a) use_FQLength
#define XSendEvent(a, b, c, d, e) use_FSendEvent
#define XWarpPointer(a, b, c, d, e, f, g, h, i) use_FWarpPointer
#define XWindowEvent(a, b, c, d) use_FWindowEvent
#endif

#endif /* FEVENT_H */
