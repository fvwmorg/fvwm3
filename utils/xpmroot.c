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
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <libs/fvwmlib.h>
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
  Atom prop, type, e_prop;
  int format;
  unsigned long length, after;
  unsigned char *data;
  int i = 1;
  Bool FreeEsetroot = False;
  Bool NotPermanent = False;
  Bool Dummy = False;

  if(argc < 2)
  {
    fprintf(stderr,"Xpmroot Version %s\n",VERSION);
    fprintf(stderr,"Usage: xpmroot [-fe -np -d] xpmfile\n");
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
  
  for(i=1;i<argc-1;i++)
  {
    if (strcasecmp(argv[i],"-fe") == 0)
    {
      FreeEsetroot = True;
    }
    else if (strcasecmp(argv[i],"-np") == 0)
    {
      NotPermanent = True;
    }
    else if (strcasecmp(argv[i],"-d") == 0)
    {
      Dummy = True;
      NotPermanent = True;
    }
    else
    {
      fprintf(stderr, "Xpmroot:  unknow option '%s'\n", argv[i]);
    }
  }

  if (Dummy || strcasecmp(argv[argc-1],"-d") == 0)
  {
    Dummy = True;
    NotPermanent = True;
  }
  else
  {
    SetRootWindow(argv[argc-1]);
  }

  prop = XInternAtom(dpy, "_XSETROOT_ID", False);
  (void)XGetWindowProperty(dpy, root, prop, 0L, 1L, True, AnyPropertyType,
			   &type, &format, &length, &after, &data);
  if (type == XA_PIXMAP && format == 32 && length == 1 && after == 0)
  {
    XKillClient(dpy, *((Pixmap *)data));
  }

  if (FreeEsetroot)
  {
    if (data != NULL)
      XFree(data);
    e_prop = XInternAtom(dpy, "ESETROOT_PMAP_ID", False);
    (void)XGetWindowProperty(dpy, root, e_prop, 0L, 1L, True, AnyPropertyType,
			     &type, &format, &length, &after, &data);
    if ((type == XA_PIXMAP) && (format == 32) && (length == 1) && (after == 0)
	&& *((Pixmap *)data) != None)
    {
      XKillClient(dpy, *((Pixmap *)data));
    }
    XDeleteProperty(dpy, root, e_prop);
  }

  if (NotPermanent)
  {
    if (rootXpm != None)
      XFreePixmap(dpy, rootXpm);
    rootXpm = None;
  }
  else
  {
    XSetCloseDownMode(dpy, RetainPermanent);
  }
  XChangeProperty(dpy, root, prop, XA_PIXMAP, 32, PropModeReplace,
		  (unsigned char *) &rootXpm, 1);
  XCloseDisplay(dpy);
  return 0;
}


void SetRootWindow(char *tline)
{
  XWindowAttributes root_attr;
  XpmAttributes xpm_attributes;
  Pixmap shapeMask;
  int val;

  if (!XGetWindowAttributes(dpy,root,&root_attr))
  {
    fprintf(stderr, "error: failed to get root window attributes\n");
    exit(0);
  }
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
