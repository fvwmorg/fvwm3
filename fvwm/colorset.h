/* -*-c-*- */

#ifndef COLORSET_H
#define COLORSET_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void parse_colorset(int n, char *line);
void cleanup_colorsets();
void alloc_colorset(int n);
void update_root_transparent_colorset(Atom prop);

#endif /* COLORSET_H */
