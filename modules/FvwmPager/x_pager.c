/****************************************************************************
 * This module is all new
 * by Rob Nation
 ****************************************************************************/

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
#include <X11/keysym.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include "libs/fvwmlib.h"
#include <libs/Module.h>
#include "libs/Colorset.h"
#include "fvwm/fvwm.h"
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
#ifdef SHAPE
extern int ShapeLabels;
#else
#define ShapeLabels 0
#endif
extern int ShowBalloons, ShowPagerBalloons, ShowIconBalloons;
extern char *BalloonFormatString;
extern char *WindowLabelFormat;
extern char *PagerFore, *PagerBack, *HilightC;
extern char *BalloonFore, *BalloonBack, *BalloonBorderColor;
extern Window BalloonView;
extern unsigned int WindowBorderWidth;
extern unsigned int MinSize;
extern Bool WindowBorders3d;
extern Picture *PixmapBack;
extern Picture *HilightPixmap;
extern int HilightDesks;
extern char fAlwaysCurrentDesk;

extern int MoveThreshold;

extern int icon_w, icon_h, icon_x, icon_y;
XFontStruct *font, *windowFont;
#ifdef I18N_MB
XFontSet fontset, windowFontset;
#ifdef __STDC__
#define XTextWidth(x,y,z) XmbTextEscapement(x ## set,y,z)
#else
#define XTextWidth(x,y,z) XmbTextEscapement(x/**/set,y,z)
#endif
#endif

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


Window		icon_win;               /* icon window */

static char *GetBalloonLabel(const PagerWindow *pw,const char *fmt);
extern void ExitPager(void);

static Pixmap default_pixmap = None;

/***********************************************************************
 *
 *  Procedure:
 *	CalcGeom - calculates the size and position of a mini-window
 *	given the real window size.
 *	You can always tell bad code by the size of the comments.
 ***********************************************************************/
static void CalcGeom(PagerWindow *t, int win_w, int win_h,
                     int *x_ret, int *y_ret, int *w_ret, int *h_ret)
{
  int virt, edge, size, page, over;

  /* coordinate of left hand edge on virtual desktop */
  virt = Scr.Vx + t->x;

  /* position of left hand edge of mini-window on pager window */
  edge = (virt * win_w) / Scr.VWidth;

  /* absolute coordinate of right hand edge on virtual desktop */
  virt += t->width - 1;

  /* scale the width of the big window to find the mini-window's width   */
  /* - this way, the width isn't vulnerable to integer arithmetic errors */
  size = (t->width * win_w) / Scr.VWidth;

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
      over = edge + size - ((page + 1) * win_w * Scr.MyDisplayWidth) / Scr.VWidth;

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
  size = (t->height * win_h) / Scr.VHeight;
  if (size < MinSize)
  {
    size = MinSize;
    page = virt / Scr.MyDisplayHeight;
    if (page == ((virt - t->height + 1) / Scr.MyDisplayHeight)) {
      over = edge + size - ((page + 1) * win_h * Scr.MyDisplayHeight) / Scr.VHeight;
      if (over > 0)
        edge -= over;
    }
  }
  *y_ret = edge;
  *h_ret = size;
}

/***********************************************************************
 *
 *  Procedure:
 *	Initialize_viz_pager - creates a temp window of the correct visual
 *      so that pixmaps may be created for use with the main window
 ***********************************************************************/
void initialize_viz_pager(void)
{
  XSetWindowAttributes attr;
  XGCValues xgcv;

  if (Pdefault) {
    Scr.Pager_w = Scr.Root;
    Scr.NormalGC = DefaultGC(dpy, Scr.screen);
  } else {
    attr.background_pixmap = None;
    attr.border_pixel = 0;
    attr.colormap = Pcmap;
    Scr.Pager_w = XCreateWindow(dpy, Scr.Root, -10, -10, 10, 10, 0, Pdepth,
				InputOutput, Pvisual,
				CWBackPixmap|CWBorderPixel|CWColormap, &attr);
    Scr.NormalGC = XCreateGC(dpy, Scr.Pager_w, 0, &xgcv);
  }
  xgcv.plane_mask = AllPlanes;
  Scr.MiniIconGC = XCreateGC(dpy, Scr.Pager_w, GCPlaneMask, &xgcv);
  Scr.black = GetColor("Black");

  /* Transparent background are only allowed when the depth matched the root */
  if (Pdepth == DefaultDepth(dpy, Scr.screen))
    default_pixmap = ParentRelative;
}

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
#ifdef I18N_MB
  char **ml;
  int mc;
  char *ds;
  XFontStruct **fs_list;
#endif

  XTextProperty name;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  extern char *WindowBack, *WindowFore, *WindowHiBack, *WindowHiFore;
  extern char *BalloonFont;
  extern int BalloonBorderWidth, BalloonYOffset;
  extern char *font_string, *smallFont;
  int n,m,w,h,i,x,y;
  XGCValues gcv;
  Pixel balloon_border_pix;
  Pixel balloon_back_pix;
  Pixel balloon_fore_pix;

#if 1
  /* I don't think that this is necessary - just let pager die */
  /* domivogt (07-mar-1999): But it is! A window being moved in the pager
   * might die at any moment causing the Xlib calls to generate BadMatch
   * errors. Without an error handler the pager will die! */
  XSetErrorHandler((XErrorHandler)FvwmErrorHandler);
#endif /* 1 */

  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);

  /* load the font */
#ifdef I18N_MB
  if (!uselabel || ((fontset =
		     XCreateFontSet(dpy, font_string, &ml, &mc, &ds)) == NULL))
  {
#ifdef STRICTLY_FIXED
    if ((fontset = XCreateFontSet(dpy, "fixed", &ml, &mc, &ds)) == NULL)
#else
    if ((fontset =
	 XCreateFontSet(dpy, "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*",
			&ml, &mc, &ds)) == NULL)
#endif
    {
      fprintf(stderr,"%s: No fonts available\n",MyName);
      exit(1);
    }
  }
  XFontsOfFontSet(fontset, &fs_list, &ml);
  font = fs_list[0];
#else
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
#endif

#ifdef SHAPE
  /* Check that shape extension exists. */
  if (ShapeLabels)
  {
    int s1, s2;
    ShapeLabels = (XShapeQueryExtension (dpy, &s1, &s2) ? 1 : 0);
  }
#endif

#ifdef I18N_MB
  if(smallFont != NULL)
  {
    windowFontset = XCreateFontSet(dpy, smallFont, &ml, &mc, &ds);
    if (windowFontset != NULL)
    {
      XFontsOfFontSet(windowFontset, &fs_list, &ml);
      windowFont = fs_list[0];
    } else {
      windowFontset = NULL;
      windowFont = NULL;
    }
  }
  else
  {
    windowFontset = NULL;
    windowFont = NULL;
  }
#else
  if(smallFont!= NULL)
  {
    windowFont= XLoadQueryFont(dpy, smallFont);
  }
  else
    windowFont= NULL;
#endif

  /* Load the colors */
  fore_pix = GetColor(PagerFore);
  back_pix = GetColor(PagerBack);
  hi_pix = GetColor(HilightC);

  if (windowcolorset >= 0) {
    win_back_pix = Colorset[windowcolorset].bg;
    win_fore_pix = Colorset[windowcolorset].fg;
    win_pix_set = True;
  } else if (WindowBack && WindowFore)
  {
    win_back_pix = GetColor(WindowBack);
    win_fore_pix = GetColor(WindowFore);
    win_pix_set = True;
  }

  if (activecolorset >= 0) {
    win_hi_back_pix = Colorset[activecolorset].bg;
    win_hi_fore_pix = Colorset[activecolorset].fg;
    win_hi_pix_set = True;
  } else if (WindowHiBack && WindowHiFore)
  {
    win_hi_back_pix = GetColor(WindowHiBack);
    win_hi_fore_pix = GetColor(WindowHiFore);
    win_hi_pix_set = True;
  }

  balloon_border_pix = (!BalloonBorderColor)
    ? fore_pix : GetColor(BalloonBorderColor);
  balloon_back_pix = (!BalloonBack) ? back_pix : GetColor(BalloonBack);
  balloon_fore_pix = (!BalloonFore) ? fore_pix : GetColor(BalloonFore);

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
  if(window_w > 0)
  {
    window_w = ((window_w - n) / (n + 1)) * (n + 1) + n;
    Scr.VScale = Columns * Scr.VWidth / (window_w - Columns + 1 - Columns * n);
  }
  if(window_h > 0)
  {
    window_h = ((window_h - m) / (m + 1)) * (m + 1) + m;
    Scr.VScale = Rows * Scr.VHeight /(window_h + 2 - Rows * (label_h - m - 1));
  }
  if(window_w <= 0)
    window_w = Columns * (Scr.VWidth / Scr.VScale + n) + Columns - 1;
  if(window_h <= 0)
    window_h = Rows * (Scr.VHeight / Scr.VScale + m + label_h + 1) - 2;

  if (is_transient)
  {
    if (window_w + window_x > Scr.MyDisplayWidth)
    {
      window_x = 0;
      xneg = 1;
    }
    if (window_h + window_y > Scr.MyDisplayHeight)
    {
      window_y = 0;
      yneg = 1;
    }
  }
  if(xneg)
  {
    sizehints.win_gravity = NorthEastGravity;
    window_x = Scr.MyDisplayWidth - window_w + window_x;
  }

  if(yneg)
  {
    window_y = Scr.MyDisplayHeight - window_h + window_y;
    if(sizehints.win_gravity == NorthEastGravity)
      sizehints.win_gravity = SouthEastGravity;
    else
      sizehints.win_gravity = SouthWestGravity;
  }

  if(usposition)
    sizehints.flags |= USPosition;

  valuemask = (CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask);
  attributes.background_pixmap = default_pixmap;
  attributes.border_pixel = 0;
  attributes.colormap = Pcmap;
  attributes.event_mask = (StructureNotifyMask);
  sizehints.width = window_w;
  sizehints.height = window_h;
  sizehints.x = window_x;
  sizehints.y = window_y;
  sizehints.width_inc = Columns*(n+1);
  sizehints.height_inc = Rows*(m+1);
  sizehints.base_width = Columns * n + Columns - 1;
  sizehints.base_height = Rows*(m + label_h+1) - 1;

  /* destroy the temp window first, don't worry if it's the Root */
  if (Scr.Pager_w != Scr.Root)
    XDestroyWindow(dpy, Scr.Pager_w);
  Scr.Pager_w = XCreateWindow (dpy, Scr.Root, window_x, window_y, window_w,
			       window_h, 0, Pdepth, InputOutput, Pvisual,
			       valuemask, &attributes);
  XSetWMProtocols(dpy,Scr.Pager_w,&wm_del_win,1);
  XSetWMNormalHints(dpy,Scr.Pager_w,&sizehints);

#ifdef I18N_MB
  if((desk1==desk2)&&(Desks[0].label != NULL))
    XmbTextListToTextProperty(dpy,&Desks[0].label,1,XStdICCTextStyle,&name);
  else
    XmbTextListToTextProperty(dpy,&pager_name,1,XStdICCTextStyle,&name);
#else
  if((desk1==desk2)&&(Desks[0].label != NULL))
    XStringListToTextProperty(&Desks[0].label,1,&name);
  else
    XStringListToTextProperty(&pager_name,1,&name);
#endif

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
  class1.res_class = "FvwmPager";

  XSetWMProperties(dpy,Scr.Pager_w,&name,&name,NULL,0,
		   &sizehints,&wmhints,&class1);
  XFree((char *)name.value);

  /* change colour/font for labelling mini-windows */
  XSetForeground(dpy, Scr.NormalGC, focus_fore_pix);
  if (windowFont != NULL)
    XSetFont(dpy, Scr.NormalGC, windowFont->fid);

  /* create the 3d bevel GC's if necessary */
  if (windowcolorset >= 0) {
    gcv.foreground = Colorset[windowcolorset].hilite;
    Scr.whGC = XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
    gcv.foreground = Colorset[windowcolorset].shadow;
    Scr.wsGC = XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
  }
  if (activecolorset >= 0) {
    gcv.foreground = Colorset[activecolorset].hilite;
    Scr.ahGC = XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
    gcv.foreground = Colorset[activecolorset].shadow;
    Scr.asGC = XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);
  }

  for(i=0;i<ndesks;i++)
  {
    w = window_w/ndesks;
    h = window_h;
    x = w*i;
    y = 0;

    /* create the GC for desk labels */
    gcv.foreground = (Desks[i].colorset < 0) ? fore_pix
      : Colorset[Desks[i].colorset].fg;
    gcv.font = font->fid;
    Desks[i].NormalGC = XCreateGC(dpy, Scr.Pager_w, GCForeground | GCFont,
				  &gcv);

    /* create the active desk hilite GC */
    if(Pdepth < 2)
      gcv.foreground = fore_pix;
    else
      gcv.foreground = (Desks[i].highcolorset < 0) ? hi_pix
	: Colorset[Desks[i].highcolorset].bg;
    Desks[i].HiliteGC = XCreateGC(dpy, Scr.Pager_w, GCForeground, &gcv);

    /* create the hilight desk title drawing GC */
    if ((Pdepth < 2) || (fore_pix == hi_pix))
      gcv.foreground = (Desks[i].highcolorset < 0) ? back_pix
	: Colorset[Desks[i].highcolorset].fg;
    else
      gcv.foreground = (Desks[i].highcolorset < 0) ? fore_pix
	: Colorset[Desks[i].highcolorset].fg;
    Desks[i].rvGC = XCreateGC(dpy, Scr.Pager_w, GCForeground | GCFont, &gcv);

    /* create the virtual page boundary GC */
    gcv.foreground = (Desks[i].colorset < 0) ? fore_pix
      : Colorset[Desks[i].colorset].fg;
    gcv.line_style = (use_dashed_separators) ? LineOnOffDash : LineSolid;
    Desks[i].DashedGC = XCreateGC(dpy, Scr.Pager_w,
				  GCForeground | GCLineStyle, &gcv);

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
    Desks[i].title_w = XCreateWindow(dpy, Scr.Pager_w, x, y, w, h, 1,
				     CopyFromParent, InputOutput,
				     CopyFromParent, valuemask, &attributes);
    attributes.event_mask = (ExposureMask | ButtonReleaseMask |
			     ButtonPressMask |ButtonMotionMask);
    desk_h = window_h - label_h;

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

    Desks[i].w = XCreateWindow(dpy, Desks[i].title_w, x, y, w, desk_h, 1,
			       CopyFromParent, InputOutput, CopyFromParent,
			       valuemask, &attributes);
    if (Desks[i].colorset > -1 &&
	Colorset[Desks[i].colorset].pixmap)
    {
      SetWindowBackground(
          dpy, Desks[i].w, w, desk_h, &Colorset[Desks[i].colorset], Pdepth,
          Scr.NormalGC, True);
    }


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

      w = (window_w - n)/(n+1);
      h = (window_h - label_h - m)/(m+1);
      Desks[i].CPagerWin=XCreateWindow(dpy, Desks[i].w, -1000, -1000, w, h, 0,
				       CopyFromParent, InputOutput,
				       CopyFromParent, valuemask, &attributes);
      if (Desks[i].highcolorset > -1 &&
	  Colorset[Desks[i].highcolorset].pixmap)
      {
	SetWindowBackground(
            dpy, Desks[i].CPagerWin, w, h, &Colorset[Desks[i].highcolorset],
            Pdepth, Scr.NormalGC, True);
      }
      XMapRaised(dpy,Desks[i].CPagerWin);
    }

    XMapRaised(dpy,Desks[i].w);
    XMapRaised(dpy,Desks[i].title_w);

    /* create balloon window
       -- ric@giccs.georgetown.edu */
    if ( ShowBalloons ) {
      valuemask = CWOverrideRedirect | CWEventMask | CWBackPixel | CWBorderPixel
	| CWColormap;

      /* tell WM to ignore this window */
      attributes.override_redirect = True;

      attributes.event_mask = ExposureMask;
      attributes.border_pixel = (Desks[i].ballooncolorset < 0)
	? balloon_border_pix
	: Colorset[Desks[i].ballooncolorset].fg;

      /* if given in config set this now, otherwise it'll be set for each
	 pager window when drawn later */
      attributes.background_pixel = (Desks[i].ballooncolorset < 0)
	? balloon_back_pix
	: Colorset[Desks[i].ballooncolorset].bg;
      if (Desks[i].ballooncolorset > -1 &&
          Colorset[Desks[i].ballooncolorset].pixmap)
      {
        valuemask |= CWBackPixmap;
        valuemask &= ~CWBackPixel;
        attributes.background_pixmap = None; /* set later */
      }

      /* get font for balloon */
#ifdef I18N_MB
      if ( (Desks[i].balloon.fontset = XCreateFontSet(dpy, BalloonFont, &ml,
						      &mc, &ds)) == NULL ) {
#ifdef STRICTLY_FIXED
	if ( (Desks[i].balloon.fontset = XCreateFontSet(dpy, "fixed", &ml, &mc,
							&ds)) == NULL )
#else
	if ( (Desks[i].balloon.fontset =
	      XCreateFontSet(dpy,
			     "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*",
			     &ml, &mc, &ds)) == NULL )
#endif
	{
	  fprintf(stderr,"%s: No fonts available.\n", MyName);
	  exit(1);
	}
	fprintf(stderr, "%s: Can't find font '%s', using fixed.\n",
		MyName, BalloonFont);
      }
      XFontsOfFontSet(Desks[i].balloon.fontset, &fs_list, &ml);
      Desks[i].balloon.font = fs_list[0];
#else
      if ( (Desks[i].balloon.font = XLoadQueryFont(dpy, BalloonFont)) == NULL )
      {
	if ( (Desks[i].balloon.font = XLoadQueryFont(dpy, "fixed")) == NULL ) {
	  fprintf(stderr,"%s: No fonts available.\n", MyName);
	  exit(1);
	}
	fprintf(stderr, "%s: Can't find font '%s', using fixed.\n",
		MyName, BalloonFont);
      }
#endif

      Desks[i].balloon.height =
	Desks[i].balloon.font->ascent + Desks[i].balloon.font->descent + 1;

      /* this may have been set in config */
      Desks[i].balloon.border = BalloonBorderWidth;


      /* we don't allow yoffset of 0 because it allows direct transit
	 from pager window to balloon window, setting up a
	 LeaveNotify/EnterNotify event loop */
      if ( BalloonYOffset )
	Desks[i].balloon.yoffset = BalloonYOffset;
      else {
	fprintf(stderr,
		"%s: Warning:"
		" you're not allowed BalloonYOffset 0; defaulting to +3\n",
		MyName);
	Desks[i].balloon.yoffset = 3;
      }

      /* now create the window */
      Desks[i].balloon.w = XCreateWindow(dpy, Scr.Root,
					 0, 0, /* coords set later */
					 1, Desks[i].balloon.height,
					 Desks[i].balloon.border, Pdepth,
					 InputOutput, Pvisual, valuemask,
					 &attributes);

      /* set font */
      gcv.font = Desks[i].balloon.font->fid;

      /* if fore given in config set now, otherwise it'll be set later */
      gcv.foreground = (Desks[i].ballooncolorset < 0)
	? balloon_fore_pix
	: Colorset[Desks[i].ballooncolorset].fg;

      Desks[i].BalloonGC = XCreateGC(dpy, Desks[i].balloon.w,
				     GCFont | GCForeground, &gcv);
/* don't do this yet, wait for map since size will change
      if (Desks[i].ballooncolorset > -1 &&
          Colorset[Desks[i].ballooncolorset].pixmap)
      {
	SetWindowBackground(dpy, Desks[i].balloon.w, 100,
			    Desks[i].balloon.height,
			    &Colorset[Desks[i].ballooncolorset],
			    Pdepth, Desks[i].BalloonGC, True);
      }
*/

      /* Make sure we don't get balloons initially with the Icon option. */
      ShowBalloons = ShowPagerBalloons;
    } /* ShowBalloons */
  }
  XMapRaised(dpy,Scr.Pager_w);
}


#ifdef SHAPE
void UpdateWindowShape ()
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

  XShapeCombineRectangles (dpy, Scr.Pager_w, ShapeBounding, 0, 0,
			   shape, shape_count, ShapeSet, 0);
}

#endif



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
      ReConfigure();
      break;
    case Expose:
      HandleExpose(Event, False);
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
	  sprintf(command,"Scroll %d %d\n", dx, dy);
	  SendInfo(fd, command, 0);
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
      else if((Event->xbutton.button == 1)||
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
	  for(i=0;i<ndesks;i++)
	    {
	      if(Event->xany.window == Desks[i].w)
		{
		  XQueryPointer(dpy, Desks[i].w, &JunkRoot, &JunkChild,
				&JunkX, &JunkY,&x, &y, &JunkMask);
		  Scroll(desk_w, desk_h, x, y, Scr.CurrentDesk);
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
	      XQueryPointer(dpy, icon_win, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY,&x, &y, &JunkMask);
	      Scroll(icon_w, icon_h, x, y, -1);
	    }
	}
      break;
    case MotionNotify:
      do_ignore_next_button_release = False;
      while(XCheckMaskEvent(dpy, PointerMotionMask | ButtonMotionMask,Event))
	;

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
    SendInfo(fd, "Silent FlipFocus NoWarp", t->w);
  }

}

void HandleExpose(XEvent *Event, Bool redraw_subwindows)
{
  int i;
  PagerWindow *t;

  for(i=0;i<ndesks;i++)
  {
    /* ric@giccs.georgetown.edu */
    if ( Event->xany.window == Desks[i].balloon.w )
    {
      DrawInBalloonWindow(i);
      return;
    }

    if((Event->xany.window == Desks[i].w)
       ||(Event->xany.window == Desks[i].title_w))
      DrawGrid(i,0);
  }
  if(Event->xany.window == icon_win)
    DrawIconGrid(0);

  for (t = Start; t != NULL; t = t->next)
  {
    if(t->PagerView == Event->xany.window || redraw_subwindows)
    {
      LabelWindow(t);
      PictureWindow(t);
      BorderWindow(t);
    }
    if(t->IconView == Event->xany.window || redraw_subwindows)
    {
      LabelIconWindow(t);
      PictureIconWindow(t);
      BorderIconWindow(t);
    }
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

  n1 = Scr.Vx / Scr.MyDisplayWidth;
  m1 = Scr.Vy / Scr.MyDisplayHeight;
  n = Scr.VxMax / Scr.MyDisplayWidth;
  m = Scr.VyMax / Scr.MyDisplayHeight;
  if (window_w > 0)
    window_w = ((window_w - n)/(n+1))*(n+1)+n;
  if (window_h > 0)
    window_h = ((window_h - m)/(m+1))*(m+1)+m;
  desk_w = (window_w - Columns + 1)/Columns;
  desk_h = (window_h - Rows*label_h - Rows + 1)/Rows;
  w = (desk_w - n)/(n+1);
  h = (desk_h - m)/(m+1);

  sizehints.width_inc = Columns*(n+1);
  sizehints.height_inc = Rows*(m+1);
  sizehints.base_width = Columns * n + Columns - 1;
  sizehints.base_height = Rows*(m + label_h+1) - 1;
  sizehints.min_width = sizehints.base_width;
  sizehints.min_height = sizehints.base_height;

  XSetWMNormalHints(dpy,Scr.Pager_w,&sizehints);

  x = (desk_w - n) * Scr.Vx / Scr.VWidth + n1;
  y = (desk_h - m) * Scr.Vy / Scr.VHeight + m1;

  for(k=0;k<Rows;k++)
  {
    for(j=0;j<Columns;j++)
    {
      i = k*Columns+j;

      if(i<ndesks)
      {
	int y_pos;
	XMoveResizeWindow(dpy,Desks[i].title_w,
			  (desk_w+1)*j-1,(desk_h+label_h+1)*k-1,
			  desk_w,desk_h+label_h);
	y_pos = (LabelsBelow ? 0 : label_h - 1);
	XMoveResizeWindow(dpy,Desks[i].w,-1, y_pos,
			  desk_w,desk_h);
        if (Desks[i].colorset > -1 &&
            Colorset[Desks[i].colorset].pixmap)
        {
          SetWindowBackground(
              dpy, Desks[i].w, desk_w, desk_h, &Colorset[Desks[i].colorset],
              Pdepth, Scr.NormalGC, True);
	}
	if (HilightDesks)
	{
	  if(i == Scr.CurrentDesk - desk1)
	    XMoveResizeWindow(dpy, Desks[i].CPagerWin, x,y,w,h);
	  else
	    XMoveResizeWindow(dpy, Desks[i].CPagerWin, -1000,-100,w,h);
	  if (Desks[i].highcolorset > -1 &&
	      Colorset[Desks[i].highcolorset].pixmap)
	  {
	    SetWindowBackground(
                dpy, Desks[i].CPagerWin, w, h, &Colorset[Desks[i].highcolorset],
                Pdepth, Scr.NormalGC, True);
	  }
	}

	XClearArea(dpy,Desks[i].w, 0, 0, 0, 0,True);
	if(uselabel)
	  XClearArea(dpy,Desks[i].title_w, 0, 0, 0, 0,True);
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
#ifdef I18N_MB
  int ret;
#endif
  char str[100],*sptr;
  static int icon_desk_shown = -1000;

  Wait = 0;
  n1 = Scr.Vx/Scr.MyDisplayWidth;
  m1 = Scr.Vy/Scr.MyDisplayHeight;
  n = Scr.VxMax / Scr.MyDisplayWidth;
  m = Scr.VyMax / Scr.MyDisplayHeight;

  x = (desk_w - n) * Scr.Vx / Scr.VWidth + n1;
  y = (desk_h - m) * Scr.Vy / Scr.VHeight + m1;
  for(i=0;i<ndesks;i++)
  {
    if (HilightDesks)
    {
      if(i == Scr.CurrentDesk - desk1)
      {
	XMoveWindow(dpy, Desks[i].CPagerWin, x,y);
	XLowerWindow(dpy,Desks[i].CPagerWin);
      }
      else
	XMoveWindow(dpy, Desks[i].CPagerWin, -1000,-1000);
    }
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
      sprintf(str,"GoToDesk %d",Scr.CurrentDesk);
      sptr = &str[0];
    }
#ifdef I18N_MB
    if ((ret = XmbTextListToTextProperty(dpy,&sptr,1,XStdICCTextStyle,&name))
        == XNoMemory)
    {
      fprintf(stderr,"%s: cannot allocate window name",MyName);
      return;
    }
    else if (ret != Success)
      return;
#else
    if (XStringListToTextProperty(&sptr,1,&name) == 0)
    {
      fprintf(stderr,"%s: cannot allocate window name",MyName);
      return;
    }
#endif
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
      MoveResizePagerView(t);
      t = t->next;
    }
}

void ReConfigureIcons(void)
{
  PagerWindow *t;
  int x, y, w, h;


  t = Start;
  while(t != NULL)
  {
    CalcGeom(t, icon_w, icon_h, &x, &y, &w, &h);
    t->icon_view_width = w;
    t->icon_view_height = h;
    if(Scr.CurrentDesk == t->desk)
      XMoveResizeWindow(dpy, t->IconView, x, y, w, h);
    else
      XMoveResizeWindow(dpy, t->IconView, -1000, -1000, w, h);
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
  int y, y1, y2, x, x1, x2,d,hor_off,ver_off,w;
  char str[15], *ptr;

  if((i < 0 ) || (i >= ndesks))
    return;

  x = Scr.MyDisplayWidth;
  y1 = 0;
  y2 = desk_h;
  while(x < Scr.VWidth)
  {
    x1 = x * desk_w / Scr.VWidth;
    if (!use_no_separators)
      XDrawLine(dpy,Desks[i].w,Desks[i].DashedGC,x1,y1,x1,y2);
    x += Scr.MyDisplayWidth;
  }

  y = Scr.MyDisplayHeight;
  x1 = 0;
  x2 = desk_w;
  while(y < Scr.VHeight)
  {
    y1 = y * desk_h / Scr.VHeight;
    if (!use_no_separators)
      XDrawLine(dpy,Desks[i].w,Desks[i].DashedGC,x1,y1,x2,y1);
    y += Scr.MyDisplayHeight;
  }
  if(((Scr.CurrentDesk - desk1)  == i) && !ShapeLabels)
  {
    if(uselabel)
      XFillRectangle(dpy,Desks[i].title_w,Desks[i].HiliteGC,
		     0,(LabelsBelow ? desk_h : 0),desk_w,label_h -1);
  }
  else
  {
    if(uselabel && erase)
      XClearArea(dpy,Desks[i].title_w,
		 0,(LabelsBelow ? desk_h : 0),desk_w,label_h - 1,False);
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
    ver_off = (LabelsBelow ? desk_h + font->ascent + 1 : font->ascent + 1);
    if(i == (Scr.CurrentDesk - desk1))
#ifdef I18N_MB
      XmbDrawString (dpy, Desks[i].title_w, fontset, Desks[i].rvGC, hor_off,
		     ver_off, ptr, strlen (ptr));
#else
      XDrawString (dpy, Desks[i].title_w, Desks[i].rvGC, hor_off, ver_off,
		   ptr, strlen (ptr));
#endif
      else
#ifdef I18N_MB
	XmbDrawString (dpy, Desks[i].title_w, fontset, Desks[i].NormalGC,
		       hor_off, ver_off, ptr, strlen (ptr));
#else
	XDrawString (dpy, Desks[i].title_w, Desks[i].NormalGC, hor_off,
		     ver_off, ptr, strlen (ptr));
#endif
  }
#ifdef SHAPE
  UpdateWindowShape ();
#endif
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

  sprintf(command,"GoToDesk 0 %d\n",Desk+desk1);
  SendInfo(fd,command,0);
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
    sprintf(command, "GoToDeskAndPage %d %d %d\n", Desk + desk1, vx, vy);
    SendInfo(fd, command, 0);

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
    sprintf(command, "GotoPage %d %d\n", x, y);
    SendInfo(fd, command, 0);
  }
  Wait = 1;
}

void IconSwitchPage(XEvent *Event)
{
  char command[34];

  sprintf(command,"GotoPage %d %d\n",
	  Event->xbutton.x * Scr.VWidth / (icon_w * Scr.MyDisplayWidth),
	  Event->xbutton.y * Scr.VHeight / (icon_h * Scr.MyDisplayHeight));
  SendInfo(fd, command, 0);
  Wait = 1;
}


void AddNewWindow(PagerWindow *t)
{
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  int i, x, y, w, h;

  i = t->desk - desk1;
  CalcGeom(t, desk_w, desk_h, &x, &y, &w, &h);
  t->pager_view_width = w;
  t->pager_view_height = h;
  valuemask = CWBackPixel | CWEventMask;
  attributes.background_pixel = t->back;
  attributes.event_mask = ExposureMask;

  /* ric@giccs.georgetown.edu -- added Enter and Leave events for
     popping up balloon window */
  attributes.event_mask = ExposureMask | EnterWindowMask | LeaveWindowMask;

  if ((i >= 0) && (i < ndesks))
  {
    t->PagerView = XCreateWindow(dpy,Desks[i].w, x, y, w, h, 0,
				 CopyFromParent, InputOutput, CopyFromParent,
				 valuemask, &attributes);
    if (windowcolorset > -1)
      SetWindowBackground(dpy, t->PagerView, w, h,
			  &Colorset[windowcolorset], Pdepth,
			  Scr.NormalGC, True);
    XMapRaised(dpy, t->PagerView);
  }
  else
    t->PagerView = None;

  CalcGeom(t, icon_w, icon_h, &x, &y, &w, &h);
  t->icon_view_width = w;
  t->icon_view_height = h;
  if(Scr.CurrentDesk == t->desk)
  {
    t->IconView = XCreateWindow(dpy,icon_win, x, y, w, h, 0, CopyFromParent,
				InputOutput, CopyFromParent, valuemask,
				&attributes);
    if (windowcolorset > -1)
      SetWindowBackground(dpy, t->IconView, w, h,
			  &Colorset[windowcolorset], Pdepth,
			  Scr.NormalGC, True);
    XGrabButton(dpy, 2, AnyModifier, t->IconView,
		True, ButtonPressMask | ButtonReleaseMask|ButtonMotionMask,
		GrabModeAsync, GrabModeAsync, None,
		None);
    XMapRaised(dpy,t->IconView);
  }
  else
  {
    t->IconView = XCreateWindow(dpy,icon_win, -1000, -1000, w, h, 0,
				CopyFromParent, InputOutput, CopyFromParent,
				valuemask, &attributes);
    if (windowcolorset > -1)
      SetWindowBackground(dpy, t->IconView, w, h,
			  &Colorset[windowcolorset], Pdepth,
			  Scr.NormalGC, True);
    XMapRaised(dpy,t->IconView);
  }
  Hilight(t,False);
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
  if ((t->pager_view_width != w) || (t->pager_view_height != h)) {
    t->pager_view_width = w;
    t->pager_view_height = h;
    size_changed = True;
  }

  if ((i >= 0) && (i < ndesks))
  {
    XReparentWindow(dpy, t->PagerView, Desks[i].w, x, y);
    if (size_changed)
      XResizeWindow(dpy, t->PagerView, w, h);
    if ((t != FocusWin) && (windowcolorset > -1) &&
	(size_changed || Colorset[windowcolorset].pixmap == ParentRelative)) {
      SetWindowBackground(dpy, t->PagerView, t->pager_view_width,
			  t->pager_view_height,
			  &Colorset[windowcolorset],
			  Pdepth, Scr.NormalGC, True);
    } else if ((t == FocusWin) && (activecolorset > -1) &&
	(size_changed || Colorset[activecolorset].pixmap == ParentRelative)) {
      SetWindowBackground(dpy, t->PagerView, t->pager_view_width,
			  t->pager_view_height,
			  &Colorset[activecolorset],
			  Pdepth, Scr.NormalGC, True);
    }
  }
  else
  {
    XDestroyWindow(dpy,t->PagerView);
    t->PagerView = None;
  }
  t->desk = i+desk1;

  CalcGeom(t, icon_w, icon_h, &x, &y, &w, &h);
  if ((t->icon_view_width != w) || (t->icon_view_height != h)) {
    t->icon_view_width = w;
    t->icon_view_height = h;
    size_changed = True;
  } else
    size_changed = False;
  if(Scr.CurrentDesk != t->desk)
    XMoveResizeWindow(dpy,t->IconView,-1000,-1000,w,h);
  else {
    XMoveResizeWindow(dpy,t->IconView,x,y,w,h);
    if ((t != FocusWin) && (windowcolorset > -1) &&
	(size_changed || Colorset[windowcolorset].pixmap == ParentRelative)) {
      SetWindowBackground(dpy, t->IconView, t->icon_view_width,
			  t->icon_view_height,
			  &Colorset[windowcolorset],
			  Pdepth, Scr.NormalGC, True);
    } else if ((t == FocusWin) && (activecolorset > -1) &&
	(size_changed || Colorset[activecolorset].pixmap == ParentRelative)) {
      SetWindowBackground(dpy, t->IconView, t->icon_view_width,
			  t->icon_view_height,
			  &Colorset[activecolorset],
			  Pdepth, Scr.NormalGC, True);
    }
  }
}

void MoveResizePagerView(PagerWindow *t)
{
  int x, y, w, h;
  Bool size_changed = False;

  CalcGeom(t, desk_w, desk_h, &x, &y, &w, &h);
  if (t->pager_view_width != w || t->pager_view_height != h) {
    t->pager_view_width = w;
    t->pager_view_height = h;
    size_changed = True;
  }
  if(t->PagerView != None) {
    XMoveResizeWindow(dpy,t->PagerView,x,y,w,h);
    if ((t != FocusWin) && (windowcolorset > -1) &&
	(size_changed || Colorset[windowcolorset].pixmap == ParentRelative)) {
      SetWindowBackground(dpy, t->PagerView, t->pager_view_width,
			  t->pager_view_height,
			  &Colorset[windowcolorset],
			  Pdepth, Scr.NormalGC, True);
    } else if ((t == FocusWin) && (activecolorset > -1) &&
	(size_changed || Colorset[activecolorset].pixmap == ParentRelative)) {
      SetWindowBackground(dpy, t->PagerView, t->pager_view_width,
			  t->pager_view_height,
			  &Colorset[activecolorset],
			  Pdepth, Scr.NormalGC, True);
    }
  } else if((t->desk >= desk1)&&(t->desk <= desk2))
    {
      XDestroyWindow(dpy,t->IconView);
      AddNewWindow(t);
      return;
    }

  CalcGeom(t, icon_w, icon_h, &x, &y, &w, &h);
  if (t->icon_view_width != w || t->icon_view_height != h) {
    t->icon_view_width = w;
    t->icon_view_height = h;
    size_changed = True;
  } else
    size_changed = False;

  if(Scr.CurrentDesk == t->desk) {
    XMoveResizeWindow(dpy,t->IconView,x,y,w,h);
    if ((t != FocusWin) && (windowcolorset > -1) &&
	(size_changed || Colorset[windowcolorset].pixmap == ParentRelative)) {
      SetWindowBackground(dpy, t->IconView, t->icon_view_width,
			  t->icon_view_height,
			  &Colorset[windowcolorset],
			  Pdepth, Scr.NormalGC, True);
    } else if ((t == FocusWin) && (activecolorset > -1) &&
	(size_changed || Colorset[activecolorset].pixmap == ParentRelative)) {
      SetWindowBackground(dpy, t->IconView, t->icon_view_width,
			  t->icon_view_height,
			  &Colorset[activecolorset],
			  Pdepth, Scr.NormalGC, True);
    }
  } else
    XMoveResizeWindow(dpy,t->IconView,-1000,-1000,w,h);
}


void MoveStickyWindows(void)
{
  PagerWindow *t;

  t = Start;
  while(t!= NULL)
    {
      if(((IS_ICONIFIED(t))&&(IS_ICON_STICKY(t)))||
	 (IS_STICKY(t)))
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
	  if(IS_STICKY(t))
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
      if(on)
	{
	  if(t->PagerView != None) {
	    if (activecolorset < 0) {
	      XSetWindowBackground(dpy,t->PagerView,focus_pix);
	      XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
	    } else
	      SetWindowBackground(dpy, t->PagerView, t->pager_view_width,
				  t->pager_view_height,
				  &Colorset[activecolorset],
				  Pdepth, Scr.NormalGC, True);
	  }
	  if (activecolorset < 0) {
	    XSetWindowBackground(dpy,t->IconView,focus_pix);
	    XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
	  } else
	    SetWindowBackground(dpy, t->IconView, t->icon_view_width,
				t->icon_view_height,
			        &Colorset[activecolorset], Pdepth,
			        Scr.NormalGC, True);
	}
      else
	{
	  if(t->PagerView != None) {
	    if (windowcolorset < 0) {
	      XSetWindowBackground(dpy,t->PagerView,t->back);
	      XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
	    } else
	      SetWindowBackground(dpy, t->PagerView, t->pager_view_width,
				  t->pager_view_height,
				  &Colorset[windowcolorset],
				  Pdepth, Scr.NormalGC, True);
	  }
	  if (windowcolorset < 0) {
	    XSetWindowBackground(dpy,t->IconView,t->back);
	    XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
	  } else
	    SetWindowBackground(dpy, t->IconView, t->icon_view_width,
				t->icon_view_height,
				&Colorset[windowcolorset], Pdepth,
				Scr.NormalGC, True);
	}
    }
}

/* Use Desk == -1 to scroll the icon window */
void Scroll(int window_w, int window_h, int x, int y, int Desk)
{
  char command[256];
  int sx, sy;
  if(Wait == 0)
    {
      /* Desk < 0 means we want to scroll an icon window */
      if(Desk >= 0 && Desk + desk1 != Scr.CurrentDesk)
	{
	  return;
	}

      /* center around mouse */
      x -= (desk_w / (1 + Scr.VxMax / Scr.MyDisplayWidth)) / 2;
      y -= (desk_h / (1 + Scr.VyMax / Scr.MyDisplayHeight)) / 2;

      if(x < 0)
	x = 0;
      if(y < 0)
	y = 0;

      if(x > window_w)
	x = window_w;
      if(y > window_h)
	y = window_h;

      sx = 0;
      sy = 0;
      if(window_w != 0)
	sx = 100 * (x * Scr.VWidth / window_w - Scr.Vx) / Scr.MyDisplayWidth;
      if(window_h != 0)
	sy = 100 * (y * Scr.VHeight / window_h - Scr.Vy) / Scr.MyDisplayHeight;
#ifdef DEBUG
      fprintf(stderr,"[scroll]: %d %d %d %d %d %d\n", window_w, window_h, x,
	      y, sx, sy);
#endif
      /* Make sure weExecuteCommandQueue(); don't get stuck a few pixels fromt the top/left border.
       * Since sx/sy are ints, values between 0 and 1 are rounded down. */
      if(sx == 0 && x == 0 && Scr.Vx != 0)
	sx = -1;
      if(sy == 0 && y == 0 && Scr.Vy != 0)
	sy = -1;

      sprintf(command,"Scroll %d %d\n",sx,sy);
      SendInfo(fd,command,0);
      Wait = 1;
    }
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
  Window JunkRoot, JunkChild;
  int JunkX, JunkY;
  unsigned JunkMask;

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

  n = Scr.VxMax / Scr.MyDisplayWidth;
  m = Scr.VyMax / Scr.MyDisplayHeight;
  n1 = (Scr.Vx + t->x) / Scr.MyDisplayWidth;
  m1 = (Scr.Vy + t->y) / Scr.MyDisplayHeight;
  wx = (Scr.Vx + t->x) * (desk_w - n) / Scr.VWidth + n1;
  wy = (Scr.Vy + t->y) * (desk_h - m) / Scr.VHeight + m1;
  wx1 = wx + (desk_w + 1) * (NewDesk % Columns);
  wy1 = wy + label_h + (desk_h + label_h + 1) * (NewDesk / Columns);

  XReparentWindow(dpy, t->PagerView, Scr.Pager_w, wx1, wy1);
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
      XMoveWindow(dpy, t->PagerView, x - (x1), y - (y1));
    }
    else if(Event->type == ButtonRelease)
    {
      XTranslateCoordinates(dpy, Event->xany.window, Scr.Pager_w,
			    Event->xbutton.x, Event->xbutton.y, &x, &y,
			    &dumwin);
      XMoveWindow(dpy, t->PagerView, x - x1, y - y1);
      finished = 1;
    }
    else if (Event->type == Expose)
    {
      HandleExpose(Event, False);
    }
  }

  if(moved)
  {
    if((x - xi < MoveThreshold)&&(y - yi < MoveThreshold)&&
       (x - xi > -MoveThreshold)&&(y -yi > -MoveThreshold))
      moved = 0;
  }
  if(KeepMoving)
  {
    NewDesk = Scr.CurrentDesk;
    if(NewDesk != t->desk)
    {
      XMoveWindow(dpy, IS_ICONIFIED(t) ? t->icon_w : t->w,
		  Scr.VWidth, Scr.VHeight);
      XSync(dpy, False);
      sprintf(command, "Silent MoveToDesk 0 %d", NewDesk);
      SendInfo(fd, command, t->w);
      t->desk = NewDesk;
    }
    if((NewDesk>=desk1)&&(NewDesk<=desk2))
      XReparentWindow(dpy, t->PagerView, Desks[NewDesk-desk1].w,0,0);
    else
    {
      XDestroyWindow(dpy,t->PagerView);
      t->PagerView = None;
    }
    XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		  &x, &y, &JunkX, &JunkY, &JunkMask);
    XUngrabPointer(dpy,CurrentTime);
    XSync(dpy,0);
    sprintf(command, "Silent Move %dp %dp", x, y);
    SendInfo(fd,command,t->w);
    SendInfo(fd,"Silent Raise",t->w);
    SendInfo(fd,"Silent Move Pointer",t->w);
    return;
  }
  else
  {
    column = (x/(desk_w+1));
    if (column >= Columns)
      column = Columns - 1;
    if (column < 0)
      column = 0;
    row =  (y/(desk_h+ label_h+1));
    if (row >= Rows)
      row = Rows - 1;
    if (row < 0)
      row = 0;
    NewDesk = column + (row)*Columns;
    while (NewDesk < 0)
    {
      NewDesk += Columns;
      if (NewDesk >= ndesks)
	NewDesk = 0;
    }
    while (NewDesk >= ndesks)
    {
      NewDesk -= Columns;
      if (NewDesk < 0)
	NewDesk = ndesks - 1;
    }
    XTranslateCoordinates(dpy, Scr.Pager_w,Desks[NewDesk].w,
			  x-x1, y-y1, &x2,&y2,&dumwin);

    n1 = x2 * Scr.VWidth / (desk_w * Scr.MyDisplayWidth);
    m1 = y2* Scr.VHeight / (desk_h * Scr.MyDisplayHeight);
    x = (x2 - n1) * Scr.VWidth / (desk_w - n) - Scr.Vx;
    y = (y2 - m1) * Scr.VHeight / (desk_h - m) - Scr.Vy;
    /* force onto desk */
    if (x + t->frame_width + Scr.Vx < 0 )
      x = - Scr.Vx - t->frame_width;
    if (y + t->frame_height + Scr.Vy < 0)
      y = - Scr.Vy - t->frame_height;
    if (x + Scr.Vx >= Scr.VWidth)
      x = Scr.VWidth - Scr.Vx - 1;
    if (y + Scr.Vy >= Scr.VHeight)
      y = Scr.VHeight - Scr.Vy - 1;
    if(((IS_ICONIFIED(t))&&(IS_ICON_STICKY(t)))||(IS_STICKY(t)))
    {
      NewDesk = Scr.CurrentDesk - desk1;
      if (x > Scr.MyDisplayWidth - 16)
	x = Scr.MyDisplayWidth - 16;
      if (y > Scr.MyDisplayHeight - 16)
	y = Scr.MyDisplayHeight - 16;
      if (x + t->width < 16)
	x = 16 - t->width;
      if (y + t->height < 16)
	y = 16 - t->height;
    }
    if(NewDesk + desk1 != t->desk)
    {
      if(((IS_ICONIFIED(t))&&(IS_ICON_STICKY(t)))||(IS_STICKY(t)))
      {
	NewDesk = Scr.CurrentDesk - desk1;
	if(t->desk != Scr.CurrentDesk)
	  ChangeDeskForWindow(t,Scr.CurrentDesk);
      }
      else
      {
	sprintf(command,"Silent MoveToDesk 0 %d", NewDesk + desk1);
	SendInfo(fd,command,t->w);
	t->desk = NewDesk + desk1;
      }
    }

    if((NewDesk >= 0)&&(NewDesk < ndesks))
    {
      XReparentWindow(dpy, t->PagerView, Desks[NewDesk].w,x,y);
      t->desk = NewDesk;
      XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
      if(moved)
      {
	if(IS_ICONIFIED(t))
	{
	  XMoveWindow(dpy,t->icon_w,x,y);
	}
	else
	{
	  XMoveWindow(dpy,t->w,x+t->border_width,
		      y+t->title_height+t->border_width);
	}
	XSync(dpy,0);
      }
      else
	MoveResizePagerView(t);
      SendInfo(fd, "Silent Raise", t->w);
    }
    if(Scr.CurrentDesk == t->desk)
    {
      XSync(dpy,0);
      usleep(5000);
      XSync(dpy,0);

      SendInfo(fd, "Silent FlipFocus NoWarp", t->w);
    }
  }
  if (is_transient)
  {
    /* does not return */
    ExitPager();
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


void BorderWindow(PagerWindow *t)
{
  if (t->PagerView != None) {
    if ((!WindowBorders3d) || (windowcolorset < 0) || (activecolorset < 0)) {
      XSetForeground(dpy, Scr.NormalGC, Scr.black);
      RelieveRectangle(dpy, t->PagerView, 0, 0, t->pager_view_width - 1,
        	       t->pager_view_height - 1, Scr.NormalGC, Scr.NormalGC,
        	       WindowBorderWidth);
    } else if (t == FocusWin)
      RelieveRectangle(dpy, t->PagerView, 0, 0, t->pager_view_width - 1,
		       t->pager_view_height - 1, Scr.ahGC, Scr.asGC,
		       WindowBorderWidth);
    else
      RelieveRectangle(dpy, t->PagerView, 0, 0, t->pager_view_width - 1,
		       t->pager_view_height - 1, Scr.whGC, Scr.wsGC,
		       WindowBorderWidth);
  }
}

void BorderIconWindow(PagerWindow *t)
{
  if (t->IconView != None) {
    if ((!WindowBorders3d) || (windowcolorset < 0) || (activecolorset < 0)) {
      XSetForeground(dpy, Scr.NormalGC, Scr.black);
      RelieveRectangle(dpy, t->IconView, 0, 0, t->icon_view_width - 1,
        	       t->icon_view_height - 1, Scr.NormalGC, Scr.NormalGC,
        	       WindowBorderWidth);
    } else if (t == FocusWin)
      RelieveRectangle(dpy, t->IconView, 0, 0, t->icon_view_width - 1,
		       t->icon_view_height - 1, Scr.ahGC, Scr.asGC,
		       WindowBorderWidth);
    else
      RelieveRectangle(dpy, t->IconView, 0, 0, t->icon_view_width - 1,
		       t->icon_view_height - 1, Scr.whGC, Scr.wsGC,
		       WindowBorderWidth);
  }
}

void LabelWindow(PagerWindow *t)
{
  if(t == FocusWin)
    XSetForeground(dpy, Scr.NormalGC, focus_fore_pix);
  else
    XSetForeground(dpy, Scr.NormalGC, t->text);

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

  /* Update the window label for this window */
  if (t->window_label)
    free(t->window_label);
  t->window_label = GetBalloonLabel(t,WindowLabelFormat);
  if(t->PagerView != None)
  {
#ifdef I18N_MB
    XmbDrawString (dpy, t->PagerView, windowFontset, Scr.NormalGC, 2,
		   windowFont->ascent+2, t->window_label,
		   strlen(t->window_label));
#else
    XDrawString (dpy, t->PagerView, Scr.NormalGC, 2,
		 windowFont->ascent+2, t->window_label,
		 strlen(t->window_label));
#endif
  }
}


void LabelIconWindow(PagerWindow *t)
{
  if(t == FocusWin)
    XSetForeground(dpy, Scr.NormalGC, focus_fore_pix);
  else
    XSetForeground(dpy, Scr.NormalGC, t->text);

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

  { /* Update the window label for this window */
    if (t->window_label)
      free(t->window_label);
    t->window_label = GetBalloonLabel(t,WindowLabelFormat);
  }
  {
#ifdef I18N_MB
  XmbDrawString (dpy, t->IconView, windowFontset, Scr.NormalGC, 2,
		 windowFont->ascent+2, t->window_label,
		 strlen(t->window_label));
#else
  XDrawString (dpy, t->IconView, Scr.NormalGC, 2,
		 windowFont->ascent+2, t->window_label,
		 strlen(t->window_label));
#endif
  }
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
	  Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
	  Globalgcv.clip_mask = t->mini_icon.mask;
	  Globalgcv.clip_x_origin = iconX;
	  Globalgcv.clip_y_origin = iconY;
  	  XChangeGC(dpy, Scr.MiniIconGC, Globalgcm, &Globalgcv);
  	  XCopyArea(dpy, t->mini_icon.picture, t->PagerView,  Scr.MiniIconGC, 0,
		    0, t->mini_icon.width, t->mini_icon.height, iconX, iconY);
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
	  Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
	  Globalgcv.clip_mask = t->mini_icon.mask;
	  Globalgcv.clip_x_origin = iconX;
	  Globalgcv.clip_y_origin = iconY;
	  XChangeGC(dpy, Scr.MiniIconGC, Globalgcm, &Globalgcv);
	  XCopyArea(dpy, t->mini_icon.picture, t->IconView, Scr.MiniIconGC, 0,
		    0, t->mini_icon.width, t->mini_icon.height, iconX, iconY);
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
  Window JunkRoot, JunkChild;
  int JunkX, JunkY;
  unsigned JunkMask;

  if(t==NULL)
    return;

  n = Scr.VxMax / Scr.MyDisplayWidth;
  m = Scr.VyMax / Scr.MyDisplayHeight;
  n1 = (Scr.Vx + t->x) / Scr.MyDisplayWidth;
  m1 = (Scr.Vy + t->y) / Scr.MyDisplayHeight;
  wx = (Scr.Vx + t->x) * (icon_w - n) / Scr.VWidth + n1;
  wy = (Scr.Vy + t->y) * (icon_h - m) / Scr.VHeight + m1;

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
	  HandleExpose(Event, False);
	}
    }

  if(moved)
    {
      if((x - xi < MoveThreshold)&&(y - yi < MoveThreshold)&&
	 (x - xi > -MoveThreshold)&&(y -yi > -MoveThreshold))
	moved = 0;
    }

  if(KeepMoving)
    {
      char command[48];

      XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &x, &y, &JunkX, &JunkY, &JunkMask);
      XUngrabPointer(dpy,CurrentTime);
      XSync(dpy,0);
      sprintf(command, "Silent Move %dp %dp", x, y);
      SendInfo(fd,command,t->w);
      SendInfo(fd,"Silent Raise",t->w);
      SendInfo(fd,"Silent Move Pointer",t->w);
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
	x = - Scr.Vx - t->icon_width;
      if (y + t->icon_height + Scr.Vy < 0)
	y = - Scr.Vy - t->icon_height;
      if (x + Scr.Vx >= Scr.VWidth)
	x = Scr.VWidth - Scr.Vx - 1;
      if (y + Scr.Vy >= Scr.VHeight)
	y = Scr.VHeight - Scr.Vy - 1;
      if(((IS_ICONIFIED(t))&&(IS_ICON_STICKY(t)))||(IS_STICKY(t)))
	{
	  if (x > Scr.MyDisplayWidth - 16)
	    x = Scr.MyDisplayWidth - 16;
	  if (y > Scr.MyDisplayHeight - 16)
	    y = Scr.MyDisplayHeight - 16;
	  if (x + t->width < 16)
	    x = 16 - t->width;
	  if (y + t->height < 16)
	    y = 16 - t->height;
	}
      if(moved)
	{
	  if(IS_ICONIFIED(t))
	    XMoveWindow(dpy,t->icon_w,x,y);
	  else
	    XMoveWindow(dpy,t->w,x,y);
	  XSync(dpy,0);
	}
      else
	MoveResizePagerView(t);
      SendInfo(fd, "Silent Raise", t->w);
      SendInfo(fd, "Silent FlipFocus NoWarp", t->w);
    }
  if (is_transient)
    {
      /* does not return */
      ExitPager();
    }
}


/* Just maps window ... draw stuff in it later after Expose event
   -- ric@giccs.georgetown.edu */
void MapBalloonWindow(PagerWindow *t, Bool is_icon_view)
{
  XWindowChanges window_changes;
  Window view, dummy;
  int view_width, view_height;
  int x, y;
  extern char *BalloonBack;
  int i;

  if ( !is_icon_view )
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
    i = 0;
  else
    i = t->desk - desk1;
  Desks[i].balloon.pw = t;

  /* get the label for this balloon */
  if (Desks[i].balloon.label)
    free(Desks[i].balloon.label);
  Desks[i].balloon.label = GetBalloonLabel(Desks[i].balloon.pw,
					   BalloonFormatString);

#if 0
  if (*Desks[i].balloon.label == 0)
    /* dont draw empty labels */
    return;
#endif

  /* calculate window width to accommodate string */
  window_changes.width = 4;
  if (*Desks[i].balloon.label)
  {
    window_changes.width +=
      XTextWidth(Desks[i].balloon.font,	Desks[i].balloon.label,
		 strlen(Desks[i].balloon.label));
  }

  /* get x and y coords relative to pager window */
  x = (view_width / 2) - (window_changes.width / 2) - Desks[i].balloon.border;

  if ( Desks[i].balloon.yoffset > 0 )
    y = view_height + Desks[i].balloon.yoffset - 1;
  else
    y = Desks[i].balloon.yoffset - Desks[i].balloon.height + 1 -
      (2 * Desks[i].balloon.border);


  /* balloon is a top-level window, therefore need to
     translate pager window coords to root window coords */
  XTranslateCoordinates(dpy, view, Scr.Root, x, y,
                        &window_changes.x, &window_changes.y, &dummy);


  /* make sure balloon doesn't go off screen
     (actually 2 pixels from edge rather than 0 just to be pretty :-) */

  /* too close to top ... make yoffset +ve */
  if ( window_changes.y < 2 ) {
    y = - Desks[i].balloon.yoffset - 1 + view_height;
    XTranslateCoordinates(dpy, view, Scr.Root, x, y,
			  &window_changes.x, &window_changes.y, &dummy);
  }

  /* too close to bottom ... make yoffset -ve */
  else if ( window_changes.y + Desks[i].balloon.height >
	    Scr.MyDisplayHeight - (2 * Desks[i].balloon.border) - 2 ) {
    y = - Desks[i].balloon.yoffset + 1 - Desks[i].balloon.height -

      (2 * Desks[i].balloon.border);
    XTranslateCoordinates(dpy, view, Scr.Root, x, y,
			  &window_changes.x, &window_changes.y, &dummy);
  }

  /* too close to left */
  if ( window_changes.x < 2 )
    window_changes.x = 2;

  /* too close to right */
  else if ( window_changes.x + window_changes.width >
            Scr.MyDisplayWidth - (2 * Desks[i].balloon.border) - 2 )
    window_changes.x = Scr.MyDisplayWidth - window_changes.width -
      (2 * Desks[i].balloon.border) - 2;


  /* make changes to window */
  XConfigureWindow(dpy, Desks[i].balloon.w, CWX | CWY | CWWidth,
		   &window_changes);

  /* if background not set in config make it match pager window */
  if ( BalloonBack == NULL )
    XSetWindowBackground(dpy, Desks[i].balloon.w, t->back);

  if (Desks[i].ballooncolorset > -1) {
    SetWindowBackground(dpy, Desks[i].balloon.w, window_changes.width,
			Desks[i].balloon.height,
			&Colorset[Desks[i].ballooncolorset],
			Pdepth, Scr.NormalGC, True);
    XSetWindowBorder(dpy, Desks[i].balloon.w,
		     Colorset[Desks[i].ballooncolorset].fg);
  }

  XMapRaised(dpy, Desks[i].balloon.w);
}


static void InsertExpand(char **dest, char *s)
{
  int len;
  char *tmp = *dest;

  if (!s || !*s)
    return;
  len = strlen(*dest) + strlen(s) + 1;
  *dest = (char *)safemalloc(len);
  strcpy(*dest, tmp);
  free(tmp);
  strcat(*dest, s);
  return;
}


/* Generate the BallonLabel from the format string
   -- disching@fmi.uni-passau.de */
char *GetBalloonLabel(const PagerWindow *pw,const char *fmt)
{
  char *dest = strdup("");
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
void UnmapBalloonWindow ()
{
  int i;
  for(i=0;i<ndesks;i++)
  {
    XUnmapWindow(dpy, Desks[i].balloon.w);
  }
  BalloonView = None;
}


/* Draws string in balloon window -- call after it's received Expose event
   -- ric@giccs.georgetown.edu */
void DrawInBalloonWindow (int i)
{
  extern char *BalloonFore;

  /* if foreground not set in config make it match pager window */
  if ( (Desks[i].ballooncolorset < 0) && BalloonFore == NULL )
    XSetForeground(dpy, Desks[i].BalloonGC, Desks[i].balloon.pw->text);

#ifdef I18N_MB
  XmbDrawString(dpy, Desks[i].balloon.w, Desks[i].balloon.fontset, Desks[i].BalloonGC,
#else
  XDrawString(dpy, Desks[i].balloon.w, Desks[i].BalloonGC,
#endif
              2, Desks[i].balloon.font->ascent,
              Desks[i].balloon.label,strlen(Desks[i].balloon.label));
}

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
	  SetWindowBackground(dpy, Desks[i].title_w, 0, 0,
			      &Colorset[colorset], Pdepth,
			      Scr.NormalGC, True);
	}
        SetWindowBackground(dpy, Desks[i].CPagerWin, 0, 0,
			    &Colorset[colorset], Pdepth,
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
	SetWindowBackground(dpy, Desks[i].title_w, 0, 0,
			    &Colorset[colorset], Pdepth,
			    Scr.NormalGC, True);
      }
      SetWindowBackground(dpy, Desks[i].w, 0, 0,
			  &Colorset[colorset], Pdepth,
			  Scr.NormalGC, True);
    }
    else if (Desks[i].highcolorset == colorset && uselabel)
      SetWindowBackground(dpy, Desks[i].title_w, 0, 0,
			  &Colorset[Desks[i].colorset], Pdepth,
			  Scr.NormalGC, True);

    if (ShowBalloons && Desks[i].ballooncolorset == colorset)
    {
      XSetWindowBackground(dpy, Desks[i].balloon.w, Colorset[colorset].fg);
      XSetForeground(dpy,Desks[i].BalloonGC,Colorset[colorset].fg);
      XClearArea(dpy, Desks[i].balloon.w, 0, 0, 0, 0, True);
    }

  }

  if (windowcolorset == colorset)
  {
    colorset_struct *wcsetp = &Colorset[colorset];

    XSetForeground(dpy, Scr.whGC, wcsetp->hilite);
    XSetForeground(dpy, Scr.wsGC, wcsetp->shadow);
    for (t = Start; t != NULL; t = t->next)
    {
      t->text = Colorset[colorset].fg;
      if(t != FocusWin) {
        if(t->PagerView != None)
          SetWindowBackground(dpy, t->PagerView, t->pager_view_width,
			      t->pager_view_height, wcsetp, Pdepth,
			      Scr.NormalGC, True);
        SetWindowBackground(dpy, t->IconView, t->icon_view_width,
			    t->icon_view_height, wcsetp, Pdepth, Scr.NormalGC,
			    True);
      }
    }
  }

  if (activecolorset == colorset)
  {
    colorset_struct *acsetp = &Colorset[colorset];

    focus_fore_pix = acsetp->fg;
    XSetForeground(dpy, Scr.ahGC, acsetp->hilite);
    XSetForeground(dpy, Scr.asGC, acsetp->shadow);
    win_hi_back_pix = acsetp->bg;
    win_hi_fore_pix = acsetp->fg;
    t = FocusWin;
    if (t)
    {
      if(t->PagerView != None)
      {
        SetWindowBackground(dpy, t->PagerView, t->pager_view_width,
			    t->pager_view_height, acsetp, Pdepth,
			    Scr.NormalGC, True);
      }
      SetWindowBackground(dpy, t->IconView, t->icon_view_width,
			  t->icon_view_height, acsetp, Pdepth, Scr.NormalGC,
			  True);
    }
  }
}
