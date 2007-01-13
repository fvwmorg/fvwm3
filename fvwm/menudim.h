/* -*-c-*- */

#ifndef MENUDIM_H
#define MENUDIM_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

#define MDIM_WIDTH(d)              ((d).width)
#define MDIM_HEIGHT(d)             ((d).height)
#define MDIM_ITEM_WIDTH(d)         ((d).item_width)
#define MDIM_SIDEPIC_X_OFFSET(d)   ((d).sidepic_x_offset)
#define MDIM_ICON_X_OFFSET(d)      ((d).icon_x_offset)
#define MDIM_TRIANGLE_X_OFFSET(d)  ((d).triangle_x_offset)
#define MDIM_ITEM_X_OFFSET(d)      ((d).item_text_x_offset)
#define MDIM_ITEM_TEXT_Y_OFFSET(d) ((d).item_text_y_offset)
#define MDIM_HILIGHT_X_OFFSET(d)   ((d).hilight_x_offset)
#define MDIM_HILIGHT_WIDTH(d)      ((d).hilight_width)
#define MDIM_SCREEN_WIDTH(d)       ((d).screen_width)
#define MDIM_SCREEN_HEIGHT(d)      ((d).screen_height)

/* ---------------------------- type definitions --------------------------- */

struct MenuDimensions
{
	/* width/height of the menu */
	int width;
	int height;
	/* width of the actual menu item */
	int item_width;
	/* offset of the sidepic */
	int sidepic_x_offset;
	/* offsets of the mini icons */
	int icon_x_offset[MAX_MENU_ITEM_MINI_ICONS];
	/* offset of the submenu triangle col */
	int triangle_x_offset;
	/* offset of the actual menu item */
	int item_text_x_offset;
	/* y offset for item text. */
	int item_text_y_offset;
	/* start of the area to be hilighted */
	int hilight_x_offset;
	/* width of the area to be hilighted */
	int hilight_width;
	/* y coordinate for item */
	int y_offset;
	/* width and height of the last screen */
	int screen_width;
	/*   the menu was mapped on */
	int screen_height;
	/* number of items in the menu */
};

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

int menudim_middle_x_offset(struct MenuDimensions *mdim);

/* ---------------------------- builtin commands --------------------------- */

#endif /* MENUDIM_H */
