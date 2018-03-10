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
#include "ClientMsg.h"


Atom _XA_WM_PROTOCOLS = None;

void send_clientmessage (Display *disp, Window w, Atom a, Time timestamp)
{
	XClientMessageEvent ev;

	if (_XA_WM_PROTOCOLS == None)
		_XA_WM_PROTOCOLS = XInternAtom(disp, "WM_PROTOCOLS", False);

	ev.type = ClientMessage;
	ev.window = w;
	ev.message_type = _XA_WM_PROTOCOLS;
	ev.format = 32;
	ev.data.l[0] = a;
	ev.data.l[1] = timestamp;
	FSendEvent(disp, w, False, 0L, (XEvent *) &ev);
}
