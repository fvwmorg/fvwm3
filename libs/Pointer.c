/* Copyright (C) 1999  Dominik Vogt
 *
 * This program is free software; you can redistribute it and/or modify
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

#include "config.h"

#include <X11/Xlib.h>
#include <stdio.h>

/*
 * This function determines the location of the mouse pointer from the event
 * if possible, if not it queries the X server. Returns False if it had to
 * query the server and the call failed.
 */
Bool GetLocationFromEventOrQuery(
	Display *dpy, Window w, XEvent *eventp, int *ret_x, int *ret_y)
{
	Window JunkW;
	int JunkC;
	unsigned int JunkM;
	Bool rc;

	switch (eventp->type)
	{
	case ButtonPress:
	case ButtonRelease:
		*ret_x = eventp->xbutton.x_root;
		*ret_y = eventp->xbutton.y_root;
		return True;
	case KeyPress:
	case KeyRelease:
		*ret_x = eventp->xkey.x_root;
		*ret_y = eventp->xkey.y_root;
		return True;
	case MotionNotify:
		*ret_x = eventp->xmotion.x_root;
		*ret_y = eventp->xmotion.y_root;
		return True;
	default:
		rc = XQueryPointer(
			dpy, w, &JunkW, &JunkW, ret_x, ret_y, &JunkC, &JunkC,
			&JunkM);
		if (rc == False)
		{
			/* pointer is on a different screen */
			*ret_x = 0;
			*ret_y = 0;
		}
		return rc;
	} /* switch */
}
