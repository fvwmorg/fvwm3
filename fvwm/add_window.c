/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */
/* This module is based on Twm, but has been siginificantly modified
 * by Rob Nation
 */
/*
 *	 Copyright 1988 by Evans & Sutherland Computer Corporation,
 *			    Salt Lake City, Utah
 *  Portions Copyright 1989 by the Massachusetts Institute of Technology
 *			  Cambridge, Massachusetts
 *
 *			     All Rights Reserved
 *
 *    Permission to use, copy, modify, and distribute this software and
 *    its documentation	 for  any  purpose  and	 without  fee is hereby
 *    granted, provided that the above copyright notice appear	in  all
 *    copies and that both  that  copyright  notice  and  this	permis-
 *    sion  notice appear in supporting	 documentation,	 and  that  the
 *    names of Evans & Sutherland and M.I.T. not be used in advertising
 *    in publicity pertaining to distribution of the  software	without
 *    specific, written prior permission.
 *
 *    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD
 *    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-
 *    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR
 *    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-
 *    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA
 *    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION	 WITH  THE  USE
 *    OR PERFORMANCE OF THIS SOFTWARE.
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/FScreen.h"
#include "libs/Picture.h"
#include "libs/PictureUtils.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "libs/Grab.h"
#include "libs/Strings.h"
#include "libs/XResource.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "execcontext.h"
#include "commands.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "add_window.h"
#include "events.h"
#include "eventhandler.h"
#include "eventmask.h"
#include "module_interface.h"
#include "stack.h"
#include "update.h"
#include "style.h"
#include "icons.h"
#include "ewmh.h"
#include "focus.h"
#include "placement.h"
#include "geometry.h"
#include "session.h"
#include "move_resize.h"
#include "borders.h"
#include "frame.h"
#include "colormaps.h"
#include "decorations.h"
#include "functions.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

char NoName[] = "Untitled"; /* name if no name in XA_WM_NAME */
char NoClass[] = "NoClass"; /* Class if no res_class in class hints */
char NoResource[] = "NoResource"; /* Class if no res_name in class hints */

/* ---------------------------- local functions ---------------------------- */

static void delete_client_context(FvwmWindow *fw)
{
	FvwmWindow *cw;

	/* We can not simply delete the context.  X might have reused the
	 * window structure so we would delete the context that was established
	 * by another FvwmWindow structure in the mean time. */
	if (XFindContext(
		    dpy, FW_W(fw), FvwmContext, (caddr_t *)&cw) !=
	    XCNOENT && cw == fw)
	{
		XDeleteContext(dpy, FW_W(fw), FvwmContext);
	}

	return;
}

/*
 *
 *  Procedure:
 *	CaptureOneWindow
 *	CaptureAllWindows
 *
 *   Decorates windows at start-up and during recaptures
 *
 */

static void CaptureOneWindow(
	const exec_context_t *exc, FvwmWindow *fw, Window window,
	Window keep_on_top_win, Window parent_win, Bool is_recapture)
{
	Window w;
	unsigned long data[1];
	initial_window_options_t win_opts;
	evh_args_t ea;
	exec_context_changes_t ecc;
	XEvent e;

	if (fw == NULL)
	{
		return;
	}
	if (IS_SCHEDULED_FOR_DESTROY(fw))
	{
		/* Fvwm might crash in complex functions if we really try to
		 * the dying window here because AddWindow() may fail and leave
		 * a destroyed window in some structures.  By the way, it is
		 * pretty useless to recapture a window that will vanish in a
		 * moment. */
		return;
	}
	/* Grab the server to make sure the window does not die during the
	 * recapture. */
	MyXGrabServer(dpy);
	if (
		!XGetGeometry(
			dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
			(unsigned int*)&JunkWidth, (unsigned int*)&JunkHeight,
			(unsigned int*)&JunkBW, (unsigned int*)&JunkDepth))
	{
		/* The window has already died, do not recapture it! */
		MyXUngrabServer(dpy);
		return;
	}
	if (XFindContext(dpy, window, FvwmContext, (caddr_t *)&fw) != XCNOENT)
	{
		Bool is_mapped = IS_MAPPED(fw);

		memset(&win_opts, 0, sizeof(win_opts));
		win_opts.initial_state = DontCareState;
		win_opts.flags.do_override_ppos = 1;
		win_opts.flags.is_recapture = 1;
		if (IS_ICONIFIED(fw))
		{
			win_opts.initial_state = IconicState;
			win_opts.flags.is_iconified_by_parent =
				IS_ICONIFIED_BY_PARENT(fw);
		}
		else
		{
			win_opts.initial_state = NormalState;
			win_opts.flags.is_iconified_by_parent = 0;
			if (Scr.CurrentDesk != fw->Desk)
			{
				SetMapStateProp(fw, NormalState);
			}
		}
		data[0] = (unsigned long) fw->Desk;
		XChangeProperty(
			dpy, FW_W(fw), _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32,
			PropModeReplace, (unsigned char *) data, 1);

		XSelectInput(dpy, FW_W(fw), NoEventMask);
		w = FW_W(fw);
		XUnmapWindow(dpy, FW_W_FRAME(fw));
		border_undraw_decorations(fw);
		RestoreWithdrawnLocation(fw, is_recapture, parent_win);
		SET_DO_REUSE_DESTROYED(fw, 1); /* RBW - 1999/03/20 */
		destroy_window(fw);
		win_opts.flags.is_menu =
			(is_recapture && fw != NULL && IS_TEAR_OFF_MENU(fw));


		fev_make_null_event(&e, dpy);
		e.xmaprequest.window = w;
		e.xmaprequest.parent = Scr.Root;
		ecc.x.etrigger = &e;
		ecc.w.fw = NULL;
		ecc.w.w = w;
		ecc.w.wcontext = C_ROOT;
		ea.exc = exc_clone_context(
			exc, &ecc, ECC_ETRIGGER | ECC_FW | ECC_W |
			ECC_WCONTEXT);
		HandleMapRequestKeepRaised(&ea, keep_on_top_win, fw, &win_opts);
		exc_destroy_context(ea.exc);

		/* HandleMapRequestKeepRaised may have destroyed the fw if the
		 * window vanished while in AddWindow(), so don't access fw
		 * anymore before checking if it is a valid window. */
		if (check_if_fvwm_window_exists(fw))
		{
			if (!fFvwmInStartup)
			{
				SET_MAP_PENDING(fw, 0);
				SET_MAPPED(fw, is_mapped);
			}
		}
	}
	MyXUngrabServer(dpy);

	return;
}

/* Put a transparent window all over the screen to hide what happens below. */
static void hide_screen(
	Bool do_hide, Window *ret_hide_win, Window *ret_parent_win)
{
	static Bool is_hidden = False;
	static Window hide_win = None;
	static Window parent_win = None;
	XSetWindowAttributes xswa;
	unsigned long valuemask;

	if (do_hide == is_hidden)
	{
		/* nothing to do */
		if (ret_hide_win)
		{
			*ret_hide_win = hide_win;
		}
		if (ret_parent_win)
		{
			*ret_parent_win = parent_win;
		}

		return;
	}
	is_hidden = do_hide;
	if (do_hide)
	{
		xswa.override_redirect = True;
		xswa.cursor = Scr.FvwmCursors[CRS_WAIT];
		xswa.backing_store = NotUseful;
		xswa.save_under = False;
		xswa.background_pixmap = None;
		valuemask = CWOverrideRedirect | CWCursor | CWSaveUnder |
			CWBackingStore | CWBackPixmap;
		hide_win = XCreateWindow(
			dpy, Scr.Root, 0, 0, Scr.MyDisplayWidth,
			Scr.MyDisplayHeight, 0, Pdepth, InputOutput,
			Pvisual, valuemask, &xswa);
		if (hide_win)
		{
			/* When recapturing, all windows are reparented to this
			 * window. If they are reparented to the root window,
			 * they will flash over the hide_win with XFree.  So
			 * reparent them to an unmapped window that looks like
			 * the root window. */
			parent_win = XCreateWindow(
				dpy, Scr.Root, 0, 0, Scr.MyDisplayWidth,
				Scr.MyDisplayHeight, 0, CopyFromParent,
				InputOutput, CopyFromParent, valuemask, &xswa);
			if (!parent_win)
			{
				XDestroyWindow(dpy, hide_win);
				hide_win = None;
			}
			else
			{
				XMapWindow(dpy, hide_win);
				XFlush(dpy);
			}
		}
	}
	else
	{
		if (hide_win != None)
		{
			XDestroyWindow(dpy, hide_win);
		}
		if (parent_win != None)
		{
			XDestroyWindow(dpy, parent_win);
		}
		XFlush(dpy);
		hide_win = None;
		parent_win = None;
	}
	if (ret_hide_win)
	{
		*ret_hide_win = hide_win;
	}
	if (ret_parent_win)
	{
		*ret_parent_win = parent_win;
	}

	return;
}

/*
 *
 *  Procedure:
 *	MappedNotOverride - checks to see if we should really
 *		put a fvwm frame on the window
 *
 *  Returned Value:
 *	1   - go ahead and frame the window
 *	0   - don't frame the window
 *
 *  Inputs:
 *	w	- the window to check
 *
 */

static int MappedNotOverride(
	Window w, initial_window_options_t *win_opts)
{
	XWindowAttributes wa;
	Atom atype;
	int aformat;
	unsigned long nitems, bytes_remain;
	unsigned char *prop;

	win_opts->initial_state = DontCareState;
	if ((w==Scr.NoFocusWin)||(!XGetWindowAttributes(dpy, w, &wa)))
	{
		return 0;
	}
	if (XGetWindowProperty(
		    dpy,w,_XA_WM_STATE,0L,3L,False,_XA_WM_STATE,
		    &atype,&aformat,&nitems,&bytes_remain,&prop)==Success)
	{
		if (prop != NULL)
		{
			win_opts->initial_state = *(long *)prop;
			XFree(prop);
		}
	}
	if (wa.override_redirect == True)
	{
		XSelectInput(dpy, w, XEVMASK_ORW);
		XFlush(dpy);
	}

	return (((win_opts->initial_state == IconicState) ||
		 (wa.map_state != IsUnmapped)) &&
		(wa.override_redirect != True));
}

static void do_recapture(F_CMD_ARGS, Bool fSingle)
{
	FvwmWindow *fw = exc->w.fw;
	int event_types[5] = {
		ButtonPress, ButtonRelease, MotionNotify, KeyPress, KeyRelease
	};

	MyXGrabServer(dpy);
	if (fSingle)
	{
		CaptureOneWindow(
			exc, fw, FW_W(fw), None, None, True);
	}
	else
	{
		CaptureAllWindows(exc, True);
	}
	/* Throw away queued up events. We don't want user input during a
	 * recapture.  The window the user clicks in might disapper at the very
	 * same moment and the click goes through to the root window. Not good
	 */
	XAllowEvents(dpy, AsyncPointer, CurrentTime);
	discard_typed_events(5, event_types);
#ifdef DEBUG_STACK_RING
	verify_stack_ring_consistency();
#endif
	MyXUngrabServer(dpy);

	return;
}

static void setup_window_structure(
	FvwmWindow **pfw, Window w, FvwmWindow *ReuseWin)
{
	FvwmWindow save_state;
	FvwmWindow *savewin = NULL;

	/*
	  Allocate space for the FvwmWindow struct, or reuse an
	  old one (on Recapture).
	*/
	if (ReuseWin == NULL)
	{
		*pfw = xmalloc(sizeof(FvwmWindow));
	}
	else
	{
		*pfw = ReuseWin;
		savewin = &save_state;
		memcpy(savewin, ReuseWin, sizeof(FvwmWindow));
	}

	/*
	  RBW - 1999/05/28 - modify this when we implement the preserving of
	  various states across a Recapture. The Destroy function in misc.c may
	  also need tweaking, depending on what you want to preserve.
	  For now, just zap any old information, except the desk.
	*/
	memset(*pfw, '\0', sizeof(FvwmWindow));
	FW_W(*pfw) = w;
	if (savewin != NULL)
	{
		(*pfw)->Desk = savewin->Desk;
		SET_SHADED(*pfw, IS_SHADED(savewin));
		SET_USED_TITLE_DIR_FOR_SHADING(
			*pfw, USED_TITLE_DIR_FOR_SHADING(savewin));
		SET_SHADED_DIR(*pfw, SHADED_DIR(savewin));
		SET_NAME_CHANGED(*pfw,IS_NAME_CHANGED(savewin));
		(*pfw)->placed_by_button = savewin->placed_by_button;
		SET_PLACED_BY_FVWM(*pfw, IS_PLACED_BY_FVWM(savewin));
		SET_HAS_EWMH_WM_ICON_HINT(*pfw, HAS_EWMH_WM_ICON_HINT(savewin));
		(*pfw)->ewmh_mini_icon_width = savewin->ewmh_mini_icon_width;
		(*pfw)->ewmh_mini_icon_height = savewin->ewmh_mini_icon_height;
		(*pfw)->ewmh_icon_width = savewin->ewmh_icon_width;
		(*pfw)->ewmh_icon_height = savewin->ewmh_icon_height;
		(*pfw)->ewmh_hint_desktop = savewin->ewmh_hint_desktop;
		/* restore ewmh state */
		EWMH_SetWMState(savewin, True);
		SET_HAS_EWMH_INIT_WM_DESKTOP(
			*pfw, HAS_EWMH_INIT_WM_DESKTOP(savewin));
		SET_HAS_EWMH_INIT_FULLSCREEN_STATE(
			*pfw, HAS_EWMH_INIT_FULLSCREEN_STATE(savewin));
		SET_HAS_EWMH_INIT_HIDDEN_STATE(
			*pfw, HAS_EWMH_INIT_HIDDEN_STATE(savewin));
		SET_HAS_EWMH_INIT_MAXHORIZ_STATE(
			*pfw, HAS_EWMH_INIT_MAXHORIZ_STATE(savewin));
		SET_HAS_EWMH_INIT_MAXVERT_STATE(
			*pfw, HAS_EWMH_INIT_MAXVERT_STATE(savewin));
		SET_HAS_EWMH_INIT_SHADED_STATE(
			*pfw, HAS_EWMH_INIT_SHADED_STATE(savewin));
		SET_HAS_EWMH_INIT_STICKY_STATE(
			*pfw, HAS_EWMH_INIT_STICKY_STATE(savewin));
		CLEAR_USER_STATES(*pfw, ~0);
		SET_USER_STATES(*pfw, GET_USER_STATES(savewin));
	}
	else
	{
		/* make sure that new windows *remember* being shaded with
		 * title dir last */
		SET_USED_TITLE_DIR_FOR_SHADING(*pfw,1);
	}

	(*pfw)->cmap_windows = (Window *)NULL;
	if (FMiniIconsSupported)
	{
		(*pfw)->mini_pixmap_file = NULL;
		(*pfw)->mini_icon = NULL;
	}

	return;
}

static void setup_name_count(FvwmWindow *fw, Bool is_icon)
{
	FvwmWindow *t;
	int count = 0;
	int win_count;
	int win_count_counterpart;
	Bool done = False;
	FlocaleNameString *titlename, *title_counterpart;
	FlocaleNameString *t_titlename, *t_title_counterpart;

	titlename = (is_icon) ?
	&(fw->icon_name) : &(fw->name);

	title_counterpart = (is_icon) ?
	&(fw->name) : &(fw->icon_name);

	if (!titlename->name)
	{
		done = True;
	}

	if (titlename->name && title_counterpart->name &&
		strcmp(titlename->name, title_counterpart->name) == 0)
	{
		count = is_icon ? fw->icon_name_count :
			fw->name_count;
	}

	while (!done)
	{
		done = True;
		for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
		{
			if (t == fw)
			{
				continue;
			}
			win_count = is_icon ? t->icon_name_count :
				t->name_count;
			win_count_counterpart = is_icon ?
				t->name_count : t->icon_name_count;

			t_titlename = is_icon ? &(t->icon_name) :
				&(t->name);
			t_title_counterpart = is_icon ? &(t->name) :
				&(t->icon_name);

			if ((t_titlename->name &&
						strcmp(titlename->name,
							t_titlename->name) == 0 &&
						win_count == count) ||
					(t_title_counterpart->name &&
					 strcmp(t_title_counterpart->name,
						 titlename->name) == 0 &&
					 win_count_counterpart == count))

			{
				count++;
				done = False;
			}
		}
	}

	if (is_icon)
	{
		fw->icon_name_count = count;
	} else {
		fw->name_count = count;
	}

	return;
}

static char *interpolate_titleformat_name(
	int *ret_bits, FvwmWindow *fw, window_style *style, Bool is_icon)
{
	char stringbuf[MAX_VISIBLE_NAME_LEN] = "";

	/* Get the title format string.  This check should be redundant thanks
	 * to the checking done in style.c
	 */
	const char *format;
	int count;

	/* MAX_WINDOW_NAME_NUMBER is defined as "999" -- that's three
	 * characters maximum.  win_name_len must be 1 larger for null.
	 */
	char win_name_len[MAX_WINDOW_NAME_NUMBER_DIGITS+1];
	char w_id[12];

	*ret_bits = 0;
	if (is_icon)
	{
		format = (style->flags.has_icon_title_format_string) ?
			SGET_ICON_TITLE_FORMAT_STRING(*style) :
			DEFAULT_ICON_TITLE_FORMAT;
	}
	else
	{
		format = (style->flags.has_title_format_string) ?
			SGET_TITLE_FORMAT_STRING(*style) :
			DEFAULT_TITLE_FORMAT;
	}

	while (*format)
	{
		int pos;

		for (pos = 0; format[pos] && format[pos] != '%'; pos++)
		{
			/* nothing */
		}

		strncat(stringbuf, format, pos);
		format += pos;

		if (*format != '%')
			continue;

		format++;
		switch (*format)
		{
			case 'n':
				/* format contains the window name */
				*ret_bits |= 1;
				if (strlen(stringbuf) +
					strlen(fw->name.name) >
					MAX_VISIBLE_NAME_LEN)
				{
					fvwm_msg(WARN,
					"interpolate_titleformat_name",
					"Visible name is too long based on "
					"TitleFormat.  Not expanding further.");

					break;
				}

				strcat(stringbuf, fw->name.name);
				break;
			case 'c':
				if (strlen(stringbuf) +
					strlen(fw->class.res_class) >
					MAX_VISIBLE_NAME_LEN)
				{
					fvwm_msg(WARN,
					"interpolate_titleformat_name",
					"Visible name is too long based on "
					"TitleFormat.  Not expanding further.");

					break;
				}
				strcat(stringbuf, fw->class.res_class);
				break;
			case 'i':
				/* format contains icon name */
				*ret_bits |= 2;
				/* Not every application will have an icon
				 * name set; don't crash trying to dereference
				 * this if the name doesn't exist.
				 */
				if (fw->icon_name.name == NULL)
					break;

				if (strlen(stringbuf) +
					strlen(fw->icon_name.name) >
					MAX_VISIBLE_NAME_LEN)
				{
					fvwm_msg(WARN,
					"interpolate_titleformat_name",
					"Visible name is too long based on "
					"TitleFormat.  Not expanding further.");

					break;
				}

				strcat(stringbuf, fw->icon_name.name);
				break;
			case 'r':
				if (strlen(stringbuf) +
					strlen(fw->class.res_name) >
					MAX_VISIBLE_NAME_LEN)
				{
					fvwm_msg(WARN,
					"interpolate_titleformat_name",
					"Visible name is too long based on "
					"TitleFormat.  Not expanding further.");

					break;
				}
				strcat(stringbuf, fw->class.res_name);
				break;
			case 't':
				setup_name_count(fw, is_icon);
				count = is_icon ? fw->icon_name_count :
					fw->name_count;

				if (count > (MAX_WINDOW_NAME_NUMBER - 1))
					count = MAX_WINDOW_NAME_NUMBER - 1;

				sprintf(win_name_len, "%d", ++count);
				strcat(stringbuf, win_name_len);
				break;
			case 'I':
				sprintf(w_id, "0x%x", (int)FW_W(fw));
				strcat(stringbuf, w_id);
				break;
			case '%':
				strcat(stringbuf, "%");
				break;
			default:
				break;
		}
		if (*format)
			format++;
	}
	/* Now allocate our string. */
	return strdup(stringbuf);
}

static void setup_class_and_resource(FvwmWindow *fw)
{
	/* removing NoClass change for now... */
	fw->class.res_name = NoResource;
	fw->class.res_class = NoClass;
	XGetClassHint(dpy, FW_W(fw), &fw->class);
	if (fw->class.res_name == NULL)
	{
		fw->class.res_name = NoResource;
	}
	if (fw->class.res_class == NULL)
	{
		fw->class.res_class = NoClass;
	}
	FetchWmProtocols (fw);
	FetchWmColormapWindows (fw);

	return;
}

static void setup_window_attr(
	FvwmWindow *fw, XWindowAttributes *ret_attr)
{
	if (XGetWindowAttributes(dpy, FW_W(fw), ret_attr) == 0)
	{
		/* can't happen because fvwm has grabbed the server and does
		 * not destroy the window itself */
	}
	fw->attr_backup.backing_store = ret_attr->backing_store;
	fw->attr_backup.border_width = ret_attr->border_width;
	fw->attr_backup.depth = ret_attr->depth;
	fw->attr_backup.bit_gravity = ret_attr->bit_gravity;
	fw->attr_backup.is_bit_gravity_stored = 0;
	fw->attr_backup.visual = ret_attr->visual;
	fw->attr_backup.colormap = ret_attr->colormap;

	return;
}

static void destroy_window_font(FvwmWindow *fw)
{
	if (IS_WINDOW_FONT_LOADED(fw) && !USING_DEFAULT_WINDOW_FONT(fw) &&
	    fw->title_font != Scr.DefaultFont)
	{
		FlocaleUnloadFont(dpy, fw->title_font);
	}
	SET_WINDOW_FONT_LOADED(fw, 0);
	/* Fall back to default font. There are some race conditions when a
	 * window is destroyed and recaptured where an invalid font might be
	 * accessed  otherwise. */
	fw->title_font = Scr.DefaultFont;
	SET_USING_DEFAULT_WINDOW_FONT(fw, 1);

	return;
}

static void destroy_icon_font(FvwmWindow *fw)
{
	if (IS_ICON_FONT_LOADED(fw) && !USING_DEFAULT_ICON_FONT(fw) &&
	    fw->icon_font != Scr.DefaultFont)
	{
		FlocaleUnloadFont(dpy, fw->icon_font);
	}
	SET_ICON_FONT_LOADED(fw, 0);
	/* Fall back to default font (see comment above). */
	fw->icon_font = Scr.DefaultFont;
	SET_USING_DEFAULT_ICON_FONT(fw, 1);

	return;
}

static void adjust_fvwm_internal_windows(FvwmWindow *fw)
{
	if (fw == Scr.Hilite)
	{
		Scr.Hilite = NULL;
	}
	update_last_screen_focus_window(fw);
	restore_focus_after_unmap(fw, False);
	frame_destroyed_frame(FW_W(fw));
	if (fw == Scr.StolenFocusFvwmWin)
	{
		Scr.StolenFocusWin = None;
		Scr.StolenFocusFvwmWin = NULL;
	}
	if (Scr.focus_in_pending_window == fw)
	{
		Scr.focus_in_pending_window = NULL;
	}
	if (Scr.focus_in_requested_window == fw)
	{
		Scr.focus_in_requested_window = NULL;
	}
	if (Scr.cascade_window == fw)
	{
		Scr.cascade_window = NULL;
	}

	return;
}

static void broadcast_mini_icon(FvwmWindow *fw)
{
	if (!FMiniIconsSupported)
	{
		return;
	}
	if (fw->mini_pixmap_file && fw->mini_icon)
	{
		BroadcastFvwmPicture(
			M_MINI_ICON,
			FW_W(fw), FW_W_FRAME(fw), (unsigned long)fw,
			fw->mini_icon, fw->mini_pixmap_file);
	}

	return;
}

static void setup_mini_icon(FvwmWindow *fw, window_style *pstyle)
{
	FvwmPictureAttributes fpa;

	if (!FMiniIconsSupported)
	{
		return;
	}
	if (SHAS_MINI_ICON(&pstyle->flags))
	{
		fw->mini_pixmap_file = SGET_MINI_ICON_NAME(*pstyle);
	}
	else
	{
		fw->mini_pixmap_file = NULL;
	}
	if (fw->mini_pixmap_file)
	{
		fpa.mask = 0;
		fw->mini_icon = PCacheFvwmPicture(
			dpy, Scr.NoFocusWin, NULL, fw->mini_pixmap_file, fpa);
	}
	else
	{
		fw->mini_icon = NULL;
	}

	return;
}

/*
 * Copy icon size limits from window_style structure to FvwmWindow
 * structure.
 */
void setup_icon_size_limits(FvwmWindow *fw, window_style *pstyle)
{
	if (SHAS_ICON_SIZE_LIMITS(&pstyle->flags))
	{
		fw->min_icon_width = SGET_MIN_ICON_WIDTH(*pstyle);
		fw->min_icon_height = SGET_MIN_ICON_HEIGHT(*pstyle);
		fw->max_icon_width = SGET_MAX_ICON_WIDTH(*pstyle);
		fw->max_icon_height = SGET_MAX_ICON_HEIGHT(*pstyle);
		fw->icon_resize_type = SGET_ICON_RESIZE_TYPE(*pstyle);
	}
	else
	{
		fw->min_icon_width = MIN_ALLOWABLE_ICON_DIMENSION;
		fw->min_icon_height = MIN_ALLOWABLE_ICON_DIMENSION;
		fw->max_icon_width = MAX_ALLOWABLE_ICON_DIMENSION;
		fw->max_icon_height = MAX_ALLOWABLE_ICON_DIMENSION;
		fw->icon_resize_type = ICON_RESIZE_TYPE_NONE;
	}

	return;
}

void setup_icon_background_parameters(FvwmWindow *fw, window_style *pstyle)
{
	if (SHAS_ICON_BACKGROUND_PADDING(&pstyle->flags))
	{
		fw->icon_background_padding =
			SGET_ICON_BACKGROUND_PADDING(*pstyle);
	}
	else
	{
		fw->icon_background_padding = ICON_BACKGROUND_PADDING;
	}
	if (SHAS_ICON_BACKGROUND_RELIEF(&pstyle->flags))
	{
		fw->icon_background_relief =
			SGET_ICON_BACKGROUND_RELIEF(*pstyle);
	}
	else
	{
		fw->icon_background_relief = ICON_RELIEF_WIDTH;
	}
	return;
}

void setup_icon_title_parameters(FvwmWindow *fw, window_style *pstyle)
{
	if (SHAS_ICON_TITLE_RELIEF(&pstyle->flags))
	{
		fw->icon_title_relief =
			SGET_ICON_TITLE_RELIEF(*pstyle);
	}
	else
	{
		fw->icon_title_relief = ICON_RELIEF_WIDTH;
	}
	return;
}

void setup_numeric_vals(FvwmWindow *fw, window_style *pstyle)
{
	/****** window shading ******/
	fw->shade_anim_steps = pstyle->shade_anim_steps;

	/****** snapattraction, snapgrid, paging ******/
	fw->snap_attraction.proximity = pstyle->snap_attraction.proximity;
	fw->snap_attraction.mode = pstyle->snap_attraction.mode;
	fw->snap_grid_x = pstyle->snap_grid_x;
	fw->snap_grid_y = pstyle->snap_grid_y;
	if (pstyle->flags.has_edge_delay_ms_move)
	{
		fw->edge_delay_ms_move = pstyle->edge_delay_ms_move;
	}
	else
	{
		fw->edge_delay_ms_move = DEFAULT_MOVE_DELAY;
	}
	if (pstyle->flags.has_edge_delay_ms_resize)
	{
		fw->edge_delay_ms_resize = pstyle->edge_delay_ms_resize;
	}
	else
	{
		fw->edge_delay_ms_resize = DEFAULT_RESIZE_DELAY;
	}
	fw->edge_resistance_move = pstyle->edge_resistance_move;
	fw->edge_resistance_xinerama_move =
		pstyle->edge_resistance_xinerama_move;

	return;
}

static void setup_frame_window(
	FvwmWindow *fw)
{
	XSetWindowAttributes attributes;
	int valuemask;
	int depth;
	Visual *visual;
	FRenderPictFormat *format;

	valuemask = CWBackingStore | CWBackPixmap | CWEventMask | CWSaveUnder
		| CWCursor;

	/* This adds preliminary support for ARGB windows in fvwm. It should
	   evolve to proper ARGB support in frames, menus and modules */
	format=FRenderFindVisualFormat(dpy, fw->attr_backup.visual);
	if (format != NULL && format->type == FRenderPictTypeDirect &&
		format->direct.alphaMask > 0)
	{
		depth = fw->attr_backup.depth;
		visual = fw->attr_backup.visual;
		attributes.colormap = fw->attr_backup.colormap;
		attributes.background_pixel = -1;
		attributes.border_pixel = -1;
		valuemask |= CWColormap | CWBackPixel | CWBorderPixel;
	}
	else
	{
		depth = CopyFromParent;
		visual = CopyFromParent;
	}
	attributes.backing_store = NotUseful;
	attributes.background_pixmap = None;
	attributes.cursor = Scr.FvwmCursors[CRS_DEFAULT];
	attributes.event_mask = XEVMASK_FRAMEW_CAPTURE;
	attributes.save_under = False;
	/* create the frame window, child of root, grandparent of client */
	FW_W_FRAME(fw) = XCreateWindow(
		dpy, Scr.Root, fw->g.frame.x, fw->g.frame.y,
		fw->g.frame.width, fw->g.frame.height, 0, depth,
		InputOutput, visual, valuemask, &attributes);
	XSaveContext(dpy, FW_W(fw), FvwmContext, (caddr_t) fw);
	XSaveContext(dpy, FW_W_FRAME(fw), FvwmContext, (caddr_t) fw);

	return;
}

static void setup_title_window(
	FvwmWindow *fw, int valuemask, XSetWindowAttributes *pattributes)
{
	valuemask |= CWCursor | CWEventMask;
	pattributes->cursor = Scr.FvwmCursors[CRS_TITLE];
	pattributes->event_mask = XEVMASK_TITLEW;

	FW_W_TITLE(fw) = XCreateWindow(
		dpy, FW_W_FRAME(fw), 0, 0, 1, 1, 0, Pdepth, InputOutput,
		Pvisual, valuemask, pattributes);
	XSaveContext(dpy, FW_W_TITLE(fw), FvwmContext, (caddr_t) fw);

	return;
}

static void destroy_title_window(FvwmWindow *fw, Bool do_only_delete_context)
{
	if (!do_only_delete_context)
	{
		XDestroyWindow(dpy, FW_W_TITLE(fw));
		FW_W_TITLE(fw) = None;
	}
	XDeleteContext(dpy, FW_W_TITLE(fw), FvwmContext);
	XFlush(dpy);
	FW_W_TITLE(fw) = None;

	return;
}

static void change_title_window(
	FvwmWindow *fw, int valuemask, XSetWindowAttributes *pattributes)
{
	if (HAS_TITLE(fw) && FW_W_TITLE(fw) == None)
	{
		setup_title_window(fw, valuemask, pattributes);
	}
	else if (!HAS_TITLE(fw) && FW_W_TITLE(fw) != None)
	{
		destroy_title_window(fw, False);
	}

	return;
}

static void setup_button_windows(
	FvwmWindow *fw, int valuemask, XSetWindowAttributes *pattributes,
	short buttons)
{
	int i;
	Bool has_button;
	Bool is_deleted = False;

	valuemask |= CWCursor | CWEventMask;
	pattributes->cursor = Scr.FvwmCursors[CRS_SYS];
	pattributes->event_mask = XEVMASK_BUTTONW;

	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		has_button = (((!(i & 1) && i / 2 < Scr.nr_left_buttons) ||
			       ( (i & 1) && i / 2 < Scr.nr_right_buttons)) &&
			      (buttons & (1 << i)));
		if (FW_W_BUTTON(fw, i) == None && has_button)
		{
			FW_W_BUTTON(fw, i) =
				XCreateWindow(
					dpy, FW_W_FRAME(fw), 0, 0, 1, 1, 0,
					Pdepth, InputOutput, Pvisual,
					valuemask, pattributes);
			XSaveContext(
				dpy, FW_W_BUTTON(fw, i), FvwmContext,
				(caddr_t)fw);
		}
		else if (FW_W_BUTTON(fw, i) != None && !has_button)
		{
			/* destroy the current button window */
			XDestroyWindow(dpy, FW_W_BUTTON(fw, i));
			XDeleteContext(dpy, FW_W_BUTTON(fw, i), FvwmContext);
			is_deleted = True;
			FW_W_BUTTON(fw, i) = None;
		}
	}
	if (is_deleted == True)
	{
		XFlush(dpy);
	}

	return;
}

static void destroy_button_windows(FvwmWindow *fw, Bool do_only_delete_context)
{
	int i;
	Bool is_deleted = False;

	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		if (FW_W_BUTTON(fw, i) != None)
		{
			if (!do_only_delete_context)
			{
				XDestroyWindow(dpy, FW_W_BUTTON(fw, i));
				FW_W_BUTTON(fw, i) = None;
			}
			XDeleteContext(dpy, FW_W_BUTTON(fw, i), FvwmContext);
			is_deleted = True;
			FW_W_BUTTON(fw, i) = None;
		}
	}
	if (is_deleted == True)
	{
		XFlush(dpy);
	}

	return;
}

static void change_button_windows(
	FvwmWindow *fw, int valuemask, XSetWindowAttributes *pattributes,
	short buttons)
{
	if (HAS_TITLE(fw))
	{
		setup_button_windows(
			fw, valuemask, pattributes, buttons);
	}
	else
	{
		destroy_button_windows(fw, False);
	}

	return;
}

static void setup_parent_window(FvwmWindow *fw)
{
	size_borders b;

	XSetWindowAttributes attributes;
	int valuemask;

	valuemask = CWBackingStore | CWBackPixmap | CWCursor | CWEventMask |
		CWSaveUnder;
	attributes.backing_store = NotUseful;
	attributes.background_pixmap = None;
	attributes.cursor = Scr.FvwmCursors[CRS_DEFAULT];
	attributes.event_mask = XEVMASK_PARENTW;
	attributes.save_under = False;

	/* This window is exactly the same size as the client for the benefit
	 * of some clients */
	get_window_borders(fw, &b);
	FW_W_PARENT(fw) = XCreateWindow(
		dpy, FW_W_FRAME(fw), b.top_left.width, b.top_left.height,
		fw->g.frame.width - b.total_size.width,
		fw->g.frame.height - b.total_size.height,
		0, CopyFromParent, InputOutput, CopyFromParent, valuemask,
		&attributes);

	XSaveContext(dpy, FW_W_PARENT(fw), FvwmContext, (caddr_t) fw);

	return;
}

static void setup_resize_handle_cursors(FvwmWindow *fw)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	int i;

	if (HAS_NO_BORDER(fw))
	{
		return;
	}
	valuemask = CWCursor;
	attributes.cursor = Scr.FvwmCursors[CRS_DEFAULT];

	for (i = 0; i < 4; i++)
	{
		if (HAS_HANDLES(fw))
		{
			attributes.cursor = Scr.FvwmCursors[CRS_TOP_LEFT + i];
		}
		XChangeWindowAttributes(
			dpy, FW_W_CORNER(fw, i), valuemask, &attributes);
		if (HAS_HANDLES(fw))
		{
			attributes.cursor = Scr.FvwmCursors[CRS_TOP + i];
		}
		XChangeWindowAttributes(
			dpy, FW_W_SIDE(fw, i), valuemask, &attributes);
	}

	return;
}

static void setup_resize_handle_windows(FvwmWindow *fw)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	int i;
	int c_grav[4] = {
		NorthWestGravity,
		NorthEastGravity,
		SouthWestGravity,
		SouthEastGravity
	};
	int s_grav[4] = {
		NorthWestGravity,
		NorthEastGravity,
		SouthWestGravity,
		NorthWestGravity
	};

	if (HAS_NO_BORDER(fw))
	{
		return;
	}
	valuemask = CWEventMask | CWBackingStore | CWSaveUnder | CWWinGravity |
		CWBorderPixel | CWColormap;
	attributes.event_mask = XEVMASK_BORDERW;
	attributes.backing_store = NotUseful;
	attributes.save_under = False;
	attributes.border_pixel = 0;
	attributes.colormap = Pcmap;
	/* Just dump the windows any old place and let frame_setup_window take
	 * care of the mess */
	for (i = 0; i < 4; i++)
	{
		attributes.win_gravity = c_grav[i];
		FW_W_CORNER(fw, i) = XCreateWindow(
			dpy, FW_W_FRAME(fw), -1, -1, 1, 1, 0, Pdepth,
			InputOutput, Pvisual, valuemask, &attributes);
		XSaveContext(
			dpy, FW_W_CORNER(fw, i), FvwmContext, (caddr_t)fw);
		attributes.win_gravity = s_grav[i];
		FW_W_SIDE(fw, i) = XCreateWindow(
			dpy, FW_W_FRAME(fw), -1, -1, 1, 1, 0, Pdepth,
			InputOutput, Pvisual, valuemask, &attributes);
		XSaveContext(dpy, FW_W_SIDE(fw, i), FvwmContext, (caddr_t)fw);
	}
	setup_resize_handle_cursors(fw);

	return;
}

static void destroy_resize_handle_windows(
	FvwmWindow *fw, Bool do_only_delete_context)
{
	int i;

	for (i = 0; i < 4 ; i++)
	{
		XDeleteContext(dpy, FW_W_SIDE(fw, i), FvwmContext);
		XDeleteContext(dpy, FW_W_CORNER(fw, i), FvwmContext);
		if (!do_only_delete_context)
		{
			XDestroyWindow(dpy, FW_W_SIDE(fw, i));
			XDestroyWindow(dpy, FW_W_CORNER(fw, i));
			FW_W_SIDE(fw, i) = None;
			FW_W_CORNER(fw, i) = None;
		}
	}
	XFlush(dpy);

	return;
}

static void change_resize_handle_windows(FvwmWindow *fw)
{
	if (!HAS_NO_BORDER(fw) && FW_W_SIDE(fw, 0) == None)
	{
		setup_resize_handle_windows(fw);
	}
	else if (HAS_NO_BORDER(fw) && FW_W_SIDE(fw, 0) != None)
	{
		destroy_resize_handle_windows(fw, False);
	}
	else
	{
		setup_resize_handle_cursors(fw);
	}

	return;
}

static void setup_frame_stacking(FvwmWindow *fw)
{
	int i;
	int n;
	Window w[10 + NUMBER_OF_TITLE_BUTTONS];

	/* Stacking order (top to bottom):
	 *  - Parent window
	 *  - Title and buttons
	 *  - Corner handles
	 *  - Side handles
	 */
	n = 0;
	if (!IS_SHADED(fw))
	{
		w[n] = FW_W_PARENT(fw);
		n++;
	}
	if (HAS_TITLE(fw))
	{
		for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i += 2)
		{
			if (FW_W_BUTTON(fw, i) != None)
			{
				w[n] = FW_W_BUTTON(fw, i);
				n++;
			}
		}
		for (i = 2 * NR_RIGHT_BUTTONS - 1; i > 0; i -= 2)
		{
			if (FW_W_BUTTON(fw, i) != None)
			{
				w[n] = FW_W_BUTTON(fw, i);
				n++;
			}
		}
		if (FW_W_TITLE(fw) != None)
		{
			w[n] = FW_W_TITLE(fw);
			n++;
		}
	}
	for (i = 0; i < 4; i++)
	{
		if (FW_W_CORNER(fw, i) != None)
		{
			w[n] = FW_W_CORNER(fw, i);
			n++;
		}
	}
	for (i = 0; i < 4; i++)
	{
		if (FW_W_SIDE(fw, i) != None)
		{
			w[n] = FW_W_SIDE(fw, i);
			n++;
		}
	}
	if (IS_SHADED(fw))
	{
		w[n] = FW_W_PARENT(fw);
		n++;
	}
	XRestackWindows(dpy, w, n);

	return;
}

static void get_default_window_attributes(
	FvwmWindow *fw, unsigned long *pvaluemask,
	XSetWindowAttributes *pattributes)
{
	*pvaluemask |= CWBackingStore | CWCursor | CWSaveUnder | CWBorderPixel |
		CWColormap | CWBackPixmap;
	pattributes->background_pixel = 0;
	pattributes->background_pixmap = None;
	pattributes->backing_store = NotUseful;
	pattributes->cursor = Scr.FvwmCursors[CRS_DEFAULT];
	pattributes->save_under = False;
	pattributes->border_pixel = 0;
	pattributes->colormap = Pcmap;

	return;
}

static void setup_auxiliary_windows(
	FvwmWindow *fw, Bool setup_frame_and_parent, short buttons)
{
	unsigned long valuemask_save = 0;
	XSetWindowAttributes attributes;

	get_default_window_attributes(fw, &valuemask_save, &attributes);

	if (setup_frame_and_parent)
	{
		setup_frame_window(fw);
		setup_parent_window(fw);
	}
	setup_resize_handle_windows(fw);
	if (HAS_TITLE(fw))
	{
		setup_title_window(fw, valuemask_save, &attributes);
		setup_button_windows(fw, valuemask_save, &attributes, buttons);
	}
	setup_frame_stacking(fw);
	XMapSubwindows (dpy, FW_W_FRAME(fw));

	return;
}

static void destroy_auxiliary_windows(
	FvwmWindow *fw, Bool destroy_frame_and_parent)
{
	if (destroy_frame_and_parent)
	{
		XDeleteContext(dpy, FW_W_FRAME(fw), FvwmContext);
		XDeleteContext(dpy, FW_W_PARENT(fw), FvwmContext);
		delete_client_context(fw);
		XDestroyWindow(dpy, FW_W_FRAME(fw));
	}
	if (HAS_TITLE(fw))
	{
		destroy_title_window(fw, True);
	}
	if (HAS_TITLE(fw))
	{
		destroy_button_windows(fw, True);
	}
	if (!HAS_NO_BORDER(fw))
	{
		destroy_resize_handle_windows(fw, True);
	}
	XFlush(dpy);

	return;
}

static void broadcast_window_names(FvwmWindow *fw, int changed_names)
{
	if (changed_names & 1)
	{
		EWMH_SetVisibleName(fw, False);
		BroadcastWindowIconNames(fw, True, False);
	}
	if (changed_names & 2)
	{
		EWMH_SetVisibleName(fw, True);
		BroadcastWindowIconNames(fw, False, True);
	}
}

static void setup_icon(FvwmWindow *fw, window_style *pstyle)
{
	int affected_titles;

	increase_icon_hint_count(fw);
	/* find a suitable icon pixmap */
	if ((fw->wmhints) && (fw->wmhints->flags & IconWindowHint))
	{
		if (SHAS_ICON(&pstyle->flags) &&
		    S_ICON_OVERRIDE(SCF(*pstyle)) == ICON_OVERRIDE)
		{
			ICON_DBG((stderr,"si: iwh ignored '%s'\n",
				  fw->name.name));
			fw->icon_bitmap_file = SGET_ICON_NAME(*pstyle);
		}
		else
		{
			ICON_DBG((stderr,"si: using iwh '%s'\n",
				  fw->name.name));
			fw->icon_bitmap_file = NULL;
		}
	}
	else if ((fw->wmhints) && (fw->wmhints->flags & IconPixmapHint))
	{
		if (SHAS_ICON(&pstyle->flags) &&
		    S_ICON_OVERRIDE(SCF(*pstyle)) != NO_ICON_OVERRIDE)
		{
			ICON_DBG((stderr,"si: iph ignored '%s'\n",
				  fw->name.name));
			fw->icon_bitmap_file = SGET_ICON_NAME(*pstyle);
		}
		else
		{
			ICON_DBG((stderr,"si: using iph '%s'\n",
				  fw->name.name));
			fw->icon_bitmap_file = NULL;
		}
	}
	else if (SHAS_ICON(&pstyle->flags))
	{
		/* an icon was specified */
		ICON_DBG((stderr,"si: using style '%s'\n", fw->name.name));
		fw->icon_bitmap_file = SGET_ICON_NAME(*pstyle);
	}
	else
	{
		/* use default icon */
		ICON_DBG((stderr,"si: using default '%s'\n", fw->name.name));
		fw->icon_bitmap_file = Scr.DefaultIcon;
	}

	/* icon name */
	if (!EWMH_WMIconName(fw, NULL, NULL, 0))
	{
		fw->icon_name.name = NoName;
		fw->icon_name.name_list = NULL;
		FlocaleGetNameProperty(
			XGetWMIconName, dpy, FW_W(fw), &(fw->icon_name));
	}
	if (fw->icon_name.name == NoName)
	{
		fw->icon_name.name = fw->name.name;
		SET_WAS_ICON_NAME_PROVIDED(fw, 0);
	}
	affected_titles = setup_visible_names(fw, 2);

	/* wait until the window is iconified and the icon window is mapped
	 * before creating the icon window
	 */
	FW_W_ICON_TITLE(fw) = None;
	broadcast_window_names(fw, affected_titles);
	if (fw->icon_bitmap_file != NULL &&
	    fw->icon_bitmap_file != Scr.DefaultIcon)
	{
		BroadcastName(M_ICON_FILE,FW_W(fw),FW_W_FRAME(fw),
			      (unsigned long)fw,fw->icon_bitmap_file);
	}

	return;
}

static void destroy_icon(FvwmWindow *fw)
{
	free_window_names(fw, False, True);
	if (IS_PIXMAP_OURS(fw))
	{
		XFreePixmap(dpy, fw->iconPixmap);
		fw->iconPixmap = None;
		if (fw->icon_maskPixmap != None)
		{
			XFreePixmap(dpy, fw->icon_maskPixmap);
			fw->icon_maskPixmap = None;
		}
		if (fw->icon_alphaPixmap != None)
		{
			XFreePixmap(dpy, fw->icon_alphaPixmap);
			fw->icon_alphaPixmap = None;
		}
		if (fw->icon_alloc_pixels != NULL)
		{
			if (fw->icon_nalloc_pixels != 0)
			{
				PictureFreeColors(
					dpy, Pcmap, fw->icon_alloc_pixels,
					fw->icon_nalloc_pixels, 0,
					fw->icon_no_limit);
			}
			free(fw->icon_alloc_pixels);
			fw->icon_alloc_pixels = NULL;
			fw->icon_nalloc_pixels = 0;
			fw->icon_no_limit = 0;
		}
	}
	if (FW_W_ICON_TITLE(fw))
	{
		XDestroyWindow(dpy, FW_W_ICON_TITLE(fw));
		XDeleteContext(dpy, FW_W_ICON_TITLE(fw), FvwmContext);
		XFlush(dpy);
	}
	if (FW_W_ICON_PIXMAP(fw) != None)
	{
		if (IS_ICON_OURS(fw))
		{
			XDestroyWindow(dpy, FW_W_ICON_PIXMAP(fw));
		}
		else
		{
			XUnmapWindow(dpy, FW_W_ICON_PIXMAP(fw));
		}
		XDeleteContext(dpy, FW_W_ICON_PIXMAP(fw), FvwmContext);
	}
	clear_icon(fw);
	XFlush(dpy);

	return;
}

static void setup_icon_boxes(FvwmWindow *fw, window_style *pstyle)
{
	icon_boxes *ib;

	/* copy iconboxes ptr (if any) */
	if (SHAS_ICON_BOXES(&pstyle->flags))
	{
		fw->IconBoxes = SGET_ICON_BOXES(*pstyle);
		for (ib = fw->IconBoxes; ib; ib = ib->next)
		{
			ib->use_count++;
		}
	}
	else
	{
		fw->IconBoxes = NULL;
	}

	return;
}

static void destroy_icon_boxes(FvwmWindow *fw)
{
	if (fw->IconBoxes)
	{
		fw->IconBoxes->use_count--;
		if (fw->IconBoxes->use_count == 0 && fw->IconBoxes->is_orphan)
		{
			/* finally destroy the icon box */
			free_icon_boxes(fw->IconBoxes);
			fw->IconBoxes = NULL;
		}
	}

	return;
}

static void setup_layer(FvwmWindow *fw, window_style *pstyle)
{
	FvwmWindow *tf;
	int layer;

	if (SUSE_LAYER(&pstyle->flags))
	{
		/* use layer from style */
		layer = SGET_LAYER(*pstyle);
	}
	else if ((tf = get_transientfor_fvwmwindow(fw)) != NULL)
	{
		/* inherit layer from transientfor window */
		layer = get_layer(tf);
	}
	else
	{
		/* use default layer */
		layer = Scr.DefaultLayer;
	}
	set_default_layer(fw, layer);
	set_layer(fw, layer);

	return;
}

static void destroy_mini_icon(FvwmWindow *fw)
{
	if (fw->mini_icon)
	{
		PDestroyFvwmPicture(dpy, fw->mini_icon);
		fw->mini_icon = 0;
	}

	return;
}

static void setup_key_and_button_grabs(FvwmWindow *fw)
{
#ifdef BUGS_ARE_COOL
	/* dv (29-May-2001): If keys are grabbed separately for C_WINDOW and
	 * the other contexts, new windows have problems when bindings are
	 * removed.  Therefore, grab all keys in a single pass through the
	 * list. */
	GrabAllWindowKeys(
		dpy, FW_W_FRAME(fw), Scr.AllBindings,
		C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR, GetUnusedModifiers(),
		True);
#endif
	GrabAllWindowKeys(
		dpy, FW_W_FRAME(fw), Scr.AllBindings,
		C_TITLE|C_RALL|C_LALL|C_SIDEBAR|C_WINDOW, GetUnusedModifiers(),
		True);
	setup_focus_policy(fw);

	return;
}

static void __add_window_handle_x_resources(FvwmWindow *fw)
{
	int client_argc = 0;
	char **client_argv = NULL;
	XrmValue rm_value;
	XrmDatabase db = NULL;
	static XrmOptionDescRec table [] = {
		{"-xrn", NULL, XrmoptionResArg, (caddr_t) NULL},
		{"-xrm", NULL, XrmoptionResArg, (caddr_t) NULL},
	};
	/* Get global X resources */
	MergeXResources(dpy, &db, False);
	/* Find out if the client requested a specific style on the command
	 * line.
	 */
	if (XGetCommand(dpy, FW_W(fw), &client_argv, &client_argc))
	{
		if (client_argc > 0 && client_argv != NULL)
		{
			/* command line takes precedence over all */
			MergeCmdLineResources(
				&db, table, 2, fw->class.res_name,
				&client_argc, client_argv, True);
		}
	}


	/* parse the database values */
	if (GetResourceString(db, "fvwmstyle", fw->class.res_name, &rm_value)
	    && rm_value.size != 0)
	{
		char *style_name;
		int name_len;
		style_name = rm_value.addr;
		name_len = rm_value.size-1;
		/* Trim spaces at the start of the name */
		while (name_len>0 && isspace(*style_name))
		{
			style_name++;
			name_len--;
		}
		/* Trim spaces at the end of the name */
		while (name_len>0 && isspace(*(style_name+name_len-1)))
		{
			name_len--;
		}
		if (name_len>0) {
			fw->style_name = xmalloc(sizeof(char) * (name_len+1));
			memcpy(fw->style_name,style_name,name_len);
			fw->style_name[name_len] = 0;
		}
	}
	XFreeStringList(client_argv);
	XrmDestroyDatabase(db);
	return;
}


static int is_geometry_invalid_with_hints(
	const rectangle *g, const XSizeHints *hints)
{
	if (hints->min_width > g->width)
	{
		return 1;
	}
	else if (hints->min_height > g->height)
	{
		return 2;
	}
	else if (hints->max_width < g->width)
	{
		return 3;
	}
	else if (hints->max_height < g->height)
	{
		return 4;
	}
	if (hints->width_inc > 0)
	{
		int remainder;

		remainder = (g->width - hints->base_width) %
			hints->width_inc;
		if (remainder != 0)
		{
			return 5;
		}
	}
	if (hints->height_inc > 0)
	{
		int remainder;

		remainder = (g->height - hints->base_height) %
			hints->height_inc;
		if (remainder != 0)
		{
			return 6;
		}
	}
	if (hints->flags & PAspect)
	{
		if (
			(long)g->width * (long)hints->min_aspect.y <
			(long)hints->min_aspect.x * (long)g->height)
		{
			/* aspect smaller than minimum aspect */
			return 7;
		}
		if (
			(long)g->width * (long)hints->max_aspect.y >
			(long)hints->max_aspect.x * (long)g->height)
		{
			/* aspect larger than maximum aspect */
			return 8;
		}
	}

	return 0;
}

/* ---------------------------- interface functions ------------------------ */

/* what_changed:
 *  1 = title name
 *  2 = icon name
 *  3 = both
 *  4 = title format style
 *  8 = icon title format style
 *
 * Returns which titles need to be updated (like what_changed).  The return
 * value can be used as the 'which' argument of update_window_names.
 */
int setup_visible_names(FvwmWindow *fw, int what_changed)
{
	char *ext_name;
	window_style style;
	int affected_titles;
	int changed_names;
	int changed_styles;
	int force_update;
	int bits;

	if (fw == NULL)
	{
		/* should never happen */
		return 0;
	}

	changed_names = (what_changed & 3);
	changed_styles = ((what_changed >> 2) & 3);
	force_update = changed_styles;
	if (fw->visible_name == NULL)
	{
		force_update |= 1;
	}
	if (fw->visible_icon_name == NULL)
	{
		force_update |= 2;
	}
	affected_titles = 0;
	affected_titles |= changed_styles;
	/* TA:  Get the window style. */
	lookup_style(fw, &style);
	if (changed_names != 0 || (force_update & 1))
	{
		ext_name = interpolate_titleformat_name(
			&bits, fw, &style, False);
		if ((changed_names & bits) || (force_update & 1))
		{
			fw->visible_name = ext_name;
			affected_titles |= 1;
		}
		else
		{
			free(ext_name);
		}
	}
	if (changed_names != 0 || (force_update & 2))
	{
		ext_name = interpolate_titleformat_name(
			&bits, fw, &style, True);
		if ((changed_names & bits) || (force_update & 2))
		{
			fw->visible_icon_name = ext_name;
			affected_titles |= 2;
		}
		else
		{
			free(ext_name);
		}
	}

	return (changed_styles) ? changed_styles : affected_titles;
}

/* changed_names:
 *   1 = title
 *   2 = icon
 *   3 = both
 */
void update_window_names(FvwmWindow *fw, int changed_names)
{
	int affected_titles;

	affected_titles = setup_visible_names(fw, changed_names);
	affected_titles |= changed_names;
	/* fix the name in the title bar */
	if (!IS_ICONIFIED(fw))
	{
		border_draw_decorations(
			fw, PART_TITLE, (Scr.Hilite == fw), True, CLEAR_ALL,
			NULL, NULL);
	}
	broadcast_window_names(fw, affected_titles);
	if (affected_titles & 2)
	{
		RedoIconName(fw);
	}

	return;
}

void setup_window_name(FvwmWindow *fw)
{
	if (!EWMH_WMName(fw, NULL, NULL, 0))
	{
		fw->name.name = NoName;
		fw->name.name_list = NULL;
		FlocaleGetNameProperty(XGetWMName, dpy, FW_W(fw), &(fw->name));
	}

	return;
}

void setup_wm_hints(FvwmWindow *fw)
{
	fw->wmhints = XGetWMHints(dpy, FW_W(fw));
	set_focus_model(fw);

	return;
}

void setup_title_geometry(
	FvwmWindow *fw, window_style *pstyle)
{
	int width;
	int offset;

	get_title_font_size_and_offset(
		fw, S_TITLE_DIR(SCF(*pstyle)),
		S_IS_LEFT_TITLE_ROTATED_CW(SCF(*pstyle)),
		S_IS_RIGHT_TITLE_ROTATED_CW(SCF(*pstyle)),
		S_IS_TOP_TITLE_ROTATED(SCF(*pstyle)),
		S_IS_BOTTOM_TITLE_ROTATED(SCF(*pstyle)),
		&width, &offset);
	fw->title_thickness = width;
	fw->title_text_offset = offset;
	fw->corner_width = fw->title_thickness + fw->boundary_width;
	if (!HAS_TITLE(fw))
	{
		fw->title_thickness = 0;
	}

	return;
}

void setup_window_font(
	FvwmWindow *fw, window_style *pstyle, Bool do_destroy)
{
	/* get rid of old font */
	if (do_destroy)
	{
		destroy_window_font(fw);
		/* destroy_window_font resets the IS_WINDOW_FONT_LOADED flag */
	}
	/* load new font */
	if (!IS_WINDOW_FONT_LOADED(fw))
	{
		if (S_HAS_WINDOW_FONT(SCF(*pstyle)) &&
		    SGET_WINDOW_FONT(*pstyle) &&
		    (fw->title_font =
		     FlocaleLoadFont(dpy, SGET_WINDOW_FONT(*pstyle), "fvwm")))
		{
			SET_USING_DEFAULT_WINDOW_FONT(fw, 0);
		}
		else
		{
			/* no explicit font or failed to load, use default font
			 * instead */
			fw->title_font = Scr.DefaultFont;
			SET_USING_DEFAULT_WINDOW_FONT(fw, 1);
		}
		SET_WINDOW_FONT_LOADED(fw, 1);
	}
	setup_title_geometry(fw, pstyle);

	return;
}

void setup_icon_font(
	FvwmWindow *fw, window_style *pstyle, Bool do_destroy)
{
	int height = 0;

	if (IS_ICON_SUPPRESSED(fw) || HAS_NO_ICON_TITLE(fw))
	{
		if (IS_ICON_FONT_LOADED(fw))
		{
			destroy_icon_font(fw);
			/* destroy_icon_font resets the IS_ICON_FONT_LOADED
			 * flag */
		}
		return;
	}
	/* get rid of old font */
	if (do_destroy && IS_ICON_FONT_LOADED(fw))
	{
		destroy_icon_font(fw);
		/* destroy_icon_font resets the IS_ICON_FONT_LOADED flag */
	}
	/* load new font */
	if (!IS_ICON_FONT_LOADED(fw))
	{
		if (S_HAS_ICON_FONT(SCF(*pstyle)) && SGET_ICON_FONT(*pstyle) &&
		    (fw->icon_font =
		     FlocaleLoadFont(dpy, SGET_ICON_FONT(*pstyle), "fvwm")))
		{
			SET_USING_DEFAULT_ICON_FONT(fw, 0);
		}
		else
		{
			/* no explicit font or failed to load, use default font
			 * instead */
			fw->icon_font = Scr.DefaultFont;
			SET_USING_DEFAULT_ICON_FONT(fw, 1);
		}
		SET_ICON_FONT_LOADED(fw, 1);
	}
	/* adjust y position of existing icons */
	height = (IS_ICON_FONT_LOADED(fw)) ? fw->icon_font->height : 0;
	if (height)
	{
		resize_icon_title_height(fw, height - fw->icon_font->height);
		/* this repositions the icon even if the window is not
		 * iconified */
		DrawIconWindow(fw, True, True, False, False, NULL);
	}

	return;
}

void setup_style_and_decor(
	FvwmWindow *fw, window_style *pstyle, short *buttons)
{
	/* first copy the static styles into the window struct */
	memcpy(&(FW_COMMON_FLAGS(fw)), &(SCF(*pstyle)),
	       sizeof(common_flags_t));
	fw->wShaped = None;
	if (FShapesSupported)
	{
		int i;
		unsigned int u;
		Bool b;
		int boundingShaped;

		SUPPRESS_UNUSED_VAR_WARNING(i);
		SUPPRESS_UNUSED_VAR_WARNING(u);
		SUPPRESS_UNUSED_VAR_WARNING(b);
		FShapeSelectInput(dpy, FW_W(fw), FShapeNotifyMask);
		if (FShapeQueryExtents(
			    dpy, FW_W(fw), &boundingShaped, &i, &i, &u, &u, &b,
			    &i, &i, &u, &u))
		{
			fw->wShaped = boundingShaped;
		}
	}

	/* search for a UseDecor tag in the style */
	if (!IS_DECOR_CHANGED(fw))
	{
		FvwmDecor *decor = &Scr.DefaultDecor;

		for (; decor; decor = decor->next)
		{
			if (StrEquals(SGET_DECOR_NAME(*pstyle), decor->tag))
			{
				fw->decor = decor;
				break;
			}
		}
	}
	if (fw->decor == NULL)
	{
		fw->decor = &Scr.DefaultDecor;
	}

	GetMwmHints(fw);
	GetOlHints(fw);

	fw->buttons = SIS_BUTTON_DISABLED(&pstyle->flags);
	SelectDecor(fw, pstyle, buttons);

	if (IS_TRANSIENT(fw) && !pstyle->flags.do_decorate_transient)
	{
		SET_HAS_HANDLES(fw, 0);
		SET_HAS_TITLE(fw, 0);
	}
	/* set boundary width to zero for shaped windows */
	if (FHaveShapeExtension)
	{
		if (fw->wShaped)
		{
			set_window_border_size(fw, fw->unshaped_boundary_width);
			SET_HAS_NO_BORDER(fw, 1);
			SET_HAS_HANDLES(fw, 0);
		}
	}

	/****** window colors ******/
	update_window_color_style(fw, pstyle);
	update_window_color_hi_style(fw, pstyle);

	/***** icons colorsets *****/
	update_icon_title_cs_style(fw, pstyle);
	update_icon_title_cs_hi_style(fw, pstyle);
	update_icon_background_cs_style(fw, pstyle);

	/***** icons title/background parameters ****/
	setup_icon_background_parameters(fw, pstyle);
	setup_icon_title_parameters(fw, pstyle);

	/****** some numeric values ******/
	setup_numeric_vals(fw, pstyle);

	/* ConfigureNotify motion method */
	if (SCR_MOTION_METHOD(&pstyle->flag_mask))
	{
		CR_MOTION_METHOD(fw) = SCR_MOTION_METHOD(&pstyle->flags);
	}

	return;
}

void change_icon_boxes(FvwmWindow *fw, window_style *pstyle)
{
	destroy_icon_boxes(fw);
	setup_icon_boxes(fw, pstyle);

	return;
}

void setup_frame_size_limits(FvwmWindow *fw, window_style *pstyle)
{
	if (SHAS_MIN_WINDOW_SIZE(&pstyle->flags))
	{
		fw->min_window_width = SGET_MIN_WINDOW_WIDTH(*pstyle);
		fw->min_window_height = SGET_MIN_WINDOW_HEIGHT(*pstyle);
	}
	else
	{
		fw->min_window_width = 0;
		fw->min_window_height = 0;
	}
	if (SHAS_MAX_WINDOW_SIZE(&pstyle->flags))
	{
		fw->max_window_width = SGET_MAX_WINDOW_WIDTH(*pstyle);
		fw->max_window_height = SGET_MAX_WINDOW_HEIGHT(*pstyle);
	}
	else
	{
		fw->max_window_width = DEFAULT_MAX_MAX_WINDOW_WIDTH;
		fw->max_window_height = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
	}

	return;
}

void setup_placement_penalty(FvwmWindow *fw, window_style *pstyle)
{
	if (!SHAS_PLACEMENT_PENALTY(&pstyle->flags))
	{
		pl_penalty_struct *p;

		p = SGET_PLACEMENT_PENALTY_PTR(*pstyle);
		*p = default_pl_penalty;
	}
	if (!SHAS_PLACEMENT_PERCENTAGE_PENALTY(&pstyle->flags))
	{
		pl_percent_penalty_struct *p;

		p = SGET_PLACEMENT_PERCENTAGE_PENALTY_PTR(*pstyle);
		*p = default_pl_percent_penalty;
	}
	fw->pl_penalty = (*pstyle).pl_penalty;
	fw->pl_percent_penalty = (*pstyle).pl_percent_penalty;

	return;
}

void setup_frame_attributes(
	FvwmWindow *fw, window_style *pstyle)
{
	XSetWindowAttributes xswa;

	/* Backing_store is controlled on the client, borders, title & buttons
	 */
	switch (pstyle->flags.use_backing_store)
	{
	case BACKINGSTORE_DEFAULT:
		xswa.backing_store = fw->attr_backup.backing_store;
		break;
	case BACKINGSTORE_ON:
		xswa.backing_store = Scr.use_backing_store;
		break;
	case BACKINGSTORE_OFF:
	default:
		xswa.backing_store = NotUseful;
		break;
	}
	/* parent_relative is applied to the frame and the parent */
	xswa.background_pixmap = pstyle->flags.use_parent_relative
		? ParentRelative : None;
	/* Save_under is only useful on the frame */
	xswa.save_under = pstyle->flags.do_save_under
		? Scr.flags.do_save_under : NotUseful;
	XChangeWindowAttributes(dpy, FW_W(fw), CWBackingStore, &xswa);
	XChangeWindowAttributes(
		dpy, FW_W_PARENT(fw), CWBackPixmap | CWBackingStore, &xswa);
	XChangeWindowAttributes(
		dpy, FW_W_FRAME(fw),
		CWBackPixmap | CWBackingStore | CWSaveUnder, &xswa);

	return;
}

void change_auxiliary_windows(FvwmWindow *fw, short buttons)
{
	unsigned long valuemask_save = 0;
	XSetWindowAttributes attributes;

	get_default_window_attributes(fw, &valuemask_save, &attributes);
	change_title_window(fw, valuemask_save, &attributes);
	change_button_windows(fw, valuemask_save, &attributes, buttons);
	change_resize_handle_windows(fw);
	setup_frame_stacking(fw);
	XMapSubwindows (dpy, FW_W_FRAME(fw));

	return;
}

void increase_icon_hint_count(FvwmWindow *fw)
{
	if (fw->wmhints &&
	    (fw->wmhints->flags & (IconWindowHint | IconPixmapHint)))
	{
		switch (WAS_ICON_HINT_PROVIDED(fw))
		{
		case ICON_HINT_NEVER:
			SET_WAS_ICON_HINT_PROVIDED(fw, ICON_HINT_ONCE);
			break;
		case ICON_HINT_ONCE:
			SET_WAS_ICON_HINT_PROVIDED(fw, ICON_HINT_MULTIPLE);
			break;
		case ICON_HINT_MULTIPLE:
		default:
			break;
		}
		ICON_DBG((stderr,"icon hint count++ (%d) '%s'\n",
			  (int)WAS_ICON_HINT_PROVIDED(fw), fw->name.name));
	}

	return;
}

void change_icon(FvwmWindow *fw, window_style *pstyle)
{
	destroy_icon(fw);
	setup_icon(fw, pstyle);

	return;
}

void change_mini_icon(FvwmWindow *fw, window_style *pstyle)
{
	FvwmPicture *old_mi = fw->mini_icon;
	destroy_mini_icon(fw);
	setup_mini_icon(fw, pstyle);
	broadcast_mini_icon(fw);
	if (old_mi != NULL && fw->mini_icon == 0)
	{
		/* this case is not handled in setup_mini_icon, so we must
		 * broadcast here explicitly */
		BroadcastFvwmPicture(
			M_MINI_ICON, FW_W(fw), FW_W_FRAME(fw),
			(unsigned long)fw, NULL, "");
	}

	return;
}

void setup_focus_policy(FvwmWindow *fw)
{
	focus_grab_buttons(fw);

	return;
}

Bool validate_transientfor(FvwmWindow *fw)
{
	XWindowAttributes wa;
	FvwmWindow *cw;
	Window w;

	w = FW_W_TRANSIENTFOR(fw);
	if (w == None || w == FW_W(fw) || w == IS_EWMH_DESKTOP(w))
	{
		FW_W_TRANSIENTFOR(fw) = Scr.Root;
		return False;
	}
	else if (XFindContext(dpy, w, FvwmContext, (caddr_t *)&cw) != XCNOENT)
	{
		if (cw == fw)
		{
			/* It's a transient of itself, ignore the hint */
			FW_W_TRANSIENTFOR(fw) = Scr.Root;
			return False;
		}
		/* Check for transient loops */
		while (XFindContext(
			       dpy, FW_W_TRANSIENTFOR(cw), FvwmContext,
			       (caddr_t *)&cw) != XCNOENT &&
		       IS_TRANSIENT(cw))
		{
			if (FW_W_TRANSIENTFOR(cw) == FW_W(fw) || cw == fw)
			{
				/* loop detected, ignore the hint */
				FW_W_TRANSIENTFOR(fw) = Scr.Root;
				return False;
			}
		}
	}
	else if (!XGetWindowAttributes(dpy, w, &wa) ||
		 wa.map_state != IsViewable)
	{
		/* transientfor does not exist or is not viewable or unmapped */
		FW_W_TRANSIENTFOR(fw) = Scr.Root;
		return False;
	}

	return True;
}

Bool setup_transientfor(FvwmWindow *fw)
{
	Bool rc;

	rc = XGetTransientForHint(dpy, FW_W(fw), &FW_W_TRANSIENTFOR(fw));
	SET_TRANSIENT(fw, rc);
	if (rc == False)
	{
		FW_W_TRANSIENTFOR(fw) = Scr.Root;
	}
	validate_transientfor(fw);

	return rc;
}

/*
 *
 *  Procedure:
 *	AddWindow - add a new window to the fvwm list
 *
 */
FvwmWindow *AddWindow(
	const char **ret_initial_map_command, const exec_context_t *exc,
	FvwmWindow *ReuseWin, initial_window_options_t * win_opts)
{
	/* new fvwm window structure */
	register FvwmWindow *fw;
	FvwmWindow *tmp;
	/* mask for create windows */
	unsigned long valuemask;
	/* attributes for create windows */
	XSetWindowAttributes attributes;
	XWindowAttributes wattr;
	/* area for merged styles */
	window_style style;
	/* used for faster access */
	style_flags *sflags;
	short buttons;
	Bool used_sm = False;
	Bool do_resize_too = False;
	size_borders b;
	frame_move_resize_args mr_args;
	mwtsm_state_args state_args;
	Window w = exc->w.w;
	const exec_context_t *exc2;
	exec_context_changes_t ecc;

	/****** init window structure ******/
	setup_window_structure(&tmp, w, ReuseWin);
	fw = tmp;

	/****** Make sure the client window still exists.  We don't want to
	 * leave an orphan frame window if it doesn't.	Since we now have the
	 * server grabbed, the window can't disappear later without having been
	 * reparented, so we'll get a DestroyNotify for it.  We won't have
	 * gotten one for anything up to here, however. ******/
	MyXGrabServer(dpy);
	if (XGetGeometry(
		    dpy, w, &JunkRoot, &JunkX, &JunkY,
		    (unsigned int*)&JunkWidth, (unsigned int*)&JunkHeight,
		    (unsigned int*)&JunkBW, (unsigned int*)&JunkDepth) == 0)
	{
		if (Scr.bo.do_display_new_window_names)
		{
			fvwm_msg(INFO, "AddWindow", "new window disappeared");
		}
		free(fw);
		MyXUngrabServer(dpy);
		return NULL;
	}

	/****** window name ******/
	setup_window_name(fw);
	setup_class_and_resource(fw);

	/****** style setup ******/
	__add_window_handle_x_resources(fw);
	/* get merged styles */
	lookup_style(fw, &style);
	sflags = SGET_FLAGS_POINTER(style);
	if (SIS_UNMANAGED(sflags))
	{
		if (Scr.bo.do_display_new_window_names)
		{
			fvwm_msg(
				INFO, "AddWindow", "new window is unmanaged:\n"
				"  name:      %s\n"
				"  icon name: (unknown)\n"
				"  resource:  %s\n"
				"  class:     %s",
				fw->name.name, fw->class.res_name,
				fw->class.res_class);
		}
		free_window_names(fw, True, True);
		if (fw->style_name)
		{
			free(fw->style_name);
		}
		free(fw);
		MyXUngrabServer(dpy);
		return AW_UNMANAGED;
	}

	/****** window attributes and hints ******/
	setup_window_attr(fw, &wattr);
	setup_wm_hints(fw);

	/****** basic style and decor ******/
	/* If the window is in the NoTitle list, or is a transient, dont
	 * decorate it.	 If its a transient, and DecorateTransients was
	 * specified, decorate anyway. */
	setup_transientfor(fw);
	if (win_opts->flags.is_menu)
	{
		SET_TEAR_OFF_MENU(fw, 1);
	}
	fw->decor = NULL;
	setup_style_and_decor(fw, &style, &buttons);

	/****** fonts ******/
	setup_window_font(fw, &style, False);
	setup_icon_font(fw, &style, False);

	/***** visible window name ****/
	setup_visible_names(fw, 1);
	EWMH_SetVisibleName(fw, False);
	if (Scr.bo.do_display_new_window_names)
	{
		fvwm_msg(
			INFO, "AddWindow", "new window:\n"
			"  name:      %s\n"
			"  icon name: %s\n"
			"  resource:  %s\n"
			"  class:     %s",
			fw->name.name, (fw->icon_name.name == NULL) ?
			"(unknown)" : fw->icon_name.name, fw->class.res_name,
			fw->class.res_class);
	}

	/****** InitialMapCommand ******/
	*ret_initial_map_command =
		(style.flags.has_initial_map_command_string) ?
		SGET_INITIAL_MAP_COMMAND_STRING(style) : NULL;

	/****** state setup ******/
	setup_icon_boxes(fw, &style);
	SET_ICONIFIED(fw, 0);
	SET_ICON_UNMAPPED(fw, 0);
	SET_MAXIMIZED(fw, 0);
	/*
	 * Reparenting generates an UnmapNotify event, followed by a MapNotify.
	 * Set the map state to 0 to prevent a transition back to
	 * WithdrawnState in HandleUnmapNotify.	 Map state gets set corrected
	 * again in HandleMapNotify.
	 */
	SET_MAPPED(fw, 0);

	/****** window list and stack ring ******/
	/* add the window to the end of the fvwm list */
	fw->next = Scr.FvwmRoot.next;
	fw->prev = &Scr.FvwmRoot;
	while (fw->next != NULL)
	{
		fw->prev = fw->next;
		fw->next = fw->next->next;
	}
	/* fw->prev points to the last window in the list, fw->next is
	 * NULL. Now fix the last window to point to fw */
	fw->prev->next = fw;
	/*
	 * RBW - 11/13/1998 - add it into the stacking order chain also.
	 * This chain is anchored at both ends on Scr.FvwmRoot, there are
	 * no null pointers.
	 */
	add_window_to_stack_ring_after(fw, &Scr.FvwmRoot);

	/****** calculate frame size ******/
	fw->hints.win_gravity = NorthWestGravity;
	GetWindowSizeHints(fw);

	/****** border width ******/
	XSetWindowBorderWidth(dpy, FW_W(fw), 0);

	/****** icon size limits ******/
	setup_icon_size_limits(fw, &style);

	/***** placement penalities *****/
	setup_placement_penalty(fw, &style);
	/*
	 * MatchWinToSM changes fw->attr and the stacking order.
	 * Thus it is important have this call *after* PlaceWindow and the
	 * stacking order initialization.
	 */
	get_window_borders(fw, &b);
	memset(&state_args, 0, sizeof(state_args));
	used_sm = MatchWinToSM(fw, &state_args, win_opts);
	if (used_sm)
	{
		/* read the requested absolute geometry */
		gravity_translate_to_northwest_geometry_no_bw(
			fw->hints.win_gravity, fw, &fw->g.normal,
			&fw->g.normal);
		gravity_resize(
			fw->hints.win_gravity, &fw->g.normal,
			b.total_size.width, b.total_size.height);
		fw->g.frame = fw->g.normal;
		fw->g.frame.x -= Scr.Vx;
		fw->g.frame.y -= Scr.Vy;

		/****** calculate frame size ******/
		setup_frame_size_limits(fw, &style);
		constrain_size(
			fw, NULL, &fw->g.frame.width,
			&fw->g.frame.height, 0, 0, 0);

		/****** maximize ******/
		if (state_args.do_max)
		{
			SET_MAXIMIZED(fw, 1);
			constrain_size(
				fw, NULL, &fw->g.max.width, &fw->g.max.height,
				0, 0, CS_UPDATE_MAX_DEFECT);
			get_relative_geometry(&fw->g.frame, &fw->g.max);
		}
		else
		{
			get_relative_geometry(&fw->g.frame, &fw->g.normal);
		}
	}
	else
	{
		rectangle attr_g;

		if (IS_SHADED(fw))
		{
			state_args.do_shade = 1;
			state_args.used_title_dir_for_shading =
				USED_TITLE_DIR_FOR_SHADING(fw);
			state_args.shade_dir = SHADED_DIR(fw);
			SET_SHADED(fw, 0);
		}
		/* Tentative size estimate */
		fw->g.frame.width = wattr.width + b.total_size.width;
		fw->g.frame.height = wattr.height + b.total_size.height;

		/****** calculate frame size ******/
		setup_frame_size_limits(fw, &style);

		/****** layer ******/
		setup_layer(fw, &style);

		/****** window placement ******/
		attr_g.x = wattr.x;
		attr_g.y = wattr.y;
		attr_g.width = wattr.width;
		attr_g.height = wattr.height;
		do_resize_too = setup_window_placement(
			fw, &style, &attr_g, win_opts, PLACE_INITIAL);
		wattr.x = attr_g.x;
		wattr.y = attr_g.y;

		/* set up geometry */
		fw->g.frame.x = wattr.x;
		fw->g.frame.y = wattr.y;
		fw->g.frame.width = wattr.width + b.total_size.width;
		fw->g.frame.height = wattr.height + b.total_size.height;
		gravity_constrain_size(
			fw->hints.win_gravity, fw, &fw->g.frame, 0);
		update_absolute_geometry(fw);
	}

	/****** auxiliary window setup ******/
	setup_auxiliary_windows(fw, True, buttons);

	/****** 'backing store' and 'save under' window setup ******/
	setup_frame_attributes(fw, &style);

	/****** reparent the window ******/
	XReparentWindow(dpy, FW_W(fw), FW_W_PARENT(fw), 0, 0);

	/****** select events ******/
	valuemask = CWEventMask | CWDontPropagate;
	if (IS_TEAR_OFF_MENU(fw))
	{
		attributes.event_mask = XEVMASK_TEAR_OFF_MENUW;
	}
	else
	{
		attributes.event_mask = XEVMASK_CLIENTW;
	}
	attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
	XChangeWindowAttributes(dpy, FW_W(fw), valuemask, &attributes);
	/****** make sure the window is not destroyed when fvwm dies ******/
	if (!IS_TEAR_OFF_MENU(fw))
	{
		/* menus were created by fvwm itself, don't add them to the
		 * save set */
		XAddToSaveSet(dpy, FW_W(fw));
	}

	/****** now we can sefely ungrab the server ******/
	MyXUngrabServer(dpy);

	/* need to set up the mini icon before drawing */
	if (FMiniIconsSupported)
	{
		setup_mini_icon(fw, &style);
	}

	/****** arrange the frame ******/

	if (is_resizing_event_pending(fw) == True)
	{
		SET_FORCE_NEXT_CR(fw, 1);
		SET_FORCE_NEXT_PN(fw, 1);
		mr_args = frame_create_move_resize_args(
			fw, FRAME_MR_FORCE_SETUP_NO_W | FRAME_MR_DONT_DRAW,
			NULL, &fw->g.frame, 0, DIR_NONE);
	}
	else
	{
		mr_args = frame_create_move_resize_args(
			fw, FRAME_MR_FORCE_SETUP | FRAME_MR_DONT_DRAW, NULL,
			&fw->g.frame, 0, DIR_NONE);
	}
	frame_move_resize(fw, mr_args);
	frame_free_move_resize_args(fw, mr_args);
	/* draw later */
	SET_WAS_NEVER_DRAWN(fw, 1);

	/****** grab keys and buttons ******/
	setup_key_and_button_grabs(fw);

	/****** inform modules of new window ******/
	BroadcastConfig(M_ADD_WINDOW,fw);
	BroadcastWindowIconNames(fw, True, False);

	/****** place the window in the stack ring ******/
	if (!position_new_window_in_stack_ring(fw, SDO_START_LOWERED(sflags)))
	{
		XWindowChanges xwc;
		xwc.sibling = FW_W_FRAME(get_next_window_in_stack_ring(fw));
		xwc.stack_mode = Above;
		XConfigureWindow(
			dpy, FW_W_FRAME(fw), CWSibling|CWStackMode, &xwc);
	}

	/* these are sent and broadcast before res_{class,name} for the benefit
	 * of FvwmIconBox which can't handle M_ICON_FILE after M_RES_NAME */
	/****** icon and mini icon ******/
	/* migo (20-Jan-2000): the logic is to unset this flag on NULL values */
	SET_WAS_ICON_NAME_PROVIDED(fw, 1);
	setup_icon(fw, &style);
	if (FMiniIconsSupported)
	{
		broadcast_mini_icon(fw);
	}
	BroadcastName(M_RES_CLASS,FW_W(fw),FW_W_FRAME(fw),
		      (unsigned long)fw,fw->class.res_class);
	BroadcastName(M_RES_NAME,FW_W(fw),FW_W_FRAME(fw),
		      (unsigned long)fw,fw->class.res_name);

	/****** stick window ******/
	if (!(fw->hints.flags & USPosition) || used_sm)
	{
		int stick_page;
		int stick_desk;

		stick_page = is_window_sticky_across_pages(fw);
		stick_desk = is_window_sticky_across_desks(fw);
		if ((stick_page &&
		     !IsRectangleOnThisPage(&fw->g.frame, Scr.CurrentDesk)) ||
		    (stick_desk && fw->Desk != Scr.CurrentDesk))
		{
			/* If it's sticky and the user didn't ask for an
			 * explicit position, force it on screen now.  Don't do
			 * that with USPosition because we have to assume the
			 * user knows what (s)he is doing.  This is necessary
			 * e.g. if we want a sticky 'panel' in FvwmButtons but
			 * don't want to see when it's mapped in the void. */
			ecc.w.fw = fw;
			ecc.w.w = FW_W_FRAME(fw);
			ecc.w.wcontext = C_FRAME;
			exc2 = exc_clone_context(
				exc, &ecc, ECC_FW | ECC_W | ECC_WCONTEXT);
			SET_STICKY_ACROSS_PAGES(fw, 0);
			SET_STICKY_ACROSS_DESKS(fw, 0);
			handle_stick(
				NULL, exc2, "", stick_page, stick_desk, 1, 0);
			exc_destroy_context(exc2);
		}
	}

	/****** resize window ******/
	if (do_resize_too)
	{
		XEvent e;

		memset(&e, 0, sizeof(e));
		FWarpPointer(
			dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
			Scr.MyDisplayHeight,
			fw->g.frame.x + (fw->g.frame.width>>1),
			fw->g.frame.y + (fw->g.frame.height>>1));
		e.xany.type = ButtonPress;
		e.xbutton.button = 1;
		e.xbutton.state = Button1Mask;
		e.xbutton.x_root = fw->g.frame.x + (fw->g.frame.width>>1);
		e.xbutton.y_root = fw->g.frame.y + (fw->g.frame.height>>1);
		e.xbutton.x = (fw->g.frame.width>>1);
		e.xbutton.y = (fw->g.frame.height>>1);
		e.xbutton.subwindow = None;
		e.xany.window = FW_W(fw);
		fev_fake_event(&e);
		ecc.x.etrigger = &e;
		ecc.w.fw = fw;
		ecc.w.wcontext = C_WINDOW;
		exc2 = exc_clone_context(
			exc, &ecc, ECC_ETRIGGER | ECC_FW | ECC_WCONTEXT);
		CMD_Resize(NULL, exc2, "");
		exc_destroy_context(exc2);
	}

	/****** window colormap ******/
	ReInstallActiveColormap();

	/****** ewmh setup *******/
	EWMH_WindowInit(fw);

	/****** windowshade ******/
	if (state_args.do_shade || SDO_START_SHADED(sflags))
	{
		rectangle big_g;
		rectangle new_g;
		frame_move_resize_args mr_args;

		if (state_args.used_title_dir_for_shading)
		{
			state_args.shade_dir = GET_TITLE_DIR(fw);
		}
		/* If we've set a style for StartShaded, ensure we override
		 * the state for it here. -- TA.
		 */
		if (SDO_START_SHADED(sflags) && !state_args.do_shade)
		{
			state_args.shade_dir = SGET_STARTS_SHADED_DIR(style);
		}
		big_g = (IS_MAXIMIZED(fw)) ? fw->g.max : fw->g.frame;
		new_g = big_g;
		get_shaded_geometry_with_dir(
			fw, &new_g, &new_g, state_args.shade_dir);
		mr_args = frame_create_move_resize_args(
			fw, FRAME_MR_SHRINK | FRAME_MR_DONT_DRAW, &big_g,
			&new_g, 0, state_args.shade_dir);
		frame_move_resize(fw, mr_args);
		SET_SHADED(fw, 1);
		SET_SHADED_DIR(fw, state_args.shade_dir);
		frame_free_move_resize_args(fw, mr_args);
	}
	if (!IS_SHADED(fw) && !IS_ICONIFIED(fw))
	{
		/* TK always wants some special treatment: If the window is
		 * simply mapped, the tk menus come up at funny Y coordinates.
		 * Tell it it's geometry *again* to work around this problem.
		 */
		SendConfigureNotify(
			fw, fw->g.frame.x, fw->g.frame.y, fw->g.frame.width,
			fw->g.frame.height, 0, False);
	}
	if (
		HAS_EWMH_INIT_MAXVERT_STATE(fw) == EWMH_STATE_HAS_HINT ||
		HAS_EWMH_INIT_MAXHORIZ_STATE(fw) == EWMH_STATE_HAS_HINT)
	{
		int h;
		int v;
		char cmd[256];

		if (
			is_function_allowed(
				F_MAXIMIZE, NULL, fw, RQORIG_PROGRAM_US,
				False))
		{
			h = (HAS_EWMH_INIT_MAXHORIZ_STATE(fw) ==
			     EWMH_STATE_HAS_HINT) ? 100 : 0;
			v = (HAS_EWMH_INIT_MAXVERT_STATE(fw) ==
			     EWMH_STATE_HAS_HINT) ? 100 : 0;
			sprintf(cmd,"Maximize on %i %i", h, v);
			execute_function_override_window(
				NULL, NULL, cmd, 0, fw);
		}
	}
	if (HAS_EWMH_INIT_FULLSCREEN_STATE(fw) == EWMH_STATE_HAS_HINT)
	{
		EWMH_fullscreen(fw);
	}
	if (!XGetGeometry(
		    dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
		    (unsigned int*)&JunkWidth, (unsigned int*)&JunkHeight,
		    (unsigned int*)&JunkBW, (unsigned int*)&JunkDepth))
	{
		/* The window has disappeared somehow.	For some reason we do
		 * not always get a DestroyNotify on the window, so make sure
		 * it is destroyed. */
		destroy_window(fw);
		fw = NULL;
	}

	return fw;
}


/*
 *
 *  Procedure:
 *	FetchWMProtocols - finds out which protocols the window supports
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 */
void FetchWmProtocols(FvwmWindow *tmp)
{
	Atom *protocols = NULL, *ap;
	unsigned long *l_protocols;
	int i, n;
	Atom atype;
	int aformat;
	unsigned long bytes_remain,nitems;

	if (tmp == NULL)
	{
		return;
	}
	/* First, try the Xlib function to read the protocols.
	 * This is what Twm uses. */
	if (XGetWMProtocols (dpy, FW_W(tmp), &protocols, &n))
	{
		for (i = 0, ap = protocols; i < n; i++, ap++)
		{
			if (*ap == (Atom)_XA_WM_TAKE_FOCUS)
			{
				SET_WM_TAKES_FOCUS(tmp, 1);
				set_focus_model(tmp);
			}
			if (*ap == (Atom)_XA_WM_DELETE_WINDOW)
			{
				SET_WM_DELETES_WINDOW(tmp, 1);
			}
		}
		if (protocols)
		{
			XFree((char *)protocols);
		}
	}
	else
	{
		/* Next, read it the hard way. mosaic from Coreldraw needs to
		 * be read in this way. */
		if ((XGetWindowProperty(
			     dpy, FW_W(tmp), _XA_WM_PROTOCOLS, 0L, 10L, False,
			     _XA_WM_PROTOCOLS, &atype, &aformat, &nitems,
			     &bytes_remain,
			     (unsigned char **)&l_protocols)) == Success)
		{
			for (i = 0; i < nitems; i++)
			{
				ap = &(l_protocols[i]);
				if (*ap == (Atom)_XA_WM_TAKE_FOCUS)
				{
					SET_WM_TAKES_FOCUS(tmp, 1);
					set_focus_model(tmp);
				}
				if (*ap == (Atom)_XA_WM_DELETE_WINDOW)
				{
					SET_WM_DELETES_WINDOW(tmp, 1);
				}
			}
			if (protocols)
			{
				XFree((char *)protocols);
			}
		}
	}

	return;
}

void GetWindowSizeHintsWithCheck(
	FvwmWindow *fw,
	int do_reject_invalid_size_constraints_on_existing_window)
{
	long supplied = 0;
	char *broken_cause = 0;
	XSizeHints orig_hints;
	XSizeHints new_hints;
	Status rc;

	new_hints = fw->hints;
	rc = FGetWMNormalHints(dpy, FW_W(fw), &orig_hints, &supplied);
	if (rc == 0)
	{
		new_hints.flags = 0;
		memset(&orig_hints, 0, sizeof(orig_hints));
	}
	else
	{
		new_hints = orig_hints;
		if (HAS_OVERRIDE_SIZE_HINTS(fw))
		{
			/* ignore the WMNormal hints */
			new_hints.flags &= ~(PMinSize | PMaxSize | PResizeInc);
			new_hints.min_width = 0;
			new_hints.min_height = 0;
			new_hints.max_width = 0;
			new_hints.max_height = 0;
			new_hints.width_inc = 1;
			new_hints.height_inc = 1;
		}
	}

	/* Beat up our copy of the hints, so that all important field are
	 * filled in! */
	if (new_hints.flags & PResizeInc)
	{
		SET_SIZE_INC_SET(fw, 1);
		if (new_hints.width_inc <= 0)
		{
			if (new_hints.width_inc < 0 ||
			    (new_hints.width_inc == 0 &&
			     (new_hints.flags & PMinSize) &&
			     (new_hints.flags & PMaxSize) &&
			     new_hints.min_width != new_hints.max_width))
			{
				broken_cause = "width_inc";
			}
			new_hints.width_inc = 1;
			SET_SIZE_INC_SET(fw, 0);
		}
		if (new_hints.height_inc <= 0)
		{
			if (new_hints.height_inc < 0 ||
			    (new_hints.height_inc == 0 &&
			     (new_hints.flags & PMinSize) &&
			     (new_hints.flags & PMaxSize) &&
			     new_hints.min_height != new_hints.max_height))
			{
				if (!broken_cause)
				{
					broken_cause = "height_inc";
				}
			}
			new_hints.height_inc = 1;
			SET_SIZE_INC_SET(fw, 0);
		}
	}
	else
	{
		SET_SIZE_INC_SET(fw, 0);
		new_hints.width_inc = 1;
		new_hints.height_inc = 1;
	}

	if (new_hints.flags & PMinSize)
	{
		if (new_hints.min_width < 0 && !broken_cause)
		{
			broken_cause = "min_width";
		}
		if (new_hints.min_height < 0 && !broken_cause)
		{
			broken_cause = "min_height";
		}
	}
	else
	{
		if (new_hints.flags & PBaseSize)
		{
			new_hints.min_width = new_hints.base_width;
			new_hints.min_height = new_hints.base_height;
		}
		else
		{
			new_hints.min_width = 1;
			new_hints.min_height = 1;
		}
	}
	if (new_hints.min_width <= 0)
	{
		new_hints.min_width = 1;
	}
	if (new_hints.min_height <= 0)
	{
		new_hints.min_height = 1;
	}

	if (new_hints.flags & PMaxSize)
	{
		if (new_hints.max_width < new_hints.min_width)
		{
			new_hints.max_width = DEFAULT_MAX_MAX_WINDOW_WIDTH;
			if (!broken_cause)
			{
				broken_cause = "max_width";
			}
		}
		if (new_hints.max_height < new_hints.min_height)
		{
			new_hints.max_height = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
			if (!broken_cause)
			{
				broken_cause = "max_height";
			}
		}
	}
	else
	{
		new_hints.max_width = DEFAULT_MAX_MAX_WINDOW_WIDTH;
		new_hints.max_height = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
	}

	/*
	 * ICCCM says that PMinSize is the default if no PBaseSize is given,
	 * and vice-versa.
	 */

	if (new_hints.flags & PBaseSize)
	{
		if (new_hints.base_width < 0)
		{
			new_hints.base_width = 0;
			if (!broken_cause)
			{
				broken_cause = "base_width";
			}
		}
		if (new_hints.base_height < 0)
		{
			new_hints.base_height = 0;
			if (!broken_cause)
			{
				broken_cause = "base_height";
			}
		}
		if ((new_hints.base_width > new_hints.min_width) ||
		    (new_hints.base_height > new_hints.min_height))
		{
			/* In this case, doing the aspect ratio calculation
			   for window_size - base_size as prescribed by the
			   ICCCM is going to fail.
			   Resetting the flag disables the use of base_size
			   in aspect ratio calculation while it is still used
			   for grid sizing.
			*/
			new_hints.flags &= ~PBaseSize;
#if 0
			/* Keep silent about this, since the Xlib manual
			 * actually recommends making min <= base <= max ! */
			broken_cause = 0;
#endif
		}
	}
	else
	{
		if (new_hints.flags & PMinSize)
		{
			new_hints.base_width = new_hints.min_width;
			new_hints.base_height = new_hints.min_height;
		}
		else
		{
			new_hints.base_width = 0;
			new_hints.base_height = 0;
		}
	}

	if (!(new_hints.flags & PWinGravity))
	{
		new_hints.win_gravity = NorthWestGravity;
	}

	if ((new_hints.flags & PMaxSize) &&
	    ((new_hints.flags & PMinSize) || (new_hints.flags & PBaseSize)))
	{
		if (new_hints.max_width < new_hints.base_width)
		{
			new_hints.max_width = DEFAULT_MAX_MAX_WINDOW_WIDTH;
			if (!broken_cause)
			{
				broken_cause = "max_width";
			}
		}
		if (new_hints.max_height < new_hints.base_height)
		{
			new_hints.max_height = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
			if (!broken_cause)
			{
				broken_cause = "max_height";
			}
		}
	}

	if (new_hints.flags & PAspect)
	{
		/*
		** check to make sure min/max aspect ratios look valid
		*/
#define maxAspectX new_hints.max_aspect.x
#define maxAspectY new_hints.max_aspect.y
#define minAspectX new_hints.min_aspect.x
#define minAspectY new_hints.min_aspect.y


		/*
		** The math looks like this:
		**
		**   minAspectX	   maxAspectX
		**   ---------- <= ----------
		**   minAspectY	   maxAspectY
		**
		** If that is multiplied out, this must be satisfied:
		**
		**   minAspectX * maxAspectY <=	 maxAspectX * minAspectY
		**
		** So, what to do if this isn't met?  Ignoring it entirely
		** seems safest.
		**
		*/

		/* We also ignore people who put negative entries into
		 * their aspect ratios. They deserve it.
		 *
		 * We cast to double here, since the values may be large.
		 */
		if ((maxAspectX < 0) || (maxAspectY < 0) ||
		    (minAspectX < 0) || (minAspectY < 0) ||
		    (((double)minAspectX * (double)maxAspectY) >
		     ((double)maxAspectX * (double)minAspectY)))
		{
			if (!broken_cause)
			{
				broken_cause = "aspect ratio";
			}
			new_hints.flags &= ~PAspect;
			fvwm_msg(
				WARN, "GetWindowSizeHints",
				"The applicaton window (window id %#lx)\n"
				"  \"%s\" has broken aspect ratio: "
				"%d/%d - %d/%d\n"
				"    fvwm is ignoring this aspect ratio.  ",
				FW_W(fw), fw->name.name, minAspectX,
				minAspectY, maxAspectX, maxAspectY);
			fvwm_msg_report_app();
		}
		else
		{
			/* protect against overflow */
			if ((maxAspectX > 65536) || (maxAspectY > 65536))
			{
				double ratio =
					(double) maxAspectX /
					(double) maxAspectY;
				if (ratio > 1.0)
				{
					maxAspectX = 65536;
					maxAspectY = 65536 / ratio;
				}
				else
				{
					maxAspectX = 65536 * ratio;
					maxAspectY = 65536;
				}
			}
			if ((minAspectX > 65536) || (minAspectY > 65536))
			{
				double ratio =
					(double) minAspectX /
					(double) minAspectY;
				if (ratio > 1.0)
				{
					minAspectX = 65536;
					minAspectY = 65536 / ratio;
				}
				else
				{
					minAspectX = 65536 * ratio;
					minAspectY = 65536;
				}
			}
		}
#undef maxAspectX
#undef maxAspectY
#undef minAspectX
#undef minAspectY
	}
	if (do_reject_invalid_size_constraints_on_existing_window)
	{
		int is_invalid;
		rectangle g;

		gravity_get_naked_geometry(
			fw->hints.win_gravity, fw, &g, &fw->g.normal);
		is_invalid = is_geometry_invalid_with_hints(&g, &new_hints);
		if (!is_invalid && IS_MAXIMIZED(fw))
		{
			gravity_get_naked_geometry(
				fw->hints.win_gravity, fw, &g, &fw->g.max);
			is_invalid = is_geometry_invalid_with_hints(
				&g, &new_hints);
		}
		if (is_invalid)
		{
			fvwm_msg(
				WARN, "GetWindowSizeHints", "reason: %d:"
				" The hints have been ignored because the"
				" window's current size would have become"
				" invalid.  The new hints will become active"
				" when the window generates the next"
				" ConfigureRequest.\n", is_invalid);
			broken_cause = "inconsistent with current size";
		}
	}
	else
	{
		fw->hints = new_hints;
		fw->orig_hints.width_inc = orig_hints.width_inc;
		fw->orig_hints.height_inc = orig_hints.height_inc;
	}
	if (broken_cause != 0)
	{
		fvwm_msg(
			WARN, "GetWindowSizeHints",
			"The application window (id %#lx)\n"
			"  \"%s\" has broken size hints (%s).\n"
			"    fvwm is ignoring those hints.  "
			"  hint override = %d, flags = %lx\n"
			"  min_width = %d, min_height = %d, "
			"max_width = %d, max_height = %d\n"
			"  width_inc = %d, height_inc = %d\n"
			"  min_aspect = %d/%d, max_aspect = %d/%d\n"
			"  base_width = %d, base_height = %d\n"
			"  win_gravity = %d\n",
			FW_W(fw), fw->name.name, broken_cause,
			HAS_OVERRIDE_SIZE_HINTS(fw),
			orig_hints.flags,
			orig_hints.min_width, orig_hints.min_height,
			orig_hints.max_width, orig_hints.max_height,
			orig_hints.width_inc, orig_hints.height_inc,
			orig_hints.min_aspect.x, orig_hints.min_aspect.y,
			orig_hints.max_aspect.x, orig_hints.max_aspect.y,
			orig_hints.base_width, orig_hints.base_height,
			orig_hints.win_gravity);
		fvwm_msg_report_app();
	}
	/* final safety net */
	if (fw->orig_hints.width_inc <= 0)
	{
		fw->orig_hints.width_inc = 1;
	}
	if (fw->orig_hints.height_inc <= 0)
	{
		fw->orig_hints.height_inc = 1;
	}

	return;
}

void GetWindowSizeHints(FvwmWindow *fw)
{
	GetWindowSizeHintsWithCheck(fw, 0);
}

/*
 *
 * Releases dynamically allocated space used to store window/icon names
 *
 */
void free_window_names(FvwmWindow *fw, Bool nukename, Bool nukeicon)
{
	if (!fw)
	{
		return;
	}

	if (nukename)
	{
		if (fw->visible_name && fw->visible_name != fw->name.name &&
		    fw->visible_name != NoName)
		{
			free(fw->visible_name);
		}
		fw->visible_name = NoName;
		if (fw->name.name)
		{
			if (fw->icon_name.name == fw->name.name)
			{
				fw->icon_name.name = NoName;
			}
			if (fw->visible_icon_name == fw->name.name)
			{
				fw->visible_icon_name = fw->icon_name.name;
			}
			if (fw->name.name != NoName)
			{
				FlocaleFreeNameProperty(&(fw->name));
				fw->visible_name = NULL;
			}
		}
	}
	if (nukeicon)
	{
		if (fw->visible_icon_name &&
		    fw->visible_icon_name != fw->name.name &&
		    fw->visible_icon_name != fw->icon_name.name &&
		    fw->visible_icon_name != NoName)
		{
			free(fw->visible_icon_name);
		}
		fw->visible_icon_name = NoName;
		if (fw->icon_name.name)
		{
			if ((fw->icon_name.name != fw->name.name) &&
			    fw->icon_name.name != NoName)
			{
				FlocaleFreeNameProperty(&(fw->icon_name));
				fw->visible_icon_name = NULL;
			}
		}
	}

	return;
}



/*
 *
 * Handles destruction of a window
 *
 */
void destroy_window(FvwmWindow *fw)
{
	/* Warning, this is also called by HandleUnmapNotify; if it ever needs
	 * to look at the event, HandleUnmapNotify will have to mash the
	 * UnmapNotify into a DestroyNotify. */
	if (!fw)
	{
		return;
	}

	/* remove window style */
	if (!IS_SCHEDULED_FOR_DESTROY(fw) && !DO_REUSE_DESTROYED(fw))
	{
		style_id_t s_id;

		memset(&s_id, 0, sizeof(style_id_t));
		SID_SET_WINDOW_ID(s_id, (XID)FW_W(fw));
		SID_SET_HAS_WINDOW_ID(s_id, True);

		style_destroy_style(s_id);
	}

	/****** remove from window list ******/
	/* if the window is sheduled fro destroy the window has been already
	 * removed from list */
	if (!IS_SCHEDULED_FOR_DESTROY(fw))
	{
		/* first, remove the window from the list of all windows! */
		if (fw->prev != NULL)
		{
			fw->prev->next = fw->next;
		}
		if (fw->next != NULL)
		{
			fw->next->prev = fw->prev;
		}
		fw->next = NULL;
		fw->prev = NULL;

		/****** also remove it from the stack ring ******/

		/*
		 * RBW - 11/13/1998 - new: have to unhook the stacking order
		 chain also. There's always a prev and next, since this is a
		 ring anchored on Scr.FvwmRoot
		*/
		remove_window_from_stack_ring(fw);
	}

	/****** check if we have to delay window destruction ******/

	if ((Scr.flags.is_executing_complex_function ||
	     Scr.flags.is_executing_menu_function) &&
	    !DO_REUSE_DESTROYED(fw))
	{
		if (IS_SCHEDULED_FOR_DESTROY(fw))
		{
			return;
		}
		/* mark window for destruction */
		SET_SCHEDULED_FOR_DESTROY(fw, 1);
		Scr.flags.is_window_scheduled_for_destroy = 1;
		/* this is necessary in case the application destroys the
		 * client window and a new window is created with the same
		 * window id */
		delete_client_context(fw);
		XFlush(dpy);
		/* unmap the the window to fake that it was already removed */
		if (IS_ICONIFIED(fw))
		{
			if (FW_W_ICON_TITLE(fw))
			{
				XUnmapWindow(dpy, FW_W_ICON_TITLE(fw));
			}
			if (FW_W_ICON_PIXMAP(fw) != None)
			{
				XUnmapWindow(dpy, FW_W_ICON_PIXMAP(fw));
			}
		}
		else
		{
			XUnmapWindow(dpy, FW_W_FRAME(fw));
		}
		adjust_fvwm_internal_windows(fw);
		BroadcastPacket(
			M_DESTROY_WINDOW, 3, (long)FW_W(fw),
			(long)FW_W_FRAME(fw), (unsigned long)fw);
		EWMH_DestroyWindow(fw);
		focus_grab_buttons_on_layer(fw->layer);
		Scr.FWScheduledForDestroy = flist_append_obj(
			Scr.FWScheduledForDestroy, fw);
		return;
	}

	/****** unmap the frame ******/

	XUnmapWindow(dpy, FW_W_FRAME(fw));
	XFlush(dpy);
	/* already done above? */
	if (!IS_SCHEDULED_FOR_DESTROY(fw))
	{
		SET_SCHEDULED_FOR_DESTROY(fw, 1);
		adjust_fvwm_internal_windows(fw);
		BroadcastPacket(
			M_DESTROY_WINDOW, 3, (long)FW_W(fw),
			(long)FW_W_FRAME(fw), (unsigned long)fw);
		EWMH_DestroyWindow(fw);
	}
	focus_grab_buttons_on_layer(fw->layer);

	/****** destroy auxiliary windows ******/

	destroy_auxiliary_windows(fw, True);

	/****** destroy icon ******/

	destroy_icon_boxes(fw);
	destroy_icon(fw);

	/****** destroy mini icon ******/

#ifdef MINI_ICON
	destroy_mini_icon(fw);
#endif

	/****** free strings ******/

	free_window_names(fw, True, True);

	if (fw->style_name)
	{
		free(fw->style_name);
		fw->style_name = NULL;
	}
	if (fw->class.res_name && fw->class.res_name != NoResource)
	{
		XFree ((char *)fw->class.res_name);
		fw->class.res_name = NoResource;
	}
	if (fw->class.res_class && fw->class.res_class != NoClass)
	{
		XFree ((char *)fw->class.res_class);
		fw->class.res_class = NoClass;
	}
	if (fw->mwm_hints)
	{
		XFree((char *)fw->mwm_hints);
		fw->mwm_hints = NULL;
	}

	/****** free fonts ******/

	destroy_window_font(fw);
	destroy_icon_font(fw);

	/****** free wmhints ******/

	if (fw->wmhints)
	{
		XFree ((char *)fw->wmhints);
		fw->wmhints = NULL;
	}

	/****** free colormap windows ******/

	if (fw->cmap_windows != (Window *)NULL)
	{
		XFree((void *)fw->cmap_windows);
		fw->cmap_windows = NULL;
	}

	/****** throw away the structure ******/

	/*  Recapture reuses this struct, so don't free it.  */
	if (!DO_REUSE_DESTROYED(fw))
	{
		free((char *)fw);
	}

	/****** cleanup ******/

	XFlush(dpy);

	return;
}


/*
 *
 *  Procedure:
 *	RestoreWithdrawnLocation
 *
 *  Puts windows back where they were before fvwm took over
 *
 */
void RestoreWithdrawnLocation(
	FvwmWindow *fw, Bool is_restart_or_recapture, Window parent)
{
	int w2,h2;
	unsigned int mask;
	XWindowChanges xwc;
	rectangle naked_g;
	rectangle unshaded_g;
	XSetWindowAttributes xswa;

	if (!fw)
	{
		return;
	}

	/* always get the latest size hints in case the application changed
	 * the gravity and we do not yet know about it */
	XSync(dpy, 0);
	GetWindowSizeHints(fw);
	SET_HAS_NEW_WM_NORMAL_HINTS(fw, 0);
	get_unshaded_geometry(fw, &unshaded_g);
	gravity_get_naked_geometry(
		fw->hints.win_gravity, fw, &naked_g, &unshaded_g);
	gravity_translate_to_northwest_geometry(
		fw->hints.win_gravity, fw, &naked_g, &naked_g);
	xwc.x = naked_g.x;
	xwc.y = naked_g.y;
	xwc.width = naked_g.width;
	xwc.height = naked_g.height;
	xwc.border_width = fw->attr_backup.border_width;
	mask = (CWX | CWY | CWWidth | CWHeight | CWBorderWidth);

	/* We can not assume that the window is currently on the screen.
	 * Although this is normally the case, it is not always true.  The
	 * most common example is when the user does something in an
	 * application which will, after some amount of computational delay,
	 * cause the window to be unmapped, but then switches screens before
	 * this happens.  The XTranslateCoordinates call above will set the
	 * window coordinates to either be larger than the screen, or negative.
	 * This will result in the window being placed in odd, or even
	 * unviewable locations when the window is remapped.  The following
	 * code forces the "relative" location to be within the bounds of the
	 * display.
	 *
	 * gpw -- 11/11/93
	 *
	 * Unfortunately, this does horrendous things during re-starts,
	 * hence the "if (restart)" clause (RN)
	 *
	 * Also, fixed so that it only does this stuff if a window is more than
	 * half off the screen. (RN)
	 */

	if (!is_restart_or_recapture)
	{
		/* Don't mess with it if its partially on the screen now */
		if (unshaded_g.x < 0 || unshaded_g.y < 0 ||
		    unshaded_g.x >= Scr.MyDisplayWidth ||
		    unshaded_g.y >= Scr.MyDisplayHeight)
		{
			w2 = (unshaded_g.width >> 1);
			h2 = (unshaded_g.height >> 1);
			if ( xwc.x < -w2 || xwc.x > Scr.MyDisplayWidth - w2)
			{
				xwc.x = xwc.x % Scr.MyDisplayWidth;
				if (xwc.x < -w2)
				{
					xwc.x += Scr.MyDisplayWidth;
				}
			}
			if (xwc.y < -h2 || xwc.y > Scr.MyDisplayHeight - h2)
			{
				xwc.y = xwc.y % Scr.MyDisplayHeight;
				if (xwc.y < -h2)
				{
					xwc.y += Scr.MyDisplayHeight;
				}
			}
		}
	}

	/* restore initial backing store setting on window */
	xswa.backing_store = fw->attr_backup.backing_store;
	XChangeWindowAttributes(dpy, FW_W(fw), CWBackingStore, &xswa);
	/* reparent to root window */
	XReparentWindow(
		dpy, FW_W(fw), (parent == None) ? Scr.Root : parent, xwc.x,
		xwc.y);

	if (IS_ICONIFIED(fw) && !IS_ICON_SUPPRESSED(fw))
	{
		if (FW_W_ICON_TITLE(fw))
		{
			XUnmapWindow(dpy, FW_W_ICON_TITLE(fw));
		}
		if (FW_W_ICON_PIXMAP(fw))
		{
			XUnmapWindow(dpy, FW_W_ICON_PIXMAP(fw));
		}
	}

	XConfigureWindow(dpy, FW_W(fw), mask, &xwc);
	if (!is_restart_or_recapture)
	{
		/* must be initial capture */
		XFlush(dpy);
	}

	return;
}


/*
 *
 *  Procedure:
 *	Reborder - Removes fvwm border windows
 *
 */
void Reborder(void)
{
	FvwmWindow *fw;

	/* put a border back around all windows */
	MyXGrabServer (dpy);

	/* force reinstall */
	InstallWindowColormaps (&Scr.FvwmRoot);

	/* RBW - 05/15/1998
	 * Grab the last window and work backwards: preserve stacking order on
	 * restart. */
	for (fw = get_prev_window_in_stack_ring(&Scr.FvwmRoot);
	     fw != &Scr.FvwmRoot; fw = get_prev_window_in_stack_ring(fw))
	{
		if (!IS_ICONIFIED(fw) && Scr.CurrentDesk != fw->Desk)
		{
			XMapWindow(dpy, FW_W(fw));
			SetMapStateProp(fw, NormalState);
		}
		RestoreWithdrawnLocation (fw, True, Scr.Root);
		XUnmapWindow(dpy,FW_W_FRAME(fw));
		XDestroyWindow(dpy,FW_W_FRAME(fw));
	}

	MyXUngrabServer (dpy);
	FOCUS_RESET();
	XFlush(dpy);

	return;
}

void CaptureAllWindows(const exec_context_t *exc, Bool is_recapture)
{
	int i,j;
	unsigned int nchildren;
	Window root, parent, *children;
	initial_window_options_t win_opts;
	FvwmWindow *fw;

	MyXGrabServer(dpy);
	if (!XQueryTree(dpy, Scr.Root, &root, &parent, &children, &nchildren))
	{
		MyXUngrabServer(dpy);
		return;
	}
	memset(&win_opts, 0, sizeof(win_opts));
	win_opts.flags.do_override_ppos = 1;
	win_opts.flags.is_recapture = 1;
	if (!(Scr.flags.are_windows_captured)) /* initial capture? */
	{
		evh_args_t ea;
		exec_context_changes_t ecc;
		XEvent e;

		/* weed out icon windows */
		for (i = 0; i < nchildren; i++)
		{
			if (children[i])
			{
				XWMHints *wmhintsp = XGetWMHints(
					dpy, children[i]);

				if (wmhintsp &&
				    wmhintsp->flags & IconWindowHint)
				{
					for (j = 0; j < nchildren; j++)
					{
						if (children[j] ==
						    wmhintsp->icon_window)
						{
							children[j] = None;
							break;
						}
					}
				}
				if (wmhintsp)
				{
					XFree ((char *) wmhintsp);
				}
			}
		}
		/* map all of the non-override, non-icon windows */
		e.type = MapRequest;
		ecc.x.etrigger = &e;
		for (i = 0; i < nchildren; i++)
		{

			if (children[i] &&
			    MappedNotOverride(children[i], &win_opts))
			{
				XUnmapWindow(dpy, children[i]);
				e.xmaprequest.window = children[i];
				e.xmaprequest.parent = Scr.Root;
				ecc.w.fw = NULL;
				ecc.w.w = children[i];
				ecc.w.wcontext = C_ROOT;
				ea.exc = exc_clone_context(
					exc, &ecc, ECC_ETRIGGER | ECC_FW |
					ECC_W | ECC_WCONTEXT);
				HandleMapRequestKeepRaised(
					&ea, None, NULL, &win_opts);
				exc_destroy_context(ea.exc);
			}
		}
		Scr.flags.are_windows_captured = 1;
	}
	else /* must be recapture */
	{
		Window keep_on_top_win;
		Window parent_win;
		Window focus_w;
		FvwmWindow *t;

		t = get_focus_window();
		focus_w = (t) ? FW_W(t) : None;
		hide_screen(True, &keep_on_top_win, &parent_win);
		/* reborder all windows */
		for (i=0;i<nchildren;i++)
		{
			if (XFindContext(
				    dpy, children[i], FvwmContext,
				    (caddr_t *)&fw) != XCNOENT)
			{
				CaptureOneWindow(
					exc, fw, children[i], keep_on_top_win,
					parent_win, is_recapture);
			}
		}
		hide_screen(False, NULL, NULL);
		/* find the window that had the focus and focus it again */
		if (focus_w)
		{
			for (t = Scr.FvwmRoot.next; t && FW_W(t) != focus_w;
			     t = t->next)
			{
				;
			}
			if (t)
			{
				SetFocusWindow(t, True, FOCUS_SET_FORCE);
			}
		}
	}
	if (nchildren > 0)
	{
		XFree((char *)children);
	}
	MyXUngrabServer(dpy);

	return;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_Recapture(F_CMD_ARGS)
{
	do_recapture(F_PASS_ARGS, False);

	return;
}

void CMD_RecaptureWindow(F_CMD_ARGS)
{
	do_recapture(F_PASS_ARGS, True);

	return;
}
