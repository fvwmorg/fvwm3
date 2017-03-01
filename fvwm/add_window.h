/* -*-c-*- */

#ifndef ADD_WINDOW_H
#define ADD_WINDOW_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

#define AW_NO_WINDOW NULL
#define AW_UNMANAGED ((void *)1)

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

int setup_visible_names(FvwmWindow *fw, int what_changed);
void update_window_names(FvwmWindow *fw, int which);
void setup_wm_hints(FvwmWindow *fw);
void setup_snapping(FvwmWindow *fw, window_style *pstyle);
void setup_placement_penalty(FvwmWindow *fw, window_style *pstyle);
void setup_focus_policy(FvwmWindow *fw);
Bool setup_transientfor(FvwmWindow *fw);
void setup_icon_size_limits(FvwmWindow *fw, window_style *pstyle);
void setup_icon_background_parameters(FvwmWindow *fw, window_style *pstyle);
void setup_icon_title_parameters(FvwmWindow *fw, window_style *pstyle);
void setup_numeric_vals(FvwmWindow *fw, window_style *pstyle);
Bool validate_transientfor(FvwmWindow *fw);
void setup_title_geometry(
	FvwmWindow *fw, window_style *pstyle);
void setup_window_font(
	FvwmWindow *fw, window_style *pstyle, Bool do_destroy);
void setup_icon_font(
	FvwmWindow *fw, window_style *pstyle, Bool do_destroy);
void setup_style_and_decor(
	FvwmWindow *fw, window_style *pstyle, short *buttons);
void setup_frame_attributes(
	FvwmWindow *fw, window_style *pstyle);
void change_auxiliary_windows(
	FvwmWindow *fw, short buttons);
void setup_frame_geometry(
	FvwmWindow *fw);
void setup_frame_size_limits(
	FvwmWindow *fw, window_style *pstyle);
void increase_icon_hint_count(
	FvwmWindow *fw);
void change_icon(
	FvwmWindow *fw, window_style *pstyle);
void change_mini_icon(
	FvwmWindow *fw, window_style *pstyle);
void change_icon_boxes(
	FvwmWindow *fw, window_style *pstyle);
void FetchWmProtocols(
	FvwmWindow *);
FvwmWindow *AddWindow(
	const char **ret_initial_map_command, const exec_context_t *exc,
	FvwmWindow *ReuseWin, initial_window_options_t * win_opts);
void GetWindowSizeHints(
	FvwmWindow *fw);
void GetWindowSizeHintsWithCheck(
	FvwmWindow *fw,
	int do_reject_invalid_size_constrains_on_existing_window);
void free_window_names(
	FvwmWindow *tmp, Bool nukename, Bool nukeicon);
void destroy_window(
	FvwmWindow *);
void RestoreWithdrawnLocation(
	FvwmWindow *tmp, Bool is_restart_or_recapture, Window parent);
void Reborder(void);
void CaptureAllWindows(const exec_context_t *exc, Bool is_recapture);

#endif /* ADD_WINDOW_H */
