#include <iostream.h>
#include <stdlib.h>
#include <math.h>
#include "WinButton.h"

WinButton::WinButton(WinBase *Parent, int new_w,int new_h,
		      int new_x, int new_y, char *new_label):
     WinText(Parent,new_w, new_h, new_x,new_y, new_label)
{
  label = new_label;
  action = 0;
  momentary = 0;
  ToggleAction = NULL;
}

WinButton::~WinButton()
{
}

void WinButton::SetToggleAction(void (*NewToggleAction)(int newstate, 
							WinButton *which))
{
  ToggleAction = NewToggleAction;
}

void WinButton::MakeMomentary()
{
  momentary = 1;
}

void WinButton::BPressCallback(XEvent *event)
{
  action = popped_out;
  popped_out = 0;
  RedrawWindow(0);
}


void WinButton::BReleaseCallback(XEvent *event)
{
  if((event->xbutton.x > w)||(event->xbutton.x <0)||
     (event->xbutton.y > h)||(event->xbutton.y <0))
    {
      popped_out = action;
      action = 0;
    }
  else
    {
      if(momentary)
	{
	  popped_out = 1;
	  if(ToggleAction != NULL)
	    ToggleAction(popped_out,this);
	}
      else
	{
	  popped_out = 1-action;
	  if(ToggleAction != NULL)
	    ToggleAction(popped_out,this);
	}
    }
  RedrawWindow(0);
}



