/****************************************************************************
 * This module is all new
 * by Rob Nation
 ****************************************************************************/
/***********************************************************************
 *
 * fvwm pager handling code
 *
 ***********************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "../../libs/fvwmlib.h"
#include "../../fvwm/fvwm.h"
#include "FvwmPager.h"

extern ScreenInfo Scr;
extern Display *dpy;

Pixel back_pix, fore_pix, hi_pix;
Pixel focus_pix;
Pixel focus_fore_pix;
extern Pixel win_back_pix, win_fore_pix, win_hi_back_pix, win_hi_fore_pix;
extern int window_w, window_h,window_x,window_y,usposition,uselabel,xneg,yneg;
extern int StartIconic;
extern int MiniIcons;
extern int ShowBalloons, ShowPagerBalloons, ShowIconBalloons;

extern int icon_w, icon_h, icon_x, icon_y;
XFontStruct *font, *windowFont;

GC NormalGC,DashedGC,HiliteGC,rvGC;
GC StdGC;
GC MiniIconGC;
GC BalloonGC;

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
XErrorHandler FvwmErrorHandler(Display *, XErrorEvent *);


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


Window		icon_win;               /* icon window */
BalloonWindow balloon;            /* balloon window */

/***********************************************************************
 *
 *  Procedure:
 *	Initialize_pager - creates the pager window, if needed
 *
 *  Inputs:
 *	x,y location of the window
 *
 ***********************************************************************/
char *pager_name = "Fvwm Pager";
XSizeHints sizehints =
{
  (PMinSize | PResizeInc | PBaseSize | PWinGravity),
  0, 0, 100, 100,	                /* x, y, width and height */
  1, 1,		                        /* Min width and height */
  0, 0,		                        /* Max width and height */
  1, 1,	                         	/* Width and height increments */
  {0, 0}, {0, 0},                       /* Aspect ratio - not used */
  1, 1,                 		/* base size */
  (NorthWestGravity)                    /* gravity */
};

void initialize_pager(void)
{
  XWMHints wmhints;
  XClassHint class1;

  XTextProperty name;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  extern char *PagerFore, *PagerBack, *HilightC;
  extern char *WindowBack, *WindowFore, *WindowHiBack, *WindowHiFore;
  extern char *BalloonFore, *BalloonBack, *BalloonFont;
  extern char *BalloonBorderColor;
  extern int BalloonBorderWidth, BalloonYOffset;
  extern char *font_string, *smallFont;
  int n,m,w,h,i,x,y;
  XGCValues gcv;
  unsigned long gcm;

#if 0
  /* I don't think that this is necessary - just let pager die */
  XSetErrorHandler((XErrorHandler)FvwmErrorHandler);
#endif /* 0 */

  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);

  /* load the font */
  if (!uselabel || ((font = XLoadQueryFont(dpy, font_string)) == NULL))
    {
      if ((font = XLoadQueryFont(dpy, "fixed")) == NULL)
	{
	  fprintf(stderr,"%s: No fonts available\n",MyName);
	  exit(1);
	}
    };
  if(uselabel)
    label_h = font->ascent + font->descent+2;
  else
    label_h = 0;


  if(smallFont!= NULL)
    {
      windowFont= XLoadQueryFont(dpy, smallFont);
    }
  else
    windowFont= NULL;

  /* Load the colors */
  fore_pix = GetColor(PagerFore);
  back_pix = GetColor(PagerBack);
  hi_pix = GetColor(HilightC);

  if (WindowBack && WindowFore && WindowHiBack && WindowHiFore)
  {
  	win_back_pix	= GetColor (WindowBack);
  	win_fore_pix	= GetColor (WindowFore);
  	win_hi_back_pix	= GetColor (WindowHiBack);
  	win_hi_fore_pix	= GetColor (WindowHiFore);
  }
  /* Load pixmaps for mono use */
  if(Scr.d_depth<2)
    {
      Scr.gray_pixmap =
	XCreatePixmapFromBitmapData(dpy,Scr.Root,g_bits, g_width,g_height,
				    fore_pix,back_pix,Scr.d_depth);
      Scr.light_gray_pixmap =
	XCreatePixmapFromBitmapData(dpy,Scr.Root,l_g_bits,l_g_width,l_g_height,
				    fore_pix,back_pix,Scr.d_depth);
      Scr.sticky_gray_pixmap =
	XCreatePixmapFromBitmapData(dpy,Scr.Root,s_g_bits,s_g_width,s_g_height,
				    fore_pix,back_pix,Scr.d_depth);
    }


  n = Scr.VxMax/Scr.MyDisplayWidth;
  m = Scr.VyMax/Scr.MyDisplayHeight;

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
  if(window_w > 0)
    {
      window_w = ((window_w - n)/(n+1))*(n+1)+n;
      Scr.VScale = Columns*(Scr.VxMax + Scr.MyDisplayWidth)/
	(window_w-Columns+1-Columns*n);
    }
  if(window_h > 0)
    {
      window_h = ((window_h - m)/(m+1))*(m+1)+m;
      Scr.VScale = Rows*(Scr.VyMax + Scr.MyDisplayHeight)/
	(window_h+2-Rows*(label_h -m-1));
    }
  if(window_w <= 0)
    window_w = Columns*((Scr.VxMax + Scr.MyDisplayWidth)/Scr.VScale + n) +
      Columns-1;
  if(window_h <= 0)
    {
      window_h = Rows*((Scr.VyMax + Scr.MyDisplayHeight)/Scr.VScale
		       + m + label_h + 1)-2;
    }

  if(xneg)
    {
      sizehints.win_gravity = NorthEastGravity;
      window_x = Scr.MyDisplayWidth - window_w + window_x -2;
    }

  if(yneg)
    {
      window_y = Scr.MyDisplayHeight - window_h + window_y -2;
      if(sizehints.win_gravity == NorthEastGravity)
	sizehints.win_gravity = SouthEastGravity;
      else
	sizehints.win_gravity = SouthWestGravity;
    }

  if(usposition)
    sizehints.flags |= USPosition;

  valuemask = (CWBackPixel | CWBorderPixel | CWEventMask);
  attributes.background_pixel = back_pix;
  attributes.border_pixel = fore_pix;
  attributes.event_mask = (StructureNotifyMask);
  sizehints.width = window_w;
  sizehints.height = window_h;
  sizehints.x = window_x;
  sizehints.y = window_y;
  sizehints.width_inc = Columns*(n+1);
  sizehints.height_inc = Rows*(m+1);
  sizehints.base_width = Columns * n + Columns - 1;
  sizehints.base_height = Rows*(m + label_h+1) -2;
  sizehints.min_width = Columns * n + Columns - 1;
  sizehints.min_height = Rows*(m + label_h+1) -2;

  Scr.Pager_w = XCreateWindow (dpy, Scr.Root, window_x, window_y, window_w,
			       window_h, (unsigned int) 1,
			       CopyFromParent, InputOutput,
			       (Visual *) CopyFromParent,
			       valuemask, &attributes);
  XSetWMProtocols(dpy,Scr.Pager_w,&wm_del_win,1);
  XSetWMNormalHints(dpy,Scr.Pager_w,&sizehints);

  if((desk1==desk2)&&(Desks[0].label != NULL))
    XStringListToTextProperty(&Desks[0].label,1,&name);
  else
    XStringListToTextProperty(&pager_name,1,&name);

  attributes.event_mask = (StructureNotifyMask| ExposureMask);
  if(icon_w < 1)
    icon_w = (window_w - Columns+1)/Columns;
  if(icon_h < 1)
    icon_h = (window_h - Rows* label_h - Rows + 1)/Rows;

  icon_w = (icon_w / (n+1)) *(n+1)+n;
  icon_h = (icon_h / (m+1)) *(m+1)+m;
  icon_win = XCreateWindow (dpy, Scr.Root, window_x, window_y,
			    icon_w,icon_h,
			    (unsigned int) 1,
			    CopyFromParent, InputOutput,
			    (Visual *) CopyFromParent,
			    valuemask, &attributes);
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
  if(icon_x > -10000)
    {
      if(icon_x < 0)
	icon_x = Scr.MyDisplayWidth + icon_x - icon_w;
      if(icon_y > -10000)
	{
	  if(icon_y < 0)
	    icon_y = Scr.MyDisplayHeight + icon_y - icon_h;
	}
      else
	icon_y = 0;
      wmhints.icon_x = icon_x;
      wmhints.icon_y = icon_y;
      wmhints.flags = IconPositionHint;
    }
  wmhints.icon_window = icon_win;
  wmhints.input = False;

  wmhints.flags |= InputHint | StateHint | IconWindowHint;

  class1.res_name = MyName;
  class1.res_class = "FvwmModule";

  XSetWMProperties(dpy,Scr.Pager_w,&name,&name,NULL,0,
		   &sizehints,&wmhints,&class1);
  XFree((char *)name.value);

  for(i=0;i<ndesks;i++)
    {
      w = window_w/ndesks;
      h = window_h;
      x = w*i;
      y = 0;

      valuemask = (CWBackPixel | CWBorderPixel | CWEventMask);
#if 0
      attributes.background_pixel = back_pix;
#else
      attributes.background_pixel = GetColor(Desks[i].Dcolor);
#endif
      attributes.border_pixel = fore_pix;
      attributes.event_mask = (ExposureMask | ButtonReleaseMask);
      Desks[i].title_w = XCreateWindow(dpy,Scr.Pager_w, x, y, w, h,1,
				       CopyFromParent,
				       InputOutput,CopyFromParent,
				       valuemask,
				       &attributes);
      attributes.event_mask = (ExposureMask | ButtonReleaseMask |
			       ButtonPressMask |ButtonMotionMask);
      desk_h = window_h - label_h;
      Desks[i].w = XCreateWindow(dpy,Desks[i].title_w, x, y, w, desk_h ,1,
				 CopyFromParent,
				 InputOutput,CopyFromParent,
				 valuemask,&attributes);


      attributes.event_mask = 0;
      attributes.background_pixel = hi_pix;

      w = (window_w - n)/(n+1);
      h = (window_h - label_h - m)/(m+1);
      Desks[i].CPagerWin=XCreateWindow(dpy,Desks[i].w,-1000, -1000, w, h,0,
				       CopyFromParent,
				       InputOutput,CopyFromParent,
				       valuemask,
				       &attributes);
      XMapRaised(dpy,Desks[i].CPagerWin);
      XMapRaised(dpy,Desks[i].w);
      XMapRaised(dpy,Desks[i].title_w);
    }
  XMapRaised(dpy,Scr.Pager_w);

  gcm = GCForeground|GCBackground|GCFont;
  gcv.foreground = fore_pix;
  gcv.background = back_pix;

  gcv.font =  font->fid;
  NormalGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
  MiniIconGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = hi_pix;
  if(Scr.d_depth < 2)
    {
      gcv.foreground = fore_pix;
      gcv.background = back_pix;
    }
  HiliteGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  if((Scr.d_depth < 2)||(fore_pix == hi_pix))
    gcv.foreground = back_pix;
  else
    gcv.foreground = fore_pix;
  rvGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  if(windowFont != NULL)
    {
      /* Create GC's for doing window labels */
      gcv.foreground = focus_fore_pix;
      gcv.background = focus_pix;
      gcv.font =  windowFont->fid;
      StdGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
    }

  gcm = gcm | GCLineStyle;
  gcv.foreground = fore_pix;
  gcv.background = back_pix;
  gcv.line_style = LineOnOffDash;
  DashedGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);


  /* create balloon window
     -- ric@giccs.georgetown.edu */
  if ( ShowBalloons ) {

    valuemask = CWOverrideRedirect | CWEventMask | CWBackPixel | CWBorderPixel;

    /* tell WM to ignore this window */
    attributes.override_redirect = True;

    attributes.event_mask = ExposureMask;
    attributes.border_pixel = GetColor(BalloonBorderColor);

    /* if given in config set this now, otherwise it'll be set for each
       pager window when drawn later */
    attributes.background_pixel =
      (BalloonBack == NULL) ? 0 : GetColor(BalloonBack);

    /* get font for balloon */
    if ( (balloon.font = XLoadQueryFont(dpy, BalloonFont)) == NULL ) {
      if ( (balloon.font = XLoadQueryFont(dpy, "fixed")) == NULL ) {
        fprintf(stderr,"%s: No fonts available.\n", MyName);
        exit(1);
      }
      fprintf(stderr, "%s: Can't find font '%s', using fixed.\n",
              MyName, BalloonFont);
    }

    balloon.height = balloon.font->ascent + balloon.font->descent + 1;

    /* this may have been set in config */
    balloon.border = BalloonBorderWidth;


    /* we don't allow yoffset of 0 because it allows direct transit
       from pager window to balloon window, setting up a
       LeaveNotify/EnterNotify event loop */
    if ( BalloonYOffset )
      balloon.yoffset = BalloonYOffset;
    else {
      fprintf(stderr,
       "%s: Warning: you're not allowed BalloonYOffset 0; defaulting to +2\n",
              MyName);
      balloon.yoffset = 2;
    }

    /* now create the window */
    balloon.w = XCreateWindow(dpy, Scr.Root,
                              0, 0,            /* coords set later */
                              1,               /* width set later */
                              balloon.height,
                              balloon.border,
                              CopyFromParent,
                              InputOutput,
                              CopyFromParent,
                              valuemask,
                              &attributes);

    /* set font */
    gcv.font = balloon.font->fid;

    /* if fore given in config set now, otherwise it'll be set later */
    gcv.foreground = (BalloonFore == NULL) ? 0 : GetColor(BalloonFore);

    BalloonGC = XCreateGC(dpy, balloon.w, GCFont | GCForeground, &gcv);

    /* Make sure we don't get balloons initially with the Icon option. */
    ShowBalloons = ShowPagerBalloons;
  } /* ShowBalloons */
}



/****************************************************************************
 *
 * Loads a single color
 *
 ****************************************************************************/
Pixel GetColor(char *name)
{
  XColor color;
  XWindowAttributes attributes;

  XGetWindowAttributes(dpy,Scr.Root,&attributes);
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


void nocolor(char *a, char *b)
{
  fprintf(stderr,"%s: can't %s %s\n", MyName, a,b);
}

/****************************************************************************
 *
 * Decide what to do about received X events
 *
 ****************************************************************************/
void DispatchEvent(XEvent *Event)
{
  int i,x,y;
  Window JunkRoot, JunkChild;
  int JunkX, JunkY;
  unsigned JunkMask;

  switch(Event->xany.type)
    {
    case EnterNotify:
      if ( ShowBalloons )
        MapBalloonWindow(Event);
      break;
    case LeaveNotify:
      if ( ShowBalloons )
        UnmapBalloonWindow();
      break;
    case ConfigureNotify:
      ReConfigure();
      break;
    case Expose:
      HandleExpose(Event);
      break;
    case ButtonRelease:
      if((Event->xbutton.button == 1)||
	 (Event->xbutton.button == 2))
	{
	  for(i=0;i<ndesks;i++)
	    {
	      if(Event->xany.window == Desks[i].w)
		SwitchToDeskAndPage(i,Event);
	      if(Event->xany.window == Desks[i].title_w)
		SwitchToDesk(i);
	    }
	  if(Event->xany.window == icon_win)
	    {
	      IconSwitchPage(Event);
	    }
	}
      else if (Event->xbutton.button == 3)
	{
	  for(i=0;i<ndesks;i++)
	    {
	      if(Event->xany.window == Desks[i].w)
		{
		  XQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
				&JunkX, &JunkY,&x, &y, &JunkMask);
		  Scroll(desk_w, desk_h, x, y, i);
		}
	    }
	  if(Event->xany.window == icon_win)
	    {
	      XQueryPointer(dpy, icon_win, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask);
	      Scroll(icon_w, icon_h, x, y, -1);
	    }
	}
      break;
    case ButtonPress:
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
	  for(i=0;i<ndesks;i++)
	    {
	      if(Event->xany.window == Desks[i].w)
		{
		  if (Scr.CurrentDesk != i + desk1)
		    {
		      SwitchToDeskAndPage(i,Event);
		      Scr.CurrentDesk = i + desk1;
		      Wait = 0;
		    }
		  XQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
				&JunkX, &JunkY,&x, &y, &JunkMask);
		  Scroll(desk_w, desk_h, x, y, i);
		  break;
		}
	    }
	  if(Event->xany.window == icon_win)
	    {
	      XQueryPointer(dpy, icon_win, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask);
	      Scroll(icon_w, icon_h, x, y, -1);
	    }
	}
      break;
    case MotionNotify:
      while(XCheckMaskEvent(dpy, PointerMotionMask | ButtonMotionMask,Event));

      if(Event->xmotion.state == Button3MotionMask)
	{
	  for(i=0;i<ndesks;i++)
	    {
	      if(Event->xany.window == Desks[i].w)
		{
		  XQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
				    &JunkX, &JunkY,&x, &y, &JunkMask);
		  Scroll(desk_w, desk_h, x, y, i);
		}
	    }
	  if(Event->xany.window == icon_win)
	    {
	      XQueryPointer(dpy, icon_win, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask);
	      Scroll(icon_w, icon_h, x, y, -1);
	    }

	}
      break;

    case ClientMessage:
      if ((Event->xclient.format==32) &&
	  (Event->xclient.data.l[0]==wm_del_win))
	{
	  exit(0);
	}
      break;
    }
}

void HandleExpose(XEvent *Event)
{
  int i;
  PagerWindow *t;

  /* ric@giccs.georgetown.edu */
  if ( Event->xany.window == balloon.w ) {
    DrawInBalloonWindow();
    return;
  }

  for(i=0;i<ndesks;i++)
    {
      if((Event->xany.window == Desks[i].w)
	 ||(Event->xany.window == Desks[i].title_w))
	DrawGrid(i,0);
    }
  if(Event->xany.window == icon_win)
    DrawIconGrid(0);

  t = Start;
  while(t!= NULL)
    {
      if(t->PagerView == Event->xany.window)
	{
	  LabelWindow(t);
	  PictureWindow(t);
	}
      else if(t->IconView == Event->xany.window)
	{
	  LabelIconWindow(t);
	  PictureIconWindow(t);
	}

      t = t->next;
    }
}


/****************************************************************************
 *
 * Respond to a change in window geometry.
 *
 ****************************************************************************/
void ReConfigure(void)
{
  Window root;
  unsigned border_width, depth;
  int n,m,w,h,n1,m1,x,y,i,j,k;


  XGetGeometry(dpy,Scr.Pager_w,&root,&x,&y,
	       (unsigned *)&window_w,(unsigned *)&window_h,
	       &border_width,&depth);


  n1 = Scr.Vx/Scr.MyDisplayWidth;
  m1 = Scr.Vy/Scr.MyDisplayHeight;
  n = (Scr.VxMax)/Scr.MyDisplayWidth;
  m = (Scr.VyMax)/Scr.MyDisplayHeight;
  desk_w = (window_w - Columns + 1)/Columns;
  desk_h = (window_h - Rows*label_h - Rows + 2)/Rows;
  w = (desk_w - n)/(n+1);
  h = (desk_h - m)/(m+1);

  sizehints.width_inc = Columns*(n+1);
  sizehints.height_inc = Rows*(m+1);
  sizehints.base_width = Columns * n + Columns - 1;
  sizehints.base_height = Rows*(m + label_h+1) -2;
  sizehints.min_width = Columns * n + Columns - 1;
  sizehints.min_height = Rows*(m + label_h+1) -2;

  XSetWMNormalHints(dpy,Scr.Pager_w,&sizehints);

  x = (desk_w-n)* Scr.Vx/(Scr.VxMax+Scr.MyDisplayWidth) +n1;
  y = (desk_h-m)*Scr.Vy/(Scr.VyMax+Scr.MyDisplayHeight) +m1;

  for(k=0;k<Rows;k++)
    {
      for(j=0;j<Columns;j++)
	{
	  i = k*Columns+j;

	  if(i<ndesks)
	    {
	      XMoveResizeWindow(dpy,Desks[i].title_w,
				(desk_w+1)*j-1,(desk_h+label_h+1)*k-1,
				desk_w,desk_h+label_h);
	      XMoveResizeWindow(dpy,Desks[i].w,-1,label_h - 1,
				desk_w,desk_h);
	      if(i == Scr.CurrentDesk - desk1)
		XMoveResizeWindow(dpy, Desks[i].CPagerWin, x,y,w,h);
	      else
		XMoveResizeWindow(dpy, Desks[i].CPagerWin, -1000,-100,w,h);

	      XClearArea(dpy,Desks[i].w, 0, 0, desk_w,
			 desk_h,True);
	      if(uselabel)
		XClearArea(dpy,Desks[i].title_w, 0, 0, desk_w,
			   label_h,True);
	    }
	}
    }
  /* reconfigure all the subordinate windows */
  ReConfigureAll();
}

void MovePage(void)
{
  int n1,m1,x,y,n,m,i;
  XTextProperty name;
  char str[100],*sptr;
  static int icon_desk_shown = -1000;

  Wait = 0;
  n1 = Scr.Vx/Scr.MyDisplayWidth;
  m1 = Scr.Vy/Scr.MyDisplayHeight;
  n = (Scr.VxMax)/Scr.MyDisplayWidth;
  m = (Scr.VyMax)/Scr.MyDisplayHeight;

  x = (desk_w-n)* Scr.Vx/(Scr.VxMax+Scr.MyDisplayWidth) +n1;
  y = (desk_h-m)*Scr.Vy/(Scr.VyMax+Scr.MyDisplayHeight) +m1;
  for(i=0;i<ndesks;i++)
    {
      if(i == Scr.CurrentDesk - desk1)
	{
	  XMoveWindow(dpy, Desks[i].CPagerWin, x,y);
	  XLowerWindow(dpy,Desks[i].CPagerWin);
	}
      else
	XMoveWindow(dpy, Desks[i].CPagerWin, -1000,-1000);
    }
  DrawIconGrid(1);

  ReConfigureIcons();

  if(Scr.CurrentDesk != icon_desk_shown)
    {
      icon_desk_shown = Scr.CurrentDesk;

      if((Scr.CurrentDesk >= desk1)&&(Scr.CurrentDesk <=desk2))
	sptr = Desks[Scr.CurrentDesk -desk1].label;
      else
	{
	  sprintf(str,"Desk %d",Scr.CurrentDesk);
	  sptr = &str[0];
	}
      if (XStringListToTextProperty(&sptr,1,&name) == 0)
	{
	  fprintf(stderr,"%s: cannot allocate window name",MyName);
	  return;
	}
      XSetWMIconName(dpy,Scr.Pager_w,&name);
    }
}

void ReConfigureAll(void)
{
  PagerWindow *t;

  t = Start;
  while(t!= NULL)
    {
      MoveResizePagerView(t);
      t = t->next;
    }
}

void ReConfigureIcons(void)
{
  PagerWindow *t;
  int x,y,w,h,n,m,n1,m1;

  n = (Scr.VxMax)/Scr.MyDisplayWidth;
  m = (Scr.VyMax)/Scr.MyDisplayHeight;

  t = Start;
  while(t!= NULL)
    {
      n1 = (Scr.Vx+t->x)/Scr.MyDisplayWidth;
      m1 = (Scr.Vy+t->y)/Scr.MyDisplayHeight;
      x = (Scr.Vx + t->x)*(icon_w-n)/(Scr.VxMax + Scr.MyDisplayWidth) +n1;
      y = (Scr.Vy + t->y)*(icon_h-m)/(Scr.VyMax + Scr.MyDisplayHeight)+m1;
      w = (Scr.Vx + t->x + t->width+2)*(icon_w-n)/
	(Scr.VxMax + Scr.MyDisplayWidth) - 2 - x + n1;
      h = (Scr.Vy + t->y + t->height+2)*(icon_h-m)/
	(Scr.VyMax + Scr.MyDisplayHeight) -2 - y +m1;

      if (w < 1)
	w = 1;
      if (h < 1)
	h = 1;

      t->icon_view_width      = w;
      t->icon_view_height     = h;
      if(Scr.CurrentDesk == t->desk)
	XMoveResizeWindow(dpy,t->IconView,x,y,w,h);
      else
	XMoveResizeWindow(dpy,t->IconView,-1000,-1000,w,h);
      t = t->next;
    }
}

/****************************************************************************
 *
 * Draw grid lines for desk #i
 *
 ****************************************************************************/
void DrawGrid(int i, int erase)
{
  int y, y1, y2, x, x1, x2,d,hor_off,w;
  int MaxW,MaxH;
  char str[15], *ptr;

  if((i < 0 ) ||(i >= ndesks))
    return;

  MaxW = (Scr.VxMax + Scr.MyDisplayWidth);
  MaxH = Scr.VyMax + Scr.MyDisplayHeight;

  x = Scr.MyDisplayWidth;
  y1 = 0;
  y2 = desk_h;
  while(x < MaxW)
    {
      x1 = x*desk_w/MaxW;
      XDrawLine(dpy,Desks[i].w,DashedGC,x1,y1,x1,y2);
      x += Scr.MyDisplayWidth;
    }

  y = Scr.MyDisplayHeight;
  x1 = 0;
  x2 = desk_w;
  while(y < MaxH)
    {
      y1 = y*(desk_h)/MaxH;
      XDrawLine(dpy,Desks[i].w,DashedGC,x1,y1,x2,y1);
      y += Scr.MyDisplayHeight;
    }
  if((Scr.CurrentDesk - desk1)  == i)
    {
      if(uselabel)
	XFillRectangle(dpy,Desks[i].title_w,HiliteGC,
		       0,0,desk_w,label_h -1);
    }
  else
    {
      if(uselabel && erase)
	XClearArea(dpy,Desks[i].title_w,
		   0,0,desk_w,label_h - 1,False);
    }

  d = desk1+i;
  ptr = Desks[i].label;
  w=XTextWidth(font,ptr,strlen(ptr));
  if( w > desk_w)
    {
      sprintf(str,"%d",d);
      ptr = str;
      w=XTextWidth(font,ptr,strlen(ptr));
    }
  if((w<= desk_w)&&(uselabel))
    {
      hor_off = (desk_w -w)/2;
      if(i == (Scr.CurrentDesk - desk1))
	XDrawString (dpy, Desks[i].title_w,rvGC,hor_off,font->ascent +1 ,
		     ptr, strlen(ptr));
      else
	XDrawString (dpy, Desks[i].title_w,NormalGC,hor_off,font->ascent+1  ,
		     ptr, strlen(ptr));
    }
}


void DrawIconGrid(int erase)
{
  int y, y1, y2, x, x1, x2,w,h,n,m,n1,m1;
  int MaxW,MaxH;

  MaxW = (Scr.VxMax + Scr.MyDisplayWidth);
  MaxH = Scr.VyMax + Scr.MyDisplayHeight;

  if(erase)
    XClearWindow(dpy,icon_win);
  x = Scr.MyDisplayWidth;
  y1 = 0;
  y2 = icon_h;
  while(x < MaxW)
    {
      x1 = x*icon_w/MaxW;
      XDrawLine(dpy,icon_win,DashedGC,x1,y1,x1,y2);
      x += Scr.MyDisplayWidth;
    }

  y = Scr.MyDisplayHeight;
  x1 = 0;
  x2 = icon_w;
  while(y < MaxH)
    {
      y1 = y*(icon_h)/MaxH;
      XDrawLine(dpy,icon_win,DashedGC,x1,y1,x2,y1);
      y += Scr.MyDisplayHeight;
    }
  n1 = Scr.Vx/Scr.MyDisplayWidth;
  m1 = Scr.Vy/Scr.MyDisplayHeight;
  n = (Scr.VxMax)/Scr.MyDisplayWidth;
  m = (Scr.VyMax)/Scr.MyDisplayHeight;
  w = (icon_w - n)/(n+1);
  h = (icon_h - m)/(m+1);

  x = (icon_w-n)* Scr.Vx/(Scr.VxMax+Scr.MyDisplayWidth) +n1;
  y = (icon_h-m)*Scr.Vy/(Scr.VyMax+Scr.MyDisplayHeight) +m1;

  XFillRectangle(dpy,icon_win,HiliteGC,
		 x,y,w,h);

}


void SwitchToDesk(int Desk)
{
  char command[256];

  sprintf(command,"Desk 0 %d\n",Desk+desk1);

  SendInfo(fd,command,0);
}


void SwitchToDeskAndPage(int Desk, XEvent *Event)
{
#ifndef NON_VIRTUAL
  char command[256];

  if (Scr.CurrentDesk != (Desk+desk1))
    {
      int vx, vy;
      SendInfo(fd,"Desk 0 10000\n",0);
      /* patch to let mouse button 3 change desks and do not cling to a page */
      vx = Event->xbutton.x*(Scr.VxMax+Scr.MyDisplayWidth)/
	(desk_w*Scr.MyDisplayWidth);
      vy = Event->xbutton.y*(Scr.VyMax+Scr.MyDisplayHeight)/
	(desk_h*Scr.MyDisplayHeight);
      Scr.Vx = vx * Scr.MyDisplayWidth;
      Scr.Vy = vy * Scr.MyDisplayHeight;
      sprintf(command,"GotoPage %d %d\n", vx, vy);
      SendInfo(fd,command,0);
      sprintf(command,"Desk 0 %d\n",Desk+desk1);
      SendInfo(fd,command,0);

    }
  else
    {
      sprintf(command,"GotoPage %d %d\n",
	      Event->xbutton.x*(Scr.VxMax+Scr.MyDisplayWidth)/
	      (desk_w*Scr.MyDisplayWidth),
	      Event->xbutton.y*(Scr.VyMax+Scr.MyDisplayHeight)/
	      (desk_h*Scr.MyDisplayHeight));
      SendInfo(fd,command,0);
    }
#endif
  Wait = 1;
}

void IconSwitchPage(XEvent *Event)
{
#ifndef NON_VIRTUAL
  char command[256];

  sprintf(command,"GotoPage %d %d\n",
	  Event->xbutton.x*(Scr.VxMax+Scr.MyDisplayWidth)/
	  (icon_w*Scr.MyDisplayWidth),
	  Event->xbutton.y*(Scr.VyMax+Scr.MyDisplayHeight)/
	  (icon_h*Scr.MyDisplayHeight));
  SendInfo(fd,command,0);
#endif
  Wait = 1;
}


void AddNewWindow(PagerWindow *t)
{
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  int i,x,y,w,h,n,m,n1,m1;

  i = t->desk - desk1;
  n = (Scr.VxMax)/Scr.MyDisplayWidth;
  m = (Scr.VyMax)/Scr.MyDisplayHeight;
  n1 = (Scr.Vx+t->x)/Scr.MyDisplayWidth;
  m1 = (Scr.Vy+t->y)/Scr.MyDisplayHeight;
  x = (Scr.Vx + t->x)*(desk_w-n)/(Scr.VxMax + Scr.MyDisplayWidth) +n1;
  y = (Scr.Vy + t->y)*(desk_h-m)/(Scr.VyMax + Scr.MyDisplayHeight)+m1;
  w = (Scr.Vx + t->x + t->width+2)*(desk_w-n)/
    (Scr.VxMax + Scr.MyDisplayWidth) - 2 - x + n1;
  h = (Scr.Vy + t->y + t->height+2)*(desk_h-m)/
    (Scr.VyMax + Scr.MyDisplayHeight) -2 - y +m1;
  if(w<1)
    w = 1;
  if(h<1)
    h = 1;

	t->pager_view_width		= w;
	t->pager_view_height	= h;
  valuemask = (CWBackPixel | CWBorderPixel | CWEventMask);
  attributes.background_pixel = t->back;
  attributes.border_pixel = fore_pix;
  attributes.event_mask = (ExposureMask);

  /* ric@giccs.georgetown.edu -- added Enter and Leave events for
     popping up balloon window */
  attributes.event_mask = (ExposureMask | EnterWindowMask | LeaveWindowMask);

  if((i >= 0)&& (i <ndesks))
    {
      t->PagerView = XCreateWindow(dpy,Desks[i].w, x, y, w, h,1,
				   CopyFromParent,
				   InputOutput,CopyFromParent,
				   valuemask,&attributes);

      XMapRaised(dpy,t->PagerView);
    }
  else
    t->PagerView = None;


  x = (Scr.Vx + t->x)*(icon_w-n)/(Scr.VxMax + Scr.MyDisplayWidth) +n1;
  y = (Scr.Vy + t->y)*(icon_h-m)/(Scr.VyMax + Scr.MyDisplayHeight)+m1;
  w = (Scr.Vx + t->x + t->width+2)*(icon_w-n)/
    (Scr.VxMax + Scr.MyDisplayWidth) - 2 - x + n1;
  h = (Scr.Vy + t->y + t->height+2)*(icon_h-m)/
    (Scr.VyMax + Scr.MyDisplayHeight) -2 - y +m1;
  if(w<1)
    w = 1;
  if(h<1)
    h = 1;

	t->icon_view_width	= w;
	t->icon_view_height	= h;
  if(Scr.CurrentDesk == t->desk)
    {
      t->IconView = XCreateWindow(dpy,icon_win, x, y, w, h,1,
				  CopyFromParent,
				  InputOutput,CopyFromParent,
				  valuemask,&attributes);
      XGrabButton(dpy, 2, AnyModifier, t->IconView,
		  True, ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
		  GrabModeAsync, GrabModeAsync, None,
		  None);
      XMapRaised(dpy,t->IconView);
    }
  else
    {
      t->IconView = XCreateWindow(dpy,icon_win, -1000, -1000, w, h,1,
				  CopyFromParent,
				  InputOutput,CopyFromParent,
				  valuemask,&attributes);
      XMapRaised(dpy,t->IconView);
    }
  Hilight(t,OFF);
}



void ChangeDeskForWindow(PagerWindow *t,long newdesk)
{
  int i,x,y,w,h,n,m,n1,m1;

  i = newdesk - desk1;

  if(t->PagerView == None)
    {
      t->desk = newdesk;
      XDestroyWindow(dpy,t->IconView);
      AddNewWindow( t);
      return;
    }

  n = (Scr.VxMax)/Scr.MyDisplayWidth;
  m = (Scr.VyMax)/Scr.MyDisplayHeight;
  n1 = (Scr.Vx+t->x)/Scr.MyDisplayWidth;
  m1 = (Scr.Vy+t->y)/Scr.MyDisplayHeight;
  x = (Scr.Vx + t->x)*(desk_w-n)/(Scr.VxMax + Scr.MyDisplayWidth) +n1;
  y = (Scr.Vy + t->y)*(desk_h-m)/(Scr.VyMax + Scr.MyDisplayHeight)+m1;
  w = (Scr.Vx + t->x + t->width+2)*(desk_w-n) /
    (Scr.VxMax + Scr.MyDisplayWidth) - 2 - x + n1;
  h = (Scr.Vy + t->y + t->height+2)*(desk_h-m) /
    (Scr.VyMax + Scr.MyDisplayHeight) -2 - y +m1;
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
	t->pager_view_width		= w;
	t->pager_view_height	= h;
  if((i >= 0)&&(i < ndesks))
    {
      XReparentWindow(dpy, t->PagerView, Desks[i].w, x,y);
      XResizeWindow(dpy,t->PagerView,w,h);
    }
  else
    {
      XDestroyWindow(dpy,t->PagerView);
      t->PagerView = None;
    }
  t->desk = i+desk1;

  x = (Scr.Vx + t->x)*(icon_w-n)/(Scr.VxMax + Scr.MyDisplayWidth) +n1;
  y = (Scr.Vy + t->y)*(icon_h-m)/(Scr.VyMax + Scr.MyDisplayHeight)+m1;
  w = (Scr.Vx + t->x + t->width+2)*(icon_w-n) /
    (Scr.VxMax + Scr.MyDisplayWidth) - 2 - x + n1;
  h = (Scr.Vy + t->y + t->height+2)*(icon_h-m) /
    (Scr.VyMax + Scr.MyDisplayHeight) -2 - y +m1;
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
	t->icon_view_width	= w;
	t->icon_view_height	= h;
  if(Scr.CurrentDesk == t->desk)
    XMoveResizeWindow(dpy,t->IconView,x,y,w,h);
  else
    XMoveResizeWindow(dpy,t->IconView,-1000,-1000,w,h);
}

void MoveResizePagerView(PagerWindow *t)
{
  int x,y,w,h,n,m,n1,m1;

  n = (Scr.VxMax)/Scr.MyDisplayWidth;
  m = (Scr.VyMax)/Scr.MyDisplayHeight;
  n1 = (Scr.Vx+t->x)/Scr.MyDisplayWidth;
  m1 = (Scr.Vy+t->y)/Scr.MyDisplayHeight;
  x = (Scr.Vx + t->x)*(desk_w-n)/(Scr.VxMax + Scr.MyDisplayWidth) +n1;
  y = (Scr.Vy + t->y)*(desk_h-m)/(Scr.VyMax + Scr.MyDisplayHeight)+m1;
  w = (Scr.Vx + t->x + t->width+2)*(desk_w-n)/
    (Scr.VxMax + Scr.MyDisplayWidth) - 2 - x + n1;
  h = (Scr.Vy + t->y + t->height+2)*(desk_h-m)/
    (Scr.VyMax + Scr.MyDisplayHeight) -2 - y +m1;

  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
	t->pager_view_width		= w;
	t->pager_view_height	= h;
  if(t->PagerView != None)
    XMoveResizeWindow(dpy,t->PagerView,x,y,w,h);
  else if((t->desk >= desk1)&&(t->desk <= desk2))
    {
      XDestroyWindow(dpy,t->IconView);
      AddNewWindow(t);
      return;
    }

  x = (Scr.Vx + t->x)*(icon_w-n)/(Scr.VxMax + Scr.MyDisplayWidth) +n1;
  y = (Scr.Vy + t->y)*(icon_h-m)/(Scr.VyMax + Scr.MyDisplayHeight)+m1;
  w = (Scr.Vx + t->x + t->width+2)*(icon_w-n)/
    (Scr.VxMax + Scr.MyDisplayWidth) - 2 - x + n1;
  h = (Scr.Vy + t->y + t->height+2)*(icon_h-m)/
    (Scr.VyMax + Scr.MyDisplayHeight) -2 - y +m1;

  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
	t->icon_view_width	= w;
	t->icon_view_height	= h;
  if(Scr.CurrentDesk == t->desk)
    XMoveResizeWindow(dpy,t->IconView,x,y,w,h);
  else
    XMoveResizeWindow(dpy,t->IconView,-1000,-1000,w,h);
}


void MoveStickyWindows(void)
{
  PagerWindow *t;

  t = Start;
  while(t!= NULL)
    {
      if(((t->flags & ICONIFIED)&&(t->flags & StickyIcon))||
	 (t->flags & STICKY))
	{
	  if(t->desk != Scr.CurrentDesk)
	    {
	      ChangeDeskForWindow(t,Scr.CurrentDesk);
	    }
	  else
	    {
	      MoveResizePagerView(t);

	    }
	}
      t = t->next;
    }
}

void Hilight(PagerWindow *t, int on)
{

  if(!t)return;

  if(Scr.d_depth < 2)
    {
      if(on)
	{
	  if(t->PagerView != None)
	    XSetWindowBackgroundPixmap(dpy,t->PagerView,Scr.gray_pixmap);
	  XSetWindowBackgroundPixmap(dpy,t->IconView,Scr.gray_pixmap);
	}
      else
	{
	  if(t->flags & STICKY)
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
    }
  else
    {
      if(on)
	{
	  if(t->PagerView != None)
	    XSetWindowBackground(dpy,t->PagerView,focus_pix);
	  XSetWindowBackground(dpy,t->IconView,focus_pix);
	}
      else
	{
	  if(t->PagerView != None)
	    XSetWindowBackground(dpy,t->PagerView,t->back);
	  XSetWindowBackground(dpy,t->IconView,t->back);
	}
    }
  if(t->PagerView != None)
    XClearWindow(dpy,t->PagerView);
  XClearWindow(dpy,t->IconView);
  LabelWindow(t);
  LabelIconWindow(t);
  PictureWindow(t);
  PictureIconWindow(t);
}

/* Use Desk == -1 to scroll the icon window */
void Scroll(int window_w, int window_h, int x, int y, int Desk)
{
#ifndef NON_VIRTUAL
  char command[256];
  int sx, sy;
  if(Wait == 0)
    {
      /* Desk < 0 means we want to scroll an icon window */
      if(Desk >= 0 && Desk + desk1 != Scr.CurrentDesk)
	{
	  return;
	}

      if(x < 0)
	x = 0;
      if(y < 0)
	y = 0;

      if(x > window_w)
	x = window_w;
      if(y > window_h)
	y = window_h;

      sx = (100*(x*(Scr.VxMax+Scr.MyDisplayWidth)/window_w- Scr.Vx)) /
	Scr.MyDisplayWidth;
      sy = (100*(y*(Scr.VyMax+Scr.MyDisplayHeight)/window_h - Scr.Vy)) /
	Scr.MyDisplayHeight;
      /* Make sure we don't get stuck a few pixels fromt the top/left border.
       * Since sx/sy are ints, values between 0 and 1 are rounded down. */
      if(sx == 0 && x == 0 && Scr.Vx != 0) sx = -1;
      if(sy == 0 && y == 0 && Scr.Vy != 0) sy = -1;

      sprintf(command,"Scroll %d %d\n",sx,sy);
      SendInfo(fd,command,0);
      Wait = 1;
    }
#endif
}

void MoveWindow(XEvent *Event)
{
  char command[100];
  int x1,y1,finished = 0,wx,wy,n,x,y,xi=0,yi=0,wx1,wy1,x2,y2;
  Window dumwin;
  PagerWindow *t;
  int m,n1,m1;
  int NewDesk,KeepMoving = 0;
  int moved = 0;
  int row,column;

  t = Start;
  while ((t != NULL)&&(t->PagerView != Event->xbutton.subwindow))
    t= t->next;

  if(t==NULL)
    {
      t = Start;
      while ((t != NULL)&&(t->IconView != Event->xbutton.subwindow))
	t= t->next;
      if(t!=NULL)
	{
	  IconMoveWindow(Event,t);
	  return;
	}
    }

  if(t == NULL)
    return;

  NewDesk = t->desk - desk1;
  if((NewDesk < 0)||(NewDesk >= ndesks))
    return;

  n = (Scr.VxMax)/Scr.MyDisplayWidth;
  m = (Scr.VyMax)/Scr.MyDisplayHeight;
  n1 = (Scr.Vx+t->x)/Scr.MyDisplayWidth;
  m1 = (Scr.Vy+t->y)/Scr.MyDisplayHeight;
  wx = (Scr.Vx + t->x)*(desk_w-n)/(Scr.VxMax + Scr.MyDisplayWidth) +n1;
  wy = (Scr.Vy + t->y)*(desk_h-m)/(Scr.VyMax + Scr.MyDisplayHeight)+m1;
  wx1 = wx+(desk_w+1)*(NewDesk%Columns);
  wy1 = wy + label_h + (desk_h+label_h+1)*(NewDesk/Columns);

  XReparentWindow(dpy, t->PagerView, Scr.Pager_w,wx1,wy1);
  XRaiseWindow(dpy,t->PagerView);

  XTranslateCoordinates(dpy, Event->xany.window, t->PagerView,
			Event->xbutton.x, Event->xbutton.y, &x1, &y1, &dumwin);
  xi = x1;
  yi = y1;
  while(!finished)
    {
      XMaskEvent(dpy,ButtonReleaseMask | ButtonMotionMask|ExposureMask,Event);

      if(Event->type == MotionNotify)
	{
	  XTranslateCoordinates(dpy, Event->xany.window, Scr.Pager_w,
				Event->xmotion.x, Event->xmotion.y, &x, &y,
				&dumwin);
	  if(moved == 0)
	    {
	      xi = x;
	      yi = y;
	      moved = 1;
	    }
	  if((x < -5)||(y<-5)||(x>window_w+5)||(y>window_h+5))
	    {
	      KeepMoving = 1;
	      finished = 1;
	    }
	  XMoveWindow(dpy,t->PagerView, x - (x1),
		      y - (y1));
	}
      else if(Event->type == ButtonRelease)
	{
	  XTranslateCoordinates(dpy, Event->xany.window, Scr.Pager_w,
				Event->xbutton.x, Event->xbutton.y, &x, &y,
				&dumwin);
	  XMoveWindow(dpy,t->PagerView, x - (x1),
		      y - (y1));
	  finished = 1;
	}
      else if (Event->type == Expose)
	{
	  HandleExpose(Event);
	}
    }

  if(moved)
    {
      if((x - xi < 3)&&(y - yi < 3)&&
	 (x - xi > -3)&&(y -yi > -3))
	moved = 0;
    }
  if(KeepMoving)
    {
      NewDesk = Scr.CurrentDesk;
      if(NewDesk != t->desk)
	{
	  XMoveWindow(dpy,t->w,Scr.MyDisplayWidth+Scr.VxMax,
		      Scr.MyDisplayHeight+Scr.VyMax);
	  XSync(dpy,0);
	  sprintf(command,"MoveToDesk 0 %d", NewDesk);
	  SendInfo(fd,command,t->w);
	  t->desk = NewDesk;
	}
      if((NewDesk>=desk1)&&(NewDesk<=desk2))
	XReparentWindow(dpy, t->PagerView, Desks[NewDesk-desk1].w,0,0);
      else
	{
	  XDestroyWindow(dpy,t->PagerView);
	  t->PagerView = None;
	}
      XTranslateCoordinates(dpy, Scr.Pager_w, Scr.Root,
			    x, y, &x1, &y1, &dumwin);
      XUngrabPointer(dpy,CurrentTime);
      XSync(dpy,0);
      if(t->flags & ICONIFIED)
	SendInfo(fd,"Move",t->icon_w);
      else
	SendInfo(fd,"Move",t->w);
      return;
    }
  else
    {
      column = (x/(desk_w+1));
      row =  (y/(desk_h+ label_h+1));
      NewDesk = column + (row)*Columns;
      if((NewDesk <0)||(NewDesk >=ndesks))
	{
	  NewDesk = Scr.CurrentDesk - desk1;
	  x = xi;
	  y = yi;
	  moved = 0;
	}
      XTranslateCoordinates(dpy, Scr.Pager_w,Desks[NewDesk].w,
			    x-x1, y-y1, &x2,&y2,&dumwin);

      n1 = x2*(Scr.VxMax + Scr.MyDisplayWidth)/(desk_w * Scr.MyDisplayWidth);
      m1 = y2*(Scr.VyMax + Scr.MyDisplayHeight)/(desk_h * Scr.MyDisplayHeight);
      x = (x2-n1)*
	(Scr.VxMax + Scr.MyDisplayWidth)/(desk_w-n) - Scr.Vx;
      y = (y2-m1)*
	(Scr.VyMax + Scr.MyDisplayHeight)/(desk_h-m) - Scr.Vy;
      if(x + t->frame_width + Scr.Vx < 0 )
	x = -Scr.Vx;
      if(y+t->frame_height + Scr.Vy< 0)
	y = -Scr.Vy;
      if(x + Scr.Vx > Scr.MyDisplayWidth+Scr.VxMax)
	x = Scr.MyDisplayWidth + Scr.VxMax - t->frame_width - Scr.Vx;
      if(y +Scr.Vy> Scr.MyDisplayHeight+Scr.VyMax)
	y = Scr.MyDisplayHeight+ Scr.VyMax - t->frame_height - Scr.Vy;
      if(((t->flags & ICONIFIED)&&(t->flags & StickyIcon))||
	 (t->flags & STICKY))
	{
	  NewDesk = Scr.CurrentDesk - desk1;
	  if(x > Scr.MyDisplayWidth -16)
	    x = Scr.MyDisplayWidth - 16;
	  if(y > Scr.MyDisplayHeight-16)
	    y = Scr.MyDisplayHeight - 16;
	  if(x + t->width < 16)
	    x = 16 - t->width;
	  if(y + t->height < 16)
	    y = 16 - t->height;
	}
      if(NewDesk +desk1 != t->desk)
	{
	  if(((t->flags & ICONIFIED)&&(t->flags & StickyIcon))||
	     (t->flags & STICKY))
	    {
	      NewDesk = Scr.CurrentDesk - desk1;
	      if(t->desk != Scr.CurrentDesk)
		ChangeDeskForWindow(t,Scr.CurrentDesk);
	    }
	  else
	    {
	      sprintf(command,"MoveToDesk 0 %d", NewDesk + desk1);
	      SendInfo(fd,command,t->w);
	      t->desk = NewDesk + desk1;
	    }
	}

      if((NewDesk >= 0)&&(NewDesk < ndesks))
	{
	  XReparentWindow(dpy, t->PagerView, Desks[NewDesk].w,x,y);
	  if(moved)
	    {
	      if(t->flags & ICONIFIED)
		XMoveWindow(dpy,t->icon_w,x,y);
	      else
		XMoveWindow(dpy,t->w,x+t->border_width,
			    y+t->title_height+t->border_width);
	      XSync(dpy,0);
	    }
	  else
	    MoveResizePagerView(t);
	  SendInfo(fd,"Raise",t->w);
	}
      if(Scr.CurrentDesk == t->desk)
	{
          XSync(dpy,0);
          usleep(5000);
          XSync(dpy,0);
	  if(t->flags & ICONIFIED)
            {
/*
    RBW - reverting to old code for 2.2...
    The new handling causes an unwanted viewport change whenever Button2
    is used; the old handling causes focus to be sent to No Input windows
    regardless of the Lenience setting. After 2.2 we will revisit this issue.
    I suspect it will involve expanding the module message to include wmhints
    and such.
*/
#if 0
            SendInfo(fd, "Focus", t->icon_w);
#else
            XSetInputFocus (dpy, t->icon_w, RevertToParent,
              Event->xbutton.time);
#endif
            }
	  else
            {
#if 0
	    SendInfo(fd, "Focus", t->w);
#else
            XSetInputFocus (dpy, t->w, RevertToParent,
              Event->xbutton.time);
#endif
            }
	}
    }
}






/***********************************************************************
 *
 *  Procedure:
 *      FvwmErrorHandler - displays info on internal errors
 *
 ************************************************************************/
XErrorHandler FvwmErrorHandler(Display *dpy, XErrorEvent *event)
{
#if 0
  Window root;
  unsigned border_width, depth;
  int x,y;

  if(XGetGeometry(dpy,Scr.Pager_w,&root,&x,&y,
		  (unsigned *)&window_w,(unsigned *)&window_h,
		  &border_width,&depth)==0)
    {
      exit(0);
    }

  return 0;
#else
  /* really should just exit here... */
  fprintf(stderr,"%s: XError!  Bagging out!\n",MyName);
  exit(0);
#endif /* 0 */
}


void LabelWindow(PagerWindow *t)
{
  XGCValues Globalgcv;
  unsigned long Globalgcm;

  if(windowFont == NULL)
    {
      return;
    }
  if (MiniIcons && t->mini_icon.picture && (t->PagerView != None))
  {
    return; /* will draw picture instead... */
  }
  if(t->icon_name == NULL)
    {
      return;
    }
  if(t == FocusWin)
    {
      Globalgcv.foreground = focus_fore_pix;
      Globalgcv.background = focus_pix;
      Globalgcm = GCForeground|GCBackground;
      XChangeGC(dpy, StdGC,Globalgcm,&Globalgcv);
    }
  else
    {
      Globalgcv.foreground = t->text;
      Globalgcv.background = t->back;
      Globalgcm = GCForeground|GCBackground;
      XChangeGC(dpy, StdGC,Globalgcm,&Globalgcv);

    }
  if(t->PagerView != None)
    {
      XClearWindow(dpy, t->PagerView);
      XDrawString (dpy, t->PagerView,StdGC,2,windowFont->ascent+2 ,
			t->icon_name, strlen(t->icon_name));
    }
}


void LabelIconWindow(PagerWindow *t)
{
  XGCValues Globalgcv;
  unsigned long Globalgcm;

  if(windowFont == NULL)
    {
      return;
    }
  if (MiniIcons && t->mini_icon.picture && (t->PagerView != None))
    {
      return; /* will draw picture instead... */
    }
  if(t->icon_name == NULL)
    {
      return;
    }

  if(t == FocusWin)
    {
      Globalgcv.foreground = focus_fore_pix;
      Globalgcv.background = focus_pix;
      Globalgcm = GCForeground|GCBackground;
      XChangeGC(dpy,StdGC,Globalgcm,&Globalgcv);
    }
  else
    {
      Globalgcv.foreground = t->text;
      Globalgcv.background = t->back;
      Globalgcm = GCForeground|GCBackground;
      XChangeGC(dpy,StdGC,Globalgcm,&Globalgcv);

    }
  XClearWindow(dpy, t->IconView);
  XDrawString (dpy, t->IconView,StdGC,2,windowFont->ascent+2 ,
	       t->icon_name, strlen(t->icon_name));

}
void PictureWindow (PagerWindow *t)
{
  XGCValues Globalgcv;
  unsigned long Globalgcm;
  int iconX;
  int iconY;
  if (MiniIcons)
    {
      if (t->mini_icon.picture && (t->PagerView != None))
	{
	  if (t->pager_view_width > t->mini_icon.width)
	    iconX = (t->pager_view_width - t->mini_icon.width) / 2;
	  else if (t->pager_view_width < t->mini_icon.width)
	    iconX = -((t->mini_icon.width - t->pager_view_width) / 2);
	  else
	    iconX = 0;
	  if (t->pager_view_height > t->mini_icon.height)
	    iconY = (t->pager_view_height - t->mini_icon.height) / 2;
	  else if (t->pager_view_height < t->mini_icon.height)
	    iconY = -((t->mini_icon.height - t->pager_view_height) / 2);
	  else
	    iconY = 0;
	  Globalgcm = GCForeground | GCBackground | GCClipMask |
	    GCClipXOrigin | GCClipYOrigin;
	  Globalgcv.clip_mask = t->mini_icon.mask;
	  Globalgcv.clip_x_origin = iconX;
	  Globalgcv.clip_y_origin = iconY;
	  if (t == FocusWin)
	    {
	      Globalgcv.foreground = focus_fore_pix;
	      Globalgcv.background = focus_pix;
	    }
	  else
	    {
	      Globalgcv.foreground = t->text;
	      Globalgcv.background = t->back;
	    }
	  XChangeGC (dpy, MiniIconGC, Globalgcm, &Globalgcv);
	  XClearWindow (dpy, t->PagerView);
	  XCopyArea (dpy, t->mini_icon.picture, t->PagerView, MiniIconGC,
		     0, 0, t->mini_icon.width, t->mini_icon.height, iconX,
		     iconY);
	}
    }
}
void PictureIconWindow (PagerWindow *t)
{
  XGCValues Globalgcv;
  unsigned long Globalgcm;
  int iconX;
  int iconY;
  if (MiniIcons)
    {
      if (t->mini_icon.picture && (t->IconView != None))
	{
	  if (t->icon_view_width > t->mini_icon.width)
	    iconX = (t->icon_view_width - t->mini_icon.width) / 2;
	  else if (t->icon_view_width < t->mini_icon.width)
	    iconX = -((t->mini_icon.width - t->icon_view_width) / 2);
	  else
	    iconX = 0;
	  if (t->icon_view_height > t->mini_icon.height)
	    iconY = (t->icon_view_height - t->mini_icon.height) / 2;
	  else if (t->icon_view_height < t->mini_icon.height)
	    iconY = -((t->mini_icon.height - t->icon_view_height) / 2);
	  else
	    iconY = 0;
	  Globalgcm = GCForeground | GCBackground | GCClipMask |
	    GCClipXOrigin | GCClipYOrigin;
	  Globalgcv.clip_mask = t->mini_icon.mask;
	  Globalgcv.clip_x_origin = iconX;
	  Globalgcv.clip_y_origin = iconY;
	  if (t == FocusWin)
	    {
	      Globalgcv.foreground = focus_fore_pix;
	      Globalgcv.background = focus_pix;
	    }
	  else
	    {
	      Globalgcv.foreground = t->text;
	      Globalgcv.background = t->back;
	    }
	  XChangeGC (dpy, MiniIconGC, Globalgcm, &Globalgcv);
	  XClearWindow (dpy, t->IconView);
	  XCopyArea (dpy, t->mini_icon.picture, t->IconView, MiniIconGC,
		     0, 0, t->mini_icon.width, t->mini_icon.height, iconX,
		     iconY);
	}
    }
}

void IconMoveWindow(XEvent *Event,PagerWindow *t)
{
  int x1,y1,finished = 0,wx,wy,n,x=0,y=0,xi=0,yi=0;
  Window dumwin;
  int m,n1,m1;
  int moved = 0;
  int KeepMoving = 0;

  if(t==NULL)
    return;

  n = (Scr.VxMax)/Scr.MyDisplayWidth;
  m = (Scr.VyMax)/Scr.MyDisplayHeight;
  n1 = (Scr.Vx+t->x)/Scr.MyDisplayWidth;
  m1 = (Scr.Vy+t->y)/Scr.MyDisplayHeight;
  wx = (Scr.Vx + t->x)*(icon_w-n)/(Scr.VxMax + Scr.MyDisplayWidth) +n1;
  wy = (Scr.Vy + t->y)*(icon_h-m)/(Scr.VyMax + Scr.MyDisplayHeight)+m1;

  XRaiseWindow(dpy,t->IconView);

  XTranslateCoordinates(dpy, Event->xany.window, t->IconView,
			Event->xbutton.x, Event->xbutton.y, &x1, &y1, &dumwin);
  while(!finished)
    {
      XMaskEvent(dpy,ButtonReleaseMask | ButtonMotionMask|ExposureMask,Event);

      if(Event->type == MotionNotify)
	{
	  x = Event->xbutton.x;
	  y = Event->xbutton.y;
	  if(moved == 0)
	    {
	      xi = x;
	      yi = y;
	      moved = 1;
	    }

	  XMoveWindow(dpy,t->IconView, x - (x1),
		      y - (y1));
	  if((x < -5)||(y < -5)||(x>icon_w+5)||(y>icon_h+5))
	    {
	      finished = 1;
	      KeepMoving = 1;
	    }
	}
      else if(Event->type == ButtonRelease)
	{
	  x = Event->xbutton.x;
	  y = Event->xbutton.y;
	  XMoveWindow(dpy,t->PagerView, x - (x1),y - (y1));
	  finished = 1;
	}
      else if (Event->type == Expose)
	{
	  HandleExpose(Event);
	}
    }

  if(moved)
    {
      if((x - xi < 3)&&(y - yi < 3)&&
	 (x - xi > -3)&&(y -yi > -3))
	moved = 0;
    }

  if(KeepMoving)
    {
      XTranslateCoordinates(dpy, t->IconView, Scr.Root,
			    x, y, &x1, &y1, &dumwin);
      XUngrabPointer(dpy,CurrentTime);
      XSync(dpy,0);
      if(t->flags & ICONIFIED)
	SendInfo(fd,"Move",t->icon_w);
      else
	SendInfo(fd,"Move",t->w);
    }
  else
    {
      x = x - x1;
      y = y - y1;
      n1 = x*(Scr.VxMax + Scr.MyDisplayWidth)/(icon_w * Scr.MyDisplayWidth);
      m1 = y*(Scr.VyMax + Scr.MyDisplayHeight)/(icon_h * Scr.MyDisplayHeight);
      x = (x-n1)*
	(Scr.VxMax + Scr.MyDisplayWidth)/(icon_w-n) - Scr.Vx;
      y = (y-m1)*
	(Scr.VyMax + Scr.MyDisplayHeight)/(icon_h-m) - Scr.Vy;

      if(((t->flags & ICONIFIED)&&(t->flags & StickyIcon))||
	 (t->flags & STICKY))
	{
	  if(x > Scr.MyDisplayWidth -16)
	    x = Scr.MyDisplayWidth - 16;
	  if(y > Scr.MyDisplayHeight-16)
	    y = Scr.MyDisplayHeight - 16;
	  if(x + t->width < 16)
	    x = 16 - t->width;
	  if(y + t->height < 16)
	    y = 16 - t->height;
	}
      if(moved)
	{
	  if(t->flags & ICONIFIED)
	    XMoveWindow(dpy,t->icon_w,x,y);
	  else
	    XMoveWindow(dpy,t->w,x,y);
	  XSync(dpy,0);
	}
      else
	{
	  MoveResizePagerView(t);
	}
      SendInfo(fd,"Raise",t->w);

      if(t->flags & ICONIFIED)
        {
/*
    RBW - reverting to old code for 2.2...temporarily. See note above, in
    MoveWindow.
*/
#if 0
          SendInfo(fd, "Focus", t->icon_w);
#else
          XSetInputFocus (dpy, t->icon_w, RevertToParent, Event->xbutton.time);
#endif
        }
      else
        {
#if 0
          SendInfo(fd, "Focus", t->w);
#else
          XSetInputFocus (dpy, t->w, RevertToParent, Event->xbutton.time);
#endif
        }
    }

}


/* Just maps window ... draw stuff in it later after Expose event
   -- ric@giccs.georgetown.edu */
void MapBalloonWindow (XEvent *event)
{
  PagerWindow *t;
  XWindowChanges window_changes;
  Window view, dummy;
  int view_width, view_height;
  int matched_window = 0;
  int x, y;
  extern char *BalloonBack;

  /* is this the best way to match X event window ID to PagerWindow ID? */
  t = Start;

  while ( ! matched_window ) {
    if (t == NULL) {
      return;
    }
    else if ( t->PagerView == event->xcrossing.window ) {
      view = t->PagerView;
      view_width = t->pager_view_width;
      view_height = t->pager_view_height;
      matched_window = 1;
    }
    else if ( t->IconView == event->xcrossing.window ) {
      view = t->IconView;
      view_width = t->icon_view_width;
      view_height = t->icon_view_height;
      matched_window = 1;
    }
    else {
      t = t->next;
    }
  }

  /* associate balloon with its pager window */
  balloon.pw = t;

  /* calculate window width to accommodate string */
  window_changes.width = 4 + XTextWidth(balloon.font, t->icon_name,
                                        strlen(t->icon_name));

  /* get x and y coords relative to pager window */
  x = (view_width / 2) - (window_changes.width / 2) - balloon.border;

  if ( balloon.yoffset > 0 )
    y = view_height + balloon.yoffset;
  else
    y = balloon.yoffset - balloon.height - (2 * balloon.border);


  /* balloon is a top-level window, therefore need to
     translate pager window coords to root window coords */
  XTranslateCoordinates(dpy, view, Scr.Root, x, y,
                        &window_changes.x, &window_changes.y, &dummy);


  /* make sure balloon doesn't go off screen
     (actually 2 pixels from edge rather than 0 just to be pretty :-) */

  /* too close to left */
  if ( window_changes.x < 2 )
    window_changes.x = 2;

  /* too close to right */
  else if ( window_changes.x + window_changes.width >
            Scr.MyDisplayWidth - (2 * balloon.border) - 2 )
    window_changes.x = Scr.MyDisplayWidth - window_changes.width -
      (2 * balloon.border) - 2;

  /* too close to top ... make yoffset +ve */
  if ( window_changes.y < 2 ) {
    y = - balloon.yoffset + view_height;
    XTranslateCoordinates(dpy, view, Scr.Root, x, y,
			  &window_changes.x, &window_changes.y, &dummy);
  }

  /* too close to bottom ... make yoffset -ve */
  else if ( window_changes.y + balloon.height >
	    Scr.MyDisplayHeight - (2 * balloon.border) - 2 ) {
    y = - balloon.yoffset - balloon.height - (2 * balloon.border);
    XTranslateCoordinates(dpy, view, Scr.Root, x, y,
			  &window_changes.x, &window_changes.y, &dummy);
  }


  /* make changes to window */
  XConfigureWindow(dpy, balloon.w, CWX | CWY | CWWidth, &window_changes);

  /* if background not set in config make it match pager window */
  if ( BalloonBack == NULL )
    XSetWindowBackground(dpy, balloon.w, t->back);

  XMapRaised(dpy, balloon.w);
}


/* -- ric@giccs.georgetown.edu */
void UnmapBalloonWindow (void)
{
  XUnmapWindow(dpy, balloon.w);
}


/* Draws string in balloon window -- call after it's received Expose event
   -- ric@giccs.georgetown.edu */
void DrawInBalloonWindow (void)
{
  extern char *BalloonFore;

  /* if foreground not set in config make it match pager window */
  if ( BalloonFore == NULL )
    XSetForeground(dpy, BalloonGC, balloon.pw->text);

  XDrawString(dpy, balloon.w, BalloonGC,
              2, balloon.font->ascent,
              balloon.pw->icon_name, strlen(balloon.pw->icon_name));
}
