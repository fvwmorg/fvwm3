#ifndef wininput_h
#define wininput_h

#include <X11/Xlib.h>
#include "WinText.h"

#define HISTORY_LENGTH 100 

class WinInput: public WinText
{
 public:
  int text_offset;
  int num_chars;
  int label_size;
  int ptr;
  int endptr;

  void (*NewLineAction)(int numchars, char *newdata);

  WinInput(WinBase *Parent, int w, int h, int x, int y, char *initlabel= NULL);
  ~WinInput();

  char *GetLine(void);
  void DrawCallback(XEvent *event);
  void KPressCallback(XEvent *event = NULL);
  void SetNewLineAction(void (*NewLineAction)(int numchars, char *newdata));
  void SetLabel(char *text);
};

#endif
