/* -*-c-*- */

#ifndef TIMEOUT_H
#define TIMEOUT_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

#define TIMEOUT_MAX_TIMEOUTS 32

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef int timeout_time_t;
typedef unsigned int timeout_mask_t;
typedef struct
{
	int n_timeouts;
	timeout_time_t *timeouts;
} timeout_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

timeout_t *timeout_create(
	int n_timeouts);
void timeout_destroy(
	timeout_t *to);
timeout_mask_t timeout_tick(
	timeout_t *to, timeout_time_t n_ticks);
void timeout_rewind(
	timeout_t *to, timeout_mask_t mask, timeout_time_t ticks_before_alarm);

#endif /* TIMEOUT_H */
