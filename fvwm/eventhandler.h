/* -*-c-*- */

#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef struct
{
	const exec_context_t *exc;
} evh_args_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

/* ---------------------------- event handlers ----------------------------- */

void HandleEvents(void);
void HandleExpose(const evh_args_t *ea);
void HandleFocusIn(const evh_args_t *ea);
void HandleFocusOut(const evh_args_t *ea);
void HandleDestroyNotify(const evh_args_t *ea);
void HandleMapRequest(const evh_args_t *ea);
void HandleMapRequestKeepRaised(
	const evh_args_t *ea, Window KeepRaised, FvwmWindow *ReuseWin,
	initial_window_options_t *win_opts);
void HandleMapNotify(const evh_args_t *ea);
void HandleUnmapNotify(const evh_args_t *ea);
void HandleMotionNotify(const evh_args_t *ea);
void HandleButtonRelease(const evh_args_t *ea);
void HandleButtonPress(const evh_args_t *ea);
void HandleEnterNotify(const evh_args_t *ea);
void HandleLeaveNotify(const evh_args_t *ea);
void HandleConfigureRequest(const evh_args_t *ea);
void HandleClientMessage(const evh_args_t *ea);
void HandlePropertyNotify(const evh_args_t *ea);
void HandleKeyPress(const evh_args_t *ea);
void HandleVisibilityNotify(const evh_args_t *ea);
STROKE_CODE(void HandleButtonRelease(const evh_args_t *ea));
STROKE_CODE(void HandleMotionNotify(const evh_args_t *ea));

#endif /* EVENTHANDLER_H */
