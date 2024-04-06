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

#include "condrc.h"

/* --------------------------- local definitions --------------------------- */

/* --------------------------- local macros -------------------------------- */

/* --------------------------- imports ------------------------------------- */

/* --------------------------- included code files ------------------------- */

/* --------------------------- local types --------------------------------- */

/* --------------------------- forward declarations ------------------------ */

/* --------------------------- local variables ----------------------------- */

/* --------------------------- exported variables (globals) ---------------- */

/* --------------------------- local functions ----------------------------- */

/* --------------------------- interface functions ------------------------- */

void
condrc_init(cond_rc_t *cond_rc)
{
	memset(cond_rc, 0, sizeof(*cond_rc));
	cond_rc->rc = COND_RC_OK;

	return;
}
