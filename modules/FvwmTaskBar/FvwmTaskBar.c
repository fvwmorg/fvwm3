/* FvwmTaskBar Module for Fvwm.
 *
 * (Much reworked version of FvwmWinList)
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * Two little hacks by DYB: 1) make size of a strip in hidden state tunable
 * (with *FvwmTaskBar: AutoHide <pixels>); 2) make process of TaskBar
 * hiding/unhiding more smooth.
 *
 * Minor hack by TKP to enable autohide to work with pages (leaves 2 pixels
 * visible so that the 1 pixel panframes don't get in the way)
 *
 * Merciless hack by RBW for the Great Style Flag Rewrite - 04/20/1999.
 * Now the GSFR is used and there are no other flags (olicha may 19, 2000).
 *
 * The author makes not guarantees or warantees, either express or
 * implied.  Feel free to use any code contained here for any purpose, as long
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
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdarg.h>

#include <unistd.h>
#include <ctype.h>

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif /* Saul */

#include <stdlib.h>

#ifdef I18N_MB
#include <X11/Xlocale.h>
#endif

#include "libs/Module.h"
#include "libs/fvwmlib.h"  /* for pixmaps routines */
#include "libs/XineramaSupport.h"
#include "libs/safemalloc.h"
#include "libs/fvwmsignal.h"
#include "libs/Colorset.h"

#include "FvwmTaskBar.h"
#include "ButtonArray.h"
#include "List.h"
#include "Colors.h"
#include "Mallocs.h"
#include "Goodies.h"
#include "Start.h"

#define MAX_NO_ICON_ACTION_LENGTH (MAX_MODULE_INPUT_TEXT_LEN - 100)

#define GRAB_EVENTS (ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|EnterWindowMask|LeaveWindowMask)
#define SomeButtonDown(a) ((a) & DEFAULT_ALL_BUTTONS_MASK)

#define DEFAULT_CLICK1 "Iconify -1, Raise, Focus"
#define DEFAULT_CLICK2 "Iconify +1, Lower"
#define DEFAULT_CLICK3 "Nop"
#define DEFAULT_CLICK_N "Nop"

#define gray_width  8
#define gray_height 8
unsigned char gray_bits[] = { 0xaa, 0x55, 0xaa, 0x55,
                              0xaa, 0x55, 0xaa, 0x55 };

#define DEFAULT_VISIBLE_PIXELS 3

/* File type information */
int   Fvwm_fd[2];
int   x_fd;

/* X related things */
Display *dpy;
Window  Root, win;
int     screen;
Pixel back;
Pixel fore;
static Pixel iconfore;
static Pixel iconback;
static Pixel focusfore;
static Pixel focusback;
GC      icongraph = None;
GC      iconbackgraph = None;
GC      focusgraph = None;
GC      focusbackgraph = None;
GC      graph = None;
GC      shadow = None;
GC      hilite = None;
GC      iconshadow = None;
GC      iconhilite = None;
GC      focusshadow = None;
GC      focushilite = None;
GC      blackgc = None;
GC      whitegc = None;
GC      checkered = None;
int     colorset = -1;
int     iconcolorset = -1;
int     focuscolorset = -1;
XFontStruct *ButtonFont, *SelButtonFont;
#ifdef I18N_MB
XFontSet ButtonFontset, SelButtonFontset;
#endif
int fontheight;
static Atom wm_del_win;
Atom MwmAtom = None;
static Bool is_dead_pipe = False;

/* Module related information */
char *Module;
int  win_width    = 5,
     win_height   = 5,
     win_grav,
     win_x,
     win_y,
     win_border = 4,
     win_has_title = 0,
     win_has_bottom_title = 0,
     win_title_height = 0,
     win_is_shaded = 0,
     button_width = DEFAULT_BTN_WIDTH,
     Clength,
     ButPressed   = -1,
     ButReleased  = -1,
     Checked      = 0,
     WindowState  = -2, /* -2 unmaped, 1 not hidden, -1 hidden,
			 *  0 hidden -> not hidden (for the events loop) */
     FocusInWin = 0;    /* 1 if the Taskbar has the focus */

static volatile sig_atomic_t AlarmSet = NOT_SET;
static volatile sig_atomic_t tip_window_alarm = False;
static volatile sig_atomic_t hide_taskbar_alarm = False;


int UpdateInterval = 30;

ButtonArray buttons;
List windows;

char *ClickAction[NUMBER_OF_MOUSE_BUTTONS] =
{
  DEFAULT_CLICK1,
  DEFAULT_CLICK2,
  DEFAULT_CLICK3
},
     *EnterAction,
     *BackColor      = "white",
     *ForeColor      = "black",
     *IconBackColor  = "white",
     *IconForeColor  = "black",
     *FocusBackColor = NULL,
     *FocusForeColor = NULL,
     *geometry       = NULL,
     *font_string    = "fixed",
     *selfont_string = NULL;

static char *AnimCommand = NULL;

int UseSkipList    = False,
    UseIconNames   = False,
    ShowTransients = False,
    adjust_flag    = False,
    AutoStick      = False,
    AutoHide       = False,
    AutoFocus      = False,
    HighlightFocus = False,
    DeskOnly       = False,
    PageOnly       = False,
    ScreenOnly     = False,
    NoBrightFocus  = False,
    ThreeDfvwm     = False,
    RowsNumber     = 1;

int VisiblePixels  = DEFAULT_VISIBLE_PIXELS;
/* This macro should be used when calculating geometry */
#define VISIBLE_PIXELS() (VisiblePixels < win_height? VisiblePixels:win_height)

rectangle screen_g;
rectangle global_scr_g;

int NRows, RowHeight, Midline;

#define COUNT_LIMIT    10000
long DeskNumber;               /* Added by Balaji R */
int  First = 1;
int  Count = 0;

/* Imported from Goodies */
extern int stwin_width, goodies_width;
extern TipStruct Tip;

/* Imported from Start */
extern int StartButtonWidth, StartButtonHeight;
extern char *StartPopup;

char *ImagePath = NULL;
char *XineramaConfig = NULL;
static int xi_screen = 0;

static void ParseConfig( void );
static void ParseConfigLine(char *tline);
static void ShutMeDown(void);
static RETSIGTYPE TerminateHandler(int sig);
static RETSIGTYPE Alarm(int sig);
static void SetAlarm(int event);
static void ClearAlarm(void);
static void DoAlarmAction(void);
static int ErrorHandler(Display*, XErrorEvent*);
static Bool change_colorset(int cset, Bool force);
int IsItemIndexIconSuppressed(List *list, int i);

/******************************************************************************
  Main - Setup the XConnection,request the window list and loop forever
    Based on main() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
int main(int argc, char **argv)
{
  const char *temp;
  char *s;
  int i;

  for (i = 3; i < NUMBER_OF_MOUSE_BUTTONS; i++)
  {
    ClickAction[i] = DEFAULT_CLICK_N;
  }

  /* Save the program name for error messages and config parsing */
  temp = argv[0];
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

#ifdef I18N_MB
  setlocale(LC_CTYPE, "");
#endif

  if((argc != 6)&&(argc != 7)) {
    fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",temp,
	    VERSION);
    exit(1);
  }

  /* alise support */
  if (argc == 7 && argv[6] != NULL)
  {
    Module = safemalloc(strlen(argv[6])+2);
    strcpy(Module,"*");
    strcat(Module, argv[6]);
    Clength = strlen(Module);
  }
  else
  {
    Module = safemalloc(strlen(temp)+2);
    strcpy(Module,"*");
    strcat(Module, temp);
    Clength = strlen(Module);
  }

  /* setup fvwm pipes */
  Fvwm_fd[0] = atoi(argv[1]);
  Fvwm_fd[1] = atoi(argv[2]);

#ifdef HAVE_SIGACTION
  {
    struct sigaction  sigact;

    sigemptyset(&sigact.sa_mask);
#ifdef SA_RESTART
    sigact.sa_flags = SA_RESTART;
#else
    sigact.sa_flags = 0;
#endif
    sigact.sa_handler = Alarm;
    sigaction(SIGALRM, &sigact, NULL);

#ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT;
#else
    sigact.sa_flags = 0;
#endif
    sigaddset(&sigact.sa_mask, SIGPIPE);
    sigaddset(&sigact.sa_mask, SIGTERM);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigact.sa_handler = TerminateHandler;
    sigaction(SIGPIPE, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGINT,  &sigact, NULL);
  }
#else
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGTERM) | sigmask(SIGPIPE) | sigmask(SIGINT) );
#endif
  signal (SIGPIPE, TerminateHandler);
  signal (SIGTERM, TerminateHandler);
  signal (SIGINT,  TerminateHandler);
  signal (SIGALRM, Alarm);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGTERM, 1);
  siginterrupt(SIGINT, 1);
#endif
#endif

  SetMessageMask(Fvwm_fd,M_ADD_WINDOW | M_CONFIGURE_WINDOW | M_DESTROY_WINDOW |
		 M_WINDOW_NAME | M_ICON_NAME | M_RES_NAME | M_DEICONIFY |
		 M_ICONIFY | M_END_WINDOWLIST | M_FOCUS_CHANGE |
		 M_CONFIG_INFO | M_END_CONFIG_INFO | M_NEW_DESK | M_SENDCONFIG
#ifdef MINI_ICONS
		 | M_MINI_ICON
#endif
		);

  /* Parse the config file */
  ParseConfig();

  /* Setup the XConnection */
  StartMeUp();
  XSetErrorHandler(ErrorHandler);

  StartButtonInit(RowHeight);

  /* init the array of buttons */
  InitArray(&buttons, StartButtonWidth + 4, 0,
                      win_width - stwin_width - 8 - StartButtonWidth -10,
                      RowHeight, button_width);
  InitList(&windows);

  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendFvwmPipe(Fvwm_fd, "Send_WindowList",0);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(Fvwm_fd);

  /* Lock on send only for iconify and deiconify (for NoIconAction) */
  if (AnimCommand && (AnimCommand[0] != 0)) {
    SetSyncMask(Fvwm_fd, M_DEICONIFY | M_ICONIFY);
    SetNoGrabMask(Fvwm_fd, M_DEICONIFY | M_ICONIFY);
  }
  /* Receive all messages from Fvwm */
  EndLessLoop();
#ifdef FVWM_DEBUG_MSGS
  if ( isTerminated )
  {
    fprintf(stderr, "%s: Received signal: exiting...\n", Module);
  }
#endif
  return 0;
}


/******************************************************************************
  EndLessLoop -  Read and redraw until we get killed, blocking when can't read
******************************************************************************/
void EndLessLoop(void)
{
  fd_set readset;
  fd_set_size_t  fd_width;
  struct timeval tv;

  fd_width = x_fd;
  if (Fvwm_fd[1] > fd_width)
    fd_width = Fvwm_fd[1];
  ++fd_width;

  while ( !isTerminated )
  {
    FD_ZERO(&readset);
    FD_SET(Fvwm_fd[1], &readset);
    FD_SET(x_fd, &readset);

    tv.tv_sec  = UpdateInterval;
    tv.tv_usec = 0;

    XFlush(dpy);
    if ( fvwmSelect(fd_width, &readset, NULL, NULL, &tv) > 0 )
    {

      if (FD_ISSET(x_fd, &readset) || XPending(dpy))
        LoopOnEvents();

      if (FD_ISSET(Fvwm_fd[1], &readset))
        ReadFvwmPipe();
    }

    DoAlarmAction();

    DrawGoodies();
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
	DeadPipe(0);
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
  int redraw=-1;
  int i = -1;
  char *string;
  long Desk;
  Picture p;
  struct ConfigWinPacket  *cfgpacket;
  int iconified;
  rectangle new_g;

/*    memset(&p, 0, sizeof(Picture)); */

  switch(type)
  {
  case M_FOCUS_CHANGE:
    i = FindItem(&windows, body[0]);
    if (win != body[0])
    {
      /* This case is handled in LoopOnEvents() */
      if (IsItemIconified(&windows, body[0]))
	i = -1;
      RadioButton(&buttons, i, BUTTON_BRIGHT);
      ButPressed = i;
      ButReleased = -1;
      redraw = 0;
      FocusInWin = 0;
    } else
      FocusInWin = 1;
    break;

  case M_ADD_WINDOW:
  case M_CONFIGURE_WINDOW:
    cfgpacket = (ConfigWinPacket *) body;
    new_g.x = cfgpacket->frame_x;
    new_g.y = cfgpacket->frame_y;
    new_g.width = cfgpacket->frame_width;
    new_g.height = cfgpacket->frame_height;
    if (!ShowTransients && (IS_TRANSIENT(cfgpacket)))
      break;
    /* Notes: users cannot modify the win_width of the taskbar (see
       the hints in StartMeUp; it is a good idea). So "we have to" modify
       the win_width here by modifying the WMNormalHints. Moreover, 1. we want
       that the TaskBar width with its border is execately the screen width
       2. be carful with dynamic style change */
    if (cfgpacket->w == win)
    {
      if (win_border != (short)cfgpacket->border_width ||
	  win_title_height != (int)cfgpacket->title_height ||
	  win_has_title != HAS_TITLE(cfgpacket) ||
	  win_has_bottom_title != HAS_BOTTOM_TITLE(cfgpacket))
      {
	XSizeHints hints;
	long dumy;

	win_border = (short)cfgpacket->border_width;
	win_width = screen_g.width-(win_border<<1);
	win_title_height = (int)cfgpacket->title_height;
	win_has_title = HAS_TITLE(cfgpacket);
	win_has_bottom_title = HAS_BOTTOM_TITLE(cfgpacket);

	if (AutoStick)
	{
	  win_x = screen_g.x + win_border;
	  if (!win_is_shaded)
	  {
	    if (win_y > Midline)
	    {
	      win_y = screen_g.height -
		(AutoHide ?
		 VISIBLE_PIXELS() - win_title_height*win_has_bottom_title :
		 win_height + win_border);
	    }
	    else
	    {
	      win_y = AutoHide ?
		VISIBLE_PIXELS() + win_title_height*win_has_bottom_title
		- win_height : win_border + win_title_height;
	    }
	  }
	  else
	  {
	    if (win_y > Midline)
	    {
	      win_y = screen_g.height - win_border -
		(win_has_bottom_title)*win_height;
	    }
	    else
	    {
	      win_y = win_title_height + win_border -
		(win_has_bottom_title)*win_height;
	    }
	  }
	  win_y += screen_g.y;
	}

	XGetWMNormalHints(dpy,win,&hints,&dumy);
	hints.min_width   = win_width;
	hints.base_width  = win_width;
	hints.max_width   = win_width;

	XSetWMNormalHints(dpy,win,&hints);

 	if (AutoStick)
	  XMoveResizeWindow(dpy, win, win_x, win_y, win_width, win_height);
	else
	  XResizeWindow(dpy, win, win_width, win_height);

	UpdateArray(&buttons, -1, -1,
		    win_width - stwin_width - 8 - StartButtonWidth -10,-1, -1);
	ArrangeButtonArray (&buttons);
      }
      if (AutoStick && win_is_shaded != IS_SHADED(cfgpacket))
      {
	win_is_shaded = IS_SHADED(cfgpacket);
	if (win_is_shaded)
	{
	  if (win_y > Midline)
	  {
	    win_y = screen_g.height - win_border -
	      (win_has_bottom_title)*win_height;
	  }
	  else
	  {
	    win_y = win_title_height + win_border -
	      (win_has_bottom_title)*win_height;
	  }
	}
	else
	{
	  if (win_y > Midline)
	  {
	    win_y = screen_g.height - win_border - win_height;
	  }
	  else
	  {
	    win_y = win_title_height + win_border;
	  }
	  if (AutoHide)
	  {
	    WindowState = 1;
	    SetAlarm(HIDE_TASK_BAR);
	  }
	}
	win_y += screen_g.y;
	XMoveWindow(dpy, win, win_x, win_y);
      }
      break;
    }

    if ((i=FindItem(&windows, body[0])) != -1)
    {
      int remove=0;
      int add = 0;
      int skiplist = 0;
      rectangle *pold_g;
      Bool is_on_desk = True;
      Bool is_changing_desk = False;
      Bool is_on_page = True;
      Bool is_changing_page = False;
      Bool is_on_screen = True;
      Bool is_changing_screen = False;
      Bool is_managed;

      if (!GetItemGeometry(&windows, i, &pold_g))
      {
	pold_g = &new_g;
      }

      if (DeskOnly && GetDeskNumber(&windows,i,&Desk))
      {
	if (IsItemIndexSticky(&windows,i))
	{
	  is_on_desk = True;
	  is_changing_desk = False;
	}
	else if (!IsItemIndexSticky(&windows,i))
	{
	  is_on_desk = (DeskNumber != cfgpacket->desk);
	  is_changing_desk = (Desk != cfgpacket->desk);
	}
	UpdateItemIndexDesk(&windows, i, cfgpacket->desk);
      }
      if (PageOnly)
      {
	is_on_page = fvwmrect_do_rectangles_intersect(&new_g, &global_scr_g);
	is_changing_page =
	  (fvwmrect_do_rectangles_intersect(pold_g, &global_scr_g) !=
	   is_on_page);
      }
      if (ScreenOnly)
      {
	is_on_screen = fvwmrect_do_rectangles_intersect(&new_g, &screen_g);
	is_changing_screen =
	  (fvwmrect_do_rectangles_intersect(pold_g, &screen_g) !=
	   is_on_screen);
      }
      UpdateItemIndexGeometry(&windows, i, &new_g);

      is_managed = (is_on_desk && is_on_page && is_on_screen);
      if (is_changing_desk || is_changing_page || is_changing_screen)
      {
	if (is_managed)
	{
	  /* window entering managed area of the virtual desktop */
	  add = 1;
	}
	else
	{
	  /* window leaving managed area of the virtual desktop */
	  remove = 1;
	}
      }

      if (UseSkipList && is_managed)
      {
	skiplist = IsItemIndexSkipWindowList(&windows,i);
	if (DO_SKIP_WINDOW_LIST(cfgpacket) && !skiplist)
	{
	  add = 0;
	  remove = 1;
	}
	if (!DO_SKIP_WINDOW_LIST(cfgpacket) && skiplist) {
	  add = 1;
	  remove = 0;
	}
      }

      UpdateItemGSFRFlags(&windows, cfgpacket);
      if (remove)
      {
	RemoveButton(&buttons, i);
	redraw = 1;
      }
      else if (add)
      {
	AddButton(&buttons, ItemName(&windows,i),
		  GetItemPicture(&windows,i), BUTTON_UP, i,
		  IsItemIndexIconified(&windows, i));
	redraw = 1;
      }
    }
    else
    {
      AddItem(&windows, cfgpacket->w,
	      cfgpacket, cfgpacket->desk, Count++);
      if (Count > COUNT_LIMIT)
	Count = 0;
      i = FindItem(&windows, cfgpacket->w);
      UpdateItemIndexGeometry(&windows, i, &new_g);
    }
    if (i != -1)
    {
      Button *temp = find_n(&buttons, i);
      if (temp && IS_ICONIFIED(cfgpacket))
	temp->iconified = 1;
    }
    break;

  case M_DESTROY_WINDOW:
    if ((i = FindItem(&windows, body[0])) == -1)
      break;
      DeleteItem(&windows, body[0]);
      RemoveButton(&buttons, i);
      redraw = 1;
    break;

#ifdef MINI_ICONS
  case M_MINI_ICON:
    if ((i = FindItem(&windows, body[0])) == -1) break;
    p.picture = body[6];
    p.mask    = body[7];
    p.width   = body[3];
    p.height  = body[4];
    p.depth   = body[5];
    UpdateItemPicture(&windows, i, &p);
    if (UpdateButton(&buttons, i, NULL, DONT_CARE) != -1) {
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
    if ((i = UpdateItemName(&windows, body[0], (char *)&body[3])) == -1)
      break;
    if (UpdateButton(&buttons, i, string, DONT_CARE) == -1)
    {
      rectangle *pold_g;

      if (GetDeskNumber(&windows, i, &Desk) == 0)
	break;
      if (DeskOnly && Desk != DeskNumber)
	break;
      if (!GetItemGeometry(&windows, i, &pold_g))
	break;
      if (PageOnly && !fvwmrect_do_rectangles_intersect(pold_g, &global_scr_g))
	break;
      if (ScreenOnly && !fvwmrect_do_rectangles_intersect(pold_g, &screen_g))
	break;
      if (UseSkipList && IsItemIndexSkipWindowList(&windows, i))
	break;
      AddButton(&buttons, string, NULL, BUTTON_UP, i,
		IsItemIndexIconified(&windows, i));
      redraw = 1;
    }
    else
      redraw = 0;
    break;

  case M_DEICONIFY:
  case M_ICONIFY:
    /* fwvm will wait for an Unlock message before continuing
     * be careful when changing this construct, make sure unlock happens
     * if AnimCommand && AnimCommand[0] != 0 */
    iconified = IsItemIconified(&windows, body[0]);
    if (((i = FindItem(&windows, body[0])) == -1)
	|| (type == M_DEICONIFY && !iconified)
	|| (type == M_ICONIFY   && iconified))
    {
      if (AnimCommand && AnimCommand[0] != 0)
	SendUnlockNotification(Fvwm_fd);
      break;
    }

    iconified = !iconified;
    UpdateItemIconifiedFlag(&windows, body[0], iconified);
    {
      Button *temp = find_n(&buttons, i);
      if (temp)
      {
	if (AnimCommand && (AnimCommand[0] != 0)
	    && IsItemIndexIconSuppressed(&windows,i))
	{
	  char buff[MAX_MODULE_INPUT_TEXT_LEN];
	  Window child;
	  int x, y;
	  int abs_x, abs_y;

	  ButtonCoordinates(&buttons, i, &x, &y);
	  XTranslateCoordinates(dpy, win, Root, x, y,
				&abs_x, &abs_y, &child);
	  if (type == M_DEICONIFY) {
	    sprintf(buff, "%s %d %d %d %d %d %d %d %d", AnimCommand,
		    abs_x, abs_y, buttons.tw-2, RowHeight,
		    (int)body[7], (int)body[8], (int)body[9],
		    (int)body[10]);
	  } else {
	    sprintf(buff, "%s %d %d %d %d %d %d %d %d", AnimCommand,
		    (int)body[7], (int)body[8], (int)body[9], (int)body[10],
		    abs_x, abs_y, buttons.tw-2, RowHeight);
	  }
	  SendText(Fvwm_fd, buff, 0);
	}
	temp->needsupdate = 1;
  	temp->iconified = iconified;
	DrawButtonArray(&buttons, 0);
      }
      if (AnimCommand && AnimCommand[0] != 0)
	SendUnlockNotification(Fvwm_fd);
    }
    if (type == M_ICONIFY && i == ButReleased) {
      RadioButton(&buttons, -1, BUTTON_UP);
      ButReleased = ButPressed = -1;
      redraw = 0;
    }
    break;

  case M_END_WINDOWLIST:
    AdjustWindow(win_width, win_height);
    XMapRaised(dpy, win);
    if (AutoHide)
      WindowState = -1;
    else
      WindowState = 1;
    break;

  case M_NEW_DESK:
    DeskNumber = body[0];
    if (!First && DeskOnly)
      redraw_buttons();
    else
      First = 0;
    break;

  case M_NEW_PAGE:
    break;

  case M_CONFIG_INFO:
    {
      char *tline;
      int cset;

      tline = (char*)&(body[3]);
      if (strncasecmp(tline, "Colorset", 8) == 0)
      {
	cset = LoadColorset(tline + 8);
	if (change_colorset(cset, False))
	{
	  redraw = 1;
	}
      }
      else if (strncasecmp(tline, XINERAMA_CONFIG_STRING,
			   strlen(XINERAMA_CONFIG_STRING)) == 0)
      {
	XineramaSupportConfigureModule(tline + strlen(XINERAMA_CONFIG_STRING));
      }
    }
    break;
  } /* switch */

  if (redraw >= 0)
    RedrawWindow(redraw);
}

void redraw_buttons()
{
  Item *item;

  FreeAllButtons(&buttons);

  for (item=windows.head; item; item=item->next)
  {
    if ((DeskNumber == item->Desk || IS_STICKY(item)) &&
	(!DO_SKIP_WINDOW_LIST(item) || !UseSkipList))
    {
      AddButton(&buttons, item->name, &(item->p), BUTTON_UP, item->count,
		IS_ICONIFIED(item));
    }
  }

  RedrawWindow(1);
}


/***********************************************************************
  Detected a broken pipe - time to exit
    Based on DeadPipe() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
 **********************************************************************/
void DeadPipe(int nonsense)
{
  is_dead_pipe = True;
  exit(1);
}

/**********************************************************************
  Reentrant signal handler that will tell the application to quit.
    It is not safe to just call "exit()" here; instead we will
    just set a flag that will break the event-loop when the
    handler returns.
 **********************************************************************/
static RETSIGTYPE
TerminateHandler(int sig)
{
  fvwmSetTerminate(sig);
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
  if (Tip.open)
  {
    RedrawTipWindow();

    /* 98-11-21 13:45, Mohsin_Ahmed, mailto:mosh@sasi.com. */
    if( Tip.type >= 0 && AutoFocus )
    {
      SendFvwmPipe(Fvwm_fd, "Iconify off, Raise, Focus",
		    ItemID( &windows, Tip.type ) );
    }
  }

  if (force)
  {
    XClearArea (dpy, win, 0, 0, win_width, win_height, False);
    DrawGoodies();
  }
  DrawButtonArray(&buttons, force);
  StartButtonDraw(force);
  if (XQLength(dpy) && !force)
    LoopOnEvents();
}

static char *configopts[] =
{
  "imagepath",
  "colorset",
  XINERAMA_CONFIG_STRING,
  NULL
};

static char *moduleopts[] =
{
  "Font",
  "SelFont",
  "Geometry",
  "Fore",
  "Back",
  "Colorset",
  "IconFore",
  "IconBack",
  "IconColorset",
  "Action",
  "UseSkipList",
  "AutoFocus",
  "AutoStick",
  "AutoHide",
  "UseIconNames",
  "ShowTransients",
  "DeskOnly",
  "UpdateInterval",
  "HighlightFocus",
  "ButtonWidth",
  "NoIconAction",
  "NoBrightFocus",
  "FocusFore",
  "FocusBack",
  "FocusColorset",
  "3DFvwm",
  "Rows",
  "PageOnly",
  "ScreenOnly",
  NULL
};

void ParseConfig(void)
{
  char *buf;

  InitGetConfigLine(Fvwm_fd,Module);
  while (GetConfigLine(Fvwm_fd,&buf), buf != NULL)
  {
    ParseConfigLine(buf);
  } /* end config lines */
} /* end function */

static void ParseConfigLine(char *tline)
{
  char *rest;
  int index;

  while (isspace((unsigned char)*tline))
    tline++;

  if (strncasecmp(tline, Module, Clength) != 0)
  {
    /* Non module spcific option */
    index = GetTokenIndex(tline, configopts, -1, &rest);
    while (*rest && *rest != '\n' && isspace(*rest))
      rest++;
    switch(index)
    {
    case 0: /* imagepath */
      if (ImagePath)
      {
	free(ImagePath);
      }
      CopyString(&ImagePath, rest);
      break;
    case 1: /* colorset */
      LoadColorset(rest);
      break;
    case 2: /* XINERAMA_CONFIG_STRING */
      if (XineramaConfig)
      {
	free(XineramaConfig);
      }
      CopyString(&XineramaConfig, rest);
      break;
    default:
      /* unknown option */
      break;
    }
  } /* if options */
  else
  {
    /* option beginning with '*ModuleName' */
    rest = tline + Clength;
    index = GetTokenIndex(rest, moduleopts, -1, &rest);
    while (*rest && *rest != '\n' && isspace(*rest))
      rest++;
    switch(index)
    {
    case 0: /* Font */
      CopyString(&font_string, rest);
      break;
    case 1: /* Selfont */
      CopyString(&selfont_string, rest);
      break;
    case 2: /* Geometry */
      while (isspace((unsigned char)*rest) && *rest != '\n' && *rest != 0)
	rest++;
      UpdateString(&geometry, rest);
      break;
    case 3: /* Fore */
      CopyString(&ForeColor, rest);
      colorset = -1;
      break;
    case 4: /* Back */
      CopyString(&BackColor, rest);
      colorset = -1;
      break;
    case 5: /* Colorset */
      colorset = -1;
      colorset = atoi(rest);
      AllocColorset(colorset);
      break;
    case 6: /* IconFore */
      CopyString(&IconForeColor, rest);
      iconcolorset = -1;
      break;
    case 7: /* IconBack */
      CopyString(&IconBackColor, rest);
      iconcolorset = -1;
      break;
    case 8: /* IconColorset */
      iconcolorset = -1;
      iconcolorset = atoi(rest);
      AllocColorset(iconcolorset);
      break;
    case 9: /* Action */
      LinkAction(rest);
      break;
    case 10: /* UseSkipList */
      UseSkipList=True;
      break;
    case 11: /* AutoFocus */
      AutoFocus=True;
      break;
    case 12: /* AutoStick */
      AutoStick=True;
      break;
    case 13: /* AutoHide */
      AutoHide=True;
      AutoStick=True;
      VisiblePixels=atoi(rest);
      if (VisiblePixels < 1) VisiblePixels = DEFAULT_VISIBLE_PIXELS;
      break;
    case 14: /* UseIconNames */
      UseIconNames=True;
      break;
    case 15: /* ShowTransients */
      ShowTransients=True;
      break;
    case 16: /* DeskOnly */
      DeskOnly=True;
      break;
    case 17: /* UpdateInterval */
      UpdateInterval = atoi(rest);
      break;
    case 18: /* HighlightFocus */
      HighlightFocus=True;
      break;
    case 19: /* ButtonWidth */
      button_width = atoi(rest);
      break;
    case 20: /* NoIconAction */
      CopyString(&AnimCommand, rest);
      if (strlen(AnimCommand) > MAX_NO_ICON_ACTION_LENGTH)
	AnimCommand[MAX_NO_ICON_ACTION_LENGTH] = 0;
      break;
    case 21: /* NoBrightFocus */
      NoBrightFocus = True;
      break;
    case 22: /* FocusFore */
      CopyString(&FocusForeColor, rest);
      focuscolorset = -1;
      break;
    case 23: /* FocusBack */
      CopyString(&FocusBackColor, rest);
      focuscolorset = -1;
      break;
    case 24: /* FocusColorset */
      focuscolorset = -1;
      focuscolorset = atoi(rest);
      AllocColorset(focuscolorset);
      break;
    case 25: /* 3DFvwm */
      ThreeDfvwm = True;
      break;
    case 26: /* Rows */
      RowsNumber = atoi(rest);
      if (!(1 <= RowsNumber && RowsNumber <= 8)) {
	RowsNumber = 1;
      }
      break;
    case 27: /* PageOnly */
fprintf(stderr,"page only\n");
      PageOnly=True;
      break;
    case 28: /* ScreenOnly */
fprintf(stderr,"screen only\n");
      ScreenOnly=True;
      break;
    default:
      if (!GoodiesParseConfig(tline) &&
	  !StartButtonParseConfig(tline))
      {
	fprintf(stderr,"%s: unknown configuration option %s", Module, tline);
      }
      break;
    } /* switch */
  } /* else options */
}

/******************************************************************************
  Alarm - Handle a SIGALRM - used to implement timeout events
******************************************************************************/
static RETSIGTYPE
Alarm(int nonsense)
{

  switch(AlarmSet)
  {
  case SHOW_TIP:
    tip_window_alarm = True;
    break;

  case HIDE_TASK_BAR:
      hide_taskbar_alarm = True;
    break;
  }

  AlarmSet = NOT_SET;
#if !defined(HAVE_SIGACTION) && !defined(USE_BSD_SIGNALS)
  signal (SIGALRM, Alarm);
#endif
}

/******************************************************************************
  DoAlarmAction -
******************************************************************************/
void
DoAlarmAction(void)
{

  if (hide_taskbar_alarm)
  {
    hide_taskbar_alarm = False;
    HideTaskBar();
  }
  else if (tip_window_alarm)
  {
    tip_window_alarm = False;
    if (AutoHide && WindowState == 0)
    {
      Window dummy_rt, dummy_c;
      int abs_x, abs_y, pos_x, pos_y;
      unsigned int dummy;
      XEvent sevent;

      /* We are now "sure" that the TaskBar is not hidden for the
	 Event loop. We send a motion notify for activating tips */
      WindowState = 1;
      XQueryPointer(dpy, win, &dummy_rt,&dummy_c, &abs_x, &abs_y,
		    &pos_x, &pos_y, &dummy);
      sevent.xmotion.x = pos_x;
      sevent.xmotion.y = pos_y;
      sevent.xany.type = MotionNotify;
      sevent.xmotion.state = 0;
      XSendEvent(dpy, win, False, EnterWindowMask, &sevent);
      Tip.type = NO_TIP;
    }
    else
      ShowTipWindow(1);
  }
}

/******************************************************************************
  CheckForTip - determine when to popup the tip window
******************************************************************************/
void CheckForTip(int x, int y)
{
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
    if (num != -1) {
      if ((Tip.type != num)  ||
          (Tip.text == NULL) || (!trunc && (strcmp(name, Tip.text) != 0)))
        PopupTipWindow(bx+3, by, name);
      Tip.type = num;
    } else {
      Tip.type = NO_TIP;
    }
  }

  if (Tip.type != NO_TIP) {
    if (AlarmSet != SHOW_TIP && !Tip.open)
      SetAlarm(SHOW_TIP);
  }
  else {
    ClearAlarm();
    if (Tip.open) ShowTipWindow(0);
  }
}


/******************************************************************************
  LoopOnEvents - Process all the X events we get
******************************************************************************/
void LoopOnEvents(void)
{
  int  num = 0;
  char tmp[100];
  XEvent Event;
  XEvent Event2;
  int x, y, redraw;
  static unsigned long lasttime = 0L;
  Time NewTimestamp = lasttime;

  while(XPending(dpy))
  {
    redraw = -1;
    XNextEvent(dpy, &Event);
    if (Event.xany.type == ConfigureNotify)
    {
      /* Purge all but the last configure events */
      while (XCheckTypedEvent(dpy, ConfigureNotify, &Event))
	;
    }

    NewTimestamp = lasttime;
    switch(Event.type)
    {
    case ButtonRelease:
      NewTimestamp = Event.xbutton.time;
      num = WhichButton(&buttons, Event.xbutton.x, Event.xbutton.y);
      if (num == -1) {
	if (MouseInStartButton(Event.xbutton.x, Event.xbutton.y))
	  StartButtonUpdate(NULL, BUTTON_UP);
      } else {
	ButReleased = ButPressed; /* Avoid race fvwm pipe */
	if (Event.xbutton.button >= 1 &&
	    Event.xbutton.button <= NUMBER_OF_MOUSE_BUTTONS)
	{
	  SendFvwmPipe(Fvwm_fd, ClickAction[Event.xbutton.button-1],
		       ItemID(&windows, num));
	}
      }

      if (MouseInStartButton(Event.xbutton.x, Event.xbutton.y)) {
	StartButtonUpdate(NULL, BUTTON_UP);
	redraw = 0;
	usleep(50000);
      }
      if (HighlightFocus) {
	if (num == ButPressed)
	  RadioButton(&buttons, num, BUTTON_DOWN);
	if (num != -1)
	  SendFvwmPipe(Fvwm_fd, "Focus 0", ItemID(&windows, num));
      }
      ButPressed = -1;
      redraw = 0;
      break;

    case ButtonPress:
      NewTimestamp = Event.xbutton.time;
      RadioButton(&buttons, -1, BUTTON_UP); /* no windows focused anymore */
      if (MouseInStartButton(Event.xbutton.x, Event.xbutton.y)) {
	StartButtonUpdate(NULL, BUTTON_DOWN);
	x = win_x;
	if (win_y < Midline) {
	  /* bar in top half of the screen */
	  y = win_y + RowHeight;
	} else {
	  /* bar in bottom of the screen */
	  y = win_y - screen_g.height;
	}
	sprintf(tmp,"Popup %s %d %d", StartPopup, x, y);
	SendFvwmPipe(Fvwm_fd, tmp, 0);
      } else {
	StartButtonUpdate(NULL, BUTTON_UP);
	if (MouseInMail(Event.xbutton.x, Event.xbutton.y)) {
	  HandleMailClick(Event);
	} else {
	  num = WhichButton(&buttons, Event.xbutton.x, Event.xbutton.y);
	  UpdateButton(&buttons, num, NULL, (ButPressed == num) ?
		       BUTTON_BRIGHT : BUTTON_DOWN);

	  ButPressed = num;
	}
      }
      redraw = 0;
      break;

    case Expose:
      if (Event.xexpose.count == 0)
      {
	if (Event.xexpose.window == Tip.win)
	  redraw = 0;
	else
	  redraw = 1;
      }
      break;

    case ClientMessage:
      if ((Event.xclient.format==32) && (Event.xclient.data.l[0]==wm_del_win))
	exit(0);
      break;

    case EnterNotify:
      NewTimestamp = Event.xcrossing.time;
      if (AutoHide)
	RevealTaskBar();

      if (Event.xcrossing.mode != NotifyNormal)
	break;
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
	  SendFvwmPipe(Fvwm_fd, "Focus 0", ItemID(&windows, num));
      }

      CheckForTip(Event.xmotion.x, Event.xmotion.y);
      break;

    case LeaveNotify:
      NewTimestamp = Event.xcrossing.time;
      ClearAlarm();
      if (Tip.open)
	ShowTipWindow(0);

      if (AutoHide)
	SetAlarm(HIDE_TASK_BAR);

      if (Event.xcrossing.mode != NotifyNormal)
	break;

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
      NewTimestamp = Event.xmotion.time;
      if (MouseInStartButton(Event.xmotion.x, Event.xbutton.y)) {
	if (SomeButtonDown(Event.xmotion.state))
	  redraw = StartButtonUpdate(NULL, BUTTON_DOWN) ? 0 : -1;
	CheckForTip(Event.xmotion.x, Event.xmotion.y);
	break;
      }
      redraw = StartButtonUpdate(NULL, BUTTON_UP) ? 0 : -1;
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
      } else if (num != -1 && num != ButPressed)
	SendFvwmPipe(Fvwm_fd, "Focus 0", ItemID(&windows, num));

      CheckForTip(Event.xmotion.x, Event.xmotion.y);
      break;

    case ConfigureNotify:
      memcpy(&Event2, &Event, sizeof(Event));
      /* eat up excess ConfigureNotify events. */
      while (XCheckTypedWindowEvent(dpy, win, ConfigureNotify, &Event2))
      {
	memcpy(&Event, &Event2, sizeof(Event));
      }
      if (Event.xconfigure.height != win_height) {
	AdjustWindow(Event.xconfigure.width, Event.xconfigure.height);
	if (AutoHide && !win_is_shaded)
	{
	  if (win_y > Midline)
	  {
	    win_y = screen_g.height - VISIBLE_PIXELS() +
	      win_title_height*win_has_bottom_title;
	  }
	  else
	  {
	    win_y = VISIBLE_PIXELS() + win_title_height*win_has_bottom_title
	      - win_height;
	  }
	  win_y += screen_g.y;
	  XSync(dpy,0);
	  XMoveWindow(dpy, win, win_x, win_y);
	  XSync(dpy,0);
	  hide_taskbar_alarm = False;
	  WindowState = -1;
	}
	else if (AutoStick)
	  WarpTaskBar(win_y, 1);
	redraw = 1;
      }
      /* useful because of dynamic style change */
      if (!Event.xconfigure.send_event)
      {
	PurgeConfigEvents();
	break;
      }
      else if (AutoHide)
	break;
      else if (Event.xconfigure.x != win_x || Event.xconfigure.y != win_y)
      {
	if (AutoStick)
	{
	  WarpTaskBar(Event.xconfigure.y, 0);
	}
	else
	{
	  win_x = Event.xconfigure.x;
	  win_y = Event.xconfigure.y;
	}
      }
      break;
    } /* switch (Event.type) */

    if (redraw >= 0)
      RedrawWindow(redraw);
    DoAlarmAction();
    if (NewTimestamp - lasttime > UpdateInterval*1000L)
    {
      DrawGoodies();
      lasttime = NewTimestamp;
    }
  }

  return;
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
  change_colorset(0, False);
}

/******************************************************************************
  LinkAction - Link an response to a users action
******************************************************************************/
void LinkAction(const char *string)
{
  const char *temp=string;
  while(isspace((unsigned char)*temp)) temp++;

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

static void CreateOrUpdateGCs(void)
{
   XGCValues gcval;
   unsigned long gcmask;
   Pixel pfore;
   Pixel pback;
   Pixel piconfore;
   Pixel piconback;
   Pixel piconhilite;
   Pixel piconshadow;
   Pixel pfocusfore;
   Pixel pfocusback;
   Pixel pfocushilite;
   Pixel pfocusshadow;
   Pixel philite;
   Pixel pshadow;

   if (colorset >= 0)
   {
     pfore = Colorset[colorset].fg;
     pback = Colorset[colorset].bg;
     philite = Colorset[colorset].hilite;
     pshadow = Colorset[colorset].shadow;
   }
   else
   {
     pfore = fore;
     pback = back;
     philite = GetHilite(back);
     if(Pdepth < 2)
       pshadow = GetShadow(fore);
     else
       pshadow = GetShadow(back);
   }
   if (iconcolorset >= 0)
   {
     piconfore = Colorset[iconcolorset].fg;
     piconback = Colorset[iconcolorset].bg;
     piconhilite = Colorset[iconcolorset].hilite;
     piconshadow = Colorset[iconcolorset].shadow;
   }
   else
   {
     piconfore = iconfore;
     piconback = iconback;
     piconhilite = GetHilite(iconback);
     if(Pdepth < 2)
       piconshadow = GetShadow(iconfore);
     else
       piconshadow = GetShadow(iconback);
   }
   if (focuscolorset >= 0)
   {
     pfocusfore = Colorset[focuscolorset].fg;
     pfocusback = Colorset[focuscolorset].bg;
     pfocushilite = Colorset[focuscolorset].hilite;
     pfocusshadow = Colorset[focuscolorset].shadow;
   }
   else
   {
     pfocusshadow = pshadow;
     if (FocusForeColor != NULL)
     {
       pfocusfore = focusfore;
       if(Pdepth < 2)
	 pfocusshadow = GetShadow(focusfore);
     }
     else
     {
       pfocusfore = fore;
     }
     if (FocusBackColor != NULL)
     {
       pfocusback = focusback;
       pfocushilite = GetHilite(focusback);
       if(!(Pdepth < 2))
	 pfocusshadow = GetShadow(focusback);
     }
     else
     {
       pfocusback = back;
       pfocushilite = philite;
     }
   }

   /* only the foreground changes for all GCs */
   gcval.background = pback;
   gcval.font = SelButtonFont->fid;
   gcval.graphics_exposures = False;

   gcmask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
   /* Normal */
   gcval.foreground = pfore;
   if (graph)
     XChangeGC(dpy,graph,gcmask,&gcval);
   else
     graph = fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

   /* iconified */
   gcval.foreground = piconfore;
   if (iconbackgraph)
     XChangeGC(dpy,icongraph,gcmask,&gcval);
   else
     icongraph = fvwmlib_XCreateGC(dpy,Root,gcmask,&gcval);

   gcval.foreground = piconback;
   if (iconbackgraph)
     XChangeGC(dpy,iconbackgraph,gcmask,&gcval);
   else
     iconbackgraph = fvwmlib_XCreateGC(dpy,Root,gcmask,&gcval);

   gcval.foreground = piconshadow;
   if (iconshadow)
     XChangeGC(dpy,iconshadow,gcmask,&gcval);
   else
     iconshadow = fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

   gcval.foreground = piconhilite;
   if (iconhilite)
     XChangeGC(dpy,iconhilite,gcmask,&gcval);
   else
     iconhilite = fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

   /* focused */
   gcval.foreground = pfocusfore;
   if (focusbackgraph)
     XChangeGC(dpy,focusgraph,gcmask,&gcval);
   else
     focusgraph = fvwmlib_XCreateGC(dpy,Root,gcmask,&gcval);

   gcval.foreground = pfocusback;
      if (focusbackgraph)
     XChangeGC(dpy,focusbackgraph,gcmask,&gcval);
   else
     focusbackgraph = fvwmlib_XCreateGC(dpy,Root,gcmask,&gcval);

   gcval.foreground = pfocusshadow;
   if (focusshadow)
     XChangeGC(dpy,focusshadow,gcmask,&gcval);
   else
     focusshadow = fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

   gcval.foreground = pfocushilite;
   if (focushilite)
     XChangeGC(dpy,focushilite,gcmask,&gcval);
   else
     focushilite = fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

   /* "normal" continued */
   gcmask = GCForeground | GCBackground | GCGraphicsExposures;
   gcval.foreground = pshadow;
   if (shadow)
     XChangeGC(dpy,shadow,gcmask,&gcval);
   else
     shadow = fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

   gcval.foreground = philite;
   if (hilite)
     XChangeGC(dpy,hilite,gcmask,&gcval);
   else
     hilite = fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

   gcval.foreground = WhitePixel(dpy, screen);;
   if (whitegc)
     XChangeGC(dpy,whitegc,gcmask,&gcval);
   else
     whitegc = fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

   gcval.foreground = BlackPixel(dpy, screen);
   if (blackgc)
     XChangeGC(dpy,blackgc,gcmask,&gcval);
   else
     blackgc = fvwmlib_XCreateGC(dpy,win,gcmask,&gcval);

   /* brighting */
   gcmask = GCForeground | GCBackground | GCTile |
            GCFillStyle  | GCGraphicsExposures;
   if (focuscolorset >= 0 || FocusBackColor != NULL)
     gcval.foreground = pfocusback;
   else
     gcval.foreground = philite;
   gcval.fill_style = FillTiled;
   gcval.tile       = XCreatePixmapFromBitmapData(dpy, win, (char *)gray_bits,
						  gray_width, gray_height,
						  gcval.foreground,
						  gcval.background,Pdepth);
   if (checkered)
     XChangeGC(dpy, checkered, gcmask, &gcval);
   else
     checkered = fvwmlib_XCreateGC(dpy, win, gcmask, &gcval);
}

static Bool change_colorset(int cset, Bool force)
{
  Bool do_redraw = False;

  if (cset < 0)
    return False;
  if (force || cset == colorset || cset == iconcolorset ||
      cset == focuscolorset)
  {
    CreateOrUpdateGCs();
    if (force || cset == colorset)
    {
      SetWindowBackground(
	dpy, win, win_width, win_height, &Colorset[colorset],
	Pdepth,	graph, True);
    }
    do_redraw = True;
  }
  do_redraw |= change_goody_colorset(cset, force);

  return do_redraw;
}


/******************************************************************************
  StartMeUp - Do X initialization things
******************************************************************************/
void StartMeUp(void)
{
   XSizeHints hints;
   XClassHint classhints;
   int ret;
   int i;
   XSetWindowAttributes attr;
#ifdef I18N_MB
  char **ml;
  int mc;
  char *ds;
  XFontStruct **fs_list;
#endif

   if (!(dpy = XOpenDisplay(""))) {
     fprintf(stderr,"%s: can't open display %s", Module,
	     XDisplayName(""));
     exit (1);
   }
   InitPictureCMap(dpy);
   XineramaSupportInit(dpy);
   if (XineramaConfig)
   {
     XineramaSupportConfigureModule(XineramaConfig);
     free(XineramaConfig);
   }
   AllocColorset(0);
   x_fd = XConnectionNumber(dpy);
   screen= DefaultScreen(dpy);
   Root = RootWindow(dpy, screen);

   if (geometry == NULL)
     UpdateString(&geometry, "+0-0");
   /* evaluate further down */
   ret = XineramaSupportParseGeometryWithScreen(
     geometry, &hints.x, &hints.y, (unsigned int *)&hints.width,
     (unsigned int *)&hints.height, &xi_screen);
   XineramaSupportGetNumberedScreenRect(
     xi_screen, &screen_g.x, &screen_g.y, &screen_g.width, &screen_g.height);
   XineramaSupportGetGlobalScrRect(
     &global_scr_g.x, &global_scr_g.y, &global_scr_g.width,
     &global_scr_g.height);
   Midline = (screen_g.height >> 1) + screen_g.y;

   if (selfont_string == NULL)
     selfont_string = font_string;

#ifdef I18N_MB
   if ((ButtonFontset=XCreateFontSet(dpy,font_string,&ml,&mc,&ds)) == NULL) {
#ifdef STRICTLY_FIXED
     if ((ButtonFontset=XCreateFontSet(dpy,"fixed",&ml,&mc,&ds)) == NULL) {
#else
     if ((ButtonFontset=XCreateFontSet(dpy,"-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*",&ml,&mc,&ds)) == NULL) {
#endif
       fprintf(stderr, "%s: Couldn't load fixed font. Exiting!\n",Module);
       exit(1);
     }
   }
   XFontsOfFontSet(ButtonFontset,&fs_list,&ml);
   ButtonFont = fs_list[0];
   if ((SelButtonFontset = XCreateFontSet(dpy,selfont_string,&ml,&mc,&ds))
       == NULL) {
#ifdef STRICTLY_FIXED
     if ((SelButtonFontset=XCreateFontSet(dpy,"fixed",&ml,&mc,&ds)) == NULL)
#else
     if ((SelButtonFontset=XCreateFontSet(dpy,"-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*",&ml,&mc,&ds)) == NULL)
#endif
       fprintf(stderr, "%s: Couldn't load fixed font. Exiting!\n",Module);
       exit(1);
   }
   XFontsOfFontSet(SelButtonFontset,&fs_list,&ml);
   SelButtonFont = fs_list[0];
#else
   if ((ButtonFont = XLoadQueryFont(dpy, font_string)) == NULL) {
     if ((ButtonFont = XLoadQueryFont(dpy, "fixed")) == NULL) {
       fprintf(stderr, "%s: Couldn't load fixed font. Exiting!\n", Module);
       exit(1);
     }
   }
   if ((SelButtonFont = XLoadQueryFont(dpy, selfont_string)) == NULL) {
     if ((SelButtonFont = XLoadQueryFont(dpy, "fixed")) == NULL) {
       fprintf(stderr, "%s: Couldn't load fixed font. Exiting!\n", Module);
       exit(1);
     }
   }
#endif

   fontheight = SelButtonFont->ascent + SelButtonFont->descent;

   NRows = 1;
   RowHeight = fontheight + 8;

   win_border = 4; /* default border width */
   win_height = RowHeight+(RowsNumber-1)*(RowHeight+2);
   win_width = screen_g.width - (win_border * 2);

   /* now evaluate the geometry parsed above */
   if (ret & YNegative)
   {
     if (AutoStick)
     {
       if (-hints.y < Midline)
	 hints.y = screen_g.height - (AutoHide ? VISIBLE_PIXELS() : win_height);
       else
	 hints.y = AutoHide ? VISIBLE_PIXELS() - win_height : 0;
       hints.y += screen_g.y;
     }
     else
     {
       hints.y += screen_g.height - win_height;
     }
   }
   else if (AutoStick)
   {
     if (hints.y < Midline)
       hints.y = AutoHide ? VISIBLE_PIXELS() - win_height : 0;
     else
       hints.y = screen_g.height - (AutoHide ? VISIBLE_PIXELS() : win_height);
     hints.y += screen_g.y;
   }

   if (ret & XNegative)
   {
     if (ret & YNegative)
       hints.win_gravity=SouthEastGravity;
     else
       hints.win_gravity=NorthEastGravity;
   }
   else
   {
     if (ret & YNegative)
       hints.win_gravity=SouthWestGravity;
     else
       hints.win_gravity=NorthWestGravity;
   }

   hints.flags=USPosition|PPosition|USSize|PSize|PResizeInc|
     PWinGravity|PMinSize|PMaxSize|PBaseSize;
   hints.x           = screen_g.x;
   hints.width       = win_width;
   hints.height      = win_height;
   hints.width_inc   = 1;
   hints.height_inc  = RowHeight+2;
   hints.min_width   = win_width;
   hints.min_height  = RowHeight;
   hints.max_width   = win_width;
   hints.max_height  = RowHeight+7*(RowHeight+2);
   hints.base_width  = win_width;
   hints.base_height = RowHeight;

   win_x = hints.x + win_border;
   win_y = hints.y;

   if(Pdepth < 2) {
     back = WhitePixel(dpy, screen);
     fore = BlackPixel(dpy, screen);
     iconback = WhitePixel(dpy, screen);
     iconfore = BlackPixel(dpy, screen);
     focusback = WhitePixel(dpy, screen);
     focusfore = BlackPixel(dpy, screen);
   } else {
     back = GetColor(BackColor);
     fore = GetColor(ForeColor);
     iconback = GetColor(IconBackColor);
     iconfore = GetColor(IconForeColor);
     if (FocusBackColor != NULL)
       focusback = GetColor(FocusBackColor);
     else
       focusback = back;
     if (FocusForeColor != NULL)
       focusfore = GetColor(FocusForeColor);
     else
       focusfore = fore;
   }

   attr.background_pixel =
     (colorset >= 0) ? Colorset[colorset].bg : back;
   attr.border_pixel = 0;
   attr.colormap = Pcmap;
   win=XCreateWindow(dpy,Root,hints.x,hints.y,hints.width,hints.height,0,Pdepth,
		     InputOutput,Pvisual,CWBackPixel|CWBorderPixel|CWColormap,
		     &attr);

   wm_del_win=XInternAtom(dpy,"WM_DELETE_WINDOW",False);
   XSetWMProtocols(dpy,win,&wm_del_win,1);

  {
    XTextProperty nametext;
    char *list[]={NULL,NULL};
    list[0] = Module+1;

    classhints.res_name=safestrdup(Module+1);
    classhints.res_class=safestrdup("FvwmTaskBar");

    if(!XStringListToTextProperty(list,1,&nametext))
    {
      fprintf(stderr,"%s: Failed to convert name to XText\n",Module);
      exit(1);
    }
    XSetWMProperties(dpy,win,&nametext,&nametext,
		     NULL,0,&hints,NULL,&classhints);
    XFree(nametext.value);
  }

   for (i = 1; i <= NUMBER_OF_MOUSE_BUTTONS; i++)
   {
     XGrabButton(dpy,i,AnyModifier,win,True,GRAB_EVENTS,GrabModeAsync,
		 GrabModeAsync,None,None);
   }

   /*   SetMwmHints(MWM_DECOR_ALL|MWM_DECOR_MAXIMIZE|MWM_DECOR_MINIMIZE,
	       MWM_FUNC_ALL|MWM_FUNC_MAXIMIZE|MWM_FUNC_MINIMIZE,
	       MWM_INPUT_MODELESS);
	       */

   CreateOrUpdateGCs();
   if (colorset >= 0)
   {
     SetWindowBackground(
       dpy, win, win_width, win_height, &Colorset[colorset],
       Pdepth, graph, False);
   }

   XSelectInput(dpy,win,(ExposureMask | KeyPressMask | PointerMotionMask |
                         EnterWindowMask | LeaveWindowMask |
                         StructureNotifyMask));

   InitGoodies();
   atexit(ShutMeDown);
}

/******************************************************************************
  ShutMeDown - Do X client cleanup
******************************************************************************/
static void
ShutMeDown(void)
{
  if (!is_dead_pipe)
  {
    FreeList(&windows);
    FreeAllButtons(&buttons);
    XFreeGC(dpy,graph);
    XFreeGC(dpy,icongraph);
    XFreeGC(dpy,focusgraph);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
  }
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
      prop.flags =
	MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS | MWM_HINTS_INPUT_MODE;

      /* HOP - LA! */
      XChangeProperty (dpy,win,
		       MwmAtom, MwmAtom,
		       32, PropModeReplace,
		       (unsigned char *)&prop,
		       PROP_MWM_HINTS_ELEMENTS);
    }
}

/***********************************************************************
 WarpTaskBar -- Enforce AutoStick feature
 ***********************************************************************/
void WarpTaskBar(int y, Bool force)
{
  /* The tests on y are really useful ! */
  if (!AutoHide &&
      ((y != screen_g.y + screen_g.height - win_height - win_border &&
	y !=  screen_g.y + win_border) || force)) {
    if (y > Midline)
    {
      win_y = screen_g.height - win_height - win_border;
    }
    else
    {
      win_y = win_border + win_title_height;
    }
    win_y += screen_g.y;
    XSync(dpy, 0);
    XMoveWindow(dpy, win, win_x, win_y);
    XSync(dpy, 0);
  }

  if (AutoHide)
     SetAlarm(HIDE_TASK_BAR);

  /* Prevent oscillations caused by race with
     time delayed TaskBarHide().  Is there any way
     to prevent these Xevents from being sent
     to the server in the first place? */
  PurgeConfigEvents();
}

/***********************************************************************
 SleepALittle -- do a small delay in a fairly portable way
 Replace "#if 1" with "#if 0" to turn delay off
 ***********************************************************************/
static void SleepALittle(void)
{
  struct timeval tv;

    tv.tv_sec  = 0;
    tv.tv_usec = 10000;
#if 1
    XSync(dpy, 0);
    select(0, NULL, NULL, NULL, &tv);
    XSync(dpy, 0);
#endif
}

/***********************************************************************
 RevealTaskBar -- Make taskbar fully visible
 ***********************************************************************/
void RevealTaskBar()
{
  int new_win_y;
  int inc_y = 2;

  ClearAlarm();
   /* do not reveal if the taskbar is already revealed */
  if (WindowState >= 0)
    return;

  /* Go faster with the number of rows */
  inc_y += (NRows >= 3 ? 3 : 0) + (NRows >= 5 ? 3 : 0);

  if (win_y < Midline)
  {
    new_win_y = screen_g.y + win_border + win_title_height;
    for (; win_y<=new_win_y; win_y +=inc_y)
    {
      XMoveWindow(dpy, win, win_x, win_y);
      SleepALittle();
    }
  }
  else
  {
    new_win_y = screen_g.y + screen_g.height - win_height - win_border;
    for (; win_y>=new_win_y; win_y -=inc_y)
    {
      XMoveWindow(dpy, win, win_x, win_y);
      SleepALittle();
    }
  }

  win_y = new_win_y;
  XMoveWindow(dpy, win, win_x, win_y);
  WindowState = 0;
}

/***********************************************************************
 HideTaskbar -- Make taskbar partially visible
 ***********************************************************************/
void HideTaskBar()
{
  int new_win_y;
  int inc_y = 1;
  Window d_rt, d_ch;
  int d_x, d_y, wx, wy;
  unsigned int mask;

  ClearAlarm();
  /* do not hide if the taskbar is already hiden */
  if (WindowState == -1 || win_is_shaded)
    return;

  if (FocusInWin)
  {
    XQueryPointer(dpy, win, &d_rt,&d_ch, &d_x, &d_y,
		  &wx, &wy, &mask);
    if (wy >= -(win_border + win_title_height*(1-win_has_bottom_title)) &&
	wy < win_height + win_border +
	win_title_height*win_has_bottom_title)
    {
      if (wy < 0 || wy >= win_height || wx < 0 || wx >= win_width)
	SetAlarm(HIDE_TASK_BAR);
      return;
    }
  }

  /* Go faster with the number of rows */
  inc_y += (NRows >= 3 ? 2 : 0) + (NRows >= 5 ? 2 : 0);

  if (win_y < Midline)
  {
    new_win_y = screen_g.y + VISIBLE_PIXELS() +
      win_title_height*win_has_bottom_title - win_height;
    for (; win_y>=new_win_y; win_y -=inc_y)
    {
      XMoveWindow(dpy, win, win_x, win_y);
      SleepALittle();
    }
  }
  else
  {
    new_win_y = screen_g.y + screen_g.height - VISIBLE_PIXELS() +
      win_title_height*win_has_bottom_title;
    for (; win_y<=new_win_y; win_y +=inc_y)
    {
      XMoveWindow(dpy, win, win_x, win_y);
      SleepALittle();
    }
  }
  win_y = new_win_y;
  XMoveWindow(dpy, win, win_x, win_y);
  WindowState = -1;
}

/***********************************************************************
 SetAlarm -- Schedule a timeout event
 ************************************************************************/
static void
SetAlarm(int event)
{
  alarm(0);  /* remove a race-condition */
  AlarmSet = event;
  alarm(1);
}

/***********************************************************************
 ClearAlarm -- Disable timeout events
 ************************************************************************/
static void
ClearAlarm(void)
{
  AlarmSet = NOT_SET;
  alarm(0);
}

/***********************************************************************
 PurgeConfigEvents -- Wait for and purge ConfigureNotify events.
 ************************************************************************/
void PurgeConfigEvents(void)
{
  XEvent Event;

  XPeekEvent(dpy, &Event);
  while (XCheckTypedWindowEvent(dpy, win, ConfigureNotify, &Event))
    ;
}

/************************************************************************
  X Error Handler
************************************************************************/
static int
ErrorHandler(Display *d, XErrorEvent *event)
{
  /* some errors are OK=ish */
  if (event->error_code == BadPixmap)
    return 0;
  if (event->error_code == BadDrawable)
    return 0;

  PrintXErrorAndCoredump(d, event, Module);
  return 0;
}
