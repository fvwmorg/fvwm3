/***********************************************************************
 * icons.c
 *      Based on icons.c of GoodStuff:
 *              Copyright 1993, Robert Nation.
 ***********************************************************************/

/* Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

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

/***********************************************************************
 *
 * Derived from fvwm icon code
 *
 ***********************************************************************/

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

#include "libs/Module.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/Picture.h"
#include "fvwm/fvwm.h"
#include "FvwmIconBox.h"

extern int save_color_limit;
extern Bool do_allow_bad_access;
extern Bool was_bad_access;
extern char *MyName;
extern int Iconcolorset;

#define ICON_EVENTS (ExposureMask |\
ButtonReleaseMask | ButtonPressMask | EnterWindowMask | LeaveWindowMask)

/****************************************************************************
 *
 * Creates an Icon Window
 * Loads an icon file and combines icon shape masks after a resize
 * special thanks to Rich Neitzel <thor@thor.atd.ucar.edu>
 *    for his patch to handle icon windows
 *
 ****************************************************************************/
void CreateIconWindow(struct icon_info *item)
{
	unsigned long valuemask;              /* mask for create windows */
	XSetWindowAttributes attributes;      /* attributes for create windows */

	attributes.background_pixel = icon_back_pix;
	attributes.border_pixel = 0;
	attributes.colormap = Pcmap;
	attributes.event_mask = ExposureMask;
	valuemask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	/* ccreate the icon label */
	item->IconWin = XCreateWindow(dpy, icon_win, 0, 0, max_icon_width,
				      max_icon_height + 10, 0, CopyFromParent,
				      CopyFromParent, CopyFromParent, valuemask,
				      &attributes);
	XSelectInput(dpy, item->IconWin, ICON_EVENTS);

	XSelectInput(dpy, item->id, PropertyChangeMask);
	item->wmhints = XGetWMHints(dpy, item->id);

	if (max_icon_height == 0)
		return;

	/* config specified icons have priority */
	if ((item->icon_file != NULL) && !(item->extra_flags & DEFAULTICON))
	{
		GetIconFromFile(item);
	}
	/* next come program specified icon windows and pixmaps*/
	if((item->icon_h == 0) && (item->icon_w == 0) && item->wmhints)
	{
		if (item->wmhints->flags & IconWindowHint)
			GetIconWindow(item);
		else if (item->wmhints->flags & IconPixmapHint)
			GetIconBitmap(item);
	}

	/* if that all fails get the default */
	if ((item->icon_file != NULL) && (item->icon_h == 0) &&
	    (item->icon_w == 0))
	{
		GetIconFromFile(item);
	}

	if ((item->icon_h == 0) && (item->icon_w == 0))
	{
		return;
	}

	/* create the window to hold the pixmap */
	/* if using a non default visual client pixmaps must have a default
	 * visual window to be drawn into */
	if (IS_ICON_OURS(item))
	{
		if (Pdefault || (item->icon_depth == 1) || IS_PIXMAP_OURS(item))
		{
			item->icon_pixmap_w = XCreateWindow(dpy, icon_win, 0, 0,
					max(max_icon_width, item->icon_w),
					max(max_icon_height, item->icon_h), 0,
					CopyFromParent, CopyFromParent,
					CopyFromParent, valuemask,
					&attributes);
		}
		else
		{
			attributes.background_pixel = 0;
			attributes.colormap = DefaultColormap(dpy, screen);
			item->icon_pixmap_w = XCreateWindow(dpy, icon_win, 0, 0,
					max(max_icon_width,item->icon_w),
					max(max_icon_height,item->icon_h), 0,
					Pdepth,
					InputOutput,
					Pvisual, valuemask,
					&attributes);
		}
		XSelectInput(dpy, item->icon_pixmap_w, ICON_EVENTS);
	}

	if (FShapesSupported && item->icon_maskPixmap != None)
	{
		int hr;
		hr = (Pdefault | (item->icon_depth == 1) |
		      IS_PIXMAP_OURS(item)) ? icon_relief/2 : 0;
		FShapeCombineMask(dpy, item->icon_pixmap_w, FShapeBounding,
				  hr, hr, item->icon_maskPixmap, FShapeSet);
	}

	if (FShapesSupported)
	{
		XSetWindowBackgroundPixmap(
			dpy, item->icon_pixmap_w, ParentRelative);
	}
}

/****************************************************************************
 *
 * Bodge the icon title and picture windows for the 3D shadows
 * Puts them in the correct location
 * This could do with some serious variable renaming
 *
 ****************************************************************************/
void AdjustIconWindow(struct icon_info *item, int n)
{
  int x=0,y=0,w,h,h2,h3,w3;

  w3 = w = max_icon_width + icon_relief;
  h3 = h2 = max_icon_height + icon_relief;
  h = h2 + 6 + Ffont->height;

  switch (primary){
  case LEFT:
  case RIGHT:
    if (secondary == BOTTOM)
      y = icon_win_height  - (n / Lines + 1)*(h + interval);
    else if (secondary == TOP)
      y = (n / Lines)*(h + interval) + interval;

    if (primary == LEFT)
      x = (n % Lines)*(w + interval) + interval;
    else
      x = icon_win_width - (n % Lines + 1)*(w + interval);
    break;
  case TOP:
  case BOTTOM:
    if (secondary == RIGHT)
      x = icon_win_width - (n / Lines + 1)*(w + interval);
    else if (secondary == LEFT)
      x = (n / Lines)*(w + interval) + interval;

    if (primary == TOP)
      y = (n % Lines)*(h + interval) + interval;
    else
      y = icon_win_height - (n % Lines + 1)*(h + interval);
    break;
  default:
    break;
  }

  item->x = x;
  item->y = y;

  if (item->icon_w > 0 && item->icon_h > 0){
    w3 = min(max_icon_width, item->icon_w) + icon_relief;
    h3 = min(max_icon_height, item->icon_h) + icon_relief;
  }
  if (max_icon_height != 0) {
    /* fudge factor for foreign visual icon pixmaps */
    int r = (Pdefault | (item->icon_depth == 1) | IS_PIXMAP_OURS(item))
	    ? 0 : icon_relief;
    XMoveResizeWindow(dpy, item->icon_pixmap_w, x + (w - w3)/2 + r/2,
		    y + (h2 - h3)/2 + r/2,w3-r,h3-r);
  }
  XMoveResizeWindow(dpy, item->IconWin, x,y + h2,w,h - h2);
}

/****************************************************************************
 *
 * Looks for icon from a file
 *
 ****************************************************************************/
void GetIconFromFile(struct icon_info *item)
{
	char *path = PictureFindImageFile(item->icon_file, imagePath, R_OK);
	FvwmPictureAttributes fpa;

	if (NULL == path)
	{
		fprintf(stderr, "[FvwmIconBox] cannot find %s on ImagePath %s\n",
			item->icon_file, imagePath);
		return;
	}
	fpa.mask = FPAM_NO_ALLOC_PIXELS; /* olicha why ? */
	if (Iconcolorset >= 0 && Colorset[Iconcolorset].do_dither_icon)
	{
		fpa.mask |= FPAM_DITHER;
	}
	item->icon_w = 0;
	item->icon_h = 0;
	if (!PImageLoadPixmapFromFile(
		dpy, Root, path, &item->iconPixmap,
		&item->icon_maskPixmap,
		&item->icon_alphaPixmap, &item->icon_w, &item->icon_h,
		&item->icon_depth, 0, NULL, fpa))
	{
		fprintf(stderr, "[FvwmIconBox] cannot load pixmap from "
			"file '%s'\n",path);
		item->icon_w = 0;
		item->icon_h = 0;
		return;
	}
	item->icon_w = min(max_icon_width, item->icon_w);
	item->icon_h = min(max_icon_height, item->icon_h);
	SET_ICON_OURS(item, True);
	SET_PIXMAP_OURS(item, True);
	free(path);
	return;
}

/****************************************************************************
 *
 * Looks for an application supplied icon window
 *
 *************************************************************************** */
void GetIconWindow(struct icon_info *item)
{
	int x, y;
	unsigned int bw;
	Window Junkroot;

	if(!XGetGeometry(dpy, item->wmhints->icon_window, &Junkroot,
			 &x, &y, (unsigned int *)&item->icon_w,
			 (unsigned int *)&item->icon_h,
			 &bw, (unsigned int *)&item->icon_depth))
	{
#if 0
		fprintf(stderr, "[%s][GetIconWindow] WARN -- '%s' has an "
			"invalid icon window hint\n", MyName, item->name);
#endif
		/* disable the icon window hint */
		item->icon_w = 0;
		item->icon_h = 0;
		item->wmhints->icon_window = None;
		item->wmhints->flags &= ~IconWindowHint;
		return;
	}

	item->icon_pixmap_w = item->wmhints->icon_window;

	if (FShapesSupported && (item->wmhints->flags & IconMaskHint))
	{
		SET_ICON_SHAPED(item, True);
		item->icon_maskPixmap = item->wmhints->icon_mask;
	}

	item->icon_w = min(max_icon_width + icon_relief,  item->icon_w);
	item->icon_h = min(max_icon_height + icon_relief, item->icon_h);

	MyXGrabServer(dpy);
	do_allow_bad_access = True;
	XReparentWindow(dpy, item->icon_pixmap_w, icon_win, 0, 0);
	XSetWindowBorderWidth(dpy, item->icon_pixmap_w, 0);
	XSelectInput(dpy, item->icon_pixmap_w, ICON_EVENTS);
	XSync(dpy, 0);
	do_allow_bad_access = False;
	if (was_bad_access)
	{
		/* give up */
#if 0
		fprintf(stderr, "[%s][GetIconWindow] WARN -- BadAcces when "
			"reparenting %s icon window\n", MyName, item->name);
#endif
		XSelectInput(dpy, item->icon_pixmap_w, NoEventMask);
		XUnmapWindow(dpy, item->icon_pixmap_w);
		XReparentWindow(dpy, item->icon_pixmap_w, Root, 0, 0);
		XUnmapWindow(dpy, item->icon_pixmap_w);
		MyXUngrabServer(dpy);
		item->icon_pixmap_w = item->wmhints->icon_window = None;
		item->wmhints->flags &= ~IconWindowHint;
		item->icon_maskPixmap = None;
		item->icon_w = 0;
		item->icon_h = 0;
		was_bad_access = False;
		return;
	}
	MyXUngrabServer(dpy);
	SET_ICON_OURS(item, False);
	SET_PIXMAP_OURS(item, False);
}

/***************************************************************************
*
 *
 * Looks for an application supplied bitmap or pixmap
 *
 ***************************************************************************
*/
void GetIconBitmap(struct icon_info *item)
{
	int x, y;
	unsigned int bw, depth;
	Window Junkroot;
	GC gc;

	item->icon_file = NULL;
	item->icon_maskPixmap = None;
	item->icon_alphaPixmap = None;
	item->iconPixmap = None;
	item->icon_w = 0;
	item->icon_h = 0;
	if (!XGetGeometry(dpy, item->wmhints->icon_pixmap, &Junkroot, &x, &y,
			  (unsigned int *)&item->icon_w,
			  (unsigned int *)&item->icon_h, &bw, &depth))
	{
		/* disable icon pixmap hint */
		item->icon_w = 0;
		item->icon_h = 0;
		item->wmhints->icon_pixmap = None;
		item->wmhints->flags &= ~IconPixmapHint;
		return;
	}
	/* sanity check the pixmap depth, it must be the same as the root or 1 */
	if (depth != 1 && depth != Pdepth)
	{
		/* disable icon pixmap hint */
		item->icon_w = 0;
		item->icon_h = 0;
		item->wmhints->icon_pixmap = None;
		item->wmhints->flags &= ~IconPixmapHint;
		return;
	}
	item->icon_depth = depth;

	if (FShapesSupported && (item->wmhints->flags & IconMaskHint))
	{
		SET_ICON_SHAPED(item, True);
		item->icon_maskPixmap = item->wmhints->icon_mask;
	}

	item->icon_w = min(max_icon_width, item->icon_w);
	item->icon_h = min(max_icon_height, item->icon_h);
	if (item->icon_w <= 0 || item->icon_h <= 0)
	{
		item->icon_w = 0;
		item->icon_h = 0;
		return;
	}

	item->iconPixmap = XCreatePixmap(dpy, Root,
					 item->icon_w, item->icon_h, depth);
	gc = fvwmlib_XCreateGC(dpy, item->iconPixmap, 0, NULL);
	XCopyArea(dpy, item->wmhints->icon_pixmap, item->iconPixmap,
		  gc, 0, 0, item->icon_w, item->icon_h, 0, 0);
	XFreeGC(dpy, gc);
	SET_PIXMAP_OURS(item, False);
	SET_ICON_OURS(item, True);
}

Bool GetBackPixmap(void)
{
	Pixmap maskPixmap;
	char *path = NULL;
	Pixmap tmp_pixmap;
	int w=0, h=0, icon_depth = 0;
	FvwmPictureAttributes fpa;

	fpa.mask = FPAM_NO_ALLOC_PIXELS | FPAM_NO_ALPHA;
	if (Pdepth <= 8)
	{
		fpa.mask |= FPAM_DITHER;
	}
	if (IconwinPixmapFile == NULL)
		return False;

	path = PictureFindImageFile(IconwinPixmapFile, imagePath, R_OK);
	if (path == NULL)
	{
		return False;
	}
	if (!PImageLoadPixmapFromFile(
		dpy, main_win, path, &tmp_pixmap, &maskPixmap, NULL, &w, &h,
		&icon_depth, 0, NULL, fpa))
	{
		w = 0;
		h = 0;
		fprintf(stderr, "[FvwmIconBox] Problem creating pixmap from "
			"file: %s\n", path);
		free(path);
		return False;
	}
	if (icon_depth == 1)
	{
		IconwinPixmap = XCreatePixmap(dpy, main_win, w, h, Pdepth);
		XCopyPlane(dpy, tmp_pixmap, IconwinPixmap, NormalGC,
			   0, 0, w, h, 0, 0, 1);
		XFreePixmap(dpy, tmp_pixmap);
	}
	else
	{
		IconwinPixmap = tmp_pixmap;
	}
	free(path);
	if (w != 0 && h != 0)
		return True;
	return False;
}
