/* -*-c-*- */

#ifndef WCONTEXT_H
#define WCONTEXT_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	/* contexts for button presses */
	C_NO_CONTEXT = 0x00,
	C_WINDOW = 0x01,
	C_TITLE = 0x02,
	C_ICON = 0x04,
	C_ROOT = 0x08,
	C_MENU = 0x10,
	/* C_ = 0x20, */
	C_L1 = 0x40,
	C_R1 = 0x80,
	C_L2 = 0x100,
	C_R2 = 0x200,
	C_L3 = 0x400,
	C_R3 = 0x800,
	C_L4 = 0x1000,
	C_R4 = 0x2000,
	C_L5 = 0x4000,
	C_R5 = 0x8000,
	C_UNMANAGED = 0x10000,
	C_EWMH_DESKTOP = 0x20000,
	C_F_TOPLEFT = 0x100000,
	C_F_TOPRIGHT = 0x200000,
	C_F_BOTTOMLEFT = 0x400000,
	C_F_BOTTOMRIGHT = 0x800000,
	C_SB_LEFT = 0x1000000,
	C_SB_RIGHT = 0x2000000,
	C_SB_TOP = 0x4000000,
	C_SB_BOTTOM = 0x8000000,
	/* C_ = 0x10000000, */
	/* C_ = 0x20000000, */
	/* C_ = 0x40000000, */
	C_IGNORE_ALL = (int)0x80000000,
	C_FRAME = (C_F_TOPLEFT|C_F_TOPRIGHT|C_F_BOTTOMLEFT|C_F_BOTTOMRIGHT),
	C_SIDEBAR = (C_SB_LEFT|C_SB_RIGHT|C_SB_TOP|C_SB_BOTTOM),
	C_RALL = (C_R1|C_R2|C_R3|C_R4|C_R5),
	C_LALL = (C_L1|C_L2|C_L3|C_L4|C_L5),
	C_DECOR = (C_LALL|C_RALL|C_TITLE|C_FRAME|C_SIDEBAR),
	C_ALL = (C_WINDOW|C_TITLE|C_ICON|C_ROOT|C_FRAME|C_SIDEBAR|\
		 C_LALL|C_RALL|C_EWMH_DESKTOP)
} win_context_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- exported variables (globals) --------------- */

extern charmap_t win_contexts[];

/* ---------------------------- interface functions ------------------------ */

int wcontext_string_to_wcontext(char *in_context, int *out_context_mask);
char wcontext_wcontext_to_char(win_context_t wcontext);
win_context_t wcontext_merge_border_wcontext(win_context_t wcontext);

#endif /* WCONTEXT_H */
