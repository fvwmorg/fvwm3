#include <iostream.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "WinBase.h"
#include "WinButton.h"

WinText::WinText(WinBase *Parent, int new_w,int new_h,
		      int new_x, int new_y, char *new_label):
     WinBase(Parent,new_w, new_h, new_x,new_y)
{
  XSetWindowAttributes attributes;      /* attributes for creating window */
  unsigned long mask;

  label = new_label;
  mask = CWBackPixel | CWEventMask;
  attributes.background_pixel = BackColor;
  attributes.event_mask = ExposureMask;
  twin = XCreateWindow(dpy,win,bw,bw,w-2*bw,h-2*bw,0,
		       CopyFromParent,InputOutput,CopyFromParent,
		       mask,&attributes);
  XMapWindow(dpy,twin);
  RegisterWindow(twin,this);
}

WinText::~WinText()
{
}

void WinText::SetLabel(char *new_label)
{
  label = new_label;
}



void WinText::DrawCallback(XEvent *event)
{
  int xoff,yoff;
  GC gc1;
  GC gc2;
  
  WinBase::DrawCallback(event);
  if(((!event)||(event->xexpose.count == 0))&&(label != NULL))
    {
      xoff = (w - XTextWidth(Font,label,strlen(label)))/2;
      yoff = h - 1 - (h - Font->ascent - Font->descent)/2 -Font->descent;
      XDrawString(dpy,twin,ForeGC, xoff-bw,yoff-bw,label,strlen(label));
    }
}

void WinText::ResizeCallback(int new_w, int new_h, XEvent *event)
{
  w = new_w;
  h = new_h;
  XResizeWindow(dpy,twin,new_w-2*bw,new_h-2*bw);
  RedrawWindow(1);
}

void WinText::SetBevelWidth(int new_bw)
{
  bw = new_bw;
  XMoveResizeWindow(dpy,twin,bw,bw,w-2*bw,h-2*bw);
  RedrawWindow(1);
}


void WinText::SetBackColor(char *newcolor = DEFAULT_BACKCOLOR)
{
  XSetWindowAttributes attributes;      /* attributes for creating window */
  unsigned long mask;

  mask = CWBackPixel;
  attributes.background_pixel = BackColor;
  XChangeWindowAttributes(dpy,twin, mask,&attributes);
  XClearWindow(dpy,twin);
  XChangeWindowAttributes(dpy,win, mask,&attributes);
  XClearWindow(dpy,win);
}

void WinText::RedrawWindow(int clear)
{
  if(clear)
    {
      XClearWindow(dpy,win);
      XClearWindow(dpy,twin);
    }
  DrawCallback(NULL);
}




