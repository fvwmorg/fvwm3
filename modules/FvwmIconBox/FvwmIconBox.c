/* FvwmIconBox Module --- Copyright 1994, 1995 Nobutaka Suzuki.
 *
 * No guarantees or warantees or anything are provided or implied in
 * any way whatsoever. Use this program at your own risk. Permission
 * to use this program for any purpose is given, as long as the
 * copyright is kept intact.
 *
 * The functions based on part of GoodStuff, FvwmScroll, FvwmWinList
 * and Fvwm are noted by a small copyright atop that function.
 * Full original copyright is described in COPYRIGHT.
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

#define NONE 0
#define VERTICAL 1
#define HORIZONTAL 2

#include "config.h"

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "libs/Module.h"
#include "libs/fvwmsignal.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "libs/defaults.h"
#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/FRender.h"
#include "libs/FRenderInit.h"

#include "libs/Colorset.h"
#include "fvwm/fvwm.h"
#include "libs/PictureGraphics.h"
#include "FvwmIconBox.h"


char *MyName;

Display *dpy;			/* which display are we talking to */
int x_fd;
fd_set_size_t fd_width;

Window Root;
int screen;

char *Back = "#5f9ea0";
char *Fore = "#82dfe3";
char *IconBack = "#cfcfcf";
char *IconFore = "black";
char *ActIconBack = "white";
char *ActIconFore = "black";
char *font_string = "fixed";
int colorset = -1;
int Iconcolorset = -1;
int IconHicolorset = -1;
Bool UseSkipList = False;
Bool Swallowed = False;

/* same strings as in misc.c */
char NoClass[] = "NoClass";
char NoResource[] = "NoResource";

Pixel fore_pix, hilite_pix, back_pix, shadow_pix;
Pixel icon_fore_pix, icon_back_pix, icon_hilite_pix, icon_shadow_pix;
Pixel act_icon_fore_pix, act_icon_back_pix,
  act_icon_hilite_pix, act_icon_shadow_pix;

GC  NormalGC,ShadowGC,ReliefGC,IconShadowGC,IconReliefGC;
FlocaleFont *Ffont;
FlocaleWinString *FwinString;
Window main_win;
Window holder_win;
Window icon_win;
Window h_scroll_bar;
Window v_scroll_bar;
Window l_button, r_button, t_button, b_button;
Window Pressed = None;

long CurrentDesk;

int Width, Height;
int UWidth, UHeight;

#define MW_EVENTS   (KeyPressMask| ExposureMask | StructureNotifyMask|\
		     ButtonReleaseMask | ButtonPressMask )
#define SCROLL_EVENTS (ExposureMask | StructureNotifyMask|\
		       ButtonReleaseMask |ButtonPressMask | PointerMotionMask)
#define BUTTON_EVENTS (ExposureMask | StructureNotifyMask|\
		       ButtonReleaseMask | ButtonPressMask |\
		       LeaveWindowMask | PointerMotionMask)

unsigned long m_mask = M_CONFIGURE_WINDOW|M_ADD_WINDOW|M_DESTROY_WINDOW|
  M_END_WINDOWLIST| M_ICONIFY|M_DEICONIFY|
  M_RES_NAME|M_RES_CLASS|M_VISIBLE_NAME|M_ICON_FILE|
  M_DEFAULTICON|M_CONFIG_INFO|M_END_CONFIG_INFO;
unsigned long mx_mask = MX_VISIBLE_ICON_NAME|MX_PROPERTY_CHANGE;

struct icon_info *Hilite;
int main_width, main_height;
int num_icons = 0;
int num_rows = 1;
int num_columns = 6;
int Lines = 6;
int max_icon_width = 48,max_icon_height = 48;
int ButtonWidth,ButtonHeight;
int geom_x = -100000, geom_y = -100000;
int gravity = NorthWestGravity;
int icon_win_x = 0, icon_win_y = 0, icon_win_width = 100,
icon_win_height = 100;
int interval = 8;
int motion = NONE;
int primary = LEFT, secondary = BOTTOM;
int xneg = 0, yneg = 0;
int ClickTime = DEFAULT_CLICKTIME;
unsigned int bar_width = 9;
int redraw_flag = 3;
int icon_relief = 4;
int margin1 = 8;
int margin2 = 6;

Pixmap IconwinPixmap = None;
char *IconwinPixmapFile = NULL;

int h_margin;
int v_margin;

int fd[2];

struct icon_info *Head = NULL;
struct icon_info *Tail = NULL;
struct iconfile *IconListHead = NULL;
struct iconfile *IconListTail = NULL;
struct iconfile *DefaultIcon = NULL;
struct mousefunc *MouseActions = NULL;
struct keyfunc *KeyActions = NULL;
char *imagePath = NULL;
char *FvwmDefaultIcon = NULL;

static Atom wm_del_win;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_NAME;

int ready = 0;
unsigned long local_flags = 0;
int sortby = UNSORT;

char* AnimCommand = NULL;

int save_color_limit = 0;                   /* color limit from config */
static Bool have_double_click = False;
static Bool is_dead_pipe = False;

static RETSIGTYPE TerminateHandler(int);
static int myErrorHandler(Display *dpy, XErrorEvent *event);
static void CleanUp(void);
static void change_colorset(int color);

Bool do_allow_bad_access = False;
Bool was_bad_access = False;

/************************************************************************
  Main
  Based on main() from GoodStuff:
  	Copyright 1993, Robert Nation.
************************************************************************/
int main(int argc, char **argv)
{
  char *display_name = NULL;
  char *temp, *s;
  XIconSize* size;

  FlocaleInit(LC_CTYPE, "", "", "FvwmIconBox");

  temp = argv[0];
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  if((argc != 6)&&(argc != 7)) {
    fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",temp,
	    VERSION);
    exit(1);
  }

  /* alise support */
  if (argc == 7 && argv[6] != NULL)
  {
    MyName = safemalloc(strlen(argv[6])+1);
    strcpy(MyName, argv[6]);
  }
  else
  {
    MyName = safemalloc(strlen(temp)+1);
    strcpy(MyName, temp);
  }

#ifdef HAVE_SIGACTION
  {
    struct sigaction  sigact;

#ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT;
#else
    sigact.sa_flags = 0;
#endif
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGPIPE);
    sigaddset(&sigact.sa_mask, SIGTERM);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigact.sa_handler = TerminateHandler;

    sigaction(SIGPIPE, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGINT,  &sigact, NULL);
  }
#else
  /*
   * No sigaction, so fall back onto standard and unreliable signals
   */
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGPIPE) | sigmask(SIGTERM) | sigmask(SIGINT) );
#endif

  signal(SIGPIPE, TerminateHandler);
  signal(SIGTERM, TerminateHandler);
  signal(SIGINT, TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGTERM, 1);
  siginterrupt(SIGINT, 1);
#endif
#endif

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  if (!(dpy = XOpenDisplay(display_name)))
    {
      fprintf(stderr,"%s: can't open display %s", MyName,
	      XDisplayName(display_name));
      exit (1);
    }
  PictureInitCMap(dpy);
  FScreenInit(dpy);
  AllocColorset(0);
  FShapeInit(dpy);
  FRenderInit(dpy);

  x_fd = XConnectionNumber(dpy);

  fd_width = GetFdWidth();

  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);
  if(Root == None)
    {
      fprintf(stderr,"%s: Screen %d is not valid ", MyName, screen);
      exit(1);
    }

  XSetErrorHandler(myErrorHandler);

  ParseOptions();
  /* m_mask/mx_mask may have changed in the ParseOptions call*/
  SetMessageMask(fd, m_mask);
  SetMessageMask(fd, mx_mask);

  if ((local_flags & SETWMICONSIZE) && (size = XAllocIconSize()) != NULL){
    size->max_width  = size->min_width  = max_icon_width + icon_relief;
    /* max_height should be >0 */
    size->max_height = size->min_height = max(1, max_icon_height) + icon_relief;
    size->width_inc  = size->height_inc = 0;
    XSetIconSizes(dpy, Root, size, 1);
    XFree(size);
  }

  CreateWindow();

  SendText(fd,"Send_WindowList",0);
  atexit(CleanUp);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);

  /* Lock on send only for iconify and deiconify (for NoIconAction) */
  SetSyncMask(fd, M_DEICONIFY | M_ICONIFY);
  SetNoGrabMask(fd, M_DEICONIFY | M_ICONIFY);

  Loop();
 return 0;
}



/************************************************************************
  Loop
  Based on Loop() from GoodStuff:
      Copyright 1993, Robert Nation.
************************************************************************/
void Loop(void)
{
  Window root;
  struct icon_info *tmp, *exhilite;
  int x, y, border_width, depth, i;
  XEvent Event;
  int tw, th;
  int diffx, diffy;
  int oldw, oldh;

  while( !isTerminated )
    {
      if(My_XNextEvent(dpy,&Event))
	{
	  switch(Event.type)
	    {
	    case Expose:
	      if(Event.xexpose.count == 0){
		if (Event.xany.window == main_win)
		{
			RedrawWindow();
		}
		else
		{
		  tmp = Head;
		  while(tmp != NULL){
		    if (Event.xany.window == tmp->icon_pixmap_w){
		      RedrawIcon(tmp, 1);
		      break;
		    }else if(Event.xany.window == tmp->IconWin){
		      RedrawIcon(tmp, 2);
		      break;
		    }
		    tmp = tmp->next;
		  }
		}
	      }
	      break;

	    case ConfigureNotify:
	      if (Event.xconfigure.window == icon_win &&
		  (colorset < 0 ||
		   Colorset[colorset].pixmap != ParentRelative))
	      {
		/* needs this event for transparency */
		break;
	      }
	      if (!XGetGeometry(dpy,main_win,&root,&x,&y,
				(unsigned int *)&tw,(unsigned int *)&th,
				(unsigned int *)&border_width,
				(unsigned int *)&depth))
	      {
		break;
	      }
	      if (ready && (tw != main_width || th != main_height)){
		main_width = tw;
		main_height= th;
		oldw = Width;
		oldh = Height;
		num_columns = (tw - h_margin - interval + 1) / UWidth;
		num_rows = (th - v_margin - interval + 1) / UHeight;
		Width = UWidth * num_columns + interval - 1;
		Height = UHeight * num_rows + interval -1;
		XMoveResizeWindow(dpy, holder_win, margin1+2,
				  margin1+2,
				  tw - h_margin, th - v_margin);
		if (!(local_flags & HIDE_H))
		  XResizeWindow(dpy, h_scroll_bar,
				Width - bar_width*2, bar_width);
		if (!(local_flags & HIDE_V))
		  XResizeWindow(dpy ,v_scroll_bar,
				bar_width, Height - bar_width*2);
		GetIconwinSize(&diffx, &diffy);
		if (primary == BOTTOM || secondary == BOTTOM)
		  icon_win_y -= Height - oldh;
		if (primary == RIGHT || secondary == RIGHT)
		  icon_win_x -= Width - oldw;
		if (icon_win_x < 0)
		  icon_win_x = 0;
		if (icon_win_y < 0)
		  icon_win_y = 0;
		if (icon_win_x + Width > icon_win_width)
		  icon_win_x = icon_win_width - Width;
		if (icon_win_y + Height > icon_win_height)
		  icon_win_y = icon_win_height - Height;
		XMoveResizeWindow(dpy, icon_win, -icon_win_x,
				  -icon_win_y, icon_win_width,
				  icon_win_height);
		AdjustIconWindows();
		if (colorset >= 0)
		  change_colorset(colorset);
		else {
		  XClearWindow(dpy,main_win);
		  RedrawWindow();
		}
	      }
	      else if (ready &&
		       (Event.xconfigure.send_event ||
			Event.xconfigure.window == icon_win) && colorset >= 0 &&
		       Colorset[colorset].pixmap == ParentRelative)
	      {
		/* moved */
		XClearArea(dpy, icon_win, 0,0,0,0, False);
		/* XClearArea(dpy, icon_win, 0,0,0,0, True); is not enough
		 * when you unshade */
		RedrawWindow();
	      }
	      break;
	    case KeyPress:
	      ExecuteKey(Event);
	      break;
	    case ButtonPress:
	      if (!(local_flags & HIDE_H)){
		if (Event.xbutton.window == h_scroll_bar)
		  motion = HORIZONTAL;
		else if (Event.xbutton.window == l_button){
		  Pressed = l_button;
		  RedrawLeftButton(ShadowGC, ReliefGC);
		}
		else if (Event.xbutton.window == r_button){
		  Pressed = r_button;
		  RedrawRightButton(ShadowGC, ReliefGC);
		}
	      }
	      if (!(local_flags & HIDE_V)){
		if (Event.xbutton.window == v_scroll_bar)
		  motion = VERTICAL;
		else if (Event.xbutton.window == t_button){
		  Pressed = t_button;
		  RedrawTopButton(ShadowGC, ReliefGC);
		}
		else if (Event.xbutton.window == b_button){
		  Pressed = b_button;
		  RedrawBottomButton(ShadowGC, ReliefGC);
		}
	      }
	      if ((tmp = Search(Event.xbutton.window)) != NULL)
		ExecuteAction(Event.xbutton.x, Event.xbutton.y, tmp);
	      break;

	    case ButtonRelease:
	      if (!(local_flags & HIDE_H)){
		if (Event.xbutton.window == h_scroll_bar && motion ==
		    HORIZONTAL)
		  HScroll(Event.xbutton.x * icon_win_width / Width);
		else if (Event.xbutton.window == l_button && Pressed ==
			 l_button){
		  Pressed = None;
		  RedrawLeftButton(ReliefGC, ShadowGC);
		  HScroll(icon_win_x - UWidth);
		}
		else if (Event.xbutton.window == r_button && Pressed ==
			 r_button){
		  Pressed = None;
		  RedrawRightButton(ReliefGC, ShadowGC);
		  HScroll(icon_win_x + UWidth);
		}
	      }
	      if (!(local_flags & HIDE_V)){
		if (Event.xbutton.window == v_scroll_bar && motion
		    == VERTICAL)
		  VScroll(Event.xbutton.y * icon_win_height / Height);
		else if (Event.xbutton.window == t_button && Pressed ==
			 t_button){
		  Pressed = None;
		  RedrawTopButton(ReliefGC, ShadowGC);
		  VScroll(icon_win_y - UHeight);
		}
		else if (Event.xbutton.window == b_button && Pressed ==
			 b_button){
		  Pressed = None;
		  RedrawBottomButton(ReliefGC, ShadowGC);
		  VScroll(icon_win_y + UHeight);
		}
	      }
	      motion = NONE;
	      break;

	    case MotionNotify:
	      if (motion == VERTICAL){
		VScroll(Event.xbutton.y * icon_win_height / Height);
	      }else if (motion == HORIZONTAL){
		HScroll(Event.xbutton.x * icon_win_width / Width);
	      }
	      break;
	    case EnterNotify:
	      if ((tmp = Search(Event.xcrossing.window)) != NULL)
		if ((exhilite = Hilite) != tmp){
		  Hilite = tmp;
		  if (exhilite != NULL)
		    RedrawIcon(exhilite, redraw_flag);
		  RedrawIcon(tmp, redraw_flag);
		}
	      break;

	    case LeaveNotify:
	      if ((tmp = Search(Event.xcrossing.window)) != NULL &&
		  tmp == Hilite){
		Hilite = NULL;
		RedrawIcon(tmp, redraw_flag);
	      }
	      if (!(local_flags & HIDE_H) && Event.xbutton.window ==
		  l_button && Pressed == l_button){
		Pressed = None;
		RedrawLeftButton(ReliefGC, ShadowGC);
	      }
	      else if (!(local_flags & HIDE_H) && Event.xbutton.window ==
		       r_button && Pressed == r_button){
		Pressed = None;
		RedrawRightButton(ReliefGC, ShadowGC);
	      }
	      else if (!(local_flags & HIDE_V) && Event.xbutton.window ==
		       t_button && Pressed == t_button){
		Pressed = None;
		RedrawTopButton(ReliefGC, ShadowGC);
	      }
	      else if (!(local_flags & HIDE_V) && Event.xbutton.window ==
		       b_button && Pressed == b_button){
		Pressed = None;
		RedrawBottomButton(ReliefGC, ShadowGC);
	      }
	      break;
	    case ClientMessage:
	      if ((Event.xclient.format==32) &&
		  (Event.xclient.data.l[0]==wm_del_win))
		exit (0);
	      break;
	    case PropertyNotify:
	      switch (Event.xproperty.atom){
	      case XA_WM_HINTS:
		if (Event.xproperty.state == PropertyDelete)
		  break;
		tmp = Head;
		i=0;
		while(tmp != NULL){
		  if (Event.xproperty.window == tmp->id)
		    break;
		  tmp = tmp->next;
		  ++i;
		}
		if (tmp == NULL || tmp->wmhints == NULL ||
		    !(tmp->extra_flags & DEFAULTICON))
		  break;
		if (tmp->wmhints)
		  XFree (tmp->wmhints);
		tmp->wmhints = XGetWMHints(dpy, tmp->id);
		if (tmp->wmhints && (tmp->wmhints->flags & IconPixmapHint)){
		  /* turn off "old" shape mask */
		  if (FShapesSupported && tmp->icon_maskPixmap != None)
		  {
		    FShapeCombineMask(
		      dpy, tmp->icon_pixmap_w, FShapeBounding, 0, 0, None,
		      FShapeSet);
		  }
		  if (tmp->iconPixmap != None)
		    XFreePixmap(dpy, tmp->iconPixmap);
		  GetIconBitmap(tmp);
		  if (FShapesSupported && tmp->icon_maskPixmap != None)
		  {
		    int hr;
		    hr =
		      (Pdefault || (tmp->icon_depth == 1) ||
		       IS_PIXMAP_OURS(tmp)) ? icon_relief/2 : 0;
		    /* This XSync is necessary, do not remove */
		    /* without it the icon window will disappear */
		    /* server: Exceed, client Sparc/Solaris 2.6 */
		    /* don't know if this is a server or xlib bug */
		    XSync(dpy, False);
		    FShapeCombineMask(
		      dpy, tmp->icon_pixmap_w, FShapeBounding, hr, hr,
		      tmp->icon_maskPixmap, FShapeSet);
		  }
		  AdjustIconWindow(tmp, i);
		  if (max_icon_height != 0)
		  RedrawIcon(tmp, 1);
		}
		break;
	      }
	      break;

	    default:
	      break;
	    }
	}
    }
  return;
}

void HScroll(int x)
{
  int oldx = icon_win_x;

  if (x + Width > icon_win_width)
    x = icon_win_width - Width;
  if (x < 0)
    x = 0;
  if (oldx != x){
    icon_win_x = x;
    XMoveWindow(dpy,icon_win, -icon_win_x, -icon_win_y);
    if (!(local_flags & HIDE_H))
      RedrawHScrollbar();
  }
}

void VScroll(int y)
{
  int oldy = icon_win_y;

  if (y + Height > icon_win_height)
    y = icon_win_height - Height;
  if (y < 0)
    y = 0;
  if (oldy != y){
    icon_win_y = y;
    XMoveWindow(dpy,icon_win, -icon_win_x, -icon_win_y);
    if (!(local_flags & HIDE_V))
      RedrawVScrollbar();
  }
}

struct icon_info *Search(Window w)
{
  struct icon_info *tmp;

  tmp = Head;
  while (tmp != NULL){
    if (tmp->IconWin == w || tmp->icon_pixmap_w == w)
      return tmp;
    tmp = tmp->next;
  }
  return NULL;
}

/************************************************************************
 *
 * Draw the window
 *
 ***********************************************************************/
void RedrawWindow(void)
{
  XEvent dummy;

  while (XCheckTypedWindowEvent (dpy, main_win, Expose, &dummy));

  RelieveRectangle(dpy, main_win, margin1, margin1, Width + 3,
                   Height + 3, ShadowGC, ReliefGC, 2);
  if (!(local_flags & HIDE_H))
    RelieveRectangle(dpy, main_win, margin1, margin1 + 4 + Height + margin2,
		     Width + 3, bar_width + 3, ShadowGC, ReliefGC, 2);
  if (!(local_flags & HIDE_V))
    RelieveRectangle(dpy, main_win, margin1 + 4 + Width + margin2, margin1,
		     bar_width + 3, Height + 3, ShadowGC, ReliefGC, 2);
  RelieveRectangle(dpy, main_win, 0, 0, Width -1 + h_margin, Height -1 + v_margin,
		   ReliefGC, ShadowGC, 2);

  /* scroll bar */
  if (!(local_flags & HIDE_H))
    RedrawHScrollbar();
  if (!(local_flags & HIDE_V))
    RedrawVScrollbar();

  /* buttons */
  if (!(local_flags & HIDE_H)){
    RedrawLeftButton(ReliefGC, ShadowGC);
    RedrawRightButton(ReliefGC, ShadowGC);
  }
  if (!(local_flags & HIDE_V)){
    RedrawTopButton(ReliefGC, ShadowGC);
    RedrawBottomButton(ReliefGC, ShadowGC);
  }

  /* icons */
  RedrawIcons();
}

void RedrawIcons(void)
{
  struct icon_info *tmp;

  tmp = Head;
  while(tmp != NULL){
    if (window_cond(tmp))
      RedrawIcon(tmp, redraw_flag);
    tmp = tmp->next;
  }
}

void RedrawIcon(struct icon_info *item, int f)
{
	unsigned long plane = 1;
	int hr, len;
	int diff, lm ,w, h, tw;
	char label[256];

	hr = icon_relief/2;

	if (Hilite == item)
	{
		XSetForeground(dpy, NormalGC, act_icon_fore_pix);
		XSetBackground(dpy, NormalGC, act_icon_back_pix);
		XSetForeground(dpy, IconReliefGC, act_icon_hilite_pix);
		XSetForeground(dpy, IconShadowGC, act_icon_shadow_pix);

		if (max_icon_height != 0 && (IS_ICON_OURS(item)))
		{
			if (item->icon_alphaPixmap != None)
				XSetWindowBackgroundPixmap(dpy,
							   item->icon_pixmap_w,
							   ParentRelative);
			else
				XSetWindowBackground(dpy, item->icon_pixmap_w,
						     act_icon_back_pix);
		}
		XSetWindowBackground(dpy, item->IconWin, act_icon_back_pix);
	}

	/* icon pixmap */
	if ((f & 1) && (IS_ICON_OURS(item)))
	{
		if (item->iconPixmap != None && item->icon_pixmap_w != None)
		{
			if (item->icon_depth == 1)
			{
				XCopyPlane(dpy,
					   item->iconPixmap,
					   item->icon_pixmap_w,
					   NormalGC,
					   0, 0, item->icon_w, item->icon_h,
					   hr, hr, plane);
			}
			else if (IS_PIXMAP_OURS(item))
			{
				if (item->icon_alphaPixmap != None)
					XClearWindow(dpy, item->icon_pixmap_w);
				PGraphicsCopyPixmaps(dpy,
						     item->iconPixmap,
						     item->icon_maskPixmap,
						     item->icon_alphaPixmap,
						     item->icon_depth,
						     item->icon_pixmap_w,
						     NormalGC,
						     0, 0,
						     item->icon_w, item->icon_h,
						     hr, hr);  
			}
			else if (Pdefault)
			{
				XCopyArea(dpy, item->iconPixmap,
					  item->icon_pixmap_w,
					  NormalGC,
					  0, 0,
					  item->icon_w, item->icon_h, hr, hr);
			}
			else
			{
				XCopyArea(dpy, item->iconPixmap,
					  item->icon_pixmap_w,
					  DefaultGC(dpy, screen),
					  0, 0,
					  item->icon_w, item->icon_h, 0, 0);
			}
		}

		if (!(IS_ICON_SHAPED(item))
		    && (Pdefault || (item->icon_depth == 1) ||
			IS_PIXMAP_OURS(item))) 
		{
			if (item->icon_w > 0 && item->icon_h > 0)
				RelieveRectangle(dpy, item->icon_pixmap_w,
						 0, 0, item->icon_w
						 +icon_relief - 1,
						 item->icon_h + icon_relief - 1,
						 IconReliefGC,
						 IconShadowGC, 2);
			else
				RelieveRectangle(dpy, item->icon_pixmap_w,
						 0, 0, max_icon_width
						 + icon_relief - 1,
						 max_icon_height + 
						 icon_relief - 1,
						 IconReliefGC,
						 IconShadowGC, 2);
		}
	}

	/* label */
	if (f & 2)
	{
		w = max_icon_width + icon_relief;
		h = max_icon_height + icon_relief;

		if (IS_ICONIFIED(item))
		{
			sprintf(label, "(%s)", item->name);
		}
		else
		{
			strcpy(label, item->name);
		}

		len = strlen(label);
		tw = FlocaleTextWidth(Ffont, label, len);
		diff = max_icon_width + icon_relief - tw;
		lm = diff/2;
		lm = lm > 4 ? lm : 4;

		FwinString->str = label;
		FwinString->win = item->IconWin;
		FwinString->gc =  NormalGC;
		FwinString->x = lm;
		FwinString->y = 3 + Ffont->ascent;

		if (Hilite == item)
		{
			XRaiseWindow(dpy, item->IconWin);
			XMoveResizeWindow(dpy, item->IconWin,
					  item->x + min(0, (diff - 8))/2,
					  item->y + h,
					  max(tw + 8, w), 6 + Ffont->height);
			XClearWindow(dpy, item->IconWin);
			FlocaleDrawString(dpy, Ffont, FwinString, 0);
			RelieveRectangle(dpy, item->IconWin, 0, 0,
					 max(tw + 8, w) - 1,
					 6 + Ffont->height - 1,
					 IconReliefGC, IconShadowGC, 2);
		}
		else
		{
			XMoveResizeWindow(dpy, item->IconWin,
					  item->x, item->y + h,
					  w, 6 + Ffont->height);
			XClearWindow(dpy, item->IconWin);
			FlocaleDrawString(dpy, Ffont, FwinString, 0);
			RelieveRectangle(dpy, item->IconWin, 0, 0,
					 w - 1, 5 + Ffont->height,
					 IconReliefGC, IconShadowGC, 2);
		}
	}

	if (Hilite == item)
	{
		XSetForeground(dpy, NormalGC, icon_fore_pix);
		XSetBackground(dpy, NormalGC, icon_back_pix);
		XSetForeground(dpy, IconReliefGC, icon_hilite_pix);
		XSetForeground(dpy, IconShadowGC, icon_shadow_pix);

		if (max_icon_height != 0 && (IS_ICON_OURS(item)) && 
		    item->icon_alphaPixmap == None)
		{
			XSetWindowBackground(dpy, item->icon_pixmap_w,
					     icon_back_pix);
		}
		XSetWindowBackground(dpy, item->IconWin, icon_back_pix);
	}
}

void animate(struct icon_info *item, unsigned long *body)
{
  if(item && AnimCommand && (AnimCommand[0] != 0))
  {
    char string[256];
    int abs_x, abs_y;
    Window junkw;

    /* check to see if the centre of the icon is displayed in the iconbox */
    XTranslateCoordinates(dpy, icon_win, main_win, item->x + (item->icon_w/2),
			  item->y + (item->icon_h/2), &abs_x, &abs_y, &junkw);
    if (junkw == None)
      return;

    /* find out where it is on screen */
    XTranslateCoordinates(dpy, icon_win, Root, item->x, item->y, &abs_x, &abs_y,
			  &junkw);
    if (IS_ICONIFIED(item))
    {
      sprintf(string, "%s %d %d %d %d %d %d %d %d",
              AnimCommand,
              (int)body[7], (int)body[8], (int)body[9], (int)body[10],
              abs_x, abs_y, item->icon_w, item->icon_h);
    } else {
      sprintf(string, "%s %d %d %d %d %d %d %d %d",
              AnimCommand,
              abs_x, abs_y, item->icon_w, item->icon_h,
              (int)body[7], (int)body[8], (int)body[9], (int)body[10]);
    }
    SendText(fd, string, 0);
  }
}

/***********************************************************************
 * RedrawHScrollbar
 * 	Based on part of Loop() of GrabWindow.c in FvwmScroll:
 *  		Copyright 1994, Robert Nation.
 ***********************************************************************/
void RedrawHScrollbar(void)
{
  int x,width;

  x = (Width - bar_width*2) * icon_win_x / icon_win_width;
  width = (Width - bar_width*2) * Width / icon_win_width;
  XClearArea(dpy, h_scroll_bar, 0, 0, Width, bar_width,False);
  RelieveRectangle(dpy, h_scroll_bar, x, 0, width - 1, bar_width - 1,
                   ReliefGC, ShadowGC, 2);
}

/***********************************************************************
 * RedrawVScrollbar
 * 	Based on part of Loop() of GrabWindow.c in FvwmScroll:
 *  		Copyright 1994, Robert Nation.
 ***********************************************************************/
void RedrawVScrollbar(void)
{
  int y, height;

  y = (Height - bar_width*2) * icon_win_y / icon_win_height;
  height = (Height - bar_width*2)*  Height / icon_win_height;
  XClearArea(dpy, v_scroll_bar, 0, 0, bar_width, Height,False);
  RelieveRectangle(dpy, v_scroll_bar, 0, y, bar_width - 1, height - 1,
                   ReliefGC, ShadowGC, 2);
}

void RedrawLeftButton(GC rgc, GC sgc)
{
  XSegment seg[4];
  int i=0;

  seg[i].x1 = 1;		seg[i].y1 = bar_width/2;
  seg[i].x2 = bar_width - 2;	seg[i++].y2 = 1;

  seg[i].x1 = 0;		seg[i].y1 = bar_width/2;
  seg[i].x2 = bar_width - 1;	seg[i++].y2 = 0;
  XDrawSegments(dpy, l_button, rgc, seg, i);

  i = 0;
  seg[i].x1 = 1;		seg[i].y1 = bar_width/2;
  seg[i].x2 = bar_width - 2;	seg[i++].y2 = bar_width - 2;

  seg[i].x1 = 0;		seg[i].y1 = bar_width/2;
  seg[i].x2 = bar_width - 1;	seg[i++].y2 = bar_width - 1;

  seg[i].x1 = bar_width - 2;	seg[i].y1 = 1;
  seg[i].x2 = bar_width - 2;	seg[i++].y2 = bar_width - 2;

  seg[i].x1 = bar_width - 1;	seg[i].y1 = 0;
  seg[i].x2 = bar_width - 1;	seg[i++].y2 = bar_width - 1;
  XDrawSegments(dpy, l_button, sgc, seg, i);
}

void RedrawRightButton(GC rgc, GC sgc)
{
  XSegment seg[4];
  int i=0;

  seg[i].x1 = 1;		seg[i].y1 = 1;
  seg[i].x2 = 1;		seg[i++].y2 = bar_width - 2;

  seg[i].x1 = 0;		seg[i].y1 = 0;
  seg[i].x2 = 0;		seg[i++].y2 = bar_width - 1;

  seg[i].x1 = 1;		seg[i].y1 = 1;
  seg[i].x2 = bar_width - 2;	seg[i++].y2 = bar_width/2;

  seg[i].x1 = 0;		seg[i].y1 = 0;
  seg[i].x2 = bar_width - 1;	seg[i++].y2 = bar_width/2;

  XDrawSegments(dpy, r_button, rgc, seg, i);

  i = 0;
  seg[i].x1 = 1;		seg[i].y1 = bar_width - 2;
  seg[i].x2 = bar_width - 2;	seg[i++].y2 = bar_width/2;

  seg[i].x1 = 0;		seg[i].y1 = bar_width - 1;
  seg[i].x2 = bar_width - 1;	seg[i++].y2 = bar_width/2;
  XDrawSegments(dpy, r_button, sgc, seg, i);
}

void RedrawTopButton(GC rgc, GC sgc)
{
  XSegment seg[4];
  int i=0;

  seg[i].x1 = bar_width/2;	seg[i].y1 = 1;
  seg[i].x2 = 1;		seg[i++].y2 = bar_width - 2;

  seg[i].x1 = bar_width/2;	seg[i].y1 = 0;
  seg[i].x2 = 0;		seg[i++].y2 = bar_width - 1;
  XDrawSegments(dpy, t_button, rgc, seg, i);

  i = 0;
  seg[i].x1 = bar_width/2;	seg[i].y1 = 1;
  seg[i].x2 = bar_width - 2;	seg[i++].y2 = bar_width - 2;

  seg[i].x1 = bar_width/2;	seg[i].y1 = 0;
  seg[i].x2 = bar_width - 1;	seg[i++].y2 = bar_width - 1;

  seg[i].x1 = 1;		seg[i].y1 = bar_width - 2;
  seg[i].x2 = bar_width - 2;	seg[i++].y2 = bar_width - 2;

  seg[i].x1 = 0;		seg[i].y1 = bar_width - 1;
  seg[i].x2 = bar_width - 1;	seg[i++].y2 = bar_width - 1;
  XDrawSegments(dpy, t_button, sgc, seg, i);
}

void RedrawBottomButton(GC rgc, GC sgc)
{
  XSegment seg[4];
  int i=0;

  seg[i].x1 = 1;		seg[i].y1 = 1;
  seg[i].x2 = bar_width/2;	seg[i++].y2 = bar_width - 2;

  seg[i].x1 = 0;		seg[i].y1 = 0;
  seg[i].x2 = bar_width/2;	seg[i++].y2 = bar_width - 1;

  seg[i].x1 = 1;		seg[i].y1 = 1;
  seg[i].x2 = bar_width - 2;	seg[i++].y2 = 1;

  seg[i].x1 = 0;		seg[i].y1 = 0;
  seg[i].x2 = bar_width - 1;	seg[i++].y2 = 0;
  XDrawSegments(dpy, b_button, rgc, seg, i);

  i = 0;
  seg[i].x1 = bar_width - 2;	seg[i].y1 = 1;
  seg[i].x2 = bar_width/2;	seg[i++].y2 = bar_width - 2;

  seg[i].x1 = bar_width - 1;	seg[i].y1 = 0;
  seg[i].x2 = bar_width/2;	seg[i++].y2 = bar_width - 1;
  XDrawSegments(dpy, b_button, sgc, seg, i);
}

/************************************************************************
 * CreateWindow --Sizes and creates the window
 * 	Based on CreateWindow() from GoodStuff:
 *		Copyright 1993, Robert Nation.
 ***********************************************************************/
void CreateWindow(void)
{
  XGCValues gcv;
  unsigned long gcm;
  unsigned long mask;
  char *list[2];
  XSetWindowAttributes attributes;
  XSizeHints mysizehints;
  XTextProperty name;
  XClassHint class_hints;

  h_margin = margin1*2 + bar_width + margin2 + 8;
  v_margin = margin1*2 + bar_width + margin2 + 8;

  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  _XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);

  /* load the font */
  Ffont = FlocaleLoadFont(dpy, font_string, MyName);
  if (Ffont == NULL)
  {
    fprintf(stderr,"%s: No fonts available, exiting\n",MyName);
    exit(1);
  }

  if ((local_flags & HIDE_H))
    v_margin -= bar_width + margin2 + 4;
  if ((local_flags & HIDE_V))
    h_margin -= bar_width + margin2 + 4;

  UWidth = max_icon_width + icon_relief + interval;
  UHeight = Ffont->height + max_icon_height + icon_relief + 6 + interval;
  Width = UWidth * num_columns + interval -1;
  Height = UHeight * num_rows + interval -1;

  mysizehints.flags = PWinGravity| PResizeInc | PMinSize;

  /* subtract one for the right/bottom border */
  mysizehints.min_width = UWidth + interval - 1 + h_margin;
  main_width = mysizehints.width = Width + h_margin;
  mysizehints.min_height = UHeight + interval - 1 + v_margin;
  main_height = mysizehints.height = Height + v_margin;
  mysizehints.width_inc = UWidth;
  mysizehints.height_inc = UHeight;

  if (geom_x > -100000)
  {
    if (xneg)
    {
      mysizehints.x = DisplayWidth(dpy,screen) + geom_x - mysizehints.width;
      gravity = NorthEastGravity;
    }
    else
    {
      mysizehints.x = geom_x;
    }
    if (yneg)
    {
      mysizehints.y = DisplayHeight(dpy,screen) + geom_y - mysizehints.height;
      gravity = SouthWestGravity;
    }
    else
    {
      mysizehints.y = geom_y;
    }

    if (xneg && yneg)
    {
      gravity = SouthEastGravity;
    }

    mysizehints.flags |= USPosition;
  }

  mysizehints.win_gravity = gravity;

  if(Pdepth < 2)
  {
    back_pix = icon_back_pix = act_icon_fore_pix = GetColor("white");
    fore_pix = icon_fore_pix = act_icon_back_pix = GetColor("black");
    hilite_pix = icon_hilite_pix = act_icon_shadow_pix = icon_back_pix;
    shadow_pix = icon_shadow_pix = act_icon_hilite_pix = icon_fore_pix;
  }
  else
  {
    fore_pix = (colorset < 0) ? GetColor(Fore) : Colorset[colorset].fg;
    back_pix = (colorset < 0) ? GetColor(Back) : Colorset[colorset].bg;
    icon_back_pix = (Iconcolorset < 0) ? GetColor(IconBack)
      : Colorset[Iconcolorset].bg;
    icon_fore_pix = (Iconcolorset < 0) ? GetColor(IconFore)
      : Colorset[Iconcolorset].fg;
    icon_hilite_pix = (Iconcolorset < 0) ? GetHilite(icon_back_pix)
      : Colorset[Iconcolorset].hilite;
    icon_shadow_pix = (Iconcolorset < 0) ? GetShadow(icon_back_pix)
      : Colorset[Iconcolorset].shadow;
    act_icon_back_pix = (IconHicolorset < 0) ? GetColor(ActIconBack)
      : Colorset[IconHicolorset].bg;
    act_icon_fore_pix = (IconHicolorset < 0) ? GetColor(ActIconFore)
      : Colorset[IconHicolorset].fg;
    act_icon_hilite_pix = (IconHicolorset < 0) ? GetHilite(act_icon_back_pix)
      : Colorset[IconHicolorset].hilite;
    act_icon_shadow_pix = (IconHicolorset < 0) ? GetShadow(act_icon_back_pix)
      : Colorset[IconHicolorset].shadow;
    hilite_pix = (colorset < 0) ? GetHilite(back_pix)
      : Colorset[colorset].hilite;
    shadow_pix = (colorset < 0) ? GetShadow(back_pix)
      : Colorset[colorset].shadow;
  }

  attributes.background_pixel = back_pix;
  attributes.border_pixel = 0;
  attributes.colormap = Pcmap;
  main_win = XCreateWindow(dpy, Root, mysizehints.x, mysizehints.y,
			   mysizehints.width, mysizehints.height, 0, Pdepth,
			   InputOutput, Pvisual,
			   CWBackPixel | CWBorderPixel | CWColormap,
			   &attributes);
  XSetWMProtocols(dpy,main_win,&wm_del_win,1);
  XSelectInput(dpy,main_win,MW_EVENTS);

  /* set normal_hits, wm_hints, and, class_hints */
  list[0]=MyName;
  list[1]=NULL;
  if (XStringListToTextProperty(list,1,&name) == 0)
  {
    fprintf(stderr,"%s: cannot allocate window name",MyName);
    return;
  }
  class_hints.res_name = MyName;
  class_hints.res_class = "FvwmIconBox";
  XSetWMProperties(dpy,main_win,&name,&name,
		   NULL,0,&mysizehints,NULL,&class_hints);
  XFree(name.value);

  mysizehints.width -= h_margin;
  mysizehints.height -= v_margin;
  holder_win = XCreateSimpleWindow(dpy,main_win,margin1+2,
				   margin1+2,
				   mysizehints.width,
				   mysizehints.height,
                                   0,fore_pix,back_pix);

  icon_win = XCreateSimpleWindow(dpy,holder_win,-icon_win_x,-icon_win_y,
				 icon_win_width,
				 icon_win_height,
				 0,fore_pix,back_pix);
  /* needed for transparency */
  XSelectInput(dpy,icon_win,StructureNotifyMask);

  gcm = GCForeground|GCBackground;
  gcv.foreground = hilite_pix;
  gcv.background = back_pix;
  ReliefGC = fvwmlib_XCreateGC(dpy, main_win, gcm, &gcv);

  gcv.foreground = shadow_pix;
  ShadowGC = fvwmlib_XCreateGC(dpy, main_win, gcm, &gcv);

  gcv.foreground = icon_hilite_pix;
  gcv.background = icon_back_pix;
  IconReliefGC = fvwmlib_XCreateGC(dpy, main_win, gcm, &gcv);

  gcv.foreground = icon_shadow_pix;
  IconShadowGC = fvwmlib_XCreateGC(dpy, main_win, gcm, &gcv);

  gcm = GCForeground|GCBackground;
  if (Ffont->font != NULL)
  {
    gcm |= GCFont;
    gcv.font = Ffont->font->fid;
  }
  gcv.foreground = icon_fore_pix;
  NormalGC = fvwmlib_XCreateGC(dpy, main_win, gcm, &gcv);

  /* init our Flocale window string for text drawing */
  FlocaleAllocateWinString(&FwinString);

  /* icon_win's background */
  if (colorset >= 0) {
    SetWindowBackground(dpy, icon_win, icon_win_width, icon_win_height,
                        &Colorset[(colorset)], Pdepth, NormalGC,
			True);
    SetWindowBackground(dpy, holder_win, mysizehints.width, mysizehints.height,
                        &Colorset[(colorset)], Pdepth, NormalGC,
			True);
    SetWindowBackground(dpy, main_win, mysizehints.width, mysizehints.height,
                        &Colorset[(colorset)], Pdepth, NormalGC,
			True);
  } else if (GetBackPixmap() == True){
    XSetWindowBackgroundPixmap(dpy, icon_win, IconwinPixmap);
    /*  special thanks to Dave Goldberg <dsg@mitre.org>
	for his helpful information */
    XFreePixmap(dpy, IconwinPixmap);
  }

  /* scroll bars */
  mask = CWWinGravity | CWBackPixel;
  attributes.background_pixel = back_pix;
  if (!(local_flags & HIDE_H)){
    attributes.win_gravity = SouthWestGravity;
    h_scroll_bar = XCreateWindow(dpy, main_win, margin1 + 2 +
				 bar_width,
				 margin1 + 6 + Height + margin2,
				 Width - bar_width*2, bar_width,
				 0, CopyFromParent,
				 InputOutput, CopyFromParent,
				 mask, &attributes);
    XSelectInput(dpy,h_scroll_bar,SCROLL_EVENTS);
  }
  if (!(local_flags & HIDE_V))
  {
    attributes.win_gravity = NorthEastGravity;
    v_scroll_bar = XCreateWindow(dpy ,main_win, margin1 + 6 +
				 Width + margin2,
				 margin1 + 2 + bar_width,
				 bar_width, Height - bar_width*2,
				 0, CopyFromParent,
				 InputOutput, CopyFromParent,
				 mask, &attributes);
    XSelectInput(dpy,v_scroll_bar,SCROLL_EVENTS);
  }

  /* buttons */
  if (!(local_flags & HIDE_H))
  {
    attributes.win_gravity = SouthWestGravity;
    l_button = XCreateWindow(dpy, main_win, margin1 + 2,
			     margin1 + 6 + Height + margin2,
			     bar_width, bar_width,
			     0, CopyFromParent,
			     InputOutput, CopyFromParent,
			     mask, &attributes);
    attributes.win_gravity = SouthEastGravity;
    r_button = XCreateWindow(dpy, main_win, margin1 + 2 + Width -
			     bar_width,
			     margin1 + 6 + Height + margin2,
			     bar_width, bar_width,
			     0, CopyFromParent,
			     InputOutput, CopyFromParent,
			     mask, &attributes);
    XSelectInput(dpy,l_button,BUTTON_EVENTS);
    XSelectInput(dpy,r_button,BUTTON_EVENTS);
  }
  if (!(local_flags & HIDE_V)){
    attributes.win_gravity = NorthEastGravity;
    t_button = XCreateWindow(dpy, main_win, margin1 + 6 +
			     Width + margin2, margin1 + 2,
			     bar_width, bar_width,
			     0, CopyFromParent,
			     InputOutput, CopyFromParent,
			     mask, &attributes);
    attributes.win_gravity = SouthEastGravity;
    b_button = XCreateWindow(dpy, main_win, margin1 + 6 +
			     Width + margin2,
			     margin1 + 2 + Height - bar_width,
			     bar_width, bar_width,
			     0, CopyFromParent,

			     InputOutput, CopyFromParent,
			     mask, &attributes);
    XSelectInput(dpy,t_button,BUTTON_EVENTS);
    XSelectInput(dpy,b_button,BUTTON_EVENTS);
  }
}

static void change_colorset(int color)
{
  int x, y;
  unsigned int bw, w, h, depth;
  Window Junkroot;

  if (color == colorset)
  {
    fore_pix = (colorset < 0) ? GetColor(Fore)
      : Colorset[colorset].fg;
    back_pix = (colorset < 0) ? GetColor(Back)
      : Colorset[colorset].bg;
    hilite_pix = (colorset < 0) ? GetHilite(back_pix)
      : Colorset[colorset].hilite;
    shadow_pix = (colorset < 0) ? GetShadow(back_pix)
      : Colorset[colorset].shadow;
    if (XGetGeometry(dpy, main_win, &Junkroot, &x, &y, &w, &h, &bw, &depth))
    {
      SetWindowBackground(
	dpy, main_win, w, h, &Colorset[(colorset)], Pdepth, NormalGC, True);
    }
    SetWindowBackground(
      dpy, icon_win, icon_win_width, icon_win_height, &Colorset[(colorset)],
      Pdepth, NormalGC, True);
    SetWindowBackground(dpy, holder_win, main_width - h_margin,
      main_height - v_margin, &Colorset[(colorset)], Pdepth, NormalGC, True);
    XSetWindowBackground(dpy, h_scroll_bar, back_pix);
    XSetWindowBackground(dpy, v_scroll_bar, back_pix);
    XSetWindowBackground(dpy, l_button, back_pix);
    XSetWindowBackground(dpy, r_button, back_pix);
    XSetWindowBackground(dpy, t_button, back_pix);
    XSetWindowBackground(dpy, b_button, back_pix);
    XSetForeground(dpy, ReliefGC, hilite_pix);
    XSetBackground(dpy, ReliefGC, back_pix);
    XSetForeground(dpy, ShadowGC, shadow_pix);
    XSetBackground(dpy, ShadowGC, back_pix);
    XClearWindow(dpy, main_win);
    XClearWindow(dpy, holder_win);
    XClearWindow(dpy, icon_win);
    XClearWindow(dpy, h_scroll_bar);
    XClearWindow(dpy, v_scroll_bar);
    XClearWindow(dpy, l_button);
    XClearWindow(dpy, r_button);
    XClearWindow(dpy, t_button);
    XClearWindow(dpy, b_button);
  }
  if (color == Iconcolorset) {
    struct icon_info *tmp;
    icon_back_pix = (Iconcolorset < 0) ? GetColor(IconBack)
      : Colorset[Iconcolorset].bg;
    icon_fore_pix = (Iconcolorset < 0) ? GetColor(IconFore)
      : Colorset[Iconcolorset].fg;
    icon_hilite_pix = (Iconcolorset < 0) ? GetHilite(icon_back_pix)
      : Colorset[Iconcolorset].hilite;
    icon_shadow_pix = (Iconcolorset < 0) ? GetShadow(icon_back_pix)
      : Colorset[Iconcolorset].shadow;
    XSetForeground(dpy, IconReliefGC, icon_hilite_pix);
    XSetBackground(dpy, IconReliefGC, icon_back_pix);
    XSetForeground(dpy, IconShadowGC, icon_shadow_pix);
    XSetBackground(dpy, IconShadowGC, icon_back_pix);
    XSetForeground(dpy, NormalGC, icon_fore_pix);
    XSetBackground(dpy, NormalGC, icon_back_pix);

    tmp = Head;
    while(tmp != NULL)
    {
      if (max_icon_height != 0 && (IS_ICON_OURS(tmp)))
        XSetWindowBackground(dpy, tmp->icon_pixmap_w, icon_back_pix);
      XSetWindowBackground(dpy, tmp->IconWin, icon_back_pix);
      XClearArea(dpy, tmp->icon_pixmap_w, 0, 0, 0, 0, True);
      XClearArea(dpy, tmp->IconWin, 0, 0, 0, 0, True);
      tmp = tmp->next;
    }
  }
  if (color == IconHicolorset)
  {
    act_icon_back_pix = (IconHicolorset < 0) ? GetColor(ActIconBack)
      : Colorset[IconHicolorset].bg;
    act_icon_fore_pix = (IconHicolorset < 0) ? GetColor(ActIconFore)
      : Colorset[IconHicolorset].fg;
    act_icon_hilite_pix = (IconHicolorset < 0) ? GetHilite(act_icon_back_pix)
      : Colorset[IconHicolorset].hilite;
    act_icon_shadow_pix = (IconHicolorset < 0) ? GetShadow(act_icon_back_pix)
      : Colorset[IconHicolorset].shadow;
  }
}

void GetIconwinSize(int *dx, int *dy)
{
  *dx = icon_win_width;
  *dy = icon_win_height;

  if (primary == LEFT || primary == RIGHT){
    icon_win_width = max(Width, UWidth * Lines + interval - 1);
    icon_win_height = max(Height, UHeight * (max(0,
						 (num_icons-1))/Lines
					     + 1) - 1 + interval);
  }else{
    icon_win_width = max(Width, UWidth * (max(0,num_icons-1) / Lines +
					  1) + interval - 1);
    icon_win_height = max(Height, UHeight * Lines - 1 + interval);
  }
  *dx = icon_win_width - *dx;
  *dy = icon_win_height - *dy;
}

void nocolor(char *a, char *b)
{
 fprintf(stderr,"%s: can't %s %s\n", MyName, a,b);
}

void MySendFvwmPipe(int *fd, char *message, unsigned long window)
{
  long w;
  char *hold, *temp, *temp_msg;
  hold = message;

  while (1)
  {
    temp = strchr(hold, ',');
    if (temp != NULL)
    {
      temp_msg = safemalloc(temp - hold + 1);
      strncpy(temp_msg, hold, temp - hold);
      temp_msg[temp - hold]='\0';
      hold = temp + 1;
    }
    else temp_msg = hold;

    if (!ExecIconBoxFunction(temp_msg))
    {
      write(fd[0], &window, sizeof(unsigned long));

      w=strlen(temp_msg);
      write(fd[0], &w, sizeof(w));
      write(fd[0], temp_msg, w);

      /* keep going */

      w = 1;
      write(fd[0], &w, sizeof(w));
    }
    if (temp_msg != hold) free(temp_msg);
    else break;
  }
}

Bool ExecIconBoxFunction(char *msg)
{
  if (strncasecmp(msg, "Next", 4) == 0){
    Next();
    return True;
  }else if (strncasecmp(msg, "Prev", 4) == 0){
    Prev();
    return True;
  }else if (strncasecmp(msg, "Left", 4) == 0){
    HScroll(icon_win_x - UWidth);
    return True;
  }else if (strncasecmp(msg, "Right", 5) == 0){
    HScroll(icon_win_x + UWidth);
    return True;
  }else if (strncasecmp(msg, "Up", 2) == 0){
    VScroll(icon_win_y - UHeight);
    return True;
  }else if (strncasecmp(msg, "Down", 4) == 0){
    VScroll(icon_win_y + UHeight);
    return True;
  }
  return False;
}

void Next(void)
{
  struct icon_info *new, *old;
  int i;

  old = new = Hilite;

  if (new != NULL)
    new = new->next;

  if (new != NULL)
    Hilite = new;
  else
    new = Hilite = Head;

  if (new == NULL)
    return;

  if (old != NULL)
    RedrawIcon(old, redraw_flag);
  if (new != NULL)
    RedrawIcon(new, redraw_flag);

  i=0;
  if (new->x < icon_win_x){
    while (new->x + UWidth * ++i < icon_win_x)
      ;
    HScroll(icon_win_x - UWidth*i);
  }else if (new->x > icon_win_x + Width){
    while (new->x - UWidth * ++i > icon_win_x + Width)
      ;
    HScroll(icon_win_x + UWidth*i);
  }

  i=0;
  if (new->y < icon_win_y){
    while (new->y + UHeight * ++i < icon_win_y)
      ;
    VScroll(icon_win_y - UHeight*i);
  }else if (new->y > icon_win_y + Height){
    while (new->y - UHeight * ++i > icon_win_y + Height)
      ;
    VScroll(icon_win_y + UHeight*i);
  }
}

void Prev(void)
{
  struct icon_info *new, *old;
  int i;

  old = new = Hilite;

  if (new != NULL)
    new = new->prev;

  if (new != NULL)
    Hilite = new;
  else
    new = Hilite = Tail;

  if (new == NULL)
    return;

  if (old != NULL)
    RedrawIcon(old, redraw_flag);
  if (new != NULL)
    RedrawIcon(new, redraw_flag);

  i=0;
  if (new->x < icon_win_x){
    while (new->x + UWidth * ++i < icon_win_x)
      ;
    HScroll(icon_win_x - UWidth*i);
  }else if (new->x > icon_win_x + Width){
    while (new->x - UWidth * ++i > icon_win_x + Width)
      ;
    HScroll(icon_win_x + UWidth*i);
  }

  i=0;
  if (new->y < icon_win_y){
    while (new->y + UHeight * ++i < icon_win_y)
      ;
    VScroll(icon_win_y - UHeight*i);
  }else if (new->y > icon_win_y + Height){
    while (new->y - UHeight * ++i > icon_win_y + Height)
      ;
    VScroll(icon_win_y + UHeight*i);
  }
}

/************************************************************************
 * TerminateHandler - signal handler to make FvwmIconBox exit cleanly
 */
static RETSIGTYPE
TerminateHandler(int sig)
{
  /*
   * This function might not return - it could "long-jump"
   * right out, so we need to do everything we need to do
   * BEFORE we call it ...
   */
  fvwmSetTerminate(sig);
}


/************************************************************************
 * DeadPipe --Dead pipe handler
 * 	Based on DeadPipe() from GoodStuff:
 *		Copyright 1993, Robert Nation.
 ***********************************************************************/
void
DeadPipe(int nonsense)
{
  is_dead_pipe = True;
  exit(0);
}

static void
CleanUp(void)
{
	struct icon_info *tmpi;
#if 0
	struct icon_info *tmpi, *tmpi2;
	struct mousefunc *tmpm, *tmpm2;
	struct keyfunc *tmpk, *tmpk2;
	struct iconfile *tmpf, *tmpf2;
#endif

	if (!is_dead_pipe)
	{
		if ((local_flags & SETWMICONSIZE))
		{
			XDeleteProperty(dpy, Root, XA_WM_ICON_SIZE);
			XFlush(dpy);
		}
	}
	/* DV: my, what is all this stuff good for? All memory gets freed
	 * automatically! 
	 * OC: Yes but the icon window must be reparented to the root window */
	tmpi = Head;
	while(tmpi != NULL)
	{
		if (!IS_ICON_OURS(tmpi) && tmpi->icon_pixmap_w &&
		    tmpi->icon_w != 0)
		{
			XSelectInput(dpy, tmpi->icon_pixmap_w, NoEventMask);
			XReparentWindow(dpy, tmpi->icon_pixmap_w, Root, 0, 0);
			XUnmapWindow(dpy, tmpi->icon_pixmap_w);
		}
		tmpi = tmpi->next;
	}
#if 0
  if (FvwmDefaultIcon != NULL)
    free(FvwmDefaultIcon);

  tmpm = MouseActions;
  while(tmpm != NULL){
    tmpm2 = tmpm;
    tmpm = tmpm->next;
    free(tmpm2->action);
    free(tmpm2);
  }

  tmpk = KeyActions;
  while(tmpk != NULL){
    tmpk2 = tmpk;
    tmpk = tmpk->next;
    free(tmpk2->action);
    free(tmpk2->name);
    free(tmpk2);
  }

  tmpf = IconListHead;
  while(tmpf != NULL){
    tmpf2 = tmpf;
    tmpf = tmpf->next;
    free(tmpf2->name);
    free(tmpf2->iconfile);
    free(tmpf2);
  }

  tmpi = Head;
  while(tmpi != NULL){
    tmpi2 = tmpi;
    tmpi = tmpi->next;
    freeitem(tmpi2, 0);
  }
#endif

  return;
}


/************************************************************************
 * ParseOptions
 * 	Based on ParseConfig() from FvwmWinList:
 *		Copyright 1994, Mike Finger.
 ***********************************************************************/
void ParseOptions(void)
{
  char *tline= NULL,*tmp;
  int Clength;

  Clength = strlen(MyName);

  InitGetConfigLine(fd,CatString3("*",MyName,0));
  GetConfigLine(fd,&tline);

  while(tline != NULL)
  {
    int g_x, g_y, flags;
    unsigned width,height;

    if(strlen(&tline[0])>1)
    {
      if (strncasecmp(tline,CatString3("*", MyName, "Geometry"),Clength+9)==0)
      {
	num_columns = 6;
	num_rows = 1;
	geom_x = -10000;
	geom_y = -10000;
	xneg = 0;
	yneg = 0;
	tmp = &tline[Clength+9];
	while(((isspace((unsigned char)*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	  tmp++;
	tmp[strlen(tmp)] = 0;
	flags = FScreenParseGeometry(tmp,&g_x,&g_y,&width,&height);
	if (flags & WidthValue)
	  num_columns = width;
	if (flags & HeightValue)
	  num_rows = height;
	if (flags & XValue)
	  geom_x = g_x;
	if (flags & YValue)
	  geom_y = g_y;
	if (flags & XNegative)
	  xneg = 1;
	if (flags & YNegative)
	  yneg = 1;
      }
      else if (strncasecmp(tline,CatString3(
			     "*", MyName, "MaxIconSize"),Clength+12)==0)
      {
	tmp = &tline[Clength+12];
	while(((isspace((unsigned char)*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	  tmp++;
	tmp[strlen(tmp)] = 0;

	flags = XParseGeometry(tmp,&g_x,&g_y,&width,&height);
	if (flags & WidthValue)
	  max_icon_width = width;
	if (flags & HeightValue)
	  max_icon_height = height;
	if (height == 0){
	  icon_relief = 0;
	  redraw_flag = 2;
	  max_icon_width += 4;
	}
      }
      else if (strncasecmp(tline, "Colorset", 8) == 0)
      {
	LoadColorset(&tline[8]);
      }
      else if (strncasecmp(tline, XINERAMA_CONFIG_STRING,
			   sizeof(XINERAMA_CONFIG_STRING) - 1) == 0)
      {
	FScreenConfigureModule(
	  tline + sizeof(XINERAMA_CONFIG_STRING) - 1);
      }
      else if (strncasecmp(
		 tline,CatString3("*",MyName,"Colorset"),Clength+9)==0)
      {
	AllocColorset(colorset = atoi(&tline[Clength+9]));
      }
      else if (strncasecmp(
		 tline,CatString3("*",MyName,"IconColorset"),Clength+13)==0)
      {
	AllocColorset(Iconcolorset = atoi(&tline[Clength+13]));
      }
      else if (strncasecmp(
		 tline,CatString3("*",MyName,"IconHiColorset"),Clength+15)==0)
      {
	AllocColorset(IconHicolorset = atoi(&tline[Clength+15]));
      }
      else if (strncasecmp(tline,CatString3("*",MyName,"Font"),Clength+5)==0)
      {
	CopyString(&font_string,&tline[Clength+5]);
      }
      else if (strncasecmp(
		 tline,CatString3("*",MyName,"IconFore"),Clength+9)==0)
      {
	CopyString(&IconFore,&tline[Clength+9]);
      }
      else if (strncasecmp(
		 tline,CatString3("*",MyName,"IconBack"),Clength+9)==0)
      {
	CopyString(&IconBack,&tline[Clength+9]);
      }
      else if (strncasecmp(
		 tline,CatString3("*",MyName,"IconHiFore"),Clength+11)==0)
      {
	CopyString(&ActIconFore,&tline[Clength+11]);
      }
      else if (strncasecmp(
		 tline,CatString3("*",MyName,"IconHiBack"),Clength+11)==0)
      {
	CopyString(&ActIconBack,&tline[Clength+11]);
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "Fore"),Clength+5)==0)
      {
	CopyString(&Fore,&tline[Clength+5]);
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "Back"),Clength+5)==0)
      {
	CopyString(&Back,&tline[Clength+5]);
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "Pixmap"),Clength+7)==0)
      {
	CopyString(&IconwinPixmapFile,&tline[Clength+7]);
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "Padding"),Clength+8)==0)
      {
	interval = max(0,atoi(&tline[Clength+8]));
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "FrameWidth"),Clength+11)==0)
      {
	sscanf(&tline[Clength+11], "%d %d", &margin1, &margin2);
	margin1 = max(0, margin1);
	margin2 = max(0, margin2);
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "Lines"),Clength+6)==0)
      {
	Lines = max(1,atoi(&tline[Clength+6]));
      }
      else if (strncasecmp(tline,CatString3("*", MyName,
					    "NoIconAction"),Clength+13) == 0)
      {
	tmp = &tline[Clength+14];
	while(((isspace((unsigned char)*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	  tmp++;
	if (tmp[strlen(tmp)-1] == '\n')
	  tmp[strlen(tmp) -1] = '\0';
	if (AnimCommand)
	  free(AnimCommand);
	AnimCommand = (char *)safemalloc((strlen(tmp) + 1) * sizeof(char));
	strcpy(AnimCommand, tmp);
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "SBWidth"),Clength+8)==0)
      {
	bar_width = max(5,atoi(&tline[Clength+8]));
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "Placement"),Clength+10)==0)
      {
	parseplacement(&tline[Clength+10]);
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "SetWMIconSize"),Clength+14)==0)
      {
	local_flags |= SETWMICONSIZE;
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "HilightFocusWin"),Clength+16)==0)
      {
	m_mask |= M_FOCUS_CHANGE;
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "UseSkipList"),Clength+10)==0)
      {
	UseSkipList = True;
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "Resolution"),Clength+11)==0)
      {
	tmp = &tline[Clength+11];
	while(((isspace((unsigned char)*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	  tmp++;
	if (strncasecmp(tmp, "Desk", 4) == 0)
	{
	  m_mask |= M_NEW_DESK;
	  local_flags |= CURRENT_ONLY;
	}
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "Mouse"),Clength+6)==0)
      {
	parsemouse(&tline[Clength + 6]);
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "Key"),Clength+4)==0)
      {
	parsekey(&tline[Clength + 4]);
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "SortIcons"),Clength+10)==0)
      {
	tmp = &tline[Clength+10];
	while(((isspace((unsigned char)*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	  tmp++;
	if (strlen(tmp) == 0)
	{
	  /* the case where no argument is given */
	  sortby = ICONNAME;
	  return;
	}
	if (strncasecmp(tmp, "WindowName", 10) == 0)
	  sortby = WINDOWNAME;
	else if (strncasecmp(tmp, "IconName", 8) == 0)
	  sortby = ICONNAME;
	else if (strncasecmp(tmp, "ResClass", 8) == 0)
	  sortby = RESCLASS;
	else if (strncasecmp(tmp, "ResName", 7) == 0)
	  sortby = RESNAME;
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    "HideSC"),Clength+7)==0)
      {
	tmp = &tline[Clength+7];
	while(((isspace((unsigned char)*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	  tmp++;
	if (strncasecmp(tmp, "Horizontal", 10) == 0)
	  local_flags |= HIDE_H;
	else if (strncasecmp(tmp, "Vertical", 8) == 0)
	  local_flags |= HIDE_V;
      }
      else if (strncasecmp(tline,CatString3("*",MyName,
					    ""),Clength+1)==0)
      {
	parseicon(&tline[Clength + 1]);
      }
      else if (strncasecmp(tline,"ImagePath",9)==0)
      {
	CopyString(&imagePath,&tline[9]);
      }
      else if (strncasecmp(tline,"ClickTime",9)==0)
      {
	ClickTime = atoi(&tline[9]);
      }
      else if (strncasecmp(tline,"ColorLimit",10)==0)
      {
	save_color_limit = atoi(&tline[10]);
      }
    }
    GetConfigLine(fd,&tline);
  }

  return;
}

void parseicon(char *tline)
{
  int len;
  struct iconfile *tmp;
  char *ptr, *start, *end;

  tmp = (struct iconfile *)safemalloc(sizeof(struct iconfile));
  memset(tmp, 0, sizeof(struct iconfile));

   /* windowname */
   tmp->name = stripcpy2(tline);
   if(tmp->name == NULL){
     free(tmp);
     return;
   }

   /* skip windowname, based on strpcpy3 of configure.c */
   while((*tline != '"')&&(tline != NULL))
     tline++;
    if(*tline != 0)
      tline++;
   while((*tline != '"')&&(tline != NULL))
     tline++;
   if(*tline == 0){
     free(tmp);
     return;
   }
   tline++;

   /* file */
   /* skip spaces */
   while(isspace((unsigned char)*tline)&&(*tline != '\n')&&(*tline != 0))
     tline++;
   start = tline;
   end = tline;
   while(!isspace((unsigned char)*end)&&(*end != '\n')&&(*end != 0))
     end++;
   len = end - start;
   ptr = safemalloc(len+1);
   strncpy(ptr, start, len);
   ptr[len] = 0;
   tmp->iconfile = ptr;

  if (strcmp(tmp->name, "*") == 0)
    DefaultIcon = tmp;

  tmp->next = NULL;

  if (IconListHead == NULL)
    IconListHead = IconListTail = tmp;
  else{
    IconListTail->next = tmp;
    IconListTail = tmp;
  }
}

void parseplacement(char *tline)
{
  char p[240], s[240];

  sscanf(tline, "%s %s", p, s);

  if (strncasecmp(p, "Left", 4) == 0)
    primary = LEFT;
  else if (strncasecmp(p, "Right", 5) == 0)
    primary = RIGHT;
  else if (strncasecmp(p, "Top", 3) == 0)
    primary = TOP;
  else if (strncasecmp(p, "Bottom", 6) == 0)
    primary = BOTTOM;

  if (strncasecmp(s, "Left", 4) == 0)
    secondary = LEFT;
  else if (strncasecmp(s, "Right", 5) == 0)
    secondary = RIGHT;
  else if (strncasecmp(s, "Top", 3) == 0)
    secondary = TOP;
  else if (strncasecmp(s, "Bottom", 6) == 0)
    secondary = BOTTOM;
}

void parsemouse(char *tline)
{
  struct mousefunc *f = NULL;
  int len;
  int n;
  int b;
  char *ptr,*start,*end,*tmp;

  f = (struct mousefunc *)safemalloc(sizeof(struct mousefunc));
  memset(f, 0, sizeof(struct mousefunc));

  /* skip spaces */
  while(isspace((unsigned char)*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;
  start = tline;
  end = tline;
  while((!isspace((unsigned char)*end))&&(*end!='\n')&&(*end!=0))
    end++;
  n = sscanf(start, "%d", &b);
  if (n > 0 && b >= 0 && b <= NUMBER_OF_MOUSE_BUTTONS)
    f->mouse = b;
  else
    f->mouse = -1;
  /* click or doubleclick */
  tline = end;
  /* skip spaces */
  while(isspace((unsigned char)*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;
  start = tline;
  end = tline;
  while((!isspace((unsigned char)*end))&&(*end!='\n')&&(*end!=0))
    end++;
  if (strncasecmp(start, "Click", 5) == 0)
    f->type = CLICK;
  else if (strncasecmp(start, "DoubleClick", 11) == 0)
  {
    f->type = DOUBLE_CLICK;
    have_double_click = True;
  }

  /* actions */
  tline = end;
  /* skip spaces */
  while(isspace((unsigned char)*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;
  start = tline;
  end = tline;
  tmp = tline;
  while((*tmp!='\n')&&(*tmp!=0)){
    if (!isspace((unsigned char)*tmp))
      end = tmp;
    tmp++;
  }
  end++;
  len = end - start;
  ptr = safemalloc(len+1);
  strncpy(ptr, start, len);
  ptr[len] = 0;
  f->action = ptr;
  f->next = MouseActions;
  MouseActions = f;
}

/***********************************************************************
  parsekey
 	Based on part of AddFunckey() of configure.c in Fvwm.
       	Copyright 1988, Evans and Sutherland Computer Corporation,
       	Copyright 1989, Massachusetts Institute of Technology,
       	Copyright 1993, Robert Nation.
 ***********************************************************************/
void parsekey(char *tline)
{
  struct keyfunc *k;
  int nlen, alen;
  char *nptr, *aptr, *start, *end, *tmp;
  int i, kmin, kmax;
  KeySym keysym;

  /* skip spaces */
  while(isspace((unsigned char)*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;
  start = tline;
  end = tline;
  while((!isspace((unsigned char)*end))&&(*end!='\n')&&(*end!=0))
    end++;
  nlen = end - start;
  nptr = safemalloc(nlen+1);
  strncpy(nptr, start, nlen);
  nptr[nlen] = 0;

  /* actions */
  tline = end;
  /* skip spaces */
  while(isspace((unsigned char)*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;
  start = tline;
  end = tline;
  tmp = tline;
  while((*tmp!='\n')&&(*tmp!=0)){
    if (!isspace((unsigned char)*tmp))
      end = tmp;
    tmp++;
  }
  end++;
  alen = end - start;
  aptr = safemalloc(alen+1);
  strncpy(aptr, start, alen);
  aptr[alen] = 0;

  if ((keysym = XStringToKeysym(nptr)) == NoSymbol ||
      XKeysymToKeycode(dpy, keysym) == 0){
    free(nptr);
    free(aptr);
    return;
  }

  XDisplayKeycodes(dpy, &kmin, &kmax);
  for (i=kmin; i<=kmax; i++)
    if (XKeycodeToKeysym(dpy, i, 0) == keysym)
      {
	k = (struct keyfunc *)safemalloc(sizeof(struct keyfunc));
	memset(k, 0, sizeof(struct keyfunc));
	k->name = nptr;
        k->keycode = i;
	k->action = aptr;
	k->next = KeyActions;
	KeyActions = k;
      }
}

/***********************************************************************
 * change_window_name
 * 	Original work from GoodStuff:
 *		Copyright 1993, Robert Nation.
 ***********************************************************************/
void change_window_name(char *str)
{
  XTextProperty name;

  if (XStringListToTextProperty(&str,1,&name) == 0)
    {
      fprintf(stderr,"%s: cannot allocate window name",MyName);
      return;
    }
  XSetWMName(dpy,main_win,&name);
  XSetWMIconName(dpy,main_win,&name);
  XFree(name.value);
}

/***********************************************************************
 * My_XNextEvent
 * 	Original work from GoodStuff:
 *		Copyright 1993, Robert Nation.
 ***********************************************************************/
int My_XNextEvent(Display *dpy, XEvent *event)
{
  fd_set in_fdset;
  static int miss_counter = 0;

  if(XPending(dpy))
    {
      XNextEvent(dpy,event);
      return 1;
    }

  FD_ZERO(&in_fdset);
  FD_SET(x_fd,&in_fdset);
  FD_SET(fd[1],&in_fdset);

  if (fvwmSelect(fd_width, &in_fdset, 0, 0, NULL) > 0)
  {

    if(FD_ISSET(x_fd, &in_fdset))
    {
      if(XPending(dpy))
	{
	  XNextEvent(dpy,event);
	  miss_counter = 0;
	  return 1;
	}
      else
	miss_counter++;
      if(miss_counter > 100)
	exit(0);
    }

    if(FD_ISSET(fd[1], &in_fdset))
    {
      FvwmPacket* packet = ReadFvwmPacket(fd[1]);
      if ( packet == NULL )
	  exit(0);
      else
	  process_message( packet->type, packet->body );
    }
  }
  return 0;
}

/**************************************************************************
 * process_message
 * 	Based on ProcessMassage() from FvwmWinList:
 *		Copyright 1994, Mike Finger.
 *************************************************************************/
int diffx, diffy;
void process_message(unsigned long type, unsigned long *body)
{
  struct icon_info *tmp = NULL, *old;
  char *str;
  long olddesk;
  struct ConfigWinPacket  *cfgpacket = (void *) body;

  switch(type)
  {
  case M_CONFIGURE_WINDOW:
    if (ready)
    {
      tmp = Head;
      while(tmp != NULL)
      {
	if (tmp->id == cfgpacket->w)
        {
	  int remove=0,add=0;

	  if (IS_ICON_SUPPRESSED(cfgpacket) != IS_ICON_SUPPRESSED(tmp))
	  {
	    if (IS_ICON_SUPPRESSED(cfgpacket))
	      add = 1;
	    else
	      remove = 1;
	    SET_ICON_SUPPRESSED(tmp,IS_ICON_SUPPRESSED(cfgpacket));
	  }
	  if (UseSkipList &&
	      DO_SKIP_WINDOW_LIST(cfgpacket) != DO_SKIP_WINDOW_LIST(tmp))
	  {
	    if (!DO_SKIP_WINDOW_LIST(cfgpacket))
	      add = 1;
	    else
	      remove = 1;
	    SET_DO_SKIP_WINDOW_LIST(tmp,DO_SKIP_WINDOW_LIST(cfgpacket));
	  }
	  if ((local_flags & CURRENT_ONLY) &&
	      tmp->desk != cfgpacket->desk && !IS_STICKY(tmp))
          {
	    olddesk = tmp->desk;
	    tmp->desk = cfgpacket->desk;
	    if (olddesk == CurrentDesk || tmp->desk == CurrentDesk)
            {
	      if (tmp->desk == CurrentDesk && sortby != UNSORT)
		SortItem(NULL);
	      if (tmp->desk == CurrentDesk)
		add = 1;
	      else
		remove = 1;
	    }
	  }else if ((IS_STICKY(cfgpacket)) && !(IS_STICKY(tmp))) /* stick */
            SET_STICKY(tmp, True);
	  else if (!(IS_STICKY(cfgpacket)) && (IS_STICKY(tmp))){ /* unstick */
            SET_STICKY(tmp, False);
	    tmp->desk = cfgpacket->desk;
	  }
	  if ((add && window_cond(tmp)) || remove) {
	      num_icons = AdjustIconWindows();
	      GetIconwinSize(&diffx, &diffy);
	      if (diffy && (primary == BOTTOM || secondary == BOTTOM))
		icon_win_y += diffy;
	      if (diffx && (primary == RIGHT || secondary == RIGHT))
		icon_win_x += diffx;
	      if (icon_win_y < 0)
		icon_win_y = 0;
	      if (icon_win_x < 0)
		icon_win_x = 0;
	      if (icon_win_x + Width > icon_win_width)
		icon_win_x = icon_win_width - Width;
	      if (icon_win_y + Height > icon_win_height)
		icon_win_y = icon_win_height - Height;
	      XMoveResizeWindow(dpy, icon_win, -icon_win_x, -icon_win_y,
				icon_win_width, icon_win_height);
	      if ((diffx || diffy) && (colorset >= 0))
		SetWindowBackground(dpy, icon_win, icon_win_width,
				    icon_win_height, &Colorset[(colorset)],
				    Pdepth, NormalGC, True);
	      if (remove){
		XUnmapWindow(dpy, tmp->IconWin);
		if (max_icon_height != 0)
		  XUnmapWindow(dpy, tmp->icon_pixmap_w);
	      }else{
		XMapWindow(dpy, tmp->IconWin);
		if (max_icon_height != 0)
		  XMapWindow(dpy, tmp->icon_pixmap_w);
	      }
	      if (!(local_flags & HIDE_H) && diffx)
		RedrawHScrollbar();
	      if (!(local_flags & HIDE_V) && diffy)
		RedrawVScrollbar();
	  }
	  return;
	}
	tmp = tmp->next;
      } /* while */
      break;
    }
  case M_ADD_WINDOW:
    if (AddItem(cfgpacket) == True && ready){
      GetIconwinSize(&diffx, &diffy);
      if (diffy && (primary == BOTTOM || secondary == BOTTOM))
	icon_win_y += diffy;
      if (diffx && (primary == RIGHT || secondary == RIGHT))
	icon_win_x += diffx;
      XMoveResizeWindow(dpy, icon_win, -icon_win_x, -icon_win_y,
			icon_win_width, icon_win_height);
      if ((diffx || diffy) && (colorset >= 0))
	SetWindowBackground(dpy, icon_win, icon_win_width,
			    icon_win_height, &Colorset[(colorset)],
			    Pdepth, NormalGC, True);
    }
    break;
  case M_DESTROY_WINDOW:
    if (DeleteItem(body[0]) && ready){
      GetIconwinSize(&diffx, &diffy);
      if (diffy && (primary == BOTTOM || secondary == BOTTOM))
	icon_win_y += diffy;
      if (diffx && (primary == RIGHT || secondary == RIGHT))
	icon_win_x += diffx;
      if (icon_win_y < 0)
	icon_win_y = 0;
      if (icon_win_x < 0)
	icon_win_x = 0;
      if (icon_win_x + Width > icon_win_width)
	icon_win_x = icon_win_width - Width;
      if (icon_win_y + Height > icon_win_height)
	icon_win_y = icon_win_height - Height;
      XMoveResizeWindow(dpy, icon_win, -icon_win_x, -icon_win_y,
			icon_win_width, icon_win_height);
      if ((diffx || diffy) && (colorset >= 0))
	SetWindowBackground(dpy, icon_win, icon_win_width,
			    icon_win_height, &Colorset[(colorset)],
			    Pdepth, NormalGC, True);
      AdjustIconWindows();
      if (!(local_flags & HIDE_H) && diffx)
	RedrawHScrollbar();
      if (!(local_flags & HIDE_V) && diffy)
	RedrawVScrollbar();
    }
    break;
  case M_ICON_FILE:
  case M_RES_CLASS:
    UpdateItem(type, body[0], (char *)&body[3]);
    break;
  case M_VISIBLE_NAME:
    tmp = UpdateItem(type, body[0], (char *)&body[3]);
    if (!ready || tmp == NULL)
      break;
    if (sortby == WINDOWNAME && tmp->IconWin != None
	&& window_cond(tmp) && SortItem(tmp) == True)
      AdjustIconWindows();
    break;
  case M_RES_NAME:
    if ((tmp = UpdateItem(type, body[0], (char *)&body[3])) == NULL)
      break;
    if (LookInList(tmp) && ready){
      if (sortby != UNSORT)
	SortItem(tmp);
      CreateIconWindow(tmp);
      AdjustIconWindows();
      if (window_cond(tmp)){
	if (max_icon_height != 0)
      XMapWindow(dpy, tmp->icon_pixmap_w);
      XMapWindow(dpy, tmp->IconWin);
	if (!(local_flags & HIDE_H))
	RedrawHScrollbar();
	if (!(local_flags & HIDE_V))
	RedrawVScrollbar();
    }
    }
    break;
  case MX_VISIBLE_ICON_NAME:
    tmp = UpdateItem(type, body[0], (char *)&body[3]);
    if (!ready || tmp == NULL)
    break;
    if (sortby != UNSORT && tmp->IconWin != None
	&& window_cond(tmp) && SortItem(tmp) == True)
      AdjustIconWindows();
    if (tmp->IconWin != None && window_cond(tmp))
      RedrawIcon(tmp, 2);
    break;
  case M_DEFAULTICON:
    str = (char *)safemalloc(strlen((char *)&body[3])+1);
    strcpy(str, (char *)&body[3]);
    if (FvwmDefaultIcon)
      free(FvwmDefaultIcon);
    FvwmDefaultIcon = str;
    break;
  case M_ICONIFY:
  case M_DEICONIFY:
    if (ready && (tmp = SetFlag(body[0], type)) != NULL)
      RedrawIcon(tmp, 2);
    if (tmp && window_cond(tmp)) animate(tmp,body);
    SendUnlockNotification(fd);
    break;
  case M_FOCUS_CHANGE:
    if (!ready)
      break;
    tmp = Head;
    while(tmp != NULL){
      if (tmp->id == body[0]) break;
      tmp = tmp->next;
    }
    old = Hilite;
    Hilite = tmp;
    if (old != NULL)
      RedrawIcon(old, redraw_flag);
    if (tmp != NULL)
      RedrawIcon(tmp, redraw_flag);
    break;
  case M_NEW_DESK:
    if (CurrentDesk != body[0])
    {
      CurrentDesk = body[0];
      if (body[0] != 10000 && ready)
      {
        /* 10000 is a "magic" number used in FvwmPager */
	if (sortby != UNSORT)
	SortItem(NULL);
	num_icons = AdjustIconWindows();
	GetIconwinSize(&diffx, &diffy);
	icon_win_x = icon_win_y = 0;
	if (primary == BOTTOM || secondary == BOTTOM)
	  icon_win_y = icon_win_height - Height;
	if (primary == RIGHT || secondary == RIGHT)
	  icon_win_x = icon_win_width - Width;
	XMoveResizeWindow(dpy, icon_win, -icon_win_x, -icon_win_y,
			  icon_win_width, icon_win_height);
	XUnmapSubwindows(dpy, icon_win);
	if ((diffx || diffy) && (colorset >= 0))
	  SetWindowBackground(dpy, icon_win, icon_win_width,
			      icon_win_height, &Colorset[(colorset)],
			      Pdepth, NormalGC, True);
	mapicons();
	if (!(local_flags & HIDE_H))
	  RedrawHScrollbar();
	if (!(local_flags & HIDE_V))
	  RedrawVScrollbar();
      }
    }
    break;
  case M_END_WINDOWLIST:
    GetIconwinSize(&diffx, &diffy);
    tmp = Head;
    while(tmp != NULL){
      CreateIconWindow(tmp);
      tmp = tmp->next;
    }
    if (sortby != UNSORT)
      SortItem(NULL);
    if (primary == BOTTOM || secondary == BOTTOM)
      icon_win_y = icon_win_height - Height;
    if (primary == RIGHT || secondary == RIGHT)
      icon_win_x = icon_win_width - Width;
    XMoveResizeWindow(dpy, icon_win, -icon_win_x, -icon_win_y,
		      icon_win_width, icon_win_height);
    if ((diffx || diffy) && (colorset >= 0))
      SetWindowBackground(dpy, icon_win, icon_win_width,
			  icon_win_height, &Colorset[(colorset)],
			  Pdepth, NormalGC, True);
    AdjustIconWindows();
    XMapWindow(dpy,main_win);
    XMapSubwindows(dpy, main_win);
    XMapWindow(dpy, icon_win);
    mapicons();
    ready = 1;
    break;
  case M_CONFIG_INFO:
    {
      char *tline, *token;
      int color;
      tline = (char*)&(body[3]);
      token = PeekToken(tline, &tline);
      if (StrEquals(token, "Colorset")) {
	color = LoadColorset(tline);
	change_colorset(color);
      }
      else if (StrEquals(token, XINERAMA_CONFIG_STRING))
      {
	  FScreenConfigureModule(tline);
      }
  }
  break;
  case MX_PROPERTY_CHANGE:
    if (body[0] == MX_PROPERTY_CHANGE_BACKGROUND &&
	(colorset > -1 && Colorset[colorset].pixmap == ParentRelative) &&
	((!Swallowed && body[2] == 0) || (Swallowed && body[2] == icon_win)))
    {
      XClearArea(dpy, icon_win, 0,0,0,0, False);
    }
    else if  (body[0] == MX_PROPERTY_CHANGE_SWALLOW && body[2] == icon_win)
    {
      Swallowed = body[1];
    }
    break;
  default:
    break;
  }
}

struct icon_info *SetFlag(unsigned long id, int t)
{
  struct icon_info *tmp;
  tmp = Head;

  while(tmp != NULL){
    if (tmp->id == id){
      if (t == M_ICONIFY)
        SET_ICONIFIED(tmp, True);
       else
        SET_ICONIFIED(tmp, False);
      return tmp;
    }
    tmp = tmp->next;
  }
  return NULL;
}

void mapicons(void)
{
  struct icon_info *tmp;
  tmp = Head;

  while(tmp != NULL){
    if (window_cond(tmp)){
      XMapWindow(dpy, tmp->IconWin);
      if (max_icon_height != 0)
	XMapWindow(dpy, tmp->icon_pixmap_w);
    }
    tmp = tmp->next;
  }
}

int AdjustIconWindows(void)
{
  struct icon_info *tmp;
  int i = 0;
  tmp = Head;

  while(tmp != NULL){
    if (window_cond(tmp) && tmp->IconWin != None)
      AdjustIconWindow(tmp, i++);
    tmp = tmp->next;
  }
  return i;
}

int window_cond(struct icon_info *item)
{
  if ((!(local_flags & CURRENT_ONLY) ||
      (IS_STICKY(item)) || (item->desk == CurrentDesk)) &&
      IS_ICON_SUPPRESSED(item) &&
      (!DO_SKIP_WINDOW_LIST(item) || !UseSkipList))
    return 1;

  return 0;
}

/************************************************************************
 * AddItem
 *	Skeleton based on AddItem() from FvwmWinList:
 *		Copyright 1994, Mike Finger.
 ***********************************************************************/
/*Bool AddItem(unsigned long id, long desk, unsigned long flags) */
Bool AddItem(ConfigWinPacket *cfgpacket)
{
  struct icon_info *new, *tmp;
  tmp = Head;

  if (cfgpacket->w == main_win || IS_TRANSIENT(cfgpacket))
    return False;

  while (tmp != NULL){
    if (tmp->id == cfgpacket->w ||
	(tmp->wmhints && (tmp->wmhints->flags & IconWindowHint) &&
	 tmp->wmhints->icon_window == cfgpacket->w))
      return False;
    tmp = tmp->next;
  }

  new = (struct icon_info *)safemalloc(sizeof(struct icon_info));
  memset(new, 0, sizeof(struct icon_info));
  new->desk = cfgpacket->desk;
  new->id = cfgpacket->w;
  new->extra_flags = DEFAULTICON;
  memcpy(&(new->flags), &(cfgpacket->flags), sizeof(new->flags));
  SET_ICON_OURS(new, True);
  SET_PIXMAP_OURS(new, True);

/* add new item to the head of the list

  new->prev = NULL;
  new->next = Head;
  if (Head != NULL)
    Head->prev = new;
  else
    Tail = new;
  Head = new; */

/* add new item to the tail of the list */
  new->prev = Tail;
  if (Tail != NULL)
    Tail->next = new;
  else
  Head = new;
  Tail = new;

  if (window_cond(new)) {
    num_icons++;
    return True;
  }

  return False;
}

/************************************************************************
 * Deletetem
 *	Skeleton based on DeleteItem() from FvwmWinList:
 *		Copyright 1994, Mike Finger.
 ***********************************************************************/
Bool DeleteItem(unsigned long id)
{
  struct icon_info *tmp = Head;

  while(tmp != NULL){
  if (tmp->id == id){
      if (window_cond(tmp))
	num_icons--;
    if (Hilite == tmp)
      Hilite = NULL;
    if ((tmp->icon_pixmap_w != None) && (IS_ICON_OURS(tmp)))
      XDestroyWindow(dpy, tmp->icon_pixmap_w);
    if (tmp->IconWin != None)
      XDestroyWindow(dpy, tmp->IconWin);
      if (tmp == Head){
    Head = tmp->next;
    if (Head != NULL)
      Head->prev = NULL;
    else
      Tail = NULL;
      }else {
      if (Tail == tmp)
	Tail = tmp->prev;
      tmp->prev->next = tmp->next;
      if (tmp->next != NULL)
	tmp->next->prev = tmp->prev;
      }
      freeitem(tmp, 1);
      return True;
    }
    tmp = tmp->next;
  }
  return False;
}

/************************************************************************
 * UpdateItem
 *	Skeleton based on UpdateItem() from FvwmWinList:
 *		Copyright 1994, Mike Finger.
 ***********************************************************************/
struct icon_info *UpdateItem(unsigned long type, unsigned long id, char *item)
{
  struct icon_info *tmp;
  char *str;
  XClassHint classhint;
  int ret;

  tmp = Head;
  while (tmp != NULL){
    if (tmp->id == id){
      str = (char *)safemalloc(strlen(item)+1);
      strcpy(str, item);

      switch (type){
      case MX_VISIBLE_ICON_NAME:
	if (tmp->name)
	  free(tmp->name);
	tmp->name = str;
	return tmp;
      case M_ICON_FILE:
	if (tmp->icon_file)
	  free(tmp->icon_file);
	tmp->icon_file = str;
	tmp->extra_flags &= ~DEFAULTICON;
	return tmp;
      case M_VISIBLE_NAME:
	if (tmp->window_name)
	  free(tmp->window_name);
	tmp->window_name = str;
	return tmp;
      case M_RES_CLASS:
	if (tmp->res_class)
	  free(tmp->res_class);
	tmp->res_class = str;
	ret = 0;
	if (sortby == RESCLASS && strcmp(NoClass, str) == 0
	    && !(ret = XGetClassHint(dpy, tmp->id, &classhint))){
	  tmp->extra_flags |= NOCLASS;
	}
	if (ret){
	  if (classhint.res_class != NULL)
	    XFree(classhint.res_class);
	  if (classhint.res_name != NULL)
	    XFree(classhint.res_name);
	}
	return tmp;
      case M_RES_NAME:
	if (tmp->res_name)
	  free(tmp->res_name);
	tmp->res_name = str;
	ret = 0;
	if (sortby == RESNAME && strcmp(NoResource, str) == 0
	    && !(ret = XGetClassHint(dpy, tmp->id, &classhint)))
	  tmp->extra_flags |= NONAME;
	if (ret){
	  if (classhint.res_class != NULL)
	    XFree(classhint.res_class);
	  if (classhint.res_name != NULL)
	    XFree(classhint.res_name);
	}
	return tmp;
      }
    }
    tmp = tmp->next;
  }
  return NULL;
}

Bool SortItem(struct icon_info *item)
{
  struct icon_info *tmp1=Head, *a, *b, *tmp2;

  if (tmp1 == NULL)
    return False;

  if (item != NULL &&
      ((itemcmp(item->prev, item) <= 0) &&
       (itemcmp(item->next, item) >= 0)))
    return False;

  while (tmp1->next != NULL){
    tmp2 = MinItem(tmp1);
    if (tmp1 == tmp2){
      tmp1 = tmp1->next;
      continue;
    }
    if (tmp1 == Head)
      Head = tmp2;
    a = tmp1->prev;
    b = tmp1->next;
    if (tmp1->prev != NULL)
      tmp1->prev->next = tmp2;
    if (b != tmp2)
      tmp1->next->prev = tmp2;
    if (b != tmp2)
      tmp2->prev->next = tmp1;
    if (tmp2->next != NULL)
      tmp2->next->prev = tmp1;
    if (b == tmp2){
      tmp1->prev = tmp2;
      tmp1->next = tmp2->next;
      tmp2->prev = a;
      tmp2->next = tmp1;
    }else{
      tmp1->prev = tmp2->prev;
      tmp1->next = tmp2->next;
      tmp2->prev = a;
      tmp2->next = b;
    }
    tmp1 = b;
  }
  Tail = tmp1;

  return True;
}

struct icon_info *MinItem(struct icon_info *head)
{
  struct icon_info *tmp, *i_min;

  if (head == NULL)
    return NULL;

  i_min = head;
  tmp = head->next;
  while (tmp != NULL){
    if (itemcmp(i_min, tmp) > 0)
      i_min = tmp;
    tmp = tmp->next;
  }
  return i_min;
}

int itemcmp(struct icon_info *item1, struct icon_info *item2)
{
  int ret1, ret2;

  if (item1 == NULL){
    if (item2 == NULL)
      return 0;
    else
      return -1;
  } else if (item2 == NULL)
    return 1;

  /* skip items not on the current desk */
  ret1 = window_cond(item1);
  ret2 = window_cond(item2);
  if (!ret1 || !ret2)
    return (ret1 - ret2);

  ret1 = 0;
  ret2 = strcmp(item1->name, item2->name);

  switch(sortby){
  case WINDOWNAME:
    ret1 = strcmp(item1->window_name, item2->window_name);
    break;
  case RESCLASS:
    if ((item1->extra_flags & NOCLASS)){
      if ((item2->extra_flags & NOCLASS))
        ret1 = 0;
      else if (!(item2->extra_flags & NOCLASS))
        ret1 = -1;
    }else if ((item2->extra_flags & NOCLASS))
      ret1 = 1;
    else
      ret1 = strcmp(item1->res_class, item2->res_class);
    break;
  case RESNAME:
    if ((item1->extra_flags & NOCLASS)){
      if ((item2->extra_flags & NOCLASS))
        ret1 = 0;
      else if (!(item2->extra_flags & NOCLASS))
        ret1 = -1;
    }else if ((item2->extra_flags & NOCLASS))
      ret1 = 1;
    else
      ret1 = strcmp(item1->res_name, item2->res_name);
    break;
  default:
    break;
  }
  return (ret1 != 0 ? ret1 : ret2);
}

/*
void ShowItem(struct icon_info *head)
{
  struct icon_info *tmp;

  fprintf(stderr, "The contents of item are as follows:\n");
  tmp = head;
  while (tmp != NULL){
    fprintf(stderr, "id:%x  name:%s resname:%s class%s iconfile:%s\n",
	    tmp->id,
	    tmp->name == NULL ? "NULL" : tmp->name, tmp->res_name,
	    tmp->res_class, tmp->icon_file);
    tmp = tmp->next;
  }
}

void ShowAction(void)
{
  struct mousefunc *tmp;

  tmp = MouseActions;
  while (tmp != NULL){
    fprintf(stderr, "mouse:%d type %d action:%s\n", tmp->mouse,
	    tmp->type, tmp->action);
    tmp = tmp->next;
  }
}

void ShowKAction(void)
{
  struct keyfunc *tmp;

  tmp = KeyActions;
  while (tmp != NULL){
    fprintf(stderr, "key:%s keycode:%d action:%s\n", tmp->name,
	    tmp->keycode, tmp->action);
    tmp = tmp->next;
  }
}
*/

void freeitem(struct icon_info *item, int d)
{
  if (item == NULL)
    return;

  if (!(IS_ICON_OURS(item))){
    if (max_icon_height != 0)
    XUnmapWindow(dpy, item->icon_pixmap_w);
    if (d == 0)
      XReparentWindow(dpy, item->icon_pixmap_w, Root, 0, 0);
  }

  if (item->name != NULL)
    free(item->name);
  if (item->window_name != NULL)
    free(item->window_name);
  if (item->res_name != NULL)
    free(item->res_name);
  if (item->res_class != NULL)
    free(item->res_class);
  if (item->icon_file != NULL)
    free(item->icon_file);
  if (item->iconPixmap != None)
    XFreePixmap(dpy, item->iconPixmap);
  if (item->icon_maskPixmap != None &&
      (item->wmhints == NULL ||
       !(item->wmhints->flags & (IconPixmapHint|IconWindowHint))))
    XFreePixmap(dpy, item->icon_maskPixmap);
  if (item->wmhints != NULL)
    XFree(item->wmhints);
  free(item);
}


/************************************************************************
 * CheckActionType
 *   Based on functions.c from Fvwm:
 ***********************************************************************/
static int CheckActionType(
  int x, int y, XEvent *d, Bool may_time_out, Bool is_button_pressed)
{
  int xcurrent,ycurrent,total = 0;
  int dist;
  Bool do_sleep = False;

  xcurrent = x;
  ycurrent = y;
  dist = DEFAULT_MOVE_THRESHOLD;

  while (total < ClickTime || !may_time_out)
  {
    if (!(x - xcurrent <= dist && xcurrent - x <= dist &&
	  y - ycurrent <= dist && ycurrent - y <= dist))
    {
      return TIMEOUT;
    }

    if (do_sleep)
    {
      usleep(20000);
    }
    else
    {
      usleep(1);
      do_sleep = 1;
    }
    total += 20;
    if (XCheckMaskEvent(dpy, ButtonReleaseMask|ButtonMotionMask|
			PointerMotionMask|ButtonPressMask|ExposureMask, d))
    {
      do_sleep = 0;
      switch (d->xany.type)
      {
      case ButtonRelease:
	return CLICK;
      case MotionNotify:
	if ((d->xmotion.state & DEFAULT_ALL_BUTTONS_MASK) ||
            !is_button_pressed)
	{
	  xcurrent = d->xmotion.x_root;
	  ycurrent = d->xmotion.y_root;
	}
        else
	{
	  return CLICK;
	}
	break;
      case ButtonPress:
	if (may_time_out)
	  is_button_pressed = True;
	break;
      default:
	/* can't happen */
	break;
      }
    }
  }

  return TIMEOUT;
}


/************************************************************************
 * ExecuteAction
* 	Based on part of ComplexFunction() of functions.c from fvwm:
	Copyright 1988, Evans and Sutherland Computer Corporation,
       	Copyright 1989, Massachusetts Institute of Technology,
       	Copyright 1993, Robert Nation.
 ***********************************************************************/
void ExecuteAction(int x, int y, struct icon_info *item)
{
  int type = NO_CLICK;
  XEvent *ev;
  XEvent d;
  struct mousefunc *tmp;

  /* Wait and see if we have a click, or a move */
  /* wait forever, see if the user releases the button */
  type = CheckActionType(x, y, &d, False, True);
  if (type == CLICK)
  {
    ev = &d;
    /* If it was a click, wait to see if its a double click */
    if (have_double_click)
    {
      type = CheckActionType(x, y, &d, True, False);
      switch (type)
      {
      case CLICK:
	type = DOUBLE_CLICK;
	ev = &d;
	break;
      case TIMEOUT:
	type = CLICK;
	break;
      default:
	/* can't happen */
	break;
      }
    }
  }
  tmp = MouseActions;

  while (tmp != NULL){
    if ((tmp->mouse == d.xbutton.button || tmp->mouse == 0) &&
	tmp->type == type)
    {
      MySendFvwmPipe(fd, tmp->action, item->id);
      return;
    }
    tmp = tmp->next;
  }
}

void ExecuteKey(XEvent event)
{
  struct icon_info *item;
  struct keyfunc *tmp;

  if ((item = Hilite) == NULL)
     if ((item = Head) == NULL)
       return;

  tmp = KeyActions;
  event.xkey.keycode =
    XKeysymToKeycode(dpy,XKeycodeToKeysym(dpy,event.xkey.keycode,0));
  while (tmp != NULL){
    if (tmp->keycode == event.xkey.keycode){
      MySendFvwmPipe(fd, tmp->action, item->id);
      return;
    }
    tmp = tmp->next;
  }
}

/***********************************************************************
 LookInList
	Based on part of LookInList() of add_window.c from fvwm:
	Copyright 1988, Evans and Sutherland Computer Corporation,
       	Copyright 1989, Massachusetts Institute of Technology,
       	Copyright 1993, Robert Nation.
 ***********************************************************************/
int LookInList(struct icon_info *item)
{
  int isdefault=1;
  char *value=NULL;
  struct iconfile *nptr;

  if (IconListHead == NULL) {
    if ((item->extra_flags & DEFAULTICON) && (FvwmDefaultIcon != NULL))
    {
      if (item->icon_file)
	free(item->icon_file);
      item->icon_file = safestrdup(FvwmDefaultIcon);
    }
    return 1;
  }


  for (nptr = IconListHead; nptr != NULL; nptr = nptr->next){
    if (nptr == DefaultIcon)
      isdefault = 1;

    if (matchWildcards(nptr->name, item->res_class) == True){
      value = nptr->iconfile;
      if (nptr != DefaultIcon)
	isdefault = 0;
    }

    if (matchWildcards(nptr->name, item->res_name) == True){
      value = nptr->iconfile;
      if (nptr != DefaultIcon)
	isdefault = 0;
    }

    if (matchWildcards(nptr->name, item->window_name) == True){
      value = nptr->iconfile;
      if (nptr != DefaultIcon)
	isdefault = 0;
    }
  }

  if (!isdefault)
  {
    if (item->icon_file)
      free(item->icon_file);
    if (value)
      item->icon_file = safestrdup(value);
    else
      item->icon_file = NULL;
    item->extra_flags &= ~DEFAULTICON;
  }
  else if ((item->extra_flags & DEFAULTICON))
  {
    if (DefaultIcon != NULL)
    {
      if (item->icon_file)
	free(item->icon_file);
      item->icon_file = safestrdup(DefaultIcon->iconfile);
    }
    else if (FvwmDefaultIcon != NULL)
    {
      if (item->icon_file)
	free(item->icon_file);
      item->icon_file = safestrdup(FvwmDefaultIcon);
    }
  }

  /* Icon is not shown if "-" is specified */
  if (item->icon_file != NULL && (strcmp(item->icon_file, "-") == 0)){
    DeleteItem(item->id);
    return 0;
  }
  return 1;
}

/***********************************************************************
 strcpy
	Based on stripcpy2() of configure.c from Fvwm:
	Copyright 1988, Evans and Sutherland Computer Corporation,
       	Copyright 1989, Massachusetts Institute of Technology,
       	Copyright 1993, Robert Nation.
 ***********************************************************************/
char *stripcpy2(char *source)
{
  char *ptr;
  int count = 0;
  while((*source != '"')&&(*source != 0))
    source++;
  if(*source == 0)
    return 0;

  source++;
  ptr = source;
  while((*ptr!='"')&&(*ptr != 0)){
    ptr++;
    count++;
  }
  ptr = safemalloc(count+1);
  strncpy(ptr,source,count);
  ptr[count]=0;
  return ptr;
}

/***********************************************************************
 Error handler
 ***********************************************************************/
static int
myErrorHandler(Display *dpy, XErrorEvent *event)
{
	if((event->error_code == BadAccess) && do_allow_bad_access)
	{
		/* may happen when reparenting a icon window */
		was_bad_access = 1;
		return 0;
	}
	/* some errors are ignored, mostly due to colorsets changing too fast */
	if (event->error_code == BadWindow)
		return 0;
	if (event->error_code == BadDrawable)
		return 0;
	if (event->error_code == BadPixmap)
		return 0;
	if (event->error_code == FRenderGetErrorCodeBase() + FRenderBadPicture)
		return 0;

	PrintXErrorAndCoredump(dpy, event, MyName);
	return 0;
}
