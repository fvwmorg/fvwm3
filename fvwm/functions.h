/* -*-c-*- */

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/* ---------------------------- included header files ---------------------- */

#include "execcontext.h"

/* ---------------------------- global definitions ------------------------- */

/* Bits for the function flag byte. */
typedef enum
{
	FUNC_NEEDS_WINDOW        = 0x01,
	FUNC_DONT_REPEAT         = 0x02,
	FUNC_ADD_TO              = 0x04,
	FUNC_DECOR               = 0x08,
	FUNC_ALLOW_UNMANAGED     = 0x10,
	/* only to be passed to execute_function() */
	FUNC_IS_UNMANAGED        = 0x20,
	FUNC_DONT_EXPAND_COMMAND = 0x40,
	FUNC_DONT_DEFER          = 0x80,

	/* The values are not used internally but by external scripts parsing
	 * functable.  Hence all the values below are 0
	 */
	/* tagging used only for building the documentation */
	FUNC_OBSOLETE            = 0,
	FUNC_DEPRECATED          = 0,
	/* command grouping (used only for building the documentation) */
	/*!!!*/
	FG_BINDING               = 0,
	FG_MODULE                = 0,
	FG_MENU                  = 0,
	FG_SESSION               = 0,
	FG_STYLE                 = 0,
	FG_MOVE                  = 0,
	FG_STATE                 = 0,
	FG_COND                  = 0,
	FG_USER                  = 0,
	FG_COLOR                 = 0,
	FG_EWMH_GNOME            = 0,
	FG_VIRTUAL               = 0,
	FG_FOCUS                 = 0,
	FG_MISC                  = 0,
	FG_OLD                   = 0
} execute_flags_t;

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* used for parsing commands*/
typedef struct
{
	char *keyword;
#ifdef __STDC__
	void (*action)(F_CMD_ARGS);
#else
	void (*action)();
#endif
	short func_t;
	FUNC_FLAGS_TYPE flags;
	int cursor;
} func_t;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void find_func_t(
	char *action, short *func_t, FUNC_FLAGS_TYPE *flags);
Bool functions_is_complex_function(
	const char *function_name);
void execute_function(
	cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	FUNC_FLAGS_TYPE exec_flags);
void execute_function_override_wcontext(
	cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	FUNC_FLAGS_TYPE exec_flags, int wcontext);
void execute_function_override_window(
	cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	FUNC_FLAGS_TYPE exec_flags, FvwmWindow *fw);

#endif /* FUNCTIONS_H */
