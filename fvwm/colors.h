/* -*-c-*- */

#ifndef COLORS_H
#define COLORS_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void FreeColors(Pixel *pixels, int n, Bool no_limit);

void CopyColor(Pixel *dst_color, Pixel *src_color, Bool do_free_dest,
	       Bool do_copy_src);

#endif /* COLORS_H */
