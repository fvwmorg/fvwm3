/* -*-c-*- */
/*
 * This module is all new
 * by Rob Nation
 */

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/*
 *
 * fvwm pager handling code
 *
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/keysym.h>

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/Module.h"
#include "libs/Colorset.h"
#include "libs/Graphics.h"
#include "fvwm/fvwm.h"
#include "libs/PictureGraphics.h"
#include "FvwmPager.h"


extern ScreenInfo Scr;
extern Display *dpy;

Pixel back_pix, fore_pix, hi_pix;
Pixel focus_pix;
Pixel focus_fore_pix;
extern int windowcolorset, activecolorset;
extern Pixel win_back_pix, win_fore_pix, win_hi_back_pix, win_hi_fore_pix;
extern Bool win_pix_set, win_hi_pix_set;
extern int window_w, window_h,window_x,window_y,usposition,uselabel,xneg,yneg;
extern int StartIconic;
extern int MiniIcons;
extern int LabelsBelow;
extern int ShapeLabels;
extern int ShowBalloons, ShowPagerBalloons, ShowIconBalloons;
extern char *BalloonFormatString;
extern char *WindowLabelFormat;
extern char *PagerFore, *PagerBack, *HilightC;
extern char *BalloonFore, *BalloonBack, *BalloonBorderColor;
extern Window BalloonView;
extern unsigned int WindowBorderWidth;
extern unsigned int MinSize;
extern Bool WindowBorders3d;
extern Bool UseSkipList;
extern FvwmPicture *PixmapBack;
extern FvwmPicture *HilightPixmap;
extern int HilightDesks;
extern char fAlwaysCurrentDesk;

extern int MoveThreshold;

extern int icon_w, icon_h, icon_x, icon_y, icon_xneg, icon_yneg;
FlocaleFont *Ffont, *FwindowFont;
FlocaleWinString *FwinString;

extern PagerWindow *Start;
extern PagerWindow *FocusWin;
static Atom wm_del_win;

extern char *MyName;

extern int desk1, desk2, ndesks;
extern int Rows,Columns;
extern int fd[2];

int desk_w = 0;
int desk_h = 0;
int label_h = 0;

DeskInfo *Desks;
int Wait = 0;
int FvwmErrorHandler(Display *, XErrorEvent *);
extern Bool is_transient;
extern Bool do_ignore_next_button_release;
extern Bool use_dashed_separators;
extern Bool use_no_separators;


/* assorted gray bitmaps for decorative borders */
#define g_width 2
#define g_height 2
static char g_bits[] = {0x02, 0x01};

#define l_g_width 4
#define l_g_height 2
static char l_g_bits[] = {0x08, 0x02};

#define s_g_width 4
#define s_g_height 4
static char s_g_bits[] = {0x01, 0x02, 0x04, 0x08};

#define label_border g_width

Window icon_win;	       /* icon window */

static int MyVx, MyVy;		/* copy of Scr.Vx/y for drag logic */

static char *GetBalloonLabel(const PagerWindow *pw,const char *fmt);
extern void ExitPager(void);

Pixmap default_pixmap = None;

#ifdef DEBUG
#define MYFPRINTF(X) \
  fprintf X;\
  fflush (stderr);
#else
#define MYFPRINTF(X)
#endif


#define  MAX_UNPROCESSED_MESSAGES 1
/* sums up pixels to scroll. If do_send_message is True a Scroll command is
 * sent back to fvwm. The function shall be called with is_message_recieved
 * True when the Scroll command has been processed by fvwm. This is checked
 * by talking to ourself. */
static void do_scroll(int sx, int sy, Bool do_send_message,
		      Bool is_message_recieved)
{
	static int psx = 0;
	static int psy = 0;
	static int messages_sent = 0;
	char command[256];
	psx+=sx;
	psy+=sy;
	if (is_message_recieved)
	{
		/* There might be other modules with the same name, or someone
		 might send ScrollDone other than the module, so just treat
		 any negative count as zero. */
		if (--messages_sent < 0)
		{
			messages_sent = 0;
		}
	}
	if ((do_send_message || messages_sent < MAX_UNPROCESSED_MESSAGES) &&
	    ( psx != 0 || psy != 0 ))
	{
		sprintf(command, "Scroll %dp %dp", psx, psy);
		SendText(fd, command, 0);
		messages_sent++;
		SendText(fd, "Send_Reply ScrollDone", 0);
		psx = 0;
		psy = 0;
	}
}

void HandleScrollDone(void)
{
	do_scroll(0, 0, True, True);
}

typedef struct
{
	int event_type;
	XEvent *ret_last_event;
} _weed_window_events_args;

static int _pred_weed_window_events(
	Display *display, XEvent *current_event, XPointer arg)
{
	_weed_window_events_args *args = (_weed_window_events_args *)arg;

	if (current_event->type == args->event_type)
	{
		if (args->ret_last_event != NULL)
		{
			*args->ret_last_event = *current_event;
		}

		return 1;
	}

	return 0;
}

/* discard certain events on a window */
static void discard_events(long event_type, Window w, XEvent *last_ev)
{
	_weed_window_events_args args;

	XSync(dpy, 0);
	args.event_type = event_type;
	args.ret_last_event = last_ev;
	FWeedIfWindowEvents(dpy, w, _pred_weed_window_events, (XPointer)&args);

	return;
}

/*
 *
 *  Procedure:
 *	CalcGeom - calculates the size and position of a mini-window
 *	given the real window size.
 *	You can always tell bad code by the size of the comments.
 */
static void CalcGeom(PagerWindow *t, int win_w, int win_h,
		     int *x_ret, int *y_ret, int *w_ret, int *h_ret)
{
  int virt, virt2, edge, edge2, size, page, over;

  /* coordinate of left hand edge on virtual desktop */
  virt = Scr.Vx + t->x;

  /* position of left hand edge of mini-window on pager window */
  edge = (virt * win_w) / Scr.VWidth;

  /* absolute coordinate of right hand edge on virtual desktop */
  virt += t->width - 1;

  /* to calculate the right edge, mirror the window and use the same
   * calculations as for the left edge for consistency. */
  virt2 = Scr.VWidth - 1 - virt;
  edge2 = (virt2 * win_w) / Scr.VWidth;

  /* then mirror it back to get the real coordinate */
  edge2 = win_w - 1 - edge2;

  /* Calculate the mini-window's width by subtracting its LHS
   * from its RHS. This theoretically means that the width will
   * vary slightly as the window travels around the screen, but
   * this way ensures that the mini-windows in the pager match
   * the actual screen layout. */
  size = edge2 - edge + 1;

  /* Make size big enough to be visible */
  if (size < MinSize) {
    size = MinSize;
    /* this mini-window has to be grown to be visible
     * which way it grows depends on some magic:
     * normally it will grow right but if the window is on the right hand
     * edge of a page it should be grown left so that the pager looks better */

    /* work out the page that the right hand edge is on */
    page = virt / Scr.MyDisplayWidth;

    /* if the left edge is on the same page then possibly move it left */
    if (page == ((virt - t->width + 1) / Scr.MyDisplayWidth)) {
      /* calculate how far the mini-window right edge overlaps the page line */
      /* beware that the "over" is actually one greater than on screen, but
	 this discrepancy is catered for in the next two lines */
      over = edge + size - ((page + 1) * win_w * Scr.MyDisplayWidth) /
	Scr.VWidth;

      /* if the mini-window right edge is beyond the mini-window pager grid */
      if (over > 0) {
	/* move it left by the amount of pager grid overlap (!== the growth) */
	edge -= over;
      }
    }
  }
  /* fill in return values */
  *x_ret = edge;
  *w_ret = size;

  /* same code for y axis */
  virt = Scr.Vy + t->y;
  edge = (virt * win_h) / Scr.VHeight;
  virt += t->height - 1;
  virt2 = Scr.VHeight - 1 - virt;
  edge2 = (virt2 * win_h) / Scr.VHeight;
  edge2 = win_h - 1 - edge2;
  size = edge2 - edge + 1;
  if (size < MinSize)
  {
    size = MinSize;
    page = virt / Scr.MyDisplayHeight;
    if (page == ((virt - t->height + 1) / Scr.MyDisplayHeight)) {
      over = edge + size - ((page + 1) * win_h * Scr.MyDisplayHeight) /
	Scr.VHeight;
      if (over > 0)
	edge -= over;
    }
  }
  *y_ret = edge;
  *h_ret = size;
}

/*
 *
 *  Procedure:
 *	Initialize_viz_pager - creates a temp window of the correct visual
 *	so that pixmaps may be created for use with the main window
 */
void initialize_viz_pager(void)
{
	XSetWindowAttributes attr;
	XGCValues xgcv;

	/* FIXME: I think that we only need that Pdepth ==
	 * DefaultDepth(dpy, Scr.screen) to use the Scr.Root for Scr.Pager_w */
	if (Pdefault)
	{
		Scr.Pager_w = Scr.Root;
	}
	else
	{
		attr.background_pixmap = None;
		attr.border_pixel = 0;
		attr.colormap = Pcmap;
		Scr.Pager_w = XCreateWindow(
			dpy, Scr.Root, -10, -10, 10, 10, 0, Pdepth,
			InputOutput, Pvisual,
			CWBackPixmap|CWBorderPixel|CWColormap, &attr);
		Scr.NormalGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, 0, &xgcv);
	}
	Scr.NormalGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, 0, NULL);

	xgcv.plane_mask = AllPlanes;
	Scr.MiniIconGC = fvwmlib_XCreateGC(
		dpy, Scr.Pager_w, GCPlaneMask, &xgcv);
	Scr.black = GetColor("Black");

	/* Transparent background are only allowed when the depth matched the
	 * root */
	if (Pdepth == DefaultDepth(dpy, Scr.screen))
	{
		default_pixmap = ParentRelative;
	}

	return;
}

/* see also change colorset */
void draw_desk_background(int i, int page_w, int page_h)
{
	if (Desks[i].colorset > -1)
	{
		XSetWindowBorder(
			dpy, Desks[i].title_w, Colorset[Desks[i].colorset].fg);
		XSetWindowBorder(
			dpy, Desks[i].w, Colorset[Desks[i].colorset].fg);
		XSetForeground(
			dpy, Desks[i].NormalGC,Colorset[Desks[i].colorset].fg);
		XSetForeground(
			dpy, Desks[i].DashedGC,Colorset[Desks[i].colorset].fg);
		if (uselabel)
		{
			if (CSET_IS_TRANSPARENT(Desks[i].colorset))
			{
					SetWindowBackground(
						dpy, Desks[i].title_w,
						desk_w, label_h,
						&Colorset[Desks[i].colorset],
						Pdepth,
						Scr.NormalGC, True);
			}
			else
			{
				SetWindowBackground(
					dpy, Desks[i].title_w, desk_w,
					desk_h + label_h,
					&Colorset[Desks[i].colorset], Pdepth,
					Scr.NormalGC, True);
			}
		}
		if (label_h != 0 && uselabel && !LabelsBelow &&
		    !CSET_IS_TRANSPARENT(Desks[i].colorset))
		{
			SetWindowBackgroundWithOffset(
				dpy, Desks[i].w, 0, -label_h, desk_w,
				desk_h + label_h, &Colorset[Desks[i].colorset],
				Pdepth, Scr.NormalGC, True);
		}
		else
		{
			if (CSET_IS_TRANSPARENT(Desks[i].colorset))
			{
				SetWindowBackground(
					dpy, Desks[i].w, desk_w, desk_h,
					&Colorset[Desks[i].colorset], Pdepth,
					Scr.NormalGC, True);
			}
			else
			{
				SetWindowBackground(
					dpy, Desks[i].w, desk_w,
					desk_h + label_h,
					&Colorset[Desks[i].colorset],
					Pdepth, Scr.NormalGC, True);
			}
		}
	}
	XClearArea(dpy,Desks[i].w, 0, 0, 0, 0,True);
	if (Desks[i].highcolorset > -1)
	{
		XSetForeground(
			dpy, Desks[i].HiliteGC,
			Colorset[Desks[i].highcolorset].bg);
		XSetForeground(
			dpy, Desks[i].rvGC, Colorset[Desks[i].highcolorset].fg);
		if (HilightDesks)
		{
			SetWindowBackground(
				dpy, Desks[i].CPagerWin, page_w, page_h,
				&Colorset[Desks[i].highcolorset], Pdepth,
				Scr.NormalGC, True);
		}
	}
	if (uselabel)
	{
		XClearArea(dpy,Desks[i].title_w, 0, 0, 0, 0,True);
	}

	return;
}

/*
 *
 *  Procedure:
 *	Initialize_pager - creates the pager window, if needed
 *
 *  Inputs:
 *	x,y location of the window
 *
 */
char *pager_name = "Fvwm Pager";
XSizeHints sizehints =
{
  (PMinSize | PResizeInc | PBaseSize | PWinGravity),
  0, 0, 100, 100,			/* x, y, width and height */
  1, 1,					/* Min width and height */
  0, 0,					/* Max width and height */
  1, 1,					/* Width and height increments */
  {0, 0}, {0, 0},			/* Aspect ratio - not used */
  1, 1,					/* base size */
  (NorthWestGravity)			/* gravity */
};

void initialize_balloon_window(void)
{
	XGCValues xgcv;
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	extern int BalloonBorderWidth;

	/* create balloon window
	   -- ric@giccs.georgetown.edu */
	if (!ShowBalloons)
	{
		Scr.balloon_w = None;
		return;
	}
	valuemask = CWOverrideRedirect | CWEventMask | CWColormap;
	/* tell WM to ignore this window */
	attributes.override_redirect = True;
	attributes.event_mask = ExposureMask;
	attributes.colormap = Pcmap;
	/* now create the window */
	Scr.balloon_w = XCreateWindow(
		dpy, Scr.Root, 0, 0, /* coords set later */ 1, 1,
		BalloonBorderWidth, Pdepth, InputOutput, Pvisual, valuemask,
		&attributes);
	Scr.balloon_gc = fvwmlib_XCreateGC(dpy, Scr.balloon_w, 0, &xgcv);
	/* Make sure we don't get balloons initially with the Icon option. */
	ShowBalloons = ShowPagerBalloons;

	return;
}

void initialize_pager(void)
{
  XWMHints wmhints;
  XClassHint class1;
  XTextProperty name;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  extern char *WindowBack, *WindowFore, *WindowHiBack, *WindowHiFore;
  extern char *BalloonFont;
  extern char *font_string, *smallFont;
  int n,m,w,h,i,x,y;
  XGCValues gcv;
  char dash_list[2];
  FlocaleFont *balloon_font;

  /* I don't think that this is necessary - just let pager die */
  /* domivogt (07-mar-1999): But it is! A window being moved in the pager
   * might die at any moment causing the Xlib calls to generate BadMatch
   * errors. Without an error handler the pager will die! */
  XSetErrorHandler(FvwmErrorHandler);

  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);

  /* load the font */
  /* Note: "font" is always created, whether labels are used or not
     because a GC below is set to use a font. dje Dec 2001.
     OK, I fixed the GC below, but now something else is blowing up.
     Right now, I've got to do some Real Life stuff, so this kludge is
     in place, its still better than I found it.
     I hope that I've fixed this (olicha)
  */
  Ffont = FlocaleLoadFont(dpy, font_string, MyName);

  label_h = (uselabel) ? Ffont->height + 2 : 0;

  /* init our Flocale window string */
  FlocaleAllocateWinString(&FwinString);

  /* Check that shape extension exists. */
  if (FHaveShapeExtension && ShapeLabels)
  {
    ShapeLabels = (FShapesSupported) ? 1 : 0;
  }

  if(smallFont != NULL)
  {
    FwindowFont = FlocaleLoadFont(dpy, smallFont, MyName);
  }

  /* Load the colors */
  fore_pix = GetColor(PagerFore);
  back_pix = GetColor(PagerBack);
  hi_pix = GetColor(HilightC);

  if (windowcolorset >= 0)
  {
    win_back_pix = Colorset[windowcolorset].bg;
    win_fore_pix = Colorset[windowcolorset].fg;
    win_pix_set = True;
  }
  else if (WindowBack && WindowFore)
  {
    win_back_pix = GetColor(WindowBack);
    win_fore_pix = GetColor(WindowFore);
    win_pix_set = True;
  }

  if (activecolorset >= 0)
  {
    win_hi_back_pix = Colorset[activecolorset].bg;
    win_hi_fore_pix = Colorset[activecolorset].fg;
    win_hi_pix_set = True;
  }
  else if (WindowHiBack && WindowHiFore)
  {
    win_hi_back_pix = GetColor(WindowHiBack);
    win_hi_fore_pix = GetColor(WindowHiFore);
    win_hi_pix_set = True;
  }

  /* Load pixmaps for mono use */
  if(Pdepth<2)
  {
    Scr.gray_pixmap =
      XCreatePixmapFromBitmapData(dpy,Scr.Pager_w,g_bits, g_width,g_height,
				  fore_pix,back_pix,Pdepth);
    Scr.light_gray_pixmap =
      XCreatePixmapFromBitmapData(dpy,Scr.Pager_w,l_g_bits,l_g_width,
				  l_g_height,
				  fore_pix,back_pix,Pdepth);
    Scr.sticky_gray_pixmap =
      XCreatePixmapFromBitmapData(dpy,Scr.Pager_w,s_g_bits,s_g_width,
				  s_g_height,
				  fore_pix,back_pix,Pdepth);
  }


  n = Scr.VxMax / Scr.MyDisplayWidth;
  m = Scr.VyMax / Scr.MyDisplayHeight;

  /* Size the window */
  if(Rows < 0)
  {
    if(Columns < 0)
    {
      Columns = ndesks;
      Rows = 1;
    }
    else
    {
      Rows = ndesks/Columns;
      if(Rows*Columns < ndesks)
	Rows++;
    }
  }
  if(Columns < 0)
  {
    if (Rows == 0)
      Rows = 1;
    Columns = ndesks/Rows;
    if(Rows*Columns < ndesks)
      Columns++;
  }

  if(Rows*Columns < ndesks)
  {
    if (Columns == 0)
      Columns = 1;
    Rows = ndesks/Columns;
    if (Rows*Columns < ndesks)
      Rows++;
  }

  sizehints.width_inc = Columns*(n+1);
  sizehints.height_inc = Rows*(m+1);
  sizehints.base_width = Columns * n + Columns - 1;
  sizehints.base_height = Rows * (m + label_h + 1) - 1;
  if (window_w > 0)
  {
    window_w = (window_w - sizehints.base_width) / sizehints.width_inc;
    window_w = window_w * sizehints.width_inc + sizehints.base_width;
  }
  else
  {
    window_w = Columns * (Scr.VWidth / Scr.VScale + n) + Columns - 1;
  }
  if (window_h > 0)
  {
    window_h = (window_h - sizehints.base_height) / sizehints.height_inc;
    window_h = window_h * sizehints.height_inc + sizehints.base_height;
  }
  else
  {
    window_h = Rows * (Scr.VHeight / Scr.VScale + m + label_h + 1) - 1;
  }
  desk_w = (window_w - Columns + 1) / Columns;
  desk_h = (window_h - Rows * label_h - Rows + 1) / Rows;
  if (is_transient)
  {
    rectangle screen_g;
    fscreen_scr_arg fscr;

    fscr.xypos.x = window_x;
    fscr.xypos.y = window_y;
    FScreenGetScrRect(
      &fscr, FSCREEN_XYPOS,
      &screen_g.x, &screen_g.y, &screen_g.width, &screen_g.height);
    if (window_w + window_x > screen_g.x + screen_g.width)
    {
      window_x = screen_g.x + screen_g.width - Scr.MyDisplayWidth;
      xneg = 1;
    }
    if (window_h + window_y > screen_g.y + screen_g.height)
    {
      window_y = screen_g.y + screen_g.height - Scr.MyDisplayHeight;
      yneg = 1;
    }
  }
  if (xneg)
  {
    sizehints.win_gravity = NorthEastGravity;
    window_x = Scr.MyDisplayWidth - window_w + window_x;
  }
  if (yneg)
  {
    window_y = Scr.MyDisplayHeight - window_h + window_y;
    if(sizehints.win_gravity == NorthEastGravity)
      sizehints.win_gravity = SouthEastGravity;
    else
      sizehints.win_gravity = SouthWestGravity;
  }
  sizehints.width = window_w;
  sizehints.height = window_h;

  if(usposition)
    sizehints.flags |= USPosition;

  valuemask = (CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask);
  attributes.background_pixmap = default_pixmap;
  attributes.border_pixel = 0;
  attributes.colormap = Pcmap;
  attributes.event_mask = (StructureNotifyMask);

  /* destroy the temp window first, don't worry if it's the Root */
  if (Scr.Pager_w != Scr.Root)
    XDestroyWindow(dpy, Scr.Pager_w);
  Scr.Pager_w = XCreateWindow (dpy, Scr.Root, window_x, window_y, window_w,
			       window_h, 0, Pdepth, InputOutput, Pvisual,
			       valuemask, &attributes);
  XSetWMProtocols(dpy,Scr.Pager_w,&wm_del_win,1);
  /* hack to prevent mapping on wrong screen with StartsOnScreen */
  FScreenMangleScreenIntoUSPosHints(FSCREEN_XYPOS, &sizehints);
  XSetWMNormalHints(dpy,Scr.Pager_w,&sizehints);
  if (is_transient)
  {
    XSetTransientForHint(dpy, Scr.Pager_w, Scr.Root);
  }

  if((desk1==desk2)&&(Desks[0].label != NULL))
  {
    if (FlocaleTextListToTextProperty(
	 dpy, &Desks[0].label, 1, XStdICCTextStyle, &name) == 0)
    {
      fprintf(stderr,"%s: fatal error: cannot allocate desk name", MyName);
      exit(0);
    }
  }
  else
  {
    if (FlocaleTextListToTextProperty(
	 dpy, &Desks[0].label, 1, XStdICCTextStyle, &name) == 0)
    {
      fprintf(stderr,"%s: fatal error: cannot allocate pager name", MyName);
      exit(0);
    }
  }

  attributes.event_mask = (StructureNotifyMask| ExposureMask);
  if(icon_w < 1)
    icon_w = (window_w - Columns+1)/Columns;
  if(icon_h < 1)
    icon_h = (window_h - Rows* label_h - Rows + 1)/Rows;

  icon_w = (icon_w / (n+1)) *(n+1)+n;
  icon_h = (icon_h / (m+1)) *(m+1)+m;
  icon_win = XCreateWindow (dpy, Scr.Root, window_x, window_y, icon_w, icon_h,
			    0, Pdepth, InputOutput, Pvisual, valuemask,
			    &attributes);
  XGrabButton(dpy, 1, AnyModifier, icon_win,
	      True, ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
	      GrabModeAsync, GrabModeAsync, None,
	      None);
  XGrabButton(dpy, 2, AnyModifier, icon_win,
	      True, ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
	      GrabModeAsync, GrabModeAsync, None,
	      None);
  XGrabButton(dpy, 3, AnyModifier, icon_win,
	      True, ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
	      GrabModeAsync, GrabModeAsync, None,
	      None);

  if(!StartIconic)
    wmhints.initial_state = NormalState;
  else
    wmhints.initial_state = IconicState;
  wmhints.flags = 0;
  if (icon_x != -10000)
  {
    if (icon_xneg)
      icon_x = Scr.MyDisplayWidth + icon_x - icon_w;
    if (icon_y != -10000)
    {
      if (icon_yneg)
	icon_y = Scr.MyDisplayHeight + icon_y - icon_h;
    }
    else
    {
      icon_y = 0;
    }
    icon_xneg = 0;
    icon_yneg = 0;
    wmhints.icon_x = icon_x;
    wmhints.icon_y = icon_y;
    wmhints.flags = IconPositionHint;
  }
  wmhints.icon_window = icon_win;
  wmhints.input = False;
  wmhints.flags |= InputHint | StateHint | IconWindowHint;

  class1.res_name = MyName;
  class1.res_class = "FvwmPager";

  XSetWMProperties(dpy,Scr.Pager_w,&name,&name,NULL,0,
		   &sizehints,&wmhints,&class1);
  XFree((char *)name.value);

  /* change colour/font for labelling mini-windows */
  XSetForeground(dpy, Scr.NormalGC, focus_fore_pix);

  if (FwindowFont != NULL && FwindowFont->font != NULL)
    XSetFont(dpy, Scr.NormalGC, FwindowFont->font->fid);

  /* create the 3d bevel GC's if necessary */
  if (windowcolorset >= 0) {
    gcv.foreground = Colorset[windowcolorset].hilite;
    Scr.whGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
    gcv.foreground = Colorset[windowcolorset].shadow;
    Scr.wsGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
  }
  if (activecolorset >= 0) {
    gcv.foreground = Colorset[activecolorset].hilite;
    Scr.ahGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
    gcv.foreground = Colorset[activecolorset].shadow;
    Scr.asGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
  }

  balloon_font = FlocaleLoadFont(dpy, BalloonFont, MyName);
  for(i=0;i<ndesks;i++)
  {
    w = window_w / Columns;
    h = window_h / Rows;
    x = (w + 1) * (i % Columns);
    y = (h + 1) * (i / Columns);

    /* create the GC for desk labels */
    gcv.foreground = (Desks[i].colorset < 0) ? fore_pix
      : Colorset[Desks[i].colorset].fg;
    if (uselabel && Ffont && Ffont->font) {
      gcv.font = Ffont->font->fid;
      Desks[i].NormalGC =
	fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground | GCFont, &gcv);
    } else {
      Desks[i].NormalGC =
	fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
    }
    /* create the active desk hilite GC */
    if(Pdepth < 2)
      gcv.foreground = fore_pix;
    else
      gcv.foreground = (Desks[i].highcolorset < 0) ? hi_pix
	: Colorset[Desks[i].highcolorset].bg;
    Desks[i].HiliteGC = fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);

    /* create the hilight desk title drawing GC */
    if ((Pdepth < 2) || (fore_pix == hi_pix))
      gcv.foreground = (Desks[i].highcolorset < 0) ? back_pix
	: Colorset[Desks[i].highcolorset].fg;
    else
      gcv.foreground = (Desks[i].highcolorset < 0) ? fore_pix
	: Colorset[Desks[i].highcolorset].fg;

    if (uselabel && Ffont && Ffont->font) {
      Desks[i].rvGC =
	fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground | GCFont, &gcv);
    } else {
      Desks[i].rvGC =
	fvwmlib_XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
    }
    /* create the virtual page boundary GC */
    gcv.foreground = (Desks[i].colorset < 0) ? fore_pix
      : Colorset[Desks[i].colorset].fg;
    gcv.line_width = 1;
    gcv.line_style = (use_dashed_separators) ? LineOnOffDash : LineSolid;
    Desks[i].DashedGC =
      fvwmlib_XCreateGC(
	dpy, Scr.Pager_w, GCForeground | GCLineStyle | GCLineWidth, &gcv);
    if (use_dashed_separators)
    {
      /* Although this should already be the default for a freshly created GC,
       * some X servers do not draw properly dashed lines if the dash style is
       * not set explicitly. */
      dash_list[0] = 4;
      dash_list[1] = 4;
      XSetDashes(dpy, Desks[i].DashedGC, 0, dash_list, 2);
    }

    valuemask = (CWBorderPixel | CWColormap | CWEventMask);
    if (Desks[i].colorset >= 0 && Colorset[Desks[i].colorset].pixmap)
    {
	valuemask |= CWBackPixmap;
	attributes.background_pixmap = Colorset[Desks[i].colorset].pixmap;
    }
    else
    {
	valuemask |= CWBackPixel;
	attributes.background_pixel = (Desks[i].colorset < 0) ?
	    (Desks[i].Dcolor ? GetColor(Desks[i].Dcolor) : back_pix)
	    : Colorset[Desks[i].colorset].bg;
    }

    attributes.border_pixel = (Desks[i].colorset < 0) ? fore_pix
      : Colorset[Desks[i].colorset].fg;
    attributes.event_mask = (ExposureMask | ButtonReleaseMask);
    Desks[i].title_w = XCreateWindow(
      dpy, Scr.Pager_w, x - 1, y - 1, w, h, 1, CopyFromParent, InputOutput,
      CopyFromParent, valuemask, &attributes);
    attributes.event_mask = (ExposureMask | ButtonReleaseMask |
			     ButtonPressMask |ButtonMotionMask);
    /* or just: desk_h = h - label_h; */
    desk_h = (window_h - Rows * label_h - Rows + 1) / Rows;

    valuemask &= ~(CWBackPixel);


    if (Desks[i].colorset > -1 &&
	Colorset[Desks[i].colorset].pixmap)
    {
      valuemask |= CWBackPixmap;
      attributes.background_pixmap = None; /* set later */
    }
    else if (Desks[i].bgPixmap)
    {
      valuemask |= CWBackPixmap;
      attributes.background_pixmap = Desks[i].bgPixmap->picture;
    }
    else if (Desks[i].Dcolor)
    {
      valuemask |= CWBackPixel;
      attributes.background_pixel = GetColor(Desks[i].Dcolor);
    }
    else if (PixmapBack)
    {
      valuemask |= CWBackPixmap;
      attributes.background_pixmap = PixmapBack->picture;
    }
    else
    {
      valuemask |= CWBackPixel;
      attributes.background_pixel = (Desks[i].colorset < 0) ? back_pix
	: Colorset[Desks[i].colorset].bg;
    }

    Desks[i].w = XCreateWindow(
	    dpy, Desks[i].title_w, x - 1, LabelsBelow ? -1 : label_h - 1, w, desk_h,
      1, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);

    if (HilightDesks)
    {
      valuemask &= ~(CWBackPixel | CWBackPixmap);

      attributes.event_mask = 0;

      if (Desks[i].highcolorset > -1 &&
	  Colorset[Desks[i].highcolorset].pixmap)
      {
	valuemask |= CWBackPixmap;
	attributes.background_pixmap = None; /* set later */
      }
      else if (HilightPixmap)
      {
	valuemask |= CWBackPixmap;
	attributes.background_pixmap = HilightPixmap->picture;
      }
      else
      {
	valuemask |= CWBackPixel;
	attributes.background_pixel = (Desks[i].highcolorset < 0) ? hi_pix
	  : Colorset[Desks[i].highcolorset].bg;
      }

      w = w / (n + 1);
      h = desk_h / (m + 1);
      Desks[i].CPagerWin=XCreateWindow(dpy, Desks[i].w, -32768, -32768, w, h, 0,
				       CopyFromParent, InputOutput,
				       CopyFromParent, valuemask, &attributes);
      draw_desk_background(i, w, h);
      XMapRaised(dpy,Desks[i].CPagerWin);
    }
    else
    {
      draw_desk_background(i, 0, 0);
    }

    XMapRaised(dpy,Desks[i].w);
    XMapRaised(dpy,Desks[i].title_w);

    /* get font for balloon */
    Desks[i].balloon.Ffont = balloon_font;
    if (Desks[i].balloon.Ffont == NULL)
    {
      fprintf(stderr, "%s: No fonts available, giving up!.\n", MyName);
    }
    Desks[i].balloon.height = Desks[i].balloon.Ffont->height + 1;
  }
  initialize_balloon_window();
  XMapRaised(dpy,Scr.Pager_w);
}


void UpdateWindowShape(void)
{
  if (FHaveShapeExtension)
  {
    int i, j, cnt, shape_count, x_pos, y_pos;
    XRectangle *shape;

    if (!ShapeLabels || !uselabel || label_h<=0)
      return;

    shape_count =
      ndesks + ((Scr.CurrentDesk < desk1 || Scr.CurrentDesk >desk2) ? 0 : 1);

    shape = (XRectangle *)alloca (shape_count * sizeof (XRectangle));

    if (shape == NULL)
      return;

    cnt = 0;
    y_pos = (LabelsBelow ? 0 : label_h);

    for (i = 0; i < Rows; ++i)
    {
      x_pos = 0;
      for (j = 0; j < Columns; ++j)
      {
	if (cnt < ndesks)
	{
	  shape[cnt].x = x_pos;
	  shape[cnt].y = y_pos;
	  shape[cnt].width = desk_w + 1;
	  shape[cnt].height = desk_h + 2;

	  if (cnt == Scr.CurrentDesk - desk1)
	  {
	    shape[ndesks].x = x_pos;
	    shape[ndesks].y =
	      (LabelsBelow ? y_pos + desk_h + 2 : y_pos - label_h);
	    shape[ndesks].width = desk_w;
	    shape[ndesks].height = label_h + 2;
	  }
	}
	++cnt;
	x_pos += desk_w + 1;
      }
      y_pos += desk_h + 2 + label_h;
    }

    FShapeCombineRectangles(
      dpy, Scr.Pager_w, FShapeBounding, 0, 0, shape, shape_count, FShapeSet, 0);
  }
}



/*
 *
 * Decide what to do about received X events
 *
 */
void DispatchEvent(XEvent *Event)
{
  int i,x,y;
  Window JunkRoot, JunkChild;
  Window w;
  int JunkX, JunkY;
  unsigned JunkMask;
  char keychar;
  KeySym keysym;
  Bool do_move_page = False;
  short dx = 0;
  short dy = 0;

  switch(Event->xany.type)
  {
  case EnterNotify:
    HandleEnterNotify(Event);
    break;
  case LeaveNotify:
    if ( ShowBalloons )
      UnmapBalloonWindow();
    break;
  case ConfigureNotify:
    fev_sanitise_configure_notify(&Event->xconfigure);
    w = Event->xconfigure.window;
    discard_events(ConfigureNotify, Event->xconfigure.window, Event);
    fev_sanitise_configure_notify(&Event->xconfigure);
    if (w != icon_win)
    {
      /* icon_win is not handled here */
      discard_events(Expose, w, NULL);
      ReConfigure();
    }
    break;
  case Expose:
    HandleExpose(Event);
    break;
  case KeyPress:
    if (is_transient)
    {
      XLookupString(&(Event->xkey), &keychar, 1, &keysym, NULL);
      switch(keysym)
      {
      case XK_Up:
	dy = -100;
	do_move_page = True;
	break;
      case XK_Down:
	dy = 100;
	do_move_page = True;
	break;
      case XK_Left:
	dx = -100;
	do_move_page = True;
	break;
      case XK_Right:
	dx = 100;
	do_move_page = True;
	break;
      default:
	/* does not return */
	ExitPager();
	break;
      }
      if (do_move_page)
      {
	char command[64];
	sprintf(command,"Scroll %d %d", dx, dy);
	SendText(fd, command, 0);
      }
    }
    break;
  case ButtonRelease:
    if (do_ignore_next_button_release)
    {
      do_ignore_next_button_release = False;
      break;
    }
    if (Event->xbutton.button == 3)
    {
      for(i=0;i<ndesks;i++)
      {
	if(Event->xany.window == Desks[i].w)
	{
	  if (FQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	  {
	    /* pointer is on a different screen - that's okay here */
	  }
	  Scroll(desk_w, desk_h, x, y, i, False);
	}
      }
      if(Event->xany.window == icon_win)
      {
	if (FQueryPointer(dpy, icon_win, &JunkRoot, &JunkChild,
			  &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	{
	  /* pointer is on a different screen - that's okay here */
	}
	Scroll(icon_w, icon_h, x, y, 0, True);
      }
      /* Flush any pending scroll operations */
      do_scroll(0, 0, True, False);
    }
    else if((Event->xbutton.button == 1)||
	    (Event->xbutton.button == 2))
    {
      for(i=0;i<ndesks;i++)
      {
	if(Event->xany.window == Desks[i].w)
	  SwitchToDeskAndPage(i,Event);
	else if(Event->xany.window == Desks[i].title_w)
	  SwitchToDesk(i);
      }
      if(Event->xany.window == icon_win)
      {
	IconSwitchPage(Event);
      }
    }
    if (is_transient)
    {
      /* does not return */
      ExitPager();
    }
    break;
  case ButtonPress:
    do_ignore_next_button_release = False;
    if ( ShowBalloons )
      UnmapBalloonWindow();
    if (((Event->xbutton.button == 2)||
	 ((Event->xbutton.button == 3)&&
	  (Event->xbutton.state & Mod1Mask)))&&
	(Event->xbutton.subwindow != None))
    {
      MoveWindow(Event);
    }
    else if (Event->xbutton.button == 3)
    {
      /* save initial virtual desk position for drag */
      MyVx=Scr.Vx;
      MyVy=Scr.Vy;
      for(i=0;i<ndesks;i++)
      {
	if(Event->xany.window == Desks[i].w)
	{
	  if (FQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	  {
	    /* pointer is on a different screen - that's okay here */
	  }
	  Scroll(desk_w, desk_h, x, y, Scr.CurrentDesk, False);
	  if (Scr.CurrentDesk != i + desk1)
	  {
	    Wait = 0;
	    SwitchToDesk(i);
	  }
	  break;
	}
      }
      if(Event->xany.window == icon_win)
      {
	if (FQueryPointer(dpy, icon_win, &JunkRoot, &JunkChild,
			  &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	{
	  /* pointer is on a different screen - that's okay here */
	}
	Scroll(icon_w, icon_h, x, y, 0, True);
      }
    }
    break;
  case MotionNotify:
    do_ignore_next_button_release = False;
    while(FCheckMaskEvent(dpy, PointerMotionMask | ButtonMotionMask,Event))
      ;

    if(Event->xmotion.state & Button3MotionMask)
    {
      for(i=0;i<ndesks;i++)
      {
	if(Event->xany.window == Desks[i].w)
	{
	  if (FQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	  {
	    /* pointer is on a different screen - that's okay here */
	  }
	  Scroll(desk_w, desk_h, x, y, i, False);
	}
      }
      if(Event->xany.window == icon_win)
      {
	if (FQueryPointer(dpy, icon_win, &JunkRoot, &JunkChild,
			  &JunkX, &JunkY,&x, &y, &JunkMask) == False)
	{
	  /* pointer is on a different screen - that's okay here */
	}
	Scroll(icon_w, icon_h, x, y, 0, True);
      }

    }
    break;

  case ClientMessage:
    if ((Event->xclient.format==32) &&
	(Event->xclient.data.l[0]==wm_del_win))
    {
      /* does not return */
      ExitPager();
    }
    break;
  }
}

void HandleEnterNotify(XEvent *Event)

{
  PagerWindow *t;
  Bool is_icon_view = False;
  extern Bool do_focus_on_enter;

  if (!ShowBalloons && !do_focus_on_enter)
    /* nothing to do */
    return;

  /* is this the best way to match X event window ID to PagerWindow ID? */
  for ( t = Start; t != NULL; t = t->next )
  {
    if ( t->PagerView == Event->xcrossing.window )
    {
      break;
    }
    if ( t->IconView == Event->xcrossing.window )
    {
      is_icon_view = True;
      break;
    }
  }
  if (t == NULL)
  {
    return;
  }

  if (ShowBalloons)
  {
    MapBalloonWindow(t, is_icon_view);
  }

  if (do_focus_on_enter)
  {
    SendText(fd, "Silent FlipFocus NoWarp", t->w);
  }

}

XRectangle get_expose_bound(XEvent *Event)
{
	XRectangle r;
	int ex, ey, ex2, ey2;

	ex = Event->xexpose.x;
	ey = Event->xexpose.y;
	ex2 = Event->xexpose.x + Event->xexpose.width;
	ey2 = Event->xexpose.y + Event->xexpose.height;
	while (FCheckTypedWindowEvent(dpy, Event->xany.window, Expose, Event))
	{
		ex = min(ex, Event->xexpose.x);
		ey = min(ey, Event->xexpose.y);
		ex2 = max(ex2, Event->xexpose.x + Event->xexpose.width);
		ey2= max(ey2 , Event->xexpose.y + Event->xexpose.height);
	}
	r.x = ex;
	r.y = ey;
	r.width = ex2-ex;
	r.height = ey2-ey;
	return r;
}

void HandleExpose(XEvent *Event)
{
	int i;
	PagerWindow *t;
	XRectangle r;

	/* it will be good to have full "clipping redraw". Do that for
	 * desk label only for now */
	for(i=0;i<ndesks;i++)
	{
		/* ric@giccs.georgetown.edu */
		if (Event->xany.window == Desks[i].w ||
		    Event->xany.window == Desks[i].title_w)
		{
			r = get_expose_bound(Event);
			DrawGrid(i, 0, Event->xany.window, &r);
			return;
		}
	}
	if (Event->xany.window == Scr.balloon_w)
	{
		DrawInBalloonWindow(Scr.balloon_desk);
		return;
	}
	if(Event->xany.window == icon_win)
		DrawIconGrid(0);

	for (t = Start; t != NULL; t = t->next)
	{
		if (t->PagerView == Event->xany.window)
		{
			LabelWindow(t);
			PictureWindow(t);
			BorderWindow(t);
		}
		else if(t->IconView == Event->xany.window)
		{
			LabelIconWindow(t);
			PictureIconWindow(t);
			BorderIconWindow(t);
		}
	}

	discard_events(Expose, Event->xany.window, NULL);
}

/*
 *
 * Respond to a change in window geometry.
 *
 */
void ReConfigure(void)
{
  Window root;
  unsigned border_width, depth;
  int n,m,w,h,n1,m1,x,y,i,j,k;
  int old_ww;
  int old_wh;
  int is_size_changed;

  old_ww = window_w;
  old_wh = window_h;
  if (!XGetGeometry(dpy, Scr.Pager_w, &root, &x, &y, (unsigned *)&window_w,
		    (unsigned *)&window_h, &border_width,&depth))
  {
    return;
  }
  is_size_changed = (old_ww != window_w || old_wh != window_h);

  n1 = Scr.Vx / Scr.MyDisplayWidth;
  m1 = Scr.Vy / Scr.MyDisplayHeight;
  n = Scr.VxMax / Scr.MyDisplayWidth;
  m = Scr.VyMax / Scr.MyDisplayHeight;

  sizehints.width_inc = Columns * (n + 1);
  sizehints.height_inc = Rows * (m + 1);
  sizehints.base_width = Columns * n + Columns - 1;
  sizehints.base_height = Rows*(m + label_h+1) - 1;
  sizehints.min_width = sizehints.base_width;
  sizehints.min_height = sizehints.base_height;
  if (window_w > 0)
  {
    window_w = (window_w - sizehints.base_width) / sizehints.width_inc;
    window_w = window_w * sizehints.width_inc + sizehints.base_width;
  }
  if (window_h > 0)
  {
    window_h = (window_h - sizehints.base_height) / sizehints.height_inc;
    window_h = window_h * sizehints.height_inc + sizehints.base_height;
  }
  desk_w = (window_w - Columns + 1) / Columns;
  desk_h = (window_h - Rows * label_h - Rows + 1) / Rows;
  w = (desk_w - n)/(n+1);
  h = (desk_h - m)/(m+1);

  XSetWMNormalHints(dpy,Scr.Pager_w,&sizehints);

  x = (desk_w - n) * Scr.Vx / Scr.VWidth + n1;
  y = (desk_h - m) * Scr.Vy / Scr.VHeight + m1;

  for(k=0;k<Rows;k++)
  {
    for(j=0;j<Columns;j++)
    {
      i = k*Columns+j;
      if (i<ndesks)
      {
	XMoveResizeWindow(
	  dpy,Desks[i].title_w, (desk_w+1)*j-1,(desk_h+label_h+1)*k-1,
	  desk_w,desk_h+label_h);
	XMoveResizeWindow(
	  dpy,Desks[i].w, -1, (LabelsBelow) ? -1 : label_h - 1, desk_w,desk_h);
	if (!is_size_changed)
	{
		if (CSET_IS_TRANSPARENT(Desks[i].colorset))
		{
			draw_desk_background(i, w, h);
		}
	}
	if (HilightDesks)
	{
	  if(i == Scr.CurrentDesk - desk1)
	    XMoveResizeWindow(dpy, Desks[i].CPagerWin, x,y,w,h);
	  else
	    XMoveResizeWindow(dpy, Desks[i].CPagerWin, -32768, -32768,w,h);
	}
	draw_desk_background(i, w, h);
      }
    }
  }
  /* reconfigure all the subordinate windows */
  ReConfigureAll();
}

/*
 *
 * Respond to a "background" change: update the Parental Relative cset
 *
 */

/* layout:
 * Root -> pager window (pr) -> title window -> desk window -> window view
 *  |                                                |-> hilight desk
 *  |-> icon_window -> icon view
 *  |-> ballon window
 */

/* update the hilight desk and the windows: desk color change */
static
void update_pr_transparent_subwindows(int i)
{
	int cset;
	int n,m,w,h;
	PagerWindow *t;

	n = Scr.VxMax / Scr.MyDisplayWidth;
	m = Scr.VyMax / Scr.MyDisplayHeight;
	w = (desk_w - n)/(n+1);
	h = (desk_h - m)/(m+1);

	if (CSET_IS_TRANSPARENT_PR(Desks[i].highcolorset) && HilightDesks)
	{
		SetWindowBackground(
			dpy, Desks[i].CPagerWin, w, h,
			&Colorset[Desks[i].highcolorset],
			Pdepth, Scr.NormalGC, True);
	}

	t = Start;
	for(t = Start; t != NULL; t = t->next)
	{
		cset = (t != FocusWin) ? windowcolorset : activecolorset;
		if (t->desk != i && !CSET_IS_TRANSPARENT_PR(cset))
		{
			continue;
		}
		if (t->PagerView != None)
		{
			SetWindowBackground(
				dpy, t->PagerView, t->pager_view_width,
				t->pager_view_height,
				&Colorset[cset], Pdepth, Scr.NormalGC, True);
		}
		if (t->IconView)
		{
			SetWindowBackground(
				dpy, t->IconView, t->icon_view_width,
				t->icon_view_height,
				&Colorset[cset], Pdepth, Scr.NormalGC, True);
		}
	}
}

/* update all the parental relative windows: pr background change */
void update_pr_transparent_windows(void)
{
	int i,j,k,cset;
	int n,m,w,h;
	PagerWindow *t;

	n = Scr.VxMax / Scr.MyDisplayWidth;
	m = Scr.VyMax / Scr.MyDisplayHeight;
	w = (desk_w - n)/(n+1);
	h = (desk_h - m)/(m+1);

	for(k=0;k<Rows;k++)
	{
		for(j=0;j<Columns;j++)
		{
			i = k*Columns+j;
			if (i < ndesks)
			{
				if (CSET_IS_TRANSPARENT_PR(Desks[i].colorset))
				{
					draw_desk_background(i, w, h);
				}
				else if (CSET_IS_TRANSPARENT_PR(
					Desks[i].highcolorset) && HilightDesks)
				{
					SetWindowBackground(
						dpy, Desks[i].CPagerWin, w, h,
						&Colorset[Desks[i].highcolorset],
						Pdepth, Scr.NormalGC, True);
				}
			}
		}
	}
	/* subordinate windows with a pr parent desk */
	t = Start;
	for(t = Start; t != NULL; t = t->next)
	{
		cset = (t != FocusWin) ? windowcolorset : activecolorset;
		if (!CSET_IS_TRANSPARENT_PR(cset) ||
		    (fAlwaysCurrentDesk &&
		     !CSET_IS_TRANSPARENT_PR(Desks[0].colorset)) ||
		    (!fAlwaysCurrentDesk &&
		     !CSET_IS_TRANSPARENT_PR(Desks[t->desk].colorset)))
		{
			continue;
		}
		if (t->PagerView != None)
		{
			SetWindowBackground(
				dpy, t->PagerView, t->pager_view_width,
				t->pager_view_height,
				&Colorset[cset], Pdepth, Scr.NormalGC, True);
		}
		if (t->desk && t->IconView)
		{
			SetWindowBackground(
				dpy, t->IconView, t->icon_view_width,
				t->icon_view_height,
				&Colorset[cset], Pdepth, Scr.NormalGC, True);
		}
	}

	/* ballon */
	if (BalloonView != None)
	{
		cset = Desks[Scr.balloon_desk].ballooncolorset;
		if (CSET_IS_TRANSPARENT_PR(cset))
		{
			XClearArea(dpy, Scr.balloon_w, 0, 0, 0, 0, True);
		}
	}
}

void MovePage(Bool is_new_desk)
{
  int n1,m1,x,y,n,m,i,w,h;
  XTextProperty name;
  char str[100],*sptr;
  static int icon_desk_shown = -1000;

  Wait = 0;
  n1 = Scr.Vx/Scr.MyDisplayWidth;
  m1 = Scr.Vy/Scr.MyDisplayHeight;
  n = Scr.VxMax / Scr.MyDisplayWidth;
  m = Scr.VyMax / Scr.MyDisplayHeight;

  x = (desk_w - n) * Scr.Vx / Scr.VWidth + n1;
  y = (desk_h - m) * Scr.Vy / Scr.VHeight + m1;
  w = (desk_w - n)/(n+1);
  h = (desk_h - m)/(m+1);

  for(i=0;i<ndesks;i++)
  {
    if (HilightDesks)
    {
      if(i == Scr.CurrentDesk - desk1)
      {
	XMoveWindow(dpy, Desks[i].CPagerWin, x,y);
	XLowerWindow(dpy,Desks[i].CPagerWin);
	if (CSET_IS_TRANSPARENT(Desks[i].highcolorset))
	{
		SetWindowBackground(
			dpy, Desks[i].CPagerWin, w, h,
			&Colorset[Desks[i].highcolorset], Pdepth,
			Scr.NormalGC, True);
	}
      }
      else
      {
	XMoveWindow(dpy, Desks[i].CPagerWin, -32768,-32768);
      }
    }
  }
  DrawIconGrid(1);

  ReConfigureIcons(!is_new_desk);

  if(Scr.CurrentDesk != icon_desk_shown)
  {
    icon_desk_shown = Scr.CurrentDesk;

    if((Scr.CurrentDesk >= desk1)&&(Scr.CurrentDesk <=desk2))
      sptr = Desks[Scr.CurrentDesk -desk1].label;
    else
    {
      sprintf(str, "GotoDesk %d", Scr.CurrentDesk);
      sptr = &str[0];
    }

    if (FlocaleTextListToTextProperty(
	  dpy, &sptr, 1, XStdICCTextStyle, &name) == 0)
    {
      fprintf(stderr,"%s: cannot allocate window name", MyName);
      return;
    }
    XSetWMIconName(dpy,Scr.Pager_w,&name);
    XFree(name.value);
  }
}

void ReConfigureAll(void)
{
  PagerWindow *t;

  t = Start;
  while(t!= NULL)
  {
    MoveResizePagerView(t, True);
    t = t->next;
  }
}

void ReConfigureIcons(Bool do_reconfigure_desk_only)
{
  PagerWindow *t;
  int x, y, w, h;

  for (t = Start; t != NULL; t = t->next)
  {
    if (do_reconfigure_desk_only && t->desk != Scr.CurrentDesk)
      continue;
    CalcGeom(t, icon_w, icon_h, &x, &y, &w, &h);
    t->icon_view_x = x;
    t->icon_view_y = y;
    t->icon_view_width = w;
    t->icon_view_height = h;
    if(Scr.CurrentDesk == t->desk)
      XMoveResizeWindow(dpy, t->IconView, x, y, w, h);
    else
      XMoveResizeWindow(dpy, t->IconView, -32768, -32768, w, h);
  }
}

/*
 *
 * Draw grid lines for desk #i
 *
 */
void DrawGrid(int desk, int erase, Window ew, XRectangle *r)
{
	int y, y1, y2, x, x1, x2,d,w;
	char str[15], *ptr;
	int cs;
	XRectangle bound;
	Region region = 0;

	if((desk < 0 ) || (desk >= ndesks))
		return;

	/* desk grid */
	if (!ew || ew == Desks[desk].w)
	{
		x = Scr.MyDisplayWidth;
		y1 = 0;
		y2 = desk_h;
		while (x < Scr.VWidth)
		{
			x1 = (x * desk_w) / Scr.VWidth;
			if (!use_no_separators)
			{
				XDrawLine(
					dpy,Desks[desk].w,Desks[desk].DashedGC,
					x1,y1,x1,y2);
			}
			x += Scr.MyDisplayWidth;
		}
		y = Scr.MyDisplayHeight;
		x1 = 0;
		x2 = desk_w;
		while(y < Scr.VHeight)
		{
			y1 = (y * desk_h) / Scr.VHeight;
			if (!use_no_separators)
			{
				XDrawLine(
					dpy,Desks[desk].w,Desks[desk].DashedGC,
					x1,y1,x2,y1);
			}
			y += Scr.MyDisplayHeight;
		}
	}

	if (ew && ew != Desks[desk].title_w)
	{
		return;
	}

	/* desk label */
	if (r)
	{
		bound.x = r->x;
		bound.y = r->y;
		bound.width = r->width;
		bound.height = r->height;
		region = XCreateRegion();
		XUnionRectWithRegion (&bound, region, region);
	}
	else
	{
		bound.x = 0;
		bound.y = (LabelsBelow ? desk_h : 0);
		bound.width = desk_w;
		bound.height = label_h;
	}

	if (FftSupport && Ffont->fftf.fftfont != NULL)
	{
		erase = True;
	}
	if(((Scr.CurrentDesk - desk1) == desk) && !ShapeLabels)
	{
		if (uselabel)
		{
			XFillRectangle(
				dpy,Desks[desk].title_w,Desks[desk].HiliteGC,
				bound.x, bound.y, bound.width, bound.height);
		}
	}
	else
	{
		if(uselabel && erase)
		{
			XClearArea(dpy,Desks[desk].title_w,
				   bound.x, bound.y, bound.width, bound.height,
				   False);
		}
	}

	d = desk1+desk;
	ptr = Desks[desk].label;
	w = FlocaleTextWidth(Ffont,ptr,strlen(ptr));
	if( w > desk_w)
	{
		sprintf(str,"%d",d);
		ptr = str;
		w = FlocaleTextWidth(Ffont,ptr,strlen(ptr));
	}
	if((w <= desk_w)&&(uselabel))
	{
		FwinString->str = ptr;
		FwinString->win = Desks[desk].title_w;
		if(desk == (Scr.CurrentDesk - desk1))
		{
			cs = Desks[desk].highcolorset;
			FwinString->gc = Desks[desk].rvGC;
		}
		else
		{
			cs = Desks[desk].colorset;
			FwinString->gc = Desks[desk].NormalGC;
		}

		FwinString->flags.has_colorset = False;
		if (cs >= 0)
		{
			FwinString->colorset = &Colorset[cs];
			FwinString->flags.has_colorset = True;
		}
		FwinString->x = (desk_w - w)/2;
		FwinString->y = (LabelsBelow ?
				 desk_h + Ffont->ascent + 1 : Ffont->ascent + 1);
		if (region)
		{
			FwinString->flags.has_clip_region = True;
			FwinString->clip_region = region;
			XSetRegion(dpy, FwinString->gc, region);
		}
		else
		{
			FwinString->flags.has_clip_region = False;
		}
		FlocaleDrawString(dpy, Ffont, FwinString, 0);
		if (region)
		{
			XDestroyRegion(region);
			FwinString->flags.has_clip_region = False;
			FwinString->clip_region = None;
			XSetClipMask(dpy, FwinString->gc, None);
		}
	}

	if (FShapesSupported)
	{
		UpdateWindowShape ();
	}
}


void DrawIconGrid(int erase)
{
  int y, y1, y2, x, x1, x2,w,h,n,m,n1,m1;
  int i;

  if(erase)
  {
    int tmp=(Scr.CurrentDesk - desk1);

    if ((tmp < 0) || (tmp >= ndesks))
    {
      if (PixmapBack)
	XSetWindowBackgroundPixmap(dpy, icon_win,PixmapBack->picture);
      else
	XSetWindowBackground(dpy, icon_win, back_pix);
    }
    else
    {
      if (Desks[tmp].bgPixmap)
	XSetWindowBackgroundPixmap(
	    dpy, icon_win,Desks[tmp].bgPixmap->picture);
      else if (Desks[tmp].Dcolor)
	XSetWindowBackground(dpy, icon_win, GetColor(Desks[tmp].Dcolor));
      else if (PixmapBack)
	XSetWindowBackgroundPixmap(dpy, icon_win, PixmapBack->picture);
      else
	XSetWindowBackground(dpy, icon_win, back_pix);
    }

    XClearWindow(dpy,icon_win);
  }

  x = Scr.MyDisplayWidth;
  y1 = 0;
  y2 = icon_h;
  while(x < Scr.VWidth)
  {
    x1 = x * icon_w / Scr.VWidth;
    if (!use_no_separators)
      for(i=0;i<ndesks;i++)
	XDrawLine(dpy,icon_win,Desks[i].DashedGC,x1,y1,x1,y2);
    x += Scr.MyDisplayWidth;
  }

  y = Scr.MyDisplayHeight;
  x1 = 0;
  x2 = icon_w;
  while(y < Scr.VHeight)
  {
    y1 = y * icon_h / Scr.VHeight;
    if (!use_no_separators)
      for(i=0;i<ndesks;i++)
	XDrawLine(dpy,icon_win,Desks[i].DashedGC,x1,y1,x2,y1);
    y += Scr.MyDisplayHeight;
  }
  n1 = Scr.Vx / Scr.MyDisplayWidth;
  m1 = Scr.Vy / Scr.MyDisplayHeight;
  n = Scr.VxMax / Scr.MyDisplayWidth;
  m = Scr.VyMax / Scr.MyDisplayHeight;
  w = (icon_w - n) / (n + 1);
  h = (icon_h - m) / (m + 1);

  x = (icon_w - n) * Scr.Vx / Scr.VWidth + n1;
  y = (icon_h - m) * Scr.Vy / Scr.VHeight + m1;

  if (HilightDesks)
  {
    if (HilightPixmap)
    {
      for(i=0;i<ndesks;i++)
	XCopyArea(dpy, HilightPixmap->picture, icon_win, Scr.NormalGC, 0, 0,
		  w, h, x, y);
    }
    else
    {
      for(i=0;i<ndesks;i++)
	XFillRectangle (dpy, icon_win, Desks[i].HiliteGC, x, y, w, h);
    }
  }
}


void SwitchToDesk(int Desk)
{
  char command[256];

  sprintf(command, "GotoDesk 0 %d", Desk + desk1);
  SendText(fd,command,0);
}


void SwitchToDeskAndPage(int Desk, XEvent *Event)
{
  char command[256];

  if (Scr.CurrentDesk != (Desk + desk1))
  {
    int vx, vy;
    /* patch to let mouse button 3 change desks and do not cling to a page */
    vx = Event->xbutton.x * Scr.VWidth / (desk_w * Scr.MyDisplayWidth);
    vy = Event->xbutton.y * Scr.VHeight / (desk_h * Scr.MyDisplayHeight);
    Scr.Vx = vx * Scr.MyDisplayWidth;
    Scr.Vy = vy * Scr.MyDisplayHeight;
    sprintf(command, "GotoDeskAndPage %d %d %d", Desk + desk1, vx, vy);
    SendText(fd, command, 0);

  }
  else
  {
    int x = Event->xbutton.x * Scr.VWidth / (desk_w * Scr.MyDisplayWidth);
    int y = Event->xbutton.y * Scr.VHeight / (desk_h * Scr.MyDisplayHeight);

    /* Fix for buggy XFree86 servers that report button release events
     * incorrectly when moving fast. Not perfect, but should at least prevent
     * that we get a random page. */
    if (x < 0)
      x = 0;
    if (y < 0)
      y = 0;
    if (x * Scr.MyDisplayWidth > Scr.VxMax)
      x = Scr.VxMax / Scr.MyDisplayWidth;
    if (y * Scr.MyDisplayHeight > Scr.VyMax)
      y = Scr.VyMax / Scr.MyDisplayHeight;
    sprintf(command, "GotoPage %d %d", x, y);
    SendText(fd, command, 0);
  }
  Wait = 1;
}

void IconSwitchPage(XEvent *Event)
{
  char command[34];

  sprintf(command,"GotoPage %d %d",
	  Event->xbutton.x * Scr.VWidth / (icon_w * Scr.MyDisplayWidth),
	  Event->xbutton.y * Scr.VHeight / (icon_h * Scr.MyDisplayHeight));
  SendText(fd, command, 0);
  Wait = 1;
}


void AddNewWindow(PagerWindow *t)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	int i, x, y, w, h;

	i = t->desk - desk1;
	CalcGeom(t, desk_w, desk_h, &x, &y, &w, &h);
	t->pager_view_x = x;
	t->pager_view_y = y;
	t->pager_view_width = w;
	t->pager_view_height = h;
	valuemask = CWBackPixel | CWEventMask;
	attributes.background_pixel = t->back;
	attributes.event_mask = ExposureMask;

	/* ric@giccs.georgetown.edu -- added Enter and Leave events for
	   popping up balloon window */
	attributes.event_mask =
		ExposureMask | EnterWindowMask | LeaveWindowMask;

	if ((i >= 0) && (i < ndesks))
	{
		t->PagerView = XCreateWindow(
			dpy,Desks[i].w, x, y, w, h, 0, CopyFromParent,
			InputOutput, CopyFromParent, valuemask, &attributes);
		if (windowcolorset > -1)
		{
			SetWindowBackground(
				dpy, t->PagerView, w, h,
				&Colorset[windowcolorset], Pdepth,
				Scr.NormalGC, True);
		}
		if (!UseSkipList || !DO_SKIP_WINDOW_LIST(t))
		{
			if (IS_ICONIFIED(t))
			{
				XMoveResizeWindow(
					dpy, t->PagerView, -32768, -32768, 1,
					1);
			}
			XMapRaised(dpy, t->PagerView);
		}
	}
	else
	{
		t->PagerView = None;
	}

	CalcGeom(t, icon_w, icon_h, &x, &y, &w, &h);
	t->icon_view_x = x;
	t->icon_view_y = y;
	t->icon_view_width = w;
	t->icon_view_height = h;
	if(Scr.CurrentDesk != t->desk)
	{
		x = -32768;
		y = -32768;
	}
	t->IconView = XCreateWindow(
		dpy,icon_win, x, y, w, h, 0, CopyFromParent, InputOutput,
		CopyFromParent, valuemask, &attributes);
	if (windowcolorset > -1)
	{
		SetWindowBackground(
			dpy, t->IconView, w, h, &Colorset[windowcolorset],
			Pdepth, Scr.NormalGC, True);
	}
	if(Scr.CurrentDesk == t->desk)
	{
		XGrabButton(
			dpy, 2, AnyModifier, t->IconView, True,
			ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
			GrabModeAsync, GrabModeAsync, None, None);
	}
	if (!UseSkipList || !DO_SKIP_WINDOW_LIST(t))
	{
		if (IS_ICONIFIED(t))
		{
			XMoveResizeWindow(
				dpy, t->IconView, -32768, -32768, 1,
				1);
		}
		XMapRaised(dpy, t->IconView);
		t->myflags.is_mapped = 1;
	}
	else
	{
		t->myflags.is_mapped = 0;
	}
	Hilight(t, False);

	return;
}


void ChangeDeskForWindow(PagerWindow *t,long newdesk)
{
  int i, x, y, w, h;
  Bool size_changed = False;

  i = newdesk - desk1;
  if(t->PagerView == None)
  {
    t->desk = newdesk;
    XDestroyWindow(dpy,t->IconView);
    AddNewWindow( t);
    return;
  }

  CalcGeom(t, desk_w, desk_h, &x, &y, &w, &h);
  size_changed = (t->pager_view_width != w || t->pager_view_height != h);
  t->pager_view_x = x;
  t->pager_view_y = y;
  t->pager_view_width = w;
  t->pager_view_height = h;

  if ((i >= 0) && (i < ndesks))
  {
    int cset;

    XReparentWindow(dpy, t->PagerView, Desks[i].w, x, y);
    if (size_changed)
      XResizeWindow(dpy, t->PagerView, w, h);
    cset = (t != FocusWin) ? windowcolorset : activecolorset;
    if (cset > -1 && (size_changed || CSET_IS_TRANSPARENT(cset)))
    {
      SetWindowBackground(
	dpy, t->PagerView, t->pager_view_width, t->pager_view_height,
	&Colorset[cset], Pdepth, Scr.NormalGC, True);
    }
  }
  else
  {
    XDestroyWindow(dpy,t->PagerView);
    t->PagerView = None;
  }
  t->desk = i+desk1;

  CalcGeom(t, icon_w, icon_h, &x, &y, &w, &h);
  size_changed = (t->icon_view_width != w || t->icon_view_height != h);
  t->icon_view_x = x;
  t->icon_view_y = y;
  t->icon_view_width = w;
  t->icon_view_height = h;
  if(Scr.CurrentDesk != t->desk)
    XMoveResizeWindow(dpy,t->IconView,-32768,-32768,w,h);
  else
  {
    int cset;

    XMoveResizeWindow(dpy,t->IconView,x,y,w,h);
    cset = (t != FocusWin) ? windowcolorset : activecolorset;
    if (cset > -1 && (size_changed || CSET_IS_TRANSPARENT(cset)))
    {
      SetWindowBackground(
	dpy, t->IconView, t->icon_view_width, t->icon_view_height,
	&Colorset[cset], Pdepth, Scr.NormalGC, True);
    }
  }
}

void MoveResizePagerView(PagerWindow *t, Bool do_force_redraw)
{
  int x, y, w, h;
  Bool size_changed;
  Bool position_changed;

  if (UseSkipList && DO_SKIP_WINDOW_LIST(t) && t->myflags.is_mapped)
  {
    if (t->PagerView)
      XUnmapWindow(dpy, t->PagerView);
    if (t->IconView)
      XUnmapWindow(dpy, t->IconView);
    t->myflags.is_mapped = 0;
  }
  else if (UseSkipList && !DO_SKIP_WINDOW_LIST(t) && !t->myflags.is_mapped)
  {
    if (t->PagerView)
      XMapRaised(dpy, t->PagerView);
    if (t->IconView)
      XMapRaised(dpy, t->IconView);
    t->myflags.is_mapped = 1;
  }

  CalcGeom(t, desk_w, desk_h, &x, &y, &w, &h);
  position_changed = (t->pager_view_x != x || t->pager_view_y != y);
  size_changed = (t->pager_view_width != w || t->pager_view_height != h);
  t->pager_view_x = x;
  t->pager_view_y = y;
  t->pager_view_width = w;
  t->pager_view_height = h;
  if (t->PagerView != None)
  {
    if (size_changed || position_changed || do_force_redraw)
    {
      int cset;

      XMoveResizeWindow(dpy, t->PagerView, x, y, w, h);
      cset = (t != FocusWin) ? windowcolorset : activecolorset;
      if (cset > -1 && (size_changed || CSET_IS_TRANSPARENT(cset)))
      {
	SetWindowBackground(
	  dpy, t->PagerView, t->pager_view_width, t->pager_view_height,
	  &Colorset[cset], Pdepth, Scr.NormalGC, True);
      }
    }
  }
  else if (t->desk >= desk1 && t->desk <= desk2)
  {
    XDestroyWindow(dpy, t->IconView);
    AddNewWindow(t);
    return;
  }

  CalcGeom(t, icon_w, icon_h, &x, &y, &w, &h);
  position_changed = (t->icon_view_x != x || t->icon_view_y != y);
  size_changed = (t->icon_view_width != w || t->icon_view_height != h);
  t->icon_view_x = x;
  t->icon_view_y = y;
  t->icon_view_width = w;
  t->icon_view_height = h;
  if (Scr.CurrentDesk == t->desk)
  {
    int cset;

    XMoveResizeWindow(dpy, t->IconView, x, y, w, h);
    cset = (t != FocusWin) ? windowcolorset : activecolorset;
    if (cset > -1 && (size_changed || CSET_IS_TRANSPARENT(cset)))
    {
      SetWindowBackground(
	dpy, t->IconView, t->icon_view_width, t->icon_view_height,
	&Colorset[cset], Pdepth, Scr.NormalGC, True);
    }
  }
  else
  {
    XMoveResizeWindow(dpy, t->IconView, -32768, -32768, w, h);
  }
}


void MoveStickyWindow(Bool is_new_page, Bool is_new_desk)
{
	PagerWindow *t;

	for (t = Start; t != NULL; t = t->next)
	{
		if (
			is_new_desk && t->desk != Scr.CurrentDesk &&
			((IS_ICONIFIED(t) && IS_ICON_STICKY_ACROSS_DESKS(t)) ||
			 IS_STICKY_ACROSS_DESKS(t)))
		{
			ChangeDeskForWindow(t, Scr.CurrentDesk);
		}
		else if (
			is_new_page &&
			((IS_ICONIFIED(t) &&
			  IS_ICON_STICKY_ACROSS_PAGES(t)) ||
			 IS_STICKY_ACROSS_PAGES(t)))
		{
			MoveResizePagerView(t, True);
		}
	}
}

void Hilight(PagerWindow *t, int on)
{

  if(!t)
    return;

  if(Pdepth < 2)
  {
    if(on)
    {
      if(t->PagerView != None)
	XSetWindowBackgroundPixmap(dpy,t->PagerView,Scr.gray_pixmap);
      XSetWindowBackgroundPixmap(dpy,t->IconView,Scr.gray_pixmap);
    }
    else
    {
      if(IS_STICKY_ACROSS_DESKS(t) || IS_STICKY_ACROSS_PAGES(t))
      {
	if(t->PagerView != None)
	  XSetWindowBackgroundPixmap(dpy,t->PagerView,
				     Scr.sticky_gray_pixmap);
	XSetWindowBackgroundPixmap(dpy,t->IconView,
				   Scr.sticky_gray_pixmap);
      }
      else
      {
	if(t->PagerView != None)
	  XSetWindowBackgroundPixmap(dpy,t->PagerView,
				     Scr.light_gray_pixmap);
	XSetWindowBackgroundPixmap(dpy,t->IconView,
				   Scr.light_gray_pixmap);
      }
    }
    if(t->PagerView != None)
      XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
    XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
  }
  else
  {
    int cset;
    Pixel pix;

    cset = (on) ? activecolorset : windowcolorset;
    pix = (on) ? focus_pix : t->back;
    if (cset < 0)
    {
      if (t->PagerView != None)
      {
	XSetWindowBackground(dpy,t->PagerView,pix);
	XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
      }
      XSetWindowBackground(dpy,t->IconView,pix);
      XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
    }
    else
    {
      if (t->PagerView != None)
      {
	SetWindowBackground(
	  dpy, t->PagerView, t->pager_view_width, t->pager_view_height,
	  &Colorset[cset], Pdepth, Scr.NormalGC, True);
      }
      SetWindowBackground(
	dpy, t->IconView, t->icon_view_width, t->icon_view_height,
	&Colorset[cset], Pdepth, Scr.NormalGC, True);
    }
  }
}

/* Use Desk == -1 to scroll the icon window */
void Scroll(int window_w, int window_h, int x, int y, int Desk,
	    Bool do_scroll_icon)
{
	static int last_sx = -999999;
	static int last_sy = -999999;
	int sx;
	int sy;
	int adjx,adjy;

	/* Desk < 0 means we want to scroll an icon window */
	if (Desk >= 0 && Desk + desk1 != Scr.CurrentDesk)
	{
		return;
	}

	/* center around mouse */
	adjx = (desk_w / (1 + Scr.VxMax / Scr.MyDisplayWidth));
	adjy = (desk_h / (1 + Scr.VyMax / Scr.MyDisplayHeight));
	x -= adjx/2;
	y -= adjy/2;

	/* adjust for pointer going out of range */
	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}

	if (x > window_w-adjx)
	{
		x = window_w-adjx;
	}
	if (y > window_h-adjy)
	{
		y = window_h-adjy;
	}

	sx = 0;
	sy = 0;
	if (window_w != 0)
	{
		sx = (x * Scr.VWidth / window_w - MyVx);
	}
	if (window_h != 0)
	{
		sy = (y * Scr.VHeight / window_h - MyVy);
	}
	MYFPRINTF((stderr,"[scroll]: %d %d %d %d %d %d\n", window_w, window_h,
		   x, y, sx, sy));
	if (sx == 0 && sy == 0)
	{
		return;
	}
	if (Wait == 0 || last_sx != sx || last_sy != sy)
	{
		do_scroll(sx, sy, False, False);

		/* Here we need to track the view offset on the desk. */
		/* sx/y are are pixels on the screen to scroll. */
		/* We don't use Scr.Vx/y since they lag the true position. */
		MyVx += sx;
		if (MyVx < 0)
		{
			MyVx = 0;
		}
		MyVy += sy;

		if (MyVy < 0)
		{
			MyVy = 0;
		}
		Wait = 1;
	}
	if (Wait == 0)
	{
		last_sx = sx;
		last_sy = sy;
	}

	return;
}

void MoveWindow(XEvent *Event)
{
	char command[100];
	int x1, y1, wx, wy, n, x, y, xi = 0, yi = 0, wx1, wy1, x2, y2;
	int finished = 0;
	Window dumwin;
	PagerWindow *t;
	int m, n1, m1;
	int NewDesk, KeepMoving = 0;
	int moved = 0;
	int row, column;
	Window JunkRoot, JunkChild;
	int JunkX, JunkY;
	unsigned JunkMask;
	int do_switch_desk_later = 0;

	t = Start;
	while (t != NULL && t->PagerView != Event->xbutton.subwindow)
	{
		t = t->next;
	}

	if (t == NULL)
	{
		t = Start;
		while (t != NULL && t->IconView != Event->xbutton.subwindow)
		{
			t = t->next;
		}
		if (t != NULL)
		{
			IconMoveWindow(Event, t);
			return;
		}
	}

	if (t == NULL || !t->allowed_actions.is_movable)
	{
		return;
	}

	NewDesk = t->desk - desk1;
	if (NewDesk < 0 || NewDesk >= ndesks)
	{
		return;
	}

	n = Scr.VxMax / Scr.MyDisplayWidth;
	m = Scr.VyMax / Scr.MyDisplayHeight;
	n1 = (Scr.Vx + t->x) / Scr.MyDisplayWidth;
	m1 = (Scr.Vy + t->y) / Scr.MyDisplayHeight;
	wx = (Scr.Vx + t->x) * (desk_w - n) / Scr.VWidth + n1;
	wy = (Scr.Vy + t->y) * (desk_h - m) / Scr.VHeight + m1;
	wx1 = wx + (desk_w + 1) * (NewDesk % Columns);
	wy1 = wy + label_h + (desk_h + label_h + 1) * (NewDesk / Columns);
	if (LabelsBelow)
	{
		wy1 -= label_h;
	}

	XReparentWindow(dpy, t->PagerView, Scr.Pager_w, wx1, wy1);
	XRaiseWindow(dpy, t->PagerView);

	XTranslateCoordinates(dpy, Event->xany.window, t->PagerView,
			      Event->xbutton.x, Event->xbutton.y, &x1, &y1,
			      &dumwin);
	xi = x1;
	yi = y1;
	while (!finished)
	{
		FMaskEvent(dpy, ButtonReleaseMask | ButtonMotionMask |
			   ExposureMask, Event);
		if (Event->type == MotionNotify)
		{
			XTranslateCoordinates(dpy, Event->xany.window,
					      Scr.Pager_w, Event->xmotion.x,
					      Event->xmotion.y, &x, &y,
					      &dumwin);

			if (moved == 0)
			{
				xi = x;
				yi = y;
				moved = 1;
			}
			if (x < -5 || y < -5 || x > window_w + 5 ||
			   y > window_h + 5)
			{
				KeepMoving = 1;
				finished = 1;
			}
			XMoveWindow(dpy, t->PagerView, x - x1, y - y1);
		}
		else if (Event->type == ButtonRelease)
		{
			XTranslateCoordinates(dpy, Event->xany.window,
					      Scr.Pager_w, Event->xbutton.x,
					      Event->xbutton.y, &x, &y,
					      &dumwin);
			XMoveWindow(dpy, t->PagerView, x - x1, y - y1);
			finished = 1;
		}
		else if (Event->type == Expose)
		{
			HandleExpose(Event);
		}
	}

	if (moved)
	{
		if (abs(x - xi) < MoveThreshold &&
		    abs(y - yi) < MoveThreshold)
		{
			moved = 0;
		}
	}
	if (KeepMoving)
	{
		NewDesk = Scr.CurrentDesk;

		if (NewDesk >= desk1 && NewDesk <= desk2)
		{
			XReparentWindow(dpy, t->PagerView,
					Desks[NewDesk-desk1].w, 0, 0);
		}
		else
		{
			XDestroyWindow(dpy, t->PagerView);
			t->PagerView = None;
		}
		if (FQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
				  &x, &y, &JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			x = 0;
			y = 0;
		}
		XUngrabPointer(dpy,CurrentTime);
		XSync(dpy,0);

		if (NewDesk != t->desk)
		{
			/* griph: This used to move to NewDesk, but NewDesk
			 * is current desk, and if fvwm is on another
			 * desk (due to async operation) we have to move
			 * the window to it anyway or "Move Pointer" will
			 * move an invisible window. */

			SendText(fd, "Silent MoveToDesk", t->w);
			t->desk = NewDesk;
		}
		SendText(fd, "Silent Raise", t->w);
		SendText(fd, "Silent Move Pointer", t->w);
		return;
	}
	else
	{
		column = x / (desk_w + 1);
		if (column >= Columns)
		{
			column = Columns - 1;
		}
		if (column < 0)
		{
			column = 0;
		}
		row = y / (desk_h + label_h + 1);
		if (row >= Rows)
		{
			row = Rows - 1;
		}
		if (row < 0)
		{
			row = 0;
		}
		NewDesk = column + row * Columns;
		while (NewDesk < 0)
		{
			NewDesk += Columns;
			if (NewDesk >= ndesks)
			{
				NewDesk = 0;
			}
		}
		while (NewDesk >= ndesks)
		{
			NewDesk -= Columns;
			if (NewDesk < 0)
			{
				NewDesk = ndesks - 1;
			}
		}
		XTranslateCoordinates(dpy, Scr.Pager_w, Desks[NewDesk].w,
				      x - x1, y - y1, &x2, &y2, &dumwin);

		n1 = x2 * Scr.VWidth / (desk_w * Scr.MyDisplayWidth);
		m1 = y2 * Scr.VHeight / (desk_h * Scr.MyDisplayHeight);
		x = (x2 - n1) * Scr.VWidth / (desk_w - n) - Scr.Vx;
		y = (y2 - m1) * Scr.VHeight / (desk_h - m) - Scr.Vy;
		/* force onto desk */
		if (x + t->frame_width + Scr.Vx < 0 )
		{
			x = -Scr.Vx - t->frame_width;
		}
		if (y + t->frame_height + Scr.Vy < 0)
		{
			y = -Scr.Vy - t->frame_height;
		}
		if (x + Scr.Vx >= Scr.VWidth)
		{
			x = Scr.VWidth - Scr.Vx - 1;
		}
		if (y + Scr.Vy >= Scr.VHeight)
		{
			y = Scr.VHeight - Scr.Vy - 1;
		}
		if ((IS_ICONIFIED(t) && IS_ICON_STICKY_ACROSS_DESKS(t)) ||
		   (IS_STICKY_ACROSS_DESKS(t)))
		{
			NewDesk = Scr.CurrentDesk - desk1;
			if (x > Scr.MyDisplayWidth - 16)
			{
				x = Scr.MyDisplayWidth - 16;
			}
			if (y > Scr.MyDisplayHeight - 16)
			{
				y = Scr.MyDisplayHeight - 16;
			}
			if (x + t->width < 16)
			{
				x = 16 - t->width;
			}
			if (y + t->height < 16)
			{
				y = 16 - t->height;
			}
		}
		if (NewDesk + desk1 != t->desk)
		{
			if ((IS_ICONIFIED(t) && IS_ICON_STICKY_ACROSS_DESKS(t))
			   || (IS_STICKY_ACROSS_DESKS(t)))
			{
				NewDesk = Scr.CurrentDesk - desk1;
				if (t->desk != Scr.CurrentDesk)
				{
					ChangeDeskForWindow(t,Scr.CurrentDesk);
				}
			}
			else if (NewDesk + desk1 != Scr.CurrentDesk)
			{
				sprintf(command, "Silent MoveToDesk 0 %d",
					NewDesk + desk1);
				SendText(fd, command, t->w);
				t->desk = NewDesk + desk1;
			}
			else
			{
				do_switch_desk_later = 1;
			}
		}

		if (NewDesk >= 0 && NewDesk < ndesks)
		{
			XReparentWindow(dpy, t->PagerView,
					Desks[NewDesk].w, x2, y2);
			t->desk = NewDesk;
			XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
			if (moved)
			{
				char buf[64];
				sprintf(buf, "Silent Move +%dp +%dp", x, y);

				SendText(fd, buf, t->w);
				XSync(dpy,0);
			}
			else
			{
				MoveResizePagerView(t, True);
			}
			SendText(fd, "Silent Raise", t->w);
		}
		if (do_switch_desk_later)
		{
			sprintf(command, "Silent MoveToDesk 0 %d", NewDesk +
				desk1);
			SendText(fd, command, t->w);
			t->desk = NewDesk + desk1;
		}
		if (Scr.CurrentDesk == t->desk)
		{
			XSync(dpy,0);
			usleep(5000);
			XSync(dpy,0);

			SendText(fd, "Silent FlipFocus NoWarp", t->w);
		}
	}
	if (is_transient)
	{
		/* does not return */
		ExitPager();
	}
}


/*
 *
 *  Procedure:
 *	FvwmErrorHandler - displays info on internal errors
 *
 */
int FvwmErrorHandler(Display *dpy, XErrorEvent *event)
{
#if 1
  extern Bool error_occured;
  error_occured = True;
  return 0;
#else
  /* really should just exit here... */
  /* domivogt (07-mar-1999): No, not really. See comment above. */
  PrintXErrorAndCoredump(dpy, event, MyName);
  return 0;
#endif /* 1 */
}

static void draw_window_border(PagerWindow *t, Window w, int width, int height)
{
  if (w != None)
  {
    if (!WindowBorders3d || windowcolorset < 0 || activecolorset < 0)
    {
      XSetForeground(dpy, Scr.NormalGC, Scr.black);
      RelieveRectangle(
	dpy, w, 0, 0, width - 1, t->pager_view_height - 1,
	Scr.NormalGC, Scr.NormalGC, WindowBorderWidth);
    }
    else if (t == FocusWin)
    {
      RelieveRectangle(
	dpy, w, 0, 0, width - 1, height - 1,
	Scr.ahGC, Scr.asGC, WindowBorderWidth);
    }
    else
    {
      RelieveRectangle(
	dpy, w, 0, 0, width - 1, height - 1,
	Scr.whGC, Scr.wsGC, WindowBorderWidth);
    }
  }
}

void BorderWindow(PagerWindow *t)
{
  draw_window_border(
    t, t->PagerView, t->pager_view_width, t->pager_view_height);
}

void BorderIconWindow(PagerWindow *t)
{
  draw_window_border(
    t, t->IconView, t->icon_view_width, t->icon_view_height);
}

/* draw the window label with simple greedy wrapping */
static void label_window_wrap(PagerWindow *t)
{
  char *cur, *next, *end;
  int space_width, cur_width;

  space_width = FlocaleTextWidth(FwindowFont, " ", 1);
  cur_width   = 0;

  cur = next = t->window_label;
  end = cur + strlen(cur);

  while (*cur) {
    while (*next) {
      int width;
      char *p;

      if (!(p = strchr(next, ' ')))
        p = end;

      width = FlocaleTextWidth(FwindowFont, next, p - next );
      if (width > t->pager_view_width - cur_width - space_width - 2*label_border)
        break;
      cur_width += width + space_width;
      next = *p ? p + 1 : p;
    }

    if (cur == next) {
      /* word too large for window */
      while (*next) {
        int len, width;

        len   = FlocaleStringNumberOfBytes(FwindowFont, next);
        width = FlocaleTextWidth(FwindowFont, next, len);

        if (width > t->pager_view_width - cur_width - 2*label_border && cur != next)
          break;

        next      += len;
        cur_width += width;
      }
    }

    FwinString->str = xmalloc(next - cur + 1);
    strncpy(FwinString->str, cur, next - cur);
    FwinString->str[next - cur] = 0;

    FlocaleDrawString(dpy, FwindowFont, FwinString, 0);

    free(FwinString->str);
    FwinString->str = NULL;

    FwinString->y += FwindowFont->height;
    cur = next;
    cur_width = 0;
  }
}

static void do_label_window(PagerWindow *t, Window w)
{
  int cs;

  if (t == FocusWin)
  {
    XSetForeground(dpy, Scr.NormalGC, focus_fore_pix);
    cs =  activecolorset;
  }
  else
  {
    XSetForeground(dpy, Scr.NormalGC, t->text);
    cs = windowcolorset;
  }

  if (FwindowFont == NULL)
  {
    return;
  }
  if (MiniIcons && t->mini_icon.picture && (t->PagerView != None))
  {
    /* will draw picture instead... */
    return;
  }
  if (t->icon_name == NULL)
  {
    return;
  }

  /* Update the window label for this window */
  if (t->window_label)
    free(t->window_label);
  t->window_label = GetBalloonLabel(t, WindowLabelFormat);
  if (w != None)
  {
    if (FftSupport && FwindowFont != NULL && FwindowFont->fftf.fftfont != NULL)
      XClearWindow(dpy, w);
    FwinString->win = w;
    FwinString->gc = Scr.NormalGC;
    FwinString->flags.has_colorset = False;
    if (cs >= 0)
    {
	    FwinString->colorset = &Colorset[cs];
	    FwinString->flags.has_colorset = True;
    }
    FwinString->x = label_border;
    FwinString->y = FwindowFont->ascent+2;

    label_window_wrap(t);
  }
}

void LabelWindow(PagerWindow *t)
{
  do_label_window(t, t->PagerView);
}

void LabelIconWindow(PagerWindow *t)
{
  do_label_window(t, t->IconView);
}

static void do_picture_window(
	PagerWindow *t, Window w, int width, int height)
{
	int iconX;
	int iconY;
	int src_x = 0;
	int src_y = 0;
	int dest_w, dest_h;
	int cs;
	FvwmRenderAttributes fra;

	if (MiniIcons)
	{
		if (t->mini_icon.picture && w != None)
		{
			dest_w = t->mini_icon.width;
			dest_h = t->mini_icon.height;
			if (width > t->mini_icon.width)
			{
				iconX = (width - t->mini_icon.width) / 2;
			}
			else if (width < t->mini_icon.width)
			{
				iconX = 0;
				src_x = (t->mini_icon.width - width) / 2;
				dest_w = width;
			}
			else
			{
				iconX = 0;
			}
			if (height > t->mini_icon.height)
			{
				iconY = (height - t->mini_icon.height) / 2;
			}
			else if (height < t->mini_icon.height)
			{
				iconY = 0;
				src_y = (t->mini_icon.height - height) / 2;
				dest_h = height;
			}
			else
			{
				iconY = 0;
			}
			fra.mask = FRAM_DEST_IS_A_WINDOW;
			if (t == FocusWin)
			{
				cs =  activecolorset;
			}
			else
			{
				cs = windowcolorset;
			}
			if (t->mini_icon.alpha != None ||
			    (cs >= 0 &&
			     Colorset[cs].icon_alpha_percent < 100))
			{
				XClearArea(dpy, w, iconX, iconY,
					   t->mini_icon.width,
					   t->mini_icon.height, False);
			}
			if (cs >= 0)
			{
				fra.mask |= FRAM_HAVE_ICON_CSET;
				fra.colorset = &Colorset[cs];
			}
			PGraphicsRenderPicture(
				dpy, w, &t->mini_icon, &fra, w,
				Scr.MiniIconGC, None, None, src_x, src_y,
				t->mini_icon.width - src_x,
				t->mini_icon.height - src_y,
				iconX, iconY, dest_w, dest_h, False);
		}
	}
}

void PictureWindow (PagerWindow *t)
{
  do_picture_window(t, t->PagerView, t->pager_view_width, t->pager_view_height);
}

void PictureIconWindow (PagerWindow *t)
{
  do_picture_window(t, t->IconView, t->icon_view_width, t->icon_view_height);
}

void IconMoveWindow(XEvent *Event, PagerWindow *t)
{
	int x1, y1, finished = 0, n, x = 0, y = 0, xi = 0, yi = 0;
	Window dumwin;
	int m, n1, m1;
	int moved = 0;
	int KeepMoving = 0;
	Window JunkRoot, JunkChild;
	int JunkX, JunkY;
	unsigned JunkMask;

	if (t == NULL || !t->allowed_actions.is_movable)
	{
		return;
	}

	n = Scr.VxMax / Scr.MyDisplayWidth;
	m = Scr.VyMax / Scr.MyDisplayHeight;
	n1 = (Scr.Vx + t->x) / Scr.MyDisplayWidth;
	m1 = (Scr.Vy + t->y) / Scr.MyDisplayHeight;

	XRaiseWindow(dpy, t->IconView);

	XTranslateCoordinates(dpy, Event->xany.window, t->IconView,
			      Event->xbutton.x, Event->xbutton.y, &x1, &y1,
			      &dumwin);
	while (!finished)
	{
		FMaskEvent(dpy, ButtonReleaseMask | ButtonMotionMask |
			   ExposureMask, Event);

		if (Event->type == MotionNotify)
		{
			x = Event->xbutton.x;
			y = Event->xbutton.y;
			if (moved == 0)
			{
				xi = x;
				yi = y;
				moved = 1;
			}

			XMoveWindow(dpy, t->IconView, x - x1, y - y1);
			if (x < -5 || y < -5 || x > icon_w + 5 ||
			    y > icon_h + 5)
			{
				finished = 1;
				KeepMoving = 1;
			}
		}
		else if (Event->type == ButtonRelease)
		{
			x = Event->xbutton.x;
			y = Event->xbutton.y;
			XMoveWindow(dpy, t->PagerView, x - x1, y - y1);
			finished = 1;
		}
		else if (Event->type == Expose)
		{
			HandleExpose(Event);
		}
	}

	if (moved)
	{
		if (abs(x - xi) < MoveThreshold &&
		    abs(y - yi) < MoveThreshold)
		{
			moved = 0;
		}
	}

	if (KeepMoving)
	{
		if (FQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
				  &x, &y, &JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			x = 0;
			y = 0;
		}
		XUngrabPointer(dpy, CurrentTime);
		XSync(dpy, 0);
		SendText(fd, "Silent Raise", t->w);
		SendText(fd, "Silent Move Pointer", t->w);
	}
	else
	{
		x = x - x1;
		y = y - y1;
		n1 = x * Scr.VWidth / (icon_w * Scr.MyDisplayWidth);
		m1 = y * Scr.VHeight / (icon_h * Scr.MyDisplayHeight);
		x = (x - n1) * Scr.VWidth / (icon_w - n) - Scr.Vx;
		y = (y - m1) * Scr.VHeight / (icon_h - m) - Scr.Vy;
		/* force onto desk */
		if (x + t->icon_width + Scr.Vx < 0 )
		{
			x = - Scr.Vx - t->icon_width;
		}
		if (y + t->icon_height + Scr.Vy < 0)
		{
			y = - Scr.Vy - t->icon_height;
		}
		if (x + Scr.Vx >= Scr.VWidth)
		{
			x = Scr.VWidth - Scr.Vx - 1;
		}
		if (y + Scr.Vy >= Scr.VHeight)
		{
			y = Scr.VHeight - Scr.Vy - 1;
		}
		if ((IS_ICONIFIED(t) && IS_ICON_STICKY_ACROSS_DESKS(t)) ||
		   IS_STICKY_ACROSS_DESKS(t))
		{
			if (x > Scr.MyDisplayWidth - 16)
			{
				x = Scr.MyDisplayWidth - 16;
			}
			if (y > Scr.MyDisplayHeight - 16)
			{
				y = Scr.MyDisplayHeight - 16;
			}
			if (x + t->width < 16)
			{
				x = 16 - t->width;
			}
			if (y + t->height < 16)
			{
				y = 16 - t->height;
			}
		}
		if (moved)
		{
			char buf[64];
			sprintf(buf, "Silent Move +%dp +%dp", x, y);
			SendText(fd, buf, t->w);
			XSync(dpy, 0);
		}
		else
		{
			MoveResizePagerView(t, True);
		}
		SendText(fd, "Silent Raise", t->w);
		SendText(fd, "Silent FlipFocus NoWarp", t->w);
	}
	if (is_transient)
	{
		/* does not return */
		ExitPager();
	}
}


void setup_balloon_window(int i)
{
	XSetWindowAttributes xswa;
	unsigned long valuemask;

	if (!ShowBalloons && Scr.balloon_w == None)
	{
		return;
	}
	XUnmapWindow(dpy, Scr.balloon_w);
	if (Desks[i].ballooncolorset < 0)
	{
		xswa.border_pixel = (!BalloonBorderColor) ?
			fore_pix : GetColor(BalloonBorderColor);
		xswa.background_pixel =
			(!BalloonBack) ? back_pix : GetColor(BalloonBack);
		valuemask = CWBackPixel | CWBorderPixel;
	}
	else
	{
		xswa.border_pixel = Colorset[Desks[i].ballooncolorset].fg;
		xswa.background_pixel = Colorset[Desks[i].ballooncolorset].bg;
		if (Colorset[Desks[i].ballooncolorset].pixmap)
		{
			/* set later */
			xswa.background_pixmap = None;
			valuemask = CWBackPixmap | CWBorderPixel;;
		}
		else
		{
			valuemask = CWBackPixel | CWBorderPixel;
		}
	}
	XChangeWindowAttributes(dpy, Scr.balloon_w, valuemask, &xswa);

	return;
}

void setup_balloon_gc(int i, PagerWindow *t)
{
	XGCValues xgcv;
	unsigned long valuemask;

	valuemask = GCForeground;
	if (Desks[i].ballooncolorset < 0)
	{
		xgcv.foreground =
			(!BalloonFore) ? fore_pix : GetColor(BalloonFore);
		if (BalloonFore == NULL)
		{
			XSetForeground(dpy, Scr.balloon_gc, t->text);
		}
	}
	else
	{
		xgcv.foreground = Colorset[Desks[i].ballooncolorset].fg;
	}
	if (Desks[i].balloon.Ffont->font != NULL)
	{
		xgcv.font = Desks[i].balloon.Ffont->font->fid;
		valuemask |= GCFont;
	}
	XChangeGC(dpy, Scr.balloon_gc, valuemask, &xgcv);

	return;
}

/* Just maps window ... draw stuff in it later after Expose event
   -- ric@giccs.georgetown.edu */
void MapBalloonWindow(PagerWindow *t, Bool is_icon_view)
{
	extern char *BalloonBack;
	extern int BalloonYOffset;
	extern int BalloonBorderWidth;
	Window view, dummy;
	int view_width, view_height;
	int x, y;
	rectangle new_g;
	int i;

	if (!is_icon_view)
	{
		view = t->PagerView;
		view_width = t->pager_view_width;
		view_height = t->pager_view_height;
	}
	else
	{
		view = t->IconView;
		view_width = t->icon_view_width;
		view_height = t->icon_view_height;
	}
	BalloonView = view;
	/* associate balloon with its pager window */
	if (fAlwaysCurrentDesk)
	{
		i = 0;
	}
	else
	{
		i = t->desk - desk1;
	}

	if (i < 0)
		return;

	Scr.balloon_desk = i;
	/* get the label for this balloon */
	if (Scr.balloon_label)
	{
		free(Scr.balloon_label);
	}
	Scr.balloon_label = GetBalloonLabel(t, BalloonFormatString);
#if 0
	if (Scr.balloon_label == NULL)
	{
		/* dont draw empty labels */
		return;
	}
#endif
	setup_balloon_window(i);
	setup_balloon_gc(i, t);
	/* calculate window width to accommodate string */
	new_g.height = Desks[i].balloon.height;
	new_g.width = 4;
	if (Scr.balloon_label)
	{
		new_g.width += FlocaleTextWidth(
			Desks[i].balloon.Ffont,	 Scr.balloon_label,
			strlen(Scr.balloon_label));
	}
	/* get x and y coords relative to pager window */
	x = (view_width / 2) - (new_g.width / 2) - BalloonBorderWidth;
	if (BalloonYOffset > 0)
	{
		y = view_height + BalloonYOffset - 1;
	}
	else
	{
		y = BalloonYOffset - new_g.height + 1 -
			(2 * BalloonBorderWidth);
	}
	/* balloon is a top-level window, therefore need to
	   translate pager window coords to root window coords */
	XTranslateCoordinates(
		dpy, view, Scr.Root, x, y, &new_g.x, &new_g.y,
		&dummy);
	/* make sure balloon doesn't go off screen
	 * (actually 2 pixels from edge rather than 0 just to be pretty :-) */
	/* too close to top ... make yoffset +ve */
	if ( new_g.y < 2 )
	{
		y = - BalloonYOffset - 1 + view_height;
		XTranslateCoordinates(
			dpy, view, Scr.Root, x, y, &new_g.x, &new_g.y, &dummy);
	}
	/* too close to bottom ... make yoffset -ve */
	else if ( new_g.y + new_g.height >
		  Scr.MyDisplayHeight - (2 * BalloonBorderWidth) - 2 )
	{
		y = - BalloonYOffset + 1 - new_g.height -
			(2 * BalloonBorderWidth);
		XTranslateCoordinates(
			dpy, view, Scr.Root, x, y, &new_g.x, &new_g.y, &dummy);
	}
	/* too close to left */
	if ( new_g.x < 2 )
	{
		new_g.x = 2;
	}
	/* too close to right */
	else if (new_g.x + new_g.width >
		 Scr.MyDisplayWidth - (2 * BalloonBorderWidth) - 2 )
	{
		new_g.x = Scr.MyDisplayWidth - new_g.width -
			(2 * BalloonBorderWidth) - 2;
	}
	/* make changes to window */
	XMoveResizeWindow(
		dpy, Scr.balloon_w, new_g.x, new_g.y, new_g.width,
		new_g.height);
	/* if background not set in config make it match pager window */
	if ( BalloonBack == NULL )
	{
		XSetWindowBackground(dpy, Scr.balloon_w, t->back);
	}
	if (Desks[i].ballooncolorset > -1)
	{
		SetWindowBackground(
			dpy, Scr.balloon_w, new_g.width, new_g.height,
			&Colorset[Desks[i].ballooncolorset], Pdepth,
			Scr.NormalGC, True);
		XSetWindowBorder(
			dpy, Scr.balloon_w,
			Colorset[Desks[i].ballooncolorset].fg);
	}
	XMapRaised(dpy, Scr.balloon_w);

	return;
}


static void InsertExpand(char **dest, char *s)
{
  int len;
  char *tmp = *dest;

  if (!s || !*s)
    return;
  len = strlen(*dest) + strlen(s) + 1;
  *dest = xmalloc(len);
  strcpy(*dest, tmp);
  free(tmp);
  strcat(*dest, s);
  return;
}


/* Generate the BallonLabel from the format string
   -- disching@fmi.uni-passau.de */
char *GetBalloonLabel(const PagerWindow *pw,const char *fmt)
{
  char *dest = xstrdup("");
  const char *pos = fmt;
  char buffer[2];

  buffer[1] = '\0';

  while (*pos) {
    if (*pos=='%' && *(pos+1)!='\0') {
      pos++;
      switch (*pos) {
      case 'i':
	InsertExpand(&dest, pw->icon_name);
	break;
      case 't':
	InsertExpand(&dest, pw->window_name);
	break;
      case 'r':
	InsertExpand(&dest, pw->res_name);
	break;
      case 'c':
	InsertExpand(&dest, pw->res_class);
	break;
      case '%':
	buffer[0] = '%';
	InsertExpand(&dest, buffer);
	break;
      default:;
      }
    } else {
      buffer[0] = *pos;
      InsertExpand(&dest, buffer);
    }
    pos++;
  }
  return dest;
}


/* -- ric@giccs.georgetown.edu */
void UnmapBalloonWindow (void)
{
  XUnmapWindow(dpy, Scr.balloon_w);
  BalloonView = None;
}


/* Draws string in balloon window -- call after it's received Expose event
   -- ric@giccs.georgetown.edu */
void DrawInBalloonWindow (int i)
{
	FwinString->str = Scr.balloon_label;
	FwinString->win = Scr.balloon_w;
	FwinString->gc = Scr.balloon_gc;
	if (Desks[i].ballooncolorset >= 0)
	{
		FwinString->colorset = &Colorset[Desks[i].ballooncolorset];
		FwinString->flags.has_colorset = True;
	}
	else
	{
		FwinString->flags.has_colorset = False;
	}
	FwinString->x = 2;
	FwinString->y = Desks[i].balloon.Ffont->ascent;
	FlocaleDrawString(dpy, Desks[i].balloon.Ffont, FwinString, 0);

	return;
}

static void set_window_colorset_background(
  PagerWindow *t, colorset_t *csetp)
{
  if (t->PagerView != None)
  {
    SetWindowBackground(
      dpy, t->PagerView, t->pager_view_width, t->pager_view_height,
      csetp, Pdepth, Scr.NormalGC, True);
  }
  SetWindowBackground(
    dpy, t->IconView, t->icon_view_width, t->icon_view_height,
    csetp, Pdepth, Scr.NormalGC, True);

  return;
}

/* should be in sync with  draw_desk_background */
void change_colorset(int colorset)
{
  int i;
  PagerWindow *t;

  /* just in case */
  if (colorset < 0)
    return;

  XSetWindowBackgroundPixmap(dpy, Scr.Pager_w, default_pixmap);
  for(i=0;i<ndesks;i++)
  {
    if (Desks[i].highcolorset == colorset)
    {
      XSetForeground(dpy, Desks[i].HiliteGC,Colorset[colorset].bg);
      XSetForeground(dpy, Desks[i].rvGC, Colorset[colorset].fg);
      if (HilightDesks)
      {
	if (uselabel && (i == Scr.CurrentDesk))
	{
	  SetWindowBackground(
	    dpy, Desks[i].title_w, desk_w, desk_h, &Colorset[colorset], Pdepth,
	    Scr.NormalGC, True);
	}
	SetWindowBackground(
	  dpy, Desks[i].CPagerWin, 0, 0, &Colorset[colorset], Pdepth,
	  Scr.NormalGC, True);
	XLowerWindow(dpy,Desks[i].CPagerWin);
      }
    }

    if (Desks[i].colorset == colorset)
    {
      XSetWindowBorder(dpy, Desks[i].title_w, Colorset[colorset].fg);
      XSetWindowBorder(dpy, Desks[i].w, Colorset[colorset].fg);
      XSetForeground(dpy, Desks[i].NormalGC,Colorset[colorset].fg);
      XSetForeground(dpy, Desks[i].DashedGC,Colorset[colorset].fg);
      if (uselabel)
      {
	SetWindowBackground(
	  dpy, Desks[i].title_w, desk_w, desk_h + label_h, &Colorset[colorset],
	  Pdepth, Scr.NormalGC, True);
      }
      if (label_h != 0 && uselabel && !LabelsBelow &&
	  !CSET_IS_TRANSPARENT_PR(Desks[i].colorset))
      {
	SetWindowBackgroundWithOffset(
	  dpy, Desks[i].w, 0, -label_h, desk_w, desk_h + label_h,
	  &Colorset[colorset], Pdepth, Scr.NormalGC, True);
      }
      else
      {
	SetWindowBackground(
	  dpy, Desks[i].w, desk_w, desk_h + label_h,
	  &Colorset[Desks[i].colorset], Pdepth, Scr.NormalGC, True);
      }
      update_pr_transparent_subwindows(i);
    }
    else if (Desks[i].highcolorset == colorset && uselabel)
    {
      SetWindowBackground(
	dpy, Desks[i].title_w, 0, 0, &Colorset[Desks[i].colorset], Pdepth,
	Scr.NormalGC, True);
    }
  }
  if (ShowBalloons && Desks[Scr.balloon_desk].ballooncolorset == colorset)
  {
    XSetWindowBackground(dpy, Scr.balloon_w, Colorset[colorset].fg);
    XSetForeground(dpy,Scr.balloon_gc, Colorset[colorset].fg);
    XClearArea(dpy, Scr.balloon_w, 0, 0, 0, 0, True);
  }

  if (windowcolorset == colorset)
  {
    colorset_t *csetp = &Colorset[colorset];

    XSetForeground(dpy, Scr.whGC, csetp->hilite);
    XSetForeground(dpy, Scr.wsGC, csetp->shadow);
    win_back_pix = csetp->bg;
    win_fore_pix = csetp->fg;
    win_pix_set = True;
    for (t = Start; t != NULL; t = t->next)
    {
      t->text = Colorset[colorset].fg;
      if (t != FocusWin)
      {
	set_window_colorset_background(t, csetp);
      }
    }
  }
  if (activecolorset == colorset)
  {
    colorset_t *csetp = &Colorset[colorset];

    focus_fore_pix = csetp->fg;
    XSetForeground(dpy, Scr.ahGC, csetp->hilite);
    XSetForeground(dpy, Scr.asGC, csetp->shadow);
    win_hi_back_pix = csetp->bg;
    win_hi_fore_pix = csetp->fg;
    win_hi_pix_set = True;
    if (FocusWin)
    {
      set_window_colorset_background(FocusWin, csetp);
    }
  }

  return;
}
