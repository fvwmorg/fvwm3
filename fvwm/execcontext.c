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

/* ---------------------------- included header files ---------------------- */

#define FEVENT_PRIVILEGED_ACCESS
#include "config.h"
#undef FEVENT_PRIVILEGED_ACCESS
#include <stdio.h>

#include "libs/safemalloc.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

#undef DEBUG_EXECCONTEXT
#ifdef DEBUG_EXECCONTEXT
static exec_context_t *x[256];
static int nx = 0;
#endif

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

static void __exc_change_context(
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
#ifdef DEBUG_EXECCONTEXT
	int i;
#endif

	exc = (exec_context_t *)safecalloc(1, sizeof(exec_context_t));
#ifdef DEBUG_EXECCONTEXT
fprintf(stderr, "xxx+0 ");
for(i=0;i<nx;i++)fprintf(stderr,"  ");
x[nx]=exc;nx++;
fprintf(stderr, "0x%08x\n", (int)exc);
#endif
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

#ifdef DEBUG_EXECCONTEXT
if (!(mask & ECC_TYPE)) abort();
#endif
	exc = (exec_context_t *)exc_create_null_context();
	__exc_change_context(exc, ecc, mask);

	return exc;
}

const exec_context_t *exc_clone_context(
	const exec_context_t *excin, exec_context_changes_t *ecc,
	exec_context_change_mask_t mask)
{
	exec_context_t *exc;
#ifdef DEBUG_EXECCONTEXT
int i;
#endif

	exc = (exec_context_t *)safemalloc(sizeof(exec_context_t));
#ifdef DEBUG_EXECCONTEXT
fprintf(stderr, "xxx+= ");
for(i=0;i<nx;i++)fprintf(stderr,"  ");
x[nx]=exc;nx++;
fprintf(stderr, "0x%08x\n", (int)exc);
#endif
	memcpy(exc, excin, sizeof(*exc));
	__exc_change_context(exc, ecc, mask);

	return exc;
}

void exc_destroy_context(
	const exec_context_t *exc)
{
#ifdef DEBUG_EXECCONTEXT
int i;
if (nx == 0||x[nx-1] != exc)abort();
nx--;
fprintf(stderr, "xxx-- ");
for(i=0;i<nx;i++)fprintf(stderr,"  ");
fprintf(stderr, "0x%08x\n", (int)exc);
#endif
	free((exec_context_t *)exc);

	return;
}
