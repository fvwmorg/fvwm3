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
