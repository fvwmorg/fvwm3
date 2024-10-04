/* -*-c-*- */

#ifndef FVWMLIB_FSM_H
#define FVWMLIB_FSM_H

/* ---------------------------- included header files ---------------------- */
#include "FSMlib.h"
#include <sys/select.h>
/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef struct
{
	FSmsConn	 	smsConn;
	FIceConn		ice_conn;
	char 		*clientId;
} fsm_client_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

int fsm_init(char *module);
void fsm_fdset(fd_set *in_fdset);
Bool fsm_process(fd_set *in_fdset);
void fsm_proxy(Display *dpy, Window win, char *sm);
void fsm_close(void);

#endif /* FVWMLIB_FSM_H */
