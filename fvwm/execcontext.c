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

#define FEVENT_PRIVILEGED_ACCESS
#include "config.h"
#include <stdio.h>

#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "libs/FEvent.h"
#include "libs/log.h"

#undef PEVENT_PRIVILEGED_ACCESS

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

#define DEBUG_EXECCONTEXT 0
static exec_context_t *x[256];
static int nx = 0;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static void _exc_change_context(
	exec_context_t *exc, exec_context_changes_t *ecc,
	exec_context_change_mask_t mask)
{
	if (mask & ECC_TYPE)
	{
		exc->type = ecc->type;
	}
	if (mask & ECC_ETRIGGER)
	{
		if (ecc->x.etrigger == NULL)
		{
			fev_copy_last_event(&exc->private_data.te);
		}
		else
		{
			exc->private_data.te = *ecc->x.etrigger;
		}
		exc->x.etrigger = &(exc->private_data.te);
	}
	if (mask & ECC_FW)
	{
		exc->w.fw = ecc->w.fw;
	}
	if (mask & ECC_W)
	{
		exc->w.w = ecc->w.w;
	}
	if (mask & ECC_WCONTEXT)
	{
		exc->w.wcontext = ecc->w.wcontext;
	}
	if (mask & ECC_MODULE)
	{
		exc->m.module = ecc->m.module;
	}

	return;
}

/* ---------------------------- interface functions ------------------------ */

const exec_context_t *exc_create_null_context(void)
{
	exec_context_t *exc;

	exc = fxcalloc(1, sizeof(exec_context_t));
	if (DEBUG_EXECCONTEXT)
	{
		int i;

		fvwm_debug(__func__, "xxx+0 ");
		for (i=0; i < nx; i++)
		{
			fvwm_debug(__func__, "  ");
		}
		x[nx] = exc;
		nx++;
		fvwm_debug(__func__, "%p\n", exc);
	}
	exc->type = EXCT_NULL;
	fev_make_null_event(&exc->private_data.te, dpy);
	exc->x.etrigger = &exc->private_data.te;
	exc->x.elast = fev_get_last_event_address();
	exc->m.module = NULL;

	return exc;
}

const exec_context_t *exc_create_context(
	exec_context_changes_t *ecc, exec_context_change_mask_t mask)
{
	exec_context_t *exc;

	if (DEBUG_EXECCONTEXT)
	{
		if (!(mask & ECC_TYPE))
		{
			abort();
		}
	}
	exc = (exec_context_t *)exc_create_null_context();
	_exc_change_context(exc, ecc, mask);

	return exc;
}

const exec_context_t *exc_clone_context(
	const exec_context_t *excin, exec_context_changes_t *ecc,
	exec_context_change_mask_t mask)
{
	exec_context_t *exc;

	exc = fxmalloc(sizeof(exec_context_t));
	if (DEBUG_EXECCONTEXT)
	{
		int i;

		fvwm_debug(__func__, "xxx+= ");
		for (i=0; i < nx; i++)
		{
			fvwm_debug(__func__, "  ");
		}
		x[nx] = exc;
		nx++;
		fvwm_debug(__func__, "%p\n", exc);
	}
	memcpy(exc, excin, sizeof(*exc));
	_exc_change_context(exc, ecc, mask);

	return exc;
}

void exc_destroy_context(
	const exec_context_t *exc)
{
	if (DEBUG_EXECCONTEXT)
	{
		int i;

		if (nx == 0||x[nx-1] != exc)abort();
		nx--;
		fvwm_debug(__func__, "xxx-- ");
		for(i=0;i<nx;i++) fvwm_debug(__func__, "  ");
		fvwm_debug(__func__, "%p\n", exc);
	}
	free((exec_context_t *)exc);

	return;
}
