/* FvwmWinList Module for Fvwm.
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * The author makes not guarantees or warantees, either express or
 * implied.  Feel free to use any contained here for any purpose, as long
 * and this and any other applicible copyrights are kept intact.
 *
 * The functions in this source file that are based on part of the FvwmIdent
 * module for Fvwm are noted by a small copyright atop that function, all
 * others are copyrighted by Mike Finger.  For those functions modified/used,
 * here is the full, origonal copyright:
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
 * Modifications Done to Add Pixmaps, focus highlighting and Current Desk Only
 * By Don Mahurin, 1996, Some of this Code was taken from FvwmTaskBar
 */

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

#ifndef NO_CONSOLE
#define NO_CONSOLE
#endif

#define YES "Yes"
#define NO  "No"

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdlib.h>

#include "libs/Module.h"
#include "libs/fvwmsignal.h"
#include "libs/fvwmlib.h"
#include "libs/XineramaSupport.h"

#include "FvwmWinList.h"
#include "ButtonArray.h"
#include "List.h"
#include "Mallocs.h"
#include "libs/Colorset.h"

#ifdef I18N_MB
#include <X11/Xlocale.h>
#ifdef __STDC__
#define XTextWidth(x,y,z) XmbTextEscapement(x ## set,y,z)
#else
#define XTextWidth(x,y,z) XmbTextEscapement(x/**/set,y,z)
#endif
#define XDrawString(t,u,v,w,x,y,z) XmbDrawString(t,u,ButtonFontset,v,w,x,y,z)
#endif

#define MAX_NO_ICON_ACTION_LENGTH (MAX_MODULE_INPUT_TEXT_LEN - 100)

#define GRAB_EVENTS (ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|EnterWindowMask|LeaveWindowMask)

#define SomeButtonDown(a) ((a) & DEFAULT_ALL_BUTTONS_MASK)
#define COMPLEX_WINDOW_PLACEMENT

/* File type information */
FILE *console;
fd_set_size_t fd_width;
int Fvwm_fd[2];
int x_fd;

/* X related things */
Display *dpy;
Window Root;
Window  win;
int screen;
int ScreenWidth;
int ScreenHeight;
Pixel back[MAX_COLOUR_SETS];
Pixel fore[MAX_COLOUR_SETS];
GC graph[MAX_COLOUR_SETS];
GC shadow[MAX_COLOUR_SETS];
GC hilite[MAX_COLOUR_SETS];
GC background[MAX_COLOUR_SETS];
Pixmap win_bg;
XFontStruct *ButtonFont;
#ifdef I18N_MB
XFontSet ButtonFontset;
#endif
int fontheight;
int buttonheight;
static Atom wm_del_win;
Atom MwmAtom = None;

/* Module related information */
char *Module;
int WindowState=0;
int win_width=5;
int win_height=5;
int win_grav;
int win_x;
int win_y;
int win_title;
#ifdef COMPLEX_WINDOW_PLACEMENT
int win_border_x=0;
int win_border_y=0;
#endif
int Clength;
int Transient=0;
int Pressed=0;
int ButPressed;
int Checked=0;
int MinWidth=DEFMINWIDTH;
int MaxWidth=DEFMAXWIDTH;
ButtonArray buttons;
List windows;
char *ClickAction[NUMBER_OF_BUTTONS] =
{
  "Iconify -1,Raise",
  "Iconify",
  "Lower"
},
  *EnterAction,
  *BackColor[MAX_COLOUR_SETS] = { "white" },
  *ForeColor[MAX_COLOUR_SETS] = { "black" },
  *geometry="";
int colorset[MAX_COLOUR_SETS];
Pixmap pixmap[MAX_COLOUR_SETS];
char *font_string = "fixed";
Bool UseSkipList = False;
Bool Anchor = True;
Bool UseIconNames = False;
Bool LeftJustify = False;
Bool TruncateLeft = False;
Bool ShowFocus = True;
Bool Follow = False;
int ReliefWidth = 2;

long CurrentDesk = 0;
int ShowCurrentDesk = 0;

static RETSIGTYPE TerminateHandler(int sig);

static char *AnimCommand = NULL;

/***************************************************************************
 * TerminateHandler - reentrant signal handler that ends the main event loop
 ***************************************************************************/
static RETSIGTYPE TerminateHandler(int sig)
{
  /*
   * This function might not return - it could "long-jump"
   * right out, so we need to do everything we need to do
   * BEFORE we call it ...
   */
  fvwmSetTerminate(sig);
}

/******************************************************************************
  Main - Setup the XConnection,request the window list and loop forever
    Based on main() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
int main(int argc, char **argv)
{
  char *temp, *s;
#ifdef HAVE_SIGACTION
  struct sigaction  sigact;
#endif
  int i;

  for (i = 3; i < NUMBER_OF_MOUSE_BUTTONS; i++)
  {
    ClickAction[i] = "Nop";
  }

  /* Save the program name for error messages and config parsing */
  temp = argv[0];
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  /* Setup my name */
  Module = safemalloc(strlen(temp)+2);
  strcpy(Module,"*");
  strcat(Module, temp);
  Clength = strlen(Module);

#ifdef I18N_MB
  setlocale(LC_CTYPE, "");
#endif
  /* Open the console for messages */
  OpenConsole();

  if((argc != 6)&&(argc != 7)) {
    fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",Module,
      VERSION);
    ConsoleMessage("%s Version %s should only be executed by fvwm!\n",Module,
      VERSION);
   exit(1);
  }


  if ((argc==7)&&(!strcasecmp(argv[6],"Transient")))
  {
    Transient=1;
  }

  Fvwm_fd[0] = atoi(argv[1]);
  Fvwm_fd[1] = atoi(argv[2]);

#ifdef HAVE_SIGACTION
#ifdef SA_INTERRUPT
  sigact.sa_flags = SA_INTERRUPT;
#else
  sigact.sa_flags = 0;
#endif
  sigemptyset(&sigact.sa_mask);
  sigaddset(&sigact.sa_mask, SIGTERM);
  sigaddset(&sigact.sa_mask, SIGPIPE);
  sigaddset(&sigact.sa_mask, SIGINT);
  sigact.sa_handler = TerminateHandler;
  sigaction(SIGPIPE, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGINT, &sigact, NULL);
#else
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGPIPE) |
                     sigmask(SIGINT)  |
                     sigmask(SIGTERM) );
#endif
  signal(SIGPIPE, TerminateHandler);
  signal(SIGTERM, TerminateHandler);
  signal(SIGINT, TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, True);
  siginterrupt(SIGTERM, True);
  siginterrupt(SIGINT, True);
#endif
#endif

  /* set default colorsets */
  colorset[0] = colorset[1] = 0;
  colorset[2] = colorset[3] = 1;
  AllocColorset(1);

  /* Parse the config file */
  ParseConfig();

  /* Setup the XConnection */
  StartMeUp();
  XSetErrorHandler(ErrorHandler);

  InitArray(&buttons,0,0,win_width, fontheight+2, ReliefWidth);
  InitList(&windows);

  fd_width = GetFdWidth();

  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SetMessageMask(Fvwm_fd,
#ifdef MINI_ICONS
		 M_MINI_ICON |
#endif
		 M_CONFIGURE_WINDOW | M_ADD_WINDOW | M_DESTROY_WINDOW |
		 M_WINDOW_NAME | M_ICON_NAME | M_DEICONIFY | M_ICONIFY |
		 M_END_WINDOWLIST | M_NEW_DESK | M_FOCUS_CHANGE |
		 M_CONFIG_INFO | M_SENDCONFIG);

  SendFvwmPipe(Fvwm_fd, "Send_WindowList",0);

  /* Recieve all messages from Fvwm */
  atexit(ShutMeDown);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(Fvwm_fd);

  /* Lock on send only for iconify and deiconify (for NoIconAction) */
  SetSyncMask(Fvwm_fd, M_DEICONIFY | M_ICONIFY);
  SetNoGrabMask(Fvwm_fd, M_DEICONIFY | M_ICONIFY);

  MainEventLoop();
  return 0;
}


/******************************************************************************
  MainEventLoop -  Read and redraw until we die, blocking when can't read
******************************************************************************/
void MainEventLoop(void)
{
  fd_set readset;

  while( !isTerminated ) {
    FD_ZERO(&readset);
    FD_SET(Fvwm_fd[1],&readset);
    FD_SET(x_fd,&readset);

    /* This code restyled after FvwmIconMan, which is simpler.
     * The biggest advantage over the original approach is
     * having one fewer select statements
     */
    XFlush(dpy);
    if (fvwmSelect(fd_width, &readset, NULL, NULL, NULL) > 0) {

      if (FD_ISSET(x_fd,&readset) || XPending(dpy)) LoopOnEvents();
      if (FD_ISSET(Fvwm_fd[1],&readset)) ReadFvwmPipe();

    }

  } /* while */
}

/******************************************************************************
  ReadFvwmPipe - Read a single message from the pipe from Fvwm
    Originally Loop() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ReadFvwmPipe(void)
{
    FvwmPacket* packet = ReadFvwmPacket(Fvwm_fd[1]);
    if ( packet == NULL )
	exit(0);
    else
	ProcessMessage( packet->type, packet->body );
}

/******************************************************************************
  ProcessMessage - Process the message coming from Fvwm
    Skeleton based on processmessage() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ProcessMessage(unsigned long type,unsigned long *body)
{
  int redraw=0,i,j;
  Item *flagitem;
  char *name,*string;
  static int current_focus=-1;
  struct ConfigWinPacket  *cfgpacket;

#ifdef MINI_ICONS
  Picture p;
#endif

  switch(type)
  {
    case M_ADD_WINDOW:
    case M_CONFIGURE_WINDOW:
      cfgpacket = (void *) body;
      /* We get the win_borders only when WinList  map it self, this is ok
       *  since we need it only after an unmap */
#ifdef COMPLEX_WINDOW_PLACEMENT
      if (cfgpacket->w == win)
      {
	win_border_x = win_border_y = cfgpacket->border_width;
	if (((win_grav == NorthWestGravity || win_grav == NorthEastGravity)
	     && !HAS_BOTTOM_TITLE(cfgpacket)) ||
	    ((win_grav == SouthWestGravity || win_grav == SouthEastGravity)
	    && HAS_BOTTOM_TITLE(cfgpacket)))
	  win_border_y +=  cfgpacket->title_height;
      }
#endif
      if ((i = FindItem(&windows,cfgpacket->w))!=-1)
      {
	flagitem=ItemFlags(&windows,cfgpacket->w);
	if(UpdateItemDesk(&windows, cfgpacket) > 0 ||
	   IS_STICKY(cfgpacket) != IS_STICKY(flagitem) ||
	   DO_SKIP_WINDOW_LIST(cfgpacket) != DO_SKIP_WINDOW_LIST(flagitem))
        {
	  UpdateItemGSFRFlags(&windows, cfgpacket);
          UpdateButtonDeskFlags(&buttons, i, cfgpacket->desk,
				IS_STICKY(cfgpacket),
				DO_SKIP_WINDOW_LIST(cfgpacket));
          AdjustWindow(False);
          RedrawWindow(True, True);
        }
	else
	  UpdateItemGSFRFlags(&windows, cfgpacket);
	break;
      }
      else
        AddItem(&windows, cfgpacket);
      break;
    case M_DESTROY_WINDOW:
      cfgpacket = (void *) body;
      if ((i=DeleteItem(&windows,cfgpacket->w))==-1) break;
      RemoveButton(&buttons,i);
      if (WindowState)
        AdjustWindow(False);
      redraw=1;
      break;

#ifdef MINI_ICONS
  case M_MINI_ICON:
    if ((i=FindItem(&windows,body[0]))==-1) break;

    if (UpdateButton(&buttons,i,NULL,-1)!=-1)
    {
      p.width = body[3];
      p.height = body[4];
      p.depth = body[5];
      p.picture = body[6];
      p.mask = body[7];

      UpdateButtonPicture(&buttons, i, &p);

      AdjustWindow(False);
      redraw = 1;
    }
    break;
#endif

    case M_WINDOW_NAME:
    case M_ICON_NAME:
      if ((type == M_ICON_NAME) ^ UseIconNames)
	break;
      string=(char *)&body[3];
      if ((i=UpdateItemName(&windows,body[0],string))==-1) break;
      flagitem = ItemFlags(&windows,body[0]);
      name=makename(string, (IS_ICONIFIED(flagitem)?True:False));

      if (UpdateButton(&buttons,i,name,-1)==-1)
      {
	AddButton(&buttons, name, NULL, 1);
	UpdateButtonSet(&buttons, i, (IS_ICONIFIED(flagitem)?1:0));
        UpdateButtonDeskFlags(&buttons,i,ItemDesk(&windows, body[0]),
			      IS_STICKY(flagitem),
			      DO_SKIP_WINDOW_LIST(flagitem));
      }
      free(name);
      if (WindowState) AdjustWindow(False);
      redraw=1;
      break;
    case M_DEICONIFY:
    case M_ICONIFY:
      /* fwvm will wait for an Unlock message before continuing
       * be careful when changing this construct, make sure unlock happens */
      if ((i = FindItem(&windows, body[0])) != -1) {
	flagitem = ItemFlags(&windows, body[0]);
	if ((type == M_DEICONIFY && IS_ICONIFIED(flagitem))
	    || (type == M_ICONIFY && !IS_ICONIFIED(flagitem))) {
	  if (IS_ICON_SUPPRESSED(flagitem) && IsItemIndexVisible(&windows,i)
	      && AnimCommand && WindowState && AnimCommand[0] != 0) {
	    char buff[MAX_MODULE_INPUT_TEXT_LEN];
	    Window child;
	    int x, y;

	    /* find out where our button is */
	    j = FindItemVisible(&windows, body[0]);
	    XTranslateCoordinates(dpy, win, Root, 0, j * buttonheight,
				  &x, &y, &child);
	    /* tell FvwmAnimate to animate to our button */
	    if (IS_ICONIFIED(flagitem)) {
	      sprintf(buff, "%s %d %d %d %d %d %d %d %d", AnimCommand,
		      x, y, win_width, buttonheight,
		      (int)body[7], (int)body[8], (int)body[9],
		      (int)body[10]);
	    } else {
	      sprintf(buff, "%s %d %d %d %d %d %d %d %d", AnimCommand,
		      (int)body[7], (int)body[8], (int)body[9], (int)body[10],
		      x, y, win_width, buttonheight);
	    }
	    SendText(Fvwm_fd, buff, 0);
	  }
	  SET_ICONIFIED(flagitem, !IS_ICONIFIED(flagitem));
	  string = ItemName(&windows, i);
	  name = makename(string, IS_ICONIFIED(flagitem));
          UpdateButtonIconified(&buttons, i, IS_ICONIFIED(flagitem));
	  if (UpdateButton(&buttons, i, name, -1) != -1) redraw = 1;
	  if (i != current_focus || (IS_ICONIFIED(flagitem)))
	    if (UpdateButtonSet(&buttons, i, (IS_ICONIFIED(flagitem)) ? 1 : 0)
		!= -1)
	      redraw = 1;
	  free(name);
	}
      }
      SendUnlockNotification(Fvwm_fd);
      break;
    case M_FOCUS_CHANGE:
      redraw = 1;
      if ((i=FindItem(&windows,body[0]))!=-1)
      {
        RadioButton(&buttons,i,ButPressed);
        if (Follow && i) { /* rearrange order */
          ReorderList(&windows,i,body[2]);
          if (WindowState) ReorderButtons(&buttons,i,body[2]);
        }
      }
      else
        RadioButton(&buttons,-1,ButPressed);
      break;
    case M_END_WINDOWLIST:
      if (!WindowState)
	MakeMeWindow();
      redraw = 1;
      break;
    case M_NEW_DESK:
      CurrentDesk = body[0];
      if(ShowCurrentDesk)
      {
        AdjustWindow(False);
        RedrawWindow(True, True);
      }
      break;
    case M_CONFIG_INFO:
      ParseConfigLine((char *)&body[3]);
      break;
  }

  if (redraw && WindowState==1) RedrawWindow(False, True);
}


/******************************************************************************
  RedrawWindow - Update the needed lines and erase any old ones
******************************************************************************/
void RedrawWindow(Bool force, Bool clear_bg)
{
  DrawButtonArray(&buttons, force, clear_bg);
  XFlush(dpy); /** Put here for VMS; ifdef if causes performance problem **/
  if (XQLength(dpy) && !force) LoopOnEvents();
}

/******************************************************************************
  ConsoleMessage - Print a message on the console.  Works like printf.
******************************************************************************/
void ConsoleMessage(const char *fmt, ...)
{
#ifndef NO_CONSOLE
  va_list args;
  FILE *filep;

  if (console==NULL) filep=stderr;
  else filep=console;
  va_start(args,fmt);
  vfprintf(filep,fmt,args);
  va_end(args);
#endif
}

/******************************************************************************
  OpenConsole - Open the console as a way of sending messages
******************************************************************************/
int OpenConsole(void)
{
#ifndef NO_CONSOLE
  if ((console=fopen("/dev/console","w"))==NULL) {
    fprintf(stderr,"%s: cannot open console\n",Module);
    return 0;
  }
#endif
  return 1;
}

/******************************************************************************
  ParseConfig - Parse the configuration file fvwm to us to use
    Based on part of main() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ParseConfig(void)
{
  char *tline;

  InitGetConfigLine(Fvwm_fd,Module);
  GetConfigLine(Fvwm_fd,&tline);
  while(tline != (char *)0)
    {
      ParseConfigLine(tline);
      GetConfigLine(Fvwm_fd,&tline);
    }
}

void
ParseConfigLine(char *tline)
{
  static Bool has_icon_back = False;
  static Bool has_icon_fore = False;
  static Bool has_icon_cset = False;

  if (!tline)
    return;

  if (strlen(tline) > 1) {
    if (strncasecmp(tline, CatString3(Module, "Font", ""), Clength + 4)	== 0)
      CopyString(&font_string, &tline[Clength + 4]);
    else if (strncasecmp(tline, CatString3(Module, "Fore", ""), Clength + 4)
	     == 0) {
      CopyString(&ForeColor[0], &tline[Clength + 4]);
      colorset[0] = -1;
      if (!has_icon_cset)
	colorset[1] = -1;
      if (!has_icon_fore)
	CopyString(&ForeColor[1], &tline[Clength + 4]);
    } else if (strncasecmp(tline, CatString3(Module, "IconFore", ""),
			   Clength + 8) == 0) {
      CopyString(&ForeColor[1], &tline[Clength + 8]);
      if (colorset[0] >= 0)
	colorset[1] = colorset[0];
      else
	colorset[1] = -1;
      has_icon_fore = True;
      has_icon_cset = False;
    } else if (strncasecmp(tline, CatString3(Module, "FocusFore", ""),
			   Clength + 9) == 0) {
      CopyString(&ForeColor[2], &tline[Clength + 9]);
      CopyString(&ForeColor[3], &tline[Clength + 9]);
      colorset[2] = colorset[3] = -1;
    } else if (strncasecmp(tline, CatString3(Module, "Geometry", ""),
			   Clength + 8) == 0)
      CopyString(&geometry, &tline[Clength + 8]);
    else if (strncasecmp(tline, CatString3(Module, "Back", ""), Clength + 4)
			 == 0) {
      CopyString(&BackColor[0], &tline[Clength + 4]);
      colorset[0] = -1;
      if (!has_icon_cset)
	colorset[1] = -1;
      if (!has_icon_back)
	CopyString(&BackColor[1], &tline[Clength + 4]);
    } else if (strncasecmp(tline, CatString3(Module, "IconBack", ""),
			   Clength + 8) == 0) {
      CopyString(&BackColor[1], &tline[Clength + 8]);
      if (colorset[0] >= 0)
	colorset[1] = colorset[0];
      else
	colorset[1] = -1;
      has_icon_fore = True;
      has_icon_cset = False;
    } else if (strncasecmp(tline, CatString3(Module, "FocusBack", ""),
			   Clength + 9) == 0) {
      CopyString(&BackColor[2], &tline[Clength + 9]);
      CopyString(&BackColor[3], &tline[Clength + 9]);
      colorset[2] = colorset[3] = -1;
    } else if (strncasecmp(tline, CatString3(Module, "NoAnchor", ""),
			   Clength + 8) == 0)
      Anchor = False;
    else if (strncasecmp(tline, CatString3(Module, "Action", ""), Clength + 6)
			 == 0)
      LinkAction(&tline[Clength + 6]);
    else if (strncasecmp(tline, CatString3(Module, "UseSkipList", ""),
			 Clength + 11) == 0)
      UseSkipList = True;
    else if (strncasecmp(tline, CatString3(Module, "UseIconNames", ""),
			 Clength + 12) == 0)
      UseIconNames = True;
    else if (strncasecmp(tline, CatString3(Module, "ShowCurrentDesk", ""),
			 Clength + 15) == 0)
      ShowCurrentDesk = 1;
    else if (strncasecmp(tline, CatString3(Module, "LeftJustify", ""),
			 Clength + 11) == 0)
      LeftJustify = True;
    else if (strncasecmp(tline, CatString3(Module, "TruncateLeft", ""),
			 Clength + 12) == 0)
      TruncateLeft = True;
    else if (strncasecmp(tline, CatString3(Module, "MinWidth", ""),
			 Clength + 8) == 0)
      MinWidth = atoi(&tline[Clength + 8]);
    else if (strncasecmp(tline, CatString3(Module, "MaxWidth", ""),
			 Clength + 8) == 0)
      MaxWidth = atoi(&tline[Clength + 8]);
    else if (strncasecmp(tline, CatString3(Module, "DontDepressFocus", ""),
			 Clength + 16) == 0)
      ShowFocus = False;
    else if (strncasecmp(tline, CatString3(Module, "FollowWindowList", ""),
			 Clength + 16) == 0)
      Follow = True;
    else if (strncasecmp(tline, CatString3(Module, "ButtonFrameWidth", ""),
			 Clength + 16) == 0) {
      ReliefWidth = atoi(&tline[Clength + 16]);
      buttonheight = fontheight + 3 + 2 * ReliefWidth;
    } else if (strncasecmp(tline, CatString3(Module, "NoIconAction", ""),
			 Clength + 12) == 0) {
      CopyString(&AnimCommand, &tline[Clength + 12]);
      if (strlen(AnimCommand) > MAX_NO_ICON_ACTION_LENGTH)
	AnimCommand[MAX_NO_ICON_ACTION_LENGTH] = 0;
    } else if (strncasecmp(tline, CatString3(Module, "Colorset", ""),
			 Clength + 8) == 0) {
      colorset[0] = atoi(&tline[Clength + 8]);
      AllocColorset(colorset[0]);
    } else if (strncasecmp(tline, CatString3(Module, "IconColorset", ""),
			 Clength + 12) == 0)
    {
      colorset[1] = atoi(&tline[Clength + 12]);
      AllocColorset(colorset[1]);
      has_icon_cset = True;
    }
    else if (strncasecmp(tline, CatString3(Module, "FocusColorset", ""),
			 Clength + 13) == 0)
    {
      colorset[2] = colorset[3] = atoi(&tline[Clength + 13]);
      AllocColorset(colorset[3]);
      has_icon_cset = True;
    }
    else if (strncasecmp(tline, "Colorset", 8) == 0) {
      int cset = LoadColorset(&tline[8]);

      if (WindowState && (cset >= 0)) {
	int i;
	Pixmap old_win_bg = win_bg;

	/* set the window background */
	win_bg = None;
	for (i = 0; i != MAX_COLOUR_SETS; i++)
	    if (colorset[i] >= 0 &&
		Colorset[colorset[i]].pixmap == ParentRelative)
	      win_bg = ParentRelative;
	if (old_win_bg != win_bg)
	  XSetWindowBackgroundPixmap(dpy, win, win_bg);
	/* refresh if this colorset is used */
	for (i = 0; i != MAX_COLOUR_SETS; i++) {
	  if (colorset[i] == cset) {
	    AdjustWindow(True);
	    RedrawWindow(True, True);
	    break;
	  }
	}
      }
    }
  }
}


/******************************************************************************
  LoopOnEvents - Process all the X events we get
******************************************************************************/
void LoopOnEvents(void)
{
  int num;
  char buffer[10];
  XEvent Event;
#if 1
  Window dummyroot,dummychild;
  int x,x1,y,y1;
  unsigned int dummy1;

  if (Transient && !Checked)
  {
    if (Pressed)
    {
      XQueryPointer(dpy,win,&dummyroot,&dummychild,&x1,&y1,&x,&y,&dummy1);
      num=WhichButton(&buttons,x,y);
      if (num!=-1)
      {
	Pressed=1;
	ButPressed=num;
	UpdateButton(&buttons, num, NULL, 0);
      }
      else
      {
	Pressed=0;
      }
    }
    Checked=1;
  }
#endif

  while(XPending(dpy))
  {
    XNextEvent(dpy,&Event);

    switch(Event.type)
    {
      case ButtonRelease:
        if (Pressed)
        {
          num=WhichButton(&buttons,Event.xbutton.x,Event.xbutton.y);
          if (num!=-1 && Event.xbutton.button >= 1 &&
	      Event.xbutton.button <= NUMBER_OF_MOUSE_BUTTONS)
	  {
            SendFvwmPipe(Fvwm_fd,
			 ClickAction[(Transient) ? 0:Event.xbutton.button-1],
			 ItemID(&windows,num));
            SwitchButton(&buttons,num);
          }
        }
        if (Transient)
	{
	  exit(0);
	}
        Pressed=0;
        ButPressed=-1;
        break;
      case ButtonPress:
        num=WhichButton(&buttons,Event.xbutton.x,Event.xbutton.y);
        if (num != -1)
        {
          SwitchButton(&buttons,num);
          ButPressed=num;
        } else ButPressed=-1;
        Pressed=1;
        break;
      case Expose:
	/* have to clear each button with its background,
	 * let the server compute the intersections */
	do
	  ExposeAllButtons(&buttons, &Event);
	while (XCheckTypedWindowEvent(dpy, win, Expose, &Event));
        /* No more expose events in the queue draw all icons,
         * text and shadows, but not the background */
        RedrawWindow(True, False);
	break;
      case ConfigureNotify:
	{
	  XEvent event;

	/* Opaque moves cause lots of these to be sent which causes flickering
	 * It's better to miss out on intermediate steps than to do them all
	 * and take too long. Look down the event queue and do the last one */
	  while (XCheckTypedWindowEvent(dpy, win, ConfigureNotify, &event))
	  {
	    /* if send_event is true it means the user has moved the window or
	     * winlist has mapped itself, take note of the new position.
	     * If it is false it comes from the Xserver and is bogus */
	    if (!event.xconfigure.send_event)
	      continue;
	    Event.xconfigure.x = event.xconfigure.x;
	    Event.xconfigure.y = event.xconfigure.y;
	    Event.xconfigure.send_event = True;
	  }
	}
        if (Event.xconfigure.send_event && (win_x != Event.xconfigure.x
            || win_y != Event.xconfigure.y)) {
	  win_x = Event.xconfigure.x;
	  win_y = Event.xconfigure.y;
	  if (win_bg == ParentRelative)
	    RedrawWindow(True, True);
	}
        break;
      case KeyPress:
        num=XLookupString(&Event.xkey,buffer,10,NULL,0);
        if (num==1)
        {
          if (buffer[0]=='q' || buffer[0]=='Q') exit(0);
          else if (buffer[0]=='i' || buffer[0]=='I') PrintList(&windows);
          else if (buffer[0]=='b' || buffer[0]=='B') PrintButtons(&buttons);
        }
        break;
      case ClientMessage:
        if ((Event.xclient.format==32) && (Event.xclient.data.l[0]==wm_del_win))
          exit(0);
      case EnterNotify:
        if (!SomeButtonDown(Event.xcrossing.state)) break;
        num=WhichButton(&buttons,Event.xcrossing.x,Event.xcrossing.y);
        if (num!=-1)
        {
          SwitchButton(&buttons,num);
          ButPressed=num;
        } else ButPressed=-1;
        Pressed=1;
        break;
      case LeaveNotify:
        if (!SomeButtonDown(Event.xcrossing.state)) break;
        if (ButPressed!=-1) SwitchButton(&buttons,ButPressed);
        Pressed=0;
        break;
      case MotionNotify:
        if (!Pressed) break;
        num=WhichButton(&buttons,Event.xmotion.x,Event.xmotion.y);
        if (num==ButPressed) break;
        if (ButPressed!=-1) SwitchButton(&buttons,ButPressed);
        if (num!=-1)
        {
          SwitchButton(&buttons,num);
          ButPressed=num;
        }
        else ButPressed=-1;

        break;
    }
  }
}


/******************************************************************************
  AdjustWindow - Resize the window according to maxwidth by number of buttons
******************************************************************************/
void AdjustWindow(Bool force)
{
  int new_width=0,new_height=0,tw,i,total,all;
  char *temp;
#ifdef MINI_ICONS
  int base_miw = 0;
#endif

  total = ItemCountVisible(&windows);
  all = ItemCount(&windows);
  if (!total)
  {
    if (WindowState==1)
    {
      XUnmapWindow(dpy,win);
      WindowState=2;
    }
    return;
  }

#ifdef MINI_ICONS
  for(i=0; i<all; i++)
  {
    if(IsButtonIndexVisible(&buttons,i) &&
       ButtonPicture(&buttons,i)->picture != None)
    {
      if (ButtonPicture(&buttons,i)->width > base_miw)
	base_miw = ButtonPicture(&buttons,i)->width;
      break;
    }
  }
  base_miw += 2;
#endif
  for(i=0;i<all;i++)
  {
    temp=ItemName(&windows,i);
    if(temp != NULL && IsItemIndexVisible(&windows,i))
    {
	tw = 8 + XTextWidth(ButtonFont,temp,strlen(temp));
	tw += XTextWidth(ButtonFont,"()",2);
#ifdef MINI_ICONS
	tw += base_miw;
#endif
	new_width=max(new_width,tw);
    }
  }
  new_width=max(new_width, MinWidth);
  new_width=min(new_width, MaxWidth);
  new_height=(total * buttonheight);

  if (WindowState && force) {
    for (i = 0; i != MAX_COLOUR_SETS; i++) {
      int cset = colorset[i];

      if (cset >= 0) {
	fore[i] = Colorset[cset].fg;
	back[i] = Colorset[cset].bg;
	XSetForeground(dpy, graph[i], fore[i]);
	XSetForeground(dpy, background[i], back[i]);
	XSetForeground(dpy, hilite[i], Colorset[cset].hilite);
	XSetForeground(dpy, shadow[i], Colorset[cset].shadow);
      }
    }
  }
  /* change the pixmap if forced or if width changed (the height is fixed) */
  if (WindowState && (force || new_width != win_width)) {
    for (i = 0; i != MAX_COLOUR_SETS; i++) {
      int cset = colorset[i];

      if (cset >= 0) {
	if (pixmap[i])
	  XFreePixmap(dpy, pixmap[i]);
	if (Colorset[cset].pixmap) {
	  pixmap[i] = CreateBackgroundPixmap(dpy, win, new_width, buttonheight,
					     &Colorset[cset], Pdepth,
					     background[i], False);
	  XSetTile(dpy, background[i], pixmap[i]);
	  XSetFillStyle(dpy, background[i], FillTiled);
	} else {
	  pixmap[i] = None;
	  XSetFillStyle(dpy, background[i], FillSolid);
	}
      }
    }
  }
  if (WindowState && (new_height!=win_height || new_width!=win_width))
  {
#ifdef COMPLEX_WINDOW_PLACEMENT
    if (Anchor)
    {
      /* compensate for fvwm borders when going from unmapped to mapped */
      int map_x_adjust = (WindowState - 1) * win_border_x;
      int map_y_adjust = (WindowState - 1) * win_border_y;

      switch (win_grav) {
	case NorthWestGravity:
	case SouthWestGravity:
	  win_x -= map_x_adjust;
	  break;
	case NorthEastGravity:
	case SouthEastGravity:
	  win_x += win_width - new_width + map_x_adjust;
	  break;
      }

      switch (win_grav) {
	case NorthWestGravity:
	case NorthEastGravity:
	  win_y -= map_y_adjust;
	  break;
	case SouthEastGravity:
	case SouthWestGravity:
	  win_y += win_height - new_height + map_y_adjust;
	  break;
      }
      /* XMoveResizeWindow(dpy, win, win_x, win_y, new_width, new_height); */
    }
    if (Anchor && WindowState == 2)
      XMoveResizeWindow(dpy, win, win_x, win_y, new_width, new_height);
    else
      XResizeWindow(dpy, win, new_width, new_height);
    /* The new code is sooo much simpler :-) */
#else
    /* This relies on fvwm honouring South/East gemoetry when resizing. */
    XResizeWindow(dpy, win, new_width, new_height);
#endif
  }
  UpdateArray(&buttons,new_width);
  if (new_height>0) win_height = new_height;
  if (new_width>0) win_width = new_width;
  if (WindowState==2)
  {
    XMapWindow(dpy,win);
    WindowState=1;
  }
}

/******************************************************************************
  makename - Based on the flags return me '(name)' or 'name'
******************************************************************************/
char *makename(const char *string, Bool iconified)
{
  char *ptr;

  if (!iconified)
    return safestrdup(string);
  ptr=safemalloc(strlen(string)+3);
  sprintf(ptr, "(%s)", string);
  return ptr;
}

/******************************************************************************
  LinkAction - Link an response to a users action
******************************************************************************/
void LinkAction(char *string)
{
char *temp;
  temp=string;
  while(isspace((unsigned char)*temp))
    temp++;
  if(strncasecmp(temp, "Click", 5)==0)
  {
    int n;
    int b;
    int i;

    i = sscanf(temp + 5, "%d%n", &b, &n);
    if (i > 0 && b >=1 && b <= NUMBER_OF_MOUSE_BUTTONS)
    {
      CopyString(&ClickAction[b - 1], temp + 5 + n);
    }
  }
  else if(strncasecmp(temp, "Enter", 5)==0)
    CopyString(&EnterAction,&temp[5]);
}

/******************************************************************************
  MakeMeWindow - Create and setup the window we will need
******************************************************************************/
void MakeMeWindow(void)
{
  XSizeHints hints;
  XGCValues gcval;
  unsigned long gcmask;
  unsigned int dummy1, dummy2;
  int x, y, ret, count;
  Window dummyroot, dummychild;
  int i;
  XSetWindowAttributes attr;

  if ((count = ItemCountVisible(&windows))==0 && Transient)
  {
    exit(0);
  }
  AdjustWindow(False);

  hints.width=win_width;
  hints.height=win_height;
  hints.win_gravity=NorthWestGravity;
  hints.flags=PSize|PWinGravity;

  if (geometry!= NULL)
  {
    ret=XineramaSupportParseGeometry(geometry,&x,&y,&dummy1,&dummy2);

    if ((ret&XValue) && (ret&YValue))
    {
      hints.x=x;
      if (ret&XNegative)
        hints.x+=XDisplayWidth(dpy,screen)-win_width;

      hints.y=y;
      if (ret&YNegative)
        hints.y+=XDisplayHeight(dpy,screen)-win_height;

      hints.flags|=USPosition;
    }

    if (ret&XNegative)
    {
      if (ret&YNegative)
	hints.win_gravity=SouthEastGravity;
      else
	hints.win_gravity=NorthEastGravity;
    }
    else
    {
      if (ret&YNegative)
	hints.win_gravity=SouthWestGravity;
      else
	hints.win_gravity=NorthWestGravity;
    }

#ifndef COMPLEX_WINDOW_PLACEMENT
    if (!Anchor)
    {
      if (hints.win_gravity == SouthWestGravity)
	hints.win_gravity=NorthWestGravity;
      else if (hints.win_gravity == SouthEastGravity)
	hints.win_gravity=NorthEastGravity;
    }
#endif
  }

  if (Transient)
  {
    XQueryPointer(dpy,Root,&dummyroot,&dummychild,&hints.x,&hints.y,&x,&y,
		  &dummy1);
    hints.x -= hints.width / 2;
    hints.y -= buttonheight / 2;
    if (hints.x + hints.width > ScreenWidth)
    {
      hints.x = ScreenWidth - hints.width;
    }
    if (hints.x < 0)
    {
      hints.x = 0;
    }
    if (hints.y + hints.height > ScreenHeight)
    {
      hints.y = ScreenHeight - hints.height;
    }
    if (hints.y < 0)
    {
      hints.y = 0;
    }
    hints.win_gravity = NorthWestGravity;
    hints.flags |= USPosition;
  }
  win_x = hints.x;
  win_y = hints.y;
  win_grav = hints.win_gravity;


  for (i = 0; i != MAX_COLOUR_SETS; i++)
    if(Pdepth < 2)
    {
      back[i] = GetColor("white");
      fore[i] = GetColor("black");
    }
    else
    {
      if (colorset[i] >= 0) {
	back[i] = Colorset[colorset[i]].bg;
	fore[i] = Colorset[colorset[i]].fg;
      } else {
	back[i] = GetColor(BackColor[i] == NULL ? BackColor[0] : BackColor[i]);
	fore[i] = GetColor(ForeColor[i] == NULL ? ForeColor[0] : ForeColor[i]);
      }
    }

  /* FvwmWinList paints the entire window and does not rely on the xserver to
   * render the background. Setting the background_pixmap to None prevents the
   * server from doing anything on expose events. The exception is when one of
   * the colorsets is transparent which requires help from the server */
  win_bg = None;
  for (i = 0; i != MAX_COLOUR_SETS; i++)
    if (colorset[i] >= 0 && Colorset[colorset[i]].pixmap == ParentRelative)
    {
      win_bg = ParentRelative;
      break;
    }
  attr.background_pixmap = win_bg;
  attr.border_pixel = 0;
  attr.colormap = Pcmap;
  win=XCreateWindow(dpy, Root, hints.x, hints.y, hints.width, hints.height, 0,
		    Pdepth, InputOutput, Pvisual,
		    CWBackPixmap | CWBorderPixel | CWColormap, &attr);


  wm_del_win=XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(dpy,win,&wm_del_win,1);

  XSetWMNormalHints(dpy,win,&hints);

  if (!Transient)
  {
    XGrabButton(dpy,1,AnyModifier,win,True,GRAB_EVENTS,GrabModeAsync,
      GrabModeAsync,None,None);
    XGrabButton(dpy,2,AnyModifier,win,True,GRAB_EVENTS,GrabModeAsync,
      GrabModeAsync,None,None);
    XGrabButton(dpy,3,AnyModifier,win,True,GRAB_EVENTS,GrabModeAsync,
      GrabModeAsync,None,None);
    SetMwmHints(
      MWM_DECOR_ALL|MWM_DECOR_RESIZEH|MWM_DECOR_MAXIMIZE|MWM_DECOR_MINIMIZE,
      MWM_FUNC_ALL|MWM_FUNC_RESIZE|MWM_FUNC_MAXIMIZE|MWM_FUNC_MINIMIZE,
      MWM_INPUT_MODELESS);
  }
  else
  {
    SetMwmHints(0,MWM_FUNC_ALL,MWM_INPUT_MODELESS);
  }

  for (i = 0; i != MAX_COLOUR_SETS; i++)
  {
    gcval.foreground=fore[i];
    gcval.background=back[i];
    gcval.font=ButtonFont->fid;
    gcmask=GCForeground|GCBackground|GCFont;
    graph[i]=fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

    if(Pdepth < 2)
      gcval.foreground=GetShadow(fore[i]);
    else
      if (colorset[i] < 0)
	gcval.foreground=GetShadow(back[i]);
      else
	gcval.foreground=Colorset[colorset[i]].shadow;
    gcval.background=back[i];
    gcmask=GCForeground|GCBackground;
    shadow[i]=fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

    if (colorset[i] < 0)
      gcval.foreground=GetHilite(back[i]);
    else
      gcval.foreground=Colorset[colorset[i]].hilite;
    gcval.background=back[i];
    gcmask=GCForeground|GCBackground;
    hilite[i]=fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

    gcval.foreground=back[i];
    gcmask=GCForeground;
    background[i]=fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

    if ((colorset[i] >= 0) && Colorset[colorset[i]].pixmap
        && Colorset[colorset[i]].pixmap != ParentRelative) {
      pixmap[i] = CreateBackgroundPixmap(dpy, win, win_width, buttonheight,
					 &Colorset[colorset[i]], Pdepth,
					 background[i], False);
      XSetTile(dpy, background[i], pixmap[i]);
      XSetFillStyle(dpy, background[i], FillTiled);
    }
  }
  AdjustWindow(True);
  XSelectInput(dpy,win,(StructureNotifyMask | ExposureMask | KeyPressMask));

  ChangeWindowName(&Module[1]);

  if (ItemCountVisible(&windows) > 0)
  {
    XMapRaised(dpy,win);
    WindowState=1;
  } else WindowState=2;

  if (Transient)
  {
    int i = 0;

    XSync(dpy,0);
    while (i < 1000 &&
	   XGrabPointer(dpy, win, True,
			ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
			PointerMotionMask|EnterWindowMask|LeaveWindowMask,
			GrabModeAsync, GrabModeAsync, None,
			None, CurrentTime) != GrabSuccess)
    {
      i++;
      /* If you go too fast, other windows may not get a change to release
       * any grab that they have. */
      usleep(1000);
    }
    XSync(dpy,0);
    if (i >= 1000)
    {
      fprintf(stderr,"failed to grab pointer\n");
      exit(1);
    }
    XQueryPointer(
      dpy,Root,&dummyroot,&dummychild,&hints.x,&hints.y,&x,&y,&dummy1);
    Pressed = !!SomeButtonDown(dummy1);
  }
}

/******************************************************************************
  StartMeUp - Do X initialization things
******************************************************************************/
void StartMeUp(void)
{
#ifdef I18N_MB
  char **ml;
  int mc;
  char *ds;
  XFontStruct **fs_list;
#endif

  if (!(dpy = XOpenDisplay("")))
  {
    fprintf(stderr,"%s: can't open display %s", Module,
      XDisplayName(""));
    exit (1);
  }
  InitPictureCMap(dpy);
  XineramaSupportInit(dpy);
  AllocColorset(0);
  x_fd = XConnectionNumber(dpy);
  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);

  ScreenHeight = DisplayHeight(dpy,screen);
  ScreenWidth = DisplayWidth(dpy,screen);

#ifdef I18N_MB
  if ((ButtonFontset=XCreateFontSet(dpy,font_string,&ml,&mc,&ds)) == NULL) {
#ifdef STRICTLY_FIXED
    if ((ButtonFontset=XCreateFontSet(dpy,"fixed",&ml,&mc,&ds)) == NULL)
    {
      exit(1);
    }
#else
    if ((ButtonFontset=XCreateFontSet(dpy,"-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*",&ml,&mc,&ds)) == NULL)
    {
      exit(1);
    }
#endif
  }
  XFontsOfFontSet(ButtonFontset,&fs_list,&ml);
  ButtonFont = fs_list[0];
#else
  if ((ButtonFont=XLoadQueryFont(dpy,font_string))==NULL)
  {
    if ((ButtonFont=XLoadQueryFont(dpy,"fixed"))==NULL)
    {
      exit(1);
    }
  }
#endif

  fontheight = ButtonFont->ascent+ButtonFont->descent;
  buttonheight = fontheight + 3 + 2 * ReliefWidth;

  win_width=XTextWidth(ButtonFont,"XXXXXXXXXXXXXXX",10);

}

/******************************************************************************
  ShutMeDown - Do X client cleanup
******************************************************************************/
void ShutMeDown(void)
{
  FreeList(&windows);
  FreeAllButtons(&buttons);
/*  XFreeGC(dpy,graph);*/
  if (WindowState) XDestroyWindow(dpy,win);
  XCloseDisplay(dpy);
}

/******************************************************************************
  ChangeWindowName - Self explanitory
    Original work from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ChangeWindowName(char *str)
{
  XTextProperty name;
  if (XStringListToTextProperty(&str,1,&name) == 0) {
    fprintf(stderr,"%s: cannot allocate window name.\n",Module);
    return;
  }
  XSetWMName(dpy,win,&name);
  XSetWMIconName(dpy,win,&name);
  XFree(name.value);
}

/**************************************************************************
 *
 * Sets mwm hints
 *
 *************************************************************************/
/*
 *  Now, if we (hopefully) have MWW - compatible window manager ,
 *  say, mwm, ncdwm, or else, we will set useful decoration style.
 *  Never check for MWM_RUNNING property.May be considered bad.
 */

void SetMwmHints(unsigned int value, unsigned int funcs, unsigned int input)
{
PropMwmHints prop;

  if (MwmAtom==None)
    {
      MwmAtom=XInternAtom(dpy,"_MOTIF_WM_HINTS",False);
    }
  if (MwmAtom!=None)
    {
      /* sh->mwm.decorations contains OR of the MWM_DECOR_XXXXX */
      prop.decorations= value;
      prop.functions = funcs;
      prop.inputMode = input;
      prop.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS |
	MWM_HINTS_INPUT_MODE;

      /* HOP - LA! */
      XChangeProperty (dpy,win,
		       MwmAtom, MwmAtom,
		       32, PropModeReplace,
		       (unsigned char *)&prop,
		       PROP_MWM_HINTS_ELEMENTS);
    }
}

/************************************************************************
  X Error Handler
************************************************************************/
int ErrorHandler(Display *d, XErrorEvent *event)
{
  /* some errors are OK=ish */
  if (event->error_code == BadPixmap)
    return 0;
  if (event->error_code == BadDrawable)
    return 0;

  PrintXErrorAndCoredump(d, event, Module);
  return 0;
}

