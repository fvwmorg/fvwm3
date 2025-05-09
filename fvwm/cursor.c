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
#include "libs/FEvent.h"
#include "libs/fvwmsignal.h"
#include "libs/log.h"
#include "fvwm.h"
#include "libs/PictureImageLoader.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "virtual.h"
#include "cursor.h"
#include "menus.h"

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#define MAX_BARRIERS 16
void create_barrier(int, int, int, int);
void destroy_barrier(int);
void destroy_all_barriers(void);
void destroy_barrier_n(int);

PointerBarrier barriers_l[MAX_BARRIERS];
PointerBarrier barriers_r[MAX_BARRIERS];
PointerBarrier barriers_t[MAX_BARRIERS];
PointerBarrier barriers_b[MAX_BARRIERS];
int num_barriers = 0;

void create_barrier(int x1, int y1, int x2, int y2)
{
	if (num_barriers >= MAX_BARRIERS) {
		fvwm_debug(__func__, "Too many barriers. Freeing previous...");
		destroy_all_barriers();
		return;
	}

	barriers_l[num_barriers] = XFixesCreatePointerBarrier(
		dpy, Scr.Root, x1, y1, x1, y2, 0, 0, NULL);
	barriers_r[num_barriers] = XFixesCreatePointerBarrier(
		dpy, Scr.Root, x2, y1, x2, y2, 0, 0, NULL);
	barriers_t[num_barriers] = XFixesCreatePointerBarrier(
		dpy, Scr.Root, x1, y1, x2, y1, 0, 0, NULL);
	barriers_b[num_barriers] = XFixesCreatePointerBarrier(
		dpy, Scr.Root, x1, y2, x2, y2, 0, 0, NULL);
	num_barriers++;
}

void destroy_barrier(int n)
{
	XFixesDestroyPointerBarrier(dpy, barriers_l[n]);
	XFixesDestroyPointerBarrier(dpy, barriers_r[n]);
	XFixesDestroyPointerBarrier(dpy, barriers_t[n]);
	XFixesDestroyPointerBarrier(dpy, barriers_b[n]);
}

void destroy_all_barriers(void)
{
	int i;

	for (i = 0; i < num_barriers; i++) {
		destroy_barrier(i);
	}

	num_barriers = 0;
}

void destroy_barrier_n(int n)
{
	int i;

	if (num_barriers == 0)
		return;

	if (n < 0)
		n += num_barriers;
	if (n < 0 || n >= num_barriers) {
		fvwm_debug(__func__, "Invalid barrier number: %d", n);
		return;
	}

	destroy_barrier(n);
	num_barriers--;
	for (i = n; i < num_barriers; i++) {
		barriers_l[i] = barriers_l[i+1];
		barriers_r[i] = barriers_r[i+1];
		barriers_t[i] = barriers_t[i+1];
		barriers_b[i] = barriers_b[i+1];
	}
}

#else
#define create_barrier(a, b, c, d)
#define destroy_barrier(n);
#define destroy_all_barriers()
#define destroy_barrier_n(a)
#endif
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

static int cursor_move_relative(int x_unit, int y_unit, int *x, int *y)
{
	int virtual_x, virtual_y;
	int x_pages, y_pages;
	int width = monitor_get_all_widths();
	int height = monitor_get_all_heights();
	struct monitor *m = monitor_get_current();

	FQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
			  x, y, &JunkX, &JunkY, &JunkMask);

	*x += x_unit;
	*y += y_unit;

	virtual_x = m->virtual_scr.Vx;
	virtual_y = m->virtual_scr.Vy;
	if (*x >= 0)
	{
		x_pages = *x / width;
	}
	else
	{
		x_pages = ((*x + 1) / width) - 1;
	}
	virtual_x += x_pages * width;
	*x -= x_pages * width;
	if (virtual_x < 0)
	{
		*x += virtual_x;
		virtual_x = 0;
	}
	else if (virtual_x > m->virtual_scr.VxMax)
	{
		*x += virtual_x - m->virtual_scr.VxMax;
		virtual_x = m->virtual_scr.VxMax;
	}

	if (*y >= 0)
	{
		y_pages = *y / height;
	}
	else
	{
		y_pages = ((*y + 1) / height) - 1;
	}
	virtual_y += y_pages * height;
	*y -= y_pages * height;

	if (virtual_y < 0)
	{
		*y += virtual_y;
		virtual_y = 0;
	}
	else if (virtual_y > m->virtual_scr.VyMax)
	{
		*y += virtual_y - m->virtual_scr.VyMax;
		virtual_y = m->virtual_scr.VyMax;
	}

	/* TA:  (2010/12/19):  Only move to the new page if scrolling is
	 * enabled and the viewport is able to change based on where the
	 * pointer is.
	 */
	if ((virtual_x != m->virtual_scr.Vx && m->virtual_scr.EdgeScrollX != 0) ||
	    (virtual_y != m->virtual_scr.Vy && m->virtual_scr.EdgeScrollY != 0))
	{
		MoveViewport(m, virtual_x, virtual_y, True);
	}

	/* TA:  (2010/12/19):  If the cursor is about to enter a pan-window, or
	 * is one, or the cursor's next step is to go beyond the page
	 * boundary, stop the cursor from moving in that direction, *if* we've
	 * disallowed edge scrolling.
	 *
	 * Whilst this stops the cursor short of the edge of the screen in a
	 * given direction, this is the desired behaviour.
	 */
	if (m->virtual_scr.EdgeScrollX == 0 &&
	    (*x >= width || *x + x_unit >= width))
		return 0;

	if (m->virtual_scr.EdgeScrollY == 0 &&
	    (*y >= height || *y + y_unit >= height))
		return 0;

	return 1;
}

static void cursor_move_screen(struct monitor *m, int x_unit, int y_unit,
			       int *x, int *y)
{
	*x = x_unit % m->si->w;
	*y = y_unit % m->si->h;

	if (*x < 0)
		*x += m->si->w;
	if (*y < 0)
		*y += m->si->h;

	*x += m->si->x;
	*y += m->si->y;

	return;
}

/* ---------------------------- interface functions ------------------------ */

/* CreateCursors - Loads fvwm cursors */
Cursor *CreateCursors(Display *disp)
{
	int i;
	/* define cursors */
	cursors[0] = None;
	for (i = 1; i < CRS_MAX; i++)
	{
		cursors[i] = XCreateFontCursor(disp, default_cursors[i]);
	}

	return cursors;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_CursorMove(F_CMD_ARGS)
{
	int x, y;
	int val1, val2;
	int val1_unit = monitor_get_all_widths();
	int val2_unit = monitor_get_all_heights();
	struct monitor	*m = NULL;
	char *option;

	/* Check if screen option is provided. */
	if ((option = PeekToken(action, NULL)) != NULL &&
	    StrEquals(option, "screen"))
	{
		option = PeekToken(action, &action); /* Skip literal screen. */
		option = PeekToken(action, &action);
		if ((m = monitor_resolve_name(option)) == NULL) {
			fvwm_debug(__func__, "Invalid screen: %s", option);
			return;
		}
		val1_unit = m->si->w;
		val2_unit = m->si->h;
	}

	if (GetTwoPercentArguments(
	    action, &val1, &val2, &val1_unit, &val2_unit) != 2)
	{
		fvwm_debug(__func__, "CursorMove needs 2 arguments");
		return;
	}

	val1 *= val1_unit / 100;
	val2 *= val2_unit / 100;

	if (m == NULL) {
		if (cursor_move_relative(val1, val2, &x, &y) == 0)
			return;
	} else {
		cursor_move_screen(m, val1, val2, &x, &y);
	}

	FWarpPointerUpdateEvpos(
		exc->x.elast, dpy, None, Scr.Root, 0, 0,
		monitor_get_all_widths(), monitor_get_all_heights(), x, y);

	return;
}

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
	struct monitor	*m = NULL;

	cursor = 0;
	cname = PeekToken(action, &action);
	if (!cname)
	{
		fvwm_debug(__func__, "Bad cursor style");

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
		fvwm_debug(__func__, "Unknown cursor name %s", cname);

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
			fvwm_debug(__func__, "Bad cursor number %s",
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
				fvwm_debug(__func__,
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
		fvwm_debug(__func__, "Cannot load cursor: %s",
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

	/* Set the cursors on all monitors. */
	RB_FOREACH(m, monitors, &monitor_q) {
		SafeDefineCursor(m->PanFrameTop.win,
		   Scr.FvwmCursors[CRS_TOP_EDGE]);
		SafeDefineCursor(m->PanFrameBottom.win,
		   Scr.FvwmCursors[CRS_BOTTOM_EDGE]);
		SafeDefineCursor(m->PanFrameLeft.win,
		   Scr.FvwmCursors[CRS_LEFT_EDGE]);
		SafeDefineCursor(
			m->PanFrameRight.win,
			Scr.FvwmCursors[CRS_RIGHT_EDGE]);
	}
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
			fvwm_debug(__func__,
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
			fvwm_debug(__func__, "unknown context '%s'",
				   option);
			break;
		}
		free(option);
	}

	return;
}

void CMD_CursorBarrier(F_CMD_ARGS)
{
	int val[4] = {0, 0, 0, 0};
	int x1, y1, x2, y2;
	rectangle r =
		{0, 0, monitor_get_all_widths(), monitor_get_all_heights()};
	bool is_coords = false;
	char *option;
	struct monitor *m = NULL;

#if !defined(HAVE_XFIXES)
	SUPPRESS_UNUSED_VAR_WARNING(x1);
	SUPPRESS_UNUSED_VAR_WARNING(y1);
	SUPPRESS_UNUSED_VAR_WARNING(x2);
	SUPPRESS_UNUSED_VAR_WARNING(y2);
	return;
#endif

	/* Note, if option is matched, the matched option must be skipped
	 * with PeekToken(action, &action) to stop infinite loop.
	 */
	while ((option = PeekToken(action, NULL)) != NULL) {
		if (StrEquals(option, "screen")) {
			option = PeekToken(action, &action); /* Skip */
			option = PeekToken(action, &action);
			if ((m = monitor_resolve_name(option)) == NULL) {
				fvwm_debug(__func__, "Invalid screen: %s",
					option);
				return;
			}
			r.x = m->si->x;
			r.y = m->si->y;
			r.width = m->si->w;
			r.height = m->si->h;
		} else if (StrEquals(option, "destroy")) {
			int n;

			option = PeekToken(action, &action); /* Skip */
			option = PeekToken(action, &action);
			if (option != NULL && sscanf(option, "%d", &n) == 1) {
				destroy_barrier_n(n);
			} else {
				destroy_all_barriers();
			}
			XSync(dpy, False);
			return;
		} else if (StrEquals(option, "coords")) {
			option = PeekToken(action, &action); /* Skip */
			is_coords = true;
		} else {
			int i;
			int unit[4] = {r.width, r.height, r.width, r.height};

			for (i = 0; i < 4; i++) {
				option = PeekToken(action, &action);
				if (GetOnePercentArgument(
				    option, &val[i], &unit[i]) == 0
				    || val[i] < 0) {
					fvwm_debug(__func__,
						"Invalid coordinates.");
					return;
				}
				val[i] = val[i] * unit[i] / 100;
			}
			break;
		}
	}

	if (is_coords) {
		x1 = r.x + val[0];
		y1 = r.y + val[1];
		x2 = r.x + val[2];
		y2 = r.y + val[3];
	} else {
		x1 = r.x + val[0];
		y1 = r.y + val[1];
		x2 = r.x + r.width - val[2];
		y2 = r.y + r.height - val[3];
	}

	create_barrier(x1, y1, x2, y2);
	XSync(dpy, False);
}
