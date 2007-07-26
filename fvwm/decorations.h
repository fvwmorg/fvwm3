/* -*-c-*- */

#ifndef DECORATIONS_H
#define DECORATIONS_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	RQORIG_PROGRAM = False,
	RQORIG_PROGRAM_US = True,
	RQORIG_MODULE_INPUT,
	RQORIG_FVWM_USER,
} request_origin_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void GetMwmHints(FvwmWindow *t);
void GetOlHints(FvwmWindow *t);
void SelectDecor(FvwmWindow *t, window_style *pstyle, short *buttons);
Bool is_function_allowed(
	int function, char *action_string, const FvwmWindow *t,
	request_origin_t request_origin, Bool do_allow_override_mwm_hints);

#endif /* DECORATIONS_H */
