/* -*-c-*- */

#ifndef CONDITIONAL_H
#define CONDITIONAL_H

/* ---------------------------- included header files ---------------------- */

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

#endif /* CONDITIONAL_H */
