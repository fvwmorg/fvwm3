#include <iostream.h>
#include <stdlib.h>
#include <math.h>
#include <X11/Intrinsic.h>
#include "WinBase.h"
#include "WinColorSelector.h"
#include "WinSlider.h"

static void positioncallback(float loc, WinSlider *which);

WinColorSelector::WinColorSelector(WinBase *Parent, int new_w,int new_h,
		     int new_x, int new_y)
     : WinBase(Parent,new_w, new_h, new_x,new_y)
{
  int slider_width;
  int slider_height;
  XColor color;


  MotionAction = NULL;
  slider_width = new_w/2 - 5;
  slider_height = (new_h-5)/3 - 5;
  if(slider_height < 20)
    slider_height = 20;

  red_value = 32000.0;
  green_value = 32000.0;
  blue_value = 32000.0;

  red_slider = new WinSlider(this,slider_width,slider_height,
			     5,5,0.0,65535.0,red_value);
  green_slider = new WinSlider(this,slider_width,slider_height,
			       5,slider_height+10,0.0,65535.0,green_value);
  blue_slider = new WinSlider(this,slider_width,slider_height,
			      5,2*slider_height+15,0.0,65535.0,blue_value);
  red_slider->SetMotionAction(positioncallback);
  green_slider->SetMotionAction(positioncallback);
  blue_slider->SetMotionAction(positioncallback);
  red_slider->SetBevelWidth(0);
  green_slider->SetBevelWidth(0);
  blue_slider->SetBevelWidth(0);
  colorpanel = new WinBase(this,new_w/2-20,new_h-20,new_w/2+10,10);
  colorpanel->PushIn();
  if(!XAllocColorCells(dpy,cmap,False,&private_planes,0,&private_pixel,1))
    {
      cerr <<"Couldn't alloc r/w color cell. Trying new map\n";
      cmap = XCopyColormapAndFree(dpy,cmap);
      if(!XAllocColorCells(dpy,cmap,False,&private_planes,0,&private_pixel,1))
	{
	  cerr <<"Couldn't alloc r/w color cell. giving up\n";
	  private_pixel = 0;
	}
      else
	{
	  XSetWindowColormap(dpy,main_window->win,cmap);
	}
    }

  
  XSetWindowBackground(dpy,colorpanel->win,private_pixel);

  if(private_pixel != 0)
    {
      color.pixel = private_pixel;
      color.red = (int)red_value;
      color.green = (int)green_value;
      color.blue = (int)blue_value;
      color.flags = DoRed | DoGreen| DoBlue;
      XStoreColor(dpy,cmap,&color);
    }
  colorpanel->Map();
  red_slider->Map();
  green_slider->Map();
  blue_slider->Map();
}

WinColorSelector::~WinColorSelector()
{
  delete red_slider;
  delete green_slider;
  delete blue_slider;
  XFreeColors(dpy,cmap,&private_pixel,1,private_planes);
  delete colorpanel;
}




void WinColorSelector::SetCurrentValue(float new_red,
				       float new_green,
				       float new_blue)
{
  red_value = new_red;
  green_value = new_green;
  blue_value = new_blue;
  positioncallback(red_value, red_slider);
  positioncallback(green_value, green_slider);
  positioncallback(blue_value, blue_slider);
  red_slider->SetCurrentValue(red_value);
  red_slider->RedrawWindow(1);
  green_slider->SetCurrentValue(green_value);
  green_slider->RedrawWindow(1);
  blue_slider->SetCurrentValue(blue_value);
  blue_slider->RedrawWindow(1);
  
  
}

void WinColorSelector::SetMotionAction(void (*NewMotionAction)
				       (float new_red, float new_green, 
					float new_blue, 
					WinColorSelector *which))
{
  MotionAction = NewMotionAction;
}

static void positioncallback(float loc, WinSlider *which)
{
  XColor color;
  WinColorSelector *t;

  t = (WinColorSelector *)(which->Parent);
  if(which == t->red_slider)
    {
      t->red_value = loc;
    }
  else if(which == t->blue_slider)
    {
      t->blue_value = loc;
    }
  else
    {
      t->green_value = loc;
    }      
  if(t->private_pixel != 0)
    {
      color.pixel = t->private_pixel;
      color.red = (int)t->red_value;
      color.green = (int)t->green_value;
      color.blue = (int)t->blue_value;
      color.flags = DoRed | DoGreen| DoBlue;
      XStoreColor(which->dpy,which->cmap,&color);
    }
  if(t->MotionAction != NULL)
    t->MotionAction(t->red_value, t->green_value, 
		    t->blue_value, 
		    (WinColorSelector *)which->Parent);
}
