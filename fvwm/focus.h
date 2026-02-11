/* -*-c-*- */

#ifndef FVWM_FOCUS_H
#define FVWM_FOCUS_H

/* ---------------------------- included header files ---------------------- */
#include "fvwm.h"

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

#define FOCUS_SET(w, fw) _focus_set(w, fw)
#define FOCUS_RESET() _focus_reset()

#define SetFocusWindow(a, b, c) _SetFocusWindow(a, b, c, False);
#define SetFocusWindowClientEntered(a, b, c) _SetFocusWindow(a, b, c, True);
#define ReturnFocusWindow(a) _ReturnFocusWindow(a);
#define DeleteFocus(a) _DeleteFocus(a);


/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

/*
 * Triggers X protocol actions to set the focus to the given window.  It also
 * stores the FvwmWindow pointer to indicate that fvwm requested focus for that
 * FvwmWindow, not the application itself or someone else.
 */
void _focus_set(Window w, FvwmWindow *fw);
void _focus_reset(void);

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
Bool focus_does_accept_input_focus(const FvwmWindow *fw);
Bool focus_is_focused(const FvwmWindow *fw);
Bool focus_query_click_to_raise(
	FvwmWindow *fw, Bool is_focused, int context);
Bool focus_query_click_to_focus(
	FvwmWindow *fw, int context);
Bool focus_query_open_grab_focus(FvwmWindow *fw, FvwmWindow *focus_win);
Bool focus_query_close_release_focus(const FvwmWindow *fw);

FvwmWindow *get_focus_window(void);
void set_focus_window(FvwmWindow *fw);
void set_focus_model(FvwmWindow *fw);
void focus_force_refresh_focus(const FvwmWindow *fw);
void refresh_focus(const FvwmWindow *fw);

#endif /* FVWM_FOCUS_H */
