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

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/extensions/randr.h>
#include <X11/extensions/Xrandr.h>

#include "defaults.h"
#include "FEvent.h"
#include "FScreen.h"
#include "fvwmlib.h"
#include "log.h"
#include "Parse.h"
#include "PictureBase.h"
#include "queue.h"
#include "strtonum.h"

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
static bool		 randr_initialised;

static void		 scan_screens(Display *);
static struct monitor	*monitor_new(const char *);
static void		 monitor_mark_new(struct monitor *);
static void		 monitor_mark_inlist(const char *);
static bool		 monitor_mark_changed(struct monitor *, XRRMonitorInfo *);
static struct monitor	*monitor_by_name(const char *);
static void		 monitor_set_coords(struct monitor *, XRRMonitorInfo);
static void		 monitor_assign_number(void);
static void		 monitor_add(XRRMonitorInfo *);

enum monitor_tracking monitor_mode;
bool			 is_tracking_shared;
struct screen_infos	 screen_info_q, screen_info_q_temp;
struct monitors		 monitor_q;
int randr_event;
const char *prev_focused_monitor;
static struct monitor	*monitor_global = NULL;

RB_GENERATE(monitors, monitor, entry, monitor_compare);

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

static struct monitor *
monitor_new(const char *name)
{
	struct monitor	*m;

	m = fxcalloc(1, sizeof *m);
	m->flags = 0;
	m->si = screen_info_new();
	m->si->name = fxstrdup(name);

	return (m);
}

static void
monitor_scan_edges(struct monitor *m)
{
	struct monitor	*m_loop;

	if (m == NULL)
		return;

	m->edge.top = m->edge.bottom = m->edge.left = m->edge.right =
		MONITOR_OUTSIDE_EDGE;

	RB_FOREACH(m_loop, monitors, &monitor_q) {
		if (m_loop == m)
			continue;

		/* Top Edge */
		if ( m->si->y == m_loop->si->y + m_loop->si->h )
			m->edge.top = MONITOR_INSIDE_EDGE;
		/* Bottom Edge */
		if ( m->si->y + m->si->h == m_loop->si->y )
			m->edge.bottom = MONITOR_INSIDE_EDGE;
		/* Left Edge */
		if ( m->si->x == m_loop->si->x + m_loop->si->w )
			m->edge.left = MONITOR_INSIDE_EDGE;
		/* Right Edge */
		if ( m->si->x + m->si->w == m_loop->si->x )
			m->edge.right = MONITOR_INSIDE_EDGE;
	}
}

struct monitor *
monitor_by_number(int n)
{
	struct monitor	*m_loop;

	RB_FOREACH(m_loop, monitors, &monitor_q) {
		if (m_loop->number == n)
			return (m_loop);
	}
	return (NULL);
}

void
monitor_refresh_global(void)
{
	/* Creating the global monitor outside of the monitor_q structure
	 * allows for only the relevant information to be presented to callers
	 * which are asking for a global screen.
	 *
	 * This structure therefore only uses some of the fields:
	 *
	 *   si->name,
	 *   si->{x, y, w, h}
	 */
	if (monitor_global == NULL)
		monitor_global = monitor_new(GLOBAL_SCREEN_NAME);

	/* At this point, the global screen has been initialised.  Refresh the
	 * coordinate list.
	 */
	monitor_global->si->x = 0;
	monitor_global->si->y = 0;
	monitor_global->si->w = monitor_get_all_widths();
	monitor_global->si->h = monitor_get_all_heights();
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
monitor_get_global(void)
{
	monitor_refresh_global();

	return monitor_global;
}

struct monitor *
monitor_get_prev(void)
{
	struct monitor	*m, *mret = NULL;
	struct monitor  *this = monitor_get_current();

	RB_FOREACH(m, monitors, &monitor_q) {
		if (m == this)
			continue;
		if (m->is_prev) {
			mret = m;
			break;
		}
	}

	/* Can be NULL -- is checked in expand.c */
	return (mret);
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
monitor_resolve_name(const char *scr)
{
	struct monitor	*m = NULL;
	int		 pos = -1;
	const char	*errstr;

	if (scr == NULL)
	{
		return NULL;
	}

	/* Try and look up the monitor as a number.  If that succeeds,
	 * try and match that number as something valid in the monitor
	 * information we have.
	 */
	pos = strtonum(scr, 0, INT_MAX, &errstr);
	if (errstr == NULL)
		return (monitor_by_number(pos));

	/* "@g" is for the global screen. */
	if (strcmp(scr, "g") == 0) {
		m = monitor_get_global();
	}
	/* "@c" is for the current screen. */
	else if (strcmp(scr, "c") == 0)
		m = monitor_get_current();
	/* "@p" is for the primary screen. */
	else if (strcmp(scr, "p") == 0)
		m = monitor_by_primary();
	else
		/* Assume the monitor name is a literal RandR name (such as
		 * HDMI2). */
		m = monitor_by_name(scr);

	return (m);
}

static struct monitor *
monitor_by_name(const char *name)
{
	struct monitor	*m, *mret;

	if (name == NULL) {
		fvwm_debug(__func__, "%s: name is NULL; shouldn't happen.  "
			   "Returning current monitor\n", __func__);
		return (monitor_get_current());
	}

	mret = NULL;
	RB_FOREACH(m, monitors, &monitor_q) {
		if (strcmp(m->si->name, name) == 0) {
			mret = m;
			break;
		}
	}

	return (mret);
}

struct monitor *
monitor_by_output(int output)
{
	struct monitor	*m, *mret = NULL;

	RB_FOREACH(m, monitors, &monitor_q) {
		if (m->si->rr_output == output) {
		       mret = m;
		       break;
		}
	}

	if (mret == NULL) {
		fvwm_debug(__func__,
			   "%s: couldn't find monitor with output '%d', "
			   "returning first output.\n", __func__, output);
		mret = RB_MIN(monitors, &monitor_q);
	}

	return (mret);
}

struct monitor *
monitor_by_primary(void)
{
	struct monitor	*m = NULL, *m_loop;

	RB_FOREACH(m_loop, monitors, &monitor_q) {
		if (m_loop->flags & MONITOR_PRIMARY) {
			m = m_loop;
			break;
		}
	}

	return (m);
}

struct monitor *
monitor_by_last_primary(void)
{
	struct monitor	*m = NULL, *m_loop;

	RB_FOREACH(m_loop, monitors, &monitor_q) {
		if (m_loop->was_primary && !(m_loop->flags & MONITOR_PRIMARY)) {
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

		m = RB_MIN(monitors, &monitor_q);
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

	if (monitor_mode == MONITOR_TRACKING_M || is_tracking_shared)
		return;

	RB_FOREACH(m, monitors, &monitor_q) {
		if (m == ref)
			continue;

		memcpy(
			&m->virtual_scr, &ref->virtual_scr,
			sizeof(m->virtual_scr));
	}
}


void
FScreenSelect(Display *dpy)
{
	XRRSelectInput(dpy, DefaultRootWindow(dpy),
		RRScreenChangeNotifyMask | RROutputChangeNotifyMask);
}

void
monitor_output_change(Display *dpy, XRRScreenChangeNotifyEvent *e)
{
	scan_screens(dpy);
}

static void
monitor_set_coords(struct monitor *m, XRRMonitorInfo rrm)
{
	if (m == NULL)
		return;

	m->si->x = rrm.x;
	m->si->y = rrm.y;
	m->si->w = rrm.width;
	m->si->h = rrm.height;
	m->si->rr_output = *rrm.outputs;
	if (rrm.primary > 0) {
		m->flags |= MONITOR_PRIMARY;
		m->was_primary = false;
	} else {
		if (m->flags & MONITOR_PRIMARY) {
			m->flags &= ~MONITOR_PRIMARY;
			m->was_primary = true;
		}
	}
}

static void
monitor_assign_number(void)
{
	struct monitor	*m;
	int		 n = 0;

	RB_FOREACH(m, monitors, &monitor_q) {
		if (m->flags & MONITOR_DISABLED)
			continue;
		m->number = n++;
	}
}

int
monitor_compare(struct monitor *a, struct monitor *b)
{
	return (a->si->y == b->si->y) ?
		(a->si->x - b->si->x) : (a->si->y - b->si->y);
}

static void
monitor_mark_new(struct monitor *m)
{
	if (m == NULL)
		return;

	m->flags |= MONITOR_NEW;

	memset(&m->virtual_scr, 0, sizeof(m->virtual_scr));
	m->virtual_scr.edge_thickness = 2;
	m->virtual_scr.last_edge_thickness = 2;

	memset(&m->ewmhc, 0, sizeof(m->ewmhc));
	m->Desktops = NULL;
	m->is_prev = false;
	m->was_primary = false;

	TAILQ_INSERT_TAIL(&screen_info_q, m->si, entry);

	fvwm_debug(__func__, "Added new monitor: %s (%p)", m->si->name, m);
}

static void
monitor_add(XRRMonitorInfo *rrm)
{
	struct monitor	*m = NULL;
	char		*name = NULL;

	if ((name = XGetAtomName(disp, rrm->name)) == NULL) {
		fvwm_debug(__func__, "Tried detecting monitor "
		    "with output '%d' but it has no name.  Skipping.",
		    (int)*rrm->outputs);
		return;
	}
	m = monitor_new(name);
	monitor_set_coords(m, *rrm);
	monitor_mark_new(m);

	XFree(name);

	RB_INSERT(monitors, &monitor_q, m);
}

static bool
monitor_mark_changed(struct monitor *m, XRRMonitorInfo *rrm)
{
	if (m == NULL || rrm == NULL)
		return (false);

	if (m->si->x != rrm->x ||
	    m->si->y != rrm->y ||
	    m->si->w != rrm->width ||
	    m->si->h != rrm->height) {
		m->flags |= MONITOR_CHANGED;

		fvwm_debug(__func__, "%s: x: %d, y: %d, w: %d, h: %d "
			"comp: x: %d, y: %d, w: %d, h: %d\n",
			m->si->name, m->si->x, m->si->y, m->si->w, m->si->h,
			rrm->x, rrm->y, rrm->width, rrm->height);

		monitor_set_coords(m, *rrm);

		return (true);
	}
	return (false);
}

static void
monitor_mark_inlist(const char *name)
{
	struct monitor	 *m = NULL;

	if (name == NULL)
		return;

	RB_FOREACH(m, monitors, &monitor_q) {
		if (strcmp(m->si->name, name) == 0) {
			m->flags |= MONITOR_FOUND;
			break;
		}
	}
}

static void
scan_screens(Display *dpy)
{
	struct monitor		*m = NULL, *m1 = NULL;
	XRRMonitorInfo		*rrm;
	char			*name = NULL;
    	int			 i, n = 0;
	Window			 root = RootWindow(dpy, DefaultScreen(dpy));

	RB_FOREACH(m, monitors, &monitor_q) {
		m->flags &= ~(MONITOR_NEW|MONITOR_CHANGED);
	}

	rrm = XRRGetMonitors(dpy, root, true, &n);
	if (n <= 0 && (!randr_initialised && monitor_get_count() == 0)) {
		fvwm_debug(__func__, "get monitors failed\n");
		exit(101);
	}

	/*
	 * 1. Handle the case where we have no monitors at all.  This is going
	 * to happen at init time.  In such cases, we can say that the
	 * monitor_q is empty.
	 *
	 * To handle this case, we create all entries, marking them as new.
	 */
	if (!randr_initialised && monitor_get_count() == 0) {
		fvwm_debug(__func__, "Case 1: Add new monitors");
		for (i = 0; i < n; i++) {
			monitor_add(&rrm[i]);
		}
		goto out;
	}

	/*
	 * 2. Handle the case where an event has occurred for a monitor.  It
	 * could be that:
	 *   - It's a new monitor.
	 *   - It's an existing monitor (position changed)
	 *   - It's an existing monitor which has been toggled on or off.
	 *
	 * In such cases, we must detect if the monitor exists and what state it
	 * is in.  Regardless of how the monitor state has changed, we flag all
	 * monitors reported by XRRGetMonitors with flag MONITOR_FOUND, which we
	 * use below to determine its new state.
	 */
        fvwm_debug(__func__, "Case 2: processing %d monitors", n);
	for (i = 0; i < n; i++) {
		if ((name = XGetAtomName(dpy, rrm[i].name)) == NULL) {
			fvwm_debug(__func__, "NAME WAS NULL!");
			continue;
		}
		if ((m = monitor_by_name(name)) == NULL) {
			/* Case 2.1 -- new monitor. */
			fvwm_debug(__func__, "Case 2.1: new monitor (%s)",
			    name);
			monitor_add(&rrm[i]);
		}

		/* Flag monitor as MONITOR_FOUND. */
		monitor_mark_inlist(name);

		if (monitor_mark_changed(m, &rrm[i])) {
			fvwm_debug(__func__, "Case 2.2: %s changed", m->si->name);
		}

		XFree(name);
	}

out:
	/* Update monitor order after changes.  Do not mix that up with the
	 * following loop or some monitors might get their flags processed
	 * twice.
	 */
	RB_FOREACH_SAFE(m, monitors, &monitor_q, m1) {
		if (m->flags & MONITOR_CHANGED) {
			RB_REMOVE(monitors, &monitor_q, m);
			RB_INSERT(monitors, &monitor_q, m);
		}
	}

	RB_FOREACH(m, monitors, &monitor_q) {
		int  flags = m->flags;
		bool found = m->flags & MONITOR_FOUND;

		/* Check for monitor connection status -- whether a monitor is
		 * active or not.  Clearing the MONITOR_FOUND flag is
		 * important here so that the monitor is reconsidered again.
		 */
		if (found && (flags & MONITOR_NEW)) {
			m->flags &= ~MONITOR_DISABLED;
			m->emit   = MONITOR_ENABLED;
		} else if (found && (flags & MONITOR_CHANGED)) {
			m->flags &= ~MONITOR_DISABLED;
			m->emit   = MONITOR_CHANGED;
		} else if (found && (flags & MONITOR_DISABLED)) {
			m->flags &= ~MONITOR_DISABLED;
			m->emit   = MONITOR_ENABLED;
		} else if (found && (! (flags & MONITOR_DISABLED))) {
			m->emit   = 0;
		} else if (!found && (flags & MONITOR_NEW)) {
			/* This case happens if !randr_initialised. */
			m->emit   = 0;
		} else if (!found && (flags & MONITOR_DISABLED)) {
			m->emit   = 0;
		} else /* (!found && (! (flags & MONITOR_DISABLED))) */ {
			m->flags |= MONITOR_DISABLED;
			m->emit   = MONITOR_DISABLED;
		}
		m->flags &= ~MONITOR_FOUND;
	}
	/* Now that all monitors have been inserted, assign them a number from
	 * 0 -> n so that they can be referenced in order.
	 */
	monitor_assign_number();
	monitor_check_primary();
	XRRFreeMonitors(rrm);
}

void FScreenInit(Display *dpy)
{
	XRRScreenResources	*res = NULL;
	struct monitor		*m;
	int			 err_base = 0, major, minor;

	if (randr_initialised)
		return;

	disp = dpy;
	randr_event = 0;
	is_randr_present = false;
	randr_initialised = false;

	if (RB_EMPTY(&monitor_q))
		RB_INIT(&monitor_q);

	if (TAILQ_EMPTY(&screen_info_q))
		TAILQ_INIT(&screen_info_q);

	if (!XRRQueryExtension(dpy, &randr_event, &err_base) ||
	    !XRRQueryVersion (dpy, &major, &minor)) {
		fvwm_debug(__func__, "RandR not present");
		goto randr_fail;
	}

	if (major == 1 && minor >= 5)
		is_randr_present = true;


	if (!is_randr_present) {
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
		fvwm_debug(__func__, "RandR present, yet no outputs found.");
		goto randr_fail;
	}
	XRRFreeScreenResources(res);

	scan_screens(dpy);
	randr_initialised = true;
	is_tracking_shared = false;

	RB_FOREACH(m, monitors, &monitor_q)
		monitor_scan_edges(m);

	return;

randr_fail:
	fprintf(stderr, "Unable to initialise RandR\n");
	exit(101);
}

void
monitor_refresh_module(Display *dpy)
{
	struct monitor	*m = NULL;

	RB_FOREACH(m, monitors, &monitor_q)
		m->flags &= ~MONITOR_NEW;

	scan_screens(dpy);

	RB_FOREACH(m, monitors, &monitor_q) {
		if (m->flags & MONITOR_NEW)
			fprintf(stderr, "MON: %s is NEW...\n", m->si->name);
	}
}

void
monitor_dump_state(struct monitor *m)
{
	struct monitor	*mcur, *m2;

	mcur = monitor_get_current();

	fvwm_debug(__func__, "Monitor Debug\n");
	fvwm_debug(__func__, "\tnumber of outputs: %d\n", monitor_get_count());
	RB_FOREACH(m2, monitors, &monitor_q) {
		if (m2 == NULL) {
			fvwm_debug(__func__,
				   "monitor in list is NULL.  Bug!\n");
			continue;
		}
		if (m != NULL && m2 != m)
			continue;
		fvwm_debug(
			__func__,
			"\tNumber:\t%d\n"
			"\tName:\t%s\n"
			"\tDisabled:\t%s\n"
			"\tIs Primary:\t%s\n"
			"\tIs Current:\t%s\n"
			"\tIs Previous:\t%s\n"
			"\tOutput:\t%d\n"
			"\tCoords:\t{x: %d, y: %d, w: %d, h: %d}\n"
			"\tVirtScr: {\n"
			"\t\tVxMax: %d, VyMax: %d, Vx: %d, Vy: %d\n"
			"\t\tEdgeScrollX: %d, EdgeScrollY: %d\n"
			"\t\tCurrentDesk: %d\n"
			"\t\tCurrentPage: {x: %d, y: %d}\n"
			"\t\tMyDisplayWidth: %d, MyDisplayHeight: %d\n\t}\n"
			"\tEWMH: {\n"
			"\t\tBaseStrut Top:    %d\n"
			"\t\tBaseStrut Bottom: %d\n"
			"\t\tBaseStrut Left:   %d\n"
			"\t\tBaseStrut Right:  %d\n\t}\n"
			"\tDesktops:\t%s\n"
			"\tFlags:%s\n\n",
			m2->number,
			m2->si->name,
			(m2->flags & MONITOR_DISABLED) ? "true" : "false",
			(m2->flags & MONITOR_PRIMARY) ? "yes" : "no",
			(mcur && m2 == mcur) ? "yes" : "no",
			m2->is_prev ? "yes" : "no",
			(int)m2->si->rr_output,
			m2->si->x, m2->si->y, m2->si->w, m2->si->h,
			m2->virtual_scr.VxMax, m2->virtual_scr.VyMax,
			m2->virtual_scr.Vx, m2->virtual_scr.Vy,
			m2->virtual_scr.EdgeScrollX,
			m2->virtual_scr.EdgeScrollY,
			m2->virtual_scr.CurrentDesk,
			(int)(m2->virtual_scr.Vx / monitor_get_all_widths()),
			(int)(m2->virtual_scr.Vy / monitor_get_all_heights()),
			monitor_get_all_widths(),
			monitor_get_all_heights(),
			m2->ewmhc.BaseStrut.top,
			m2->ewmhc.BaseStrut.bottom,
			m2->ewmhc.BaseStrut.left,
			m2->ewmhc.BaseStrut.right,
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

	if (RB_EMPTY(&monitor_q))
		return c;

	RB_FOREACH(m, monitors, &monitor_q) {
		if (m->flags & MONITOR_DISABLED)
			continue;
		c++;
	}
	return (c);
}

struct monitor *
FindScreenOfXY(int x, int y)
{
	struct monitor	*m, *m_min = NULL;
	int dmin = INT_MAX;
	int all_widths = monitor_get_all_widths();
	int all_heights = monitor_get_all_heights();

	x %= all_widths;
	y %= all_heights;
	if (x < 0)
		x += all_widths;
	if (y < 0)
		y += all_heights;

	RB_FOREACH(m, monitors, &monitor_q) {
		/* If we have more than one screen configured, then don't match
		 * on the global screen, as that's separate to XY positioning
		 * which is only concerned with the *specific* screen.
		 */
		if (monitor_get_count() > 0 &&
		    strcmp(m->si->name, GLOBAL_SCREEN_NAME) == 0)
			continue;
		if (x >= m->si->x && x < m->si->x + m->si->w &&
		    y >= m->si->y && y < m->si->y + m->si->h)
			return (m);
	}

	/* No monitor found, point is in a dead area.
	 * Find closest monitor using the taxi cab metric.
	 */
	RB_FOREACH(m, monitors, &monitor_q) {
		int d = 0;

		/* Skip global screen */
		if (m == monitor_global)
			continue;

		if (x < m->si->x)
			d += m->si->x  - x;
		if (x > m->si->x + m->si->w)
			d += x - m->si->x - m->si->w;
		if (y < m->si->y)
			d += m->si->y - y;
		if (y > m->si->y + m->si->h)
			d += y - m->si->y - m->si->h;
		if (d < dmin) {
			dmin = d;
			m_min = m;
		}
	}

	/* Shouldn't happen. */
	if (m_min == NULL) {
		fvwm_debug(__func__, "Couldn't find any monitor");
		exit(106);
	}
	return (m_min);
}

/* Returns the primary screen if arg.name is NULL. */
static struct monitor *
FindScreen(fscreen_scr_arg *arg, fscreen_scr_t screen)
{
	struct monitor	*m = NULL;
	fscreen_scr_arg  tmp;

	if (monitor_get_count() == 0) {
		return (monitor_get_global());
	}

	switch (screen)
	{
	case FSCREEN_GLOBAL:
		m = monitor_get_global();
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
		if (arg == NULL || arg->name == NULL)
		{
			m = monitor_by_primary();
		}
		else
		{
			m = monitor_resolve_name(arg->name);
		}
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
 *                  of of the primary screen if arg.name is NULL.
 *
 * The function returns False if the global screen was returned and more than
 * one screen is configured.  Otherwise it returns True.
 */
Bool FScreenGetScrRect(fscreen_scr_arg *arg, fscreen_scr_t screen,
			int *x, int *y, int *w, int *h)
{
	struct monitor	*m = FindScreen(arg, screen);
	if (m == NULL) {
		fvwm_debug(__func__, "m is NULL, using global screen\n");
		m = monitor_get_global();
	}

	if (x)
		*x = m->si->x;
	if (y)
		*y = m->si->y;
	if (w)
		*w = m->si->w;
	if (h)
		*h = m->si->h;

	return !((monitor_get_count() > 1) && m == monitor_global);
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
	int sx = 0;
	int sy = 0;
	int sw = 0;
	int sh = 0;
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
	int sx = 0;
	int sy = 0;
	int sw = 0;
	int sh = 0;
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
	int sx = 0;
	int sy = 0;
	int sw = 0;
	int sh = 0;

	FScreenGetScrRect(arg, screen, &sx, &sy, &sw, &sh);

	return (rec->x + rec->width > sx && rec->x < sx + sw &&
		rec->y + rec->height > sy && rec->y < sy + sh) ? True : False;
}

/*
 * FScreenParseGeometry
 *     Does the same as XParseGeometry, but handles additional "@scr".
 *     Since it isn't safe to define "ScreenValue" constant (actual values
 *     of other "XXXValue" are specified in Xutil.h, not by us, so there can
 *     be a clash).  *_screen_return is set to the screen's name or NULL if no
 *     present in parse_string.
 *
 */
int FScreenParseGeometryWithScreen(
	char *parsestring, int *x_return, int *y_return,
	unsigned int *width_return, unsigned int *height_return,
	char **screen_return)
{
	char *geom_str;
	int   ret;
	char *at;

	if (screen_return != NULL)
	{
		*screen_return = NULL;
	}
	/* Safety net */
	if (parsestring == NULL || *parsestring == '\0')
	{
		return 0;
	}

	/* No screen specified; parse geometry as standard. */
	at = strchr(parsestring, '@');
	if (at == NULL)
	{
		geom_str = parsestring;
	}
	else
	{
		/* If the geometry specification contains an '@' symbol, assume
		 * the screen is specified.  This must be the name of the
		 * monitor in question.
		 */
		int atpos;

		if (screen_return != NULL)
		{
			*screen_return = at + 1;
		}
		atpos = (int)(at - parsestring);
		geom_str = fxstrdup(parsestring);
		geom_str[atpos] = 0;
	}

	/* Do the parsing */
	ret = XParseGeometry(
		geom_str, x_return, y_return, width_return, height_return);

	if (geom_str != parsestring)
	{
		free(geom_str);
	}

	return ret;
}

/* Same as above, but dump screen return value to keep compatible with the X
 * function. */
int FScreenParseGeometry(
	char *parsestring, int *x_return, int *y_return,
	unsigned int *width_return, unsigned int *height_return)
{
	struct monitor	*m = NULL;
	int		 rc, x, y, w, h;

	x = 0;
	y = 0;
	w = monitor_get_all_widths();
	h = monitor_get_all_heights();

	{
		char *scr;

		rc = FScreenParseGeometryWithScreen(
			parsestring, x_return, y_return, width_return,
			height_return, &scr);

		if ((m = monitor_resolve_name(scr)) == NULL) {
			/* If the monitor we tried to look up is NULL, it will
			 * be because there was no geometry string specifying
			 * the monitor name.
			 *
			 * In such cases, we should assume that the user has
			 * requested the geometry to be relevant to the global
			 * screen.
			 */
			m = monitor_get_global();
		}
		x = m->si->x;
		y = m->si->y;
		w = m->si->w;
		h = m->si->h;
	}

	/* adapt geometry to selected screen */
	if (rc & XValue)
	{
		if (rc & XNegative)
			*x_return -= (monitor_get_all_widths() - w - x);
		else
			*x_return += x;
	}
	if (rc & YValue)
	{
		if (rc & YNegative)
			*y_return -= (monitor_get_all_heights() - h - y);
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
	int		 ret;
	int		 saved;
	int		 x, y;
	unsigned int	 w = 0, h = 0;
	int		 grav, x_grav, y_grav;
	int		 scr_x, scr_y;
	int		 scr_w, scr_h;

	/* I. Do the parsing and strip off extra bits */
	{
		char *scr;

		ret = FScreenParseGeometryWithScreen(
			parsestring, &x, &y, &w, &h, &scr);
		saved = ret & (XNegative | YNegative);
		ret  &= flags;

		arg.mouse_ev = NULL;
		arg.name = scr;
		FScreenGetScrRect(
			&arg, FSCREEN_BY_NAME, &scr_x, &scr_y, &scr_w, &scr_h);
	}

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
