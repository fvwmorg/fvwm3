/* -*-c-*- */

#ifndef FVWM_FUNCTIONS_H
#define FVWM_FUNCTIONS_H

/* ---------------------------- included header files ---------------------- */

#include "execcontext.h"
#include "functable.h"
#include "functable_complex.h"

/* ---------------------------- global definitions ------------------------- */

/* Bits for the function flag byte. */
typedef enum
{
	FUNC_NEEDS_WINDOW        = 0x01,
	/*free                   = 0x02,*/
	FUNC_ADD_TO              = 0x04,
	FUNC_DECOR               = 0x08,
	FUNC_ALLOW_UNMANAGED     = 0x10,
	/* only used in _execute_command_line */
	FUNC_IS_MOVE_TYPE        = 0x20,
	/* only to be passed to execute_function() */
	FUNC_IS_UNMANAGED        = 0x40,
	FUNC_DONT_EXPAND_COMMAND = 0x80,
	FUNC_DONT_DEFER          = 0x100,

	/* The values are not used internally but by external scripts parsing
	 * functable.  Hence all the values below are 0
	 */
	/* tagging used only for building the documentation */
	FUNC_OBSOLETE            = 0,
	FUNC_DEPRECATED          = 0,
	/* command grouping (used only for building the documentation) */
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

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

/* needs to be called before any command line can be executed */
void functions_init(void);
void find_func_t(char *action, short *func_t, func_flags_t *flags);
void execute_function(F_CMD_ARGS, func_flags_t exec_flags);
void execute_function_override_wcontext(
	F_CMD_ARGS, func_flags_t exec_flags, int wcontext);
void execute_function_override_window(
	F_CMD_ARGS, func_flags_t exec_flags, FvwmWindow *fw);

#endif /* FVWM_FUNCTIONS_H */
