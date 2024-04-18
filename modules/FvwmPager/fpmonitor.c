/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.         See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"
#include "libs/FScreen.h"
#include "libs/fvwmlib.h"
#include "libs/Module.h"
#include "libs/Strings.h"
#include "fvwm/fvwm.h"
#include "FvwmPager.h"

struct fpmonitor *fpmonitor_new(struct monitor *m)
{
	struct fpmonitor	*fp;

	fp = fxcalloc(1, sizeof(*fp));
	if (HilightDesks)
		fp->CPagerWin = fxcalloc(1, ndesks * sizeof(*fp->CPagerWin));
	else
		fp->CPagerWin = NULL;
	fp->m = m;
	fp->disabled = false;
	TAILQ_INSERT_TAIL(&fp_monitor_q, fp, entry);

	return (fp);
}

struct fpmonitor *fpmonitor_this(struct monitor *m_find)
{
	struct monitor *m;
	struct fpmonitor *fp = NULL;

	if (m_find != NULL) {
		/* We've been asked to find a specific monitor. */
		m = m_find;
	} else if (monitor_to_track != NULL) {
		return (monitor_to_track);
	} else {
		m = monitor_get_current();
	}

	if (m == NULL || m->flags & MONITOR_DISABLED)
		return (NULL);
	TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
		if (fp->m == m)
			return (fp);
	}
	return (NULL);
}

struct fpmonitor *fpmonitor_by_name(const char *name)
{
	struct fpmonitor *fm;

	if (name == NULL || StrEquals(name, "none"))
		return (NULL);

	TAILQ_FOREACH(fm, &fp_monitor_q, entry) {
		if (fm->m != NULL && (strcmp(name, fm->m->si->name) == 0))
			return (fm);
	}
	return (NULL);
}

struct fpmonitor *fpmonitor_from_desk(int desk)
{
	struct fpmonitor *fp;

	TAILQ_FOREACH(fp, &fp_monitor_q, entry)
		if (!fp->disabled && fp->m->virtual_scr.CurrentDesk == desk)
			return fp;

	return NULL;
}

struct fpmonitor *fpmonitor_from_xy(int x, int y)
{
	struct fpmonitor *fp;

	if (monitor_to_track == NULL && monitor_mode != MONITOR_TRACKING_G) {
		x %= fpmonitor_get_all_widths();
		y %= fpmonitor_get_all_heights();

		TAILQ_FOREACH(fp, &fp_monitor_q, entry)
			if (x >= fp->m->si->x &&
			    x < fp->m->si->x + fp->m->si->w &&
			    y >= fp->m->si->y &&
			    y < fp->m->si->y + fp->m->si->h)
				return fp;
	} else {
		return fpmonitor_this(NULL);
	}

	return NULL;
}

struct fpmonitor *fpmonitor_from_n(int n)
{
	struct fpmonitor *fp;
	int c = 0;

	TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
		if (fp->disabled)
			continue;
		if (c == n)
			return fp;
		c++;
	}
	return fp;
}

void fpmonitor_disable(struct fpmonitor *fp)
{
	int i;

	fp->disabled = true;
	for (i = 0; i < ndesks; i++) {
		XMoveWindow(dpy, fp->CPagerWin[i], -32768,-32768);
	}

	if (fp == monitor_to_track)
		monitor_to_track = fpmonitor_this(NULL);
}

int fpmonitor_get_all_widths(void)
{
	struct fpmonitor *fp = TAILQ_FIRST(&fp_monitor_q);

	if (fp->scr_width <= 0)
		return (monitor_get_all_widths());

	return (fp->scr_width);
}

int fpmonitor_get_all_heights(void)
{
	struct fpmonitor *fp = TAILQ_FIRST(&fp_monitor_q);

	if (fp->scr_height <= 0)
		return (monitor_get_all_heights());
	return (fp->scr_height);
}

int fpmonitor_count(void)
{
	struct fpmonitor *fp;
	int c = 0;

	TAILQ_FOREACH(fp, &fp_monitor_q, entry)
		if (!fp->disabled)
			c++;
	return (c);
}
