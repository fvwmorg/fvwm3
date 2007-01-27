/* -*-c-*- */
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
#include "libs/FShape.h"
#include "libs/Colorset.h"
#include "libs/PictureBase.h"
#include "libs/FRenderInit.h"
#include "libs/Strings.h"
#include "libs/System.h"
#include "libs/Target.h"
#include "libs/XError.h"
#include "FvwmScroll.h"

ModuleArgs *module;
fd_set_size_t fd_width;
int fd[2];

Display *dpy;                   /* which display are we talking to */
Window Root;
int screen;
GC hiliteGC, shadowGC;
int x_fd;
int ScreenWidth, ScreenHeight;

/* default colorset to use, set to -1 when explicitly setting colors */
int colorset = 0;

char *BackColor = "black";

static int ErrorHandler(Display*, XErrorEvent*);

/*
 *
 *  Procedure:
 *      main - start of module
 *
 */
int main(int argc, char **argv)
{
  char *tline;

  module = ParseModuleArgs(argc,argv,0); /* no alias allowed */
  if (module==NULL)
  {
    fprintf(stderr,"FvwmScroll Version %s should only be executed by fvwm!\n",
	    VERSION);
    exit(1);
  }

  if(module->user_argc >= 1)
  {
    extern int Reduction_H;
    extern int Percent_H;
    int len;
    len = strlen(module->user_argv[0])-1;
    if (len >= 0 && module->user_argv[0][len] == 'p')
    {
      module->user_argv[0][len] = '\0';
      Percent_H = atoi(module->user_argv[0]);
    }
    else
    {
      Reduction_H = atoi(module->user_argv[0]);
    }
  }

  if(module->user_argc >= 2)
  {
    extern int Reduction_V;
    extern int Percent_V;
    int len;
    len = strlen(module->user_argv[1])-1;
    if (len >= 0 && module->user_argv[1][len] == 'p')
    {
      module->user_argv[1][len] = '\0';
      Percent_V = atoi(module->user_argv[1]);
    }
    else
    {
      Reduction_V = atoi(module->user_argv[1]);
    }
  }

  /* Dead pipe == dead fvwm */
  signal (SIGPIPE, DeadPipe);

  fd[0] = module->to_fvwm;
  fd[1] = module->from_fvwm;

  /* Open the Display */
  if (!(dpy = XOpenDisplay(NULL)))
  {
    fprintf(stderr,"%s: can't open display\n", module->name);
    exit (1);
  }
  x_fd = XConnectionNumber(dpy);
  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);

  ScreenHeight = DisplayHeight(dpy,screen);
  ScreenWidth = DisplayWidth(dpy,screen);

  SetMessageMask(fd, M_CONFIG_INFO | M_END_CONFIG_INFO | M_SENDCONFIG);
  SetMessageMask(fd, MX_PROPERTY_CHANGE);
  flib_init_graphics(dpy);

  /* scan config file for set-up parameters */
  /* Colors and fonts */
  InitGetConfigLine(fd,CatString3("*",module->name,0));
  GetConfigLine(fd,&tline);

  while(tline != (char *)0)
  {
    if(strlen(tline)>1)
    {
      if(strncasecmp(tline,CatString3("*",module->name, "Back"),
		     module->namelen+4)==0)
      {
	CopyString(&BackColor,&tline[module->namelen+4]);
	colorset = -1;
      }
      else if(strncasecmp(tline,CatString3("*",module->name,"Colorset"),
                          module->namelen+8)==0)
      {
	sscanf(&tline[module->namelen+8], "%d", &colorset);
	AllocColorset(colorset);
      }
      else if(strncasecmp(tline, "Colorset", 8) == 0)
      {
	LoadColorset(&tline[8]);
      }
    }
    GetConfigLine(fd,&tline);
  }

  XSetErrorHandler(ErrorHandler);

  if(module->window == 0)
    GetTargetWindow(&module->window);

  if(module->window == 0)
    return 0;

  fd_width = GetFdWidth();

  GrabWindow(module->window);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);

  Loop(module->window);
  return 0;
}

/*
 *
 * Detected a broken pipe - time to exit
 *
 */
RETSIGTYPE DeadPipe(int nonsense)
{
  extern Atom wm_del_win;

  XReparentWindow(dpy,module->window,Root,0,0);
  send_clientmessage (dpy, module->window, wm_del_win, CurrentTime);
  XSync(dpy,0);
  exit(0);
  SIGNAL_RETURN;
}


/*
 *
 * If no application window was indicated on the command line, prompt
 * the user to select one
 *
 */
void GetTargetWindow(Window *app_win)
{
  Window target_win;

  fvwmlib_get_target_window(dpy, screen, module->name, app_win, True);
  target_win = fvwmlib_client_window(dpy, *app_win);
  if(target_win != None)
    *app_win = target_win;
}

/*
  X Error Handler
*/
static int
ErrorHandler(Display *dpy, XErrorEvent *event)
{
  /* some errors are OK=ish */
  if (event->error_code == BadPixmap)
    return 0;
  if (event->error_code == BadDrawable)
    return 0;
#if 0
  if (FRenderGetErrorCodeBase() + FRenderBadPicture == event->error_code)
    return 0;
#endif

  PrintXErrorAndCoredump(dpy, event, module->name);
  return 0;
}
