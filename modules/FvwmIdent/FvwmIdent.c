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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "libs/ftime.h"

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/Module.h"
#include "libs/Colorset.h"
#include "libs/fvwmsignal.h"
#include "libs/Flocale.h"
#include "libs/Parse.h"
#include "libs/FRenderInit.h"
#include "libs/Graphics.h"
#include "libs/System.h"
#include "libs/Target.h"
#include "libs/XError.h"

#include "FvwmIdent.h"

static RETSIGTYPE TerminateHandler(int);

static ModuleArgs *module;
static fd_set_size_t fd_width;
static int fd[2];

static Display *dpy;                    /* which display are we talking to */
static Window Root;
static GC gc;

static FlocaleFont *Ffont;
static FlocaleWinString *FwinString;

static int screen;
static int x_fd;

static char *yes = "Yes";
static char *no = "No";

/* default colorset to use, set to -1 when explicitly setting colors */
static int colorset = 0;

static char *BackColor = "white";
static char *ForeColor = "black";
static char *font_string = NULL;

static Pixel fore_pix;
static Pixel back_pix;
static Window main_win;
static Bool UsePixmapDrawing = False; /* if True draw everything in a pixmap
				       * and set the window background. Use
				       * this with Xft */
static int main_width;
static int main_height;

static EventMask mw_events =
  ExposureMask | ButtonPressMask | KeyPressMask |
  ButtonReleaseMask | KeyReleaseMask | StructureNotifyMask;

static Atom wm_del_win;

static struct target_struct target;
static int found=0;

static int ListSize=0;

static struct Item* itemlistRoot = NULL;
static int max_col1, max_col2;
static char id[15], desktop[10], swidth[10], sheight[10], borderw[10];
static char geometry[30], mymin_aspect[11], max_aspect[11], layer[10];
static char ewmh_init_state[512];
static char xin_str[10];

/* FIXME: default layer should be received from fvwm */
#define default_layer DEFAULT_DEFAULT_LAYER
static int minimal_layer = default_layer;
static int my_layer = default_layer;

/*
 *
 *  Procedure:
 *      main - start of module
 *
 */
int main(int argc, char **argv)
{
	char *display_name = NULL;
	char *tline;

	FlocaleInit(LC_CTYPE, "", "", "FvwmIdent");

	module = ParseModuleArgs(argc,argv,0); /* no alias */
	if (module == NULL)
	{
		fprintf(
			stderr, "FvwmIdent Version %s should only be executed"
			" by fvwm!\n", VERSION);
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

	fd[0] = module->to_fvwm;
	fd[1] = module->from_fvwm;

	/* Open the Display */
	if (!(dpy = XOpenDisplay(display_name)))
	{
		fprintf(stderr,"%s: can't open display %s", module->name,
			XDisplayName(display_name));
		exit (1);
	}
	x_fd = XConnectionNumber(dpy);
	screen= DefaultScreen(dpy);
	Root = RootWindow(dpy, screen);
	XSetErrorHandler(ErrorHandler);

	flib_init_graphics(dpy);
	FlocaleAllocateWinString(&FwinString);

	SetMessageMask(fd, M_CONFIGURE_WINDOW | M_WINDOW_NAME | M_ICON_NAME
		       | M_RES_CLASS | M_RES_NAME | M_END_WINDOWLIST |
		       M_CONFIG_INFO | M_END_CONFIG_INFO | M_SENDCONFIG);
	SetMessageMask(fd, MX_PROPERTY_CHANGE);
	/* scan config file for set-up parameters */
	/* Colors and fonts */

	InitGetConfigLine(fd,CatString3("*",module->name,0));
	GetConfigLine(fd,&tline);

	while (tline != (char *)0)
	{
		if (strlen(tline) <= 1)
		{
			continue;
		}
		if (strncasecmp(tline,
				CatString3("*",module->name,0),
				module->namelen+1) == 0)
		{
			tline += (module->namelen +1);
			if (strncasecmp(tline, "Font", 4) == 0)
			{
				CopyStringWithQuotes(&font_string, &tline[4]);
			}
			else if (strncasecmp(tline, "Fore", 4) == 0)
			{
				CopyString(&ForeColor, &tline[4]);
				colorset = -1;
			}
			else if (strncasecmp(tline, "Back", 4) == 0)
			{
				CopyString(&BackColor, &tline[4]);
				colorset = -1;
			}
			else if (strncasecmp(tline, "Colorset", 8) == 0)
			{
				sscanf(&tline[8], "%d", &colorset);
				AllocColorset(colorset);
			}
			else if (strncasecmp(tline, "MinimalLayer", 12) == 0)
			{
				char *layer_str = PeekToken(&tline[12], NULL);
				if (layer_str == NULL)
				{
					minimal_layer = default_layer;
				}
				else if (sscanf(
					layer_str, "%d", &minimal_layer) != 1)
				{
					if (strncasecmp(
						layer_str, "none", 4) == 0)
					{
						minimal_layer = -1;
					}
					else
					{
						minimal_layer = default_layer;
					}
				}
			}
		}
		else if (strncasecmp(tline, "Colorset", 8) == 0)
		{
			LoadColorset(&tline[8]);
		}
		else if (strncasecmp(
			tline, XINERAMA_CONFIG_STRING,
			sizeof(XINERAMA_CONFIG_STRING) - 1) == 0)
		{
			FScreenConfigureModule(
				tline + sizeof(XINERAMA_CONFIG_STRING) - 1);
		}
		GetConfigLine(fd, &tline);
	}

	if(module->window == 0)
	{
		fvwmlib_get_target_window(
			dpy, screen, module->name, &(module->window), True);
	}

	fd_width = GetFdWidth();

	/* Create a list of all windows */
	/* Request a list of all windows,
	 * wait for ConfigureWindow packets */
	SendText(fd, "Send_WindowList", 0);

	/* tell fvwm we're running */
	SendFinishedStartupNotification(fd);
	if (module->window == Root)
	{
		exit(0);
	}

	Loop(fd);
	return 0;
}

/*
 *
 * Read the entire window list from fvwm
 *
 */
void Loop(int *fd)
{
	while (1)
	{
		FvwmPacket* packet = ReadFvwmPacket(fd[1]);
		if ( packet == NULL )
		{
			exit(0);
		}
		else
		{
			process_message( packet->type, packet->body );
		}
	}
}


/*
 *
 * Process window list messages
 *
 */
void process_message(unsigned long type,unsigned long *body)
{
	switch(type)
	{
		/* should turn off this packet but it comes after config_list
		 * so have to accept at least one */
	case M_CONFIGURE_WINDOW:
		list_configure(body);
		break;
	case M_WINDOW_NAME:
		list_window_name(body);
		break;
	case M_ICON_NAME:
		list_icon_name(body);
		break;
	case M_RES_CLASS:
		list_class(body);
		break;
	case M_RES_NAME:
		list_res_name(body);
		break;
	case M_END_WINDOWLIST:
		list_end();
		break;
	default:
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

/*
 *
 * Got window configuration info - if its our window, save data
 *
 */
void list_configure(unsigned long *body)
{
	struct ConfigWinPacket  *cfgpacket = (void *) body;

	if (
		(module->window == cfgpacket->frame)||
		(module->window == cfgpacket->w) ||
		((cfgpacket->icon_w != 0)&&
		 (module->window == cfgpacket->icon_w)) ||
		((cfgpacket->icon_pixmap_w)&&
		 (module->window == cfgpacket->icon_pixmap_w)))
	{
		module->window = cfgpacket->frame;
		target.id = cfgpacket->w;
		target.frame = cfgpacket->frame;
		target.frame_x = cfgpacket->frame_x;
		target.frame_y = cfgpacket->frame_y;
		target.frame_w = cfgpacket->frame_width;
		target.frame_h = cfgpacket->frame_height;
		target.desktop = cfgpacket->desk;
		target.layer = cfgpacket->layer;
		memcpy(&target.flags,
		       &(cfgpacket->flags), sizeof(cfgpacket->flags));
		target.title_h = cfgpacket->title_height;
		target.title_dir = GET_TITLE_DIR(cfgpacket);
		target.border_w = cfgpacket->border_width;
		target.base_w = cfgpacket->hints_base_width;
		target.base_h = cfgpacket->hints_base_height;
		target.width_inc = cfgpacket->orig_hints_width_inc;
		target.height_inc = cfgpacket->orig_hints_height_inc;
		target.gravity = cfgpacket->hints_win_gravity;
		target.ewmh_hint_layer = cfgpacket->ewmh_hint_layer;
		target.ewmh_hint_desktop = cfgpacket->ewmh_hint_desktop;
		target.ewmh_window_type = cfgpacket->ewmh_window_type;
		found = 1;

		my_layer = (int)target.layer;
		if (my_layer < minimal_layer)
		{
			my_layer = minimal_layer;
		}
	}
}

/*
 *
 * Capture  Window name info
 *
 */
void list_window_name(unsigned long *body)
{
	if (
		(module->window == (Window)body[1])||
		(module->window == (Window)body[0]))
	{
		strncpy(target.name,(char *)&body[3],255);
	}
}

/*
 *
 * Capture  Window Icon name info
 *
 */
void list_icon_name(unsigned long *body)
{
	if (
		(module->window == (Window)body[1])||
		(module->window == (Window)body[0]))
	{
		strncpy(target.icon_name,(char *)&body[3],255);
	}
}


/*
 *
 * Capture  Window class name info
 *
 */
void list_class(unsigned long *body)
{
	if (
		(module->window == (Window)body[1])||
		(module->window == (Window)body[0]))
	{
		strncpy(target.class,(char *)&body[3],255);
	}
}


/*
 *
 * Capture  Window resource info
 *
 */
void list_res_name(unsigned long *body)
{
	if ((module->window == (Window)body[1])||
	    (module->window == (Window)body[0]))
	{
		strncpy(target.res,(char *)&body[3],255);
	}
}

void list_property_change(unsigned long *body)
{
	if (body[0] == MX_PROPERTY_CHANGE_BACKGROUND && body[2] == 0 &&
	    CSET_IS_TRANSPARENT_PR(colorset))
	{
		if (UsePixmapDrawing)
		{
			PixmapDrawWindow(
				main_width, main_height);
		}
		else
		{
			UpdateBackgroundTransparency(
				dpy, main_win, main_width,
				main_height,
				&Colorset[(colorset)], Pdepth,
				gc, True);
		}
	}
}

void list_config_info(unsigned long *body)
{
	char *tline, *token;

	tline = (char*)&body[3];
	tline = GetNextToken(tline, &token);
	if (StrEquals(token, "Colorset") && colorset >= 0 &&
	    LoadColorset(tline) == colorset)
	{
		if (FftSupport && Ffont->fftf.fftfont != NULL)
		{
			UsePixmapDrawing = True;
		}
		/* track all colorset changes & update display if necessary */
		/* ask for movement events iff transparent */
		if (CSET_IS_TRANSPARENT(colorset))
		{

			mw_events |= StructureNotifyMask;
			if (CSET_IS_TRANSPARENT_PR_PURE(colorset))
			{
				UsePixmapDrawing = False;
			}
		}
		else
		{
			mw_events &= ~(StructureNotifyMask);
		}
		if (UsePixmapDrawing)
		{
#if 0
			mw_events &= ~(ExposureMask);
#endif
		}
		else
		{
			mw_events |= ExposureMask;
		}
		XSelectInput(dpy, main_win, mw_events);
		XSetForeground(dpy, gc, Colorset[colorset].fg);
		if (UsePixmapDrawing)
		{
			PixmapDrawWindow(main_width, main_height);
		}
		else
		{
			SetWindowBackground(
				dpy, main_win, main_width, main_height,
				&Colorset[colorset], Pdepth, gc, True);
		}
	}
	else if (StrEquals(token, XINERAMA_CONFIG_STRING))
	{
		FScreenConfigureModule(tline);
	}
	if (token)
		free(token);
}

/*
 *
 * Process X Events
 *
 */
int ProcessXEvent(int x, int y)
{
	XEvent Event,event;
	static int is_key_pressed = 0;
	static int is_button_pressed = 0;
	char buf[32];
	static int ex=10000, ey=10000, ex2=0, ey2=0;

	while (FPending(dpy))
	{
		FNextEvent(dpy,&Event);
		switch(Event.type)
		{
		case Expose:
			ex = min(ex, Event.xexpose.x);
			ey = min(ey, Event.xexpose.y);
			ex2 = max(ex2, Event.xexpose.x + Event.xexpose.width);
			ey2=max(ey2 , Event.xexpose.y + Event.xexpose.height);
			while (FCheckTypedEvent(dpy, Expose, &Event))
			{
				ex = min(ex, Event.xexpose.x);
				ey = min(ey, Event.xexpose.y);
				ex2 = max(
					ex2,
					Event.xexpose.x + Event.xexpose.width);
				ey2=max(ey2,
					Event.xexpose.y + Event.xexpose.height);
			}
			if (FftSupport && Ffont->fftf.fftfont != NULL)
			{
				XClearArea(
					dpy, main_win,
					ex, ey, ex2-ex, ey2-ey, False);
			}
			DrawItems(main_win, ex, ey, ex2-ex, ey2-ey);
			ex = ey = 10000;
			ex2 = ey2 = 0;
			break;
		case KeyPress:
			is_key_pressed = 1;
			break;
		case ButtonPress:
			is_button_pressed = Event.xbutton.button;
			break;
		case KeyRelease:
			if (is_key_pressed)
			{
				exit(0);
			}
			break;
		case ButtonRelease:
			if (is_button_pressed)
			{
				if (is_button_pressed == 2 &&
				    Event.xbutton.button == 2)
				{
					/* select a new window when
					 * button 2 is pressed */
					SetMessageMask(
						fd, M_CONFIGURE_WINDOW   |
						M_WINDOW_NAME    |
						M_ICON_NAME      |
						M_RES_CLASS      |
						M_RES_NAME       |
						M_END_WINDOWLIST |
						M_CONFIG_INFO    |
						M_END_CONFIG_INFO|
						M_SENDCONFIG);
					SendText(fd, "Send_WindowList", 0);
					XDestroyWindow(dpy, main_win);
					DestroyList();
					fvwmlib_get_target_window(
						dpy, screen, module->name,
						 &(module->window), True);
					found = 0;
					return 1;
				}
				else
				{
					exit(0);
				}
			}
			break;
		case ClientMessage:
			if (Event.xclient.format==32 &&
			    Event.xclient.data.l[0]==wm_del_win)
			{
				exit(0);
			}
			break;
		case ReparentNotify:
			if (minimal_layer >= 0)
			{
				sprintf(buf, "Layer 0 %d", my_layer);
				SendText(fd, buf, main_win);
			}
			SendText(fd, "Raise", main_win);
			break;
		case ConfigureNotify:
			fev_sanitise_configure_notify(&Event.xconfigure);
                       /* this only happens with transparent windows,
			* slurp up as many events as possible before
			* redrawing to reduce flickering */
			while (FCheckTypedEvent(
				dpy, ConfigureNotify, &event))
			{
				fev_sanitise_configure_notify(
					&event.xconfigure);
				if (!event.xconfigure.send_event)
					continue;
				Event.xconfigure.x = event.xconfigure.x;
				Event.xconfigure.y = event.xconfigure.y;
				Event.xconfigure.send_event = True;
			}
			/* Only refresh if moved */
			if ((Event.xconfigure.send_event ||
			     CSET_IS_TRANSPARENT_PR_PURE(colorset)) &&
			    (x != Event.xconfigure.x ||
			     y != Event.xconfigure.y))
			{
				static Bool is_initial_cn = True;
				Bool do_eat_expose = False;

				x = Event.xconfigure.x;
				y = Event.xconfigure.y;
				/* flush any expose events */
				if (UsePixmapDrawing)
				{
					PixmapDrawWindow(
						main_width, main_height);
					do_eat_expose = True;
				}
				else if (colorset == -1)
				{
					do_eat_expose = True;
				}
				else if (UpdateBackgroundTransparency(
					dpy, main_win, main_width,
					main_height,
					&Colorset[(colorset)], Pdepth,
					gc, True) == True)
				{
					do_eat_expose = True;
				}
				if (do_eat_expose == True &&
				    is_initial_cn == False)
				{
					while (FCheckTypedEvent(
						dpy, Expose, &Event))
					{
						/* nothing */
					}
				}
				is_initial_cn = False;
			}
			break;
		}
	}
	XFlush (dpy);

	return 0;
}

/*
 *
 * End of window list, open an x window and display data in it
 *
 */
void list_end(void)
{
	XSizeHints mysizehints;
	XGCValues gcv;
	unsigned long gcm;
	int lmax,height;
	int x,y;
	XSetWindowAttributes attributes;

	if(!found)
	{
		exit(0);
	}

	/* tell fvwm to only send config messages */
	SetMessageMask(fd, M_CONFIG_INFO | M_SENDCONFIG);

	if ((Ffont = FlocaleLoadFont(dpy, font_string, module->name)) == NULL)
	{
		fprintf(
			stderr,"%s: cannot load font, exiting\n",
			module->name);
		exit(1);
	}

	/* chose the rendering methode */
	if (FftSupport && Ffont->fftf.fftfont != NULL)
	{
		UsePixmapDrawing = True;
	}
	/* make window infomation list */
	MakeList();

	/* size and create the window */
	lmax = max_col1 + max_col2 + 15;

	height = ListSize * (Ffont->height);

	mysizehints.flags=
		USSize|USPosition|PWinGravity|PResizeInc|PBaseSize|PMinSize|
		PMaxSize;
	main_width = mysizehints.width = lmax + 10;
	main_height = mysizehints.height = height + 10;
	mysizehints.width_inc = 1;
	mysizehints.height_inc = 1;
	mysizehints.base_height = mysizehints.height;
	mysizehints.base_width = mysizehints.width;
	mysizehints.min_height = mysizehints.height;
	mysizehints.min_width = mysizehints.width;
	mysizehints.max_height = mysizehints.height;
	mysizehints.max_width = mysizehints.width;
	mysizehints.win_gravity = NorthWestGravity;
	{
		int sx;
		int sy;
		int sw;
		int sh;
		Window JunkW;
		int JunkC;
		unsigned int JunkM;
		fscreen_scr_arg fscr;

		if (!FQueryPointer(
			dpy, Root, &JunkW, &JunkW, &x, &y, &JunkC, &JunkC,
			&JunkM))
		{
			/* pointer is on a different screen */
			x = 0;
			y = 0;
		}

		fscr.xypos.x = x;
		fscr.xypos.y = y;
		FScreenGetScrRect(&fscr, FSCREEN_XYPOS, &sx, &sy, &sw, &sh);
		if (y + height + 100 > sy + sh)
		{
			y = sy + sh - height - 10;
			mysizehints.win_gravity = SouthWestGravity;
		}
		if (x + lmax + 100 > sx + sw)
		{
			x = sx + sw - lmax - 10;
			if (mysizehints.win_gravity == SouthWestGravity)
				mysizehints.win_gravity = SouthEastGravity;
			else
				mysizehints.win_gravity = NorthEastGravity;
		}
	}

	if (Pdepth < 2)
	{
		back_pix = GetColor("white");
		fore_pix = GetColor("black");
	}
	else
	{
		back_pix = (colorset < 0)?
			GetColor(BackColor) : Colorset[colorset].bg;
		fore_pix = (colorset < 0)?
			GetColor(ForeColor) : Colorset[colorset].fg;
	}

	attributes.colormap = Pcmap;
	attributes.border_pixel = 0;
	attributes.background_pixel = back_pix;
	main_win = XCreateWindow(
		dpy, Root, x, y, mysizehints.width, mysizehints.height, 0,
		Pdepth, InputOutput, Pvisual, CWColormap | CWBackPixel |
		CWBorderPixel, &attributes);
	wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
	XSetWMProtocols(dpy,main_win,&wm_del_win,1);

	/* hack to prevent mapping on wrong screen with StartsOnScreen */
	FScreenMangleScreenIntoUSPosHints(FSCREEN_XYPOS, &mysizehints);
	XSetWMNormalHints(dpy,main_win,&mysizehints);
	/* have to ask for configure events when transparent */
	if (CSET_IS_TRANSPARENT(colorset))
	{
		mw_events |= StructureNotifyMask;
		if (CSET_IS_TRANSPARENT_PR_PURE(colorset))
		{
			UsePixmapDrawing = 0;
		}
	}
	if (!UsePixmapDrawing)
	{
		mw_events |= ExposureMask;
	}

	XSelectInput(dpy, main_win, mw_events);
	change_window_name(module->name);

	gcm = GCForeground;
	gcv.foreground = fore_pix;
	if (Ffont->font != NULL)
	{
		gcm |= GCFont;
		gcv.font = Ffont->font->fid;
	}
	gc = fvwmlib_XCreateGC(dpy, main_win, gcm, &gcv);
	if (UsePixmapDrawing)
	{
		PixmapDrawWindow(main_width, main_height);
	}
	else if (colorset >= 0)
	{
		SetWindowBackground(
			dpy, main_win, main_width, main_height,
			&Colorset[(colorset)], Pdepth, gc, True);
	}
	XMapWindow(dpy,main_win);

	/* Window is created. Display it until the user clicks or deletes it.
	 * also grok any dynamic config changes */
	while(1)
	{
		FvwmPacket* packet;
		int x_fd = XConnectionNumber(dpy);
		fd_set fdset;

		FD_ZERO(&fdset);
		FD_SET(fd[1], &fdset);
		FD_SET(x_fd, &fdset);

		/* process all X events first */
		if (ProcessXEvent(x,y) == 1)
		{
			return;
		}

		/* wait for X-event or config line */
		select(fd_width, SELECT_FD_SET_CAST &fdset, NULL, NULL, NULL );

		/* parse any dynamic config lines */
		if (FD_ISSET(fd[1], &fdset))
		{
			packet = ReadFvwmPacket(fd[1]);
			if (packet == NULL)
				exit(0);
			if (packet &&  packet->type == MX_PROPERTY_CHANGE)
			{
				list_property_change(packet->body);
			}
			if (packet && packet->type == M_CONFIG_INFO)
			{
				list_config_info(packet->body);
			}
		}
	}
}

/*
 *
 * Draw the items
 *
 */
void DrawItems(Drawable d, int x, int y, int w, int h)
{
	int fontheight,i=0;
	struct Item *cur = itemlistRoot;
	Region region = 0;

	fontheight = Ffont->height;
	FwinString->win = d;
	FwinString->gc = gc;
	FwinString->flags.has_clip_region = False;
	if (w > 0)
	{
		XRectangle r;

		r.x = x;
		r.y = y;
		r.width = w;
		r.height = h;

		region = XCreateRegion();
		XUnionRectWithRegion(&r, region, region);
		XSetRegion(dpy, gc, region);
		FwinString->flags.has_clip_region = True;
		FwinString->clip_region = region;
	}

	if (colorset >= 0)
	{
		FwinString->colorset = &Colorset[colorset];
		FwinString->flags.has_colorset = True;
	}
	while(cur != NULL) /* may be optimised */
	{
		/* first column */
		FwinString->str = cur->col1;
		FwinString->x = 5;
		FwinString->y = 5 + Ffont->ascent + i * fontheight;
		FlocaleDrawString(dpy, Ffont, FwinString, 0);

		/* second column */
		FwinString->str = cur->col2;
		FwinString->x = 10 + max_col1;
		FlocaleDrawString(dpy, Ffont, FwinString, 0);

		++i;
		cur = cur->next;
	}
	if (FwinString->flags.has_clip_region)
	{
		XDestroyRegion(region);
		XSetClipMask(dpy, gc, None);
	}
	XFlush (dpy);
}

void PixmapDrawWindow(int w, int h)
{
	Pixmap pix;
	XGCValues gcv;
	unsigned long gcm;

	if (colorset >= 0)
	{
		Pixmap cs_pix;
		cs_pix = CreateBackgroundPixmap(dpy, main_win, w, h,
						&Colorset[(colorset)],
						Pdepth, gc, False);
		if (cs_pix == ParentRelative)
		{
			pix = cs_pix;
		}
		else
		{
			pix = CreateTiledPixmap(
				dpy, cs_pix, 0,0,w,h,Pdepth, gc);
			XFreePixmap(dpy, cs_pix);
		}
	}
	else
	{
		gcm = GCForeground;
		gcv.foreground = back_pix;
		XChangeGC(dpy, gc, gcm, &gcv);
		pix = XCreatePixmap(dpy, main_win, w, h, Pdepth);
		XFillRectangle(dpy, pix, gc, 0, 0, w, h);
		gcv.foreground = fore_pix;
		XChangeGC(dpy, gc, gcm, &gcv);
	}

	if (pix != ParentRelative)
	{
		DrawItems(pix, 0, 0, 0, 0);
		XSetWindowBackgroundPixmap(dpy, main_win, pix);
		XClearWindow(dpy, main_win);
		XFreePixmap(dpy, pix);
	}
	else
	{
		XSetWindowBackgroundPixmap(dpy, main_win, pix);
		XClearWindow(dpy, main_win);
		DrawItems(main_win, 0, 0, 0, 0);
	}
}

/*
 *  Change the window name displayed in the title bar.
 */
void change_window_name(char *str)
{
	XTextProperty name;
	XClassHint myclasshints;

	if (XStringListToTextProperty(&str,1,&name) == 0)
	{
		fprintf(stderr,"%s: cannot allocate window name",module->name);
		return;
	}
	XSetWMName(dpy,main_win,&name);
	XSetWMIconName(dpy,main_win,&name);
	XFree(name.value);
	myclasshints.res_name = str;
	myclasshints.res_class = "FvwmIdent";
	XSetClassHint(dpy,main_win,&myclasshints);
}


void DestroyList(void)
{
	struct Item *t;
	struct Item *tmp;

	for (t = itemlistRoot; t; t = tmp)
	{
		tmp = t->next;
		free(t);
	}
	itemlistRoot = NULL;
}

/*
*
* Add s1(string at first column) and s2(string at second column) to itemlist
*
*/
void AddToList(char *s1, char* s2)
{
	int tw1, tw2;
	struct Item* item, *cur = itemlistRoot;

	tw1 = FlocaleTextWidth(Ffont, s1, strlen(s1));
	tw2 = FlocaleTextWidth(Ffont, s2, strlen(s2));
	max_col1 = max_col1 > tw1 ? max_col1 : tw1;
	max_col2 = max_col2 > tw2 ? max_col2 : tw2;

	item = xmalloc(sizeof(struct Item));
	item->col1 = s1;
	item->col2 = s2;
	item->next = NULL;

	if (cur == NULL)
	{
		itemlistRoot = item;
	}
	else
	{
		while(cur->next != NULL)
		{
			cur = cur->next;
		}
		cur->next = item;
	}
	ListSize++;
}

void MakeList(void)
{
	int bw,width,height,x1,y1,x2,y2;
	char loc[20];
	static char xstr[6],ystr[6];
	/* GSFR - quick hack because the new macros depend on a prt reference
	 */
	struct target_struct  *targ  =  &target;

	ListSize = 0;

	bw = 2*target.border_w;
	width = target.frame_w - bw;
	height = target.frame_h - bw;
	if (target.title_dir == DIR_W || target.title_dir == DIR_E)
	{
		width -= target.title_h;
	}
	else if (target.title_dir == DIR_N || target.title_dir == DIR_S)
	{
		height -= target.title_h;
	}

	sprintf(desktop, "%ld",  target.desktop);
	sprintf(layer,   "%ld",  target.layer);
	sprintf(id,      "0x%x", (unsigned int)target.id);
	sprintf(swidth,  "%d",   width);
	sprintf(sheight, "%d",   height);
	sprintf(borderw, "%ld",  target.border_w);
	sprintf(xstr,    "%ld",  target.frame_x);
	sprintf(ystr,    "%ld",  target.frame_y);

	AddToList("Name:",          target.name);
	AddToList("Icon Name:",     target.icon_name);
	AddToList("Class:",         target.class);
	AddToList("Resource:",      target.res);
	AddToList("Window ID:",     id);
	AddToList("Desk:",          desktop);
	if (FScreenIsEnabled()) {
		sprintf(xin_str, "%d",
			FScreenOfPointerXY(target.frame_x, target.frame_y));
		AddToList("Xinerama Screen:", xin_str);
	}
	AddToList("Layer:",         layer);
	AddToList("Width:",         swidth);
	AddToList("Height:",        sheight);
	AddToList("X (current page):",   xstr);
	AddToList("Y (current page):",   ystr);
	AddToList("Boundary Width:", borderw);

	AddToList("StickyPage:",    (IS_STICKY_ACROSS_PAGES(targ) ? yes : no));
	AddToList("StickyDesk:",    (IS_STICKY_ACROSS_DESKS(targ) ? yes : no));
	AddToList("StickyPageIcon:",
		(IS_ICON_STICKY_ACROSS_PAGES(targ) ? yes : no));
	AddToList("StickyDeskIcon:",
		(IS_ICON_STICKY_ACROSS_DESKS(targ) ? yes : no));
	AddToList("NoTitle:",       (HAS_TITLE(targ)    ? no : yes));
	AddToList("Iconified:",     (IS_ICONIFIED(targ) ? yes : no));
	AddToList("Transient:",     (IS_TRANSIENT(targ) ? yes : no));
	AddToList("WindowListSkip:", (DO_SKIP_WINDOW_LIST(targ) ? yes : no));

	switch(target.gravity)
	{
	case ForgetGravity:
		AddToList("Gravity:", "Forget");
		break;
	case NorthWestGravity:
		AddToList("Gravity:", "NorthWest");
		break;
	case NorthGravity:
		AddToList("Gravity:", "North");
		break;
	case NorthEastGravity:
		AddToList("Gravity:", "NorthEast");
		break;
	case WestGravity:
		AddToList("Gravity:", "West");
		break;
	case CenterGravity:
		AddToList("Gravity:", "Center");
		break;
	case EastGravity:
		AddToList("Gravity:", "East");
		break;
	case SouthWestGravity:
		AddToList("Gravity:", "SouthWest");
		break;
	case SouthGravity:
		AddToList("Gravity:", "South");
		break;
	case SouthEastGravity:
		AddToList("Gravity:", "SouthEast");
		break;
	case StaticGravity:
		AddToList("Gravity:", "Static");
		break;
	default:
		AddToList("Gravity:", "Unknown");
		break;
	}
	x1 = target.frame_x;
	if(x1 < 0)
	{
		x1 = 0;
	}
	x2 = DisplayWidth(dpy,screen) - x1 - target.frame_w;
	if(x2 < 0)
	{
		x2 = 0;
	}
	y1 = target.frame_y;
	if(y1 < 0)
	{
		y1 = 0;
	}
	y2 = DisplayHeight(dpy,screen) - y1 -  target.frame_h;
	if(y2 < 0)
	{
		y2 = 0;
	}
	width = (width - target.base_w)/target.width_inc;
	height = (height - target.base_h)/target.height_inc;

	sprintf(loc,"%dx%d",width,height);
	strcpy(geometry, loc);

	if ((target.gravity == EastGravity) ||
	    (target.gravity == NorthEastGravity)||
	    (target.gravity == SouthEastGravity))
	{
		sprintf(loc,"-%d",x2);
	}
	else
	{
		sprintf(loc,"+%d",x1);
	}
	strcat(geometry, loc);

	if((target.gravity == SouthGravity)||
	   (target.gravity == SouthEastGravity)||
	   (target.gravity == SouthWestGravity))
	{
		sprintf(loc,"-%d",y2);
	}
	else
	{
		sprintf(loc,"+%d",y1);
	}
	strcat(geometry, loc);
	AddToList("Geometry:", geometry);

	{
		Atom *protocols = NULL, *ap;
		Atom _XA_WM_TAKE_FOCUS = XInternAtom(
			dpy, "WM_TAKE_FOCUS", False);
		XWMHints *wmhintsp = XGetWMHints(dpy,target.id);
		int i,n;
		Boolean HasTakeFocus=False,InputField=True;
		char *focus_policy="",*ifstr="",*tfstr="";

		if (wmhintsp)
		{
			InputField=wmhintsp->input;
			ifstr=InputField?"True":"False";
			XFree(wmhintsp);
		}
		else
		{
			ifstr="XWMHints missing";
		}
		if (XGetWMProtocols(dpy,target.id,&protocols,&n))
		{
			for (i = 0, ap = protocols; i < n; i++, ap++)
			{
				if (*ap == (Atom)_XA_WM_TAKE_FOCUS)
					HasTakeFocus = True;
			}
			tfstr=HasTakeFocus?"Present":"Absent";
			XFree(protocols);
		}
		else
		{
			tfstr="XGetWMProtocols failed";
		}
		if (HasTakeFocus)
		{
			if (InputField)
			{
				focus_policy = "Locally Active";
			}
			else
			{
				focus_policy = "Globally Active";
			}
		}
		else
		{
			if (InputField)
			{
				focus_policy = "Passive";
			}
			else
			{
				focus_policy = "No Input";
			}
		}
		AddToList("Focus Policy:",focus_policy);
		AddToList("  - Input Field:",ifstr);
		AddToList("  - WM_TAKE_FOCUS:",tfstr);
		{
			/* flags hints that were supplied */
			long supplied_return;
			int getrc;
			XSizeHints *size_hints =
				XAllocSizeHints(); /* the size hints */
			if ((getrc = XGetWMSizeHints(
				dpy,target.id, /* get size hints */
				size_hints,    /* Hints */
				&supplied_return,
				XA_WM_ZOOM_HINTS)))
			{
				if (supplied_return & PAspect)
				{ /* if window has a aspect ratio */
					sprintf(
						mymin_aspect, "%d/%d",
						size_hints->min_aspect.x,
						size_hints->min_aspect.y);
					AddToList(
						"Minimum aspect ratio:",
						mymin_aspect);
					sprintf(
						max_aspect, "%d/%d",
						size_hints->max_aspect.x,
						size_hints->max_aspect.y);
					AddToList(
						"Maximum aspect ratio:",
						max_aspect);
				} /* end aspect ratio */
				XFree(size_hints);
			} /* end getsizehints worked */
		}
	}

	/* EWMH window type */
	if (target.ewmh_window_type == EWMH_WINDOW_TYPE_DESKTOP_ID)
		AddToList("EWMH Window Type:","Desktop");
	else if (target.ewmh_window_type == EWMH_WINDOW_TYPE_DIALOG_ID)
		AddToList("EWMH Window Type:","Dialog");
	else if (target.ewmh_window_type == EWMH_WINDOW_TYPE_DOCK_ID)
		AddToList("EWMH Window Type:","Dock");
	else if (target.ewmh_window_type == EWMH_WINDOW_TYPE_MENU_ID)
		AddToList("EWMH Window Type:","Menu");
	else if (target.ewmh_window_type == EWMH_WINDOW_TYPE_NORMAL_ID)
		AddToList("EWMH Window Type:","Normal");
	else if (target.ewmh_window_type == EWMH_WINDOW_TYPE_TOOLBAR_ID)
		AddToList("EWMH Window Type:","ToolBar");

	/* EWMH wm state */
	ewmh_init_state[0] = '\0';
	if (HAS_EWMH_INIT_FULLSCREEN_STATE(targ) == EWMH_STATE_HAS_HINT)
	{
		strcat(ewmh_init_state, "FullScreen ");
	}
	if (HAS_EWMH_INIT_HIDDEN_STATE(targ) == EWMH_STATE_HAS_HINT)
	{
		strcat(ewmh_init_state, "Iconic ");
	}
	if (HAS_EWMH_INIT_MAXHORIZ_STATE(targ) == EWMH_STATE_HAS_HINT)
	{
		strcat(ewmh_init_state, "MaxHoriz ");
	}
	if (HAS_EWMH_INIT_MAXVERT_STATE(targ) == EWMH_STATE_HAS_HINT)
	{
		strcat(ewmh_init_state, "MaxVert ");
	}
	if (HAS_EWMH_INIT_MODAL_STATE(targ) == EWMH_STATE_HAS_HINT)
	{
		strcat(ewmh_init_state, "Modal ");
	}
	if (HAS_EWMH_INIT_SHADED_STATE(targ)== EWMH_STATE_HAS_HINT)
	{
		strcat(ewmh_init_state, "Shaded ");
	}
	if (HAS_EWMH_INIT_SKIP_PAGER_STATE(targ) == EWMH_STATE_HAS_HINT ||
	    HAS_EWMH_INIT_SKIP_TASKBAR_STATE(targ) == EWMH_STATE_HAS_HINT )
	{
		strcat(ewmh_init_state, "SkipList ");
	}
	if (HAS_EWMH_INIT_STICKY_STATE(targ) == EWMH_STATE_HAS_HINT ||
	    (HAS_EWMH_INIT_WM_DESKTOP(targ) ==  EWMH_STATE_HAS_HINT &&
	     (target.ewmh_hint_desktop == (unsigned long)-2 ||
	      target.ewmh_hint_desktop == (unsigned long)-1)))
	{
		strcat(ewmh_init_state, "Sticky ");
	}
	/* FIXME: we should use the fvwm default layers */
	if (target.ewmh_hint_layer == 6)
	{
		strcat(ewmh_init_state, "StaysOnTop ");
	}
	else if (target.ewmh_hint_layer == 2)
	{
		strcat(ewmh_init_state, "StaysOnBottom ");
	}
	if (HAS_EWMH_INIT_WM_DESKTOP(targ) == EWMH_STATE_HAS_HINT &&
	    target.ewmh_hint_desktop < 256)
	{
		strcat(ewmh_init_state, "StartOnDesk");
		sprintf(ewmh_init_state, "%s %lu ",
			ewmh_init_state, target.ewmh_hint_desktop);
	}
	if (ewmh_init_state[0] != '\0')
	{
		/* remove ending space */
		ewmh_init_state[strlen(ewmh_init_state)-1] = '\0';
		AddToList("EWMH Init State:",ewmh_init_state);
	}
}

/*
  X Error Handler
*/
static int
ErrorHandler(Display *d, XErrorEvent *event)
{
#if 0
	if (event->error_code == BadPixmap)
	{
		return 0;
	}
	if (event->error_code == BadDrawable)
	{
		return 0;
	}
	if (event->error_code == FRenderGetErrorCodeBase() + FRenderBadPicture)
	{
		return 0;
	}
#endif

	PrintXErrorAndCoredump(d, event, module->name);
	return 0;
}
