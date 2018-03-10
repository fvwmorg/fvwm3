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

#include <X11/keysym.h>

#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "misc.h"
#include "screen.h"
#include "menudim.h"
#include "menuroot.h"
#include "menuparameters.h"
#include "menugeometry.h"

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

Bool menu_get_geometry(
	struct MenuRoot *mr, Window *root_return, int *x_return, int *y_return,
	int *width_return, int *height_return, int *border_width_return,
	int *depth_return)
{
	Status rc;
	Bool brc;
	int root_x;
	int root_y;

	rc = XGetGeometry(
		dpy, MR_WINDOW(mr), root_return, x_return, y_return,
		(unsigned int*)width_return, (unsigned int*)height_return,
		(unsigned int*)border_width_return,
		(unsigned int*)depth_return);
	if (rc == 0)
	{
		return False;
	}
	if (!MR_IS_TEAR_OFF_MENU(mr))
	{
		return True;
	}
	brc = XTranslateCoordinates(
		dpy, MR_WINDOW(mr), Scr.Root, *x_return, *y_return, &root_x,
		&root_y, &JunkChild);
	if (brc == True)
	{
		*x_return = root_x;
		*y_return = root_y;
	}
	else
	{
		*x_return = 0;
		*y_return = 0;
	}

	return brc;
}

Bool menu_get_outer_geometry(
	struct MenuRoot *mr, struct MenuParameters *pmp, Window *root_return,
	int *x_return, int *y_return, int *width_return, int *height_return,
	int *border_width_return, int *depth_return)
{
	if (MR_IS_TEAR_OFF_MENU(mr))
	{
		return XGetGeometry(
			dpy,
			FW_W_FRAME(pmp->tear_off_root_menu_window),
			root_return,x_return,y_return,
			(unsigned int*)width_return,
			(unsigned int*)height_return,
			(unsigned int*)border_width_return,
			(unsigned int*)depth_return);
	}
	else
	{
	  	return menu_get_geometry(
			mr,root_return,x_return,y_return,
			width_return, height_return, border_width_return,
			depth_return);
	}
}
