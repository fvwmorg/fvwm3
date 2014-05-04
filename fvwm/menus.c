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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/* IMPORTANT NOTE: Do *not* use any constant numbers in this file. All values
 * have to be #defined in the section below or in defaults.h to ensure full
 * control over the menus. */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#include <assert.h>
#include <X11/keysym.h>

#include "libs/ftime.h"
#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/Grab.h"
#include "libs/Parse.h"
#include "libs/ColorUtils.h"
#include "libs/Picture.h"
#include "libs/PictureUtils.h"
#include "libs/Graphics.h"
#include "libs/PictureGraphics.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "events.h"
#include "eventhandler.h"
#include "eventmask.h"
#include "cursor.h"
#include "functions.h"
#include "commands.h"
#include "misc.h"
#include "screen.h"
#include "colormaps.h"
#include "geometry.h"
#include "move_resize.h"
#include "menudim.h"
#include "menuitem.h"
#include "menuroot.h"
#include "menustyle.h"
#include "bindings.h"
#include "menubindings.h"
#include "menugeometry.h"
#include "menuparameters.h"
#include "menus.h"
#include "libs/FGettext.h"

/* ---------------------------- local definitions -------------------------- */

/* used in float to int arithmetic */
#define ROUNDING_ERROR_TOLERANCE  0.005

/* ---------------------------- local macros ------------------------------- */

#define SCTX_SET_MI(ctx,item)	((ctx).type = SCTX_MENU_ITEM, \
				 (ctx).menu_item.menu_item = (item))
#define SCTX_GET_MI(ctx)	((ctx).type == SCTX_MENU_ITEM ? \
				 (ctx).menu_item.menu_item : NULL)
#define SCTX_SET_MR(ctx,root)	((ctx).type = SCTX_MENU_ROOT, \
				 (ctx).menu_root.menu_root = (root))
#define SCTX_GET_MR(ctx)	((ctx).type == SCTX_MENU_ROOT ? \
				 (ctx).menu_root.menu_root : NULL)

/* ---------------------------- imports ------------------------------------ */

/* This external is safe. It's written only during startup. */
extern XContext MenuContext;

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* patch to pass the last popups position hints to popup_func */
typedef struct
{
	struct
	{
		unsigned is_last_menu_pos_hints_valid : 1;
		unsigned do_ignore_pos_hints : 1;
		unsigned do_warp_title : 1;
	} flags;
	struct MenuPosHints pos_hints;
} saved_pos_hints;

typedef struct MenuInfo
{
	MenuRoot *all;
	int n_destroyed_menus;
} MenuInfo;

typedef struct MenuSizingParameters
{
	MenuRoot *menu;
	/* number of item labels present in the item format */
	int used_item_labels;
	/* same for mini icons */
	int used_mini_icons;
	struct
	{
		int sidepic_width;
		MenuItemPartSizesT i;
	} max;
	struct
	{
		unsigned is_popup_indicator_used : 1;
	} flags;
} MenuSizingParameters;

typedef enum
{
	SCTX_MENU_ROOT,
	SCTX_MENU_ITEM
} string_context_type_t;

typedef union
{
	string_context_type_t type;
	struct
	{
		string_context_type_t type;
		MenuRoot *menu_root;
	} menu_root;
	struct
	{
		string_context_type_t type;
		MenuItem *menu_item;
	} menu_item;
} string_context_t;

typedef struct
{
	char delimiter;
	Bool (*string_handler)(
		char *string, char delimiter,
		string_context_t *user_data);
} string_def_t;

/* ---------------------------- menu loop types ---------------------------- */

typedef enum
{
	MENU_MLOOP_RET_NORMAL,
	MENU_MLOOP_RET_LOOP,
	MENU_MLOOP_RET_END
} mloop_ret_code_t;

typedef struct
{
	unsigned do_popup_immediately : 1;
	/* used for delay popups, to just popup the menu */
	unsigned do_popup_now : 1;
	unsigned do_popdown_now : 1;
	/* used for keystrokes, to popup and move to that menu */
	unsigned do_popup_and_warp : 1;
	unsigned do_force_reposition : 1;
	unsigned do_force_popup : 1;
	unsigned do_popdown : 1;
	unsigned do_popup : 1;
	unsigned do_menu : 1;
	unsigned do_recycle_event : 1;
	unsigned do_propagate_event_into_submenu : 1;
	unsigned has_mouse_moved : 1;
	unsigned is_off_menu_allowed : 1;
	unsigned is_key_press : 1;
	unsigned is_item_entered_by_key_press : 1;
	unsigned is_motion_faked : 1;
	unsigned is_popped_up_by_timeout : 1;
	unsigned is_pointer_in_active_item_area : 1;
	unsigned is_motion_first : 1;
	unsigned is_release_first : 1;
	unsigned is_submenu_mapped : 1;
	unsigned was_item_unposted : 1;
	unsigned is_button_release : 1;
} mloop_flags_t;

typedef struct
{
	MenuItem *mi;
	MenuRoot *mrMi;
	int x_offset;
	int popdown_delay_10ms;
	int popup_delay_10ms;
} mloop_evh_data_t;

typedef struct
{
	MenuRoot *mrPopup;
	MenuRoot *mrPopdown;
	mloop_flags_t mif;
	/* used to reduce network traffic with delayed popup/popdown */
	MenuItem *mi_with_popup;
	MenuItem *mi_wants_popup;
	MenuItem *miRemovedSubmenu;
} mloop_evh_input_t;

/* values that are set once when the menu loop is entered */
typedef struct mloop_static_info_t
{
	int x_init;
	int y_init;
	Time t0;
	unsigned int event_mask;
} mloop_static_info_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* This global is saved and restored every time a function is called that
 * might modify them, so we can safely let it live outside a function. */
static saved_pos_hints last_saved_pos_hints;

/* structures for menus */
static MenuInfo Menus;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static void __menu_execute_function(const exec_context_t **pexc, char *action)
{
	const exec_context_t *exc;
	exec_context_changes_t ecc;
	int old_emf;

	ecc.w.w = ((*pexc)->w.fw) ? FW_W((*pexc)->w.fw) : None;
	exc = exc_clone_context(*pexc, &ecc, ECC_W);
	old_emf = Scr.flags.is_executing_menu_function;
	Scr.flags.is_executing_menu_function = 1;
	execute_function(NULL, exc, action, FUNC_DONT_EXPAND_COMMAND);
	Scr.flags.is_executing_menu_function = old_emf;
	exc_destroy_context(exc);
	/* See if the window has been deleted */
	if (!check_if_fvwm_window_exists((*pexc)->w.fw))
	{
		ecc.w.fw = NULL;
		ecc.w.w = None;
		ecc.w.wcontext = 0;
		exc = exc_clone_context(
			*pexc, &ecc, ECC_FW | ECC_W | ECC_WCONTEXT);
		exc_destroy_context(*pexc);
		*pexc = exc;
	}

	return;
}

static Bool pointer_in_active_item_area(int x_offset, MenuRoot *mr)
{
	float ratio = (float)MST_ACTIVE_AREA_PERCENT(mr) / 100.0;

	if (MST_ACTIVE_AREA_PERCENT(mr) >= 100)
	{
		return False;
	}
	if (MST_USE_LEFT_SUBMENUS(mr))
	{
		return (x_offset <=
			MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) -
			MR_ITEM_WIDTH(mr) * ratio);
	}
	else
	{
		return (x_offset >=
			MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) * ratio);
	}
}

static Bool pointer_in_passive_item_area(int x_offset, MenuRoot *mr)
{
	float ratio = (float)MST_ACTIVE_AREA_PERCENT(mr) / 100.0;

	if (MST_ACTIVE_AREA_PERCENT(mr) >= 100)
	{
		return False;
	}
	if (MST_USE_LEFT_SUBMENUS(mr))
	{
		return (x_offset >=
			MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) * ratio);
	}
	else
	{
		return (x_offset <=
			MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) -
			MR_ITEM_WIDTH(mr) * ratio);
	}
}

/*
 * warping functions
 */

static void warp_pointer_to_title(MenuRoot *mr)
{
	FWarpPointer(
		dpy, 0, MR_WINDOW(mr), 0, 0, 0, 0,
		menudim_middle_x_offset(&MR_DIM(mr)),
		menuitem_middle_y_offset(MR_FIRST_ITEM(mr), MR_STYLE(mr)));
}

static MenuItem *warp_pointer_to_item(
	MenuRoot *mr, MenuItem *mi, Bool do_skip_title)
{
	if (do_skip_title)
	{
		while (MI_NEXT_ITEM(mi) != NULL &&
		       (!MI_IS_SELECTABLE(mi) || MI_IS_TEAR_OFF_BAR(mi)))
		{
			/* skip separators, titles and tear off bars until the
			 * first 'real' item is found */
			mi = MI_NEXT_ITEM(mi);
		}
	}
	if (mi == NULL)
	{
		mi = MR_LAST_ITEM(mr);
	}
	if (mi == NULL)
	{
		return mi;
	}
	FWarpPointer(
		dpy, 0, MR_WINDOW(mr), 0, 0, 0, 0,
		menudim_middle_x_offset(&MR_DIM(mr)),
		menuitem_middle_y_offset(mi, MR_STYLE(mr)));

	return mi;
}

/*
 * menu animation functions
 */

/* prepares the parameters to be passed to AnimatedMoveOfWindow
 * mr - the menu instance that holds the menu item
 * fw - the FvwmWindow structure to check against allowed functions */
static void get_menu_repaint_transparent_parameters(
	MenuRepaintTransparentParameters *pmrtp, MenuRoot *mr, FvwmWindow *fw)
{
	pmrtp->mr = mr;
	pmrtp->fw = fw;

	return;
}


/* Undo the animation of a menu */
static void animated_move_back(
	MenuRoot *mr, Bool do_warp_pointer, FvwmWindow *fw)
{
	MenuRepaintTransparentParameters mrtp;
	int act_x;
	int act_y;

	if (MR_XANIMATION(mr) == 0)
	{
		return;
	}
	if (menu_get_geometry(
		    mr, &JunkRoot, &act_x, &act_y, &JunkWidth, &JunkHeight,
		    &JunkBW, &JunkDepth))
	{
		Bool transparent_bg = False;

		/* move it back */
		if (ST_HAS_MENU_CSET(MR_STYLE(mr)) &&
		    CSET_IS_TRANSPARENT(ST_CSET_MENU(MR_STYLE(mr))))
		{
			transparent_bg = True;
			get_menu_repaint_transparent_parameters(
				&mrtp, mr, fw);
		}
		AnimatedMoveOfWindow(
			MR_WINDOW(mr), act_x, act_y, act_x - MR_XANIMATION(mr),
			act_y, do_warp_pointer, -1, NULL,
			(transparent_bg)? &mrtp:NULL);
		MR_XANIMATION(mr) = 0;
	}

	return;
}

/* move a menu or a tear-off menu preserving transparency.
 * tear-off menus are moved with their frame coordinates. */
static void move_any_menu(
	MenuRoot *mr, MenuParameters *pmp, int endX, int endY)
{
	if (MR_IS_TEAR_OFF_MENU(mr))
	{
		float fFull = 1.0;

		/* this moves the tearoff menu, updating of transparency
		 * will not be as good as if menu repaint parameters
		 * are used. */
		AnimatedMoveFvwmWindow(
			pmp->tear_off_root_menu_window,
			FW_W_FRAME(pmp->tear_off_root_menu_window),
			-1, -1, endX, endY, False, 0, &fFull);
	}
	else
	{
		int x;
		int y;
		int JunkDept;

		menu_get_geometry(mr, &JunkRoot, &x, &y, &JunkWidth,
				  &JunkHeight, &JunkBW, &JunkDept);
		if (x == endX && y == endY)
		{
			return;
		}
		if (ST_HAS_MENU_CSET(MR_STYLE(mr)) &&
		    CSET_IS_TRANSPARENT(ST_CSET_MENU(MR_STYLE(mr))))
		{
			MenuRepaintTransparentParameters mrtp;

			get_menu_repaint_transparent_parameters(
				&mrtp, mr, (*pmp->pexc)->w.fw);
			update_transparent_menu_bg(
				&mrtp, x, y, endX, endY, endX, endY);
			XMoveWindow(dpy, MR_WINDOW(mr), endX, endY);
			repaint_transparent_menu(
				&mrtp, False, endX,endY, endX, endY, True);
      	      	}
		else
		{
			XMoveWindow(dpy, MR_WINDOW(mr), endX, endY);
		}
	}
}

/* ---------------------------- submenu function --------------------------- */

/* Search for a submenu that was popped up by the given item in the given
 * instance of the menu. */
static MenuRoot *seek_submenu_instance(
	MenuRoot *parent_menu, MenuItem *parent_item)
{
	MenuRoot *mr;

	for (mr = Menus.all; mr != NULL; mr = MR_NEXT_MENU(mr))
	{
		if (MR_PARENT_MENU(mr) == parent_menu &&
		    MR_PARENT_ITEM(mr) == parent_item)
		{
			/* here it is */
			break;
		}
	}

	return mr;
}

static Bool is_submenu_mapped(MenuRoot *parent_menu, MenuItem *parent_item)
{
	XWindowAttributes win_attribs;
	MenuRoot *mr;

	mr = seek_submenu_instance(parent_menu, parent_item);
	if (mr == NULL)
	{
		return False;
	}

	if (MR_WINDOW(mr) == None)
	{
		return False;
	}
	if (!XGetWindowAttributes(dpy, MR_WINDOW(mr), &win_attribs))
	{
		return False;
	}

	return (win_attribs.map_state == IsViewable);
}

/* Returns the menu root that a given menu item pops up */
static MenuRoot *mr_popup_for_mi(MenuRoot *mr, MenuItem *mi)
{
	char *menu_name;
	MenuRoot *menu = NULL;

	/* This checks if mi is != NULL too */
	if (!mi || !MI_IS_POPUP(mi))
	{
		return NULL;
	}

	/* first look for a menu that is aleady mapped */
	menu = seek_submenu_instance(mr, mi);
	if (menu)
	{
		return menu;
	}

	/* just look past "Popup " in the action, and find that menu root */
	menu_name = PeekToken(SkipNTokens(MI_ACTION(mi), 1), NULL);
	menu = menus_find_menu(menu_name);

	return menu;
}

/* ---------------------------- item handling ------------------------------ */

/*
 * find_entry()
 *
 * Returns the menu item the pointer is over and optionally the offset
 * from the left side of the menu entry (if px_offset is != NULL) and
 * the MenuRoot the pointer is over (if pmr is != NULL).
 */
static MenuItem *find_entry(
	MenuParameters *pmp,
	int *px_offset /*NULL means don't return this value */,
	MenuRoot **pmr /*NULL means don't return this value */,
	/* values passed in from caller it FQueryPointer was already called
	 * there */
	Window p_child, int p_rx, int p_ry)
{
	MenuItem *mi;
	MenuRoot *mr;
	int root_x, root_y;
	int x, y;
	Window Child;
	int r;

	/* x_offset returns the x offset of the pointer in the found menu item
	 */
	if (px_offset)
	{
		*px_offset = 0;
	}
	if (pmr)
	{
		*pmr = NULL;
	}
	/* get the pointer position */
	if (p_rx < 0)
	{
		if (!FQueryPointer(
			    dpy, Scr.Root, &JunkRoot, &Child,
			    &root_x, &root_y, &JunkX, &JunkY, &JunkMask))
		{
			/* pointer is on a different screen */
			return NULL;
		}
	}
	else
	{
		root_x = p_rx;
		root_y = p_ry;
		Child = p_child;
	}
	/* find out the menu the pointer is in */
	if (pmp->tear_off_root_menu_window != NULL &&
	    Child == FW_W_FRAME(pmp->tear_off_root_menu_window))
	{
		/* we're in the top level torn off menu */
		Child = FW_W(pmp->tear_off_root_menu_window);
	}
	if (XFindContext(dpy, Child, MenuContext, (caddr_t *)&mr) == XCNOENT)
	{
		return NULL;
	}
	/* get position in that child window */
	if (!XTranslateCoordinates(
		    dpy, Scr.Root, MR_WINDOW(mr), root_x, root_y, &x, &y,
		    &JunkChild))
	{
		return NULL;
	}
	if (x < 0 || y < 0 || x >= MR_WIDTH(mr) || y >= MR_HEIGHT(mr))
	{
		return NULL;
	}
	if (pmr)
	{
		*pmr = mr;
	}
	r = MST_RELIEF_THICKNESS(mr);
	/* look for the entry that the mouse is in */
	for (mi = MR_FIRST_ITEM(mr); mi; mi = MI_NEXT_ITEM(mi))
	{
		int a;
		int b;

		a = (MI_PREV_ITEM(mi) &&
		     MI_IS_SELECTABLE(MI_PREV_ITEM(mi))) ? r / 2 : 0;
		if (!MI_IS_SELECTABLE(mi))
		{
			b = 0;
		}
		else if (MI_NEXT_ITEM(mi) &&
			 MI_IS_SELECTABLE(MI_NEXT_ITEM(mi)))
		{
			b = r / 2;
		}
		else
		{
			b = r;
		}
		if (y >= MI_Y_OFFSET(mi) - a &&
		    y < MI_Y_OFFSET(mi) + MI_HEIGHT(mi) + b)
		{
			break;
		}
	}
	if (x < MR_ITEM_X_OFFSET(mr) ||
	    x >= MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) - 1)
	{
		mi = NULL;
	}
	if (mi && px_offset)
	{
		*px_offset = x;
	}

	return mi;
}

/* ---------------------------- keyboard shortcuts ------------------------- */

static Bool is_double_click(
	Time t0, MenuItem *mi, MenuParameters *pmp, MenuReturn *pmret,
	double_keypress *pdkp, Bool has_mouse_moved)
{
	if ((*pmp->pexc)->x.elast->type == KeyPress)
	{
		return False;
	}
	if (fev_get_evtime() - t0 >= MST_DOUBLE_CLICK_TIME(pmp->menu))
	{
		return False;
	}
	if (has_mouse_moved)
	{
		return False;
	}
	if (!pmp->flags.has_default_action &&
	    (mi && mi == MR_FIRST_ITEM(pmp->menu) && MI_IS_SELECTABLE(mi)))
	{
		return False;
	}
	if (pmp->flags.is_submenu)
	{
		return False;
	}
	if (pmp->flags.is_invoked_by_key_press && pdkp->timestamp == 0)
	{
		return False;
	}

	return True;
}

/* ---------------------------- item label parsing ------------------------- */

/*
 * Procedure:
 *      scanForHotkeys - Look for hotkey markers in a MenuItem
 *                                                      (pete@tecc.co.uk)
 *
 * Inputs:
 *      it      - MenuItem to scan
 *      column  - The column number in which to look for a hotkey.
 *
 */
static void scanForHotkeys(
	MenuItem *it, int column)
{
	char *start;
	char *s;
	char *t;

	/* Get start of string */
	start = MI_LABEL(it)[column];
	/* Scan whole string */
	for (s = start; *s != '\0'; s++)
	{
		if (*s != '&')
		{
			continue;
		}
		if (s[1] != '&')
		{
			/* found a hotkey - only one hotkey per item */
			break;
		}
		/* Just an escaped '&'; copy the string down over it    */
		for (t = s; *t != '\0'; t++)
		{
			t[0] = t[1];
		}

	}
	if (*s != 0)
	{
		/* It's a hot key marker - work out the offset value */
		MI_HOTKEY_COFFSET(it) = s - start;
		MI_HOTKEY_COLUMN(it) = column;
		MI_HAS_HOTKEY(it) = (s[1] != '\0');
		MI_IS_HOTKEY_AUTOMATIC(it) = 0;
		for ( ; *s != '\0'; s++)
		{
			/* Copy down.. */
			s[0] = s[1];
		}
	}

	return;
}

static void __copy_down(char *remove_from, char *remove_to)
{
	char *t1;
	char *t2;

	for (t1 = remove_from, t2 = remove_to; *t2 != '\0'; t2++, t1++)
	{
		*t1 = *t2;
	}
	*t1 = '\0';

	return;
}

static int __check_for_delimiter(char *s, const string_def_t *string_defs)
{
	int type;

	for (type = 0; string_defs[type].delimiter != '\0'; type++)
	{
		if (s[0] == string_defs[type].delimiter)
		{
			if (s[1] != string_defs[type].delimiter)
			{
				return type;
			}
			else
			{
				/* escaped delimiter, copy the
				 * string down over it */
				__copy_down(s, s+1);

				return -1;
			}
		}
	}

	return -1;
}

/* This scans for strings within delimiters and calls a callback based
 * on the delimiter found for each found string */
static void scanForStrings(
	char *instring, const string_def_t *string_defs,
	string_context_t *context)
{
	char *s;
	int type;
	char *string;

	type = -1;
	/* string is set whenever type >= 0, and unused otherwise
	 * set to NULL to supress compiler warning */
	string = NULL;
	for (s = instring; *s != '\0'; s++)
	{
		if (type < 0)
		{
			/* look for starting delimiters */
			type = __check_for_delimiter(s, string_defs);
			if (type >= 0)
			{
				/* start of a string */
				string = s + 1;
			}
		}
		else if (
			s[0] == string_defs[type].delimiter &&
			s[1] != string_defs[type].delimiter)
		{
			/* found ending delimiter */
			Bool is_valid;

			/* terminate the string pointer */
			s[0] = '\0';
			is_valid = string_defs[type].string_handler(
				string, string_defs[type].delimiter, context);
			/* restore the string */
			s[0] = string_defs[type].delimiter;

			if (is_valid)
			{
				/* the string was OK, remove it from
				 * instring */
				__copy_down(string - 1, s + 1);
				/* continue next iteration at the
				 * first character after the string */
				s = string - 2;
			}

			type = -1;
		}
		else if (s[0] == string_defs[type].delimiter)
		{
			/* escaped delimiter, copy the string down over
			 * it */
			__copy_down(s, s + 1);
		}
	}
}

/* Side picture support: this scans for a color int the menu name
   for colorization */
static Bool __scan_for_color(
	char *name, char type, string_context_t *context)
{
	if (type != '^' || SCTX_GET_MR(*context) == NULL)
	{
		abort();
	}

	if (MR_HAS_SIDECOLOR(SCTX_GET_MR(*context)))
	{
		return False;
	}

	MR_SIDECOLOR(SCTX_GET_MR(*context)) = GetColor(name);
	MR_HAS_SIDECOLOR(SCTX_GET_MR(*context)) = True;

	return True;
}

static Bool __scan_for_pixmap(
	char *name, char type, string_context_t *context)
{
	FvwmPicture *p;
	FvwmPictureAttributes fpa;
	int current_mini_icon;

	/* check that more pictures are allowed before trying to load the
	 * picture */
	current_mini_icon = -999999999;
	switch (type)
	{
	case '@':
		if (SCTX_GET_MR(*context) == NULL)
		{
			abort();
		}
		if (MR_SIDEPIC(SCTX_GET_MR(*context)))
		{
			return False;
		}
		break;
	case '*':
		/* menu item picture, requires menu item */
		if (SCTX_GET_MI(*context) == NULL)
		{
			abort();
		}
		if (MI_PICTURE(SCTX_GET_MI(*context)))
		{
			return False;
		}
		break;
	case '%':
		/* mini icon - look for next free spot */
		if (SCTX_GET_MI(*context) == NULL)
		{
			abort();
		}
		current_mini_icon  = 0;
		while (current_mini_icon < MAX_MENU_ITEM_MINI_ICONS)
		{
			if (
				MI_MINI_ICON(SCTX_GET_MI(*context))
				[current_mini_icon])
			{
				current_mini_icon++;
			}
			else
			{
				break;
			}
		}
		if (current_mini_icon == MAX_MENU_ITEM_MINI_ICONS)
		{
			return False;
		}
		break;
	default:
		abort();
	}

	fpa.mask = 0;
	p = PCacheFvwmPicture(
		dpy, Scr.NoFocusWin, NULL, name, fpa);
	if (!p)
	{
		fvwm_msg(WARN, "scanForPixmap",
			 "Couldn't load image from %s", name);
		fvwm_msg(WARN, "scanForPixmap",
			 "Check that FVWM has support for the filetype it's "
			 "being asked to load.");

		/* return true to make missing pictures not appear in the
		 * label/name */
		return True;
	}

	switch (type)
	{
	case '@':
		MR_SIDEPIC(SCTX_GET_MR(*context)) = p;

		break;
	case '*':
		MI_PICTURE(SCTX_GET_MI(*context)) = p;
		MI_HAS_PICTURE(SCTX_GET_MI(*context)) = True;

		break;
	case '%':
		MI_MINI_ICON(SCTX_GET_MI(*context))[current_mini_icon] = p;
		MI_HAS_PICTURE(SCTX_GET_MI(*context)) = True;

		break;
	}

	return True;
}

/* ---------------------------- item list handling ------------------------- */

static void unlink_item_from_menu(
	MenuRoot *mr, MenuItem *mi)
{
	MenuItem *next;
	MenuItem *prev;

	next = MI_NEXT_ITEM(mi);
	prev = MI_PREV_ITEM(mi);
	if (next != NULL)
	{
		MI_PREV_ITEM(next) = prev;
	}
	else
	{
		MR_LAST_ITEM(mr) = prev;
	}
	if (prev != NULL)
	{
		MI_NEXT_ITEM(prev) = next;
	}
	else
	{
		MR_FIRST_ITEM(mr) = next;
	}
	MI_NEXT_ITEM(mi) = NULL;
	MI_PREV_ITEM(mi) = NULL;
	MR_ITEMS(mr)--;

	return;
}

/* Add the given menu item to the menu.  If the first item of the menu is a
 * title, and the do_replace_title flag is True, the old title is deleted and
 * replaced by the new item.  Otherwise the item is appended at the end of the
 * menu. */
static void append_item_to_menu(
	MenuRoot *mr, MenuItem *mi, Bool do_replace_title)
{
	if (MR_FIRST_ITEM(mr) == NULL)
	{
		MR_FIRST_ITEM(mr) = mi;
		MR_LAST_ITEM(mr) = mi;
		MI_NEXT_ITEM(mi) = NULL;
		MI_PREV_ITEM(mi) = NULL;
	}
	else if (do_replace_title)
	{
		if (MI_IS_TITLE(MR_FIRST_ITEM(mr)))
		{
			if (MR_FIRST_ITEM(mr) == MR_LAST_ITEM(mr))
			{
				MR_LAST_ITEM(mr) = mi;
			}
			if (MI_NEXT_ITEM(MR_FIRST_ITEM(mr)) != NULL)
			{
				MI_PREV_ITEM(MI_NEXT_ITEM(
						     MR_FIRST_ITEM(mr))) = mi;
			}
			MI_NEXT_ITEM(mi) = MI_NEXT_ITEM(MR_FIRST_ITEM(mr));
			menuitem_free(MR_FIRST_ITEM(mr));
		}
		else
		{
			MI_PREV_ITEM(MR_FIRST_ITEM(mr)) = mi;
			MI_NEXT_ITEM(mi) = MR_FIRST_ITEM(mr);
		}
		MI_PREV_ITEM(mi) = NULL;
		MR_FIRST_ITEM(mr) = mi;
	}
	else
	{
		MI_NEXT_ITEM(MR_LAST_ITEM(mr)) = mi;
		MI_PREV_ITEM(mi) = MR_LAST_ITEM(mr);
		MR_LAST_ITEM(mr) = mi;
	}

	return;
}

static void clone_menu_item_list(
	MenuRoot *dest_mr, MenuRoot *src_mr)
{
	MenuItem *mi;
	MenuItem *cloned_mi;
	MenuRoot *mr;

	MR_FIRST_ITEM(dest_mr) = NULL;
	MR_LAST_ITEM(dest_mr) = NULL;
	/* traverse the menu and all its continuations */
	for (mr = src_mr; mr != NULL; mr = MR_CONTINUATION_MENU(mr))
	{
		/* duplicate all items in the current menu */
		for (mi = MR_FIRST_ITEM(mr); mi != NULL; mi = MI_NEXT_ITEM(mi))
		{
			if (MI_IS_CONTINUATION(mi))
			{
				/* skip this item */
				continue;
			}
			cloned_mi = menuitem_clone(mi);
			append_item_to_menu(dest_mr, cloned_mi, False);
		}
	}

	return;
}

/* ---------------------------- MenuRoot maintenance functions ------------- */

/* Extract interesting values from the item format string that are needed by
 * the size_menu_... functions. */
static void calculate_item_sizes(MenuSizingParameters *msp)
{
	MenuItem *mi;
	MenuItemPartSizesT mipst;
	int i;
	Bool do_reverse_icon_order =
		(MST_USE_LEFT_SUBMENUS(msp->menu)) ? True : False;

	memset(&(msp->max), 0, sizeof(msp->max));
	/* Calculate the widths for all columns of all items. */
	for (mi = MR_FIRST_ITEM(msp->menu); mi != NULL; mi = MI_NEXT_ITEM(mi))
	{
		if (MI_IS_TITLE(mi))
		{
			menuitem_get_size(
				mi, &mipst, MST_PTITLEFONT(msp->menu),
				do_reverse_icon_order);
		}
		else
		{
			menuitem_get_size(
				mi, &mipst, MST_PSTDFONT(msp->menu),
				do_reverse_icon_order);
		}
		/* adjust maximums */
		if (msp->max.i.triangle_width < mipst.triangle_width)
		{
			msp->max.i.triangle_width = mipst.triangle_width;
		}
		if (msp->max.i.title_width < mipst.title_width)
		{
			msp->max.i.title_width = mipst.title_width;
		}
		for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
		{
			if (msp->max.i.label_width[i] < mipst.label_width[i])
			{
				msp->max.i.label_width[i] =
					mipst.label_width[i];
			}
		}
		if (msp->max.i.picture_width < mipst.picture_width)
		{
			msp->max.i.picture_width = mipst.picture_width;
		}
		for (i = 0; i < MAX_MENU_ITEM_MINI_ICONS; i++)
		{
			if (msp->max.i.icon_width[i] < mipst.icon_width[i])
			{
				msp->max.i.icon_width[i] = mipst.icon_width[i];
			}
		}
	}
	if (MR_SIDEPIC(msp->menu))
	{
		msp->max.sidepic_width = MR_SIDEPIC(msp->menu)->width;
	}
	else if (MST_SIDEPIC(msp->menu))
	{
		msp->max.sidepic_width = MST_SIDEPIC(msp->menu)->width;
	}

	return;
}

/*
 *
 * Calculate the positions of the columns in the menu.
 * Called by make_menu().
 *
 */
static void size_menu_horizontally(MenuSizingParameters *msp)
{
	MenuItem *mi;
	Bool sidepic_is_left = True;
	int total_width;
	int sidepic_space = 0;
	int label_offset[MAX_MENU_ITEM_LABELS];
	char lcr_column[MAX_MENU_ITEM_LABELS];
	int i;
	int d;
	int relief_thickness = MST_RELIEF_THICKNESS(msp->menu);
	int *item_order[
		MAX_MENU_ITEM_LABELS + MAX_MENU_ITEM_MINI_ICONS +
		1 /* triangle */ + 2 /* relief markers */];
	int used_objects = 0;
	int left_objects = 0;
	int right_objects = 0;
	int x;
	unsigned char icons_placed = 0;
	Bool sidepic_placed = False;
	Bool triangle_placed = False;
	Bool relief_begin_placed = False;
	Bool relief_end_placed = False;
	char *format;
	Bool first = True;
	Bool done = False;
	Bool is_last_object_left = True;
	unsigned char columns_placed = 0;
	int relief_gap = 0;
	int gap_left;
	int gap_right;
	int chars;

	memset(item_order, 0, sizeof(item_order));
	for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
	{
		lcr_column[i] = 'l';
	}

	/* Now calculate the offsets for the columns. */
	format = MST_ITEM_FORMAT(msp->menu);
	if (!format)
	{
		format = (MST_USE_LEFT_SUBMENUS(msp->menu)) ?
			DEFAULT_LEFT_MENU_ITEM_FORMAT :
			DEFAULT_MENU_ITEM_FORMAT;
	}
	/* Place the individual items off the menu in case they are not
	 * set in the format string. */
	for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
	{
		label_offset[i] = 2 * Scr.MyDisplayWidth;
	}

	x = MST_BORDER_WIDTH(msp->menu);
	while (*format && !done)
	{
		switch (*format)
		{
		case '%':
			format++;
			chars = 0;
			gap_left = 0;
			gap_right = 0;

			/* Insert a gap of %d pixels. */
			if (sscanf(format, "%d.%d%n", &gap_left,
				   &gap_right, &chars) >= 2 ||
			    (sscanf(format, "%d.%n", &gap_left,
				    &chars) >= 1 && chars > 0) ||
			    sscanf(format, "%d%n", &gap_left,
				   &chars) >= 1 ||
			    sscanf(format, ".%d%n", &gap_right,
				   &chars) >= 1)
			{
				if (gap_left > MR_SCREEN_WIDTH(msp->menu) ||
				    gap_left < -MR_SCREEN_WIDTH(msp->menu))
				{
					gap_left = 0;
				}
				if (gap_right > MR_SCREEN_HEIGHT(msp->menu) ||
				    gap_right < -MR_SCREEN_HEIGHT(msp->menu))
				{
					gap_right = 0;
				}
				/* Skip the number. */
				format += chars;
			}
			else if (*format == '.')
			{
				/* Skip a dot without values */
				format++;
			}
			if (!*format)
			{
				break;
			}
			switch (*format)
			{
			case 'l':
			case 'c':
			case 'r':
				/* A left, center or right aligned column. */
				if (columns_placed >=
				    MAX_MENU_ITEM_LABELS)
				{
					break;
				}
				if (
					msp->max.i.label_width[columns_placed]
					<= 0)
				{
					columns_placed++;
					break;
				}
				lcr_column[columns_placed] = *format;
				x += gap_left;
				label_offset[columns_placed] = x;
				x += msp->max.i.label_width[columns_placed] +
					gap_right;
				item_order[used_objects++] =
					&(label_offset[columns_placed]);
				if (is_last_object_left && (*format == 'l'))
				{
					left_objects++;
				}
				else
				{
					is_last_object_left = False;
					if (*format == 'r')
					{
						right_objects++;
					}
					else
					{
						right_objects = 0;
					}
				}
				columns_placed++;
				break;
			case 's':
				/* the sidepic */
				if (sidepic_placed)
				{
					break;
				}
				sidepic_placed = True;
				if (msp->max.sidepic_width <= 0)
				{
					break;
				}
				x += gap_left;
				MR_SIDEPIC_X_OFFSET(msp->menu) = x;
				sidepic_is_left = first;
				sidepic_space = msp->max.sidepic_width +
					((sidepic_is_left) ?
					 gap_left : gap_right);
				x += msp->max.sidepic_width + gap_right;
				break;
			case 'i':
				/* a mini icon */
				if (icons_placed >=
				    MAX_MENU_ITEM_MINI_ICONS)
				{
					break;
				}
				if (msp->max.i.icon_width[icons_placed] > 0)
				{
					x += gap_left;
					MR_ICON_X_OFFSET(msp->menu)
						[icons_placed] = x;
					x += msp->max.i.icon_width
						[icons_placed] + gap_right;
					item_order[used_objects++] =
						&(MR_ICON_X_OFFSET(msp->menu)
						  [icons_placed]);
					if (is_last_object_left)
					{
						left_objects++;
					}
					else
					{
						right_objects++;
					}
				}
				icons_placed++;
				break;
			case '|':
				if (!relief_begin_placed)
				{
					relief_begin_placed = True;
					x += gap_left;
					MR_HILIGHT_X_OFFSET(msp->menu) =
						x;
					x += relief_thickness +
						gap_right;
					relief_gap += gap_right;
					item_order[used_objects++] =
						&(MR_HILIGHT_X_OFFSET(
							  msp->menu));
					if (is_last_object_left)
					{
						left_objects++;
					}
					else
					{
						right_objects = 0;
					}
				}
				else if (!relief_end_placed)
				{
					relief_end_placed = True;
					x += relief_thickness + gap_left;
					/* This is a hack: for now we record
					 * the x coordinate of the end of the
					 * hilight area, but later we'll place
					 * the width in here. */
					MR_HILIGHT_WIDTH(msp->menu) = x;
					x += gap_right;
					relief_gap += gap_left;
					item_order[used_objects++] =
						&(MR_HILIGHT_WIDTH(msp->menu));
					right_objects++;
				}
				break;
			case '>':
			case '<':
				/* the triangle for popup menus */
				if (triangle_placed)
				{
					break;
				}
				triangle_placed = True;
				if (msp->max.i.triangle_width > 0)
				{
					x += gap_left;
					MR_TRIANGLE_X_OFFSET(
						msp->menu) = x;
					MR_IS_LEFT_TRIANGLE(msp->menu) =
						(*format == '<');
					x += msp->max.i.triangle_width +
						gap_right;
					item_order[used_objects++] =
						&(MR_TRIANGLE_X_OFFSET(
							  msp->menu));
					if (is_last_object_left &&
					    *format == '<')
					{
						left_objects++;
					}
					else
					{
						is_last_object_left = False;
						right_objects++;
					}
				}
				break;
			case 'p':
				/* Simply add a gap. */
				x += gap_right + gap_left;
				break;
			case '\t':
				x += MENU_TAB_WIDTH * FlocaleTextWidth(
					MST_PSTDFONT(
						msp->menu), " ", 1);
				break;
			case ' ':
				/* Advance the x position. */
				x += FlocaleTextWidth(
					MST_PSTDFONT(msp->menu), format,
					1);
				break;
			default:
				/* Ignore unknown characters. */
				break;
			} /* switch (*format) */
			break;
		case '\t':
			x += MENU_TAB_WIDTH * FlocaleTextWidth(
				MST_PSTDFONT(msp->menu), " ", 1);
			break;
		case ' ':
			/* Advance the x position. */
			x += FlocaleTextWidth(
				MST_PSTDFONT(msp->menu), format, 1);
			break;
		default:
			/* Ignore unknown characters. */
			break;
		} /* switch (*format) */
		format++;
		first = False;
	} /* while (*format) */
	/* stored for vertical sizing */
	msp->used_item_labels = columns_placed;
	msp->used_mini_icons = icons_placed;

	/* Hide unplaced parts of the menu. */
	if (!sidepic_placed)
	{
		MR_SIDEPIC_X_OFFSET(msp->menu) =
			2 * MR_SCREEN_WIDTH(msp->menu);
	}
	for (i = icons_placed; i < MAX_MENU_ITEM_MINI_ICONS; i++)
	{
		MR_ICON_X_OFFSET(msp->menu)[i] =
			2 * MR_SCREEN_WIDTH(msp->menu);
	}
	if (!triangle_placed)
	{
		MR_TRIANGLE_X_OFFSET(msp->menu) =
			2 * MR_SCREEN_WIDTH(msp->menu);
	}
	msp->flags.is_popup_indicator_used = triangle_placed;

	total_width = x - MST_BORDER_WIDTH(msp->menu);
	d = (sidepic_space + 2 * relief_thickness +
	     max(msp->max.i.title_width, msp->max.i.picture_width)) -
		total_width;
	if (d > 0)
	{
		int m = 1 - left_objects;
		int n = 1 + used_objects - left_objects - right_objects;

		/* The title is larger than all menu items. Stretch the
		 * gaps between the items up to the total width of the
		 * title. */
		for (i = 0; i < used_objects; i++)
		{
			if (i < left_objects)
			{
				continue;
			}
			if (i >= used_objects - right_objects)
			{
				/* Right aligned item. */
				*(item_order[i]) += d;
			}
			else
			{
				/* Neither left nor right aligned item.
				 * Divide the overhead gap evenly
				 * between the items. */
				*(item_order[i]) += d * (m + i) / n;
			}
		}
		total_width += d;
		if (!sidepic_is_left)
		{
			MR_SIDEPIC_X_OFFSET(msp->menu) += d;
		}
	} /* if (d > 0) */
	MR_WIDTH(msp->menu) =
		total_width + 2 * MST_BORDER_WIDTH(msp->menu);
	MR_ITEM_WIDTH(msp->menu) = total_width - sidepic_space;
	MR_ITEM_X_OFFSET(msp->menu) = MST_BORDER_WIDTH(msp->menu);
	if (sidepic_is_left)
	{
		MR_ITEM_X_OFFSET(msp->menu) += sidepic_space;
	}
	if (!relief_begin_placed)
	{
		MR_HILIGHT_X_OFFSET(msp->menu) = MR_ITEM_X_OFFSET(msp->menu);
	}
	if (relief_end_placed)
	{
		MR_HILIGHT_WIDTH(msp->menu) =
			MR_HILIGHT_WIDTH(msp->menu) -
			MR_HILIGHT_X_OFFSET(msp->menu);
	}
	else
	{
		MR_HILIGHT_WIDTH(msp->menu) =
			MR_ITEM_WIDTH(msp->menu) +
			MR_ITEM_X_OFFSET(msp->menu) -
			MR_HILIGHT_X_OFFSET(msp->menu);
	}

	/* Now calculate the offsets for the individual labels. */
	for (mi = MR_FIRST_ITEM(msp->menu); mi != NULL; mi = MI_NEXT_ITEM(mi))
	{
		for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
		{
			if (MI_LABEL(mi)[i] == NULL)
			{
				continue;
			}
			if (!MI_IS_TITLE(mi) || !MI_IS_TITLE_CENTERED(mi))
			{
				switch (lcr_column[i])
				{
				case 'l':
					MI_LABEL_OFFSET(mi)[i] =
						label_offset[i];
					break;
				case 'c':
					MI_LABEL_OFFSET(mi)[i] =
						label_offset[i] +
						(msp->max.i.label_width[i] -
						 MI_LABEL_OFFSET(mi)[i]) / 2;
					break;
				case 'r':
					MI_LABEL_OFFSET(mi)[i] =
						label_offset[i] +
						msp->max.i.label_width[i] -
						MI_LABEL_OFFSET(mi)[i];
					break;
				}
			}
			else
			{
				/* This is a centered title item (indicated by
				 * negative width). */
				MI_LABEL_OFFSET(mi)[i] =
					menudim_middle_x_offset(
						&MR_DIM(msp->menu)) -
					MI_LABEL_OFFSET(mi)[i] / 2;
			}
		} /* for */
	} /* for */

	return;
}

static int calc_more_item_height(MenuSizingParameters *msp)
{
	int height;

	height =
		MST_PSTDFONT(msp->menu)->height +
		MST_ITEM_GAP_ABOVE(msp->menu) +
		MST_ITEM_GAP_BELOW(msp->menu) +
		MST_RELIEF_THICKNESS(msp->menu);

	return height;
}

static int calc_normal_item_height(MenuSizingParameters *msp, MenuItem *mi)
{
	int height;

	height =
		MST_ITEM_GAP_ABOVE(msp->menu) +
		MST_ITEM_GAP_BELOW(msp->menu) +
		MST_RELIEF_THICKNESS(msp->menu);
	/* Normal text entry or an entry with a sub menu triangle */
	if (
		(MI_HAS_TEXT(mi) && msp->used_item_labels) ||
		(MI_IS_POPUP(mi) &&
		 msp->flags.is_popup_indicator_used))
	{
		height += MST_PSTDFONT(msp->menu)->height;
	}

	return height;
}

/*
 *
 * Calculate the positions of the columns in the menu.
 * Called by make_menu().
 *
 */
static Bool size_menu_vertically(MenuSizingParameters *msp)
{
	MenuItem *mi;
	int y;
	int cItems;
	int relief_thickness = MST_RELIEF_THICKNESS(msp->menu);
	int i;
	Bool has_continuation_menu = False;

	MR_ITEM_TEXT_Y_OFFSET(msp->menu) =
		MST_PSTDFONT(msp->menu)->ascent + relief_thickness +
		MST_ITEM_GAP_ABOVE(msp->menu);

	/* mi_prev trails one behind mi, since we need to move that
	   into a newly-made menu if we run out of space */
	y = MST_BORDER_WIDTH(msp->menu) + MST_VERTICAL_MARGIN_TOP(msp->menu);
	for (
		cItems = 0, mi = MR_FIRST_ITEM(msp->menu); mi != NULL;
		mi = MI_NEXT_ITEM(mi), cItems++)
	{
		Bool last_item_has_relief =
			(MI_PREV_ITEM(mi)) ?
			MI_IS_SELECTABLE(MI_PREV_ITEM(mi)) : False;
		Bool has_mini_icon = False;
		int separator_height;
		int menu_height;

		separator_height = (last_item_has_relief) ?
			MENU_SEPARATOR_HEIGHT + relief_thickness :
			MENU_SEPARATOR_TOTAL_HEIGHT;
		MI_Y_OFFSET(mi) = y;
		if (MI_IS_TITLE(mi))
		{
			MI_HEIGHT(mi) = MST_PTITLEFONT(msp->menu)->height +
				MST_TITLE_GAP_ABOVE(msp->menu) +
				MST_TITLE_GAP_BELOW(msp->menu);
		}
		else if (MI_IS_SEPARATOR(mi))
		{
			/* Separator */
			MI_HEIGHT(mi) = separator_height;
		}
		else if (MI_IS_TEAR_OFF_BAR(mi))
		{
			/* Tear off bar */
			MI_HEIGHT(mi) = relief_thickness +
				MENU_TEAR_OFF_BAR_HEIGHT;
		}
		else
		{
			MI_HEIGHT(mi) = calc_normal_item_height(msp, mi);
		}
		if (MI_IS_TITLE(mi))
		{
			/* add space for the underlines */
			switch (MST_TITLE_UNDERLINES(msp->menu))
			{
			case 0:
				if (last_item_has_relief)
					MI_HEIGHT(mi) += relief_thickness;
				break;
			case 1:
				if (mi != MR_FIRST_ITEM(msp->menu))
				{
					/* Space to draw the separator plus a
					 * gap above */
					MI_HEIGHT(mi) += separator_height;
				}
				if (MI_NEXT_ITEM(mi) != NULL)
				{
					/* Space to draw the separator */
					MI_HEIGHT(mi) += MENU_SEPARATOR_HEIGHT;
				}
				break;
			default:
				/* Space to draw n underlines. */
				MI_HEIGHT(mi) +=
					MENU_UNDERLINE_HEIGHT *
					MST_TITLE_UNDERLINES(msp->menu);
				if (last_item_has_relief)
					MI_HEIGHT(mi) += relief_thickness;
				break;
			}
		}
		for (i = 0; i < msp->used_mini_icons; i++)
		{
			if (MI_MINI_ICON(mi)[i])
			{
				has_mini_icon = True;
			}
			if (MI_MINI_ICON(mi)[i] &&
			    MI_HEIGHT(mi) <
			    MI_MINI_ICON(mi)[i]->height + relief_thickness)
			{
				MI_HEIGHT(mi) = MI_MINI_ICON(mi)[i]->height +
					relief_thickness;
			}
		}
		if (MI_PICTURE(mi))
		{
			if ((MI_HAS_TEXT(mi) && msp->used_item_labels) ||
			    has_mini_icon)
			{
				MI_HEIGHT(mi) += MI_PICTURE(mi)->height;
			}
			else
			{
				MI_HEIGHT(mi) = MI_PICTURE(mi)->height +
					relief_thickness;
			}
		}
		y += MI_HEIGHT(mi);
		/* this item would have to be the last item, or else
		 * we need to add a "More..." entry pointing to a new menu */
		menu_height =
			y + MST_BORDER_WIDTH(msp->menu) +
			MST_VERTICAL_MARGIN_BOTTOM(msp->menu) +
			((MI_IS_SELECTABLE(mi)) ? relief_thickness : 0);
		if (menu_height > MR_SCREEN_HEIGHT(msp->menu))
		{
			/* Item does not fit on screen anymore. */
			char *t;
			char *tempname;
			const char *gt_name;
			char *name;
			MenuRoot *menuContinuation;
			int more_item_height;

			more_item_height = calc_more_item_height(msp);
			/* Remove items form the menu until it fits (plus a
			 * 'More' entry). */
			while (
				MI_PREV_ITEM(mi) != NULL &&
				menu_height > MR_SCREEN_HEIGHT(msp->menu))
			{
				/* Remove current item. */
				y -= MI_HEIGHT(mi);
				mi = MI_PREV_ITEM(mi);
				cItems--;
				menu_height =
					y + MST_BORDER_WIDTH(msp->menu) +
					more_item_height + relief_thickness +
					MST_VERTICAL_MARGIN_BOTTOM(msp->menu);
			}
			if (
				MI_PREV_ITEM(mi) == NULL ||
				menu_height > MR_SCREEN_HEIGHT(msp->menu))
			{
				fvwm_msg(ERR, "size_menu_vertically",
					 "Menu entry does not fit on screen");
				/* leave a coredump */
				abort();
				exit(1);
			}

			t = EscapeString(MR_NAME(msp->menu), "\"", '\\');
			tempname = xmalloc(
				(10 + strlen(t)) * sizeof(char));
			strcpy(tempname, "Popup \"");
			strcat(tempname, t);
			strcat(tempname, "$\"");
			free(t);
			/* NewMenuRoot inserts at the head of the list of menus
			   but, we need it at the end.  (Give it just the name,
			   * which is 6 chars past the action since
			   * strlen("Popup ")==6 ) */
			t = xmalloc(strlen(MR_NAME(msp->menu)) + 2);
			strcpy(t, MR_NAME(msp->menu));
			strcat(t, "$");
			menuContinuation = NewMenuRoot(t);
			free(t);
			MR_CONTINUATION_MENU(msp->menu) = menuContinuation;

			/* Now move this item and the remaining items into the
			 * new menu */
			MR_FIRST_ITEM(menuContinuation) = MI_NEXT_ITEM(mi);
			MR_LAST_ITEM(menuContinuation) =
				MR_LAST_ITEM(msp->menu);
			MR_ITEMS(menuContinuation) =
				MR_ITEMS(msp->menu) - cItems;
			MI_PREV_ITEM(MI_NEXT_ITEM(mi)) = NULL;

			/* mi_prev is now the last item in the parent menu */
			MR_LAST_ITEM(msp->menu) = mi;
			MR_ITEMS(msp->menu) = cItems;
			MI_NEXT_ITEM(mi) = NULL;

			/* use the same style for the submenu */
			MR_STYLE(menuContinuation) = MR_STYLE(msp->menu);
			MR_IS_LEFT_TRIANGLE(menuContinuation) =
				MR_IS_LEFT_TRIANGLE(msp->menu);
			/* migo: propagate missing_submenu_func */
			if (MR_MISSING_SUBMENU_FUNC(msp->menu))
			{
				MR_MISSING_SUBMENU_FUNC(menuContinuation) =
					xstrdup(MR_MISSING_SUBMENU_FUNC(
							   msp->menu));
			}
			/* don't propagate sidepic, sidecolor, popup and
			 * popdown actions */
			/* And add the entry pointing to the new menu */
			gt_name = gettext("More&...");
			name = xstrdup(gt_name);
			AddToMenu(
				msp->menu, name, tempname,
				False /* no pixmap scan */, False, True);
			free(name);
			free(tempname);
			has_continuation_menu = True;
		}
	} /* for */
	/* The menu may be empty here! */
	if (MR_LAST_ITEM(msp->menu) != NULL &&
	    MI_IS_SELECTABLE(MR_LAST_ITEM(msp->menu)))
	{
		y += relief_thickness;
	}
	MR_HEIGHT(msp->menu) =
		y + MST_BORDER_WIDTH(msp->menu) +
		MST_VERTICAL_MARGIN_BOTTOM(msp->menu);

	return has_continuation_menu;
}

/*
 *
 * Merge menu continuations back into the original menu.
 * Called by make_menu().
 *
 */
static void merge_continuation_menus(MenuRoot *mr)
{
	/* merge menu continuations into one menu again - needed when changing
	 * the font size of a long menu. */
	while (MR_CONTINUATION_MENU(mr) != NULL)
	{
		MenuRoot *cont = MR_CONTINUATION_MENU(mr);

		/* link first item of continuation to item before 'more...' */
		MI_NEXT_ITEM(MI_PREV_ITEM(MR_LAST_ITEM(mr))) =
			MR_FIRST_ITEM(cont);
		MI_PREV_ITEM(MR_FIRST_ITEM(cont)) =
			MI_PREV_ITEM(MR_LAST_ITEM(mr));
		menuitem_free(MR_LAST_ITEM(mr));
		MR_LAST_ITEM(mr) = MR_LAST_ITEM(cont);
		MR_CONTINUATION_MENU(mr) = MR_CONTINUATION_MENU(cont);
		/* fake an empty menu so that DestroyMenu does not destroy the
		 * items. */
		MR_FIRST_ITEM(cont) = NULL;
		DestroyMenu(cont, False, False);
	}

	return;
}

/*
 *
 * Creates the window for the menu.
 *
 */
static void make_menu_window(MenuRoot *mr, Bool is_tear_off)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	int w;
	int h;
	unsigned int evmask;

	w = MR_WIDTH(mr);
	if (w == 0)
	{
		w = 1;
	}
	h = MR_HEIGHT(mr);
	if (h == 0)
	{
		h = 1;
	}

	attributes.background_pixel = (MST_HAS_MENU_CSET(mr)) ?
		Colorset[MST_CSET_MENU(mr)].bg : MST_MENU_COLORS(mr).back;
	if (MR_WINDOW(mr) != None)
	{
		/* just resize the existing window */
		XResizeWindow(dpy, MR_WINDOW(mr), w, h);
		/* and change the background color */
		valuemask = CWBackPixel | CWCursor;
		XChangeWindowAttributes(
			dpy, MR_WINDOW(mr), valuemask, &attributes);
	}
	else
	{
		/* create a new window */
		valuemask = CWBackPixel | CWEventMask | CWCursor | CWColormap
			| CWBorderPixel | CWSaveUnder;
		attributes.border_pixel = 0;
		attributes.colormap = Pcmap;
		evmask = XEVMASK_MENUW;
		attributes.event_mask = 0;
		attributes.cursor = Scr.FvwmCursors[CRS_MENU];
		attributes.save_under = True;

		/* Create a display used to create the window.  Can't use the
		 * normal display because 'xkill' would  kill the window
		 * manager if used on a tear off menu.  The display can't be
		 * deleted right now because that would either destroy the new
		 * window or leave it as an orphan if fvwm dies or is
		 * restarted. */
		if (is_tear_off)
		{
			MR_CREATE_DPY(mr) = XOpenDisplay(display_name);
			if (MR_CREATE_DPY(mr) == NULL)
			{
				/* Doh.  Use the standard display instead. */
				MR_CREATE_DPY(mr) = dpy;
			}
		}
		else
		{
			MR_CREATE_DPY(mr) = dpy;
		}
		MR_WINDOW(mr) = XCreateWindow(
			MR_CREATE_DPY(mr), Scr.Root, 0, 0, w, h,
			0, Pdepth, InputOutput, Pvisual, valuemask,
			&attributes);
		if (MR_CREATE_DPY(mr) != dpy)
		{
			/* We *must* synchronize the display here.  Otherwise
			 * the request will never be processed. */
			XSync(MR_CREATE_DPY(mr), 1);
		}
		if (MR_WINDOW(mr) != None)
		{
			/* select events for the window from the standard
			 * display */
			XSelectInput(dpy, MR_WINDOW(mr), evmask);
		}
		XSaveContext(dpy, MR_WINDOW(mr), MenuContext,(caddr_t)mr);
	}

	return;
}


/*
 *
 * Generates the window for a menu
 *
 */
static void make_menu(MenuRoot *mr, Bool is_tear_off)
{
	MenuSizingParameters msp;
	Bool has_continuation_menu = False;

	if (MR_MAPPED_COPIES(mr) > 0)
	{
		return;
	}
	merge_continuation_menus(mr);
	do
	{
		memset(&msp, 0, sizeof(MenuSizingParameters));
		msp.menu = mr;
		calculate_item_sizes(&msp);
		/* Call size_menu_horizontally first because it calculated
		 * some values used by size_menu_vertically. */
		size_menu_horizontally(&msp);
		has_continuation_menu = size_menu_vertically(&msp);
		/* repeat this step if the menu was split */
	} while (has_continuation_menu);
	MR_USED_MINI_ICONS(mr) = msp.used_mini_icons;

	MR_XANIMATION(mr) = 0;
	memset(&(MR_DYNAMIC_FLAGS(mr)), 0, sizeof(MR_DYNAMIC_FLAGS(mr)));

	/* create a new window for the menu */
	make_menu_window(mr, is_tear_off);
	MR_IS_UPDATED(mr) = 0;

	return;
}

/* Make sure the menu is properly rebuilt when the style or the menu has
 * changed. */
static void update_menu(MenuRoot *mr, MenuParameters *pmp)
{
	int sw;
	int sh;
	Bool has_screen_size_changed = False;
	fscreen_scr_arg fscr;

	if (MST_IS_UPDATED(mr))
	{
		/* The menu style has changed. */
		MenuRoot *menu;

		for (menu = Menus.all; menu; menu = MR_NEXT_MENU(menu))
		{
			if (MR_STYLE(menu) == MR_STYLE(mr))
			{
				/* Mark all other menus with the same style as
				 * changed. */
				MR_IS_UPDATED(menu) = 1;
			}
		}
		MST_IS_UPDATED(mr) = 0;
	}
	fscr.xypos.x = pmp->screen_origin_x;
	fscr.xypos.y = pmp->screen_origin_y;
	FScreenGetScrRect(&fscr, FSCREEN_XYPOS, &JunkX, &JunkY, &sw, &sh);
	if (sw != MR_SCREEN_WIDTH(mr) || sh != MR_SCREEN_HEIGHT(mr))
	{
		has_screen_size_changed = True;
		MR_SCREEN_WIDTH(mr) = sw;
		MR_SCREEN_HEIGHT(mr) = sh;
	}
	if (MR_IS_UPDATED(mr) || has_screen_size_changed)
	{
		/* The menu or the screen dimensions have changed. We have to
		 * re-make it. */
		make_menu(mr, False);
	}

	return;
}

/*
 *
 *  Procedure:
 *      copy_menu_root - creates a new instance of an existing menu
 *
 *  Returned Value:
 *      (MenuRoot *)
 *
 *  Inputs:
 *      mr      - the MenuRoot structure of the existing menu
 *
 */
static MenuRoot *copy_menu_root(MenuRoot *mr)
{
	MenuRoot *tmp;

	if (!mr || MR_COPIES(mr) >= MAX_MENU_COPIES)
	{
		return NULL;
	}
	tmp = xmalloc(sizeof(*tmp));
	tmp->d = xcalloc(1, sizeof(MenuRootDynamic));
	tmp->s = mr->s;

	MR_COPIES(mr)++;
	MR_ORIGINAL_MENU(tmp) = MR_ORIGINAL_MENU(mr);
	MR_CONTINUATION_MENU(tmp) = MR_CONTINUATION_MENU(mr);
	MR_NEXT_MENU(tmp) = MR_NEXT_MENU(mr);
	MR_NEXT_MENU(mr) = tmp;
	MR_WINDOW(tmp) = None;
	memset(&(MR_DYNAMIC_FLAGS(tmp)), 0, sizeof(MR_DYNAMIC_FLAGS(tmp)));

	return tmp;
}

/*
 *
 *  Procedure:
 *      clone_menu - duplicates an existing menu in newly allocated
 memory.  The new menu is independent of the original.
 *
 *  Returned Value:
 *      (MenuRoot *)
 *
 *  Inputs:
 *      mr      - the MenuRoot structure of the existing menu
 *
 */
static void clone_menu_root_static(
	MenuRoot *dest_mr, MenuRoot *src_mr)
{
	dest_mr->s = xmalloc(sizeof(MenuRootStatic));
	/* copy everything */
	memcpy(dest_mr->s, src_mr->s, sizeof(MenuRootStatic));
	/* special treatment for a few parts */
	if (MR_NAME(src_mr) != NULL)
	{
		MR_NAME(dest_mr) = xstrdup(MR_NAME(src_mr));
	}
	MR_COPIES(dest_mr) = 1;
	MR_MAPPED_COPIES(dest_mr) = 0;
	MR_POPUP_ACTION(dest_mr) = NULL;
	MR_POPDOWN_ACTION(dest_mr) = NULL;
	if (MR_MISSING_SUBMENU_FUNC(src_mr))
	{
		MR_MISSING_SUBMENU_FUNC(dest_mr) =
			xstrdup(MR_MISSING_SUBMENU_FUNC(src_mr));
	}
	if (MR_HAS_SIDECOLOR(src_mr))
	{
		MR_SIDECOLOR(dest_mr) =
			fvwmlib_clone_color(MR_SIDECOLOR(src_mr));
	}
	MR_SIDEPIC(dest_mr) = PCloneFvwmPicture(MR_SIDEPIC(src_mr));
	clone_menu_item_list(dest_mr, src_mr);

	return;
}

static MenuRoot *clone_menu(MenuRoot *mr)
{
	MenuRoot *new_mr;

	new_mr = xmalloc(sizeof *new_mr);
	new_mr->d = xcalloc(1, sizeof(MenuRootDynamic));
	clone_menu_root_static(new_mr, mr);

	return new_mr;
}

/* ---------------------------- position hints ----------------------------- */

static int float_to_int_with_tolerance(float f)
{
	int low;

	if (f < 0)
	{
		low = (int)(f - ROUNDING_ERROR_TOLERANCE);
	}
	else
	{
		low = (int)(f + ROUNDING_ERROR_TOLERANCE);
	}
	if ((int)f != low)
	{
		return low;
	}
	else
	{
		return (int)(f);
	}
}

static void get_xy_from_position_hints(
	struct MenuPosHints *ph, int width, int height, int context_width,
	Bool do_reverse_x, int *ret_x, int *ret_y)
{
	float x_add;
	float y_add;

	*ret_x = ph->x;
	*ret_y = ph->y;
	if (ph->is_menu_relative)
	{
		if (do_reverse_x)
		{
			*ret_x -= ph->x_offset;
			x_add = width * (-1.0 - ph->x_factor) +
				ph->menu_width * (1.0 - ph->context_x_factor);
		}
		else
		{
			*ret_x += ph->x_offset;
			x_add = width * ph->x_factor +
				ph->menu_width * ph->context_x_factor;
		}
		y_add = height * ph->y_factor;
	}
	else
	{
		x_add = width * ph->x_factor;
		y_add = height * ph->y_factor;
	}
	*ret_x += float_to_int_with_tolerance(x_add);
	*ret_y += float_to_int_with_tolerance(y_add);

	return;
}

/*
 * Used by get_menu_options
 *
 * The vars are named for the x-direction, but this is used for both x and y
 */
static char *get_one_menu_position_argument(
	char *action, int x, int w, int *pFinalX, int *x_offset,
	float *width_factor, float *context_width_factor,
	Bool *is_menu_relative)
{
	char *token, *orgtoken, *naction;
	char c;
	int val;
	int chars;
	float fval;
	float factor = (float)w/100;
	float x_add = 0;

	naction = GetNextToken(action, &token);
	if (token == NULL)
	{
		return action;
	}
	orgtoken = token;
	*pFinalX = x;
	*x_offset = 0;
	*width_factor = 0.0;
	*context_width_factor = 0.0;
	if (sscanf(token,"o%d%n", &val, &chars) >= 1)
	{
		fval = val;
		token += chars;
		x_add += fval*factor;
		*width_factor -= fval / 100.0;
		*context_width_factor += fval / 100.0;
	}
	else if (token[0] == 'c')
	{
		token++;
		x_add += ((float)w) / 2.0;
		*width_factor -= 0.5;
		*context_width_factor += 0.5;
	}
	while (*token != 0)
	{
		if (sscanf(token,"%d%n", &val, &chars) < 1)
		{
			naction = action;
			break;
		}
		fval = (float)val;
		token += chars;
		if (sscanf(token,"%c", &c) == 1)
		{
			switch (c)
			{
			case 'm':
				token++;
				*width_factor += fval / 100.0;
				*is_menu_relative = True;
				break;
			case 'p':
				token++;
				x_add += val;
				*x_offset += val;
				break;
			default:
				x_add += fval * factor;
				*context_width_factor += fval / 100.0;
				break;
			}
		}
		else
		{
			x_add += fval * factor;
			*context_width_factor += fval / 100.0;
		}
	}
	*pFinalX += float_to_int_with_tolerance(x_add);
	free(orgtoken);

	return naction;
}

/* Returns the menu options for the menu that a given menu item pops up */
static void get_popup_options(
	MenuParameters *pmp, MenuItem *mi, MenuOptions *pops)
{
	if (!mi)
	{
		return;
	}
	pops->flags.has_poshints = 0;
	pops->pos_hints.has_screen_origin = True;
	pops->pos_hints.screen_origin_x = pmp->screen_origin_x;
	pops->pos_hints.screen_origin_y = pmp->screen_origin_y;
	/* just look past "Popup <name>" in the action */
	get_menu_options(
		SkipNTokens(MI_ACTION(mi), 2), MR_WINDOW(pmp->menu), NULL,
		NULL, pmp->menu, mi, pops);

	return;
}

/* ---------------------------- menu painting functions --------------------- */

static void clear_expose_menu_area(Window win, XEvent *e)
{
	if (e == NULL)
	{
		XClearWindow(dpy, win);
	}
	else
	{
		XClearArea(
			dpy, win, e->xexpose.x, e->xexpose.y, e->xexpose.width,
			e->xexpose.height, False);
	}

	return;
}

/*
 *
 * Draws a picture on the left side of the menu
 * What about a SidePic Colorset ? (olicha 2002-08-21)
 *
 */
static void paint_side_pic(MenuRoot *mr, XEvent *pevent)
{
	GC gc;
	FvwmPicture *sidePic;
	int ys;
	int yt;
	int h;
	int bw = MST_BORDER_WIDTH(mr);

	if (MR_SIDEPIC(mr))
	{
		sidePic = MR_SIDEPIC(mr);
	}
	else if (MST_SIDEPIC(mr))
	{
		sidePic = MST_SIDEPIC(mr);
	}
	else
	{
		return;
	}
	if (Pdepth < 2)
	{
		/* ? */
		gc = SHADOW_GC(MST_MENU_INACTIVE_GCS(mr));
	}
	else
	{
		gc = FORE_GC(MST_MENU_INACTIVE_GCS(mr));
	}
	if (sidePic->height > MR_HEIGHT(mr) - 2 * bw)
	{
		h = MR_HEIGHT(mr) - 2 * bw;
		ys = sidePic->height - h;
		yt = bw;
	}
	else
	{
		h = sidePic->height;
		ys = 0;
		yt = MR_HEIGHT(mr) - bw - sidePic->height;
	}
	if (pevent != NULL && pevent->type == Expose)
	{
		if (
			pevent->xexpose.x + pevent->xexpose.width <
			MR_SIDEPIC_X_OFFSET(mr) ||
			pevent->xexpose.x >=
			MR_SIDEPIC_X_OFFSET(mr) +  sidePic->width)
		{
			/* out of x-range for side bar */
			return;
		}
		if (
			pevent->xexpose.y + pevent->xexpose.height < bw ||
			pevent->xexpose.y >= bw + MR_HEIGHT(mr))
		{
			/* in the border */
			return;
		}
		if (
			!(MR_HAS_SIDECOLOR(mr) || MST_HAS_SIDE_COLOR(mr)) &&
			pevent->xexpose.y + pevent->xexpose.height < yt)
		{
			/* outside picture and no background */
			return;
		}

	}
	if (MR_HAS_SIDECOLOR(mr))
	{
		Globalgcv.foreground = MR_SIDECOLOR(mr);
	}
	else if (MST_HAS_SIDE_COLOR(mr))
	{
		Globalgcv.foreground = MST_SIDE_COLOR(mr);
	}
	if (MR_HAS_SIDECOLOR(mr) || MST_HAS_SIDE_COLOR(mr))
	{
		Globalgcm = GCForeground;
		XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
		XFillRectangle(
			dpy, MR_WINDOW(mr), Scr.ScratchGC1,
			MR_SIDEPIC_X_OFFSET(mr), bw, sidePic->width,
			MR_HEIGHT(mr) - 2 * bw);
	}
	else if (sidePic->alpha != None)
	{
		XClearArea(
			dpy, MR_WINDOW(mr),
			MR_SIDEPIC_X_OFFSET(mr), yt, sidePic->width, h, False);
	}
	PGraphicsRenderPicture(
		dpy, MR_WINDOW(mr), sidePic, 0, MR_WINDOW(mr),
		gc, Scr.MonoGC, Scr.AlphaGC,
		0, ys, sidePic->width, h,
		MR_SIDEPIC_X_OFFSET(mr), yt, sidePic->width, h, False);

	return;
}

static Bool paint_menu_gradient_background(
	MenuRoot *mr, XEvent *pevent)
{
	MenuStyle *ms = MR_STYLE(mr);
	int bw = MST_BORDER_WIDTH(mr);
	XRectangle bounds;
	Pixmap pmap;
	GC pmapgc;
	XGCValues gcv;
	unsigned long gcm = GCLineWidth;
	int switcher = -1;
	Bool do_clear = False;

	gcv.line_width = 1;
	bounds.x = bw;
	bounds.y = bw;
	bounds.width = MR_WIDTH(mr) - bw;
	bounds.height = MR_HEIGHT(mr) - bw;
	/* H, V, D and B gradients are optimized and have
	 * their own code here. (if no dither) */
	if (!ST_FACE(ms).u.grad.do_dither)
	{
		switcher = ST_FACE(ms).gradient_type;
	}
	switch (switcher)
	{
	case H_GRADIENT:
		if (MR_IS_BACKGROUND_SET(mr) == False)
		{
			register int i;
			register int dw;

			pmap = XCreatePixmap(
				dpy, MR_WINDOW(mr), MR_WIDTH(mr),
				DEFAULT_MENU_GRADIENT_PIXMAP_THICKNESS,
				Pdepth);
			pmapgc = fvwmlib_XCreateGC(dpy, pmap, gcm, &gcv);
			dw = (float)
				(bounds.width / ST_FACE(ms).u.grad.npixels)
				+ 1;
			for (i = 0; i < ST_FACE(ms).u.grad.npixels; i++)
			{
				int x;

				x = i * bounds.width /
					ST_FACE(ms).u.grad.npixels;
				XSetForeground(
					dpy, pmapgc,
					ST_FACE(ms).u.grad.xcs[i].pixel);
				XFillRectangle(
					dpy, pmap, pmapgc, x, 0, dw,
					DEFAULT_MENU_GRADIENT_PIXMAP_THICKNESS);
			}
			XSetWindowBackgroundPixmap(dpy, MR_WINDOW(mr), pmap);
			XFreeGC(dpy,pmapgc);
			XFreePixmap(dpy,pmap);
			MR_IS_BACKGROUND_SET(mr) = True;
		}
		do_clear = True;
		break;
	case V_GRADIENT:
		if (MR_IS_BACKGROUND_SET(mr) == False)
		{
			register int i;
			register int dh;
			static int best_tile_width = 0;
			int junk;

			if (best_tile_width == 0)
			{
				int tw = DEFAULT_MENU_GRADIENT_PIXMAP_THICKNESS;

				if (!XQueryBestTile(
					    dpy, Scr.screen, tw, tw,
					    (unsigned int*)&best_tile_width,
					    (unsigned int*)&junk))
				{
					/* call failed, use default and risk a
					 * screwed up tile */
					best_tile_width = tw;
				}
			}
			pmap = XCreatePixmap(
				dpy, MR_WINDOW(mr), best_tile_width,
				MR_HEIGHT(mr), Pdepth);
			pmapgc = fvwmlib_XCreateGC(dpy, pmap, gcm, &gcv);
			dh = (float) (bounds.height /
				      ST_FACE(ms).u.grad.npixels) + 1;
			for (i = 0; i < ST_FACE(ms).u.grad.npixels; i++)
			{
				int y;

				y = i * bounds.height /
					ST_FACE(ms).u.grad.npixels;
				XSetForeground(
					dpy, pmapgc,
					ST_FACE(ms).u.grad.xcs[i].pixel);
				XFillRectangle(
					dpy, pmap, pmapgc, 0, y,
					best_tile_width, dh);
			}
			XSetWindowBackgroundPixmap(dpy, MR_WINDOW(mr), pmap);
			XFreeGC(dpy,pmapgc);
			XFreePixmap(dpy,pmap);
			MR_IS_BACKGROUND_SET(mr) = True;
		}
		do_clear = True;
		break;
	case D_GRADIENT:
	case B_GRADIENT:
	{
		register int i = 0, numLines;
		int cindex = -1;
		XRectangle r;

		if (pevent)
		{
			r.x = pevent->xexpose.x;
			r.y = pevent->xexpose.y;
			r.width = pevent->xexpose.width;
			r.height = pevent->xexpose.height;
		}
		else
		{
			r.x = bw;
			r.y = bw;
			r.width = MR_WIDTH(mr) - 2 * bw;
			r.height = MR_HEIGHT(mr) - 2 * bw;
		}
		XSetClipRectangles(
			dpy, Scr.TransMaskGC, 0, 0, &r, 1, Unsorted);
		numLines = MR_WIDTH(mr) + MR_HEIGHT(mr) - 2 * bw;
		for (i = 0; i < numLines; i++)
		{
			if ((int)(i * ST_FACE(ms).u.grad.npixels / numLines) >
			    cindex)
			{
				/* pick the next colour (skip if necc.) */
				cindex = i * ST_FACE(ms).u.grad.npixels /
					numLines;
				XSetForeground(
					dpy, Scr.TransMaskGC,
					ST_FACE(ms).u.grad.xcs[cindex].pixel);
			}
			if (ST_FACE(ms).gradient_type == D_GRADIENT)
			{
				XDrawLine(dpy, MR_WINDOW(mr),
					  Scr.TransMaskGC, 0, i, i, 0);
			}
			else /* B_GRADIENT */
			{
				XDrawLine(dpy, MR_WINDOW(mr), Scr.TransMaskGC,
					  0, MR_HEIGHT(mr) - 1 - i, i,
					  MR_HEIGHT(mr) - 1);
			}
		}
	}
	XSetClipMask(dpy, Scr.TransMaskGC, None);
	break;
	default:
		if (MR_IS_BACKGROUND_SET(mr) == False)
		{
			int g_width;
			int g_height;

			/* let library take care of all other gradients */
			pmap = XCreatePixmap(
				dpy, MR_WINDOW(mr), MR_WIDTH(mr),
				MR_HEIGHT(mr), Pdepth);
			pmapgc = fvwmlib_XCreateGC(dpy, pmap, gcm, &gcv);

			/* find out the size the pixmap should be */
			CalculateGradientDimensions(
				dpy, MR_WINDOW(mr), ST_FACE(ms).u.grad.npixels,
				ST_FACE(ms).gradient_type,
				ST_FACE(ms).u.grad.do_dither, &g_width,
				&g_height);
			/* draw the gradient directly into the window */
			CreateGradientPixmap(
				dpy, MR_WINDOW(mr), pmapgc,
				ST_FACE(ms).gradient_type, g_width, g_height,
				ST_FACE(ms).u.grad.npixels,
				ST_FACE(ms).u.grad.xcs,
				ST_FACE(ms).u.grad.do_dither,
				&(MR_STORED_PIXELS(mr).d_pixels),
				&(MR_STORED_PIXELS(mr).d_npixels),
				pmap, bw, bw,
				MR_WIDTH(mr) - bw, MR_HEIGHT(mr) - bw, NULL);
			XSetWindowBackgroundPixmap(dpy, MR_WINDOW(mr), pmap);
			XFreeGC(dpy, pmapgc);
			XFreePixmap(dpy, pmap);
			MR_IS_BACKGROUND_SET(mr) = True;
		}
		do_clear = True;
		break;
	}
	return do_clear;
}

static Bool paint_menu_pixmap_background(
	MenuRoot *mr, XEvent *pevent)
{
	MenuStyle *ms = MR_STYLE(mr);
	int width, height, x, y;
	int bw = MST_BORDER_WIDTH(mr);
	FvwmPicture *p;

	p = ST_FACE(ms).u.p;
	width = MR_WIDTH(mr) - 2 * bw;
	height = MR_HEIGHT(mr) - 2 * bw;
	y = (int)(height - p->height) / 2;
	x = (int)(width - p->width) / 2;
	if (x < bw)
	{
		x = bw;
	}
	if (y < bw)
	{
		y = bw;
	}
	if (width > p->width)
	{
		width = p->width;
	}
	if (height > p->height)
	{
		height = p->height;
	}
	if (width > MR_WIDTH(mr) - x - bw)
	{
		width = MR_WIDTH(mr) - x - bw;
	}
	if (height > MR_HEIGHT(mr) - y - bw)
	{
		height = MR_HEIGHT(mr) - y - bw;
	}
	XSetClipMask(dpy, Scr.TransMaskGC, p->mask);
	XSetClipOrigin(dpy, Scr.TransMaskGC, x, y);
	XCopyArea(
		dpy, p->picture, MR_WINDOW(mr), Scr.TransMaskGC,
		bw, bw, width, height, x, y);

	return False;
}

/*
 *
 *  Procedure:
 *      get_menu_paint_item_parameters - prepares the parameters to be
 *      passed to menuitem_paint().
 *
 *      mr - the menu instance that holds the menu item
 *      mi - the menu item to redraw
 *      fw - the FvwmWindow structure to check against allowed functions
 *
 */
static void get_menu_paint_item_parameters(
	MenuPaintItemParameters *mpip, MenuRoot *mr, MenuItem *mi,
	FvwmWindow *fw, XEvent *pevent, Bool do_redraw_menu_border)
{
	mpip->ms = MR_STYLE(mr);
	mpip->w = MR_WINDOW(mr);
	mpip->selected_item = MR_SELECTED_ITEM(mr);
	mpip->dim = &MR_DIM(mr);
	mpip->fw = fw;
	mpip->used_mini_icons = MR_USED_MINI_ICONS(mr);
	mpip->cb_mr = mr;
	mpip->cb_reset_bg = paint_menu_gradient_background;
	mpip->flags.is_first_item = (MR_FIRST_ITEM(mr) == mi);
	mpip->flags.is_left_triangle = MR_IS_LEFT_TRIANGLE(mr);
	mpip->ev = pevent;

	return;
}

/*
 *
 *  Procedure:
 *      paint_menu - draws the entire menu
 *
 */
static void paint_menu(
	MenuRoot *mr, XEvent *pevent, FvwmWindow *fw)
{
	MenuItem *mi;
	MenuStyle *ms = MR_STYLE(mr);
	int bw = MST_BORDER_WIDTH(mr);
	int relief_thickness = ST_RELIEF_THICKNESS(MR_STYLE(mr));

	if (fw && !check_if_fvwm_window_exists(fw))
	{
		fw = NULL;
	}
	if (MR_IS_PAINTED(mr) && pevent &&
	    (pevent->xexpose.x >= MR_WIDTH(mr) - bw ||
	     pevent->xexpose.x + pevent->xexpose.width <= bw ||
	     pevent->xexpose.y >= MR_HEIGHT(mr) - bw ||
	     pevent->xexpose.y + pevent->xexpose.height <= bw))
	{
		/* Only the border was obscured. Redraw it centrally instead of
		 * redrawing several menu items. */
		RelieveRectangle(
			dpy, MR_WINDOW(mr), 0, 0, MR_WIDTH(mr) - 1,
			MR_HEIGHT(mr) - 1, (Pdepth < 2) ?
			SHADOW_GC(MST_MENU_INACTIVE_GCS(mr)) :
			HILIGHT_GC(MST_MENU_INACTIVE_GCS(mr)),
			SHADOW_GC(MST_MENU_INACTIVE_GCS(mr)), bw);
		{
			return;
		}
	}
	MR_IS_PAINTED(mr) = 1;
	/* paint the menu background */
	if (ms && ST_HAS_MENU_CSET(ms))
	{
		if (MR_IS_BACKGROUND_SET(mr) == False)
		{
			SetWindowBackground(
				dpy, MR_WINDOW(mr), MR_WIDTH(mr),
				MR_HEIGHT(mr), &Colorset[ST_CSET_MENU(ms)],
				Pdepth, FORE_GC(ST_MENU_INACTIVE_GCS(ms)),
				True);
			MR_IS_BACKGROUND_SET(mr) = True;
		}
	}
	else if (ms)
	{
		int type;
		Bool do_clear = False;

		type = ST_FACE(ms).type;
		switch(type)
		{
		case SolidMenu:
			XSetWindowBackground(
				dpy, MR_WINDOW(mr), MST_FACE(mr).u.back);
			do_clear = True;
			break;
		case GradientMenu:
			do_clear = paint_menu_gradient_background(mr, pevent);
			break;
		case PixmapMenu:
			do_clear = paint_menu_pixmap_background(mr, pevent);
			break;
		case TiledPixmapMenu:
			XSetWindowBackgroundPixmap(
				dpy, MR_WINDOW(mr), ST_FACE(ms).u.p->picture);
			do_clear = True;
			break;
		} /* switch(type) */
		if (do_clear == True)
		{
			clear_expose_menu_area(MR_WINDOW(mr), pevent);
		}
	} /* if (ms) */
	/* draw the relief */
	RelieveRectangle(dpy, MR_WINDOW(mr), 0, 0, MR_WIDTH(mr) - 1,
			 MR_HEIGHT(mr) - 1, (Pdepth < 2) ?
			 SHADOW_GC(MST_MENU_INACTIVE_GCS(mr)) :
			 HILIGHT_GC(MST_MENU_INACTIVE_GCS(mr)),
			 SHADOW_GC(MST_MENU_INACTIVE_GCS(mr)), bw);
	/* paint the menu items */
	for (mi = MR_FIRST_ITEM(mr); mi != NULL; mi = MI_NEXT_ITEM(mi))
	{
		int do_draw = 0;

		/* be smart about handling the expose, redraw only the entries
		 * that we need to */
		if (pevent == NULL)
		{
			do_draw = True;
		}
		else
		{
			register int b_offset;

			b_offset = MI_Y_OFFSET(mi) + MI_HEIGHT(mi);
			if (MR_SELECTED_ITEM(mr) == mi)
			{
				b_offset += relief_thickness;
			}
			if (pevent->xexpose.y < b_offset &&
			    (pevent->xexpose.y + pevent->xexpose.height) >
			    MI_Y_OFFSET(mi))
			{
				do_draw = True;
			}
		}
		if (do_draw)
		{
			MenuPaintItemParameters mpip;

			get_menu_paint_item_parameters(
				&mpip, mr, NULL, fw, pevent, True);
			mpip.flags.is_first_item = (MR_FIRST_ITEM(mr) == mi);
			menuitem_paint(mi, &mpip);
		}
	}
	paint_side_pic(mr, pevent);
	XFlush(dpy);

	return;
}

/* Set the selected-ness state of the menuitem passed in */
static void select_menu_item(
	MenuRoot *mr, MenuItem *mi, Bool select, FvwmWindow *fw)
{
	if (select == True && MR_SELECTED_ITEM(mr) != NULL &&
	    MR_SELECTED_ITEM(mr) != mi)
	{
		select_menu_item(mr, MR_SELECTED_ITEM(mr), False, fw);
	}
	else if (select == False && MR_SELECTED_ITEM(mr) == NULL)
	{
		return;
	}
	else if (select == True && MR_SELECTED_ITEM(mr) == mi)
	{
		return;
	}
	if (!MI_IS_SELECTABLE(mi))
	{
		return;
	}

	if (select == False)
	{
		MI_WAS_DESELECTED(mi) = True;
	}

	if (!MST_HAS_MENU_CSET(mr))
	{
		switch (MST_FACE(mr).type)
		{
		case GradientMenu:
			if (select == True)
			{
				int iy;
				int ih;
				int mw;
				XEvent e;

				if (!MR_IS_PAINTED(mr))
				{
					FCheckWeedTypedWindowEvents(
						dpy, MR_WINDOW(mr), Expose,
						NULL);
					paint_menu(mr, NULL, fw);
				}
				iy = MI_Y_OFFSET(mi);
				ih = MI_HEIGHT(mi) +
					(MI_IS_SELECTABLE(mi) ?
					 MST_RELIEF_THICKNESS(mr) : 0);
				if (iy < 0)
				{
					ih += iy;
					iy = 0;
				}
				mw = MR_WIDTH(mr) - 2 * MST_BORDER_WIDTH(mr);
				if (iy + ih > MR_HEIGHT(mr))
					ih = MR_HEIGHT(mr) - iy;
				/* grab image */
				MR_STORED_ITEM(mr).stored =
					XCreatePixmap(
						dpy, Scr.NoFocusWin, mw, ih,
						Pdepth);
				XSetGraphicsExposures(
					dpy,
					FORE_GC(MST_MENU_INACTIVE_GCS(mr)),
					True);
				XSync(dpy, 0);
				while (FCheckTypedEvent(dpy, NoExpose, &e))
				{
					/* nothing to do here */
				}
				XCopyArea(
					dpy, MR_WINDOW(mr),
					MR_STORED_ITEM(mr).stored,
					FORE_GC(MST_MENU_INACTIVE_GCS(mr)),
					MST_BORDER_WIDTH(mr), iy, mw, ih, 0,
					0);
				XSync(dpy, 0);
				if (FCheckTypedEvent(dpy, NoExpose, &e))
				{
					MR_STORED_ITEM(mr).y = iy;
					MR_STORED_ITEM(mr).width = mw;
					MR_STORED_ITEM(mr).height = ih;
				}
				else
				{
					XFreePixmap(
						dpy,
						MR_STORED_ITEM(mr).stored);
					MR_STORED_ITEM(mr).stored = None;
					MR_STORED_ITEM(mr).width = 0;
					MR_STORED_ITEM(mr).height = 0;
					MR_STORED_ITEM(mr).y = 0;
				}
				XSetGraphicsExposures(
					dpy,
					FORE_GC(MST_MENU_INACTIVE_GCS(mr)),
					False);
			}
			else if (select == False &&
				 MR_STORED_ITEM(mr).width != 0)
			{
				/* ungrab image */
				XCopyArea(
					dpy, MR_STORED_ITEM(mr).stored,
					MR_WINDOW(mr),
					FORE_GC(MST_MENU_INACTIVE_GCS(mr)),
					0, 0, MR_STORED_ITEM(mr).width,
					MR_STORED_ITEM(mr).height,
					MST_BORDER_WIDTH(mr),
					MR_STORED_ITEM(mr).y);

				if (MR_STORED_ITEM(mr).stored != None)
				{
					XFreePixmap(
						dpy, MR_STORED_ITEM(mr).stored);
				}
				MR_STORED_ITEM(mr).stored = None;
				MR_STORED_ITEM(mr).width = 0;
				MR_STORED_ITEM(mr).height = 0;
				MR_STORED_ITEM(mr).y = 0;
			}
			else if (select == False)
			{
				XEvent e;
				FvwmPicture *sidePic = NULL;

				e.type = Expose;
				e.xexpose.x = MST_BORDER_WIDTH(mr);
				e.xexpose.y = MI_Y_OFFSET(mi);
				e.xexpose.width = MR_WIDTH(mr) - 2 *
					MST_BORDER_WIDTH(mr);
				e.xexpose.height = MI_HEIGHT(mi) +
					(MI_IS_SELECTABLE(mi) ?
					 MST_RELIEF_THICKNESS(mr) : 0);
				if (MR_SIDEPIC(mr))
				{
					sidePic = MR_SIDEPIC(mr);
				}
				else if (MST_SIDEPIC(mr))
				{
					sidePic = MST_SIDEPIC(mr);
				}
				if (sidePic)
				{
					e.xexpose.width -= sidePic->width;
					if (MR_SIDEPIC_X_OFFSET(mr) ==
					    MST_BORDER_WIDTH(mr))
					{
						e.xexpose.x += sidePic->width;
					}
				}
				MR_SELECTED_ITEM(mr) = (select) ? mi : NULL;
				paint_menu(mr, &e, fw);
			}
			break;
		default:
			if (MR_STORED_ITEM(mr).width != 0)
			{
				if (MR_STORED_ITEM(mr).stored != None)
				{
					XFreePixmap(
						dpy,
						MR_STORED_ITEM(mr).stored);
				}
				MR_STORED_ITEM(mr).stored = None;
				MR_STORED_ITEM(mr).width = 0;
				MR_STORED_ITEM(mr).height = 0;
				MR_STORED_ITEM(mr).y = 0;
			}
			break;
		} /* switch */
	} /* if */

	if (fw && !check_if_fvwm_window_exists(fw))
	{
		fw = NULL;
	}
	MR_SELECTED_ITEM(mr) = (select) ? mi : NULL;
	if (MR_IS_PAINTED(mr))
	{
		MenuPaintItemParameters mpip;

		get_menu_paint_item_parameters(&mpip, mr, mi, fw, NULL, False);
		menuitem_paint(mi, &mpip);
	}

	return;
}

/* ---------------------------- popping menu up or down -------------------- */

static int get_left_popup_x_position(MenuRoot *mr, MenuRoot *submenu, int x)
{
	if (MST_USE_LEFT_SUBMENUS(mr))
	{
		return (x - MST_POPUP_OFFSET_ADD(mr) - MR_WIDTH(submenu) +
			MR_WIDTH(mr) *
			(100 - MST_POPUP_OFFSET_PERCENT(mr)) / 100);
	}
	else
	{
		return (x - MR_WIDTH(submenu) + MST_BORDER_WIDTH(mr));
	}
}

static int get_right_popup_x_position(MenuRoot *mr, MenuRoot *submenu, int x)
{
	if (MST_USE_LEFT_SUBMENUS(mr))
	{
		return (x + MR_WIDTH(mr) - MST_BORDER_WIDTH(mr));
	}
	else
	{
		return (x + MR_WIDTH(mr) * MST_POPUP_OFFSET_PERCENT(mr) / 100 +
			MST_POPUP_OFFSET_ADD(mr));
	}
}

static void get_prefered_popup_position(
	MenuRoot *mr, MenuRoot *submenu, int *px, int *py,
	Bool *pprefer_left_submenus)
{
	int menu_x, menu_y;
	MenuItem *mi = NULL;

	if (!menu_get_geometry(
		    mr, &JunkRoot, &menu_x, &menu_y, &JunkWidth, &JunkHeight,
		    &JunkBW, &JunkDepth))
	{
		*px = 0;
		*py = 0;
		*pprefer_left_submenus = False;
		fvwm_msg(ERR, "get_prefered_popup_position",
			 "can't get geometry of menu %s", MR_NAME(mr));
		return;
	}
	/* set the direction flag */
	*pprefer_left_submenus =
		(MR_HAS_POPPED_UP_LEFT(mr) ||
		 (MST_USE_LEFT_SUBMENUS(mr) &&
		  !MR_HAS_POPPED_UP_RIGHT(mr)));
	/* get the x position */
	if (*pprefer_left_submenus)
	{
		*px = get_left_popup_x_position(mr, submenu, menu_x);
	}
	else
	{
		*px = get_right_popup_x_position(mr, submenu, menu_x);
	}
	/* get the y position */
	if (MR_SELECTED_ITEM(mr))
	{
		mi = MR_SELECTED_ITEM(mr);
	}
	if (mi)
	{
		*py = menu_y + MI_Y_OFFSET(mi) -
			MST_BORDER_WIDTH(mr) + MST_RELIEF_THICKNESS(mr);
	}
	else
	{
		*py = menu_y;
	}
}

static int do_menus_overlap(
	MenuRoot *mr, int x, int y, int width, int height, int h_tolerance,
	int v_tolerance, int s_tolerance, Bool allow_popup_offset_tolerance)
{
	int prior_x, prior_y, x_overlap;
	int prior_width, prior_height;

	if (mr == NULL)
	{
		return 0;
	}
	if (!menu_get_geometry(
		    mr, &JunkRoot,&prior_x,&prior_y, &prior_width,
		    &prior_height, &JunkBW, &JunkDepth))
	{
		return 0;
	}
	x_overlap = 0;
	if (allow_popup_offset_tolerance)
	{
		if (MST_POPUP_OFFSET_ADD(mr) < 0)
		{
			s_tolerance = -MST_POPUP_OFFSET_ADD(mr);
		}
		if (MST_USE_LEFT_SUBMENUS(mr))
		{
			prior_x += (100 - MST_POPUP_OFFSET_PERCENT(mr)) / 100;
		}
		else
		{
			prior_width *=
				(float)(MST_POPUP_OFFSET_PERCENT(mr)) / 100.0;
		}
	}
	if (MST_USE_LEFT_SUBMENUS(mr))
	{
		int t = s_tolerance;
		s_tolerance = h_tolerance;
		h_tolerance = t;
	}
	if (y < prior_y + prior_height - v_tolerance &&
	    prior_y < y + height - v_tolerance &&
	    x < prior_x + prior_width - s_tolerance &&
	    prior_x < x + width - h_tolerance)
	{
		x_overlap = x - prior_x;
		if (x <= prior_x)
		{
			x_overlap--;
		}
	}

	return x_overlap;
}

/*
 *
 *  Procedure:
 *      pop_menu_up - pop up a pull down menu
 *
 *  Inputs:
 *      x, y      - location of upper left of menu
 *      do_warp_to_item - warp pointer to the first item after title
 *      pops      - pointer to the menu options for new menu
 *
 */
static int pop_menu_up(
	MenuRoot **pmenu, MenuParameters *pmp, MenuRoot *parent_menu,
	MenuItem *parent_item, const exec_context_t **pexc, int x, int y,
	Bool prefer_left_submenu, Bool do_warp_to_item, MenuOptions *pops,
	Bool *ret_overlap, Window popdown_window)
{
	Bool do_warp_to_title = False;
	int x_overlap = 0;
	int x_clipped_overlap;
	MenuRoot *mrMi;
	MenuRoot *mr;
	FvwmWindow *fw;
	int context;
	int bw = 0;
	int bwp = 0;
	int prev_x;
	int prev_y;
	int prev_width;
	int prev_height;
	unsigned int event_mask;
	int scr_x, scr_y;
	int scr_w, scr_h;

	mr = *pmenu;
	if (!mr ||
	    (MR_MAPPED_COPIES(mr) > 0 && MR_COPIES(mr) >= MAX_MENU_COPIES))
	{
		return False;
	}

	/*
	 * handle dynamic menu actions
	 */

	/* First of all, execute the popup action (if defined). */
	if (MR_POPUP_ACTION(mr))
	{
		char *menu_name;
		saved_pos_hints pos_hints;
		Bool is_busy_grabbed = False;
		int mapped_copies = MR_MAPPED_COPIES(mr);

		/* save variables that we still need but that may be
		 * overwritten */
		menu_name = xstrdup(MR_NAME(mr));
		pos_hints = last_saved_pos_hints;
		if (Scr.BusyCursor & BUSY_DYNAMICMENU)
		{
			is_busy_grabbed = GrabEm(CRS_WAIT, GRAB_BUSYMENU);
		}
		/* Execute the action */
		__menu_execute_function(pmp->pexc, MR_POPUP_ACTION(mr));
		if (is_busy_grabbed)
		{
			UngrabEm(GRAB_BUSYMENU);
		}
		/* restore the stuff we saved */
		last_saved_pos_hints = pos_hints;
		if (mapped_copies == 0)
		{
			/* Now let's see if the menu still exists. It may have
			 * been destroyed and recreated, so we have to look for
			 * a menu with the saved name. menus_find_menu() always
			 * returns the original menu, not one of its copies, so
			 * below logic would fail miserably if used with a menu
			 * copy. On the other hand, menu copies can't be
			 * deleted within a dynamic popup action, so just
			 * ignore this case. */
			*pmenu = menus_find_menu(menu_name);
			if (menu_name)
			{
				free(menu_name);
			}
			mr = *pmenu;
		}
	}
	if (mr)
	{
		update_menu(mr, pmp);
	}
	if (mr == NULL || MR_FIRST_ITEM(mr) == NULL || MR_ITEMS(mr) == 0)
	{
		/* The menu deleted itself or all its items or it has been
		 * empty from the start. */
		return False;
	}
	fw = (*pexc)->w.fw;
	if (fw && !check_if_fvwm_window_exists(fw))
	{
		fw = NULL;
	}
	context = (*pexc)->w.wcontext;

	/*
	 * Create a new menu instance (if necessary)
	 */

	if (MR_MAPPED_COPIES(mr) > 0)
	{
		/* create a new instance of the menu */
		*pmenu = copy_menu_root(*pmenu);
		if (!*pmenu)
		{
			return False;
		}
		make_menu_window(*pmenu, False);
		mr = *pmenu;
	}

	/*
	 * Evaluate position hints
	 */

	/* calculate position from position hints if available */
	if (pops->flags.has_poshints &&
	    !last_saved_pos_hints.flags.do_ignore_pos_hints)
	{
		get_xy_from_position_hints(
			&(pops->pos_hints), MR_WIDTH(mr), MR_HEIGHT(mr),
			(parent_menu) ? MR_WIDTH(parent_menu) : 0,
			prefer_left_submenu, &x, &y);
	}

	/*
	 * Initialise new menu
	 */

	MR_PARENT_MENU(mr) = parent_menu;
	MR_PARENT_ITEM(mr) = parent_item;
	MR_IS_PAINTED(mr) = 0;
	MR_IS_LEFT(mr) = 0;
	MR_IS_RIGHT(mr) = 0;
	MR_IS_UP(mr) = 0;
	MR_IS_DOWN(mr) = 0;
	MR_XANIMATION(mr) = 0;
	InstallFvwmColormap();

	/*
	 * Handle popups from button clicks on buttons in the title bar,
	 * or the title bar itself. Position hints override this.
	 */

	if (!pops->flags.has_poshints && fw && parent_menu == NULL &&
	    pmp->flags.is_first_root_menu)
	{
		int cx;
		int cy;
		int gx;
		int gy;
		Bool has_context;
		rectangle button_g;

		has_context = get_title_button_geometry(
			fw, &button_g, context);
		if (has_context)
		{
			fscreen_scr_arg fscr;

			get_title_gravity_factors(fw, &gx, &gy);
			cx = button_g.x + button_g.width / 2;
			cy = button_g.y + button_g.height / 2;
			if (gx != 0)
			{
				x = button_g.x +
					(gx == 1) * button_g.width -
					(gx == -1) * MR_WIDTH(mr);
			}
			if (gy != 0)
			{
				y = button_g.y +
					(gy == 1) * button_g.height -
					(gy == -1) * MR_HEIGHT(mr);
			}
			if (gx == 0 && x < button_g.x)
			{
				x = button_g.x;
			}
			else if (gx == 0 && x + MR_WIDTH(mr) >=
				 button_g.x + button_g.width)
			{
				x = button_g.x + button_g.width - MR_WIDTH(mr);
			}
			if (gy == 0 && y < button_g.y)
			{
				y = button_g.y;
			}
			else if (gy == 0 && y + MR_HEIGHT(mr) >=
				 button_g.y + button_g.height)
			{
				y = button_g.y + button_g.height -
					MR_HEIGHT(mr);
			}
			pops->pos_hints.has_screen_origin = True;
			fscr.xypos.x = cx;
			fscr.xypos.y = cy;
			if (FScreenGetScrRect(
				    &fscr, FSCREEN_XYPOS, &JunkX, &JunkY,
				    &JunkWidth, &JunkHeight))
			{
				/* use current cx/cy */
			}
			else if (FQueryPointer(
					 dpy, Scr.Root, &JunkRoot, &JunkChild,
					 &cx, &cy, &JunkX, &JunkY, &JunkMask))
			{
				/* use pointer's position */
			}
			else
			{
				cx = -1;
				cy = -1;
			}
			pops->pos_hints.screen_origin_x = cx;
			pops->pos_hints.screen_origin_y = cy;
		}
	} /* if (pops->flags.has_poshints) */

	/*
	 * Clip to screen
	 */

	{
		fscreen_scr_arg fscr;

		fscr.xypos.x = pops->pos_hints.screen_origin_x;
		fscr.xypos.y = pops->pos_hints.screen_origin_y;
		/* clip to screen */
		FScreenClipToScreen(
			&fscr, FSCREEN_XYPOS, &x, &y, MR_WIDTH(mr),
			MR_HEIGHT(mr));
		/* "this" screen is defined -- so get its coords for future
		 * reference */
		FScreenGetScrRect(
			&fscr, FSCREEN_XYPOS, &scr_x, &scr_y, &scr_w, &scr_h);
	}

	if (parent_menu)
	{
		bw = MST_BORDER_WIDTH(mr);
		bwp = MST_BORDER_WIDTH(parent_menu);
		x_overlap = do_menus_overlap(
			parent_menu, x, y, MR_WIDTH(mr), MR_HEIGHT(mr), bwp,
			bwp, bwp, True);
	}

	/*
	 * Calculate position and animate menus
	 */

	if (parent_menu == NULL ||
	    !menu_get_geometry(
		    parent_menu, &JunkRoot, &prev_x, &prev_y, &prev_width,
		    &prev_height, &JunkBW, &JunkDepth))
	{
		MR_HAS_POPPED_UP_LEFT(mr) = 0;
		MR_HAS_POPPED_UP_RIGHT(mr) = 0;
	}
	else
	{
		int left_x;
		int right_x;
		Bool use_left_submenus = MST_USE_LEFT_SUBMENUS(mr);
		MenuRepaintTransparentParameters mrtp;
		Bool transparent_bg = False;

		/* check if menus overlap */
		x_clipped_overlap =
			do_menus_overlap(
				parent_menu, x, y, MR_WIDTH(mr), MR_HEIGHT(mr),
				bwp, bwp, bwp, True);
		if (x_clipped_overlap &&
		    (!pops->flags.has_poshints ||
		     pops->pos_hints.is_relative == False ||
		     x_overlap == 0))
		{
			/* menus do overlap, but do not reposition if overlap
			 * was caused by relative positioning hints */
			left_x = get_left_popup_x_position(
				parent_menu, mr, prev_x);
			right_x = get_right_popup_x_position(
				parent_menu, mr, prev_x);
			if (use_left_submenus)
			{
				if (left_x + MR_WIDTH(mr) < prev_x + bwp)
				{
					left_x = prev_x + bwp - MR_WIDTH(mr);
				}
			}
			else
			{
				if (right_x > prev_x + prev_width - bwp)
				{
					right_x = prev_x + prev_width - bwp;
				}
			}

			/*
			 * Animate parent menu
			 */

			if (MST_IS_ANIMATED(mr))
			{
				/* animate previous out of the way */
				int a_left_x;
				int a_right_x;
				int end_x;
				Window w;

				if (use_left_submenus)
				{
					if (prev_x - left_x < MR_WIDTH(mr))
					{
						a_right_x =
							x + (prev_x - left_x);
					}
					else
					{
						a_right_x =
							x + MR_WIDTH(mr) - bw;
					}
					a_left_x = x - MR_WIDTH(parent_menu);
				}
				else
				{
					if (right_x - prev_x < prev_width)
					{
						a_left_x =
							x + (prev_x - right_x);
					}
					else
					{
						a_left_x =
							x - prev_width + bw;
					}
					a_right_x = x + MR_WIDTH(mr);
				}
				if (prefer_left_submenu)
				{
					/* popup menu is left of old menu, try
					 * to move prior menu right */
					if (a_right_x + prev_width <=
					    scr_x + scr_w)
					{
						end_x = a_right_x;
					}
					else if (a_left_x >= scr_x)
					{
						end_x = a_left_x;
					}
					else
					{
						end_x = scr_x + scr_w -
							prev_width;
					}
				}
				else
				{
					/* popup menu is right of old menu, try
					 * to move prior menu left */
					if (a_left_x >= scr_x)
					{
						end_x = a_left_x;
					}
					else if (a_right_x + prev_width <=
						 scr_x + scr_w)
					{
						end_x = a_right_x;
					}
					else
					{
						end_x = scr_x;
					}
				}
				if (end_x == a_left_x || end_x == 0)
				{
					MR_HAS_POPPED_UP_LEFT(mr) = 0;
					MR_HAS_POPPED_UP_RIGHT(mr) = 1;
				}
				else
				{
					MR_HAS_POPPED_UP_LEFT(mr) = 1;
					MR_HAS_POPPED_UP_RIGHT(mr) = 0;
				}
				MR_XANIMATION(parent_menu) += end_x - prev_x;
				if (ST_HAS_MENU_CSET(MR_STYLE(parent_menu)) &&
				    CSET_IS_TRANSPARENT(
					    ST_CSET_MENU(
						    MR_STYLE(parent_menu))))
				{
					transparent_bg = True;
					get_menu_repaint_transparent_parameters(
						&mrtp, parent_menu, fw);
				}

				if (MR_IS_TEAR_OFF_MENU(parent_menu))
				{
					int cx;
					int cy;

					w = FW_W_FRAME(
						pmp->tear_off_root_menu_window
						);
					if (XGetGeometry(
						    dpy, w, &JunkRoot, &cx,
						    &cy,
						    (unsigned int*)&JunkWidth,
						    (unsigned int*)&JunkHeight,
						    (unsigned int*)&JunkBW,
						    (unsigned int*)&JunkDepth))
					{
						end_x += (cx - prev_x );
						prev_x = cx;
						prev_y = cy;
					}
				}
				else
				{
					w = MR_WINDOW(parent_menu);
				}
				AnimatedMoveOfWindow(
					w, prev_x, prev_y, end_x, prev_y, True,
					-1, NULL,
					(transparent_bg)? &mrtp:NULL);
			} /* if (MST_IS_ANIMATED(mr)) */

			/*
			 * Try the other side of the parent menu
			 */

			else if (!pops->flags.is_fixed)
			{
				Bool may_use_left;
				Bool may_use_right;
				Bool do_use_left;
				Bool use_left_as_last_resort;

				use_left_as_last_resort =
					(left_x > scr_x + scr_w - right_x -
					 MR_WIDTH(mr));
				may_use_left = (left_x >= scr_x);
				may_use_right = (right_x + MR_WIDTH(mr) <=
						 scr_x + scr_w);
				if (!may_use_left && !may_use_right)
				{
					/* If everything goes wrong, put the
					 * submenu on the side with more free
					 * space. */
					do_use_left = use_left_as_last_resort;
				}
				else if (may_use_left &&
					 (prefer_left_submenu ||
					  !may_use_right))
				{
					do_use_left = True;
				}
				else
				{
					do_use_left = False;
				}
				x = (do_use_left) ? left_x : right_x;
				MR_HAS_POPPED_UP_LEFT(mr) = do_use_left;
				MR_HAS_POPPED_UP_RIGHT(mr) = !do_use_left;

				/* Force the menu onto the screen, but leave at
				 * least PARENT_MENU_FORCE_VISIBLE_WIDTH pixels
				 * of the parent menu visible */
				if (x + MR_WIDTH(mr) > scr_x + scr_w)
				{
					int d = x + MR_WIDTH(mr) -
						(scr_x + scr_w);
					int c;

					if (prev_width >=
					    PARENT_MENU_FORCE_VISIBLE_WIDTH)
					{
						c = PARENT_MENU_FORCE_VISIBLE_WIDTH +
							prev_x;
					}
					else
					{
						c = prev_x + prev_width;
					}

					if (x - c >= d || x <= prev_x)
					{
						x -= d;
					}
					else if (x > c)
					{
						x = c;
					}
				}
				if (x < scr_x)
				{
					int c = prev_width -
						PARENT_MENU_FORCE_VISIBLE_WIDTH;

					if (c < 0)
					{
						c = 0;
					}
					if (scr_x - x > c)
					{
						x += c;
					}
					else
					{
						x = scr_x;
					}
				}
			} /* else if (non-overlapping menu style) */
		} /* if (x_clipped_overlap && ...) */
		else
		{
			MR_HAS_POPPED_UP_LEFT(mr) = prefer_left_submenu;
			MR_HAS_POPPED_UP_RIGHT(mr) = !prefer_left_submenu;
		}

		if (x < prev_x)
		{
			MR_IS_LEFT(mr) = 1;
		}
		if (x + MR_WIDTH(mr) > prev_x + prev_width)
		{
			MR_IS_RIGHT(mr) = 1;
		}
		if (y < prev_y)
		{
			MR_IS_UP(mr) = 1;
		}
		if (y + MR_HEIGHT(mr) > prev_y + prev_height)
		{
			MR_IS_DOWN(mr) = 1;
		}
		if (!MR_IS_LEFT(mr) && !MR_IS_RIGHT(mr))
		{
			MR_IS_LEFT(mr) = 1;
			MR_IS_RIGHT(mr) = 1;
		}
	} /* if (parent_menu) */

	/*
	 * Make sure we have the correct events selected
	 */

	if (pmp->tear_off_root_menu_window == NULL)
	{
		/* normal menus and sub menus */
		event_mask = XEVMASK_MENUW;
	}
	else if (parent_menu == NULL)
	{
		/* tear off menu needs more events */
		event_mask = XEVMASK_TEAR_OFF_MENUW;
	}
	else
	{
		/* sub menus of tear off menus need LeaveNotify */
		event_mask = XEVMASK_TEAR_OFF_SUBMENUW;
	}

	/*
	 * Pop up the menu
	 */

	XMoveWindow(dpy, MR_WINDOW(mr), x, y);
	XSelectInput(dpy, MR_WINDOW(mr), event_mask);
	XMapRaised(dpy, MR_WINDOW(mr));
	if (popdown_window)
		XUnmapWindow(dpy, popdown_window);
	XFlush(dpy);
	MR_MAPPED_COPIES(mr)++;
	MST_USAGE_COUNT(mr)++;
	if (ret_overlap)
	{
		*ret_overlap =
			do_menus_overlap(
				parent_menu, x, y, MR_WIDTH(mr), MR_HEIGHT(mr),
				0, 0, 0, False) ? True : False;
	}

	/*
	 * Warp the pointer
	 */

	if (!do_warp_to_item && parent_menu != NULL)
	{
		MenuItem *mi;

		mi = find_entry(pmp, NULL, &mrMi, None, -1, -1);
		if (mi && mrMi == mr && mi != MR_FIRST_ITEM(mrMi))
		{
			/* pointer is on an item of the popup */
			if (MST_DO_WARP_TO_TITLE(mr))
			{
				/* warp pointer if not on a root menu */
				do_warp_to_title = True;
			}
		}
	} /* if (!do_warp_to_item) */

	if (pops->flags.do_not_warp)
	{
		do_warp_to_title = False;
	}
	else if (pops->flags.do_warp_title)
	{
		do_warp_to_title = True;
	}
	if (pops->flags.has_poshints &&
	    !last_saved_pos_hints.flags.do_ignore_pos_hints &&
	    pops->flags.do_warp_title)
	{
		do_warp_to_title = True;
	}
	if (do_warp_to_item)
	{
		/* also warp */
		MR_SELECTED_ITEM(mr) = NULL;
		warp_pointer_to_item(
			mr, MR_FIRST_ITEM(mr), True /* skip Title */);
		select_menu_item(mr, MR_SELECTED_ITEM(mr), True, fw);
	}
	else if (do_warp_to_title)
	{
		/* Warp pointer to middle of top line, since we don't
		 * want the user to come up directly on an option */
		warp_pointer_to_title(mr);
	}

	return True;
}

/*
 *
 *  Procedure:
 *      pop_menu_down - unhighlight the current menu selection and
 *                    take down the menus
 *
 *      mr     - menu to pop down; this pointer is invalid after the function
 *               returns. Don't use it anymore!
 *      parent - the menu that has spawned mr (may be NULL). this is
 *               used to see if mr was spawned by itself on some level.
 *               this is a hack to allow specifying 'Popup foo' within
 *               menu foo. You must use the MenuRoot that is currently
 *               being processed here. DO NOT USE MR_PARENT_MENU(mr) here!
 *
 */
static void pop_menu_down(MenuRoot **pmr, MenuParameters *pmp)
{
	MenuItem *mi;

	assert(*pmr);
	memset(&(MR_DYNAMIC_FLAGS(*pmr)), 0, sizeof(MR_DYNAMIC_FLAGS(*pmr)));
	XUnmapWindow(dpy, MR_WINDOW(*pmr));
	MR_MAPPED_COPIES(*pmr)--;
	MST_USAGE_COUNT(*pmr)--;
	UninstallFvwmColormap();
	XFlush(dpy);
	if ((mi = MR_SELECTED_ITEM(*pmr)) != NULL)
	{
		select_menu_item(*pmr, mi, False, (*pmp->pexc)->w.fw);
	}
	if (MR_STORED_PIXELS(*pmr).d_pixels != NULL)
	{
		PictureFreeColors(
			dpy, Pcmap, MR_STORED_PIXELS(*pmr).d_pixels,
			MR_STORED_PIXELS(*pmr).d_npixels, 0, False);
		free(MR_STORED_PIXELS(*pmr).d_pixels);
		MR_STORED_PIXELS(*pmr).d_pixels = NULL;
	}
	if (MR_COPIES(*pmr) > 1)
	{
		/* delete this instance of the menu */
		DestroyMenu(*pmr, False, False);
	}
	else if (MR_POPDOWN_ACTION(*pmr))
	{
		/* Finally execute the popdown action (if defined). */
		saved_pos_hints pos_hints;

		/* save variables that we still need but that may be
		 * overwritten */
		pos_hints = last_saved_pos_hints;
		/* Execute the action */
		__menu_execute_function(pmp->pexc, MR_POPDOWN_ACTION(*pmr));
		/* restore the stuff we saved */
		last_saved_pos_hints = pos_hints;
	}

	return;
}

/*
 *
 *  Procedure:
 *      pop_menu_down_and_repaint_parent - Pops down a menu and repaints the
 *      overlapped portions of the parent menu. This is done only if
 *      *fSubmenuOverlaps is True. *fSubmenuOverlaps is set to False
 *      afterwards.
 *
 */
static void pop_menu_down_and_repaint_parent(
	MenuRoot **pmr, Bool *fSubmenuOverlaps, MenuParameters *pmp)
{
	MenuRoot *parent = MR_PARENT_MENU(*pmr);
	XEvent event;
	int mr_x;
	int mr_y;
	int mr_width;
	int mr_height;
	int parent_x;
	int parent_y;
	int parent_width;
	int parent_height;

	if (*fSubmenuOverlaps && parent)
	{
		/* popping down the menu may destroy the menu via the dynamic
		 * popdown action! Thus we must not access *pmr afterwards. */
		/* Create a fake event to pass into paint_menu */
		event.type = Expose;
		if (!menu_get_geometry(
			    *pmr, &JunkRoot, &mr_x, &mr_y, &mr_width,
			    &mr_height, &JunkBW, &JunkDepth) ||
		    !menu_get_geometry(
			    parent, &JunkRoot, &parent_x, &parent_y,
			    &parent_width, &parent_height, &JunkBW,
			    &JunkDepth))
		{
			pop_menu_down(pmr, pmp);
			paint_menu(parent, NULL, (*pmp->pexc)->w.fw);
		}
		else
		{
			pop_menu_down(pmr, pmp);
			event.xexpose.x = mr_x - parent_x;
			event.xexpose.width = mr_width;
			if (event.xexpose.x < 0)
			{
				event.xexpose.width += event.xexpose.x;
				event.xexpose.x = 0;
			}
			if (event.xexpose.x + event.xexpose.width >
			    parent_width)
			{
				event.xexpose.width =
					parent_width - event.xexpose.x;
			}
			event.xexpose.y = mr_y - parent_y;
			event.xexpose.height = mr_height;
			if (event.xexpose.y < 0)
			{
				event.xexpose.height += event.xexpose.y;
				event.xexpose.y = 0;
			}
			if (event.xexpose.y + event.xexpose.height >
			    parent_height)
			{
				event.xexpose.height =
					parent_height - event.xexpose.y;
			}
			flush_accumulate_expose(MR_WINDOW(parent), &event);
			paint_menu(parent, &event, (*pmp->pexc)->w.fw);
		}
	}
	else
	{
		/* popping down the menu may destroy the menu via the dynamic
		 * popdown action! Thus we must not access *pmr afterwards. */
		pop_menu_down(pmr, pmp);
	}
	*fSubmenuOverlaps = False;

	return;
}

/* ---------------------------- menu main loop ------------------------------ */

static void __mloop_init(
	MenuParameters *pmp, MenuReturn *pmret,
	mloop_evh_input_t *in, mloop_evh_data_t *med, mloop_static_info_t *msi,
	MenuOptions *pops)
{
	memset(in, 0, sizeof(*in));
	in->mif.do_force_reposition = 1;
	memset(med, 0, sizeof(*med));
	msi->t0 = fev_get_evtime();
	pmret->rc = MENU_NOP;
	memset(pops, 0, sizeof(*pops));
	/* remember where the pointer was so we can tell if it has moved */
	if (FQueryPointer(
		    dpy, Scr.Root, &JunkRoot, &JunkChild, &(msi->x_init),
		    &(msi->y_init), &JunkX, &JunkY, &JunkMask) == False)
	{
		/* pointer is on a different screen */
		msi->x_init = 0;
		msi->y_init = 0;
	}
	/* get the event mask right */
	msi->event_mask = (pmp->tear_off_root_menu_window == NULL) ?
		XEVMASK_MENU : XEVMASK_TEAR_OFF_MENU;

	return;
}

static void __mloop_get_event_timeout_loop(
	MenuParameters *pmp,
	mloop_evh_input_t *in, mloop_evh_data_t *med, mloop_static_info_t *msi)
{
	XEvent e = *(*pmp->pexc)->x.elast;

	while (!FPending(dpy) || !FCheckMaskEvent(dpy, msi->event_mask, &e))
	{
		Bool is_popup_timed_out =
			(MST_POPUP_DELAY(pmp->menu) > 0 &&
			 med->popup_delay_10ms++ >=
			 MST_POPUP_DELAY(pmp->menu) + 1);
		Bool is_popdown_timed_out =
			(MST_POPDOWN_DELAY(pmp->menu) > 0 && in->mrPopdown &&
			 med->popdown_delay_10ms++ >=
			 MST_POPDOWN_DELAY(pmp->menu) + 1);
		Bool do_fake_motion = False;

		if (is_popup_timed_out)
		{
			med->popup_delay_10ms = MST_POPUP_DELAY(pmp->menu);
		}
		if (is_popdown_timed_out)
		{
			med->popdown_delay_10ms = MST_POPDOWN_DELAY(pmp->menu);
		}
		if (
			in->mif.do_force_popup ||
			(is_popup_timed_out &&
			 (is_popdown_timed_out ||
			  in->mif.is_item_entered_by_key_press ||
			  !in->mrPopdown ||
			  MST_DO_POPDOWN_IMMEDIATELY(pmp->menu))))
		{
			in->mif.do_popup_now = True;
			in->mif.do_force_popup = False;
			do_fake_motion = True;
			in->mif.is_popped_up_by_timeout = True;
			is_popdown_timed_out = True;
		}
		if ((in->mrPopdown || in->mrPopup) &&
		    (is_popdown_timed_out ||
		     MST_DO_POPDOWN_IMMEDIATELY(pmp->menu)))
		{
			MenuRoot *m = (in->mrPopdown) ?
				in->mrPopdown : in->mrPopup;

			if (!m || med->mi != MR_PARENT_ITEM(m))
			{
				in->mif.do_popdown_now = True;
				do_fake_motion = True;
				if (MST_DO_POPUP_IMMEDIATELY(pmp->menu))
				{
					in->mif.do_popup_now = True;
					in->mif.is_popped_up_by_timeout = True;
				}
				else if (
					!MST_DO_POPUP_IMMEDIATELY(pmp->menu) &&
					in->mif.is_pointer_in_active_item_area)
				{
					in->mif.do_popup_now = True;
					in->mif.is_popped_up_by_timeout = True;
				}
				else if (
					!MST_DO_POPUP_IMMEDIATELY(pmp->menu) &&
					!MST_DO_POPDOWN_IMMEDIATELY(pmp->menu)
					&& MST_POPUP_DELAY(pmp->menu) <=
					MST_POPDOWN_DELAY(pmp->menu) &&
					med->popup_delay_10ms ==
					med->popdown_delay_10ms)
				{
					in->mif.do_popup_now = True;
					in->mif.is_popped_up_by_timeout = True;
				}
			}
		}
		if (in->mif.do_popup_now && med->mi == in->miRemovedSubmenu &&
		    !in->mif.is_key_press)
		{
			/* prevent popping up the menu again with
			 * RemoveSubemenus */
			in->mif.do_popup_now = False;
			in->mif.do_force_popup = False;
			do_fake_motion = in->mif.do_popdown_now;
			in->mif.is_popped_up_by_timeout = False;
		}

		if (do_fake_motion)
		{
			/* fake a motion event, and set in->mif.do_popup_now */
			e.type = MotionNotify;
			e.xmotion.time = fev_get_evtime();
			fev_fake_event(&e);
			in->mif.is_motion_faked = True;
			break;
		}
		usleep(10000 /* 10 ms*/);
	}

	return;
}

static mloop_ret_code_t __mloop_get_event(
	MenuParameters *pmp, MenuReturn *pmret,
	mloop_evh_input_t *in, mloop_evh_data_t *med, mloop_static_info_t *msi)
{
	XEvent e = *(*pmp->pexc)->x.elast;

	in->mif.do_popup_and_warp = False;
	in->mif.do_popup_now = False;
	in->mif.do_popdown_now = False;
	in->mif.do_propagate_event_into_submenu = False;
	in->mif.is_key_press = False;
	in->mif.is_button_release = 0;
	if (pmp->event_propagate_to_submenu)
	{
		/* handle an event that was passed in from the parent menu */
		fev_fake_event(pmp->event_propagate_to_submenu);
		pmp->event_propagate_to_submenu = NULL;
	}
	else if (in->mif.do_recycle_event)
	{
		in->mif.is_popped_up_by_timeout = False;
		in->mif.do_recycle_event = 0;
		if (pmp->menu != pmret->target_menu)
		{
			/* the event is for a previous menu, just close this
			 * one */
			pmret->rc = MENU_PROPAGATE_EVENT;
			return MENU_MLOOP_RET_END;
		}
		if ((*pmp->pexc)->x.elast->type == KeyPress)
		{
			/* since the pointer has been warped since the key was
			 * pressed, fake a different key press position */
			if (FQueryPointer(
				    dpy, Scr.Root, &JunkRoot, &JunkChild,
				    &e.xkey.x_root, &e.xkey.y_root,
				    &JunkX, &JunkY, &e.xkey.state) == False)
			{
				/* pointer is on a different screen */
				e.xkey.x_root = 0;
				e.xkey.y_root = 0;
			}
			fev_fake_event(&e);
			med->mi = MR_SELECTED_ITEM(pmp->menu);
		}
	}
	else if (pmp->tear_off_root_menu_window != NULL &&
		 FCheckTypedWindowEvent(
			 dpy, FW_W_PARENT(pmp->tear_off_root_menu_window),
			 ClientMessage, &e))
	{
		/* Got a ClientMessage for the tear out menu */
	}
	else
	{
		if (in->mif.do_force_reposition)
		{
			e.type = MotionNotify;
			e.xmotion.time = fev_get_evtime();
			in->mif.is_motion_faked = True;
			in->mif.do_force_reposition = False;
			in->mif.is_popped_up_by_timeout = False;
			fev_fake_event(&e);
		}
		else if (!FCheckMaskEvent(dpy, ExposureMask, &e))
		{
			Bool is_popdown_timer_active = False;
			Bool is_popup_timer_active = False;

			if (MST_POPDOWN_DELAY(pmp->menu) > 0 &&
			    in->mi_with_popup != NULL &&
			    in->mi_with_popup != MR_SELECTED_ITEM(pmp->menu))
			{
				is_popdown_timer_active = True;
			}
			if (MST_POPUP_DELAY(pmp->menu) > 0 &&
			    !in->mif.is_popped_up_by_timeout &&
			    in->mi_wants_popup != NULL)
			{
				is_popup_timer_active = True;
			}
			/* handle exposure events first */
			if (in->mif.do_force_popup ||
			    in->mif.is_pointer_in_active_item_area ||
			    is_popdown_timer_active || is_popup_timer_active)
			{
				__mloop_get_event_timeout_loop(
					pmp, in, med, msi);
			}
			else
			{
				/* block until there is an event */
				FMaskEvent(dpy, msi->event_mask, &e);
				in->mif.is_popped_up_by_timeout = False;
			}
		}
		else
		{
			in->mif.is_popped_up_by_timeout = False;
		}
	}

	in->mif.is_pointer_in_active_item_area = False;
	if (e.type == MotionNotify)
	{
		/* discard any extra motion events before a release */
		while (FCheckMaskEvent(
			       dpy, ButtonMotionMask | ButtonReleaseMask,
			       &e) && (e.type != ButtonRelease))
		{
			/* nothing */
		}
	}

	return MENU_MLOOP_RET_NORMAL;
}

static mloop_ret_code_t __mloop_handle_event(
	MenuParameters *pmp, MenuReturn *pmret, double_keypress *pdkp,
	mloop_evh_input_t *in, mloop_evh_data_t *med, mloop_static_info_t *msi)
{
	MenuRoot *tmrMi;
	Bool rc;

	pmret->rc = MENU_NOP;
	switch ((*pmp->pexc)->x.elast->type)
	{
	case ButtonRelease:
		in->mif.is_button_release = 1;
		med->mi = find_entry(
			pmp, &med->x_offset, &med->mrMi,
			(*pmp->pexc)->x.elast->xbutton.subwindow,
			(*pmp->pexc)->x.elast->xbutton.x_root,
			(*pmp->pexc)->x.elast->xbutton.y_root);
		/* hold the menu up when the button is released
		 * for the first time if released OFF of the menu */
		if (pmp->flags.is_sticky && !in->mif.is_motion_first)
		{
			in->mif.is_release_first = True;
			pmp->flags.is_sticky = False;
			return MENU_MLOOP_RET_LOOP;
		}
		if (med->mrMi != NULL)
		{
			int menu_x;
			int menu_y;

			menu_shortcuts(
				med->mrMi, pmp, pmret, (*pmp->pexc)->x.elast,
				&med->mi, pdkp, &menu_x, &menu_y);
			if (pmret->rc == MENU_NEWITEM_MOVEMENU)
			{
				move_any_menu(med->mrMi, pmp, menu_x, menu_y);
				pmret->rc = MENU_NEWITEM;
			}
			else if (pmret->rc == MENU_NEWITEM_FIND)
			{
				med->mi = find_entry(
					pmp, NULL, NULL, None, -1, -1);
				pmret->rc = MENU_NEWITEM;
			}
		}
		else
		{
			pmret->rc = MENU_SELECTED;
		}
		switch (pmret->rc)
		{
		case MENU_NOP:
			return MENU_MLOOP_RET_LOOP;
		case MENU_POPDOWN:
		case MENU_ABORTED:
		case MENU_DOUBLE_CLICKED:
		case MENU_TEAR_OFF:
		case MENU_KILL_TEAR_OFF_MENU:
		case MENU_EXEC_CMD:
			return MENU_MLOOP_RET_END;
		case MENU_POPUP:
			/* Allow for MoveLeft/MoveRight action to work with
			 * Mouse */
			in->mif.do_popup_and_warp = True;
		case MENU_NEWITEM:
			/* unpost the menu if posted */
			pmret->flags.is_menu_posted = 0;
			return MENU_MLOOP_RET_NORMAL;
		case MENU_SELECTED:
			in->mif.was_item_unposted = 0;
			if (pmret->flags.is_menu_posted && med->mrMi != NULL)
			{
				if (pmret->flags.do_unpost_submenu)
				{
					pmret->flags.do_unpost_submenu = 0;
					pmret->flags.is_menu_posted = 0;
					/* just ignore the event */
					return MENU_MLOOP_RET_LOOP;
				}
				else if (med->mi && MI_IS_POPUP(med->mi)
					 && med->mrMi == pmp->menu)
				{
					/* post menu - done below */
				}
				else if (MR_PARENT_ITEM(pmp->menu) &&
					 med->mi == MR_PARENT_ITEM(pmp->menu))
				{
					/* propagate back to parent menu
					 * and unpost current menu */
					pmret->flags.do_unpost_submenu = 1;
					pmret->rc = MENU_PROPAGATE_EVENT;
					pmret->target_menu = med->mrMi;
					return MENU_MLOOP_RET_END;
				}
				else if (med->mrMi != pmp->menu &&
					 med->mrMi != in->mrPopup)
				{
					/* unpost and propagate back to
					 * ancestor */
					pmret->flags.is_menu_posted = 0;
					pmret->rc = MENU_PROPAGATE_EVENT;
					pmret->target_menu = med->mrMi;
					return MENU_MLOOP_RET_END;
				}
				else if (in->mrPopup && med->mrMi ==
					 in->mrPopup)
				{
					/* unpost and propagate into
					 * submenu */
					in->mif.was_item_unposted = 1;
					in->mif.do_propagate_event_into_submenu
						= True;
					break;
				}
				else
				{
					/* simply unpost the menu */
					pmret->flags.is_menu_posted = 0;
				}
			}
			if (is_double_click(
				    msi->t0, med->mi, pmp, pmret, pdkp,
				    in->mif.has_mouse_moved))
			{
				pmret->rc = MENU_DOUBLE_CLICKED;
				return MENU_MLOOP_RET_END;
			}
			if (med->mi == NULL)
			{
				pmret->rc = MENU_ABORTED;
			}
			else if (MI_IS_POPUP(med->mi))
			{
				switch (MST_DO_POPUP_AS(pmp->menu))
				{
				case MDP_POST_MENU:
					if (in->mif.was_item_unposted)
					{
						pmret->flags.is_menu_posted =
							0;
						pmret->rc = MENU_UNPOST;
						pmret->target_menu = NULL;
						in->mif.do_popup_now = False;
					}
					else
					{
						pmret->flags.is_menu_posted =
							1;
						pmret->rc = MENU_POST;
						pmret->target_menu = NULL;
						in->mif.do_popup_now = True;
						if ((in->mrPopup ||
						     in->mrPopdown)
						    && med->mi !=
						    MR_SELECTED_ITEM(
							    pmp->menu))
						{
							in->mif.do_popdown_now
								= True;
						}
					}
					return MENU_MLOOP_RET_NORMAL;
				case MDP_IGNORE:
					pmret->rc = MENU_NOP;
					return MENU_MLOOP_RET_NORMAL;
				case MDP_CLOSE:
					pmret->rc = MENU_ABORTED;
					break;
				case MDP_ROOT_MENU:
				default:
					break;
				}
			}
			break;
		default:
			break;
		}
		pdkp->timestamp = 0;

		return MENU_MLOOP_RET_END;

	case ButtonPress:
		/* if the first event is a button press allow the release to
		 * select something */
		pmp->flags.is_sticky = False;
		return MENU_MLOOP_RET_LOOP;

	case VisibilityNotify:
		return MENU_MLOOP_RET_LOOP;

	case KeyRelease:
		if ((*pmp->pexc)->x.elast->xkey.keycode !=
		    MST_SELECT_ON_RELEASE_KEY(pmp->menu))
		{
			return MENU_MLOOP_RET_LOOP;
		}
		/* fall through to KeyPress */

	case KeyPress:
		/* Handle a key press events to allow mouseless operation */
		in->mif.is_key_press = True;
		med->x_offset = menudim_middle_x_offset(&MR_DIM(pmp->menu));

		/* if there is a posted menu we may have to move back into a
		 * previous menu or possibly ignore the mouse position */
		if (pmret->flags.is_menu_posted)
		{
			MenuRoot *l_mrMi;
			int l_x_offset;
			XEvent e;

			pmret->flags.is_menu_posted = 0;
			(void)find_entry(
				pmp, &l_x_offset, &l_mrMi, None, -1, -1);
			if (l_mrMi != NULL)
			{
				if (pmp->menu != l_mrMi)
				{
					/* unpost the menu and propagate the
					 * event to the correct menu */
					pmret->rc = MENU_PROPAGATE_EVENT;
					pmret->target_menu = l_mrMi;
					return MENU_MLOOP_RET_END;
				}
			}
			med->mi = MR_SELECTED_ITEM(pmp->menu);
			e = *(*pmp->pexc)->x.elast;
			e.xkey.x_root = med->x_offset;
			e.xkey.y_root = menuitem_middle_y_offset(
				med->mi, MR_STYLE(pmp->menu));
			fev_fake_event(&e);
		}

		/* now handle the actual key press */
		{
			int menu_x;
			int menu_y;

			menu_shortcuts(
				pmp->menu, pmp, pmret, (*pmp->pexc)->x.elast,
				&med->mi, pdkp, &menu_x, &menu_y);
			if (pmret->rc == MENU_NEWITEM_MOVEMENU)
			{
				move_any_menu(pmp->menu, pmp, menu_x, menu_y);
				pmret->rc = MENU_NEWITEM;
			}
			else if (pmret->rc == MENU_NEWITEM_FIND)
			{
				med->mi = find_entry(
					pmp, NULL, NULL, None, -1, -1);
				pmret->rc = MENU_NEWITEM;
			}
		}
		if (pmret->rc != MENU_NOP)
		{
			/* using a key 'unposts' the posted menu */
			pmret->flags.is_menu_posted = 0;
		}
		switch (pmret->rc)
		{
		case MENU_SELECTED:
			if (med->mi && MI_IS_POPUP(med->mi))
			{
				switch (MST_DO_POPUP_AS(pmp->menu))
				{
				case MDP_POST_MENU:
					pmret->rc = MENU_POPUP;
					break;
				case MDP_IGNORE:
					pmret->rc = MENU_NOP;
					return MENU_MLOOP_RET_NORMAL;
				case MDP_CLOSE:
					pmret->rc = MENU_ABORTED;
					return MENU_MLOOP_RET_END;
				case MDP_ROOT_MENU:
				default:
					return MENU_MLOOP_RET_END;
				}
				break;
			}
			return MENU_MLOOP_RET_END;
		case MENU_POPDOWN:
		case MENU_ABORTED:
		case MENU_DOUBLE_CLICKED:
		case MENU_TEAR_OFF:
		case MENU_KILL_TEAR_OFF_MENU:
		case MENU_EXEC_CMD:
			return MENU_MLOOP_RET_END;
		case MENU_NEWITEM:
		case MENU_POPUP:
			if (med->mrMi == NULL)
			{
				/* Set the MenuRoot of the current item in case
				 * we have just warped to the menu from the
				 * void or unposted a popup menu. */
				med->mrMi = pmp->menu;
			}
			/*tmrMi = med->mrMi;*/
			break;
		default:
			break;
		}
		/* now warp to the new menu item, if any */
		if (pmret->rc == MENU_NEWITEM && med->mi)
		{
			warp_pointer_to_item(med->mrMi, med->mi, False);
		}
		if (pmret->rc == MENU_POPUP && med->mi && MI_IS_POPUP(med->mi))
		{
			in->mif.do_popup_and_warp = True;
			break;
		}
		break;

	case MotionNotify:
	{
		int p_rx = -1;
		int p_ry = -1;
		Window p_child = None;

		if (in->mif.has_mouse_moved == False)
		{
			if (FQueryPointer(
				    dpy, Scr.Root, &JunkRoot, &p_child, &p_rx,
				    &p_ry, &JunkX, &JunkY, &JunkMask) == False)
			{
				/* pointer is on a different screen */
				p_rx = 0;
				p_ry = 0;
			}
			if (p_rx - msi->x_init > Scr.MoveThreshold ||
			    msi->x_init - p_rx > Scr.MoveThreshold ||
			    p_ry - msi->y_init > Scr.MoveThreshold ||
			    msi->y_init - p_ry > Scr.MoveThreshold)
			{
				/* remember that this isn't just a click any
				 * more since the pointer moved */
				in->mif.has_mouse_moved = True;
			}
		}
		med->mi = find_entry(
			pmp, &med->x_offset, &med->mrMi, p_child, p_rx, p_ry);
		if (pmret->flags.is_menu_posted &&
		    med->mrMi != pmret->target_menu)
		{
			/* ignore mouse movement outside a posted menu */
			med->mrMi = NULL;
			med->mi = NULL;
		}
		if (!in->mif.is_release_first && !in->mif.is_motion_faked &&
		    in->mif.has_mouse_moved)
		{
			in->mif.is_motion_first = True;
		}
		in->mif.is_motion_faked = False;
		break;
	}

	case Expose:
		/* grab our expose events, let the rest go through */
		XFlush(dpy);
		rc = menu_expose((*pmp->pexc)->x.elast, (*pmp->pexc)->w.fw);
		/* we want to dispatch this too so that icons and maybe tear
		 * off get redrawn after being obscured by menus. */
		if (rc == False)
		{
			dispatch_event((*pmp->pexc)->x.elast);
		}
		return MENU_MLOOP_RET_LOOP;

	case ClientMessage:
		if ((*pmp->pexc)->x.elast->xclient.format == 32 &&
		    (*pmp->pexc)->x.elast->xclient.data.l[0] ==
		    _XA_WM_DELETE_WINDOW &&
		    pmp->tear_off_root_menu_window != NULL &&
		    (*pmp->pexc)->x.elast->xclient.window == FW_W_PARENT(
			    pmp->tear_off_root_menu_window))
		{
			/* handle deletion of tear out menus */
			pmret->rc = MENU_KILL_TEAR_OFF_MENU;
			return MENU_MLOOP_RET_END;
		}
		break;

	case EnterNotify:
		/* ignore EnterNotify events */
		break;

	case LeaveNotify:
		if (pmp->tear_off_root_menu_window != NULL &&
		    find_entry(pmp, NULL, &tmrMi, None, -1, -1) == NULL &&
		    tmrMi == NULL)
		{
			/* handle deletion of tear out menus */
			pmret->rc = MENU_ABORTED;
			return MENU_MLOOP_RET_END;
		}
		/* ignore it */
		return MENU_MLOOP_RET_LOOP;

	case UnmapNotify:
		/* should never happen, but does not hurt */
		if (pmp->tear_off_root_menu_window != NULL &&
		    (*pmp->pexc)->x.elast->xunmap.window ==
		    FW_W(pmp->tear_off_root_menu_window))
		{
			/* handle deletion of tear out menus */
			pmret->rc = MENU_KILL_TEAR_OFF_MENU;
			/* extra safety: pass event back to main event loop to
			 * make sure the window is destroyed */
			FPutBackEvent(dpy, (*pmp->pexc)->x.elast);
			return MENU_MLOOP_RET_END;
		}
		break;

	default:
		/* We must not dispatch events here.  There is no guarantee
		 * that dispatch_event doesn't destroy a window stored in the
		 * menu structures.  Anyway, no events should ever get here
		 * except to tear off menus and these must be handled
		 * individually. */
#if 0
		dispatch_event((*pmp->pexc)->x.elast);
#endif
		break;
	}

	return MENU_MLOOP_RET_NORMAL;
}

static void __mloop_select_item(
	MenuParameters *pmp, mloop_evh_input_t *in, mloop_evh_data_t *med,
	Bool does_submenu_overlap, Bool *pdoes_popdown_submenu_overlap)
{
	in->mif.is_item_entered_by_key_press = in->mif.is_key_press;
	med->popup_delay_10ms = 0;
	/* we're on the same menu, but a different item, so we need to unselect
	 * the old item */
	if (MR_SELECTED_ITEM(pmp->menu))
	{
		/* something else was already selected on this menu.  We have
		 * to pop down the menu before unselecting the item in case we
		 * are using gradient menus. The recalled image would paint
		 * over the submenu. */
		if (in->mrPopup && in->mrPopup != in->mrPopdown)
		{
			in->mrPopdown = in->mrPopup;
			med->popdown_delay_10ms = 0;
			*pdoes_popdown_submenu_overlap = does_submenu_overlap;
			in->mrPopup = NULL;
		}
		select_menu_item(
			pmp->menu, MR_SELECTED_ITEM(pmp->menu), False,
			(*pmp->pexc)->w.fw);
	}
	/* highlight the new item; sets MR_SELECTED_ITEM(pmp->menu) too */
	select_menu_item(pmp->menu, med->mi, True, (*pmp->pexc)->w.fw);

	return;
}

static void __mloop_wants_popup(
	MenuParameters *pmp, mloop_evh_input_t *in, mloop_evh_data_t *med,
	MenuRoot *mrMiPopup)
{
	Bool do_it_now = False;

	in->mi_wants_popup = med->mi;
	if (in->mif.do_popup_now)
	{
		do_it_now = True;
	}
	else if (MST_DO_POPUP_IMMEDIATELY(pmp->menu) &&
		 med->mi != in->miRemovedSubmenu)
	{
		if (in->mif.is_key_press ||
		    MST_DO_POPDOWN_IMMEDIATELY(pmp->menu) || !in->mrPopdown)
		{
			do_it_now = True;
		}
	}
	else if (pointer_in_active_item_area(med->x_offset, med->mrMi))
	{
		if (in->mif.is_key_press || med->mi == in->miRemovedSubmenu ||
		    MST_DO_POPDOWN_IMMEDIATELY(pmp->menu) || !in->mrPopdown)
		{
			do_it_now = True;
		}
		else
		{
			in->mif.is_pointer_in_active_item_area = 1;
		}
	}
	if (do_it_now)
	{
		in->miRemovedSubmenu = NULL;
		/* must create a new menu or popup */
		if (in->mrPopup == NULL || in->mrPopup != mrMiPopup)
		{
			if (in->mif.do_popup_now)
			{
				in->mif.do_popup = True;
			}
			else
			{
				/* pop up in next pass through loop */
				in->mif.do_force_popup = True;
			}
		}
		else if (in->mif.do_popup_and_warp)
		{
			warp_pointer_to_item(
				in->mrPopup, MR_FIRST_ITEM(in->mrPopup), True);
		}
	}

	return;
}

static mloop_ret_code_t __mloop_make_popup(
	MenuParameters *pmp, MenuReturn *pmret,
	mloop_evh_input_t *in, mloop_evh_data_t *med,
	MenuOptions *pops, Bool *pdoes_submenu_overlap)
{
	/* create a popup menu */
	if (!is_submenu_mapped(pmp->menu, med->mi))
	{
		/* We want to pop prepop menus so it doesn't *have* to be
		   unpopped; do_menu pops down any menus it pops up, but we
		   want to be able to popdown w/o actually removing the menu */
		int x;
		int y;
		Bool prefer_left_submenus;

		/* Make sure we are using the latest style and menu layout. */
		update_menu(in->mrPopup, pmp);

		get_prefered_popup_position(
			pmp->menu, in->mrPopup, &x, &y, &prefer_left_submenus);
		/* Note that we don't care if popping up the menu works. If it
		 * doesn't we'll catch it below. */
		pop_menu_up(
			&in->mrPopup, pmp, pmp->menu, med->mi, pmp->pexc, x, y,
			prefer_left_submenus, in->mif.do_popup_and_warp, pops,
			pdoes_submenu_overlap,
			(in->mrPopdown) ? MR_WINDOW(in->mrPopdown) : None);
		in->mi_with_popup = med->mi;
		in->mi_wants_popup = NULL;
		if (in->mrPopup == NULL)
		{
			/* the menu deleted itself when execution the dynamic
			 * popup * action */
			pmret->rc = MENU_ERROR;
			return MENU_MLOOP_RET_END;
		}
		MR_SUBMENU_ITEM(pmp->menu) = med->mi;
	}

	return MENU_MLOOP_RET_NORMAL;
}

static mloop_ret_code_t __mloop_get_mi_actions(
	MenuParameters *pmp, MenuReturn *pmret, double_keypress *pdkp,
	mloop_evh_input_t *in, mloop_evh_data_t *med, mloop_static_info_t *msi,
	MenuRoot *mrMiPopup, Bool *pdoes_submenu_overlap,
	Bool *pdoes_popdown_submenu_overlap)
{
	in->mif.do_popdown = False;
	in->mif.do_popup = False;
	in->mif.do_menu = False;
	in->mif.is_submenu_mapped = False;
	if (!in->mrPopup && in->mrPopdown &&
	    med->mi == MR_PARENT_ITEM(in->mrPopdown))
	{
		/* We're again on the item that we left before.
		 * Deschedule popping it down. */
		in->mrPopup = in->mrPopdown;
		in->mrPopdown = NULL;
	}
	if (med->mrMi == in->mrPopup)
	{
		/* must make current popup menu a real menu */
		in->mif.do_menu = True;
		in->mif.is_submenu_mapped = True;
	}
	else if (pmret->rc == MENU_POST)
	{
		/* must create a real menu and warp into it */
		if (in->mrPopup == NULL || in->mrPopup != mrMiPopup)
		{
			in->mif.do_popup = True;
		}
		in->mif.do_menu = True;
		in->mif.is_submenu_mapped = True;
	}
	else if (in->mif.do_popup_and_warp)
	{
		/* must create a real menu and warp into it */
		if (in->mrPopup == NULL || in->mrPopup != mrMiPopup)
		{
			in->mif.do_popup = True;
		}
		else
		{
			XRaiseWindow(dpy, MR_WINDOW(in->mrPopup));
			warp_pointer_to_item(
				in->mrPopup, MR_FIRST_ITEM(in->mrPopup), True);
			in->mif.do_menu = True;
			in->mif.is_submenu_mapped = True;
		}
	}
	else if (med->mi && MI_IS_POPUP(med->mi))
	{
		if ((*pmp->pexc)->x.elast->type == ButtonPress &&
		    is_double_click(
			    msi->t0, med->mi, pmp, pmret, pdkp,
			    in->mif.has_mouse_moved))
		{
			pmret->rc = MENU_DOUBLE_CLICKED;
			return MENU_MLOOP_RET_END;
		}
		else if (!in->mrPopup || in->mrPopup != mrMiPopup ||
			 in->mif.do_popup_and_warp)
		{
			__mloop_wants_popup(pmp, in, med, mrMiPopup);
		}
	}
	if (in->mif.do_popdown_now)
	{
		in->mif.do_popdown = True;
		in->mif.do_popdown_now = False;
	}
	else if (in->mif.do_popup)
	{
		if (in->mrPopup && in->mrPopup != mrMiPopup)
		{
			in->mif.do_popdown = True;
			in->mrPopdown = in->mrPopup;
			med->popdown_delay_10ms = 0;
			*pdoes_popdown_submenu_overlap =
				*pdoes_submenu_overlap;
		}
		else if (in->mrPopdown && in->mrPopdown != mrMiPopup)
		{
			in->mif.do_popdown = True;
		}
		else if (in->mrPopdown && in->mrPopdown == mrMiPopup)
		{
			in->mrPopup = in->mrPopdown;
			*pdoes_submenu_overlap =
				*pdoes_popdown_submenu_overlap;
			in->mrPopdown = NULL;
			in->mif.do_popup = False;
			in->mif.do_popdown = False;
		}
	}

	return MENU_MLOOP_RET_NORMAL;
}

static void __mloop_do_popdown(
	MenuParameters *pmp, mloop_evh_input_t *in,
	Bool *pdoes_popdown_submenu_overlap)
{
	if (in->mrPopdown)
	{
		pop_menu_down_and_repaint_parent(
			&in->mrPopdown, pdoes_popdown_submenu_overlap, pmp);
		in->mi_with_popup = NULL;
		MR_SUBMENU_ITEM(pmp->menu) = NULL;
		if (in->mrPopup == in->mrPopdown)
		{
			in->mrPopup = NULL;
		}
		in->mrPopdown = NULL;
	}
	in->mif.do_popdown = False;

	return;
}

static mloop_ret_code_t __mloop_do_popup(
	MenuParameters *pmp, MenuReturn *pmret,
	mloop_evh_input_t *in, mloop_evh_data_t *med,
	MenuOptions *pops, MenuRoot *mrMiPopup, Bool *pdoes_submenu_overlap,
	Bool *pdoes_popdown_submenu_overlap)
{
	if (!MR_IS_PAINTED(pmp->menu))
	{
		/* draw the parent menu if it is not already drawn */
		FCheckWeedTypedWindowEvents(
			dpy, MR_WINDOW(pmp->menu), Expose, NULL);
		paint_menu(pmp->menu, NULL, (*pmp->pexc)->w.fw);
	}
	/* get pos hints for item's action */
	get_popup_options(pmp, med->mi, pops);
	if (med->mrMi == pmp->menu && mrMiPopup == NULL &&
	    MI_IS_POPUP(med->mi) && MR_MISSING_SUBMENU_FUNC(pmp->menu))
	{
		/* We're on a submenu item, but the submenu does not exist.
		 * The user defined missing_submenu_action may create it. */
		Bool is_complex_function;
		Bool is_busy_grabbed = False;
		char *menu_name;
		char *action;
		char *missing_action = MR_MISSING_SUBMENU_FUNC(pmp->menu);

		menu_name = PeekToken(
			SkipNTokens(MI_ACTION(med->mi), 1), NULL);
		if (!menu_name)
		{
			menu_name = "";
		}
		is_complex_function =
			functions_is_complex_function(missing_action);
		if (is_complex_function)
		{
			/*TA:  FIXME!  Use xasprintf()!! */
			char *action_ptr;
			action =
				xmalloc(
					strlen("Function") + 3 +
					strlen(missing_action) * 2 + 3 +
					strlen(menu_name) * 2 + 1);
			strcpy(action, "Function ");
			action_ptr = action + strlen(action);
			action_ptr = QuoteString(action_ptr, missing_action);
			*action_ptr++ = ' ';
			action_ptr = QuoteString(action_ptr, menu_name);
		}
		else
		{
			action = MR_MISSING_SUBMENU_FUNC(pmp->menu);
		}
		if (Scr.BusyCursor & BUSY_DYNAMICMENU)
		{
			is_busy_grabbed = GrabEm(CRS_WAIT, GRAB_BUSYMENU);
		}
		/* Execute the action */
		__menu_execute_function(pmp->pexc, action);
		if (is_complex_function)
		{
			free(action);
		}
		if (is_busy_grabbed)
		{
			UngrabEm(GRAB_BUSYMENU);
		}
		/* Let's see if the menu exists now. */
		mrMiPopup = mr_popup_for_mi(pmp->menu, med->mi);
	} /* run MISSING_SUBMENU_FUNCTION */
	in->mrPopup = mrMiPopup;
	if (!in->mrPopup)
	{
		in->mif.do_menu = False;
		pmret->flags.is_menu_posted = 0;
	}
	else
	{
		if (__mloop_make_popup(
			    pmp, pmret, in, med, pops, pdoes_submenu_overlap)
		    == MENU_MLOOP_RET_END)
		{
			return MENU_MLOOP_RET_END;
		}
	} /* else (in->mrPopup) */
	if (in->mif.do_popdown)
	{
		if (in->mrPopdown)
		{
			if (in->mrPopdown != in->mrPopup)
			{
				if (in->mi_with_popup ==
				    MR_PARENT_ITEM(in->mrPopdown))
				{
					in->mi_with_popup = NULL;
				}
				pop_menu_down_and_repaint_parent(
					&in->mrPopdown,
					pdoes_popdown_submenu_overlap, pmp);
			}
			in->mrPopdown = NULL;
		}
		in->mif.do_popdown = False;
	}
	if (in->mrPopup)
	{
		if (MR_PARENT_MENU(in->mrPopup) == pmp->menu)
		{
			med->mi = find_entry(
				pmp, NULL, &med->mrMi, None, -1, -1);
			if (med->mi && med->mrMi == in->mrPopup)
			{
				in->mif.do_menu = True;
				in->mif.is_submenu_mapped = True;
			}
		}
		else
		{
			/* This menu must be already mapped somewhere else, so
			 * ignore it completely.  This can only happen if we
			 * have reached the maximum allowed number of menu
			 * copies. */
			in->mif.do_menu = False;
			in->mif.do_popdown = False;
			in->mrPopup = NULL;
		}
	}

	return MENU_MLOOP_RET_NORMAL;
}

static mloop_ret_code_t __mloop_do_menu(
	MenuParameters *pmp, MenuReturn *pmret, double_keypress *pdkp,
	mloop_evh_input_t *in, mloop_evh_data_t *med, MenuOptions *pops,
	Bool *pdoes_submenu_overlap)
{
	MenuParameters mp;
	XEvent e;

	memset(&mp, 0, sizeof(mp));
	mp.menu = in->mrPopup;
	mp.pexc = pmp->pexc;
	mp.parent_menu = pmp->menu;
	mp.parent_item = med->mi;
	mp.tear_off_root_menu_window = pmp->tear_off_root_menu_window;
	MR_IS_TEAR_OFF_MENU(in->mrPopup) = 0;
	mp.flags.has_default_action = False;
	mp.flags.is_already_mapped = in->mif.is_submenu_mapped;
	mp.flags.is_sticky = False;
	mp.flags.is_submenu = True;
	mp.flags.is_triggered_by_keypress = !!in->mif.do_popup_and_warp;
	mp.pops = pops;
	mp.ret_paction = pmp->ret_paction;
	mp.screen_origin_x = pmp->screen_origin_x;
	mp.screen_origin_y = pmp->screen_origin_y;
	if (in->mif.do_propagate_event_into_submenu)
	{
		e = *(*pmp->pexc)->x.elast;
		mp.event_propagate_to_submenu = &e;
	}
	else
	{
		mp.event_propagate_to_submenu = NULL;
	}

	/* recursively do the new menu we've moved into */
	do_menu(&mp, pmret);

	in->mif.do_propagate_event_into_submenu = False;
	if (pmret->rc == MENU_PROPAGATE_EVENT)
	{
		in->mif.do_recycle_event = 1;
		return MENU_MLOOP_RET_LOOP;
	}
	if (IS_MENU_RETURN(pmret->rc))
	{
		pdkp->timestamp = 0;
		return MENU_MLOOP_RET_END;
	}
	if (MST_DO_UNMAP_SUBMENU_ON_POPDOWN(pmp->menu) &&
	    pmret->flags.is_key_press)
	{
		in->miRemovedSubmenu = MR_PARENT_ITEM(in->mrPopup);
		pop_menu_down_and_repaint_parent(
			&in->mrPopup, pdoes_submenu_overlap, pmp);
		in->mi_with_popup = NULL;
		MR_SUBMENU_ITEM(pmp->menu) = NULL;
		if (in->mrPopup == in->mrPopdown)
		{
			in->mrPopdown = NULL;
		}
		in->mrPopup = NULL;
	}
	if (pmret->rc == MENU_POPDOWN)
	{
		med->popup_delay_10ms = 0;
		in->mif.do_force_reposition = True;
	}

	return MENU_MLOOP_RET_NORMAL;
}

static mloop_ret_code_t __mloop_handle_action_with_mi(
	MenuParameters *pmp, MenuReturn *pmret, double_keypress *pdkp,
	mloop_evh_input_t *in, mloop_evh_data_t *med, mloop_static_info_t *msi,
	MenuOptions *pops, Bool *pdoes_submenu_overlap,
	Bool *pdoes_popdown_submenu_overlap)
{
	MenuItem *tmi;
	MenuRoot *tmrMi;
	MenuRoot *mrMiPopup = NULL;

	pmret->flags.do_unpost_submenu = 0;
	/* we're on a menu item */
	in->mif.is_off_menu_allowed = False;
	if (med->mrMi == pmp->menu && med->mi != in->miRemovedSubmenu)
	{
		in->miRemovedSubmenu = NULL;
	}
	if (med->mrMi != pmp->menu && med->mrMi != in->mrPopup &&
	    med->mrMi != in->mrPopdown)
	{
		/* we're on an item from a prior menu */
		if (med->mrMi != MR_PARENT_MENU(pmp->menu))
		{
			/* the event is for a previous menu, just close
			 * this one */
			pmret->rc = MENU_PROPAGATE_EVENT;
			pmret->target_menu = med->mrMi;
		}
		else
		{
			pmret->rc = MENU_POPDOWN;
		}
		pdkp->timestamp = 0;
		return MENU_MLOOP_RET_END;
	}
	if (med->mi != MR_SELECTED_ITEM(pmp->menu) && med->mrMi == pmp->menu)
	{
		/* new item of the same menu */
		__mloop_select_item(
			pmp, in, med, *pdoes_submenu_overlap,
			pdoes_popdown_submenu_overlap);
	}
	else if (med->mi != MR_SELECTED_ITEM(pmp->menu) && med->mrMi &&
		 med->mrMi == in->mrPopdown)
	{
		/* we're on the popup menu of a different menu item of this
		 * menu */
		med->mi = MR_PARENT_ITEM(in->mrPopdown);
		in->mrPopup = in->mrPopdown;
		in->mrPopdown = NULL;
		*pdoes_submenu_overlap = *pdoes_popdown_submenu_overlap;
		select_menu_item(pmp->menu, med->mi, True, (*pmp->pexc)->w.fw);
	}
	mrMiPopup = mr_popup_for_mi(pmp->menu, med->mi);
	/* check what has to be done with the item */
	if (__mloop_get_mi_actions(
		    pmp, pmret, pdkp, in, med, msi, mrMiPopup,
		    pdoes_submenu_overlap, pdoes_popdown_submenu_overlap) ==
	    MENU_MLOOP_RET_END)
	{
		return MENU_MLOOP_RET_END;
	}
	/* do what needs to be done */
	if (in->mif.do_popdown && !in->mif.do_popup)
	{
		/* popdown previous popup */
		__mloop_do_popdown(pmp, in, pdoes_popdown_submenu_overlap);
	}
	if (in->mif.do_popup)
	{
		if (__mloop_do_popup(
			    pmp, pmret, in, med, pops, mrMiPopup,
			    pdoes_submenu_overlap,
			    pdoes_popdown_submenu_overlap) ==
		    MENU_MLOOP_RET_END)
		{
			return MENU_MLOOP_RET_END;
		}
	}
	/* remember the 'posted' menu */
	if (pmret->flags.is_menu_posted && in->mrPopup != NULL &&
	    pmret->target_menu == NULL)
	{
		pmret->target_menu = in->mrPopup;
	}
	else if (pmret->flags.is_menu_posted && in->mrPopup == NULL)
	{
		pmret->flags.is_menu_posted = 0;
	}
	if (in->mif.do_menu)
	{
		mloop_ret_code_t rc;

		rc = __mloop_do_menu(
			pmp, pmret, pdkp, in, med, pops,
			pdoes_submenu_overlap);
		if (rc != MENU_MLOOP_RET_NORMAL)
		{
			return rc;
		}
	}
	/* Now check whether we can animate the current popup menu back to the
	 * original place to unobscure the current menu;  this happens only
	 * when using animation */
	if (in->mrPopup && MR_XANIMATION(in->mrPopup) &&
	    (tmi = find_entry(pmp, NULL, &tmrMi, None, -1, -1))  &&
	    (tmi == MR_SELECTED_ITEM(pmp->menu) || tmrMi != pmp->menu))
	{
		animated_move_back(in->mrPopup, False, (*pmp->pexc)->w.fw);
	}
	/* now check whether we should animate the current real menu
	 * over to the right to unobscure the prior menu; only a very
	 * limited case where this might be helpful and not too disruptive */
	/* but this cause terrible back-and-forth under certain circonstance,
	 * I think we should disable this ... 2002-09-17 olicha */
	if (in->mrPopup == NULL && pmp->parent_menu != NULL &&
	    MR_XANIMATION(pmp->menu) != 0 &&
	    pointer_in_passive_item_area(med->x_offset, med->mrMi))
	{
		/* we have to see if we need menu to be moved */
		animated_move_back(pmp->menu, True, (*pmp->pexc)->w.fw);
		if (in->mrPopdown)
		{
			if (in->mrPopdown != in->mrPopup)
			{
				pop_menu_down_and_repaint_parent(
					&in->mrPopdown,
					pdoes_popdown_submenu_overlap, pmp);
				in->mi_with_popup = NULL;
			}
			in->mrPopdown = NULL;
		}
		in->mif.do_popdown = False;
	}

	return MENU_MLOOP_RET_NORMAL;
}

static mloop_ret_code_t __mloop_handle_action_without_mi(
	MenuParameters *pmp, MenuReturn *pmret, double_keypress *pdkp,
	mloop_evh_input_t *in, mloop_evh_data_t *med, mloop_static_info_t *msi,
	MenuOptions *pops, Bool *pdoes_submenu_overlap,
	Bool *pdoes_popdown_submenu_overlap)
{
	pmret->flags.do_unpost_submenu = 0;
	/* moved off menu, deselect selected item... */
	if (!MR_SELECTED_ITEM(pmp->menu) ||
	    in->mif.is_off_menu_allowed == True || pmret->flags.is_menu_posted)
	{
		/* nothing to do */
		return MENU_MLOOP_RET_NORMAL;
	}
	if (in->mrPopup)
	{
		int x, y, mx, my;
		int mw, mh;

		if (FQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
				  &x, &y, &JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			x = 0;
			y = 0;
		}
		if (menu_get_geometry(
			    pmp->menu, &JunkRoot, &mx, &my, &mw, &mh, &JunkBW,
			    &JunkDepth) &&
		    ((!MR_IS_LEFT(in->mrPopup)  && x < mx) ||
		     (!MR_IS_RIGHT(in->mrPopup) && x > mx + mw) ||
		     (!MR_IS_UP(in->mrPopup)    && y < my) ||
		     (!MR_IS_DOWN(in->mrPopup)  && y > my + mh)))
		{
			select_menu_item(
				pmp->menu, MR_SELECTED_ITEM(pmp->menu), False,
				(*pmp->pexc)->w.fw);
			pop_menu_down_and_repaint_parent(
				&in->mrPopup, pdoes_submenu_overlap, pmp);
			in->mi_with_popup = NULL;
			MR_SUBMENU_ITEM(pmp->menu) = NULL;
			if (in->mrPopup == in->mrPopdown)
			{
				in->mrPopdown = NULL;
			}
			in->mrPopup = NULL;
		}
		else if (x < mx || x >= mx + mw || y < my || y >= my + mh)
		{
			/* pointer is outside the menu but do not pop down */
			in->mif.is_off_menu_allowed = True;
		}
		else
		{
			/* Pointer is still in the menu. Postpone the decision
			 * if we have to pop down. */
		}
	} /* if (in->mrPopup) */
	else
	{
		select_menu_item(
			pmp->menu, MR_SELECTED_ITEM(pmp->menu), False,
			(*pmp->pexc)->w.fw);
	}

	return MENU_MLOOP_RET_NORMAL;
}

static void __mloop_exit_warp_back(MenuParameters *pmp)
{
	MenuRoot *tmrMi;

	if (pmp->parent_menu && MR_SELECTED_ITEM(pmp->parent_menu))
	{
		warp_pointer_to_item(
			pmp->parent_menu, MR_SELECTED_ITEM(pmp->parent_menu),
			False);
		(void)find_entry(pmp, NULL, &tmrMi, None, -1, -1);
		if (pmp->parent_menu != tmrMi && MR_XANIMATION(pmp->menu) == 0)
		{
			/* Warping didn't take us to the correct menu, i.e. the
			 * spot we want to warp to is obscured. So raise our
			 * window first. */
			XRaiseWindow(dpy, MR_WINDOW(pmp->parent_menu));
		}
	}

	return;
}

static void __mloop_exit_select_in_place(
	MenuParameters *pmp, mloop_evh_data_t *med, MenuOptions *pops)
{
	MenuRoot *submenu;
	XWindowAttributes win_attribs;

	last_saved_pos_hints.flags.is_last_menu_pos_hints_valid = True;
	last_saved_pos_hints.pos_hints.x_offset = 0;
	last_saved_pos_hints.pos_hints.x_factor = 0;
	last_saved_pos_hints.pos_hints.context_x_factor = 0;
	last_saved_pos_hints.pos_hints.y_factor = 0;
	last_saved_pos_hints.pos_hints.is_relative = False;
	last_saved_pos_hints.pos_hints.is_menu_relative = False;
	submenu = mr_popup_for_mi(pmp->menu, med->mi);
	if (submenu && MR_WINDOW(submenu) != None &&
	    XGetWindowAttributes(dpy, MR_WINDOW(submenu), &win_attribs) &&
	    win_attribs.map_state == IsViewable &&
	    menu_get_geometry(
		    submenu, &JunkRoot, &last_saved_pos_hints.pos_hints.x,
		    &last_saved_pos_hints.pos_hints.y, &JunkWidth, &JunkHeight,
		    &JunkBW, &JunkDepth))
	{
		/* The submenu is mapped, just take its position and put it in
		 * the position hints. */
	}
	else if (pops->flags.has_poshints)
	{
		last_saved_pos_hints.pos_hints = pops->pos_hints;
	}
	else
	{
		Bool dummy;

		get_prefered_popup_position(
			pmp->menu, submenu, &last_saved_pos_hints. pos_hints.x,
			&last_saved_pos_hints. pos_hints.y, &dummy);
	}
	if (pops->flags.do_warp_on_select)
	{
		last_saved_pos_hints.flags.do_warp_title = 1;
	}

	return;
}

static void __mloop_exit_selected(
	MenuParameters *pmp, MenuReturn *pmret, mloop_evh_data_t *med,
	MenuOptions *pops)
{
	/* save action to execute so that the menu may be destroyed now */
	if (pmp->ret_paction)
	{
		if (pmret->rc == MENU_EXEC_CMD)
		{
			*pmp->ret_paction = xstrdup(*pmp->ret_paction);
		}
		else
		{
			if (*pmp->ret_paction)
			{
				free(*pmp->ret_paction);
			}
			*pmp->ret_paction = (med->mi) ?
				xstrdup(MI_ACTION(med->mi)) : NULL;
		}
	}
	if (
		pmp->ret_paction && *pmp->ret_paction &&
		med->mi && MI_IS_POPUP(med->mi))
	{
		get_popup_options(pmp, med->mi, pops);
		if (pops->flags.do_select_in_place)
		{
			__mloop_exit_select_in_place(pmp, med, pops);
		}
		else
		{
			last_saved_pos_hints.flags.do_ignore_pos_hints = True;
		} /* pops->flags.do_select_in_place */
		last_saved_pos_hints.pos_hints.screen_origin_x =
			pmp->screen_origin_x;
		last_saved_pos_hints.pos_hints.screen_origin_y =
			pmp->screen_origin_y;
	}

	return;
}

static void __mloop_exit(
	MenuParameters *pmp, MenuReturn *pmret, double_keypress *pdkp,
	mloop_evh_input_t *in, mloop_evh_data_t *med, mloop_static_info_t *msi,
	MenuOptions *pops)
{
	Bool no = False;
	Bool do_deselect = False;

	if (in->mrPopdown)
	{
		pop_menu_down_and_repaint_parent(&in->mrPopdown, &no, pmp);
		MR_SUBMENU_ITEM(pmp->menu) = NULL;
	}
	if (
		pmret->rc == MENU_SELECTED && is_double_click(
			msi->t0, med->mi, pmp, pmret, pdkp,
			in->mif.has_mouse_moved))
	{
		pmret->rc = MENU_DOUBLE_CLICKED;
	}
	if (
		pmret->rc == MENU_SELECTED && med->mi &&
		MI_FUNC_TYPE(med->mi) == F_TEARMENUOFF)
	{
		pmret->rc = (MR_IS_TEAR_OFF_MENU(pmp->menu)) ?
			MENU_KILL_TEAR_OFF_MENU : MENU_TEAR_OFF;
	}
	switch (pmret->rc)
	{
	case MENU_POPDOWN:
	case MENU_PROPAGATE_EVENT:
	case MENU_DOUBLE_CLICKED:
		do_deselect = True;
		/* Allow popdown to warp back pointer to main menu with mouse
		   button control. (MoveLeft/MoveRight on a mouse binding) */
		if (((pmret->rc == MENU_POPDOWN && in->mif.is_button_release)
		     || in->mif.is_key_press) &&
		    pmret->rc != MENU_DOUBLE_CLICKED)
		{
			if (!pmp->flags.is_submenu)
			{
				/* abort a root menu rather than pop it down */
				pmret->rc = MENU_ABORTED;
			}
			__mloop_exit_warp_back(pmp);
		}
		break;
	case MENU_ABORTED:
	case MENU_TEAR_OFF:
	case MENU_KILL_TEAR_OFF_MENU:
		do_deselect = True;
		break;
	case MENU_SELECTED:
	case MENU_EXEC_CMD:
		__mloop_exit_selected(pmp, pmret, med, pops);
		pmret->rc = MENU_DONE;
		break;
	default:
		break;
	}
	if (do_deselect && MR_SELECTED_ITEM(pmp->menu))
	{
		select_menu_item(
			pmp->menu, MR_SELECTED_ITEM(pmp->menu), False,
			(*pmp->pexc)->w.fw);
	}
	if (pmret->rc == MENU_SUBMENU_TORN_OFF)
	{
		in->mrPopup = NULL;
		MR_SUBMENU_ITEM(pmp->menu) = NULL;
	}
	if (in->mrPopup)
	{
		pop_menu_down_and_repaint_parent(&in->mrPopup, &no, pmp);
		MR_SUBMENU_ITEM(pmp->menu) = NULL;
	}
	pmret->flags.is_key_press = in->mif.is_key_press;

	return;
}

static void __menu_loop(
	MenuParameters *pmp, MenuReturn *pmret, double_keypress *pdkp)
{
	mloop_evh_input_t mei;
	mloop_ret_code_t mloop_ret;
	mloop_evh_data_t med;
	mloop_static_info_t msi;
	MenuOptions mops;
	Bool is_finished;
	Bool does_submenu_overlap = False;
	Bool does_popdown_submenu_overlap = False;

	__mloop_init(pmp, pmret, &mei, &med, &msi, &mops);
	for (is_finished = False; !is_finished; )
	{
		mloop_ret = __mloop_get_event(pmp, pmret, &mei, &med, &msi);
		switch (mloop_ret)
		{
		case MENU_MLOOP_RET_END:
			is_finished = True;
		case MENU_MLOOP_RET_LOOP:
			continue;
		default:
			break;
		}
		mloop_ret = __mloop_handle_event(
			pmp, pmret, pdkp, &mei, &med, &msi);
		switch (mloop_ret)
		{
		case MENU_MLOOP_RET_END:
			is_finished = True;
			continue;
		case MENU_MLOOP_RET_LOOP:
			continue;
		default:
			break;
		}
		/* Now handle new menu items, whether it is from a keypress or
		 * a pointer motion event. */
		if (med.mi != NULL)
		{
			mloop_ret = __mloop_handle_action_with_mi(
				pmp, pmret, pdkp, &mei, &med, &msi, &mops,
				&does_submenu_overlap,
				&does_popdown_submenu_overlap);
		}
		else
		{
			mloop_ret = __mloop_handle_action_without_mi(
				pmp, pmret, pdkp, &mei, &med, &msi, &mops,
				&does_submenu_overlap,
				&does_popdown_submenu_overlap);
		}
		if (mloop_ret == MENU_MLOOP_RET_END)
		{
			is_finished = True;
		}
		XFlush(dpy);
	}
	__mloop_exit(pmp, pmret, pdkp, &mei, &med, &msi, &mops);

	return;
}

/*
 * Functions dealing with tear off menus
 */

static char *menu_strip_tear_off_title(MenuRoot *mr)
{
	MenuItem *mi;
	int i;
	int len;
	char *name;

	for (mi = MR_FIRST_ITEM(mr); mi != NULL; mi = MI_NEXT_ITEM(mi))
	{
		if (MI_IS_TITLE(mi))
		{
			break;
		}
		else if (!MI_IS_SEPARATOR(mi) && !MI_IS_TEAR_OFF_BAR(mi))
		{
			/* a normal item, no title found */
			return NULL;
		}
		/* skip separators and tear off bars */
	}
	if (mi == NULL || !MI_HAS_TEXT(mi) || MI_NEXT_ITEM(mi) == NULL)
	{
		return NULL;
	}
	/* extract the window title from the labels */
	for (i = 0, len = 0; i < MAX_MENU_ITEM_LABELS; i++)
	{
		if (MI_LABEL(mi)[i] != 0)
		{
			len += strlen(MI_LABEL(mi)[i]) + 1;
		}
	}
	if (len == 0)
	{
		return NULL;
	}
	name = xmalloc(len + 1);
	*name = 0;
	for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
	{
		if (MI_LABEL(mi)[i] != 0)
		{
			strcat(name, MI_LABEL(mi)[i]);
			strcat(name, " ");
		}
	}
	/* strip the last space */
	name[len - 1] = 0;
	/* unlink and destroy the item */
	unlink_item_from_menu(mr, mi);
	menuitem_free(mi);

	return name;
}

static int _pred_menu_window_weed_events(
	Display *display, XEvent *event, XPointer arg)
{
	switch (event->type)
	{
	case CirculateNotify:
	case ConfigureNotify:
	case CreateNotify:
	case DestroyNotify:
	case GravityNotify:
	case MapNotify:
	case ReparentNotify:
	case UnmapNotify:
		/* events in SubstructureNotifyMask */
		return 1;
	default:
		return 0;
	}
}

static void menu_tear_off(MenuRoot *mr_to_copy)
{
	MenuRoot *mr;
	MenuStyle *ms;
	XEvent ev;
	XSizeHints menusizehints;
	XClassHint menuclasshints;
	XTextProperty menunametext;
	XWMHints menuwmhints;
	char *list[] ={ NULL, NULL };
	char *t;
	char *name = NULL;
	Atom protocols[1];
	int x = 0;
	int y = 0;
	unsigned int add_mask = 0;
	initial_window_options_t win_opts;
	evh_args_t ea;
	exec_context_changes_t ecc;
	char *buffer;
	char *action;
	cond_rc_t *cond_rc = NULL;
	const exec_context_t *exc = NULL;

	/* keep the menu open */
	if (MR_WINDOW(mr_to_copy) != None)
	{
		XSync(dpy, 0);
		FWeedIfWindowEvents(
			dpy, MR_WINDOW(mr_to_copy),
			_pred_menu_window_weed_events, NULL);
	}
	mr = clone_menu(mr_to_copy);
	/* also dump the menu style */
	buffer = xmalloc(23);
	sprintf(buffer,"%lu",(unsigned long)mr);
	action = buffer;
	ms = menustyle_parse_style(F_PASS_ARGS);
	if (!ms)
	{
		/* this must never happen */
		fvwm_msg(
			ERR, "menu_tear_off",
			"impossible to create %s menu style", buffer);
		free(buffer);
		DestroyMenu(mr, False, False);
		return;
	}
	free(buffer);
	menustyle_copy(MR_STYLE(mr_to_copy),ms);
	MR_STYLE(mr) = ms;
	MST_USAGE_COUNT(mr) = 0;
	name = menu_strip_tear_off_title(mr);
	/* create the menu window and size the menu */
	make_menu(mr, True);
	/* set position */
	if (menu_get_geometry(
		    mr_to_copy, &JunkRoot, &x, &y, &JunkWidth, &JunkHeight,
		    &JunkBW, &JunkDepth))
	{
		add_mask = PPosition;
		XMoveWindow(dpy, MR_WINDOW(mr), x, y);
	}
	else
	{
		add_mask = 0;
	}
	/* focus policy */
	menuwmhints.flags = InputHint;
	menuwmhints.input = False;
	/* size hints */
	menusizehints.flags = PBaseSize | PMinSize | PMaxSize | add_mask;
	menusizehints.base_width  = 0;
	menusizehints.base_height = 0;
	menusizehints.min_width = MR_WIDTH(mr);
	menusizehints.min_height = MR_HEIGHT(mr);
	menusizehints.max_width = MR_WIDTH(mr);
	menusizehints.max_height = MR_HEIGHT(mr);
	/* class, resource and names */
	menuclasshints.res_name =
		(name != NULL) ? name : xstrdup(MR_NAME(mr));
	menuclasshints.res_class = xstrdup("fvwm_menu");
	for (t = menuclasshints.res_name; t != NULL && *t != 0; t++)
	{
		/* replace tabs in the title with spaces */
		if (*t == '\t')
		{
			*t = ' ';
		}
	}
	list[0] = menuclasshints.res_name;
	menunametext.value = NULL;
	XStringListToTextProperty(list, 1, &menunametext);
	/* set all properties and hints */
	XSetWMProperties(
		dpy, MR_WINDOW(mr), &menunametext, &menunametext, NULL, 0,
		&menusizehints, NULL, &menuclasshints);
	XSetWMHints(dpy, MR_WINDOW(mr), &menuwmhints);
	protocols[0] = _XA_WM_DELETE_WINDOW;
	XSetWMProtocols(dpy, MR_WINDOW(mr), &(protocols[0]), 1);

	/* free memory */
	if (menunametext.value != NULL)
	{
		XFree(menunametext.value);
	}
	free(menuclasshints.res_class);
	free(menuclasshints.res_name);

	/* manage the window */
	memset(&win_opts, 0, sizeof(win_opts));
	win_opts.flags.is_menu = True;
	ev.type = MapRequest;
	ev.xmaprequest.send_event = True;
	ev.xmaprequest.display = dpy;
	ev.xmaprequest.parent = Scr.Root;
	ev.xmaprequest.window = MR_WINDOW(mr);
	fev_fake_event(&ev);
	ecc.type = EXCT_NULL;
	ecc.x.etrigger = &ev;
	ecc.w.w = MR_WINDOW(mr);
	ecc.w.wcontext = C_ROOT;
	ea.exc = exc_create_context(
		&ecc, ECC_TYPE | ECC_ETRIGGER | ECC_W | ECC_WCONTEXT);
	HandleMapRequestKeepRaised(&ea, None, NULL, &win_opts);
	exc_destroy_context(ea.exc);

	return;
}

static void do_menu_close_tear_off_menu(MenuRoot *mr, MenuParameters mp)
{
	pop_menu_down(&mr, &mp);
	menustyle_free(MR_STYLE(mr));
	DestroyMenu(mr, False, False);
}

/* ---------------------------- interface functions ------------------------- */

void menus_init(void)
{
	memset((&Menus), 0, sizeof(MenuInfo));

	return;
}

/* menus_find_menu expects a token as the input. Make sure you have used
 * GetNextToken before passing a menu name to remove quotes (if necessary) */
MenuRoot *menus_find_menu(char *name)
{
	MenuRoot *mr;

	if (name == NULL)
	{
		return NULL;
	}
	for (mr = Menus.all; mr != NULL; mr = MR_NEXT_MENU(mr))
	{
		if (!MR_IS_DESTROYED(mr) && mr == MR_ORIGINAL_MENU(mr) &&
		    MR_NAME(mr) != NULL && StrEquals(name, MR_NAME(mr)))
		{
			break;
		}
	}

	return mr;
}

void menus_remove_style_from_menus(MenuStyle *ms)
{
	MenuRoot *mr;

	for (mr = Menus.all; mr; mr = MR_NEXT_MENU(mr))
	{
		if (MR_STYLE(mr) == ms)
		{
			MR_STYLE(mr) = menustyle_get_default_style();
			MR_IS_UPDATED(mr) = 1;
		}
	}

	return;
}

/*
 * Functions dealing with tear off menus
 */

void menu_enter_tear_off_menu(const exec_context_t *exc)
{
	MenuRoot *mr;
	char *ret_action = NULL;
	MenuOptions mops;
	MenuParameters mp;
	MenuReturn mret;
	const exec_context_t *exc2;
	exec_context_changes_t ecc;

	if (XFindContext(
		    dpy, FW_W(exc->w.fw), MenuContext,
		    (caddr_t *)&mr) == XCNOENT)
	{
		return;
	}
	ecc.w.fw = NULL;
	ecc.w.w = None;
	ecc.w.wcontext = C_ROOT;
	exc2 = exc_clone_context(exc, &ecc, ECC_FW | ECC_W | ECC_WCONTEXT);
	memset(&mops, 0, sizeof(mops));
	memset(&mret, 0, sizeof(MenuReturn));
	memset(&mp, 0, sizeof(mp));
	mp.menu = mr;
	mp.pexc = &exc2;
	mp.tear_off_root_menu_window = exc->w.fw;
	MR_IS_TEAR_OFF_MENU(mr) = 1;
	mp.flags.has_default_action = 0;
	mp.flags.is_already_mapped = True;
	mp.flags.is_sticky = False;
	mp.flags.is_submenu = False;
	mp.pops = &mops;
	mp.ret_paction = &ret_action;
	do_menu(&mp, &mret);
	exc_destroy_context(exc2);

	return;
}

void menu_close_tear_off_menu(FvwmWindow *fw)
{
	MenuRoot *mr;
	MenuParameters mp;
	const exec_context_t *exc;
	exec_context_changes_t ecc;

	if (XFindContext(
		    dpy, FW_W(fw), MenuContext, (caddr_t *)&mr) == XCNOENT)
	{
		return;
	}
	memset(&mp, 0, sizeof(mp));
	mp.menu = mr;
	ecc.w.fw = NULL;
	ecc.w.w = None;
	ecc.w.wcontext = C_ROOT;
	exc = exc_create_context(&ecc, ECC_FW | ECC_W | ECC_WCONTEXT);
	mp.pexc = &exc;
	do_menu_close_tear_off_menu(mr, mp);
	exc_destroy_context(exc);

	return;
}

Bool menu_redraw_transparent_tear_off_menu(FvwmWindow *fw, Bool pr_only)
{
	MenuRoot *mr;
	MenuStyle *ms = NULL;
	int cs;

	if (!(IS_TEAR_OFF_MENU(fw) &&
	      XFindContext(dpy, FW_W(fw), MenuContext,(caddr_t *)&mr) !=
	      XCNOENT &&
	      (ms = MR_STYLE(mr)) &&
	      ST_HAS_MENU_CSET(ms)))
	{
		return False;
	}

	cs = ST_CSET_MENU(ms);

	if (!CSET_IS_TRANSPARENT(cs) ||
	    (pr_only && !CSET_IS_TRANSPARENT_PR(cs)))
	{
		return False;
	}

	return UpdateBackgroundTransparency(
		dpy, MR_WINDOW(mr), MR_WIDTH(mr),
		MR_HEIGHT(mr),
		&Colorset[ST_CSET_MENU(ms)],
		Pdepth,
		FORE_GC(MST_MENU_INACTIVE_GCS(mr)),
		True);
}

/*
 *
 * Initiates a menu pop-up
 *
 * fStick = True  = sticky menu, stays up on initial button release.
 * fStick = False = transient menu, drops on initial release.
 *
 * eventp = 0: menu opened by mouse, do not warp
 * eventp > 1: root menu opened by keypress with 'Menu', warp pointer and
 *             allow 'double-keypress'.
 * eventp = 1: menu opened by keypress, warp but forbid 'double-keypress'
 *             this should always be used except in the call in 'staysup_func'
 *             in builtin.c
 *
 * Returns one of MENU_NOP, MENU_ERROR, MENU_ABORTED, MENU_DONE
 * do_menu() may destroy the *pmp->pexec member and replace it with a new
 * copy.  Be sure not to rely on the original structure being kept intact
 * when calling do_menu().
 */
void do_menu(MenuParameters *pmp, MenuReturn *pmret)
{
	int x;
	int y;
	Bool fWasAlreadyPopped = False;
	Bool key_press;
	Bool is_pointer_grabbed = False;
	Bool is_pointer_ungrabbed = False;
	Bool do_menu_interaction;
	Bool do_check_pop_down;
	Bool do_warp;
	Time t0 = fev_get_evtime();
	XEvent tmpevent;
	double_keypress dkp;
	/* don't save these ones, we want them to work even within recursive
	 * menus popped up by dynamic actions. */
	static int indirect_depth = 0;
	static int x_start;
	static int y_start;
	int scr_x, scr_y;
	int scr_w, scr_h;

	pmret->rc = MENU_NOP;
	if (pmp->flags.is_sticky && !pmp->flags.is_submenu)
	{
		FCheckTypedEvent(dpy, ButtonPressMask, &tmpevent);
	}
	if (pmp->menu == NULL)
	{
		pmret->rc = MENU_ERROR;
		return;
	}
	key_press = pmp->flags.is_triggered_by_keypress;

	/* Try to pick a root-relative optimal x,y to
	 * put the mouse right on the title w/o warping */
	if (FQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
			  &x, &y, &JunkX, &JunkY, &JunkMask) == False)
	{
		/* pointer is on a different screen */
		x = 0;
		y = 0;
	}
	/* Save these - we want to warp back here if this is a top level
	 * menu brought up by a keystroke */
	if (!pmp->flags.is_submenu)
	{
		pmp->flags.is_invoked_by_key_press = key_press;
		pmp->flags.is_first_root_menu = !indirect_depth;
	}
	else
	{
		pmp->flags.is_first_root_menu = 0;
	}
	if (!pmp->flags.is_submenu && indirect_depth == 0)
	{
		if (key_press)
		{
			x_start = x;
			y_start = y;
		}
		else
		{
			x_start = -1;
			y_start = -1;
		}
		pmp->screen_origin_x = pmp->pops->pos_hints.screen_origin_x;
		pmp->screen_origin_y = pmp->pops->pos_hints.screen_origin_y;
	}
	if (pmp->pops->flags.do_warp_title)
	{
		do_warp = True;
	}
	else if (pmp->pops->flags.do_not_warp)
	{
		do_warp = False;
	}
	else if (key_press)
	{
		do_warp = True;
	}
	else
	{
		do_warp = False;
	}
	/* Figure out where we should popup, if possible */
	if (!pmp->flags.is_already_mapped)
	{
		Bool prefer_left_submenus = False;
		/* Make sure we are using the latest style and menu layout. */
		update_menu(pmp->menu, pmp);

		if (pmp->flags.is_submenu)
		{
			/* this is a submenu popup */
			get_prefered_popup_position(
				pmp->parent_menu, pmp->menu, &x, &y,
				&prefer_left_submenus);
		}
		else
		{
			fscreen_scr_arg fscr;

			/* we're a top level menu */
			if (!GrabEm(CRS_MENU, GRAB_MENU))
			{
				/* GrabEm specifies the cursor to use */
				XBell(dpy, 0);
				pmret->rc = MENU_ABORTED;
				return;
			}
			is_pointer_grabbed = True;
			/* Make the menu appear under the pointer rather than
			 * warping */
			fscr.xypos.x = x;
			fscr.xypos.y = y;
			FScreenGetScrRect(
				&fscr, FSCREEN_XYPOS, &scr_x, &scr_y, &scr_w,
				&scr_h);
			x -= menudim_middle_x_offset(&MR_DIM(pmp->menu));
			y -= menuitem_middle_y_offset(
				MR_FIRST_ITEM(pmp->menu), MR_STYLE(pmp->menu));
			if (x < scr_x)
			{
				x = scr_x;
			}
			if (y < scr_y)
			{
				y = scr_y;
			}
		}

		/* pop_menu_up may move the x,y to make it fit on screen more
		 * nicely. It might also move parent_menu out of the way. */
		if (!pop_menu_up(
			    &(pmp->menu), pmp, pmp->parent_menu, NULL,
			    pmp->pexc, x, y, prefer_left_submenus, do_warp,
			    pmp->pops, NULL, None))
		{
			XBell(dpy, 0);
			UngrabEm(GRAB_MENU);
			pmret->rc = MENU_ERROR;
			return;
		}
	}
	else
	{
		fWasAlreadyPopped = True;
		if (pmp->tear_off_root_menu_window != NULL)
		{
			if (!GrabEm(CRS_MENU, GRAB_MENU))
			{
				/* GrabEm specifies the cursor to use */
				XBell(dpy, 0);
				pmret->rc = MENU_ABORTED;
				return;
			}
			is_pointer_grabbed = True;
		}
		if (key_press)
		{
			warp_pointer_to_item(
				pmp->menu, MR_FIRST_ITEM(pmp->menu),
				True /* skip Title */);
		}
	}

	/* Remember the key that popped up the root menu. */
	if (pmp->flags.is_submenu)
	{
		dkp.timestamp = 0;
	}
	else
	{
		if (pmp->flags.is_triggered_by_keypress)
		{
			/* we have a real key event */
			dkp.keystate = (*pmp->pexc)->x.etrigger->xkey.state;
			dkp.keycode = (*pmp->pexc)->x.etrigger->xkey.keycode;
		}
		dkp.timestamp =
			(key_press && pmp->flags.has_default_action) ? t0 : 0;
	}
	if (!pmp->flags.is_submenu && indirect_depth == 0)
	{
		/* we need to grab the keyboard so we are sure no key presses
		 * are lost */
		MyXGrabKeyboard(dpy);
	}
	/* This may loop for tear off menus */
	for (do_menu_interaction = True; do_menu_interaction == True; )
	{
		if (is_pointer_ungrabbed && !GrabEm(CRS_MENU, GRAB_MENU))
		{
			/* re-grab the pointer in this cycle */
			XBell(dpy, 0);
			pmret->rc = MENU_ABORTED;
			break;
		}
		do_menu_interaction = False;
		if (pmp->pops->flags.do_tear_off_immediately == 1)
		{
			pmret->rc = MENU_TEAR_OFF;
		}
		else
		{
			if (!pmp->flags.is_submenu)
			{
				XSelectInput(
					dpy, Scr.NoFocusWin, XEVMASK_MENUNFW);
				XFlush(dpy);
			}
			__menu_loop(pmp, pmret, &dkp);
			if (!pmp->flags.is_submenu)
			{
				XSelectInput(
					dpy, Scr.NoFocusWin, XEVMASK_NOFOCUSW);
				XFlush(dpy);
			}
		}
		do_check_pop_down = False;
		switch (pmret->rc)
		{
		case MENU_TEAR_OFF:
			menu_tear_off(pmp->menu);
			pop_menu_down(&pmp->menu, pmp);
			break;
		case MENU_KILL_TEAR_OFF_MENU:
			if (MR_IS_TEAR_OFF_MENU(pmp->menu))
			{
				/* kill the menu */
				do_menu_close_tear_off_menu(pmp->menu, *pmp);
				pmret->rc = MENU_ABORTED;
			}
			else
			{
				/* pass return code up to the torn off menu */
			}
			break;
		case MENU_DOUBLE_CLICKED:
		case MENU_DONE:
			if (MR_IS_TEAR_OFF_MENU(pmp->menu))
			{
				do_menu_interaction = True;
			}
			else
			{
				do_check_pop_down = True;
			}
			break;
		case MENU_SUBMENU_TORN_OFF:
			pmret->rc = MENU_ABORTED;
			do_check_pop_down = True;
			break;
		default:
			do_check_pop_down = True;
			break;
		}
		if (do_check_pop_down == True)
		{
			/* popping down may destroy the menu via the dynamic
			 * popdown action! */
			if (!MR_IS_TEAR_OFF_MENU(pmp->menu) &&
			    fWasAlreadyPopped == False)
			{
				pop_menu_down(&pmp->menu, pmp);
			}
		}
		XFlush(dpy);

		if (!pmp->flags.is_submenu && x_start >= 0 && y_start >= 0 &&
		    pmret->flags.is_key_press && pmret->rc != MENU_TEAR_OFF)
		{
			/* warp pointer back to where invoked if this was
			 * brought up with a keypress and we're returning from
			 * a top level menu, and a button release event didn't
			 * end it */
			FWarpPointer(
				dpy, 0, Scr.Root, 0, 0, Scr.MyDisplayWidth,
				Scr.MyDisplayHeight, x_start, y_start);
			if ((*pmp->pexc)->x.elast->type == KeyPress)
			{
				XEvent e = *(*pmp->pexc)->x.elast;

				e.xkey.x_root = x_start;
				e.xkey.y_root = y_start;
				fev_fake_event(&e);
			}
		}
		if (pmret->rc == MENU_TEAR_OFF)
		{
			pmret->rc = MENU_SUBMENU_TORN_OFF;
		}

		dkp.timestamp = 0;
		if (is_pointer_grabbed)
		{
			UngrabEm(GRAB_MENU);
			WaitForButtonsUp(True);
			is_pointer_ungrabbed = True;
		}
		if (!pmp->flags.is_submenu)
		{
			if (pmret->rc == MENU_DONE)
			{
				if (pmp->ret_paction && *pmp->ret_paction)
				{
					indirect_depth++;
					/* Execute the action */
					__menu_execute_function(
						pmp->pexc, *pmp->ret_paction);
					indirect_depth--;
					free(*pmp->ret_paction);
					*pmp->ret_paction = NULL;
				}
				last_saved_pos_hints.flags.
					do_ignore_pos_hints = False;
				last_saved_pos_hints.flags.
					is_last_menu_pos_hints_valid = False;
			}
			if (indirect_depth == 0)
			{
				last_saved_pos_hints.flags.
					do_ignore_pos_hints = False;
				last_saved_pos_hints.flags.
					is_last_menu_pos_hints_valid = False;
			}
		}
	}
	if (!pmp->flags.is_submenu && indirect_depth == 0)
	{
		/* release the keyboard when the last menu closes */
		MyXUngrabKeyboard(dpy);
	}

	return;
}

Bool menu_expose(XEvent *event, FvwmWindow *fw)
{
	MenuRoot *mr = NULL;

	if ((XFindContext(
		     dpy, event->xany.window, MenuContext, (caddr_t *)&mr) !=
	     XCNOENT))
	{
		flush_accumulate_expose(event->xany.window, event);
		paint_menu(mr, event, fw);

		return True;
	}
	else
	{
		return False;
	}
}

/*
 *
 *  Procedure:
 *      update_transparent_menu_bg - set the background of the menu to
 *      match a forseen move of a menu. If the background is updated
 *	for the target position before the move is done, and repainted
 *	after the move, the move will look more seamless.
 *
 *	This method should be folleowd by a call to repaint_transparent_menu
 *	with the same step_x, stey_y, end_x and any_y, with is_bg_set True
 */
void update_transparent_menu_bg(
	MenuRepaintTransparentParameters *prtm,
	int current_x, int current_y, int step_x, int step_y,
	int end_x, int end_y)
{
	MenuRoot *mr;
	MenuStyle *ms;
	Bool last = False;

	mr = prtm->mr;
	ms = MR_STYLE(mr);
	if (step_x == end_x && step_y == end_y)
	{
		last = True;
	}
	if (!last && CSET_IS_TRANSPARENT_PR_TINT(ST_CSET_MENU(ms)))
	{
		/* too slow ... */
		return;
	}
	SetWindowBackgroundWithOffset(
		dpy, MR_WINDOW(mr), step_x - current_x, step_y - current_y,
		MR_WIDTH(mr), MR_HEIGHT(mr),
		&Colorset[ST_CSET_MENU(ms)], Pdepth,
		FORE_GC(MST_MENU_INACTIVE_GCS(mr)), False);
}


/*
 *
 *  Procedure:
 *      repaint_transparent_menu - repaint the menu background if it is
 *      tranparent during an animated move. Called in move_resize.c
 *      (AnimatedMoveAnyWindow). Performance improvement Welcome!
 *  ideas: - write the foreground into a pixmap and a mask the first time
 *          this function is called. Then use these pixmaps to draw
 *	    the items
 *         - Use a Buffer if !IS_TRANSPARENT_PR_PURE and if we do not have one
 *           already
 */
void repaint_transparent_menu(
	MenuRepaintTransparentParameters *prtm,
	Bool first, int x, int y, int end_x, int end_y, Bool is_bg_set)
{
	MenuItem *mi;
	MenuRoot *mr;
	MenuStyle *ms;
	int h = 0;
	int s_h = 0;
	int e_h = 0;
	Bool last = False;

	mr = prtm->mr;
	ms = MR_STYLE(mr);
	if (x == end_x && y == end_y)
	{
		last = True;
	}
	if (!last && CSET_IS_TRANSPARENT_PR_TINT(ST_CSET_MENU(ms)))
	{
		/* too slow ... */
		return;
	}
	if (!is_bg_set)
	{
		SetWindowBackground(
			dpy, MR_WINDOW(mr), MR_WIDTH(mr), MR_HEIGHT(mr),
			&Colorset[ST_CSET_MENU(ms)], Pdepth,
			FORE_GC(MST_MENU_INACTIVE_GCS(mr)), False);
	}
	/* redraw the background of non active item */
	for (mi = MR_FIRST_ITEM(mr); mi != NULL; mi = MI_NEXT_ITEM(mi))
	{
		if (mi == MR_SELECTED_ITEM(mr) && MST_DO_HILIGHT_BACK(mr))
		{
			int left;

			left = MR_HILIGHT_X_OFFSET(mr) - MR_ITEM_X_OFFSET(mr);
			if (left > 0)
			{
				XClearArea(
					dpy, MR_WINDOW(mr),
					MR_ITEM_X_OFFSET(mr), MI_Y_OFFSET(mi),
					left, MI_HEIGHT(mi) +
					MST_RELIEF_THICKNESS(mr), 0);
			}
			h = MI_HEIGHT(mi);
			continue;
		}
		if (h == 0)
		{
			s_h += MI_HEIGHT(mi);
		}
		else
		{
			e_h += MI_HEIGHT(mi);
		}
	}
	XClearArea(
		dpy, MR_WINDOW(mr), MR_ITEM_X_OFFSET(mr), MST_BORDER_WIDTH(mr),
		MR_ITEM_WIDTH(mr), s_h, 0);
	if (e_h != 0)
	{
		XClearArea(
			dpy, MR_WINDOW(mr), MR_ITEM_X_OFFSET(mr),
			s_h + h + MST_RELIEF_THICKNESS(mr) +
			MST_BORDER_WIDTH(mr), MR_ITEM_WIDTH(mr), e_h, 0);
	}

	/* now redraw the items */
	for (mi = MR_FIRST_ITEM(mr); mi != NULL; mi = MI_NEXT_ITEM(mi))
	{
		MenuPaintItemParameters mpip;

		if (mi == MR_SELECTED_ITEM(mr) && MST_DO_HILIGHT_BACK(mr) &&
		    !CSET_IS_TRANSPARENT_PR_TINT(ST_CSET_MENU(ms)))
		{
			continue;
		}
		get_menu_paint_item_parameters(
			&mpip, mr, NULL, prtm->fw, NULL, True);
		mpip.flags.is_first_item = (MR_FIRST_ITEM(mr) == mi);
		menuitem_paint(mi, &mpip);
	}

	/* if we have a side pic and no side colors we shound repaint the side
	 * pic */
	if ((MR_SIDEPIC(mr) || MST_SIDEPIC(mr)) &&
	    !MR_HAS_SIDECOLOR(mr) && !MST_HAS_SIDE_COLOR(mr))
	{
		FvwmPicture *sidePic;
		if (MR_SIDEPIC(mr))
		{
			sidePic = MR_SIDEPIC(mr);
		}
		else if (MST_SIDEPIC(mr))
		{
			sidePic = MST_SIDEPIC(mr);
		}
		else
		{
			return;
		}
		XClearArea(
			dpy, MR_WINDOW(mr), MR_SIDEPIC_X_OFFSET(mr),
			MST_BORDER_WIDTH(mr), sidePic->width,
			MR_HEIGHT(mr) - 2 * MST_BORDER_WIDTH(mr), 0);
		paint_side_pic(mr, NULL);
	}
}

Bool DestroyMenu(MenuRoot *mr, Bool do_recreate, Bool is_command_request)
{
	MenuItem *mi,*tmp2;
	MenuRoot *tmp, *prev;
	Bool in_list = True;

	if (mr == NULL)
	{
		return False;
	}

	/* seek menu in master list */
	tmp = Menus.all;
	prev = NULL;
	while (tmp && tmp != mr)
	{
		prev = tmp;
		tmp = MR_NEXT_MENU(tmp);
	}
	if (tmp != mr)
	{
		/* no such menu */
		in_list = False;
	}

	if (MR_MAPPED_COPIES(mr) > 0 &&
	    (is_command_request || MR_COPIES(mr) == 1))
	{
		/* can't destroy a menu while in use */
		fvwm_msg(ERR, "DestroyMenu", "Menu %s is in use", MR_NAME(mr));
		return False;
	}

	if (in_list && MR_COPIES(mr) > 1 && MR_ORIGINAL_MENU(mr) == mr)
	{
		MenuRoot *m;
		MenuRoot *new_orig;

		/* find a new 'original' menu */
		for (m = Menus.all, new_orig = NULL; m; m = MR_NEXT_MENU(m))
		{
			if (m != mr && MR_ORIGINAL_MENU(m) == mr)
			{
				if (new_orig == NULL)
				{
					new_orig = m;
				}
				MR_ORIGINAL_MENU(m) = new_orig;
			}
		}
		MR_ORIGINAL_MENU(mr) = new_orig;
		/* now dump old original menu */
	}

	MR_COPIES(mr)--;
	if (MR_STORED_ITEM(mr).stored)
	{
		XFreePixmap(dpy, MR_STORED_ITEM(mr).stored);
	}
	if (MR_COPIES(mr) > 0)
	{
		do_recreate = False;
	}
	else
	{
		/* free all items */
		mi = MR_FIRST_ITEM(mr);
		while (mi != NULL)
		{
			tmp2 = MI_NEXT_ITEM(mi);
			menuitem_free(mi);
			mi = tmp2;
		}
		if (do_recreate)
		{
			/* just dump the menu items but keep the menu itself */
			MR_COPIES(mr)++;
			MR_FIRST_ITEM(mr) = NULL;
			MR_LAST_ITEM(mr) = NULL;
			MR_SELECTED_ITEM(mr) = NULL;
			MR_CONTINUATION_MENU(mr) = NULL;
			MR_PARENT_MENU(mr) = NULL;
			MR_ITEMS(mr) = 0;
			memset(&(MR_STORED_ITEM(mr)), 0 ,
			       sizeof(MR_STORED_ITEM(mr)));
			MR_IS_UPDATED(mr) = 1;
			return True;
		}
	}

	/* unlink menu from list */
	if (in_list)
	{
		if (prev == NULL)
		{
			Menus.all = MR_NEXT_MENU(mr);
		}
		else
		{
			MR_NEXT_MENU(prev) = MR_NEXT_MENU(mr);
		}
	}
	/* destroy the window and the display */
	if (MR_WINDOW(mr) != None)
	{
		XDeleteContext(dpy, MR_WINDOW(mr), MenuContext);
		XFlush(dpy);
		XDestroyWindow(MR_CREATE_DPY(mr), MR_WINDOW(mr));
		MR_WINDOW(mr) = None;
		XFlush(MR_CREATE_DPY(mr));
	}
	if (MR_CREATE_DPY(mr) != NULL && MR_CREATE_DPY(mr) != dpy)
	{
		XCloseDisplay(MR_CREATE_DPY(mr));
		MR_CREATE_DPY(mr) = NULL;
	}

	if (MR_COPIES(mr) == 0)
	{
		if (MR_POPUP_ACTION(mr))
		{
			free(MR_POPUP_ACTION(mr));
		}
		if (MR_POPDOWN_ACTION(mr))
		{
			free(MR_POPDOWN_ACTION(mr));
		}
		if (MR_MISSING_SUBMENU_FUNC(mr))
		{
			free(MR_MISSING_SUBMENU_FUNC(mr));
		}
		free(MR_NAME(mr));
		if (MR_SIDEPIC(mr))
		{
			PDestroyFvwmPicture(dpy, MR_SIDEPIC(mr));
		}
		memset(mr->s, 0, sizeof(*(mr->s)));
		free(mr->s);
	}
	memset(mr->d, 0, sizeof(*(mr->d)));
	free(mr->d);
	memset(mr, 0, sizeof(*mr));
	free(mr);

	return True;
}

/* FollowMenuContinuations
 * Given an menu root, return the menu root to add to by
 * following continuation links until there are no more
 */
MenuRoot *FollowMenuContinuations(MenuRoot *mr, MenuRoot **pmrPrior )
{
	*pmrPrior = NULL;
	while ((mr != NULL) &&
	       (MR_CONTINUATION_MENU(mr) != NULL))
	{
		*pmrPrior = mr;
		mr = MR_CONTINUATION_MENU(mr);
	}

	return mr;
}

/*
 *
 *  Procedure:
 *      AddToMenu - add an item to a root menu
 *
 *  Returned Value:
 *      (MenuItem *)
 *
 *  Inputs:
 *      menu    - pointer to the root menu to add the item
 *      item    - the text to appear in the menu
 *      action  - the string to possibly execute
 *
 * ckh - need to add boolean to say whether or not to expand for pixmaps,
 *       so built in window list can handle windows w/ * and % in title.
 *
 */
void AddToMenu(
	MenuRoot *mr, char *item, char *action, Bool fPixmapsOk, Bool fNoPlus,
	Bool is_continuation_item)
{
	MenuItem *tmp;
	char *start;
	char *end;
	char *token = NULL;
	char *option = NULL;
	int i;
	int is_empty;
	Bool do_replace_title;

	if (MR_MAPPED_COPIES(mr) > 0)
	{
		/* whoa, we can't handle *everything* */
		return;
	}
	if ((item == NULL || *item == 0) && (action == NULL || *action == 0) &&
	    fNoPlus)
	{
		return;
	}
	/* empty items screw up our menu when painted, so we replace them with
	 * a separator */
	if (item == NULL)
		item = "";

	/*
	 * Handle dynamic actions
	 */

	if (StrEquals(item, "DynamicPopupAction"))
	{
		if (MR_POPUP_ACTION(mr))
		{
			free(MR_POPUP_ACTION(mr));
		}
		if (!action || *action == 0)
		{
			MR_POPUP_ACTION(mr) = NULL;
		}
		else
		{
			MR_POPUP_ACTION(mr) = stripcpy(action);
		}
		return;
	}
	else if (StrEquals(item, "DynamicPopdownAction"))
	{
		if (MR_POPDOWN_ACTION(mr))
		{
			free(MR_POPDOWN_ACTION(mr));
		}
		if (!action || *action == 0)
		{
			MR_POPDOWN_ACTION(mr) = NULL;
		}
		else
		{
			MR_POPDOWN_ACTION(mr) = stripcpy(action);
		}
		return;
	}
	else if (StrEquals(item, "MissingSubmenuFunction"))
	{
		if (MR_MISSING_SUBMENU_FUNC(mr))
		{
			free(MR_MISSING_SUBMENU_FUNC(mr));
		}
		if (!action || *action == 0)
		{
			MR_MISSING_SUBMENU_FUNC(mr) = NULL;
		}
		else
		{
			MR_MISSING_SUBMENU_FUNC(mr) = stripcpy(action);
		}
		return;
	}

	/*
	 * Parse the action
	 */

	if (action == NULL || *action == 0)
	{
		action = "Nop";
	}
	GetNextToken(GetNextToken(action, &token), &option);
	tmp = menuitem_create();
	if (MR_FIRST_ITEM(mr) != NULL && StrEquals(token, "title") &&
	    option != NULL && StrEquals(option, "top"))
	{
		do_replace_title = True;
	}
	else
	{
		do_replace_title = False;
	}
	append_item_to_menu(mr, tmp, do_replace_title);
	if (token)
	{
		free(token);
	}
	if (option)
	{
		free(option);
	}

	MI_ACTION(tmp) = stripcpy(action);

	/*
	 * Parse the labels
	 */

	start = item;
	end = item;
	for (i = 0; i < MAX_MENU_ITEM_LABELS; i++, start = end)
	{
		/* Read label up to next tab. */
		if (*end)
		{
			if (i < MAX_MENU_ITEM_LABELS - 1)
			{
				while (*end && *end != '\t')
				{
					/* seek next tab or end of string */
					end++;
				}
			}
			else
			{
				/* remove all tabs in last label */
				while (*end)
				{
					if (*end == '\t')
					{
						*end = ' ';
					}
					end++;
				}
			}
			/* Copy the label. */
			/* TA:  FIXME!  Use asprintf(!) */
			MI_LABEL(tmp)[i] = xmalloc(end - start + 1);
			strncpy(MI_LABEL(tmp)[i], start, end - start);
			(MI_LABEL(tmp)[i])[end - start] = 0;
			if (*end == '\t')
			{
				/* skip the tab */
				end++;
			}
		}
		else
		{
			MI_LABEL(tmp)[i] = NULL;
		}

		/* Parse the label. */
		if (MI_LABEL(tmp)[i] != NULL)
		{
			if (fPixmapsOk)
			{
				string_def_t item_pixmaps[] = {
					{'*', __scan_for_pixmap},
					{'%', __scan_for_pixmap},
					{'\0', NULL}};
				string_context_t ctx;

				SCTX_SET_MI(ctx,tmp);
				scanForStrings(
					MI_LABEL(tmp)[i], item_pixmaps, &ctx);
			}
			if (!MI_HAS_HOTKEY(tmp))
			{
				/* pete@tecc.co.uk */
				scanForHotkeys(tmp, i);

				if (!MI_HAS_HOTKEY(tmp) &&
				    *MI_LABEL(tmp)[i] != 0)
				{
					MI_HOTKEY_COFFSET(tmp) = 0;
					MI_HOTKEY_COLUMN(tmp) = i;
					MI_HAS_HOTKEY(tmp) = 1;
					MI_IS_HOTKEY_AUTOMATIC(tmp) = 1;
				}
			}
			if (*(MI_LABEL(tmp)[i]))
			{
				MI_HAS_TEXT(tmp) = True;
			}
			else
			{
				free(MI_LABEL(tmp)[i]);
				MI_LABEL(tmp)[i] = NULL;
			}
		}
		MI_LABEL_STRLEN(tmp)[i] =
			(MI_LABEL(tmp)[i]) ? strlen(MI_LABEL(tmp)[i]) : 0;
	} /* for */

	/*
	 * Set the type flags
	 */

	if (is_continuation_item)
	{
		MI_IS_CONTINUATION(tmp) = True;
	}
	find_func_t(MI_ACTION(tmp), &(MI_FUNC_TYPE(tmp)), NULL);
	switch (MI_FUNC_TYPE(tmp))
	{
	case F_POPUP:
		MI_IS_POPUP(tmp) = True;
	case F_WINDOWLIST:
	case F_STAYSUP:
		MI_IS_MENU(tmp) = True;
		break;
	case F_TITLE:
		MI_IS_TITLE(tmp) = True;
		break;
	default:
		break;
	}
	is_empty = (!MI_HAS_TEXT(tmp) && !MI_HAS_PICTURE(tmp));
	if (is_empty)
	{
		if (MI_FUNC_TYPE(tmp) == F_TEARMENUOFF)
		{
			MI_IS_TEAR_OFF_BAR(tmp) = 1;
			MI_IS_SEPARATOR(tmp) = 0;
		}
		else
		{
			MI_IS_TEAR_OFF_BAR(tmp) = 0;
			MI_IS_SEPARATOR(tmp) = 1;
		}
	}
	if (MI_IS_SEPARATOR(tmp))
	{
		/* An empty title is handled like a separator. */
		MI_IS_TITLE(tmp) = False;
	}
	MI_IS_SELECTABLE(tmp) =
		((MI_HAS_TEXT(tmp) || MI_HAS_PICTURE(tmp) ||
		  MI_IS_TEAR_OFF_BAR(tmp)) && !MI_IS_TITLE(tmp));

	/*
	 * misc stuff
	 */

	MR_ITEMS(mr)++;
	MR_IS_UPDATED(mr) = 1;

	return;
}

/*
 *
 *  Procedure:
 *      NewMenuRoot - create a new menu root
 *
 *  Returned Value:
 *      (MenuRoot *)
 *
 *  Inputs:
 *      name    - the name of the menu root
 *
 */
MenuRoot *NewMenuRoot(char *name)
{
	MenuRoot *mr;
	string_def_t root_strings[] = {
		{'@', __scan_for_pixmap},
		{'^', __scan_for_color},
		{'\0', NULL}};
	string_context_t ctx;

	mr = xmalloc(sizeof *mr);
	mr->s = xcalloc(1, sizeof(MenuRootStatic));
	mr->d = xcalloc(1, sizeof(MenuRootDynamic));

	MR_NEXT_MENU(mr) = Menus.all;
	MR_NAME(mr) = xstrdup(name);
	MR_WINDOW(mr) = None;
	SCTX_SET_MR(ctx,mr);
	MR_HAS_SIDECOLOR(mr) = False;
	scanForStrings(
		MR_NAME(mr), root_strings, &ctx);
	MR_STYLE(mr) = menustyle_get_default_style();
	MR_ORIGINAL_MENU(mr) = mr;
	MR_COPIES(mr) = 1;
	MR_IS_UPDATED(mr) = 1;

	Menus.all = mr;
	return mr;
}

void SetMenuCursor(Cursor cursor)
{
	MenuRoot *mr;

	mr = Menus.all;
	for (mr = Menus.all; mr; mr = MR_NEXT_MENU(mr))
	{
		if (MR_WINDOW(mr))
		{
			XDefineCursor(dpy, MR_WINDOW(mr), cursor);
		}
	}

	return;
}

void UpdateAllMenuStyles(void)
{
	MenuStyle *ms;

	for (ms = menustyle_get_default_style(); ms; ms = ST_NEXT_STYLE(ms))
	{
		menustyle_update(ms);
	}

	return;
}

void UpdateMenuColorset(int cset)
{
	MenuStyle *ms;
	FvwmWindow *t;

	for (ms = menustyle_get_default_style(); ms; ms = ST_NEXT_STYLE(ms))
	{
		if ((ST_HAS_MENU_CSET(ms) && ST_CSET_MENU(ms) == cset) ||
		    (ST_HAS_ACTIVE_CSET(ms) && ST_CSET_ACTIVE(ms) == cset) ||
		    (ST_HAS_GREYED_CSET(ms) && ST_CSET_GREYED(ms) == cset) ||
		    (ST_HAS_TITLE_CSET(ms) && ST_CSET_TITLE(ms) == cset))
		{
			menustyle_update(ms);
		}
	}

	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		MenuRoot *mr = NULL;
		MenuStyle *ms = NULL;

		if (IS_TEAR_OFF_MENU(t) &&
		    XFindContext(dpy, FW_W(t), MenuContext, (caddr_t *)&mr) !=
		    XCNOENT &&
		    (ms = MR_STYLE(mr)))
		{
			if   (ST_HAS_MENU_CSET(ms) &&
			      ST_CSET_MENU(ms) == cset)
			{
				SetWindowBackground(
					dpy, MR_WINDOW(mr), MR_WIDTH(mr),
					MR_HEIGHT(mr),
					&Colorset[ST_CSET_MENU(ms)],
					Pdepth,
					FORE_GC(MST_MENU_INACTIVE_GCS(mr)),
					True);
			}
			else if ((ST_HAS_ACTIVE_CSET(ms) &&
				  ST_CSET_ACTIVE(ms) == cset) ||
				 (ST_HAS_GREYED_CSET(ms) &&
				  ST_CSET_GREYED(ms) == cset))
			{
				paint_menu(mr, NULL, NULL);
			}
		}
	}

	return;
}

/* This is called by the window list function */
void change_mr_menu_style(MenuRoot *mr, char *stylename)
{
	MenuStyle *ms = NULL;

	ms = menustyle_find(stylename);
	if (ms == NULL)
	{
		return;
	}
	if (MR_MAPPED_COPIES(mr) != 0)
	{
		fvwm_msg(ERR,"ChangeMenuStyle", "menu %s is in use",
			 MR_NAME(mr));
	}
	else
	{
		MR_STYLE(mr) = ms;
		MR_IS_UPDATED(mr) = 1;
	}

	return;
}

void add_another_menu_item(char *action)
{
	MenuRoot *mr;
	MenuRoot *mrPrior;
	char *rest,*item;

	mr = FollowMenuContinuations(Scr.last_added_item.item, &mrPrior);
	if (mr == NULL)
	{
		return;
	}
	if (MR_MAPPED_COPIES(mr) != 0)
	{
		fvwm_msg(ERR,"add_another_menu_item", "menu is in use");
		return;
	}
	rest = GetNextToken(action,&item);
	AddToMenu(mr, item, rest, True /* pixmap scan */, False, False);
	if (item)
	{
		free(item);
	}

	return;
}

/*
 * get_menu_options is used for Menu, Popup and WindowList
 * It parses strings matching
 *
 *   [ [context-rectangle] x y ] [special-options] [other arguments]
 *
 * and returns a pointer to the first part of the input string that doesn't
 * match this syntax.
 *
 * See documentation for a detailed description.
 */
char *get_menu_options(
	char *action, Window w, FvwmWindow *fw, XEvent *e, MenuRoot *mr,
	MenuItem *mi, MenuOptions *pops)
{
	char *tok = NULL;
	char *naction = action;
	char *taction;
	int x;
	int y;
	int button;
	int width;
	int height;
	int dummy_int;
	float dummy_float;
	Bool dummy_flag;
	Window context_window = None;
	Bool fHasContext, fUseItemOffset, fRectangleContext, fXineramaRoot;
	Bool fValidPosHints =
		last_saved_pos_hints.flags.is_last_menu_pos_hints_valid;
	Bool is_action_empty = False;
	Bool once_more = True;
	Bool is_icon_context;
	rectangle icon_g;

	/* If this is set we may want to reverse the position hints, so don't
	 * sum up the totals right now. This is useful for the SubmenusLeft
	 * style. */

	fXineramaRoot = False;
	last_saved_pos_hints.flags.is_last_menu_pos_hints_valid = False;
	if (pops == NULL)
	{
		fvwm_msg(ERR,
			 "get_menu_options","no MenuOptions pointer passed");
		return action;
	}

	taction = action;
	memset(&(pops->flags), 0, sizeof(pops->flags));
	pops->flags.has_poshints = 0;
	if (!action || *action == 0)
	{
		is_action_empty = True;
	}
	while (action != NULL && *action != 0 && once_more)
	{
		/* ^ just to be able to jump to end of loop without 'goto' */
		pops->pos_hints.is_relative = False;
		pops->pos_hints.menu_width = 0;
		pops->pos_hints.is_menu_relative = False;
		/* parse context argument (if present) */
		naction = GetNextToken(taction, &tok);
		if (!tok)
		{
			/* no context string */
			fHasContext = False;
			is_action_empty = True;
			break;
		}
		/* set to False for absolute hints! */
		pops->pos_hints.is_relative = True;
		fUseItemOffset = False;
		fHasContext = True;
		fRectangleContext = False;
		is_icon_context = False;
		if (StrEquals(tok, "context"))
		{
			if (mi && mr)
			{
				context_window = MR_WINDOW(mr);
			}
			else if (fw)
			{
				if (IS_ICONIFIED(fw))
				{
					is_icon_context = True;
					get_icon_geometry(fw, &icon_g);
					context_window = None;
				}
				else
				{
					context_window =
						FW_W_FRAME(fw);
				}
			}
			else
			{
				context_window = w;
			}
		}
		else if (StrEquals(tok,"menu"))
		{
			if (mr)
			{
				context_window = MR_WINDOW(mr);
				pops->pos_hints.is_menu_relative = True;
				pops->pos_hints.menu_width = MR_WIDTH(mr);
			}
		}
		else if (StrEquals(tok,"item"))
		{
			if (mi && mr)
			{
				context_window = MR_WINDOW(mr);
				fUseItemOffset = True;
				pops->pos_hints.is_menu_relative = True;
				pops->pos_hints.menu_width = MR_WIDTH(mr);
			}
		}
		else if (StrEquals(tok,"icon"))
		{
			if (fw && IS_ICONIFIED(fw))
			{
				is_icon_context = True;
				get_icon_geometry(fw, &icon_g);
				context_window = None;
			}
		}
		else if (StrEquals(tok,"window"))
		{
			if (fw && !IS_ICONIFIED(fw))
			{
				context_window = FW_W_FRAME(fw);
			}
		}
		else if (StrEquals(tok,"interior"))
		{
			if (fw && !IS_ICONIFIED(fw))
			{
				context_window = FW_W(fw);
			}
		}
		else if (StrEquals(tok,"title"))
		{
			if (fw)
			{
				if (IS_ICONIFIED(fw))
				{
					context_window =
						FW_W_ICON_TITLE(fw);
				}
				else
				{
					context_window = FW_W_TITLE(fw);
				}
			}
		}
		else if (strncasecmp(tok,"button",6) == 0)
		{
			if (sscanf(&(tok[6]),"%d",&button) != 1 ||
			    tok[6] == '+' || tok[6] == '-' || button < 0 ||
			    button > 9)
			{
				fHasContext = False;
			}
			else if (fw && !IS_ICONIFIED(fw))
			{
				button = BUTTON_INDEX(button);
				context_window = FW_W_BUTTON(fw, button);
			}
		}
		else if (StrEquals(tok,"root"))
		{
			context_window = Scr.Root;
			pops->pos_hints.is_relative = False;
		}
		else if (StrEquals(tok,"xineramaroot"))
		{
			context_window = Scr.Root;
			pops->pos_hints.is_relative = False;
			fXineramaRoot = True;
		}
		else if (StrEquals(tok,"mouse"))
		{
			context_window = None;
		}
		else if (StrEquals(tok,"rectangle"))
		{
			int flags;
			int screen;
			int sx;
			int sy;
			int sw;
			int sh;

			/* parse the rectangle */
			free(tok);
			naction = GetNextToken(naction, &tok);
			if (tok == NULL)
			{
				fvwm_msg(ERR, "get_menu_options",
					 "missing rectangle geometry");
				if (!pops->pos_hints.has_screen_origin)
				{
					/* xinerama: emergency fallback */
					pops->pos_hints.has_screen_origin = 1;
					pops->pos_hints.screen_origin_x = 0;
					pops->pos_hints.screen_origin_y = 0;
				}
				return action;
			}
			flags = FScreenParseGeometryWithScreen(
				tok, &x, &y,
				(unsigned int*)&width,
				(unsigned int*)&height, &screen);
			if ((flags & (XValue | YValue)) != (XValue | YValue))
			{
				free(tok);
				fvwm_msg(ERR, "get_menu_options",
					 "invalid rectangle geometry");
				if (!pops->pos_hints.has_screen_origin)
				{
					/* xinerama: emergency fallback */
					pops->pos_hints.has_screen_origin = 1;
					pops->pos_hints.screen_origin_x = 0;
					pops->pos_hints.screen_origin_y = 0;
				}
				return action;
			}
			pops->pos_hints.has_screen_origin = 1;
			FScreenGetScrRect(NULL, screen, &sx, &sy, &sw, &sh);
			pops->pos_hints.screen_origin_x = sx;
			pops->pos_hints.screen_origin_y = sy;
			if (!(flags & WidthValue))
			{
				width = 1;
			}
			if (!(flags & HeightValue))
			{
				height = 1;
			}
			x += sx;
			y += sy;
			if (flags & XNegative)
			{
				/* x is negative */
				x = sx + sw + x;
			}
			if (flags & YNegative)
			{
				/* y is negative */
				y = sy + sh + y;
			}
			pops->pos_hints.is_relative = False;
			fRectangleContext = True;
		}
		else if (StrEquals(tok,"this"))
		{
			MenuRoot *dummy_mr;

			context_window = w;
			if (XFindContext(
				    dpy, w, MenuContext,
				    (caddr_t *)&dummy_mr) != XCNOENT)
			{
				if (mr)
				{
					/* the parent menu */
					pops->pos_hints.is_menu_relative =
						True;
					pops->pos_hints.menu_width =
						MR_WIDTH(mr);
				}
			}
		}
		else
		{
			/* no context string */
			fHasContext = False;
		}

		if (tok)
		{
			free(tok);
		}
		if (fHasContext)
		{
			taction = naction;
		}
		else
		{
			naction = action;
		}

		if (fRectangleContext)
		{
			if (!pops->pos_hints.has_screen_origin)
			{
				/* xinerama: use global screen as reference */
				pops->pos_hints.has_screen_origin = 1;
				pops->pos_hints.screen_origin_x = -1;
				pops->pos_hints.screen_origin_y = -1;
			}
			/* nothing else to do */
		}
		else if (!is_icon_context &&
			 (!fHasContext || !context_window ||
			  !XGetGeometry(
				  dpy, context_window, &JunkRoot, &JunkX,
				  &JunkY, (unsigned int*)&width,
				  (unsigned int*)&height,
				  (unsigned int*)&JunkBW,
				  (unsigned int*)&JunkDepth) ||
			  !XTranslateCoordinates(
				  dpy, context_window, Scr.Root, 0, 0, &x, &y,
				  &JunkChild)))
		{
			/* no window or could not get geometry */
			if (FQueryPointer(
				    dpy,Scr.Root, &JunkRoot, &JunkChild, &x,
				    &y, &JunkX, &JunkY, &JunkMask) == False)
			{
				/* pointer is on a different screen - that's
				 * okay here */
				x = 0;
				y = 0;
			}
			width = height = 1;
			if (!pops->pos_hints.has_screen_origin)
			{
				/* xinerama: use screen with pinter as
				 * reference */
				pops->pos_hints.has_screen_origin = 1;
				pops->pos_hints.screen_origin_x = x;
				pops->pos_hints.screen_origin_y = y;
			}
		}
		else
		{
			if (is_icon_context)
			{
				x = icon_g.x;
				y = icon_g.y;
				width = icon_g.width;
				height = icon_g.height;
			}
			/* we have a context window */
			if (fUseItemOffset)
			{
				y += MI_Y_OFFSET(mi);
				height = MI_HEIGHT(mi);
			}
			if (!pops->pos_hints.has_screen_origin)
			{
				pops->pos_hints.has_screen_origin = 1;
				if (fXineramaRoot)
				{
					/* use whole screen */
					pops->pos_hints.screen_origin_x = -1;
					pops->pos_hints.screen_origin_y = -1;
				}
				else if (context_window == Scr.Root)
				{
					/* xinerama: use screen that contains
					 * the window center as reference */
					if (!fev_get_evpos_or_query(
						    dpy, context_window, e,
						    &pops->pos_hints.
						    screen_origin_x,
						    &pops->pos_hints.
						    screen_origin_y))
					{
						pops->pos_hints.
							screen_origin_x = 0;
						pops->pos_hints.
							screen_origin_y = 0;
					}
					else
					{
						fscreen_scr_arg fscr;

						fscr.xypos.x = pops->pos_hints.
							screen_origin_x;
						fscr.xypos.y = pops->pos_hints.
							screen_origin_y;
						FScreenGetScrRect(
							&fscr, FSCREEN_XYPOS,
							&x, &y, &width,
							&height);
					}
				}
				else
				{
					/* xinerama: use screen that contains
					 * the window center as reference */
					pops->pos_hints.screen_origin_x =
						JunkX + width / 2;
					pops->pos_hints.screen_origin_y =
						JunkY + height / 2;
				}
			}
		}

		/* parse position arguments */
		taction = get_one_menu_position_argument(
			naction, x, width, &(pops->pos_hints.x),
			&(pops->pos_hints.x_offset),
			&(pops->pos_hints.x_factor),
			&(pops->pos_hints.context_x_factor),
			&pops->pos_hints.is_menu_relative);
		if (pops->pos_hints.is_menu_relative)
		{
			pops->pos_hints.x = x;
			if (pops->pos_hints.menu_width == 0 && mr)
			{
				pops->pos_hints.menu_width = MR_WIDTH(mr);
			}
		}
		naction = get_one_menu_position_argument(
			taction, y, height, &(pops->pos_hints.y), &dummy_int,
			&(pops->pos_hints.y_factor), &dummy_float,
			&dummy_flag);
		if (naction == taction)
		{
			/* argument is missing or invalid */
			if (fHasContext)
			{
				fvwm_msg(ERR, "get_menu_options",
					 "invalid position arguments");
			}
			naction = action;
			taction = action;
			break;
		}
		taction = naction;
		pops->flags.has_poshints = 1;
		if (fValidPosHints == True &&
		    pops->pos_hints.is_relative == True)
		{
			pops->pos_hints = last_saved_pos_hints.pos_hints;
		}
		/* we want to do this only once */
		once_more = False;
	} /* while */
	if (is_action_empty)
	{
		if (!pops->pos_hints.has_screen_origin)
		{
			pops->pos_hints.has_screen_origin = 1;
			if (!fev_get_evpos_or_query(
				    dpy, Scr.Root, e,
				    &pops->pos_hints.screen_origin_x,
				    &pops->pos_hints.screen_origin_y))
			{
				pops->pos_hints.screen_origin_x = 0;
				pops->pos_hints.screen_origin_y = 0;
			}
		}
	}

	if (!pops->flags.has_poshints && fValidPosHints)
	{
		pops->flags.has_poshints = 1;
		pops->pos_hints = last_saved_pos_hints.pos_hints;
		pops->pos_hints.is_relative = False;
	}

	action = naction;
	/* to keep Purify silent */
	pops->flags.do_select_in_place = 0;
	/* parse additional options */
	while (naction && *naction)
	{
		naction = GetNextToken(action, &tok);
		if (!tok)
		{
			break;
		}
		if (StrEquals(tok, "WarpTitle"))
		{
			pops->flags.do_warp_title = 1;
			pops->flags.do_not_warp = 0;
		}
		else if (StrEquals(tok, "NoWarp"))
		{
			pops->flags.do_warp_title = 0;
			pops->flags.do_not_warp = 1;
		}
		else if (StrEquals(tok, "Fixed"))
		{
			pops->flags.is_fixed = 1;
		}
		else if (StrEquals(tok, "SelectInPlace"))
		{
			pops->flags.do_select_in_place = 1;
		}
		else if (StrEquals(tok, "SelectWarp"))
		{
			pops->flags.do_warp_on_select = 1;
		}
		else if (StrEquals(tok, "TearOffImmediately"))
		{
			pops->flags.do_tear_off_immediately = 1;
		}
		else
		{
			free (tok);
			break;
		}
		action = naction;
		free (tok);
	}
	if (!pops->flags.do_select_in_place)
	{
		pops->flags.do_warp_on_select = 0;
	}

	return action;
}

/* ---------------------------- new menu loop code ------------------------- */

#if 0
/*!!!*/
typedef enum
{
	MTR_XEVENT = 0x1,
	MTR_FAKE_ENTER_ITEM = 0x2,
	MTR_PROPAGATE_XEVENT_UP = 0x4,
	MTR_PROPAGATE_XEVENT_DOWN = 0x8,
	MTR_POPUP_TIMEOUT = 0x10,
	MTR_POPDOWN_TIMEOUT = 0x20
} mloop_trigger_type_t;

typedef_struct
{
	mloop_trigger_type_t type;
	XEvent trigger_ev;
	int ticks_passed;
} mloop_trigger_t;

typedef_struct
{
	int popup_10ms;
	int popdown_10ms;
} mloop_timeouts_t;

typedef struct
{
	MenuRoot *current_menu;
	XEvent *in_ev;
	struct
	{
		unsigned do_fake_enter_item : 1;
		unsigned do_propagete_event_up : 1;
		unsigned do_propagete_event_down : 1;
	} flags;
} mloop_get_trigger_input_t;

/*!!!static*/
mloop_trigger_type_t __mloop_get_trigger(
	mloop_trigger_t *ret_trigger, mloop_timeouts_t *io_timeouts,
	const mloop_get_trigger_input_t * const in,
	{
		if (in_out->in_flags->do_propagate_event_down)
		{
			return MTR_PROPAGATE_XEVENT_DOWN;
		}
		else if (in_out->in_flags->do_propagate_event_up)
		{
			if (a != b)
			{
				return MTR_PROPAGATE_XEVENT_UP;
			}
			else
			{
			}
			/*!!!return propagate up*/
			/*!!!*/
		}
		/*!!!read event or wait for timeout*/
		while (0/*!!!not finished*/)
		{
			/*!!!rc = 0*/
			if (0/*!!!wait for tiomeout*/)
			{
				/*!!!check for event*/
			}
			else
			{
				/*!!!block for event*/
			}
			if (0/*got event*/)
			{
				/*!!!rc = MTR_XEVENT*/
			}
			if (0/*!!!popup timed out;break*/)
			{
				/*!!!rc = MTR_POPUP;break*/
			}
			if (0/*!!!popdown timed out;break*/)
			{
				/*!!!rc = MTR_POPDOWN;break*/
			}
			/*!!!sleep*/
		}
		if (0/*!!!rc == MTR_XEVENT && evtype == MotionNotify*/)
		{
			/*!!!eat up further MotionNotify events*/
		}

		return 0/*!!!rc*/;
	}

	/*!!!static*/
	void __menu_loop_new(
		MenuParameters *pmp, MenuReturn *pmret, double_keypress *pdkp)
	{
		mloop_evh_input_t mei;
#if 0
		mloop_ret_code_t mloop_ret;
#endif
		mloop_evh_data_t med;
		mloop_static_info_t msi;
		MenuOptions mops;
		Bool is_finished;

		/*!!!init menu loop*/
		__mloop_init(pmp, pmret, &mei, &med, &msi, &mops);
		for (is_finished = False; !is_finished; )
		{
			mloop_trigger_type_t mtr;

			mtr = __mloop_get_trigger(
				pmp, pmret, &mei, &med, &msi);
			switch (mtr)
			{
			case MTR_XEVENT:
				/*!!!handle event*/
				break;
			case MTR_FAKE_ENTER_ITEM:
				/*!!!fake enter item*/
				break;
			case MTR_PROPAGATE_XEVENT_UP:
				/*!!!handle propagation*/
				break;
			case MTR_PROPAGATE_XEVENT_DOWN:
				/*!!!handle propagation*/
				break;
			case MTR_POPUP_TIMEOUT:
				/*!!!handle popup*/
				break;
			case MTR_POPDOWN_TIMEOUT:
				/*!!!handle popdown*/
				break;
			}





#if 0
			mloop_ret = __mloop_handle_event(
				pmp, pmret, pdkp, &mei, &med, &msi);
			switch (mloop_ret)
			{
			case MENU_MLOOP_RET_LOOP:
				continue;
			case MENU_MLOOP_RET_END:
				is_finished = True;
				break;
			default:
				break;
			}
			/* Now handle new menu items, whether it is from a
			 * keypress or a pointer motion event. */
			if (med.mi != NULL)
			{
				mloop_ret = __mloop_handle_action_with_mi(
					pmp, pmret, pdkp, &mei, &med, &msi,
					&mops);
			}
			else
			{
				mloop_ret = __mloop_handle_action_without_mi(
					pmp, pmret, pdkp, &mei, &med, &msi,
					&mops);
			}
			if (mloop_ret == MENU_MLOOP_RET_END)
			{
				is_finished = True;
			}
			XFlush(dpy);
#endif
		}
		__mloop_exit(pmp, pmret, pdkp, &mei, &med, &msi, &mops);

		return;
	}
#endif
