/* FvwmWinList Module for Fvwm.
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * The author makes not guarantees or warantees, either express or
 * implied.  Feel free to use any contained here for any purpose, as long
 * and this and any other applicible copyrights are kept intact.

 * The functions in this source file that are based on part of the FvwmIdent
 * module for Fvwm are noted by a small copyright atop that function, all others
 * are copyrighted by Mike Finger.  For those functions modified/used, here is
 *  the full, origonal copyright:
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.

 * Modifications Done to Add Pixmaps, focus highlighting and Current Desk Only
 * By Don Mahurin, 1996, Some of this Code was taken from FvwmTaskBar

 * Bug Notes:(Don Mahurin)

 * Moving a window doesnt send M_CONFIGURE, as I thought it should. Desktop
 * for that button is not updated.

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

#ifdef I18N_MB
#include <X11/Xlocale.h>
#ifdef __STDC__
#define XTextWidth(x,y,z) XmbTextEscapement(x ## set,y,z)
#else
#define XTextWidth(x,y,z) XmbTextEscapement(x/**/set,y,z)
#endif
#define XDrawString(t,u,v,w,x,y,z) XmbDrawString(t,u,ButtonFontset,v,w,x,y,z)
#endif

#include "libs/Module.h"
#include "libs/fvwmsignal.h"

#include "FvwmWinList.h"
#include "ButtonArray.h"
#include "List.h"
#include "Mallocs.h"
#include "libs/Colorset.h"

#define GRAB_EVENTS (ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|EnterWindowMask|LeaveWindowMask)

#define SomeButtonDown(a) ((a)&Button1Mask||(a)&Button2Mask||(a)&Button3Mask)

/* File type information */
FILE *console;
fd_set_size_t fd_width;
int Fvwm_fd[2];
int x_fd;

/* X related things */
Display *dpy;
Window Root, win;
int screen,ScreenWidth,ScreenHeight;
Pixel back[MAX_COLOUR_SETS], fore[MAX_COLOUR_SETS];
GC  graph[MAX_COLOUR_SETS],shadow[MAX_COLOUR_SETS],hilite[MAX_COLOUR_SETS];
GC  background[MAX_COLOUR_SETS];
XFontStruct *ButtonFont;
#ifdef I18N_MB
XFontSet ButtonFontset;
#endif
int fontheight;
static Atom wm_del_win;
Atom MwmAtom = None;

/* Module related information */
char *Module;
int WindowIsUp=0,win_width=5,win_height=5,win_grav,win_x,win_y,win_title,win_border;
int Clength,Transient=0,Pressed=0,ButPressed,Checked=0;
int MinWidth=DEFMINWIDTH,MaxWidth=DEFMAXWIDTH;
ButtonArray buttons;
List windows;
char *ClickAction[3]={"Iconify -1,Raise","Iconify","Lower"},*EnterAction,
      *BackColor[MAX_COLOUR_SETS] = { "white" },
      *ForeColor[MAX_COLOUR_SETS] = { "black" },
      *geometry="";
int colorset[MAX_COLOUR_SETS];
Pixmap pixmap[MAX_COLOUR_SETS];
char *font_string = "fixed";
Bool UseSkipList = False, Anchor = True, UseIconNames = False,
     LeftJustify = False, TruncateLeft = False, ShowFocus = True,
     Follow = False;
int ReliefWidth = 2;

long CurrentDesk = 0;
int ShowCurrentDesk = 0;

static RETSIGTYPE TerminateHandler(int sig);

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

int ItemCountD(List *list )
{
	if(!ShowCurrentDesk)
		return ItemCount(list);
	/* else */
	return ItemCountDesk(list, CurrentDesk);

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


  if ((argc==7)&&(!strcasecmp(argv[6],"Transient"))) Transient=1;

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

  SetMessageMask(Fvwm_fd,M_CONFIGURE_WINDOW | M_RES_CLASS | M_RES_NAME |
                 M_ADD_WINDOW | M_DESTROY_WINDOW | M_ICON_NAME |
                 M_DEICONIFY | M_ICONIFY | M_END_WINDOWLIST |
                 M_NEW_DESK | M_NEW_PAGE | M_FOCUS_CHANGE | M_WINDOW_NAME |
#ifdef MINI_ICONS
                 M_MINI_ICON |
#endif
                 M_STRING | M_CONFIG_INFO | M_SENDCONFIG);

  SendFvwmPipe("Send_WindowList",0);

  /* Recieve all messages from Fvwm */
  atexit(ShutMeDown);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(Fvwm_fd);

  MainEventLoop();
#ifdef FVWM_DEBUG_MSGS
  if ( debug_term_signal )
  {
    fvwm_msg(DBG, "main", "Terminated by signal %d", debug_term_signal);
  }
#endif
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
  int redraw=0,i;
  Item *flagitem;
  char *name,*string;
  static int current_focus=-1;
  struct ConfigWinPacket  *cfgpacket;

  Picture p;

  switch(type)
  {
    case M_ADD_WINDOW:
    case M_CONFIGURE_WINDOW:
      cfgpacket = (void *) body;
      if ((i = FindItem(&windows,cfgpacket->w))!=-1)
      {
	if(UpdateItemDesk(&windows, i, cfgpacket->desk) > 0)
        {
          AdjustWindow(False);
          RedrawWindow(True);
        }
	break;
      }
      if (!(DO_SKIP_WINDOW_LIST(cfgpacket)) || !UseSkipList)
        AddItem(&windows, cfgpacket);
      break;
    case M_DESTROY_WINDOW:
      cfgpacket = (void *) body;
      if ((i=DeleteItem(&windows,cfgpacket->w))==-1) break;
      RemoveButton(&buttons,i);
      if (WindowIsUp)
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
      redraw = 0;
    }
    break;
#endif

    case M_WINDOW_NAME:
    case M_ICON_NAME:
      if ((type==M_ICON_NAME && !UseIconNames) ||
          (type==M_WINDOW_NAME && UseIconNames)) break;
      if ((i=UpdateItemName(&windows,body[0],(char *)&body[3]))==-1) break;
      string=(char *)&body[3];
      flagitem = ItemFlags(&windows,body[0]);
      name=makename(string, (IS_ICONIFIED(flagitem)?True:False));
      if (UpdateButton(&buttons,i,name,-1)==-1)
      {
	AddButton(&buttons, name, NULL, 1);
        flagitem = ItemFlags(&windows,body[0]);
	UpdateButtonSet(&buttons, i, (IS_ICONIFIED(flagitem)?1:0));
        UpdateButtonDesk(&buttons,i,ItemDesk(&windows, body[0]));
      }
      free(name);
      if (WindowIsUp) AdjustWindow(False);
      redraw=1;
      break;
    case M_DEICONIFY:
    case M_ICONIFY:
      if ((i=FindItem(&windows,body[0]))==-1) break;
      flagitem=ItemFlags(&windows,body[0]);
      if (type==M_DEICONIFY && !(IS_ICONIFIED(flagitem))) break;
      if (type==M_ICONIFY && (IS_ICONIFIED(flagitem))) break;
      SET_ICONIFIED(flagitem, (IS_ICONIFIED(flagitem)?False:True));

      string=ItemName(&windows,i);
      name=makename(string, (IS_ICONIFIED(flagitem)?True:False));
      if (UpdateButton(&buttons,i,name,-1)!=-1) redraw=1;
      if (i!=current_focus||(IS_ICONIFIED(flagitem)))
        if (UpdateButtonSet(&buttons,i,(IS_ICONIFIED(flagitem)) ? 1 : 0)!=-1) redraw=1;
      free(name);
      break;
    case M_FOCUS_CHANGE:
      redraw = 1;
      if ((i=FindItem(&windows,body[0]))!=-1)
      {
/* RBW- wait a minute! - this never did anything anyway...
        flags=ItemFlags(&windows,body[0]);
        UpdateItemFlags(&windows,body[0],flags);
*/
        RadioButton(&buttons,i);
        if (Follow && i) { /* rearrange order */
          ReorderList(&windows,i,body[2]);
          if (WindowIsUp) ReorderButtons(&buttons,i,body[2]);
        }
      }
      else
        RadioButton(&buttons,-1);
      break;
    case M_END_WINDOWLIST:
      if (!WindowIsUp) MakeMeWindow();
      redraw = 1;
      break;
    case M_NEW_DESK:
      CurrentDesk = body[0];
      if(ShowCurrentDesk)
      {
        AdjustWindow(False);
        RedrawWindow(True);
      }
      break;
    case M_NEW_PAGE:
      break;
    case M_CONFIG_INFO:
      ParseConfigLine((char *)&body[3]);
      break;
  }

  if (redraw && WindowIsUp==1) RedrawWindow(False);
}

/******************************************************************************
  SendFvwmPipe - Send a message back to fvwm
    Based on SendInfo() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void SendFvwmPipe(char *message,unsigned long window)
{
  int w;
  char *hold,*temp,*temp_msg;

  hold=message;

  while(1)
  {
    temp=strchr(hold,',');
    if (temp!=NULL)
    {
      temp_msg=safemalloc(temp-hold+1);
      strncpy(temp_msg,hold,(temp-hold));
      temp_msg[(temp-hold)]='\0';
      hold=temp+1;
    } else temp_msg=hold;

    write(Fvwm_fd[0],&window, sizeof(unsigned long));

    w=strlen(temp_msg);
    write(Fvwm_fd[0],&w,sizeof(int));
    write(Fvwm_fd[0],temp_msg,w);

    /* keep going */
    w=1;
    write(Fvwm_fd[0],&w,sizeof(int));

    if(temp_msg!=hold) free(temp_msg);
    else break;
  }
}

/***********************************************************************
  Detected a broken pipe - time to exit
    Based on DeadPipe() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
 **********************************************************************/
void DeadPipe(int nonsense)
{
  /* ShutMeDown(1); */
  /*
   * do not call ShutMeDown, it may make X calls which are not allowed
   * in a signal hander.
   *
   * THIS IS NO LONGER A SIGNAL HANDLER - we may now shut down gracefully
   * NOTE: ShutMeDown will now be called automatically by exit().
   */
  exit(1);
}

/******************************************************************************
  WaitForExpose - Used to wait for expose event so we don't draw too early
******************************************************************************/
void WaitForExpose(void)
{
  XEvent Event;

  while(1)
  {
    /*
     * Temporary solution to stop the process blocking
     * in XNextEvent once we have been asked to quit.
     * There is still a small race condition between
     * checking the flag and calling the X-Server, but
     * we can fix that ...
     */
    if (isTerminated)
    {
      /* Just exit - the installed exit-procedure will clean up */
      exit(0);
    }
    /**/

    XNextEvent(dpy,&Event);
    if (Event.type==Expose)
    {
      if (Event.xexpose.count==0) break;
    }
  }
}

/******************************************************************************
  RedrawWindow - Update the needed lines and erase any old ones
******************************************************************************/
void RedrawWindow(Bool force)
{
  DrawButtonArray(&buttons, force);
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
  if (!tline)
    return;

  if (strlen(tline) > 1) {
    if (strncasecmp(tline, CatString3(Module, "Font", ""), Clength + 4)	== 0)
      CopyString(&font_string, &tline[Clength + 4]);
    else if (strncasecmp(tline, CatString3(Module, "Fore", ""), Clength + 4)
	     == 0) {
      CopyString(&ForeColor[0], &tline[Clength + 4]);
      colorset[0] = -1;
    } else if (strncasecmp(tline, CatString3(Module, "IconFore", ""),
			   Clength + 8) == 0) {
      CopyString(&ForeColor[1], &tline[Clength + 8]);
      colorset[1] = -1;
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
    } else if (strncasecmp(tline, CatString3(Module, "IconBack", ""),
			   Clength + 8) == 0) {
      CopyString(&BackColor[1], &tline[Clength + 8]);
      colorset[1] = -1;
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
			 Clength + 16) == 0)
      ReliefWidth = atoi(&tline[Clength + 16]);
    else if (strncasecmp(tline, CatString3(Module, "Colorset", ""),
			 Clength + 8) == 0)
      colorset[0] = atoi(&tline[Clength + 8]);
    else if (strncasecmp(tline, CatString3(Module, "IconColorset", ""),
			 Clength + 12) == 0)
      colorset[1] = atoi(&tline[Clength + 12]);
    else if (strncasecmp(tline, CatString3(Module, "FocusColorset", ""),
			 Clength + 13) == 0)
      colorset[2] = colorset[3] = atoi(&tline[Clength + 13]);
    else if (strncasecmp(tline, "Colorset", 8) == 0) {
      int cset = LoadColorset(&tline[8]);

      if (WindowIsUp && (cset >= 0)) {
	int i;

	for (i = 0; i != MAX_COLOUR_SETS; i++) {
	  if (colorset[i] == cset) {
	    AdjustWindow(True);
	    RedrawWindow(True);
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
  Window dummyroot,dummychild;
  int x,x1,y,y1;
  unsigned int dummy1;

  if (Transient && !Checked)
  {
    XQueryPointer(dpy,win,&dummyroot,&dummychild,&x1,&y1,&x,&y,&dummy1);
    num=WhichButton(&buttons,x,y);
    if (num!=-1)
    {
      Pressed=1;
      ButPressed=num;
      SwitchButton(&buttons,num);
    } else Pressed=0;
    Checked=1;
  }

  while(XPending(dpy))
  {
    XNextEvent(dpy,&Event);

    switch(Event.type)
    {
      case ButtonRelease:
        if (Pressed)
        {
          num=WhichButton(&buttons,Event.xbutton.x,Event.xbutton.y);
          if (num!=-1)
          {
            SendFvwmPipe(ClickAction[(Transient) ? 0:Event.xbutton.button-1],
              ItemID(&windows,num));
            SwitchButton(&buttons,num);
          }
        }
        if (Transient) exit(0);
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
        if (Event.xexpose.count==0)
          RedrawWindow(True);
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
  int new_width=0,new_height=0,tw,i,total;
  char *temp;

  total = ItemCountD(&windows );
  if (!total)
  {
    if (WindowIsUp==1)
    {
      XUnmapWindow(dpy,win);
      WindowIsUp=2;
    }
    return;
  }
  for(i=0;i<total;i++)
  {
    temp=ItemName(&windows,i);
    if(temp != NULL)
    {
	tw=10+XTextWidth(ButtonFont,temp,strlen(temp));
	tw+=XTextWidth(ButtonFont,"()",2);

#ifdef MINI_ICONS
	tw+=14; /* for title icon */ /* Magic Number ? */
#endif

	new_width=max(new_width,tw);
    }
  }
  new_width=max(new_width, MinWidth);
  new_width=min(new_width, MaxWidth);
  new_height=(total * (fontheight + 3 + 2 * ReliefWidth));
  if (force || (WindowIsUp && (new_height != win_height
			       || new_width != win_width))) {
    for (i = 0; i != MAX_COLOUR_SETS; i++) {
      int cset = colorset[i];

      if (cset >= 0) {
        cset = cset % nColorsets;
	fore[i] = Colorset[cset].fg;
	back[i] = Colorset[cset].bg;
	XSetForeground(dpy, graph[i], fore[i]);
	XSetForeground(dpy, background[i], back[i]);
	XSetForeground(dpy, hilite[i], Colorset[cset].hilite);
	XSetForeground(dpy, shadow[i], Colorset[cset].shadow);
	if (pixmap[i])
	  XFreePixmap(dpy, pixmap[i]);
	if (Colorset[cset].pixmap) {
	  pixmap[i] = CreateBackgroundPixmap(dpy, win, new_width,
					     fontheight + 3 + 2 * ReliefWidth,
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
  if (WindowIsUp && (new_height!=win_height  || new_width!=win_width))
  {
    if (Anchor)
    {
      Window child;

      MyXGrabServer(dpy);
      XTranslateCoordinates(dpy, win, Root, 0, 0, &win_x, &win_y, &child);
      if (win_grav == SouthEastGravity || win_grav == NorthEastGravity)
        win_x += win_width - new_width;
      if (win_grav == SouthEastGravity || win_grav == SouthWestGravity)
        win_y += win_height - new_height;

      XMoveResizeWindow(dpy, win, win_x, win_y, new_width, new_height);
      MyXUngrabServer(dpy);
    }
    else
      XResizeWindow(dpy, win, new_width, new_height);

  }
  UpdateArray(&buttons,-1,-1,new_width,-1);
  if (new_height>0) win_height = new_height;
  if (new_width>0) win_width = new_width;
  if (WindowIsUp==2)
  {
    XMapWindow(dpy,win);
    WindowIsUp=1;
    WaitForExpose();
  }
}

/******************************************************************************
  makename - Based on the flags return me '(name)' or 'name'
******************************************************************************/
char *makename(const char *string, Bool iconified)
{
char *ptr;
  ptr=safemalloc(strlen(string)+3);
  *ptr = '\0';
  if (iconified) strcpy(ptr,"(");
  strcat(ptr,string);
  if (iconified) strcat(ptr,")");
  return ptr;
}

/******************************************************************************
  LinkAction - Link an response to a users action
******************************************************************************/
void LinkAction(char *string)
{
char *temp;
  temp=string;
  while(isspace((unsigned char)*temp)) temp++;
  if(strncasecmp(temp, "Click1", 6)==0)
    CopyString(&ClickAction[0],&temp[6]);
  else if(strncasecmp(temp, "Click2", 6)==0)
    CopyString(&ClickAction[1],&temp[6]);
  else if(strncasecmp(temp, "Click3", 6)==0)
    CopyString(&ClickAction[2],&temp[6]);
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


  if ((count = ItemCountD(&windows))==0 && Transient) exit(0);
  AdjustWindow(False);

  hints.width=win_width;
  hints.height=win_height;
  hints.win_gravity=NorthWestGravity;
  hints.flags=PSize|PWinGravity;

  if (geometry!= NULL)
  {
    ret=XParseGeometry(geometry,&x,&y,&dummy1,&dummy2);

    if (ret&XValue && ret &YValue)
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
      if (ret&YNegative) hints.win_gravity=SouthEastGravity;
      else  hints.win_gravity=NorthEastGravity;
    }
    else
    {
      if (ret&YNegative) hints.win_gravity=SouthWestGravity;
      else  hints.win_gravity=NorthWestGravity;
    }

  }

  if (Transient)
  {
    XQueryPointer(dpy,Root,&dummyroot,&dummychild,&hints.x,&hints.y,&x,&y,
		  &dummy1);
    hints.win_gravity=NorthWestGravity;
    hints.flags |= USPosition;
  }
  win_grav=hints.win_gravity;
  win_x=hints.x;
  win_y=hints.y;


  for (i = 0; i != MAX_COLOUR_SETS; i++)
    if(Pdepth < 2)
    {
      back[i] = GetColor("white");
      fore[i] = GetColor("black");
    }
    else
    {
      if (colorset[i] >= 0) {
        back[i] = Colorset[colorset[i] % nColorsets].bg;
        fore[i] = Colorset[colorset[i] % nColorsets].fg;
      } else {
	back[i] = GetColor(BackColor[i] == NULL ? BackColor[0] : BackColor[i]);
	fore[i] = GetColor(ForeColor[i] == NULL ? ForeColor[0] : ForeColor[i]);
      }
    }

  /* set a NULL background to prevent annoying flashes when using colorsets */
  attr.background_pixmap = None;
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
    graph[i]=XCreateGC(dpy,win,gcmask,&gcval);

    if(Pdepth < 2)
      gcval.foreground=GetShadow(fore[i]);
    else
      if (colorset[i] < 0)
	gcval.foreground=GetShadow(back[i]);
      else
	gcval.foreground=Colorset[colorset[i] % nColorsets].shadow;
    gcval.background=back[i];
    gcmask=GCForeground|GCBackground;
    shadow[i]=XCreateGC(dpy,win,gcmask,&gcval);

    if (colorset[i] < 0)
      gcval.foreground=GetHilite(back[i]);
    else
      gcval.foreground=Colorset[colorset[i] % nColorsets].hilite;
    gcval.background=back[i];
    gcmask=GCForeground|GCBackground;
    hilite[i]=XCreateGC(dpy,win,gcmask,&gcval);

    gcval.foreground=back[i];
    gcmask=GCForeground;
    background[i]=XCreateGC(dpy,win,gcmask,&gcval);
  }
  AdjustWindow(True);
  XSelectInput(dpy,win,(ExposureMask | KeyPressMask));

  ChangeWindowName(&Module[1]);

  if (ItemCountD(&windows) > 0)
  {
    XMapRaised(dpy,win);
    WaitForExpose();
    WindowIsUp=1;
  } else WindowIsUp=2;

  if (Transient)
  {
    if ( XGrabPointer(dpy,win,True,GRAB_EVENTS,GrabModeAsync,GrabModeAsync,
      None,None,CurrentTime)!=GrabSuccess) exit(1);
    XQueryPointer(dpy,Root,&dummyroot,&dummychild,&hints.x,&hints.y,&x,&y,&dummy1);
    if (!SomeButtonDown(dummy1)) exit(0);
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
  AllocColorset(0);
  x_fd = XConnectionNumber(dpy);
  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);

  ScreenHeight = DisplayHeight(dpy,screen);
  ScreenWidth = DisplayWidth(dpy,screen);

#ifdef I18N_MB
  if ((ButtonFontset=XCreateFontSet(dpy,font_string,&ml,&mc,&ds)) == NULL) {
#ifdef STRICTLY_FIXED
    if ((ButtonFontset=XCreateFontSet(dpy,"fixed",&ml,&mc,&ds)) == NULL) exit(1);
#else
    if ((ButtonFontset=XCreateFontSet(dpy,"-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*",&ml,&mc,&ds)) == NULL) exit(1);
#endif
  }
  XFontsOfFontSet(ButtonFontset,&fs_list,&ml);
  ButtonFont = fs_list[0];
#else
  if ((ButtonFont=XLoadQueryFont(dpy,font_string))==NULL)
  {
    if ((ButtonFont=XLoadQueryFont(dpy,"fixed"))==NULL) exit(1);
  }
#endif

  fontheight = ButtonFont->ascent+ButtonFont->descent;

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
  if (WindowIsUp) XDestroyWindow(dpy,win);
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
    PrintXErrorAndCoredump(d, event, Module);
    return 0;
}

