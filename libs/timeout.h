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

#ifndef TIMEOUT_H
#define TIMEOUT_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

#define TIMEOUT_MAX_TIMEOUTS 32

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

typedef int timeout_time_t;
typedef unsigned int timeout_mask_t;
typedef struct
{
	int n_timeouts;
	timeout_time_t *timeouts;
} timeout_t;

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

timeout_t *timeout_create(
	int n_timeouts);
void timeout_destroy(
	timeout_t *to);
timeout_mask_t timeout_tick(
	timeout_t *to, timeout_time_t n_ticks);
void timeout_rewind(
	timeout_t *to, timeout_mask_t mask, timeout_time_t ticks_before_alarm);

#endif /* TIMEOUT_H */
