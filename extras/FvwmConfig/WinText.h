#ifndef wintext_h
#define wintext_h

#include <X11/Xlib.h>
#include "WinBase.h"

class WinText: public WinBase
{
 public:
  char *label;
  Window twin;

  WinText(WinBase *Parent, int w, int h, int x, int y, char *label);
  ~WinText();

  void SetLabel(char *new_label);
  void SetBackColor(char *newcolor);
  void DrawCallback(XEvent *event = NULL);
  virtual void ResizeCallback(int new_w, int new_h, XEvent *event = NULL);
  void SetBevelWidth(int bw);
  void RedrawWindow(int clear);
};

#endif
