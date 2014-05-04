/* -*-c-*- */
/* This module, and the entire ModuleDebugger program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "libs/ftime.h"
#include <ctype.h>

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif /* Saul */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/keysym.h>

#include "libs/Module.h"
#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/FRenderInit.h"
#include "libs/Colorset.h"
#include "libs/Flocale.h"
#include "libs/fvwmsignal.h"
#include "libs/Grab.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/System.h"

#include "fvwm/fvwm.h"
#include "FvwmPager.h"


char *MyName;
int fd[2];

PagerStringList *FindDeskStrings(int desk);
PagerStringList *NewPagerStringItem(PagerStringList *last, int desk);
extern FlocaleFont *FwindowFont;
extern Pixmap default_pixmap;

/*
 *
 * Screen, font, etc info
 *
 */
ScreenInfo Scr;
PagerWindow *Start = NULL;
PagerWindow *FocusWin = NULL;

Display *dpy;                   /* which display are we talking to */
int x_fd;
fd_set_size_t fd_width;

char *PagerFore = NULL;
char *PagerBack = NULL;
char *font_string = NULL;
char *smallFont = NULL;
char *HilightC = NULL;

FvwmPicture *HilightPixmap = NULL;
int HilightDesks = 1;

char *WindowBack = NULL;
char *WindowFore = NULL;
char *WindowHiBack = NULL;
char *WindowHiFore = NULL;
char *WindowLabelFormat = NULL;

unsigned int WindowBorderWidth = DEFAULT_PAGER_WINDOW_BORDER_WIDTH;
unsigned int MinSize = 2 * DEFAULT_PAGER_WINDOW_BORDER_WIDTH + 1;
Bool WindowBorders3d = False;

Bool UseSkipList = False;

FvwmPicture *PixmapBack = NULL;

char *ImagePath = NULL;

#define DEFAULT_PAGER_MOVE_THRESHOLD 3
int MoveThreshold = DEFAULT_PAGER_MOVE_THRESHOLD;

int ShowBalloons = 0, ShowPagerBalloons = 0, ShowIconBalloons = 0;
Bool do_focus_on_enter = False;
Bool use_dashed_separators = True;
Bool use_no_separators = False;
char *BalloonTypeString = NULL;
char *BalloonBack = NULL;
char *BalloonFore = NULL;
char *BalloonFont = NULL;
char *BalloonBorderColor = NULL;
char *BalloonFormatString = NULL;
int BalloonBorderWidth = 1;
int BalloonYOffset = 3;
Window BalloonView = None;

int window_w=0, window_h=0, window_x=0, window_y=0;
int icon_x=-10000, icon_y=-10000, icon_w=0, icon_h=0;
int usposition = 0,uselabel = 1;
int xneg = 0, yneg = 0;
int icon_xneg = 0, icon_yneg = 0;
extern DeskInfo *Desks;
int StartIconic = 0;
int MiniIcons = 0;
int LabelsBelow = 0;
int ShapeLabels = 0;
int Rows = -1, Columns = -1;
int desk1=0, desk2 =0;
int ndesks = 0;
int windowcolorset = -1;
int activecolorset = -1;
static int globalcolorset = -1;
static int globalballooncolorset = -1;
static int globalhighcolorset = -1;
Pixel win_back_pix;
Pixel win_fore_pix;
Bool win_pix_set = False;
Pixel win_hi_back_pix;
Pixel win_hi_fore_pix;
Bool win_hi_pix_set = False;
char fAlwaysCurrentDesk = 0;
PagerStringList string_list = { NULL, 0, -1, -1, -1, NULL, NULL, NULL };
Bool is_transient = False;
Bool do_ignore_next_button_release = False;
Bool error_occured = False;
Bool Swallowed = False;

static void SetDeskLabel(int desk, const char *label);
static RETSIGTYPE TerminateHandler(int);
void ExitPager(void);

/*
 *
 *  Procedure:
 *      main - start of module
 *
 */
int main(int argc, char **argv)
{
  char *display_name = NULL;
  int itemp,i;
  char line[100];
  short opt_num;
  Window JunkRoot, JunkChild;
  int JunkX, JunkY;
  unsigned JunkMask;

  FlocaleInit(LC_CTYPE, "", "", "FvwmPager");

  /* Tell the FEvent module an event type that is not used by fvwm. */
  fev_init_invalid_event_type(KeymapNotify);

  /* Save our program  name - for error messages */
  MyName = GetFileNameFromPath(argv[0]);

  if(argc  < 6)
    {
      fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MyName,
	      VERSION);
      exit(1);
    }

#ifdef HAVE_SIGACTION
  {
    struct sigaction  sigact;

    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGPIPE);
    sigaddset(&sigact.sa_mask, SIGTERM);
    sigaddset(&sigact.sa_mask, SIGQUIT);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGHUP);
# ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT;
# else
    sigact.sa_flags = 0;
# endif
    sigact.sa_handler = TerminateHandler;

    sigaction(SIGPIPE, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGINT,  &sigact, NULL);
    sigaction(SIGHUP,  &sigact, NULL);
  }
#else
  /* We don't have sigaction(), so fall back to less robust methods.  */
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGPIPE) |
		     sigmask(SIGTERM) |
		     sigmask(SIGQUIT) |
		     sigmask(SIGINT) |
		     sigmask(SIGHUP) );
#endif
  signal(SIGPIPE, TerminateHandler);
  signal(SIGTERM, TerminateHandler);
  signal(SIGQUIT, TerminateHandler);
  signal(SIGINT,  TerminateHandler);
  signal(SIGHUP,  TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGTERM, 1);
  siginterrupt(SIGQUIT, 1);
  siginterrupt(SIGINT, 1);
  siginterrupt(SIGHUP, 1);
#endif
#endif

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  fd_width = GetFdWidth();

  opt_num = 6;
  if (argc >= 7 && (StrEquals(argv[opt_num], "-transient") ||
		    StrEquals(argv[opt_num], "transient")))
  {
    opt_num++;
    is_transient = True;
      do_ignore_next_button_release = True;
  }

  /* Check for an alias */
  if (argc >= opt_num + 1)
    {
      char *s;

      if (!StrEquals(argv[opt_num], "*"))
      {
	for (s = argv[opt_num]; *s; s++)
	{
	  if (!isdigit(*s) &&
	      (*s != '-' || s != argv[opt_num] || *(s+1) == 0))
	  {
	    free(MyName);
	    MyName = xstrdup(argv[opt_num]);
	    opt_num++;
	    break;
	  }
	}
      }
    }

  if (argc < opt_num + 1)
    {
      desk1 = Scr.CurrentDesk;
      desk2 = Scr.CurrentDesk;
    }
  else if (StrEquals(argv[opt_num], "*"))
    {
      desk1 = Scr.CurrentDesk;
      desk2 = Scr.CurrentDesk;
      fAlwaysCurrentDesk = 1;
    }
  else
    {
      desk1 = atoi(argv[opt_num]);
      if (argc == opt_num+1)
	desk2 = desk1;
      else
	desk2 = atoi(argv[opt_num+1]);
      if(desk2 < desk1)
	{
	  itemp = desk1;
	  desk1 = desk2;
	  desk2 = itemp;
	}
    }
  ndesks = desk2 - desk1 + 1;

  Desks = xcalloc(1, ndesks*sizeof(DeskInfo));
  for(i=0;i<ndesks;i++)
    {
      sprintf(line,"Desk %d",i+desk1);
      CopyString(&Desks[i].label,line);
      Desks[i].colorset = -1;
      Desks[i].highcolorset = -1;
      Desks[i].ballooncolorset = -1;
    }

  /* Initialize X connection */
  if (!(dpy = XOpenDisplay(display_name)))
    {
      fprintf(stderr,"%s: can't open display %s", MyName,
	      XDisplayName(display_name));
      exit (1);
    }

  x_fd = XConnectionNumber(dpy);
  flib_init_graphics(dpy);

  Scr.screen = DefaultScreen(dpy);
  Scr.Root = RootWindow(dpy, Scr.screen);
  /* make a temp window for any pixmaps, deleted later */
  initialize_viz_pager();

  SetMessageMask(fd,
		 M_VISIBLE_NAME |
		 M_ADD_WINDOW|
		 M_CONFIGURE_WINDOW|
		 M_DESTROY_WINDOW|
		 M_FOCUS_CHANGE|
		 M_NEW_PAGE|
		 M_NEW_DESK|
		 M_RAISE_WINDOW|
		 M_LOWER_WINDOW|
		 M_ICONIFY|
		 M_ICON_LOCATION|
		 M_DEICONIFY|
		 M_RES_NAME|
		 M_RES_CLASS|
		 M_CONFIG_INFO|
		 M_END_CONFIG_INFO|
		 M_MINI_ICON|
		 M_END_WINDOWLIST|
		 M_RESTACK);
  SetMessageMask(fd,
		 MX_VISIBLE_ICON_NAME|
		 MX_PROPERTY_CHANGE|
		 MX_REPLY);
  ParseOptions();
  if (is_transient)
  {
    if (FQueryPointer(
	  dpy, Scr.Root, &JunkRoot, &JunkChild, &window_x, &window_y, &JunkX,
	  &JunkY, &JunkMask) == False)
    {
      /* pointer is on a different screen */
      window_x = 0;
      window_y = 0;
    }
    usposition = 1;
    xneg = 0;
    yneg = 0;
  }

  if (PagerFore == NULL)
    PagerFore = xstrdup("black");

  if (PagerBack == NULL)
    PagerBack = xstrdup("white");

  if (HilightC == NULL)
    HilightC = xstrdup(PagerFore);

  if (WindowLabelFormat == NULL)
    WindowLabelFormat = xstrdup("%i");

  if ((HilightC == NULL) && (HilightPixmap == NULL))
    HilightDesks = 0;

  if (BalloonBorderColor == NULL)
    BalloonBorderColor = xstrdup("black");

  if (BalloonTypeString == NULL)
    BalloonTypeString = xstrdup("%i");

  if (BalloonFormatString == NULL)
    BalloonFormatString = xstrdup("%i");

  /* open a pager window */
  initialize_pager();

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo(fd,"Send_WindowList",0);

  if (is_transient)
  {
    Bool is_pointer_grabbed = False;
    Bool is_keyboard_grabbed = False;
    XSync(dpy,0);
    for (i = 0; i < 50 && !(is_pointer_grabbed && is_keyboard_grabbed); i++)
    {
      if (!is_pointer_grabbed &&
	  XGrabPointer(
	    dpy, Scr.Root, True,
	    ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
	    PointerMotionMask|EnterWindowMask|LeaveWindowMask, GrabModeAsync,
	    GrabModeAsync, None, None, CurrentTime) == GrabSuccess)
      {
	is_pointer_grabbed = True;
      }
      if (!is_keyboard_grabbed &&
	  XGrabKeyboard(
	    dpy, Scr.Root, True, GrabModeAsync, GrabModeAsync, CurrentTime) ==
	  GrabSuccess)
      {
	is_keyboard_grabbed = True;
      }
      /* If you go too fast, other windows may not get a change to release
       * any grab that they have. */
      usleep(20000);
    }
    if (!is_pointer_grabbed)
    {
      XBell(dpy, 0);
      fprintf(stderr,
	      "%s: could not grab pointer in transient mode. exiting.\n",
	      MyName);
      exit(1);
    }

    XSync(dpy,0);
  }

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);

  Loop(fd);
#ifdef FVWM_DEBUG_MSGS
  if ( isTerminated )
  {
    fprintf(stderr, "%s: Received signal: exiting...\n", MyName);
  }
#endif

  return 0;
}

/*
 *
 *  Procedure:
 *      Loop - wait for data to process
 *
 */
void Loop(int *fd)
{
  XEvent Event;

  while( !isTerminated )
  {
    if(My_XNextEvent(dpy,&Event))
      DispatchEvent(&Event);
    if (error_occured)
    {
      Window root;
      unsigned border_width, depth;
      int x,y;

      if(XGetGeometry(dpy,Scr.Pager_w,&root,&x,&y,
		      (unsigned *)&window_w,(unsigned *)&window_h,
		      &border_width,&depth)==0)
      {
	/* does not return */
	ExitPager();
      }
      error_occured = False;
    }
  }
}


/*
 *
 *  Procedure:
 *      Process message - examines packet types, and takes appropriate action
 *
 */
void process_message( FvwmPacket* packet )
{
  unsigned long type = packet->type;
  unsigned long length = packet->size;
  unsigned long* body = packet->body;

  switch (type)
    {
    case M_ADD_WINDOW:
      list_configure(body);
      break;
    case M_CONFIGURE_WINDOW:
      list_configure(body);
      break;
    case M_DESTROY_WINDOW:
      list_destroy(body);
      break;
    case M_FOCUS_CHANGE:
      list_focus(body);
      break;
    case M_NEW_PAGE:
      list_new_page(body);
      break;
    case M_NEW_DESK:
      list_new_desk(body);
      break;
    case M_RAISE_WINDOW:
      list_raise(body);
      break;
    case M_LOWER_WINDOW:
      list_lower(body);
      break;
    case M_ICONIFY:
    case M_ICON_LOCATION:
      list_iconify(body);
      break;
    case M_DEICONIFY:
      list_deiconify(body, length);
      break;
    case M_RES_CLASS:
    case M_RES_NAME:
    case M_VISIBLE_NAME:
      list_window_name(body,type);
      break;
    case MX_VISIBLE_ICON_NAME:
      list_icon_name(body);
      break;
    case M_MINI_ICON:
      list_mini_icon(body);
      break;
    case M_END_WINDOWLIST:
      list_end();
      break;
    case M_RESTACK:
      list_restack(body,length);
      break;
    case M_CONFIG_INFO:
      list_config_info(body);
      break;
    case MX_PROPERTY_CHANGE:
      list_property_change(body);
      break;
    case MX_REPLY:
	    list_reply(body);
	    break;
    default:
      /* ignore unknown packet */
      break;
    }
}


/*
 *
 *  Procedure:
 *      SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 */
static RETSIGTYPE
TerminateHandler(int sig)
{
  fvwmSetTerminate(sig);
  SIGNAL_RETURN;
}

RETSIGTYPE DeadPipe(int nonsense)
{
  exit(0);
  SIGNAL_RETURN;
}


/*
 *  Procedure:
 *	handle_config_win_package - updates a PagerWindow
 *		with respect to a ConfigWinPacket
 */
void handle_config_win_package(PagerWindow *t,
			       ConfigWinPacket *cfgpacket)
{
	if (t->w != None && t->w != cfgpacket->w)
	{
		/* Should never happen */
		fprintf(stderr,"%s: Error: Internal window list corrupt\n",MyName);
		/* might be a good idea to exit here */
		return;
	}
	t->w = cfgpacket->w;
	t->frame = cfgpacket->frame;
	t->frame_x = cfgpacket->frame_x;
	t->frame_y = cfgpacket->frame_y;
	t->frame_width = cfgpacket->frame_width;
	t->frame_height = cfgpacket->frame_height;

	t->desk = cfgpacket->desk;

	t->title_height = cfgpacket->title_height;
	t->border_width = cfgpacket->border_width;

	t->icon_w = cfgpacket->icon_w;
	t->icon_pixmap_w = cfgpacket->icon_pixmap_w;

	memcpy(&(t->flags), &(cfgpacket->flags), sizeof(cfgpacket->flags));
	memcpy(&(t->allowed_actions), &(cfgpacket->allowed_actions),
	       sizeof(cfgpacket->allowed_actions));

	if (win_pix_set)
	{
		t->text = win_fore_pix;
		t->back = win_back_pix;
	}
	else
	{
		t->text = cfgpacket->TextPixel;
		t->back = cfgpacket->BackPixel;
	}

	if (IS_ICONIFIED(t))
	{
		/* For new windows icon_x and icon_y will be zero until
		 * iconify message is recived
		 */
		t->x = t->icon_x;
		t->y = t->icon_y;
		t->width = t->icon_width;
		t->height = t->icon_height;
		if(IS_ICON_SUPPRESSED(t) || t->width == 0 || t->height == 0)
		{
			t->x = -32768;
			t->y = -32768;
		}
	}
	else
	{
		t->x = t->frame_x;
		t->y = t->frame_y;
		t->width = t->frame_width;
		t->height = t->frame_height;
	}
}

/*
 *
 *  Procedure:
 *      list_add - displays packet contents to stderr
 *
 */
void list_add(unsigned long *body)
{
	PagerWindow *t,**prev;
	int i=0;
	struct ConfigWinPacket  *cfgpacket = (void *) body;

	t = Start;
	prev = &Start;

	while(t!= NULL)
	{
		if (t->w == cfgpacket->w)
		{
			/* it's already there, do nothing */
			return;
		}
		prev = &(t->next);
		t = t->next;
		i++;
	}
	*prev = xcalloc(1, sizeof(PagerWindow));
	handle_config_win_package(*prev, cfgpacket);
	AddNewWindow(*prev);

	return;
}

/*
 *
 *  Procedure:
 *      list_configure - displays packet contents to stderr
 *
 */
void list_configure(unsigned long *body)
{
  PagerWindow *t;
  Window target_w;
  struct ConfigWinPacket  *cfgpacket = (void *) body;
  Bool is_new_desk;

  target_w = cfgpacket->w;
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
  {
    t = t->next;
  }
  if(t== NULL)
  {
    list_add(body);
    return;
  }

  is_new_desk = (t->desk != cfgpacket->desk);
  handle_config_win_package(t, cfgpacket);

  if (is_new_desk)
  {
    ChangeDeskForWindow(t, cfgpacket->desk);
  }
  else
  {
    MoveResizePagerView(t, False);
  }
}

/*
 *
 *  Procedure:
 *      list_destroy - displays packet contents to stderr
 *
 */
void list_destroy(unsigned long *body)
{
  PagerWindow *t,**prev;
  Window target_w;

  target_w = body[0];
  t = Start;
  prev = &Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      prev = &(t->next);
      t = t->next;
    }
  if(t!= NULL)
    {
      if(prev != NULL)
	*prev = t->next;
      /* remove window from the chain */
      if(t->PagerView != None)
	XDestroyWindow(dpy,t->PagerView);
      XDestroyWindow(dpy,t->IconView);
      if(FocusWin == t)
	FocusWin = NULL;
      if(t->res_class != NULL)
	free(t->res_class);
      if(t->res_name != NULL)
	free(t->res_name);
      if(t->window_name != NULL)
	free(t->window_name);
      if(t->icon_name != NULL)
	free(t->icon_name);
      free(t);
    }
}

/*
 *
 *  Procedure:
 *      list_focus - displays packet contents to stderr
 *
 */
void list_focus(unsigned long *body)
{
  PagerWindow *t;
  Window target_w;
  extern Pixel focus_pix, focus_fore_pix;
  Bool do_force_update = False;

  target_w = body[0];
  if (win_hi_pix_set)
  {
    if (focus_pix != win_hi_back_pix || focus_fore_pix != win_hi_fore_pix)
    {
      do_force_update = True;
    }
    focus_pix = win_hi_back_pix;
    focus_fore_pix = win_hi_fore_pix;
  }
  else
  {
    if (focus_pix != body[4] || focus_fore_pix != body[3])
    {
      do_force_update = True;
    }
    focus_pix = body[4];
    focus_fore_pix = body[3];
  }
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
  {
    t = t->next;
  }
  if (t != FocusWin || do_force_update)
  {
    if (FocusWin != NULL)
      Hilight(FocusWin, False);
    FocusWin = t;
    if (FocusWin != NULL)
      Hilight(FocusWin, True);
  }
}

/*
 *
 *  Procedure:
 *      list_new_page - displays packet contents to stderr
 *
 */
void list_new_page(unsigned long *body)
{
  Scr.Vx = body[0];
  Scr.Vy = body[1];
  if (Scr.CurrentDesk != body[2])
  {
      /* first handle the new desk */
      body[0] = body[2];
      list_new_desk(body);
  }
  if (Scr.VxPages != body[5] || Scr.VyPages != body[6])
  {
    Scr.VxPages = body[5];
    Scr.VyPages = body[6];
    Scr.VWidth = Scr.VxPages * Scr.MyDisplayWidth;
    Scr.VHeight = Scr.VyPages * Scr.MyDisplayHeight;
    Scr.VxMax = Scr.VWidth - Scr.MyDisplayWidth;
    Scr.VyMax = Scr.VHeight - Scr.MyDisplayHeight;
    ReConfigure();
  }
  MovePage(False);
  MoveStickyWindow(True, False);
  Hilight(FocusWin,True);
}

/*
 *
 *  Procedure:
 *      list_new_desk - displays packet contents to stderr
 *
 */
void list_new_desk(unsigned long *body)
{
  int oldDesk;
  int change_cs = -1;
  int change_ballooncs = -1;
  int change_highcs = -1;

  oldDesk = Scr.CurrentDesk;
  Scr.CurrentDesk = (long)body[0];
  if (fAlwaysCurrentDesk && oldDesk != Scr.CurrentDesk)
  {
    PagerWindow *t;
    PagerStringList *item;
    char line[100];

    desk1 = Scr.CurrentDesk;
    desk2 = Scr.CurrentDesk;
    for (t = Start; t != NULL; t = t->next)
    {
      if (t->desk == oldDesk || t->desk == Scr.CurrentDesk)
	ChangeDeskForWindow(t, t->desk);
    }
    item = FindDeskStrings(Scr.CurrentDesk);
    if (Desks[0].label != NULL)
    {
      free(Desks[0].label);
      Desks[0].label = NULL;
    }
    if (item->next != NULL && item->next->label != NULL)
    {
      CopyString(&Desks[0].label, item->next->label);
    }
    else
    {
      sprintf(line, "Desk %d", desk1);
      CopyString(&Desks[0].label, line);
    }
    XStoreName(dpy, Scr.Pager_w, Desks[0].label);
    XSetIconName(dpy, Scr.Pager_w, Desks[0].label);

    if (Desks[0].bgPixmap != NULL)
    {
      PDestroyFvwmPicture(dpy, Desks[0].bgPixmap);
      Desks[0].bgPixmap = NULL;
    }

    if (Desks[0].Dcolor != NULL)
    {
      free (Desks[0].Dcolor);
      Desks[0].Dcolor = NULL;
    }

    if (item->next != NULL)
    {
      change_cs = item->next->colorset;
      change_ballooncs = item->next->ballooncolorset;
      change_highcs = item->next->highcolorset;
    }
    if (change_cs < 0)
    {
      change_cs = globalcolorset;
    }
    Desks[0].colorset = change_cs;
    if (change_cs > -1)
    {
      /* use our colour set if we have one */
      change_colorset(change_cs);
    }
    else if (item->next != NULL && item->next->bgPixmap != NULL)
    {
      Desks[0].bgPixmap = item->next->bgPixmap;
      Desks[0].bgPixmap->count++;
      XSetWindowBackgroundPixmap(dpy, Desks[0].w,
				 Desks[0].bgPixmap->picture);
    }
    else if (item->next != NULL && item->next->Dcolor != NULL)
    {
      CopyString(&Desks[0].Dcolor, item->next->Dcolor);
      XSetWindowBackground(dpy, Desks[0].w, GetColor(Desks[0].Dcolor));
    }
    else if (PixmapBack != NULL)
    {
      Desks[0].bgPixmap = PixmapBack;
      Desks[0].bgPixmap->count++;
      XSetWindowBackgroundPixmap(dpy, Desks[0].w,
				 Desks[0].bgPixmap->picture);
    }
    else
    {
      CopyString(&Desks[0].Dcolor, PagerBack);
      XSetWindowBackground(dpy, Desks[0].w, GetColor(Desks[0].Dcolor));
    }

    if (item->next != NULL && item->next->Dcolor != NULL)
    {
      CopyString(&Desks[0].Dcolor, item->next->Dcolor);
    }
    else
    {
      CopyString(&Desks[0].Dcolor, PagerBack);
    }
    if (change_cs < 0 || Colorset[change_cs].pixmap != ParentRelative)
      XSetWindowBackground(dpy, Desks[0].title_w, GetColor(Desks[0].Dcolor));

    /* update the colour sets for the desk */
    if (change_ballooncs < 0)
    {
      change_ballooncs = globalballooncolorset;
    }
    Desks[0].ballooncolorset = change_ballooncs;
    if (change_highcs < 0)
    {
      change_highcs = globalhighcolorset;
    }
    Desks[0].highcolorset = change_highcs;
    if (change_ballooncs > -1 && change_ballooncs != change_cs)
    {
      change_colorset(change_ballooncs);
    }
    if (change_highcs > -1 && change_highcs != change_cs &&
	change_highcs != change_ballooncs)
    {
      change_colorset(change_highcs);
    }
    XClearWindow(dpy, Desks[0].w);
    XClearWindow(dpy, Desks[0].title_w);
  } /* if (fAlwaysCurrentDesk && oldDesk != Scr.CurrentDesk) */
  else if (!fAlwaysCurrentDesk)
  {
    int i;
    char *name;
    char line[100];

    i = Scr.CurrentDesk - desk1;
    if (i >= 0 && i < ndesks && Desks[i].label != NULL)
    {
      name = Desks[i].label;
    }
    else
    {
      sprintf(line, "Desk %d", Scr.CurrentDesk);
      name = &(line[0]);
    }
    XStoreName(dpy, Scr.Pager_w, name);
    XSetIconName(dpy, Scr.Pager_w, name);
  }

  MovePage(True);
  DrawGrid(oldDesk - desk1, 1, None, NULL);
  DrawGrid(Scr.CurrentDesk - desk1, 1, None, NULL);
  MoveStickyWindow(False, True);
/*
  Hilight(FocusWin,False);
*/
  Hilight(FocusWin,True);
}

/*
 *
 *  Procedure:
 *      list_raise - displays packet contents to stderr
 *
 */
void list_raise(unsigned long *body)
{
  PagerWindow *t;
  Window target_w;

  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t!= NULL)
    {
      if(t->PagerView != None)
	XRaiseWindow(dpy,t->PagerView);
      XRaiseWindow(dpy,t->IconView);
    }
}


/*
 *
 *  Procedure:
 *      list_lower - displays packet contents to stderr
 *
 */
void list_lower(unsigned long *body)
{
  PagerWindow *t;
  Window target_w;

  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t!= NULL)
    {
      if(t->PagerView != None)
	XLowerWindow(dpy,t->PagerView);
      if (HilightDesks && (t->desk - desk1>=0) && (t->desk - desk1<ndesks))
	XLowerWindow(dpy,Desks[t->desk - desk1].CPagerWin);
      XLowerWindow(dpy,t->IconView);
    }
}


/*
 *
 *  Procedure:
 *      list_iconify - displays packet contents to stderr
 *
 */
void list_iconify(unsigned long *body)
{
	PagerWindow *t;
	Window target_w;

	target_w = body[0];
	t = Start;
	while (t != NULL && t->w != target_w)
	{
		t = t->next;
	}
	if (t != NULL)
	{
		t->t = (char *)body[2];
		t->frame = body[1];
		t->icon_x = body[3];
		t->icon_y = body[4];
		t->icon_width = body[5];
		t->icon_height = body[6];
		SET_ICONIFIED(t, True);
		if (IS_ICON_SUPPRESSED(t) || t->icon_width == 0 ||
		    t->icon_height == 0)
		{
			t->x = -32768;
			t->y = -32768;
		}
		else
		{
			t->x = t->icon_x;
			t->y = t->icon_y;
		}
		t->width = t->icon_width;
		t->height = t->icon_height;
		/* if iconifying main pager window turn balloons on or off */
		if ( t->w == Scr.Pager_w )
		{
			ShowBalloons = ShowIconBalloons;
		}
		MoveResizePagerView(t, True);
	}

	return;
}


/*
 *
 *  Procedure:
 *      list_deiconify - displays packet contents to stderr
 *
 */

void list_deiconify(unsigned long *body, unsigned long length)
{
  PagerWindow *t;
  Window target_w;

  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
  {
    t = t->next;
  }
  if (t == NULL)
  {
    return;
  }
  else
  {
    SET_ICONIFIED(t, False);
    if (length >= 11 + FvwmPacketHeaderSize)
    {
      t->frame_x = body[7];
      t->frame_y = body[8];
      t->frame_width = body[9];
      t->frame_height = body[10];
    }
    t->x = t->frame_x;
    t->y = t->frame_y;
    t->width = t->frame_width;
    t->height = t->frame_height;

    /* if deiconifying main pager window turn balloons on or off */
    if ( t->w == Scr.Pager_w )
      ShowBalloons = ShowPagerBalloons;

    MoveResizePagerView(t, True);
  }

  return;
}


void list_window_name(unsigned long *body,unsigned long type)
{
  PagerWindow *t;
  Window target_w;

  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t!= NULL)
    {
      switch (type) {
      case M_RES_CLASS:
	if(t->res_class != NULL)
	  free(t->res_class);
	CopyString(&t->res_class,(char *)(&body[3]));
	break;
      case M_RES_NAME:
	if(t->res_name != NULL)
	  free(t->res_name);
	CopyString(&t->res_name,(char *)(&body[3]));
	break;
      case M_VISIBLE_NAME:
	if(t->window_name != NULL)
	  free(t->window_name);
	CopyString(&t->window_name,(char *)(&body[3]));
	break;
      }
      /* repaint by clearing window */
      if ((FwindowFont != NULL) && (t->icon_name != NULL)
	   && !(MiniIcons && t->mini_icon.picture)) {
	if (t->PagerView)
	  XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
	if (t->IconView)
	  XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
      }
      if (ShowBalloons && BalloonView)
      {
	/* update balloons */
	if (BalloonView == t->PagerView)
	{
	  UnmapBalloonWindow();
	  MapBalloonWindow(t, False);
	}
	else if (BalloonView == t->IconView)
	{
	  UnmapBalloonWindow();
	  MapBalloonWindow(t, True);
	}
      }
    }
}

/*
 *
 *  Procedure:
 *      list_icon_name - displays packet contents to stderr
 *
 */
void list_icon_name(unsigned long *body)
{
  PagerWindow *t;
  Window target_w;

  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t!= NULL)
    {
      if(t->icon_name != NULL)
	free(t->icon_name);
      CopyString(&t->icon_name,(char *)(&body[3]));
      /* repaint by clearing window */
      if ((FwindowFont != NULL) && (t->icon_name != NULL)
	   && !(MiniIcons && t->mini_icon.picture)) {
	if (t->PagerView)
	  XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
	if (t->IconView)
	  XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
      }
    }
}


void list_mini_icon(unsigned long *body)
{
  PagerWindow   *t;
  MiniIconPacket *mip = (MiniIconPacket *) body;

  t = Start;
  while (t && (t->w != mip->w))
    t = t->next;
  if (t)
  {
    t->mini_icon.width   = mip->width;
    t->mini_icon.height  = mip->height;
    t->mini_icon.depth   = mip->depth;
    t->mini_icon.picture = mip->picture;
    t->mini_icon.mask    = mip->mask;
    t->mini_icon.alpha   = mip->alpha;
    /* repaint by clearing window */
    if (MiniIcons && t->mini_icon.picture) {
      if (t->PagerView)
	XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
      if (t->IconView)
	XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
    }
  }
}

void list_restack(unsigned long *body, unsigned long length)
{
  PagerWindow *t;
  Window target_w;
  Window *wins;
  int i, j, d;

  wins = xmalloc(length * sizeof (Window));
  /* first restack in the icon view */
  j = 0;
  for (i = 0; i < (length - FvwmPacketHeaderSize); i += 3)
  {
    target_w = body[i];
    t = Start;
    while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
    if (t != NULL)
    {
      wins[j++] = t->IconView;
    }
  }
  XRestackWindows(dpy, wins, j);

  /* now restack each desk separately, since they have separate roots */
  for (d = 0; d < ndesks; d++)
  {
    j = 0;
    for (i = 0; i < (length - 4); i+=3)
    {
      target_w = body[i];
      t = Start;
      while((t!= NULL)&&((t->w != target_w)||(t->desk != d+desk1)))
      {
	t = t->next;
      }
      if (t != NULL)
      {
	if (t->PagerView != None)
	{
	  wins[j++] = t->PagerView;
	}
      }
    }
    XRestackWindows(dpy, wins, j);
  }
  free (wins);
}


/*
 *
 *  Procedure:
 *      list_end - displays packet contents to stderr
 *
 */
void list_end(void)
{
  unsigned int nchildren,i;
  Window root, parent, *children;
  PagerWindow *ptr;

  if(!XQueryTree(dpy, Scr.Root, &root, &parent, &children,
		 &nchildren))
    return;

  for(i=0; i<nchildren;i++)
    {
      ptr = Start;
      while(ptr != NULL)
	{
	  if((ptr->frame == children[i])||(ptr->icon_w == children[i])||
	     (ptr->icon_pixmap_w == children[i]))
	    {
	      if(ptr->PagerView != None)
		XRaiseWindow(dpy,ptr->PagerView);
	      XRaiseWindow(dpy,ptr->IconView);
	    }
	  ptr = ptr->next;
	}
    }


  if(nchildren > 0)
    XFree((char *)children);
}


void list_config_info(unsigned long *body)
{
  char *tline, *token;
  int color;

  tline = (char*)&(body[3]);
  token = PeekToken(tline, &tline);
  if (StrEquals(token, "Colorset"))
  {
    color = LoadColorset(tline);
    change_colorset(color);
  }
  else if (StrEquals(token, XINERAMA_CONFIG_STRING))
  {
    FScreenConfigureModule(tline);
  }
  else if (StrEquals(token, "DesktopName"))
  {
    int val;
    if (GetIntegerArguments(tline, &tline, &val, 1) > 0)
    {
      SetDeskLabel(val, (const char *)tline);
    }
    else
    {
      return;
    }
    if (fAlwaysCurrentDesk)
    {
      if (Scr.CurrentDesk == val)
	val = 0;
    }
    else if ((val >= desk1) && (val <=desk2))
    {
      val = val - desk1;
    }
    DrawGrid(val, True, None, NULL);
  }
}

void list_property_change(unsigned long *body)
{
  if (body[0] == MX_PROPERTY_CHANGE_BACKGROUND)
  {

    if (((!Swallowed && body[2] == 0) || (Swallowed && body[2] == Scr.Pager_w)))
    {
	    update_pr_transparent_windows();
    }
  }
  else if  (body[0] == MX_PROPERTY_CHANGE_SWALLOW && body[2] == Scr.Pager_w)
  {
    Swallowed = body[1];
  }
}


void list_reply(unsigned long *body)
{
	char *tline;
	tline = (char*)&(body[3]);
	if (strcmp(tline, "ScrollDone") == 0)
	{
		HandleScrollDone();
	}
}

/*
 *
 * Waits for next X event, or for an auto-raise timeout.
 *
 */
int My_XNextEvent(Display *dpy, XEvent *event)
{
  fd_set in_fdset;
  static int miss_counter = 0;

  if(FPending(dpy))
  {
    FNextEvent(dpy,event);
    return 1;
  }

  FD_ZERO(&in_fdset);
  FD_SET(x_fd,&in_fdset);
  FD_SET(fd[1],&in_fdset);

  if (fvwmSelect(fd_width, &in_fdset, 0, 0, NULL) > 0)
  {
  if(FD_ISSET(x_fd, &in_fdset))
    {
      if(FPending(dpy))
	{
	  FNextEvent(dpy,event);
	  miss_counter = 0;
	  return 1;
	}
	miss_counter++;
#ifdef WORRY_ABOUT_MISSED_XEVENTS
	if(miss_counter > 100)
	  DeadPipe(0);
#endif
    }

  if(FD_ISSET(fd[1], &in_fdset))
    {
      FvwmPacket* packet = ReadFvwmPacket(fd[1]);
      if ( packet == NULL )
	  exit(0);
      process_message( packet );
    }
  }
  return 0;
}



/* This function is really tricky. The two offsets are the offsets of the
 * colorset members of the DeskInfo and PagerSringList structures to be
 * modified. The lines accessing this info look very ugly, but they work. */
static void ParseColorset(char *arg1, char *arg2, void *offset_deskinfo,
			  void *offset_item, int *colorset_global)
{
  Bool all_desks = False;
  int colorset = 0;
  int i;
  int desk;
  unsigned long colorset_offset = (unsigned long)offset_deskinfo;
  unsigned long item_colorset_offset = (unsigned long)offset_item;

  sscanf(arg2, "%d", &colorset);
  AllocColorset(colorset);
  if (StrEquals(arg1, "*"))
  {
    all_desks = True;
    desk = Scr.CurrentDesk;
  }
  else
  {
    desk = desk1;
    sscanf(arg1,"%d",&desk);
  }
  if (fAlwaysCurrentDesk)
  {
    if (all_desks)
    {
      *colorset_global = colorset;
    }
    else
    {
      PagerStringList *item;

      item = FindDeskStrings(desk);
      if (item->next != NULL)
      {
	*(int *)(((char *)(item->next)) + item_colorset_offset) = colorset;
      }
      else
      {
	/* new Dcolor and desktop */
	item = NewPagerStringItem(item, desk);
	*(int *)(((char *)item) + item_colorset_offset) = colorset;
      }
    }
    if (desk == Scr.CurrentDesk || all_desks)
    {
      *(int *)(((char *)&Desks[0]) + colorset_offset) = colorset;
    }
  }
  else if (all_desks)
  {
    for (i = 0; i < ndesks; i++)
    {
      *(int *)(((char *)&Desks[i]) + colorset_offset) = colorset;
    }
  }
  else if((desk >= desk1)&&(desk <=desk2))
  {
    *(int *)(((char *)&Desks[desk - desk1]) + colorset_offset) = colorset;
  }

  return;
}

static void SetDeskLabel(int desk, const char *label)
{
  PagerStringList *item;

  if (fAlwaysCurrentDesk)
  {
    item = FindDeskStrings(desk);
    if (item->next != NULL)
    {
      /* replace label */
      if (item->next->label != NULL)
      {
	free(item->next->label);
	item->next->label = NULL;
      }
      CopyString(&(item->next->label), label);
    }
    else
    {
      /* new Dcolor and desktop */
      item = NewPagerStringItem(item, desk);
      CopyString(&(item->label), label);
    }
    if (desk == Scr.CurrentDesk)
    {
      free(Desks[0].label);
      CopyString(&Desks[0].label, label);
    }
  }
  else if((desk >= desk1)&&(desk <=desk2))
  {
    free(Desks[desk - desk1].label);
    CopyString(&Desks[desk - desk1].label, label);
  }
}

/*
 *
 * This routine is responsible for reading and parsing the config file
 *
 */
void ParseOptions(void)
{
  char *tline= NULL;
  int desk;
  int dx = 3;
  int dy = 3;
  FvwmPictureAttributes fpa;
  Scr.FvwmRoot = NULL;
  Scr.Hilite = NULL;
  Scr.VScale = 32;

  Scr.MyDisplayWidth = DisplayWidth(dpy, Scr.screen);
  Scr.MyDisplayHeight = DisplayHeight(dpy, Scr.screen);

  fpa.mask = 0;
  if (Pdepth <= 8)
  {
	  fpa.mask |= FPAM_DITHER;
  }
  InitGetConfigLine(fd,CatString3("*",MyName,0));
  for (GetConfigLine(fd,&tline); tline != NULL; GetConfigLine(fd,&tline))
  {
    int g_x, g_y, flags;
    unsigned width,height;
    char *resource;
    char *arg1;
    char *arg2;
    char *tline2;
    char *token;
    char *next;
    Bool MoveThresholdSetForModule = False;

    arg1 = arg2 = NULL;

    token = PeekToken(tline, &next);
    if (StrEquals(token, "Colorset"))
    {
      LoadColorset(next);
      continue;
    }
    else if (StrEquals(token, XINERAMA_CONFIG_STRING))
    {
      FScreenConfigureModule(next);
    }
    else if (StrEquals(token, "DesktopSize"))
    {
      token = PeekToken(next, &next);
      if (token)
      {
	sscanf(token, "%d", &dx);
	token = PeekToken(next, &next);
	if (token)
	  sscanf(token, "%d", &dy);
      }
      continue;
    }
    else if (StrEquals(token, "ImagePath"))
    {
      if (ImagePath != NULL)
      {
	free(ImagePath);
	ImagePath = NULL;
      }
      GetNextToken(next, &ImagePath);

      continue;
    }
    else if (StrEquals(token, "MoveThreshold"))
    {
      if (!MoveThresholdSetForModule)
      {
	int val;
	if (GetIntegerArguments(next, NULL, &val, 1) > 0)
	{
	  if (val >= 0)
	    MoveThreshold = val;
	  else
	    MoveThreshold = DEFAULT_PAGER_MOVE_THRESHOLD;
	}
      }
      continue;
    }
    else if (StrEquals(token, "DesktopName"))
    {
      int val;
      if (GetIntegerArguments(next, &next, &val, 1) > 0)
      {
	SetDeskLabel(val, (const char *)next);
      }
      continue;
    }

    tline2 = GetModuleResource(tline, &resource, MyName);
    if (!resource)
      continue;
    tline2 = GetNextToken(tline2, &arg1);
    if (!arg1)
    {
      arg1 = xmalloc(1);
      arg1[0] = 0;
    }
    tline2 = GetNextToken(tline2, &arg2);
    if (!arg2)
    {
      arg2 = xmalloc(1);
      arg2[0] = 0;
    }

    if(StrEquals(resource,"Colorset"))
    {
      ParseColorset(arg1, arg2,
		    &(((DeskInfo *)(NULL))->colorset),
		    &(((PagerStringList *)(NULL))->colorset),
		    &globalcolorset);
    }
    else if(StrEquals(resource,"BalloonColorset"))
    {
      ParseColorset(arg1, arg2,
		    &(((DeskInfo *)(NULL))->ballooncolorset),
		    &(((PagerStringList *)(NULL))->ballooncolorset),
		    &globalballooncolorset);
    }
    else if(StrEquals(resource,"HilightColorset"))
    {
      ParseColorset(arg1, arg2,
		    &(((DeskInfo *)(NULL))->highcolorset),
		    &(((PagerStringList *)(NULL))->highcolorset),
		    &globalhighcolorset);
    }
    else if (StrEquals(resource, "Geometry"))
    {
      window_w = 0;
      window_h = 0;
      window_x = 0;
      window_y = 0;
      xneg = 0;
      yneg = 0;
      usposition = 0;
      flags = FScreenParseGeometry(arg1,&g_x,&g_y,&width,&height);
      if (flags & WidthValue)
      {
	window_w = width;
      }
      if (flags & HeightValue)
      {
	window_h = height;
      }
      if (flags & XValue)
      {
	window_x = g_x;
	usposition = 1;
	if (flags & XNegative)
	{
	  xneg = 1;
	}
      }
      if (flags & YValue)
      {
	window_y = g_y;
	usposition = 1;
	if (flags & YNegative)
	{
	  yneg = 1;
	}
      }
    }
    else if (StrEquals(resource, "IconGeometry"))
    {
      icon_w = 0;
      icon_h = 0;
      icon_x = -10000;
      icon_y = -10000;
      icon_xneg = 0;
      icon_yneg = 0;
      flags = FScreenParseGeometry(arg1,&g_x,&g_y,&width,&height);
      if (flags & WidthValue)
	icon_w = width;
      if (flags & HeightValue)
	icon_h = height;
      if (flags & XValue)
      {
	icon_x = g_x;
	if (flags & XNegative)
	{
	  icon_xneg = 1;
	}
      }
      if (flags & YValue)
      {
	icon_y = g_y;
	if (flags & YNegative)
	{
	  icon_yneg = 1;
	}
      }
    }
    else if (StrEquals(resource, "Label"))
    {
      if (StrEquals(arg1, "*"))
      {
	desk = Scr.CurrentDesk;
      }
      else
      {
	desk = desk1;
	sscanf(arg1,"%d",&desk);
      }
      SetDeskLabel(desk, (const char *)arg2);
    }
    else if (StrEquals(resource, "Font"))
    {
      if (font_string)
	free(font_string);
      CopyStringWithQuotes(&font_string, next);
      if(strncasecmp(font_string,"none",4) == 0)
      {
	uselabel = 0;
	if (font_string)
	{
	  free(font_string);
	  font_string = NULL;
	}
      }
    }
    else if (StrEquals(resource, "Fore"))
    {
      if(Pdepth > 1)
      {
	if (PagerFore)
	  free(PagerFore);
	CopyString(&PagerFore,arg1);
      }
    }
    else if (StrEquals(resource, "Back"))
    {
      if(Pdepth > 1)
      {
	if (PagerBack)
	  free(PagerBack);
	CopyString(&PagerBack,arg1);
      }
    }
    else if (StrEquals(resource, "DeskColor"))
    {
      if (StrEquals(arg1, "*"))
      {
	desk = Scr.CurrentDesk;
      }
      else
      {
	desk = desk1;
	sscanf(arg1,"%d",&desk);
      }
      if (fAlwaysCurrentDesk)
      {
	PagerStringList *item;

	item = FindDeskStrings(desk);
	if (item->next != NULL)
	{
	  /* replace Dcolor */
	  if (item->next->Dcolor != NULL)
	  {
	    free(item->next->Dcolor);
	    item->next->Dcolor = NULL;
	  }
	  CopyString(&(item->next->Dcolor), arg2);
	}
	else
	{
	  /* new Dcolor and desktop */
	  item = NewPagerStringItem(item, desk);
	  CopyString(&(item->Dcolor), arg2);
	}
	if (desk == Scr.CurrentDesk)
	{
	  free(Desks[0].Dcolor);
	  CopyString(&Desks[0].Dcolor, arg2);
	}
      }
      else if((desk >= desk1)&&(desk <=desk2))
      {
	free(Desks[desk - desk1].Dcolor);
	CopyString(&Desks[desk - desk1].Dcolor, arg2);
      }
    }
    else if (StrEquals(resource, "DeskPixmap"))
    {
      if (StrEquals(arg1, "*"))
      {
	desk = Scr.CurrentDesk;
      }
      else
      {
	desk = desk1;
	sscanf(arg1,"%d",&desk);
      }
      if (fAlwaysCurrentDesk)
      {
	PagerStringList *item;

	item = FindDeskStrings(desk);

	if (item->next != NULL)
	{
	  if (item->next->bgPixmap != NULL)
	  {
	    PDestroyFvwmPicture(dpy, item->next->bgPixmap);
	    item->next->bgPixmap = NULL;
	  }
	  item->next->bgPixmap = PCacheFvwmPicture(
		  dpy, Scr.Pager_w, ImagePath, arg2, fpa);
	}
	else
	{
	  /* new Dcolor and desktop */
	  item = NewPagerStringItem(item, desk);
	  item->bgPixmap = PCacheFvwmPicture(
		  dpy, Scr.Pager_w, ImagePath, arg2, fpa);
	}
	if (desk == Scr.CurrentDesk)
	{
	  if (Desks[0].bgPixmap != NULL)
	  {
	    PDestroyFvwmPicture(dpy, Desks[0].bgPixmap);
	    Desks[0].bgPixmap = NULL;
	  }

	  Desks[0].bgPixmap = PCacheFvwmPicture(
		  dpy, Scr.Pager_w, ImagePath, arg2, fpa);
	}
      }
      else if((desk >= desk1)&&(desk <=desk2))
      {
	int dNr = desk - desk1;

	if (Desks[dNr].bgPixmap != NULL)
	{
	  PDestroyFvwmPicture(dpy, Desks[dNr].bgPixmap);
	  Desks[dNr].bgPixmap = NULL;
	}
	Desks[dNr].bgPixmap = PCacheFvwmPicture(
		dpy, Scr.Pager_w, ImagePath, arg2, fpa);
      }

    }
    else if (StrEquals(resource, "Pixmap"))
    {
      if(Pdepth > 1)
      {
	if (PixmapBack) {
	  PDestroyFvwmPicture (dpy, PixmapBack);
	  PixmapBack = NULL;
	}

	PixmapBack = PCacheFvwmPicture(
		dpy, Scr.Pager_w, ImagePath, arg1, fpa);

      }
    }
    else if (StrEquals(resource, "HilightPixmap"))
    {
      if(Pdepth > 1)
      {
	if (HilightPixmap) {
	  PDestroyFvwmPicture (dpy, HilightPixmap);
	  HilightPixmap = NULL;
	}

	HilightPixmap = PCacheFvwmPicture (
		dpy, Scr.Pager_w, ImagePath, arg1, fpa);

      }
    }
    else if (StrEquals(resource, "DeskHilight"))
    {
      HilightDesks = 1;
    }
    else if (StrEquals(resource, "NoDeskHilight"))
    {
      HilightDesks = 0;
    }
    else if (StrEquals(resource, "Hilight"))
    {
      if(Pdepth > 1)
      {
	if (HilightC)
	  free(HilightC);
	CopyString(&HilightC,arg1);
      }
    }
    else if (StrEquals(resource, "SmallFont"))
    {
      if (smallFont)
	free(smallFont);
      CopyStringWithQuotes(&smallFont, next);
      if (strncasecmp(smallFont,"none",4) == 0)
      {
	free(smallFont);
	smallFont = NULL;
      }
    }
    else if (StrEquals(resource, "MiniIcons"))
    {
      MiniIcons = 1;
    }
    else if (StrEquals(resource, "StartIconic"))
    {
      StartIconic = 1;
    }
    else if (StrEquals(resource, "NoStartIconic"))
    {
      StartIconic = 0;
    }
    else if (StrEquals(resource, "LabelsBelow"))
    {
      LabelsBelow = 1;
    }
    else if (StrEquals(resource, "LabelsAbove"))
    {
      LabelsBelow = 0;
    }
    else if (FHaveShapeExtension && StrEquals(resource, "ShapeLabels"))
    {
      ShapeLabels = 1;
    }
    else if (FHaveShapeExtension && StrEquals(resource, "NoShapeLabels"))
    {
      ShapeLabels = 0;
    }
    else if (StrEquals(resource, "Rows"))
    {
      sscanf(arg1,"%d",&Rows);
    }
    else if (StrEquals(resource, "Columns"))
    {
      sscanf(arg1,"%d",&Columns);
    }
    else if (StrEquals(resource, "DeskTopScale"))
    {
      sscanf(arg1,"%d",&Scr.VScale);
    }
    else if (StrEquals(resource, "WindowColors"))
    {
      if (Pdepth > 1)
      {
	if (WindowFore)
	  free(WindowFore);
	if (WindowBack)
	  free(WindowBack);
	if (WindowHiFore)
	  free(WindowHiFore);
	if (WindowHiBack)
	  free(WindowHiBack);
	CopyString(&WindowFore, arg1);
	CopyString(&WindowBack, arg2);
	tline2 = GetNextToken(tline2, &WindowHiFore);
	GetNextToken(tline2, &WindowHiBack);
      }
    }
    else if (StrEquals(resource, "WindowBorderWidth"))
    {
      sscanf(arg1, "%d", &WindowBorderWidth);
      MinSize = 2 * WindowBorderWidth + 1;
    }
    else if (StrEquals(resource, "Window3dBorders"))
    {
      WindowBorders3d = True;
    }
    else if (StrEquals(resource,"WindowColorsets"))
    {
      sscanf(arg1,"%d",&windowcolorset);
      AllocColorset(windowcolorset);
      sscanf(arg2,"%d",&activecolorset);
      AllocColorset(activecolorset);
    }
    else if (StrEquals(resource,"WindowLabelFormat"))
    {
      if (WindowLabelFormat)
	free(WindowLabelFormat);
      CopyString(&WindowLabelFormat,arg1);
    }
    else if (StrEquals(resource,"UseSkipList"))
    {
      UseSkipList = True;
    }
    else if (StrEquals(resource, "MoveThreshold"))
    {
      int val;
      if (GetIntegerArguments(next, NULL, &val, 1) > 0 && val >= 0)
      {
	MoveThreshold = val;
	MoveThresholdSetForModule = True;
      }
    }
    else if (StrEquals(resource, "SloppyFocus"))
    {
      do_focus_on_enter = True;
    }
    else if (StrEquals(resource, "SolidSeparators"))
    {
      use_dashed_separators = False;
      use_no_separators = False;
    }
    else if (StrEquals(resource, "NoSeparators"))
    {
      use_no_separators = True;
    }
    /* ... and get Balloon config options ...
       -- ric@giccs.georgetown.edu */
    else if (StrEquals(resource, "Balloons"))
    {
      if (BalloonTypeString)
	free(BalloonTypeString);
      CopyString(&BalloonTypeString, arg1);

      if ( strncasecmp(BalloonTypeString, "Pager", 5) == 0 ) {
	ShowPagerBalloons = 1;
	ShowIconBalloons = 0;
      }
      else if ( strncasecmp(BalloonTypeString, "Icon", 4) == 0 ) {
	ShowPagerBalloons = 0;
	ShowIconBalloons = 1;
      }
      else {
	ShowPagerBalloons = 1;
	ShowIconBalloons = 1;
      }

      /* turn this on initially so balloon window is created; later this
	 variable is changed to match ShowPagerBalloons or ShowIconBalloons
	 whenever we receive iconify or deiconify packets */
      ShowBalloons = 1;
    }

    else if (StrEquals(resource, "BalloonBack"))
    {
      if (Pdepth > 1)
      {
	if (BalloonBack)
	  free(BalloonBack);
	CopyString(&BalloonBack, arg1);
      }
    }

    else if (StrEquals(resource, "BalloonFore"))
    {
      if (Pdepth > 1)
      {
	if (BalloonFore)
	  free(BalloonFore);
	CopyString(&BalloonFore, arg1);
      }
    }

    else if (StrEquals(resource, "BalloonFont"))
    {
      if (BalloonFont)
	free(BalloonFont);
      CopyStringWithQuotes(&BalloonFont, next);
    }

    else if (StrEquals(resource, "BalloonBorderColor"))
    {
      if (BalloonBorderColor)
	free(BalloonBorderColor);
      CopyString(&BalloonBorderColor, arg1);
    }

    else if (StrEquals(resource, "BalloonBorderWidth"))
    {
      sscanf(arg1, "%d", &BalloonBorderWidth);
    }

    else if (StrEquals(resource, "BalloonYOffset"))
    {
      sscanf(arg1, "%d", &BalloonYOffset);
      if (BalloonYOffset == 0)
      {
	fprintf(stderr,
		"%s: Warning:"
		" you're not allowed BalloonYOffset 0; defaulting to +3\n",
		MyName);
	/* we don't allow yoffset of 0 because it allows direct transit from
	 * pager window to balloon window, setting up a LeaveNotify/EnterNotify
	 * event loop */
	BalloonYOffset = 3;
      }
    }
    else if (StrEquals(resource,"BalloonStringFormat"))
    {
      if (BalloonFormatString)
	free(BalloonFormatString);
      CopyString(&BalloonFormatString,arg1);
    }

    free(resource);
    free(arg1);
    free(arg2);
  }

  Scr.VxMax = dx * Scr.MyDisplayWidth - Scr.MyDisplayWidth;
  Scr.VyMax = dy * Scr.MyDisplayHeight - Scr.MyDisplayHeight;
  if (Scr.VxMax < 0)
    Scr.VxMax = 0;
  if (Scr.VyMax < 0)
    Scr.VyMax = 0;
  Scr.VWidth = Scr.VxMax + Scr.MyDisplayWidth;
  Scr.VHeight = Scr.VyMax + Scr.MyDisplayHeight;
  Scr.VxPages = Scr.VWidth / Scr.MyDisplayWidth;
  Scr.VyPages = Scr.VHeight / Scr.MyDisplayHeight;
  Scr.Vx = 0;
  Scr.Vy = 0;

  return;
}

/* Returns the item in the sring list that has item->next->desk == desk or
 * the last item (item->next == NULL) if no entry matches the desk number. */
PagerStringList *FindDeskStrings(int desk)
{
  PagerStringList *item;

  item = &string_list;
  while (item->next != NULL)
  {
    if (item->next->desk == desk)
      break;
    item = item->next;
  }
  return item;
}

PagerStringList *NewPagerStringItem(PagerStringList *last, int desk)
{
  PagerStringList *newitem;

  newitem = xcalloc(1, sizeof(PagerStringList));
  last->next = newitem;
  newitem->colorset = -1;
  newitem->highcolorset = -1;
  newitem->ballooncolorset = -1;
  newitem->desk = desk;

  return newitem;
}

void ExitPager(void)
{
  if (is_transient)
  {
    XUngrabPointer(dpy,CurrentTime);
    MyXUngrabServer(dpy);
    XSync(dpy,0);
  }
  XUngrabKeyboard(dpy, CurrentTime);
  exit(0);
}
