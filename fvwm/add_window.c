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
/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified
 * by Rob Nation
 ****************************************************************************/
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/FScreen.h"
#include <libs/gravity.h>
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "commands.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "defaults.h"
#include "add_window.h"
#include "events.h"
#include "module_interface.h"
#include "stack.h"
#include "update.h"
#include "style.h"
#include "icons.h"
#include "gnome.h"
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

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

char NoName[] = "Untitled"; /* name if no name in XA_WM_NAME */
char NoClass[] = "NoClass"; /* Class if no res_class in class hints */
char NoResource[] = "NoResource"; /* Class if no res_name in class hints */
long isIconicState = 0;
Bool PPosOverride = False;
Bool isIconifiedByParent = False;

/* ---------------------------- local functions ----------------------------- */

/***********************************************************************
 *
 *  Procedure:
 *      CaptureOneWindow
 *      CaptureAllWindows
 *
 *   Decorates windows at start-up and during recaptures
 *
 ***********************************************************************/

static void CaptureOneWindow(
	FvwmWindow *fw, Window window, Window keep_on_top_win,
	Window parent_win, Bool is_recapture)
{
	Window w;
	unsigned long data[1];

	isIconicState = DontCareState;
	if (fw == NULL)
	{
		return;
	}
	if (IS_SCHEDULED_FOR_DESTROY(fw))
	{
		/* Fvwm might crash in complex functions if we really try to
		 * the dying window here because AddWindow() may fail and leave
		 * a destroyed window in Fw.  Any, by the way, it is
		 * pretty useless to recapture a * window that will vanish in a
		 * moment. */
		return;
	}
	/* Grab the server to make sure the window does not die during the
	 * recapture. */
	MyXGrabServer(dpy);
	if (!XGetGeometry(dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY, &JunkWidth,
			  &JunkHeight, &JunkBW,  &JunkDepth))
	{
		/* The window has already died, do not recapture it! */
		MyXUngrabServer(dpy);
		return;
	}
	if (XFindContext(dpy, window, FvwmContext, (caddr_t *)&fw) != XCNOENT)
	{
		Bool f = PPosOverride;
		Bool is_mapped = IS_MAPPED(fw);
		Bool is_menu;

		PPosOverride = True;
		if (IS_ICONIFIED(fw))
		{
			isIconicState = IconicState;
			isIconifiedByParent = IS_ICONIFIED_BY_PARENT(fw);
		}
		else
		{
			isIconicState = NormalState;
			isIconifiedByParent = 0;
			if (Scr.CurrentDesk != fw->Desk)
				SetMapStateProp(fw, NormalState);
		}
		data[0] = (unsigned long) fw->Desk;
		XChangeProperty(
			dpy, FW_W(fw), _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32,
			PropModeReplace, (unsigned char *) data, 1);

		/* are all these really needed ? */
		/* EWMH_SetWMDesktop(fw); */
		GNOME_SetHints(fw);
		GNOME_SetDesk(fw);
		GNOME_SetLayer(fw);
		GNOME_SetWinArea(fw);

		XSelectInput(dpy, FW_W(fw), NoEventMask);
		w = FW_W(fw);
		XUnmapWindow(dpy, FW_W_FRAME(fw));
		RestoreWithdrawnLocation(fw, is_recapture, parent_win);
		SET_DO_REUSE_DESTROYED(fw, 1); /* RBW - 1999/03/20 */
		destroy_window(fw);
		Event.xmaprequest.window = w;
		if (is_recapture && fw != NULL && IS_TEAR_OFF_MENU(fw))
		{
			is_menu = True;
		}
		else
		{
			is_menu = False;
		}
		HandleMapRequestKeepRaised(keep_on_top_win, fw, is_menu);
		if (!fFvwmInStartup)
		{
			SET_MAP_PENDING(fw, 0);
			SET_MAPPED(fw, is_mapped);
		}
		/* Clean out isIconicState here, otherwise all new windos may
		 * start iconified. */
		isIconicState = DontCareState;
		/* restore previous value */
		PPosOverride = f;
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
			Scr.MyDisplayHeight, 0, CopyFromParent, InputOutput,
			CopyFromParent, valuemask, &xswa);
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

/***********************************************************************
 *
 *  Procedure:
 *	MappedNotOverride - checks to see if we should really
 *		put a fvwm frame on the window
 *
 *  Returned Value:
 *	TRUE	- go ahead and frame the window
 *	FALSE	- don't frame the window
 *
 *  Inputs:
 *	w	- the window to check
 *
 ***********************************************************************/

static int MappedNotOverride(Window w)
{
	XWindowAttributes wa;
        Atom atype;
	int aformat;
	unsigned long nitems, bytes_remain;
	unsigned char *prop;

	isIconicState = DontCareState;

	if ((w==Scr.NoFocusWin)||(!XGetWindowAttributes(dpy, w, &wa)))
		return False;

	if (XGetWindowProperty(
		    dpy,w,_XA_WM_STATE,0L,3L,False,_XA_WM_STATE,
		    &atype,&aformat,&nitems,&bytes_remain,&prop)==Success)
	{
		if (prop != NULL)
		{
			isIconicState = *(long *)prop;
			XFree(prop);
		}
	}
	if (wa.override_redirect == True)
	{
		XSelectInput(dpy, w, XEVMASK_ORW);
	}
	return (((isIconicState == IconicState) ||
		 (wa.map_state != IsUnmapped)) &&
		(wa.override_redirect != True));
}

static void do_recapture(F_CMD_ARGS, Bool fSingle)
{
	if (fSingle)
	{
		if (DeferExecution(eventp, &w, &fw, &context, CRS_SELECT,
				   ButtonRelease))
		{
			return;
		}
	}
	MyXGrabServer(dpy);
	if (fSingle)
	{
		CaptureOneWindow(
			fw, FW_W(fw), None, None, True);
	}
	else
	{
		CaptureAllWindows(True);
	}
	/* Throw away queued up events. We don't want user input during a
	 * recapture.  The window the user clicks in might disapper at the very
	 * same moment and the click goes through to the root window. Not good
	 */
	XAllowEvents(dpy, AsyncPointer, CurrentTime);
	discard_events(
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|\
		PointerMotionMask|KeyPressMask|KeyReleaseMask);
#ifdef DEBUG_STACK_RING
	verify_stack_ring_consistency();
#endif
	MyXUngrabServer(dpy);
}

static Bool setup_window_structure(
	FvwmWindow **pfw, Window w, FvwmWindow *ReuseWin)
{
	FvwmWindow save_state;
	FvwmWindow *savewin = NULL;

	memset(&save_state, '\0', sizeof(FvwmWindow));

	/*
	  Allocate space for the FvwmWindow struct, or reuse an
	  old one (on Recapture).
	*/
	if (ReuseWin == NULL)
	{
		*pfw = (FvwmWindow *)safemalloc(sizeof(FvwmWindow));
		if (*pfw == (FvwmWindow *)0)
		{
			return False;
		}
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
		SET_SHADED_DIR(*pfw, SHADED_DIR(savewin));
		SET_PLACED_WB3(*pfw,IS_PLACED_WB3(savewin));
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
	}

	(*pfw)->cmap_windows = (Window *)NULL;
#ifdef MINI_ICONS
	(*pfw)->mini_pixmap_file = NULL;
	(*pfw)->mini_icon = NULL;
#endif

	return True;
}

static void setup_window_name_count(FvwmWindow *fw)
{
	FvwmWindow *t;
	int count = 0;
	Bool done = False;

	if (!fw->name.name)
		done = True;

	if (fw->icon_name.name &&
	    strcmp(fw->name.name, fw->icon_name.name) == 0)
		count = fw->icon_name_count;

	while (!done)
	{
		done = True;
		for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
		{
			if (t == fw)
			{
				continue;
			}
			if ((t->name.name &&
			     strcmp(t->name.name, fw->name.name) == 0 &&
			     t->name_count == count) ||
			    (t->icon_name.name &&
			     strcmp(t->icon_name.name, fw->name.name) == 0 &&
			     t->icon_name_count == count))
			{
				count++;
				done = False;
			}
		}
	}
	fw->name_count = count;

	return;
}

static void setup_icon_name_count(FvwmWindow *fw)
{
	FvwmWindow *t;
	int count = 0;
	Bool done = False;

	if (!fw->icon_name.name)
		done = True;

	if (fw->name.name &&
	    strcmp(fw->name.name, fw->icon_name.name) == 0)
	{
		count = fw->name_count;
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
			if ((t->icon_name.name &&
			     strcmp(t->icon_name.name,
				    fw->icon_name.name) == 0 &&
			     t->icon_name_count == count) ||
			    (t->name.name &&
			     strcmp(t->name.name, fw->icon_name.name) == 0 &&
			     t->name_count == count))
			{
				count++;
				done = False;
			}
		}
	}
	fw->icon_name_count = count;

	return;
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
	extern FvwmWindow *ButtonWindow;

	if (fw == Scr.Hilite)
	{
		Scr.Hilite = NULL;
	}
	update_last_screen_focus_window(fw);
	if (ButtonWindow == fw)
	{
		ButtonWindow = NULL;
	}
	restore_focus_after_unmap(fw, False);
	frame_destroyed_frame(FW_W(fw));

	return;
}

/* ---------------------------- interface functions ------------------------- */

void setup_visible_name(FvwmWindow *fw, Bool is_icon)
{
	char *ext_name;
	char *name;
	int count;
	int len;

	if (fw == NULL)
	{
		/* should never happen */
		return;
	}

	if (is_icon)
	{
		if (fw->visible_icon_name != NULL &&
		    fw->visible_icon_name != fw->icon_name.name &&
		    fw->visible_icon_name != fw->name.name &&
		    fw->visible_icon_name != NoName)
		{
			free(fw->visible_icon_name);
			fw->visible_icon_name = NULL;
		}
		name = fw->icon_name.name;
		setup_icon_name_count(fw);
		count = fw->icon_name_count;
	}
	else
	{
		if (fw->visible_name != NULL &&
		    fw->visible_name != fw->name.name &&
		    fw->visible_name != NoName)
		{
			free(fw->visible_name);
			fw->visible_name = NULL;
		}
		name = fw->name.name;
		setup_window_name_count(fw);
		count = fw->name_count;
	}

	if (name == NULL)
	{
		return; /* should never happen */
	}

	if (count > MAX_WINDOW_NAME_NUMBER - 1)
	{
		count = MAX_WINDOW_NAME_NUMBER - 1;
	}

	if (count != 0 &&
	    ((is_icon && USE_INDEXED_ICON_NAME(fw)) ||
	     (!is_icon && USE_INDEXED_WINDOW_NAME(fw))))
	{
		len = strlen(name);
		count++;
		ext_name = (char *)safemalloc(
			len + MAX_WINDOW_NAME_NUMBER_DIGITS + 4);
		sprintf(ext_name,"%s (%d)", name, count);
	}
	else
	{
		ext_name = name;
	}

	if (is_icon)
	{
		fw->visible_icon_name = ext_name;
	}
	else
	{
		fw->visible_name = ext_name;
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
		fw, STITLE_DIR(pstyle->flags),
		STITLE_TEXT_DIR_MODE(pstyle->flags), &width, &offset);
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
		if (SFHAS_WINDOW_FONT(*pstyle) && SGET_WINDOW_FONT(*pstyle) &&
		    (fw->title_font =
		     FlocaleLoadFont(dpy, SGET_WINDOW_FONT(*pstyle), "FVWM")))
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
	int height;

	height = (IS_ICON_FONT_LOADED(fw)) ? fw->icon_font->height : 0;
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
		if (SFHAS_ICON_FONT(*pstyle) && SGET_ICON_FONT(*pstyle) &&
		    (fw->icon_font =
		     FlocaleLoadFont(dpy, SGET_ICON_FONT(*pstyle), "FVWM")))
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
	if (height)
	{
		resize_icon_title_height(fw, height - fw->icon_font->height);
		/* this repositions the icon even if the window is not
		 * iconified */
		DrawIconWindow(fw);
	}

	return;
}

void setup_style_and_decor(
	FvwmWindow *fw, window_style *pstyle, short *buttons)
{
	/* first copy the static styles into the window struct */
	memcpy(&(FW_COMMON_FLAGS(fw)), &(pstyle->flags.common),
	       sizeof(common_flags_type));
	fw->wShaped = None;
	if (FShapesSupported)
	{
		int i;
		unsigned int u;
		Bool b;
		int boundingShaped;

		/* suppress compiler warnings w/o shape extension */
		i = 0;
		u = 0;
		b = False;

		FShapeSelectInput(dpy, FW_W(fw), FShapeNotifyMask);
		if (FShapeQueryExtents(
			    dpy, FW_W(fw), &boundingShaped, &i, &i, &u, &u, &b,
			    &i, &i, &u, &u))
		{
			fw->wShaped = boundingShaped;
		}
	}

	/*  Assume that we'll decorate */
	SET_HAS_BORDER(fw, 1);
	SET_HAS_HANDLES(fw, 1);
	SET_HAS_TITLE(fw, 1);

#ifdef USEDECOR
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
#endif

	GetMwmHints(fw);
	GetOlHints(fw);

	fw->buttons = SIS_BUTTON_DISABLED(&pstyle->flags);
	SelectDecor(fw, pstyle, buttons);

	if (IS_TRANSIENT(fw) && !pstyle->flags.do_decorate_transient)
	{
		SET_HAS_BORDER(fw, 0);
		SET_HAS_HANDLES(fw, 0);
		SET_HAS_TITLE(fw, 0);
	}
	/* set boundary width to zero for shaped windows */
	if (FHaveShapeExtension)
	{
		if (fw->wShaped)
		{
			set_window_border_size(fw, 0);
			SET_HAS_BORDER(fw, 0);
			SET_HAS_HANDLES(fw, 0);
		}
	}

	/****** window colors ******/
	update_window_color_style(fw, pstyle);
	update_window_color_hi_style(fw, pstyle);

	/****** window shading ******/
	fw->shade_anim_steps = pstyle->shade_anim_steps;

	/****** GNOME style hints ******/
	if (!SDO_IGNORE_GNOME_HINTS(pstyle->flags))
	{
		GNOME_GetStyle(fw, pstyle);
	}

	return;
}

void setup_icon_boxes(FvwmWindow *fw, window_style *pstyle)
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

void destroy_icon_boxes(FvwmWindow *fw)
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

void change_icon_boxes(FvwmWindow *fw, window_style *pstyle)
{
	destroy_icon_boxes(fw);
	setup_icon_boxes(fw, pstyle);

	return;
}

void setup_layer(FvwmWindow *fw, window_style *pstyle)
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

void setup_frame_size_limits(FvwmWindow *fw, window_style *pstyle)
{
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

Bool setup_window_placement(
	FvwmWindow *fw, window_style *pstyle, rectangle *attr_g)
{
	int client_argc = 0;
	char **client_argv = NULL;
	XrmValue rm_value;
	int tmpno1 = -1, tmpno2 = -1, tmpno3 = -1, spargs = 0;
	/* Used to parse command line of clients for specific desk requests. */
	/* Todo: check for multiple desks. */
	XrmDatabase db = NULL;
	static XrmOptionDescRec table [] = {
		/* Want to accept "-workspace N" or -xrm "fvwm*desk:N" as
		 * options to specify the desktop. I have to include dummy
		 * options that are meaningless since Xrm seems to allow -w to
		 * match -workspace if there would be no ambiguity. */
		{"-workspacf", "*junk", XrmoptionSepArg, (caddr_t) NULL},
		{"-workspace", "*desk",	XrmoptionSepArg, (caddr_t) NULL},
		{"-xrn", NULL, XrmoptionResArg, (caddr_t) NULL},
		{"-xrm", NULL, XrmoptionResArg, (caddr_t) NULL},
	};

	/* Find out if the client requested a specific desk on the command
	 * line. */
	/*  RBW - 11/20/1998 - allow a desk of -1 to work.  */

	if (XGetCommand(dpy, FW_W(fw), &client_argv, &client_argc) &&
	    client_argc > 0 && client_argv != NULL)
	{
		/* Get global X resources */
		MergeXResources(dpy, &db, False);

		/* command line takes precedence over all */
		MergeCmdLineResources(
			&db, table, 4, client_argv[0], &client_argc,
			client_argv, True);

		/* Now parse the database values: */
		if (GetResourceString(db, "desk", client_argv[0], &rm_value) &&
		    rm_value.size != 0)
		{
			SGET_START_DESK(*pstyle) = atoi(rm_value.addr);
			/*  RBW - 11/20/1998  */
			if (SGET_START_DESK(*pstyle) > -1)
			{
				SSET_START_DESK(
					*pstyle, SGET_START_DESK(*pstyle) + 1);
			}
			pstyle->flags.use_start_on_desk = 1;
		}
		if (GetResourceString(
			    db, "fvwmscreen", client_argv[0], &rm_value) &&
		    rm_value.size != 0)
		{
			SSET_START_SCREEN(
				*pstyle, FScreenGetScreenArgument(
					rm_value.addr, 'c'));
			pstyle->flags.use_start_on_screen = 1;
		}
		if (GetResourceString(db, "page", client_argv[0], &rm_value) &&
		    rm_value.size != 0)
		{
			spargs = sscanf(
				rm_value.addr, "%d %d %d", &tmpno1, &tmpno2,
				&tmpno3);
			switch (spargs)
			{
			case 1:
				pstyle->flags.use_start_on_desk = 1;
				SSET_START_DESK(*pstyle, (tmpno1 > -1) ?
						tmpno1 + 1 : tmpno1);
				break;
			case 2:
				pstyle->flags.use_start_on_desk = 1;
				SSET_START_PAGE_X(*pstyle, (tmpno1 > -1) ?
						  tmpno1 + 1 : tmpno1);
				SSET_START_PAGE_Y(*pstyle, (tmpno2 > -1) ?
						  tmpno2 + 1 : tmpno2);
				break;
			case 3:
				pstyle->flags.use_start_on_desk = 1;
				SSET_START_DESK(*pstyle, (tmpno1 > -1) ?
						tmpno1 + 1 : tmpno1);
				SSET_START_PAGE_X(*pstyle, (tmpno2 > -1) ?
						  tmpno2 + 1 : tmpno2);
				SSET_START_PAGE_Y(*pstyle, (tmpno3 > -1) ?
						  tmpno3 + 1 : tmpno3);
				break;
			default:
				break;
			}
		}

		XFreeStringList(client_argv);
		XrmDestroyDatabase(db);
	}

	return PlaceWindow(
		fw, &pstyle->flags, attr_g, SGET_START_DESK(*pstyle),
		SGET_START_PAGE_X(*pstyle), SGET_START_PAGE_Y(*pstyle),
		SGET_START_SCREEN(*pstyle), PLACE_INITIAL);
}

void setup_placement_penalty(FvwmWindow *fw, window_style *pstyle)
{
	int i;

	if (!SHAS_PLACEMENT_PENALTY(&pstyle->flags))
	{
		SSET_NORMAL_PLACEMENT_PENALTY(*pstyle, 1);
		SSET_ONTOP_PLACEMENT_PENALTY(*pstyle, PLACEMENT_AVOID_ONTOP);
		SSET_ICON_PLACEMENT_PENALTY(*pstyle, PLACEMENT_AVOID_ICON);
		SSET_STICKY_PLACEMENT_PENALTY(*pstyle, PLACEMENT_AVOID_STICKY);
		SSET_BELOW_PLACEMENT_PENALTY(*pstyle, PLACEMENT_AVOID_BELOW);
		SSET_EWMH_STRUT_PLACEMENT_PENALTY(
			*pstyle, PLACEMENT_AVOID_EWMH_STRUT);
	}
	if (!SHAS_PLACEMENT_PERCENTAGE_PENALTY(&pstyle->flags))
	{
		SSET_99_PLACEMENT_PERCENTAGE_PENALTY(
			*pstyle, PLACEMENT_AVOID_COVER_99);
		SSET_95_PLACEMENT_PERCENTAGE_PENALTY(
			*pstyle, PLACEMENT_AVOID_COVER_95);
		SSET_85_PLACEMENT_PERCENTAGE_PENALTY(
			*pstyle, PLACEMENT_AVOID_COVER_85);
		SSET_75_PLACEMENT_PERCENTAGE_PENALTY(
			*pstyle, PLACEMENT_AVOID_COVER_75);
	}
	for (i = 0; i < 6; i++)
	{
		fw->placement_penalty[i] = (*pstyle).placement_penalty[i];
	}
	for (i = 0; i < 4; i++)
	{
		fw->placement_percentage_penalty[i] =
			(*pstyle).placement_percentage_penalty[i];
	}

	return;
}

void get_default_window_attributes(
	FvwmWindow *fw, unsigned long *pvaluemask,
	XSetWindowAttributes *pattributes)
{
	if (Pdepth < 2)
	{
		*pvaluemask = CWBackPixmap;
		if (IS_STICKY(fw))
		{
			pattributes->background_pixmap = Scr.sticky_gray_pixmap;
		}
		else
		{
			pattributes->background_pixmap = Scr.light_gray_pixmap;
		}
	}
	else
	{
		*pvaluemask = CWBackPixel;
		pattributes->background_pixel = fw->colors.back;
		pattributes->background_pixmap = None;
	}

	*pvaluemask |= CWBackingStore | CWCursor | CWSaveUnder;
	pattributes->backing_store = NotUseful;
	pattributes->cursor = Scr.FvwmCursors[CRS_DEFAULT];
	pattributes->save_under = False;

	return;
}

void setup_frame_window(
	FvwmWindow *fw)
{
	XSetWindowAttributes attributes;
	int valuemask;

	valuemask = CWBackingStore | CWBackPixmap | CWEventMask | CWSaveUnder;
	attributes.backing_store = NotUseful;
	attributes.background_pixmap = None;
	attributes.event_mask = XEVMASK_FRAMEW;
	attributes.save_under = False;
	/* create the frame window, child of root, grandparent of client */
	FW_W_FRAME(fw) = XCreateWindow(
		dpy, Scr.Root, fw->frame_g.x, fw->frame_g.y,
		fw->frame_g.width, fw->frame_g.height, 0, CopyFromParent,
		InputOutput, CopyFromParent, valuemask, &attributes);
	XSaveContext(dpy, FW_W(fw), FvwmContext, (caddr_t) fw);
	XSaveContext(dpy, FW_W_FRAME(fw), FvwmContext, (caddr_t) fw);

	return;
}

void setup_decor_window(
	FvwmWindow *fw, int valuemask, XSetWindowAttributes *pattributes)
{
	/* stash attribute bits in case BorderStyle TiledPixmap overwrites */
	Pixmap TexturePixmap;
	Pixmap TexturePixmapSave = pattributes->background_pixmap;

	valuemask |= CWBorderPixel | CWColormap | CWEventMask;
	pattributes->border_pixel = 0;
	pattributes->colormap = Pcmap;
	pattributes->event_mask = XEVMASK_DECORW;
	if (DFS_FACE_TYPE(GetDecor(fw, BorderStyle.inactive.style)) ==
	    TiledPixmapButton)
	{
		TexturePixmap = GetDecor(fw,BorderStyle.inactive.u.p->picture);
		if (TexturePixmap)
		{
			pattributes->background_pixmap = TexturePixmap;
			valuemask = (valuemask & ~CWBackPixel) | CWBackPixmap;
		}
	}

	/* restore background */
	pattributes->background_pixmap = TexturePixmapSave;

	return;
}

void setup_title_window(
	FvwmWindow *fw, int valuemask, XSetWindowAttributes *pattributes)
{
	valuemask |= CWCursor | CWEventMask;
	pattributes->cursor = Scr.FvwmCursors[CRS_TITLE];
	pattributes->event_mask = XEVMASK_TITLEW;

	FW_W_TITLE(fw) = XCreateWindow(
		dpy, FW_W_FRAME(fw), 0, 0, 1, 1, 0, CopyFromParent,
		InputOutput, CopyFromParent, valuemask, pattributes);
	XSaveContext(dpy, FW_W_TITLE(fw), FvwmContext, (caddr_t) fw);

	return;
}

void destroy_title_window(FvwmWindow *fw, Bool do_only_delete_context)
{
	if (!do_only_delete_context)
	{
		XDestroyWindow(dpy, FW_W_TITLE(fw));
		FW_W_TITLE(fw) = None;
	}
	XDeleteContext(dpy, FW_W_TITLE(fw), FvwmContext);
	FW_W_TITLE(fw) = None;

	return;
}

void change_title_window(
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

void setup_button_windows(
	FvwmWindow *fw, int valuemask, XSetWindowAttributes *pattributes,
	short buttons)
{
	int i;
	Bool has_button;

	valuemask |= CWCursor | CWEventMask;
	pattributes->cursor = Scr.FvwmCursors[CRS_SYS];
	pattributes->event_mask = XEVMASK_BUTTONW;

	for (i = 0; i < NUMBER_OF_BUTTONS; i++)
	{
		has_button = (((!(i & 1) && i / 2 < Scr.nr_left_buttons) ||
			       ( (i & 1) && i / 2 < Scr.nr_right_buttons)) &&
			      (buttons & (1 << i)));
		if (FW_W_BUTTON(fw, i) == None && has_button)
		{
			FW_W_BUTTON(fw, i) =
				XCreateWindow(
					dpy, FW_W_FRAME(fw), 0, 0, 1, 1, 0,
					CopyFromParent, InputOutput,
					CopyFromParent, valuemask, pattributes);
			XSaveContext(
				dpy, FW_W_BUTTON(fw, i), FvwmContext,
				(caddr_t)fw);
		}
		else if (FW_W_BUTTON(fw, i) != None && !has_button)
		{
			/* destroy the current button window */
			XDestroyWindow(dpy, FW_W_BUTTON(fw, i));
			XDeleteContext(dpy, FW_W_BUTTON(fw, i), FvwmContext);
			FW_W_BUTTON(fw, i) = None;
		}
	}

	return;
}

void destroy_button_windows(FvwmWindow *fw, Bool do_only_delete_context)
{
	int i;

	for (i = 0; i < NUMBER_OF_BUTTONS; i++)
	{
		if (FW_W_BUTTON(fw, i) != None)
		{
			if (!do_only_delete_context)
			{
				XDestroyWindow(dpy, FW_W_BUTTON(fw, i));
				FW_W_BUTTON(fw, i) = None;
			}
			XDeleteContext(dpy, FW_W_BUTTON(fw, i), FvwmContext);
			FW_W_BUTTON(fw, i) = None;
		}
	}

	return;
}

void change_button_windows(
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


void setup_parent_window(FvwmWindow *fw)
{
	size_borders b;

	XSetWindowAttributes attributes;
	int valuemask = CWBackingStore | CWBackPixmap | CWCursor | CWEventMask |
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
		fw->frame_g.width - b.total_size.width,
		fw->frame_g.height - b.total_size.height,
		0, CopyFromParent, InputOutput, CopyFromParent, valuemask,
		&attributes);

	XSaveContext(dpy, FW_W_PARENT(fw), FvwmContext, (caddr_t) fw);

	return;
}

void setup_resize_handle_cursors(FvwmWindow *fw)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	int i;

	if (!HAS_BORDER(fw))
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

void setup_resize_handle_windows(FvwmWindow *fw)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	int i;

	if (!HAS_BORDER(fw))
	{
		return;
	}
	valuemask = CWEventMask | CWBackingStore | CWSaveUnder;
	attributes.event_mask = XEVMASK_BORDERW;
	attributes.backing_store = NotUseful;
	attributes.save_under = False;
	/* Just dump the windows any old place and let frame_setup_window take
	 * care of the mess */
	for (i = 0; i < 4; i++)
	{
		FW_W_CORNER(fw, i) = XCreateWindow(
			dpy, FW_W_FRAME(fw), -1, -1, 1, 1, 0, 0, InputOutput,
			DefaultVisual(dpy, Scr.screen), valuemask, &attributes);
		XSaveContext(
			dpy, FW_W_CORNER(fw, i), FvwmContext, (caddr_t)fw);
		FW_W_SIDE(fw, i) = XCreateWindow(
			dpy, FW_W_FRAME(fw), -1, -1, 1, 1, 0, 0, InputOutput,
			DefaultVisual(dpy, Scr.screen), valuemask, &attributes);
		XSaveContext(dpy, FW_W_SIDE(fw, i), FvwmContext, (caddr_t)fw);
	}
	setup_resize_handle_cursors(fw);

	return;
}

void destroy_resize_handle_windows(
	FvwmWindow *fw, Bool do_only_delete_context)
{
	int i;

	for (i = 0; i < 4 ; i++)
	{
		if (!do_only_delete_context)
		{
			XDestroyWindow(dpy, FW_W_SIDE(fw, i));
			XDestroyWindow(dpy, FW_W_CORNER(fw, i));
			FW_W_SIDE(fw, i) = None;
			FW_W_CORNER(fw, i) = None;
		}
		XDeleteContext(dpy, FW_W_SIDE(fw, i), FvwmContext);
		XDeleteContext(dpy, FW_W_CORNER(fw, i), FvwmContext);
	}

	return;
}

void change_resize_handle_windows(FvwmWindow *fw)
{
	if (HAS_BORDER(fw) && FW_W_SIDE(fw, 0) == None)
	{
		setup_resize_handle_windows(fw);
	}
	else if (!HAS_BORDER(fw) && FW_W_SIDE(fw, 0) != None)
	{
		destroy_resize_handle_windows(fw, False);
	}
	else
	{
		setup_resize_handle_cursors(fw);
	}

	return;
}

void setup_frame_stacking(FvwmWindow *fw)
{
	int i;
	int n;
	Window w[10 + NUMBER_OF_BUTTONS];

	/* Stacking order (top to bottom):
	 *  - Parent window
	 *  - Title and buttons
	 *  - Corner handles
	 *  - Side handles
	 */
	n = 0;
	w[n] = FW_W_PARENT(fw);
	n++;
	if (HAS_TITLE(fw))
	{
		for (i = 0; i < NUMBER_OF_BUTTONS; i++)
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
	XRestackWindows(dpy, w, n);

	return;
}

void setup_auxiliary_windows(
	FvwmWindow *fw, Bool setup_frame_and_parent, short buttons)
{
	unsigned long valuemask_save = 0;
	XSetWindowAttributes attributes;

	get_default_window_attributes(fw, &valuemask_save, &attributes);

	/****** frame window ******/
	if (setup_frame_and_parent)
	{
		setup_frame_window(fw);
		setup_parent_window(fw);
	}

	/****** resize handle windows ******/
	setup_resize_handle_windows(fw);

	/****** title window ******/
	if (HAS_TITLE(fw))
	{
		setup_title_window(fw, valuemask_save, &attributes);
		setup_button_windows(fw, valuemask_save, &attributes, buttons);
	}

	/****** setup frame stacking order ******/
	setup_frame_stacking(fw);
	XMapSubwindows (dpy, FW_W_FRAME(fw));

	return;
}

void setup_frame_attributes(
	FvwmWindow *fw, window_style *pstyle)
{
	int i;
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
	XChangeWindowAttributes(dpy, FW_W(fw), CWBackingStore, &xswa);
	if (pstyle->flags.use_backing_store == BACKINGSTORE_OFF)
	{
		xswa.backing_store = NotUseful;
	}
	if (HAS_TITLE(fw))
	{
		XChangeWindowAttributes(
			dpy, FW_W_TITLE(fw), CWBackingStore, &xswa);
		for (i = 0; i < NUMBER_OF_BUTTONS; i++)
		{
			if (FW_W_BUTTON(fw, i))
			{
				XChangeWindowAttributes(
					dpy, FW_W_BUTTON(fw, i),
					CWBackingStore, &xswa);
			}
		}
	}

	/* parent_relative is applied to the frame and the parent */
	xswa.background_pixmap = pstyle->flags.use_parent_relative
		? ParentRelative : None;
	XChangeWindowAttributes(dpy, FW_W_FRAME(fw), CWBackPixmap, &xswa);
	XChangeWindowAttributes(dpy, FW_W_PARENT(fw), CWBackPixmap, &xswa);

	/* Save_under is only useful on the frame */
	xswa.save_under = pstyle->flags.do_save_under
		? Scr.flags.do_save_under : NotUseful;
	XChangeWindowAttributes(dpy, FW_W_FRAME(fw), CWSaveUnder, &xswa);

	return;
}

void destroy_auxiliary_windows(
	FvwmWindow *fw, Bool destroy_frame_and_parent)
{
	if (destroy_frame_and_parent)
	{
		XDestroyWindow(dpy, FW_W_FRAME(fw));
		XDeleteContext(dpy, FW_W_FRAME(fw), FvwmContext);
		XDeleteContext(dpy, FW_W_PARENT(fw), FvwmContext);
		XDeleteContext(dpy, FW_W(fw), FvwmContext);
	}
	if (HAS_TITLE(fw))
	{
		destroy_title_window(fw, True);
	}
	if (HAS_TITLE(fw))
	{
		destroy_button_windows(fw, True);
	}
	if (HAS_BORDER(fw))
	{
		destroy_resize_handle_windows(fw, True);
	}

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
			  (int)WAS_ICON_HINT_PROVIDED(fw), fw->name));
	}

	return;
}

static void setup_icon(FvwmWindow *fw, window_style *pstyle)
{
	increase_icon_hint_count(fw);
	/* find a suitable icon pixmap */
	if ((fw->wmhints) && (fw->wmhints->flags & IconWindowHint))
	{
		if (SHAS_ICON(&pstyle->flags) &&
		    SICON_OVERRIDE(pstyle->flags) == ICON_OVERRIDE)
		{
			ICON_DBG((stderr,"si: iwh ignored '%s'\n", fw->name));
			fw->icon_bitmap_file = SGET_ICON_NAME(*pstyle);
		}
		else
		{
			ICON_DBG((stderr,"si: using iwh '%s'\n", fw->name));
			fw->icon_bitmap_file = NULL;
		}
	}
	else if ((fw->wmhints) && (fw->wmhints->flags & IconPixmapHint))
	{
		if (SHAS_ICON(&pstyle->flags) &&
		    SICON_OVERRIDE(pstyle->flags) != NO_ICON_OVERRIDE)
		{
			ICON_DBG((stderr,"si: iph ignored '%s'\n", fw->name));
			fw->icon_bitmap_file = SGET_ICON_NAME(*pstyle);
		}
		else
		{
			ICON_DBG((stderr,"si: using iph '%s'\n", fw->name));
			fw->icon_bitmap_file = NULL;
		}
	}
	else if (SHAS_ICON(&pstyle->flags))
	{
		/* an icon was specified */
		ICON_DBG((stderr,"si: using style '%s'\n", fw->name));
		fw->icon_bitmap_file = SGET_ICON_NAME(*pstyle);
	}
	else
	{
		/* use default icon */
		ICON_DBG((stderr,"si: using default '%s'\n", fw->name));
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
	setup_visible_name(fw, True);


	/* wait until the window is iconified and the icon window is mapped
	 * before creating the icon window
	 */
	FW_W_ICON_TITLE(fw) = None;

	EWMH_SetVisibleName(fw, True);
	BroadcastWindowIconNames(fw, False, True);
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
	if (FW_W_ICON_TITLE(fw))
	{
		if (IS_PIXMAP_OURS(fw))
		{
			XFreePixmap(dpy, fw->iconPixmap);
			if (fw->icon_maskPixmap != None)
			{
				XFreePixmap(dpy, fw->icon_maskPixmap);
			}
		}
		XDestroyWindow(dpy, FW_W_ICON_TITLE(fw));
		XDeleteContext(dpy, FW_W_ICON_TITLE(fw), FvwmContext);
	}
	if ((IS_ICON_OURS(fw))&&(FW_W_ICON_PIXMAP(fw) != None))
		XDestroyWindow(dpy, FW_W_ICON_PIXMAP(fw));
	if (FW_W_ICON_PIXMAP(fw) != None)
	{
		XDeleteContext(dpy, FW_W_ICON_PIXMAP(fw), FvwmContext);
	}
	clear_icon(fw);

	return;
}

void change_icon(FvwmWindow *fw, window_style *pstyle)
{
	destroy_icon(fw);
	setup_icon(fw, pstyle);

	return;
}

#ifdef MINI_ICONS
void setup_mini_icon(FvwmWindow *fw, window_style *pstyle)
{
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
		fw->mini_icon = CachePicture(dpy, Scr.NoFocusWin, NULL,
					     fw->mini_pixmap_file,
					     Scr.ColorLimit);
		if (fw->mini_icon != NULL)
		{
			BroadcastMiniIcon(
				M_MINI_ICON,
				FW_W(fw), FW_W_FRAME(fw), (unsigned long)fw,
				fw->mini_icon->width, fw->mini_icon->height,
				fw->mini_icon->depth, fw->mini_icon->picture,
				fw->mini_icon->mask, fw->mini_pixmap_file);
		}
	}
	else
	{
		fw->mini_icon = NULL;
	}

	return;
}

void destroy_mini_icon(FvwmWindow *fw)
{
	if (fw->mini_icon)
	{
		DestroyPicture(dpy, fw->mini_icon);
		fw->mini_icon = 0;
	}

	return;
}

void change_mini_icon(FvwmWindow *fw, window_style *pstyle)
{
	Picture *old_mi = fw->mini_icon;
	destroy_mini_icon(fw);
	setup_mini_icon(fw, pstyle);
	if (old_mi != NULL && fw->mini_icon == 0)
	{
		/* this case is not handled in setup_mini_icon, so we must
		 * broadcast here explicitly */
		BroadcastMiniIcon(
			M_MINI_ICON, FW_W(fw), FW_W_FRAME(fw),
			(unsigned long)fw, 0, 0, 0, 0, 0, "");
	}

	return;
}
#endif

void setup_focus_policy(FvwmWindow *fw)
{
	focus_grab_buttons(fw, False);

	return;
}


void setup_key_and_button_grabs(FvwmWindow *fw)
{
#ifdef BUGS_ARE_COOL
	/* dv (29-May-2001): If keys are grabbed separately for C_WINDOW and
	 * the other contexts, new windows have problems when bindings are
	 * removed.  Therefore, grab all keys in a single pass through the
	 * list. */
	GrabAllWindowKeysAndButtons(
		dpy, FW_W_PARENT(fw), Scr.AllBindings, C_WINDOW,
		GetUnusedModifiers(), None, True);
	GrabAllWindowKeys(
		dpy, FW_W_FRAME(fw), Scr.AllBindings,
		C_TITLE|C_RALL|C_LALL|C_SIDEBAR, GetUnusedModifiers(), True);
#endif
	GrabAllWindowButtons(
		dpy, FW_W_PARENT(fw), Scr.AllBindings, C_WINDOW,
		GetUnusedModifiers(), None, True);
	GrabAllWindowKeys(
		dpy, FW_W_FRAME(fw), Scr.AllBindings,
		C_TITLE|C_RALL|C_LALL|C_SIDEBAR|C_WINDOW, GetUnusedModifiers(),
		True);
	setup_focus_policy(fw);
	/* Special handling for MouseFocusClickRaises *only*. If a
	 * MouseFocusClickRaises window was so raised, it is now ungrabbed. If
	 * it's no longer on top of its layer because of the new window we just
	 * added, we have to restore the grab so it can be raised again. */
	if (Scr.Ungrabbed && DO_RAISE_MOUSE_FOCUS_CLICK(Scr.Ungrabbed) &&
	    (HAS_SLOPPY_FOCUS(Scr.Ungrabbed) || HAS_MOUSE_FOCUS(Scr.Ungrabbed))
	    && !is_on_top_of_layer(Scr.Ungrabbed))
	{
		focus_grab_buttons(Scr.Ungrabbed, False);
	}

	return;
}


/***********************************************************************
 *
 *  Procedure:
 *	AddWindow - add a new window to the fvwm list
 *
 ***********************************************************************/
FvwmWindow *AddWindow(
	Window w, FvwmWindow *ReuseWin, Bool is_menu)
{
	/* new fvwm window structure */
	register FvwmWindow *fw = NULL;
	FvwmWindow *tmpfw = NULL;
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
	extern FvwmWindow *colormap_win;
	extern Bool PPosOverride;
	int do_shade = 0;
	int shade_dir = 0;
	int do_maximize = 0;
	Bool used_sm = False;
	Bool do_resize_too = False;
	size_borders b;

	/****** init window structure ******/
	if (!setup_window_structure(&tmpfw, w, ReuseWin))
	{
		fvwm_msg(ERR, "AddWindow",
			 "Bad return code from setup_window_structure");
		return NULL;
	}
	fw = tmpfw;

	/****** safety check, window might disappear before we get to it ******/
	if (!PPosOverride &&
	    XGetGeometry(dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
			 &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
	{
		free((char *)fw);
		return NULL;
	}

	/****** Make sure the client window still exists.  We don't want to
	 * leave an orphan frame window if it doesn't.  Since we now have the
	 * server grabbed, the window can't disappear later without having been
	 * reparented, so we'll get a DestroyNotify for it.  We won't have
	 * gotten one for anything up to here, however. ******/
	MyXGrabServer(dpy);
	if (XGetGeometry(dpy, w, &JunkRoot, &JunkX, &JunkY,
			 &JunkWidth, &JunkHeight,
			 &JunkBW,  &JunkDepth) == 0)
	{
		free((char *)fw);
		MyXUngrabServer(dpy);
		return NULL;
	}

	/****** window name ******/
	setup_window_name(fw);
	setup_class_and_resource(fw);
	setup_window_attr(fw, &wattr);
	setup_wm_hints(fw);

	/****** basic style and decor ******/
	/* If the window is in the NoTitle list, or is a transient, dont
	 * decorate it.  If its a transient, and DecorateTransients was
	 * specified, decorate anyway. */
	/* get merged styles */
	lookup_style(fw, &style);
	sflags = SGET_FLAGS_POINTER(style);
	SET_TRANSIENT(
		fw, !!XGetTransientForHint(
			dpy, FW_W(fw), &FW_W_TRANSIENTFOR (fw)));
	if (is_menu)
	{
		SET_TEAR_OFF_MENU(fw, 1);
	}
	fw->decor = NULL;
	setup_style_and_decor(fw, &style, &buttons);

	/****** fonts ******/
	setup_window_font(fw, &style, False);
	setup_icon_font(fw, &style, False);

	/***** visible window name ****/
	setup_visible_name(fw, False);
	EWMH_SetVisibleName(fw, False);

	/****** state setup ******/
	setup_icon_boxes(fw, &style);
	SET_ICONIFIED(fw, 0);
	SET_ICON_UNMAPPED(fw, 0);
	SET_MAXIMIZED(fw, 0);
	/*
	 * Reparenting generates an UnmapNotify event, followed by a MapNotify.
	 * Set the map state to FALSE to prevent a transition back to
	 * WithdrawnState in HandleUnmapNotify.  Map state gets set corrected
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

	/***** placement penalities *****/
	setup_placement_penalty(fw, &style);
	/*
	 * MatchWinToSM changes fw->attr and the stacking order.
	 * Thus it is important have this call *after* PlaceWindow and the
	 * stacking order initialization.
	 */
	get_window_borders(fw, &b);
	used_sm = MatchWinToSM(fw, &do_shade, &shade_dir, &do_maximize);
	if (used_sm)
	{
		/* read the requested absolute geometry */
		gravity_translate_to_northwest_geometry_no_bw(
			fw->hints.win_gravity, fw, &fw->normal_g,
			&fw->normal_g);
		gravity_resize(
			fw->hints.win_gravity, &fw->normal_g,
			b.total_size.width, b.total_size.height);
		fw->frame_g = fw->normal_g;
		fw->frame_g.x -= Scr.Vx;
		fw->frame_g.y -= Scr.Vy;

		/****** calculate frame size ******/
		setup_frame_size_limits(fw, &style);
		constrain_size(
			fw, (unsigned int *)&fw->frame_g.width,
			(unsigned int *)&fw->frame_g.height, 0, 0, 0);

		/****** maximize ******/
		if (do_maximize)
		{
			SET_MAXIMIZED(fw, 1);
			constrain_size(
				fw, (unsigned int *)&fw->max_g.width,
				(unsigned int *)&fw->max_g.height, 0, 0,
				CS_UPDATE_MAX_DEFECT);
			get_relative_geometry(&fw->frame_g, &fw->max_g);
		}
		else
		{
			get_relative_geometry(&fw->frame_g, &fw->normal_g);
		}
	}
	else
	{
		rectangle attr_g;

		if (IS_SHADED(fw))
		{
			do_shade = 1;
			shade_dir = SHADED_DIR(fw);
			SET_SHADED(fw, 0);
		}
		/* Tentative size estimate */
		fw->frame_g.width = wattr.width + b.total_size.width;
		fw->frame_g.height = wattr.height + b.total_size.height;

		/****** calculate frame size ******/
		setup_frame_size_limits(fw, &style);

		/****** layer ******/
		setup_layer(fw, &style);

		/****** window placement ******/
		attr_g.x = wattr.x;
		attr_g.y = wattr.y;
		attr_g.width = wattr.width;
		attr_g.height = wattr.height;
		do_resize_too = setup_window_placement(fw, &style, &attr_g);
		wattr.x = attr_g.x;
		wattr.y = attr_g.y;

		/* set up geometry */
		fw->frame_g.x = wattr.x;
		fw->frame_g.y = wattr.y;
		fw->frame_g.width = wattr.width + b.total_size.width;
		fw->frame_g.height = wattr.height + b.total_size.height;
		gravity_constrain_size(
			fw->hints.win_gravity, fw, &fw->frame_g, 0);

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

	/****** arrange the frame ******/
	frame_force_setup_window(
		fw, fw->frame_g.x, fw->frame_g.y,
		fw->frame_g.width, fw->frame_g.height, True);

	/****** grab keys and buttons ******/
	setup_key_and_button_grabs(fw);

	/****** place the window in the stack ring ******/
	if (!position_new_window_in_stack_ring(fw, SDO_START_LOWERED(sflags)))
	{
		XWindowChanges xwc;
		xwc.sibling = FW_W_FRAME(get_next_window_in_stack_ring(fw));
		xwc.stack_mode = Above;
		XConfigureWindow(
			dpy, FW_W_FRAME(fw), CWSibling|CWStackMode, &xwc);
	}

	/****** now we can sefely ungrab the server ******/
	MyXUngrabServer(dpy);

	/****** inform modules of new window ******/
	BroadcastConfig(M_ADD_WINDOW,fw);
	BroadcastWindowIconNames(fw, True, False);

	/* these are sent and broadcast before res_{class,name} for the benefit
	 * of FvwmIconBox which can't handle M_ICON_FILE after M_RES_NAME */
	/****** icon and mini icon ******/
	/* migo (20-Jan-2000): the logic is to unset this flag on NULL values */
	SET_WAS_ICON_NAME_PROVIDED(fw, 1);
	setup_icon(fw, &style);
#ifdef MINI_ICONS
	setup_mini_icon(fw, &style);
#endif

	BroadcastName(M_RES_CLASS,FW_W(fw),FW_W_FRAME(fw),
		      (unsigned long)fw,fw->class.res_class);
	BroadcastName(M_RES_NAME,FW_W(fw),FW_W_FRAME(fw),
		      (unsigned long)fw,fw->class.res_name);

	/****** stick window ******/
	if (IS_STICKY(fw) && (!(fw->hints.flags & USPosition) || used_sm))
	{
		/* If it's sticky and the user didn't ask for an explicit
		 * position, force it on screen now.  Don't do that with
		 * USPosition because we have to assume the user knows what
		 * (s)he is doing.  This is necessary e.g. if we want a sticky
		 * 'panel' in FvwmButtons but don't want to see when it's
		 * mapped in the void. */
		if (!IsRectangleOnThisPage(&fw->frame_g, Scr.CurrentDesk))
		{
			SET_STICKY(fw, 0);
			handle_stick(
				NULL, &Event, FW_W_FRAME(fw), fw, C_FRAME, "",
			        0, 1);
		}
	}

	/****** resize window ******/
	if (do_resize_too)
	{
		XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
			     Scr.MyDisplayHeight,
			     fw->frame_g.x + (fw->frame_g.width>>1),
			     fw->frame_g.y + (fw->frame_g.height>>1));
		Event.xany.type = ButtonPress;
		Event.xbutton.button = 1;
		Event.xbutton.x_root = fw->frame_g.x + (fw->frame_g.width>>1);
		Event.xbutton.y_root = fw->frame_g.y + (fw->frame_g.height>>1);
		Event.xbutton.x = (fw->frame_g.width>>1);
		Event.xbutton.y = (fw->frame_g.height>>1);
		Event.xbutton.subwindow = None;
		Event.xany.window = FW_W(fw);
		CMD_Resize(NULL, &Event , FW_W(fw), fw, C_WINDOW, "", 0);
	}

	/****** window colormap ******/
	InstallWindowColormaps(colormap_win);

	/****** ewmh setup *******/
	EWMH_WindowInit(fw);

	/****** gnome setup ******/
	/* set GNOME hints on the window from flags set on fw */
	GNOME_SetHints(fw);
	GNOME_SetLayer(fw);
	GNOME_SetDesk(fw);
	GNOME_SetWinArea(fw);

	/****** windowshade ******/
	if (do_shade)
	{
		rectangle big_g;

		big_g = (IS_MAXIMIZED(fw)) ? fw->max_g : fw->frame_g;
		get_shaded_geometry(fw, &fw->frame_g, &fw->frame_g);
		XLowerWindow(dpy, FW_W_PARENT(fw));
		SET_SHADED(fw, 1);
		SET_SHADED_DIR(fw, do_shade);
		frame_setup_window(
			fw, fw->frame_g.x, fw->frame_g.y,
			fw->frame_g.width, fw->frame_g.height, False);
	}
	if (!XGetGeometry(dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY, &JunkWidth,
			  &JunkHeight, &JunkBW,  &JunkDepth))
	{
		/* The window has disappeared somehow.  For some reason we do
		 * not always get a DestroyNotify on the window, so make sure
		 * it is destroyed. */
		destroy_window(fw);
		fw = NULL;
	}

	return fw;
}


/***********************************************************************
 *
 *  Procedure:
 *	FetchWMProtocols - finds out which protocols the window supports
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/
void FetchWmProtocols(FvwmWindow *tmp)
{
	Atom *protocols = NULL, *ap;
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
			     (unsigned char **)&protocols)) == Success)
		{
			for (i = 0, ap = protocols; i < nitems; i++, ap++)
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
	}

	return;
}



/***********************************************************************
 *
 *  Procedure:
 *	GetWindowSizeHints - gets application supplied size info
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/

void GetWindowSizeHints(FvwmWindow *tmp)
{
	long supplied = 0;
	char *broken_cause ="";
	XSizeHints orig_hints;

	if (HAS_OVERRIDE_SIZE_HINTS(tmp) ||
	    !XGetWMNormalHints(dpy, FW_W(tmp), &tmp->hints, &supplied))
	{
		tmp->hints.flags = 0;
		memset(&orig_hints, 0, sizeof(orig_hints));
	}
	else
	{
		memcpy(&orig_hints, &tmp->hints, sizeof(orig_hints));
	}

	/* Beat up our copy of the hints, so that all important field are
	 * filled in! */
	if (tmp->hints.flags & PResizeInc)
	{
		SET_SIZE_INC_SET(tmp, 1);
		if (tmp->hints.width_inc <= 0)
		{
			if (tmp->hints.width_inc < 0 ||
			    (tmp->hints.width_inc == 0 &&
			     (tmp->hints.flags & PMinSize) &&
			     (tmp->hints.flags & PMaxSize) &&
			     tmp->hints.min_width != tmp->hints.max_width))
			{
				broken_cause = "width_inc";
			}
			tmp->hints.width_inc = 1;
			SET_SIZE_INC_SET(tmp, 0);
		}
		if (tmp->hints.height_inc <= 0)
		{
			if (tmp->hints.height_inc < 0 ||
			    (tmp->hints.height_inc == 0 &&
			     (tmp->hints.flags & PMinSize) &&
			     (tmp->hints.flags & PMaxSize) &&
			     tmp->hints.min_height != tmp->hints.max_height))
			{
				if (!*broken_cause)
				{
					broken_cause = "height_inc";
				}
			}
			tmp->hints.height_inc = 1;
			SET_SIZE_INC_SET(tmp, 0);
		}
	}
	else
	{
		SET_SIZE_INC_SET(tmp, 0);
		tmp->hints.width_inc = 1;
		tmp->hints.height_inc = 1;
	}

	if (tmp->hints.flags & PMinSize)
	{
		if (tmp->hints.min_width < 0 && !*broken_cause)
		{
			broken_cause = "min_width";
		}
		if (tmp->hints.min_height < 0 && !*broken_cause)
		{
			broken_cause = "min_height";
		}
	}
	else
	{
		if (tmp->hints.flags & PBaseSize)
		{
			tmp->hints.min_width = tmp->hints.base_width;
			tmp->hints.min_height = tmp->hints.base_height;
		}
		else
		{
			tmp->hints.min_width = 1;
			tmp->hints.min_height = 1;
		}
	}
	if (tmp->hints.min_width <= 0)
	{
		tmp->hints.min_width = 1;
	}
	if (tmp->hints.min_height <= 0)
	{
		tmp->hints.min_height = 1;
	}

	if (tmp->hints.flags & PMaxSize)
	{
		if (tmp->hints.max_width < tmp->hints.min_width)
		{
			tmp->hints.max_width = DEFAULT_MAX_MAX_WINDOW_WIDTH;
			if (!*broken_cause)
			{
				broken_cause = "max_width";
			}
		}
		if (tmp->hints.max_height < tmp->hints.min_height)
		{
			tmp->hints.max_height = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
			if (!*broken_cause)
			{
				broken_cause = "max_height";
			}
		}
	}
	else
	{
		tmp->hints.max_width = DEFAULT_MAX_MAX_WINDOW_WIDTH;
		tmp->hints.max_height = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
	}

	/*
	 * ICCCM says that PMinSize is the default if no PBaseSize is given,
	 * and vice-versa.
	 */

	if (tmp->hints.flags & PBaseSize)
	{
		if (tmp->hints.base_width < 0)
		{
			tmp->hints.base_width = 0;
			if (!*broken_cause)
			{
				broken_cause = "base_width";
			}
		}
		if (tmp->hints.base_height < 0)
		{
			tmp->hints.base_height = 0;
			if (!*broken_cause)
			{
				broken_cause = "base_height";
			}
		}
		if ((tmp->hints.base_width > tmp->hints.min_width) ||
		    (tmp->hints.base_height > tmp->hints.min_height))
		{
			/* In this case, doing the aspect ratio calculation
			   for window_size - base_size as prescribed by the
			   ICCCM is going to fail.
			   Resetting the flag disables the use of base_size
			   in aspect ratio calculation while it is still used
			   for grid sizing.
			*/
			tmp->hints.flags &= ~PBaseSize;
#if 0
			/* Keep silent about this, since the Xlib manual
			 * actually recommends making min <= base <= max ! */
			broken_cause = "";
#endif
		}
	}
	else
	{
		if (tmp->hints.flags & PMinSize)
		{
			tmp->hints.base_width = tmp->hints.min_width;
			tmp->hints.base_height = tmp->hints.min_height;
		}
		else
		{
			tmp->hints.base_width = 0;
			tmp->hints.base_height = 0;
		}
	}

	if (!(tmp->hints.flags & PWinGravity))
	{
		tmp->hints.win_gravity = NorthWestGravity;
	}

	if (tmp->hints.flags & PAspect)
	{
		/*
		** check to make sure min/max aspect ratios look valid
		*/
#define maxAspectX tmp->hints.max_aspect.x
#define maxAspectY tmp->hints.max_aspect.y
#define minAspectX tmp->hints.min_aspect.x
#define minAspectY tmp->hints.min_aspect.y


		/*
		** The math looks like this:
		**
		**   minAspectX    maxAspectX
		**   ---------- <= ----------
		**   minAspectY    maxAspectY
		**
		** If that is multiplied out, this must be satisfied:
		**
		**   minAspectX * maxAspectY <=  maxAspectX * minAspectY
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
			if (!*broken_cause)
			{
				broken_cause = "aspect ratio";
			}
			tmp->hints.flags &= ~PAspect;
			fvwm_msg(
				WARN, "GetWindowSizeHints", "%s window %#lx"
				" has broken aspect ratio: %d/%d - %d/%d\n",
				tmp->name, FW_W(tmp), minAspectX, minAspectY,
				maxAspectX, maxAspectY);
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
	}

	if (*broken_cause != 0)
	{
		fvwm_msg(WARN, "GetWindowSizeHints",
			 "%s window %#lx has broken (%s) size hints\n"
			 "Please send a bug report to the application owner,\n"
			 "\tyou may use fvwm-workers@fvwm.org as a reference.\n"
			 "hint override = %d\n"
			 "flags = %lx\n"
			 "min_width = %d, min_height = %d, "
			 "max_width = %d, max_height = %d\n"
			 "width_inc = %d, height_inc = %d\n"
			 "min_aspect = %d/%d, max_aspect = %d/%d\n"
			 "base_width = %d, base_height = %d\n"
			 "win_gravity = %d\n",
			 tmp->name, FW_W(tmp), broken_cause,
			 HAS_OVERRIDE_SIZE_HINTS(tmp),
			 orig_hints.flags,
			 orig_hints.min_width, orig_hints.min_height,
			 orig_hints.max_width, orig_hints.max_height,
			 orig_hints.width_inc, orig_hints.height_inc,
			 orig_hints.min_aspect.x, orig_hints.min_aspect.y,
			 orig_hints.max_aspect.x, orig_hints.max_aspect.y,
			 orig_hints.base_width, orig_hints.base_height,
			 orig_hints.win_gravity);
	}

	return;
}

/**************************************************************************
 *
 * Releases dynamically allocated space used to store window/icon names
 *
 **************************************************************************/
void free_window_names(FvwmWindow *fw, Bool nukename, Bool nukeicon)
{
	if (!fw)
	{
		return;
	}

	if (nukename && fw->visible_name &&
	    fw->visible_name != fw->name.name &&
	    fw->visible_name != NoName)
	{
		free(fw->visible_name);
		fw->visible_name = NULL;
	}
	if (nukeicon && fw->visible_icon_name &&
	    fw->visible_icon_name != fw->name.name &&
	    fw->visible_icon_name != fw->icon_name.name &&
	    fw->visible_icon_name != NoName)
	{
		free(fw->visible_icon_name);
		fw->visible_icon_name = NULL;
	}
	if (nukename && fw->name.name)
	{
#ifdef CODE_WITH_LEAK_I_THINK
		if (fw->name.name != fw->icon_name.name &&
		    fw->name.name != NoName)
		{
			FlocaleFreeNameProperty(&(fw->name));
		}
#else
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
#endif
	}
	if (nukeicon && fw->icon_name.name)
	{
#ifdef CODE_WITH_LEAK_I_THINK
		if ((fw->name.name != fw->icon_name.name || nukename) &&
		    fw->icon_name.name != NoName)
		{
			FlocaleFreeNameProperty(&(fw->icon_name));
			fw->visible_icon_name = NULL;
		}
#else
		if ((fw->name.name != fw->icon_name.name) &&
		    fw->icon_name.name != NoName)
		{
			FlocaleFreeNameProperty(&(fw->icon_name));
			fw->visible_icon_name = NULL;
		}
#endif
	}

	return;
}



/***************************************************************************
 *
 * Handles destruction of a window
 *
 ****************************************************************************/
void destroy_window(FvwmWindow *fw)
{
	extern Bool PPosOverride;

	/*
	 * Warning, this is also called by HandleUnmapNotify; if it ever needs
	 * to look at the event, HandleUnmapNotify will have to mash the
	 * UnmapNotify into a DestroyNotify. */
	if (!fw)
	{
		return;
	}

	/****** remove from window list ******/

	/* first of all, remove the window from the list of all windows! */
	fw->prev->next = fw->next;
	if (fw->next != NULL)
	{
		fw->next->prev = fw->prev;
	}

	/****** also remove it from the stack ring ******/

	/*
	 * RBW - 11/13/1998 - new: have to unhook the stacking order chain also.
	 * There's always a prev and next, since this is a ring anchored on
	 * Scr.FvwmRoot
	 */
	remove_window_from_stack_ring(fw);

	/****** check if we have to delay window destruction ******/

	if (Scr.flags.is_executing_complex_function && !DO_REUSE_DESTROYED(fw))
	{
		/* mark window for destruction */
		SET_SCHEDULED_FOR_DESTROY(fw, 1);
		Scr.flags.is_window_scheduled_for_destroy = 1;
		/* this is necessary in case the application destroys the
		 * client window and a new window is created with the same
		 * window id */
		XDeleteContext(dpy, FW_W(fw), FvwmContext);
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
		BroadcastPacket(M_DESTROY_WINDOW, 3,
				FW_W(fw), FW_W_FRAME(fw), (unsigned long)fw);
		EWMH_DestroyWindow(fw);
		return;
	}

	/****** unmap the frame ******/

	XUnmapWindow(dpy, FW_W_FRAME(fw));

	if (!PPosOverride)
	{
		XFlush(dpy);
	}

	/* already done above? */
	if (!IS_SCHEDULED_FOR_DESTROY(fw))
	{
		/****** adjust fvwm internal windows and the focus ******/

		adjust_fvwm_internal_windows(fw);

		/****** broadcast ******/

		BroadcastPacket(M_DESTROY_WINDOW, 3,
				FW_W(fw), FW_W_FRAME(fw), (unsigned long)fw);
		EWMH_DestroyWindow(fw);
	}

	/****** adjust fvwm internal windows II ******/

	/* restore_focus_after_unmap takes care of Scr.pushed_window and
	 * colormap_win */
	if (fw == Scr.Ungrabbed)
	{
		Scr.Ungrabbed = NULL;
	}

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

	if (!PPosOverride)
	{
		XFlush(dpy);
	}

	return;
}


/***********************************************************************
 *
 *  Procedure:
 *	RestoreWithdrawnLocation
 *
 *  Puts windows back where they were before fvwm took over
 *
 ************************************************************************/
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
	 * unviewable locations when the window is remapped.  The following code
	 * forces the "relative" location to be within the bounds of the
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


/***********************************************************************
 *
 *  Procedure:
 *	Reborder - Removes fvwm border windows
 *
 ************************************************************************/
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

void CaptureAllWindows(Bool is_recapture)
{
	int i,j;
	unsigned int nchildren;
	Window root, parent, *children;
	FvwmWindow *fw;		/* temp fvwm window structure */

	MyXGrabServer(dpy);

	if (!XQueryTree(dpy, Scr.Root, &root, &parent, &children, &nchildren))
	{
		MyXUngrabServer(dpy);
		return;
	}

	PPosOverride = True;
	if (!(Scr.flags.windows_captured)) /* initial capture? */
	{
		/*
		** weed out icon windows
		*/
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
		/*
		** map all of the non-override, non-icon windows
		*/
		for (i=0;i<nchildren;i++)
		{
			if (children[i] && MappedNotOverride(children[i]))
			{
				XUnmapWindow(dpy, children[i]);
				Event.xmaprequest.window = children[i];
				HandleMapRequestKeepRaised(None, NULL, False);
			}
		}
		Scr.flags.windows_captured = 1;
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
					fw, children[i], keep_on_top_win,
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
				SetFocusWindow(t, False, True);
			}
		}
	}

	isIconicState = DontCareState;

	if (nchildren > 0)
		XFree((char *)children);

	/* after the windows already on the screen are in place,
	 * don't use PPosition */
	PPosOverride = False;
	MyXUngrabServer(dpy);

	return;
}

/* ---------------------------- builtin commands ---------------------------- */

void CMD_Recapture(F_CMD_ARGS)
{
	do_recapture(F_PASS_ARGS, False);
}

void CMD_RecaptureWindow(F_CMD_ARGS)
{
	do_recapture(F_PASS_ARGS, True);
}
