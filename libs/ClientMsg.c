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

/***************************************************************************
 *
 * ICCCM Client Messages - Section 4.2.8 of the ICCCM dictates that all
 * client messages will have the following form:
 *
 *     event type	ClientMessage
 *     message type	_XA_WM_PROTOCOLS
 *     window		tmp->w
 *     format		32
 *     data[0]		message atom
 *     data[1]		time stamp
 *
 ****************************************************************************/
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

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
  XSendEvent (disp, w, False, 0L, (XEvent *) &ev);
}
