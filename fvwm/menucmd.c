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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/* IMPORTANT NOTE: Do *not* use any constant numbers in this file. All values
 * have to be #defined in the section below or defaults.h to ensure full
 * control over the menus. */

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <X11/keysym.h>

#include <libs/fvwmlib.h>
#include "fvwm.h"
#include "externs.h"
#include "events.h"
#include "menus.h"
#include "cursor.h"
#include "functions.h"
#include "repeat.h"
#include "misc.h"
#include "move_resize.h"
#include "screen.h"
#include "borders.h"
#include "colors.h"
#include "colormaps.h"
#include "decorations.h"
#include "colorset.h"
#include "defaults.h"
#include "libs/FScreen.h"
#include "libs/Flocale.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static void menu_func(F_CMD_ARGS, Bool fStaysUp)
{
	MenuRoot *menu;
	char *ret_action = NULL;
	MenuOptions mops;
	char *menu_name = NULL;
	XEvent *teventp;
	MenuParameters mp;
	MenuReturn mret;
	FvwmWindow *fw2;
	int tc;
	extern FvwmWindow *Fw;
	extern FvwmWindow *ButtonWindow;
	extern int Context;

	memset(&mops, 0, sizeof(mops));
	memset(&mret, 0, sizeof(MenuReturn));
	action = GetNextToken(action,&menu_name);
	action = get_menu_options(
		action, w, fw, eventp, NULL, NULL, &mops);
	while (action && *action && isspace((unsigned char)*action))
	{
		action++;
	}
	if (action && *action == 0)
	{
		action = NULL;
	}
	menu = menus_find_menu(menu_name);
	if (menu == NULL)
	{
		if (menu_name)
		{
			fvwm_msg(ERR,"menu_func","No such menu %s",menu_name);
			free(menu_name);
		}
		return;
	}
	if (menu_name &&
	    set_repeat_data(
		    menu_name, (fStaysUp) ? REPEAT_MENU : REPEAT_POPUP,NULL))
	{
		free(menu_name);
	}

	if (!action && eventp && eventp->type == KeyPress)
	{
		teventp = (XEvent *)1;
	}
	else
	{
		teventp = eventp;
	}

	memset(&mp, 0, sizeof(mp));
	mp.menu = menu;
	fw2 = (fw != None) ? fw : Fw;
	mp.pfw = &fw2;
	mp.button_window = ButtonWindow;
	MR_IS_TEAR_OFF_MENU(menu) = 0;
	tc = Context;
	mp.pcontext = &tc;
	mp.flags.has_default_action = (action != NULL);
	mp.flags.is_menu_from_frame_or_window_or_titlebar = False;
	mp.flags.is_sticky = fStaysUp;
	mp.flags.is_submenu = False;
	mp.flags.is_already_mapped = False;
	mp.eventp = teventp;
	mp.pops = &mops;
	mp.ret_paction = &ret_action;

	do_menu(&mp, &mret);
	if (mret.rc == MENU_DOUBLE_CLICKED && action)
	{
		old_execute_function(F_PASS_EXEC_ARGS, 0, NULL);
	}

	return;
}

/* ---------------------------- interface functions ------------------------- */

/* ---------------------------- builtin commands ---------------------------- */

/* the function for the "Popup" command */
void CMD_Popup(F_CMD_ARGS)
{
	menu_func(F_PASS_ARGS, False);

	return;
}

/* the function for the "Menu" command */
void CMD_Menu(F_CMD_ARGS)
{
	menu_func(F_PASS_ARGS, True);

	return;
}

void CMD_AddToMenu(F_CMD_ARGS)
{
	MenuRoot *mr;
	MenuRoot *mrPrior;
	char *token, *rest,*item;

	token = PeekToken(action, &rest);
	if (!token)
	{
		return;
	}
	mr = menus_find_menu(token);
	if (mr && MR_MAPPED_COPIES(mr) != 0)
	{
		fvwm_msg(ERR,"add_item_to_menu", "menu %s is in use", token);
		return;
	}
	mr = FollowMenuContinuations(menus_find_menu(token), &mrPrior);
	if (mr == NULL)
	{
		mr = NewMenuRoot(token);
	}

	/* Set + state to last menu */
	set_last_added_item(ADDED_MENU, mr);

	rest = GetNextToken(rest, &item);
	AddToMenu(mr, item, rest, True /* pixmap scan */, True, False);
	if (item)
	{
		free(item);
	}

	return;
}

void CMD_DestroyMenu(F_CMD_ARGS)
{
	MenuRoot *mr;
	MenuRoot *mrContinuation;
	Bool do_recreate = False;
	char *token;

	token = PeekToken(action, &action);
	if (!token)
	{
		return;
	}
	if (StrEquals(token, "recreate"))
	{
		do_recreate = True;
		token = PeekToken(action, NULL);
	}
	mr = menus_find_menu(token);
	if (Scr.last_added_item.type == ADDED_MENU)
	{
		set_last_added_item(ADDED_NONE, NULL);
	}
	while (mr)
	{
		/* save continuation before destroy */
		mrContinuation = MR_CONTINUATION_MENU(mr);
		if (!DestroyMenu(mr, do_recreate, True))
		{
			return;
		}
		/* Don't recreate the continuations */
		do_recreate = False;
		mr = mrContinuation;
	}

	return;
}

void CMD_DestroyMenuStyle(F_CMD_ARGS)
{
	MenuStyle *ms = NULL;
	char *name = NULL;

	name = PeekToken(action, NULL);
	if (name == NULL)
	{
		fvwm_msg(ERR,"DestroyMenuStyle", "needs one parameter");
		return;
	}

	ms = menustyle_find(name);
	if (ms == NULL)
	{
		return;
	}
	else if (ms == menustyle_get_default_style())
	{
		fvwm_msg(ERR,"DestroyMenuStyle",
			 "cannot destroy default menu style. "
			 "To reset the default menu style use\n  %s",
			 DEFAULT_MENU_STYLE);
		return;
	}
	else if (ST_USAGE_COUNT(ms) != 0)
	{
		fvwm_msg(ERR, "DestroyMenuStyle", "menu style %s is in use",
			 name);
		return;
	}
	else
	{
		menustyle_free(ms);
	}
	menus_remove_style_from_menus(ms);

	return;
}

void CMD_ChangeMenuStyle(F_CMD_ARGS)
{
	char *name = NULL;
	char *menuname = NULL;
	MenuStyle *ms = NULL;
	MenuRoot *mr = NULL;

	name = PeekToken(action, &action);
	if (name == NULL)
	{
		fvwm_msg(ERR,"ChangeMenuStyle",
			 "needs at least two parameters");
		return;
	}

	ms = menustyle_find(name);
	if (ms == NULL)
	{
		fvwm_msg(ERR,"ChangeMenuStyle", "cannot find style %s", name);
		return;
	}

	menuname = PeekToken(action, &action);
	while (menuname && *menuname)
	{
		mr = menus_find_menu(menuname);
		if (mr == NULL)
		{
			fvwm_msg(ERR, "ChangeMenuStyle", "cannot find menu %s",
				 menuname);
			break;
		}
		if (MR_MAPPED_COPIES(mr) != 0)
		{
			fvwm_msg(ERR, "ChangeMenuStyle", "menu %s is in use",
				 menuname);
		}
		else
		{
			MR_STYLE(mr) = ms;
			MR_IS_UPDATED(mr) = 1;
		}
		menuname = PeekToken(action, &action);
	}

	return;
}
