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

/* This module was all new
 * by Rob Nation
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 *
 * The highlight and shadow logic is now in libs/ColorUtils.c.
 * Its completely new.
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "libs/fvwmlib.h"
#include "libs/PictureBase.h"
#include "libs/PictureUtils.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"

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

/* Free an array of colours (n colours), never free black */
void FreeColors(Pixel *pixels, int n, Bool no_limit)
{
	int i;

	/* We don't ever free black - dirty hack to allow freeing colours at
	 * all */
	/* olicha: ???? */
	for (i = 0; i < n; i++)
	{
		if (pixels[i] != 0)
		PictureFreeColors(dpy, Pcmap, pixels + i, 1, 0, no_limit);
	}

	return;
}

/* Copy one color and reallocate it */
void CopyColor(Pixel *dst_color, Pixel *src_color, Bool do_free_dest,
	       Bool do_copy_src)
{
	if (do_free_dest)
	{
		FreeColors(dst_color, 1, True);
	}
	if (do_copy_src)
	{
		*dst_color = fvwmlib_clone_color(*src_color);
	}
}
