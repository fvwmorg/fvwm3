/* -*-c-*- */

#ifndef MENU_PARAMETERS_H
#define MENU_PARAMETERS_H

/* ---------------------------- included header files ---------------------- */

/* Do not #include any files - the file including this file has to take care of
 * it. */

/* ---------------------------- forward declarations ----------------------- */

struct MenuRoot;
struct MenuParameters;
struct MenuReturn;
struct MenuItem;
struct FvwmWindow;

/* ---------------------------- type definitions --------------------------- */

/* Return values for UpdateMenu, do_menu, menuShortcuts.  This is a lame
 * hack, in that "_BUTTON" is added to mean a button-release caused the
 * return-- the macros below help deal with the ugliness. */
typedef enum MenuRC
{
	MENU_ERROR = -1,
	MENU_NOP = 0,
	MENU_DONE,
	MENU_ABORTED,
	MENU_SUBMENU_DONE,
	MENU_DOUBLE_CLICKED,
	MENU_POPUP,
	MENU_POPDOWN,
	MENU_SELECTED,
	MENU_NEWITEM,
	MENU_NEWITEM_MOVEMENU,
	MENU_NEWITEM_FIND,
	MENU_POST,
	MENU_UNPOST,
	MENU_TEAR_OFF,
	MENU_SUBMENU_TORN_OFF,
	MENU_KILL_TEAR_OFF_MENU,
	/* propagate the event to a different menu */
	MENU_PROPAGATE_EVENT
} MenuRC;

typedef struct MenuReturn
{
	MenuRC rc;
	struct MenuRoot *target_menu;
	struct
	{
		unsigned do_unpost_submenu : 1;
		unsigned is_first_item_selected : 1;
		unsigned is_key_press : 1;
		unsigned is_menu_posted : 1;
	} flags;
} MenuReturn;

typedef struct MenuPosHints
{
	/* suggested x/y position */
	int x;
	int y;
	/* additional offset to x */
	int x_offset;
	/* width of the parent menu or item */
	int menu_width;
	/* to take menu width into account (0, -1 or -0.5) */
	float x_factor;
	/* additional offset factor to x */
	float context_x_factor;
	/* same with height */
	float y_factor;
	int screen_origin_x;
	int screen_origin_y;
	/* False if referring to absolute screen position */
	Bool is_relative;
	/* True if referring to a part of a menu */
	Bool is_menu_relative;
	Bool has_screen_origin;
} MenuPosHints;

typedef struct MenuOptions
{
	struct MenuPosHints pos_hints;
	/* A position on the Xinerama screen on which the menu should be
	 * started. */
	struct
	{
		unsigned do_not_warp : 1;
		unsigned do_warp_on_select : 1;
		unsigned do_warp_title : 1;
		unsigned do_select_in_place : 1;
		unsigned do_tear_off_immediately : 1;
		unsigned has_poshints : 1;
		unsigned is_fixed : 1;
	} flags;
} MenuOptions;

typedef struct MenuParameters
{
	struct MenuRoot *menu;
	struct MenuRoot *parent_menu;
	struct MenuItem *parent_item;
	const exec_context_t **pexc;
	struct FvwmWindow *tear_off_root_menu_window;
	char **ret_paction;
	XEvent *event_propagate_to_submenu;
	struct MenuOptions *pops;
	/* A position on the Xinerama screen on which the menu should be
	 * started. */
	int screen_origin_x;
	int screen_origin_y;
	struct
	{
		unsigned has_default_action : 1;
		unsigned is_already_mapped : 1;
		unsigned is_first_root_menu : 1;
		unsigned is_invoked_by_key_press : 1;
		unsigned is_sticky : 1;
		unsigned is_submenu : 1;
		unsigned is_triggered_by_keypress : 1;
	} flags;
} MenuParameters;

typedef struct MenuRepaintTransparentParameters
{
	struct MenuRoot *mr;
	struct FvwmWindow *fw;
} MenuRepaintTransparentParameters;

#endif /* MENU_PARAMETERS_H */
