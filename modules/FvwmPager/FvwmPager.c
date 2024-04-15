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
#include "libs/safemalloc.h"
#include <ctype.h>

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif /* Saul */

#include "libs/Module.h"
#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/FRenderInit.h"
#include "libs/FEvent.h"
#include "libs/Colorset.h"
#include "libs/Flocale.h"
#include "libs/fvwmsignal.h"
#include "libs/Grab.h"
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
char		*smallFont = NULL;
char		*ImagePath = NULL;
char		*font_string = NULL;
char		*BalloonFont = NULL;
char		*WindowLabelFormat = NULL;
Pixmap		default_pixmap = None;
FlocaleFont	*Ffont;
FlocaleFont	*FwindowFont;
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
bool	xneg = false;
bool	yneg = false;
bool	IsShared = false;
bool	icon_xneg = false;
bool	icon_yneg = false;
bool	MiniIcons = false;
bool	Swallowed = false;
bool	usposition = false;
bool	UseSkipList = false;
bool	StartIconic = false;
bool	LabelsBelow = false;
bool	ShapeLabels = false;
bool	is_transient = false;
bool	HilightDesks = true;
bool	HilightLabels = true;
bool	error_occured = false;
bool	FocusAfterMove = false;
bool	use_desk_label = true;
bool	WindowBorders3d = false;
bool	HideSmallWindows = false;
bool	use_no_separators = false;
bool	use_monitor_label = false;
bool	do_focus_on_enter = false;
bool	fAlwaysCurrentDesk = false;
bool	CurrentDeskPerMonitor = false;
bool	use_dashed_separators = true;
bool	do_ignore_next_button_release = false;

/* Screen / Windows */
int			fd[2];
char			*MyName;
Display			*dpy;
DeskInfo		*Desks;
ScreenInfo		Scr;
PagerWindow		*Start = NULL;
PagerWindow		*FocusWin = NULL;
struct balloon_data	Balloon;
struct desk_styles	desk_style_q;

/* Monitors */
struct fpmonitor	*current_monitor = NULL;
struct fpmonitor	*monitor_to_track = NULL;
char			*preferred_monitor = NULL;
struct fpmonitors	fp_monitor_q;

static int x_fd;
static fd_set_size_t fd_width;
static bool fp_new_block = false;

static void SetDeskLabel(int desk, const char *label);
static RETSIGTYPE TerminateHandler(int);
static void list_monitor_focus(unsigned long *);
static struct fpmonitor *fpmonitor_new(struct monitor *);
static void fpmonitor_disable(struct fpmonitor *);
static void parse_monitor_line(char *);
static void parse_desktop_size_line(char *);
static void parse_desktop_configuration_line(char *);
static PagerWindow *find_pager_window(Window target_w);

struct fpmonitor *
fpmonitor_new(struct monitor *m)
{
	struct fpmonitor	*fp;

	fp = fxcalloc(1, sizeof(*fp));
	if (HilightDesks)
		fp->CPagerWin = fxcalloc(1, ndesks * sizeof(*fp->CPagerWin));
	else
		fp->CPagerWin = NULL;
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

	if (fp == monitor_to_track)
		monitor_to_track = fpmonitor_this(NULL);

}

struct fpmonitor *
fpmonitor_by_name(const char *name)
{
	struct fpmonitor *fm;

	if (name == NULL || StrEquals(name, "none"))
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
	struct monitor *m;
	struct fpmonitor *fp = NULL;

	if (m_find != NULL) {
		/* We've been asked to find a specific monitor. */
		m = m_find;
	} else if (monitor_to_track != NULL) {
		return (monitor_to_track);
	} else {
		m = monitor_get_current();
	}

	if (m == NULL || m->flags & MONITOR_DISABLED)
		return (NULL);
	TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
		if (fp->m == m)
			return (fp);
	}
	return (NULL);

}

void initialize_colorsets(void)
{
	DeskStyle *style;

	TAILQ_FOREACH(style, &desk_style_q, entry) {
		if (style->cs >= 0) {
			style->fg = Colorset[style->cs].fg;
			style->bg = Colorset[style->cs].bg;
		}
		if (style->hi_cs >= 0) {
			style->hi_fg = Colorset[style->hi_cs].fg;
			style->hi_bg = Colorset[style->hi_cs].bg;
		}
	}
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
  TAILQ_INIT(&desk_style_q);

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
      fAlwaysCurrentDesk = true;
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

  /* Initialize default DeskStyle, stored on desk -1. */
  {
	DeskStyle *default_style;

	default_style = fxcalloc(1, sizeof(DeskStyle));
	default_style->desk = -1;
	default_style->label = fxstrdup("-");

	default_style->use_label_pixmap = true;
	default_style->cs = -1;
	default_style->hi_cs = -1;
	default_style->win_cs = -1;
	default_style->focus_cs = -1;
	default_style->balloon_cs = -1;
	default_style->fg = GetSimpleColor("black");
	default_style->bg = GetSimpleColor("white");
	default_style->hi_fg = GetSimpleColor("black");
	default_style->hi_bg = GetSimpleColor("grey");
	default_style->win_fg = None; /* Use fvwm pixel unless defined. */
	default_style->win_bg = None;
	default_style->focus_fg = None;
	default_style->focus_bg = None;
	default_style->balloon_fg = None;
	default_style->balloon_bg = None;
	default_style->balloon_border = None;
	default_style->bgPixmap = NULL;
	default_style->hiPixmap = NULL;
	TAILQ_INSERT_TAIL(&desk_style_q, default_style, entry);
  }

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

  Desks = fxcalloc(1, ndesks*sizeof(DeskInfo));
  for(i = 0; i < ndesks; i++)
  {
	Desks[i].style = FindDeskStyle(i);
	Desks[i].fp = fpmonitor_from_desk(i + desk1);
  }

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

  if (WindowLabelFormat == NULL)
    WindowLabelFormat = fxstrdup("%i");

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo(fd,"Send_WindowList",0);

  /* open a pager window */
  initialize_colorsets();
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

      if(XGetGeometry(dpy,Scr.pager_w,&root,&x,&y,
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
 *  Procedure:
 *      Process message - examines packet types, and takes appropriate action
 */
void process_message(FvwmPacket* packet)
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
			list_window_name(body, type);
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
			list_restack(body, length);
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
 *  Procedure:
 *      SIGPIPE handler - SIGPIPE means fvwm is dying
 */
static RETSIGTYPE TerminateHandler(int sig)
{
	fvwmSetTerminate(sig);
	SIGNAL_RETURN;
}

RETSIGTYPE DeadPipe(int nonsense)
{
	exit(0);
	SIGNAL_RETURN;
}

/* Convince method to find target window. */
PagerWindow *find_pager_window(Window target_w)
{
	PagerWindow *t;

	t = Start;
	while(t != NULL && t->w != target_w)
		t = t->next;

	return t;
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

	/* These are used for windows if no pixel is set. */
	t->text = cfgpacket->TextPixel;
	t->back = cfgpacket->BackPixel;

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

void list_add(unsigned long *body)
{
	PagerWindow *t, **prev;
	struct ConfigWinPacket  *cfgpacket = (void *)body;

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

void list_configure(unsigned long *body)
{
	PagerWindow *t;
	struct ConfigWinPacket  *cfgpacket = (void *)body;
	bool is_new_desk;

	t = find_pager_window(cfgpacket->w);
	if (t == NULL)
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

void list_destroy(unsigned long *body)
{
	PagerWindow *t, **prev;
	Window target_w;

	target_w = body[0];
	t = Start;
	prev = &Start;
	while(t != NULL && t->w != target_w)
	{
		prev = &(t->next);
		t = t->next;
	}

	if(t != NULL)
	{
		if(prev != NULL)
			*prev = t->next;
			/* remove window from the chain */

		XDestroyWindow(dpy, t->PagerView);
		XDestroyWindow(dpy, t->IconView);
		if(FocusWin == t)
			FocusWin = NULL;

		free(t->res_class);
		free(t->res_name);
		free(t->window_name);
		free(t->icon_name);
		free(t->window_label);
		free(t);
	}
}

void list_monitor_focus(unsigned long *body)
{
	return;
}

void list_focus(unsigned long *body)
{
	PagerWindow *t = find_pager_window(body[0]);
	bool do_force_update = false;

	/* Update Pixels data. */
	if (Scr.focus_win_fg != body[3] || Scr.focus_win_bg != body[4])
		do_force_update = true;
	Scr.focus_win_fg = body[3];
	Scr.focus_win_bg = body[4];

	if (do_force_update || t != FocusWin) {
		PagerWindow *focus = FocusWin;
		FocusWin = t;

		update_window_background(focus);
		update_window_background(t);
	}
}

void list_new_page(unsigned long *body)
{
	bool do_reconfigure = false;
	int mon_num = body[7];
	struct monitor *m;
	struct fpmonitor *fp;

	m = monitor_by_output(mon_num);
	/* Don't allow monitor_by_output to fallback to RB_MIN. */
	if (m->si->rr_output != mon_num)
		return;

	/* Still need to update the monitors when tracking a single
	 * monitor, but don't need to reconfigure anything.
	 */
	if (monitor_to_track != NULL && m == monitor_to_track->m) {
		fp = monitor_to_track;
		do_reconfigure = true;
	} else {
		fp = fpmonitor_this(m);
		if (fp == NULL)
			return;
		do_reconfigure = true;
	}

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
		if (do_reconfigure)
			ReConfigure();
	}

	if (do_reconfigure) {
		MovePage();
		MoveStickyWindows(true, false);
		update_window_background(FocusWin);
	}
}

void list_new_desk(unsigned long *body)
{
	int oldDesk, newDesk;
	int mon_num = body[1];
	struct monitor *mout;
	struct fpmonitor *fp;
	DeskStyle *style;

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
	newDesk = fp->m->virtual_scr.CurrentDesk = (long)body[0];
	if (newDesk >= desk1 && newDesk <= desk2)
		Desks[newDesk - desk1].fp = fp;

	/* Only update FvwmPager's desk for the monitors being tracked.
	 * Exceptions:
	 *   + If CurrentDeskPerMonitor is set, always update desk.
	 *   + If current_monitor is set, only update if that monitor changes
	 *     and tracking is per-monitor or shared.
	 */
	if (!CurrentDeskPerMonitor && ((monitor_to_track != NULL &&
	    mout != monitor_to_track->m) ||
	    (current_monitor != NULL &&
	    monitor_to_track == NULL &&
	    mout != current_monitor->m &&
	    (monitor_mode == MONITOR_TRACKING_M ||
	    is_tracking_shared))))
		return;

	/* Update the current desk. */
	desk_i = newDesk;
	style = FindDeskStyle(newDesk);
	/* Create and update GCs if needed. */
	if (oldDesk != newDesk) {
		if (style->label_gc)
			update_desk_style_gcs(style);
		else
			initialize_desk_style_gcs(style);
	}

	/* Keep monitors in sync when tracking is global. */
	if (monitor_mode == MONITOR_TRACKING_G)
		monitor_assign_virtual(fp->m);

	/* If always tracking current desk. Update Desks[0]. */
	if (fAlwaysCurrentDesk && oldDesk != newDesk)
	{
		PagerWindow *t;

		desk1 = desk2 = newDesk;
		for (t = Start; t != NULL; t = t->next)
		{
			if (t->desk == oldDesk || t->desk == newDesk)
				ChangeDeskForWindow(t, t->desk);
		}

		/* Update DeskStyle */
		Desks[0].style = style;
		update_desk_background(0);
		update_monitor_locations(0);
		update_monitor_backgrounds(0);
		ReConfigureAll();
	}

	XStoreName(dpy, Scr.pager_w, style->label);
	XSetIconName(dpy, Scr.pager_w, style->label);
	MovePage();
	draw_desk_grid(oldDesk - desk1);
	draw_desk_grid(newDesk - desk1);
	MoveStickyWindows(false, true);
	update_window_background(FocusWin);
}

void list_raise(unsigned long *body)
{
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

	XRaiseWindow(dpy, t->PagerView);
	XRaiseWindow(dpy, t->IconView);
}

void list_lower(unsigned long *body)
{
	struct fpmonitor *fp;
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

	XLowerWindow(dpy, t->PagerView);
	if (HilightDesks && t->desk >= desk1 && t->desk <= desk2) {
		TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
			XLowerWindow(dpy, fp->CPagerWin[t->desk - desk1]);
		}
	}
	XLowerWindow(dpy, t->IconView);
}

void list_iconify(unsigned long *body)
{
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

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
	if (t->w == Scr.pager_w)
		Balloon.show = Balloon.show_in_icon;
	MoveResizePagerView(t, true);
}

void list_deiconify(unsigned long *body, unsigned long length)
{
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

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
	if (t->w == Scr.pager_w)
		Balloon.show = Balloon.show_in_pager;

	MoveResizePagerView(t, true);
}

void list_window_name(unsigned long *body, unsigned long type)
{
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

	switch (type) {
		case M_RES_CLASS:
			free(t->res_class);
			CopyString(&t->res_class, (char *)(&body[3]));
			break;
		case M_RES_NAME:
			free(t->res_name);
			CopyString(&t->res_name, (char *)(&body[3]));
			break;
		case M_VISIBLE_NAME:
			free(t->window_name);
			CopyString(&t->window_name, (char *)(&body[3]));
			break;
	}

	/* repaint by clearing window */
	if (FwindowFont != NULL && t->icon_name != NULL &&
	    !(MiniIcons && t->mini_icon.picture))
	{
		XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
		XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
	}
	if (Balloon.is_mapped)
	{
		/* update balloons */
		UnmapBalloonWindow();
		MapBalloonWindow(t, Balloon.is_icon);
	}
}

void list_icon_name(unsigned long *body)
{
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

	free(t->icon_name);
	CopyString(&t->icon_name, (char *)(&body[3]));
	/* repaint by clearing window */
	if (FwindowFont != NULL && t->icon_name != NULL &&
	    !(MiniIcons && t->mini_icon.picture))
	{
		XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
		XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
	}
}

void list_mini_icon(unsigned long *body)
{
	MiniIconPacket *mip = (MiniIconPacket *) body;
	PagerWindow *t = find_pager_window(mip->w);

	if (t == NULL)
		return;

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

void list_restack(unsigned long *body, unsigned long length)
{
	PagerWindow *t;
	PagerWindow *pager_wins[length];
	Window *wins;
	int i, j, desk;

	wins = fxmalloc(length * sizeof (Window));

	/* Should figure out how to do this with a single loop of the list,
	 * not one per desk + icon window.
	 */

	/* first restack in the icon view */
	j = 0;
	for (i = 0; i < (length - FvwmPacketHeaderSize); i += 3)
	{
		t = find_pager_window(body[i]);
		if (t != NULL) {
			pager_wins[j] = t;
			wins[j++] = t->IconView;
		}
	}
	XRestackWindows(dpy, wins, j);
	length = j;

	/* now restack each desk separately, since they have separate roots */
	for (desk = 0; desk < ndesks; desk++)
	{
		j = 0;
		for (i = 0; i < length; i++)
		{
			t = pager_wins[i];
			if (t != NULL && t->desk == desk + desk1)
				wins[j++] = t->PagerView;
		}
		XRestackWindows(dpy, wins, j);
	}

	free(wins);
}

void list_end(void)
{
	unsigned int nchildren, i;
	Window root, parent, *children;
	PagerWindow *ptr;

	if (!XQueryTree(dpy, Scr.Root, &root, &parent, &children, &nchildren))
		return;

	for(i = 0; i < nchildren; i++)
	{
		ptr = Start;
		while(ptr != NULL)
		{
			if (ptr->frame == children[i] ||
			    ptr->icon_w == children[i] ||
			    ptr->icon_pixmap_w == children[i])
			{
				XRaiseWindow(dpy, ptr->PagerView);
				XRaiseWindow(dpy, ptr->IconView);
			}
			ptr = ptr->next;
		}
	}

	if (nchildren > 0)
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
		DeskStyle *style;

		color = LoadColorset(tline);
		TAILQ_FOREACH(style, &desk_style_q, entry) {
			if (style->cs == color || style->hi_cs == color)
			{
				update_desk_style_gcs(style);
				if (style->desk < desk1 || style->desk > desk2)
					continue;

				update_desk_background(style->desk - desk1);
				update_monitor_backgrounds(style->desk - desk1);
			}
			if (style->win_cs == color || style->focus_cs == color) {
				PagerWindow *t = Start;

				update_desk_style_gcs(style);
				while (t != NULL) {
					if (t->desk != style->desk) {
						t = t->next;
						continue;
					}
					update_desk_style_gcs(style);
					update_window_background(t);
					update_window_decor(t);
					t = t->next;
				}
			}
		}
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
			TAILQ_FOREACH(m, &fp_monitor_q, entry) {
				if (m->m->virtual_scr.CurrentDesk == val)
					val = 0;
			}
		}
		else if ((val >= desk1) && (val <=desk2))
		{
			val = val - desk1;
		}
		draw_desk_grid(val);
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
		if (((!Swallowed && body[2] == 0) ||
		    (Swallowed && body[2] == Scr.pager_w)))
			update_pr_transparent_windows();
	}
	else if  (body[0] == MX_PROPERTY_CHANGE_SWALLOW &&
		  body[2] == Scr.pager_w)
	{
		Swallowed = body[1];
	}
}


void list_reply(unsigned long *body)
{
	char *tline;
	tline = (char*)&(body[3]);

	if (strcmp(tline, "ScrollDone") == 0)
		HandleScrollDone();
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



/* This function is really tricky. An offset of the colorset members
 * in the DeskStyle strut is used to set the desired colorset.
 * The lines accessing this info look very ugly, but they work.
 */
static void SetDeskStyleColorset(char *arg1, char *arg2, void *offset_style)
{
  int colorset = 0;
  int desk;
  unsigned long offset = (unsigned long)offset_style;
  DeskStyle *style;

  sscanf(arg2, "%d", &colorset);
  AllocColorset(colorset);
  if (arg1[0] == '*') {
	  TAILQ_FOREACH(style, &desk_style_q, entry) {
		  *(int *)(((char *)style) + offset) = colorset;
	  }
  } else {
	  desk = desk1;
	  sscanf(arg1, "%d", &desk);

	  style = FindDeskStyle(desk);
	  *(int *)(((char *)style) + offset) = colorset;
  }

  return;
}

static void SetDeskStylePixel(char *arg1, char *arg2, void *offset_style)
{
  int desk;
  unsigned long offset = (unsigned long)offset_style;
  DeskStyle *style;
  Pixel pix;

  pix = GetSimpleColor(arg2);
  if (arg1[0] == '*') {
	  TAILQ_FOREACH(style, &desk_style_q, entry) {
		  *(unsigned long *)(((char *)style) + offset) = pix;
	  }
  } else {
	  desk = desk1;
	  sscanf(arg1, "%d", &desk);

	  style = FindDeskStyle(desk);
	  *(unsigned long *)(((char *)style) + offset) = pix;
  }

  return;
}

static void SetDeskStyleBool(char *arg1, char *arg2, void *offset_style)
{
  int desk;
  unsigned long offset = (unsigned long)offset_style;
  DeskStyle *style;
  bool val;

  /* We are a bit greedy here, but it is okay. */
  if (arg2[0] == 't' || arg2[0] == 'T' || arg2[0] == '1') {
	  val = true;
  } else if (arg2[0] == 'f' || arg2[0] == 'F' || arg2[0] == '0') {
	  val = false;
  } else {
	  /* But not too greedy. */
	  return;
  }



  if (arg1[0] == '*') {
	  TAILQ_FOREACH(style, &desk_style_q, entry) {
		  *(bool *)(((char *)style) + offset) = val;
	  }
  } else {
	  desk = desk1;
	  sscanf(arg1, "%d", &desk);

	  style = FindDeskStyle(desk);
	  *(bool *)(((char *)style) + offset) = val;
  }

  return;
}

static void SetDeskLabel(int desk, const char *label)
{
	DeskStyle *style = FindDeskStyle(desk);

	free(style->label);
	CopyString(&(style->label), label);
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
	    strcmp(fp->m->si->name, preferred_monitor) == 0)
	{
		monitor_to_track = fp;
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
			monitor_mode = mmode;
			is_tracking_shared = is_shared;
		}
}

/*
 * This routine is responsible for reading and parsing the config file
 */
void ParseOptions(void)
{
	char *tline= NULL;
	char *mname;
	bool MoveThresholdSetForModule = false;

	FvwmPictureAttributes fpa;
	Scr.VScale = 32;

	fpa.mask = 0;
	if (Pdepth <= 8)
	{
		fpa.mask |= FPAM_DITHER;
	}

	xasprintf(&mname, "*%s", MyName);
	InitGetConfigLine(fd, mname);
	free(mname);

	for (GetConfigLine(fd, &tline); tline != NULL;
	     GetConfigLine(fd, &tline))
	{
		int g_x, g_y, flags;
		unsigned width, height;
		char *resource;
		char *arg1 = NULL;
		char *arg2 = NULL;
		char *tline2;
		char *token;
		char *next;

		token = PeekToken(tline, &next);

		/* Step 1: Initial configuration broadcasts are parsed here.
		 * This needs to match list_config_info(), along with having
		 * a few extra broadcasts that are sent during initialization.
		 */
		if (token[0] == '*') {
			/* Module configuration item, skip to next step. */
		} else if (StrEquals(token, "Colorset")) {
			LoadColorset(next);
			continue;
		} else if (StrEquals(token, "DesktopSize")) {
			parse_desktop_size_line(next);
			continue;
		} else if (StrEquals(token, "ImagePath")) {
			free(ImagePath);
			ImagePath = NULL;
			GetNextToken(next, &ImagePath);
			continue;
		} else if (StrEquals(token, "MoveThreshold")) {
			if (MoveThresholdSetForModule)
				continue;

			int val;
			if (GetIntegerArguments(next, NULL, &val, 1) > 0)
				MoveThreshold = (val >= 0) ? val :
						DEFAULT_PAGER_MOVE_THRESHOLD;
			continue;
		} else if (StrEquals(token, "DesktopName")) {
			int val;

			if (GetIntegerArguments(next, &next, &val, 1) > 0)
				SetDeskLabel(val, (const char *)next);
			continue;
		} else if (StrEquals(token, "Monitor")) {
			parse_monitor_line(next);
			continue;
		} else if (StrEquals(token, "DesktopConfiguration")) {
			parse_desktop_configuration_line(next);
			continue;
		} else {
			/* Module configuration lines have skipped this. */
			continue;
		}

		/* Step 2: Parse module configuration options. */
		tline2 = GetModuleResource(tline, &resource, MyName);
		if (!resource)
			continue;

		/* Start by looking for options with no parameters. */
		flags = 1;
		if (StrEquals(resource, "DeskLabels")) {
			use_desk_label = true;
		} else if (StrEquals(resource, "NoDeskLabels")) {
			use_desk_label = false;
		} else if (StrEquals(resource, "MonitorLabels")) {
			use_monitor_label = true;
		} else if (StrEquals(resource, "NoMonitorLabels")) {
			use_monitor_label = false;
		} else if (StrEquals(resource, "CurrentDeskPerMonitor")) {
			CurrentDeskPerMonitor = true;
		} else if (StrEquals(resource, "CurrentDeskGlobal")) {
			CurrentDeskPerMonitor = false;
		} else if (StrEquals(resource, "IsShared")) {
			IsShared = true;
		} else if (StrEquals(resource, "IsNotShared")) {
			IsShared = false;
		} else if (StrEquals(resource, "DeskHilight")) {
			HilightDesks = true;
		} else if (StrEquals(resource, "NoDeskHilight")) {
			HilightDesks = false;
		} else if (StrEquals(resource, "LabelHilight")) {
			HilightLabels = true;
		} else if (StrEquals(resource, "NoLabelHilight")) {
			HilightLabels = false;
		} else if (StrEquals(resource, "MiniIcons")) {
			MiniIcons = true;
		} else if (StrEquals(resource, "StartIconic")) {
			StartIconic = true;
		} else if (StrEquals(resource, "NoStartIconic")) {
			StartIconic = false;
		} else if (StrEquals(resource, "LabelsBelow")) {
			LabelsBelow = true;
		} else if (StrEquals(resource, "LabelsAbove")) {
			LabelsBelow = false;
		} else if (StrEquals(resource, "ShapeLabels")) {
			if (FHaveShapeExtension)
				ShapeLabels = true;
		} else if (StrEquals(resource, "NoShapeLabels")) {
			ShapeLabels = false;
		} else if (StrEquals(resource, "HideSmallWindows")) {
			HideSmallWindows = true;
		} else if (StrEquals(resource, "Window3dBorders")) {
			WindowBorders3d = true;
		} else if (StrEquals(resource,"UseSkipList")) {
			UseSkipList = true;
		} else if (StrEquals(resource, "SloppyFocus")) {
			do_focus_on_enter = true;
		} else if (StrEquals(resource, "FocusAfterMove")) {
			FocusAfterMove = true;
		} else if (StrEquals(resource, "SolidSeparators")) {
			use_dashed_separators = false;
			use_no_separators = false;
		} else if (StrEquals(resource, "NoSeparators")) {
			use_no_separators = true;
		} else {
			/* No Match, set this to continue parsing. */
			flags = 0;
		}
		if (flags == 1) {
			free(resource);
			continue;
		}

		/* Now look for options with additional inputs.
		 * Many inputs are of the form: [desk] value
		 * Since desk is optional, inputs are stored as:
		 *     One input:  arg1 = '*';     arg2 = input1
		 *     Two inputs: arg1 = input1;  arg2 = input2
		 * Options that expect one input should use "next".
		 */
		tline2 = GetNextToken(tline2, &arg1);
		if (!arg1)
			/* No inputs, nothing to do. */
			continue;

		tline2 = GetNextToken(tline2, &arg2);
		if (!arg2)
		{
			arg2 = arg1;
			arg1 = NULL;
			arg1 = fxmalloc(1);
			arg1[0] = '*';
		}

		next = SkipSpaces(next, NULL, 0);
		if (StrEquals(resource, "Monitor")) {
			free(preferred_monitor);
			if (StrEquals(next, "none")) {
				monitor_to_track = NULL;
				preferred_monitor = NULL;
				goto free_vars;
			}
			monitor_to_track = fpmonitor_by_name(next);
			/* Fallback to current monitor. */
			if (monitor_to_track == NULL)
				monitor_to_track = fpmonitor_this(NULL);
			preferred_monitor = fxstrdup(next);
		} else if (StrEquals(resource, "CurrentMonitor")) {
			current_monitor = (StrEquals(next, "none")) ?
				NULL : fpmonitor_by_name(next);
		} else if(StrEquals(resource, "Colorset")) {
			SetDeskStyleColorset(arg1, arg2,
				&(((DeskStyle *)(NULL))->cs));
		} else if(StrEquals(resource, "BalloonColorset")) {
			SetDeskStyleColorset(arg1, arg2,
				&(((DeskStyle *)(NULL))->balloon_cs));
		} else if(StrEquals(resource, "HilightColorset")) {
			SetDeskStyleColorset(arg1, arg2,
				&(((DeskStyle *)(NULL))->hi_cs));
		} else if (StrEquals(resource, "WindowColorset")) {
			SetDeskStyleColorset(arg1, arg2,
				&(((DeskStyle *)(NULL))->win_cs));
		} else if (StrEquals(resource, "FocusColorset")) {
			SetDeskStyleColorset(arg1, arg2,
				&(((DeskStyle *)(NULL))->focus_cs));
		} else if (StrEquals(resource, "WindowColorsets")) {
			/* Backwards compatibility. */
			if (arg1[0] != '*') {
				SetDeskStyleColorset("*", arg1,
					&(((DeskStyle *)(NULL))->win_cs));
				SetDeskStyleColorset("*", arg2,
					&(((DeskStyle *)(NULL))->focus_cs));
			}
		} else if (StrEquals(resource, "Fore")) {
			if (Pdepth > 0)
				SetDeskStylePixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->fg));
		} else if (StrEquals(resource, "Back") ||
			   StrEquals(resource, "DeskColor"))
		{
			if (Pdepth > 0)
				SetDeskStylePixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->bg));
		} else if (StrEquals(resource, "HiFore")) {
			if (Pdepth > 0)
				SetDeskStylePixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->hi_fg));
		} else if (StrEquals(resource, "HiBack") ||
			   StrEquals(resource, "Hilight"))
		{
			if (Pdepth > 0)
				SetDeskStylePixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->hi_bg));
		} else if (StrEquals(resource, "WindowFore")) {
			if (Pdepth > 1)
				SetDeskStylePixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->win_fg));
		} else if (StrEquals(resource, "WindowBack")) {
			if (Pdepth > 1)
				SetDeskStylePixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->win_bg));
		} else if (StrEquals(resource, "FocusFore")) {
			if (Pdepth > 1)
				SetDeskStylePixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->focus_fg));
		} else if (StrEquals(resource, "FocusBack")) {
			if (Pdepth > 1)
				SetDeskStylePixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->focus_bg));
		} else if (StrEquals(resource, "WindowColors")) {
			/* Backwards compatibility. */
			if (Pdepth > 1 && arg1[0] != '*')
			{
				SetDeskStylePixel("*", arg1,
					&(((DeskStyle *)(NULL))->win_bg));
				SetDeskStylePixel("*", arg2,
					&(((DeskStyle *)(NULL))->win_bg));
				free(arg2);
				tline2 = GetNextToken(tline2, &arg2);
				SetDeskStylePixel("*", arg2,
					&(((DeskStyle *)(NULL))->focus_fg));
				free(arg2);
				tline2 = GetNextToken(tline2, &arg2);
				SetDeskStylePixel("*", arg2,
					&(((DeskStyle *)(NULL))->focus_bg));
			}
		} else if (StrEquals(resource, "BalloonFore")) {
			if (Pdepth > 1)
				SetDeskStylePixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->balloon_fg));
		} else if (StrEquals(resource, "BalloonBack")) {
			if (Pdepth > 1)
				SetDeskStylePixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->balloon_bg));
		} else if (StrEquals(resource, "BalloonBorderColor")) {
			if (Pdepth > 1)
				SetDeskStylePixel(arg1, arg2,
					&(((DeskStyle *)(NULL))->balloon_border));
		} else if (StrEquals(resource, "LabelPixmap")) {
			SetDeskStyleBool(arg1, arg2,
				&(((DeskStyle *)(NULL))->use_label_pixmap));
		} else if (StrEquals(resource, "Geometry")) {
			flags = FScreenParseGeometry(
					next, &g_x, &g_y, &width, &height);
			if (flags & WidthValue)
				pwindow.width = width;
			if (flags & HeightValue)
				pwindow.height = height;
			if (flags & XValue) {
				pwindow.x = g_x;
				usposition = true;
				if (flags & XNegative)
					xneg = true;
			}
			if (flags & YValue) {
				pwindow.y = g_y;
				usposition = true;
				if (flags & YNegative)
					yneg = true;
			}
		} else if (StrEquals(resource, "IconGeometry")) {
			flags = FScreenParseGeometry(
					next, &g_x, &g_y, &width, &height);
			if (flags & WidthValue)
				icon.width = width;
			if (flags & HeightValue)
				icon.height = height;
			if (flags & XValue) {
				icon.x = g_x;
				if (flags & XNegative)
					icon_xneg = true;
			}
			if (flags & YValue) {
				icon.y = g_y;
				if (flags & YNegative)
					icon_yneg = true;
			}
		} else if (StrEquals(resource, "Font")) {
			free(font_string);
			CopyStringWithQuotes(&font_string, next);
			if(strncasecmp(font_string, "none", 4) == 0) {
				use_desk_label = false;
				use_monitor_label = false;
				free(font_string);
				font_string = NULL;
			}
		} else if (StrEquals(resource, "Pixmap") ||
			   StrEquals(resource, "DeskPixmap"))
		{
			if (Pdepth == 0)
				goto free_vars;

			DeskStyle *style;
			if (arg1[0] == '*') {
				TAILQ_FOREACH(style, &desk_style_q, entry) {
					if (style->bgPixmap != NULL) {
						PDestroyFvwmPicture(
							dpy, style->bgPixmap);
						style->bgPixmap = NULL;
					}
					style->bgPixmap = PCacheFvwmPicture(
						dpy, Scr.pager_w, ImagePath,
						arg2, fpa);
				}
			} else {
				int desk = 0;
				sscanf(arg1, "%d", &desk);
				style = FindDeskStyle(desk);
				if (style->bgPixmap != NULL) {
					PDestroyFvwmPicture(
						dpy, style->bgPixmap);
					style->bgPixmap = NULL;
				}
				style->bgPixmap = PCacheFvwmPicture(dpy,
					Scr.pager_w, ImagePath, arg2, fpa);
			}
		} else if (StrEquals(resource, "HilightPixmap")) {
			if (Pdepth == 0)
				goto free_vars;

			DeskStyle *style;
			if (arg1[0] == '*') {
				TAILQ_FOREACH(style, &desk_style_q, entry) {
					if (style->hiPixmap != NULL) {
						PDestroyFvwmPicture(
							dpy, style->hiPixmap);
						style->hiPixmap = NULL;
					}
					style->hiPixmap = PCacheFvwmPicture(
						dpy, Scr.pager_w, ImagePath,
						arg2, fpa);
				}
			} else {
				int desk = 0;
				sscanf(arg1, "%d", &desk);
				style = FindDeskStyle(desk);
				if (style->hiPixmap != NULL) {
					PDestroyFvwmPicture(
						dpy, style->hiPixmap);
					style->hiPixmap = NULL;
				}
				style->hiPixmap = PCacheFvwmPicture(dpy,
					Scr.pager_w, ImagePath, arg2, fpa);
			}
		} else if (StrEquals(resource, "WindowFont") ||
			   StrEquals(resource, "SmallFont"))
		{
			free(smallFont);
			CopyStringWithQuotes(&smallFont, next);
			if (strncasecmp(smallFont, "none", 4) == 0) {
				free(smallFont);
				smallFont = NULL;
			}
		} else if (StrEquals(resource, "Rows")) {
			sscanf(next, "%d", &Rows);
		} else if (StrEquals(resource, "Columns")) {
			sscanf(next, "%d", &Columns);
		} else if (StrEquals(resource, "DesktopScale")) {
			sscanf(next, "%d", &Scr.VScale);
		} else if (StrEquals(resource, "WindowBorderWidth")) {
			MinSize = MinSize - 2*WindowBorderWidth;
			sscanf(next, "%d", &WindowBorderWidth);
			if (WindowBorderWidth > 0)
				MinSize = 2 * WindowBorderWidth + MinSize;
			else
				MinSize = MinSize + 2 *
					  DEFAULT_PAGER_WINDOW_BORDER_WIDTH;
		} else if (StrEquals(resource, "WindowMinSize")) {
			sscanf(next, "%d", &MinSize);
			if (MinSize > 0)
				MinSize = 2 * WindowBorderWidth + MinSize;
			else
				MinSize = 2 * WindowBorderWidth +
					  DEFAULT_PAGER_WINDOW_MIN_SIZE;
		} else if (StrEquals(resource,"WindowLabelFormat")) {
			free(WindowLabelFormat);
			CopyString(&WindowLabelFormat, next);
		} else if (StrEquals(resource, "MoveThreshold")) {
			int val;
			if (GetIntegerArguments(next, NULL, &val, 1) > 0 &&
			    val >= 0)
			{
				MoveThreshold = val;
				MoveThresholdSetForModule = true;
			}
		} else if (StrEquals(resource, "Balloons")) {
			if (StrEquals(next, "Pager")) {
				Balloon.show_in_pager = true;
				Balloon.show_in_icon = false;
			} else if (StrEquals(next, "Icon")) {
				Balloon.show_in_pager = false;
				Balloon.show_in_icon = true;
			} else if (StrEquals(next, "None")) {
				Balloon.show_in_pager = false;
				Balloon.show_in_icon = false;
			} else {
				Balloon.show_in_pager = true;
				Balloon.show_in_icon = true;
			}
		} else if (StrEquals(resource, "BalloonFont")) {
			free(BalloonFont);
			CopyStringWithQuotes(&BalloonFont, next);
		} else if (StrEquals(resource, "BalloonBorderWidth")) {
			sscanf(next, "%d", &(Balloon.border_width));
		} else if (StrEquals(resource, "BalloonYOffset")) {
			sscanf(next, "%d", &(Balloon.y_offset));
		} else if (StrEquals(resource,"BalloonStringFormat")) {
			free(Balloon.label_format);
			CopyString(&(Balloon.label_format), next);
		}

free_vars:
		free(resource);
		free(arg1);
		free(arg2);
	}

	return;
}

/* Returns the DeskStyle for inputted desk. */
DeskStyle *FindDeskStyle(int desk)
{
	DeskStyle *style;

	/* Find matching style to return. */
	TAILQ_FOREACH(style, &desk_style_q, entry) {
		if (style->desk == desk)
			return style;
	}

	/* No matching style found. Create new style and return it. */
	style = NULL;
	DeskStyle *default_style = TAILQ_FIRST(&desk_style_q);

	style = fxcalloc(1, sizeof(DeskStyle));
	memcpy(style, default_style, sizeof *style);

	style->desk = desk;
	xasprintf(&style->label, "Desk %d", desk);

	TAILQ_INSERT_TAIL(&desk_style_q, style, entry);
	return style;
}

void ExitPager(void)
{
  DeskStyle *style, *style2;
  struct fpmonitor *fp, *fp1;
  PagerWindow *t, *t2;

  /* Balloons */
  free(Balloon.label);
  free(Balloon.label_format);

  /* Monitors */
  TAILQ_FOREACH_SAFE(fp, &fp_monitor_q, entry, fp1) {
	TAILQ_REMOVE(&fp_monitor_q, fp, entry);
	free(fp->CPagerWin);
	free(fp);
  }

  /* DeskStyles */
  TAILQ_FOREACH_SAFE(style, &desk_style_q, entry, style2) {
	TAILQ_REMOVE(&desk_style_q, style, entry);
	free(style->label);
	free(style);
  }
  free(Desks);

  /* PagerWindows */
	t2 = Start;
	while (t2 != NULL) {
		t = t2;
		t2 = t2->next;

		XDestroyWindow(dpy, t->PagerView);
		XDestroyWindow(dpy, t->IconView);
		free(t->res_class);
		free(t->res_name);
		free(t->window_name);
		free(t->icon_name);
		free(t->window_label);
		free(t);
	}

  if (is_transient)
  {
    XUngrabPointer(dpy,CurrentTime);
    MyXUngrabServer(dpy);
    XSync(dpy,0);
  }
  XUngrabKeyboard(dpy, CurrentTime);
  free(preferred_monitor);
  exit(0);
}