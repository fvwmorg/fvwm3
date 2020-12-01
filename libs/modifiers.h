/* -*-c-*- */

#ifndef FVWMLIB_MODIFIERS_H
#define FVWMLIB_MODIFIERS_H

/* ---------------------------- included header files ---------------------- */
#include "charmap.h"
#include "fvwm_x11.h"

/* ---------------------------- global definitions ------------------------- */

#define ALL_MODIFIERS (ShiftMask|LockMask|ControlMask|Mod1Mask|Mod2Mask|\
	Mod3Mask|Mod4Mask|Mod5Mask)

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

extern charmap_t key_modifiers[];
extern unsigned int modifier_mapindex_to_mask[];

/* ---------------------------- interface functions ------------------------ */

int modifiers_string_to_modmask(char *in_modifiers, int *out_modifier_mask);

#endif /* FVWMLIB_MODIFIERS_H */
