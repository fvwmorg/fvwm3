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

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/* ---------------------------- included header files ----------------------- */

#include "execcontext.h"

/* ---------------------------- global definitions -------------------------- */

/* Bits for the function flag byte. */
enum
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
};

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

/* used for parsing commands*/
typedef struct
{
	char *keyword;
#ifdef __STDC__
	void (*action)(F_CMD_ARGS);
#else
	void (*action)();
#endif
	short func_type;
	FUNC_FLAGS_TYPE flags;
	int cursor;
	int evtype;
} func_type;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

void find_func_type(
	char *action, short *func_type, unsigned char *flags);
Bool functions_is_complex_function(
	const char *function_name);
void execute_function(
	fvwm_cond_func_rc *cond_rc, const exec_context_t *exc, char *action,
	FUNC_FLAGS_TYPE exec_flags);
void execute_function_override_wcontext(
	fvwm_cond_func_rc *cond_rc, const exec_context_t *exc, char *action,
	FUNC_FLAGS_TYPE exec_flags, int wcontext);
void execute_function_override_window(
	fvwm_cond_func_rc *cond_rc, const exec_context_t *exc, char *action,
	FUNC_FLAGS_TYPE exec_flags, FvwmWindow *fw);

#endif /* FUNCTIONS_H */
