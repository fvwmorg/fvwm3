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
  int i, x, y;
  GC ReliefGC,ShadowGC;
  Pixel BackColor;
  Pixmap BackPixmap,TextColor;
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
  Pixmap TexturePixmap = None;
  XSetWindowAttributes notex_attributes;
  unsigned long notex_valuemask;
#endif
  Bool NewColor = False;
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  Window w;
#ifdef BORDERSTYLE
  int borderflags;
#endif /* BORDERSTYLE */
  int rwidth; /* relief width */
#ifdef WINDOWSHADE
  Bool shaded;
#endif

fprintf(stderr,"border %d %d %d %d\n", t, onoroff, force, expose_win);
  if(!t)
    return;

/* get the border style bits */
#ifdef BORDERSTYLE
      borderflags = onoroff
	  ? GetDecor(t,BorderStyle.active.style)
	  : GetDecor(t,BorderStyle.inactive.style);
#endif /* BORDERSTYLE */

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
  }

  /* MWMBorder style means thin 3d effects */
  rwidth = ((t->flags & MWMBorders) ? 1 : 2);

#ifdef WINDOWSHADE
  shaded =  t->buttons & WSHADE;
#endif

  if(t->flags & ICONIFIED)
  {
    DrawIconWindow(t);
    return;
  }

#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
  if (TexturePixmap)
  {
      attributes.background_pixmap = TexturePixmap;
      valuemask = CWBackPixmap;
      if (Scr.d_depth < 2) {
	  notex_attributes.background_pixmap = BackPixmap;
	  notex_valuemask = CWBackPixmap;
      } else {
	  notex_attributes.background_pixel = BackColor;
	  notex_valuemask = CWBackPixel;
      }
  }
  else
#endif
  if (Scr.d_depth < 2)
  {
      attributes.background_pixmap = BackPixmap;
      valuemask = CWBackPixmap;
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
      notex_attributes.background_pixmap = BackPixmap;
      notex_valuemask = CWBackPixmap;
#endif
  }
  else
  {
      attributes.background_pixel = BackColor;
      valuemask = CWBackPixel;
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
      notex_attributes.background_pixel = BackColor;
      notex_valuemask = CWBackPixel;
#endif
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
			RelieveRectangle(dpy,t->left_w[i],0,0,
				         t->title_height - 1, t->title_height - 1,
				         (inverted ? ReliefGC : ShadowGC),
				         (inverted ? ShadowGC : ReliefGC), rwidth);
		    else
			RelieveRectangle(dpy,t->left_w[i],0,0,
				         t->title_height - 1, t->title_height - 1,
				         (inverted ? ShadowGC : ReliefGC),
				         (inverted ? ReliefGC : ShadowGC), rwidth);
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
			RelieveRectangle(dpy,t->right_w[i],0,0,
				        t->title_height - 1, t->title_height - 1,
				        (inverted ? ReliefGC : ShadowGC),
				        (inverted ? ShadowGC : ReliefGC), rwidth);
		    else
			RelieveRectangle(dpy,t->right_w[i],0,0,
                                         t->title_height - 1, t->title_height - 1,
				         (inverted ? ShadowGC : ReliefGC),
				         (inverted ? ReliefGC : ShadowGC), rwidth);
		}
	    }
	}
    }
    SetTitleBar(t,onoroff, False);

  }

  /* draw the border */
  /* resize handles are InputOnly so draw in the frame and it will show through */
  {
    /* for mono - put a black border on
     * for color, make it the color of the decoration background */
    if(t->boundary_width < 2)
    {
      flush_expose (t->frame);
      if(Scr.d_depth <2)
      {
        XSetWindowBackgroundPixmap(dpy,t->frame,BackPixmap);
        XClearWindow(dpy,t->frame);
      }
      else
      {
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
	XSetWindowBackgroundPixmap(dpy,t->frame,TexturePixmap);
#endif
        XClearWindow(dpy,t->frame);
      }
    }
    else
    {
      GC rgc,sgc;

      /* change this after GSFR, won't depend on MWMButtons */
      if(!(t->flags & MWMButtons)&&(PressedW == t->frame)) {
        sgc=ReliefGC;
        rgc=ShadowGC;
      } else {
        rgc=ReliefGC;
        sgc=ShadowGC;
      }
      ChangeWindowColor(t->frame, valuemask);
      if((flush_expose(t->frame))||(expose_win == t->frame)||
         (expose_win == None))
      {
        /* remove any lines drawn in the border if hidden handles or noinset
         * and if a handle was pressed */
#ifdef BORDERSTYLE
        if ((PressedW == None) && (borderflags & (NoInset | HiddenHandles)))
          XClearWindow(dpy, t->frame);
#endif

        /* draw the inside relief */
        if(
#ifdef BORDERSTYLE
           !(borderflags & NoInset)
#endif
           && t->boundary_width > 2)
        {
          int height = t->frame_height - (t->boundary_width * 2 ) + 1;
#ifdef WINDOWSHADE
          /* Inset for shaded windows goes to the bottom */
          if (shaded) height += t->boundary_width;
#endif
          /* draw a single pixel band for MWMBorders and the top FvwmBorder line */
          RelieveRectangle(dpy, t->frame,
                           t->boundary_width - 1, t->boundary_width - 1,
                           t->frame_width - (t->boundary_width * 2 ) + 1,
                           height,
                           sgc, (t->flags & MWMBorders) ? rgc : sgc, 1);
          /* draw the rest of FvwmBorder Inset */
          if (!(t->flags & MWMBorders))
            RelieveRectangle(dpy, t->frame,
                             t->boundary_width - 2, t->boundary_width - 2,
                             t->frame_width - (t->boundary_width * 2) + 4,
                             height + 3,
                             sgc, rgc, 2);
        }

        /* draw the outside relief */
        if (t->flags & MWMBorders)
          RelieveRectangle(dpy, t->frame,
                           0, 0, t->frame_width - 1, t->frame_height - 1,
                           rgc, sgc, 2);
        else { /* FVWMBorder style has an extra line of shadow on top and left */
          RelieveRectangle(dpy, t->frame,
                           1, 1, t->frame_width - 2, t->frame_height - 2,
                           rgc, sgc, 2);
          RelieveRectangle(dpy, t->frame,
                           0, 0, t->frame_width - 1, t->frame_height - 1,
                           sgc, sgc, 1);
        }

        /* draw the handles as eight marks rectangles around the border */
        if ((t->flags & BORDER) && (t->boundary_width > 2))
#ifdef BORDERSTYLE
        if (!(borderflags & HiddenHandles))
#endif
        {
          /* MWM border windows have thin 3d effects
           * FvwmBorders have 3 pixels top/left, 2 bot/right so this makes
           * calculating the length of the marks difficult, top and bottom
           * marks for FvwmBorders are different if NoInset is specified */
          int tlength = t->boundary_width - 1;
          int blength = tlength;
          int badjust = 3 - rwidth; /* offset from ottom and right edge */
          XSegment marks[8];
          int j;

#ifdef BORDERSTYLE
          /* NoInset FvwmBorder windows need special treatment */
          if ((borderflags & NoInset) && (rwidth == 2)) {
            blength += 1;
          }
#endif BORDERSTYLE

          for (j = 0; j < rwidth; ) { /* j is incremented below */
            /* shorten marks for beveled effect */
            tlength -= 2 * j;
            blength -= 2 * j;
            badjust += j;

            /* hilite marks */
            i = 0;

            /* top left */
            marks[i].x2 = marks[i].x1 = t->corner_width + j;
            marks[i].y2 = (marks[i].y1 = 1 + j) + tlength;
            i++;
            /* top right */
            marks[i].x2 = marks[i].x1 = t->frame_width - t->corner_width + j;
            marks[i].y2 = (marks[i].y1 = 1 + j) + tlength;
            i++;
#ifdef WINDOWSHADE
            if (!shaded)
#endif
            { /* bot left */
              marks[i].x2 = marks[i].x1 = t->corner_width + j;
              marks[i].y2 = (marks[i].y1 = t->frame_height - badjust) - blength;
              i++;
              /* bot right */
              marks[i].x2 = marks[i].x1 = t->frame_width - t->corner_width + j;
              marks[i].y2 = (marks[i].y1 = t->frame_height - badjust) - blength;
              i++;
              /* left top */
              marks[i].x2 = (marks[i].x1 = 1 + j) + tlength;
              marks[i].y2 = marks[i].y1 = t->corner_width + j;
              i++;
              /* left bot */
              marks[i].x2 = (marks[i].x1 = 1 + j) + tlength;
              marks[i].y2 = marks[i].y1 = t->frame_height - t->corner_width + j;
              i++;
              /* right top */
              marks[i].x2 = (marks[i].x1 = t->frame_width - badjust) - blength;
              marks[i].y2 = marks[i].y1 = t->corner_width + j;
              i++;
              /* right bot */
              marks[i].x2 = (marks[i].x1 = t->frame_width - badjust) - blength;
              marks[i].y2 = marks[i].y1 = t->frame_height - t->corner_width + j;
              i++;
            }
            XDrawSegments(dpy, t->frame, rgc, marks, i);

            /* shadow marks, reuse the array assuming XDraw doesn't trash it */
            i = 0;
            j += j + 1; /* yuck, but j goes 0->1 in the first pass, 1->3 in 2nd */
            /* top left */
            marks[i].x2 = (marks[i].x1 -= j);
            i++;
            /* top right */
            marks[i].x2 = (marks[i].x1 -= j);
            i++;
#ifdef WINDOWSHADE
            if (!shaded)
#endif
            { /* bot left */
              marks[i].x2 = (marks[i].x1 -= j);
              i++;
              /* bot right */
              marks[i].x2 = (marks[i].x1 -= j);
              i++;
              /* left top */
              marks[i].y2 = (marks[i].y1 -= j);
              i++;
              /* left bot */
              marks[i].y2 = (marks[i].y1 -= j);
              i++;
              /* right top */
              marks[i].y2 = (marks[i].y1 -= j);
              i++;
              /* right bot */
              marks[i].y2 = (marks[i].y1 -= j);
              i++;
            }
            XDrawSegments(dpy, t->frame, sgc, marks, i);
          }
        }


        /* now draw the pressed in part on top */
        /* a bit hacky to draw twice but you should see the code it replaces
           never mind the esoterics, feel the thin-ness */
        /* undocumented dependancy on MWMbuttons !!!
           MWMButtons style makes the border undepressable */
        /* GSFR candidate */
        if((t->flags & BORDER) && !(t->flags & MWMButtons)) {
          if (PressedW == t->sides[0]) { /* top */
            RelieveRectangle(dpy, t->frame,
                             t->corner_width, 1,
                             t->frame_width - 2 * t->corner_width - 1,
                             t->boundary_width - 2,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->sides[1]) { /* right */
            RelieveRectangle(dpy, t->frame,
                             t->frame_width - t->boundary_width + rwidth - 1, t->corner_width,
                             t->boundary_width - 2,
                             t->frame_height - 2 * t->corner_width - 1,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->sides[2]) { /* bottom */
            RelieveRectangle(dpy, t->frame,
                             t->corner_width, t->frame_height - t->boundary_width + rwidth - 1,
                             t->frame_width - 2 * t->corner_width - 1,
                             t->boundary_width - 2,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->sides[3]) { /* left */
            RelieveRectangle(dpy, t->frame,
                             1, t->corner_width,
                             t->boundary_width - 2,
                             t->frame_height - 2 * t->corner_width - 1,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->corners[0]) { /* top left */
            /* drawn as two relieved rectangles, this will have to change if the
               title bar and buttons ever get to be InputOnly windows */
            RelieveRectangle(dpy, t->frame,
                             t->boundary_width - rwidth, t->boundary_width - rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             rgc, sgc, rwidth);
            RelieveRectangle(dpy, t->frame,
                             1, 1,
                             t->corner_width - 2, t->corner_width - 2,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->corners[1]) { /* top right */
            RelieveRectangle(dpy, t->frame,
                             t->frame_width - t->corner_width + rwidth - 2, t->boundary_width - rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             rgc, sgc, rwidth);
            RelieveRectangle(dpy, t->frame,
                             t->frame_width - t->corner_width, 1,
                             t->corner_width - 3 + rwidth, t->corner_width - 2,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->corners[2]) { /* bottom left */
            RelieveRectangle(dpy, t->frame,
                             t->boundary_width - rwidth, t->frame_height - t->corner_width + rwidth -2,
                             t->corner_width - t->boundary_width + rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             rgc, sgc, rwidth);
            RelieveRectangle(dpy, t->frame,
                             1, t->frame_height - t->corner_width,
                             t->corner_width - 2, t->corner_width - 3 + rwidth,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->corners[3]) { /* bottom right */
            RelieveRectangle(dpy, t->frame,
                             t->frame_width - t->corner_width + rwidth - 2, t->frame_height - t->corner_width + rwidth - 2,
                             t->corner_width - t->boundary_width + rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             rgc, sgc, rwidth);
            RelieveRectangle(dpy, t->frame,
                             t->frame_width - t->corner_width, t->frame_height - t->corner_width,
                             t->corner_width - 3 + rwidth, t->corner_width - 3 + rwidth,
                             sgc, rgc, rwidth);
          }
        }
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
  int rwidth; /* relief width */

  if(!t)
    return;
  if(!(t->flags & TITLE))
    return;

  rwidth = (t->flags & MWMBorders) ? 1 : 2;

  if (onoroff)
  {
    Forecolor = GetDecor(t,HiColors.fore);
    BackColor = GetDecor(t,HiColors.back);
    ReliefGC = GetDecor(t,HiReliefGC);
    ShadowGC = GetDecor(t,HiShadowGC);
  }
  else
  {
    Forecolor = t->TextPixel;
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
    RelieveRectangle(dpy,t->title_w,0,0,hor_off-3,t->title_height - 1,
                     ReliefGC, ShadowGC, rwidth);
    RelieveRectangle(dpy,t->title_w,hor_off+w+2,0,
                     t->title_width - w - hor_off-3,t->title_height - 1,
                     ReliefGC, ShadowGC, rwidth);
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
	      RelieveRectangle(dpy,t->title_w,0,0,
	                       t->title_width - 1,t->title_height - 1,
			       ShadowGC, ReliefGC, rwidth);
	  else
	      RelieveRectangle(dpy,t->title_w,0,0,
	                       t->title_width - 1,t->title_height - 1,
	                       ReliefGC, ShadowGC, rwidth);
      }

      if(t->name != (char *)NULL)
        XDrawString (dpy, t->title_w,Scr.ScratchGC3,hor_off,
                     GetDecor(t,WindowFont.y)+1,
                     t->name, strlen(t->name));
  }
  /* now, draw lines in title bar if it's a sticky window */
  if(t->flags & STICKY || Scr.StipledTitles)
  {
    /* an odd number of lines every 4 pixels */
    int num = (int)(t->title_height/8) * 2 - 1;
    int min = t->title_height / 2 - num * 2 + 1;
    int max = t->title_height / 2 + num * 2 - 3;
    for(i = min; i <= max; i += 4)
    {
      RelieveRectangle(dpy, t->title_w,
                       4, i, hor_off - 10, 1,
                       ShadowGC, ReliefGC, 1);
      RelieveRectangle(dpy, t->title_w,
                       hor_off + w + 6, i, t->title_width - hor_off - w - 11, 1,
                       ShadowGC, ReliefGC, 1);
    }
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

void SetupFrame(FvwmWindow *tmp_win,int x,int y,int w,int h,Bool sendEvent,
		Bool curr_shading)
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
      tmp_win->title_height = GetDecor(tmp_win,TitleHeight);

    tmp_win->title_width = w - (left + right) * tmp_win->title_height
                           - 2 * tmp_win->boundary_width;


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

      xwc.x=w-tmp_win->boundary_width;
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
      tmp_win->corner_width = GetDecor(tmp_win,TitleHeight) +
        tmp_win->boundary_width ;

      if(w < 2*tmp_win->corner_width)
        tmp_win->corner_width = w/3;
      if((h < 2*tmp_win->corner_width)
#ifdef WINDOWSHADE
        &&!shaded
#endif
         )
        tmp_win->corner_width = h/3;
      xwidth = w - 2*tmp_win->corner_width;
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
          xwc.x = w - tmp_win->boundary_width;
          xwc.y = tmp_win->corner_width;
          xwc.width = tmp_win->boundary_width;
          xwc.height = ywidth;

        }
        else if(i==2)
        {
          xwc.x = tmp_win->corner_width;
          xwc.y = h - tmp_win->boundary_width;
          xwc.height = tmp_win->boundary_width;
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
          xwc.x = w - tmp_win->corner_width;
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
  cx = tmp_win->boundary_width;
  cy = tmp_win->title_height + tmp_win->boundary_width;

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
  if (!curr_shading) {
      frame_wc.x = tmp_win->frame_x = x;
      frame_wc.y = tmp_win->frame_y = y;
      frame_wc.width = tmp_win->frame_width = w;
      frame_wc.height = tmp_win->frame_height = h;
      frame_mask = (CWX | CWY | CWWidth | CWHeight);
      XConfigureWindow (dpy, tmp_win->frame, frame_mask, &frame_wc);
  }
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

    client_event.xconfigure.border_width = 0;
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
      rect.width = w - 2*tmp_win->boundary_width;
      rect.height = tmp_win->title_height;


      XShapeCombineRectangles(dpy,tmp_win->frame,ShapeBounding,
                              0,0,&rect,1,ShapeUnion,Unsorted);
    }
  }
#endif
}
