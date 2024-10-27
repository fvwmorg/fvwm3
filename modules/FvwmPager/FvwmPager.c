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
#include "libs/ftime.h"
#include "libs/safemalloc.h"
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
#include "libs/Picture.h"
#include "libs/Strings.h"
#include "libs/System.h"

#include "fvwm/fvwm.h"
#include "libs/vpacket.h"
#include "FvwmPager.h"

/*
 * Shared variables.
 */
/* Colors, Pixmaps, Fonts, etc. */
char		*ImagePath = NULL;
char		*BalloonFont = NULL;
char		*WindowLabelFormat = NULL;
FlocaleWinString	*FwinString;
Atom		wm_del_win;
int		mouse_action[P_MOUSE_BUTTONS];
char		**mouse_cmd;

/* Sizes / Dimensions */
int		Rows = -1;
int		desk1 = 0;
int		desk2 = 0;
int		desk_i = 0;
int		ndesks = 0;
int		desk_w = 0;
int		desk_h = 0;
int		label_h = 0;
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
bool	ewmhiwa = false;
bool	IsShared = false;
bool	MiniIcons = false;
bool	Swallowed = false;
bool	UseSkipList = false;
bool	StartIconic = false;
bool	LabelsBelow = false;
bool	ShapeLabels = false;
bool	is_transient = false;
bool	HilightDesks = true;
bool	HilightLabels = true;
bool	error_occured = false;
bool	use_desk_label = true;
bool	WindowBorders3d = false;
bool	HideSmallWindows = false;
bool	SendCmdAfterMove = false;
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

static void Loop(int *fd);
static void TerminateHandler(int);
static int My_XNextEvent(Display *dpy, XEvent *event);

/* Procedure: main - start of module */
int main(int argc, char **argv)
{
	short opt_num;

	/* Save our program name */
	MyName = GetFileNameFromPath(argv[0]);

	if (argc  < 6) {
		fprintf(stderr, "%s must be executed by fvwm!\n", MyName),
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
	fvwmSetSignalMask(sigmask(SIGPIPE) |
			  sigmask(SIGTERM) |
			  sigmask(SIGQUIT) |
			  sigmask(SIGINT) |
			  sigmask(SIGHUP));
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

	/* Parse command line options */
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
	if (argc >= opt_num + 1) {
		char *s;

		if (!StrEquals(argv[opt_num], "*")) {
			for (s = argv[opt_num]; *s; s++) {
				if (!isdigit(*s) && (*s != '-' ||
				    s != argv[opt_num] || *(s+1) == 0))
				{
					free(MyName);
					MyName = fxstrdup(argv[opt_num]);
					opt_num++;
					break;
				}
			}
		}
	}

	/* Determine which desks to show. */
	if (argc < opt_num + 1) {
		desk1 = desk2 = 0;
	} else if (StrEquals(argv[opt_num], "*")) {
		desk1 = desk2 = 0;
		fAlwaysCurrentDesk = true;
	} else {
		desk1 = atoi(argv[opt_num]);
		if (argc == opt_num+1)
			desk2 = desk1;
		else
			desk2 = atoi(argv[opt_num+1]);
		if (desk1 < 0)
			desk1 = 0;
		if (desk2 < 0)
			desk2 = 0;
		if (desk2 < desk1) {
			int dtemp = desk1;
			desk1 = desk2;
			desk2 = dtemp;
		}
	}
	ndesks = desk2 - desk1 + 1;

	/* Tell the FEvent module an event type that is not used by fvwm. */
	/* Is this needed, I don't see this event being used. */
	fev_init_invalid_event_type(KeymapNotify);

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
		       M_RESTACK|M_STRING);
	SetMessageMask(fd,
		       MX_VISIBLE_ICON_NAME|
		       MX_PROPERTY_CHANGE|
		       MX_MONITOR_FOCUS);

	/* Initialize X connection */
	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "%s: can't open display", MyName);
		exit(1);
	}
	flib_init_graphics(dpy);
	x_fd = XConnectionNumber(dpy);
	Scr.screen = DefaultScreen(dpy);
	Scr.Root = RootWindow(dpy, Scr.screen);

	/* Initialize pager components. */
	init_fvwm_pager();

	/* tell fvwm we're running */
	SendFinishedStartupNotification(fd);

	/* Event loop */
	Loop(fd);

#ifdef FVWM_DEBUG_MSGS
	if (isTerminated)
		fprintf(stderr, "%s: Received signal: exiting...\n", MyName);
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
 *      SIGPIPE handler - SIGPIPE means fvwm is dying
 */
static void TerminateHandler(int sig)
{
	fvwmSetTerminate(sig);
	return;
}

#if 0
/* Is this used? */
void DeadPipe(int nonsense)
{
	exit(0);
	return;
}
#endif

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
	initialize_desk_style_gcs(style);

	TAILQ_INSERT_TAIL(&desk_style_q, style, entry);
	return style;
}

void ExitPager(void)
{
	int i;
	DeskStyle *style, *style2;
	struct fpmonitor *fp, *fp1;
	PagerWindow *t, *t2;

	/* Balloons */
	free(Balloon.label);
	free(Balloon.label_format);

	/* Fonts */
	FlocaleUnloadFont(dpy, Scr.Ffont);
	FlocaleUnloadFont(dpy, Scr.winFfont);
	FlocaleUnloadFont(dpy, Balloon.Ffont);

	/* Monitors */
	TAILQ_FOREACH_SAFE(fp, &fp_monitor_q, entry, fp1) {
		TAILQ_REMOVE(&fp_monitor_q, fp, entry);
		free(fp->CPagerWin);
		free(fp);
	}

	/* DeskStyles */
	TAILQ_FOREACH_SAFE(style, &desk_style_q, entry, style2) {
		TAILQ_REMOVE(&desk_style_q, style, entry);
		PDestroyFvwmPicture(dpy, style->bgPixmap);
		PDestroyFvwmPicture(dpy, style->hiPixmap);
		free(style->label);
		free(style);
	}
	free(Desks);

	/* Mouse Bindings */
	for (i = 0; i < P_MOUSE_BUTTONS; i++) {
		free(mouse_cmd[i]);
	}
	free(mouse_cmd);

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

	/* Additional pointers */
	free(preferred_monitor);
	free(ImagePath);
	free(WindowLabelFormat);

	if (is_transient) {
		XUngrabPointer(dpy, CurrentTime);
		MyXUngrabServer(dpy);
		XSync(dpy, 0);
	}
	XUngrabKeyboard(dpy, CurrentTime);

	exit(0);
}
