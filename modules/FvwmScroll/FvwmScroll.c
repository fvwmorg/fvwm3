/* This module, and the entire NoClutter program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation and Nobutaka Suzuki <nobuta-s@is.aist-nara.ac.jp>
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#include "libs/Module.h"
#include "libs/fvwmlib.h"
#include "libs/Picture.h"
#include "libs/Colorset.h"
#include "FvwmScroll.h"

char *MyName;
fd_set_size_t fd_width;
int fd[2];

Display *dpy;			/* which display are we talking to */
Window Root;
int screen;
GC hiliteGC, shadowGC;
int x_fd;
int ScreenWidth, ScreenHeight;

/* default colorset to use, set to -1 when explicitly setting colors */
int colorset = 0;

char *BackColor = "black";

Window app_win;

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
int main(int argc, char **argv)
{
  char *temp, *s;
  int Clength;
  char *tline;

  /* Save the program name for error messages and config parsing */
  temp = argv[0];
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = safemalloc(strlen(temp)+2);
  strcpy(MyName,"*");
  strcat(MyName, temp);
  Clength = strlen(MyName);

  if(argc < 6)
    {
      fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MyName,
	      VERSION);
      exit(1);
    }

  if(argc >= 7)
    {
      extern int Reduction_H;
      Reduction_H = atoi(argv[6]);
    }

  if(argc >= 8)
    {
      extern int Reduction_V;
      Reduction_V = atoi(argv[7]);
    }

  /* Dead pipe == dead fvwm */
  signal (SIGPIPE, DeadPipe);

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  /* An application window may have already been selected - look for it */
  sscanf(argv[4],"%x",(unsigned int *)&app_win);

  /* Open the Display */
  if (!(dpy = XOpenDisplay(NULL)))
    {
      fprintf(stderr,"%s: can't open display\n", MyName);
      exit (1);
    }
  x_fd = XConnectionNumber(dpy);
  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);

  ScreenHeight = DisplayHeight(dpy,screen);
  ScreenWidth = DisplayWidth(dpy,screen);

  SetMessageMask(fd, M_CONFIG_INFO | M_END_CONFIG_INFO | M_SENDCONFIG);
  InitPictureCMap(dpy);
  /* prevent core dumps if fvwm doesn't provide any colorsets */
  AllocColorset(0);

  /* scan config file for set-up parameters */
  /* Colors and fonts */
  InitGetConfigLine(fd,MyName);
  GetConfigLine(fd,&tline);

  while(tline != (char *)0)
    {
      if(strlen(tline)>1)
	{
	  if(strncasecmp(tline,CatString3(MyName, "Back",""),
			   Clength+4)==0)
	    {
	      CopyString(&BackColor,&tline[Clength+4]);
	      colorset = -1;
	    }
    else if(strncasecmp(tline,CatString3(MyName,"Colorset",""),Clength+8)==0)
      {
        sscanf(&tline[Clength+8], "%d", &colorset);
      }
    else if(strncasecmp(tline, "Colorset", 8) == 0)
      {
        LoadColorset(&tline[8]);
	    }
	}
      GetConfigLine(fd,&tline);
    }

  if(app_win == 0)
    GetTargetWindow(&app_win);

  if(app_win == 0)
    return 0;

  fd_width = GetFdWidth();

  GrabWindow(app_win);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);

  Loop(app_win);
  return 0;
}

/***********************************************************************
 *
 * Detected a broken pipe - time to exit
 *
 **********************************************************************/
void DeadPipe(int nonsense)
{
  extern Atom wm_del_win;

  XReparentWindow(dpy,app_win,Root,0,0);
  send_clientmessage (dpy, app_win, wm_del_win, CurrentTime);
  XSync(dpy,0);
  exit(0);
}


/**********************************************************************
 *
 * If no application window was indicated on the command line, prompt
 * the user to select one
 *
 *********************************************************************/
void GetTargetWindow(Window *app_win)
{
  XEvent eventp;
  int val = -10,trials;
  Window target_win;

  trials = 0;
  while((trials <100)&&(val != GrabSuccess))
    {
      val=XGrabPointer(dpy, Root, True,
		       ButtonReleaseMask,
		       GrabModeAsync, GrabModeAsync, Root,
		       XCreateFontCursor(dpy,XC_crosshair),
		       CurrentTime);
      if(val != GrabSuccess)
	{
	  usleep(1000);
	}
      trials++;
    }
  if(val != GrabSuccess)
    {
      fprintf(stderr,"%s: Couldn't grab the cursor!\n",MyName);
      exit(1);
    }
  XMaskEvent(dpy, ButtonReleaseMask,&eventp);
  XUngrabPointer(dpy,CurrentTime);
  XSync(dpy,0);
  *app_win = eventp.xany.window;
  if(eventp.xbutton.subwindow != None)
    *app_win = eventp.xbutton.subwindow;

  /* Don't allow operations on the root window */
  if (*app_win == Root)
  {
    *app_win = None;
    return;
  }

  target_win = ClientWindow(*app_win);
  if(target_win != None)
    *app_win = target_win;
}


void nocolor(char *a, char *b)
{
 fprintf(stderr,"FvwmScroll: can't %s %s\n", a,b);
}



/****************************************************************************
 *
 * Find the actual application
 *
 ***************************************************************************/
Window ClientWindow(Window input)
{
  Atom _XA_WM_STATE;
  unsigned int nchildren;
  Window root, parent, *children,target;
  unsigned long nitems, bytesafter;
  unsigned char *prop;
  Atom atype;
  int aformat;
  int i;

  _XA_WM_STATE = XInternAtom (dpy, "WM_STATE", False);

  if (XGetWindowProperty (dpy,input, _XA_WM_STATE , 0L,
			  3L , False, _XA_WM_STATE,&atype,
			  &aformat, &nitems, &bytesafter,
			  &prop) == Success)
    {
      if(prop != NULL)
	{
	  XFree(prop);
	  return input;
	}
    }

  if(!XQueryTree(dpy, input, &root, &parent, &children, &nchildren))
    return None;

  for (i = 0; i < nchildren; i++)
    {
      target = ClientWindow(children[i]);
      if(target != None)
	{
	  XFree((char *)children);
	  return target;
	}
    }
  XFree((char *)children);
  return None;
}
