/* -*-c-*- */
/*
 * GNOME WM Compliance adapted for FVWM
 * Properties set on the root window (or desktop window)
 *
 * Even though the rest of fvwm is GPL consider this file
 * Public Domain - use it however you see fit to make
 * your WM GNOME compiant
 *
 * written by Raster
 * adapted for FVWM by Jay Painter <jpaint@gnu.org>
 */
#include "config.h"

#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "commands.h"
#include "misc.h"
#include "screen.h"
#include "gnome.h"
#include "move_resize.h"
#include "stack.h"
#include "update.h"
#include "style.h"
#include "virtual.h"
#include "window_flags.h"
#include "borders.h"
#include "decorations.h"


/*
 * Properties set on the root window (or desktop window)
 */

/* WIN_AREA CARD32[2] contains the current desktop area X,Y */
#define XA_WIN_AREA                        "_WIN_AREA"

/* WIN_AREA CARD32[2] contains the current desktop area size WxH */
#define XA_WIN_AREA_COUNT                        "_WIN_AREA_COUNT"

/* array of atoms - atom being one of the following atoms */
#define XA_WIN_PROTOCOLS                   "_WIN_PROTOCOLS"

/* array of iocn in various sizes */
/* Type: array of CARD32 */
/*       first item is icon count (n) */
/*       second item is icon record length (in CARD32s) */
/*       this is followed by (n) icon records as follows */
/*           pixmap (XID) */
/*           mask (XID) */
/*           width (CARD32) */
/*           height (CARD32) */
/*           depth (of pixmap, mask is assumed to be of depth 1) (CARD32) */
/*           drawable (screen root drawable of pixmap) (XID) */
/*           ... additional fields can be added at the end of this list */
#define XA_WIN_ICONS                    "_WIN_ICONS"

/* WIN_WORKSPACE CARD32 contains the current desktop number */
#define XA_WIN_WORKSPACE                   "_WIN_WORKSPACE"
/* WIN_WORKSPACE_COUNT CARD32 contains the number of desktops */
#define XA_WIN_WORKSPACE_COUNT             "_WIN_WORKSPACE_COUNT"

/* WIN_WORKSPACE_NAMES StringList (Text Property) of workspace names */
/* unused by enlightenment */
#define XA_WIN_WORKSPACE_NAMES             "_WIN_WORKSPACE_NAMES"

/* *** Don't use this.. iffy at best. ** */
/* The available work area for client windows. The WM can set this and the WM */
/* and/or clients may change it at any time. If it is changed the WM and/or  */
/* clients should honor the changes. If this property does not exist a client */
/* or WM can create it. */
/*
 * CARD32              min_x;
 * CARD32              min_y;
 * CARD32              max_x;
 * CARD32              max_y;
 */
#define XA_WIN_WORKAREA                    "_WIN_WORKAREA"
/* array of 4 CARD32's */

/* This is a list of window id's the WM is currently managing - primarily */
/* for being able to have external "tasklist" apps */
#define XA_WIN_CLIENT_LIST                 "_WIN_CLIENT_LIST"
/* array of N XID's */

/*
 * Properties on client windows
 */

/* The layer the window exists in */
/*      0 = Desktop */
/*      1 = Below */
/*      2 = Normal (default app layer) */
/*      4 = OnTop */
/*      6 = Dock (always on top - for panel) */
/* The app sets this alone, not the WM. If this property changes the WM */
/* should comply and change the appearance/behavior of the Client window */
/* if this hint does not exist the WM Will create it on the Client window */
#define WIN_LAYER_DESKTOP                0
#define WIN_LAYER_BELOW                  2
#define WIN_LAYER_NORMAL                 4
#define WIN_LAYER_ONTOP                  6
#define WIN_LAYER_DOCK                   8
#define WIN_LAYER_ABOVE_DOCK             10
#define WIN_LAYER_MENU                   12
#define XA_WIN_LAYER                     "_WIN_LAYER"
/* WIN_LAYER = CARD32 */

/* flags for the window's state. The WM will change these as needed when */
/* state changes. If the property contains info on client map, E will modify */
/* the windows state accordingly. if the Hint does not exist the WM will */
/* create it on the client window. 0 for the bit means off, 1 means on. */
/* unused (default) values are 0 */

/* removed Minimized - no explanation of what it really means - ambiguity */
/* should not be here if not clear */
#define WIN_STATE_STICKY          (1<<0) /* everyone knows sticky */
#define WIN_STATE_RESERVED_BIT1   (1<<1) /* removed minimize here */
#define WIN_STATE_MAXIMIZED_VERT  (1<<2) /* window in maximized V state */
#define WIN_STATE_MAXIMIZED_HORIZ (1<<3) /* window in maximized H state */
#define WIN_STATE_HIDDEN          (1<<4) /* not on taskbar but window visible */
#define WIN_STATE_SHADED          (1<<5) /* shaded (NeXT style) */
#define WIN_STATE_HID_WORKSPACE   (1<<6) /* not on current desktop */
#define WIN_STATE_HID_TRANSIENT   (1<<7) /* Owner of transient is hidden */
#define WIN_STATE_FIXED_POSITION  (1<<8) /* window is fixed in position even */
#define WIN_STATE_ARRANGE_IGNORE  (1<<9) /* ignore for auto arranging */
/* when scrolling about large */
/* virtual desktops ala fvwm */
#define XA_WIN_STATE              "_WIN_STATE"
/* WIN_STATE = CARD32 */

/* Preferences for behavior for app */
/* ONLY the client sets this */
#define WIN_HINTS_SKIP_FOCUS      (1<<0) /* "alt-tab" skips this win */
#define WIN_HINTS_SKIP_WINLIST    (1<<1) /* not in win list */
#define WIN_HINTS_SKIP_TASKBAR    (1<<2) /* not on taskbar */
#define WIN_HINTS_GROUP_TRANSIENT (1<<3) /* ??????? */
#define WIN_HINTS_FOCUS_ON_CLICK  (1<<4) /* app only accepts focus when clicked
					  */
#define XA_WIN_HINTS                     "_WIN_HINTS"
/* WIN_HINTS = CARD32 */

/* Application state - also "color reactiveness" - the app can keep changing */
/* this property when it changes its state and the WM or monitoring program */
/* will pick this up and dpylay somehting accordingly. ONLY the client sets */
/* this. */
#define WIN_APP_STATE_NONE                 0
#define WIN_APP_STATE_ACTIVE1              1
#define WIN_APP_STATE_ACTIVE2              2
#define WIN_APP_STATE_ERROR1               3
#define WIN_APP_STATE_ERROR2               4
#define WIN_APP_STATE_FATAL_ERROR1         5
#define WIN_APP_STATE_FATAL_ERROR2         6
#define WIN_APP_STATE_IDLE1                7
#define WIN_APP_STATE_IDLE2                8
#define WIN_APP_STATE_WAITING1             9
#define WIN_APP_STATE_WAITING2             10
#define WIN_APP_STATE_WORKING1             11
#define WIN_APP_STATE_WORKING2             12
#define WIN_APP_STATE_NEED_USER_INPUT1     13
#define WIN_APP_STATE_NEED_USER_INPUT2     14
#define WIN_APP_STATE_STRUGGLING1          15
#define WIN_APP_STATE_STRUGGLING2          16
#define WIN_APP_STATE_DISK_TRAFFIC1        17
#define WIN_APP_STATE_DISK_TRAFFIC2        18
#define WIN_APP_STATE_NETWORK_TRAFFIC1     19
#define WIN_APP_STATE_NETWORK_TRAFFIC2     20
#define WIN_APP_STATE_OVERLOADED1          21
#define WIN_APP_STATE_OVERLOADED2          22
#define WIN_APP_STATE_PERCENT000_1         23
#define WIN_APP_STATE_PERCENT000_2         24
#define WIN_APP_STATE_PERCENT010_1         25
#define WIN_APP_STATE_PERCENT010_2         26
#define WIN_APP_STATE_PERCENT020_1         27
#define WIN_APP_STATE_PERCENT020_2         28
#define WIN_APP_STATE_PERCENT030_1         29
#define WIN_APP_STATE_PERCENT030_2         30
#define WIN_APP_STATE_PERCENT040_1         31
#define WIN_APP_STATE_PERCENT040_2         32
#define WIN_APP_STATE_PERCENT050_1         33
#define WIN_APP_STATE_PERCENT050_2         34
#define WIN_APP_STATE_PERCENT060_1         35
#define WIN_APP_STATE_PERCENT060_2         36
#define WIN_APP_STATE_PERCENT070_1         37
#define WIN_APP_STATE_PERCENT070_2         38
#define WIN_APP_STATE_PERCENT080_1         39
#define WIN_APP_STATE_PERCENT080_2         40
#define WIN_APP_STATE_PERCENT090_1         41
#define WIN_APP_STATE_PERCENT090_2         42
#define WIN_APP_STATE_PERCENT100_1         43
#define WIN_APP_STATE_PERCENT100_2         44
#define XA_WIN_APP_STATE                   "_WIN_APP_STATE"
/* WIN_APP_STATE = CARD32 */

/* Expanded space occupied - this is the area on screen the app's window */
/* will occupy when "expanded" - ie if you have a button on an app that */
/* "hides" it by reducing its size, this is the geometry of the expanded */
/* window - so the window manager can allow for this when doign auto */
/* positioing of client windows assuming the app can at any point use this */
/* this area and thus try and keep it clear. ONLY the client sets this */
/*
 * CARD32              x;
 * CARD32              y;
 * CARD32              width;
 * CARD32              height;
 */
#define XA_WIN_EXPANDED_SIZE               "_WIN_EXPANDED_SIZE"
/* array of 4 CARD32's */

/* CARD32 that contians the desktop number the application is on If the */
/* application's state is "sticky" it is irrelevant. Only the WM should */
/* change this. */
#define XA_WIN_WORKSPACE                   "_WIN_WORKSPACE"

/* This atom is a 32-bit integer that is either 0 or 1 (currently). */
/* 0 denotes everything is as per usual but 1 denotes that ALL configure */
/* requests by the client on the client window with this property are */
/* not just a simple "moving" of the window, but the result of a user */
/* moving the window BUT the client handling that interaction by moving */
/* its own window. The window manager should respond accordingly by assuming */
/* any configure requests for this window whilst this atom is "active" in */
/* the "1" state are a client move and should handle flipping desktops if */
/* the window is being dragged "off screem" or across desktop boundaries */
/* etc. This atom is ONLY ever set by the client */
#define XA_WIN_CLIENT_MOVING               "_WIN_CLIENT_MOVING"
/* WIN_CLIENT_MOVING = CARD32 */

/* Designed for checking if the WIN_ supporting WM is still there  */
/* and kicking about - basically check this property - check the window */
/* ID it points to - then check that window Id has this property too */
/* if that is the case the WIN_ supporting WM is there and alive and the */
/* list of WIN_PROTOCOLS is valid */
#define XA_WIN_SUPPORTING_WM_CHECK         "_WIN_SUPPORTING_WM_CHECK"
/* CARD32 */

/* for root window button clicks */
#define XA_WIN_DESKTOP_BUTTON_PROXY        "_WIN_DESKTOP_BUTTON_PROXY"
/* CARD32 */


/* for the root window clicks */
static Window __button_proxy = 0;


/* how many desks does gnome know about */
static unsigned int gnome_max_desk = 1;

static int atom_size(int format)
{
	if (format == 32)
	{
		return sizeof(long);
	}
	else
	{
		return (format >> 3);
	}
}

static void *
AtomGet(Window win, Atom to_get, Atom type, int *size)
{
	unsigned char *retval;
	Atom type_ret;
	unsigned long bytes_after, num_ret;
	int format_ret;
	long length;
	void *data;

	retval = NULL;
	length = 0x7fffffff;
	XGetWindowProperty(
		dpy, win, to_get, 0, length, False, type, &type_ret,
		&format_ret, &num_ret, &bytes_after, &retval);

	if ((retval) && (num_ret > 0) && (format_ret > 0))
	{
		int asize;

		asize = atom_size(format_ret);
		data = safemalloc(num_ret * asize);
		if (data)
		{
			memcpy(data, retval, num_ret * asize);
		}
		XFree(retval);
		*size = num_ret * (format_ret >> 3);
		return data;
	}

	return NULL;
}


/*** GET WINDOW PROPERTIES ***/

#if 0
static void
GNOME_GetHintIcons(FvwmWindow *fwin)
{
	Atom atom_get;
	long *retval;
	int size;
	unsigned int i;
	Pixmap pmap;
	Pixmap mask;

	atom_get = XInternAtom(dpy, XA_WIN_ICONS, False);
	retval = AtomGet(FW_W(fwin), atom_get, XA_PIXMAP, &size);
	if (retval)
	{
		int n;

		n = size / atom_size(32);
		for (i = 0; i < n; i += 2)
		{
			pmap = retval[i];
			mask = retval[i + 1];
		}
		free(retval);
	}

	return;
}

static void
GNOME_GetHintLayer(FvwmWindow *fwin)
{
	Atom atom_get;
	long *retval;
	int size;

	atom_get = XInternAtom(dpy, XA_WIN_LAYER, False);
	retval = AtomGet(FW_W(fwin), atom_get, XA_CARDINAL, &size);
	if (retval)
	{
		set_layer(fwin, *retval);
		free(retval);
	}

	return;
}

static void
GNOME_GetHintState(FvwmWindow *fwin)
{
	Atom atom_get;
	long *retval;
	int size;

	atom_get = XInternAtom(dpy, XA_WIN_STATE, False);
	retval = AtomGet(FW_W(fwin), atom_get, XA_CARDINAL, &size);
	if (retval)
	{
		if (*retval & WIN_STATE_STICKY)
		{
			SET_STICKY(fwin, 1);
		}

		if (*retval & WIN_STATE_SHADED)
		{
			/* Fixme: how do we find out the correct shade
			 * direction here? */
			SET_SHADED(fwin, 1);
			SET_SHADED_DIR(fwin, DIR_N);
		}

		if (*retval & WIN_STATE_FIXED_POSITION)
		{
			SET_FIXED(fwin, 1);
		}

		free(retval);
	}

	return;
}

static void
GNOME_GetHintAppState(FvwmWindow *fwin)
{
	Atom atom_get;
	unsigned char *retval;
	int size;

	/* have nothing interesting to do with an app state (lamp) right now */
	atom_get = XInternAtom(dpy, XA_WIN_APP_STATE, False);
	retval = AtomGet(FW_W(fwin), atom_get, XA_CARDINAL, &size);
	if (retval)
	{
		free(retval);
	}

	return;
}

static void
GNOME_GetHintDesktop(FvwmWindow *fwin)
{
	Atom atom_get;
	unsigned char *retval;
	int size;
	int *desk;

	atom_get = XInternAtom(dpy, XA_WIN_WORKSPACE, False);
	retval = AtomGet(FW_W(fwin), atom_get, XA_CARDINAL, &size);
	if (retval)
	{
		desk = (int *)retval;
		fwin->Desk = *desk;

		/* XXX: hide window if it's not on the current desktop! */
		free(retval);
	}

	return;
}

static void
GNOME_GetHint(FvwmWindow *fwin)
{
	Atom atom_get;
	int *retval;
	int size;

	atom_get = XInternAtom(dpy, XA_WIN_HINTS, False);
	retval = AtomGet(FW_W(fwin), atom_get, XA_CARDINAL, &size);
	if (retval)
	{
		if (*retval & WIN_HINTS_SKIP_WINLIST)
		{
			SET_DO_SKIP_WINDOW_LIST(fwin, 1);
		}
		/* XXX: unimplimented */
		if (*retval & WIN_HINTS_SKIP_TASKBAR)
		{
			;
		}
		if (*retval & WIN_HINTS_SKIP_FOCUS)
		{
			;
		}
		if (*retval & WIN_HINTS_FOCUS_ON_CLICK)
		{
			;
		}
		/* if (*retval & WIN_HINTS_DO_NOT_COVER);*/

		free(retval);
	}

	return;
}

static void
GNOME_GetExpandedSize(FvwmWindow *fwin)
{
	Atom atom_get;
	long *retval;
	int size;

	/* unimplimented */
	int expanded_x, expanded_y, expanded_width, expanded_height;

	atom_get = XInternAtom(dpy, XA_WIN_EXPANDED_SIZE, False);
	retval = AtomGet(FW_W(fwin), atom_get, XA_CARDINAL, &size);
	if (retval)
	{
		expanded_x = retval[0];
		expanded_y = retval[1];
		expanded_width = retval[2];
		expanded_height = retval[3];
		free(retval);
	}

	return;
}

void
GNOME_GetHints(FvwmWindow *fwin)
{
	if (DO_IGNORE_GNOME_HINTS(fwin))
	{
		return;
	}
	GNOME_GetHintDesktop(fwin);
	GNOME_GetHintIcons(fwin);
	GNOME_GetHintLayer(fwin);
	GNOME_GetHintState(fwin);
	GNOME_GetHintAppState(fwin);
	GNOME_GetHint(fwin);
	GNOME_GetExpandedSize(fwin);

	return;
}
#endif

void
GNOME_SetHints(FvwmWindow *fwin)
{
	Atom atom_set;
	long val;

	atom_set = XInternAtom(dpy, XA_WIN_STATE, False);
	val = 0;

	if (IS_STICKY_ACROSS_PAGES(fwin) && IS_STICKY_ACROSS_DESKS(fwin))
	{
		val |= WIN_STATE_STICKY;
	}
	if (IS_SHADED(fwin))
	{
		val |= WIN_STATE_SHADED;
	}
	if (!is_function_allowed(F_MOVE, NULL, fwin, True, False))
	{
		val |= WIN_STATE_FIXED_POSITION;
	}
	XChangeProperty(dpy, FW_W(fwin), atom_set, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)&val, 1);

	return;
}


/* this duplicates most of the above... and it assumes
   that style is inizialized to zero.
*/
void
GNOME_GetStyle (FvwmWindow *fwin, window_style *style)
{
	Atom atom_get;
	unsigned char *retval;
	int size;

	if (S_DO_IGNORE_GNOME_HINTS(SCF(*style)))
	{
		return;
	}
	/* Desktop */
	atom_get = XInternAtom(dpy, XA_WIN_WORKSPACE, False);
	retval = AtomGet(FW_W(fwin), atom_get, XA_CARDINAL, &size);
	if (retval)
	{
		SSET_START_DESK(*style, *(int*)retval);
		/* Allow special case of -1 to work. */
		if (SGET_START_DESK(*style) > -1)
		{
			SSET_START_DESK(*style, SGET_START_DESK(*style) + 1);
		}
		style->flags.use_start_on_desk = 1;
		style->flag_mask.use_start_on_desk = 1;
		free(retval);
	}

	/* Icons - not implemented */

	/* Layer */
	atom_get = XInternAtom(dpy, XA_WIN_LAYER, False);
	retval = AtomGet(FW_W(fwin), atom_get, XA_CARDINAL, &size);
	if (retval)
	{
		SSET_LAYER(*style, *(int*)retval);
		style->flags.use_layer = (SGET_LAYER(*style) >= 0) ? 1 : 0;
		style->flag_mask.use_layer = 1;
		free(retval);
	}

	/* State */
	atom_get = XInternAtom(dpy, XA_WIN_STATE, False);
	retval = AtomGet(FW_W(fwin), atom_get, XA_CARDINAL, &size);
	if (retval)
	{
		if (*(int*)retval & WIN_STATE_STICKY)
		{
			S_SET_IS_STICKY_ACROSS_PAGES(SCF(*style), 1);
			S_SET_IS_STICKY_ACROSS_PAGES(SCM(*style), 1);
			S_SET_IS_STICKY_ACROSS_PAGES(SCC(*style), 1);
			S_SET_IS_STICKY_ACROSS_DESKS(SCF(*style), 1);
			S_SET_IS_STICKY_ACROSS_DESKS(SCM(*style), 1);
			S_SET_IS_STICKY_ACROSS_DESKS(SCC(*style), 1);
		}

		if (*(int*)retval & WIN_STATE_SHADED)
		{
			/* unimplemented, since we don't have a
			   start_shaded flag. SM code relies on a
			   separate do_shade flag, but that is ugly.
			*/
		}

		if ((*(int*)retval & WIN_STATE_FIXED_POSITION))
		{
			S_SET_IS_FIXED(SCF(*style), 1);
			S_SET_IS_FIXED(SCM(*style), 1);
			S_SET_IS_FIXED(SCC(*style), 1);
		}

		free(retval);
	}

	/* App state - not implemented */

	/* Hints */
	atom_get = XInternAtom(dpy, XA_WIN_HINTS, False);
	retval = AtomGet(FW_W(fwin), atom_get, XA_CARDINAL, &size);
	if (retval)
	{
		if (*retval & WIN_HINTS_SKIP_WINLIST)
		{
			S_SET_DO_WINDOW_LIST_SKIP(SCF(*style), 1);
			S_SET_DO_WINDOW_LIST_SKIP(SCM(*style), 1);
			S_SET_DO_WINDOW_LIST_SKIP(SCC(*style), 1);
		}

		free(retval);
	}

	/* Expanded size - not implemented */

	return;
}


/*** SET WINDOW PROPERTIES ***/
void
GNOME_SetDesk(FvwmWindow *fwin)
{
	Atom atom_set;
	long val;

	atom_set = XInternAtom(dpy, XA_WIN_WORKSPACE, False);
	val = fwin->Desk;
	if (val >= gnome_max_desk)
	{
		GNOME_SetDeskCount();
	}
	XChangeProperty(
		dpy, FW_W(fwin), atom_set, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)&val, 1);

	return;
}


void
GNOME_SetLayer(FvwmWindow *fwin)
{
	Atom atom_set;
	long val;

	atom_set = XInternAtom(dpy, XA_WIN_LAYER, False);
	val = get_layer(fwin);
	XChangeProperty(
		dpy, FW_W(fwin), atom_set, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)&val, 1);

	return;
}


/*** INITIALIZE GNOME WM SUPPORT ***/

/* sets the virtual desktop grid properties */
void
GNOME_SetAreaCount(void)
{
	Atom atom_set;
	long val[2];

	atom_set = XInternAtom(dpy, XA_WIN_AREA_COUNT, False);
	val[0] = (Scr.VxMax + Scr.MyDisplayWidth) / Scr.MyDisplayWidth;
	val[1] = (Scr.VyMax + Scr.MyDisplayHeight) / Scr.MyDisplayHeight;
	XChangeProperty(
		dpy, Scr.Root, atom_set, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)val, 2);

	return;
}


/* sets the property indicating the current virtual desktop
 * location
 */
void
GNOME_SetCurrentArea(void)
{
	Atom atom_set;
	long val[2];

	atom_set = XInternAtom(dpy, XA_WIN_AREA, False);
	val[0] = Scr.Vx / Scr.MyDisplayWidth;
	val[1] = Scr.Vy / Scr.MyDisplayHeight;
	XChangeProperty(
		dpy, Scr.Root, atom_set, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)val, 2);

	return;
}

/* FIXME: what to do about negative desks ? */
/* ignore them! */
void
GNOME_SetDeskCount(void)
{
	Atom atom_set;
	long val;
	FvwmWindow *t;

	atom_set = XInternAtom(dpy, XA_WIN_WORKSPACE_COUNT, False);
	val = 0;
	if (Scr.CurrentDesk > 0)
	{
		val = Scr.CurrentDesk;
	}
	for (t = get_next_window_in_stack_ring(&Scr.FvwmRoot);
	     t != &Scr.FvwmRoot; t = get_next_window_in_stack_ring(t))
	{
		if (t->Desk > val)
		{
			val = t->Desk;
		}
	}
	val++;
	if (gnome_max_desk > val)
	{
		val = gnome_max_desk;
	}
	XChangeProperty(
		dpy, Scr.Root, atom_set, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)&val, 1);

	return;
}


/* XXX: this function is hard-coded to one! -JMP */
void
GNOME_SetCurrentDesk(void)
{
	Atom atom_set;
	long val;

	atom_set = XInternAtom(dpy, XA_WIN_WORKSPACE, False);
	val = (long) Scr.CurrentDesk;
	if (val >= gnome_max_desk)
	{
		GNOME_SetDeskCount();
	}
	XChangeProperty(
		dpy, Scr.Root, atom_set, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)&val, 1);
	GNOME_SetCurrentArea();

	return;
}


/* sets the names assigned to the desktops */
void
GNOME_SetDeskNames(void)
{
	Atom atom_set;
	XTextProperty text;
	char *names[1];

	atom_set = XInternAtom(dpy, XA_WIN_WORKSPACE_NAMES, False);
	names[0] = "GNOME Desktop";
	if (XStringListToTextProperty(names, 1, &text))
	{
		XSetTextProperty(dpy, Scr.Root, &text, atom_set);
		XFree(text.value);
	}

	return;
}


void
GNOME_SetClientList(void)
{
	Atom atom_set;
	Window *wl=NULL;
	int displayed=0;
	int i = 0;
	FvwmWindow *t;

	/* Find out how many window pointers we need to store, allocate
	 * the space and go through the list again to actually store them.
	 */
	for (t = Scr.FvwmRoot.next; t; t = t->next)
	{
		if (!DO_SKIP_WINDOW_LIST(t))
		{
			displayed++;
		}
	}
	if(displayed)
	{
		wl=(Window*)safemalloc(sizeof(Window*)*displayed);
		for (t = Scr.FvwmRoot.next; t; t = t->next)
		{
			if (!DO_SKIP_WINDOW_LIST(t))
			{
				wl[i++] = FW_W(t);
			}
		}
	}
	/* Update X */
	atom_set = XInternAtom(dpy, XA_WIN_CLIENT_LIST, False);
	XChangeProperty(
		dpy, Scr.Root, atom_set, XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)wl, i);
	if(wl)
		free(wl);

	return;
}

void
GNOME_SetWinArea(FvwmWindow *w)
{
	Atom atom_set;
	long val[2];

	atom_set = XInternAtom(dpy, XA_WIN_AREA, False);

	if (!DO_SKIP_WINDOW_LIST(w))
	{
		if (IsRectangleOnThisPage(&(w->frame_g), w->Desk))
		{
			val[0] = Scr.Vx / Scr.MyDisplayWidth;
			val[1] = Scr.Vy / Scr.MyDisplayHeight;
		}
		else
		{
			val[0] = (w->frame_g.x + Scr.Vx) / Scr.MyDisplayWidth;
			if (val[0] < 0 &&
			    w->frame_g.x + Scr.Vx + w->frame_g.width > 0)
			{
				val[0] = 0;
			}
			val[1] = (w->frame_g.y + Scr.Vy) / Scr.MyDisplayHeight;
			if (val[1] < 0 &&
			    w->frame_g.y + Scr.Vy + w->frame_g.height > 0)
			{
				val[1] = 0;
			}
		}
		XChangeProperty(
			dpy, FW_W(w), atom_set, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)val, 2);
	}

	return;
}

void
GNOME_Init(void)
{
	int i;
	Atom atom_set, list[11];
	long val;

	atom_set = XInternAtom(dpy, XA_WIN_PROTOCOLS, False);
	/* these indicate what GNOME compliance properties have been
	 * implemented */
	i = 0;
	list[i++] = XInternAtom(dpy, XA_WIN_LAYER, False);
	list[i++] = XInternAtom(dpy, XA_WIN_STATE, False);
	list[i++] = XInternAtom(dpy, XA_WIN_HINTS, False);
	/* list[i++] = XInternAtom(dpy, XA_WIN_APP_STATE, False); */
	/* list[i++] = XInternAtom(dpy, XA_WIN_EXPANDED_SIZE, False); */
	/* list[i++] = XInternAtom(dpy, XA_WIN_ICONS, False); */
	list[i++] = XInternAtom(dpy, XA_WIN_WORKSPACE, False);
	list[i++] = XInternAtom(dpy, XA_WIN_WORKSPACE_COUNT, False);
	list[i++] = XInternAtom(dpy, XA_WIN_WORKSPACE_NAMES, False);
	list[i++] = XInternAtom(dpy, XA_WIN_CLIENT_LIST, False);
	list[i++] = XInternAtom(dpy, XA_WIN_DESKTOP_BUTTON_PROXY, False);
	XChangeProperty(
		dpy, Scr.Root, atom_set, XA_ATOM, 32, PropModeReplace,
		(unsigned char *)list, i);

	/*--------------------------------------------------------------------
	  why use two windows when one does just fine ?
	  --------------------------------------------------------------------*/
	/* create a window which is a child of the root window and set the
	 * XA_WIN_SUPPORTING_WM_CHECK property on it, and also set the
	 * same property on the root window with the window ID of of the
	 * window we just created
	 */
	__button_proxy = XCreateWindow(dpy, Scr.Root, -10, -10, 10, 10, 0, 0,
				       InputOnly, CopyFromParent, 0, NULL);
	atom_set = XInternAtom(dpy, XA_WIN_SUPPORTING_WM_CHECK, False);
	val = __button_proxy;
	XChangeProperty(
		dpy, Scr.Root, atom_set, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)&val, 1);
	XChangeProperty(
		dpy, __button_proxy, atom_set, XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)&val, 1);

	/* create a window which is the child of the root window for proxying
	 * root window clicks
	 */
	atom_set = XInternAtom(dpy, XA_WIN_DESKTOP_BUTTON_PROXY, False);
	XChangeProperty(
		dpy, Scr.Root, atom_set, XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)&val, 1);
	XChangeProperty(
		dpy, __button_proxy, atom_set, XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)&val, 1);

	/* some initial settings */
	GNOME_SetDeskCount();
	GNOME_SetCurrentDesk();
	GNOME_SetDeskNames();
	GNOME_SetAreaCount();
	GNOME_SetCurrentArea();
	GNOME_SetClientList();

	return;
}


/*-----------------------------------------------------------------------
  tell gnome how many desks to show
  -----------------------------------------------------------------------*/
void
CMD_GnomeShowDesks(F_CMD_ARGS)
{
	int n;
	Atom atom_set;
	long val[1];

	atom_set = XInternAtom(dpy, XA_WIN_WORKSPACE_COUNT, False);
	DBUG("GNOME_ShowDesks","Routine Entered");
	n = GetIntegerArguments(action, NULL, (int *)val, 1);
	if(n != 1)
	{
		fvwm_msg(ERR, "GnomeShowDesks", "requires one argument");
		return;
	}
	gnome_max_desk = val[0];
	XChangeProperty(
		dpy, Scr.Root, atom_set, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)(&val[0]), 1);

	return;
}


/*** RECIVING MESSAGES ***/
int
GNOME_ProcessClientMessage(const exec_context_t *exc)
{
	Atom a;
	FvwmWindow *fw = exc->w.fw;
	XEvent *ev = exc->x.etrigger;
	long p0 = 0;
	long p1 = 0;
	const char* invalid_req = "---";
	const char* invalid_prop = "---";

	if (fw != NULL  && DO_IGNORE_GNOME_HINTS(fw))
	{
		return 0;
	}
	a = XInternAtom(dpy, XA_WIN_AREA, False);
	if (ev->xclient.message_type == a)
	{
		/* convert to integer grid */
		p0 = ev->xclient.data.l[0] * Scr.MyDisplayWidth;
		p1 = ev->xclient.data.l[1] * Scr.MyDisplayHeight;
		if (p0 < 0 || p0 > 0x7fffffff || p1 < 0 || p1 > 0x7fffffff)
		{
			invalid_prop = "page";
			invalid_req = "_WIN_AREA";
			goto err_msg;
		}
		MoveViewport(p0, p1, 1);

		return 1;
	}
	a = XInternAtom(dpy, XA_WIN_WORKSPACE, False);
	if (ev->xclient.message_type == a)
	{
		p0 = ev->xclient.data.l[0];
		if (p0 < 0 || p0 > 0x7fffffff)
		{
			invalid_prop = "desk";
			invalid_req = "_WIN_WORKSPACE";
			goto err_msg;
		}
		goto_desk(ev->xclient.data.l[0]);

		return 1;
	}
	a = XInternAtom(dpy, XA_WIN_LAYER, False);
	if (ev->xclient.message_type == a && fw)
	{
		p0 = ev->xclient.data.l[0];
		if (p0 < 0 || p0 > 0x7fffffff)
		{
			invalid_prop = "layer";
			invalid_req = "_WIN_LAYER";
			goto err_msg;
		}
		new_layer(fw, ev->xclient.data.l[0]);

		return 1;
	}
	a = XInternAtom(dpy, XA_WIN_STATE, False);
	if (ev->xclient.message_type == a && fw)
	{
		GNOME_HandlePropRequest(
			exc, ev->xclient.data.l[0], ev->xclient.data.l[1]);

		return 1;
	}

	return 0;

  err_msg:
	fvwm_msg(
		WARN, "GNOME_ProcessClientMessage",
		"The application window (id %#lx)\n"
		"  \"%s\" has requested an invalid %s (%ld, %ld)\n"
		"  using a %s client message.\n"
		"    fvwm is ignoring this request.\n",
		fw ? FW_W(fw) : 0, fw ? fw->name.name : "(none)", invalid_prop,
		p0, p1, invalid_req);
	fvwm_msg_report_app_and_workers();

	return 0;

}

void GNOME_HandlePropRequest(
	const exec_context_t *exc, unsigned int propm, unsigned int prop)
{
	Atom atype;
	Atom a;
	int aformat;
	FvwmWindow *fwin;
	unsigned char *indi;
	unsigned long nitems, bytes_remain, oldprop;
	indi = (unsigned char *)&oldprop;
	DBUG("HandleGnomePropRequest","Routine Entered");

	a = XInternAtom(dpy, XA_WIN_STATE, False);

	for (fwin = Scr.FvwmRoot.next; fwin; fwin = fwin->next)
	{
		if (FW_W(fwin) == exc->w.w)
		{
			break;
		}
	}
	/*--------------------------------------------------------------------
	  First change the props on the window so the gnome app knows we did
	  somthing.
	  --------------------------------------------------------------------*/
#ifdef FVWM_DEBUG_MSGS
	fvwm_msg(
		DBG, "HandleGnomePropRequest", "window is %d", (int)FW_W(fwin));
#endif
	if (fwin == NULL)
	{
		return;
	}
	if (DO_IGNORE_GNOME_HINTS(fwin))
	{
		return;
	}

#ifdef FVWM_DEBUG_MSGS
	fvwm_msg(DBG, "HandleGnomePropRequest", "prop req is %d", prop);
	fvwm_msg(DBG, "HandleGnomePropRequest", "propm req is %d", propm);
#endif
	XGetWindowProperty(dpy, FW_W(fwin), a, 0L, 3L, False, a,
			   &atype, &aformat, &nitems, &bytes_remain, &indi);
	oldprop &= ~(propm);
	oldprop |= (propm & prop);
#ifdef FVWM_DEBUG_MSGS
	fvwm_msg(
		DBG, "HandleGnomePropRequest", "oldprop req is %d",
		(int)oldprop);
#endif
	XChangeProperty(
		dpy, FW_W(fwin), a, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)&oldprop, 1);

	/*--------------------------------------------------------------------
	  Then apply the requests
	  --------------------------------------------------------------------*/

	/*--------------------------------------------------------------------
	  Sticky
	  --------------------------------------------------------------------*/
	if (propm & WIN_STATE_STICKY)
	{
		if (prop & WIN_STATE_STICKY)
		{
			SET_STICKY_ACROSS_PAGES(fwin, 1);
			SET_STICKY_ACROSS_DESKS(fwin, 1);
		}
		else
		{
			SET_STICKY_ACROSS_PAGES(fwin, 0);
			SET_STICKY_ACROSS_DESKS(fwin, 0);
		}
		border_draw_decorations(
			fwin, PART_TITLE, (Scr.Hilite==fwin), True, CLEAR_ALL,
			NULL, NULL);
	}
	/*--------------------------------------------------------------------
	  WindowShade
	  -------------------------------------------------------------------*/
	if (propm & WIN_STATE_SHADED)
	{
		if (prop & WIN_STATE_SHADED)
		{
			/* shade up */
			execute_function_override_window(
				NULL, exc, "WindowShade True", 0, fwin);
		}
		else
		{
			/* shade down */
			execute_function_override_window(
				NULL, exc, "WindowShade False", 0, fwin);
		}
	}

	return;
}

void
GNOME_ProxyButtonEvent(const XEvent *ev)
{
	switch (ev->type)
	{
	case ButtonPress:
		XUngrabPointer(dpy, CurrentTime);
		FSendEvent(
			dpy, __button_proxy, False, SubstructureNotifyMask,
			(XEvent *)ev);
		break;

	case ButtonRelease:
		FSendEvent(
			dpy, __button_proxy, False, SubstructureNotifyMask,
			(XEvent *)ev);
		break;
	}

	return;
}


/* this is the function entered into FVWM's functions table */
void
CMD_GnomeButton(F_CMD_ARGS)
{
	/* send off the button press event */
	GNOME_ProxyButtonEvent(exc->x.etrigger);

	return;
}
