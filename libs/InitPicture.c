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

/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/
/*
  Changed 02/12/97 by Dan Espen:
  - added routines to determine color closeness, for color use reduction.
  Some of the logic comes from pixy2, so the copyright is below.
*/
/*
 * Copyright 1996, Romano Giannetti. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
 * Romano Giannetti - Dipartimento di Ingegneria dell'Informazione
 *                    via Diotisalvi, 2  PISA
 * mailto:romano@iet.unipi.it
 * http://www.iet.unipi.it/~romano
 *
 */

/****************************************************************************
 *
 * Routines to handle initialization, loading, and removing of xpm's or mono-
 * icon images.
 *
 ****************************************************************************/

#include "config.h"

#include <stdio.h>

#include <X11/Xlib.h>

#include <fvwmlib.h>

Bool Pdefault;
Visual *Pvisual;
static Visual *FvwmVisual;
Colormap Pcmap;
static Colormap FvwmCmap;
unsigned int Pdepth;
static unsigned int FvwmDepth;
Display *Pdpy;            /* Save area for display pointer */

void InitPictureCMap(Display *dpy) {
	char *envp;

	Pdpy = dpy;
	/* if fvwm has not set this env-var it is using the default visual */
	envp = getenv("FVWM_VISUALID");
	if ((envp != NULL) && (strlen(envp) > 0)) {
		/* convert the env-vars to a visual and colormap */
		int viscount;
		XVisualInfo vizinfo, *xvi;

		sscanf(envp, "%lx", &vizinfo.visualid);
		xvi = XGetVisualInfo(dpy, VisualIDMask, &vizinfo, &viscount);
		Pvisual = xvi->visual;
		Pdepth = xvi->depth;
		sscanf(getenv("FVWM_COLORMAP"), "%lx", &Pcmap);
		Pdefault = False;
	} else {
		int screen = DefaultScreen(dpy);

		Pvisual = DefaultVisual(dpy, screen);
		Pdepth = DefaultDepth(dpy, screen);
		Pcmap = DefaultColormap(dpy, screen);
		Pdefault = True;
	}
	FvwmVisual = Pvisual;
	FvwmDepth = Pdepth;
	FvwmCmap = Pcmap;

	return;
}

void UseDefaultVisual(void)
{
	int screen = DefaultScreen(Pdpy);

	Pvisual = DefaultVisual(Pdpy, screen);
	Pdepth = DefaultDepth(Pdpy, screen);
	Pcmap = DefaultColormap(Pdpy, screen);

	return;
}

void UseFvwmVisual(void)
{
	Pvisual = FvwmVisual;
	Pdepth = FvwmDepth;
	Pcmap = FvwmCmap;

	return;
}

void SaveFvwmVisual(void)
{
	FvwmVisual = Pvisual;
	FvwmDepth = Pdepth;
	FvwmCmap = Pcmap;

	return;
}

static char* imagePath = FVWM_IMAGEPATH;

void SetImagePath( const char* newpath )
{
	static int need_to_free = 0;
	setPath( &imagePath, newpath, need_to_free );
	need_to_free = 1;

	return;
}

char* GetImagePath()
{
	return imagePath;
}

/****************************************************************************
 *
 * Find the specified image file somewhere along the given path.
 *
 * There is a possible race condition here:  We check the file and later
 * do something with it.  By then, the file might not be accessible.
 * Oh well.
 *
 ****************************************************************************/
char* findImageFile( const char* icon, const char* pathlist, int type )
{
	if ( pathlist == NULL )
	{
		pathlist = imagePath;
	}

	return searchPath( pathlist, icon, ".gz", type );
}

