/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/


/***********************************************************************
 *
 * fvwm window border drawing code
 *
 ***********************************************************************/

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "fvwm.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

void DrawButton(FvwmWindow *t,
                Window win,
                int W,
                int H,
                ButtonFace *bf,
                GC ReliefGC, GC ShadowGC,
		Boolean inverted,
		int stateflags);

#ifdef VECTOR_BUTTONS
void DrawLinePattern(Window win,
                     GC ReliefGC,
                     GC ShadowGC,
		     struct vector_coords *coords,
                     int w, int h);
#endif

/* macro rules to get button state */
#if defined(ACTIVEDOWN_BTNS) && defined(INACTIVE_BTNS)
#define GetButtonState(window)						\
        (onoroff ? ((PressedW == (window)) ? ActiveDown : ActiveUp)	\
        : Inactive)
#else
#ifdef ACTIVEDOWN_BTNS
#define GetButtonState(window)						\
        ((PressedW == (window)) ? ActiveDown : ActiveUp)
#endif
#ifdef INACTIVE_BTNS
#define GetButtonState(window) (onoroff ? ActiveUp : Inactive)
#endif
#endif

#if !defined(ACTIVEDOWN_BTNS) && !defined(INACTIVE_BTNS)
#define GetButtonState(window) ActiveUp
#endif

/* macro to change window background color/pixmap */
#define ChangeWindowColor(window,valuemask) {				\
        if(NewColor)							\
        {								\
          XChangeWindowAttributes(dpy,window,valuemask, &attributes);	\
          XClearWindow(dpy,window);					\
        }								\
      }

extern Window PressedW;
XGCValues Globalgcv;
unsigned long Globalgcm;
/****************************************************************************
 *
 * Redraws the windows borders
 *
 ****************************************************************************/
void SetBorder (FvwmWindow *t, Bool onoroff,Bool force,Bool Mapped,
		Window expose_win)
{
  int y, i, x;
  GC ReliefGC,ShadowGC;
  Pixel BorderColor,BackColor;
  Pixmap BackPixmap,TextColor;
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
  Pixmap TexturePixmap = None;
  XSetWindowAttributes notex_attributes;
  unsigned long notex_valuemask;
#endif
  Bool NewColor = False;
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  static unsigned int corners[4];
  Window w;

  corners[0] = TOP_HILITE | LEFT_HILITE;
  corners[1] = TOP_HILITE | RIGHT_HILITE;
  corners[2] = BOTTOM_HILITE | LEFT_HILITE;
  corners[3] = BOTTOM_HILITE | RIGHT_HILITE;

  if(!t)
    return;

  if (onoroff)
  {
    /* don't re-draw just for kicks */
    if((!force)&&(Scr.Hilite == t))
      return;

    if(Scr.Hilite != t)
      NewColor = True;

    /* make sure that the previously highlighted window got unhighlighted */
    if((Scr.Hilite != t)&&(Scr.Hilite != NULL))
      SetBorder(Scr.Hilite,False,False,True,None);

#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
    /* are we using textured borders? */
    if ((GetDecor(t,BorderStyle.active.style)
	 & ButtonFaceTypeMask) == TiledPixmapButton)
	TexturePixmap = GetDecor(t,BorderStyle.active.u.p->picture);
#endif

    /* set the keyboard focus */
    if((Mapped)&&(t->flags&MAPPED)&&(Scr.Hilite != t))
      w = t->w;
    else if((t->flags&ICONIFIED)&&
            (Scr.Hilite !=t)&&(!(t->flags &SUPPRESSICON)))
      w = t->icon_w;
    Scr.Hilite = t;

    TextColor = GetDecor(t,HiColors.fore);
    BackPixmap= Scr.gray_pixmap;
    BackColor = GetDecor(t,HiColors.back);
    ReliefGC = GetDecor(t,HiReliefGC);
    ShadowGC = GetDecor(t,HiShadowGC);
    BorderColor = GetDecor(t,HiRelief.back);
  }
  else
  {
    /* don't re-draw just for kicks */
    if((!force)&&(Scr.Hilite != t))
      return;

    if(Scr.Hilite == t)
    {
      Scr.Hilite = NULL;
      NewColor = True;
    }

#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
    if ((GetDecor(t,BorderStyle.inactive.style)
	 & ButtonFaceTypeMask) == TiledPixmapButton)
	TexturePixmap = GetDecor(t,BorderStyle.inactive.u.p->picture);
#endif

    TextColor =t->TextPixel;
    BackPixmap = Scr.light_gray_pixmap;
    if(t->flags & STICKY)
      BackPixmap = Scr.sticky_gray_pixmap;
    BackColor = t->BackPixel;
    Globalgcv.foreground = t->ReliefPixel;
    Globalgcm = GCForeground;
    XChangeGC(dpy,Scr.ScratchGC1,Globalgcm,&Globalgcv);
    ReliefGC = Scr.ScratchGC1;

    Globalgcv.foreground = t->ShadowPixel;
    XChangeGC(dpy,Scr.ScratchGC2,Globalgcm,&Globalgcv);
    ShadowGC = Scr.ScratchGC2;
    BorderColor = t->ShadowPixel;
  }

  if(t->flags & ICONIFIED)
  {
    DrawIconWindow(t);
    return;
  }

#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
  valuemask =
      notex_valuemask =
      CWBorderPixel;
  attributes.border_pixel =
      notex_attributes.border_pixel =
      BorderColor;
#else
  valuemask = CWBorderPixel;
  attributes.border_pixel = BorderColor;
#endif

#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
  if (TexturePixmap)
  {
      attributes.background_pixmap = TexturePixmap;
      valuemask |= CWBackPixmap;
      if (Scr.d_depth < 2) {
	  notex_attributes.background_pixmap = BackPixmap;
	  notex_valuemask |= CWBackPixmap;
      } else {
	  notex_attributes.background_pixel = BackColor;
	  notex_valuemask |= CWBackPixel;
      }
  }
  else
#endif
  if (Scr.d_depth < 2)
  {
      attributes.background_pixmap = BackPixmap;
      valuemask |= CWBackPixmap;
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
      notex_attributes.background_pixmap = BackPixmap;
      notex_valuemask |= CWBackPixmap;
#endif
  }
  else
  {
      attributes.background_pixel = BackColor;
      valuemask |= CWBackPixel;
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
      notex_attributes.background_pixel = BackColor;
      notex_valuemask |= CWBackPixel;
#endif
  }

  if(t->flags & (TITLE|BORDER))
  {
    XSetWindowBorder(dpy,t->Parent,BorderColor);
    XSetWindowBorder(dpy,t->frame,BorderColor);
  }
  if(t->flags & TITLE)
  {
    ChangeWindowColor(t->title_w,valuemask);
    for(i=0;i<Scr.nr_left_buttons;++i)
    {
	if(t->left_w[i] != None)
	{
	    enum ButtonState bs = GetButtonState(t->left_w[i]);
	    ButtonFace *bf = &GetDecor(t,left_buttons[i].state[bs]);
#if !(defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE))
	    ChangeWindowColor(t->left_w[i],valuemask);
#endif
	    if(flush_expose(t->left_w[i])||(expose_win == t->left_w[i])||
	       (expose_win == None)
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
	       || NewColor
#endif
		)
	    {
		int inverted = PressedW == t->left_w[i];
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
		if (bf->style & UseBorderStyle)
		    XChangeWindowAttributes(dpy, t->left_w[i],
					    valuemask, &attributes);
		else
		    XChangeWindowAttributes(dpy, t->left_w[i],
					    notex_valuemask, &notex_attributes);
		XClearWindow(dpy, t->left_w[i]);
#endif
#ifdef EXTENDED_TITLESTYLE
		if (bf->style & UseTitleStyle) {
		    ButtonFace *tsbf = &GetDecor(t,titlebar.state[bs]);
#ifdef MULTISTYLE
		    for (; tsbf; tsbf = tsbf->next)
#endif
			DrawButton(t, t->left_w[i],
				   t->title_height, t->title_height,
				   tsbf, ReliefGC, ShadowGC,
				   inverted, GetDecor(t,left_buttons[i].flags));
		}
#endif /* EXTENDED_TITLESTYLE */
#ifdef MULTISTYLE
		for (; bf; bf = bf->next)
#endif
		    DrawButton(t, t->left_w[i],
			       t->title_height, t->title_height,
			       bf, ReliefGC, ShadowGC,
			       inverted, GetDecor(t,left_buttons[i].flags));

		if (!(GetDecor(t,left_buttons[i].state[bs].style) & FlatButton)) {
		    if (GetDecor(t,left_buttons[i].state[bs].style) & SunkButton)
			RelieveWindow(t,t->left_w[i],0,0,
				      t->title_height, t->title_height,
				      (inverted ? ReliefGC : ShadowGC),
				      (inverted ? ShadowGC : ReliefGC),
				      BOTTOM_HILITE);
		    else
			RelieveWindow(t,t->left_w[i],0,0,
				      t->title_height, t->title_height,
				      (inverted ? ShadowGC : ReliefGC),
				      (inverted ? ReliefGC : ShadowGC),
				      BOTTOM_HILITE);
		}
	    }
	}
    }
    for(i=0;i<Scr.nr_right_buttons;++i)
    {
	if(t->right_w[i] != None)
	{
	    enum ButtonState bs = GetButtonState(t->right_w[i]);
	    ButtonFace *bf = &GetDecor(t,right_buttons[i].state[bs]);
#if !(defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE))
	    ChangeWindowColor(t->right_w[i],valuemask);
#endif
	    if(flush_expose(t->right_w[i])||(expose_win==t->right_w[i])||
	       (expose_win == None)
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
	       || NewColor
#endif
		)
	    {
		int inverted = PressedW == t->right_w[i];
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
		if (bf->style & UseBorderStyle)
		    XChangeWindowAttributes(dpy, t->right_w[i],
					    valuemask, &attributes);
		else
		    XChangeWindowAttributes(dpy, t->right_w[i],
					    notex_valuemask, &notex_attributes);
		XClearWindow(dpy, t->right_w[i]);
#endif
#ifdef EXTENDED_TITLESTYLE
		if (bf->style & UseTitleStyle) {
		    ButtonFace *tsbf = &GetDecor(t,titlebar.state[bs]);
#ifdef MULTISTYLE
		    for (; tsbf; tsbf = tsbf->next)
#endif
			DrawButton(t, t->right_w[i],
				   t->title_height, t->title_height,
				   tsbf, ReliefGC, ShadowGC,
				   inverted, GetDecor(t,right_buttons[i].flags));
		}
#endif /* EXTENDED_TITLESTYLE */
#ifdef MULTISTYLE
		for (; bf; bf = bf->next)
#endif
		    DrawButton(t, t->right_w[i],
			       t->title_height, t->title_height,
			       bf, ReliefGC, ShadowGC,
			       inverted, GetDecor(t,right_buttons[i].flags));

		if (!(GetDecor(t,right_buttons[i].state[bs].style) & FlatButton)) {
		    if (GetDecor(t,right_buttons[i].state[bs].style) & SunkButton)
			RelieveWindow(t,t->right_w[i],0,0,
				      t->title_height, t->title_height,
				      (inverted ? ReliefGC : ShadowGC),
				      (inverted ? ShadowGC : ReliefGC),
				      BOTTOM_HILITE);
		    else
			RelieveWindow(t,t->right_w[i],0,0,
				      t->title_height, t->title_height,
				      (inverted ? ShadowGC : ReliefGC),
				      (inverted ? ReliefGC : ShadowGC),
				      BOTTOM_HILITE);
		}
	    }
	}
    }
    SetTitleBar(t,onoroff, False);

  }

  if(t->flags & BORDER)
  {
    /* draw relief lines */
    y= t->frame_height - 2*t->corner_width;
    x = t->frame_width-  2*t->corner_width +t->bw;

    for(i=0;i<4;i++)
    {
      int vertical = i % 2;
#ifdef BORDERSTYLE
      int flags = onoroff
	  ? GetDecor(t,BorderStyle.active.style)
	  : GetDecor(t,BorderStyle.inactive.style);
#endif /* BORDERSTYLE */

      ChangeWindowColor(t->sides[i],valuemask);
      if((flush_expose (t->sides[i]))||(expose_win == t->sides[i])||
         (expose_win == None))
      {
        GC sgc,rgc;

        sgc=ShadowGC;
        rgc=ReliefGC;
        if(!(t->flags & MWMButtons)&&(PressedW == t->sides[i]))
        {
          sgc = ReliefGC;
          rgc = ShadowGC;
        }
        /* index    side
         * 0        TOP
         * 1        RIGHT
         * 2        BOTTOM
         * 3        LEFT
         */

#ifdef BORDERSTYLE
        if (flags&HiddenHandles) {
	    if (flags&NoInset)
		RelieveWindowHH(t,t->sides[i],0,0,
				((vertical)?t->boundary_width:x),
				((vertical)?y:t->boundary_width),
				rgc, sgc, vertical
				? (i == 3 ? LEFT_HILITE : RIGHT_HILITE)
				: (i ? BOTTOM_HILITE : TOP_HILITE),
				(0x0001<<i)
		    );
	    else
		RelieveWindowHH(t,t->sides[i],0,0,
				((vertical)?t->boundary_width:x),
				((vertical)?y:t->boundary_width),
				rgc, sgc, vertical
				? (LEFT_HILITE|RIGHT_HILITE)
				: (TOP_HILITE|BOTTOM_HILITE),
				(0x0001<<i)
		    );
        } else
#endif /* BORDERSTYLE */
	    RelieveWindow(t,t->sides[i],0,0,
			  ((i%2)?t->boundary_width:x),
			  ((i%2)?y:t->boundary_width),
			  rgc, sgc, (0x0001<<i));
      }
      ChangeWindowColor(t->corners[i],valuemask);
      if((flush_expose(t->corners[i]))||(expose_win==t->corners[i])||
         (expose_win == None))
      {
        GC rgc,sgc;

        rgc = ReliefGC;
        sgc = ShadowGC;
        if(!(t->flags & MWMButtons)&&(PressedW == t->corners[i]))
        {
          sgc = ReliefGC;
          rgc = ShadowGC;
        }
#ifdef BORDERSTYLE
        if (flags&HiddenHandles) {
	    RelieveWindowHH(t,t->corners[i],0,0,t->corner_width,
			    ((i/2)?t->corner_width+t->bw:t->corner_width),
			    rgc,sgc, corners[i], corners[i]);

	    if (!(flags&NoInset)) {
		if (t->boundary_width > 1)
		    RelieveParts(t,i|HH_HILITE,
				 ((i/2)?rgc:sgc),(vertical?rgc:sgc));
		else
		    RelieveParts(t,i|HH_HILITE,
				 ((i/2)?sgc:sgc),(vertical?sgc:sgc));
	    }
         } else {
#endif /* ! BORDERSTYLE */
	     RelieveWindow(t,t->corners[i],0,0,t->corner_width,
			   ((i/2)?t->corner_width+t->bw:t->corner_width),
			   rgc,sgc, corners[i]);

	     if(t->boundary_width > 1)
		 RelieveParts(t,i,((i/2)?rgc:sgc),(vertical?rgc:sgc));
	     else
		 RelieveParts(t,i,((i/2)?sgc:sgc),(vertical?sgc:sgc));
#ifdef BORDERSTYLE
         }
#endif
      }
    }
  }
  else      /* no decorative border */
  {
    /* for mono - put a black border on
     * for color, make it the color of the decoration background */
    if(t->boundary_width < 2)
    {
      flush_expose (t->frame);
      if(Scr.d_depth <2)
      {
        XSetWindowBorder(dpy,t->frame,TextColor);
        XSetWindowBorder(dpy,t->Parent,TextColor);
        XSetWindowBackgroundPixmap(dpy,t->frame,BackPixmap);
        XClearWindow(dpy,t->frame);
        XSetWindowBackgroundPixmap(dpy,t->Parent,BackPixmap);
        XClearWindow(dpy,t->Parent);
      }
      else
      {
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
	XSetWindowBackgroundPixmap(dpy,t->frame,TexturePixmap);
#endif
        XSetWindowBorder(dpy,t->frame,BorderColor);
        XClearWindow(dpy,t->frame);
        XSetWindowBackground(dpy,t->Parent,BorderColor);
        XSetWindowBorder(dpy,t->Parent,BorderColor);
        XClearWindow(dpy,t->Parent);
        XSetWindowBorder(dpy,t->w,BorderColor);
      }
    }
    else
    {
      GC rgc,sgc;

      XSetWindowBorder(dpy,t->Parent,BorderColor);
      XSetWindowBorder(dpy,t->frame,BorderColor);

      rgc=ReliefGC;
      sgc=ShadowGC;
      if(!(t->flags & MWMButtons)&&(PressedW == t->frame))
      {
        sgc=ReliefGC;
        rgc=ShadowGC;
      }
      ChangeWindowColor(t->frame, valuemask);
      if((flush_expose(t->frame))||(expose_win == t->frame)||
         (expose_win == None))
      {
        if(t->boundary_width > 2)
        {
          RelieveWindow(t,t->frame,t->boundary_width-1 - t->bw,
                        t->boundary_width-1-t->bw,
                        t->frame_width-
                        (t->boundary_width<<1)+2+3*t->bw,
                        t->frame_height-
                        (t->boundary_width<<1)+2+3*t->bw,
                        sgc,rgc,
                        TOP_HILITE|LEFT_HILITE|RIGHT_HILITE|
                        BOTTOM_HILITE);
          RelieveWindow(t,t->frame,0,0,t->frame_width+t->bw,
                        t->frame_height+t->bw,rgc,sgc,
                        TOP_HILITE|LEFT_HILITE|RIGHT_HILITE|
                        BOTTOM_HILITE);
        }
        else
        {
          RelieveWindow(t,t->frame,0,0,t->frame_width+t->bw,
                        t->frame_height+t->bw,rgc,rgc,
                        TOP_HILITE|LEFT_HILITE|RIGHT_HILITE|
                        BOTTOM_HILITE);
        }
      }
      else
      {
        XSetWindowBackground(dpy,t->Parent,BorderColor);
      }
    }
  }
  /* Sync to make the border-color change look fast! */
  XSync(dpy,0);

}

/****************************************************************************
 *
 *  Redraws buttons (veliaa@rpi.edu)
 *
 ****************************************************************************/
void DrawButton(FvwmWindow *t, Window win, int w, int h,
		ButtonFace *bf, GC ReliefGC, GC ShadowGC,
		Boolean inverted, int stateflags)
{
    register int type = bf->style & ButtonFaceTypeMask;
#ifdef PIXMAP_BUTTONS
    Picture *p;
    int border = 0;
    int width, height, x, y;
#endif

    switch (type)
    {
    case SimpleButton:
	break;

    case SolidButton:
	XSetWindowBackground(dpy, win, bf->u.back);
	flush_expose(win);
	XClearWindow(dpy,win);
	break;

#ifdef VECTOR_BUTTONS
    case VectorButton:
	if((t->flags & MWMButtons)
           && (stateflags & MWMDecorMaximize)
	   && (t->flags & MAXIMIZED))
	    DrawLinePattern(win,
			    ShadowGC, ReliefGC,
			    &bf->vector,
			    w, h);
	else
	    DrawLinePattern(win,
			    ReliefGC, ShadowGC,
			    &bf->vector,
			    w, h);
	break;
#endif /* VECTOR_BUTTONS */

#ifdef PIXMAP_BUTTONS
#ifdef MINI_ICONS
    case MiniIconButton:
    case PixmapButton:
	if (type == PixmapButton)
	    p = bf->u.p;
	else {
	    if (!t->mini_icon)
		break;
	    p = t->mini_icon;
	}
#else
    case PixmapButton:
	p = bf->u.p;
#endif /* MINI_ICONS */
	if (bf->style & FlatButton)
	    border = 0;
	else
	    border = t->flags & MWMBorders ? 1 : 2;
	width = w - border * 2; height = h - border * 2;

	x = border;
	if (bf->style&HOffCenter) {
	    if (bf->style&HRight)
		x += (int)(width - p->width);
	} else
	    x += (int)(width - p->width) / 2;

	y = border;
	if (bf->style&VOffCenter) {
	    if (bf->style&VBottom)
		y += (int)(height - p->height);
	} else
	    y += (int)(height - p->height) / 2;

	if (x < border) x = border;
	if (y < border) y = border;
	if (width > p->width) width = p->width;
	if (height > p->height) height = p->height;
	if (width > w - x - border) width = w - x - border;
	if (height > h - y - border) height = h - y - border;

	XSetClipMask(dpy, Scr.TransMaskGC, p->mask);
	XSetClipOrigin(dpy, Scr.TransMaskGC, x, y);
	XCopyArea(dpy, p->picture, win, Scr.TransMaskGC,
		  0, 0, width, height, x, y);
	break;

   case TiledPixmapButton:
	XSetWindowBackgroundPixmap(dpy, win, bf->u.p->picture);
	flush_expose(win);
	XClearWindow(dpy,win);
	break;
#endif /* PIXMAP_BUTTONS */

#ifdef GRADIENT_BUTTONS
    case HGradButton:
    case VGradButton:
    {
	XRectangle bounds;
	bounds.x = bounds.y = 0;
	bounds.width = w;
	bounds.height = h;
	flush_expose(win);

#ifdef PIXMAP_BUTTONS
	XSetClipMask(dpy, Scr.TransMaskGC, None);
#endif
	if (type == HGradButton) {
	    register int i = 0, dw = bounds.width
		/ bf->u.grad.npixels + 1;
	    while (i < bf->u.grad.npixels)
	    {
		unsigned short x = i * bounds.width / bf->u.grad.npixels;
		XSetForeground(dpy, Scr.TransMaskGC, bf->u.grad.pixels[ i++ ]);
		XFillRectangle(dpy, win, Scr.TransMaskGC,
			       bounds.x + x, bounds.y,
			       dw, bounds.height);
	    }
	} else {
	    register int i = 0, dh = bounds.height
		/ bf->u.grad.npixels + 1;
	    while (i < bf->u.grad.npixels)
	    {
		unsigned short y = i * bounds.height / bf->u.grad.npixels;
		XSetForeground(dpy, Scr.TransMaskGC, bf->u.grad.pixels[ i++ ]);
		XFillRectangle(dpy, win, Scr.TransMaskGC,
			       bounds.x, bounds.y + y,
			       bounds.width, dh);
	    }
	}
    }
    break;
#endif /* GRADIENT_BUTTONS */

    default:
	fvwm_msg(ERR,"DrawButton","unknown button type");
	break;
    }
}

/****************************************************************************
 *
 *  Redraws just the title bar
 *
 ****************************************************************************/
void SetTitleBar (FvwmWindow *t,Bool onoroff, Bool NewTitle)
{
  int hor_off, w, i;
  enum ButtonState title_state;
  ButtonFaceStyle tb_style;
  int tb_flags;
  GC ReliefGC, ShadowGC, tGC;
  Pixel Forecolor, BackColor;

  if(!t)
    return;
  if(!(t->flags & TITLE))
    return;

  if (onoroff)
  {
    Forecolor = GetDecor(t,HiColors.fore);
    BackColor = GetDecor(t,HiColors.back);
    ReliefGC = GetDecor(t,HiReliefGC);
    ShadowGC = GetDecor(t,HiShadowGC);
  }
  else
  {
    Forecolor =t->TextPixel;
    BackColor = t->BackPixel;
    Globalgcv.foreground = t->ReliefPixel;
    Globalgcm = GCForeground;
    XChangeGC(dpy,Scr.ScratchGC1,Globalgcm,&Globalgcv);
    ReliefGC = Scr.ScratchGC1;

    Globalgcv.foreground = t->ShadowPixel;
    XChangeGC(dpy,Scr.ScratchGC2,Globalgcm,&Globalgcv);
    ShadowGC = Scr.ScratchGC2;
  }
  if(PressedW==t->title_w)
  {
    tGC = ShadowGC;
    ShadowGC = ReliefGC;
    ReliefGC = tGC;
  }
  flush_expose(t->title_w);

  if(t->name != (char *)NULL)
  {
    w=XTextWidth(GetDecor(t,WindowFont.font),t->name,strlen(t->name));
    if(w > t->title_width-12)
      w = t->title_width-4;
    if(w < 0)
      w = 0;
  }
  else
    w = 0;

  title_state = GetButtonState(t->title_w);
  tb_style = GetDecor(t,titlebar.state[title_state].style);
  tb_flags = GetDecor(t,titlebar.flags);
  if (tb_flags & HOffCenter) {
      if (tb_flags & HRight)
	  hor_off = t->title_width - w - 10;
      else
	  hor_off = 10;
  } else
      hor_off = (t->title_width - w) / 2;

  NewFontAndColor(GetDecor(t,WindowFont.font->fid),Forecolor, BackColor);

  /* the next bit tries to minimize redraw based upon compilation options (veliaa@rpi.edu) */
#ifdef EXTENDED_TITLESTYLE
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
  /* we need to check for UseBorderStyle for the titlebar */
  {
      ButtonFace *bf = onoroff
	  ? &GetDecor(t,BorderStyle.active)
	  : &GetDecor(t,BorderStyle.inactive);

      if ((tb_style & UseBorderStyle)
	  && ((bf->style & ButtonFaceTypeMask) == TiledPixmapButton))
	  XSetWindowBackgroundPixmap(dpy,t->title_w,bf->u.p->picture);
  }
#endif /* PIXMAP_BUTTONS && BORDERSTYLE */
  XClearWindow(dpy,t->title_w);
#else /* ! EXTENDED_TITLESTYLE */
  /* if no extended titlestyle, only clear when necessary */
  if (NewTitle)
      XClearWindow(dpy,t->title_w);
#endif /* EXTENDED_TITLESTYLE */

  /* for mono, we clear an area in the title bar where the window
   * title goes, so that its more legible. For color, no need */
  if(Scr.d_depth<2)
  {
    RelieveWindow(t,t->title_w,0,0,hor_off-2,t->title_height,
                  ReliefGC, ShadowGC, BOTTOM_HILITE);
    RelieveWindow(t,t->title_w,hor_off+w+2,0,
                  t->title_width - w - hor_off-2,t->title_height,
                  ReliefGC, ShadowGC, BOTTOM_HILITE);
    XFillRectangle(dpy,t->title_w,
                   (PressedW==t->title_w?ShadowGC:ReliefGC),
                   hor_off - 2, 0, w+4,t->title_height);

    XDrawLine(dpy,t->title_w,ShadowGC,hor_off+w+1,0,hor_off+w+1,
              t->title_height);
    if(t->name != (char *)NULL)
      XDrawString (dpy, t->title_w,Scr.ScratchGC3,hor_off,
                   GetDecor(t,WindowFont.y)+1,
                   t->name, strlen(t->name));
  }
  else
  {
#ifdef EXTENDED_TITLESTYLE
      ButtonFace *bf = &GetDecor(t,titlebar.state[title_state]);
      /* draw compound titlebar (veliaa@rpi.edu) */
      if (PressedW == t->title_w) {
#ifdef MULTISTYLE
	  for (; bf; bf = bf->next)
#endif
	      DrawButton(t, t->title_w, t->title_width, t->title_height,
			 bf, ShadowGC, ReliefGC, True, 0);
      } else {
#ifdef MULTISTYLE
	  for (; bf; bf = bf->next)
#endif
	      DrawButton(t, t->title_w, t->title_width, t->title_height,
			 bf, ReliefGC, ShadowGC, False, 0);
      }
#endif /* EXTENDED_TITLESTYLE */

      if (!(tb_style & FlatButton)) {
	  if (tb_style & SunkButton)
	      RelieveWindow(t,t->title_w,0,0,t->title_width,t->title_height,
			    ShadowGC, ReliefGC, BOTTOM_HILITE);
	  else
	      RelieveWindow(t,t->title_w,0,0,t->title_width,t->title_height,
			    ReliefGC, ShadowGC, BOTTOM_HILITE);
      }

      if(t->name != (char *)NULL)
        XDrawString (dpy, t->title_w,Scr.ScratchGC3,hor_off,
                     GetDecor(t,WindowFont.y)+1,
                     t->name, strlen(t->name));
  }
  /* now, draw lines in title bar if it's a sticky window */
  if(t->flags & STICKY || Scr.StipledTitles)
  {
    for(i=0 ;i< t->title_height/2-3;i+=4)
    {
      XDrawLine(dpy,t->title_w,ShadowGC,4,t->title_height/2 - i-1,
                hor_off-6,t->title_height/2-i-1);
      XDrawLine(dpy,t->title_w,ShadowGC,6+hor_off+w,t->title_height/2 -i-1,
                t->title_width-5,t->title_height/2- i-1);
      XDrawLine(dpy,t->title_w,ReliefGC,4,t->title_height/2 - i,
                hor_off-6,t->title_height/2 - i);
      XDrawLine(dpy,t->title_w,ReliefGC,6+hor_off+w,t->title_height/2-i,
                t->title_width-5,t->title_height/2 - i);

      XDrawLine(dpy,t->title_w,ShadowGC,4,t->title_height/2 + i-1,
                hor_off-6,t->title_height/2+i-1);
      XDrawLine(dpy,t->title_w,ShadowGC,6+hor_off+w,t->title_height/2+i-1,
                t->title_width-5,t->title_height/2 + i-1);
      XDrawLine(dpy,t->title_w,ReliefGC,4,t->title_height/2 + i,
                hor_off-6,t->title_height/2 + i);
      XDrawLine(dpy,t->title_w,ReliefGC,6+hor_off+w,t->title_height/2+i,
                t->title_width-5,t->title_height/2 + i);
    }
  }


  XFlush(dpy);
}




/****************************************************************************
 *
 *  Draws the relief pattern around a window
 *
 ****************************************************************************/
void RelieveWindow(FvwmWindow *t,Window win, int x,int y,int w,int h,
		   GC ReliefGC, GC ShadowGC, int hilite)
{
  XSegment seg[4];
  int i;
  int edge;

  edge = 0;
  if((win == t->sides[0])||(win == t->sides[1])||
     (win == t->sides[2])||(win == t->sides[3]))
    edge = -1;
  if(win == t->corners[0])
    edge = 1;
  if(win == t->corners[1])
    edge = 2;
  if(win == t->corners[2])
    edge = 3;
  if(win == t->corners[3])
    edge = 4;

  i=0;
  seg[i].x1 = x;        seg[i].y1   = y;
  seg[i].x2 = w+x-1;    seg[i++].y2 = y;

  seg[i].x1 = x;        seg[i].y1   = y;
  seg[i].x2 = x;        seg[i++].y2 = h+y-1;

  if(((t->boundary_width > 2)||(edge == 0))&&
     ((t->boundary_width > 3)||(edge < 1))&&
     (!(t->flags & MWMBorders)||
      (((edge==0)||(t->boundary_width > 3))&&(hilite & TOP_HILITE))))
  {
    seg[i].x1 = x+1;      seg[i].y1   = y+1;
    seg[i].x2 = x+w-2;    seg[i++].y2 = y+1;
  }
  if(((t->boundary_width > 2)||(edge == 0))&&
     ((t->boundary_width > 3)||(edge < 1))&&
     (!(t->flags & MWMBorders)||
      (((edge==0)||(t->boundary_width > 3))&&(hilite & LEFT_HILITE))))
  {
    seg[i].x1 = x+1;      seg[i].y1   = y+1;
    seg[i].x2 = x+1;      seg[i++].y2 = y+h-2;
  }
  XDrawSegments(dpy, win, ReliefGC, seg, i);

  i=0;
  seg[i].x1 = x;        seg[i].y1   = y+h-1;
  seg[i].x2 = w+x-1;    seg[i++].y2 = y+h-1;

  if(((t->boundary_width > 2)||(edge == 0))&&
     (!(t->flags & MWMBorders)||
      (((edge==0)||(t->boundary_width > 3))&&(hilite & BOTTOM_HILITE))))
  {
    seg[i].x1 = x+1;      seg[i].y1   = y+h-2;
    seg[i].x2 = x+w-2;    seg[i++].y2 = y+h-2;
  }

  seg[i].x1 = x+w-1;    seg[i].y1   = y;
  seg[i].x2 = x+w-1;    seg[i++].y2 = y+h-1;

  if(((t->boundary_width > 2)||(edge == 0))&&
     (!(t->flags & MWMBorders)||
      (((edge==0)||(t->boundary_width > 3))&&(hilite & RIGHT_HILITE))))
  {
    seg[i].x1 = x+w-2;    seg[i].y1   = y+1;
    seg[i].x2 = x+w-2;    seg[i++].y2 = y+h-2;
  }
  XDrawSegments(dpy, win, ShadowGC, seg, i);
}

#ifdef BORDERSTYLE
/****************************************************************************
 *
 *  Draws the relief pattern around a window for HiddenHandle borders
 *
 *  (veliaa@rpi.edu)
 *
 ****************************************************************************/
void RelieveWindowHH(FvwmWindow *t,Window win, int x,int y,int w,int h,
		     GC ReliefGC, GC ShadowGC, int draw, int hilite)
{
    XSegment seg[4];
    int i = 0;
    int edge = 0, a = 0, b = 0;

    if(win == t->sides[0]) {
	edge = 5;
	b = 1;
    }
    else if (win == t->sides[1]) {
	a = 1;
	edge = 6;
    }
    else if (win == t->sides[2]) {
	edge = 7;
	b = 1;
    }
    else if (win == t->sides[3]) {
	edge = 8;
	a = 1;
    } else if (win == t->corners[0])
	edge = 1;
    else if (win == t->corners[1])
	edge = 2;
    else if (win == t->corners[2])
	edge = 3;
    else if (win == t->corners[3])
	edge = 4;

    if (draw & TOP_HILITE) {
	seg[i].x1 = x;        seg[i].y1   = y;
	seg[i].x2 = w+x-1;    seg[i++].y2 = y;

	if(((t->boundary_width > 2)||(edge == 0))&&
	   ((t->boundary_width > 3)||(edge < 1))&&
	   (!(t->flags & MWMBorders)||
	    (((edge==0)||(t->boundary_width > 3))&&(hilite & TOP_HILITE))))
	{
	    seg[i].x1 = x+((edge == 2)|| b ? 0 : 1);  seg[i].y1 = y+1;
	    seg[i].x2 = x+w-1-((edge == 1)|| b ? 0 : 1);    seg[i++].y2 = y+1;
	}
    }

    if (draw & LEFT_HILITE) {
	seg[i].x1 = x;        seg[i].y1   = y;
	seg[i].x2 = x;        seg[i++].y2 = h+y-1;

	if(((t->boundary_width > 2)||(edge == 0))&&
	   ((t->boundary_width > 3)||(edge < 1))&&
	   (!(t->flags & MWMBorders)||
	    (((edge==0)||(t->boundary_width > 3))&&(hilite & LEFT_HILITE))))
	{
	    seg[i].x1 = x+1;      seg[i].y1   = y+((edge == 3)|| a ? 0 : 1);
	    seg[i].x2 = x+1;      seg[i++].y2 = y+h-1-((edge == 1)|| a ? 0 : 1);
	}
    }
    XDrawSegments(dpy, win, ReliefGC, seg, i);

    i=0;

    if (draw & BOTTOM_HILITE) {
	seg[i].x1 = x;        seg[i].y1   = y+h-1;
	seg[i].x2 = w+x-1;    seg[i++].y2 = y+h-1;

	if(((t->boundary_width > 2)||(edge == 0))&&
	   (!(t->flags & MWMBorders)||
	    (((edge==0)||(t->boundary_width > 3))&&(hilite & BOTTOM_HILITE))))
	{
	    seg[i].x1 = x+(b ||(edge == 4) ? 0 : 1);      seg[i].y1   = y+h-2;
	    seg[i].x2 = x+w-((edge == 3) ? 0 : 1);    seg[i++].y2 = y+h-2;
	}
    }

    if (draw & RIGHT_HILITE) {
	seg[i].x1 = x+w-1;    seg[i].y1   = y;
	seg[i].x2 = x+w-1;    seg[i++].y2 = y+h-1;

	if(((t->boundary_width > 2)||(edge == 0))&&
	   (!(t->flags & MWMBorders)||
	    (((edge==0)||(t->boundary_width > 3))&&(hilite & RIGHT_HILITE))))
	{
	    seg[i].x1 = x+w-2;    seg[i].y1   = y+(a ||(edge == 4) ? 0 : 1);
	    seg[i].x2 = x+w-2;    seg[i++].y2 = y+h-1-((edge == 2)|| a ? 0 : 1);
	}
    }
    XDrawSegments(dpy, win, ShadowGC, seg, i);
}
#endif /* BORDERSTYLE */

void RelieveParts(FvwmWindow *t,int i,GC hor, GC vert)
{
  XSegment seg[2];
  int n = 0, hh = i & HH_HILITE;
  i &= FULL_HILITE;

  if((t->flags & MWMBorders)||(t->boundary_width < 3))
  {
    switch(i)
    {
      case 0:
        seg[0].x1 = t->boundary_width-1;
        seg[0].x2 = t->corner_width;
        seg[0].y1 = t->boundary_width-1;
        seg[0].y2 = t->boundary_width-1;
        n=1;
        break;
      case 1:
        seg[0].x1 = 0;
        seg[0].x2 = t->corner_width - t->boundary_width /* -1*/ ;
        seg[0].y1 = t->boundary_width-1;
        seg[0].y2 = t->boundary_width-1;
        n=1;
        break;
      case 2:
        seg[0].x1 = t->boundary_width-1;
        seg[0].x2 = t->corner_width - (hh ? 1 : 2);
        seg[0].y1 = t->corner_width - t->boundary_width+t->bw;
        seg[0].y2 = t->corner_width - t->boundary_width+t->bw;
        n=1;
        break;
      case 3:
        seg[0].x1 = 0;
        seg[0].x2 = t->corner_width - t->boundary_width;
        seg[0].y1 = t->corner_width - t->boundary_width+t->bw;
        seg[0].y2 = t->corner_width - t->boundary_width+t->bw;
        n=1;
        break;
    }
    XDrawSegments(dpy, t->corners[i], hor, seg, n);
    switch(i)
    {
      case 0:
        seg[0].y1 = t->boundary_width-1;
        seg[0].y2 = t->corner_width;
        seg[0].x1 = t->boundary_width-1;
        seg[0].x2 = t->boundary_width-1;
        n=1;
        break;
      case 1:
        seg[0].y1 = t->boundary_width -1;
        seg[0].y2 = t->corner_width - (hh ? 0 : 2);
        seg[0].x1 = t->corner_width - t->boundary_width;
        seg[0].x2 = t->corner_width - t->boundary_width;
        n=1;
        break;
      case 2:
        seg[0].y1 = 0;
        seg[0].y2 = t->corner_width - t->boundary_width;
        seg[0].x1 = t->boundary_width-1;
        seg[0].x2 = t->boundary_width-1;
        n=1;
        break;
      case 3:
        seg[0].y1 = 0;
        seg[0].y2 = t->corner_width - t->boundary_width + t->bw;
        seg[0].x1 = t->corner_width - t->boundary_width;
        seg[0].x2 = t->corner_width - t->boundary_width;
        n=1;
        break;
    }
    XDrawSegments(dpy, t->corners[i], vert, seg, 1);
  }
  else
  {
    switch(i)
    {
      case 0:
        seg[0].x1 = t->boundary_width-2;
        seg[0].x2 = t->corner_width;
        seg[0].y1 = t->boundary_width-2;
        seg[0].y2 = t->boundary_width-2;

        seg[1].x1 = t->boundary_width-2;
        seg[1].x2 = t->corner_width;
        seg[1].y1 = t->boundary_width-1;
        seg[1].y2 = t->boundary_width-1;
        n=2;
        break;
      case 1:
        seg[0].x1 = (hh ? 0 : 1);
        seg[0].x2 = t->corner_width - t->boundary_width;
        seg[0].y1 = t->boundary_width-2;
        seg[0].y2 = t->boundary_width-2;

        seg[1].x1 = 0;
        seg[1].x2 = t->corner_width - t->boundary_width-1;
        seg[1].y1 = t->boundary_width-1;
        seg[1].y2 = t->boundary_width-1;
        n=2;
        break;
      case 2:
        seg[0].x1 = t->boundary_width-1;
        seg[0].x2 = t->corner_width - (hh ? 1 : 2);
        seg[0].y1 = t->corner_width - t->boundary_width+1;
        seg[0].y2 = t->corner_width - t->boundary_width+1;
        n=1;
        if(t->boundary_width > 3)
        {
          seg[1].x1 = t->boundary_width-2;
          seg[1].x2 = t->corner_width - (hh ? 1 : 3);
          seg[1].y1 = t->corner_width - t->boundary_width + 2;
          seg[1].y2 = t->corner_width - t->boundary_width + 2;
          n=2;
        }
        break;
      case 3:
        seg[0].x1 = 0;
        seg[0].x2 = t->corner_width - t->boundary_width;
        seg[0].y1 = t->corner_width - t->boundary_width+1;
        seg[0].y2 = t->corner_width - t->boundary_width+1;
        n=1;
        if(t->boundary_width > 3)
        {
          seg[0].x2 = t->corner_width - t->boundary_width + 1;

          seg[1].x1 = 0;
          seg[1].x2 = t->corner_width - t->boundary_width + 1;
          seg[1].y1 = t->corner_width - t->boundary_width + 2;
          seg[1].y2 = t->corner_width - t->boundary_width + 2;
          n=2;
        }
        break;
    }
    XDrawSegments(dpy, t->corners[i], hor, seg, n);
    switch(i)
    {
      case 0:
        seg[0].y1 = t->boundary_width-2;
        seg[0].y2 = t->corner_width;
        seg[0].x1 = t->boundary_width-2;
        seg[0].x2 = t->boundary_width-2;

        seg[1].y1 = t->boundary_width-2;
        seg[1].y2 = t->corner_width;
        seg[1].x1 = t->boundary_width-1;
        seg[1].x2 = t->boundary_width-1;
        n=2;
        break;
      case 1:
        seg[0].y1 = t->boundary_width-1;
        seg[0].y2 = t->corner_width - (hh ? 1 : 2);
        seg[0].x1 = t->corner_width - t->boundary_width;
        seg[0].x2 = t->corner_width - t->boundary_width;
        n=1;
        if(t->boundary_width > 3)
        {
          seg[1].y1 = t->boundary_width-2;
          seg[1].y2 = t->corner_width - (hh ? 1 : 3);
          seg[1].x1 = t->corner_width - t->boundary_width+1;
          seg[1].x2 = t->corner_width - t->boundary_width+1;
          n=2;
        }
        break;
      case 2:
        seg[0].y1 = (hh ? 0 : 1);
        seg[0].y2 = t->corner_width - t->boundary_width+1;
        seg[0].x1 = t->boundary_width-2;
        seg[0].x2 = t->boundary_width-2;
        n=1;

        if(t->boundary_width > 3)
        {
          seg[1].y1 = 0;
          seg[1].y2 = t->corner_width - t->boundary_width;
          seg[1].x1 = t->boundary_width-1;
          seg[1].x2 = t->boundary_width-1;
        }
        break;
      case 3:
        seg[0].y1 = 0;
        seg[0].y2 = t->corner_width - t->boundary_width + 1;
        seg[0].x1 = t->corner_width - t->boundary_width;
        seg[0].x2 = t->corner_width - t->boundary_width;
        n=1;

        if(t->boundary_width > 3)
        {
          seg[0].y2 = t->corner_width - t->boundary_width + 2;
          seg[1].y1 = 0;
          seg[1].y2 = t->corner_width - t->boundary_width + 2;
          seg[1].x1 = t->corner_width - t->boundary_width + 1;
          seg[1].x2 = t->corner_width - t->boundary_width + 1;
          n=2;
        }
        break;
    }
    XDrawSegments(dpy, t->corners[i], vert, seg, n);
  }
}

#ifdef VECTOR_BUTTONS
/****************************************************************************
 *
 *  Draws a little pattern within a window (more complex)
 *
 ****************************************************************************/
void DrawLinePattern(Window win,
                     GC ReliefGC,
                     GC ShadowGC,
		     struct vector_coords *coords,
                     int w, int h)
{
  int i = 1;
  for (; i < coords->num; ++i)
  {
      XDrawLine(dpy,win,
		coords->line_style[i] ? ReliefGC : ShadowGC,
		w * coords->x[i-1]/100,
		h * coords->y[i-1]/100,
		w * coords->x[i]/100,
		h * coords->y[i]/100);
  }
}
#endif /* VECTOR_BUTTONS */


/***********************************************************************
 *
 *  Procedure:
 *      Setupframe - set window sizes, this was called from either
 *              AddWindow, EndResize, or HandleConfigureNotify.
 *
 *  Inputs:
 *      tmp_win - the FvwmWindow pointer
 *      x       - the x coordinate of the upper-left outer corner of the frame
 *      y       - the y coordinate of the upper-left outer corner of the frame
 *      w       - the width of the frame window w/o border
 *      h       - the height of the frame window w/o border
 *
 *  Special Considerations:
 *      This routine will check to make sure the window is not completely
 *      off the display, if it is, it'll bring some of it back on.
 *
 *      The tmp_win->frame_XXX variables should NOT be updated with the
 *      values of x,y,w,h prior to calling this routine, since the new
 *      values are compared against the old to see whether a synthetic
 *      ConfigureNotify event should be sent.  (It should be sent if the
 *      window was moved but not resized.)
 *
 ************************************************************************/

void SetupFrame(FvwmWindow *tmp_win,int x,int y,int w,int h,Bool sendEvent)
{
  XEvent client_event;
  XWindowChanges frame_wc, xwc;
  unsigned long frame_mask, xwcm;
  int cx,cy,i;
  Bool Resized = False, Moved = False;
  int xwidth,ywidth,left,right;
#ifdef WINDOWSHADE
  int shaded = tmp_win->buttons & WSHADE;
#endif

#ifdef FVWM_DEBUG_MSGS
  fvwm_msg(DBG,"SetupFrame",
           "Routine Entered (x == %d, y == %d, w == %d, h == %d)",
           x, y, w, h);
#endif

  /* if windows is not being maximized, save size in case of maximization */
  if (!(tmp_win->flags & MAXIMIZED)
#ifdef WINDOWSHADE
      && !shaded
#endif
      )
  {
    tmp_win->orig_x = x;
    tmp_win->orig_y = y;
    tmp_win->orig_wd = w;
    tmp_win->orig_ht = h;
  }

  if((w != tmp_win->frame_width) || (h != tmp_win->frame_height))
    Resized = True;
  if ((x != tmp_win->frame_x || y != tmp_win->frame_y))
    Moved = True;

  /*
   * According to the July 27, 1988 ICCCM draft, we should send a
   * "synthetic" ConfigureNotify event to the client if the window
   * was moved but not resized.
   */
  if (Moved && !Resized)
    sendEvent = True;

  if (Resized)
  {
    left = tmp_win->nr_left_buttons;
    right = tmp_win->nr_right_buttons;

    if (tmp_win->flags & TITLE)
      tmp_win->title_height = GetDecor(tmp_win,TitleHeight) + tmp_win->bw;

    tmp_win->title_width= w-
      (left+right)*tmp_win->title_height
      -2*tmp_win->boundary_width+tmp_win->bw;


    if(tmp_win->title_width < 1)
      tmp_win->title_width = 1;

    if (tmp_win->flags & TITLE)
    {
      xwcm = CWWidth | CWX | CWY | CWHeight;
      tmp_win->title_x = tmp_win->boundary_width+
        (left)*tmp_win->title_height;
      if(tmp_win->title_x >=  w - tmp_win->boundary_width)
        tmp_win->title_x = -10;
      tmp_win->title_y = tmp_win->boundary_width;

      xwc.width = tmp_win->title_width;

      xwc.height = tmp_win->title_height;
      xwc.x = tmp_win->title_x;
      xwc.y = tmp_win->title_y;
      XConfigureWindow(dpy, tmp_win->title_w, xwcm, &xwc);


      xwcm = CWX | CWY | CWHeight | CWWidth;
      xwc.height = tmp_win->title_height;
      xwc.width = tmp_win->title_height;

      xwc.y = tmp_win->boundary_width;
      xwc.x = tmp_win->boundary_width;
      for(i=0;i<Scr.nr_left_buttons;i++)
      {
        if(tmp_win->left_w[i] != None)
        {
          if(xwc.x + tmp_win->title_height < w-tmp_win->boundary_width)
            XConfigureWindow(dpy, tmp_win->left_w[i], xwcm, &xwc);
          else
          {
            xwc.x = -tmp_win->title_height;
            XConfigureWindow(dpy, tmp_win->left_w[i], xwcm, &xwc);
          }
          xwc.x += tmp_win->title_height;
        }
      }

      xwc.x=w-tmp_win->boundary_width+tmp_win->bw;
      for(i=0;i<Scr.nr_right_buttons;i++)
      {
        if(tmp_win->right_w[i] != None)
        {
          xwc.x -=tmp_win->title_height;
          if(xwc.x > tmp_win->boundary_width)
            XConfigureWindow(dpy, tmp_win->right_w[i], xwcm, &xwc);
          else
          {
            xwc.x = -tmp_win->title_height;
            XConfigureWindow(dpy, tmp_win->right_w[i], xwcm, &xwc);
          }
        }
      }
    }

    if(tmp_win->flags & BORDER)
    {
      tmp_win->corner_width = GetDecor(tmp_win,TitleHeight) + tmp_win->bw +
        tmp_win->boundary_width ;

      if(w < 2*tmp_win->corner_width)
        tmp_win->corner_width = w/3;
      if((h < 2*tmp_win->corner_width)
#ifdef WINDOWSHADE
        &&!shaded
#endif
         )
        tmp_win->corner_width = h/3;
      xwidth = w - 2*tmp_win->corner_width+tmp_win->bw;
      ywidth = h - 2*tmp_win->corner_width;
      xwcm = CWWidth | CWHeight | CWX | CWY;
      if(xwidth<2)
        xwidth = 2;
      if(ywidth<2)
        ywidth = 2;

      for(i=0;i<4;i++)
      {
        if(i==0)
        {
          xwc.x = tmp_win->corner_width;
          xwc.y = 0;
          xwc.height = tmp_win->boundary_width;
          xwc.width = xwidth;
        }
        else if (i==1)
        {
          xwc.x = w - tmp_win->boundary_width+tmp_win->bw;
          xwc.y = tmp_win->corner_width;
          xwc.width = tmp_win->boundary_width;
          xwc.height = ywidth;

        }
        else if(i==2)
        {
          xwc.x = tmp_win->corner_width;
          xwc.y = h - tmp_win->boundary_width+tmp_win->bw;
          xwc.height = tmp_win->boundary_width+tmp_win->bw;
          xwc.width = xwidth;
        }
        else
        {
          xwc.x = 0;
          xwc.y = tmp_win->corner_width;
          xwc.width = tmp_win->boundary_width;
          xwc.height = ywidth;
        }
#ifdef WINDOWSHADE
        if (!shaded||(i!=2))
#endif
          XConfigureWindow(dpy, tmp_win->sides[i], xwcm, &xwc);
      }

      xwcm = CWX|CWY|CWWidth|CWHeight;
      xwc.width = tmp_win->corner_width;
      xwc.height = tmp_win->corner_width;
      for(i=0;i<4;i++)
      {
        if(i%2)
          xwc.x = w - tmp_win->corner_width+tmp_win->bw;
        else
          xwc.x = 0;

        if(i/2)
          xwc.y = h - tmp_win->corner_width;
        else
          xwc.y = 0;

#ifdef WINDOWSHADE
	if (!shaded||(i==0)||(i==1))
#endif
	    XConfigureWindow(dpy, tmp_win->corners[i], xwcm, &xwc);
      }

    }
  }
  tmp_win->attr.width = w - 2*tmp_win->boundary_width;
  tmp_win->attr.height = h - tmp_win->title_height
    - 2*tmp_win->boundary_width;
  /* may need to omit the -1 for shaped windows, next two lines*/
  cx = tmp_win->boundary_width-tmp_win->bw;
  cy = tmp_win->title_height + tmp_win->boundary_width-tmp_win->bw;

#ifdef WINDOWSHADE
  if (!shaded) {
#endif
      XResizeWindow(dpy, tmp_win->w, tmp_win->attr.width,
		    tmp_win->attr.height);
      XMoveResizeWindow(dpy, tmp_win->Parent, cx,cy,
                        tmp_win->attr.width, tmp_win->attr.height);
#ifdef WINDOWSHADE
  }
#endif

  /*
   * fix up frame and assign size/location values in tmp_win
   */
  frame_wc.x = tmp_win->frame_x = x;
  frame_wc.y = tmp_win->frame_y = y;
  frame_wc.width = tmp_win->frame_width = w;
  frame_wc.height = tmp_win->frame_height = h;
  frame_mask = (CWX | CWY | CWWidth | CWHeight);
  XConfigureWindow (dpy, tmp_win->frame, frame_mask, &frame_wc);
#ifdef FVWM_DEBUG_MSGS
  fvwm_msg(DBG,"SetupFrame",
           "New frame dimensions (x == %d, y == %d, w == %d, h == %d)",
           frame_wc.x, frame_wc.y, frame_wc.width, frame_wc.height);
#endif

#ifdef SHAPE
  if (ShapesSupported)
  {
    if ((Resized)&&(tmp_win->wShaped))
    {
      SetShape(tmp_win,w);
    }
  }
#endif /* SHAPE */
  XSync(dpy,0);
  if (sendEvent
#ifdef WINDOWSHADE
      && !shaded
#endif
      )
  {
    client_event.type = ConfigureNotify;
    client_event.xconfigure.display = dpy;
    client_event.xconfigure.event = tmp_win->w;
    client_event.xconfigure.window = tmp_win->w;

    client_event.xconfigure.x = x + tmp_win->boundary_width;
    client_event.xconfigure.y = y + tmp_win->title_height+
      tmp_win->boundary_width;
    client_event.xconfigure.width = w-2*tmp_win->boundary_width;
    client_event.xconfigure.height =h-2*tmp_win->boundary_width -
      tmp_win->title_height;

    client_event.xconfigure.border_width =tmp_win->bw;
    /* Real ConfigureNotify events say we're above title window, so ... */
    /* what if we don't have a title ????? */
    client_event.xconfigure.above = tmp_win->frame;
    client_event.xconfigure.override_redirect = False;
    XSendEvent(dpy, tmp_win->w, False, StructureNotifyMask, &client_event);
#ifdef FVWM_DEBUG_MSGS
    fvwm_msg(DBG,"SetupFrame","Sent ConfigureNotify (w == %d, h == %d)",
             client_event.xconfigure.width,client_event.xconfigure.height);
#endif
  }
  XSync(dpy,0);

  BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);
}


/****************************************************************************
 *
 * Sets up the shaped window borders
 *
 ****************************************************************************/
void SetShape(FvwmWindow *tmp_win, int w)
{
#ifdef SHAPE
  if (ShapesSupported)
  {
    XRectangle rect;

    XShapeCombineShape (dpy, tmp_win->frame, ShapeBounding,
                        tmp_win->boundary_width,
                        tmp_win->title_height+tmp_win->boundary_width,
                        tmp_win->w,
                        ShapeBounding, ShapeSet);
    if (tmp_win->title_w)
    {
      /* windows w/ titles */
      rect.x = tmp_win->boundary_width;
      rect.y = tmp_win->title_y;
      rect.width = w - 2*tmp_win->boundary_width+tmp_win->bw;
      rect.height = tmp_win->title_height;


      XShapeCombineRectangles(dpy,tmp_win->frame,ShapeBounding,
                              0,0,&rect,1,ShapeUnion,Unsorted);
    }
  }
#endif
}
