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

/*
** MyXGrabServer & MyXUngrabServer - to handle nested grab server calls
*/

#include "config.h"

#include "Grab.h"

/* Made into global for module interface.  See module.c. */
int myxgrabcount = 0;
static unsigned int keyboard_grab_count = 0;
static unsigned int key_grab_count = 0;

void MyXGrabServer(Display *disp)
{
	if (myxgrabcount == 0)
	{
		XSync(disp, 0);
		XGrabServer(disp);
	}
	XSync(disp, 0);
	++myxgrabcount;
}

void MyXUngrabServer(Display *disp)
{
	if (--myxgrabcount < 0) /* should never happen */
	{
		myxgrabcount = 0;
	}
	if (myxgrabcount == 0)
	{
		XUngrabServer(disp);
	}
	XSync(disp, 0);
}

void MyXGrabKeyboard(Display *dpy)
{
	keyboard_grab_count++;
	XGrabKeyboard(
		dpy, RootWindow(dpy, DefaultScreen(dpy)), False, GrabModeAsync,
		GrabModeAsync, CurrentTime);

	return;
}

void MyXUngrabKeyboard(Display *dpy)
{
	if (keyboard_grab_count > 0)
	{
		keyboard_grab_count--;
	}
	if (keyboard_grab_count == 0 && key_grab_count == 0)
	{
		XUngrabKeyboard(dpy, CurrentTime);
	}

	return;
}

void MyXGrabKey(Display *disp)
{
	key_grab_count++;

	return;
}

void MyXUngrabKey(Display *disp)
{
	if (key_grab_count > 0)
	{
		key_grab_count--;
		keyboard_grab_count++;
		MyXUngrabKeyboard(disp);
	}

	return;
}
