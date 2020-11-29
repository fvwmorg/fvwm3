/* -*-c-*- */

#ifndef EXPAND_H
#define EXPAND_H
#include "condrc.h"
#include "execcontext.h"

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

char *expand_vars(
	char *input, char *arguments[], Bool addto, Bool ismod,
	cond_rc_t *cond_rc, const exec_context_t *exc);

#endif /* EXPAND_H */
