/* -*-c-*- */

/*
** MyXGrabServer & MyXUngrabServer - to handle nested grab server calls
*/

#include <X11/Xlib.h>

void MyXGrabServer(Display *disp);
void MyXUngrabServer(Display *disp);
void MyXUngrabKeyboard(Display *disp);
void MyXGrabKeyboard(Display *disp);
void MyXGrabKey(Display *disp);
void MyXUngrabKey(Display *disp);
