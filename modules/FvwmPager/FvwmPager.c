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
#include <stdbool.h>
#include <sys/wait.h>
#include "libs/FScreen.h"
#include "libs/ftime.h"
#include <ctype.h>

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif /* Saul */

#include "libs/Colorset.h"
#include "libs/FEvent.h"
#include "libs/Flocale.h"
#include "libs/FRenderInit.h"
#include "libs/FShape.h"
#include "libs/fvwmlib.h"
#include "libs/fvwmsignal.h"
#include "libs/Grab.h"
#include "libs/log.h"
#include "libs/Module.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/System.h"

#include "fvwm/fvwm.h"
#include "FvwmPager.h"

/*
 *
 * Shared variables.
 *
 */
/* Colors, Pixmaps, Fonts, etc. */
char		*HilightC = NULL;
char		*PagerFore = NULL;
char		*PagerBack = NULL;
char		*smallFont = NULL;
char		*ImagePath = NULL;
char		*WindowBack = NULL;
char		*WindowFore = NULL;
char		*font_string = NULL;
char		*BalloonFore = NULL;
char		*BalloonBack = NULL;
char		*BalloonFont = NULL;
char		*WindowHiFore = NULL;
char		*WindowHiBack = NULL;
char		*WindowLabelFormat = NULL;
char		*BalloonTypeString = NULL;
char		*BalloonBorderColor = NULL;
char		*BalloonFormatString = NULL;
Pixel		hi_pix;
Pixel		back_pix;
Pixel		fore_pix;
Pixel		focus_pix;
Pixel		win_back_pix;
Pixel		win_fore_pix;
Pixel		focus_fore_pix;
Pixel		win_hi_back_pix;
Pixel		win_hi_fore_pix;
Pixmap		default_pixmap = None;
FlocaleFont	*Ffont;
FlocaleFont	*FwindowFont;
FvwmPicture	*PixmapBack = NULL;
FvwmPicture	*HilightPixmap = NULL;
FlocaleWinString	*FwinString;

/* Sizes / Dimensions */
int		Rows = -1;
int		desk1 = 0;
int		desk2 = 0;
int		desk_i = 0;
int		ndesks = 0;
int		Columns = -1;
int		MoveThreshold = DEFAULT_PAGER_MOVE_THRESHOLD;
int		BalloonBorderWidth = DEFAULT_BALLOON_BORDER_WIDTH;
int		BalloonYOffset = DEFAULT_BALLOON_Y_OFFSET;
unsigned int	WindowBorderWidth = DEFAULT_PAGER_WINDOW_BORDER_WIDTH;
unsigned int	MinSize = 2 *  DEFAULT_PAGER_WINDOW_BORDER_WIDTH +
				  DEFAULT_PAGER_WINDOW_MIN_SIZE;
rectangle	pwindow = {0, 0, 0, 0};
rectangle	icon = {-10000, -10000, 0, 0};

/* Settings */
int	windowcolorset = -1;
int	activecolorset = -1;
bool	xneg = false;
bool	yneg = false;
bool	icon_xneg = false;
bool	icon_yneg = false;
bool	MiniIcons = false;
bool	Swallowed = false;
bool	usposition = false;
bool	UseSkipList = false;
bool	StartIconic = false;
bool	LabelsBelow = false;
bool	ShapeLabels = false;
bool	win_pix_set = false;
bool	is_transient = false;
bool	HilightDesks = true;
bool	ShowBalloons = false;
bool	error_occured = false;
bool	use_desk_label = true;
bool	win_hi_pix_set = false;
bool	WindowBorders3d = false;
bool	HideSmallWindows = false;
bool	ShowIconBalloons = false;
bool	use_no_separators = false;
bool	use_monitor_label = false;
bool	ShowPagerBalloons = false;
bool	do_focus_on_enter = false;
bool	fAlwaysCurrentDesk = false;
bool	use_dashed_separators = true;
bool	do_ignore_next_button_release = false;

/* Screen / Windows */
int		fd[2];
char		*MyName;
Window		icon_win;
Window		BalloonView = None;
Display		*dpy;
DeskInfo	*Desks;
ScreenInfo	Scr;
PagerWindow	*Start = NULL;
PagerWindow	*FocusWin = NULL;

/* Monitors */
bool			fp_is_tracking_shared = false;
char			*monitor_to_track = NULL;
char			*preferred_monitor = NULL;
struct fpmonitors	fp_monitor_q;
enum monitor_tracking	fp_monitor_mode = MONITOR_TRACKING_G;

static int x_fd;
static fd_set_size_t fd_width;
static int globalcolorset = -1;
static int globalballooncolorset = -1;
static int globalhighcolorset = -1;
static PagerStringList string_list = { NULL, 0, -1, -1, -1, NULL, NULL, NULL };
static bool fp_new_block = false;

static PagerStringList *FindDeskStrings(int desk);
static PagerStringList *NewPagerStringItem(PagerStringList *last, int desk);
static void SetDeskLabel(struct fpmonitor *, int desk, const char *label);
static RETSIGTYPE TerminateHandler(int);
static void list_monitor_focus(unsigned long *);
static struct fpmonitor *fpmonitor_new(struct monitor *);
static void fpmonitor_disable(struct fpmonitor *);
static void parse_monitor_line(char *);
static void parse_desktop_size_line(char *);
static void parse_desktop_configuration_line(char *);

struct fpmonitor *
fpmonitor_new(struct monitor *m)
{
	struct fpmonitor	*fp;

	fp = fxcalloc(1, sizeof(*fp));
	fp->CPagerWin = fxcalloc(1, ndesks * sizeof(*fp->CPagerWin));
	fp->m = m;
	fp->disabled = false;
	TAILQ_INSERT_TAIL(&fp_monitor_q, fp, entry);

	return (fp);
}

void fpmonitor_disable(struct fpmonitor *fp)
{
	int i;

	fp->disabled = true;
	for (i = 0; i < ndesks; i++) {
		XMoveWindow(dpy, fp->CPagerWin[i], -32768,-32768);
	}

	if (monitor_to_track != NULL &&
		strcmp(monitor_to_track, fp->m->si->name) == 0)
	{
		free(monitor_to_track);
		struct fpmonitor *tm = fpmonitor_this(NULL);
		monitor_to_track = fxstrdup(tm->m->si->name);
	}
}

struct fpmonitor *
fpmonitor_by_name(const char *name)
{
	struct fpmonitor *fm;

	if (name == NULL)
		return (NULL);

	TAILQ_FOREACH(fm, &fp_monitor_q, entry) {
		if (fm->m != NULL && (strcmp(name, fm->m->si->name) == 0))
			return (fm);
	}
	return (NULL);
}

struct fpmonitor *
fpmonitor_this(struct monitor *m_find)
{
	struct monitor *m = NULL;
	struct fpmonitor *fp = NULL;

	if (m_find != NULL) {
		/* We've been asked to find a specific monitor. */
		m = m_find;
		goto search;
	}
	if (monitor_to_track == NULL)
		m = monitor_get_current();
	else
		m = monitor_resolve_name(monitor_to_track);

	if (m == NULL)
		m = monitor_get_current();

search:
	if (m == NULL || m->flags & MONITOR_DISABLED)
		return (NULL);
	TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
		if (fp->m == m)
			return (fp);
	}
	return (NULL);

}

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
  struct monitor *mon;

  FlocaleInit(LC_CTYPE, "", "", "FvwmPager");

  /* Tell the FEvent module an event type that is not used by fvwm. */
  fev_init_invalid_event_type(KeymapNotify);

  /* Save our program  name - for error messages */
  MyName = GetFileNameFromPath(argv[0]);

  if(argc  < 6)
    {
      fvwm_debug(__func__, "%s Version %s should only be executed by fvwm!\n",
                 MyName,
                 VERSION);
      exit(1);
    }

  TAILQ_INIT(&fp_monitor_q);

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
    is_transient = true;
      do_ignore_next_button_release = true;
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
	    MyName = fxstrdup(argv[opt_num]);
	    opt_num++;
	    break;
	  }
	}
      }
    }

  if (argc < opt_num + 1)
    {
      desk1 = 0;
      desk2 = 0;
    }
  else if (StrEquals(argv[opt_num], "*"))
    {
      desk1 = 0;
      desk2 = 0;
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

  Desks = fxcalloc(1, ndesks*sizeof(DeskInfo));
  for(i=0;i<ndesks;i++)
    {
      snprintf(line,sizeof(line),"Desk %d",i+desk1);
      CopyString(&Desks[i].label,line);
      Desks[i].colorset = -1;
      Desks[i].highcolorset = -1;
      Desks[i].ballooncolorset = -1;
    }

  /* Initialize X connection */
  if (!(dpy = XOpenDisplay(display_name)))
    {
      fvwm_debug(__func__, "%s: can't open display %s", MyName,
                 XDisplayName(display_name));
      exit (1);
    }

  flib_init_graphics(dpy);

  x_fd = XConnectionNumber(dpy);

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
		 MX_MONITOR_FOCUS);

  RB_FOREACH(mon, monitors, &monitor_q)
	(void)fpmonitor_new(mon);

  ParseOptions();
  if (is_transient)
  {
    FQueryPointer(
	  dpy, Scr.Root, &JunkRoot, &JunkChild, &pwindow.x, &pwindow.y, &JunkX,
	  &JunkY, &JunkMask);
    usposition = false;
    xneg = false;
    yneg = false;
  }

  if (PagerFore == NULL)
    PagerFore = fxstrdup("black");

  if (PagerBack == NULL)
    PagerBack = fxstrdup("white");

  if (HilightC == NULL)
    HilightC = fxstrdup(PagerFore);

  if (WindowLabelFormat == NULL)
    WindowLabelFormat = fxstrdup("%i");

  if ((HilightC == NULL) && (HilightPixmap == NULL))
    HilightDesks = false;

  if (BalloonBorderColor == NULL)
    BalloonBorderColor = fxstrdup("black");

  if (BalloonTypeString == NULL)
    BalloonTypeString = fxstrdup("%i");

  if (BalloonFormatString == NULL)
    BalloonFormatString = fxstrdup("%i");

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo(fd,"Send_WindowList",0);

  /* open a pager window */
  initialise_common_pager_fragments();
  initialize_pager();

  if (is_transient)
  {
    bool is_pointer_grabbed = false;
    bool is_keyboard_grabbed = false;
    XSync(dpy,0);
    for (i = 0; i < 50 && !(is_pointer_grabbed && is_keyboard_grabbed); i++)
    {
      if (!is_pointer_grabbed &&
	  XGrabPointer(
	    dpy, Scr.Root, true,
	    ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
	    PointerMotionMask|EnterWindowMask|LeaveWindowMask, GrabModeAsync,
	    GrabModeAsync, None, None, CurrentTime) == GrabSuccess)
      {
	is_pointer_grabbed = true;
      }
      if (!is_keyboard_grabbed &&
	  XGrabKeyboard(
	    dpy, Scr.Root, true, GrabModeAsync, GrabModeAsync, CurrentTime) ==
	  GrabSuccess)
      {
	is_keyboard_grabbed = true;
      }
      /* If you go too fast, other windows may not get a change to release
       * any grab that they have. */
      usleep(20000);
    }
    if (!is_pointer_grabbed)
    {
      XBell(dpy, 0);
      fvwm_debug(__func__,
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
    fvwm_debug(__func__, "%s: Received signal: exiting...\n", MyName);
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
		      (unsigned *)&pwindow.width,(unsigned *)&pwindow.height,
		      &border_width,&depth)==0)
      {
	/* does not return */
	ExitPager();
      }
      error_occured = false;
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
    case MX_MONITOR_FOCUS:
      list_monitor_focus(body);
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
		fvwm_debug(__func__,
			   "%s: Error: Internal window list corrupt\n",MyName);
		/* might be a good idea to exit here */
		return;
	}
	t->w = cfgpacket->w;
	t->frame = cfgpacket->frame;
	t->frame_x = cfgpacket->frame_x;
	t->frame_y = cfgpacket->frame_y;
	t->frame_width = cfgpacket->frame_width;
	t->frame_height = cfgpacket->frame_height;
	t->m = monitor_by_output(cfgpacket->monitor_id);

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
	struct ConfigWinPacket  *cfgpacket = (void *) body;

	t = Start;
	prev = &Start;

	while(t != NULL)
	{
		if (t->w == cfgpacket->w)
		{
			/* it's already there, do nothing */
			return;
		}
		prev = &(t->next);
		t = t->next;
	}
	*prev = fxcalloc(1, sizeof(PagerWindow));
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
  bool is_new_desk;
  struct monitor *newm;

  target_w = cfgpacket->w;
  t = Start;
  newm = monitor_by_output((int)cfgpacket->monitor_id);

  while( (t != NULL) && (t->w != target_w))
  {
	  if (t->m != newm) {
		  t = t->next;
		  continue;
	  }
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
    ChangeDeskForWindow(t, cfgpacket->desk);
  MoveResizePagerView(t, true);
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
      XDestroyWindow(dpy,t->PagerView);
      XDestroyWindow(dpy,t->IconView);
      if(FocusWin == t)
	FocusWin = NULL;
      free(t->res_class);
      free(t->res_name);
      free(t->window_name);
      free(t->icon_name);
      free(t);
    }
}

void list_monitor_focus(unsigned long *body)
{
	return;
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
  bool do_force_update = false;

  target_w = body[0];
  if (win_hi_pix_set)
  {
    if (focus_pix != win_hi_back_pix || focus_fore_pix != win_hi_fore_pix)
    {
      do_force_update = true;
    }
    focus_pix = win_hi_back_pix;
    focus_fore_pix = win_hi_fore_pix;
  }
  else
  {
    if (focus_pix != body[4] || focus_fore_pix != body[3])
    {
      do_force_update = true;
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
      Hilight(FocusWin, false);
    FocusWin = t;
    if (FocusWin != NULL)
      Hilight(FocusWin, true);
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
	int mon_num = body[7];
	struct monitor *m;
	struct fpmonitor *fp;

	m = monitor_by_output(mon_num);
	/* Don't allow monitor_by_output to fallback to RB_MIN. */
	if (m->si->rr_output != mon_num)
		return;

	if (monitor_to_track != NULL &&
	    strcmp(m->si->name, monitor_to_track) != 0)
		return;

	if ((fp = fpmonitor_this(m)) == NULL)
		return;

	fp->virtual_scr.Vx = fp->m->virtual_scr.Vx = body[0];
	fp->virtual_scr.Vy = fp->m->virtual_scr.Vy = body[1];
	if (fp->m->virtual_scr.CurrentDesk != body[2]) {
		/* first handle the new desk */
		body[0] = body[2];
		body[1] = mon_num;
		list_new_desk(body);
	}
	if (fp->virtual_scr.VxPages != body[5] || fp->virtual_scr.VyPages != body[6])
	{
		fp->virtual_scr.VxPages = body[5];
		fp->virtual_scr.VyPages = body[6];
		fp->virtual_scr.VWidth = fp->virtual_scr.VxPages * fpmonitor_get_all_widths();
		fp->virtual_scr.VHeight = fp->virtual_scr.VyPages * fpmonitor_get_all_heights();
		fp->virtual_scr.VxMax = fp->virtual_scr.VWidth - fpmonitor_get_all_widths();
		fp->virtual_scr.VyMax = fp->virtual_scr.VHeight - fpmonitor_get_all_heights();
		ReConfigure();
	}

	MovePage(false);
	MoveStickyWindow(true, false);
	Hilight(FocusWin,true);
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
  int mon_num = body[1];
  struct monitor *mout;
  struct fpmonitor *fp;

  mout = monitor_by_output(mon_num);
  /* Don't allow monitor_by_output to fallback to RB_MIN. */
  if (mout->si->rr_output != mon_num)
	  return;

  fp = fpmonitor_this(mout);

  if (fp == NULL)
	  return;

  /* Best to update the desk, even if the monitor isn't being
   * tracked, as this could change.
   */
  oldDesk = fp->m->virtual_scr.CurrentDesk;
  fp->m->virtual_scr.CurrentDesk = (long)body[0];

  /* If the monitor for which the event was sent, does not match the monitor
   * itself, then don't change the FvwmPager's desk.  Only do this if we're
   * tracking a specific monitor though.
   */
  if (monitor_to_track != NULL &&
      (strcmp(fp->m->si->name, monitor_to_track) != 0))
	  return;

  /* Update the icon window to always track current desk. */
  desk_i = fp->m->virtual_scr.CurrentDesk;

  if (fp_monitor_mode == MONITOR_TRACKING_G)
    monitor_assign_virtual(fp->m);

  if (fAlwaysCurrentDesk && oldDesk != fp->m->virtual_scr.CurrentDesk)
  {
    PagerWindow *t;
    PagerStringList *item;
    char line[100];

    desk1 = fp->m->virtual_scr.CurrentDesk;
    desk2 = fp->m->virtual_scr.CurrentDesk;
    for (t = Start; t != NULL; t = t->next)
    {
      if (t->desk == oldDesk || t->desk == fp->m->virtual_scr.CurrentDesk)
	ChangeDeskForWindow(t, t->desk);
    }
    item = FindDeskStrings(fp->m->virtual_scr.CurrentDesk);
    free(Desks[0].label);
    Desks[0].label = NULL;
    if (item->next != NULL && item->next->label != NULL)
    {
      CopyString(&Desks[0].label, item->next->label);
    }
    else
    {
      snprintf(line, sizeof(line), "Desk %d", desk1);
      CopyString(&Desks[0].label, line);
    }
    XStoreName(dpy, Scr.Pager_w, Desks[0].label);
    XSetIconName(dpy, Scr.Pager_w, Desks[0].label);

    if (Desks[0].bgPixmap != NULL)
    {
      PDestroyFvwmPicture(dpy, Desks[0].bgPixmap);
      Desks[0].bgPixmap = NULL;
    }
    free (Desks[0].Dcolor);
    Desks[0].Dcolor = NULL;

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

    i = fp->m->virtual_scr.CurrentDesk - desk1;
    if (i >= 0 && i < ndesks && Desks[i].label != NULL)
    {
      name = Desks[i].label;
    }
    else
    {
      snprintf(line, sizeof(line), "Desk %d", fp->m->virtual_scr.CurrentDesk);
      name = &(line[0]);
    }
    XStoreName(dpy, Scr.Pager_w, name);
    XSetIconName(dpy, Scr.Pager_w, name);
  }

  MovePage(true);
  DrawGrid(oldDesk - desk1, 1, None, NULL);
  DrawGrid(fp->m->virtual_scr.CurrentDesk - desk1, 1, None, NULL);
  MoveStickyWindow(false, true);
/*
  Hilight(FocusWin,false);
*/
  Hilight(FocusWin,true);
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
  struct fpmonitor *m;

  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
      t = t->next;
    }
  if(t!= NULL)
    {
      XLowerWindow(dpy,t->PagerView);
      if (HilightDesks && (t->desk - desk1>=0) && (t->desk - desk1<ndesks)) {
	TAILQ_FOREACH(m, &fp_monitor_q, entry) {
		XLowerWindow(dpy, m->CPagerWin[t->desk - desk1]);
	}
      }
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
		SET_ICONIFIED(t, true);
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
		MoveResizePagerView(t, true);
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
    SET_ICONIFIED(t, false);
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

    MoveResizePagerView(t, true);
  }

  return;
}


void list_window_name(unsigned long *body,unsigned long type)
{
  PagerWindow *t;
  Window target_w;
  struct fpmonitor *fp = fpmonitor_this(NULL);

  if (fp == NULL)
    return;

  if (monitor_to_track != NULL && strcmp(fp->m->si->name, monitor_to_track) != 0)
	  return;

  target_w = body[0];
  t = Start;
  while((t!= NULL)&&(t->w != target_w))
    {
	    if (fp->m != NULL && t->m != fp->m) {
		    t = t->next;
		    continue;
	    }
      t = t->next;
    }
  if(t!= NULL)
    {
      switch (type) {
      case M_RES_CLASS:
	free(t->res_class);
	CopyString(&t->res_class,(char *)(&body[3]));
	break;
      case M_RES_NAME:
	free(t->res_name);
	CopyString(&t->res_name,(char *)(&body[3]));
	break;
      case M_VISIBLE_NAME:
	free(t->window_name);
	CopyString(&t->window_name,(char *)(&body[3]));
	break;
      }
      /* repaint by clearing window */
      if ((FwindowFont != NULL) && (t->icon_name != NULL)
	   && !(MiniIcons && t->mini_icon.picture)) {
	XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
	XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
      }
      if (ShowBalloons && BalloonView)
      {
	/* update balloons */
	if (BalloonView == t->PagerView)
	{
	  UnmapBalloonWindow();
	  MapBalloonWindow(t, false);
	}
	else if (BalloonView == t->IconView)
	{
	  UnmapBalloonWindow();
	  MapBalloonWindow(t, true);
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
      free(t->icon_name);
      CopyString(&t->icon_name,(char *)(&body[3]));
      /* repaint by clearing window */
      if ((FwindowFont != NULL) && (t->icon_name != NULL)
	   && !(MiniIcons && t->mini_icon.picture)) {
	XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
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
      XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
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

  wins = fxmalloc(length * sizeof (Window));
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
	wins[j++] = t->PagerView;
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
	struct fpmonitor *m;
	char *tline, *token;

	tline = (char*)&(body[3]);
	token = PeekToken(tline, &tline);
	if (StrEquals(token, "Colorset"))
	{
		int color;

		color = LoadColorset(tline);
		change_colorset(color);
	}
	else if (StrEquals(token, "DesktopName"))
	{
		int val;
		if (GetIntegerArguments(tline, &tline, &val, 1) > 0)
		{
			TAILQ_FOREACH(m, &fp_monitor_q, entry)
				SetDeskLabel(m, val, (const char *)tline);
		}
		else
		{
			return;
		}
		if (fAlwaysCurrentDesk)
		{
			TAILQ_FOREACH(m, &fp_monitor_q, entry) {
				if (m->m->virtual_scr.CurrentDesk == val)
					val = 0;
			}
		}
		else if ((val >= desk1) && (val <=desk2))
		{
			val = val - desk1;
		}
		DrawGrid(val, true, None, NULL);
	} else if (StrEquals(token, "Monitor")) {
		parse_monitor_line(tline);
		ReConfigure();
	} else if (StrEquals(token, "DesktopSize")) {
		parse_desktop_size_line(tline);
	} else if (StrEquals(token, "DesktopConfiguration")) {
		parse_desktop_configuration_line(tline);
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
	  return 1;
	}
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
  bool all_desks = false;
  int colorset = 0;
  int i;
  int desk;
  unsigned long colorset_offset = (unsigned long)offset_deskinfo;
  unsigned long item_colorset_offset = (unsigned long)offset_item;
  struct fpmonitor *fp = fpmonitor_this(NULL);

  if (fp == NULL)
    return;

  sscanf(arg2, "%d", &colorset);
  AllocColorset(colorset);
  if (StrEquals(arg1, "*"))
  {
    all_desks = true;
    desk = 0;
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
    if (desk == fp->m->virtual_scr.CurrentDesk || all_desks)
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

static void SetDeskLabel(struct fpmonitor *m, int desk, const char *label)
{
  PagerStringList *item;

  if (fAlwaysCurrentDesk)
  {
    item = FindDeskStrings(desk);
    if (item->next != NULL)
    {
      free(item->next->label);
      item->next->label = NULL;
      CopyString(&(item->next->label), label);
    }
    else
    {
      /* new Dcolor and desktop */
      item = NewPagerStringItem(item, desk);
      CopyString(&(item->label), label);
    }
    if (desk == m->m->virtual_scr.CurrentDesk)
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

void parse_monitor_line(char *tline)
{
	int		  dx, dy, Vx, Vy, VxMax, VyMax, CurrentDesk;
	int		  scr_width, scr_height;
	int		  flags;
	char		 *mname;
	struct monitor	 *tm;
	struct fpmonitor *fp;

	tline = GetNextToken(tline, &mname);

	sscanf(tline, "%d %d %d %d %d %d %d %d %d %d", &flags,
	    &dx, &dy, &Vx, &Vy, &VxMax, &VyMax, &CurrentDesk,
	    &scr_width, &scr_height);

	monitor_refresh_module(dpy);

	tm = monitor_resolve_name(mname);
	if (tm == NULL)
		return;

	if (flags & MONITOR_DISABLED) {
		fp = fpmonitor_by_name(mname);
		if (fp != NULL) {
			bool disabled = fp->disabled;
			fpmonitor_disable(fp);
			if (!disabled)
				ReConfigure();
		}
		return;
	}

	if ((fp = fpmonitor_this(tm)) == NULL) {
		if (fp_new_block)
			return;
		fp_new_block = true;
		fp = fpmonitor_new(tm);
		fp->m->flags |= MONITOR_NEW;
	}

	/* This ensures that if monitor_to_track gets disconnected
	 * then reconnected, the pager can resume tracking it.
	 */
	if (preferred_monitor != NULL &&
		strcmp(fp->m->si->name, preferred_monitor) == 0 &&
		strcmp(preferred_monitor, monitor_to_track) != 0)
	{
		if (monitor_to_track != NULL)
			free(monitor_to_track);
		monitor_to_track = fxstrdup(preferred_monitor);
	}
	fp->disabled = false;
	fp->scr_width = scr_width;
	fp->scr_height = scr_height;
	fp->m->dx = dx;
	fp->m->dy = dy;
	fp->m->virtual_scr.Vx = fp->virtual_scr.Vx = Vx;
	fp->m->virtual_scr.Vy = fp->virtual_scr.Vy = Vy;
	fp->m->virtual_scr.VxMax = fp->virtual_scr.VxMax = VxMax;
	fp->m->virtual_scr.VyMax = fp->virtual_scr.VyMax = VyMax;
	fp->m->virtual_scr.CurrentDesk = CurrentDesk;

	fp->virtual_scr.VxMax = dx * fpmonitor_get_all_widths() - fpmonitor_get_all_widths();
	fp->virtual_scr.VyMax = dy * fpmonitor_get_all_heights() - fpmonitor_get_all_heights();
	if (fp->virtual_scr.VxMax < 0)
		fp->virtual_scr.VxMax = 0;
	if (fp->virtual_scr.VyMax < 0)
		fp->virtual_scr.VyMax = 0;
	fp->virtual_scr.VWidth = fp->virtual_scr.VxMax + fpmonitor_get_all_widths();
	fp->virtual_scr.VHeight = fp->virtual_scr.VyMax + fpmonitor_get_all_heights();
	fp->virtual_scr.VxPages = fp->virtual_scr.VWidth / fpmonitor_get_all_widths();
	fp->virtual_scr.VyPages = fp->virtual_scr.VHeight / fpmonitor_get_all_heights();

	if (fp->m != NULL && fp->m->flags & MONITOR_NEW) {
		fp->m->flags &= ~MONITOR_NEW;
		initialize_fpmonitor_windows(fp);
	}
	fp_new_block = false;
}

void parse_desktop_size_line(char *tline)
{
	int dx, dy;
	struct fpmonitor *m;

	sscanf(tline, "%d %d", &dx, &dy);

	TAILQ_FOREACH(m, &fp_monitor_q, entry) {
		m->virtual_scr.VxMax = dx * fpmonitor_get_all_widths() - fpmonitor_get_all_widths();
		m->virtual_scr.VyMax = dy * fpmonitor_get_all_heights() - fpmonitor_get_all_heights();
		if (m->virtual_scr.VxMax < 0)
			m->virtual_scr.VxMax = 0;
		if (m->virtual_scr.VyMax < 0)
			m->virtual_scr.VyMax = 0;
		m->virtual_scr.VWidth = m->virtual_scr.VxMax + fpmonitor_get_all_widths();
		m->virtual_scr.VHeight = m->virtual_scr.VyMax + fpmonitor_get_all_heights();
		m->virtual_scr.VxPages = m->virtual_scr.VWidth / fpmonitor_get_all_widths();
		m->virtual_scr.VyPages = m->virtual_scr.VHeight / fpmonitor_get_all_heights();
	}
}

void parse_desktop_configuration_line(char *tline)
{
		int mmode, is_shared;

		sscanf(tline, "%d %d", &mmode, &is_shared);

		if (mmode > 0) {
			fp_monitor_mode = mmode;
			fp_is_tracking_shared = is_shared;
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
  char *mname;
  int desk;
  FvwmPictureAttributes fpa;
  Scr.FvwmRoot = NULL;
  Scr.Hilite = NULL;
  Scr.VScale = 32;

  fpa.mask = 0;
  if (Pdepth <= 8)
  {
	  fpa.mask |= FPAM_DITHER;
  }

  xasprintf(&mname, "*%s", MyName);
  InitGetConfigLine(fd, mname);
  free(mname);

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
    bool MoveThresholdSetForModule = false;
    struct fpmonitor	*m = NULL;

    arg1 = arg2 = NULL;

    token = PeekToken(tline, &next);

    /* Step 1: Initial configuration broadcasts are parsed here.
     * This needs to match list_config_info(), along with having
     * a few extra broadcasts that are only sent during initialization.
     */
    if (token[0] == '*') {
        /* This is a module configuration item, skip to next step. */
    }
    else if (StrEquals(token, "Colorset"))
    {
      LoadColorset(next);
      continue;
    }
    else if (StrEquals(token, "DesktopSize"))
    {
      parse_desktop_size_line(next);
      continue;
    }
    else if (StrEquals(token, "ImagePath"))
    {
      free(ImagePath);
ImagePath = NULL;
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
	      TAILQ_FOREACH(m, &fp_monitor_q, entry) {
		      SetDeskLabel(m, val, (const char *)next);
	      }
      }
      continue;
    }
    else if (StrEquals(token, "Monitor"))
    {
	parse_monitor_line(next);
	continue;
    }
    else if (StrEquals(token, "DesktopConfiguration"))
    {
	parse_desktop_configuration_line(next);
	continue;
    }
    else
    {
      /* Module configuration lines have skipped this if block. */
      continue;
    }

    /* Step 2: Parse module configuration options. */
    tline2 = GetModuleResource(tline, &resource, MyName);
    if (!resource)
      continue;
    tline2 = GetNextToken(tline2, &arg1);
    if (!arg1)
    {
      arg1 = fxmalloc(1);
      arg1[0] = 0;
    }
    tline2 = GetNextToken(tline2, &arg2);
    if (!arg2)
    {
      arg2 = fxmalloc(1);
      arg2[0] = 0;
    }

    if (StrEquals(resource, "Monitor")) {
	    struct monitor *tm = monitor_get_current();

	    free(monitor_to_track);
	    next = SkipSpaces(next, NULL, 0);

	    if (next == NULL) {
		    fvwm_debug(__func__, "FvwmPager: no monitor name given "
				    "using current monitor\n");
		    /* m already set... */
		    monitor_to_track = fxstrdup(tm->si->name);
		    continue;
	    }
	    if ((m = fpmonitor_by_name(next)) == NULL) {
		    fvwm_debug(__func__, "FvwmPager: monitor '%s' not found "
                               "using current monitor", next);
		    monitor_to_track = fxstrdup(tm->si->name);
		    continue;
	    }
	    fvwm_debug(__func__, "Assigning monitor: %s\n", m->m->si->name);
	    monitor_to_track = fxstrdup(m->m->si->name);
	    if (preferred_monitor != NULL)
		free(preferred_monitor);
	    preferred_monitor = fxstrdup(monitor_to_track);
    } else if(StrEquals(resource, "DeskLabels")) {
	    use_desk_label = true;
    } else if(StrEquals(resource, "NoDeskLabels")) {
	    use_desk_label = false;
    } else if(StrEquals(resource, "MonitorLabels")) {
	    use_monitor_label = true;
    } else if(StrEquals(resource, "NoMonitorLabels")) {
	    use_monitor_label = false;
    }
    else if(StrEquals(resource,"Colorset"))
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
      flags = FScreenParseGeometry(arg1,&g_x,&g_y,&width,&height);
      if (flags & WidthValue)
      {
	pwindow.width = width;
      }
      if (flags & HeightValue)
      {
	pwindow.height = height;
      }
      if (flags & XValue)
      {
	pwindow.x = g_x;
	usposition = true;
	if (flags & XNegative)
	{
	  xneg = true;
	}
      }
      if (flags & YValue)
      {
	pwindow.y = g_y;
	usposition = true;
	if (flags & YNegative)
	{
	  yneg = true;
	}
      }
    }
    else if (StrEquals(resource, "IconGeometry"))
    {
      flags = FScreenParseGeometry(arg1,&g_x,&g_y,&width,&height);
      if (flags & WidthValue)
	icon.width = width;
      if (flags & HeightValue)
	icon.height = height;
      if (flags & XValue)
      {
	icon.x = g_x;
	if (flags & XNegative)
	{
	  icon_xneg = true;
	}
      }
      if (flags & YValue)
      {
	icon.y = g_y;
	if (flags & YNegative)
	{
	  icon_yneg = true;
	}
      }
    }
    else if (StrEquals(resource, "Font"))
    {
      free(font_string);
      CopyStringWithQuotes(&font_string, next);
      if(strncasecmp(font_string,"none",4) == 0)
      {
	use_desk_label = false;
	use_monitor_label = false;
	free(font_string);
	font_string = NULL;
      }
    }
    else if (StrEquals(resource, "Fore"))
    {
      if(Pdepth > 1)
      {
	free(PagerFore);
	CopyString(&PagerFore,arg1);
      }
    }
    else if (StrEquals(resource, "Back"))
    {
      if(Pdepth > 1)
      {
	free(PagerBack);
	CopyString(&PagerBack,arg1);
      }
    }
    else if (StrEquals(resource, "DeskColor"))
    {
      if (StrEquals(arg1, "*"))
      {
	desk = 0;
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
	  free(item->next->Dcolor);
	  item->next->Dcolor = NULL;
	  CopyString(&(item->next->Dcolor), arg2);
	}
	else
	{
	  /* new Dcolor and desktop */
	  item = NewPagerStringItem(item, desk);
	  CopyString(&(item->Dcolor), arg2);
	}
	struct fpmonitor *fp = fpmonitor_this(NULL);

	if (desk == fp->m->virtual_scr.CurrentDesk)
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
	desk = 0;
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
	struct fpmonitor *fp = fpmonitor_this(NULL);
	if (desk == fp->m->virtual_scr.CurrentDesk)
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
      HilightDesks = true;
    }
    else if (StrEquals(resource, "NoDeskHilight"))
    {
      HilightDesks = false;
    }
    else if (StrEquals(resource, "Hilight"))
    {
      if(Pdepth > 1)
      {
	free(HilightC);
	CopyString(&HilightC,arg1);
      }
    }
    else if (StrEquals(resource, "SmallFont"))
    {
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
      MiniIcons = true;
    }
    else if (StrEquals(resource, "StartIconic"))
    {
      StartIconic = true;
    }
    else if (StrEquals(resource, "NoStartIconic"))
    {
      StartIconic = false;
    }
    else if (StrEquals(resource, "LabelsBelow"))
    {
      LabelsBelow = true;
    }
    else if (StrEquals(resource, "LabelsAbove"))
    {
      LabelsBelow = false;
    }
    else if (FHaveShapeExtension && StrEquals(resource, "ShapeLabels"))
    {
      ShapeLabels = true;
    }
    else if (FHaveShapeExtension && StrEquals(resource, "NoShapeLabels"))
    {
      ShapeLabels = false;
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
	free(WindowFore);
	free(WindowBack);
	free(WindowHiFore);
	free(WindowHiBack);
	CopyString(&WindowFore, arg1);
	CopyString(&WindowBack, arg2);
	tline2 = GetNextToken(tline2, &WindowHiFore);
	GetNextToken(tline2, &WindowHiBack);
      }
    }
    else if (StrEquals(resource, "WindowBorderWidth"))
    {
      MinSize = MinSize - 2*WindowBorderWidth;
      sscanf(arg1, "%d", &WindowBorderWidth);
      if (WindowBorderWidth > 0)
        MinSize = 2 * WindowBorderWidth + MinSize;
      else
        MinSize = 2 * DEFAULT_PAGER_WINDOW_BORDER_WIDTH + MinSize;
    }
    else if (StrEquals(resource, "WindowMinSize"))
    {
      sscanf(arg1, "%d", &MinSize);
      if (MinSize > 0)
        MinSize = 2 * WindowBorderWidth + MinSize;
      else
        MinSize = 2 * WindowBorderWidth + DEFAULT_PAGER_WINDOW_MIN_SIZE;
    }
    else if (StrEquals(resource, "HideSmallWindows"))
    {
      HideSmallWindows = true;
    }
    else if (StrEquals(resource, "Window3dBorders"))
    {
      WindowBorders3d = true;
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
      free(WindowLabelFormat);
      CopyString(&WindowLabelFormat,arg1);
    }
    else if (StrEquals(resource,"UseSkipList"))
    {
      UseSkipList = true;
    }
    else if (StrEquals(resource, "MoveThreshold"))
    {
      int val;
      if (GetIntegerArguments(next, NULL, &val, 1) > 0 && val >= 0)
      {
	MoveThreshold = val;
	MoveThresholdSetForModule = true;
      }
    }
    else if (StrEquals(resource, "SloppyFocus"))
    {
      do_focus_on_enter = true;
    }
    else if (StrEquals(resource, "SolidSeparators"))
    {
      use_dashed_separators = false;
      use_no_separators = false;
    }
    else if (StrEquals(resource, "NoSeparators"))
    {
      use_no_separators = true;
    }
    /* ... and get Balloon config options ...
       -- ric@giccs.georgetown.edu */
    else if (StrEquals(resource, "Balloons"))
    {
      free(BalloonTypeString);
      CopyString(&BalloonTypeString, arg1);

      if ( strncasecmp(BalloonTypeString, "Pager", 5) == 0 ) {
	ShowPagerBalloons = true;
	ShowIconBalloons = false;
      }
      else if ( strncasecmp(BalloonTypeString, "Icon", 4) == 0 ) {
	ShowPagerBalloons = false;
	ShowIconBalloons = true;
      }
      else {
	ShowPagerBalloons = true;
	ShowIconBalloons = true;
      }

      /* turn this on initially so balloon window is created; later this
	 variable is changed to match ShowPagerBalloons or ShowIconBalloons
	 whenever we receive iconify or deiconify packets */
      ShowBalloons = true;
    }

    else if (StrEquals(resource, "BalloonBack"))
    {
      if (Pdepth > 1)
      {
	free(BalloonBack);
	CopyString(&BalloonBack, arg1);
      }
    }

    else if (StrEquals(resource, "BalloonFore"))
    {
      if (Pdepth > 1)
      {
	free(BalloonFore);
	CopyString(&BalloonFore, arg1);
      }
    }

    else if (StrEquals(resource, "BalloonFont"))
    {
      free(BalloonFont);
      CopyStringWithQuotes(&BalloonFont, next);
    }

    else if (StrEquals(resource, "BalloonBorderColor"))
    {
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
	fvwm_debug(__func__,
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
      free(BalloonFormatString);
      CopyString(&BalloonFormatString,arg1);
    }

    free(resource);
    free(arg1);
    free(arg2);
  }

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

  newitem = fxcalloc(1, sizeof(PagerStringList));
  last->next = newitem;
  newitem->colorset = -1;
  newitem->highcolorset = -1;
  newitem->ballooncolorset = -1;
  newitem->desk = desk;

  return newitem;
}

void ExitPager(void)
{
  struct fpmonitor *fp, *fp1;

  TAILQ_FOREACH_SAFE(fp, &fp_monitor_q, entry, fp1) {
	TAILQ_REMOVE(&fp_monitor_q, fp, entry);
	free(fp->CPagerWin);
	free(fp);
  }

  if (is_transient)
  {
    XUngrabPointer(dpy,CurrentTime);
    MyXUngrabServer(dpy);
    XSync(dpy,0);
  }
  XUngrabKeyboard(dpy, CurrentTime);
  free(monitor_to_track);
  free(preferred_monitor);
  exit(0);
}
