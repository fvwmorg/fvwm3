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

#ifndef FOCUS_H
#define FOCUS_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

#define DEBUG_FOCUS 0

/* ---------------------------- global macros ------------------------------- */

#define FOCUS_SET(w) XSetInputFocus(dpy, w, RevertToParent, CurrentTime)
#define FOCUS_RESET() \
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime)

#if DEBUG_FOCUS
#define SetFocusWindow(a, b, c) \
{ \
	FvwmWindow *_fw; \
	_fw = (a); \
	fprintf(stderr, "sfw: %s:%d 0x%08x '%s'\n", __FILE__, __LINE__, \
		(int)_fw, (_fw) ? _fw->visible_name : "(null)"); \
	_SetFocusWindow(_fw, b, c, False); \
}
#define SetFocusWindowClientEntered(a, b, c, d) \
{ \
	FvwmWindow *_fw; \
	_fw = (a); \
	fprintf(stderr, "sfw: %s:%d 0x%08x '%s'\n", __FILE__, __LINE__, \
		(int)_fw, (_fw) ? _fw->visible_name : "(null)"); \
	_SetFocusWindow(_fw, b, c, True); \
}
#define ReturnFocusWindow(a) \
{ \
	FvwmWindow *_fw; \
	_fw = (a); \
	fprintf(stderr, "rfw: %s:%d 0x%08x '%s'\n", __FILE__, __LINE__, \
		(int)_fw, (_fw) ? _fw->visible_name : "(null)"); \
	_ReturnFocusWindow(_fw); \
}
#define DeleteFocus(a) \
{ \
	fprintf(stderr, "df: %s:%d\n", __FILE__, __LINE__); \
	_DeleteFocus(a); \
}
#define ForceDeleteFocus() \
{ \
	fprintf(stderr, "fdf: %s:%d\n", __FILE__, __LINE__); \
	_ForceDeleteFocus(); \
}
#else
#define SetFocusWindow(a, b, c) _SetFocusWindow(a, b, c, False);
#define SetFocusWindowClientEntered(a, b, c) _SetFocusWindow(a, b, c, True);
#define ReturnFocusWindow(a) _ReturnFocusWindow(a);
#define DeleteFocus(a) _DeleteFocus(a);
#define ForceDeleteFocus() _ForceDeleteFocus();
#endif


/* ---------------------------- type definitions ---------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

/********************************************************************
 *
 * Sets/deletes the input focus to the indicated window.
 *
 **********************************************************************/
void _SetFocusWindow(
	FvwmWindow *fw, Bool do_allow_force_broadcast,
	fpol_set_focus_by_t set_by, Bool client_entered);
void _ReturnFocusWindow(FvwmWindow *fw);
void _DeleteFocus(Bool do_allow_force_broadcast);
void _ForceDeleteFocus(void);
void restore_focus_after_unmap(
	const FvwmWindow *fw, Bool do_skip_marked_transients);

/**
 * These need documentation
 **/
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
