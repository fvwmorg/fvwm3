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

#include "libs/fvwmlib.h"
#include "libs/ColorUtils.h"
#include "libs/Cursor.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "fvwm.h"
#include "libs/PictureImageLoader.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "cursor.h"
#include "menus.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static Cursor cursors[CRS_MAX];
static const unsigned int default_cursors[CRS_MAX] =
{
	None,
	XC_top_left_corner,      /* CRS_POSITION */
	XC_top_left_arrow,       /* CRS_TITLE */
	XC_top_left_arrow,       /* CRS_DEFAULT */
	XC_hand2,                /* CRS_SYS */
	XC_fleur,                /* CRS_MOVE */
	XC_sizing,               /* CRS_RESIZE */
	XC_watch,                /* CRS_WAIT */
	XC_top_left_arrow,       /* CRS_MENU */
	XC_crosshair,            /* CRS_SELECT */
	XC_pirate,               /* CRS_DESTROY */
	XC_top_side,             /* CRS_TOP */
	XC_right_side,           /* CRS_RIGHT */
	XC_bottom_side,          /* CRS_BOTTOM */
	XC_left_side,            /* CRS_LEFT */
	XC_top_left_corner,      /* CRS_TOP_LEFT */
	XC_top_right_corner,     /* CRS_TOP_RIGHT */
	XC_bottom_left_corner,   /* CRS_BOTTOM_LEFT */
	XC_bottom_right_corner,  /* CRS_BOTTOM_RIGHT */
	XC_top_side,             /* CRS_TOP_EDGE */
	XC_right_side,           /* CRS_RIGHT_EDGE */
	XC_bottom_side,          /* CRS_BOTTOM_EDGE */
	XC_left_side,            /* CRS_LEFT_EDGE */
	XC_left_ptr,             /* CRS_ROOT */
	XC_plus                  /* CRS_STROKE */
};

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static void SafeDefineCursor(Window w, Cursor cursor)
{
	if (w)
	{
		XDefineCursor(dpy,w,cursor);
	}

	return;
}

/* ---------------------------- interface functions ------------------------ */

/* CreateCursors - Loads fvwm cursors */
Cursor *CreateCursors(Display *dpy)
{
	int i;
	/* define cursors */
	cursors[0] = None;
	for (i = 1; i < CRS_MAX; i++)
	{
		cursors[i] = XCreateFontCursor(dpy, default_cursors[i]);
	}

	return cursors;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_CursorStyle(F_CMD_ARGS)
{
	char *cname=NULL;
	char *newcursor=NULL;
	char *errpos = NULL;
	char *fore = NULL;
	char *back = NULL;
	XColor colors[2];
	int index;
	int nc;
	int i;
	int my_nc;
	FvwmWindow *fw2;
	Cursor cursor;

	cursor = 0;
	cname = PeekToken(action, &action);
	if (!cname)
	{
		fvwm_msg(ERR, "CursorStyle", "Bad cursor style");

		return;
	}
	if (StrEquals("POSITION", cname))
	{
		index = CRS_POSITION;
	}
	else if (StrEquals("DEFAULT", cname))
	{
		index = CRS_DEFAULT;
	}
	else if (StrEquals("SYS", cname))
	{
		index = CRS_SYS;
	}
	else if (StrEquals("TITLE", cname))
	{
		index = CRS_TITLE;
	}
	else if (StrEquals("MOVE", cname))
	{
		index = CRS_MOVE;
	}
	else if (StrEquals("RESIZE", cname))
	{
		index = CRS_RESIZE;
	}
	else if (StrEquals("MENU", cname))
	{
		index = CRS_MENU;
	}
	else if (StrEquals("WAIT", cname))
	{
		index = CRS_WAIT;
	}
	else if (StrEquals("SELECT", cname))
	{
		index = CRS_SELECT;
	}
	else if (StrEquals("DESTROY", cname))
	{
		index = CRS_DESTROY;
	}
	else if (StrEquals("LEFT", cname))
	{
		index = CRS_LEFT;
	}
	else if (StrEquals("RIGHT", cname))
	{
		index = CRS_RIGHT;
	}
	else if (StrEquals("TOP", cname))
	{
		index = CRS_TOP;
	}
	else if (StrEquals("BOTTOM", cname))
	{
		index = CRS_BOTTOM;
	}
	else if (StrEquals("TOP_LEFT", cname))
	{
		index = CRS_TOP_LEFT;
	}
	else if (StrEquals("TOP_RIGHT", cname))
	{
		index = CRS_TOP_RIGHT;
	}
	else if (StrEquals("BOTTOM_LEFT", cname))
	{
		index = CRS_BOTTOM_LEFT;
	}
	else if (StrEquals("BOTTOM_RIGHT", cname))
	{
		index = CRS_BOTTOM_RIGHT;
	}
	else if (StrEquals("LEFT_EDGE", cname))
	{
		index = CRS_LEFT_EDGE;
	}
	else if (StrEquals("RIGHT_EDGE", cname))
	{
		index = CRS_RIGHT_EDGE;
	}
	else if (StrEquals("TOP_EDGE", cname))
	{
		index = CRS_TOP_EDGE;
	}
	else if (StrEquals("BOTTOM_EDGE", cname))
	{
		index = CRS_BOTTOM_EDGE;
	}
	else if (StrEquals("ROOT", cname))
	{
		index = CRS_ROOT;
	}
	else if (StrEquals("STROKE", cname))
	{
		index = CRS_STROKE;
	}
	else
	{
		fvwm_msg(ERR, "CursorStyle", "Unknown cursor name %s", cname);

		return;
	}
	cname = 0;

	/* check if the cursor is given by X11 name */
	action = GetNextToken(action, &newcursor);
	if (newcursor)
	{
		my_nc = fvwmCursorNameToIndex(newcursor);
	}
	else
	{
		my_nc = default_cursors[index];
	}

	if (my_nc == -1)
	{
		nc = strtol(newcursor, &errpos, 10);
		if (errpos && *errpos == '\0')
		{
			my_nc = 0;
		}
	}
	else
	{
		nc = my_nc;
	}

	if (my_nc > -1)
	{
		/* newcursor was a number or the name of a X11 cursor */
		if ((nc < 0) || (nc >= XC_num_glyphs) || ((nc % 2) != 0))
		{
			fvwm_msg(
				ERR, "CursorStyle", "Bad cursor number %s",
				newcursor);
			free(newcursor);

			return;
		}
		cursor = XCreateFontCursor(dpy, nc);
	}
	else
	{
		/* newcursor was not a number neither a X11 cursor name */
		if (
			StrEquals("none", newcursor) ||
			StrEquals("tiny", newcursor))
		{
			XColor nccol;

			XSetForeground(
				dpy, Scr.MonoGC,
				(tolower(*newcursor) == 'n') ? 0 : 1);
			XFillRectangle(
				dpy, Scr.ScratchMonoPixmap, Scr.MonoGC,
				0, 0, 1, 1);
			cursor = XCreatePixmapCursor(
				dpy, Scr.ScratchMonoPixmap,
				Scr.ScratchMonoPixmap, &nccol, &nccol, 0, 0);
		}
		else
		{
			char *path;
			char *tmp;
			int hotspot[2];

			hotspot[0] = -1;
			hotspot[1] = -1;
			path = PictureFindImageFile(newcursor, NULL, R_OK);
			if (!path)
			{
				fvwm_msg(
					ERR, "CursorStyle",
					"Cursor %s not found", newcursor);
				free(newcursor);

				return;
			}
			if (GetIntegerArguments(action, &tmp, hotspot, 2) == 2)
			{
				action = tmp;
			}
			cursor = PImageLoadCursorFromFile(
				dpy, Scr.Root, path, hotspot[0], hotspot[1]);
			free(path);
		}
	}
	if (!cursor)
	{
		fvwm_msg(
			ERR, "CursorStyle", "Cannot load cursor: %s",
			newcursor);
		free(newcursor);

		return;
	}
	free(newcursor);
	newcursor = 0;

	/* replace the cursor defn */
	if (Scr.FvwmCursors[index])
	{
		XFreeCursor(dpy, Scr.FvwmCursors[index]);
	}
	Scr.FvwmCursors[index] = cursor;

	/* look for optional color arguments */
	action = GetNextToken(action, &fore);
	action = GetNextToken(action, &back);
	if (fore && back)
	{
		colors[0].pixel = GetColor(fore);
		colors[1].pixel = GetColor(back);
		XQueryColors (dpy, Pcmap, colors, 2);
		XRecolorCursor(
			dpy, Scr.FvwmCursors[index], &(colors[0]),
			&(colors[1]));
	}
	if (fore)
	{
		free(fore);
	}
	if (back)
	{
		free(back);
	}

	/* redefine all the windows using cursors */
	for (fw2 = Scr.FvwmRoot.next; fw2; fw2 = fw2->next)
	{
		if (!HAS_HANDLES(fw2))
		{
			/* Ignore windows without handles */
			continue;
		}
		for (i = 0; i < 4; i++)
		{
			SafeDefineCursor(
				FW_W_CORNER(fw2, i),
				Scr.FvwmCursors[CRS_TOP_LEFT + i]);
			SafeDefineCursor(
				FW_W_SIDE(fw2, i),
				Scr.FvwmCursors[CRS_TOP + i]);
		}
		for (i = 0; i / 2 < Scr.nr_left_buttons; i += 2)
		{
			SafeDefineCursor(
				FW_W_BUTTON(fw2, i), Scr.FvwmCursors[CRS_SYS]);
		}
		for (i = 1; i / 2 < Scr.nr_right_buttons; i += 2)
		{
			SafeDefineCursor(
				FW_W_BUTTON(fw2, i), Scr.FvwmCursors[CRS_SYS]);
		}
		SafeDefineCursor(FW_W_TITLE(fw2), Scr.FvwmCursors[CRS_TITLE]);
		if (index == CRS_DEFAULT)
		{
			SafeDefineCursor(
				FW_W_FRAME(fw2), Scr.FvwmCursors[CRS_DEFAULT]);
			SafeDefineCursor(
				FW_W_PARENT(fw2), Scr.FvwmCursors[CRS_DEFAULT]);
			if (IS_ICONIFIED(fw2))
			{
				if (!HAS_NO_ICON_TITLE(fw2))
				{
					SafeDefineCursor(
						FW_W_ICON_TITLE(fw2),
						Scr.FvwmCursors[CRS_DEFAULT]);
				}
				if (FW_W_ICON_PIXMAP(fw2) != None)
				{
					SafeDefineCursor(
						FW_W_ICON_PIXMAP(fw2),
						Scr.FvwmCursors[CRS_DEFAULT]);
				}
			}
		}
	}

	/* Do the menus for good measure */
	SetMenuCursor(Scr.FvwmCursors[CRS_MENU]);

	SafeDefineCursor(Scr.PanFrameTop.win, Scr.FvwmCursors[CRS_TOP_EDGE]);
	SafeDefineCursor(
		Scr.PanFrameBottom.win, Scr.FvwmCursors[CRS_BOTTOM_EDGE]);
	SafeDefineCursor(Scr.PanFrameLeft.win, Scr.FvwmCursors[CRS_LEFT_EDGE]);
	SafeDefineCursor(
		Scr.PanFrameRight.win, Scr.FvwmCursors[CRS_RIGHT_EDGE]);
	/* migo (04/Nov/1999): don't annoy users which use xsetroot */
	if (index == CRS_ROOT)
	{
		SafeDefineCursor(Scr.Root, Scr.FvwmCursors[CRS_ROOT]);
	}

	return;
}

/* Defines in which cases fvwm "grab" the cursor during execution of certain
 * functions. */
void CMD_BusyCursor(F_CMD_ARGS)
{
	char *option = NULL;
	char *optstring = NULL;
	char *args = NULL;
	int flag = -1;
	char *optlist[] = {
		"read", "wait", "modulesynchronous", "dynamicmenu", "*", NULL
	};

	while (action  && *action != '\0')
	{
		action = GetQuotedString(
			action, &optstring, ",", NULL, NULL, NULL);
		if (!optstring)
		{
			break;
		}

		args = GetNextToken(optstring, &option);
		if (!option)
		{
			free(optstring);
			break;
		}

		flag = ParseToggleArgument(args, NULL, -1, True);
		free(optstring);
		if (flag == -1)
		{
			fvwm_msg(ERR, "BusyCursor",
				 "error in boolean specification");
			free(option);
			break;
		}

		switch (GetTokenIndex(option, optlist, 0, NULL))
		{
		case 0: /* read */
			if (flag)
			{
				Scr.BusyCursor |= BUSY_READ;
			}
			else
			{
				Scr.BusyCursor &= ~BUSY_READ;
			}
			break;

		case 1: /* wait */
			if (flag)
			{
				Scr.BusyCursor |= BUSY_WAIT;
			}
			else
			{
				Scr.BusyCursor &= ~BUSY_WAIT;
			}
			break;

		case 2: /* modulesynchronous */
			if (flag)
			{
				Scr.BusyCursor |= BUSY_MODULESYNCHRONOUS;
			}
			else
			{
				Scr.BusyCursor &= ~BUSY_MODULESYNCHRONOUS;
			}
			break;

		case 3: /* dynamicmenu */
			if (flag)
			{
				Scr.BusyCursor |= BUSY_DYNAMICMENU;
			}
			else
			{
				Scr.BusyCursor &= ~BUSY_DYNAMICMENU;
			}
			break;

		case 4: /* "*" */
			if (flag)
			{
				Scr.BusyCursor |= BUSY_ALL;
			}
			else
			{
				Scr.BusyCursor &= ~(BUSY_ALL);
			}
			break;

		default:
			fvwm_msg(ERR, "BusyCursor", "unknown context '%s'",
				 option);
			break;
		}
		free(option);
	}

	return;
}
