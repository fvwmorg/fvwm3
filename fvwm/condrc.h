/* -*-c-*- */

#ifndef CONDRC_H
#define CONDRC_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	COND_RC_BREAK = -2,
	COND_RC_ERROR = -1,
	COND_RC_NO_MATCH = 0,
	COND_RC_OK = 1
} cond_rc_enum;

typedef struct
{
	cond_rc_enum rc;
	int break_levels;
} cond_rc_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void condrc_init(cond_rc_t *cond_rc);

#endif /* CONDRC_H */
