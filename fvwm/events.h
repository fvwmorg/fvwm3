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
	Atom atom;
	int event_type;
	int stop_at_event_type;
	char do_stop_at_event_type;
} flush_property_notify_args;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void dispatch_event(XEvent *e);
int GetContext(FvwmWindow **ret_fw, FvwmWindow *t, const XEvent *e, Window *w);
int My_XNextEvent(Display *dpy, XEvent *event);
void flush_accumulate_expose(Window w, XEvent *e);
void handle_all_expose(void);
Bool StashEventTime(const XEvent *ev);
void CoerceEnterNotifyOnCurrentWindow(void);
void InitEventHandlerJumpTable(void);
void SendConfigureNotify(
	FvwmWindow *fw, int x, int y, int w, int h, int bw,
	Bool send_for_frame_too);
void WaitForButtonsUp(Bool do_handle_expose);
int discard_typed_events(int num_event_types, int *event_types);
int flush_property_notify_stop_at_event_type(
	Atom atom, Window w, char do_stop_at_event_type,
	int stop_at_event_type);
void sync_server(int toggle);
Bool is_resizing_event_pending(FvwmWindow *fw);
void events_handle_configure_request(
	XConfigureRequestEvent cre, FvwmWindow *fw, Bool force_use_grav,
	int force_gravity);
Bool test_typed_window_event(Display *display, XEvent *event, char *arg);

#endif /* EVENTS_H */
