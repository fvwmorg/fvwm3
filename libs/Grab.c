/*
** MyXGrabServer & MyXUngrabServer - to handle nested grab server calls
*/

#include <X11/Xlib.h>

static int xgrabcount = 0;

void MyXGrabServer(Display *disp)
{
  if (xgrabcount == 0)
  {
    XGrabServer(disp);
  }
  ++xgrabcount;
}

void MyXUngrabServer(Display *disp)
{
  if (--xgrabcount < 0) /* should never happen */
  {
    /* fvwm_msg(ERR,"MyXUngrabServer","too many ungrabs!\n"); */
    xgrabcount = 0;
  }
  if (xgrabcount == 0)
  {
    XUngrabServer(disp);
  }
}

