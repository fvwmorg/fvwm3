/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified
 * by Rob Nation
 ****************************************************************************/
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/

#ifndef _MENUS_
#define _MENUS_

#include "menudim.h"
#include "menustyle.h"
#include "menuitem.h"

#define MENU_IS_LEFT  0x01
#define MENU_IS_RIGHT 0x02
#define MENU_IS_UP    0x04
#define MENU_IS_DOWN  0x08

/************************
 * MENU ROOT STRUCTURES *
 ************************/

/* This struct contains the parts of a root menu that are shared among all
 * copies of the menu */
typedef struct MenuRootStatic
{
	/* first item in menu */
	MenuItem *first;
	/* last item in menu */
	MenuItem *last;

	/* # of copies, 0 if none except this one */
	int copies;
	/* # of mapped instances */
	int usage_count;
	/* name of root */
	char *name;
	MenuDimensions dim;
	unsigned short items;
	FvwmPicture *sidePic;
	Pixel sideColor;
	/* Menu Face */
	MenuStyle *ms;
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
	MenuItem *selected_item;
	/* item that has it's submenu mapped */
	MenuItem *submenu_item;
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
} MenuRootDynamic;

/* access macros to static menu members */
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

/***********************************
 * MENU OPTIONS AND POSITION HINTS *
 ***********************************/

typedef struct
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

typedef struct
{
	MenuPosHints pos_hints;
	/* A position on the Xinerama screen on which the menu should be
	 * started. */
	struct
	{
		unsigned do_not_warp : 1;
		unsigned do_warp_on_select : 1;
		unsigned do_warp_title : 1;
		unsigned do_select_in_place : 1;
		unsigned has_poshints : 1;
		unsigned is_fixed : 1;
	} flags;
} MenuOptions;

/****************************
 * MISCELLANEOUS MENU STUFF *
 ****************************/

typedef struct
{
	MenuRoot *menu;
	MenuRoot *parent_menu;
	MenuItem *parent_item;
	FvwmWindow **pfw;
	FvwmWindow *button_window;
	FvwmWindow *tear_off_root_menu_window;
	int *pcontext;
	XEvent *eventp;
	char **ret_paction;
	XEvent *event_propagate_to_submenu;
	MenuOptions *pops;
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
	} flags;
} MenuParameters;

/* Return values for UpdateMenu, do_menu, menuShortcuts.  This is a lame
 * hack, in that "_BUTTON" is added to mean a button-release caused the
 * return-- the macros below help deal with the ugliness. */
typedef enum
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
	MENU_POST,
	MENU_UNPOST,
	MENU_TEAR_OFF,
	MENU_SUBMENU_TORN_OFF,
	MENU_KILL_TEAR_OFF_MENU,
	/* propagate the event to a different menu */
	MENU_PROPAGATE_EVENT
} MenuRC;

typedef struct
{
	MenuRC rc;
	MenuRoot *menu;
	MenuRoot *parent_menu;
	struct
	{
		unsigned do_unpost_submenu : 1;
		unsigned is_first_item_selected : 1;
		unsigned is_key_press : 1;
		unsigned is_menu_posted : 1;
	} flags;
} MenuReturn;

#define IS_MENU_RETURN(x) \
  ((x)==MENU_DONE || (x)==MENU_ABORTED || (x)==MENU_SUBMENU_TORN_OFF)


/**********************
 * EXPORTED FUNCTIONS *
 **********************/

void menus_init(void);
MenuRoot *menus_find_menu(char *name);
void menus_remove_style_from_menus(MenuStyle *ms);
MenuRoot *FollowMenuContinuations(MenuRoot *mr,MenuRoot **pmrPrior);
MenuRoot *NewMenuRoot(char *name);
void AddToMenu(MenuRoot *, char *, char *, Bool, Bool, Bool);
void menu_enter_tear_off_menu(FvwmWindow *fw);
void menu_close_tear_off_menu(FvwmWindow *fw);
void do_menu(MenuParameters *pmp, MenuReturn *pret);
char *get_menu_options(
	char *action, Window w, FvwmWindow *fw, XEvent *e, MenuRoot *mr,
	MenuItem *mi, MenuOptions *pops);
Bool DestroyMenu(MenuRoot *mr, Bool do_recreate, Bool is_command_request);
void add_another_menu_item(char *action);
void change_mr_menu_style(MenuRoot *mr, char *stylename);
void UpdateAllMenuStyles(void);
void UpdateMenuColorset(int cset);
void SetMenuCursor(Cursor cursor);
void ParentalMenuRePaint(MenuRoot *mr);
void menu_expose(XEvent *event, FvwmWindow *fw);

#endif /* _MENUS_ */
