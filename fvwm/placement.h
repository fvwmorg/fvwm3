/* -*-c-*- */

#ifndef PLACEMENT_H
#define PLACEMENT_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

#define NORMAL_PLACEMENT_PENALTY(fw)      (fw->placement_penalty[0])
#define ONTOP_PLACEMENT_PENALTY(fw)       (fw->placement_penalty[1])
#define ICON_PLACEMENT_PENALTY(fw)        (fw->placement_penalty[2])
#define STICKY_PLACEMENT_PENALTY(fw)      (fw->placement_penalty[3])
#define BELOW_PLACEMENT_PENALTY(fw)       (fw->placement_penalty[4])
#define EWMH_STRUT_PLACEMENT_PENALTY(fw)  (fw->placement_penalty[5])

#define PERCENTAGE_99_PENALTY(fw) (fw->placement_percentage_penalty[0])
#define PERCENTAGE_95_PENALTY(fw) (fw->placement_percentage_penalty[1])
#define PERCENTAGE_85_PENALTY(fw) (fw->placement_percentage_penalty[2])
#define PERCENTAGE_75_PENALTY(fw) (fw->placement_percentage_penalty[3])

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	PLACE_INITIAL,
	PLACE_AGAIN
} placement_mode_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

Bool setup_window_placement(
	FvwmWindow *fw, window_style *pstyle, rectangle *attr_g,
	initial_window_options_t *win_opts, placement_mode_t mode);

#endif /* PLACEMENT_H */
