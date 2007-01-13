/* -*-c-*- */

#ifndef MENU_ROOT_H
#define MENU_ROOT_H

/* ---------------------------- included header files ---------------------- */

/* Do not #include any files - the file including this file has to take care of
 * it. */

/* ---------------------------- forward declarations ----------------------- */

struct MenuItem;
struct MenuStyle;

/* ---------------------------- type definitions --------------------------- */

/*
 * MENU ROOT STRUCTURES
 */

/* This struct contains the parts of a root menu that are shared among all
 * copies of the menu */
typedef struct MenuRootStatic
{
	/* first item in menu */
	struct MenuItem *first;
	/* last item in menu */
	struct MenuItem *last;

	/* # of copies, 0 if none except this one */
	int copies;
	/* # of mapped instances */
	int usage_count;
	/* name of root */
	char *name;
	struct MenuDimensions dim;
	int items;
	FvwmPicture *sidePic;
	Pixel sideColor;
	unsigned int used_mini_icons;
	/* Menu Face */
	struct MenuStyle *ms;
	/* permanent flags */
	struct
	{
		unsigned has_side_color : 1;
		unsigned is_left_triangle : 1;
		unsigned is_updated : 1;
	} flags;
	struct
	{
		char *popup_action;
		char *popdown_action;
		char *missing_submenu_func;
	} dynamic;
} MenuRootStatic;

/* access macros to static menu members */
#define MR_FIRST_ITEM(m)         ((m)->s->first)
#define MR_LAST_ITEM(m)          ((m)->s->last)
#define MR_COPIES(m)             ((m)->s->copies)
#define MR_MAPPED_COPIES(m)      ((m)->s->usage_count)
#define MR_NAME(m)               ((m)->s->name)
#define MR_DIM(m)                ((m)->s->dim)
#define MR_WIDTH(m)              MDIM_WIDTH((m)->s->dim)
#define MR_HEIGHT(m)             MDIM_HEIGHT((m)->s->dim)
#define MR_ITEM_WIDTH(m)         MDIM_ITEM_WIDTH((m)->s->dim)
#define MR_SIDEPIC_X_OFFSET(m)   MDIM_SIDEPIC_X_OFFSET((m)->s->dim)
#define MR_ICON_X_OFFSET(m)      MDIM_ICON_X_OFFSET((m)->s->dim)
#define MR_TRIANGLE_X_OFFSET(m)  MDIM_TRIANGLE_X_OFFSET((m)->s->dim)
#define MR_ITEM_X_OFFSET(m)      MDIM_ITEM_X_OFFSET((m)->s->dim)
#define MR_ITEM_TEXT_Y_OFFSET(m) MDIM_ITEM_TEXT_Y_OFFSET((m)->s->dim)
#define MR_HILIGHT_X_OFFSET(m)   MDIM_HILIGHT_X_OFFSET((m)->s->dim)
#define MR_HILIGHT_WIDTH(m)      MDIM_HILIGHT_WIDTH((m)->s->dim)
#define MR_SCREEN_WIDTH(m)       MDIM_SCREEN_WIDTH((m)->s->dim)
#define MR_SCREEN_HEIGHT(m)      MDIM_SCREEN_HEIGHT((m)->s->dim)
#define MR_ITEMS(m)              ((m)->s->items)
#define MR_SIDEPIC(m)            ((m)->s->sidePic)
#define MR_SIDECOLOR(m)          ((m)->s->sideColor)
#define MR_USED_MINI_ICONS(m)    ((m)->s->used_mini_icons)
#define MR_STYLE(m)              ((m)->s->ms)
/* flags */
#define MR_FLAGS(m)              ((m)->s->flags)
#define MR_POPUP_ACTION(m)       ((m)->s->dynamic.popup_action)
#define MR_POPDOWN_ACTION(m)     ((m)->s->dynamic.popdown_action)
#define MR_MISSING_SUBMENU_FUNC(m) ((m)->s->dynamic.missing_submenu_func)
#define MR_HAS_SIDECOLOR(m)      ((m)->s->flags.has_side_color)
#define MR_IS_LEFT_TRIANGLE(m)   ((m)->s->flags.is_left_triangle)
#define MR_IS_UPDATED(m)         ((m)->s->flags.is_updated)

/* This struct contains the parts of a root menu that differ in all copies of
 * the menu */
typedef struct MenuRootDynamic
{
	/* the first copy of the current menu */
	struct MenuRoot *original_menu;
	/* next in list of root menus */
	struct MenuRoot *next_menu;
	/* continuation of this menu (too tall for screen) */
	struct MenuRoot *continuation_menu;
	/* can get the menu that this popped up through selected_item->mr when
	 * selected is a popup menu item */
	/* the menu that popped this up, if any */
	struct MenuRoot *parent_menu;
	/* the menu item that popped this up, if any */
	struct MenuItem *parent_item;
	/* the display used to create the menu.  Can't use the normal display
	 * because 'xkill' would kill the window manager if used on a tear off
	 * menu. */
	Display *create_dpy;
	/* the window of the menu */
	Window window;
	/* the selected item in menu */
	struct MenuItem *selected_item;
	/* item that has it's submenu mapped */
	struct MenuItem *submenu_item;
	/* x distance window was moved by animation */
	int xanimation;
	/* dynamic temp flags */
	struct
	{
		/* is win background set? */
		unsigned is_background_set : 1;
		unsigned is_destroyed : 1;
		/* menu direction relative to parent menu */
		unsigned is_left : 1;
		unsigned is_right : 1;
		unsigned is_up : 1;
		unsigned is_down : 1;
		unsigned is_painted : 1;
		unsigned is_tear_off_menu : 1;
		unsigned has_popped_up_left : 1;
		unsigned has_popped_up_right : 1;
	} dflags;
	struct
	{
		Pixmap stored;
		int width;
		int height;
		int y;
	} stored_item;
	struct
	{
		Pixel *d_pixels;
		unsigned int d_npixels;
	} stored_pixels;
	/* alloc pixels when dithering is used for gradients */
} MenuRootDynamic;

/* access macros to dynamic menu members */
#define MR_ORIGINAL_MENU(m)         ((m)->d->original_menu)
#define MR_NEXT_MENU(m)             ((m)->d->next_menu)
#define MR_CONTINUATION_MENU(m)     ((m)->d->continuation_menu)
#define MR_PARENT_MENU(m)           ((m)->d->parent_menu)
#define MR_PARENT_ITEM(m)           ((m)->d->parent_item)
#define MR_CREATE_DPY(m)            ((m)->d->create_dpy)
#define MR_WINDOW(m)                ((m)->d->window)
#define MR_SELECTED_ITEM(m)         ((m)->d->selected_item)
#define MR_SUBMENU_ITEM(m)          ((m)->d->submenu_item)
#define MR_XANIMATION(m)            ((m)->d->xanimation)
#define MR_STORED_ITEM(m)           ((m)->d->stored_item)
#define MR_STORED_PIXELS(m)         ((m)->d->stored_pixels)
/* flags */
#define MR_DYNAMIC_FLAGS(m)         ((m)->d->dflags)
#define MR_IS_BACKGROUND_SET(m)     ((m)->d->dflags.is_background_set)
#define MR_IS_DESTROYED(m)          ((m)->d->dflags.is_destroyed)
#define MR_IS_LEFT(m)               ((m)->d->dflags.is_left)
#define MR_IS_RIGHT(m)              ((m)->d->dflags.is_right)
#define MR_IS_UP(m)                 ((m)->d->dflags.is_up)
#define MR_IS_DOWN(m)               ((m)->d->dflags.is_down)
#define MR_IS_PAINTED(m)            ((m)->d->dflags.is_painted)
#define MR_IS_TEAR_OFF_MENU(m)      ((m)->d->dflags.is_tear_off_menu)
#define MR_HAS_POPPED_UP_LEFT(m)    ((m)->d->dflags.has_popped_up_left)
#define MR_HAS_POPPED_UP_RIGHT(m)   ((m)->d->dflags.has_popped_up_right)

typedef struct MenuRoot
{
	MenuRootStatic *s;
	MenuRootDynamic *d;
} MenuRoot;
/* don't forget to initialise new members in NewMenuRoot()! */

#endif /* MENU_ROOT_H */
