/* This module, and the entire NoClutter program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation and Nobutaka Suzuki <nobuta-s@is.aist-nara.ac.jp>
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#define TRUE 1
#define FALSE 0

#include "../../configure.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/WinUtil.h>        

#include "../../fvwm/module.h"
#include "FvwmScroll.h"
#include "../../version.h"

char *MyName;
int fd_width;
int fd[2];

Display *dpy;			/* which display are we talking to */
Window Root;
int screen;
int x_fd;
int d_depth;
int ScreenWidth, ScreenHeight;

char *BackColor = "black";

Window app_win;

#define MW_EVENTS   (ExposureMask | ButtonReleaseMask | KeyReleaseMask)


/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
void main(int argc, char **argv)
{
  char *temp, *s;
  FILE *file;
  char *display_name = NULL;
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
  if (!(dpy = XOpenDisplay(display_name))) 
    {
      fprintf(stderr,"%s: can't open display %s", MyName,
	      XDisplayName(display_name));
      exit (1);
    }
  x_fd = XConnectionNumber(dpy);
  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);
  d_depth = DefaultDepth(dpy, screen);

  ScreenHeight = DisplayHeight(dpy,screen);
  ScreenWidth = DisplayWidth(dpy,screen);

  /* scan config file for set-up parameters */
  /* Colors and fonts */
  GetConfigLine(fd,&tline);

  while(tline != (char *)0)
    {
      if(strlen(tline)>1)
	{
	  if(mystrncasecmp(tline,CatString3(MyName, "Back",""),
			   Clength+4)==0)
	    {
	      CopyString(&BackColor,&tline[Clength+4]);
	    }	
	}
      GetConfigLine(fd,&tline);
    }
  
  /* sever our connection with fvwm */
  close(fd[0]);
  close(fd[1]);
  if(app_win == 0)
    GetTargetWindow(&app_win);

  if(app_win == 0)
    return;

  fd_width = GetFdWidth();

  GrabWindow(app_win);
  Loop(app_win);
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
	  sleep_a_little(1000);
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
  
  target_win = ClientWindow(*app_win);
  if(target_win != None)
    *app_win = target_win;
}


void nocolor(char *a, char *b)
{
 fprintf(stderr,"FvwmInitBanner: can't %s %s\n", a,b);
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
