/* -*-c-*- */
/* Copyright (C) 2001  Dominik Vogt */
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
#include <X11/Xlib.h>
#include <stdio.h>
#include "fvwmlib.h"
#include "Event.h"

/*
 * Return the subwindow member of an event if the event type has one.
 */
Window GetSubwindowFromEvent(Display *dpy, const XEvent *eventp)
{
	if (eventp == NULL)
	{
		return None;
	}
	switch (eventp->type)
	{
	case ButtonPress:
	case ButtonRelease:
		return eventp->xbutton.subwindow;
	case KeyPress:
	case KeyRelease:
		return eventp->xkey.subwindow;
	case EnterNotify:
	case LeaveNotify:
		return eventp->xcrossing.subwindow;
	case MotionNotify:
		return eventp->xmotion.subwindow;
	default:
		return None;
	}
}
