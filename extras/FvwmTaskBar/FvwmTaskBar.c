/* FvwmTaskBar Module for Fvwm.
 *
 * (Much reworked version of FvwmWinList)
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 * Minor hack by TKP to enable autohide to work with pages (leaves 2 pixels
 * visible so that the 1 pixel panframes don't get in the way)
 *
 * The author makes not guarantees or warantees, either express or
 * implied.  Feel free to use any contained here for any purpose, as long
 * and this and any other applicable copyrights are kept intact.

 * The functions in this source file that are based on part of the FvwmIdent
 * module for Fvwm are noted by a small copyright atop that function, all others
 * are copyrighted by Mike Finger.  For those functions modified/used, here is
 *  the full, original copyright:
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdarg.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif
#include <unistd.h>
#include <ctype.h>
#ifdef ISC /* Saul */
#include <sys/bsdtypes.h> /* Saul */
#endif /* Saul */
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>

#include "../../fvwm/module.h"
#include "../../libs/fvwmlib.h"  /* for pixmaps routines */


#include "FvwmTaskBar.h"
#include "ButtonArray.h"
#include "List.h"
#include "Colors.h"
#include "Mallocs.h"
#include "Goodies.h"
#include "Start.h"


#define GRAB_EVENTS (ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|EnterWindowMask|LeaveWindowMask)
#define SomeButtonDown(a) ((a)&Button1Mask||(a)&Button2Mask||(a)&Button3Mask)

#define DEFAULT_CLICK1 "Iconify -1, Raise, Focus"
#define DEFAULT_CLICK2 "Iconify +1, Lower"
#define DEFAULT_CLICK3 "Nop"

#define gray_width  8
#define gray_height 8
unsigned char gray_bits[] = { 0xaa, 0x55, 0xaa, 0x55,
                              0xaa, 0x55, 0xaa, 0x55 };
GC checkered;

/* File type information */
FILE  *console;
int   fd_width;
int   Fvwm_fd[2];
int   x_fd;

/* X related things */
Display *dpy;
Window  Root, win;
int     screen, d_depth;
Pixel   back, fore;
GC      graph, shadow, hilite, blackgc, whitegc;
XFontStruct *ButtonFont, *SelButtonFont;
int fontheight;
static Atom wm_del_win;
Atom MwmAtom = None;

/* Module related information */
char *Module;
int  win_width    = 5,
     win_height   = 5,
     win_grav,
     win_x,
     win_y,
     win_border,
     button_width = DEFAULT_BTN_WIDTH,
     Clength,
     ButPressed   = -1,
     ButReleased  = -1,
     Checked      = 0,
     BelayHide    = False,
     AlarmSet     = NOT_SET;

int UpdateInterval = 30;

ButtonArray buttons;
List windows;
List swallowed;

char *ClickAction[3] = { DEFAULT_CLICK1, DEFAULT_CLICK2, DEFAULT_CLICK3 },
     *EnterAction,
     *BackColor      = "white",
     *ForeColor      = "black",
     *geometry       = NULL,
     *font_string    = "fixed",
     *selfont_string = NULL;

int UseSkipList    = False,
    UseIconNames   = False,
    ShowTransients = False,
    adjust_flag    = False,
    AutoStick      = False,
    AutoHide       = False,
    HighlightFocus = False;

unsigned int ScreenWidth, ScreenHeight;

int NRows, RowHeight, Midline;

/* Imported from Goodies */
extern int stwin_width, goodies_width;
extern TipStruct Tip;

/* Imported from Start */
extern int StartButtonWidth, StartButtonHeight;
extern char *StartPopup;

Colormap PictureCMap;

char *IconPath   = NULL,
     *PixmapPath = NULL;

static void ParseConfigLine(unsigned char *tline); /* prototype */

/******************************************************************************
  Main - Setup the XConnection,request the window list and loop forever
    Based on main() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
int main(int argc, char **argv)
{
  char *temp, *s;

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

  /* Open the console for messages */
  OpenConsole();

  if((argc != 6)&&(argc != 7)) {
    fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",Module,
	    VERSION);
    ConsoleMessage("%s Version %s should only be executed by fvwm!\n",Module,
		   VERSION);
    exit(1);
  }

  /* setup fvwm pipes */
  Fvwm_fd[0] = atoi(argv[1]);
  Fvwm_fd[1] = atoi(argv[2]);
  fd_width = GetFdWidth();

  signal (SIGPIPE, DeadPipe);
  signal (SIGALRM, Alarm);

  SetMessageMask(Fvwm_fd,M_ADD_WINDOW | M_CONFIGURE_WINDOW | M_DESTROY_WINDOW |
		 M_WINDOW_NAME | M_ICON_NAME | M_RES_NAME | M_DEICONIFY | M_ICONIFY |
		 M_END_WINDOWLIST | M_FOCUS_CHANGE |
		M_CONFIG_INFO | M_END_CONFIG_INFO
#ifdef FVWM95
		| M_FUNCTION_END |
		M_SCROLLREGION
#endif
#ifdef MINI_ICONS
		 | M_MINI_ICON
#endif
		);

  /* Parse the config file */
  InitList(&swallowed);

  ParseConfig();

  /* Setup the XConnection */
  StartMeUp();
  XSetErrorHandler((XErrorHandler) ErrorHandler);

  InitPictureCMap(dpy, Root);

  StartButtonInit(RowHeight);

  /* init the array of buttons */
  InitArray(&buttons, StartButtonWidth + 4, 0,
                      win_width - stwin_width - 8 - StartButtonWidth -10,
                      RowHeight, button_width);
  InitList(&windows);

  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendFvwmPipe("Send_WindowList",0);

  /* Receive all messages from Fvwm */
  EndLessLoop();
}

/******************************************************************************
  EndLessLoop -  Read and redraw until we get killed, blocking when can't read
******************************************************************************/
void EndLessLoop()
{
  fd_set readset;
  struct timeval tv;

  while(1) {
    FD_ZERO(&readset);
    FD_SET(Fvwm_fd[1], &readset);
    FD_SET(x_fd, &readset);
    XPending(dpy);
    tv.tv_sec  = 0;
    tv.tv_usec = 0;
#ifdef __hpux
    if (!select(fd_width, (int *)&readset, NULL, NULL, &tv)) {
      while(1) {
        FD_ZERO(&readset);
        FD_SET(Fvwm_fd[1], &readset);
        FD_SET(x_fd, &readset);
        XPending(dpy);

        tv.tv_sec  = UpdateInterval;
        tv.tv_usec = 0;
        if (select(fd_width, (int *)&readset, NULL, NULL, &tv) <= 0) 
          DrawGoodies();
        else
          break;
      }
    }
#else
    if (!select(fd_width, &readset, NULL, NULL, &tv)) {
      while(1) {
        FD_ZERO(&readset);
        FD_SET(Fvwm_fd[1], &readset);
        FD_SET(x_fd, &readset);
        XPending(dpy);

        tv.tv_sec  = UpdateInterval;
        tv.tv_usec = 0;

        if (select(fd_width, &readset, NULL, NULL, &tv) <= 0)
          DrawGoodies();
        else
          break;
      }
    }
#endif

    if (FD_ISSET(x_fd, &readset))
      LoopOnEvents();

    if (FD_ISSET(Fvwm_fd[1], &readset)) {
      ReadFvwmPipe();
      DrawGoodies();
    }

  }
}


/******************************************************************************
  ReadFvwmPipe - Read a single message from the pipe from Fvwm
    Originally Loop() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ReadFvwmPipe()
{
  int count,total,count2=0,body_length;
  unsigned long header[HEADER_SIZE],*body;
  char *cbody;
  if(ReadFvwmPacket(Fvwm_fd[1],header,&body) > 0)
    {
      ProcessMessage(header[1],body);
      free(body);
    }
}

/******************************************************************************
  ProcessMessage - Process the message coming from Fvwm
    Skeleton based on processmessage() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ProcessMessage(unsigned long type,unsigned long *body)
{
  int redraw=-1;
  int i;
  long flags;
  char *name,*string;
  Picture p;
  
  switch(type) {
  case M_FOCUS_CHANGE:
    i = FindItem(&windows, body[0]);
    if (win != body[0]) { /* This case is handled in LoopOnEvents() */
      if (ItemFlags(&windows, body[0]) & ICONIFIED) i = -1;
      RadioButton(&buttons, i, BUTTON_BRIGHT);
      ButPressed = i;
      ButReleased = -1;
      redraw = 0;
    }
    break;

  case M_ADD_WINDOW:
  case M_CONFIGURE_WINDOW:
    /* Matched only at startup when default border width and
       actual border width differ. Don't assign win_width here so 
       Window redraw and rewarping gets handled by XEvent
       ConfigureNotify code. */
    if (!ShowTransients && (body[8] & TRANSIENT)) break;
    if (body[0] == win) {
      if (win_border != (int)body[10]) {
	win_x = win_border = (int)body[10];

        if (win_y > Midline)
          win_y = ScreenHeight - (AutoHide ? 2 : win_height + win_border);
        else 
          win_y = AutoHide ? 2 - win_height : win_border;

        XMoveResizeWindow(dpy, win, win_x, win_y,
                          ScreenWidth-(win_border<<1), win_height);

      }
      break;
    }

    if (FindItem(&windows,body[0]) != -1) break;
    if (!(body[8] & WINDOWLISTSKIP) || !UseSkipList) {
      AddItem(&windows, body[0], body[8]);
    }
    break;

  case M_DESTROY_WINDOW:
    if ((i = DeleteItem(&windows, body[0])) == -1) break;
    if (FindItem(&swallowed, body[0]) != -1) break;
    RemoveButton(&buttons, i);
    redraw = 1;
    break;

#ifdef MINI_ICONS
  case M_MINI_ICON:
    if ((i = FindItem(&windows, body[0])) == -1) break;
    if (UpdateButton(&buttons, i, NULL, DONT_CARE) != -1) {
      p.picture = body[6];
      p.mask    = body[7];
      p.width   = body[3];
      p.height  = body[4];
      p.depth   = body[5];
      UpdateButtonPicture(&buttons, i, &p);
      redraw = 0;
    }
    break;
#endif

  case M_WINDOW_NAME:
  case M_ICON_NAME:
    if ((type == M_ICON_NAME   && !UseIconNames) || 
	(type == M_WINDOW_NAME &&  UseIconNames)) break;
    string = (char *) &body[3];
    if ((i = FindNameItem(&swallowed, string)) != -1) {
      if (ItemIndexFlags(&swallowed, i) == F_NOT_SWALLOWED) {
	Swallow(body);
	break;
      }
    }
    if ((i = UpdateItemName(&windows, body[0], (char *)&body[3])) == -1) 
      break;
    if (UpdateButton(&buttons, i, string, DONT_CARE) == -1)
      {
      AddButton(&buttons, string, NULL, BUTTON_UP);
      redraw = 1;
      }
    else
      redraw = 0;
    break;

  case M_DEICONIFY:
  case M_ICONIFY:
    if ((i = FindItem(&windows, body[0])) == -1) break;
    flags = ItemFlags(&windows, body[0]);
    if (type == M_DEICONIFY && !(flags & ICONIFIED)) break;
    if (type == M_ICONIFY   &&   flags & ICONIFIED) break;
    flags ^= ICONIFIED;
    if (type == M_ICONIFY && i == ButReleased) {
      RadioButton(&buttons, -1, BUTTON_UP);
      ButReleased = ButPressed = -1;
      redraw = 0;
    }
    UpdateItemFlags(&windows, body[0], flags);
    break;

  case M_END_WINDOWLIST:
    XMapRaised(dpy, win);
    break;

#ifdef FVWM95
  case M_FUNCTION_END:
    StartButtonUpdate(NULL, BUTTON_UP);
    
    if (AutoHide && !BelayHide) /* We don't want the taskbar to hide */
      SetAlarm(HIDE_TASK_BAR);  /* after a Focus or Iconify function */

    redraw = 0;
    break;

  /* Added a new Fvwm Event because scrolling regions interfere 
     with EnterNotify event when taskbar is hidden. */
  case M_SCROLLREGION:
    if (AutoHide && ((win_y <  Midline && body[1] < 4) ||
                     (win_y >= Midline && body[1] > ScreenHeight-4)))
      RevealTaskBar();
    break;

#endif /*FVWM95*/

  case M_NEW_DESK:
    break;

  case M_NEW_PAGE:
    break;

  }
  
  if (redraw >= 0) RedrawWindow(redraw);
}

/******************************************************************************
  SendFvwmPipe - Send a message back to fvwm 
    Based on SendInfo() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void SendFvwmPipe(char *message, unsigned long window)
{
  int  w;
  char *hold, *temp, *temp_msg;

  hold = message;
  
  while(1) {
    temp = strchr(hold, ',');
    if (temp != NULL) {
      temp_msg = malloc(temp-hold+1);
      strncpy(temp_msg, hold, (temp-hold));
      temp_msg[(temp-hold)] = '\0';
      hold = temp+1;
    } else temp_msg = hold;
    
    write(Fvwm_fd[0], &window, sizeof(unsigned long));
    
    w=strlen(temp_msg);
    write(Fvwm_fd[0], &w, sizeof(int));
    write(Fvwm_fd[0], temp_msg, w);

    /* keep going */
    w = 1;
    write(Fvwm_fd[0], &w, sizeof(int));

    if(temp_msg != hold)
      free(temp_msg);
    else
      break;
  }
}

/***********************************************************************
  Detected a broken pipe - time to exit 
    Based on DeadPipe() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
 **********************************************************************/
void DeadPipe(int nonsense)
{
  ConsoleMessage("received SIGPIPE signal: exiting...\n");
  ShutMeDown(1);
}

/******************************************************************************
  WaitForExpose - Used to wait for expose event so we don't draw too early
******************************************************************************/
void WaitForExpose(void)
{
  XEvent Event;

  while(1) {
    XNextEvent(dpy, &Event);
    if (Event.type == Expose) {
      if (Event.xexpose.count == 0) break;
    }
  }
}

/******************************************************************************
  RedrawWindow - Update the needed lines and erase any old ones
******************************************************************************/
void RedrawWindow(int force)
{
  if (Tip.open) RedrawTipWindow();
  if (force) {
    XClearArea (dpy, win, 0, 0, win_width, win_height, False);
    DrawGoodies();
  }
  DrawButtonArray(&buttons, force);
  StartButtonDraw(force);
  if (XQLength(dpy) && !force) LoopOnEvents();
}

/******************************************************************************
  ConsoleMessage - Print a message on the console.  Works like printf.
******************************************************************************/
void ConsoleMessage(char *fmt, ...)
{
#ifndef NO_CONSOLE
  va_list args;
  FILE *filep;

  if (console == NULL)
    filep = stderr;
  else
    filep = console;
  va_start(args, fmt);
  vfprintf(filep, fmt, args);
  va_end(args);
  fflush(console);
#endif
}

/******************************************************************************
  OpenConsole - Open the console as a way of sending messages
******************************************************************************/
int OpenConsole()
{
#ifndef NO_CONSOLE
  if ((console = fopen("/dev/console","w")) == NULL) {
    fprintf(stderr, "%s: cannot open console\n", Module);
    return 0;
  }
#endif
  return 1;
}

void ParseConfig() {
  char *buf;
  while (GetConfigLine(Fvwm_fd,&buf), buf != NULL) {
    ParseConfigLine(buf);
  } /* end config lines */
} /* end function */

static void ParseConfigLine(unsigned char *tline) {
  char line[256];
  char *str;
  int i, j;

  while (isspace(*tline))tline++;
  if(strncasecmp(tline, CatString3(Module, "Font",""),Clength+4)==0)
    CopyString(&font_string,&tline[Clength+4]);
  else if(strncasecmp(tline, CatString3(Module, "SelFont",""),Clength+7)==0)
    CopyString(&selfont_string,&tline[Clength+7]);
  else if(strncasecmp(tline,CatString3(Module,"Fore",""), Clength+4)==0) {
    CopyString(&ForeColor,&tline[Clength+4]);
  } else if(strncasecmp(tline,CatString3(Module, "Geometry",""), Clength+8)==0) {
    str = &tline[Clength+9];
    while(((isspace(*str))&&(*str != '\n'))&&(*str != 0))	str++;
    str[strlen(str)-1] = 0;
    UpdateString(&geometry,str);
  } else if(strncasecmp(tline,CatString3(Module, "Back",""), Clength+4)==0)
    CopyString(&BackColor,&tline[Clength+4]);
  else if(strncasecmp(tline,CatString3(Module, "Action",""), Clength+6)==0)
    LinkAction(&tline[Clength+6]);
  else if(strncasecmp(tline,CatString3(Module, "UseSkipList",""),
                      Clength+11)==0) UseSkipList=True;
  else if(strncasecmp(tline,CatString3(Module, "AutoStick",""),
                      Clength+9)==0) AutoStick=True;
  else if(strncasecmp(tline,CatString3(Module, "AutoHide",""),
                      Clength+4)==0) { AutoHide=True; AutoStick=True; }
  else if(strncasecmp(tline,CatString3(Module, "UseIconNames",""),
                      Clength+12)==0) UseIconNames=True;
  else if(strncasecmp(tline,CatString3(Module, "ShowTransients",""),
                      Clength+14)==0) ShowTransients=True;
  else if(strncasecmp(tline,CatString3(Module, "UpdateInterval",""),
                      Clength+14)==0)
    UpdateInterval=atoi(&tline[Clength+14]);
  else if(strncasecmp(tline,CatString3(Module, "HighlightFocus",""),
                      Clength+14)==0) HighlightFocus=True;
  else if(strncasecmp(tline,CatString3(Module, "SwallowModule",""),
                      Clength+13)==0) {
	
    /* tell fvwm to launch the module for us */
    str = safemalloc(strlen(&tline[Clength+13]) + 6);
    sprintf(str, "Module %s",&tline[Clength+13]);
    ConsoleMessage("Trying to: %s", str);
    SendFvwmPipe(str, 0);
	
	/* Remember the anticipated window's name for swallowing */
    i = 4;	      
    while((str[i] != 0)&&
          (str[i] != '"'))
      i++;
    j = ++i;
    while((str[i] != 0)&&
          (str[i] != '"'))
      i++;
    if (i > j) {
      str[i] = 0;
      ConsoleMessage("Looking for window: [%s]\n", &str[j]);
      AddItemName(&swallowed, &str[j], F_NOT_SWALLOWED);
    }	
    free(str);
  } else if(strncasecmp(tline,CatString3(Module, "Swallow",""),
                        Clength+7)==0) {
	
    /* tell fvwm to Exec the process for us */
    str = safemalloc(strlen(&tline[Clength+7]) + 6);
    sprintf(str, "Exec %s",&tline[Clength+7]);
    ConsoleMessage("Trying to: %s", str);
    SendFvwmPipe(str, 0);
	
	/* Remember the anticipated window's name for swallowing */
    i = 4;	      
    while((str[i] != 0)&&
          (str[i] != '"'))
      i++;
    j = ++i;
    while((str[i] != 0)&&
          (str[i] != '"'))
      i++;
    if (i > j) {
      str[i] = 0;
      ConsoleMessage("Looking for window: [%s]\n", &str[j]);
      AddItemName(&swallowed, &str[j], F_NOT_SWALLOWED);
    }	
    free(str);
  } else if(strncasecmp(tline,"ButtonWidth",11) == 0) {
    button_width = atoi(&tline[11]);
  } else if(strncasecmp(tline,"IconPath",8) == 0) {
    CopyString(&IconPath, &tline[8]);
  } else if(strncasecmp(tline,"PixmapPath",10) == 0) {
    CopyString(&PixmapPath, &tline[10]);
  } else {
    GoodiesParseConfig(tline, Module);
    StartButtonParseConfig(tline, Module);
  }
}

/******************************************************************************
  Swallow a process window
******************************************************************************/
void Swallow(unsigned long *body) {
  XSizeHints hints;
  long supplied;
  int h,w;

  /* Swallow the window */
  XUnmapWindow(dpy, body[0]);

  if (!XGetWMNormalHints (dpy, (Window)body[0], &hints, &supplied))
    hints.flags = 0;
  h = win_height - 10; w = 80;
  ConstrainSize(&hints, &w, &h);
  XResizeWindow(dpy,(Window)body[0], w, h);

  stwin_width += w + 2;

  XReparentWindow(dpy,(Window)body[0], win,
		  win_width - stwin_width + goodies_width + 2, 5);
  goodies_width += w + 2;
  
  XMapWindow(dpy,body[0]);
  XSelectInput(dpy,(Window)body[0],
	       PropertyChangeMask|StructureNotifyMask);


  /* Do not swallow it next time */
  UpdateNameItem(&swallowed, (char*)&body[3], body[0], F_SWALLOWED);
  ConsoleMessage("-> Swallowed: %s\n",(char*)&body[3]);
  RedrawWindow(1);
}

/******************************************************************************
  Alarm - Handle a SIGALRM - used to implement timeout events
******************************************************************************/
void Alarm(int nonsense) {

  switch(AlarmSet) {

  case SHOW_TIP:
    ShowTipWindow(1);
    break;

  case HIDE_TASK_BAR:
    HideTaskBar();
    break;
  }

  AlarmSet = NOT_SET;
  signal (SIGALRM, Alarm);
}

/******************************************************************************
  CheckForTip - determine when to popup the tip window
******************************************************************************/
void CheckForTip(int x, int y) {
  int  num, bx, by, trunc;
  char *name;

  if (MouseInStartButton(x, y)) {
    if (Tip.type != START_TIP) PopupTipWindow(3, 0, "Click here to start");
    Tip.type = START_TIP;
  }
  else if (MouseInMail(x, y)) {
    if (Tip.type != MAIL_TIP) CreateMailTipWindow();
    Tip.type = MAIL_TIP;
  }
  else if (MouseInClock(x, y)) {
    if (Tip.type != DATE_TIP) CreateDateWindow();
    Tip.type = DATE_TIP;
  }
  else {
    num = LocateButton(&buttons, x, y, &bx, &by, &name, &trunc);
    if (num != -1 && trunc) {
      if ((Tip.type != num)  || 
          (Tip.text == NULL) || (strcmp(name, Tip.text) != 0))
        PopupTipWindow(bx+3, by, name);
      Tip.type = num;
    } else {
      Tip.type = NO_TIP;
    }
  }

  if (Tip.type != NO_TIP) {
    if (!AlarmSet && !Tip.open) {
      alarm(1);
      AlarmSet = 1;
    }
    if (AlarmSet != SHOW_TIP && !Tip.open) 
      SetAlarm(SHOW_TIP);
  } else {
    if (AlarmSet) {
      alarm(0);
      AlarmSet = 0;
    }
    ClearAlarm();
    if (Tip.open) ShowTipWindow(0);
  }
}


/******************************************************************************
  LoopOnEvents - Process all the X events we get
******************************************************************************/
void LoopOnEvents()
{
  int  num;
  char tmp[100];
  XEvent Event;
  int x, x1, y, y1, redraw;
  static unsigned long lasttime = 0L;

  while(XPending(dpy)) {
    redraw = -1;
    XNextEvent(dpy, &Event);

    switch(Event.type) {
      case ButtonRelease:
        num = WhichButton(&buttons, Event.xbutton.x, Event.xbutton.y);
        if (num != -1) {
          ButReleased = ButPressed; /* Avoid race fvwm pipe */
          BelayHide = True; /* Don't AutoHide when function ends */
          SendFvwmPipe(ClickAction[Event.xbutton.button-1], 
                       ItemID(&windows, num));
          redraw = 0;
        }

        if (HighlightFocus) {
          if (num == ButPressed) RadioButton(&buttons, num, BUTTON_DOWN);
          if (num != -1) SendFvwmPipe("Focus 0", ItemID(&windows, num));
        }
        ButPressed = -1;
        break;

      case ButtonPress:
        RadioButton(&buttons, -1, BUTTON_UP); /* no windows focused anymore */
	if (MouseInStartButton(Event.xbutton.x, Event.xbutton.y)) {
	  StartButtonUpdate(NULL, BUTTON_DOWN);
          x = win_x;
          if (win_y < Midline) {
            /* bar in top half of the screen */
            y = win_y + RowHeight;
          } else {
            /* bar in bottom of the screen */
            y = win_y - ScreenHeight;
          }
          sprintf(tmp,"Popup %s %d %d", StartPopup, x, y);
          SendFvwmPipe(tmp, ItemID(&windows, num));
        } else if (MouseInMail(Event.xbutton.x, Event.xbutton.y)) {
           HandleMailClick(Event);
	} else {
          num = WhichButton(&buttons, Event.xbutton.x, Event.xbutton.y);
          UpdateButton(&buttons, num, NULL, (ButPressed == num) ?
                                              BUTTON_BRIGHT : BUTTON_DOWN);
 
/*	  UpdateButton(&buttons, num, NULL, BUTTON_DOWN);*/
          ButPressed = num;
	}
/*      else { / * Move taskbar * /

          XUngrabPointer(dpy, CurrentTime);
          SendFvwmPipe("Move", win);

        }
*/
        redraw = 0;
        break;

      case Expose:
        if (Event.xexpose.count == 0)
          if (Event.xexpose.window == Tip.win)
            redraw = 0;
          else
            redraw = 1;
        break;

      case ClientMessage:
        if ((Event.xclient.format==32) && (Event.xclient.data.l[0]==wm_del_win))
          ShutMeDown(0);
        break;

      case EnterNotify:
        if (AutoHide) RevealTaskBar();

        if (Event.xcrossing.mode != NotifyNormal) break;
        num = WhichButton(&buttons, Event.xcrossing.x, Event.xcrossing.y);
        if (!HighlightFocus) {
          if (SomeButtonDown(Event.xcrossing.state)) {
            if (num != -1) {
              RadioButton(&buttons, num, BUTTON_DOWN);
              ButPressed = num;
              redraw = 0;
            } else {
              ButPressed = -1;
            }
          }
        } else {
          if (num != -1 && num != ButPressed)
            SendFvwmPipe("Focus 0", ItemID(&windows, num));
        }

        CheckForTip(Event.xmotion.x, Event.xmotion.y);
        break;

      case LeaveNotify:
        ClearAlarm();
        if (Tip.open) ShowTipWindow(0);

        if (Event.xcrossing.mode != NotifyNormal) break;

        if (AutoHide) SetAlarm(HIDE_TASK_BAR);

        if (!HighlightFocus) {
          if (SomeButtonDown(Event.xcrossing.state)) {
            if (ButPressed != -1) {
              RadioButton(&buttons, -1, BUTTON_UP);
              ButPressed = -1;
              redraw = 0;
            }
          } else {
            if (ButReleased != -1) {
              RadioButton(&buttons, -1, BUTTON_UP);
              ButReleased = -1;
              redraw = 0;
            }
          }
        }
        break;

      case MotionNotify:
        num = WhichButton(&buttons, Event.xmotion.x, Event.xmotion.y);
        if (!HighlightFocus) {
          if (SomeButtonDown(Event.xmotion.state) && num != ButPressed) {
            if (num != -1) {
              RadioButton(&buttons, num, BUTTON_DOWN);
              ButPressed = num;
            } else {
              RadioButton(&buttons, num, BUTTON_UP);
              ButPressed = -1;
            }
            redraw = 0;
          }
        } else {
          if (num != -1 && num != ButPressed)
            SendFvwmPipe("Focus 0", ItemID(&windows, num));
        }

        CheckForTip(Event.xmotion.x, Event.xmotion.y);
        break;
      
      case ConfigureNotify:
        if ((Event.xconfigure.width != win_width ||
	     Event.xconfigure.height != win_height)) {
	  AdjustWindow(Event.xconfigure.width, Event.xconfigure.height);
          if (AutoStick) WarpTaskBar(win_y);
	  redraw = 1;
        }
        else if (Event.xconfigure.x != win_x || Event.xconfigure.y != win_y) {
          if (AutoStick) {
            WarpTaskBar(Event.xconfigure.y);
          } else {
            win_x = Event.xconfigure.x;
            win_y = Event.xconfigure.y;
          }
        }
        break;
    }

    if (redraw >= 0) RedrawWindow(redraw);

    if (Event.xkey.time - lasttime > UpdateInterval*1000L) {
      DrawGoodies();
      lasttime = Event.xkey.time;
    }
  }

} 

/***********************************
  AdjustWindow - Resize the window 
  **********************************/
void AdjustWindow(int width, int height)
{
  NRows = (height+2)/RowHeight;
  win_height = height;
  win_width = width;
  ArrangeButtonArray(&buttons);
}

/******************************************************************************
  makename - Based on the flags return me '(name)' or 'name'
******************************************************************************/
char *makename(char *string,long flags)
{
  char *ptr;

  ptr=safemalloc(strlen(string)+3);
  if (flags&ICONIFIED) strcpy(ptr,"(");
  else strcpy(ptr,"");
  strcat(ptr,string);
  if (flags&ICONIFIED) strcat(ptr,")");
  return ptr;
}

/******************************************************************************
  LinkAction - Link an response to a users action
******************************************************************************/
void LinkAction(char *string)
{
char *temp;
  temp=string;
  while(isspace(*temp)) temp++;
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
  StartMeUp - Do X initialization things
******************************************************************************/
void StartMeUp()
{
   XSizeHints hints;
   XGCValues gcval;
   unsigned long gcmask;
   unsigned int dummy1,dummy2;
   int x,y,ret,count;
   Window dummyroot,dummychild;

   if (!(dpy = XOpenDisplay(""))) {
      fprintf(stderr,"%s: can't open display %s", Module,
	      XDisplayName(""));
      exit (1);
   }
   x_fd = XConnectionNumber(dpy);
   screen= DefaultScreen(dpy);
   Root = RootWindow(dpy, screen);
   d_depth = DefaultDepth(dpy, screen);

   ScreenWidth  = XDisplayWidth(dpy, screen);
   ScreenHeight = XDisplayHeight(dpy, screen);

   Midline = (int) (ScreenHeight >> 1);
   
   if (selfont_string == NULL) selfont_string = font_string;

   if ((ButtonFont = XLoadQueryFont(dpy, font_string)) == NULL) {
     if ((ButtonFont = XLoadQueryFont(dpy, "fixed")) == NULL) {
       ConsoleMessage("Couldn't load fixed font. Exiting!\n");
       exit(1);
     }
   }
   if ((SelButtonFont = XLoadQueryFont(dpy, selfont_string)) == NULL) {
     if ((SelButtonFont = XLoadQueryFont(dpy, "fixed")) == NULL) {
       ConsoleMessage("Couldn't load fixed font. Exiting!\n");
       exit(1);
     }
   }
   
   fontheight = SelButtonFont->ascent + SelButtonFont->descent;

   NRows = 1;
   RowHeight = fontheight + 8;

   win_border = 4; /* default border width */
   win_height = RowHeight;
   win_width = ScreenWidth - (win_border << 1);

   ret = XParseGeometry(geometry, &hints.x, &hints.y,
	 	        (unsigned int *)&hints.width,
		        (unsigned int *)&hints.height);

   if (ret & YNegative)
     hints.y = ScreenHeight - (AutoHide ? 1 : win_height + (win_border<<1));
   else 
     hints.y = AutoHide ? 1 - win_height : 0;
   
   hints.flags=USPosition|PPosition|USSize|PSize|PResizeInc|
     PWinGravity|PMinSize|PMaxSize|PBaseSize;
   hints.x           = 0;
   hints.width       = win_width;
   hints.height      = RowHeight;
   hints.width_inc   = win_width;
   hints.height_inc  = RowHeight+2;
   hints.win_gravity = NorthWestGravity;
   hints.min_width   = win_width;
   hints.min_height  = RowHeight;
   hints.min_height  = win_height;
   hints.max_width   = win_width;
   hints.max_height  = RowHeight+7*(RowHeight+2) + 1;
   hints.base_width  = win_width;
   hints.base_height = RowHeight;

   win_x = hints.x;
   win_y = hints.y;

   if(d_depth < 2) {
     back = GetColor("white");
     fore = GetColor("black");
   } else {
     back = GetColor(BackColor);
     fore = GetColor(ForeColor);
   }

   win=XCreateSimpleWindow(dpy,Root,hints.x,hints.y,
			   hints.width,hints.height,0,
			   fore,back);

   wm_del_win=XInternAtom(dpy,"WM_DELETE_WINDOW",False);
   XSetWMProtocols(dpy,win,&wm_del_win,1);
   
   XSetWMNormalHints(dpy,win,&hints);
   
   XGrabButton(dpy,1,AnyModifier,win,True,GRAB_EVENTS,GrabModeAsync,
	       GrabModeAsync,None,None);
   XGrabButton(dpy,2,AnyModifier,win,True,GRAB_EVENTS,GrabModeAsync,
	       GrabModeAsync,None,None);
   XGrabButton(dpy,3,AnyModifier,win,True,GRAB_EVENTS,GrabModeAsync,
	       GrabModeAsync,None,None);

   /*   SetMwmHints(MWM_DECOR_ALL|MWM_DECOR_MAXIMIZE|MWM_DECOR_MINIMIZE,
	       MWM_FUNC_ALL|MWM_FUNC_MAXIMIZE|MWM_FUNC_MINIMIZE,
	       MWM_INPUT_MODELESS);
	       */
   gcmask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
   gcval.foreground = fore;
   gcval.background = back;
   gcval.font = SelButtonFont->fid;
   gcval.graphics_exposures = False;
   graph = XCreateGC(dpy,Root,gcmask,&gcval);

   if(d_depth < 2) 
     gcval.foreground = GetShadow(fore);
   else 
     gcval.foreground = GetShadow(back);
   gcval.background = back;
   gcmask = GCForeground | GCBackground | GCGraphicsExposures;
   shadow = XCreateGC(dpy,Root,gcmask,&gcval);

   gcval.foreground = GetHilite(back);
   gcval.background = back;
   hilite = XCreateGC(dpy,Root,gcmask,&gcval);
   
   gcval.foreground = GetColor("white");;
   gcval.background = back;
   whitegc = XCreateGC(dpy,Root,gcmask,&gcval);
   
   gcval.foreground = GetColor("black");
   gcval.background = back;
   blackgc = XCreateGC(dpy,Root,gcmask,&gcval);

   gcmask = GCForeground | GCBackground | GCTile | 
            GCFillStyle  | GCGraphicsExposures;
   gcval.foreground = GetHilite(back);
   gcval.background = back;
   gcval.fill_style = FillTiled;
   gcval.tile       = XCreatePixmapFromBitmapData(dpy, Root, (char *)gray_bits,
						  gray_width, gray_height,
						  gcval.foreground, gcval.background, d_depth);
   checkered = XCreateGC(dpy, Root, gcmask, &gcval);

   XSelectInput(dpy,win,(ExposureMask | KeyPressMask | PointerMotionMask |
                         EnterWindowMask | LeaveWindowMask |
                         StructureNotifyMask));
   /* ResizeRedirectMask |   */
   ChangeWindowName(&Module[1]);

   InitGoodies();

}

/******************************************************************************
  ShutMeDown - Do X client cleanup
******************************************************************************/
void ShutMeDown(int exitstat)
{
  FreeList(&windows);
  FreeAllButtons(&buttons);
  XFreeGC(dpy,graph);
  XDestroyWindow(dpy, win);
  XCloseDisplay(dpy);
  exit(exitstat);
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
      prop.flags = MWM_HINTS_DECORATIONS| MWM_HINTS_FUNCTIONS | MWM_HINTS_INPUT_MODE;
      
      /* HOP - LA! */
      XChangeProperty (dpy,win,
		       MwmAtom, MwmAtom,
		       32, PropModeReplace,
		       (unsigned char *)&prop,
		       PROP_MWM_HINTS_ELEMENTS);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *      ConstrainSize - adjust the given width and height to account for the
 *              constraints imposed by size hints
 *
 *      The general algorithm, especially the aspect ratio stuff, is
 *      borrowed from uwm's CheckConsistency routine.
 * 
 ***********************************************************************/
void ConstrainSize (XSizeHints *hints, int *widthp, int *heightp)
{
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
#define _min(a,b) (((a) < (b)) ? (a) : (b))

  
  int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
  int baseWidth, baseHeight;
  int dwidth = *widthp, dheight = *heightp;

  if(hints->flags & PMinSize)
    {
      minWidth = hints->min_width;
      minHeight = hints->min_height;
      if(hints->flags & PBaseSize)
	{
	  baseWidth = hints->base_width;
	  baseHeight = hints->base_height;
	}
      else
	{
	  baseWidth = hints->min_width;
	  baseHeight = hints->min_height;
	}
    }
  else if(hints->flags & PBaseSize)
    {
      minWidth = hints->base_width;
      minHeight = hints->base_height;
      baseWidth = hints->base_width;
      baseHeight = hints->base_height;
    }
  else
    {
      minWidth = 1;
      minHeight = 1;
      baseWidth = 1;
      baseHeight = 1;
    }
  
  if(hints->flags & PMaxSize)
    {
      maxWidth = hints->max_width;
      maxHeight = hints->max_height;
    }
  else
    {
      maxWidth = 10000;
      maxHeight = 10000;
    }
  if(hints->flags & PResizeInc)
    {
      xinc = hints->width_inc;
      yinc = hints->height_inc;
    }
  else
    {
      xinc = 1;
      yinc = 1;
    }
  
  /*
   * First, clamp to min and max values
   */
  if (dwidth < minWidth) dwidth = minWidth;
  if (dheight < minHeight) dheight = minHeight;
  
  if (dwidth > maxWidth) dwidth = maxWidth;
  if (dheight > maxHeight) dheight = maxHeight;
  
  
  /*
   * Second, fit to base + N * inc
   */
  dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
  dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;
  
  
  /*
   * Third, adjust for aspect ratio
   */
#define maxAspectX hints->max_aspect.x
#define maxAspectY hints->max_aspect.y
#define minAspectX hints->min_aspect.x
#define minAspectY hints->min_aspect.y
  /*
   * The math looks like this:
   *
   * minAspectX    dwidth     maxAspectX
   * ---------- <= ------- <= ----------
   * minAspectY    dheight    maxAspectY
   *
   * If that is multiplied out, then the width and height are
   * invalid in the following situations:
   *
   * minAspectX * dheight > minAspectY * dwidth
   * maxAspectX * dheight < maxAspectY * dwidth
   * 
   */
  
  if (hints->flags & PAspect)
    {
      if (minAspectX * dheight > minAspectY * dwidth)
	{
	  delta = makemult(minAspectX * dheight / minAspectY - dwidth,
			   xinc);
	  if (dwidth + delta <= maxWidth) 
	    dwidth += delta;
	  else
	    {
	      delta = makemult(dheight - dwidth*minAspectY/minAspectX,
			       yinc);
	      if (dheight - delta >= minHeight) dheight -= delta;
	    }
	}
      
      if (maxAspectX * dheight < maxAspectY * dwidth)
	{
	  delta = makemult(dwidth * maxAspectY / maxAspectX - dheight,
			   yinc);
	  if (dheight + delta <= maxHeight)
	    dheight += delta;
	  else
	    {
	      delta = makemult(dwidth - maxAspectX*dheight/maxAspectY,
			       xinc);
	      if (dwidth - delta >= minWidth) dwidth -= delta;
	    }
	}
    }
  
  *widthp = dwidth;
  *heightp = dheight;
  return;
}

/***********************************************************************
 WarpTaskBar -- Enforce AutoStick feature
 ***********************************************************************/
void WarpTaskBar(int y) {
  win_x = win_border;

  if (y < Midline)
    win_y = win_border;
  else
    win_y = (int)ScreenHeight - win_height - win_border;

  XMoveWindow(dpy, win, win_x, win_y);
  if (AutoHide) SetAlarm(HIDE_TASK_BAR);

  /* Prevent oscillations caused by race with 
     time delayed TaskBarHide().  Is there any way
     to prevent these Xevents from being sent
     to the server in the first place? */
  PurgeConfigEvents(); 
}

/***********************************************************************
 RevealTaskBar -- Make taskbar fully visible
 ***********************************************************************/
void RevealTaskBar() {
  ClearAlarm();

  if (win_y < Midline) 
    win_y = win_border;
  else 
    win_y = (int)ScreenHeight - win_height - win_border;

  BelayHide = False; 
  XMoveWindow(dpy, win, win_x, win_y);
}

/***********************************************************************
 HideTaskbar -- Make taskbar partially visible
 ***********************************************************************/
void HideTaskBar() {
  ClearAlarm();

  if (win_y < Midline) 
    win_y = 2 - win_height;
  else
    win_y = (int)ScreenHeight - 2;

  XMoveWindow(dpy, win, win_x, win_y);
}

/***********************************************************************
 SetAlarm -- Schedule a timeout event
 ************************************************************************/
void SetAlarm(int event) {
  AlarmSet = event;
  alarm(1);
}

/***********************************************************************
 ClearAlarm -- Disable timeout events
 ************************************************************************/
void ClearAlarm(void) {
  if(AlarmSet) {
    AlarmSet = NOT_SET;
    alarm(0);
  }
}

/***********************************************************************
 PurgeConfigEvents -- Wait for and purge ConfigureNotify events.
 ************************************************************************/
void PurgeConfigEvents(void) {
  XEvent Event;

  XPeekEvent(dpy, &Event);
  while (XCheckTypedWindowEvent(dpy, win, ConfigureNotify, &Event));
}

/************************************************************************
  X Error Handler
************************************************************************/
XErrorHandler ErrorHandler(Display *d, XErrorEvent *event)
  {
  char errmsg[256];

  XGetErrorText(d, event->error_code, errmsg, 256);
  ConsoleMessage("%s failed request: %s\n", Module, errmsg);
  ConsoleMessage("Major opcode: 0x%x, resource id: 0x%x\n",
                  event->request_code, event->resourceid);
  }
