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
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/Parse.h"
#include "libs/lang-strings.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "commands.h"
#include "misc.h"
#include "screen.h"
#include "update.h"
#include "style.h"
#include "geometry.h"
#include "decorations.h"

/* ---------------------------- local definitions -------------------------- */

/* Motif window hints */
#define MWM_HINTS_FUNCTIONS           (1L << 0)
#define MWM_HINTS_DECORATIONS         (1L << 1)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

/* bit definitions for MwmHints.decorations */
#if 0
#define MWM_DECOR_ALL                 (1L << 0)
#define MWM_DECOR_BORDER              (1L << 1)
#define MWM_DECOR_RESIZEH             (1L << 2)
#define MWM_DECOR_TITLE               (1L << 3)
#define MWM_DECOR_MENU                (1L << 4)
#define MWM_DECOR_MINIMIZE            (1L << 5)
#define MWM_DECOR_MAXIMIZE            (1L << 6)
#endif

#define PROP_MOTIF_WM_HINTS_ELEMENTS  4
#define PROP_MWM_HINTS_ELEMENTS       PROP_MOTIF_WM_HINTS_ELEMENTS

/* bit definitions for OL hints; I just
 *  made these up, OL stores hints as atoms */
#define OL_DECOR_CLOSE                (1L << 0)
#define OL_DECOR_RESIZEH              (1L << 1)
#define OL_DECOR_HEADER               (1L << 2)
#define OL_DECOR_ICON_NAME            (1L << 3)
#define OL_DECOR_ALL                  \
  (OL_DECOR_CLOSE | OL_DECOR_RESIZEH | OL_DECOR_HEADER | OL_DECOR_ICON_NAME)
/* indicates if there are any OL hints */
#define OL_ANY_HINTS                  (1L << 7)

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

extern Atom _XA_MwmAtom;

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* Motif  window hints */
typedef struct
{
	long flags;
	long functions;
	long decorations;
	long inputMode;
} PropMotifWmHints;

typedef PropMotifWmHints PropMwmHints;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

/*
 *
 * Reads the property MOTIF_WM_HINTS
 *
 */
void GetMwmHints(FvwmWindow *t)
{
	int actual_format;
	Atom actual_type;
	unsigned long nitems, bytesafter;

	if (t->mwm_hints)
	{
		XFree((char *)t->mwm_hints);
		t->mwm_hints = NULL;
	}
	if (XGetWindowProperty(
		    dpy, FW_W(t), _XA_MwmAtom, 0L, 32L, False,
		    _XA_MwmAtom, &actual_type, &actual_format, &nitems,
		    &bytesafter,(unsigned char **)&t->mwm_hints)==Success)
	{
		if (nitems >= PROP_MOTIF_WM_HINTS_ELEMENTS)
		{
			return;
		}
	}

	t->mwm_hints = NULL;

	return;
}

/*
 *
 * Reads the openlook properties _OL_WIN_ATTR, _OL_DECOR_ADD, _OL_DECOR_DEL
 *
 * _OL_WIN_ATTR - the win_type field is the either the first atom if the
 * property has length three or the second atom if the property has
 * length five.  It can be any of:
 *     _OL_WT_BASE (no changes)
 *     _OL_WT_CMD (no minimize decoration)
 *     _OL_WT_HELP (no minimize, maximize or resize handle decorations)
 *     _OL_WT_NOTICE (no minimize, maximize, system menu, resize handle
 *         or titlebar decorations)
 *     _OL_WT_OTHER (no minimize, maximize, system menu, resize handle
 *         or titlebar decorations)
 * In addition, if the _OL_WIN_ATTR property is in the three atom format
 * or if the type is _OL_WT_OTHER, then the icon name is not displayed
 * (same behavior as olvwm).
 *
 * _OL_DECOR_ADD or _OL_DECOR_DEL - indivdually add or remove minimize
 * button (_OL_DECOR_CLOSE), resize handles (_OL_DECOR_RESIZE), title bar
 * (_OL_DECOR_HEADER), or icon name (_OL_DECOR_ICON_NAME).
 *
 * The documentation for the Open Look hints was taken from "Advanced X
 * Window Application Programming", Eric F. Johnson and Kevin Reichard
 * (M&T Books), and the olvwm source code (available at ftp.x.org in
 * /R5contrib).
 */
void GetOlHints(FvwmWindow *t)
{
	int actual_format;
	Atom actual_type;
	unsigned long nitems, bytesafter;
	Atom *hints;
	int i;
	Atom win_type;

	t->ol_hints = OL_DECOR_ALL;

	if (XGetWindowProperty(
		    dpy, FW_W(t), _XA_OL_WIN_ATTR, 0L, 32L, False,
		    _XA_OL_WIN_ATTR, &actual_type, &actual_format,
		    &nitems, &bytesafter, (unsigned char **)&hints) == Success)
	{
		if (nitems > 0)
		{
			t->ol_hints |= OL_ANY_HINTS;

			if (nitems == 3)
			{
				win_type = hints[0];
			}
			else
			{
				win_type = hints[1];
			}

			/* got this from olvwm and sort of mapped it to
			 * fvwm/MWM hints */
			if (win_type == _XA_OL_WT_BASE)
			{
				t->ol_hints = OL_DECOR_ALL;
			}
			else if (win_type == _XA_OL_WT_CMD)
			{
				t->ol_hints = OL_DECOR_ALL & ~OL_DECOR_CLOSE;
			}
			else if (win_type == _XA_OL_WT_HELP)
			{
				t->ol_hints = OL_DECOR_ALL &
					~(OL_DECOR_CLOSE | OL_DECOR_RESIZEH);
			}
			else if (win_type == _XA_OL_WT_NOTICE)
			{
				t->ol_hints = OL_DECOR_ALL &
					~(OL_DECOR_CLOSE | OL_DECOR_RESIZEH |
					  OL_DECOR_HEADER | OL_DECOR_ICON_NAME);
			}
			else if (win_type == _XA_OL_WT_OTHER)
			{
				t->ol_hints = 0;
			}
			else
			{
				t->ol_hints = OL_DECOR_ALL;
			}

			if (nitems == 3)
			{
				t->ol_hints &= ~OL_DECOR_ICON_NAME;
			}
		}

		if (hints)
		{
			XFree (hints);
		}
	}

	if (XGetWindowProperty(
		    dpy, FW_W(t), _XA_OL_DECOR_ADD, 0L, 32L, False,
		    XA_ATOM, &actual_type, &actual_format, &nitems,
		    &bytesafter,(unsigned char **)&hints)==Success)
	{
		for (i = 0; i < nitems; i++)
		{
			t->ol_hints |= OL_ANY_HINTS;
			if (hints[i] == _XA_OL_DECOR_CLOSE)
			{
				t->ol_hints |= OL_DECOR_CLOSE;
			}
			else if (hints[i] == _XA_OL_DECOR_RESIZE)
			{
				t->ol_hints |= OL_DECOR_RESIZEH;
			}
			else if (hints[i] == _XA_OL_DECOR_HEADER)
			{
				t->ol_hints |= OL_DECOR_HEADER;
			}
			else if (hints[i] == _XA_OL_DECOR_ICON_NAME)
			{
				t->ol_hints |= OL_DECOR_ICON_NAME;
			}
		}
		if (hints)
		{
			XFree (hints);
		}
	}

	if (XGetWindowProperty(
		    dpy, FW_W(t), _XA_OL_DECOR_DEL, 0L, 32L, False,
		    XA_ATOM, &actual_type, &actual_format, &nitems,
		    &bytesafter,(unsigned char **)&hints)==Success)
	{
		for (i = 0; i < nitems; i++)
		{
			t->ol_hints |= OL_ANY_HINTS;
			if (hints[i] == _XA_OL_DECOR_CLOSE)
			{
				t->ol_hints &= ~OL_DECOR_CLOSE;
			}
			else if (hints[i] == _XA_OL_DECOR_RESIZE)
			{
				t->ol_hints &= ~OL_DECOR_RESIZEH;
			}
			else if (hints[i] == _XA_OL_DECOR_HEADER)
			{
				t->ol_hints &= ~OL_DECOR_HEADER;
			}
			else if (hints[i] == _XA_OL_DECOR_ICON_NAME)
			{
				t->ol_hints &= ~OL_DECOR_ICON_NAME;
			}
		}
		if (hints)
		{
			XFree (hints);
		}
	}

	return;
}


/*
 *
 * Interprets the property MOTIF_WM_HINTS, sets decoration and functions
 * accordingly
 *
 */
void SelectDecor(FvwmWindow *t, window_style *pstyle, short *buttons)
{
	int decor;
	int i;
	int border_width;
	int handle_width;
	int used_width;
	PropMwmHints *prop;
	style_flags *sflags = &(pstyle->flags);

	border_width = (SHAS_BORDER_WIDTH(sflags)) ?
		SGET_BORDER_WIDTH(*pstyle) : DEFAULT_BORDER_WIDTH;
	if (border_width > MAX_BORDER_WIDTH)
	{
		border_width = MAX_BORDER_WIDTH;
	}
	handle_width = (SHAS_HANDLE_WIDTH(sflags)) ?
		SGET_HANDLE_WIDTH(*pstyle) : DEFAULT_HANDLE_WIDTH;
	if (handle_width > MAX_HANDLE_WIDTH)
	{
		handle_width = MAX_HANDLE_WIDTH;
	}

	*buttons = (1 << NUMBER_OF_TITLE_BUTTONS) - 1;

	decor = MWM_DECOR_ALL;
	t->functions = MWM_FUNC_ALL;
	if (t->mwm_hints)
	{
		prop = (PropMwmHints *)t->mwm_hints;
		if (SHAS_MWM_DECOR(sflags))
		{
			if (prop->flags & MWM_HINTS_DECORATIONS)
			{
				decor = 0;
				if (prop->decorations & 0x1)
				{
					decor |= MWM_DECOR_ALL;
				}
				if (prop->decorations & 0x2)
				{
					decor |= MWM_DECOR_BORDER;
				}
				if (prop->decorations & 0x4)
				{
					decor |= MWM_DECOR_RESIZEH;
				}
				if (prop->decorations & 0x8)
				{
					decor |= MWM_DECOR_TITLE;
				}
				if (prop->decorations & 0x10)
				{
					decor |= MWM_DECOR_MENU;
				}
				if (prop->decorations & 0x20)
				{
					decor |= MWM_DECOR_MINIMIZE;
				}
				if (prop->decorations & 0x40)
				{
					decor |= MWM_DECOR_MAXIMIZE;
				}
			}
		}
		if (SHAS_MWM_FUNCTIONS(sflags))
		{
			if (prop->flags & MWM_HINTS_FUNCTIONS)
			{
				t->functions = prop->functions;
			}
		}
	}

	/* functions affect the decorations! if the user says
	 * no iconify function, then the iconify button doesn't show
	 * up. */
	if (t->functions & MWM_FUNC_ALL)
	{
		/* If we get ALL + some other things, that means to use
		 * ALL except the other things... */
		t->functions &= ~MWM_FUNC_ALL;
		t->functions =
			(MWM_FUNC_RESIZE | MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE |
			 MWM_FUNC_MAXIMIZE | MWM_FUNC_CLOSE) &
			(~(t->functions));
	}
	if (SHAS_MWM_FUNCTIONS(sflags) && IS_TRANSIENT(t))
	{
		t->functions &= ~(MWM_FUNC_MAXIMIZE|MWM_FUNC_MINIMIZE);
	}

	if (decor & MWM_DECOR_ALL)
	{
		/* If we get ALL + some other things, that means to use
		 * ALL except the other things... */
		decor &= ~MWM_DECOR_ALL;
		decor = MWM_DECOR_EVERYTHING & (~decor);
	}

	/* now add/remove any functions specified in the OL hints */
	if (SHAS_OL_DECOR(sflags) && (t->ol_hints & OL_ANY_HINTS))
	{
		if (t->ol_hints & OL_DECOR_CLOSE)
		{
			t->functions |= MWM_FUNC_MINIMIZE;
			decor      |= MWM_FUNC_MINIMIZE;
		}
		else
		{
			t->functions &= ~MWM_FUNC_MINIMIZE;
			decor      &= ~MWM_FUNC_MINIMIZE;
		}
		if (t->ol_hints & OL_DECOR_RESIZEH)
		{
			t->functions |= (MWM_FUNC_RESIZE | MWM_FUNC_MAXIMIZE);
			decor      |= (MWM_FUNC_RESIZE | MWM_FUNC_MAXIMIZE);
		}
		else
		{
			t->functions &= ~(MWM_FUNC_RESIZE | MWM_FUNC_MAXIMIZE);
			decor      &= ~(MWM_FUNC_RESIZE | MWM_FUNC_MAXIMIZE);
		}
		if (t->ol_hints & OL_DECOR_HEADER)
		{
			t->functions |= (MWM_DECOR_MENU | MWM_FUNC_MINIMIZE |
					 MWM_FUNC_MAXIMIZE | MWM_DECOR_TITLE);
			decor      |= (MWM_DECOR_MENU | MWM_FUNC_MINIMIZE |
				       MWM_FUNC_MAXIMIZE | MWM_DECOR_TITLE);
		}
		else
		{
			t->functions &= ~(MWM_DECOR_MENU | MWM_FUNC_MINIMIZE |
					  MWM_FUNC_MAXIMIZE | MWM_DECOR_TITLE);
			decor      &= ~(MWM_DECOR_MENU | MWM_FUNC_MINIMIZE |
					MWM_FUNC_MAXIMIZE | MWM_DECOR_TITLE);
		}
		if (t->ol_hints & OL_DECOR_ICON_NAME)
		{
			SET_HAS_NO_ICON_TITLE(t, 0);
		}
		else
		{
			SET_HAS_NO_ICON_TITLE(t, 1);
		}
	}

	/* Now I have the un-altered decor and functions, but with the
	 * ALL attribute cleared and interpreted. I need to modify the
	 * decorations that are affected by the functions */
	if (!(t->functions & MWM_FUNC_RESIZE))
	{
		decor &= ~MWM_DECOR_RESIZEH;
	}
	/* MWM_FUNC_MOVE has no impact on decorations. */
	if (!(t->functions & MWM_FUNC_MINIMIZE))
	{
		decor &= ~MWM_DECOR_MINIMIZE;
	}
	if (!(t->functions & MWM_FUNC_MAXIMIZE))
	{
		decor &= ~MWM_DECOR_MAXIMIZE;
	}
	/* MWM_FUNC_CLOSE has no impact on decorations. */

	/* This rule is implicit, but its easier to deal with if
	 * I take care of it now */
	if (decor & (MWM_DECOR_MENU| MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE))
	{
		decor |= MWM_DECOR_TITLE;
	}

	/* Selected the mwm-decor field, now trim down, based on
	 * .fvwmrc entries */
	if (SHAS_NO_TITLE(sflags) ||
	    (!SDO_DECORATE_TRANSIENT(sflags) && IS_TRANSIENT(t)))
	{
		decor &= ~MWM_DECOR_TITLE;
	}

	if (SHAS_NO_HANDLES(sflags) ||
	    (!SDO_DECORATE_TRANSIENT(sflags) && IS_TRANSIENT(t)))
	{
		decor &= ~MWM_DECOR_RESIZEH;
	}

	if (SHAS_MWM_DECOR(sflags) && IS_TRANSIENT(t))
	{
		decor &= ~(MWM_DECOR_MAXIMIZE|MWM_DECOR_MINIMIZE);
	}

	if (FShapesSupported)
	{
		if (t->wShaped)
		{
			decor &= ~(MWM_DECOR_BORDER|MWM_DECOR_RESIZEH);
		}
	}
	if (IS_EWMH_FULLSCREEN(t))
	{
		decor &=~(MWM_DECOR_BORDER|MWM_DECOR_RESIZEH|MWM_DECOR_TITLE);
	}

	/* Assume no decorations, and build up */
	SET_HAS_TITLE(t, 0);
	SET_HAS_HANDLES(t, 0);

	used_width = 0;
	if (decor & MWM_DECOR_BORDER)
	{
		/* A narrow border is displayed (5 pixels - 2 relief, 1 top,
		 * (2 shadow) */
		used_width = border_width;
	}
	if (decor & MWM_DECOR_TITLE)
	{
		/*      A title bar with no buttons in it
		 * window gets a 1 pixel wide black border. */
		SET_HAS_TITLE(t, 1);
	}
	if (decor & MWM_DECOR_RESIZEH)
	{
		/* A wide border, with corner tiles is desplayed
		 * (10 pixels - 2 relief, 2 shadow) */
		SET_HAS_HANDLES(t, 1);
		used_width = handle_width;
	}
	SET_HAS_NO_BORDER(t, S_HAS_NO_BORDER(SFC(*sflags)) || used_width <= 0);
	if (HAS_NO_BORDER(t))
	{
		used_width = 0;
	}
	SET_HAS_HANDLES(t, (!HAS_NO_BORDER(t) && HAS_HANDLES(t)));
	set_window_border_size(t, used_width);
	if (!(decor & MWM_DECOR_MENU))
	{
		/*      title-bar menu button omitted
		 * window gets 1 pixel wide black border */
		/* disable any buttons with the MWMDecorMenu flag */
		int i;
		for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; ++i)
		{
			if (TB_HAS_MWM_DECOR_MENU(GetDecor(t, buttons[i])))
			{
				*buttons &= ~(1 << i);
			}
		}
	}
	if (!(decor & MWM_DECOR_MINIMIZE))
	{
		/* title-bar + iconify button, no menu button.
		 * window gets 1 pixel wide black border */
		/* disable any buttons with the MWMDecorMinimize/MWMDecorShaded
		 * flag */
		int i;
		for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; ++i)
		{
			if (TB_HAS_MWM_DECOR_MINIMIZE(GetDecor(t, buttons[i])))
			{
				*buttons &= ~(1 << i);
			}
		}
	}
	if (!(decor & MWM_DECOR_MAXIMIZE))
	{
		/* title-bar + maximize button, no menu button, no iconify.
		 * window has 1 pixel wide black border */
		/* disable any buttons with the MWMDecorMaximize flag */
		int i;
		for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; ++i)
		{
			if (TB_HAS_MWM_DECOR_MAXIMIZE(GetDecor(t, buttons[i])))
			{
				*buttons &= ~(1 << i);
			}
		}
	}
	for (i = (1 << (NUMBER_OF_TITLE_BUTTONS - 1)); i; i >>= 1)
	{
		if (t->buttons & i)
			*buttons &= ~i;
	}

	t->nr_left_buttons = Scr.nr_left_buttons;
	t->nr_right_buttons = Scr.nr_right_buttons;

	for (i = 0; i / 2 < Scr.nr_left_buttons; i += 2)
	{
		if ((*buttons & (1 << i)) == 0)
			t->nr_left_buttons--;
	}
	for (i = 1; i / 2 < Scr.nr_right_buttons; i += 2)
	{
		if ((*buttons & (1 << i)) == 0)
			t->nr_right_buttons--;
	}

	return;
}

static Bool __is_resize_allowed(
	const FvwmWindow *t, int functions, request_origin_t request_origin)
{
        if (!HAS_OVERRIDE_SIZE_HINTS(t) &&
	    t->hints.min_width == t->hints.max_width &&
	    t->hints.min_height == t->hints.max_height)
	{
	        return False;
	}
	if (request_origin && IS_SIZE_FIXED(t))
	{
	        return False;
	}
	else if (!request_origin && IS_PSIZE_FIXED(t))
	{
	        return False;
	}
	if (request_origin && !(functions & MWM_FUNC_RESIZE))
	{
	        return False;
	}

	return True;
}

/*
** seemed kind of silly to have check_allowed_function and
** check_allowed_function2 partially overlapping in their checks, so I
** combined them here and made them wrapper functions instead.
*/
Bool is_function_allowed(
	int function, char *action_string, const FvwmWindow *t,
	request_origin_t request_origin, Bool do_allow_override_mwm_hints)
{
	unsigned int functions;
	char *functionlist[] = {
		MOVE_STRING,
		RESIZE_STRING1,
		RESIZE_STRING2,
		MINIMIZE_STRING,
		MINIMIZE_STRING2,
		MAXIMIZE_STRING,
		CLOSE_STRING1,
		CLOSE_STRING2,
		CLOSE_STRING3,
		CLOSE_STRING4,
		NULL
	};

	if (t == NULL)
	{
		return True;  /* this logic come from animated menu */
	}
	if (do_allow_override_mwm_hints && HAS_MWM_OVERRIDE_HINTS(t))
	{
		/* allow everything */
		functions = ~0;
	}
	else
	{
		/* restrict by mwm hints */
		functions = t->functions;
	}

	/* Hate to do it, but for lack of a better idea, check based on the
	 * menu entry name */
	/* Complex functions are a little tricky, ignore them if no menu item*/
	if (function == F_FUNCTION && action_string != NULL)
	{
		int i;

		/* remap to regular actions */

		i = GetTokenIndex(action_string, functionlist, -1, NULL);
		switch (i)
		{
		case 0:
			function = F_MOVE;
			break;
		case 1:
			function = F_RESIZE;
			break;
		case 2:
			function = F_RESIZE;
			break;
		case 3:
			function = F_ICONIFY;
			break;
		case 4:
			function = F_ICONIFY;
			break;
		case 5:
			function = F_MAXIMIZE;
			break;
		case 6:
			function = F_CLOSE;
			break;
		case 7:
			function = F_DELETE;
			break;
		case 8:
			function = F_DESTROY;
			break;
		case 9:
			function = F_QUIT;
			break;
		default:
			break;
		}
	}
	/* now do the real checks */
	switch(function)
	{
	case F_DELETE:
	        if (IS_UNCLOSABLE(t))
		{
		        return False;
		}
		if (IS_TEAR_OFF_MENU(t))
		{
			/* always allow this on tear off menus */
			break;
		}
		if (!WM_DELETES_WINDOW(t))
		{
			return False;
		}
		/* fall through to close clause */
	case F_CLOSE:
	        if (IS_UNCLOSABLE(t))
		{
		        return False;
		}
		if (IS_TEAR_OFF_MENU(t))
		{
			/* always allow this on tear off menus */
			break;
		}
		if (!(functions & MWM_FUNC_CLOSE))
		{
			return False;
		}
		break;
	case F_DESTROY: /* shouldn't destroy always be allowed??? */
	        if (IS_UNCLOSABLE(t))
		{
		        return False;
		}
		if (IS_TEAR_OFF_MENU(t))
		{
			/* always allow this on tear off menus */
			break;
		}
		if (!(functions & MWM_FUNC_CLOSE))
		{
			return False;
		}
		break;
	case F_RESIZE:
	        if(!__is_resize_allowed(t, functions, request_origin))
		{
		        return False;
		}
		break;
	case F_ICONIFY:
		if ((!IS_ICONIFIED(t) && !(functions & MWM_FUNC_MINIMIZE)) ||
		    IS_UNICONIFIABLE(t))
		{
			return False;
		}
		break;
	case F_MAXIMIZE:
	        if (IS_MAXIMIZE_FIXED_SIZE_DISALLOWED(t) &&
		    !__is_resize_allowed(t, functions, request_origin))
		{
		       return False;
		}
		if ((request_origin && !(functions & MWM_FUNC_MAXIMIZE)) ||
		    IS_UNMAXIMIZABLE(t))
		{
			return False;
		}
		break;
	case F_MOVE:
		/* Move is a funny hint. Keeps it out of the menu, but you're
		 * still allowed to move. */
		if (request_origin && IS_FIXED(t))
		{
			return False;
		}
		else if (!request_origin && IS_FIXED_PPOS(t))
		{
			return False;
		}
		if (request_origin && !(functions & MWM_FUNC_MOVE))
		{
			return False;
		}
		break;
	case F_FUNCTION:
	default:
		break;
	} /* end of switch */

	/* if we fell through, just return True */
	return True;
}
