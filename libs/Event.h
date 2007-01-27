#ifndef LIB_EVENT_H
#define LIB_EVENT_H

/*
 * Return the subwindow member of an event if the event type has one.
 */
Window GetSubwindowFromEvent(Display *dpy, const XEvent *eventp);

#endif /* LIB_EVENT_H */
