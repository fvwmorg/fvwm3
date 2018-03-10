/* -*-c-*- */
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

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#include <X11/keysym.h>

#include "libs/Parse.h"
#include "libs/Bindings.h"
#include "libs/charmap.h"
#include "libs/Flocale.h"
#include "libs/wcontext.h"
#include "fvwm.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "misc.h"
#include "screen.h"
#include "bindings.h"

#include "move_resize.h"
#include "menudim.h"
#include "menustyle.h"
#include "menuitem.h"
#include "menuroot.h"
#include "menuparameters.h"
#include "menugeometry.h"
#include "menubindings.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* menu bindings are kept in a local binding list separate from other
 * bindings*/
static Binding *menu_bindings_regular = NULL;
static Binding *menu_bindings_fallback = NULL;
static Binding **menu_bindings = NULL;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/*
 * Returns the position of the item in the menu, but counts
 * only items that can be selected (i.e. nor separators or
 * titles). The count begins with 0.
 */
static int get_selectable_item_index(
	MenuRoot *mr, MenuItem *mi_target, int *ret_sections)
{
	int i = 0;
	int s = 0;
	MenuItem *mi;
	int is_last_selectable = 0;

	for (mi = MR_FIRST_ITEM(mr); mi && mi != mi_target;
	     mi = MI_NEXT_ITEM(mi))
	{
		if (MI_IS_SELECTABLE(mi))
		{
			i++;
			is_last_selectable = 1;
		}
		else if (is_last_selectable == 1)
		{
			s++;
			is_last_selectable = 0;
		}
	}
	if (ret_sections)
	{
		*ret_sections = s;
	}
	if (mi == mi_target)
	{
		return i;
	}

	return -1;
}

static MenuItem *get_selectable_item_from_index(MenuRoot *mr, int index)
{
	int i = -1;
	MenuItem *mi;
	MenuItem *mi_last_ok = NULL;

	for (mi = MR_FIRST_ITEM(mr); mi && (i < index || mi_last_ok == NULL);
	     mi=MI_NEXT_ITEM(mi))
	{
		if (MI_IS_SELECTABLE(mi))
		{
			mi_last_ok = mi;
			i++;
		}
	}

	return mi_last_ok;
}

static MenuItem *get_selectable_item_from_section(MenuRoot *mr, int section)
{
	int i = 0;
	MenuItem *mi;
	MenuItem *mi_last_ok = NULL;
	int is_last_selectable = 0;

	for (
		mi = MR_FIRST_ITEM(mr);
		mi && (i <= section || mi_last_ok == NULL);
		mi=MI_NEXT_ITEM(mi))
	{
		if (MI_IS_SELECTABLE(mi))
		{
			if (!is_last_selectable)
			{
				mi_last_ok = mi;
				is_last_selectable = 1;
			}
		}
		else if (is_last_selectable)
		{
			i++;
			is_last_selectable = 0;
		}
	}

	return mi_last_ok;
}

static int get_selectable_item_count(MenuRoot *mr, int *ret_sections)
{
	int count;

	count = get_selectable_item_index(mr, MR_LAST_ITEM(mr), ret_sections);
	if (MR_LAST_ITEM(mr) && MI_IS_SELECTABLE(MR_LAST_ITEM(mr)))
	{
		count++;
	}

	return count;
}

static Binding *__menu_binding_is_mouse(
	Binding *blist, XEvent* event, int context)
{
	Binding *b;
	int real_mod;

	real_mod = event->xbutton.state - (1 << (7 + event->xbutton.button));
	for (b = blist; b != NULL; b = b->NextBinding)
	{
		if (
			BIND_IS_MOUSE_BINDING(b->type) &&
			(b->Button_Key == 0 ||
			 event->xbutton.button == b->Button_Key) &&
			(b->Modifier == AnyModifier ||
			 MaskUsedModifiers(b->Modifier) ==
			 MaskUsedModifiers(real_mod)) &&
			(b->Context == C_MENU ||
			 (b->Context & context) != C_MENU))
		{
			break;
		}
	}

	return b;
}

static void parse_menu_action(
	struct MenuRoot *mr, const char *action, menu_shortcut_action *saction,
	int *items_to_move, int *do_skip_section, char **ret_cmd)
{
	char *optlist[] = {
		"MenuClose",
		"MenuEnterContinuation",
		"MenuEnterSubmenu",
		"MenuLeaveSubmenu",
		"MenuMoveCursor",
		"MenuCursorLeft",
		"MenuCursorRight",
		"MenuSelectItem",
		"MenuScroll",
		"MenuTearOff",
		"MenuCloseAndExec",
		NULL
	};
	int index;
	char *options;
	int num;
	int suffix[2];
	int count[2];

	*ret_cmd = NULL;
	options = GetNextTokenIndex((char *)action, optlist, 0, &index);
	switch (index)
	{
	case 0: /* MenuClose */
		*saction = SA_ABORT;
		break;
	case 1: /* MenuEnterContinuation */
		*saction = (MR_CONTINUATION_MENU(mr) != NULL) ?
			SA_CONTINUE : SA_ENTER;
		break;
	case 2: /* MenuEnterSubmenu */
		*saction = SA_ENTER;
		break;
	case 3: /* MenuLeaveSubmenu */
		*saction = SA_LEAVE;
		break;
	case 4: /* MenuMoveCursor */
		num = GetSuffixedIntegerArguments(options, NULL, count, 2,
						  "s", suffix);
		if (num == 2)
		{
			if (suffix[0] != 0 || count[0] != 0)
			{
				fvwm_msg(ERR, "parse_menu_action",
					 "invalid MenuMoveCursor arguments "
					 "'%s'", options);
				*saction = SA_NONE;
				break;
			}
			if (count[1] < 0)
			{
				*saction = SA_LAST;
				*items_to_move = 1 + count[1];
			}
			else
			{
				*saction = SA_FIRST;
				*items_to_move = count[1];
			}

			if (suffix[1] == 1)
			{
				*do_skip_section = 1;
			}
		}
		else if (num == 1)
		{
			*saction = SA_MOVE_ITEMS;
			*items_to_move = count[0];
			if (suffix[0] == 1)
			{
				*do_skip_section = 1;
			}
		}
		else
		{
			fvwm_msg(ERR, "parse_menu_action",
				 "invalid MenuMoveCursor arguments '%s'",
				 options);
			*saction = SA_NONE;
			break;
		}
		break;
	case 5: /* MenuCursorLeft */
		*saction = (MST_USE_LEFT_SUBMENUS(mr)) ? SA_ENTER : SA_LEAVE;
		break;
	case 6: /* MenuCursorRight */
		*saction = (MST_USE_LEFT_SUBMENUS(mr)) ? SA_LEAVE : SA_ENTER;
		break;
	case 7: /* MenuSelectItem */
		*saction = SA_SELECT;
		break;
	case 8: /* MenuScroll */
		if (MST_MOUSE_WHEEL(mr) == MMW_OFF)
		{
			*saction = SA_SELECT;
		}
		else
		{
			num = GetSuffixedIntegerArguments(options, NULL, count,
							  1, "s", suffix);
			if (num == 1)
			{
				*saction = SA_SCROLL;
				*items_to_move = count[0];
				if (suffix[0] == 1)
				{
					*do_skip_section = 1;
				}
			}
			else
			{
				fvwm_msg(ERR, "parse_menu_action",
					 "invalid MenuScroll arguments '%s'",
					 options);
				*saction = SA_NONE;
				break;
			}
		}
		break;
	case 9: /* MenuTearOff */
		*saction = SA_TEAROFF;
		break;
	case 10: /* MenuCloseAndExecute */
		*saction = SA_EXEC_CMD;
		*ret_cmd = options;
		break;
	default:
		fvwm_msg(
			ERR, "parse_menu_action", "unknown action '%s'",
			action);
		*saction = SA_NONE;
	}

	return;
}

static Binding *__menu_binding_is_key(
	Binding *blist, XEvent* event, int context)
{
	Binding *b;

	for (b = blist; b != NULL; b = b->NextBinding)
	{
		if (
			BIND_IS_KEY_BINDING(b->type) &&
			event->xkey.keycode == b->Button_Key &&
			(b->Modifier == AnyModifier ||
			 MaskUsedModifiers(b->Modifier) ==
			 MaskUsedModifiers(event->xkey.state)) &&
			(b->Context == C_MENU ||
			 (b->Context & context) != C_MENU))
		{
			break;
		}
	}

	return b;
}

/* ---------------------------- interface functions ------------------------ */

void menu_bindings_startup_complete(void)
{
	menu_bindings = &menu_bindings_regular;

	return;
}

Binding *menu_binding_is_mouse(XEvent* event, int context)
{
	Binding *b;

	b = __menu_binding_is_mouse(menu_bindings_regular, event, context);
	if (b == NULL)
	{
		b = __menu_binding_is_mouse(
			menu_bindings_fallback, event, context);
	}

	return b;
}

Binding *menu_binding_is_key(XEvent* event, int context)
{
	Binding *b;

	b = __menu_binding_is_key(menu_bindings_regular, event, context);
	if (b == NULL)
	{
		b = __menu_binding_is_key(
			menu_bindings_fallback, event, context);
	}

	return b;
}

int menu_binding(
	Display *dpy, binding_t type, int button, KeySym keysym,
	int context, int modifier, char *action, char *menu_style)
{
	Binding *rmlist;
	int rc;
	Bool dummy;

	if (menu_bindings == NULL)
	{
		menu_bindings = &menu_bindings_fallback;
	}
	rmlist = NULL;
	if (~(~context | C_MENU | C_TITLE | C_MENU_ITEM | C_SIDEBAR) != 0)
	{
		fvwm_msg(
			ERR, "menu_binding",
			"invalid context in combination with menu context.");

		return 1;
	}
	if (menu_style != NULL)
	{
		/*!!! fixme - make either match a menu style or a menu name */
		fvwm_msg(
			ERR, "menu_binding", "a window name may not be"
			" specified without a menu context.");

		return 1;
	}
	/*
	 * Remove the "old" bindings if any
	 */
	/* BEGIN remove */
	CollectBindingList(
		dpy, menu_bindings, &rmlist, &dummy, type, STROKE_ARG(NULL)
		button, keysym, modifier, context, menu_style);
	if (rmlist != NULL)
	{
		FreeBindingList(rmlist);
	}
	else if (
		keysym == 0 && button != 0 && modifier == 0 &&
		strcmp(action,"-") == 0 && context == C_MENU)
	{
		/* Warn if Mouse n M N - occurs without removing any binding.
		 The user most likely want Mouse n MT A - instead. */
		fvwm_msg(
			WARN, "menu_binding",
			"The syntax for disabling the tear off button has "
			"changed.");
	}
	if (strcmp(action,"-") == 0)
	{
		return 0;
	}
	/* END remove */
	if ((modifier & AnyModifier) && (modifier & (~AnyModifier)))
	{
		fvwm_msg(
			WARN, "menu_binding", "Binding specified AnyModifier"
			" and other modifers too. Excess modifiers are"
			" ignored.");
		modifier = AnyModifier;
	}
	/* Warn about Mouse n M N TearOff. */
	if (
		keysym == 0 && button != 0 && modifier == 0 &&
		strcasecmp(action,"tearoff") == 0 && context == C_MENU)
	{
		fvwm_msg(OLD, "menu_binding",
			 "The syntax for disabling the tear off button has "
			 "changed. The TearOff action is no longer possible "
			 "in menu bindings.");
	}
	rc = AddBinding(
		dpy, menu_bindings, type, STROKE_ARG(NULL) button, keysym,
		NULL, modifier, context, (void *)action, NULL, menu_style);

	return rc;
}

void menu_shortcuts(
	struct MenuRoot *mr, struct MenuParameters *pmp,
	struct MenuReturn *pmret, XEvent *event, struct MenuItem **pmi_current,
	double_keypress *pdkp, int *ret_menu_x, int *ret_menu_y)
{
	int with_control;
	int with_shift;
	int with_meta;
	KeySym keysym;
	char ckeychar;
	int ikeychar;
	MenuItem *new_item;
	MenuItem *mi_current;
	int index;
	int mx;
	int my;
	int menu_x;
	int menu_y;
	int menu_width;
	int menu_height;
	int items_to_move;
	int do_skip_section;
	menu_shortcut_action saction;
	Binding *binding;
	int context;
	int is_geometry_known;
	char *command;

	ckeychar = 0;
	new_item = NULL;
	mi_current = pmi_current ? *pmi_current : NULL;
	do_skip_section = 0;
	saction = SA_NONE;
	context = C_MENU;
	is_geometry_known = 0;
	context = C_MENU;
	command = 0;
	if (mi_current)
	{
		if (MI_IS_TITLE(mi_current))
		{
			context |= C_TITLE;
		}
		else
		{
			/* menu item context, use I (icon) for it */
			context |= C_MENU_ITEM;
		}
	}
	else
	{
		if (
			menu_get_geometry(
				mr, &JunkRoot, &menu_x, &menu_y, &menu_width,
				&menu_height, &JunkBW, &JunkDepth))
		{
			is_geometry_known = 1;
			if (
				FQueryPointer(
					dpy, Scr.Root, &JunkRoot, &JunkChild,
					&mx, &my, &JunkX, &JunkY, &JunkMask) ==
				0)
			{
				/* pointer is on a different screen */
				mx = 0;
				my = 0;
			}
			else if (
				mx >= menu_x && mx < menu_x + menu_width &&
				my >= menu_y && my < menu_y + menu_height)
			{
				/* pointer is on the meny somewhere not over
				 * an item */
				if (my < menu_y + MST_BORDER_WIDTH(mr))
				{
					/* upper border context (-)*/
					context |= C_SB_TOP;
				}
				else if (
					my >= menu_y + menu_height -
					MST_BORDER_WIDTH(mr))
				{
					/* lower border context (_) */
					context |= C_SB_BOTTOM;
				}
				else if (mx < menu_x + MR_ITEM_X_OFFSET(mr))
				{
					/* left border or left sidepic */
					context |= C_SB_LEFT;
				}
				else if (mx < menu_x + MST_BORDER_WIDTH(mr))
				{
					/* left border context ([)*/
					context |= C_SB_LEFT;
				}
				else if (
					mx >= menu_x + MR_ITEM_X_OFFSET(mr) +
					MR_ITEM_WIDTH(mr))
				{
					/* right sidepic or right border */
					context |= C_SB_RIGHT;
				}
				else if (
					mx >= menu_x + menu_width -
					MST_BORDER_WIDTH(mr))
				{
					/* right border context (])*/
					context |= C_SB_RIGHT;
				}
			}
		}
		else
		{
			fvwm_msg(
				ERR, "menu_shortcuts", "can't get geometry of"
				" menu %s", MR_NAME(mr));
		}
	}

	if (event->type == KeyRelease)
	{
		/* This function is only called with a KeyRelease event if the
		 * user released the 'select' key (s)he configured. */
		pmret->rc = MENU_SELECTED;

		return;
	}

	items_to_move = 0;
	pmret->rc = MENU_NOP;

	/*** handle mouse events ***/
	if (event->type == ButtonRelease)
	{
	        /*** Read the control keys stats ***/
		with_control = event->xbutton.state & ControlMask ? 1 : 0;
		with_shift = event->xbutton.state & ShiftMask ? 1: 0;
		with_meta = event->xbutton.state & Mod1Mask ? 1: 0;
		/** handle menu bindings **/
		binding = menu_binding_is_mouse(event, context);
		if (binding != NULL)
		{
			parse_menu_action(
				mr, binding->Action, &saction, &items_to_move,
				&do_skip_section, &command);
		}
		index = 0;
		ikeychar = 0;
	}
	else /* Should be KeyPressed */
	{
	        /*** Read the control keys stats ***/
		with_control = event->xkey.state & ControlMask? 1 : 0;
		with_shift = event->xkey.state & ShiftMask? 1: 0;
		with_meta = event->xkey.state & Mod1Mask? 1: 0;

		/*** handle double-keypress ***/

		if (pdkp->timestamp &&
		    fev_get_evtime() - pdkp->timestamp <
		    MST_DOUBLE_CLICK_TIME(pmp->menu) &&
		    event->xkey.state == pdkp->keystate &&
		    event->xkey.keycode == pdkp->keycode)
		{
			*pmi_current = NULL;
			pmret->rc = MENU_DOUBLE_CLICKED;

			return;
		}
		pdkp->timestamp = 0;

		/*** find out the key ***/

		/* Is it okay to treat keysym-s as Ascii?
		 * No, because the keypad numbers don't work.
		 * Use XlookupString */
		index = XLookupString(&(event->xkey), &ckeychar, 1, &keysym,
				      NULL);
		ikeychar = (int)ckeychar;
	}
	/*** Try to match hot keys ***/

	/* Need isascii here - isgraph might coredump! */
	if (index == 1 && isascii(ikeychar) && isgraph(ikeychar) &&
	    with_control == 0 && with_meta == 0)
	{
		/* allow any printable character to be a keysym, but be sure
		 * control isn't pressed */
		MenuItem *mi;
		MenuItem *mi1;
		int key;
		int countHotkey = 0;

		/* if this is a letter set it to lower case */
		if (isupper(ikeychar))
		{
			ikeychar = tolower(ikeychar) ;
		}

		/* MMH mikehan@best.com 2/7/99
		 * Multiple hotkeys per menu
		 * Search menu for matching hotkey;
		 * remember how many we found and where we found it */
		mi = (mi_current == NULL || mi_current == MR_LAST_ITEM(mr)) ?
			MR_FIRST_ITEM(mr) : MI_NEXT_ITEM(mi_current);
		mi1 = mi;
		do
		{
			if (MI_HAS_HOTKEY(mi) && !MI_IS_TITLE(mi) &&
			    (!MI_IS_HOTKEY_AUTOMATIC(mi) ||
			     MST_USE_AUTOMATIC_HOTKEYS(mr)))
			{
				key = (MI_LABEL(mi)[(int)MI_HOTKEY_COLUMN(mi)])
					[MI_HOTKEY_COFFSET(mi)];
				key = tolower(key);
				if (ikeychar == key)
				{
					if (++countHotkey == 1)
					{
						new_item = mi;
					}
				}
			}
			mi = (mi == MR_LAST_ITEM(mr)) ?
				MR_FIRST_ITEM(mr) : MI_NEXT_ITEM(mi);
		}
		while (mi != mi1);

		/* For multiple instances of a single hotkey, just move the
		 * selection */
		/* TA:  2011-07-24:  But if the user has turned off
		 * "UniqueHotkeyActivatedImmediate", keep the menu open until
		 * the user has asked for that entry to be enacted.  This also
		 * implies the style "TitleWarpOff" and we're not over a popup
		 * item, in which case the pointer is warped to the submenu in
		 * the usual way.
		 */
		if ((countHotkey > 1) || (countHotkey >=1 && (
					  (!MST_DO_WARP_TO_TITLE(mr) ||
					   !MI_IS_POPUP(new_item))
					  ) &&
					  !MST_HOTKEY_ACTIVATES_IMMEDIATE(mr)))
		{
			*pmi_current = new_item;
			pmret->rc = MENU_NEWITEM;

			return;
		}
		/* Do things the old way for unique hotkeys in the menu */
		else if (countHotkey == 1)
		{
			*pmi_current = new_item;
			if (new_item && MI_IS_POPUP(new_item))
			{
				pmret->rc = MENU_POPUP;
			}
			else
			{
				pmret->rc = MENU_SELECTED;
			}

			return;
		}
		/* MMH mikehan@best.com 2/7/99 */
	}

	/*** now determine the action to take ***/

	/** handle menu key bindings **/
	if (
		event->type == KeyPress && keysym == XK_Escape &&
		with_control == 0 && with_shift == 0 &&
		with_meta == 0)
	{
		/* Don't allow override of Escape with no modifiers */
		saction = SA_ABORT;

	}
	else if (event->type == KeyPress)
	{
		binding = menu_binding_is_key(event, context);
		if (binding != NULL)
		{
			parse_menu_action(
				mr, binding->Action, &saction, &items_to_move,
				&do_skip_section, &command);
		}
	}

	if (
		!mi_current &&
		(saction == SA_ENTER || saction == SA_MOVE_ITEMS ||
		 saction == SA_SELECT || saction == SA_SCROLL))
	{
		if (is_geometry_known)
		{
			if (my < menu_y + MST_BORDER_WIDTH(mr))
			{
				saction = SA_FIRST;
			}
			else if (my > menu_y + menu_height -
				 MST_BORDER_WIDTH(mr))
			{
				saction = SA_LAST;
			}
			else
			{
				saction = SA_WARPBACK;
			}
		}
		else
		{
			saction = SA_FIRST;
		}
	}

	/*** execute the necessary actions ***/

	switch (saction)
	{
	case SA_ENTER:
		if (mi_current && MI_IS_POPUP(mi_current))
		{
			pmret->rc = MENU_POPUP;
		}
		else
		{
			pmret->rc = MENU_NOP;
		}
		break;

	case SA_LEAVE:
		pmret->rc =
			(MR_IS_TEAR_OFF_MENU(mr)) ? MENU_NOP : MENU_POPDOWN;
		break;
	case SA_FIRST:
		if (do_skip_section)
		{
			*pmi_current = get_selectable_item_from_section(
				mr, items_to_move);
		}
		else
		{
			*pmi_current = get_selectable_item_from_index(
				mr, items_to_move);
		}
		if (*pmi_current != NULL)
		{
			pmret->rc = MENU_NEWITEM;
		}
		else
		{
			pmret->rc = MENU_NOP;
		}
		break;
	case SA_LAST:
		if (do_skip_section)
		{
			get_selectable_item_count(mr, &index);
			index += items_to_move;
			if (index < 0)
			{
				index = 0;
			}
			*pmi_current = get_selectable_item_from_section(
				mr, index);
			if (*pmi_current != NULL)
			{
				pmret->rc = MENU_NEWITEM;
			}
			else
			{
				pmret->rc = MENU_NOP;
			}
		}
		else
		{
			index = get_selectable_item_count(mr, NULL);
			if (index > 0)
			{
				index += items_to_move;
				if (index < 0)
				{
					index = 0;
				}
				*pmi_current = get_selectable_item_from_index(
					mr, index);
				pmret->rc = (*pmi_current) ?
					MENU_NEWITEM : MENU_NOP;
			}
			else
			{
				pmret->rc = MENU_NOP;
			}
		}
		break;
	case SA_MOVE_ITEMS:
		if (do_skip_section)
		{
			int section;
			int count;

			get_selectable_item_count(mr, &count);
			get_selectable_item_index(mr, mi_current, &section);
			section += items_to_move;
			if (section < 0)
				section = count;
			else if (section > count)
				section = 0;
			index = section;
		}
		else if (items_to_move < 0)
		{
			index = get_selectable_item_index(
				mr, mi_current, NULL);
			if (index == 0)
				/* wraparound */
				index = get_selectable_item_count(mr, NULL);
			else
			{
				index += items_to_move;
			}
		}
		else
		{
			index = get_selectable_item_index(
				mr, mi_current, NULL) +
				items_to_move;
			/* correct for the case that we're between items */
			if (!MI_IS_SELECTABLE(mi_current))
			{
				index--;
			}
		}
		if (do_skip_section)
		{
			new_item = get_selectable_item_from_section(mr, index);
		}
		else
		{
			new_item = get_selectable_item_from_index(mr, index);
			if (items_to_move > 0 && new_item == mi_current)
			{
				new_item =
					get_selectable_item_from_index(mr, 0);
			}
		}
		if (new_item)
		{
			*pmi_current = new_item;
			pmret->rc = MENU_NEWITEM;
		}
		else
		{
			pmret->rc = MENU_NOP;
		}
		break;
	case SA_CONTINUE:
		*pmi_current = MR_LAST_ITEM(mr);
		if (*pmi_current && MI_IS_POPUP(*pmi_current))
		{
			/* enter the submenu */
			pmret->rc = MENU_POPUP;
		}
		else
		{
			/* do nothing */
			*pmi_current = mi_current;
			pmret->rc = MENU_NOP;
		}
		break;
	case SA_WARPBACK:
		/* Warp the pointer back into the menu. */
		FWarpPointer(
			dpy, 0, MR_WINDOW(mr), 0, 0, 0, 0,
			menudim_middle_x_offset(&MR_DIM(mr)), my - menu_y);
		pmret->rc = MENU_NEWITEM_FIND;
		break;
	case SA_SELECT:
		pmret->rc = MENU_SELECTED;
		return;
	case SA_ABORT:
		pmret->rc =
			(MR_IS_TEAR_OFF_MENU(mr)) ?
			MENU_KILL_TEAR_OFF_MENU : MENU_ABORTED;
		return;
	case SA_TEAROFF:
		pmret->rc =
			(MR_IS_TEAR_OFF_MENU(mr)) ? MENU_NOP : MENU_TEAR_OFF;
		return;
	case SA_SCROLL:
		if (MST_MOUSE_WHEEL(mr) == MMW_MENU)
		{
			items_to_move *= -1;
		}
		if (
			!menu_get_outer_geometry(
				mr, pmp, &JunkRoot, &menu_x, &menu_y,
				&JunkWidth, &menu_height,
				&JunkBW, &JunkDepth))
		{
			fvwm_msg(
				ERR, "menu_shortcuts",
				"can't get geometry of menu %s", MR_NAME(mr));
			return;
		}
		if (do_skip_section)
		{
			int count;

			get_selectable_item_count(mr, &count);
			get_selectable_item_index(mr, mi_current, &index);
			index += items_to_move;
			if (index < 0)
			{
				index = 0;
			}
			else if (index > count)
			{
				index = count;
			}
			new_item = get_selectable_item_from_section(mr, index);
		}
		else
		{
			index = get_selectable_item_index(
				mr, mi_current, NULL);
			if (items_to_move > 0 && !MI_IS_SELECTABLE(mi_current))
			{
				index--;
			}
			index += items_to_move;
			new_item = get_selectable_item_from_index(mr, index);
		}

		if (
			new_item &&
			((items_to_move < 0 &&
			  MI_Y_OFFSET(new_item) > MI_Y_OFFSET(mi_current)) ||
			 (items_to_move > 0 &&
			  MI_Y_OFFSET(new_item) < MI_Y_OFFSET(mi_current))))
		{
			/* never scroll in the "wrong" direction */
			new_item = NULL;
		}

		if (new_item)
		{
			*pmi_current = new_item;
			pmret->rc = MENU_NEWITEM;
			/* Have to work with relative positions or tear off
			 * menus will be hard to reposition */
			if (
				FQueryPointer(
					dpy, MR_WINDOW(mr), &JunkRoot,
					&JunkChild, &JunkX, &JunkY, &mx, &my,
					&JunkMask) ==  0)
			{
				/* This should not happen */
			    	mx = 0;
				my = 0;
			}

			if (MST_MOUSE_WHEEL(mr) == MMW_POINTER)
			{
				if (event->type == ButtonRelease)
				{

				  	FWarpPointer(
						dpy, 0, 0, 0, 0, 0, 0, 0,
						-my +
						menuitem_middle_y_offset(
							new_item,
							MR_STYLE(mr)));
				}
				/* pointer wrapped elsewhere for key events */
			}
			else
			{
				int old_y = menu_y;

				menu_y += my - menuitem_middle_y_offset(
					new_item, MR_STYLE(mr));

				if (
					!MST_SCROLL_OFF_PAGE(mr) &&
					menu_height < MR_SCREEN_HEIGHT(mr))
				{
					if (menu_y < 0)
					{
						FWarpPointer(dpy, 0, 0, 0, 0,
							     0, 0, 0,-menu_y);
						menu_y=0;
					}

					if (
						menu_y + menu_height >
						MR_SCREEN_HEIGHT(mr))
					{
						FWarpPointer(
							dpy, 0, 0, 0, 0, 0, 0,
							0,
							MR_SCREEN_HEIGHT(mr) -
							menu_y - menu_height);
						menu_y = MR_SCREEN_HEIGHT(mr) -
							menu_height;
					}
				}
				if (old_y != menu_y)
				{
					pmret->rc = MENU_NEWITEM_MOVEMENU;
					*ret_menu_x = menu_x;
					*ret_menu_y = menu_y;
				}
				else
				{
					pmret->rc = MENU_NEWITEM;
				}
			}
		}
		else
		{
			pmret->rc = MENU_NOP;
		}
		break;
	case SA_EXEC_CMD:
		pmret->rc = MENU_EXEC_CMD;
		*pmp->ret_paction = command;
		break;
	case SA_NONE:
	default:
		pmret->rc = MENU_NOP;
		break;
	}
	if (saction != SA_SCROLL && pmret->rc == MENU_NEWITEM)
	{
		if (!menu_get_outer_geometry(
			    mr, pmp, &JunkRoot, &menu_x, &menu_y,
			    &JunkWidth, &menu_height,
			    &JunkBW, &JunkDepth))
		{
			fvwm_msg(
				ERR, "menu_shortcuts",
				"can't get geometry of menu %s", MR_NAME(mr));
			return;
		}
		if (menu_y < 0 || menu_y + menu_height > MR_SCREEN_HEIGHT(mr))
		{
			menu_y = (menu_y < 0) ?
				0 : MR_SCREEN_HEIGHT(mr) - menu_height;
			pmret->rc = MENU_NEWITEM_MOVEMENU;
			*ret_menu_x = menu_x;
			*ret_menu_y = menu_y;
		}
	}

	return;
}
