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

#include "fvwmlib.h"
#include "FScreen.h"
#include "FShape.h"
#include "Fsvg.h"
#include "FRenderInit.h"
#include "Graphics.h"
#include "PictureBase.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

void flib_init_graphics(Display *dpy)
{
	PictureInitCMap(dpy);
	FScreenInit(dpy);
	/* Initialise default colorset */
	AllocColorset(0);
	FShapeInit(dpy);
	FRenderInit(dpy);
#ifdef Frsvg_init
	Frsvg_init();
#endif

	return;
}
