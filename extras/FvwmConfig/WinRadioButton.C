#include <iostream.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "WinRadioButton.h"

WinRadioButton::WinRadioButton(WinBase *Parent, int new_w,int new_h,
		      int new_x, int new_y, char *new_label):
     WinButton(Parent,new_w, new_h, new_x,new_y, new_label)
{
  int xoff;

  button_size = 10;
  if(button_size > w)
    button_size = w;
  if(button_size > h)
    button_size = h;
  xoff = button_size+4;
  XMoveResizeWindow(dpy,twin,xoff,0,w-xoff,h);
}

void WinRadioButton::DrawCallback(XEvent *event)
{
  int xoff,yoff,i;
  GC gc1;
  GC gc2;
  int y1,y2;

  if((!event)||(event->xexpose.count == 0))
    {
      y1 = (h-button_size)/2-1;
      y2 = button_size + (h-button_size)/2-1;

      if(popped_out)
	{
	  gc1 = ShadowGC;
	  gc2 = ReliefGC;
	}
      else
	{
	  gc2 = ShadowGC;
	  gc1 = ReliefGC;
	}
      for(i=0;i<bw;i++)
	{
	  XDrawLine(dpy,win,gc1,i,    y2-i,button_size-1-i,y2-i);
	  XDrawLine(dpy,win,gc1,button_size-1-i,y1+i,button_size-1-i,y2-i);
	  XDrawLine(dpy,win,gc2,i,    y1+i,button_size-1-i,y1+i);
	  XDrawLine(dpy,win,gc2,i,    y1+i,i,    y2-i);
	}
      if(label != NULL)
	{
	  yoff = h -1 - (h - Font->ascent - Font->descent)/2 -Font->descent;
	  XDrawString(dpy,twin,ForeGC, 0,yoff,label,strlen(label));
	}
    }
}

void WinRadioButton::ResizeCallback(int new_w, int new_h, XEvent *event)
{
  int xoff;
  w = new_w;
  h = new_h;
  
  button_size = 10;
  if(button_size > w)
    button_size = w;
  if(button_size > h)
    button_size = h;

  xoff = button_size+4;
  XMoveResizeWindow(dpy,twin,xoff,0,w-xoff,h);
  RedrawWindow(1);
}

void WinRadioButton::SetBevelWidth(int new_bw)
{
  bw = new_bw;
  RedrawWindow(1);

}
