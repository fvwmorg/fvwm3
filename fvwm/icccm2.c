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

#include "config.h"

#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <X11/Xatom.h>

#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "misc.h"
#include "screen.h"
#include "eventmask.h"

Time managing_since;

Atom _XA_WM_SX;
Atom _XA_MANAGER;
Atom _XA_ATOM_PAIR;
Atom _XA_WM_COLORMAP_NOTIFY;
#define _XA_TARGETS   conversion_targets[0]
#define _XA_MULTIPLE  conversion_targets[1]
#define _XA_TIMESTAMP conversion_targets[2]
#define _XA_VERSION   conversion_targets[3]
#define MAX_TARGETS                      4

long conversion_targets[MAX_TARGETS];

long icccm_version[] = { 2, 0 };

void
SetupICCCM2(Bool replace_wm)
{
	Window running_wm_win;
	XSetWindowAttributes attr;
	XEvent xev;
	XClientMessageEvent ev;
	char wm_sx[20];

	sprintf(wm_sx, "WM_S%lu", Scr.screen);
	_XA_WM_SX =     XInternAtom(dpy, wm_sx, False);
	_XA_MANAGER =   XInternAtom(dpy, "MANAGER", False);
	_XA_ATOM_PAIR = XInternAtom(dpy, "ATOM_PAIR", False);
	_XA_TARGETS =   XInternAtom(dpy, "TARGETS", False);
	_XA_MULTIPLE =  XInternAtom(dpy, "MULTIPLE", False);
	_XA_TIMESTAMP = XInternAtom(dpy, "TIMESTAMP", False);
	_XA_VERSION =   XInternAtom(dpy, "VERSION", False);
	_XA_WM_COLORMAP_NOTIFY = XInternAtom(dpy, "WM_COLORMAP_NOTIFY", False);

	/* Check for a running ICCCM 2.0 compliant WM */
	running_wm_win = XGetSelectionOwner(dpy, _XA_WM_SX);
	if (running_wm_win != None)
	{
		DBUG(
			"SetupICCCM2",
			"another ICCCM 2.0 compliant WM is running");
		if (!replace_wm)
		{
			fvwm_msg(
				ERR, "SetupICCCM2",
				"another ICCCM 2.0 compliant WM is running,"
				" try -replace");
			exit(1);
		}
		/* We need to know when the old manager is gone.
		   Thus we wait until it destroys running_wm_win. */
		attr.event_mask = StructureNotifyMask;
		XChangeWindowAttributes(
			dpy, running_wm_win, CWEventMask, &attr);
	}

	/* We are not yet in the event loop, thus fev_get_evtime() will not
	 * be ready.  Have to get a timestamp manually by provoking a
	 * PropertyNotify. */
	managing_since = get_server_time();

	XSetSelectionOwner(dpy, _XA_WM_SX, Scr.NoFocusWin, managing_since);
	if (XGetSelectionOwner(dpy, _XA_WM_SX) != Scr.NoFocusWin)
	{
		fvwm_msg(
			ERR, "SetupICCCM2",
			"failed to acquire selection ownership");
		exit(1);
	}

	/* Announce ourself as the new wm */
	ev.type = ClientMessage;
	ev.window = Scr.Root;
	ev.message_type = _XA_MANAGER;
	ev.format = 32;
	ev.data.l[0] = managing_since;
	ev.data.l[1] = _XA_WM_SX;
	FSendEvent(dpy, Scr.Root, False, StructureNotifyMask,(XEvent*)&ev);

	if (running_wm_win != None) {
		/* Wait for the old wm to finish. */
		/* FIXME: need a timeout here. */
		DBUG("SetupICCCM2", "waiting for WM to give up");
		do {
			FWindowEvent(
				dpy, running_wm_win, StructureNotifyMask, &xev);
		} while (xev.type != DestroyNotify);
	}

	/* restore NoFocusWin event mask */
	attr.event_mask = XEVMASK_NOFOCUSW;
	XChangeWindowAttributes(dpy, Scr.NoFocusWin, CWEventMask, &attr);

	return;
}

/* We must make sure that we have released SubstructureRedirect
   before we destroy manager_win, so that another wm can start
   successfully. */
void
CloseICCCM2(void)
{
	DBUG("CloseICCCM2", "good luck, new wm");
	XSelectInput(dpy, Scr.Root, NoEventMask);
	XFlush(dpy);

	return;
}

/* FIXME: property change actually succeeded */
static Bool
convertProperty(Window w, Atom target, Atom property)
{
	if (target == _XA_TARGETS)
	{
		XChangeProperty(
			dpy, w, property, XA_ATOM, 32, PropModeReplace,
			(unsigned char *)conversion_targets, MAX_TARGETS);
	}
	else if (target == _XA_TIMESTAMP)
	{
		long local_managing_since;

		local_managing_since = managing_since;
		XChangeProperty(
			dpy, w, property, XA_INTEGER, 32, PropModeReplace,
			(unsigned char *)&local_managing_since, 1);
	}
	else if (target == _XA_VERSION)
	{
		XChangeProperty(
			dpy, w, property, XA_INTEGER, 32, PropModeReplace,
			(unsigned char *)icccm_version, 2);
	}
	else
	{
		return False;
	}
	/* FIXME: This is ugly. We should rather select for
	   PropertyNotify on the window, return to the main loop,
	   and send the SelectionNotify once we are sure the property
	   has arrived. Problem: this needs a list of pending
	   SelectionNotifys. */
	XFlush(dpy);

	return True;
}

void
icccm2_handle_selection_request(const XEvent *e)
{
	Atom type;
	unsigned long *adata;
	int i, format;
	unsigned long num, rest;
	unsigned char *data;
	XSelectionRequestEvent ev = e->xselectionrequest;
	XSelectionEvent reply;

	reply.type = SelectionNotify;
	reply.display = dpy;
	reply.requestor = ev.requestor;
	reply.selection = ev.selection;
	reply.target = ev.target;
	reply.property = None;
	reply.time = ev.time;

	if (ev.target == _XA_MULTIPLE)
	{
		if (ev.property != None)
		{
			XGetWindowProperty(
				dpy, ev.requestor, ev.property, 0L, 256L, False,
				_XA_ATOM_PAIR, &type, &format, &num, &rest,
				&data);
			/* FIXME: to be 100% correct, should deal with
			 * rest > 0, but since we have 4 possible targets, we
			 * will hardly ever meet multiple requests with a
			 * length > 8
			 */
			adata = (unsigned long *)data;
			for(i = 0; i < num; i += 2)
			{
				if (!convertProperty(
					    ev.requestor, adata[i],
					    adata[i+1]))
				{
					adata[i+1] = None;
				}
			}
			XChangeProperty(
				dpy, ev.requestor, ev.property, _XA_ATOM_PAIR,
				32, PropModeReplace, data, num);
			XFree(data);
		}
	}
	else
	{
		if (ev.property == None)
		{
			ev.property = ev.target;
		}
		if (convertProperty(ev.requestor, ev.target, ev.property))
		{
			reply.property = ev.property;
		}
	}
	FSendEvent(dpy, ev.requestor, False, 0L,(XEvent*)&reply);
	XFlush(dpy);

	return;
}

/* If another wm is requesting ownership of the selection,
   we receive a SelectionClear event. In that case, we have to
   release all resources and destroy manager_win. Done() calls
   CloseICCCM2() after undecorating all windows. */
void
icccm2_handle_selection_clear(void)
{
	DBUG("HandleSelectionClear", "I lost my selection!");
	Done(0, NULL);

	return;
}
