/* -*-c-*- */

#ifndef EVENTS_H
#define EVENTS_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef struct
{
	Window w;
	int event_type;
} test_typed_window_event_args;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void dispatch_event(XEvent *e);
int GetContext(FvwmWindow **ret_fw, FvwmWindow *t, const XEvent *e, Window *w);
int My_XNextEvent(Display *dpy, XEvent *event);
int flush_expose(Window w);
int flush_accumulate_expose(Window w, XEvent *e);
void handle_all_expose(void);
Bool StashEventTime(const XEvent *ev);
void CoerceEnterNotifyOnCurrentWindow(void);
void InitEventHandlerJumpTable(void);
void SendConfigureNotify(
	FvwmWindow *fw, int x, int y, int w, int h, int bw,
	Bool send_for_frame_too);
void WaitForButtonsUp(Bool do_handle_expose);
int discard_events(long event_mask);
int discard_window_events(Window w, long event_mask);
int flush_property_notify(Atom atom, Window w);
void sync_server(int toggle);
Bool is_resizing_event_pending(FvwmWindow *fw);
void events_handle_configure_request(
	XConfigureRequestEvent cre, FvwmWindow *fw, Bool force_use_grav);
Bool test_button_event(Display *display, XEvent *event, char *arg);
Bool test_typed_window_event(Display *display, XEvent *event, char *arg);

#endif /* EVENTS_H */
