/* -*-c-*- */

/*
 * ICCCM Client Messages - Section 4.2.8 of the ICCCM dictates that all
 * client messages will have the following form:
 *
 *     event type       ClientMessage
 *     message type     _XA_WM_PROTOCOLS
 *     window           tmp->w
 *     format           32
 *     data[0]          message atom
 *     data[1]          time stamp
 */

#ifndef FVWMLIB_CLIENTMSG_H
#define FVWMLIB_CLIENTMSG_H

#include "fvwm_x11.h"

void
send_clientmessage(Display *disp, Window w, Atom a, Time timestamp);

extern Atom _XA_WM_PROTOCOLS;

#endif /* FVWMLIB_CLIENTMSG_H */
