/* -*-c-*- */

#ifndef FVWM_PLACEMENT_H
#define FVWM_PLACEMENT_H

/* ---------------------------- included header files ---------------------- */
#include "fvwm.h"

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	PLACE_INITIAL,
	PLACE_AGAIN
} placement_mode_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

extern const pl_penalty_struct default_pl_penalty;
extern const pl_percent_penalty_struct default_pl_percent_penalty;

/* ---------------------------- interface functions ------------------------ */

Bool setup_window_placement(
	FvwmWindow *fw, window_style *pstyle, rectangle *attr_g,
	initial_window_options_t *win_opts, placement_mode_t mode);

void adjust_for_shared_placement(FvwmWindow *, const exec_context_t *);

#endif /* FVWM_PLACEMENT_H */
