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
	FUNC_DONT_DEFER          = 0x80
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
