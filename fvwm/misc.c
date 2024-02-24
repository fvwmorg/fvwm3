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
#include <stdarg.h>

#include "libs/log.h"
#include "libs/fvwm_x11.h"
#include "libs/ftime.h"
#include "libs/Parse.h"
#include "libs/Target.h"
#include "libs/fvwmrect.h"
#include "libs/FEvent.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "cursor.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "events.h"
#include "eventmask.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

#define GRAB_EVMASK (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | \
	PointerMotionMask | EnterWindowMask | LeaveWindowMask)

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static int grab_count[GRAB_MAXVAL] = { 1, 1, 0, 0, 0, 0, 0 };

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/*
 * Change the appearance of the grabbed cursor.
 */
static void change_grab_cursor(int cursor)
{
	if (cursor != None)
	{
		XChangeActivePointerGrab(
			dpy, GRAB_EVMASK, Scr.FvwmCursors[cursor], CurrentTime);
	}

	return;
}

/* ---------------------------- interface functions ------------------------ */

int GetTwoArguments(
	char *action, int *val1, int *val2, int *val1_unit, int *val2_unit)
{
	*val1_unit = monitor_get_all_widths();
	*val2_unit = monitor_get_all_heights();

	return GetTwoPercentArguments(action, val1, val2, val1_unit, val2_unit);
}

/*
 * Grab the pointer.
 * grab_context: GRAB_NORMAL, GRAB_BUSY, GRAB_MENU, GRAB_BUSYMENU,
 * GRAB_PASSIVE.
 * GRAB_STARTUP and GRAB_NONE are used at startup but are not made
 * to be grab_context.
 * GRAB_PASSIVE does not actually grab, but only delays the following ungrab
 * until the GRAB_PASSIVE is released too.
 */
#define DEBUG_GRAB 0
#if DEBUG_GRAB
void print_grab_stats(char *text)
{
	int i;

	fvwm_debug(__func__, "grab_stats (%s):", text);
	for (i = 0; i < GRAB_MAXVAL; i++)
	{
		fvwm_debug(__func__, " %d", grab_count[i]);
	}
	fvwm_debug(__func__, " \n");

	return;
}
#endif

Bool GrabEm(int cursor, int grab_context)
{
	int i = 0;
	int val = 0;
	int rep;
	Window grab_win;
	extern Window PressedW;

	if (grab_context <= GRAB_STARTUP || grab_context >= GRAB_MAXVAL)
	{
		fvwm_debug(__func__, "Bug: Called with illegal context %d",
			   grab_context);
		return False;
	}

	if (grab_context == GRAB_PASSIVE)
	{
		grab_count[grab_context]++;
		grab_count[GRAB_ALL]++;
		return True;
	}

	if (grab_count[GRAB_ALL] > grab_count[GRAB_PASSIVE])
	{
		/* already grabbed, just change the grab cursor */
		if (grab_context == GRAB_FREEZE_CURSOR)
		{
			if (XGrabPointer(
				    dpy, (PressedW != None) ?
				    PressedW : Scr.NoFocusWin, True,
				    GRAB_EVMASK, GrabModeAsync, GrabModeAsync,
				    None, Scr.FvwmCursors[CRS_DEFAULT],
				    CurrentTime) != GrabSuccess)
			{
				return False;
			}
			return True;
		}
		grab_count[grab_context]++;
		grab_count[GRAB_ALL]++;
		if (grab_context != GRAB_BUSY || grab_count[GRAB_STARTUP] == 0)
		{
			change_grab_cursor(cursor);
		}
		return True;
	}

	/* move the keyboard focus prior to grabbing the pointer to
	 * eliminate the enterNotify and exitNotify events that go
	 * to the windows. But GRAB_BUSY. */
	switch (grab_context)
	{
	case GRAB_BUSY:
		if ( Scr.Hilite != NULL )
		{
			grab_win = FW_W(Scr.Hilite);
		}
		else
		{
			grab_win = Scr.Root;
		}
		/* retry to grab the busy cursor only once */
		rep = 2;
		break;
	case GRAB_PASSIVE:
		/* cannot happen */
		return False;
	case GRAB_FREEZE_CURSOR:
		grab_win = (PressedW != None) ? PressedW : Scr.NoFocusWin;
		rep = 2;
		break;
	default:
		grab_win = Scr.Root;
		rep = NUMBER_OF_GRAB_ATTEMPTS;
		break;
	}

	XFlush(dpy);
	while (i < rep &&
	       (val = XGrabPointer(
		      dpy, grab_win, True, GRAB_EVMASK, GrabModeAsync,
		      GrabModeAsync, None,
		      (grab_context == GRAB_FREEZE_CURSOR) ?
		      None : Scr.FvwmCursors[cursor], CurrentTime) !=
		GrabSuccess))
	{
		switch (val)
		{
		case GrabInvalidTime:
		case GrabNotViewable:
			/* give up */
			i += rep;
			break;
		case GrabSuccess:
			break;
		case AlreadyGrabbed:
		case GrabFrozen:
		default:
			/* If you go too fast, other windows may not get a
			 * chance to release any grab that they have. */
			i++;
			if (grab_context == GRAB_FREEZE_CURSOR)
			{
				break;
			}
			if (i < rep)
			{
				usleep(1000 * TIME_BETWEEN_GRAB_ATTEMPTS);
			}
			break;
		}
	}
	XFlush(dpy);

	/* If we fall out of the loop without grabbing the pointer, its
	 * time to give up */
	if (val != GrabSuccess)
	{
		return False;
	}
	grab_count[grab_context]++;
	grab_count[GRAB_ALL]++;
#if DEBUG_GRAB
	print_grab_stats("grabbed");
#endif
	return True;
}


/*
 *
 * UnGrab the pointer
 *
 */
Bool UngrabEm(int ungrab_context)
{
	if (ungrab_context <= GRAB_ALL || ungrab_context >= GRAB_MAXVAL)
	{
		fvwm_debug(__func__, "Bug: Called with illegal context %d",
			   ungrab_context);
		return False;
	}

	if (grab_count[ungrab_context] == 0 || grab_count[GRAB_ALL] == 0)
	{
		/* context is not grabbed */
		return False;
	}

	XFlush(dpy);
	grab_count[ungrab_context]--;
	grab_count[GRAB_ALL]--;
	if (grab_count[GRAB_ALL] > 0)
	{
		int new_cursor = None;

		/* there are still grabs left - switch grab cursor */
		switch (ungrab_context)
		{
		case GRAB_NORMAL:
		case GRAB_BUSY:
		case GRAB_MENU:
			if (grab_count[GRAB_BUSYMENU] > 0)
			{
				new_cursor = CRS_WAIT;
			}
			else if (grab_count[GRAB_BUSY] > 0)
			{
				new_cursor = CRS_WAIT;
			}
			else if (grab_count[GRAB_MENU] > 0)
			{
				new_cursor = CRS_MENU;
			}
			else
			{
				new_cursor = None;
			}
			break;
		case GRAB_BUSYMENU:
			/* switch back from busymenu cursor to normal menu
			 * cursor */
			new_cursor = CRS_MENU;
			break;
		default:
			new_cursor = None;
			break;
		}
		if (grab_count[GRAB_ALL] > grab_count[GRAB_PASSIVE])
		{
#if DEBUG_GRAB
			print_grab_stats("-restore");
#endif
			change_grab_cursor(new_cursor);
		}
	}
	else
	{
#if DEBUG_GRAB
		print_grab_stats("-ungrab");
#endif
		XUngrabPointer(dpy, CurrentTime);
	}
	XFlush(dpy);

	return True;
}

void fvwm_msg_report_app(void)
{
	fvwm_debug(__func__,
		   "    If you are having a problem with the application, send a"
		   " bug report\n"
		   "    with this message included to the application owner.\n"
		   "    There is no need to notify fvwm-workers@fvwm.org.\n");

	return;
}

void fvwm_msg_report_app_and_workers(void)
{
	fvwm_debug(__func__,
		   "    If you are having a problem with the application, send"
		   " a bug report with\n"
		   "    this message included to the application owner and"
		   " notify\n"
		   "    fvwm-workers@fvwm.org.\n");

	return;
}

/* Store the last item that was added with '+' */
void set_last_added_item(last_added_item_t type, void *item)
{
	Scr.last_added_item.type = type;
	Scr.last_added_item.item = item;

	return;
}

/* some fancy font handling stuff */
void NewFontAndColor(FlocaleFont *flf, Pixel color, Pixel backcolor)
{
	Globalgcm = GCForeground | GCBackground;
	if (flf->font)
	{
		Globalgcm |= GCFont;
		Globalgcv.font = flf->font->fid;
	}
	Globalgcv.foreground = color;
	Globalgcv.background = backcolor;
	XChangeGC(dpy,Scr.TitleGC,Globalgcm,&Globalgcv);

	return;
}


/*
 *
 * For menus, move, and resize operations, we can effect keyboard
 * shortcuts by warping the pointer.
 *
 */
void Keyboard_shortcuts(
	XEvent *ev, FvwmWindow *fw, int *x_defect, int *y_defect,
	int ReturnEvent)
{
	int x_move_size = 0;
	int y_move_size = 0;

	if (fw)
	{
		x_move_size = fw->hints.width_inc;
		y_move_size = fw->hints.height_inc;
	}
	fvwmlib_keyboard_shortcuts(
		dpy, Scr.screen, ev, x_move_size, y_move_size, x_defect,
		y_defect, ReturnEvent);

	return;
}


/*
 *
 * Check if the given FvwmWindow structure still points to a valid window.
 *
 */

Bool check_if_fvwm_window_exists(FvwmWindow *fw)
{
	FvwmWindow *t;

	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if (t == fw)
			return True;
	}
	return False;
}

/* rounds x down to the next multiple of m */
int truncate_to_multiple (int x, int m)
{
	return (x < 0) ? (m * (((x + 1) / m) - 1)) : (m * (x / m));
}

Bool IsRectangleOnThisPage(struct monitor *m, const rectangle *rec, int desk)
{
#if 0
	fprintf(stderr, "%s: I think I want monitor '%s'\n", __func__,
		m->name);
	fprintf(stderr, "%s: fw desk is: %d, mon desk is: %d\n", __func__,
		desk, m->virtual_scr.CurrentDesk);
	fprintf(stderr, "%s: fw rec: {x: %d, y: %d, w: %d, h: %d}\n", __func__,
		rec->x, rec->y, rec->width, rec->height);
	fprintf(stderr, "%s: mon: {MyDH: %d, MyDW: %d}\n", __func__,
		m->virtual_scr.MyDisplayWidth, m->virtual_scr.MyDisplayHeight);
#endif
	if (m == NULL)
		return (False);

	if (rec->x + (signed int)rec->width > 0 &&
		(rec->x < 0 || rec->x < monitor_get_all_widths()) &&
		rec->y + (signed int)rec->height > 0 &&
		(rec->y < 0 || rec->y < monitor_get_all_heights())) {
			return (True);
	}
	return (False);
}

/* returns the FvwmWindow that contains the pointer or NULL if none */
FvwmWindow *get_pointer_fvwm_window(void)
{
	int x,y;
	Window win;
	Window ancestor;
	FvwmWindow *t;

	FQueryPointer(
		    dpy, Scr.Root, &JunkRoot, &win, &JunkX, &JunkY,
		    &x, &y, &JunkMask);
	for (t = NULL ; win != Scr.Root && win != None; win = ancestor)
	{
		Window root = None;
		Window *children;
		unsigned int nchildren;

		if (XFindContext(dpy, win, FvwmContext, (caddr_t *) &t) !=
		    XCNOENT)
		{
			/* found a matching window context */
			return t;
		}
		/* get next higher ancestor window */
		children = NULL;
		if (!XQueryTree(
			    dpy, win, &root, &ancestor, &children, &nchildren))
		{
			return NULL;
		}
		if (children)
		{
			XFree(children);
		}
	}

	return t;
}


/* Returns the current X server time */
Time get_server_time(void)
{
	XEvent xev;
	XSetWindowAttributes attr;

	/* add PropChange to NoFocusWin events */
	attr.event_mask = PropertyChangeMask;
	XChangeWindowAttributes (dpy, Scr.NoFocusWin, CWEventMask, &attr);
	/* provoke an event */
	XChangeProperty(
		dpy, Scr.NoFocusWin, XA_WM_CLASS, XA_STRING, 8, PropModeAppend,
		NULL, 0);
	FWindowEvent(dpy, Scr.NoFocusWin, PropertyChangeMask, &xev);
	attr.event_mask = XEVMASK_NOFOCUSW;
	XChangeWindowAttributes(dpy, Scr.NoFocusWin, CWEventMask, &attr);

	return xev.xproperty.time;
}

void print_g(char *text, rectangle *g)
{
	fvwm_debug(__func__, "%s: ", (text != NULL) ? text : "");
	if (g == NULL)
	{
		fvwm_debug(__func__, "(null)\n");

		return;
	}
	fvwm_debug(__func__, "%4d %4d %4dx%4d (%4d - %4d, %4d - %4d)\n",
		   g->x, g->y, g->width, g->height,
		   g->x, g->x + g->width, g->y, g->y + g->height);

	return;
}