#ifndef winradiobutton_h
#define winradiobutton_h

#include <X11/Xlib.h>
#include "WinButton.h"

class WinRadioButton: public WinButton
{
  int button_size;
 public:
  WinRadioButton(WinBase *Parent, int w, int h, int x, int y, char *label);
  void DrawCallback(XEvent *event = NULL);
  void ResizeCallback(int new_w, int new_h, XEvent *event);
  void SetBevelWidth(int new_bw);
};

#endif
