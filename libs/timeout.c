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

#include "safemalloc.h"
#include "timeout.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

timeout_t *timeout_create(
	int n_timeouts)
{
	timeout_t *to;

	if (n_timeouts < 0 || n_timeouts > TIMEOUT_MAX_TIMEOUTS)
	{
		return NULL;
	}
	to = xcalloc(1, sizeof(timeout_t));
	to->n_timeouts = n_timeouts;
	to->timeouts = xcalloc(1, n_timeouts * sizeof(timeout_time_t));

	return to;
}

void timeout_destroy(
	timeout_t *to)
{
	if (to == NULL)
	{
		return;
	}
	if (to->timeouts != NULL)
	{
		free(to->timeouts);
	}
	free(to);

	return;
}

timeout_mask_t timeout_tick(
	timeout_t *to, timeout_time_t n_ticks)
{
	timeout_mask_t mask;
	int i;

	if (n_ticks <= 0)
	{
		return 0;
	}
	for (i = 0, mask = 0; i < to->n_timeouts; i++)
	{
		if (to->timeouts[i] > n_ticks)
		{
			to->timeouts[i] -= n_ticks;
		}
		else if (to->timeouts[i] > 0)
		{
			to->timeouts[i] = 0;
			mask |= 1 << i;
		}
	}

	return mask;
}

void timeout_rewind(
	timeout_t *to, timeout_mask_t mask, timeout_time_t ticks_before_alarm)
{
	int i;

	if (ticks_before_alarm < 0)
	{
		return;
	}
	for (i = 0; i < to->n_timeouts; i++)
	{
		if (mask & (1 << i))
		{
			to->timeouts[i] = ticks_before_alarm;
		}
	}

	return;
}
