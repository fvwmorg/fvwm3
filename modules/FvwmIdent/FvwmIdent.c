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

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#ifdef I18N_MB
#include <X11/Xlocale.h>
#endif

#include "libs/fvwmlib.h"
#include "libs/XineramaSupport.h"
#include "libs/Module.h"
#include "libs/Picture.h"
#include "libs/Colorset.h"
#include "libs/fvwmsignal.h"
#include "FvwmIdent.h"

static RETSIGTYPE TerminateHandler(int);

static char *MyName;
static fd_set_size_t fd_width;
static int fd[2];

static Display *dpy;			/* which display are we talking to */
static Window Root;
static GC gc;
static XFontStruct *font;
#ifdef I18N_MB
static XFontSet fontset;
#endif

static int screen;
static int x_fd;

static char *yes = "Yes";
static char *no = "No";

/* default colorset to use, set to -1 when explicitly setting colors */
static int colorset = 0;

static char *BackColor = "white";
static char *ForeColor = "black";
static char *font_string = "fixed";

static Pixel fore_pix;
static Window main_win;
static Window app_win;

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

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
int main(int argc, char **argv)
{
  char *temp, *s;
  char *display_name = NULL;
  int Clength;
  char *tline;

#ifdef I18N_MB
  setlocale(LC_CTYPE, "");
#endif

  /* Save the program name for error messages and config parsing */
  temp = argv[0];
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = safemalloc(strlen(temp)+2);
  strcpy(MyName,"*");
  strcat(MyName, temp);
  Clength = strlen(MyName);

  if((argc != 6)&&(argc != 7))
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

  /* An application window may have already been selected - look for it */
  sscanf(argv[4],"%x",(unsigned int *)&app_win);

  /* Open the Display */
  if (!(dpy = XOpenDisplay(display_name)))
    {
      fprintf(stderr,"%s: can't open display %s", MyName,
	      XDisplayName(display_name));
      exit (1);
    }
  x_fd = XConnectionNumber(dpy);
  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);

  InitPictureCMap(dpy);
  XineramaSupportInit(dpy);
  /* prevent core dumps if fvwm doesn't provide any colorsets */
  AllocColorset(0);

  SetMessageMask(fd, M_CONFIGURE_WINDOW | M_WINDOW_NAME | M_ICON_NAME
                 | M_RES_CLASS | M_RES_NAME | M_END_WINDOWLIST | M_CONFIG_INFO
                 | M_END_CONFIG_INFO | M_SENDCONFIG);
  /* scan config file for set-up parameters */
  /* Colors and fonts */

  InitGetConfigLine(fd,MyName);
  GetConfigLine(fd,&tline);

  while(tline != (char *)0) {
    if(strlen(tline)>1) {
      if(strncasecmp(tline, CatString3(MyName,"Font",""),Clength+4)==0) {
        CopyString(&font_string,&tline[Clength+4]);
      }
      else if(strncasecmp(tline,CatString3(MyName,"Fore",""), Clength+4)==0) {
        CopyString(&ForeColor,&tline[Clength+4]);
        colorset = -1;
      }
      else if(strncasecmp(tline,CatString3(MyName, "Back",""), Clength+4)==0) {
        CopyString(&BackColor,&tline[Clength+4]);
        colorset = -1;
      }
      else if(strncasecmp(tline,CatString3(MyName,"Colorset",""),Clength+8)==0){
        sscanf(&tline[Clength+8], "%d", &colorset);
        AllocColorset(colorset);
      }
      else if(strncasecmp(tline, "Colorset", 8) == 0){
        LoadColorset(&tline[8]);
      }
      else if(strncasecmp(tline, XINERAMA_CONFIG_STRING,
			  strlen(XINERAMA_CONFIG_STRING)) == 0){
	XineramaSupportConfigureModule(tline + strlen(XINERAMA_CONFIG_STRING));
      }
    }
    GetConfigLine(fd,&tline);
  }

  if(app_win == 0)
    fvwmlib_get_target_window(dpy, screen, MyName, &app_win, True);

  fd_width = GetFdWidth();

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo(fd,"Send_WindowList",0);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);
  if (app_win == Root)
    exit(0);

  Loop(fd);
  return 0;
}

/**************************************************************************
 *
 * Read the entire window list from fvwm
 *
 *************************************************************************/
void Loop(int *fd)
{
  while (1)
  {
    FvwmPacket* packet = ReadFvwmPacket(fd[1]);
    if ( packet == NULL )
      exit(0);
    else
      process_message( packet->type, packet->body );
  }
}


/**************************************************************************
 *
 * Process window list messages
 *
 *************************************************************************/
void process_message(unsigned long type,unsigned long *body)
{
  switch(type)
    {
    /* should turn off this packet but it comes after config_list so have to
       accept at least one */
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




/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
static RETSIGTYPE
TerminateHandler(int sig)
{
  fvwmSetTerminate(sig);
}

/***********************************************************************
 *
 * Got window configuration info - if its our window, save data
 *
 ***********************************************************************/
void list_configure(unsigned long *body)
{
  struct ConfigWinPacket  *cfgpacket = (void *) body;

  if((app_win == cfgpacket->frame)||(app_win == cfgpacket->w)
     ||((cfgpacket->icon_w != 0)&&(app_win == cfgpacket->icon_w))
     ||((cfgpacket->icon_pixmap_w)&&(app_win == cfgpacket->icon_pixmap_w)))
    {
      app_win = cfgpacket->frame;
      target.id = cfgpacket->w;
      target.frame = cfgpacket->frame;
      target.frame_x = cfgpacket->frame_x;
      target.frame_y = cfgpacket->frame_y;
      target.frame_w = cfgpacket->frame_width;
      target.frame_h = cfgpacket->frame_height;
      target.desktop = cfgpacket->desk;
      target.layer = cfgpacket->layer;
      memcpy(&target.flags, &(cfgpacket->flags), sizeof(cfgpacket->flags));
      target.title_h = cfgpacket->title_height;
      target.border_w = cfgpacket->border_width;
      target.base_w = cfgpacket->hints_base_width;
      target.base_h = cfgpacket->hints_base_height;
      target.width_inc = cfgpacket->hints_width_inc;
      target.height_inc = cfgpacket->hints_height_inc;
      target.gravity = cfgpacket->hints_win_gravity;
      found = 1;
    }
}

/*************************************************************************
 *
 * Capture  Window name info
 *
 ************************************************************************/
void list_window_name(unsigned long *body)
{
  if((app_win == (Window)body[1])||(app_win == (Window)body[0]))
    {
      strncpy(target.name,(char *)&body[3],255);
    }
}

/*************************************************************************
 *
 * Capture  Window Icon name info
 *
 ************************************************************************/
void list_icon_name(unsigned long *body)
{
  if((app_win == (Window)body[1])||(app_win == (Window)body[0]))
    {
      strncpy(target.icon_name,(char *)&body[3],255);
    }
}


/*************************************************************************
 *
 * Capture  Window class name info
 *
 ************************************************************************/
void list_class(unsigned long *body)
{
  if((app_win == (Window)body[1])||(app_win == (Window)body[0]))
    {
      strncpy(target.class,(char *)&body[3],255);
    }
}


/*************************************************************************
 *
 * Capture  Window resource info
 *
 ************************************************************************/
void list_res_name(unsigned long *body)
{
  if((app_win == (Window)body[1])||(app_win == (Window)body[0]))
    {
      strncpy(target.res,(char *)&body[3],255);
    }
}

/*************************************************************************
 *
 * End of window list, open an x window and display data in it
 *
 ************************************************************************/
XSizeHints mysizehints;
void list_end(void)
{
  XGCValues gcv;
  unsigned long gcm;
  int lmax,height;
  XEvent Event;
  int x,y;
  XSetWindowAttributes attributes;
  int is_key_pressed = 0;
  int is_button_pressed = 0;
#ifdef I18N_MB
  char **ml;
  int mc;
  char *ds;
  XFontStruct **fs_list;
#endif

  if(!found)
    {
/*    fprintf(stderr,"%s: Couldn't find app window\n",MyName); */
      exit(0);
    }

  /* tell fvwm to only send config messages */
  SetMessageMask(fd, M_CONFIG_INFO | M_SENDCONFIG);

#ifdef I18N_MB
  if ((fontset = XCreateFontSet(dpy, font_string, &ml, &mc, &ds)) == NULL) {
#ifdef STRICTLY_FIXED
      if ((fontset = XCreateFontSet(dpy, "fixed", &ml, &mc, &ds)) == NULL)
#else
      if ((fontset = XCreateFontSet(dpy, "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*", &ml, &mc, &ds)) == NULL)
#endif
	  exit(1);
  }
  XFontsOfFontSet(fontset, &fs_list, &ml);
  font = fs_list[0];
#else
  /* load the font */
  if ((font = XLoadQueryFont(dpy, font_string)) == NULL)
    if ((font = XLoadQueryFont(dpy, "fixed")) == NULL)
      exit(1);
#endif

  /* make window infomation list */
  MakeList();

  /* size and create the window */
  lmax = max_col1 + max_col2 + 15;

  height = ListSize * (font->ascent + font->descent);

  mysizehints.flags=
    USSize|USPosition|PWinGravity|PResizeInc|PBaseSize|PMinSize|PMaxSize;
  mysizehints.width = lmax + 10;
  mysizehints.height= height + 10;
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

    if (!XQueryPointer(
	  dpy, Root, &JunkW, &JunkW, &x, &y, &JunkC, &JunkC, &JunkM))
    {
      x = 0;
      y = 0;
    }

    XineramaSupportGetCurrentScrRect(NULL, &sx, &sy, &sw, &sh);
    XineramaSupportGetScrRect(x, y, &sx, &sy, &sw, &sh);
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
  mysizehints.x = x;
  mysizehints.y = y;

  if (Pdepth < 2)
  {
    attributes.background_pixel = GetColor("white");
    fore_pix = GetColor("black");
  }
  else
  {
    attributes.background_pixel = (colorset < 0)
				  ? GetColor(BackColor)
    				  : Colorset[colorset].bg;
    fore_pix = (colorset < 0) ? GetColor(ForeColor)
			      : Colorset[colorset].fg;
  }

  attributes.colormap = Pcmap;
  attributes.border_pixel = 0;
  main_win = XCreateWindow(dpy, Root, x, y,
			   mysizehints.width, mysizehints.height, 0, Pdepth,
			   InputOutput, Pvisual,
			   CWColormap | CWBackPixel | CWBorderPixel,
			   &attributes);
  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(dpy,main_win,&wm_del_win,1);

  XSetWMNormalHints(dpy,main_win,&mysizehints);
  /* have to ask for configure events when transparent */
  if (colorset >= 0 && Colorset[colorset].pixmap == ParentRelative)
    mw_events |= StructureNotifyMask;
  XSelectInput(dpy, main_win, mw_events);
  change_window_name(&MyName[1]);

  gcm = GCForeground|GCFont;
  gcv.foreground = fore_pix;
  gcv.font = font->fid;
  gc = fvwmlib_XCreateGC(dpy, main_win, gcm, &gcv);

  if (colorset >= 0)
    SetWindowBackground(dpy, main_win, mysizehints.width, mysizehints.height,
			&Colorset[(colorset)], Pdepth, gc, True);

  XMapWindow(dpy,main_win);

  /* Window is created. Display it until the user clicks or deletes it. */
  /* also grok any dynamic config changes */
  while(1)
  {
    FvwmPacket* packet;
    int x_fd = XConnectionNumber(dpy);
    fd_set fdset;
    char buf[32];

    FD_ZERO(&fdset);
    FD_SET(fd[1], &fdset);
    FD_SET(x_fd, &fdset);

    /* process all X events first */
    while (XPending(dpy)) {
      XNextEvent(dpy,&Event);
      switch(Event.type) {
        case Expose:
          while (XCheckTypedEvent(dpy, Expose, &Event)) ;
	  RedrawWindow();
	  break;
        case KeyPress:
	  is_key_pressed = 1;
	  break;
        case ButtonPress:
	  is_button_pressed = Event.xbutton.button;
	  break;
        case KeyRelease:
	  if (is_key_pressed)
	    exit(0);
	  else
	    break;
        case ButtonRelease:
	  if (is_button_pressed)
	  {
	    if (is_button_pressed == 2 && Event.xbutton.button == 2)
	    {
	      /* select a new window when button 2 is pressed */
	      SetMessageMask(
		fd, M_CONFIGURE_WINDOW | M_WINDOW_NAME | M_ICON_NAME
		| M_RES_CLASS | M_RES_NAME | M_END_WINDOWLIST | M_CONFIG_INFO
		| M_END_CONFIG_INFO | M_SENDCONFIG);
	      SendInfo(fd,"Send_WindowList",0);
	      XDestroyWindow(dpy, main_win);
	      DestroyList();
	      fvwmlib_get_target_window(dpy, screen, MyName, &app_win, True);
	      found = 0;
	      return;
	    }
	    else
	    {
	      exit(0);
	    }
	  }
	  break;
        case ClientMessage:
	  if (Event.xclient.format==32 && Event.xclient.data.l[0]==wm_del_win)
	    exit(0);
	  break;
	case ReparentNotify:
	  sprintf(buf, "Layer 0 %d", (int)target.layer);
	  SendInfo(fd, buf, main_win);
	  SendInfo(fd, "Raise", main_win);
	  break;
	case ConfigureNotify:
	{
	  /* this only happens with transparent windows, slurp up as many
	   * events as possible before redrawing to reduce flickering */
	  XEvent event;
	  while (XCheckTypedEvent(dpy, ConfigureNotify, &event))
	  {
	    if (!event.xconfigure.send_event)
	      continue;
            Event.xconfigure.x = event.xconfigure.x;
            Event.xconfigure.y = event.xconfigure.y;
            Event.xconfigure.send_event = True;
	  }
	  /* only refresh if moved */
	  if (x != Event.xconfigure.x || y != Event.xconfigure.y)
	  {
	    x = Event.xconfigure.x;
	    y = Event.xconfigure.y;
	    /* flush any expose events */
	    while (XCheckTypedEvent(dpy, Expose, &Event)) ;
	    XClearWindow(dpy, main_win);
	    RedrawWindow();
	  }
	  break;
	}
      }
    }

    /* wait for X-event or config line */
    select( fd_width, SELECT_FD_SET_CAST &fdset, NULL, NULL, NULL );

    /* parse any dynamic config lines */
    if (FD_ISSET(fd[1], &fdset)) {
      char *tline, *token;

      packet = ReadFvwmPacket(fd[1]);
      if (packet == NULL)
	exit(0);
      if (colorset >= 0 && packet && packet->type == M_CONFIG_INFO) {
	tline = (char*)&(packet->body[3]);
	tline = GetNextToken(tline, &token);
	if (StrEquals(token, "Colorset")) {
	  /* track all colorset changes and update display if necessary */
	  if (LoadColorset(tline) == colorset) {
	    XSetForeground(dpy, gc, Colorset[colorset].fg);
	    SetWindowBackground(dpy, main_win, mysizehints.width,
				mysizehints.height,
				&Colorset[colorset], Pdepth, gc,
				True);
	    /* ask for movement events iff transparent */
	    if (Colorset[colorset].pixmap == ParentRelative)
	      mw_events |= StructureNotifyMask;
	    else
	      mw_events &= ~StructureNotifyMask;
	    XSelectInput(dpy, main_win, mw_events);
	  }
	}
	else if (StrEquals(token, XINERAMA_CONFIG_STRING)) {
	  XineramaSupportConfigureModule(tline);
	}
	free(token);
      }
    }
  }
}



/************************************************************************
 *
 * Draw the window
 *
 ***********************************************************************/
void RedrawWindow(void)
{
  int fontheight,i=0;
  struct Item *cur = itemlistRoot;

  fontheight = font->ascent + font->descent;

  while(cur != NULL)
    {
      /* first column */
#ifdef I18N_MB
      XmbDrawString(dpy, main_win, fontset, gc, 5,
		  5 + font->ascent + i * fontheight, cur->col1,
		  strlen(cur->col1));
#else
      XDrawString(dpy, main_win, gc, 5,
		  5 + font->ascent + i * fontheight, cur->col1,
		  strlen(cur->col1));
#endif
      /* second column */
#ifdef I18N_MB
      XmbDrawString(dpy, main_win, fontset, gc, 10 + max_col1,
		  5 + font->ascent + i * fontheight, cur->col2,
		  strlen(cur->col2));
#else
      XDrawString(dpy, main_win, gc, 10 + max_col1,
		  5 + font->ascent + i * fontheight, cur->col2,
		  strlen(cur->col2));
#endif
      ++i;
      cur = cur->next;
    }
}

/**************************************************************************
 *  Change the window name displayed in the title bar.
 **************************************************************************/
void change_window_name(char *str)
{
  XTextProperty name;
  XClassHint myclasshints;

  if (XStringListToTextProperty(&str,1,&name) == 0)
    {
      fprintf(stderr,"%s: cannot allocate window name",MyName);
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

/**************************************************************************
*
* Add s1(string at first column) and s2(string at second column) to itemlist
*
 *************************************************************************/
void AddToList(char *s1, char* s2)
{
  int tw1, tw2;
  struct Item* item, *cur = itemlistRoot;

  tw1 = XTextWidth(font, s1, strlen(s1));
  tw2 = XTextWidth(font, s2, strlen(s2));
  max_col1 = max_col1 > tw1 ? max_col1 : tw1;
  max_col2 = max_col2 > tw2 ? max_col2 : tw2;

  item = (struct Item*)safemalloc(sizeof(struct Item));

  item->col1 = s1;
  item->col2 = s2;
  item->next = NULL;

  if (cur == NULL)
    itemlistRoot = item;
  else {
    while(cur->next != NULL)
       cur = cur->next;
    cur->next = item;
  }
  ListSize++;
}

void MakeList(void)
{
  int bw,width,height,x1,y1,x2,y2;
  char loc[20];
  static char xstr[6],ystr[6];
  /* GSFR - quick hack because the new macros depend on a prt reference  */
  struct target_struct  *targ  =  &target;

  ListSize = 0;

  bw = 2*target.border_w;
  width = target.frame_w - bw;
  height = target.frame_h - target.title_h - bw;

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
  AddToList("Layer:",         layer);
  AddToList("Width:",         swidth);
  AddToList("Height:",        sheight);
  AddToList("X (current page):",   xstr);
  AddToList("Y (current page):",   ystr);
  AddToList("Boundary Width:", borderw);

  AddToList("Sticky:",        (IS_STICKY(targ)    ? yes : no));
  AddToList("NoTitle:",       (HAS_TITLE(targ)    ? no : yes));
  AddToList("Iconified:",     (IS_ICONIFIED(targ) ? yes : no));
  AddToList("Transient:",     (IS_TRANSIENT(targ) ? yes : no));

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
    x1 = 0;
  x2 = DisplayWidth(dpy,screen) - x1 - target.frame_w;
  if(x2 < 0)
    x2 = 0;
  y1 = target.frame_y;
  if(y1 < 0)
    y1 = 0;
  y2 = DisplayHeight(dpy,screen) - y1 -  target.frame_h;
    if(y2 < 0)
    y2 = 0;
  width = (width - target.base_w)/target.width_inc;
  height = (height - target.base_h)/target.height_inc;

  sprintf(loc,"%dx%d",width,height);
  strcpy(geometry, loc);

  if ((target.gravity == EastGravity) ||(target.gravity == NorthEastGravity)||
      (target.gravity == SouthEastGravity))
    sprintf(loc,"-%d",x2);
  else
    sprintf(loc,"+%d",x1);
  strcat(geometry, loc);

  if((target.gravity == SouthGravity)||(target.gravity == SouthEastGravity)||
     (target.gravity == SouthWestGravity))
    sprintf(loc,"-%d",y2);
  else
    sprintf(loc,"+%d",y1);
  strcat(geometry, loc);
  AddToList("Geometry:", geometry);

  {
    Atom *protocols = NULL, *ap;
    Atom _XA_WM_TAKE_FOCUS = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
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
      long supplied_return;             /* flags, hints that were supplied */
      int getrc;
      XSizeHints *size_hints = XAllocSizeHints(); /* the size hints */
      if ((getrc = XGetWMSizeHints(dpy,target.id, /* get size hints */
                          size_hints,    /* Hints */
                          &supplied_return,
                          XA_WM_ZOOM_HINTS))) {
        if (supplied_return & PAspect) { /* if window has a aspect ratio */
          sprintf(mymin_aspect, "%d/%d", size_hints->min_aspect.x,
                  size_hints->min_aspect.y);
          AddToList("Minimum aspect ratio:",mymin_aspect);
          sprintf(max_aspect, "%d/%d", size_hints->max_aspect.x,
                  size_hints->max_aspect.y);
          AddToList("Maximum aspect ratio:",max_aspect);
        } /* end aspect ratio */
        XFree(size_hints);
      } /* end getsizehints worked */
    }
  }
}
