/* -*-c-*- */

#ifndef EXPAND_H
#define EXPAND_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

char *expand_vars(
	char *input, char *arguments[], FvwmWindow *fw, Bool addto, Bool ismod,
	fvwm_cond_func_rc *cond_rc);

#endif /* EXPAND_H */
