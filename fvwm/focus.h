/* -*-c-*- */

#ifndef FOCUS_H
#define FOCUS_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

#define FOCUS_SET(w) XSetInputFocus(dpy, w, RevertToParent, CurrentTime)
#define FOCUS_RESET() \
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime)

#define SetFocusWindow(a, b, c) _SetFocusWindow(a, b, c, False);
#define SetFocusWindowClientEntered(a, b, c) _SetFocusWindow(a, b, c, True);
#define ReturnFocusWindow(a) _ReturnFocusWindow(a);
#define DeleteFocus(a) _DeleteFocus(a);
#define ForceDeleteFocus() _ForceDeleteFocus();


/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

/*
 *
 * Sets/deletes the input focus to the indicated window.
 *
 */
void _SetFocusWindow(
	FvwmWindow *fw, Bool do_allow_force_broadcast,
	fpol_set_focus_by_t set_by, Bool client_entered);
void _ReturnFocusWindow(FvwmWindow *fw);
void _DeleteFocus(Bool do_allow_force_broadcast);
void _ForceDeleteFocus(void);
void restore_focus_after_unmap(
	const FvwmWindow *fw, Bool do_skip_marked_transients);

/*
 * These need documentation
 */
Bool IsLastFocusSetByMouse(void);
void focus_grab_buttons(FvwmWindow *fw);
void focus_grab_buttons_client_entered(FvwmWindow *fw);
void focus_grab_buttons_on_layer(int layer);
void focus_grab_buttons_all(void);
void focus_grab_buttons_on_pointer_window(void);
Bool focus_does_accept_input_focus(const FvwmWindow *fw);
Bool focus_is_focused(const FvwmWindow *fw);
Bool focus_query_click_to_raise(
	FvwmWindow *fw, Bool is_focused, int context);
Bool focus_query_click_to_focus(
	FvwmWindow *fw, int context);
Bool focus_query_open_grab_focus(FvwmWindow *fw, FvwmWindow *focus_win);
Bool focus_query_close_release_focus(const FvwmWindow *fw);
FvwmWindow *focus_get_transientfor_fwin(const FvwmWindow *fw);

FvwmWindow *get_focus_window(void);
void set_focus_window(FvwmWindow *fw);
FvwmWindow *get_last_screen_focus_window(void);
void set_last_screen_focus_window(FvwmWindow *fw);
void update_last_screen_focus_window(FvwmWindow *fw);
void set_focus_model(FvwmWindow *fw);
void focus_force_refresh_focus(const FvwmWindow *fw);
void refresh_focus(const FvwmWindow *fw);

#endif /* FOCUS_H */
