/* -*-c-*- */

#ifndef FVWM_CONDITIONAL_H
#define FVWM_CONDITIONAL_H

/* ---------------------------- included header files ---------------------- */
#include "fvwm.h"

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* Window mask for Circulate and Direction functions */
typedef struct WindowConditionMask
{
	struct
	{
		unsigned do_accept_focus : 1;
		unsigned do_check_desk : 1;
		unsigned do_check_screen : 1;
		unsigned do_check_cond_desk : 1;
		unsigned do_check_desk_and_global_page : 1;
		unsigned do_check_desk_and_page : 1;
		unsigned do_check_global_page : 1;
		unsigned do_check_overlapped : 1;
		unsigned do_check_page : 1;
		unsigned do_not_check_screen : 1;
		unsigned needs_current_desk : 1;
		unsigned needs_current_desk_and_global_page : 1;
		unsigned needs_current_desk_and_page : 1;
		unsigned needs_current_global_page : 1;
		unsigned needs_current_page : 1;
#define NEEDS_ANY   0
#define NEEDS_TRUE  1
#define NEEDS_FALSE 2
		unsigned needs_focus : 2;
		unsigned needs_overlapped : 2;
		unsigned needs_pointer : 2;
		unsigned needs_same_layer : 1;
		unsigned use_circulate_hit : 1;
		unsigned use_circulate_hit_icon : 1;
		unsigned use_circulate_hit_shaded : 1;
		unsigned use_do_accept_focus : 1;
	} my_flags;
	window_flags flags;
	window_flags flag_mask;
	struct name_condition *name_condition;
	int layer;
	int desk;
	struct monitor *screen;
	int placed_by_button_mask;
	int placed_by_button_set_mask;
} WindowConditionMask;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

/* Condition matching routines */
char *CreateFlagString(char *string, char **restptr);
void DefaultConditionMask(WindowConditionMask *mask);
void CreateConditionMask(char *flags, WindowConditionMask *mask);
void FreeConditionMask(WindowConditionMask *mask);
Bool MatchesConditionMask(FvwmWindow *fw, WindowConditionMask *mask);

#endif /* FVWM_CONDITIONAL_H */
