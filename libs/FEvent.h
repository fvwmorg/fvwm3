/* -*-c-*- */

#ifndef FEVENT_H
#define FEVENT_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

#define FEV_IS_EVENT_INVALID(e) \
	(fev_is_invalid_event_type_set && (e).type == fev_invalid_event_type)

#define FEV_HAS_EVENT_WINDOW(type) \
	(( \
		 (type) != GraphicsExpose && \
		 (type) != NoExpose && \
		 (type) != SelectionNotify && \
		 (type) != SelectionRequest) ? 1 : 0)

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* Exported to be used in FEV_IS_EVENT_INVALID().  Do not use. */
extern char fev_is_invalid_event_type_set;
/* Exported to be used in FEV_IS_EVENT_INVALID().  Do not use. */
extern int fev_invalid_event_type;

/* ---------------------------- interface functions (privileged access) ---- */

#ifdef FEVENT_PRIVILEGED_ACCESS
void fev_copy_last_event(XEvent *dest);
XEvent *fev_get_last_event_address(void);
#endif

/* ---------------------------- interface functions (normal_access) -------- */

/* Sets the event type that is used by FWeedIfEvents() to mark an event as
 * invalid.  Needs to be called before FWeedIfEvents() can be used. */
void fev_init_invalid_event_type(int invalid_event_type);

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

/* ---------------------------- Functions not present in Xlib -------------- */

/* Iterates over all events currentliy in the input queue and calls the
 * weed_predicate procedure for them.  The predicate may return
 *  0 = keep event and continue weeding
 *  1 = invalidate event and continue weeding
 *  2 = keep event and stop weeding
 *  3 = invalidate event and stop weeding
 * Events are marked as invalid by overwriting the event type with the invalid
 * event type configured with fev_init_invalid_event_type().  Returns the
 * number of invalidated events.
 *
 * The return codes 2 and 3 of the weed_predicate procedure can be used to
 * stop weeding if another event gets "in the way".  For example, when merging
 * Expose events, one might want to stop merging when a ConfigureRequest event
 * is encountered in the queue as that event may change the visible are of the
 * window.
 *
 * Weeded events can still be returned by functions that do not check the event
 * type, e.g. FNextEvent(), FWindowEvent(), FMaskEvent(), FPeekEvent etc.  It
 * is the responsibility of the caller to discard these events.
 *
 * If the weed_predicate is a NULL pointer, no call is made and the result for
 * all events is assumed to be 1.
 */
int FWeedIfEvents(
	Display *display,
	int (*weed_predicate) (
		Display *display, XEvent *current_event, XPointer arg),
	XPointer arg);

/* Same as FWeedIfEvents but weeds only events for the given window.  The
 * weed_predicate is only called for events with a matching window.  */
int FWeedIfWindowEvents(
	Display *display, Window window,
	int (*weed_predicate) (
		Display *display, XEvent *current_event, XPointer arg),
	XPointer arg);

/* Same as FWeedIfEvents but weeds only events of the given type for the given
 * window.  If last_event is not NULL, a copy of the last weeded event is
 * returned through *last_event (valid if a value > 0 is treturned). */
int FCheckWeedTypedWindowEvents(
	Display *display, Window window, int event_type, XEvent *last_event);

/* Like FCheckIfEvent but does not remove the event from the queue. */
int FCheckPeekIfEvent(
        Display *display, XEvent *event_return,
        Bool (*predicate) (Display *display, XEvent *event, XPointer arg),
        XPointer arg);

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
int FWarpPointerUpdateEvpos(
	XEvent *ev, Display *display, Window src_w, Window dest_w, int src_x,
	int src_y, unsigned int src_width, unsigned int src_height,
	int dest_x, int dest_y);
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
