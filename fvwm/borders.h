/* -*-c-*- */

#ifndef FVWM_BORDERS_H
#define FVWM_BORDERS_H

/* ---------------------------- included header files ---------------------- */
#include "fvwm.h"
#include "screen.h"

/* ---------------------------- global definitions ------------------------- */

#define NPARTS_KEEP_STATE 12

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	PART_NONE      = 0x0,
	/* the border drawing code relies on the fact that the border bits
	 * occupy the lowest ten bits in this order! */
	PART_BORDER_N  = 0x1,
	PART_BORDER_S  = 0x2,
	PART_BORDER_E  = 0x4,
	PART_BORDER_W  = 0x8,
	PART_BORDER_NW = 0x10,
	PART_BORDER_NE = 0x20,
	PART_BORDER_SW = 0x40,
	PART_BORDER_SE = 0x80,
	PART_TITLE     = 0x100,
	PART_BUTTONS   = 0x200,
	PART_X_HANDLES = 0x400,
	PART_Y_HANDLES = 0x800,
	/* combinations of the above values */
	PART_SIDES     = 0x0f,
	PART_CORNERS   = 0xf0,
	PART_FRAME     = 0xff,
	PART_TITLEBAR  = 0x300,
	PART_HANDLES   = 0xc00,
	PART_ALL       = 0xfff
} window_parts;

typedef enum
{
	CLEAR_NONE     = 0x0,
	CLEAR_FRAME    = 0x1,
	CLEAR_TITLE    = 0x2,
	CLEAR_BUTTONS  = 0x4,
	CLEAR_ALL      = 0x7
} clear_window_parts;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

DecorFace *border_get_border_style(
	FvwmWindow *fw, Bool has_focus);
int border_is_using_border_style(
	FvwmWindow *fw, Bool has_focus);
int border_context_to_parts(
	int context);
void border_get_part_geometry(
	FvwmWindow *fw, window_parts part, rectangle *sidebar_g,
	rectangle *ret_g, Window *ret_w);
int get_button_number(int context);
void border_draw_decorations(
	FvwmWindow *t, window_parts draw_parts, Bool has_focus, int force,
	clear_window_parts clear_parts, rectangle *old_g, rectangle *new_g);
void border_undraw_decorations(
	FvwmWindow *fw);
void border_redraw_decorations(
	FvwmWindow *fw);
unsigned int border_get_transparent_decorations_part(
	FvwmWindow *fw);
#endif /* FVWM_BORDERS_H */
