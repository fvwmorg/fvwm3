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

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "execcontext.h"
#include "eventhandler.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "colormaps.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static Bool client_controls_colormaps = False;
static Colormap last_cmap = None;

/* ---------------------------- exported variables (globals) --------------- */

const FvwmWindow *colormap_win;

/* ---------------------------- local functions ---------------------------- */

/* ---------------------------- interface functions ------------------------ */

void set_client_controls_colormaps(Bool flag)
{
	client_controls_colormaps = flag;

	return;
}

/* HandleColormapNotify - colormap notify event handler
 *
 * This procedure handles both a client changing its own colormap, and
 * a client explicitly installing its colormap itself (only the window
 * manager should do that, so we must set it correctly).
 */
void colormap_handle_colormap_notify(const evh_args_t *ea)
{
	XEvent evdummy;
	XColormapEvent *cevent = (XColormapEvent *)ea->exc->x.etrigger;
	Bool ReInstall = False;
	XWindowAttributes attr;
	FvwmWindow *fw = ea->exc->w.fw;

	if (!fw)
	{
		return;
	}
	if (cevent->new)
	{
		if (XGetWindowAttributes(dpy, FW_W(fw), &attr) != 0)
		{
			fw->attr_backup.colormap = attr.colormap;
			if (fw == colormap_win &&
			    fw->number_cmap_windows == 0)
			{
				last_cmap = attr.colormap;
			}
			ReInstall = True;
		}
	}
	else if ((cevent->state == ColormapUninstalled)&&
		(last_cmap == cevent->colormap))
	{
		/* Some window installed its colormap, change it back */
		ReInstall = True;
	}

	while (FCheckTypedEvent(dpy, ColormapNotify, &evdummy))
	{
		if (XFindContext(
			    dpy, cevent->window, FvwmContext,
			    (caddr_t *) &fw) == XCNOENT)
		{
			fw = NULL;
		}
		if ((fw)&&(cevent->new))
		{
			if (XGetWindowAttributes(dpy, FW_W(fw), &attr) != 0)
			{
				fw->attr_backup.colormap = attr.colormap;
				if (fw == colormap_win &&
				    fw->number_cmap_windows == 0)
				{
					last_cmap = attr.colormap;
				}
				ReInstall = True;
			}
		}
		else if ((fw)&&
			(cevent->state == ColormapUninstalled)&&
			(last_cmap == cevent->colormap))
		{
			/* Some window installed its colormap, change it back */
			ReInstall = True;
		}
		else if ((fw)&&
			(cevent->state == ColormapInstalled)&&
			(last_cmap == cevent->colormap))
		{
			/* The last color map installed was the correct one.
			 * Do not change anything */
			ReInstall = False;
		}
	}

	/* Reinstall the colormap that we think should be installed,
	 * UNLESS and unrecognized window has the focus - it might be
	 * an override-redirect window that has its own colormap. */
	if (ReInstall && Scr.UnknownWinFocused == None &&
	    !client_controls_colormaps)
	{
		XInstallColormap(dpy,last_cmap);
	}

	return;
}

/* Re-Install the active colormap */
void ReInstallActiveColormap(void)
{
	InstallWindowColormaps(colormap_win);

	return;
}

/*  Procedure:
 *      InstallWindowColormaps - install the colormaps for one fvwm window
 *
 *  Inputs:
 *      type    - type of event that caused the installation
 *      tmp     - for a subset of event types, the address of the
 *                window structure, whose colormaps are to be installed.
 */
void InstallWindowColormaps(const FvwmWindow *fw)
{
	int i;
	XWindowAttributes attributes;
	Window w;
	Bool ThisWinInstalled = False;

	/* If no window, then install fvwm colormap */
	if (!fw)
	{
		fw = &Scr.FvwmRoot;
	}

	colormap_win = fw;
	/* Save the colormap to be loaded for when force loading of
	 * root colormap(s) ends.
	 */
	Scr.pushed_window = fw;
	/* Don't load any new colormap if root/fvwm colormap(s) has been
	 * force loaded.
	 */
	if (Scr.root_pushes || Scr.fvwm_pushes || client_controls_colormaps)
	{
		return;
	}

	if (fw->number_cmap_windows > 0)
	{
		for (i=fw->number_cmap_windows -1; i>=0;i--)
		{
			w = fw->cmap_windows[i];
			if (w == FW_W(fw))
			{
				ThisWinInstalled = True;
			}
			if (!XGetWindowAttributes(dpy,w,&attributes))
			{
				attributes.colormap = last_cmap;
			}

			/*
			 * On Sun X servers, don't install 24 bit TrueColor
			 * colourmaps.  Despite what the server says, these
			 * colourmaps are always installed. */
			if (last_cmap != attributes.colormap
#if defined(sun) && defined(TRUECOLOR_ALWAYS_INSTALLED)
			   && !(attributes.depth == 24 &&
				attributes.visual->class == TrueColor)
#endif
				)
			{
				last_cmap = attributes.colormap;
				XInstallColormap(dpy, last_cmap);
			}
		}
	}

	if (!ThisWinInstalled)
	{
		if (last_cmap != fw->attr_backup.colormap
#if defined(sun) && defined(TRUECOLOR_ALWAYS_INSTALLED)
		   && !(fw->attr_backup.depth == 24 &&
			fw->attr_backup.visual->class == TrueColor)
#endif
			)
		{
			last_cmap = fw->attr_backup.colormap;
			XInstallColormap(dpy, last_cmap);
		}
	}

	return;
}


/* Force (un)loads root colormap(s)
 *
 * These matching routines provide a mechanism to insure that
 * the root colormap(s) is installed during operations like
 * rubber banding that require colors from
 * that colormap.  Calls may be nested arbitrarily deeply,
 * as long as there is one UninstallRootColormap call per
 * InstallRootColormap call.
 *
 * {Uni,I}nstall{Root,Fvwm}Colormap calls may be freely intermixed
 */
void InstallRootColormap(void)
{
	if (last_cmap != DefaultColormap(dpy, Scr.screen))
	{
		last_cmap = DefaultColormap(dpy, Scr.screen);
		XInstallColormap(dpy, last_cmap);
	}
	Scr.root_pushes++;

	return;
}

/* Unstacks one layer of root colormap pushing
 * If we peel off the last layer, re-install the application colormap
 * or the fvwm colormap if fvwm has a menu posted
 */
void UninstallRootColormap(void)
{
	if (Scr.root_pushes)
	{
		Scr.root_pushes--;
	}

	if (!Scr.root_pushes)
	{
		if (!Scr.fvwm_pushes)
		{
			InstallWindowColormaps(Scr.pushed_window);
		}
		else if (last_cmap != Pcmap)
		{
			last_cmap = Pcmap;
			XInstallColormap(dpy, last_cmap);
		}
	}

	return;
}

/*  Procedures:
 *      {Uni/I}nstallFvwmColormap - Force (un)loads fvwm colormap(s)
 *      This is used to ensure the fvwm colormap is installed during
 *      menu operations
 */
void InstallFvwmColormap(void)
{
	if (last_cmap != Pcmap)
	{
		last_cmap = Pcmap;
		XInstallColormap(dpy, last_cmap);
	}
	Scr.fvwm_pushes++;

	return;
}

void UninstallFvwmColormap(void)
{
	if (Scr.fvwm_pushes)
	{
		Scr.fvwm_pushes--;
	}

	if (!Scr.fvwm_pushes)
	{
		if (!Scr.root_pushes)
		{
			InstallWindowColormaps(Scr.pushed_window);
		}
		else if (last_cmap != DefaultColormap(dpy, Scr.screen))
		{
			last_cmap = DefaultColormap(dpy, Scr.screen);
			XInstallColormap(dpy, last_cmap);
		}
	}

	return;
}

/* Gets the WM_COLORMAP_WINDOWS property from the window
 *
 * This property typically doesn't exist, but a few applications
 * use it. These seem to occur mostly on SGI machines.
 */
void FetchWmColormapWindows (FvwmWindow *fw)
{
	XWindowAttributes getattribs;
	XSetWindowAttributes setattribs;
	long i;
	unsigned long valuemask;

	if (fw->cmap_windows != (Window *)NULL)
	{
		XFree((void *)fw->cmap_windows);
	}

	if (!XGetWMColormapWindows (dpy, FW_W(fw), &(fw->cmap_windows),
				   &(fw->number_cmap_windows)))
	{
		fw->number_cmap_windows = 0;
		fw->cmap_windows = NULL;
	}
	/* we need to be notified of Enter events into these subwindows
	 * so we can set their colormaps */
	if (fw->number_cmap_windows != 0)
	{
		for (i = 0; i < fw->number_cmap_windows; i++)
		{
			if (XGetWindowAttributes(
				    dpy, fw->cmap_windows[i], &getattribs))
			{
				valuemask = CWEventMask;
				setattribs.event_mask =
					getattribs.your_event_mask |
					EnterWindowMask | LeaveWindowMask;
				XChangeWindowAttributes(
					dpy, fw->cmap_windows[i], valuemask,
					&setattribs);
			}
		}
	}
	return;
}

/* Looks through the window list for any matching COLORMAP_WINDOWS
 *   windows and installs the colormap if one exists.
 */
void EnterSubWindowColormap(Window win)
{
	FvwmWindow         *t;
	long                i;
	XWindowAttributes   attribs;

	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if (t->number_cmap_windows != 0)
		{
			for (i = 0; i < t->number_cmap_windows; i++)
			{
				if (t->cmap_windows[i] == win)
				{
					if (XGetWindowAttributes(
						    dpy,win,&attribs))
					{
						last_cmap = attribs.colormap;
						XInstallColormap(
							dpy, last_cmap);
					}
					return;
				}
			}
		}
	}

	return;
}

void LeaveSubWindowColormap(Window win)
{
	FvwmWindow *t;
	long i;
	int bWinInList, bParentInList;

	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if (t->number_cmap_windows != 0)
		{
			bWinInList = 0;
			bParentInList = 0;
			for (i=0;i<t->number_cmap_windows;i++)
			{
				if (t->cmap_windows[i] == win)
				{
					bWinInList = 1;
				}
				if (t->cmap_windows[i] == FW_W(t))
				{
					bParentInList = 1;
				}
			}
			if (bWinInList)
			{
				if (bParentInList)
				{
					InstallWindowColormaps(t);
				}
				else
				{
					InstallWindowColormaps(NULL);
				}
				return;
			}
		}
	}

	return;
}
