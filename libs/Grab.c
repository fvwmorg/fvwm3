/*
** MyXGrabServer & MyXUngrabServer - to handle nested grab server calls
*/

#include <X11/Xlib.h>

/* Made into global for module interface.  See module.c. */
int myxgrabcount = 0;

void MyXGrabServer(Display *disp)
{
  if (myxgrabcount == 0)
  {
    XGrabServer(disp);
  }
  ++myxgrabcount;
}

void MyXUngrabServer(Display *disp)
{
  if (--myxgrabcount < 0) /* should never happen */
  {
    /* fvwm_msg(ERR,"MyXUngrabServer","too many ungrabs!\n"); */
    myxgrabcount = 0;
  }
  if (myxgrabcount == 0)
  {
    XUngrabServer(disp);
  }
}

