/* -*-c-*- */
/*
 * Fvwmbuttons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 */

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

/*
 *
 * Derived from fvwm icon code
 *
 */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "FvwmButtons.h"
#include "libs/PictureGraphics.h"
#include "libs/Colorset.h"
#include "libs/Rectangles.h"


/*
 *
 * Combines icon shape masks after a resize
 *
 */
Bool GetIconPosition(button_info *b,
	FvwmPicture *pic, int *r_x, int *r_y, int *r_w, int *r_h)
{
#ifdef NO_ICONS
	return False;
#else
	int x, y, width, height;
	int xoff, yoff;
	int framew, xpad, ypad;
	FlocaleFont *Ffont;
	int BW,BH;
	Bool has_title = (buttonTitle(b) != NULL ? True : False);

	buttonInfo(b, &x, &y, &xpad, &ypad, &framew);
	framew = abs(framew);
	Ffont = buttonFont(b);

	width = pic->width;
	height = pic->height;
	BW = buttonWidth(b);
	BH = buttonHeight(b);

	width = min(width, BW - 2 * (xpad + framew));

	if (has_title == True && Ffont && !(buttonJustify(b) & b_Horizontal))
	{
		height = min(height, BH - 2 * (ypad + framew)
			- Ffont->ascent - Ffont->descent);
	}
	else
	{
		height = min(height,BH-2*(ypad+framew));
	}

	if (b->flags.b_Right)
	{
		xoff = BW-framew - xpad-width;
	}
	else if (b->flags.b_Left)
	{
		xoff = framew + xpad;
	}
	else
	{
		if (buttonJustify(b) & b_Horizontal)
		{
			xoff = 0;
		}
		else
		{
			xoff = (BW - width) >> 1;
		}
		if (xoff < framew + xpad)
		{
			xoff = framew + xpad;
		}
	}

	if (has_title == True && Ffont && !(buttonJustify(b) & b_Horizontal))
	{
		yoff = (BH - (height + Ffont->height)) >> 1;
	}
	else
	{
		yoff = (BH - height) >> 1;
	}

	if (yoff < framew + ypad)
	{
		yoff = framew + ypad;
	}

	x += xoff;
	y += yoff;

	*r_x = x;
	*r_y = y;
	*r_w = width;
	*r_h = height;

	return True;
#endif
}

void DrawForegroundIcon(button_info *b, XEvent *pev)
{
#ifndef NO_ICONS
	int x, y, w, h;
	int cset;
	XRectangle clip;
	FvwmRenderAttributes fra;
	FvwmPicture *pic = buttonIcon(b);

	if (!GetIconPosition(b, pic, &x, &y, &w, &h))
	{
		return;
	}

	if (w < 1 || h < 1)
	{
		return; /* No need drawing to this */
	}

	clip.x = x;
	clip.y = y;
	clip.width = w;
	clip.height = h;

	if (pev)
	{
		if (!frect_get_intersection(
			x, y, w, h,
			pev->xexpose.x, pev->xexpose.y,
			pev->xexpose.width, pev->xexpose.height,
			&clip))
		{
			return;
		}
	}

	if (0 && !pev)
	{
		XClearArea(
			Dpy, MyWindow, clip.x, clip.y, clip.width, clip.height,
			False);
	}

	cset = buttonColorset(b);
	fra.mask = FRAM_DEST_IS_A_WINDOW;
	if (cset >= 0)
	{
		fra.mask |= FRAM_HAVE_ICON_CSET;
		fra.colorset = &Colorset[cset];
	}

	PGraphicsRenderPicture(
		Dpy, MyWindow, pic, &fra, MyWindow,
		NormalGC, None, None,
		clip.x - x, clip.y - y, clip.width, clip.height,
		clip.x, clip.y, clip.width, clip.height, False);
#endif
}
