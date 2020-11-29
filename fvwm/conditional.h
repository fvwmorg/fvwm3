/* -*-c-*- */

#ifndef FVWM_CONDITIONAL_H
#define FVWM_CONDITIONAL_H

/* ---------------------------- included header files ---------------------- */
#include "fvwm.h"

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

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
