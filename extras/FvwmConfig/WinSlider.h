#ifndef winslider_h
#define winslider_h

#include <X11/Xlib.h>
#include "WinBase.h"

class WinSlider: public WinBase
{
 public:
  float min_value;
  float max_value;
  float current_value;
  
  void (*MotionAction)(float new_location, WinSlider *which);

  WinSlider(WinBase *Parent, int w, int h, int x, int y,
	    float min_val=0, float max_val=100, 
	    float init_val=50);
  ~WinSlider();

  void SetMinValue(float new_val);
  void SetMaxValue(float new_val);
  void SetCurrentValue(float new_val);
  void UpdatePosition(int newx, int newy);
  void SetMotionAction(void (*NewMotionAction)(float new_location,
					       WinSlider *which));
  void DrawCallback(XEvent *event = NULL);
  void BPressCallback(XEvent *event = NULL);
  void BReleaseCallback(XEvent *event = NULL);
  void MotionCallback(XEvent *event = NULL);
};

#endif
