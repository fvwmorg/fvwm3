/* -*-c-*- */
/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
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

/* ------------------------------- includes -------------------------------- */
#include "config.h"
#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "libs/ftime.h"
#include <sys/stat.h>

#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "libs/defaults.h"
#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/FRenderInit.h"
#include "libs/Grab.h"
#include "libs/gravity.h"
#include "fvwm/fvwm.h"
#include "libs/Module.h"
#include "libs/fvwmsignal.h"
#include "libs/Colorset.h"
#include "libs/vpacket.h"
#include "libs/FRender.h"
#include "libs/fsm.h"
#include "libs/ColorUtils.h"
#include "libs/Graphics.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/System.h"
#include "libs/wild.h"
#include "libs/WinMagic.h"
#include "libs/XError.h"

#include "FvwmButtons.h"
#include "misc.h" /* ConstrainSize() */
#include "parse.h" /* ParseConfiguration(), parse_window_geometry() */
#include "draw.h"
#include "dynamic.h"


#define MW_EVENTS ( \
	ExposureMask | \
	StructureNotifyMask | \
	ButtonReleaseMask | \
	ButtonPressMask | \
	LeaveWindowMask | \
	EnterWindowMask | \
	PointerMotionMask | \
	KeyReleaseMask | \
	KeyPressMask | \
	ButtonMotionMask | \
	0)
/* SW_EVENTS are for swallowed windows... */
#define SW_EVENTS ( \
	PropertyChangeMask | \
	StructureNotifyMask | \
	ResizeRedirectMask | \
	SubstructureNotifyMask | \
	0)
/* PA_EVENTS are for swallowed panels... */
#define PA_EVENTS ( \
	StructureNotifyMask | \
	0)

extern int nColorsets;  /* in libs/Colorsets.c */

/* --------------------------- external functions -------------------------- */
extern void DumpButtons(button_info *);
extern void SaveButtons(button_info *);

/* ------------------------------ prototypes ------------------------------- */

RETSIGTYPE DeadPipe(int nonsense);
static void DeadPipeCleanup(void);
static RETSIGTYPE TerminateHandler(int sig);
void SetButtonSize(button_info *, int, int);
/* main */
void Loop(void);
void RedrawWindow(void);
void RecursiveLoadData(button_info *, int *, int *);
void CreateUberButtonWindow(button_info *, int, int);
int My_FNextEvent(Display *dpy, XEvent *event);
void process_message(unsigned long type, unsigned long *body);
extern void send_clientmessage(Display *disp, Window w, Atom a, Time timestamp);
void parse_message_line(char *line);
void CheckForHangon(unsigned long *);
static Window GetRealGeometry(
	Display *, Window, int *, int *, unsigned int *, unsigned int *,
	unsigned int *, unsigned int *);
static void GetPanelGeometry(
	Bool get_big, button_info *b, int lb, int tb, int rb, int bb,
	int *x, int *y, int *w, int *h);
void swallow(unsigned long *);
void AddButtonAction(button_info *, int, char *);
char *GetButtonAction(button_info *, int);
static void update_root_transparency(XEvent *Event);
static void change_colorset(int colorset, XEvent *Event);

void DebugEvents(XEvent *);

static void HandlePanelPress(button_info *b);

/* -------------------------------- globals ---------------------------------*/

Display *Dpy;
Window Root;
Window MyWindow;
char *MyName;
int screen;

static int x_fd;
static fd_set_size_t fd_width;

char *config_file = NULL;

static Atom _XA_WM_DEL_WIN;

char *imagePath = NULL;

static Pixel hilite_pix, back_pix, shadow_pix, fore_pix;
GC NormalGC;
/* needed for relief drawing only */
GC ShadowGC;
/* needed for transparency */
GC transGC = NULL;

int Width, Height;
static int x = -30000, y = -30000;
int w = -1;
int h = -1;
static int gravity = NorthWestGravity;
int new_desk = 0;
static int ready = 0;
static int xneg = 0;
static int yneg = 0;
int button_width = 0;
int button_height = 0;
int button_lborder = 0;
int button_rborder = 0;
int button_tborder = 0;
int button_bborder = 0;
Bool has_button_geometry = 0;
Bool is_transient = 0;
Bool is_transient_panel = 0;

/* $CurrentButton is set on ButtonPress, $ActiveButton is set whenever the
   mouse is over a button that is redrawn specially. */
button_info *CurrentButton = NULL, *ActiveButton = NULL;
Bool is_pointer_in_current_button = False;
int fd[2];

button_info *UberButton = NULL;

int dpw;
int dph;

Bool do_allow_bad_access = False;
Bool was_bad_access = False;
Bool swallowed = False;
Window swallower_win = 0;

/* ------------------------------ Misc functions ----------------------------*/

/**
*** Some fancy routines straight out of the manual :-) Used in DeadPipe.
**/
Bool DestroyedWindow(Display *d, XEvent *e, char *a)
{
	if (e->xany.window == (Window)a &&
		((e->type == DestroyNotify
			&& e->xdestroywindow.window == (Window)a) ||
		(e->type == UnmapNotify && e->xunmap.window == (Window)a)))
	{
		return True;
	}
	return False;
}

static Window SwallowedWindow(button_info *b)
{
	return (b->flags.b_Panel) ? b->PanelWin : b->IconWin;
}

int IsThereADestroyEvent(button_info *b)
{
	XEvent event;
	return FCheckIfEvent(
		Dpy, &event, DestroyedWindow, (char *)SwallowedWindow(b));
}

/**
*** DeadPipe()
*** Externally callable function to quit! Note that DeadPipeCleanup
*** is an exit-procedure and so will be called automatically
**/
RETSIGTYPE DeadPipe(int whatever)
{
	exit(0);
	SIGNAL_RETURN;
}

/**
*** TerminateHandler()
*** Signal handler that will make the event-loop terminate
**/
static RETSIGTYPE
TerminateHandler(int sig)
{
	fvwmSetTerminate(sig);
	SIGNAL_RETURN;
}

/**
*** DeadPipeCleanup()
*** Remove all the windows from the Button-Bar, and close them as necessary
**/
static void DeadPipeCleanup(void)
{
	button_info *b, *ub = UberButton;
	int button = -1;
	Window swin;

	signal(SIGPIPE, SIG_IGN);/* Xsync may cause SIGPIPE */

	while (NextButton(&ub, &b, &button, 0))
	{
		swin = SwallowedWindow(b);
		/* delete swallowed windows */
		if ((buttonSwallowCount(b) == 3) && swin)
		{
#ifdef DEBUG_HANGON
			fprintf(stderr,
				"%s: Button 0x%06x window 0x%x (\"%s\") is ",
				MyName, (ushort)b, (ushort)swin, b->hangon);
#endif
			if (!IsThereADestroyEvent(b))
			{ /* Has someone destroyed it? */
				if (!(buttonSwallow(b)&b_NoClose))
				{
					if (buttonSwallow(b)&b_Kill)
					{
						XKillClient(Dpy, swin);
#ifdef DEBUG_HANGON
						fprintf(stderr, "now killed\n");
#endif
					}
					else
					{
						send_clientmessage(
							Dpy, swin,
							_XA_WM_DEL_WIN,
							CurrentTime);
#ifdef DEBUG_HANGON
						fprintf(stderr,
							"now deleted\n");
#endif
					}
				}
				else
				{
#ifdef DEBUG_HANGON
					fprintf(stderr, "now unswallowed\n");
#endif
					if (b->swallow & b_FvwmModule)
					{
						char cmd[256];

						sprintf(
						  cmd,
						  "PropertyChange %u %u %lu",
						  MX_PROPERTY_CHANGE_SWALLOW,
						  0, swin);
						SendText(fd, cmd, 0);
					}
					fsm_proxy(Dpy, swin, NULL);
					XReparentWindow(
						Dpy, swin, Root, b->x, b->y);
					XMoveWindow(Dpy, swin, b->x, b->y);
					XResizeWindow(Dpy, swin, b->w, b->h);
					XSetWindowBorderWidth(Dpy, swin, b->bw);
					if (b->flags.b_Panel)
					{
						XMapWindow(Dpy, swin);
					}
				}
			}
#ifdef DEBUG_HANGON
			else
			{
				fprintf(stderr, "already handled\n");
			}
#endif
		}
	}

	XSync(Dpy, 0);

	fsm_close();
	/* Hey, we have to free the pictures too! */
	button = -1;
	ub = UberButton;
	while (NextButton(&ub, &b, &button, 1))
	{
		/* The picture pointer is NULL if the pixmap came from a
		 * colorset. */
		if (b->flags.b_Icon && b->icon != NULL)
		{
			PDestroyFvwmPicture(Dpy, b->icon);
		}
		if (b->flags.b_IconBack && b->backicon != NULL)
		{
			PDestroyFvwmPicture(Dpy, b->icon);
		}
		if (b->flags.b_Container && b->c->flags.b_IconBack &&
			!(b->c->flags.b_TransBack) && b->c->backicon != NULL)
		{
			PDestroyFvwmPicture(Dpy, b->c->backicon);
		}
	}
}

/**
*** SetButtonSize()
*** Propagates global geometry down through the button hierarchy.
**/
void SetButtonSize(button_info *ub, int w, int h)
{
	int i = 0;
	int dx;
	int dy;

	if (!ub || !(ub->flags.b_Container))
	{
		fprintf(stderr,
			"%s: BUG: Tried to set size of noncontainer\n", MyName);
		exit(2);
	}
	if (ub->c->num_rows == 0 || ub->c->num_columns == 0)
	{
		fprintf(stderr,
			"%s: BUG: Set size when rows/cols was unset\n", MyName);
		exit(2);
	}

	if (ub->parent)
	{
		i = buttonNum(ub);
		ub->c->xpos = buttonXPos(ub, i);
		ub->c->ypos = buttonYPos(ub, i);
	}
	dx = buttonXPad(ub) + buttonFrame(ub);
	dy = buttonYPad(ub) + buttonFrame(ub);
	ub->c->xpos += dx;
	ub->c->ypos += dy;
	w -= 2 * dx;
	h -= 2 * dy;
	ub->c->width = w;
	ub->c->height = h;

	i = 0;
	while (i < ub->c->num_buttons)
	{
		if (ub->c->buttons[i] && ub->c->buttons[i]->flags.b_Container)
		{
			SetButtonSize(
				ub->c->buttons[i],
				buttonWidth(ub->c->buttons[i]),
				buttonHeight(ub->c->buttons[i]));
		}
		i++;
	}
}


/**
*** AddButtonAction()
**/
void AddButtonAction(button_info *b, int n, char *action)
{
	int l;
	char *s;
	char *t;

	if (!b || n < 0 || n > NUMBER_OF_EXTENDED_MOUSE_BUTTONS || !action)
	{
		fprintf(stderr, "%s: BUG: AddButtonAction failed\n", MyName);
		exit(2);
	}
	if (b->flags.b_Action)
	{
		if (b->action[n])
		{
			free(b->action[n]);
		}
	}
	else
	{
		int i;
		b->action = xmalloc(
			(NUMBER_OF_EXTENDED_MOUSE_BUTTONS + 1) * sizeof(char*));
		for (i = 0; i <= NUMBER_OF_EXTENDED_MOUSE_BUTTONS;
			b->action[i++] = NULL)
		{
		}
		b->flags.b_Action = 1;
	}

	while (*action && isspace((unsigned char)*action))
	{
		action++;
	}
	l = strlen(action);
	if (l > 1)
	{
		switch (action[0])
		{
		case '\"':
		case '\'':
		case '`':
			s = SkipQuote(action, NULL, "", "");
			/* Strip outer quotes */
			if (*s == 0)
			{
				action++;
				l -= 2;
			}
			break;
		default:
			break;
		}
	}
	t = xmalloc(l + 1);
	memmove(t, action, l);
	t[l] = 0;
	b->action[n] = t;
}


/**
*** GetButtonAction()
**/
char *GetButtonAction(button_info *b, int n)
{
	rectangle r;
	char *act;

	if (!b || !(b->flags.b_Action) || !(b->action) || n < 0 ||
		n > NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
	{
		return NULL;
	}
	if (!b->action[n])
	{
		return NULL;
	}
	get_button_root_geometry(&r, b);
	act = module_expand_action(
		Dpy, screen, b->action[n], &r, UberButton->c->fore,
		UberButton->c->back);

	return act;
}

Pixmap shapeMask = None;
/**
*** SetTransparentBackground()
*** use the Shape extension to create a transparent background.
***   Patrice Fortier & others.
**/
void SetTransparentBackground(button_info *ub, int w, int h)
{
	if (FShapesSupported)
	{
		button_info *b;
		int i = -1;
		XGCValues gcv;

		if (shapeMask != None)
			XFreePixmap(Dpy, shapeMask);
		shapeMask = XCreatePixmap(Dpy, MyWindow, w, h, 1);
		if (transGC == NULL)
		{
			transGC = fvwmlib_XCreateGC(Dpy, shapeMask, 0, &gcv);
		}
		XSetClipMask(Dpy, transGC, None);
		XSetForeground(Dpy, transGC, 1);
		XFillRectangle(Dpy, shapeMask, transGC, x, y, w, h);

		while (NextButton(&ub, &b, &i, 0))
		{
			RedrawButton(b, DRAW_FORCE, NULL);
		}
		XFlush(Dpy);
	}
}

/**
*** myErrorHandler()
*** Shows X errors made by FvwmButtons.
**/
XErrorHandler oldErrorHandler = NULL;
int myErrorHandler(Display *dpy, XErrorEvent *event)
{
	/* some errors are acceptable, mostly they're caused by
	 * trying to update a lost  window */
	if ((event->error_code == BadAccess) && do_allow_bad_access)
	{
		was_bad_access = 1;
		return 0;
	}
	if ((event->error_code == BadWindow)
		|| (event->error_code == BadDrawable)
		|| (event->error_code == BadMatch)
		|| (event->request_code == X_GrabButton)
		|| (event->request_code == X_GetGeometry)
		|| (event->error_code == BadPixmap)
		|| (event->error_code ==
			FRenderGetErrorCodeBase() + FRenderBadPicture))
		return 0;

	PrintXErrorAndCoredump(dpy, event, MyName);

	/* return (*oldErrorHandler)(dpy, event); */
	return 0;
}

/* ---------------------------------- main ----------------------------------*/

/**
*** main()
**/
int main(int argc, char **argv)
{
	int i;
	Window root;
	int x, y, maxx, maxy, border_width;
	unsigned int depth;
	button_info *b, *ub;
	int geom_option_argc = 0;
	XSetWindowAttributes xswa;

	FlocaleInit(LC_CTYPE, "", "", "FvwmButtons");

	MyName = GetFileNameFromPath(argv[0]);

#ifdef HAVE_SIGACTION
	{
		struct sigaction  sigact;

		sigemptyset(&sigact.sa_mask);
		sigaddset(&sigact.sa_mask, SIGPIPE);
		sigaddset(&sigact.sa_mask, SIGINT);
		sigaddset(&sigact.sa_mask, SIGHUP);
		sigaddset(&sigact.sa_mask, SIGQUIT);
		sigaddset(&sigact.sa_mask, SIGTERM);
#ifdef SA_INTERRUPT
		sigact.sa_flags = SA_INTERRUPT;
# else
		sigact.sa_flags = 0;
#endif
		sigact.sa_handler = TerminateHandler;

		sigaction(SIGPIPE, &sigact, NULL);
		sigaction(SIGINT,  &sigact, NULL);
		sigaction(SIGHUP,  &sigact, NULL);
		sigaction(SIGQUIT, &sigact, NULL);
		sigaction(SIGTERM, &sigact, NULL);
	}
#else
	/* We don't have sigaction(), so fall back to less robust methods. */
#ifdef USE_BSD_SIGNALS
	fvwmSetSignalMask(
		sigmask(SIGPIPE) |
		sigmask(SIGINT)  |
		sigmask(SIGHUP)  |
		sigmask(SIGQUIT) |
		sigmask(SIGTERM));
#endif

	signal(SIGPIPE, TerminateHandler);
	signal(SIGINT,  TerminateHandler);
	signal(SIGHUP,  TerminateHandler);
	signal(SIGQUIT, TerminateHandler);
	signal(SIGTERM, TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
	siginterrupt(SIGPIPE, 1);
	siginterrupt(SIGINT, 1);
	siginterrupt(SIGHUP, 1);
	siginterrupt(SIGQUIT, 1);
	siginterrupt(SIGTERM, 1);
#endif
#endif

	if (argc<6 || argc>10)
	{
		fprintf(stderr,
			"%s version %s should only be executed by fvwm!\n",
			MyName, VERSION);
		exit(1);
	}

	for (i = 6; i < argc && i < 11; i++)
	{
		static Bool has_name = 0;
		static Bool has_file = 0;
		static Bool has_geometry = 0;

		if (!has_geometry && strcmp(argv[i], "-g") == 0)
		{
			has_geometry = 1;
			if (++i < argc)
			{
				/* can't do that now because the UberButton */
				/* has not been set up */
				geom_option_argc = i;
			}
		}
		else if (!is_transient && !is_transient_panel &&
			(strcmp(argv[i], "-transient") == 0 ||
			strcmp(argv[i], "transient") == 0))
		{
			is_transient = 1;
		}
		else if (!is_transient && !is_transient_panel &&
			(strcmp(argv[i], "-transientpanel") == 0 ||
			strcmp(argv[i], "transientpanel") == 0))
		{
			is_transient_panel = 1;
		}
		else if (!has_name)
		/* There is a naming argument here! */
		{
			free(MyName);
			MyName = xstrdup(argv[i]);
			has_name = 1;
		}
		else if (!has_file)
		/* There is a config file here! */
		{
			config_file = xstrdup(argv[i]);
			has_file = 1;
		}
	}

	fd[0] = atoi(argv[1]);
	fd[1] = atoi(argv[2]);
	if (!(Dpy = XOpenDisplay(NULL)))
	{
		fprintf(stderr,
			"%s: Can't open display %s", MyName,
			XDisplayName(NULL));
		exit (1);
	}
	flib_init_graphics(Dpy);

	x_fd = XConnectionNumber(Dpy);
	fd_width = GetFdWidth();

	screen = DefaultScreen(Dpy);
	Root = RootWindow(Dpy, screen);
	if (Root == None)
	{
		fprintf(stderr, "%s: Screen %d is not valid\n", MyName, screen);
		exit(1);
	}

	oldErrorHandler = XSetErrorHandler(myErrorHandler);

	UberButton = xmalloc(sizeof(button_info));
	memset(UberButton, 0, sizeof(button_info));
	UberButton->BWidth = 1;
	UberButton->BHeight = 1;
	MakeContainer(UberButton);

	dpw = DisplayWidth(Dpy, screen);
	dph = DisplayHeight(Dpy, screen);

#ifdef DEBUG_INIT
	fprintf(stderr, "%s: Parsing...", MyName);
#endif

	UberButton->title   = MyName;
	UberButton->swallow = 1; /* the panel is shown */

	/* parse module options */
	ParseConfiguration(UberButton);

	/* To avoid an infinite loop of Enter/Leave-Notify events, if the user
	   uses ActiveIcon with "Pixmap none" they MUST specify ActiveColorset.
	*/
	if (FShapesSupported && (UberButton->c->flags.b_TransBack) &&
		!(UberButton->c->flags.b_ActiveColorset))
	{
		i = -1;
		ub = UberButton;
		while (NextButton(&ub, &b, &i, 0))
		{
			if (b->flags.b_ActiveIcon || b->flags.b_ActiveTitle)
			{
				fprintf(stderr,
					"%s: Must specify ActiveColorset when "
					"using ActiveIcon or ActiveTitle with "
					"\"Pixmap none\".\n", MyName);
				exit(0);
			}
		}
	}

	/* we can't set the size if it was specified in pixels per button here;
	 * delay until after call to RecursiveLoadData. */
	/* parse the geometry string */
	if (geom_option_argc != 0)
	{
		parse_window_geometry(argv[geom_option_argc], 0);
	}

	/* Don't quit if only a subpanel is empty */
	if (UberButton->c->num_buttons == 0)
	{
		fprintf(stderr, "%s: No buttons defined. Quitting\n", MyName);
		exit(0);
	}

#ifdef DEBUG_INIT
	fprintf(stderr, "OK\n%s: Shuffling...", MyName);
#endif

	ShuffleButtons(UberButton);
	NumberButtons(UberButton);

#ifdef DEBUG_INIT
	fprintf(stderr, "OK\n%s: Creating Main Window ...", MyName);
#endif

	xswa.colormap = Pcmap;
	xswa.border_pixel = 0;
	xswa.background_pixmap = None;
	MyWindow = XCreateWindow(
		Dpy, Root, 0, 0, 1, 1, 0, Pdepth, InputOutput, Pvisual,
		CWColormap | CWBackPixmap | CWBorderPixel, &xswa);

#ifdef DEBUG_INIT
	fprintf(stderr, "OK\n%s: Loading data...\n", MyName);
#endif

	/* Load fonts and icons, calculate max buttonsize */
	maxx = 0;
	maxy = 0;
	RecursiveLoadData(UberButton, &maxx, &maxy);

	/* now we can size the main window if pixels per button were specified
	 */
	if (has_button_geometry && button_width > 0 && button_height > 0)
	{
		w = button_width * UberButton->c->num_columns;
		h = button_height * UberButton->c->num_rows;
	}

#ifdef DEBUG_INIT
	fprintf(stderr, "%s: Configuring main window...", MyName);
#endif

	CreateUberButtonWindow(UberButton, maxx, maxy);

	if (!XGetGeometry(
		Dpy, MyWindow, &root, &x, &y, (unsigned int *)&Width,
		(unsigned int *)&Height, (unsigned int *)&border_width, &depth))
	{
		fprintf(stderr, "%s: Failed to get window geometry\n", MyName);
		exit(0);
	}
	SetButtonSize(UberButton, Width, Height);
	i = -1;
	ub = UberButton;

	if (FShapesSupported)
	{
		if (UberButton->c->flags.b_TransBack)
		{
			SetTransparentBackground(UberButton, Width, Height);
		}
	}

	i = -1;
	ub = UberButton;
	while (NextButton(&ub, &b, &i, 0))
	{
		MakeButton(b);
	}

#ifdef DEBUG_INIT
	fprintf(stderr, "OK\n%s: Mapping windows...", MyName);
#endif

	XMapSubwindows(Dpy, MyWindow);
	XMapWindow(Dpy, MyWindow);

	SetMessageMask(
		fd, M_NEW_DESK | M_END_WINDOWLIST | M_MAP | M_WINDOW_NAME
		| M_RES_CLASS | M_CONFIG_INFO | M_END_CONFIG_INFO | M_RES_NAME
		| M_SENDCONFIG | M_ADD_WINDOW | M_CONFIGURE_WINDOW | M_STRING);
	SetMessageMask(fd, MX_PROPERTY_CHANGE);

	/* request a window list, since this triggers a response which
	 * will tell us the current desktop and paging status, needed to
	 * indent buttons correctly */
	SendText(fd, "Send_WindowList", 0);

#ifdef DEBUG_INIT
	fprintf(stderr, "OK\n%s: Startup complete\n", MyName);
#endif

	/*
	** Now that we have finished initialising everything,
	** it is safe(r) to install the clean-up handlers ...
	*/
	atexit(DeadPipeCleanup);

	/* tell fvwm we're running */
	SendFinishedStartupNotification(fd);

	Loop();

	return 0;
}

static button_info *handle_new_position(
	button_info *b, int pos_x, int pos_y)
{
	Bool f = is_pointer_in_current_button, redraw = False;

	if (pos_x < 0 || pos_x >= Width ||
	    pos_y < 0 || pos_y >= Height)
	{
		/* cursor is outside of FvwmButtons window. */
		return b;
	}

	/* find out which button the cursor is in now. */
	b = select_button(UberButton, pos_x, pos_y);

	is_pointer_in_current_button =
		(CurrentButton && CurrentButton == b);
	if (CurrentButton && is_pointer_in_current_button != f)
	{
		redraw = True;
	}

	if (b != ActiveButton && CurrentButton == NULL)
	{
		if (ActiveButton)
		{
			button_info *tmp = ActiveButton;
			ActiveButton = b;
			RedrawButton(tmp, DRAW_FORCE, NULL);
		}
		if (
			b->flags.b_ActiveIcon ||
			b->flags.b_ActiveTitle ||
			b->flags.b_ActiveColorset ||
			UberButton->c->flags.b_ActiveColorset)
		{
			ActiveButton = b;
			redraw = True;
		}
	}
	if (redraw)
	{
		RedrawButton(b, DRAW_FORCE, NULL);
	}

	return b;
}

void ButtonPressProcess (button_info *b, char **act)
{
	FlocaleNameString tmp;
    	int i,i2;

        memset(&tmp, 0, sizeof(tmp));
	if (b && !*act && b->flags.b_Panel)
	{
		ActiveButton = b;
	  	HandlePanelPress(b);
		if (b->newflags.panel_mapped == 0)
	  	{
	    		if (is_transient)
	    		{
	      			/* terminate if transient and panel has been
				 * unmapped */
	      			exit(0);
	    		}
	    		else if (is_transient_panel)
	    		{
	      			XWithdrawWindow(Dpy, MyWindow, screen);
	    		}
	  	}
	} /* panel */
	else
	{
		if (!*act)
	    		*act=GetButtonAction(b,0);
	  	if (b && b == CurrentButton && *act)
	  	{
	    		if (strncasecmp(*act,"Exec",4) == 0 &&
				isspace((*act)[4]))
			{

				/* Look for Exec "identifier", in which case
				 * the button stays down until window
				 * "identifier" materializes */
				i = 4;
				while(isspace((unsigned char)(*act)[i]))
					i++;
				if((*act)[i] == '"')
				{
					i2=i+1;
					while((*act)[i2]!=0 && (*act)[i2]!='"')
						i2++;

					if(i2-i>1)
					{
						b->flags.b_Hangon = 1;
						b->hangon = xmalloc(i2-i);
						strncpy(
							b->hangon, &(*act)[i+1],
							i2-i-1);
						b->hangon[i2-i-1] = 0;
					}
					i2++;
				}
				else
					i2=i;

				tmp.name = xmalloc(strlen(*act)+1);
				strcpy(tmp.name, "Exec ");
				while (
					(*act)[i2]!=0 &&
					isspace((unsigned char)(*act)[i2]))
				{
					i2++;
				}
				strcat(tmp.name ,&(*act)[i2]);
				SendText(fd,tmp.name,0);
				if (is_transient)
				{
					/* and exit */
					exit(0);
				}
				if (is_transient_panel)
				{
					XWithdrawWindow(Dpy, MyWindow, screen);
				}
				free(tmp.name);
			} /* exec */
			else if(strncasecmp(*act,"DumpButtons",11)==0)
				DumpButtons(UberButton);
			else if(strncasecmp(*act,"SaveButtons",11)==0)
				SaveButtons(UberButton);
			else
			{
				SendText(fd,*act,0);
				if (is_transient)
				{
					/* and exit */
					exit(0);
				}
				if (is_transient_panel)
				{
					XWithdrawWindow(Dpy, MyWindow, screen);
				}
			}
		} /* *act */
	} /* !panel */
}

/* -------------------------------- Main Loop -------------------------------*/

/**
*** Loop
**/
void Loop(void)
{
	XEvent Event;
	KeySym keysym;
	char buffer[10], *act;
	int i, button;
	int x;
	int y;
	button_info *ub, *b;
	button_info *tmpb;
	FlocaleNameString tmp;

	tmp.name = NULL;
	tmp.name_list = NULL;
	while ( !isTerminated )
	{
		if (My_FNextEvent(Dpy, &Event))
		{
			if (FShapesSupported && Event.type == FShapeEventBase + FShapeNotify)
			{
				FShapeEvent *sev = (FShapeEvent *) &Event;

				if (sev->kind != FShapeBounding)
				{
					return;
				}
				if (UberButton->c->flags.b_TransBack)
				{
					SetTransparentBackground(
						UberButton, Width, Height);
				}
				continue;
			}
			switch (Event.type)
			{
			case Expose:
				{
					int ex, ey, ex2, ey2;

					if (Event.xexpose.window != MyWindow)
					{
						break;
					}
					ex = Event.xexpose.x;
					ey = Event.xexpose.y;
					ex2= Event.xexpose.x +
						Event.xexpose.width;
					ey2= Event.xexpose.y +
						Event.xexpose.height;
					while (
						FCheckTypedWindowEvent(
							Dpy, MyWindow, Expose,
							&Event))
					{
					 /* maybe we should not purge here,
					  * this may interfere
					  * with configure notify */
					 ex = min(ex, Event.xexpose.x);
					 ey = min(ey, Event.xexpose.y);
					 ex2 = max(ex2, Event.xexpose.x
						+ Event.xexpose.width);
					 ey2 = max(ey2, Event.xexpose.y
						+ Event.xexpose.height);
					}
					Event.xexpose.x = ex;
					Event.xexpose.y = ey;
					Event.xexpose.width = ex2-ex;
					Event.xexpose.height = ey2-ey;
					button = -1;
					ub = UberButton;
					while (NextButton(&ub, &b, &button, 1))
					{
						Bool r = False;

						tmpb = b;
						while (tmpb->parent != NULL)
						{
							if (tmpb->flags.b_Container)
							{
								int b =
								buttonNum(tmpb);
								x = buttonXPos(
									tmpb,
									b);
								y = buttonYPos(
									tmpb,
									b);
							}
							else
							{
								x = buttonXPos(
									tmpb,
									button);
								y = buttonYPos(
									tmpb,
									button);
							}
							if (!(ex > x +
							buttonWidth(tmpb) ||
							ex2 < x ||
							ey > y +
							buttonHeight(tmpb) ||
							ey2 < y))
							{
								r = True;
							}
							tmpb = tmpb->parent;
						}
						if (r)
						{
							if (ready < 1 &&
							!(b->flags.b_Container))
							{
								MakeButton(b);
							}
							RedrawButton(
							b, DRAW_ALL, &Event);
						}
					}
					if (ready < 1)
					{
						ready++;
					}
				}
				break;

/* Start of TO-BE-REFORMATTED-BLOCK */

      case ConfigureNotify:
      {
	XEvent event;
	unsigned int depth, tw, th, border_width;
	Window root;

	fev_sanitise_configure_notify(&Event.xconfigure);
	while (FCheckTypedWindowEvent(Dpy, MyWindow, ConfigureNotify, &event))
	{
	  if (!event.xconfigure.send_event &&
	      Event.xconfigure.window != MyWindow)
	    continue;
	  fev_sanitise_configure_notify(&event.xconfigure);
	  Event.xconfigure.x = event.xconfigure.x;
	  Event.xconfigure.y = event.xconfigure.y;
	  Event.xconfigure.send_event = True;
	}
	if (!XGetGeometry(Dpy, MyWindow, &root, &x, &y, &tw, &th,
			  &border_width, &depth))
	{
	  break;
	}
	if (tw != Width || th != Height)
	{
	  Width = tw;
	  Height = th;
	  SetButtonSize(UberButton, Width, Height);
	  button = -1;
	  ub = UberButton;
	  /* what follow can be optimized */
	  while (NextButton(&ub, &b, &button, 0))
	    MakeButton(b);
	  if (FShapesSupported && UberButton->c->flags.b_TransBack)
	  {
	    SetTransparentBackground(UberButton, Width, Height);
	  }
	  for (i = 0; i < nColorsets; i++)
		  change_colorset(i, &Event);
	  if (!UberButton->c->flags.b_Colorset)
	  {
		  XClearWindow(Dpy, MyWindow);
		  RedrawWindow();
	  }
	}
	else if (Event.xconfigure.window == MyWindow &&
		 Event.xconfigure.send_event)
	{
	  update_root_transparency(&Event);
	}
      }
      break;

	case EnterNotify:
		b = handle_new_position(
			b, Event.xcrossing.x, Event.xcrossing.y);
		break;

	case MotionNotify:
		b = handle_new_position(b, Event.xmotion.x, Event.xmotion.y);
		break;

	case LeaveNotify:
		if (
			Event.xcrossing.x < 0 || Event.xcrossing.x >= Width ||
			Event.xcrossing.y < 0 || Event.xcrossing.y >= Height ||

			/* We get LeaveNotify events when the mouse enters
			 * a swallowed window of FvwmButtons, but we're not
			 * interested in these situations. */
			!select_button(UberButton, x, y)->flags.b_Swallow ||

			/* But we're interested in those situations when
			 * the mouse enters a window which overlaps with
			 * the swallowed window. */
			Event.xcrossing.subwindow != None)
		{
			if (ActiveButton)
			{
				b = ActiveButton;
				ActiveButton = NULL;
				RedrawButton(b, DRAW_FORCE, NULL);
			}
			if (CurrentButton)
			{
				RedrawButton(b, DRAW_FORCE, NULL);
			}
		}
		break;

      case KeyPress:
	XLookupString(&Event.xkey, buffer, 10, &keysym, 0);
	if (
		keysym != XK_Return && keysym != XK_KP_Enter &&
		keysym != XK_Linefeed)
	  break;                        /* fall through to ButtonPress */

      case ButtonPress:
	if (Event.xbutton.window == MyWindow)
	{
	  x = Event.xbutton.x;
	  y = Event.xbutton.y;
	}
	else
	{
	  Window dummy;

	  XTranslateCoordinates(
	    Dpy, Event.xbutton.window, MyWindow, Event.xbutton.x,
	    Event.xbutton.y, &x, &y, &dummy);
	}
	if (CurrentButton)
	{
	  b = CurrentButton;
	  CurrentButton = NULL;
	  ActiveButton = select_button(UberButton, x, y);
	  RedrawButton(b, DRAW_FORCE, NULL);
	  if (ActiveButton != b)
	  {
		  RedrawButton(ActiveButton, DRAW_FORCE, NULL);
		  b = ActiveButton;
	  }
	  break;
	}
	if (Event.xbutton.state & DEFAULT_ALL_BUTTONS_MASK)
	{
	  break;
	}

	CurrentButton = b = select_button(UberButton, x, y);
	is_pointer_in_current_button = True;

	act = NULL;
	if (!b->flags.b_Panel &&
	    (!b || !b->flags.b_Action ||
	     ((act = GetButtonAction(b, Event.xbutton.button)) == NULL &&
	      (act = GetButtonAction(b, 0)) == NULL)))
	{
	  CurrentButton = NULL;
	  break;
	}

	/* Undraw ActiveButton (if there is one). */
	if (ActiveButton)
	{
	  /* $b & $ActiveButton are always the same button. */
	  button_info *tmp = ActiveButton;
	  ActiveButton = NULL;
	  RedrawButton(tmp, DRAW_FORCE, NULL);
	}
	else
	  RedrawButton(b, DRAW_FORCE, NULL);
	if (!act)
	{
	  break;
	}
	if (act && !b->flags.b_ActionOnPress &&
	    strncasecmp(act, "popup", 5) != 0)
	{
	  free(act);
	  act = NULL;
	  break;
	}
	else /* i.e. action is Popup */
	{
	  XUngrabPointer(Dpy, CurrentTime); /* And fall through */
	}
	if (act)
	{
	  free(act);
	  act = NULL;
	}
	/* fall through */

      case KeyRelease:
      case ButtonRelease:
	if (CurrentButton == NULL || !is_pointer_in_current_button)
	{
		if (CurrentButton)
		{
			button_info *tmp = CurrentButton;
			CurrentButton = NULL;
			RedrawButton(tmp, DRAW_FORCE, NULL);
		}

		CurrentButton = NULL;
		ActiveButton = b;
		if (ActiveButton)
			RedrawButton(ActiveButton, DRAW_FORCE, NULL);
		break;
	}
	if (Event.xbutton.window == MyWindow)
	{
	  x = Event.xbutton.x;
	  y = Event.xbutton.y;
	}
	else
	{
	  Window dummy;

	  XTranslateCoordinates(
	    Dpy, Event.xbutton.window, MyWindow, Event.xbutton.x,
	    Event.xbutton.y, &x, &y, &dummy);
	}
	b = select_button(UberButton, x, y);
	act = GetButtonAction(b,Event.xbutton.button);

	ButtonPressProcess(b, &act);

	if (act != NULL)
	{
	  free(act);
	  act = NULL;
	}
	b = CurrentButton;
	CurrentButton = NULL;
	ActiveButton = b;
	if (b && !b->flags.b_Hangon)
	  RedrawButton(b, DRAW_FORCE, NULL);
	break;

      case ClientMessage:
	if (Event.xclient.format == 32 &&
	   Event.xclient.data.l[0]==_XA_WM_DEL_WIN)
	{
	  DeadPipe(1);
	}
	break;

      case PropertyNotify:
	if (Event.xany.window == None)
	  break;
	ub = UberButton;button = -1;
	while (NextButton(&ub, &b, &button, 0))
	{
	  Window swin = SwallowedWindow(b);

	  if ((buttonSwallowCount(b) == 3) && Event.xany.window == swin)
	  {
	    if (Event.xproperty.atom == XA_WM_NAME &&
	       buttonSwallow(b)&b_UseTitle)
	    {
	      if (b->flags.b_Title)
		free(b->title);
	      b->flags.b_Title = 1;
	      FlocaleGetNameProperty(XGetWMName, Dpy, swin, &tmp);
	      if (tmp.name != NULL)
	      {
		CopyString(&b->title, tmp.name);
		FlocaleFreeNameProperty(&tmp);
	      }
	      else
	      {
		CopyString(&b->title, "");
	      }
	      MakeButton(b);
	    }
	    else if ((Event.xproperty.atom == XA_WM_NORMAL_HINTS) &&
		    (!(buttonSwallow(b)&b_NoHints)))
	    {
	      long supp;
	      if (!FGetWMNormalHints(Dpy, swin, b->hints, &supp))
		b->hints->flags = 0;
	      MakeButton(b);
	      if (FShapesSupported)
	      {
		if (UberButton->c->flags.b_TransBack)
		{
		  SetTransparentBackground(UberButton, Width, Height);
		}
	      }
	    }
	    RedrawButton(b, DRAW_FORCE, NULL);
	  }
	}
	break;

      case MapNotify:
      case UnmapNotify:

	ub = UberButton;
	button = -1;
	while (NextButton(&ub, &b, &button, 0))
	{
	  if (b->flags.b_Panel && Event.xdestroywindow.window == b->PanelWin)
	  {
	    /* A panel has been unmapped, update the button */
	    b->newflags.panel_mapped = (Event.type == MapNotify);
	    RedrawButton(b, DRAW_FORCE, NULL);
	    break;
	  }
	}
	break;

      case DestroyNotify:
	ub = UberButton;button = -1;
	while (NextButton(&ub, &b, &button, 0))
	{
	  Window swin = SwallowedWindow(b);

	  if ((buttonSwallowCount(b) == 3) && Event.xdestroywindow.window == swin)
	  {
#ifdef DEBUG_HANGON
	    fprintf(stderr,
		    "%s: Button 0x%06x lost its window 0x%x (\"%s\")",
		    MyName, (ushort)b, (ushort)swin, b->hangon);
#endif
	    b->swallow &= ~b_Count;
	    if (b->flags.b_Panel)
	      b->PanelWin = None;
	    else
	      b->IconWin = None;
	    if (buttonSwallow(b)&b_Respawn && b->hangon && b->spawn)
	    {
	      char *p;

#ifdef DEBUG_HANGON
	      fprintf(stderr, ", respawning\n");
#endif
	      if (b->newflags.is_panel && is_transient)
	      {
		/* terminate if transient and a panel has been killed */
		exit(0);
	      }
	      b->swallow |= 1;
	      if (!b->newflags.is_panel)
	      {
		b->flags.b_Swallow = 1;
		b->flags.b_Hangon = 1;
	      }
	      else
	      {
		b->flags.b_Panel = 1;
		b->flags.b_Hangon = 1;
		b->newflags.panel_mapped = 0;
	      }
	      p = module_expand_action(
		      Dpy, screen, b->spawn, NULL, UberButton->c->fore,
		      UberButton->c->back);
	      if (p)
	      {
		exec_swallow(p, b);
		free(p);
	      }
	      RedrawButton(b, DRAW_CLEAN, NULL); /* ? */
	      if (is_transient_panel)
	      {
		XWithdrawWindow(Dpy, MyWindow, screen);
	      }
	    }
	    else if (b->newflags.do_swallow_new && b->hangon && b->spawn &&
		    !b->newflags.is_panel)
	    {
#ifdef DEBUG_HANGON
	      fprintf(stderr, ", waiting for respawned window\n");
#endif
	      b->swallow |= 1;
	      b->flags.b_Swallow = 1;
	      b->flags.b_Hangon = 1;
	      RedrawButton(b, DRAW_CLEAN, NULL);
	      if (is_transient_panel)
	      {
		XWithdrawWindow(Dpy, MyWindow, screen);
	      }
	    }
	    else
	    {
	      b->flags.b_Swallow = 0;
	      b->flags.b_Panel = 0;
	      RedrawButton(b, DRAW_FORCE, NULL);
#ifdef DEBUG_HANGON
	      fprintf(stderr, "\n");
#endif
	    }
	    break;
	  }
	}
	break;

/* End of TO-BE-REFORMATTED-BLOCK */

			default:
#ifdef DEBUG_EVENTS
				fprintf(stderr,
					"%s: Event fell through unhandled\n",
					MyName);
#endif
				break;
			} /* switch */
		} /* event handling */
	} /* while */

}

/**
*** RedrawWindow()
*** Draws the window by traversing the button tree
**/
void RedrawWindow(void)
{
	int button;
	XEvent dummy;
	button_info *ub, *b;
	static Bool initial_redraw = True;

	if (ready < 1)
	{
		return;
	}

	/* Flush expose events */
	while (FCheckTypedWindowEvent(Dpy, MyWindow, Expose, &dummy))
	{
	}

	/* Clean out the entire window first */
	if (initial_redraw == False)
	{
		XClearWindow(Dpy, MyWindow);
	}
	else
	{
		initial_redraw = False;
	}

	button = -1;
	ub = UberButton;
	while (NextButton(&ub, &b, &button, 1))
	{
		RedrawButton(b, DRAW_ALL, NULL);
	}
}

/**
*** LoadIconFile()
**/
int LoadIconFile(const char *s, FvwmPicture **p, int cset)
{
	FvwmPictureAttributes fpa;

	fpa.mask = 0;
	if (cset >= 0 && Colorset[cset].do_dither_icon)
	{
		fpa.mask |= FPAM_DITHER;
	}
	if (UberButton->c->flags.b_TransBack)
	{
		fpa.mask |= FPAM_NO_ALPHA;
	}
	*p = PCacheFvwmPicture(Dpy, MyWindow, imagePath, s, fpa);
	if (*p)
	{
		return 1;
	}
	return 0;
}

/**
*** RecursiveLoadData()
*** Loads colors, fonts and icons, and calculates buttonsizes
**/
void RecursiveLoadData(button_info *b, int *maxx, int *maxy)
{
  int i, x = 0, y = 0, ix, iy, tx, ty, hix, hiy, htx, hty, pix, piy, ptx, pty;
  FlocaleFont *Ffont;

  if (!b)
    return;

#ifdef DEBUG_LOADDATA
  fprintf(stderr, "%s: Loading: Button 0x%06x: colors", MyName, (ushort)b);
#endif


  /* initialise button colours and background */
  if (b->flags.b_Colorset)
  {
    int cset = b->colorset;

    /* override normal icon options */
    if (b->flags.b_IconBack && !b->flags.b_TransBack)
    {
      free(b->back);
    }
    b->flags.b_IconBack = 0;
    b->flags.b_TransBack = 0;

    /* fetch the colours from the colorset */
    b->fc = Colorset[cset].fg;
    b->bc = Colorset[cset].bg;
    b->hc = Colorset[cset].hilite;
    b->sc = Colorset[cset].shadow;
    if (Colorset[cset].pixmap != None)
    {
      /* we have a pixmap */
      b->backicon = NULL;
      b->flags.b_IconBack = 1;
    }
  }
  else /* no colorset */
  {
    b->colorset = -1;
    if (b->flags.b_Fore)
      b->fc = GetColor(b->fore);
    if (b->flags.b_Back)
    {
      b->bc = GetColor(b->back);
      b->hc = GetHilite(b->bc);
      b->sc = GetShadow(b->bc);
      if (b->flags.b_IconBack)
      {
	if (!LoadIconFile(b->back, &b->backicon, b->colorset))
	  b->flags.b_Back = 0;
      }
    } /* b_Back */
  } /* !b_Colorset */

  /* initialise container colours and background */
  if (b->flags.b_Container)
  {
    if (b->c->flags.b_Colorset)
    {
      int cset = b->c->colorset;

      /* override normal icon options */
      if (b->c->flags.b_IconBack && !b->c->flags.b_TransBack)
      {
	free(b->c->back_file);
      }
      b->c->flags.b_IconBack = 0;
      b->c->flags.b_TransBack = 0;

      /* fetch the colours from the colorset */
      b->c->fc = Colorset[cset].fg;
      b->c->bc = Colorset[cset].bg;
      b->c->hc = Colorset[cset].hilite;
      b->c->sc = Colorset[cset].shadow;
      if (Colorset[cset].pixmap != None)
      {
	/* we have a pixmap */
	b->c->backicon = NULL;
	b->c->flags.b_IconBack = 1;
      }
    }
    else /* no colorset */
    {
       b->c->colorset = -1;
#ifdef DEBUG_LOADDATA
      fprintf(stderr, ", colors2");
#endif
      if (b->c->flags.b_Fore)
	b->c->fc = GetColor(b->c->fore);
      if (b->c->flags.b_Back)
      {
	b->c->bc = GetColor(b->c->back);
	b->c->hc = GetHilite(b->c->bc);
	b->c->sc = GetShadow(b->c->bc);
	if (b->c->flags.b_IconBack && !b->c->flags.b_TransBack)
	{
	  if (!LoadIconFile(b->c->back_file, &b->c->backicon, -1))
	    b->c->flags.b_IconBack = 0;
	}
      }
    } /* !b_Colorset */
  } /* b_Container */



  /* Load the font */
  if (b->flags.b_Font)
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr, ", font \"%s\"", b->font_string);
#endif

    if (strncasecmp(b->font_string, "none", 4) == 0)
    {
      b->Ffont = NULL;
    }
    else if (!(b->Ffont = FlocaleLoadFont(Dpy, b->font_string, MyName)))
    {
      b->flags.b_Font = 0;
    }
  }

  if (b->flags.b_Container && b->c->flags.b_Font)
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr, ", font2 \"%s\"", b->c->font_string);
#endif
    if (strncasecmp(b->c->font_string, "none", 4) == 0)
    {
      b->c->Ffont = NULL;
    }
    else if (!(b->c->Ffont = FlocaleLoadFont(Dpy, b->c->font_string, MyName)))
    {
      b->c->flags.b_Font = 0;
    }
  }

  /* Calculate subbutton sizes */
  if (b->flags.b_Container && b->c->num_buttons)
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr, ", entering container\n");
#endif
    for (i = 0; i < b->c->num_buttons; i++)
      if (b->c->buttons[i])
	RecursiveLoadData(b->c->buttons[i], &x, &y);

    if (b->c->flags.b_Size)
    {
      x = b->c->minx;
      y = b->c->miny;
    }
#ifdef DEBUG_LOADDATA
    fprintf(stderr, "%s: Loading: Back to container 0x%06x", MyName, (ushort)b);
#endif

    x *= b->c->num_columns;
    y *= b->c->num_rows;
    b->c->width = x;
    b->c->height = y;
  }

  /* $ix & $iy are dimensions of Icon
     $tx & $ty are dimensions of Title
     $hix & $hiy are dimensions of ActiveIcon
     $htx & $hty are dimensions of ActiveTitle
     $pix & $piy are dimensions of PressIcon
     $ptx & $pty are dimensions of PressTitle

     Note that if No ActiveIcon is specified, Icon is displayed during hover.
     Similarly for ActiveTitle, PressIcon & PressTitle. */
  ix = iy = tx = ty = hix = hiy = htx = hty = pix = piy = ptx = pty = 0;

  /* Load the icon */
  if (
	  b->flags.b_Icon &&
	  LoadIconFile(b->icon_file, &b->icon, buttonColorset(b)))
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr, ", icon \"%s\"", b->icon_file);
#endif
    ix = b->icon->width;
    iy = b->icon->height;
  }
  else
    b->flags.b_Icon = 0;

  /* load the active icon. */
  if (b->flags.b_ActiveIcon &&
      LoadIconFile(b->active_icon_file, &b->activeicon, buttonColorset(b)))
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr, ", active icon \"%s\"", b->active_icon_file);
#endif

    hix = b->activeicon->width;
    hiy = b->activeicon->height;
  }
  else
  {
    hix = ix;
    hiy = iy;
    b->flags.b_ActiveIcon = 0;
  }

  /* load the press icon. */
  if (b->flags.b_PressIcon &&
      LoadIconFile(b->press_icon_file, &b->pressicon, buttonColorset(b)))
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr, ", press icon \"%s\"", b->press_icon_file);
#endif

    pix = b->pressicon->width;
    piy = b->pressicon->height;
  }
  else
  {
    pix = ix;
    piy = iy;
    b->flags.b_PressIcon = 0;
  }

  /* calculate Title dimensions. */
  if (b->flags.b_Title && (Ffont = buttonFont(b)))
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr, ", title \"%s\"", b->title);
#endif
    if (buttonJustify(b)&b_Horizontal)
    {
      tx = buttonXPad(b) + FlocaleTextWidth(Ffont, b->title, strlen(b->title));
      ty = Ffont->height;
    }
    else
    {
      tx = FlocaleTextWidth(Ffont, b->title, strlen(b->title));
      ty = Ffont->height;
    }
  }

  /* calculate ActiveTitle dimensions. */
  if (b->flags.b_ActiveTitle && (Ffont = buttonFont(b)))
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr, ", title \"%s\"", b->title);
#endif
    if (buttonJustify(b) & b_Horizontal)
    {
      htx = buttonXPad(b) + FlocaleTextWidth(Ffont, b->activeTitle,
	      strlen(b->activeTitle));
      hty = Ffont->height;
    }
    else
    {
      htx = FlocaleTextWidth(Ffont, b->activeTitle, strlen(b->activeTitle));
      hty = Ffont->height;
    }
  }
  else
  {
    htx = tx;
    hty = ty;
  }

  /* calculate PressTitle dimensions. */
  if (b->flags.b_PressTitle && (Ffont = buttonFont(b)))
  {
#ifdef DEBUG_LOADDATA
    fprintf(stderr, ", title \"%s\"", b->title);
#endif
    if (buttonJustify(b) & b_Horizontal)
    {
      ptx = buttonXPad(b) + FlocaleTextWidth(Ffont, b->pressTitle,
	      strlen(b->pressTitle));
      pty = Ffont->height;
    }
    else
    {
      ptx = FlocaleTextWidth(Ffont, b->pressTitle, strlen(b->pressTitle));
      pty = Ffont->height;
    }
  }
  else
  {
    ptx = tx;
    pty = ty;
  }

  x += max(max(ix, tx), max(hix, htx));
  y += max(iy + ty, hiy + hty);

  if (b->flags.b_Size)
  {
    x = b->minx;
    y = b->miny;
  }

  x += 2 * (buttonFrame(b) + buttonXPad(b));
  y += 2 * (buttonFrame(b) + buttonYPad(b));

  x /= b->BWidth;
  y /= b->BHeight;

  *maxx = max(x, *maxx);
  *maxy = max(y, *maxy);
#ifdef DEBUG_LOADDATA
  fprintf(stderr, ", size %ux%u, done\n", x, y);
#endif
}


static void HandlePanelPress(button_info *b)
{
  int x1, y1, w1, h1;
  int x2, y2, w2, h2;
  int px, py, pw, ph, pbw;
  int ax, ay, aw, ah, abw;
  int tb, bb, lb, rb;
  int steps = b->slide_steps;
  int i;
  unsigned int d;
  Window JunkW;
  Window ancestor;
  XSizeHints mysizehints;
  long supplied;
  Bool is_mapped;
  XWindowAttributes xwa;
  char cmd[64];
  button_info *tmp;

  if (XGetWindowAttributes(Dpy, b->PanelWin, &xwa))
    is_mapped = (xwa.map_state == IsViewable);
  else
    is_mapped = False;

  /* force StaticGravity on window */
  mysizehints.flags = 0;
  FGetWMNormalHints(Dpy, b->PanelWin, &mysizehints, &supplied);
  mysizehints.flags |= PWinGravity;
  mysizehints.win_gravity = StaticGravity;
  XSetWMNormalHints(Dpy, b->PanelWin, &mysizehints);
  if (is_mapped &&
      XGetGeometry(Dpy, b->PanelWin, &JunkW, &b->x, &b->y, &b->w, &b->h,
		   &b->bw, &d))
  {
    /* get the current geometry */
    XTranslateCoordinates(
      Dpy, b->PanelWin, Root, b->x, b->y, &b->x, &b->y, &JunkW);
  }
  else
  {
    XWMHints wmhints;

    /* Make sure the icon is unmapped first. Needed to work properly with
     * shaded and iconified windows. */
    XWithdrawWindow(Dpy, b->PanelWin, screen);

    /* now request mapping in the void and wait until it gets mapped */
    mysizehints.flags = 0;
    FGetWMNormalHints(Dpy, b->PanelWin, &mysizehints, &supplied);
    mysizehints.flags |= USPosition;
    /* hack to prevent mapping panels on wrong screen with StartsOnScreen */
    FScreenMangleScreenIntoUSPosHints(FSCREEN_XYPOS, &mysizehints);
    XSetWMNormalHints(Dpy, b->PanelWin, &mysizehints);
    /* make sure its not mapped as an icon */
    wmhints.flags = StateHint;
    wmhints.initial_state = NormalState;
    XSetWMHints(Dpy, b->PanelWin, &wmhints);

    /* map the window in the void */
    XMoveWindow(Dpy, b->PanelWin, 32767, 32767);
    XMapWindow(Dpy, b->PanelWin);
    XSync(Dpy, 0);

    /* give the X server the CPU to do something */
    usleep(1000);

    /* wait until it appears or one second has passed */
    for (i = 0; i < 10; i++)
    {
      if (!XGetWindowAttributes(Dpy, b->PanelWin, &xwa))
      {
	/* the window has been destroyed */
	XUnmapWindow(Dpy, b->PanelWin);
	XSync(Dpy, 0);
	RedrawButton(b, DRAW_CLEAN, NULL);
	return;
      }
      if (xwa.map_state == IsViewable)
      {
	/* okay, it's mapped */
	break;
      }
      /* still unmapped wait 0.1 seconds and try again */
      XSync(Dpy, 0);
      usleep(100000);
    }
    if (xwa.map_state != IsViewable)
    {
      /* give up after one second */
      XUnmapWindow(Dpy, b->PanelWin);
      XSync(Dpy, 0);
      RedrawButton(b, DRAW_FORCE, NULL);
      return;
    }
  }

  /* We're sure that the window is mapped and decorated now. Find the top level
   * ancestor now. */
  MyXGrabServer(Dpy);
  lb = 0;
  tb = 0;
  rb = 0;
  bb = 0;
  if (!b->panel_flags.ignore_lrborder || !b->panel_flags.ignore_tbborder)
  {
    ancestor = GetTopAncestorWindow(Dpy, b->PanelWin);
    if (ancestor)
    {
      if (
	XGetGeometry(Dpy, ancestor, &JunkW, &ax, &ay, (unsigned int *)&aw,
		     (unsigned int *)&ah, (unsigned int *)&abw, &d) &&
	XGetGeometry(Dpy, b->PanelWin, &JunkW, &px, &py, (unsigned int *)&pw,
		     (unsigned int *)&ph, (unsigned int *)&pbw, &d) &&
	XTranslateCoordinates(
	  Dpy, b->PanelWin, ancestor, 0, 0, &px, &py, &JunkW))
      {
	/* calculate the 'border' width in all four directions */
	if (!b->panel_flags.ignore_lrborder)
	{
	  lb = max(px, 0) + abw;
	  rb = max((aw + abw) - (px + pw), 0) + abw;
	}
	if (!b->panel_flags.ignore_tbborder)
	{
	  tb = max(py, 0) + abw;
	  bb = max((ah + abw) - (py + ph), 0) + abw;
	}
      }
    }
  }
  /* now find the source and destination positions for the slide operation */
  GetPanelGeometry(is_mapped, b, lb, tb, rb, bb, &x1, &y1, &w1, &h1);
  GetPanelGeometry(!is_mapped, b, lb, tb, rb, bb, &x2, &y2, &w2, &h2);
  MyXUngrabServer(Dpy);

  /* to force fvwm to map the window where we want */
  if (is_mapped)
  {
    /* don't slide the window if it has been moved or resized */
    if (b->x != x1 || b->y != y1 || b->w != w1 || b->h != h1)
    {
      steps = 0;
    }
  }

  /* redraw our button */
  b->newflags.panel_mapped = !is_mapped;

  /* make sure the window maps on the current desk */
  sprintf(cmd, "Silent WindowId 0x%08x (!Sticky) MoveToDesk 0",
	  (int)b->PanelWin);
  SendInfo(fd, cmd, b->PanelWin);
  SlideWindow(Dpy, b->PanelWin,
	      x1, y1, w1, h1,
	      x2, y2, w2, h2,
	      steps, b->slide_delay_ms, NULL, b->panel_flags.smooth,
	      (b->swallow&b_NoHints) ? 0 : 1);
  if (is_mapped)
  {
    XUnmapWindow(Dpy, b->PanelWin);
  }
  tmp = CurrentButton;
  CurrentButton = NULL;
  RedrawButton(b, DRAW_FORCE, NULL);
  CurrentButton = tmp;
  XSync(Dpy, 0);
  /* Give fvwm a chance to update the window.  Otherwise the window may end up
   * too small.  This doesn't prevent this completely, but makes it much less
   * likely. */
  usleep(250000);

  return;
}


/**
*** CreateUberButtonWindow()
*** Sizes and creates the window
**/
void CreateUberButtonWindow(button_info *ub, int maxx, int maxy)
{
	XSizeHints mysizehints;
	XGCValues gcv;
	unsigned long gcm;
	XClassHint myclasshints;
	int wx;
	int wy;

	x = UberButton->x; /* Geometry x where to put the panel */
	y = UberButton->y; /* Geometry y where to put the panel */
	xneg = UberButton->w;
	yneg = UberButton->h;

	if (maxx < 16)
	{
		maxx = 16;
	}
	if (maxy < 16)
	{
		maxy = 16;
	}

#ifdef DEBUG_INIT
	fprintf(stderr, "making atoms...");
#endif

	_XA_WM_DEL_WIN = XInternAtom(Dpy, "WM_DELETE_WINDOW", 0);

#ifdef DEBUG_INIT
	fprintf(stderr, "sizing...");
#endif

	mysizehints.flags = PWinGravity | PBaseSize;
	mysizehints.base_width  = 0;
	mysizehints.base_height = 0;
	mysizehints.width  = mysizehints.base_width + maxx;
	mysizehints.height = mysizehints.base_height + maxy;

	if (w>-1) /* from geometry */
	{
#ifdef DEBUG_INIT
		fprintf(stderr, "constraining (w=%i)...", w);
#endif
		ConstrainSize(&mysizehints, &w, &h);
		mysizehints.width = w;
		mysizehints.height = h;
		mysizehints.flags |= USSize;
	}

#ifdef DEBUG_INIT
	fprintf(stderr, "gravity...");
#endif
	wx = 0;
	wy = 0;
	if (x > -100000)
	{
		if (xneg)
		{
			wx = DisplayWidth(Dpy, screen) + x - mysizehints.width;
			gravity = NorthEastGravity;
		}
		else
		{
			wx = x;
		}
		if (yneg)
		{
			wy = DisplayHeight(Dpy, screen) + y - mysizehints.height;
			gravity = SouthWestGravity;
		}
		else
		{
			wy = y;
		}
		if (xneg && yneg)
		{
			gravity = SouthEastGravity;
		}
		mysizehints.flags |= USPosition;

		/* hack to prevent mapping panels on wrong screen with
		 * StartsOnScreen */
		FScreenMangleScreenIntoUSPosHints(FSCREEN_XYPOS, &mysizehints);
	}
	mysizehints.win_gravity = gravity;

#ifdef DEBUG_INIT
	if (mysizehints.flags&USPosition)
	{
		fprintf(stderr,
			"create(%i,%i,%u,%u,1,%u,%u)...",
			wx, wy, mysizehints.width, mysizehints.height,
			(ushort)fore_pix, (ushort)back_pix);
	}
	else
	{
		fprintf(stderr,
			"create(-,-,%u,%u,1,%u,%u)...",
			mysizehints.width, mysizehints.height,
		(ushort)fore_pix, (ushort)back_pix);
	}
#endif

	XMoveResizeWindow(
		Dpy, MyWindow, wx, wy, mysizehints.width, mysizehints.height);

	XSetWMNormalHints(Dpy, MyWindow, &mysizehints);

	if (is_transient)
	{
		XSetTransientForHint(Dpy, MyWindow, Root);
	}

#ifdef DEBUG_INIT
	fprintf(stderr, "colors...");
#endif
	if (Pdepth < 2)
	{
		back_pix = GetColor("white");
		fore_pix = GetColor("black");
		hilite_pix = back_pix;
		shadow_pix = fore_pix;
	}
	else
	{
		fore_pix = ub->c->fc;
		back_pix = ub->c->bc;
		hilite_pix = ub->c->hc;
		shadow_pix = ub->c->sc;
	}


#ifdef DEBUG_INIT
	fprintf(stderr, "properties...");
#endif
	XSetWMProtocols(Dpy, MyWindow, &_XA_WM_DEL_WIN, 1);

	myclasshints.res_name = xstrdup(MyName);
	myclasshints.res_class = xstrdup("FvwmButtons");

	{
		XTextProperty mynametext;
		char *list[] = { NULL, NULL };
		list[0] = MyName;
		if (!XStringListToTextProperty(list, 1, &mynametext))
		{
			fprintf(stderr,
				"%s: Failed to convert name to XText\n",
				MyName);
			exit(1);
		}
		XSetWMProperties(
			Dpy, MyWindow, &mynametext, &mynametext, NULL, 0,
			&mysizehints, NULL, &myclasshints);
		XFree(mynametext.value);
	}

	XSelectInput(Dpy, MyWindow, MW_EVENTS);

#ifdef DEBUG_INIT
	fprintf(stderr, "GC...");
#endif
	gcm = GCForeground|GCBackground;
	gcv.foreground = fore_pix;
	gcv.background = back_pix;
	if (ub && ub->c && ub->c->Ffont &&  ub->c->Ffont->font && ub->Ffont)
	{
		gcv.font = ub->c->Ffont->font->fid;
		gcm |= GCFont;
	}
	NormalGC = fvwmlib_XCreateGC(Dpy, MyWindow, gcm, &gcv);
	gcv.foreground = shadow_pix;
	gcv.background = fore_pix;
	ShadowGC = fvwmlib_XCreateGC(Dpy, MyWindow, gcm, &gcv);

	if (ub->c->flags.b_Colorset)
	{
		SetWindowBackground(
			Dpy, MyWindow, mysizehints.width, mysizehints.height,
			&Colorset[ub->c->colorset], Pdepth, NormalGC, True);
	}
	else if (ub->c->flags.b_IconBack && !ub->c->flags.b_TransBack)
	{
		XSetWindowBackgroundPixmap(
			Dpy, MyWindow, ub->c->backicon->picture);
	}
	else
	{
		XSetWindowBackground(Dpy, MyWindow, back_pix);
	}

	free(myclasshints.res_class);
	free(myclasshints.res_name);
}

/* --------------------------------------------------------------------------*/

#ifdef DEBUG_EVENTS
void DebugEvents(XEvent *event)
{
	char *event_names[] =
	{
		NULL,
		NULL,
		"KeyPress",
		"KeyRelease",
		"ButtonPress",
		"ButtonRelease",
		"MotionNotify",
		"EnterNotify",
		"LeaveNotify",
		"FocusIn",
		"FocusOut",
		"KeymapNotify",
		"Expose",
		"GraphicsExpose",
		"NoExpose",
		"VisibilityNotify",
		"CreateNotify",
		"DestroyNotify",
		"UnmapNotify",
		"MapNotify",
		"MapRequest",
		"ReparentNotify",
		"ConfigureNotify",
		"ConfigureRequest",
		"GravityNotify",
		"ResizeRequest",
		"CirculateNotify",
		"CirculateRequest",
		"PropertyNotify",
		"SelectionClear",
		"SelectionRequest",
		"SelectionNotify",
		"ColormapNotify",
		"ClientMessage",
		"MappingNotify"
	};
	fprintf(stderr, "%s: Received %s event from window 0x%x\n",
		MyName, event_names[event->type], (ushort)event->xany.window);
}
#endif

/**
*** My_FNextEvent()
*** Waits for next X event, or for an auto-raise timeout.
**/
int My_FNextEvent(Display *Dpy, XEvent *event)
{
	fd_set in_fdset;
	static int miss_counter = 0;
	static Bool fsm_pending = False;
	struct timeval tv;

	if (FPending(Dpy))
	{
		FNextEvent(Dpy, event);
#ifdef DEBUG_EVENTS
		DebugEvents(event);
#endif
		return 1;
	}

	FD_ZERO(&in_fdset);
	FD_SET(x_fd, &in_fdset);
	FD_SET(fd[1], &in_fdset);
	fsm_fdset(&in_fdset);

	if (fsm_pending)
	{
		tv.tv_sec  = 0;
		tv.tv_usec = 10000; /* 10 ms */
	}

	if (fvwmSelect(
		    fd_width, SELECT_FD_SET_CAST &in_fdset, 0, 0,
		    (fsm_pending)? &tv:NULL) > 0)
	{
		if (FD_ISSET(x_fd, &in_fdset))
		{
			if (FPending(Dpy))
			{
				FNextEvent(Dpy, event);
				miss_counter = 0;
#ifdef DEBUG_EVENTS
				DebugEvents(event);
#endif
				return 1;
			}
			else
			{
				miss_counter++;
			}
			if (miss_counter > 100)
			{
				DeadPipe(0);
			}
		}

		if (FD_ISSET(fd[1], &in_fdset))
		{
			FvwmPacket* packet = ReadFvwmPacket(fd[1]);
			if ( packet == NULL )
			{
				DeadPipe(0);
			}
			else
			{
				process_message( packet->type, packet->body );
			}
		}

		fsm_pending = fsm_process(&in_fdset);

	}
	else
	{
		if (fsm_pending)
		{
			fsm_pending = fsm_process(&in_fdset);
		}
	}
	return 0;
}

/**
*** SpawnSome()
*** Is run after the windowlist is checked. If any button hangs on UseOld,
*** it has failed, so we try to spawn a window for them.
**/
void SpawnSome(void)
{
	static char first = 1;
	button_info *b, *ub = UberButton;
	int button = -1;
	char *p;

	if (!first)
	{
		return;
	}
	first = 0;
	while (NextButton(&ub, &b, &button, 0))
	{
		if ((buttonSwallowCount(b) == 1) && b->flags.b_Hangon &&
			(buttonSwallow(b)&b_UseOld))
		{
			if (b->spawn)
			{
#ifdef DEBUG_HANGON
				fprintf(stderr,
					"%s: Button 0x%06x did not find a "
					"\"%s\" window, %s",
					MyName, (ushort)b, b->hangon,
					"spawning own\n");
#endif
				p = module_expand_action(
					Dpy, screen, b->spawn, NULL,
					UberButton->c->fore,
					UberButton->c->back);
				if (p)
				{
					exec_swallow(p, b);
					free(p);
				}
			}
		}
	}
}

/* This function is a real hack. It forces our nice background upon the
 * windows of the swallowed application! */
void change_swallowed_window_colorset(button_info *b, Bool do_clear)
{
	Window w = SwallowedWindow(b);
	Window *children = None;
	int nchildren;

	nchildren = GetEqualSizeChildren(
		Dpy, w, Pdepth, XVisualIDFromVisual(Pvisual), Pcmap, &children);
	SetWindowBackground(
		Dpy, w, buttonWidth(b), buttonHeight(b),
		&Colorset[b->colorset], Pdepth, NormalGC, do_clear);
	while (nchildren-- > 0)
	{
		SetWindowBackground(
			Dpy, children[nchildren], buttonWidth(b),
			buttonHeight(b), &Colorset[b->colorset], Pdepth,
			NormalGC, do_clear);
	}
	if (children)
	{
		XFree(children);
	}

	return;
}

static void send_bg_change_to_module(button_info *b, XEvent *Event)
{
	int bx, by, bpx, bpy, bf;

	if (Event != NULL)
	{
		buttonInfo(b, &bx, &by, &bpx, &bpy, &bf);
		Event->xconfigure.x = UberButton->x + bx;
		Event->xconfigure.y = UberButton->y + by;
		Event->xconfigure.width = b->icon_w;
		Event->xconfigure.height = b->icon_h;
		Event->xconfigure.window = SwallowedWindow(b);
		FSendEvent(Dpy, SwallowedWindow(b), False, NoEventMask, Event);
	}
	else
	{
		char cmd[256];
		sprintf(cmd, "PropertyChange %u %u %lu",
			MX_PROPERTY_CHANGE_BACKGROUND, 0, SwallowedWindow(b));
		SendText(fd, cmd, 0);
	}
}

static void update_root_transparency(XEvent *Event)
{
	button_info *ub, *b;
	int button;
	Bool bg_is_transparent = CSET_IS_TRANSPARENT(UberButton->c->colorset);

	if (bg_is_transparent)
	{
		SetWindowBackground(
			Dpy, MyWindow, UberButton->c->width,
			UberButton->c->height,
			&Colorset[UberButton->c->colorset],
			Pdepth, NormalGC, True);
		/* the window is redraw by exposure. It is very difficult
		 * the clear only the needed part and then redraw the
		 * cleared buttons (pbs with containers which contain containers
		 * ...) */
	}

	if (!bg_is_transparent)
	{
		/* update root transparent button (and its childs) */
		ub = UberButton;
		button = -1;
		while (NextButton(&ub, &b, &button, 1))
		{
			button_info *tmpb;
			Bool redraw = False;

			tmpb = b;
			/* see if _a_ parent (!= UberButton) is root trans */
			while (tmpb->parent != NULL && !redraw)
			{
				if (CSET_IS_TRANSPARENT_ROOT(
					buttonColorset(tmpb)))
				{
					redraw = True;
				}
				tmpb = tmpb->parent;
			}
			if (redraw)
			{
				RedrawButton(b, DRAW_ALL, NULL);
			}
		}
	}


	/* Handle the swallowed window */
	ub = UberButton;
	button = -1;
	while (NextButton(&ub, &b, &button, 0))
	{
		if (buttonSwallowCount(b) == 3 && b->flags.b_Swallow)
		{
			/* should handle 2 cases: module or not */
			if (b->swallow & b_FvwmModule)
			{
				if (bg_is_transparent)
				{
					XSync(Dpy, 0);
					send_bg_change_to_module(b, Event);
				}
			}
			else if (b->flags.b_Colorset &&
				 CSET_IS_TRANSPARENT(b->colorset))
			{
				change_swallowed_window_colorset(b, True);
			}
		}
	}
}


static void recursive_change_colorset(
	container_info *c, int colorset, XEvent *Event)
{
	int button;
	button_info *ub, *b;
	button_info *tmpb;

	button = -1;
	ub = UberButton;
	while (NextButton(&ub, &b, &button, 1))
	{
		Bool redraw = False;

		tmpb = b;
		/* see if _a_ parent (!= UberButton) has colorset as cset */
		while (tmpb->parent != NULL && !redraw)
		{
			if (buttonColorset(tmpb) == colorset)
			{
				redraw = True;
			}
			tmpb = tmpb->parent;
		}
		if (!redraw)
		{
			continue;
		}
		if (b->flags.b_Swallow)
		{
			/* swallowed window */
			if (buttonSwallowCount(b) == 3 && b->IconWin != None)
			{
				RedrawButton(b, DRAW_ALL, NULL);
				if (b->flags.b_Colorset &&
				    !(b->swallow&b_FvwmModule))
				{
					change_swallowed_window_colorset(
						b, True);
				}
			}
		}
		else
		{
			RedrawButton(b, DRAW_ALL, NULL);
		}
	}

	return;
}

static void change_colorset(int colorset, XEvent *Event)
{
	if (UberButton->c->flags.b_Colorset &&
	    colorset == UberButton->c->colorset)
	{
		button_info *ub, *b;
		int button;

		SetWindowBackground(
			Dpy, MyWindow, UberButton->c->width,
			UberButton->c->height,
			&Colorset[colorset], Pdepth, NormalGC, True);

		/* some change are done by exposing. Do the other one */
		button = -1;
		ub = UberButton;
		while (NextButton(&ub, &b, &button, 0))
		{
			if (b->flags.b_Swallow && buttonSwallowCount(b) == 3)
			{
				if (b->swallow&b_FvwmModule)
				{
					/* the bg has changed send the info to
					 * modules */
					XSync(Dpy, 0);
					send_bg_change_to_module(b, Event);
				}
				else if (CSET_IS_TRANSPARENT_PR(b->colorset) &&
					!buttonBackgroundButton(b->parent, NULL))
				{
					/* the swallowed app has a ParentRel bg
					 * and the bg of the button is the bg */
					change_swallowed_window_colorset(
						b, True);
				}
			}
		}
		return;
	}

	recursive_change_colorset(
		UberButton->c, colorset, Event);
}

static void handle_config_info_packet(unsigned long *body)
{
	char *tline, *token;
	int colorset;

	tline = (char*)&(body[3]);
	token = PeekToken(tline, &tline);
	if (StrEquals(token, "Colorset"))
	{
		colorset = LoadColorset(tline);
		change_colorset(colorset, NULL);
	}
	else if (StrEquals(token, XINERAMA_CONFIG_STRING))
	{
		FScreenConfigureModule(tline);
	}
	return;
}


/**
*** process_message()
*** Process window list messages
**/
void process_message(unsigned long type, unsigned long *body)
{
	struct ConfigWinPacket  *cfgpacket;

	switch (type)
	{
	case M_NEW_DESK:
		new_desk = body[0];
		{
			int button = -1;
			button_info *b, *ub;

			ub = UberButton;
			while (NextButton(&ub, &b, &button, 1))
			{
				RedrawButton(b, DRAW_FORCE, NULL);
			}
		    }
		break;
	case M_END_WINDOWLIST:
		SpawnSome();
		break;
	case M_MAP:
		swallow(body);
		break;
	case M_RES_NAME:
	case M_RES_CLASS:
	case M_WINDOW_NAME:
		CheckForHangon(body);
		break;
	case M_CONFIG_INFO:
		handle_config_info_packet((unsigned long*)body);
		break;
	case M_ADD_WINDOW:
	case M_CONFIGURE_WINDOW:
		cfgpacket = (void *) body;
		if (cfgpacket->w == MyWindow)
		{
			button_lborder = button_rborder = button_tborder
				= button_bborder = cfgpacket->border_width;
			switch (GET_TITLE_DIR(cfgpacket))
			{
			case DIR_N:
				button_tborder += cfgpacket->title_height;
				break;
			case DIR_S:
				button_bborder += cfgpacket->title_height;
				break;
			case DIR_W:
				button_lborder += cfgpacket->title_height;
				break;
			case DIR_E:
				button_rborder += cfgpacket->title_height;
				break;
			}
		}
		break;
	case MX_PROPERTY_CHANGE:
		if (body[0] == MX_PROPERTY_CHANGE_BACKGROUND &&
			((!swallowed && body[2] == 0)
			|| (swallowed && body[2] == MyWindow)))
		{
			if (CSET_IS_TRANSPARENT_PR(UberButton->c->colorset))
			{
				update_root_transparency(NULL);
			}
		}
		else if (body[0] == MX_PROPERTY_CHANGE_SWALLOW
			&& body[2] == MyWindow)
		{
			char *str;
			unsigned long u;
			Window s, swin;
			char cmd[256];
			button_info *b, *ub = UberButton;
			int button = -1;

			str = (char *)&body[3];
			swallowed = body[1];
			if (swallowed && str && sscanf(str, "%lu", &u) == 1)
			{
				swallower_win = (Window)u;
			}
			else
			{
				swallower_win = 0;
			}
			/* update the swallower */
			s = ((swallower_win && swallowed) ? swallower_win :
				MyWindow);
			while (NextButton(&ub, &b, &button, 0))
			{
				swin = SwallowedWindow(b);
				if ((buttonSwallowCount(b) == 3) && swin)
				{
					sprintf(cmd,
						"PropertyChange %u %u %lu %lu",
						MX_PROPERTY_CHANGE_SWALLOW, 1,
						swin, s);
					SendText(fd, cmd, 0);
				}
			}
		}
		break;
	case M_STRING:
		parse_message_line((char *)&body[3]);
		break;
	default:
		break;
	}
}


/* --------------------------------- swallow code -------------------------- */

/**
*** CheckForHangon()
*** Is the window here now?
**/
void CheckForHangon(unsigned long *body)
{
	button_info *b, *ub = UberButton;
	int button = -1;
	ushort d;
	char *cbody;
	cbody = (char *)&body[3];

	while (NextButton(&ub, &b, &button, 0))
	{
		if (b->flags.b_Hangon && matchWildcards(b->hangon, cbody))
		{
			/* Is this a swallowing button in state 1? */
			if (buttonSwallowCount(b) == 1)
			{
				b->swallow &= ~b_Count;
				b->swallow |= 2;

				/* must grab the server here to make sure
				 * the window is not swallowed by some
				 * other application. */
				if (b->flags.b_Panel)
				{
					b->PanelWin = (Window)body[0];
				}
				else
				{
					b->IconWin = (Window)body[0];
				}
				b->flags.b_Hangon = 0;

				/* We get the parent of the window to compare
				 * with later... */

				b->IconWinParent =
					GetRealGeometry(
						Dpy, SwallowedWindow(b),
						&b->x, &b->y, &b->w, &b->h,
						&b->bw, &d);

#ifdef DEBUG_HANGON
				fprintf(stderr,
					"%s: Button 0x%06x %s 0x%lx \"%s\", "
					"parent 0x%lx\n", MyName, (ushort)b,
					"will swallow window", body[0], cbody,
					b->IconWinParent);
#endif

				if (buttonSwallow(b)&b_UseOld)
				{
					swallow(body);
				}
			}
			else
			{
				/* Else it is an executing button with a
				 * confirmed kill */

#ifdef DEBUG_HANGON
				fprintf(stderr,
					"%s: Button 0x%06x %s 0x%lx "\"%s\", "
					"released\n", MyName, (int)b,
					"hung on window", body[0], cbody);
#endif
				b->flags.b_Hangon = 0;
				free(b->hangon);
				b->hangon = NULL;
				RedrawButton(b, DRAW_FORCE, NULL);
			}
			break;
		}
		else if (buttonSwallowCount(b) >= 2
			&& (Window)body[0] == SwallowedWindow(b))
		{
			/* This window has already been swallowed by
			 * someone else! */
			break;
		}
	}
}

/**
*** GetRealGeometry()
*** Traverses window tree to find the real x,y of a window. Any simpler?
*** Returns parent window, or None if failed.
**/
Window GetRealGeometry(
	Display *dpy, Window win, int *x, int *y, unsigned int *w,
	unsigned int *h, unsigned int *bw, unsigned int *d)
{
	Window root;
	Window rp = None;
	Window *children = None;
	unsigned int n;

	if (!XGetGeometry(dpy, win, &root, x, y, w, h, bw, d))
	{
		return None;
	}

	/* Now, x and y are not correct. They are parent relative, not root...
	 * Hmm, ever heard of XTranslateCoordinates!? */
	XTranslateCoordinates(dpy, win, root, *x, *y, x, y, &rp);
	if (!XQueryTree(dpy, win, &root, &rp, &children, &n))
	{
		return None;
	}
	if (children)
	{
		XFree(children);
	}

	return rp;
}

static void GetPanelGeometry(
	Bool get_big, button_info *b, int lb, int tb, int rb, int bb,
	int *x, int *y, int *w, int *h)
{
	int bx, by, bw, bh;
	int  ftb = 0, fbb = 0, frb = 0, flb = 0;
	Window JunkWin;

	/* take in acount or not the FvwmButtons borders width */
	if (b->panel_flags.buttons_tbborder)
	{
		ftb = button_tborder;
		fbb = button_bborder;
	}
	if (b->panel_flags.buttons_lrborder)
	{
		frb = button_rborder;
		flb = button_lborder;
	}

	switch (b->slide_context)
	{
	case SLIDE_CONTEXT_PB: /* slide relatively to the panel button */
		bx = buttonXPos(b, b->n);
		by = buttonYPos(b, b->n);
		bw = buttonWidth(b);
		bh = buttonHeight(b);
		XTranslateCoordinates(
			Dpy, MyWindow, Root, bx, by, &bx, &by, &JunkWin);
		break;
	case SLIDE_CONTEXT_MODULE: /* slide relatively to FvwmButtons */
		{
			unsigned int dum, dum2;

			if (XGetGeometry(
				Dpy, MyWindow, &JunkWin, &bx, &by,
				(unsigned int *)&bw, (unsigned int *)&bh,
				(unsigned int *)&dum, &dum2))
			{
				XTranslateCoordinates(
					Dpy, MyWindow, Root, bx, by, &bx, &by,
					&JunkWin);
				break;
			}
			else
			{
				/* fall thorugh to SLIDE_CONTEXT_ROOT */
			}
		}
	case SLIDE_CONTEXT_ROOT: /* slide relatively to the root windows */
		switch (b->slide_direction)
		{
		case SLIDE_UP:
			bx = 0;
			by = dph;
			break;
		case SLIDE_DOWN:
			bx = 0;
			by = -dph;
			break;
		case SLIDE_RIGHT:
			bx = -dpw;
			by = 0;
			break;
		case SLIDE_LEFT:
			bx = dpw;
			by = 0;
			break;
		}
		bw = dpw;
		bh = dph;
		break;
	}

	/* relative corrections in % or pixel */
	*x = 0;
	*y = 0;
	if (b->relative_x != 0)
	{
		if (b->panel_flags.relative_x_pixel)
		{
			*x = b->relative_x;
		}
		else
		{
			*x = (int)(b->relative_x * bw) / 100;
		}
	}
	if (b->relative_y != 0)
	{
		if (b->panel_flags.relative_y_pixel)
		{
			*y = b->relative_y;
		}
		else
		{
			*y = (int)(b->relative_y * bh) / 100;
		}
	}

	switch (b->slide_direction)
	{
	case SLIDE_UP:
		switch (b->slide_position)
		{
		case SLIDE_POSITION_CENTER:
			*x += bx + (int)(bw - (b->w + (rb - lb))
				+ (frb - flb)) / 2;
			break;
		case SLIDE_POSITION_LEFT_TOP:
			*x += bx + lb - flb;
			break;
		case SLIDE_POSITION_RIGHT_BOTTOM:
			*x += bx + bw - b->w - rb + frb;
			break;
		}
		*w = b->w ;
		if (get_big)
		{
			*y += by - (int)b->h - bb - ftb;
			*h = b->h;
		}
		else
		{
			*y += by - bb - ftb;
			*h = 0;
		}
		break;
	case SLIDE_DOWN:
		switch (b->slide_position)
		{
		case SLIDE_POSITION_CENTER:
			*x += bx + (int)(bw - (b->w + (rb - lb))
				+ (frb - flb)) / 2;
			break;
		case SLIDE_POSITION_LEFT_TOP:
			*x += bx + lb - flb;
			break;
		case SLIDE_POSITION_RIGHT_BOTTOM:
			*x += bx + bw - b->w - rb + frb;
			break;
		}
		*y += by + (int)bh + tb + fbb;
		*w = b->w;
		if (get_big)
		{
			*h = b->h;
		}
		else
		{
			*h = 0;
		}
		break;
	case SLIDE_LEFT:
		switch (b->slide_position)
		{
		case SLIDE_POSITION_CENTER:
			*y += by + (int)(bh - b->h + (tb - bb)
				+ (fbb - ftb)) / 2;
			break;
		case SLIDE_POSITION_LEFT_TOP:
			*y += by + tb - ftb;
			break;
		case SLIDE_POSITION_RIGHT_BOTTOM:
			*y += by + bh - b->h - bb + fbb;
			break;
		}
		*h = b->h;
		if (get_big)
		{
			*x += bx - (int)b->w - rb - flb;
			*w = b->w;
		}
		else
		{
			*x += bx - rb - flb;
			*w = 0;
		}
		break;
	case SLIDE_RIGHT:
		switch (b->slide_position)
		{
		case SLIDE_POSITION_CENTER:
			*y += by + (int)(bh - b->h + (tb - bb)
				+ (fbb - ftb)) / 2;
			break;
		case SLIDE_POSITION_LEFT_TOP:
			*y += by + tb - ftb;
			break;
		case SLIDE_POSITION_RIGHT_BOTTOM:
			*y += by + bh - b->h - bb + fbb;
			break;
		}
		*x += bx + (int)bw + lb + frb;
		*h = b->h;
		if (get_big)
		{
			*w = b->w;
		}
		else
		{
			*w = 0;
		}
		break;
	}

	return;
}


/**
*** swallow()
*** Executed when swallowed windows get mapped
**/
void swallow(unsigned long *body)
{
	button_info *ub = UberButton, *b;
	int button = -1;
	int i;
	unsigned int d;
	Window p;
	FlocaleNameString temp;
	temp.name = NULL;
	temp.name_list = NULL;

	while (NextButton(&ub, &b, &button, 0))
	{
		Window swin = SwallowedWindow(b);

		if ((swin == (Window)body[0]) && (buttonSwallowCount(b) == 2))
		{
			/* Store the geometry in case we need to unswallow.
			 * Get parent */

			p = GetRealGeometry(
				Dpy, swin, &b->x, &b->y, &b->w, &b->h, &b->bw,
				&d);
#ifdef DEBUG_HANGON
			fprintf(stderr,
				"%s: Button 0x%06x %s 0x%lx, parent 0x%lx\n",
				MyName, (ushort)b, "trying to swallow window",
				body[0], p);
#endif

			if (p == None)
			/* This means the window is no more */ /* NO! wrong */
			{
				fprintf(stderr,
					"%s: Window 0x%lx (\"%s\") disappeared"
					" before swallow complete\n",
				        MyName, swin, b->hangon);
				/* Now what? For now: give up that button */
				b->flags.b_Hangon = 0;
				b->flags.b_Swallow = 0;
				b->flags.b_Panel = 0;
				return;
			}

			if (p != b->IconWinParent)
			{
				/* The window has been reparented */
				fprintf(stderr,
					"%s: Window 0x%lx (\"%s\") was grabbed"
					" by someone else (window 0x%lx)\n",
				        MyName, swin, b->hangon, p);

				/* Request a new windowlist, we might have
				   ignored another matching window.. */
				SendText(fd, "Send_WindowList", 0);

				/* Back one square and lose one turn */
				b->swallow &= ~b_Count;
				b->swallow |= 1;
				b->flags.b_Hangon = 1;
				return;
			}

#ifdef DEBUG_HANGON
			fprintf(stderr,
				"%s: Button 0x%06x swallowed window 0x%lx\n",
				MyName, (ushort)b, body[0]);
#endif

			if (b->flags.b_Swallow)
			{
				/* "Swallow" the window! Place it in the void
				 * so we don't see it until it's MoveResize'd */

				MyXGrabServer(Dpy);
				do_allow_bad_access = True;
				XSelectInput(Dpy, swin, SW_EVENTS);
				XSync(Dpy, 0);
				do_allow_bad_access = False;
				if (was_bad_access)
				{
					/* BadAccess means that the window is
					 * already swallowed by someone else.
					 * Wait for another window. */
					was_bad_access = False;
					/* Back one square and lose one turn */
					b->swallow &= ~b_Count;
					b->swallow |= 1;
					b->flags.b_Hangon = 1;
					MyXUngrabServer(Dpy);
					return;
				}
				/*error checking*/
				for (i = 0;
					!b->flags.b_ActionIgnoresClientWindow
					&&
					i <= NUMBER_OF_EXTENDED_MOUSE_BUTTONS;
					i++)
				{
					if (b->action != NULL && b->action[i] != NULL)
					{
						XGrabButton(
							Dpy, i, 0, b->IconWin,
							False, ButtonPressMask
							| ButtonReleaseMask,
							GrabModeAsync,
							GrabModeAsync,
							None, None);
						if (i == 0)
						{
							break;
						}
					}
				}
				XUnmapWindow(Dpy, swin);
				XReparentWindow(
					Dpy, swin, MyWindow, -32768, -32768);
				fsm_proxy(Dpy, swin, getenv("SESSION_MANAGER"));
				XSync(Dpy, 0);
				MyXUngrabServer(Dpy);
				XSync(Dpy, 0);
				usleep(50000);
				b->swallow &= ~b_Count;
				b->swallow |= 3;
				if (buttonSwallow(b) & b_UseTitle)
				{
					if (b->flags.b_Title)
					{
						free(b->title);
					}
					b->flags.b_Title = 1;
					FlocaleGetNameProperty(
						XGetWMName, Dpy, swin, &temp);
					if (temp.name != NULL)
					{
						CopyString(
							&b->title, temp.name);
						FlocaleFreeNameProperty(&temp);
					}
					else
					{
						CopyString(&b->title, "");
					}
				}

				MakeButton(b);
				XMapWindow(Dpy, swin);
				XSync(Dpy, 0);
				if (b->swallow & b_FvwmModule)
				{
					/* if we swallow a module we send an avertisment */
					char cmd[256];

					sprintf(cmd,
						"PropertyChange %u %u %lu %lu",
						MX_PROPERTY_CHANGE_SWALLOW, 1,
						SwallowedWindow(b),
						swallower_win ? swallower_win
							: MyWindow);
					SendText(fd, cmd, 0);
				}
				if (b->flags.b_Colorset &&  !(b->swallow & b_FvwmModule))
				{
					/* A short delay to give the application
					 * the chance to set the background
					 * itself, so we can override it.
					 * If we don't do this, the application
					 * may override our background. On the
					 * other hand it may still override our
					 * background, but our chances are a bit
					 * better. */

					usleep(250000);
					XSync(Dpy, 0);
					change_swallowed_window_colorset(b, True);
				}
				if (FShapesSupported)
				{
					if (UberButton->c->flags.b_TransBack)
					{
						SetTransparentBackground(
							UberButton, Width,
							Height);
						FShapeSelectInput(
							Dpy, swin,
							FShapeNotifyMask);
					}
				}
				/* Redraw and force cleaning the background to erase the old button
				 * title. */
				RedrawButton(b, DRAW_FORCE, NULL);
			}
			else /* (b->flags & b_Panel) */
			{
				b->swallow &= ~b_Count;
				b->swallow |= 3;
				XSelectInput(Dpy, swin, PA_EVENTS);
				if (!XWithdrawWindow(Dpy, swin, screen))
				{
					/* oops. what can we do now?
					 * let's pretend the unmap worked. */
				}
				b->newflags.panel_mapped = 0;
			}
			break;
		}
	}
}


void exec_swallow(char *action, button_info *b)
{
	static char *my_sm_env = NULL;
	static char *orig_sm_env = NULL;
	static int len = 0;
	static Bool sm_initialized = False;
	static Bool session_manager = False;
	char *cmd;

	if (!action)
	{
		return;
	}

	if (!sm_initialized)
	{
		/* use sm only when needed */
		sm_initialized = True;
		orig_sm_env = getenv("SESSION_MANAGER");
		if (orig_sm_env && !StrEquals("", orig_sm_env))
		{
			/* this set the new SESSION_MANAGER env */
			session_manager = fsm_init(MyName);
		}
	}

	if (!session_manager /*|| (buttonSwallow(b)&b_NoClose)*/)
	{
		SendText(fd, action, 0);
		return;
	}

	if (my_sm_env == NULL)
	{
		my_sm_env = getenv("SESSION_MANAGER");
		len = 45 + strlen(my_sm_env) + strlen(orig_sm_env);
	}

	/* TA:  FIXME!  xasprintf() */
	cmd = xmalloc(len + strlen(action));
	sprintf(
		cmd,
		"FSMExecFuncWithSessionManagment \"%s\" \"%s\" \"%s\"",
		my_sm_env, action, orig_sm_env);
	SendText(fd, cmd, 0);
	free(cmd);
}
