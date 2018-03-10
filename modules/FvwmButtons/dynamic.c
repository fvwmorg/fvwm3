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

/* ---------------------------- included header files ----------------------- */

#include "config.h"
#include <stdarg.h>
#include <stdio.h>

/* the following 5 are just to satisfy X11/extensions/shape.h on some systems */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "libs/Module.h"
#include "libs/Strings.h"
#include "libs/Parse.h"
#include "FvwmButtons.h"
#include "button.h"
#include "draw.h"
#include "icons.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

extern int Width, Height;
extern int screen;

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static Bool silent = False;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static void show_error(const char *msg, ...)
{
	va_list args;

	if (silent == True)
	{
		return;
	}
	va_start(args, msg);
	vfprintf(stderr, CatString3(MyName, ": ", msg), args);
	va_end(args);
}



#if 0
/* to be used in module_expand_action */
static module_expand_vars mev =
{
	/* number of vars */
	4,
	/* array of var names */
	{ "title", "icon", }
	/* array of values, void *, will be filled later */
	{ NULL, NULL },
	/* array of booleans, 0 if string, 1 if int */
	{ 0, 0, },
};
#endif

static char *expand_button_vars(const button_info *b, const char *line)
{
	/* not fully implemented yet, should expand $title, $icon and so on */
	rectangle r;
	char *expanded_line;

	/* there should be a function evaluating fore/back from colorset */
	char *fore = (b->flags.b_Fore) ? b->fore : "black";
	char *back = (b->flags.b_Back) ? b->back : "gray";

	get_button_root_geometry(&r, b);
	expanded_line = module_expand_action(
		Dpy, screen, (char *)line, &r, fore, back);

	return expanded_line;
}

static button_info *parse_button_id(char **line)
{
	button_info *b = NULL, *ub = UberButton;
	int mask;
	int x, y;
	int count, i;
	char *rest;
	char *s;

	s = PeekToken(*line, &rest);
	*line = NULL;
	if (!s)
	{
		show_error("No button id specified\n");
		return NULL;
	}

	if (*s == '+')
	{
		unsigned int JunkWidth;
		unsigned int JunkHeight;

		mask = XParseGeometry(s, &x, &y, &JunkWidth, &JunkHeight);
		if (!(mask & XValue) || (mask & XNegative) ||
		    !(mask & YValue) || (mask & YNegative))
		{
			show_error("Illegal button position '%s'\n", s);
			return NULL;
		}
		if (x < 0 || y < 0)
		{
			show_error("Button column/row must not be negative\n");
			return NULL;
		}
		b = get_xy_button(ub, y, x);
		if (b == NULL)
		{
			show_error(
				"Button at column %d row %d not found\n", x, y);
			return NULL;
		}
	}
	else if (isdigit(*s))
	{
		x = atoi(s);
		i = count = -1;
		/* find the button */
		while (NextButton(&ub, &b, &i, 0))
		{
			if (++count == x)
				break;
		}
		if (count != x || b == NULL)
		{
			show_error("Button number %d not found\n", x);
			return NULL;
		}
	}
	else if (isalpha(*s))
	{
		Bool found = False;
		i = -1;
		/* find the button */
		while (NextButton(&ub, &b, &i, 0))
		{
			if (b->flags.b_Id && StrEquals(b->id, s))
			{
				found = True;
				break;
			}
		}
		if (found == False)
		{
			show_error("Button id '%s' does not exist\n", s);
			return NULL;
		}
	}
	else
	{
		show_error("Invalid button id '%s' specified\n", s);
		return NULL;
	}

	*line = rest;
	return b;
}

/* ---------------------------- interface functions ------------------------- */

static char *actions[] =
{
	"Silent", "ChangeButton", "ExpandButtonVars", "PressButton", NULL
};

static char *button_options[] =
{
	"Title", "Icon", "ActiveTitle", "ActiveIcon", "PressTitle", "PressIcon",
	"Colorset", NULL
};

void parse_message_line(char *line)
{
	char *rest;
	int action = -1;
	button_info *b;
	char *act;
	char *buttonn;
	int mousebutton;

	silent = False;
	do
	{
		action = GetTokenIndex(line, actions, -1, &rest);
		if (action == -1)
		{
			show_error("Message not understood: %s", line);
			return;
		}
		if (action == 0)
		{
			silent = True;
			break;
		}
	} while (action == 0);

	/* If silent was given (to surpress erros from parsing commands, then
	 * retokenise the rest of the line, so that token matching for the
	 * actual command is possible.
	 */
	if (silent == True) {
		line = PeekToken(line, &rest);
		action = GetTokenIndex(rest, actions, -1, &rest);
	}

	/* find out which button */
	b = parse_button_id(&rest);
	if (!b || !rest)
	{
		return;
	}

	switch (action)
	{
	case 1:
		/* ChangeButton */
		/* The dimensions of individual buttons (& the overall size of
		 * the FvwmButtons window) is based on the initial
		 * configuration for the module. In some configurations,
		 * dynamically adding/changing a title/icon may mean it no
		 * longer fits on a button. Currently, there are no checks for
		 * this occurance. */
		while (rest && rest[0] != '\0')
		{
			char *option_pair;
			int option, i;
			char *value0, *value;
			FvwmPicture *icon;

			/* parse option and value and give diagnostics */
			rest = GetQuotedString(
				rest, &option_pair, ",", NULL, NULL, NULL);
			while (isspace(*rest))
			{
				rest++;
			}
			if (!option_pair)
				continue;

			option = GetTokenIndex(
				option_pair, button_options, -1, &value0);
			if (option < 0)
			{
				show_error(
					"Unsupported button option line '%s'\n",
					option_pair);
				free(option_pair);
				continue;
			}

			GetNextToken(value0, &value);
			free(option_pair);

			if (value == NULL)
			{
				show_error(
					"No title/icon to change specified.\n");
				continue;
			}
			switch (option)
			{
			case 0:
				/* Title */
				if (b->flags.b_Title)
					free(b->title);
				b->flags.b_Title = 1;
				b->title = value;
				value = NULL;
				break;
			case 2:
				/* ActiveTitle */
				if (b->flags.b_ActiveTitle)
					free(b->activeTitle);
				b->flags.b_ActiveTitle = 1;
				b->title = value;
				value = NULL;
				break;
			case 4:
				/* PressTitle */
				if (b->flags.b_PressTitle)
					free(b->pressTitle);
				b->flags.b_PressTitle = 1;
				b->title = value;
				value = NULL;
				break;
			case 6:
				/* Colorset */
				i = atoi(value);
				b->colorset = i;
				b->flags.b_Colorset = 1;
				AllocColorset(i);
				break;
			default:
				if (
					LoadIconFile(
						value, &icon, b->colorset) == 0)
				{
					show_error(
						"Cannot load icon \"%s\"\n",
						value);
				}
				else
				{
				    switch (option)
				    {
					case 1: /* Icon */
						if (b->flags.b_Icon)
						{
							free(b->icon_file);
							PDestroyFvwmPicture(
								Dpy, b->icon);
						}
						b->flags.b_Icon = 1;
						b->icon_file = value;
						value = NULL;
						b->icon = icon;
						break;
					case 3: /* ActiveIcon */
						if (b->flags.b_ActiveIcon)
						{
							free(b->active_icon_file);
							PDestroyFvwmPicture(
								Dpy,
								b->activeicon);
						}
						b->flags.b_ActiveIcon = 1;
						b->active_icon_file = value;
						value = NULL;
						b->activeicon = icon;
						break;
					case 5: /* PressIcon */
						if (b->flags.b_PressIcon)
						{
							free(b->press_icon_file);
							PDestroyFvwmPicture(
								Dpy,
								b->pressicon);
						}
						b->flags.b_PressIcon = 1;
						b->press_icon_file = value;
						value = NULL;
						b->pressicon = icon;
						break;
					}
				}
				break;
			}

			if (value)
		    {
			    free(value);
		    }
		}

		RedrawButton(b, DRAW_FORCE, NULL);
		if (UberButton->c->flags.b_TransBack)
		{
			SetTransparentBackground(UberButton, Width, Height);
		}
		break;

	case 2:
		/* ExpandButtonVars */
		line = expand_button_vars(b, rest);
		if (line)
		{
			SendText(fd, line, 0);
			free(line);
		}
		break;
	case 3:
		/* PressButton */
		rest = GetQuotedString(rest, &buttonn, "", NULL, NULL, NULL);
		if (buttonn)
		{
			mousebutton = atoi(buttonn);
			free(buttonn);
			if (
				mousebutton <= 0 ||
				mousebutton > NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
			{
				mousebutton = 1;
			}
		}
		else
		{
			mousebutton = 1;
		}
		CurrentButton = b;
		act = GetButtonAction(b, mousebutton);
		ButtonPressProcess(b, &act);
		if (act)
		{
			free(act);
		}
		CurrentButton = NULL;

		break;
	}

	return;
}
