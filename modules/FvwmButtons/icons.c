/* -*-c-*- */
/*
 * Fvwmbuttons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <stdlib.h>

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
 * Creates an Icon Window
 *
 */
void CreateIconWindow(button_info *b)
{
#ifndef NO_ICONS
	unsigned long valuemask;             /* mask for create windows */
	XSetWindowAttributes attributes;     /* attributes for create windows */
	Pixel bc,fc;
	int cset;
	FvwmRenderAttributes fra;
	Pixmap temp;

	if(!(b->flags&b_Icon))
	{
		return;
	}

	if(b->IconWin != None)
	{
		fprintf(stderr,"%s: BUG: Icon window already created "
			"for 0x%lx!\n", MyName,(unsigned long)b);
		return;
	}
	if(b->icon->width<1 || b->icon->height<1)
	{
		fprintf(stderr,"%s: BUG: Illegal iconwindow "
			"tried created\n",MyName);
		exit(2);
	}

	cset = buttonColorset(b);
	if (b->icon->alpha != None ||
	    (cset >= 0 && Colorset[cset].icon_alpha_percent < 100 &&
	     !(UberButton->c->flags&b_TransBack)))
	{
		/* in this case we drawn on the button, with a shaped
		 * Buttons we do not load the alpha channel */
		b->flags |= b_IconAlpha;
		return;
	}

	cset = buttonColorset(b);
	fra.mask = FRAM_DEST_IS_A_WINDOW;
	if (cset >= 0)
	{
		bc = Colorset[cset].bg;
		fc = Colorset[cset].fg;
		fra.mask |= FRAM_HAVE_ICON_CSET;
		fra.colorset = &Colorset[cset];
		if (Colorset[cset].icon_alpha_percent < 100)
		{
			fra.added_alpha_percent = 100;
			fra.mask |= FRAM_HAVE_ADDED_ALPHA;
		}
	}
	else
	{
		bc = buttonBack(b);
		fc = buttonFore(b);
		fra.mask = 0;
	}

	valuemask = CWColormap | CWBorderPixel | CWBackPixel |
		CWEventMask | CWBackPixmap;
	attributes.colormap = Pcmap;
	attributes.border_pixel = 0;
	attributes.background_pixel = bc;
	attributes.background_pixmap = None;
	attributes.event_mask = ExposureMask;

	b->IconWin=XCreateWindow(
		Dpy, MyWindow, 0, 0, b->icon->width, b->icon->height, 0,
		Pdepth, InputOutput, Pvisual, valuemask, &attributes);
	if (attributes.background_pixel != None)
	{
		XSetWindowBackground(
			Dpy, b->IconWin, attributes.background_pixel);
	}

	if (FShapesSupported)
	{
		if (b->icon->mask!=None)
		{
			FShapeCombineMask(Dpy, b->IconWin, FShapeBounding,
					  0, 0, b->icon->mask, FShapeSet);
		}
	}

	if(b->icon->depth != Pdepth)
	{
		/* bitmap icon */
		XGCValues gcv;

		gcv.background= bc;
		gcv.foreground= fc;
		XChangeGC(Dpy,NormalGC,GCForeground | GCBackground,&gcv);

		if (FShapesSupported)
		{
			FShapeCombineMask(Dpy, b->IconWin, FShapeBounding,
					  0, 0, b->icon->picture, FShapeSet);
		}
	}

	if (cset >= 0 && Colorset[cset].icon_tint_percent > 0)
	{
		temp = XCreatePixmap(
			Dpy, MyWindow, b->icon->width, b->icon->height, Pdepth);
		PGraphicsRenderPicture(
			Dpy, MyWindow, b->icon, &fra, temp,
			NormalGC, None, None,
			0, 0, b->icon->width, b->icon->height,
			0, 0, 0, 0, False);
		XSetWindowBackgroundPixmap(Dpy, b->IconWin, temp);
		XFreePixmap(Dpy,temp);
	}
	else
	{
		/* pixmap icon */
		XSetWindowBackgroundPixmap(Dpy, b->IconWin, b->icon->picture);
	}

	return;
#endif
}

void DestroyIconWindow(button_info *b)
{
#ifndef NO_ICONS
	if(!(b->flags&b_Icon) || (b->flags&b_IconAlpha))
	{
		b->flags &= ~b_IconAlpha;
		return;
	}
	XDestroyWindow(Dpy, b->IconWin);
	b->IconWin = None;
#endif
}

/*
 *
 * Combines icon shape masks after a resize
 *
 */
Bool GetIconWindowPosition(
	button_info *b, int *r_x, int *r_y, int *r_w, int *r_h)
{
#ifdef NO_ICONS
	return 0;
#else
	int x,y,w,h;
	int xoff,yoff;
	int framew,xpad,ypad;
	FlocaleFont *Ffont;
	int BW,BH;

	if(!b || !(b->flags&b_Icon))
		return 0;

	if(!b->IconWin && !(b->flags&b_IconAlpha))
	{
		fprintf(stderr,"%s: DEBUG: Tried to configure erroneous "
			"iconwindow\n", MyName);
		exit(2);
	}

	buttonInfo(b,&x,&y,&xpad,&ypad,&framew);
	framew=abs(framew);
	Ffont = buttonFont(b);

	w = b->icon->width;
	h = b->icon->height;
	BW = buttonWidth(b);
	BH = buttonHeight(b);

	w=min(w,BW-2*(xpad+framew));

	if(b->flags&b_Title && Ffont && !(buttonJustify(b)&b_Horizontal))
		h = min(h,BH-2*(ypad+framew)-Ffont->ascent-Ffont->descent);
	else
		h = min(h,BH-2*(ypad+framew));

	if (b->flags & b_Right)
		xoff = BW-framew-xpad-w;
	else if (b->flags & b_Left)
		xoff = framew+xpad;
	else
	{
		if(buttonJustify(b)&b_Horizontal)
			xoff=0;
		else
			xoff=(BW-w)>>1;
		if(xoff < framew+xpad)
			xoff = framew+xpad;
	}

	if(b->flags&b_Title && Ffont && !(buttonJustify(b)&b_Horizontal))
		yoff=(BH-(h+Ffont->height))>>1;
	else
		yoff=(BH-h)>>1;

	if(yoff < framew+ypad)
		yoff = framew+ypad;

	x += xoff;
	y += yoff;

	*r_x = x;
	*r_y = y;
	*r_w = w;
	*r_h = h;

	return 1;
#endif
}

void DrawForegroundIcon(button_info *b, XEvent *pev)
{
#ifndef NO_ICONS
	int x,y,w,h;
	int cset;
	XRectangle clip;
	FvwmRenderAttributes fra;

	if (!GetIconWindowPosition(b,&x,&y,&w,&h))
	{
		return;
	}

	if(w < 1 || h < 1)
	{
		return; /* No need drawing to this */
	}

	if (!(b->flags & b_IconAlpha))
	{
		return;
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
		Dpy, MyWindow, b->icon, &fra, MyWindow,
		NormalGC, None, None,
		clip.x - x, clip.y - y, clip.width, clip.height,
		clip.x, clip.y, clip.width, clip.height, False);
#endif
}

void ConfigureIconWindow(button_info *b, XEvent *pev)
{
#ifndef NO_ICONS
	int x,y,w,h;

	if (!b->IconWin)
	{
		return;
	}
	if (!GetIconWindowPosition(b,&x,&y,&w,&h))
	{
		return;
	}
	if (!b->IconWin)
	{
		return;
	}

	if(w < 1 || h < 1)
	{
		if (b->IconWin)
			XMoveResizeWindow(Dpy, b->IconWin, 2000,2000,1,1);
		return; /* No need drawing to this */
	}

	if (!pev && b->IconWin)
	{
		XMoveResizeWindow(Dpy, b->IconWin, x,y,w,h);
	}

	return;

	if (!(b->flags & b_IconAlpha))
	{
		return;
	}

#endif
}
