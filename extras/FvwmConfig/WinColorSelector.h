#ifndef winColorSelector_h
#define winColorSelector_h

#include <X11/Xlib.h>
#include "WinBase.h"
#include "WinSlider.h"

class WinColorSelector: public WinBase
{
 public:
  float red_value;
  float green_value;
  float blue_value;
  unsigned long private_planes;
  unsigned long private_pixel;
  WinSlider *red_slider;
  WinSlider *green_slider;
  WinSlider *blue_slider;
  WinBase *colorpanel;

  void (*MotionAction)(float new_red, float new_green, float new_blue, 
		       WinColorSelector *which);

  WinColorSelector(WinBase *Parent, int w, int h, int x, int y);
  ~WinColorSelector();

  void SetCurrentValue(float new_red, float new_green, float new_blue);
  void UpdatePosition(int newx, int newy);
  void SetMotionAction(void (*MotionAction)(float new_red, float new_green, 
					    float new_blue, 
					    WinColorSelector *which));

};

#endif
