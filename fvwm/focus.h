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

/* ---------------------------- global macros ------------------------------- */

#define FOCUS_SET(w) XSetInputFocus(dpy, w, RevertToParent, CurrentTime)
#define FOCUS_RESET() \
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime)

/* ---------------------------- type definitions ---------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

/********************************************************************
 *
 * Sets/deletes the input focus to the indicated window.
 *
 **********************************************************************/
void SetFocusWindow(
	FvwmWindow *fw, Bool do_allow_force_broadcast,
	fpol_set_focus_by_t set_by);
void ReturnFocusWindow(FvwmWindow *Fw);
void DeleteFocus(Bool do_allow_force_broadcast);
void ForceDeleteFocus(void);
void restore_focus_after_unmap(
	FvwmWindow *fw, Bool do_skip_marked_transients);

/**
 * These need documentation
 **/
Bool IsLastFocusSetByMouse(void);
void focus_grab_buttons(FvwmWindow *fw, Bool is_focused);
void focus_grab_buttons_on_pointer_window(void);
Bool focus_does_accept_input_focus(FvwmWindow *fw);
Bool focus_is_focused(FvwmWindow *fw);
Bool focus_query_click_to_raise(
	FvwmWindow *Fw, Bool is_focused, Bool is_client_click);
Bool focus_query_open_grab_focus(FvwmWindow *fw, FvwmWindow *focus_win);
Bool focus_query_close_release_focus(FvwmWindow *fw);
FvwmWindow *focus_get_transientfor_fwin(FvwmWindow *fw);

FvwmWindow *get_focus_window(void);
void set_focus_window(FvwmWindow *fw);
FvwmWindow *get_last_screen_focus_window(void);
void set_last_screen_focus_window(FvwmWindow *fw);
void update_last_screen_focus_window(FvwmWindow *fw);
void set_focus_model(FvwmWindow *fw);
void focus_force_refresh_focus(FvwmWindow *fw);
void refresh_focus(FvwmWindow *fw);

#endif /* FOCUS_H */
