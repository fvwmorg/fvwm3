/* -*-c-*- */

#ifndef FVWM_COLORSET_H
#define FVWM_COLORSET_H

/* ---------------------------- included header files ---------------------- */
#include "libs/fvwm_x11.h"

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void
parse_colorset(int n, char *line);
void
cleanup_colorsets(void);
void
alloc_colorset(int n);
void
update_root_transparent_colorset(Atom prop);

#endif /* FVWM_COLORSET_H */
