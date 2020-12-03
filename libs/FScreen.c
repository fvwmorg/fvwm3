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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <err.h>

#include "defaults.h"
#include "fvwmlib.h"
#include "Parse.h"
#include "PictureBase.h"
#include "FScreen.h"
#include "FEvent.h"

#ifdef HAVE_XRANDR
#	define IS_RANDR_ENABLED 1
#else
#	define IS_RANDR_ENABLED 0
#endif

#define GLOBAL_SCREEN_NAME "_global"

/* In fact, only corners matter -- there will never be GRAV_NONE */
enum {GRAV_POS = 0, GRAV_NONE = 1, GRAV_NEG = 2};
static int grav_matrix[3][3] =
{
	{ NorthWestGravity, NorthGravity,  NorthEastGravity },
	{ WestGravity,      CenterGravity, EastGravity },
	{ SouthWestGravity, SouthGravity,  SouthEastGravity }
};
#define DEFAULT_GRAVITY NorthWestGravity

static Display *disp;
static bool		 is_randr_present;

static struct monitor	*monitor_new(void);
static void		 scan_screens(Display *);
static void		 monitor_check_primary(void);

enum monitor_tracking monitor_mode;
struct screen_infos	 screen_info_q;
struct monitors		monitor_q;
int randr_event;
const char *prev_focused_monitor;

static void GetMouseXY(XEvent *eventp, int *x, int *y)
{
	XEvent e;

	if (eventp == NULL)
	{
		eventp = &e;
		e.type = 0;
	}
	fev_get_evpos_or_query(
		disp, DefaultRootWindow(disp), eventp, x, y);
}

Bool FScreenIsEnabled(void)
{
	return (IS_RANDR_ENABLED);
}

struct monitor *
monitor_new(void)
{
	struct monitor	*m;

	m = fxcalloc(1, sizeof *m);

	return (m);
}

struct screen_info *
screen_info_new(void)
{
	struct screen_info	*si;

	si = fxcalloc(1, sizeof *si);

	return (si);
}

struct screen_info *
screen_info_by_name(const char *name)
{
	struct screen_info	*si;

	TAILQ_FOREACH(si, &screen_info_q, entry) {
		if (strcmp(si->name, name) == 0)
			return (si);
	}

	return (NULL);
}

struct monitor *
monitor_get_current(void)
{
	int		 JunkX = 0, JunkY = 0, x, y;
	Window		 JunkRoot, JunkChild;
	unsigned int	 JunkMask;
	struct monitor	  *mon;
	static const char *cur_mon;

	prev_focused_monitor = cur_mon;

	FQueryPointer(disp, DefaultRootWindow(disp), &JunkRoot, &JunkChild,
			&JunkX, &JunkY, &x, &y, &JunkMask);

	mon = FindScreenOfXY(x, y);

	if (mon != NULL)
		cur_mon = mon->si->name;

	return (mon);
}

struct monitor *
monitor_by_name(const char *name)
{
	struct monitor	*m, *mret = NULL;

	if (name == NULL) {
		fvwm_debug(__func__, "%s: name is NULL; shouldn't happen.  "
			   "Returning current monitor\n", __func__);
		return (monitor_get_current());
	}

	TAILQ_FOREACH(m, &monitor_q, entry) {
		if (strcmp(m->si->name, name) == 0) {
			mret = m;
			break;
		}
	}

	if (mret == NULL && (strcmp(name, GLOBAL_SCREEN_NAME) == 0)) {
		if (monitor_get_count() == 1) {
			/* In this case, the global screen was requested, but
			 * we've only one monitor in use.  Return this monitor
			 * instead.
			 */
		    return (TAILQ_FIRST(&monitor_q));
		} else {
			/* Return the current monitor. */
			mret = monitor_get_current();
		}
	}

	/* Then we couldn't find the named monitor at all.  Return the current
	 * monitor instead.
	 */

	if (mret == NULL) {
		mret = monitor_get_current();
		if (mret == NULL)
			return (NULL);
	}

	return (mret);
}

struct monitor *
monitor_by_output(int output)
{
	struct monitor	*m, *mret = NULL;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		if (m->si->rr_output == output) {
		       mret = m;
		       break;
		}
	}

	if (mret == NULL) {
		fvwm_debug(__func__,
			   "%s: couldn't find monitor with output '%d', "
			   "returning first output.\n", __func__, output);
		mret = TAILQ_FIRST(&monitor_q);
	}

	return (mret);
}

struct monitor *
monitor_by_primary(void)
{
	struct monitor	*m = NULL, *m_loop;

	TAILQ_FOREACH(m_loop, &monitor_q, entry) {
		if (m_loop->flags & MONITOR_PRIMARY) {
			m = m_loop;
			break;
		}
	}

	return (m);
}

static void
monitor_check_primary(void)
{
	if (monitor_by_primary() == NULL) {
		struct monitor	*m;

		m = TAILQ_FIRST(&monitor_q);
		m->flags |= MONITOR_PRIMARY;
	}
}

int
monitor_get_all_widths(void)
{
	return (DisplayWidth(disp, DefaultScreen(disp)));
}

int
monitor_get_all_heights(void)
{
	return (DisplayHeight(disp, DefaultScreen(disp)));
}

void
monitor_assign_virtual(struct monitor *ref)
{
	struct monitor	*m;

	if (monitor_mode == MONITOR_TRACKING_M)
		return;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		if (m == ref)
			continue;

		memcpy(&m->virtual_scr, &ref->virtual_scr, sizeof(m->virtual_scr));
	}
}


void
FScreenSelect(Display *dpy)
{
	XRRSelectInput(disp, DefaultRootWindow(disp),
		RRScreenChangeNotifyMask | RROutputChangeNotifyMask);
}

void
monitor_output_change(Display *dpy, XRRScreenChangeNotifyEvent *e)
{
	XRRScreenResources	*res;
	struct monitor		*m = NULL;

	fvwm_debug(__func__, "%s: outputs have changed\n", __func__);

	if ((res = XRRGetScreenResources(dpy, e->root)) == NULL) {
		fvwm_debug(__func__,
			   "%s: couldn't acquire screen resources\n",
			   __func__);
		return;
	}

	{
		struct screen_info	*si;
		XRROutputInfo		*oinfo = NULL;
		int			 i;

		scan_screens(dpy);

		for (i = 0; i < res->noutput; i++) {
			oinfo = XRRGetOutputInfo(dpy, res, res->outputs[i]);
			if (oinfo == NULL)
				continue;

			si = screen_info_by_name(oinfo->name);

			if (si == NULL)
				continue;

			TAILQ_FOREACH(m, &monitor_q, entry) {
				if (m->si == si) {
					m->emit &= ~MONITOR_ALL;
					break;
				}
			}

			if (m == NULL)
				continue;

			if (oinfo->connection == RR_Connected &&
			    m->flags & MONITOR_ENABLED) {
				if (m->flags & MONITOR_CHANGED)
					m->emit |= MONITOR_CHANGED;
				continue;
			}

			if (oinfo->connection == RR_Disconnected &&
			    m->flags & MONITOR_DISABLED) {
				continue;
			}

			switch (oinfo->connection) {
			case RR_Connected:
				m->flags &= ~MONITOR_DISABLED;
				m->flags |= MONITOR_ENABLED;

				m->emit |= MONITOR_ENABLED;
				break;
			case RR_Disconnected:
				m->flags &= ~MONITOR_ENABLED;
				m->flags |= MONITOR_DISABLED;
				m->emit |= MONITOR_DISABLED;
				break;
			default:
				break;
			}
		}
		XRRFreeOutputInfo(oinfo);
	}
	XRRFreeScreenResources(res);

	monitor_check_primary();
}

static void
scan_screens(Display *dpy)
{
	XRRMonitorInfo		*rrm;
	struct monitor 		*m;
    	int			 i, n = 0;
	Window			 root = RootWindow(dpy, DefaultScreen(dpy));

	rrm = XRRGetMonitors(dpy, root, false, &n);
	if (n <= 0) {
		fvwm_debug(__func__, "get monitors failed\n");
		exit(101);
	}

	for (i = 0; i < n; i++) {

		char *name = XGetAtomName(dpy, rrm[i].name);

		if (name == NULL)
			name = "";

		if (((m = monitor_by_name(name)) == NULL) ||
		    (m != NULL && strcmp(m->si->name, name) != 0)) {
			m = monitor_new();
			m->flags |= MONITOR_NEW;
			m->si = screen_info_new();
			m->si->name = strdup(name);
			memset(&m->virtual_scr, 0, sizeof(m->virtual_scr));

			TAILQ_INSERT_TAIL(&screen_info_q, m->si, entry);
			TAILQ_INSERT_TAIL(&monitor_q, m, entry);

			goto set_coords;
		}

		if ((strcmp(m->si->name, name) == 0) &&
		    (m->si->x != rrm[i].x || m->si->y != rrm[i].y ||
		    m->si->w != rrm[i].width || m->si->h != rrm[i].height)) {
			if (m->flags & MONITOR_ENABLED)
				m->flags |= MONITOR_CHANGED;
		}

set_coords:
		m->si->x = rrm[i].x;
		m->si->y = rrm[i].y;
		m->si->w = rrm[i].width;
		m->si->h = rrm[i].height;
		m->si->rr_output = *rrm[i].outputs;
		if (rrm[i].primary > 0)
			m->flags |= MONITOR_PRIMARY;
		else
			m->flags &= ~MONITOR_PRIMARY;
		m->virtual_scr.MyDisplayWidth = monitor_get_all_widths();
		m->virtual_scr.MyDisplayHeight = monitor_get_all_heights();

	}

	monitor_check_primary();
}

void FScreenInit(Display *dpy)
{
	XRRScreenResources	*res = NULL;
	struct monitor		*m;
	int			 err_base = 0, major, minor;

	disp = dpy;
	randr_event = 0;
	is_randr_present = false;

	if (TAILQ_EMPTY(&monitor_q))
		TAILQ_INIT(&monitor_q);

	if (TAILQ_EMPTY(&screen_info_q))
		TAILQ_INIT(&screen_info_q);

	if (!XRRQueryExtension(dpy, &randr_event, &err_base) ||
	    !XRRQueryVersion (dpy, &major, &minor)) {
		fvwm_debug(__func__, "RandR not present");
		goto randr_fail;
	}

	if (major == 1 && minor >= 5)
		is_randr_present = true;


	if (FScreenIsEnabled() && !is_randr_present) {
		/* Something went wrong. */
		fvwm_debug(__func__, "Couldn't initialise XRandR: %s\n",
			   strerror(errno));
		goto randr_fail;
	}

	fvwm_debug(__func__, "Using RandR %d.%d\n", major, minor);

	/* XRandR is present, so query the screens we have. */
	res = XRRGetScreenResources(dpy, DefaultRootWindow(dpy));

	if (res == NULL || (res != NULL && res->noutput == 0)) {
		XRRFreeScreenResources(res);
		fvwm_debug(__func__, "RandR present, yet no ouputs found.");
		goto randr_fail;
	}
	XRRFreeScreenResources(res);

	scan_screens(dpy);

	TAILQ_FOREACH(m, &monitor_q, entry) {
		m->Desktops = fxcalloc(1, sizeof *m->Desktops);
		m->Desktops->name = NULL;
		m->Desktops->next = NULL;
		m->Desktops->desk = 0;
		m->flags |= (MONITOR_NEW|MONITOR_ENABLED);
	}

	monitor_check_primary();

	return;

randr_fail:
	fprintf(stderr, "Unable to initialise RandR\n");
	exit(101);
}

void
monitor_dump_state(struct monitor *m)
{
	struct monitor	*mcur, *m2;

	mcur = monitor_get_current();

	fvwm_debug(__func__, "Monitor Debug\n");
	fvwm_debug(__func__, "\tnumber of outputs: %d\n", monitor_get_count());
	TAILQ_FOREACH(m2, &monitor_q, entry) {
		if (m2 == NULL) {
			fvwm_debug(__func__,
				   "monitor in list is NULL.  Bug!\n");
			continue;
		}
		if (m != NULL && m2 != m)
			continue;
		fvwm_debug(__func__,
			   "\tName:\t%s\n"
			   "\tDisabled:\t%s\n"
			   "\tIs Primary:\t%s\n"
			   "\tIs Current:\t%s\n"
			   "\tOutput:\t%d\n"
			   "\tCoords:\t{x: %d, y: %d, w: %d, h: %d}\n"
			   "\tVirtScr: {\n"
			   "\t\tVxMax: %d, VyMax: %d, Vx: %d, Vy: %d\n"
			   "\t\tEdgeScrollX: %d, EdgeScrollY: %d\n"
			   "\t\tCurrentDesk: %d\n"
			   "\t\tCurrentPage: {x: %d, y: %d}\n"
			   "\t\tMyDisplayWidth: %d, MyDisplayHeight: %d\n\t}\n"
			   "\tDesktops:\t%s\n"
			   "\tFlags:%s\n\n",
			   m2->si->name,
			   (m2->flags & MONITOR_DISABLED) ? "true" : "false",
			   (m2->flags & MONITOR_PRIMARY) ? "yes" : "no",
			   (mcur && m2 == mcur) ? "yes" : "no",
			   (int)m2->si->rr_output,
			   m2->si->x, m2->si->y, m2->si->w, m2->si->h,
			   m2->virtual_scr.VxMax, m2->virtual_scr.VyMax,
			   m2->virtual_scr.Vx, m2->virtual_scr.Vy,
			   m2->virtual_scr.EdgeScrollX,
			   m2->virtual_scr.EdgeScrollY,
			   m2->virtual_scr.CurrentDesk,
			   (int)(m2->virtual_scr.Vx / m2->virtual_scr.MyDisplayWidth),
			   (int)(m2->virtual_scr.Vy / m2->virtual_scr.MyDisplayHeight),
			   m2->virtual_scr.MyDisplayWidth,
			   m2->virtual_scr.MyDisplayHeight,
			   m2->Desktops ? "yes" : "no",
			   monitor_mode == MONITOR_TRACKING_G ? "global" :
			   monitor_mode == MONITOR_TRACKING_M ? "per-monitor" :
			   "Unknown"
		);
	}
}

int
monitor_get_count(void)
{
	struct monitor	*m = NULL;
	int		 c = 0;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		if (m->flags & MONITOR_DISABLED)
			continue;
		c++;
	}
	return (c);
}

struct monitor *
FindScreenOfXY(int x, int y)
{
	struct monitor	*m;
	int		 xa, ya;
	int		 all_widths, all_heights;

	all_widths =  monitor_get_all_widths();
	all_heights = monitor_get_all_heights();

	xa = abs(x);
	ya = abs(y);

	xa %= all_widths;
	ya %= all_heights;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		/* If we have more than one screen configured, then don't match
		 * on the global screen, as that's separate to XY positioning
		 * which is only concerned with the *specific* screen.
		 */
		if (monitor_get_count() > 0 &&
		    strcmp(m->si->name, GLOBAL_SCREEN_NAME) == 0)
			continue;
		if (xa >= m->si->x && xa < m->si->x + m->si->w &&
		    ya >= m->si->y && ya < m->si->y + m->si->h)
			return (m);
	}

	/* FIXME: this is a convenience, but could confuse callers. */
	if (m == NULL)
		return TAILQ_FIRST(&monitor_q);

	return (NULL);
}

static struct monitor *
FindScreen(fscreen_scr_arg *arg, fscreen_scr_t screen)
{
	struct monitor	*m = NULL;
	fscreen_scr_arg  tmp;

	if (monitor_get_count() == 0)
		return (monitor_by_name(GLOBAL_SCREEN_NAME));

	switch (screen)
	{
	case FSCREEN_GLOBAL:
		m = monitor_by_name(GLOBAL_SCREEN_NAME);
		break;
	case FSCREEN_PRIMARY:
		m = monitor_by_primary();
		break;
	case FSCREEN_CURRENT:
		/* translate to xypos format */
		if (!arg)
		{
			tmp.mouse_ev = NULL;
			arg = &tmp;
		}
		GetMouseXY(arg->mouse_ev, &arg->xypos.x, &arg->xypos.y);
		/* fall through */
	case FSCREEN_XYPOS:
		/* translate to screen number */
		if (!arg)
		{
			tmp.xypos.x = 0;
			tmp.xypos.y = 0;
			arg = &tmp;
		}
		m = FindScreenOfXY(arg->xypos.x, arg->xypos.y);
		break;
	case FSCREEN_BY_NAME:
		if (arg == NULL || arg->name == NULL) {
			/* XXX: Work out what to do. */
			break;
		}
		m = monitor_by_name(arg->name);
		break;
	default:
		/* XXX: Possible error condition here? */
		break;
	}

	return (m);
}

/* Given pointer coordinates, return the screen the pointer is on.
 *
 * Perhaps most useful with $[pointer.screen]
 */
const char *
FScreenOfPointerXY(int x, int y)
{
	struct monitor	*m;

	m = FindScreenOfXY(x, y);

	return (m != NULL) ? m->si->name : "unknown";
}

/* Returns the specified screens geometry rectangle.  screen can be a screen
 * number or any of the values FSCREEN_GLOBAL, FSCREEN_CURRENT,
 * FSCREEN_PRIMARY or FSCREEN_XYPOS.  The arg union only has a meaning for
 * FSCREEN_CURRENT and FSCREEN_XYARG.  For FSCREEN_CURRENT its mouse_ev member
 * may be given.  It is tried to find out the pointer position from the event
 * first before querying the pointer.  For FSCREEN_XYPOS the xpos member is used
 * to fetch the x/y position of the point on the screen.  If arg is NULL, the
 * position 0 0 is assumed instead.
 *
 * Any of the arguments arg, x, y, w and h may be NULL.
 *
 * FSCREEN_GLOBAL:  return the global screen dimensions
 * FSCREEN_CURRENT: return dimensions of the screen with the pointer
 * FSCREEN_PRIMARY: return the primary screen dimensions
 * FSCREEN_XYPOS:   return dimensions of the screen with the given coordinates
 * FSCREEN_BY_NAME: return dimensions of the screen with the given name
 *
 * The function returns False if the global screen was returned and more than
 * one screen is configured.  Otherwise it returns True.
 */
Bool FScreenGetScrRect(fscreen_scr_arg *arg, fscreen_scr_t screen,
			int *x, int *y, int *w, int *h)
{
	struct monitor	*m = FindScreen(arg, screen);
	if (m == NULL) {
		fvwm_debug(__func__, "%s: m is NULL\n", __func__);
		return (True);
	}

	if (x)
		*x = m->si->x;
	if (y)
		*y = m->si->y;
	if (w)
		*w = m->si->w;
	if (h)
		*h = m->si->h;

	return !((monitor_get_count() > 1) &&
		(strcmp(m->si->name, GLOBAL_SCREEN_NAME) == 0));
}

/* Translates the coodinates *x *y from the screen specified by arg_src and
 * screen_src to coordinates on the screen specified by arg_dest and
 * screen_dest. (see FScreenGetScrRect for more details). */
void FScreenTranslateCoordinates(
	fscreen_scr_arg *arg_src, fscreen_scr_t screen_src,
	fscreen_scr_arg *arg_dest, fscreen_scr_t screen_dest,
	int *x, int *y)
{
	int x_src;
	int y_src;
	int x_dest;
	int y_dest;

	FScreenGetScrRect(arg_src, screen_src, &x_src, &y_src, NULL, NULL);
	FScreenGetScrRect(arg_dest, screen_dest, &x_dest, &y_dest, NULL, NULL);

	if (x)
	{
		*x = *x + x_src - x_dest;
	}
	if (y)
	{
		*y = *y + y_src - y_dest;
	}

	return;
}

/* Arguments work exactly like for FScreenGetScrRect() */
int FScreenClipToScreen(fscreen_scr_arg *arg, fscreen_scr_t screen,
			int *x, int *y, int w, int h)
{
	int sx;
	int sy;
	int sw;
	int sh;
	int lx = (x) ? *x : 0;
	int ly = (y) ? *y : 0;
	int x_grav = GRAV_POS;
	int y_grav = GRAV_POS;

	FScreenGetScrRect(arg, screen, &sx, &sy, &sw, &sh);
	if (lx + w > sx + sw)
	{
		lx = sx + sw  - w;
		x_grav = GRAV_NEG;
	}
	if (ly + h > sy + sh)
	{
		ly = sy + sh - h;
		y_grav = GRAV_NEG;
	}
	if (lx < sx)
	{
		lx = sx;
		x_grav = GRAV_POS;
	}
	if (ly < sy)
	{
		ly = sy;
		y_grav = GRAV_POS;
	}
	if (x)
	{
		*x = lx;
	}
	if (y)
	{
		*y = ly;
	}

	return grav_matrix[y_grav][x_grav];
}

/* Arguments work exactly like for FScreenGetScrRect() */
void FScreenCenterOnScreen(fscreen_scr_arg *arg, fscreen_scr_t screen,
			   int *x, int *y, int w, int h)
{
	int sx;
	int sy;
	int sw;
	int sh;
	int lx;
	int ly;

	FScreenGetScrRect(arg, screen, &sx, &sy, &sw, &sh);
	lx = (sw  - w) / 2;
	ly = (sh - h) / 2;
	if (lx < 0)
		lx = 0;
	if (ly < 0)
		ly = 0;
	lx += sx;
	ly += sy;
	if (x)
	{
		*x = lx;
	}
	if (y)
	{
		*y = ly;
	}
}

void FScreenGetResistanceRect(
	int wx, int wy, unsigned int ww, unsigned int wh, int *x0, int *y0,
	int *x1, int *y1)
{
	fscreen_scr_arg arg;

	arg.xypos.x = wx + ww / 2;
	arg.xypos.y = wy + wh / 2;
	FScreenGetScrRect(&arg, FSCREEN_XYPOS, x0, y0, x1, y1);
	*x1 += *x0;
	*y1 += *y0;
}

/* Arguments work exactly like for FScreenGetScrRect() */
Bool FScreenIsRectangleOnScreen(fscreen_scr_arg *arg, fscreen_scr_t screen,
				rectangle *rec)
{
	int sx;
	int sy;
	int sw;
	int sh;

	FScreenGetScrRect(arg, screen, &sx, &sy, &sw, &sh);

	return (rec->x + rec->width > sx && rec->x < sx + sw &&
		rec->y + rec->height > sy && rec->y < sy + sh) ? True : False;
}

/*
 * FScreenParseGeometry
 *     Does the same as XParseGeometry, but handles additional "@scr".
 *     Since it isn't safe to define "ScreenValue" constant (actual values
 *     of other "XXXValue" are specified in Xutil.h, not by us, so there can
 *     be a clash), the screen value is always returned, even if it wasn't
 *     present in `parse_string' (set to default in that case).
 *
 */
int FScreenParseGeometryWithScreen(
	char *parsestring, int *x_return, int *y_return,
	unsigned int *width_return, unsigned int *height_return,
	char **screen_return)
{
	char *copy, *geom_str = NULL;
	int   ret;

	/* Safety net */
	if (parsestring == NULL || *parsestring == '\0') {
		*screen_return = NULL;
		return 0;
	}

	/* No screen specified; parse geometry as standard. */
	if (strchr(parsestring, '@') == NULL) {
		*screen_return = NULL;
		copy = fxstrdup(parsestring);
		goto parse_geometry;
	}

	/* If the geometry specification contains an '@' symbol, assume the
	 * screen is specified.  This must be the name of the monitor in
	 * question!
	 */
	copy = fxstrdup(parsestring);
	copy = strsep(&parsestring, "@");

	*screen_return = fxstrdup(parsestring);
	geom_str = strsep(&copy, "@");
	copy = geom_str;

parse_geometry:
	/* Do the parsing */
	ret = XParseGeometry(
		copy, x_return, y_return, width_return, height_return);

	if (*screen_return == NULL)
		*screen_return = fxstrdup(monitor_by_primary()->si->name);

	return ret;
}

/* Same as above, but dump screen return value to keep compatible with the X
 * function. */
int FScreenParseGeometry(
	char *parsestring, int *x_return, int *y_return,
	unsigned int *width_return, unsigned int *height_return)
{
	struct monitor	*m = monitor_get_current();
	char		*scr = NULL;
	int		 rc, x, y, w, h;

	x = y = 0;
	w = monitor_get_all_widths();
	h = monitor_get_all_heights();

	rc = FScreenParseGeometryWithScreen(
		parsestring, x_return, y_return, width_return, height_return,
		&scr);

	if (scr != NULL) {
		/* If we've asked for "@g" then use the global screen.  The
		 * x,y,w,h values are already assigned, so skip that.
		 */
		if (strcmp(scr, "g") == 0)
			goto adapt;

		/* "@c" is for the current screen. */
		if (strcmp(scr, "c") == 0)
			goto assign;

		/* "@p" is for the primary screen. */
		if (strcmp(scr, "p") == 0) {
			m = monitor_by_primary();
			goto assign;
		}
		m = monitor_by_name(scr);
	}
assign:
		x = m->si->x;
		y = m->si->y;
		w = m->si->w;
		h = m->si->h;
adapt:
	free(scr);

	/* adapt geometry to selected screen */
	if (rc & XValue)
	{
		if (rc & XNegative)
			*x_return -= (m->virtual_scr.MyDisplayWidth - w - x);
		else
			*x_return += x;
	}
	if (rc & YValue)
	{
		if (rc & YNegative)
			*y_return -= (m->virtual_scr.MyDisplayHeight - h - y);
		else
			*y_return += y;
	}
	return rc;
}


/*  FScreenGetGeometry
 *      Parses the geometry in a form: XGeometry[@screen], i.e.
 *          [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>][@<screen>]
 *          where <screen> is either a number or "G" (global) "C" (current)
 *          or "P" (primary)
 *
 *  Args:
 *      parsestring, x_r, y_r, w_r, h_r  the same as in XParseGeometry
 *      hints  window hints structure, may be NULL
 *      flags  bitmask of allowed flags (XValue, WidthValue, XNegative...)
 *
 *  Note1:
 *      hints->width and hints->height will be used to calc negative geometry
 *      if width/height isn't specified in the geometry itself.
 *
 *  Note2:
 *      This function's behaviour is crafted to sutisfy/emulate the
 *      FvwmWinList::MakeMeWindow()'s behaviour.
 *
 *  Note3:
 *      A special value of `flags' when [XY]Value are there but [XY]Negative
 *      aren't, means that in case of negative geometry specification
 *      x_r/y_r values will be promoted to the screen border, but w/h
 *      wouldn't be subtracted, so that the program can do x-=w later
 *      ([XY]Negative *will* be returned, albeit absent in `flags').
 *          This option is supposed for proggies like FvwmButtons, which
 *      receive geometry specification long before they are able to actually
 *      use it (and which calculate w/h themselves).
 *          (The same effect can't be obtained with omitting {Width,Height}Value
 *      in the flags, since the app may wish to get the dimensions but apply
 *      some constraints later (as FvwmButtons do, BTW...).)
 *          This option can be also useful in cases where dimensions are
 *      specified not in pixels but in some other units (e.g., charcells).
 */
int FScreenGetGeometry(
	char *parsestring, int *x_return, int *y_return,
	int *width_return, int *height_return, XSizeHints *hints, int flags)
{
	fscreen_scr_arg	 arg;
	char		*scr = NULL;
	int		 ret;
	int		 saved;
	int		 x, y;
	unsigned int	 w = 0, h = 0;
	int		 grav, x_grav, y_grav;
	int		 scr_x, scr_y;
	int		 scr_w, scr_h;

	/* I. Do the parsing and strip off extra bits */
	ret = FScreenParseGeometryWithScreen(parsestring, &x, &y, &w, &h, &scr);
	saved = ret & (XNegative | YNegative);
	ret  &= flags;

	arg.mouse_ev = NULL;
	arg.name = scr;
	FScreenGetScrRect(&arg, FSCREEN_BY_NAME, &scr_x, &scr_y, &scr_w, &scr_h);

	/* II. Interpret and fill in the values */

	/* Fill in dimensions for future negative calculations if
	 * omitted/forbidden */
	/* Maybe should use *x_return,*y_return if hints==NULL?
	 * Unreliable... */
	if (hints != NULL  &&  hints->flags & PSize)
	{
		if ((ret & WidthValue)  == 0)
		{
			w = hints->width;
		}
		if ((ret & HeightValue) == 0)
		{
			h = hints->height;
		}
	}
	else
	{
		/* This branch is required for case when size *is* specified,
		 * but masked off */
		if ((ret & WidthValue)  == 0)
		{
			w = 0;
		}
		if ((ret & HeightValue) == 0)
		{
			h = 0;
		}
	}

	/* Advance coords to the screen... */
	x += scr_x;
	y += scr_y;

	/* ...and process negative geometries */
	if (saved & XNegative)
	{
		x += scr_w;
	}
	if (saved & YNegative)
	{
		y += scr_h;
	}
	if (ret & XNegative)
	{
		x -= w;
	}
	if (ret & YNegative)
	{
		y -= h;
	}

	/* Restore negative bits */
	ret |= saved;

	/* Guess orientation */
	x_grav = (ret & XNegative)? GRAV_NEG : GRAV_POS;
	y_grav = (ret & YNegative)? GRAV_NEG : GRAV_POS;
	grav   = grav_matrix[y_grav][x_grav];

	/* Return the values */
	if (ret & XValue)
	{
		*x_return = x;
		if (hints != NULL)
		{
			hints->x = x;
		}
	}
	if (ret & YValue)
	{
		*y_return = y;
		if (hints != NULL)
		{
			hints->y = y;
		}
	}
	if (ret & WidthValue)
	{
		*width_return = w;
		if (hints != NULL)
		{
			hints->width = w;
		}
	}
	if (ret & HeightValue)
	{
		*height_return = h;
		if (hints != NULL)
		{
			hints->height = h;
		}
	}
	if (1 /*flags & GravityValue*/  &&  grav != DEFAULT_GRAVITY)
	{
		if (hints != NULL  &&  hints->flags & PWinGravity)
		{
			hints->win_gravity = grav;
		}
	}
	if (hints != NULL  &&  ret & XValue  &&  ret & YValue)
		hints->flags |= USPosition;

	return ret;
}

/*  FScreenMangleScreenIntoUSPosHints
 *      A hack to mangle the screen number into the XSizeHints structure.
 *      If the USPosition flag is set, hints->x is set to the magic number and
 *      hints->y is set to the screen number.  If the USPosition flag is clear,
 *      x and y are set to zero.
 *
 *  Note:  This is a *hack* to allow modules to specify the target screen for
 *  their windows and have the StartsOnScreen style set for them at the same
 *  time.  Do *not* rely on the mechanism described above.
 */
void FScreenMangleScreenIntoUSPosHints(fscreen_scr_t screen, XSizeHints *hints)
{
	if (hints->flags & USPosition)
	{
		hints->x = FSCREEN_MANGLE_USPOS_HINTS_MAGIC;
		hints->y = (short)screen;
	}
	else
	{
		hints->x = 0;
		hints->y = 0;
	}

	return;
}

/*  FScreenMangleScreenIntoUSPosHints
 *      A hack to mangle the screen number into the XSizeHints structure.
 *      If the USPosition flag is set, hints->x is set to the magic number and
 *      hints->y is set to the screen spec.  If the USPosition flag is clear,
 *      x and y are set to zero.
 *
 *  Note:  This is a *hack* to allow modules to specify the target screen for
 *  their windows and have the StartsOnScreen style set for them at the same
 *  time.  Do *not* rely on the mechanism described above.
 */
int FScreenFetchMangledScreenFromUSPosHints(XSizeHints *hints)
{
	int screen;

	if ((hints->flags & USPosition) &&
	    hints->x == FSCREEN_MANGLE_USPOS_HINTS_MAGIC)
	{
		screen = hints->y;
	} else
		screen = 0;

	return screen;
}
