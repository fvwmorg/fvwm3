#ifndef winbutton_h
#define winbutton_h

#include <X11/Xlib.h>
#include "WinText.h"

class WinButton: public WinText
{
 public:
  char momentary;
  char action;
  void (*ToggleAction)(int newstate, WinButton *which);

  WinButton(WinBase *Parent, int w, int h, int x, int y, char *label);
  ~WinButton();

  void SetToggleAction(void (*ToggleAction)(int newstate, WinButton *which));
  void MakeMomentary();
  void BPressCallback(XEvent *event = NULL);
  void BReleaseCallback(XEvent *event = NULL);
};

#endif
