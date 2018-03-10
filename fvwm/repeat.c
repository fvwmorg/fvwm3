/* -*-c-*- */
/* Copyright (C) 1999  Dominik Vogt */
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

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "repeat.h"
#include "libs/Parse.h"


/* If non-zero we are already repeating a function, so don't record the
 * command again. */
static int repeat_depth = 0;

#if 0
typedef struct
{
	char *start;
	char *end;
} double_ended_string;

static struct
{
	double_ended_string string;
	double_ended_string old;
	double_ended_string builtin;
	double_ended_string function;
	double_ended_string top_function;
	double_ended_string module;
	double_ended_string menu;
	double_ended_string popup;
	double_ended_string menu_or_popup;
	int page_x;
	int page_y;
	int desk;
	FvwmWindow *fvwm_window;
} last;
#endif

static struct
{
	char *command_line;
	char *menu_name;
} last = {
	NULL,
	NULL
};

#if 0
char *repeat_last_function = NULL;
char *repeat_last_complex_function = NULL;
char *repeat_last_builtin_function = NULL;
char *repeat_last_module = NULL;
char *repeat_last_top_function = NULL;
char *repeat_last_menu = NULL;
FvwmWindow *repeat_last_fvwm_window = NULL;
#endif

/* Stores the contents of the data pointer internally for the repeat command.
 * The type of data is determined by the 'type' parameter. If this function is
 * called to set a string value representing an fvwm builtin function the
 * 'builtin' can be set to the F_... value in the function table in
 * functions.c. If this value is set certain functions are not recorded.
 * The 'depth' parameter indicates the recursion depth of the current data
 * pointer (i.e. the first function call has a depth of one, functions called
 * from within this function have depth 2 and higher, this may be applicable
 * to future enhancements like menus).
 *
 * TODO: [finish and update description]
 */
Bool set_repeat_data(void *data, repeat_t type, const func_t *builtin)
{
	/* No history recording during startup. */
	if (fFvwmInStartup)
	{
		return True;
	}

	switch(type)
	{
	case REPEAT_COMMAND:
		if (last.command_line == (char *)data)
		{
			/* Already stored, no need to free the data pointer. */
			return False;
		}
		if (data == NULL || repeat_depth != 0)
		{
			/* Ignoring the data, must free it outside of this
			 * call. */
			return True;
		}
		if (builtin && (builtin->flags & FUNC_DONT_REPEAT))
		{
			/* Dont' record functions that have the
			 * FUNC_DONT_REPEAT flag set. */
			return True;
		}
		if (last.command_line)
		{
			free(last.command_line);
		}
		/* Store a backup. */
		last.command_line = (char *)data;
		/* Since we stored the pointer the caller must not free it. */
		return False;
	case REPEAT_MENU:
	case REPEAT_POPUP:
		if (last.menu_name)
		{
			free(last.menu_name);
		}
		last.menu_name = (char *)data;
		/* Since we stored the pointer the caller must not free it. */
		return False;
	case REPEAT_PAGE:
	case REPEAT_DESK:
	case REPEAT_DESK_AND_PAGE:
		return True;
	case REPEAT_FVWM_WINDOW:
		return True;
	case REPEAT_NONE:
	default:
		return True;
	}
}

void CMD_Repeat(F_CMD_ARGS)
{
	int index;
	char *optlist[] = {
		"command",
		NULL
	};

	repeat_depth++;
	/* Replay the backup, we don't want the repeat command recorded. */
	GetNextTokenIndex(action, optlist, 0, &index);
	switch (index)
	{
	case 0: /* command */
	default:
		action = last.command_line;
		execute_function(
			cond_rc, exc, action, FUNC_DONT_EXPAND_COMMAND);
		break;
	}
	repeat_depth--;

	return;
}
