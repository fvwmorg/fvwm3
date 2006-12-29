/* -*-c-*- */

#ifndef MENU_GEOMETRY_H
#define MENU_GEOMETRY_H

/* ---------------------------- included header files ---------------------- */

/* Do not #include any files - the file including this file has to take care of
 * it. */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- forward declarations ----------------------- */

struct MenuRoot;
struct MenuParameters;

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* Do not use global variable.  Full stop. */

/* ---------------------------- interface functions ------------------------ */

/*
 * All functions in this file are to be used from menu*.c *only*
 */

Bool menu_get_geometry(
	struct MenuRoot *mr, Window *root_return, int *x_return, int *y_return,
	unsigned int *width_return, unsigned int *height_return,
	unsigned int *border_width_return, unsigned int *depth_return);

Bool menu_get_outer_geometry(
	struct MenuRoot *mr, struct MenuParameters *pmp, Window *root_return,
	int *x_return, int *y_return, unsigned int *width_return,
	unsigned int *height_return, unsigned int *border_width_return,
	unsigned int *depth_return);


#endif /* MENU_GEOMETRY_H */
