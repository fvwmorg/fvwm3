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

/* ---------------------------- included header files ----------------------- */

#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xdefs.h>

/* the following 5 are just to satisfy X11/extensions/shape.h on some systems */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "libs/FShape.h"
#include "libs/Module.h"
#include "libs/Strings.h"
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

static void change_button_title(button_info *b, const char *text)
{
	if (!(b->flags & b_Title))
	{
		show_error("Cannot create a title, only change one\n");
		return;
	}
	if (text == NULL)
	{
		show_error("No title to change specified, unsupported\n");
		return;
	}
	free(b->title);
	CopyString(&b->title, text);
	return;
}

static void change_button_icon(button_info *b, const char *file)
{
	FvwmPicture *new_icon;

	if (!(b->flags & b_Icon))
	{
		show_error("Cannot create an icon, only change one\n");
		return;
	}
	if (file == NULL)
	{
		show_error("No icon to change specified, unsupported\n");
		return;
	}
	if (LoadIconFile(file, &new_icon, b->colorset) == 0)
	{
		show_error("Cannot load icon %s\n", file);
		return;
	}
	free(b->icon_file);
	CopyString(&b->icon_file, file);
	PDestroyFvwmPicture(Dpy, b->icon);
	XDestroyWindow(Dpy, b->IconWin);
	b->IconWin = None;
	b->icon = new_icon;
	CreateIconWindow(b);
	ConfigureIconWindow(b);
	XMapWindow(Dpy, b->IconWin);
	return;
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
	char *fore = (b->flags & b_Fore) ? b->fore : "black";
	char *back = (b->flags & b_Back) ? b->back : "gray";

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
			if ((b->flags & b_Id) && StrEquals(b->id, s))
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
	"Silent", "ChangeButton", "ExpandButtonVars", NULL
};
static char *button_options[] =
{
	"Title", "Icon", NULL
};

void parse_message_line(char *line)
{
	char *rest;
	int action = -1;
	button_info *b;

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
			line = rest;
		}
	} while (action == 0);

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
		while (rest && rest[0] != '\0')
		{
			char *option_pair;
			int option;
			char *value0, *value;

			/* parse option and value and give diagnostics */
			rest = GetQuotedString(
				rest, &option_pair, ",", NULL, NULL, NULL);
			while (isspace(*rest))
			{
				rest++;
			}
	                if (!option_pair)
        	        {
                	        continue;
	                }

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

			switch (option)
			{
			case 0:
				/* Title */
				change_button_title(b, value);
				break;
			case 1:
				/* Icon */
				change_button_icon(b, value);
				break;
			}

			if (value)
			{
				free(value);
			}
		}

		RedrawButton(b, 2);
		if (FShapesSupported)
		{
			if (UberButton->c->flags & b_TransBack)
				SetTransparentBackground(
					UberButton, Width, Height);
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
	}

	return;
}
