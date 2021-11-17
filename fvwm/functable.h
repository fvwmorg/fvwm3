/* -*-c-*- */

#ifndef FVWM_FUNCTABLE_H
#define FVWM_FUNCTABLE_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

#define PRE_KEEPRC "keeprc"
#define PRE_REPEAT "repeat"
#define PRE_SILENT "silent"

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef unsigned int func_flags_t;

/* used for parsing commands*/
typedef struct
{
	char *keyword;
#ifdef __STDC__
	void (*action)(F_CMD_ARGS);
#else
	void (*action)();
#endif
	short func_c;
	func_flags_t flags;
	int cursor;
} func_t;

/* ---------------------------- exported variables (globals) --------------- */

extern const func_t func_table[];

/* ---------------------------- interface functions ------------------------ */

const func_t *find_builtin_function(const char *func);

#endif /* FVWM_FUNCTABLE_H */
