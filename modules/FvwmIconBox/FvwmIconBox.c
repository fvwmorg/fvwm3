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

#define TRUE 1
#define FALSE 0

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

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "../../fvwm/module.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

/* just as same as wild.c */
#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

#include "FvwmIconBox.h"

char *MyName;

XFontStruct *font;

Display *dpy;			/* which display are we talking to */
int x_fd,fd_width;

Window Root;
int screen;
int d_depth;

char *Back = "#5f9ea0";
char *Fore = "#82dfe3";
char *IconBack = "#cfcfcf";
char *IconFore = "black";
char *ActIconBack = "white";
char *ActIconFore = "black";
char *font_string = "fixed";

/* same strings as in misc.c */
char NoClass[] = "NoClass";
char NoResource[] = "NoResource";

Pixel fore_pix, hilite_pix, back_pix, shadow_pix;
Pixel icon_fore_pix, icon_back_pix, icon_hilite_pix, icon_shadow_pix;
Pixel act_icon_fore_pix, act_icon_back_pix, act_icon_hilite_pix, act_icon_shadow_pix;

GC  NormalGC,ShadowGC,ReliefGC,IconShadowGC,IconReliefGC;
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
		 M_END_WINDOWLIST| M_ICONIFY|M_DEICONIFY|M_ICON_NAME|
		 M_RES_NAME|M_RES_CLASS|M_WINDOW_NAME|M_ICON_FILE|
		 M_DEFAULTICON|M_CONFIG_INFO|M_END_CONFIG_INFO;

struct icon_info *Hilite;
int main_width, main_height;
int num_icons = 0;
int num_rows = 1;
int num_columns = 6;
int Lines = 6;
int max_icon_width = 48,max_icon_height = 48;
int ButtonWidth,ButtonHeight;
int x= -100000,y= -100000,w= -1,h= -1,gravity = NorthWestGravity;
int icon_win_x = 0, icon_win_y = 0, icon_win_width = 100,
icon_win_height = 100;
int interval = 8;
int motion = NONE;
int primary = LEFT, secondary = BOTTOM;
int xneg = 0, yneg = 0;
int ClickTime = 150;
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
char *iconPath = NULL;
char *pixmapPath = NULL;
char *FvwmDefaultIcon = NULL;

static Atom wm_del_win;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_NAME;

int ready = 0;
unsigned long local_flags = 0;
int sortby = UNSORT;

int save_color_limit;                   /* color limit from config */

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

  temp = argv[0];

  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;


  MyName = safemalloc(strlen(temp)+1);
  strcpy(MyName, temp);

  signal (SIGPIPE, DeadPipe);

  if((argc != 6)&&(argc != 7))
    {
      fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MyName,
	      VERSION);
      exit(1);
    }

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  if (!(dpy = XOpenDisplay(display_name)))
    {
      fprintf(stderr,"%s: can't open display %s", MyName,
	      XDisplayName(display_name));
      exit (1);
    }
  x_fd = XConnectionNumber(dpy);

  fd_width = GetFdWidth();

  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);
  if(Root == None)
    {
      fprintf(stderr,"%s: Screen %d is not valid ", MyName, screen);
      exit(1);
    }
  InitPictureCMap(dpy,Root); /* store the root cmap */
  d_depth = DefaultDepth(dpy, screen);

  XSetErrorHandler((XErrorHandler)myErrorHandler);

  ParseOptions();

  SetMessageMask(fd, m_mask);

  if ((local_flags & SETWMICONSIZE) && (size = XAllocIconSize()) != NULL){
    size->max_width  = size->min_width  = max_icon_width + icon_relief;
    size->max_height = size->min_height = max_icon_height + icon_relief;
    size->width_inc  = size->height_inc = 0;
    XSetIconSizes(dpy, Root, size, 1);
    XFree(size);
  }

  CreateWindow();

  SendFvwmPipe(fd,"Send_WindowList",0);

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
  int x,y,border_width,depth;
  int i, hr = icon_relief/2;
  XEvent Event;
  int tw,th;
  int diffx, diffy;
  int oldw, oldh;

  while(1)
    {
      if(My_XNextEvent(dpy,&Event))
	{
	  switch(Event.type)
	    {
	    case Expose:
	      if(Event.xexpose.count == 0){
		if (Event.xany.window == main_win){
		  RedrawWindow();
		}else{
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
	      XGetGeometry(dpy,main_win,&root,&x,&y,
			   (unsigned int *)&tw,(unsigned int *)&th,
			   (unsigned int *)&border_width,
			   (unsigned int *)&depth);
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
		XClearWindow(dpy,main_win);
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
		DeadPipe(1);
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
		if (tmp == NULL || tmp->wmhints == NULL || !(tmp->extra_flags & DEFAULTICON))
		  break;
		if (tmp->wmhints)
		  XFree (tmp->wmhints);
		tmp->wmhints = XGetWMHints(dpy, tmp->id);
		if (tmp->wmhints && (tmp->wmhints->flags & IconPixmapHint)){
#ifdef SHAPE
		/* turn off "old" shape mask */
		if (tmp->icon_maskPixmap != None)
		  XShapeCombineMask(dpy, tmp->icon_pixmap_w,
				    ShapeBounding, 0, 0, None, ShapeSet);
#endif
		  if (tmp->iconPixmap != None)
		    XFreePixmap(dpy, tmp->iconPixmap);
		  GetIconBitmap(tmp);
#ifdef SHAPE
		  if (tmp->icon_maskPixmap != None)
		    XShapeCombineMask(dpy, tmp->icon_pixmap_w,
				      ShapeBounding, hr, hr,
				      tmp->icon_maskPixmap, ShapeSet);
#endif
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

  RelieveWindow(main_win, margin1, margin1, Width + 4,
		Height + 4, ShadowGC,ReliefGC);
  if (!(local_flags & HIDE_H))
    RelieveWindow(main_win, margin1, margin1 + 4 + Height + margin2,
		  Width + 4, bar_width+4, ShadowGC,ReliefGC);
  if (!(local_flags & HIDE_V))
    RelieveWindow(main_win, margin1 + 4 + Width + margin2, margin1,
		  bar_width+4, Height + 4, ShadowGC,ReliefGC);
  RelieveWindow(main_win, 0, 0, Width + h_margin, Height + v_margin,
		ReliefGC, ShadowGC);

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
    if (desk_cond(tmp))
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

  if (Hilite == item){
    XSetForeground(dpy, NormalGC, act_icon_fore_pix);
    XSetBackground(dpy, NormalGC, act_icon_back_pix);
    XSetForeground(dpy, IconReliefGC, act_icon_hilite_pix);
    XSetForeground(dpy, IconShadowGC, act_icon_shadow_pix);

    if (max_icon_height != 0 && (item->flags & ICON_OURS))
      XSetWindowBackground(dpy, item->icon_pixmap_w, act_icon_back_pix);
    XSetWindowBackground(dpy, item->IconWin, act_icon_back_pix);
  }

  /* icon pixmap */
  if ((f & 1) && (item->flags & ICON_OURS)){
    if (item->iconPixmap != None && item->icon_pixmap_w != None){
      if (item->icon_depth != d_depth)
	XCopyPlane(dpy, item->iconPixmap, item->icon_pixmap_w, NormalGC,
		   0, 0, item->icon_w, item->icon_h,
		   hr, hr, plane);
      else
	XCopyArea(dpy, item->iconPixmap, item->icon_pixmap_w, NormalGC,
		  0, 0, item->icon_w, item->icon_h, hr, hr);
      }
    if (!(item->flags & SHAPED_ICON)){
      if (item->icon_w > 0 && item->icon_h > 0)
	RelieveWindow(item->icon_pixmap_w, 0, 0, item->icon_w
		      +icon_relief,
		      item->icon_h + icon_relief, IconReliefGC,
		      IconShadowGC);
      else
	RelieveWindow(item->icon_pixmap_w, 0, 0, max_icon_width
		    +icon_relief,
		    max_icon_height + icon_relief, IconReliefGC,
		    IconShadowGC);
    }
  }

  /* label */
  if (f & 2){
    w = max_icon_width + icon_relief;
    h = max_icon_height + icon_relief;

    if (item->flags & ICONIFIED){
      sprintf(label, "(%s)", item->name);
    }else
      strcpy(label, item->name);

    len = strlen(label);
    tw = XTextWidth(font, label, len);
    diff = max_icon_width + icon_relief - tw;
    lm = diff/2;
    lm = lm > 4 ? lm : 4;

    if (Hilite == item){
      XRaiseWindow(dpy, item->IconWin);
      XMoveResizeWindow(dpy, item->IconWin,
			item->x + min(0, (diff - 8))/2,
			item->y + h,
			max(tw + 8, w), 6 + font->ascent +
			font->descent);
      XClearWindow(dpy, item->IconWin);
      XDrawString(dpy, item->IconWin, NormalGC, lm, 3 + font->ascent,
		  label, len);
      RelieveWindow(item->IconWin, 0, 0,
		    max(tw + 8, w), 6 + font->ascent +
		    font->descent, IconReliefGC, IconShadowGC);
    }else{
      XMoveResizeWindow(dpy, item->IconWin, item->x, item->y + h,
			w, 6 + font->ascent + font->descent);
      XClearWindow(dpy, item->IconWin);
      XDrawString(dpy, item->IconWin, NormalGC, lm, 3 + font->ascent,
		  label, len);
      RelieveWindow(item->IconWin, 0, 0,
		    w, 6 + font->ascent + font->descent,
		    IconReliefGC, IconShadowGC);
    }
  }

  if (Hilite == item){
    XSetForeground(dpy, NormalGC, icon_fore_pix);
    XSetBackground(dpy, NormalGC, icon_back_pix);
    XSetForeground(dpy, IconReliefGC, icon_hilite_pix);
    XSetForeground(dpy, IconShadowGC, icon_shadow_pix);

    if (max_icon_height != 0 && (item->flags & ICON_OURS))
      XSetWindowBackground(dpy, item->icon_pixmap_w, icon_back_pix);
    XSetWindowBackground(dpy, item->IconWin, icon_back_pix);
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
  RelieveWindow(h_scroll_bar, x, 0, width, bar_width, ReliefGC, ShadowGC);
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
  RelieveWindow(v_scroll_bar, 0, y, bar_width, height, ReliefGC, ShadowGC);
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
  RelieveWindow
    Original work from GoodStuff:
      Copyright 1993, Robert Nation.
************************************************************************/
void RelieveWindow(Window win,int x,int y,int w,int h, GC rgc,GC sgc)
{
  XSegment seg[4];
  int i;

  i=0;
  seg[i].x1 = x;        seg[i].y1   = y;
  seg[i].x2 = w+x-1;    seg[i++].y2 = y;

  seg[i].x1 = x;        seg[i].y1   = y;
  seg[i].x2 = x;        seg[i++].y2 = h+y-1;

  seg[i].x1 = x+1;      seg[i].y1   = y+1;
  seg[i].x2 = x+w-2;    seg[i++].y2 = y+1;

  seg[i].x1 = x+1;      seg[i].y1   = y+1;
  seg[i].x2 = x+1;      seg[i++].y2 = y+h-2;
  XDrawSegments(dpy, win, rgc, seg, i);

  i=0;
  seg[i].x1 = x;        seg[i].y1   = y+h-1;
  seg[i].x2 = w+x-1;    seg[i++].y2 = y+h-1;

  seg[i].x1 = x+w-1;    seg[i].y1   = y;
  seg[i].x2 = x+w-1;    seg[i++].y2 = y+h-1;
  if(d_depth<2)
    XDrawSegments(dpy, win, ShadowGC, seg, i);
  else
    XDrawSegments(dpy, win, sgc, seg, i);

  i=0;
  seg[i].x1 = x+1;      seg[i].y1   = y+h-2;
  seg[i].x2 = x+w-2;    seg[i++].y2 = y+h-2;

  seg[i].x1 = x+w-2;    seg[i].y1   = y+1;
  seg[i].x2 = x+w-2;    seg[i++].y2 = y+h-2;

  XDrawSegments(dpy, win, sgc, seg, i);
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
  if ((font = XLoadQueryFont(dpy, font_string)) == NULL)
    {
      if ((font = XLoadQueryFont(dpy, "fixed")) == NULL)
	{
	  fprintf(stderr,"%s: No fonts available\n",MyName);
	  exit(1);
	}
    };

  if ((local_flags & HIDE_H))
    v_margin -= bar_width + margin2 + 4;
  if ((local_flags & HIDE_V))
    h_margin -= bar_width + margin2 + 4;

  UWidth = max_icon_width + icon_relief + interval;
  UHeight = font->ascent + font->descent + max_icon_height +
    icon_relief + 6 + interval;
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

  if(x > -100000)
    {
      if (xneg)
	{
	  mysizehints.x = DisplayWidth(dpy,screen) + x - mysizehints.width;
	  gravity = NorthEastGravity;
	}
      else
	mysizehints.x = x;
      if (yneg)
	{
	  mysizehints.y = DisplayHeight(dpy,screen) + y - mysizehints.height;
	  gravity = SouthWestGravity;
	}
      else
	mysizehints.y = y;

      if((xneg) && (yneg))
	gravity = SouthEastGravity;

      mysizehints.flags |= USPosition;
    }

  mysizehints.win_gravity = gravity;

  if(d_depth < 2)
    {
      back_pix = icon_back_pix = act_icon_fore_pix = GetColor("white");
      fore_pix = icon_fore_pix = act_icon_back_pix = GetColor("black");
      hilite_pix = icon_hilite_pix = act_icon_shadow_pix = icon_back_pix;
      shadow_pix = icon_shadow_pix = act_icon_hilite_pix = icon_fore_pix;
    }
  else
    {
      fore_pix = GetColor(Fore);
      back_pix = GetColor(Back);
      icon_back_pix = GetColor(IconBack);
      icon_fore_pix = GetColor(IconFore);
      icon_hilite_pix = GetHilite(icon_back_pix);
      icon_shadow_pix = GetShadow(icon_back_pix);
      act_icon_back_pix = GetColor(ActIconBack);
      act_icon_fore_pix = GetColor(ActIconFore);
      act_icon_hilite_pix = GetHilite(act_icon_back_pix);
      act_icon_shadow_pix = GetShadow(act_icon_back_pix);
      hilite_pix = GetHilite(back_pix);
      shadow_pix = GetShadow(back_pix);
    }

  main_win = XCreateSimpleWindow(dpy,Root,mysizehints.x,mysizehints.y,
				 mysizehints.width,mysizehints.height,
				 0,fore_pix,back_pix);
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

  gcm = GCForeground|GCBackground;
  gcv.foreground = hilite_pix;
  gcv.background = back_pix;
  ReliefGC = XCreateGC(dpy, Root, gcm, &gcv);

  gcm = GCForeground|GCBackground;
  gcv.foreground = shadow_pix;
  gcv.background = back_pix;
  ShadowGC = XCreateGC(dpy, Root, gcm, &gcv);

  gcm = GCForeground|GCBackground;
  gcv.foreground = icon_hilite_pix;
  gcv.background = icon_back_pix;
  IconReliefGC = XCreateGC(dpy, Root, gcm, &gcv);

  gcm = GCForeground|GCBackground;
  gcv.foreground = icon_shadow_pix;
  gcv.background = icon_back_pix;
  IconShadowGC = XCreateGC(dpy, Root, gcm, &gcv);

  gcm = GCForeground|GCBackground|GCFont;
  gcv.foreground = fore_pix;
  gcv.background = back_pix;
  gcv.font =  font->fid;
  NormalGC = XCreateGC(dpy, Root, gcm, &gcv);


  /* icon_win's background */
  if (GetBackPixmap() == True){
    XSetWindowBackgroundPixmap(dpy, icon_win, IconwinPixmap);
    /*  special thanks to Dave Goldberg <dsg@mitre.org>
	for his helpful information */
    XFreePixmap(dpy, IconwinPixmap);
  }

  XSetForeground(dpy, NormalGC, icon_fore_pix);
  XSetBackground(dpy, NormalGC, icon_back_pix);

  /* scroll bars */
  mask = CWWinGravity | CWBackPixel;
  attributes.background_pixel = back_pix;
  if (!(local_flags & HIDE_H)){
    attributes.win_gravity = SouthWestGravity;
    h_scroll_bar = XCreateWindow(dpy ,main_win, margin1 + 2 +
				 bar_width,
				 margin1 + 6 + Height + margin2,
				 Width - bar_width*2, bar_width,
				 0, CopyFromParent,
				 InputOutput, CopyFromParent,
				 mask, &attributes);
  XSelectInput(dpy,h_scroll_bar,SCROLL_EVENTS);
  }
  if (!(local_flags & HIDE_V)){
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
  if (!(local_flags & HIDE_H)){
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

/************************************************************************
 * nocolor
 * 	Original work from GoodStuff:
 *		Copyright 1993, Robert Nation.
 ***********************************************************************/
void nocolor(char *a, char *b)
{
 fprintf(stderr,"%s: can't %s %s\n", MyName, a,b);
}


/************************************************************************
 * GetColor
 * 	Original work from GoodStuff:
 *		Copyright 1993, Robert Nation.
 ***********************************************************************/
Pixel GetColor(char *name)
{
  XColor color;
  XWindowAttributes attributes;

  XGetWindowAttributes(dpy,Root,&attributes);
  color.pixel = 0;
   if (!XParseColor (dpy, attributes.colormap, name, &color))
     {
       nocolor("parse",name);
     }
   else if(!XAllocColor (dpy, attributes.colormap, &color))
     {
       nocolor("alloc",name);
     }
  return color.pixel;
}

/************************************************************************
  SendFvwmPipe - Send a message back to fvwm
    Original work from FvwmWinList:
      Copyright 1994, Mike Finger.
************************************************************************/
void SendFvwmPipe(int *fd, char *message,unsigned long window)
{
  int w;
  char *hold,*temp,*temp_msg;
  hold=message;

  while(1) {
    temp=strchr(hold,',');
    if (temp!=NULL) {
      temp_msg=malloc(temp-hold+1);
      strncpy(temp_msg,hold,(temp-hold));
      temp_msg[(temp-hold)]='\0';
      hold=temp+1;
    } else temp_msg=hold;

    if (!ExecIconBoxFunction(temp_msg)){
      write(fd[0],&window, sizeof(unsigned long));

      w=strlen(temp_msg);
      write(fd[0],&w,sizeof(int));
      write(fd[0],temp_msg,w);

      /* keep going */
      w=1;
      write(fd[0],&w,sizeof(int));

    }
    if(temp_msg!=hold) free(temp_msg);
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
 * DeadPipe --Dead pipe handler
 * 	Based on DeadPipe() from GoodStuff:
 *		Copyright 1993, Robert Nation.
 ***********************************************************************/
void DeadPipe(int nonsense)
{
#if 0 /* can't do X or malloc stuff in a signal handler, so just exit */
  struct icon_info *tmpi, *tmpi2;
  struct mousefunc *tmpm, *tmpm2;
  struct keyfunc *tmpk, *tmpk2;
  struct iconfile *tmpf, *tmpf2;

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
  if ((local_flags & SETWMICONSIZE))
    XDeleteProperty(dpy, Root, XA_WM_ICON_SIZE);
  XSync(dpy,0);
#endif /* 0 */
  exit(0);
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

  GetConfigLine(fd,&tline);

  while(tline != NULL)
    {
      int g_x, g_y, flags;
      unsigned width,height;

      if(strlen(&tline[0])>1){
	if (strncasecmp(tline,CatString3("*", MyName,
					  "Geometry"),Clength+9)==0){
	  tmp = &tline[Clength+9];
	  while(((isspace(*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	    tmp++;
	  tmp[strlen(tmp)-1] = 0;
	  flags = XParseGeometry(tmp,&g_x,&g_y,&width,&height);
	  if (flags & WidthValue)
	    num_columns = width;
	  if (flags & HeightValue)
	    num_rows = height;
	  if (flags & XValue)
	    x = g_x;
	  if (flags & YValue)
	    y = g_y;
	  if (flags & XNegative)
	    xneg = 1;
	  if (flags & YNegative)
	    yneg = 1;
	} else if (strncasecmp(tline,CatString3("*", MyName,
						"MaxIconSize"),Clength+12)==0){
	  tmp = &tline[Clength+12];
	  while(((isspace(*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	    tmp++;
	  tmp[strlen(tmp)-1] = 0;

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
	}else if
	  (strncasecmp(tline,CatString3("*",MyName,"Font"),Clength+5)==0)
	  CopyString(&font_string,&tline[Clength+5]);
	else if
	  (strncasecmp(tline,CatString3("*",MyName,"IconFore"),Clength+9)==0)
	  CopyString(&IconFore,&tline[Clength+9]);
	else if
	  (strncasecmp(tline,CatString3("*",MyName,"IconBack"),Clength+9)==0)
	    CopyString(&IconBack,&tline[Clength+9]);
	else if
	  (strncasecmp(tline,CatString3("*",MyName,"IconHiFore"),Clength+11)==0)
	  CopyString(&ActIconFore,&tline[Clength+11]);
	else if
	  (strncasecmp(tline,CatString3("*",MyName,"IconHiBack"),Clength+11)==0)
	  CopyString(&ActIconBack,&tline[Clength+11]);
	else if (strncasecmp(tline,CatString3("*",MyName,
					      "Fore"),Clength+5)==0)
	  CopyString(&Fore,&tline[Clength+5]);
	else if (strncasecmp(tline,CatString3("*",MyName,
					      "Back"),Clength+5)==0)
	  CopyString(&Back,&tline[Clength+5]);
	else if (strncasecmp(tline,CatString3("*",MyName,
					      "Pixmap"),Clength+7)==0)
	  CopyString(&IconwinPixmapFile,&tline[Clength+7]);
	else if (strncasecmp(tline,CatString3("*",MyName,
					      "Padding"),Clength+8)==0)
	  interval = max(0,atoi(&tline[Clength+8]));
	else if (strncasecmp(tline,CatString3("*",MyName,
					      "FrameWidth"),Clength+11)==0){
	  sscanf(&tline[Clength+11], "%d %d", &margin1, &margin2);
	  margin1 = max(0, margin1);
	  margin2 = max(0, margin2);
	}else if (strncasecmp(tline,CatString3("*",MyName,
					      "Lines"),Clength+6)==0)
	  Lines = max(1,atoi(&tline[Clength+6]));
	else if (strncasecmp(tline,CatString3("*",MyName,
					      "SBWidth"),Clength+8)==0)
	  bar_width = max(5,atoi(&tline[Clength+8]));
	else if (strncasecmp(tline,CatString3("*",MyName,
						"Placement"),Clength+10)==0)
	  parseplacement(&tline[Clength+10]);
	else if (strncasecmp(tline,CatString3("*",MyName,
					      "SetWMIconSize"),Clength+14)==0)
	  local_flags |= SETWMICONSIZE;
	else if (strncasecmp(tline,CatString3("*",MyName,
					      "HilightFocusWin"),Clength+16)==0)
	  m_mask |= M_FOCUS_CHANGE;
	else if (strncasecmp(tline,CatString3("*",MyName,
					      "Resolution"),Clength+11)==0){
	  tmp = &tline[Clength+11];
	  while(((isspace(*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	    tmp++;
	  if (strncasecmp(tmp, "Desk", 4) == 0){
	    m_mask |= M_NEW_DESK;
	    local_flags |= CURRENT_ONLY;
	  }
	}else if (strncasecmp(tline,CatString3("*",MyName,
					      "Mouse"),Clength+6)==0)
	  parsemouse(&tline[Clength + 6]);
	else if (strncasecmp(tline,CatString3("*",MyName,
					      "Key"),Clength+4)==0)
	  parsekey(&tline[Clength + 4]);
	else if (strncasecmp(tline,CatString3("*",MyName,
					      "SortIcons"),Clength+10)==0){
	  tmp = &tline[Clength+10];
	  while(((isspace(*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	    tmp++;
	  if (strlen(tmp) == 0){ /* the case where no argument is given */
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
	}else if (strncasecmp(tline,CatString3("*",MyName,
					      "HideSC"),Clength+7)==0){
	  tmp = &tline[Clength+7];
	  while(((isspace(*tmp))&&(*tmp != '\n'))&&(*tmp != 0))
	    tmp++;
	  if (strncasecmp(tmp, "Horizontal", 10) == 0)
	    local_flags |= HIDE_H;
	  else if (strncasecmp(tmp, "Vertical", 8) == 0)
	    local_flags |= HIDE_V;
	}else if (strncasecmp(tline,CatString3("*",MyName,
					      ""),Clength+1)==0)
	  parseicon(&tline[Clength + 1]);
	else if (strncasecmp(tline,"IconPath",8)==0)
	  CopyString(&iconPath,&tline[8]);
	else if (strncasecmp(tline,"PixmapPath",10)==0)
	  CopyString(&pixmapPath,&tline[10]);
	else if (strncasecmp(tline,"ClickTime",9)==0)
	  ClickTime = atoi(&tline[9]);
	else if (strncasecmp(tline,"ColorLimit",10)==0) {
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
   while(isspace(*tline)&&(*tline != '\n')&&(*tline != 0))
     tline++;
   start = tline;
   end = tline;
   while(!isspace(*end)&&(*end != '\n')&&(*end != 0))
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
  char *ptr,*start,*end,*tmp;

  f = (struct mousefunc *)safemalloc(sizeof(struct mousefunc));
  f->next = NULL;
  f->mouse = 0;

  /* skip spaces */
  while(isspace(*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;
  start = tline;
  end = tline;
  while((!isspace(*end))&&(*end!='\n')&&(*end!=0))
    end++;
  if (strncasecmp(start, "1", 1) == 0)
    f->mouse = Button1;
  else if (strncasecmp(start, "2", 1) == 0)
    f->mouse = Button2;
  else if (strncasecmp(start, "3", 1) == 0)
    f->mouse = Button3;
  /* click or doubleclick */
  tline = end;
  /* skip spaces */
  while(isspace(*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;
  start = tline;
  end = tline;
  while((!isspace(*end))&&(*end!='\n')&&(*end!=0))
    end++;
  if (strncasecmp(start, "Click", 5) == 0)
    f->type = CLICK;
  else if (strncasecmp(start, "DoubleClick", 11) == 0)
    f->type = DOUBLE_CLICK;

  /* actions */
  tline = end;
  /* skip spaces */
  while(isspace(*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;
  start = tline;
  end = tline;
  tmp = tline;
  while((*tmp!='\n')&&(*tmp!=0)){
    if (!isspace(*tmp))
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
  while(isspace(*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;
  start = tline;
  end = tline;
  while((!isspace(*end))&&(*end!='\n')&&(*end!=0))
    end++;
  nlen = end - start;
  nptr = safemalloc(nlen+1);
  strncpy(nptr, start, nlen);
  nptr[nlen] = 0;

  /* actions */
  tline = end;
  /* skip spaces */
  while(isspace(*tline)&&(*tline != '\n')&&(*tline != 0))
    tline++;
  start = tline;
  end = tline;
  tmp = tline;
  while((*tmp!='\n')&&(*tmp!=0)){
    if (!isspace(*tmp))
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
  unsigned long header[HEADER_SIZE];
  static int miss_counter = 0;
  unsigned long *body;

  if(XPending(dpy))
    {
      XNextEvent(dpy,event);
      return 1;
    }

  FD_ZERO(&in_fdset);
  FD_SET(x_fd,&in_fdset);
  FD_SET(fd[1],&in_fdset);

  select(fd_width,SELECT_TYPE_ARG234 &in_fdset, 0, 0, NULL);

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
	DeadPipe(0);
    }

  if(FD_ISSET(fd[1], &in_fdset))
    {
      if(ReadFvwmPacket(fd[1],header,&body) > 0)
	{
	  process_message(header[1],body);
	  free(body);
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
  struct icon_info *tmp, *old;
  char *str;
  long olddesk;

  switch(type){
  case M_CONFIGURE_WINDOW:
    if (ready){
      if (!(local_flags & CURRENT_ONLY)) break;
      tmp = Head;
      while(tmp != NULL){
	if (tmp->id == body[0]){
	  if ((tmp->desk != body[7]) && !(tmp->flags & STICKY)){
	    olddesk = tmp->desk;
	    tmp->desk = body[7];
	    if (olddesk == CurrentDesk || tmp->desk == CurrentDesk){
	      if (tmp->desk == CurrentDesk && sortby != UNSORT)
		SortItem(NULL);
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
	      if (tmp->desk == CurrentDesk){
		XMapWindow(dpy, tmp->IconWin);
		if (max_icon_height != 0)
		  XMapWindow(dpy, tmp->icon_pixmap_w);
	      }else{
		XUnmapWindow(dpy, tmp->IconWin);
		if (max_icon_height != 0)
		  XUnmapWindow(dpy, tmp->icon_pixmap_w);
	      }
	      if (!(local_flags & HIDE_H) && diffx)
		RedrawHScrollbar();
	      if (!(local_flags & HIDE_V) && diffy)
		RedrawVScrollbar();
	    }
	  }else if ((body[8] & STICKY) && !(tmp->flags & STICKY)) /* stick */
	    tmp->flags |= STICKY;
	  else if (!(body[8] & STICKY) && (tmp->flags & STICKY)){ /* unstick */
	    tmp->flags &= ~STICKY;
	    tmp->desk = body[7];
	  }
	  return;
	}
	tmp = tmp->next;
      }
      break;
    }
  case M_ADD_WINDOW:
    if (AddItem(body[0], body[7], body[8]) == True && ready){
      GetIconwinSize(&diffx, &diffy);
      if (diffy && (primary == BOTTOM || secondary == BOTTOM))
	icon_win_y += diffy;
      if (diffx && (primary == RIGHT || secondary == RIGHT))
	icon_win_x += diffx;
      XMoveResizeWindow(dpy, icon_win, -icon_win_x, -icon_win_y,
			icon_win_width, icon_win_height);
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
  case M_WINDOW_NAME:
    tmp = UpdateItem(type, body[0], (char *)&body[3]);
    if (!ready || tmp == NULL)
      break;
    if (sortby == WINDOWNAME && tmp->IconWin != None
	&& desk_cond(tmp) && SortItem(tmp) == True)
      AdjustIconWindows();
    break;
  case M_RES_NAME:
    if ((tmp = UpdateItem(type, body[0], (char *)&body[3])) == NULL)
      break;
    if (LookInList(tmp) && ready){
      if (sortby != UNSORT)
	SortItem(tmp);
      CreateIconWindow(tmp);
      ConfigureIconWindow(tmp);
      AdjustIconWindows();
      if (desk_cond(tmp)){
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
  case M_ICON_NAME:
    tmp = UpdateItem(type, body[0], (char *)&body[3]);
    if (!ready || tmp == NULL)
    break;
    if (sortby != UNSORT && tmp->IconWin != None
	&& desk_cond(tmp) && SortItem(tmp) == True)
      AdjustIconWindows();
    if (tmp->IconWin != None && desk_cond(tmp))
      RedrawIcon(tmp, 2);
    break;
  case M_DEFAULTICON:
    str = (char *)safemalloc(strlen((char *)&body[3])+1);
    strcpy(str, (char *)&body[3]);
    FvwmDefaultIcon = str;
    break;
  case M_ICONIFY:
  case M_DEICONIFY:
    if (ready && (tmp = SetFlag(body[0], type)) != NULL)
      RedrawIcon(tmp, 2);
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
    if (CurrentDesk != body[0]){
      CurrentDesk = body[0];
      if (body[0] != 10000 && ready){ /* 10000 is a "magic" number used in FvwmPager */
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
      ConfigureIconWindow(tmp);
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
    AdjustIconWindows();
    XMapWindow(dpy,main_win);
    XMapSubwindows(dpy, main_win);
    XMapWindow(dpy, icon_win);
    mapicons();
    ready = 1;
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
	tmp->flags |= ICONIFIED;
       else
	tmp->flags ^= ICONIFIED;
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
    if (desk_cond(tmp)){
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
    if (desk_cond(tmp) && tmp->IconWin != None)
      AdjustIconWindow(tmp, i++);
    tmp = tmp->next;
  }
  return i;
}

int desk_cond(struct icon_info *item)
{
  if (!(local_flags & CURRENT_ONLY) ||
      (item->flags & STICKY) || (item->desk == CurrentDesk))
    return 1;

  return 0;
}

/************************************************************************
 * AddItem
 *	Skeleton based on AddItem() from FvwmWinList:
 *		Copyright 1994, Mike Finger.
 ***********************************************************************/
Bool AddItem(unsigned long id, long desk, unsigned long flags)
{
  struct icon_info *new, *tmp;
  tmp = Head;

  if (id == main_win || (flags & TRANSIENT) || !(flags & SUPPRESSICON))
    return False;

  while (tmp != NULL){
    if (tmp->id == id ||
	(tmp->wmhints && (tmp->wmhints->flags & IconWindowHint) &&
	 tmp->wmhints->icon_window == id))
      return False;
    tmp = tmp->next;
  }

  new = (struct icon_info *)safemalloc(sizeof(struct icon_info));
  new->name = NULL;
  new->window_name = NULL;
  new->res_class = NULL;
  new->res_name = NULL;
  new->action = NULL;
  new->icon_file = NULL;
  new->icon_w = 0;
  new->icon_h = 0;
  new->IconWin = None;
  new->iconPixmap = None;
  new->icon_maskPixmap = None;
  new->icon_pixmap_w = None;
  new->icon_depth = 0;
  new->desk = desk;
  new->id = id;
  new->extra_flags = DEFAULTICON;
  new->flags = flags | ICON_OURS;
  new->wmhints = NULL;

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
  new->next = NULL;
  if (Tail != NULL)
    Tail->next = new;
  else
  Head = new;
  Tail = new;

  if (desk_cond(new)) num_icons++;

  return True;
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
      if (desk_cond(tmp))
	num_icons--;
    if (Hilite == tmp)
      Hilite = NULL;
    if ((tmp->icon_pixmap_w != None) && (tmp->flags & ICON_OURS))
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
      case M_ICON_NAME:
	if (tmp->name != NULL)
	  free(tmp->name);
	tmp->name = str;
	return tmp;
      case M_ICON_FILE:
	tmp->icon_file = str;
	tmp->extra_flags &= ~DEFAULTICON;
	return tmp;
      case M_WINDOW_NAME:
	if (tmp->window_name != NULL)
	  free(tmp->window_name);
	tmp->window_name = str;
	return tmp;
      case M_RES_CLASS:
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
  ret1 = desk_cond(item1);
  ret2 = desk_cond(item2);
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

  if (!(item->flags & ICON_OURS)){
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
  if (item->wmhints != NULL)
    XFree(item->wmhints);
  if (item->iconPixmap != None)
    XFreePixmap(dpy, item->iconPixmap);
  if (item->icon_maskPixmap != None &&
      (item->wmhints == NULL ||
       !(item->wmhints->flags & (IconPixmapHint|IconWindowHint))))
    XFreePixmap(dpy, item->icon_maskPixmap);

  free(item);
}


/************************************************************************
  IsClick
  	Based on functions.c from Fvwm:
	Copyright 1988, Evans and Sutherland Computer Corporation,
       	Copyright 1989, Massachusetts Institute of Technology,
       	Copyright 1993, Robert Nation.
 ***********************************************************************/
Bool IsClick(int x,int y,unsigned EndMask, XEvent *d)
{
  int xcurrent,ycurrent,total = 0;

  xcurrent = x;
  ycurrent = y;
  while((total < ClickTime)&&
        (x - xcurrent < 5)&&(x - xcurrent > -5)&&
        (y - ycurrent < 5)&&(y - ycurrent > -5))
    {
      usleep(10000);
      total+=10;
      if(XCheckMaskEvent (dpy,EndMask, d))
	return True;
      if(XCheckMaskEvent (dpy,ButtonMotionMask|PointerMotionMask, d))
        {
          xcurrent = d->xmotion.x_root;
          ycurrent = d->xmotion.y_root;
        }
    }
  return False;
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
  /* wait 100 msec, see if the used releases the button */
  if(IsClick(x,y,ButtonReleaseMask,&d))
    {
      type = CLICK;
      ev = &d;
    }

  /* If it was a click, wait to see if its a double click */
  if((type == CLICK) && (IsClick(x,y,ButtonPressMask, &d)))
    {
      type = ONE_AND_A_HALF_CLICKS;
      ev = &d;
    }
  if((type == ONE_AND_A_HALF_CLICKS) &&
     (IsClick(x,y,ButtonReleaseMask, &d)))
    {
      type = DOUBLE_CLICK;
      ev = &d;
    }
  tmp = MouseActions;

  while (tmp != NULL){
    if (tmp->mouse == d.xbutton.button && tmp->type == type){
      SendFvwmPipe(fd, tmp->action, item->id);
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
      SendFvwmPipe(fd, tmp->action, item->id);
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
      item->icon_file = FvwmDefaultIcon;
    return 1;
  }


  for (nptr = IconListHead; nptr != NULL; nptr = nptr->next){
    if (nptr == DefaultIcon)
      isdefault = 1;

    if (matchWildcards(nptr->name, item->res_class) == TRUE){
      value = nptr->iconfile;
      if (nptr != DefaultIcon)
	isdefault = 0;
    }

    if (matchWildcards(nptr->name, item->res_name) == TRUE){
      value = nptr->iconfile;
      if (nptr != DefaultIcon)
	isdefault = 0;
    }

    if (matchWildcards(nptr->name, item->window_name) == TRUE){
      value = nptr->iconfile;
      if (nptr != DefaultIcon)
	isdefault = 0;
    }
  }

  if (!isdefault){
    item->icon_file = value;
    item->extra_flags &= ~DEFAULTICON;
  }else if ((item->extra_flags & DEFAULTICON)){
    if (DefaultIcon != NULL)
      item->icon_file = DefaultIcon->iconfile;
    else if (FvwmDefaultIcon != NULL)
      item->icon_file = FvwmDefaultIcon;
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
XErrorHandler myErrorHandler(Display *dpy, XErrorEvent *event)
{
  char msg[256];

  if (event->error_code == BadWindow)
    return 0;

  XGetErrorText(dpy, event->error_code, msg, 256);

  fprintf(stderr, "Error in %s:  %s \n", MyName, msg);
  fprintf(stderr, "Major opcode of failed request:  %d \n",
	  event->request_code);
  fprintf(stderr, "Resource id of failed request:  0x%lx \n",
	  event->resourceid);
  return 0;
}
