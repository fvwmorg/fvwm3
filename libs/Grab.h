/* -*-c-*- */

/*
** MyXGrabServer & MyXUngrabServer - to handle nested grab server calls
*/

#ifndef FVWMLIB_GRAB_H
#define FVWMLIB_GRAB_H

#include "fvwm_x11.h"

void
MyXGrabServer(Display *disp);
void
MyXUngrabServer(Display *disp);
void
MyXUngrabKeyboard(Display *disp);
void
MyXGrabKeyboard(Display *disp);
void
MyXGrabKey(Display *disp);
void
MyXUngrabKey(Display *disp);

#endif /* FVWMLIB_GRAB_H */
