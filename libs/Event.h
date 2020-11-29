#ifndef FVWMLIB_EVENT_H
#define FVWMLIB_EVENT_H

#include <X11/Xlib.h>

/*
 * Return the subwindow member of an event if the event type has one.
 */
Window GetSubwindowFromEvent(Display *dpy, const XEvent *eventp);

#endif /* FVWMLIB_EVENT_H */
