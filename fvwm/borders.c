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
#include "icons.h"
#include "screen.h"
#include "misc.h"
#include "Module.h"
#include "borders.h"
#include "module_interface.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

extern Window PressedW;
extern FvwmDecor *cur_decor;
XGCValues Globalgcv;
unsigned long Globalgcm;

extern Boolean ReadButtonFace(
  char *s, ButtonFace *bf, int button, int verbose);
extern void FreeButtonFace(Display *dpy, ButtonFace *bf);

/* change window background color/pixmap */
static void change_window_color(Window w, unsigned long valuemask,
				XSetWindowAttributes *attributes)
{
  XChangeWindowAttributes(dpy, w, valuemask, attributes);
  XClearWindow(dpy, w);
}

#ifdef VECTOR_BUTTONS
/****************************************************************************
 *
 *  Draws a little pattern within a window (more complex)
 *
 ****************************************************************************/
static void DrawLinePattern(Window win, GC ReliefGC, GC ShadowGC,
			    struct vector_coords *coords, int w, int h)
{
  int i;
  for (i = 1; i < coords->num; ++i)
  {
      XDrawLine(dpy,win,
		(coords->line_style & (1 << i))  ? ReliefGC : ShadowGC,
		w * coords->x[i-1]/100,
		h * coords->y[i-1]/100,
		w * coords->x[i]/100,
		h * coords->y[i]/100);
  }
}
#endif /* VECTOR_BUTTONS */

/****************************************************************************
 *
 *  Redraws buttons (veliaa@rpi.edu)
 *
 ****************************************************************************/
static void DrawButton(FvwmWindow *t, Window win, int w, int h,
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
      if(HAS_MWM_BUTTONS(t) &&
	 ((stateflags & MWMDecorMaximize && IS_MAXIMIZED(t)) ||
	  (stateflags & MWMDecorShade && IS_SHADED(t)) ||
	  (stateflags & MWMDecorStick && IS_STICKY(t))))
	DrawLinePattern(win, ShadowGC, ReliefGC, &bf->u.vector, w, h);
      else
	DrawLinePattern(win, ReliefGC, ShadowGC, &bf->u.vector, w, h);
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
	border = HAS_MWM_BORDER(t) ? 1 : 2;
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

      if (x < border)
	x = border;
      if (y < border)
	y = border;
      if (width > p->width)
	width = p->width;
      if (height > p->height)
	height = p->height;
      if (width > w - x - border)
	width = w - x - border;
      if (height > h - y - border)
	height = h - y - border;

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
    case GradientButton:
    {
      XRectangle bounds;
      bounds.x = bounds.y = 0;
      bounds.width = w;
      bounds.height = h;
      flush_expose(win);

#ifdef PIXMAP_BUTTONS
      XSetClipMask(dpy, Scr.TransMaskGC, None);
#endif

      switch (bf->u.grad.gradient_type)
      {
      case H_GRADIENT:
        {
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
	}
	break;
      case V_GRADIENT:
        {
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
	break;
      case B_GRADIENT:
      case D_GRADIENT:
      case R_GRADIENT:
      case Y_GRADIENT:
      case S_GRADIENT:
      case C_GRADIENT:
      default:
        {
	  unsigned int width;
	  unsigned int height;
	  Pixmap pixmap;
	  Pixmap stretched_pixmap;

	  /***
	   *** This stuff is experimental and *very* inefficient. For every
	   *** redraw it creates and destroys three pixmaps
	   ***/

	  /* find out the size the pixmap should be */
	  CalculateGradientDimensions(
	    dpy, win, bf->u.grad.npixels, bf->u.grad.gradient_type,
	    &width, &height);
	  /* create a pixmap with the gradient */
	  pixmap = CreateGradientPixmap(
	    dpy, win, Scr.TransMaskGC, bf->u.grad.gradient_type, width, height,
	    bf->u.grad.npixels, bf->u.grad.pixels);
	  if (pixmap != None)
	  {
	    /* scale the pixmap to the proper dimensions */
	    stretched_pixmap = CreateStretchPixmap(
	      dpy, pixmap, width, height, /*!!!*/Pdepth, bounds.width,
	      bounds.height, Scr.TransMaskGC);
	    XFreePixmap(dpy, pixmap);
	    /* copy the pixmap into the window */
	    if (stretched_pixmap)
	    {
	      XCopyArea(dpy, stretched_pixmap, win, Scr.TransMaskGC, 0, 0,
			bounds.width, bounds.height, bounds.x, bounds.y);
	      XFreePixmap(dpy, pixmap);
	    }
	  }
        }
	break;
      }
      break;
    }
    break;
#endif /* GRADIENT_BUTTONS */

    default:
      fvwm_msg(ERR,"DrawButton","unknown button type");
      break;
    }
}

/* rules to get button state */
static enum ButtonState get_button_state(Bool onoroff, Bool toggled, Window w)
{
#if 0
#if defined(ACTIVEDOWN_BTNS) && defined(INACTIVE_BTNS)
  if (!onoroff)
    return toggled ? ToggledInactive : Inactive;
  else
    return (PressedW == w)
      ? (toggled ? ToggledActiveDown : ActiveDown)
      : (toggled ? ToggledActiveUp : ActiveUp);
#elif defined(ACTIVEDOWN_BTNS) && !defined(INACTIVE_BTNS)
  return (PressedW == w)
      ? (toggled ? ToggledActiveDown : ActiveDown)
      : (toggled ? ToggledActiveUp : ActiveUp);
#elif !defined(ACTIVEDOWN_BTNS) && defined(INACTIVE_BTNS)
  return (onoroff) ?
      : (toggled ? ToggledActiveUp : ActiveUp)
      ? (toggled ? ToggledInactive : Inactive)
#elif !defined(ACTIVEDOWN_BTNS) && !defined(INACTIVE_BTNS)
  return toggled ? ToggledActiveUp : ActiveUp;
#endif

#else
  if (Scr.gs.use_active_down_buttons)
  {
    if (Scr.gs.use_inactive_buttons && !onoroff)
      return (toggled) ? ToggledInactive : Inactive;
    else
      return (PressedW == w)
	? (toggled ? ToggledActiveDown : ActiveDown)
	: (toggled ? ToggledActiveUp : ActiveUp);
  }
  else
  {
    if (Scr.gs.use_inactive_buttons && !onoroff)
    {
      return (toggled) ? ToggledInactive : Inactive;
    }
    else
    {
      return (toggled) ? ToggledActiveUp : ActiveUp;
    }
  }
#endif
}

/****************************************************************************
 *
 * Redraws the windows borders and the title bar
 *
 ****************************************************************************/
void SetBorder (FvwmWindow *t, Bool onoroff,Bool force,Bool Mapped,
		Window expose_win)
{
  RedrawBorder(t, onoroff, force, Mapped, expose_win);
  SetTitleBar(t,onoroff, False);
}


/****************************************************************************
 *
 * Redraws the windows borders
 *
 ****************************************************************************/
void RedrawBorder (FvwmWindow *t, Bool onoroff,Bool force,Bool Mapped,
		   Window expose_win)
{
  int i;
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
#ifdef BORDERSTYLE
  int borderflags;
#endif /* BORDERSTYLE */
  int rwidth; /* relief width */
  Bool shaded;

  if(!t)
    return;

  if(HAS_NEVER_FOCUS(t))
    onoroff = False;

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

    if((Scr.Hilite != t)||(force > 1))
      NewColor = True;

    /* make sure that the previously highlighted window got unhighlighted */
    if((Scr.Hilite != t)&&(Scr.Hilite != NULL))
      SetBorder(Scr.Hilite,False,False,True,None);

#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
    /* are we using textured borders? */
    if ((GetDecor(t,BorderStyle.active.style)
	 & ButtonFaceTypeMask) == TiledPixmapButton)
    {
      TexturePixmap = GetDecor(t,BorderStyle.active.u.p->picture);
    }
#endif

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

    if((Scr.Hilite == t)||(force > 1))
    {
      Scr.Hilite = NULL;
      NewColor = True;
    }

#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
    if ((GetDecor(t,BorderStyle.inactive.style)
	 & ButtonFaceTypeMask) == TiledPixmapButton)
    {
      TexturePixmap = GetDecor(t,BorderStyle.inactive.u.p->picture);
    }
#endif

    TextColor =t->TextPixel;
    BackPixmap = Scr.light_gray_pixmap;
    if(IS_STICKY(t))
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
  rwidth = (HAS_MWM_BORDER(t) ? 1 : 2);

  shaded = IS_SHADED(t);

  if(IS_ICONIFIED(t))
  {
    DrawIconWindow(t);
    return;
  }

#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
  if (TexturePixmap)
  {
    attributes.background_pixmap = TexturePixmap;
    valuemask = CWBackPixmap;
    if (Pdepth < 2)
    {
      notex_attributes.background_pixmap = BackPixmap;
      notex_valuemask = CWBackPixmap;
    }
    else
    {
      notex_attributes.background_pixel = BackColor;
      notex_valuemask = CWBackPixel;
    }
  }
  else
#endif
  if (Pdepth < 2)
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

  if(HAS_TITLE(t))
  {
    if (NewColor)
    {
      change_window_color(t->title_w, notex_valuemask, &notex_attributes);
    }
    for(i=0;i<Scr.nr_left_buttons;++i)
    {
      if(t->left_w[i] != None)
      {
	int stateflags = GetDecor(t,left_buttons[i].flags);
	Bool toggled =
	  (HAS_MWM_BUTTONS(t) &&
	   ((stateflags & MWMDecorMaximize && IS_MAXIMIZED(t)) ||
	    (stateflags & MWMDecorShade && IS_SHADED(t)) ||
	    (stateflags & MWMDecorStick && IS_STICKY(t))));
	enum ButtonState bs = get_button_state(onoroff, toggled, t->left_w[i]);
	ButtonFace *bf = &GetDecor(t,left_buttons[i].state[bs]);
#if !(defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE))
	if (NewColor)
	  change_window_color(t->left_w[i], valuemask, &attributes);
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
	  {
	    XChangeWindowAttributes(dpy, t->left_w[i],
				    valuemask, &attributes);
	  }
	  else
	  {
	    XChangeWindowAttributes(dpy, t->left_w[i], notex_valuemask,
				    &notex_attributes);
	  }
	  XClearWindow(dpy, t->left_w[i]);
#endif
#ifdef EXTENDED_TITLESTYLE
	  if (bf->style & UseTitleStyle)
	  {
	    ButtonFace *tsbf = &GetDecor(t,titlebar.state[bs]);
#ifdef MULTISTYLE
	    for (; tsbf; tsbf = tsbf->next)
#endif
	      DrawButton(t, t->left_w[i],
			 t->title_g.height, t->title_g.height,
			 tsbf, ReliefGC, ShadowGC,
			 inverted,
			 GetDecor(t,left_buttons[i].flags));
	  }
#endif /* EXTENDED_TITLESTYLE */
#ifdef MULTISTYLE
	  for (; bf; bf = bf->next)
#endif
	    DrawButton(t, t->left_w[i],
		       t->title_g.height, t->title_g.height,
		       bf, ReliefGC, ShadowGC,
		       inverted, GetDecor(t,left_buttons[i].flags));

	  if (!(GetDecor(t,left_buttons[i].state[bs].style) &
		FlatButton)) {
	    if (GetDecor(t,left_buttons[i].state[bs].style) &
		SunkButton)
	      RelieveRectangle(dpy,t->left_w[i],0,0,
			       t->title_g.height - 1,
			       t->title_g.height - 1,
			       (inverted ? ReliefGC : ShadowGC),
			       (inverted ? ShadowGC : ReliefGC),
			       rwidth);
	    else
	      RelieveRectangle(dpy,t->left_w[i],0,0,
			       t->title_g.height - 1,
			       t->title_g.height - 1,
			       (inverted ? ShadowGC : ReliefGC),
			       (inverted ? ReliefGC : ShadowGC),
			       rwidth);
	  }
	}
      }
    }
    for(i=0;i<Scr.nr_right_buttons;++i)
    {
      if(t->right_w[i] != None)
      {
	int stateflags = GetDecor(t,right_buttons[i].flags);
	Bool toggled =
	  (HAS_MWM_BUTTONS(t) &&
	   ((stateflags & MWMDecorMaximize && IS_MAXIMIZED(t)) ||
	    (stateflags & MWMDecorShade && IS_SHADED(t)) ||
	    (stateflags & MWMDecorStick && IS_STICKY(t))));
	enum ButtonState bs = get_button_state(onoroff, toggled, t->right_w[i]);
	ButtonFace *bf = &GetDecor(t,right_buttons[i].state[bs]);
#if !(defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE))
	if (NewColor)
	  change_window_color(t->right_w[i], valuemask, &attributes);
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
				    notex_valuemask,
				    &notex_attributes);
	  XClearWindow(dpy, t->right_w[i]);
#endif
#ifdef EXTENDED_TITLESTYLE
	  if (bf->style & UseTitleStyle) {
	    ButtonFace *tsbf = &GetDecor(t,titlebar.state[bs]);
#ifdef MULTISTYLE
	    for (; tsbf; tsbf = tsbf->next)
#endif
	      DrawButton(t, t->right_w[i],
			 t->title_g.height, t->title_g.height,
			 tsbf, ReliefGC, ShadowGC,
			 inverted,
			 GetDecor(t,right_buttons[i].flags));
	  }
#endif /* EXTENDED_TITLESTYLE */
#ifdef MULTISTYLE
	  for (; bf; bf = bf->next)
#endif
	    DrawButton(t, t->right_w[i],
		       t->title_g.height, t->title_g.height,
		       bf, ReliefGC, ShadowGC,
		       inverted, GetDecor(t,right_buttons[i].flags));

	  if (!(GetDecor(t,right_buttons[i].state[bs].style) &
		FlatButton)) {
	    if (GetDecor(t,right_buttons[i].state[bs].style) &
		SunkButton)
	      RelieveRectangle(dpy,t->right_w[i],0,0,
			       t->title_g.height - 1,
			       t->title_g.height - 1,
			       (inverted ? ReliefGC : ShadowGC),
			       (inverted ? ShadowGC : ReliefGC),
			       rwidth);
	    else
	      RelieveRectangle(dpy,t->right_w[i],0,0,
			       t->title_g.height - 1,
			       t->title_g.height - 1,
			       (inverted ? ShadowGC : ReliefGC),
			       (inverted ? ReliefGC : ShadowGC),
			       rwidth);
	  }
	}
      }
    }
  }

  /* draw the border */
  /* resize handles are InputOnly so draw in the frame and it will show
   * through */
  {
    /* for mono - put a black border on
     * for color, make it the color of the decoration background */
    if(t->boundary_width < 2)
    {
      flush_expose (t->frame);
      if(Pdepth <2)
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

      if(HAS_DEPRESSABLE_BORDER(t)&&(PressedW == t->frame)) {
        sgc=ReliefGC;
        rgc=ShadowGC;
      } else {
        rgc=ReliefGC;
        sgc=ShadowGC;
      }
      if (NewColor)
	change_window_color(t->frame, valuemask, &attributes);
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
        if(t->boundary_width > 2
#ifdef BORDERSTYLE
           && !(borderflags & NoInset)
#endif
	   )
        {
          int height = t->frame_g.height - (t->boundary_width * 2 ) + 1;
          /* Inset for shaded windows goes to the bottom */
          if (shaded) height += t->boundary_width;
          /* draw a single pixel band for MWMBorders and the top FvwmBorder
	   * line */
          RelieveRectangle(dpy, t->frame,
                           t->boundary_width - 1, t->boundary_width - 1,
                           t->frame_g.width - (t->boundary_width * 2 ) + 1,
                           height,
                           sgc, HAS_MWM_BORDER(t) ? rgc : sgc, 1);
          /* draw the rest of FvwmBorder Inset */
          if (!HAS_MWM_BORDER(t))
            RelieveRectangle(dpy, t->frame,
                             t->boundary_width - 2, t->boundary_width - 2,
                             t->frame_g.width - (t->boundary_width * 2) + 4,
                             height + 3,
                             sgc, rgc, 2);
        }

        /* draw the outside relief */
        if (HAS_MWM_BORDER(t))
          RelieveRectangle(dpy, t->frame,
                           0, 0, t->frame_g.width - 1, t->frame_g.height - 1,
                           rgc, sgc, 2);
        else {
	  /* FVWMBorder style has an extra line of shadow on top and left */
          RelieveRectangle(dpy, t->frame,
                           1, 1, t->frame_g.width - 2, t->frame_g.height - 2,
                           rgc, sgc, 2);
          RelieveRectangle(dpy, t->frame,
                           0, 0, t->frame_g.width - 1, t->frame_g.height - 1,
                           sgc, sgc, 1);
        }

        /* draw the handles as eight marks rectangles around the border */
        if (HAS_BORDER(t) && (t->boundary_width > 2))
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
#endif /* BORDERSTYLE */

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
            marks[i].x2 = marks[i].x1 = t->frame_g.width - t->corner_width + j;
            marks[i].y2 = (marks[i].y1 = 1 + j) + tlength;
            i++;
            if (!shaded)
            {
	      /* bot left */
              marks[i].x2 = marks[i].x1 = t->corner_width + j;
              marks[i].y2 = (marks[i].y1 = t->frame_g.height-badjust)-blength;
              i++;
              /* bot right */
              marks[i].x2 = marks[i].x1 = t->frame_g.width - t->corner_width+j;
              marks[i].y2 = (marks[i].y1 = t->frame_g.height-badjust)-blength;
              i++;
              /* left top */
              marks[i].x2 = (marks[i].x1 = 1 + j) + tlength;
              marks[i].y2 = marks[i].y1 = t->corner_width + j;
              i++;
              /* left bot */
              marks[i].x2 = (marks[i].x1 = 1 + j) + tlength;
              marks[i].y2 = marks[i].y1 = t->frame_g.height-t->corner_width+j;
              i++;
              /* right top */
              marks[i].x2 = (marks[i].x1 = t->frame_g.width-badjust) - blength;
              marks[i].y2 = marks[i].y1 = t->corner_width + j;
              i++;
              /* right bot */
              marks[i].x2 = (marks[i].x1 = t->frame_g.width-badjust) - blength;
              marks[i].y2 = marks[i].y1 = t->frame_g.height-t->corner_width +j;
              i++;
            }
            XDrawSegments(dpy, t->frame, rgc, marks, i);

            /* shadow marks, reuse the array assuming XDraw doesn't trash it */
            i = 0;
	    /* yuck, but j goes 0->1 in the first pass, 1->3 in 2nd */
            j += j + 1;
            /* top left */
            marks[i].x2 = (marks[i].x1 -= j);
            i++;
            /* top right */
            marks[i].x2 = (marks[i].x1 -= j);
            i++;
            if (!shaded)
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
        if(HAS_BORDER(t) && HAS_DEPRESSABLE_BORDER(t)) {
          if (PressedW == t->sides[0]) { /* top */
            RelieveRectangle(dpy, t->frame,
                             t->corner_width, 1,
                             t->frame_g.width - 2 * t->corner_width - 1,
                             t->boundary_width - 2,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->sides[1]) { /* right */
            RelieveRectangle(dpy, t->frame,
                             t->frame_g.width - t->boundary_width + rwidth - 1,
			     t->corner_width,
                             t->boundary_width - 2,
                             t->frame_g.height - 2 * t->corner_width - 1,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->sides[2]) { /* bottom */
            RelieveRectangle(dpy, t->frame,
                             t->corner_width,
			     t->frame_g.height - t->boundary_width + rwidth - 1,
                             t->frame_g.width - 2 * t->corner_width - 1,
                             t->boundary_width - 2,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->sides[3]) { /* left */
            RelieveRectangle(dpy, t->frame,
                             1, t->corner_width,
                             t->boundary_width - 2,
                             t->frame_g.height - 2 * t->corner_width - 1,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->corners[0]) { /* top left */
            /* drawn as two relieved rectangles, this will have to change if
	     * the title bar and buttons ever get to be InputOnly windows */
            RelieveRectangle(dpy, t->frame,
                             t->boundary_width - rwidth,
			     t->boundary_width - rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             rgc, sgc, rwidth);
            RelieveRectangle(dpy, t->frame,
                             1, 1,
                             t->corner_width - 2, t->corner_width - 2,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->corners[1]) { /* top right */
            RelieveRectangle(dpy, t->frame,
                             t->frame_g.width - t->corner_width + rwidth - 2,
			     t->boundary_width - rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             rgc, sgc, rwidth);
            RelieveRectangle(dpy, t->frame,
                             t->frame_g.width - t->corner_width, 1,
                             t->corner_width - 3 + rwidth, t->corner_width - 2,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->corners[2]) { /* bottom left */
            RelieveRectangle(dpy, t->frame,
                             t->boundary_width - rwidth,
			     t->frame_g.height - t->corner_width + rwidth -2,
                             t->corner_width - t->boundary_width + rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             rgc, sgc, rwidth);
            RelieveRectangle(dpy, t->frame,
                             1, t->frame_g.height - t->corner_width,
                             t->corner_width - 2, t->corner_width - 3 + rwidth,
                             sgc, rgc, rwidth);
          } else if (PressedW == t->corners[3]) { /* bottom right */
            RelieveRectangle(dpy, t->frame,
                             t->frame_g.width - t->corner_width + rwidth - 2,
			     t->frame_g.height - t->corner_width + rwidth - 2,
                             t->corner_width - t->boundary_width + rwidth,
                             t->corner_width - t->boundary_width + rwidth,
                             rgc, sgc, rwidth);
            RelieveRectangle(dpy, t->frame,
                             t->frame_g.width - t->corner_width,
			     t->frame_g.height - t->corner_width,
                             t->corner_width - 3 + rwidth,
			     t->corner_width - 3 + rwidth,
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
  if(!HAS_TITLE(t))
    return;

  rwidth = (HAS_MWM_BORDER(t)) ? 1 : 2;

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
#ifdef I18N_MB /* cannot use macro here, rewriting... */
    w=XmbTextEscapement(GetDecor(t,WindowFont.fontset),t->name,
			strlen(t->name));
#else
    w=XTextWidth(GetDecor(t,WindowFont.font),t->name,
		 strlen(t->name));
#endif
    if(w > t->title_g.width-12)
      w = t->title_g.width-4;
    if(w < 0)
      w = 0;
  }
  else
    w = 0;

  tb_flags = GetDecor(t,titlebar.flags);
  {
    Bool toggled =
      (HAS_MWM_BUTTONS(t) &&
       ((tb_flags & MWMDecorMaximize && IS_MAXIMIZED(t)) ||
	(tb_flags & MWMDecorShade && IS_SHADED(t)) ||
	(tb_flags & MWMDecorStick && IS_STICKY(t))));
    title_state = get_button_state(onoroff, toggled, t->title_w);
  }
  tb_style = GetDecor(t,titlebar.state[title_state].style);
  if (tb_flags & HOffCenter) {
    if (tb_flags & HRight)
      hor_off = t->title_g.width - w - 10;
    else
      hor_off = 10;
  } else
    hor_off = (t->title_g.width - w) / 2;

  NewFontAndColor(GetDecor(t,WindowFont.font->fid),Forecolor, BackColor);

  /* the next bit tries to minimize redraw based upon compilation options
   * (veliaa@rpi.edu) */
#ifdef EXTENDED_TITLESTYLE
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
  /* we need to check for UseBorderStyle for the titlebar */
  {
      ButtonFace *bf = onoroff
	  ? &GetDecor(t,BorderStyle.active)
	  : &GetDecor(t,BorderStyle.inactive);

      if ((tb_style & UseBorderStyle) &&
	  ((bf->style & ButtonFaceTypeMask) == TiledPixmapButton))
      {
	XSetWindowBackgroundPixmap(dpy,t->title_w,bf->u.p->picture);
      }
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
  if(Pdepth<2)
  {
    RelieveRectangle(dpy,t->title_w,0,0,hor_off-3,t->title_g.height - 1,
                     ReliefGC, ShadowGC, rwidth);
    RelieveRectangle(dpy,t->title_w,hor_off+w+2,0,
                     t->title_g.width - w - hor_off-3,t->title_g.height - 1,
                     ReliefGC, ShadowGC, rwidth);
    XFillRectangle(dpy,t->title_w,
                   (PressedW==t->title_w?ShadowGC:ReliefGC),
                   hor_off - 2, 0, w+4,t->title_g.height);

    XDrawLine(dpy,t->title_w,ShadowGC,hor_off+w+1,0,hor_off+w+1,
              t->title_g.height);
    if(t->name != (char *)NULL)
#ifdef I18N_MB
      XmbDrawString (dpy, t->title_w, GetDecor(t,WindowFont.fontset), Scr.ScratchGC3,hor_off,
#else
      XDrawString (dpy, t->title_w,Scr.ScratchGC3,hor_off,
#endif
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
	      DrawButton(t, t->title_w, t->title_g.width, t->title_g.height,
			 bf, ShadowGC, ReliefGC, True, 0);
      } else {
#ifdef MULTISTYLE
	  for (; bf; bf = bf->next)
#endif
	      DrawButton(t, t->title_w, t->title_g.width, t->title_g.height,
			 bf, ReliefGC, ShadowGC, False, 0);
      }
#endif /* EXTENDED_TITLESTYLE */

      if (!(tb_style & FlatButton))
      {
	if (tb_style & SunkButton)
	  RelieveRectangle(dpy,t->title_w,0,0,
			   t->title_g.width - 1,t->title_g.height - 1,
			   ShadowGC, ReliefGC, rwidth);
	else
	  RelieveRectangle(dpy,t->title_w,0,0,
			   t->title_g.width - 1,t->title_g.height - 1,
			   ReliefGC, ShadowGC, rwidth);
      }

      if(t->name != (char *)NULL)
#ifdef I18N_MB
        XmbDrawString (dpy, t->title_w, GetDecor(t,WindowFont.fontset), Scr.ScratchGC3,hor_off,
#else
        XDrawString (dpy, t->title_w,Scr.ScratchGC3,hor_off,
#endif
                     GetDecor(t,WindowFont.y)+1,
                     t->name, strlen(t->name));
  }
  /* now, draw lines in title bar if it's a sticky window */
  if(IS_STICKY(t) || Scr.go.StipledTitles)
  {
    /* an odd number of lines every 4 pixels */
    int num = (int)(t->title_g.height/8) * 2 - 1;
    int min = t->title_g.height / 2 - num * 2 + 1;
    int max = t->title_g.height / 2 + num * 2 - 3;
    for(i = min; i <= max; i += 4)
    {
      RelieveRectangle(dpy, t->title_w,
                       4, i, hor_off - 10, 1,
                       ShadowGC, ReliefGC, 1);
      RelieveRectangle(dpy, t->title_w,
                       hor_off + w + 6, i, t->title_g.width - hor_off - w - 11,
		       1, ShadowGC, ReliefGC, 1);
    }
  }

}



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
  int shaded = IS_SHADED(tmp_win);

#ifdef FVWM_DEBUG_MSGS
  fvwm_msg(DBG,"SetupFrame",
           "Routine Entered (x == %d, y == %d, w == %d, h == %d)",
           x, y, w, h);
#endif

  /* if windows is not being maximized, save size in case of maximization */
  if (!IS_MAXIMIZED(tmp_win))
  {
    /* store orig values in absolute coords */
    tmp_win->orig_g.x = x + Scr.Vx;
    tmp_win->orig_g.y = y + Scr.Vy;
    tmp_win->orig_g.width = w;
    if (!shaded)
      tmp_win->orig_g.height = h;
  }

  if((w != tmp_win->frame_g.width) || (h != tmp_win->frame_g.height))
    Resized = True;
  if ((x != tmp_win->frame_g.x || y != tmp_win->frame_g.y))
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

    if (HAS_TITLE(tmp_win))
      tmp_win->title_g.height = GetDecor(tmp_win,TitleHeight);

    tmp_win->title_g.width = w - (left + right) * tmp_win->title_g.height
                           - 2 * tmp_win->boundary_width;


    if(tmp_win->title_g.width < 1)
      tmp_win->title_g.width = 1;

    if (HAS_TITLE(tmp_win))
    {
      xwcm = CWWidth | CWX | CWY | CWHeight;
      tmp_win->title_g.x = tmp_win->boundary_width+
        (left)*tmp_win->title_g.height;
      if(tmp_win->title_g.x >=  w - tmp_win->boundary_width)
        tmp_win->title_g.x = -10;
      tmp_win->title_g.y = tmp_win->boundary_width;

      xwc.width = tmp_win->title_g.width;

      xwc.height = tmp_win->title_g.height;
      xwc.x = tmp_win->title_g.x;
      xwc.y = tmp_win->title_g.y;
      XConfigureWindow(dpy, tmp_win->title_w, xwcm, &xwc);


      xwcm = CWX | CWY | CWHeight | CWWidth;
      xwc.height = tmp_win->title_g.height;
      xwc.width = tmp_win->title_g.height;

      xwc.y = tmp_win->boundary_width;
      xwc.x = tmp_win->boundary_width;
      for(i=0;i<Scr.nr_left_buttons;i++)
      {
        if(tmp_win->left_w[i] != None)
        {
          if(xwc.x + tmp_win->title_g.height < w-tmp_win->boundary_width)
            XConfigureWindow(dpy, tmp_win->left_w[i], xwcm, &xwc);
          else
          {
            xwc.x = -tmp_win->title_g.height;
            XConfigureWindow(dpy, tmp_win->left_w[i], xwcm, &xwc);
          }
          xwc.x += tmp_win->title_g.height;
        }
      }

      xwc.x=w-tmp_win->boundary_width;
      for(i=0;i<Scr.nr_right_buttons;i++)
      {
        if(tmp_win->right_w[i] != None)
        {
          xwc.x -=tmp_win->title_g.height;
          if(xwc.x > tmp_win->boundary_width)
            XConfigureWindow(dpy, tmp_win->right_w[i], xwcm, &xwc);
          else
          {
            xwc.x = -tmp_win->title_g.height;
            XConfigureWindow(dpy, tmp_win->right_w[i], xwcm, &xwc);
          }
        }
      }
    }

    if(HAS_BORDER(tmp_win))
    {
      tmp_win->corner_width = GetDecor(tmp_win,TitleHeight) +
        tmp_win->boundary_width ;

      if(w < 2*tmp_win->corner_width)
        tmp_win->corner_width = w/3;
      if((h < 2*tmp_win->corner_width) && (!shaded))
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
        if (!shaded||(i!=2))
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

	if (!shaded||(i==0)||(i==1))
	    XConfigureWindow(dpy, tmp_win->corners[i], xwcm, &xwc);
      }

    }
  }
  tmp_win->attr.width = w - 2*tmp_win->boundary_width;
  tmp_win->attr.height = h - tmp_win->title_g.height
    - 2*tmp_win->boundary_width;
  cx = tmp_win->boundary_width;
  cy = tmp_win->title_g.height + tmp_win->boundary_width;

  if (!shaded)
  {
    XResizeWindow(dpy, tmp_win->w, tmp_win->attr.width, tmp_win->attr.height);
    XMoveResizeWindow(dpy, tmp_win->Parent, cx,cy, tmp_win->attr.width,
		      tmp_win->attr.height);
  }

  /*
   * fix up frame and assign size/location values in tmp_win
   */
  if (!curr_shading) {
      frame_wc.x = tmp_win->frame_g.x = x;
      frame_wc.y = tmp_win->frame_g.y = y;
      frame_wc.width = tmp_win->frame_g.width = w;
      frame_wc.height = tmp_win->frame_g.height = h;
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
  if (sendEvent /*&& !shaded*/)
  {
    client_event.type = ConfigureNotify;
    client_event.xconfigure.display = dpy;
    client_event.xconfigure.event = tmp_win->w;
    client_event.xconfigure.window = tmp_win->w;

    client_event.xconfigure.x = x + tmp_win->boundary_width;
    client_event.xconfigure.y = y + tmp_win->title_g.height+
      tmp_win->boundary_width;
    client_event.xconfigure.width = w-2*tmp_win->boundary_width;
    client_event.xconfigure.height =h-2*tmp_win->boundary_width -
      tmp_win->title_g.height;

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
#if 1
    /* This is for buggy tk, which waits for the real ConfigureNotify
       on frame instead of the synthetic one on w. The geometry data
       in the event will not be correct for the frame, but tk doesn't
       look at that data anyway. */
    client_event.xconfigure.event = tmp_win->frame;
    client_event.xconfigure.window = tmp_win->frame;

    XSendEvent(dpy, tmp_win->frame, False,StructureNotifyMask,&client_event);
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
    if (tmp_win->wShaped)
    {
      XRectangle rect;

      XShapeCombineShape (dpy, tmp_win->frame, ShapeBounding,
			  tmp_win->boundary_width,
			  tmp_win->title_g.height+tmp_win->boundary_width,
			  tmp_win->w,
			  ShapeBounding, ShapeSet);
      if (tmp_win->title_w)
	{
	  /* windows w/ titles */
	  rect.x = tmp_win->boundary_width;
	  rect.y = tmp_win->title_g.y;
	  rect.width = w - 2*tmp_win->boundary_width;
	  rect.height = tmp_win->title_g.height;


	  XShapeCombineRectangles(dpy,tmp_win->frame,ShapeBounding,
				  0,0,&rect,1,ShapeUnion,Unsorted);
	}
    }
    else
      XShapeCombineMask (dpy, tmp_win->frame, ShapeBounding,
			 0, 0, None, ShapeSet);
  }
#endif
}

/****************************************************************************
 *
 *  Sets the allowed button states
 *
 ****************************************************************************/
void cmd_button_state(F_CMD_ARGS)
{
  char *token;

  while ((token = PeekToken(action, &action)))
  {
    static char first = True;
    if (!token && first)
    {
      Scr.gs.use_active_down_buttons = DEFAULT_USE_ACTIVE_DOWN_BUTTONS;
      Scr.gs.use_inactive_buttons = DEFAULT_USE_INACTIVE_BUTTONS;
      return;
    }
    first = False;
    if (StrEquals("activedown", token))
    {
      Scr.gs.use_active_down_buttons =
	ParseToggleArgument(action, &action, DEFAULT_USE_ACTIVE_DOWN_BUTTONS,
			    True);
    }
    else if (StrEquals("inactive", token))
    {
      Scr.gs.use_inactive_buttons =
	ParseToggleArgument(action, &action, DEFAULT_USE_ACTIVE_DOWN_BUTTONS,
			    True);
    }
    else
    {
      Scr.gs.use_active_down_buttons = DEFAULT_USE_ACTIVE_DOWN_BUTTONS;
      Scr.gs.use_inactive_buttons = DEFAULT_USE_INACTIVE_BUTTONS;
      fvwm_msg(ERR, "cmd_button_state", "unknown button state %s\n", token);
      return;
    }
  }
  return;
}


#ifdef BORDERSTYLE
/****************************************************************************
 *
 *  Sets the border style (veliaa@rpi.edu)
 *
 ****************************************************************************/
void SetBorderStyle(F_CMD_ARGS)
{
    char *parm = NULL, *prev = action;
#ifdef USEDECOR
    FvwmDecor *fl = cur_decor ? cur_decor : &Scr.DefaultDecor;
#else
    FvwmDecor *fl = &Scr.DefaultDecor;
#endif

    action = GetNextToken(action, &parm);
    while (parm)
    {
	if (StrEquals(parm,"active") || StrEquals(parm,"inactive"))
	{
	    int len;
	    char *end, *tmp;
	    ButtonFace tmpbf, *bf;
	    tmpbf.style = SimpleButton;
#ifdef MULTISTYLE
	    tmpbf.next = NULL;
#endif
#ifdef MINI_ICONS
	    tmpbf.u.p = NULL;
#endif
	    if (StrEquals(parm,"active"))
		bf = &fl->BorderStyle.active;
	    else
		bf = &fl->BorderStyle.inactive;
	    while (isspace(*action)) ++action;
	    if (*action != '(') {
		if (!*action) {
		    fvwm_msg(ERR,"SetBorderStyle",
			     "error in %s border specification", parm);
		    free(parm);
		    return;
		}
		free(parm);
		while (isspace(*action)) ++action;
		if (ReadButtonFace(action, &tmpbf,-1,True)) {
		    FreeButtonFace(dpy, bf);
		    *bf = tmpbf;
		}
		break;
	    }
	    end = strchr(++action, ')');
	    if (!end) {
		fvwm_msg(ERR,"SetBorderStyle",
			 "error in %s border specification", parm);
		free(parm);
		return;
	    }
	    len = end - action + 1;
	    tmp = safemalloc(len);
	    strncpy(tmp, action, len - 1);
	    tmp[len - 1] = 0;
	    ReadButtonFace(tmp, bf,-1,True);
	    free(tmp);
	    action = end + 1;
	}
	else if (strcmp(parm,"--")==0) {
	    if (ReadButtonFace(prev, &fl->BorderStyle.active,-1,True))
		ReadButtonFace(prev, &fl->BorderStyle.inactive,-1,False);
	    free(parm);
	    break;
	} else {
	    ButtonFace tmpbf;
	    tmpbf.style = SimpleButton;
#ifdef MULTISTYLE
	    tmpbf.next = NULL;
#endif
#ifdef MINI_ICONS
	    tmpbf.u.p = NULL;
#endif
	    if (ReadButtonFace(prev, &tmpbf,-1,True)) {
		FreeButtonFace(dpy,&fl->BorderStyle.active);
		fl->BorderStyle.active = tmpbf;
		ReadButtonFace(prev, &fl->BorderStyle.inactive,-1,False);
	    }
	    free(parm);
	    break;
	}
	free(parm);
	prev = action;
	action = GetNextToken(action,&parm);
    }
}
#endif
