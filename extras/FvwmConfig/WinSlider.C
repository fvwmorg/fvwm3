#include <iostream.h>
#include <stdlib.h>
#include <math.h>
#include <X11/Intrinsic.h>
#include "WinBase.h"
#include "WinSlider.h"

WinSlider::WinSlider(WinBase *Parent, int new_w,int new_h,
		     int new_x, int new_y, 
		     float min_val, float max_val, float init_val)
     : WinBase(Parent,new_w, new_h, new_x,new_y)
{
  min_value = min_val;
  max_value = max_val;
  MotionAction = NULL;
  current_value = init_val;

  XSetWindowAttributes attributes;      /* attributes for creating window */
  unsigned long mask;

  mask = CWEventMask;
  attributes.event_mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
    ExposureMask|KeyPressMask|StructureNotifyMask;
  XChangeWindowAttributes(dpy,win,mask,&attributes);
}

WinSlider::~WinSlider()
{
}


void WinSlider::SetMinValue(float new_val)
{
  min_value = new_val;
}


void WinSlider::SetMaxValue(float new_val)
{
  max_value = new_val;
}


void WinSlider::SetCurrentValue(float new_val)
{
  current_value = new_val;
}

void WinSlider::SetMotionAction(void (*NewMotionAction)(float new_location, 
							WinSlider *which))
{
  MotionAction = NewMotionAction;
}

void WinSlider::DrawCallback(XEvent *event)
{
  int xoff,yoff,x1,x2,x3,x4,y1,y2,y3,y4;
  GC gc1;
  GC gc2;

  WinBase::DrawCallback(event);
  if((!event)||(event->xexpose.count == 0))
    {
      x1 = (int)(w * 0.1);
      x2 = (int)(w * 0.9);
      y1 = h-15;
      y2 = h-11;
      XDrawLine(dpy,win,ShadowGC,x1+1,y1,x2-1,y1);
      XDrawLine(dpy,win,ShadowGC,x1,y1+1,x1,y2-1);
      XDrawLine(dpy,win,ReliefGC,x1+1,y2,x2-1,y2);
      XDrawLine(dpy,win,ReliefGC,x2,y1+1,x2,y2-1);
      
      x3 = (int)((current_value - min_value)/(max_value - min_value) * w * 0.8 - 2 + x1);
      x4 = x3+5;
      y3 = h-20;
      y4 = h-6;
      XDrawLine(dpy,win,ShadowGC,x3+1,y3,x4-1,y3);
      XDrawLine(dpy,win,ShadowGC,x3,y3+1,x3,y4-1);
      XDrawLine(dpy,win,ReliefGC,x3+1,y4,x4-1,y4);
      XDrawLine(dpy,win,ReliefGC,x4,y3+1,x4,y4-1);
      
      if(x3 > x1)
	{
	  XDrawLine(dpy,win,ShadowGC,x1+1,y1+2,x3-1,y1+2);
	  XDrawLine(dpy,win,ShadowGC,x1+1,y1+3,x3-1,y1+3);
	  XDrawLine(dpy,win,ShadowGC,x1+1,y1+4,x3-1,y1+4);
	}
      XClearArea(dpy,win,x3+1,y3+1,x4-x3-1,y4-y3-1,0);
    }
}



void WinSlider::BPressCallback(XEvent *event)
{
  int newx, newy;
  newx = event->xbutton.x;
  newy = event->xbutton.y;
  UpdatePosition(newx,newy);
}

void WinSlider::BReleaseCallback(XEvent *event)
{
  int newx, newy;

  newx = event->xbutton.x;
  newy = event->xbutton.y;

  UpdatePosition(newx,newy);
}
void WinSlider::MotionCallback(XEvent *event)
{
  int newx, newy;
  newx = event->xmotion.x;
  newy = event->xmotion.y;

  UpdatePosition(newx,newy);
}


void WinSlider::UpdatePosition(int newx,int newy)
{
  int x3,x4,y3,y4,x1,y1;

  x1 = (int)(w * 0.1);
  x3 = (int)((current_value - min_value)/(max_value - min_value) * w * 0.8 
	     - 2 + x1);
  x4 = x3+5;
  y3 = h-20;
  y4 = h-6;
  y1 = h-15;
  XClearArea(dpy,win,x3,y3,x4-x3+1,y4-y3+1,0);
  XClearArea(dpy,win,x1,y1+2,x3-x1,3,0);

  
  current_value = (max_value - min_value)*(newx - w*0.1)/(w*0.8) + min_value;
  if(current_value < min_value)
    current_value = min_value;
  if(current_value > max_value)
    current_value = max_value;
  
  
  DrawCallback(NULL);
  if(MotionAction != NULL)
    {
      MotionAction(current_value, this);
    }
}
