/* -*-c-*- */

#ifndef FVWM_BINDINGS_H
#define FVWM_BINDINGS_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void update_key_bindings(void);
unsigned int MaskUsedModifiers(unsigned int in_modifiers);
unsigned int GetUnusedModifiers(void);
void print_bindings(void);

#endif /* FVWM_BINDINGS_H */
