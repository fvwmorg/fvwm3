/****************************************************************************
 * This is an all new program to set the root window to an Xpm pixmap.
 * Copyright 1993, Rob Nation 
 * You may use this file for anything you want, as long as the copyright
 * is kept intact. No guarantees of any sort are made in any way regarding
 * this program or anything related to it.
 ****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include "../libs/fvwmlib.h"     
#include <X11/xpm.h> /* Has to be after Intrinsic.h gets included */

int save_colors = 0;
Display *dpy;
int screen;
Window root;
char *display_name = NULL;
void SetRootWindow(char *tline);
Pixmap rootXpm;

int main(int argc, char **argv)
{
  Atom prop, type;
  int format;
  unsigned long length, after;
  unsigned char *data;

  if(argc != 2)
    {
      fprintf(stderr,"Xpmroot Version %s\n",VERSION);
      fprintf(stderr,"Usage: xpmroot xpmfile\n");
      fprintf(stderr,"Try Again\n");
      exit(1);
    }
  dpy = XOpenDisplay(display_name);
  if (!dpy) 
    {
      fprintf(stderr, "Xpmroot:  unable to open display '%s'\n",
	      XDisplayName (display_name));
      exit (2);
    }
  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);
  
  SetRootWindow(argv[1]);

  prop = XInternAtom(dpy, "_XSETROOT_ID", False);
  
  (void)XGetWindowProperty(dpy, root, prop, 0L, 1L, True, AnyPropertyType,
			   &type, &format, &length, &after, &data);
  if ((type == XA_PIXMAP) && (format == 32) && (length == 1) && (after == 0))
    XKillClient(dpy, *((Pixmap *)data));

  XChangeProperty(dpy, root, prop, XA_PIXMAP, 32, PropModeReplace,
		  (unsigned char *) &rootXpm, 1);
  XSetCloseDownMode(dpy, RetainPermanent);
  XCloseDisplay(dpy);
  return 0;
}


void SetRootWindow(char *tline)
{
  XWindowAttributes root_attr;
  XpmAttributes xpm_attributes;
  Pixmap shapeMask;
  int val;

  XGetWindowAttributes(dpy,root,&root_attr);
  xpm_attributes.colormap = root_attr.colormap;
  xpm_attributes.valuemask = XpmSize | XpmReturnPixels|XpmColormap;
  if((val = XpmReadFileToPixmap(dpy,root, tline,
			 &rootXpm, &shapeMask, 
			 &xpm_attributes))!= XpmSuccess) 
    {
      if(val == XpmOpenFailed)
	fprintf(stderr, "Couldn't open pixmap file\n");
      else if(val == XpmColorFailed)
	fprintf(stderr, "Couldn't allocate required colors\n");
      else if(val == XpmFileInvalid)
	fprintf(stderr, "Invalid Format for an Xpm File\n");
      else if(val == XpmColorError)
	fprintf(stderr, "Invalid Color specified in Xpm FIle\n");
      else if(val == XpmNoMemory)
	fprintf(stderr, "Insufficient Memory\n");
      exit(1);
    }

  XSetWindowBackgroundPixmap(dpy, root, rootXpm);
  save_colors = 1;
  XClearWindow(dpy,root);

}
