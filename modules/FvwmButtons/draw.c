/*
 * Fvwmbuttons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
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

/* ------------------------------- includes -------------------------------- */

#include "config.h"

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "libs/Colorset.h"
#include "libs/fvwmlib.h"
#include "FvwmButtons.h"
#include "misc.h" /* ConstrainSize() */
#include "icons.h" /* ConfigureIconWindow() */
#include "button.h"
#include "draw.h"

/* ---------------- Functions that design and draw buttons ----------------- */

/**
*** RelieveButton()
*** Draws the relief pattern around a window.
**/
void RelieveButton(Window wn,int width,int x,int y,int w,int h,Pixel relief,
		   Pixel shadow,int rev)
{
  XGCValues gcv;
  GC swapGC, reliefGC, shadowGC;

  if(!width)
    return;

  gcv.foreground=relief;
  XChangeGC(Dpy,NormalGC,GCForeground,&gcv);
  reliefGC=NormalGC;

  gcv.foreground=shadow;
  XChangeGC(Dpy,ShadowGC,GCForeground,&gcv);
  shadowGC=ShadowGC;

  if(width<0)
  {
    width=-width;
    rev = !rev;
  }
  if(rev)
  {
    swapGC=reliefGC;
    reliefGC=shadowGC;
    shadowGC=swapGC;
  }

  RelieveRectangle(Dpy,wn,x,y,w-1,h-1,reliefGC,shadowGC,width);
}

/**
*** MakeButton()
*** To position subwindows in a button: icons and swallowed windows.
**/
void MakeButton(button_info *b)
{
  /* This is resposible for drawing the contents of a button, placing the
   * icon and/or swallowed item in the correct position inside potential
   * padding or frame.
   */
  int ih,iw,ix,iy;
  FlocaleFont *Ffont;

  if(!b)
  {
    fprintf(stderr,"%s: BUG: MakeButton called with NULL pointer\n",MyName);
    exit(2);
  }
  if(b->flags&b_Container)
  {
    fprintf(stderr,"%s: BUG: MakeButton called with container\n",MyName);
    exit(2);
  }

  if ((b->flags & b_Swallow) && !(b->flags & b_Icon) &&
      buttonSwallowCount(b) < 3)
  {
    return;
  }

  /* Check if parent container has an icon as background */
  if (b->parent->c->flags&b_IconBack || b->parent->c->flags&b_IconParent)
    b->flags|=b_IconParent;

  /* Check if parent container has a colorset definition as background */
  if (b->parent->c->flags&b_Colorset)
    b->flags|=b_ColorsetParent;

  Ffont = buttonFont(b);

  GetInternalSize(b,&ix,&iy,&iw,&ih);

  /* At this point iw,ih,ix and iy should be correct. Now all we have to do is
     place title and iconwin in their proper positions */

  /* For now, use the old routine in icons.h for buttons with icons */
  if(b->flags&b_Icon)
  {
    ConfigureIconWindow(b);
  }
  /* For now, hardcoded window centered, title bottom centered, below window */
  if(buttonSwallowCount(b)==3 && (b->flags & b_Swallow))
  {
    long supplied;
    if(!b->IconWin)
    {
      fprintf(stderr,"%s: BUG: Swallowed window has no IconWin\n",MyName);
      exit(2);
    }

    if (b->flags&b_Title && Ffont && !(buttonJustify(b)&b_Horizontal))
      ih -= Ffont->height;

    b->icon_w=iw;
    b->icon_h=ih;

    if (iw>0 && ih>0)
    {
      if (!(buttonSwallow(b)&b_NoHints))
      {
	if(!XGetWMNormalHints(Dpy,b->IconWin,b->hints,&supplied))
	  b->hints->flags=0;
	ConstrainSize(b->hints, &b->icon_w, &b->icon_h);
      }
      if (b->flags & b_Right)
	ix += iw-b->icon_w;
      else if (!(b->flags & b_Left))
	ix += (iw-b->icon_w)/2;
      XMoveResizeWindow(Dpy,b->IconWin,ix,iy+(ih-b->icon_h)/2,
			b->icon_w,b->icon_h);
    }
    else
    {
      XMoveWindow(Dpy,b->IconWin, -32768, -32768);
    }
  }
}

/**
*** RedrawButton()
*** Writes out title, if any, and displays the bevel right, by calling
*** RelieveButton. If clean is nonzero, also clears background. If clean is 2,
*** forces clearing the background (used in case a panel has lost its window).
**/
void RedrawButton(button_info *b,int clean)
{
  int i,j,k,BH,BW;
  int f,x,y,px,py;
  int ix,iy,iw,ih;
  FlocaleFont *Ffont = buttonFont(b);
  XGCValues gcv;
  unsigned long gcm;
  int rev = 0;
  int rev_xor = 0;
  Pixel fc;
  Pixel bc;
  Pixel hc;
  Pixel sc;
  int cset;

  cset = buttonColorset(b);
  if (cset >= 0)
  {
    fc = Colorset[cset].fg;
    bc = Colorset[cset].bg;
    hc = Colorset[cset].hilite;
    sc = Colorset[cset].shadow;
  }
  else
  {
    fc = buttonFore(b);
    bc = buttonBack(b);
    hc = buttonHilite(b);
    sc = buttonShadow(b);
  }

  BW = buttonWidth(b);
  BH = buttonHeight(b);
  buttonInfo(b,&x,&y,&px,&py,&f);
  GetInternalSize(b,&ix,&iy,&iw,&ih);

  /* This probably isn't the place for this, but it seems to work here and not
     elsewhere, so... */
  if((b->flags & b_Swallow) && (buttonSwallowCount(b)==3) && b->IconWin!=None)
    XSetWindowBorderWidth(Dpy,b->IconWin,0);

  /* ----------------------------------------------------------------------- */

  if (b->flags & b_Panel)
  {
    if ((b->newflags.panel_mapped))
    {
      rev = 1;
    }
    if (b == CurrentButton && is_pointer_in_current_button)
    {
      rev_xor = 1;
    }
  }
  else if ((b->flags & b_Hangon) ||
	   (b == CurrentButton && is_pointer_in_current_button))
  {
    /* Hanging swallow or held down by user */
    rev = 1;
  }
  if ((b->flags & b_Panel) && !(b->flags&b_Title))
  {
    clean = 1;
    XClearArea(Dpy, MyWindow, ix, iy, iw, ih, False);
  }
  if (b->flags&b_Action) /* If this is a Desk button that takes you to here.. */
  {
    int n;

    for (n = 0; n < 4; n++)
    {
      if (b->action[n])
      {
	if (strncasecmp(b->action[n], "GotoDeskAndPage", 15) == 0)
	{
	  k = sscanf(&b->action[n][15], "%d", &i);
	  if (k == 1 && i == new_desk)
	  {
	    rev=1;
	  }
	  break;
	}
	else if (strncasecmp(b->action[n], "Desk", 4) == 0)
	{
	  k = sscanf(&b->action[n][4], "%d%d", &i, &j);
	  if (k == 2 && i == 0 && j == new_desk)
	  {
	    rev=1;
	  }
	  break;
	}
	else if (strncasecmp(b->action[n], "GotoDesk", 8) == 0)
	{
	  k = sscanf(&b->action[n][8], "%d%d", &i, &j);
	  if (k == 2 && i == 0 && j == new_desk)
	  {
	    rev=1;
	  }
	  break;
	}
      }
    }
  }
  if (FftSupport && (b->flags&b_Title) && Ffont && Ffont->fftf.fftfont != NULL)
  {
    XClearArea(Dpy, MyWindow, ix, iy, iw, ih, False);
    clean = 1;
  }
  if(b->flags&b_Icon && b->icon->alpha != None)
  {
    ConfigureIconWindow(b);
  }

  RelieveButton(MyWindow,f,x,y,BW,BH,hc,sc,rev ^ rev_xor);

  /* ----------------------------------------------------------------------- */

  f=abs(f);

  if(clean && BW>2*f && BH>2*f)
  {
    gcv.foreground = bc;
    XChangeGC(Dpy,NormalGC,GCForeground,&gcv);

    if(b->flags&b_Container)
    {
      if (b->c->flags & b_Colorset)
      {
	SetRectangleBackground(
	  Dpy, MyWindow, ix, iy, iw, ih,
	  &Colorset[b->c->colorset], Pdepth, NormalGC);
      }
      else
      {
	int x1 = x + f;
	int y1 = y + f;
	int w1 = px;
	int h1 = py;
	int w2 = w1;
	int h2 = h1;
	int w = BW - 2 * f;
	int h = BH - 2 * f;

	w2 += iw - b->c->width;
	h2 += ih - b->c->height;

	if(w1)
	  XFillRectangle(Dpy,MyWindow,NormalGC,x1,y1,w1,h);
	if(w2)
	  XFillRectangle(Dpy,MyWindow,NormalGC,x1+w-w2,y1,w2,h);
	if(h1)
	  XFillRectangle(Dpy,MyWindow,NormalGC,x1,y1,w,h1);
	if(h2)
	  XFillRectangle(Dpy,MyWindow,NormalGC,x1,y1+h-h2,w,h2);
      }
    } /* container */
    else if (buttonSwallowCount(b) == 3 && (b->flags & b_Swallow) &&
	     b->flags & b_Colorset)
    {
      /* Set the back color of the buttons for shaped apps (olicha 00-03-09)
       * and also for transparent modules */
      SetRectangleBackground(
	Dpy, MyWindow, x+f, y+f, BW-2*f, BH-2*f, &Colorset[b->colorset],
	Pdepth, NormalGC);
      if (!(b->flags & b_FvwmModule))
	change_swallowed_window_colorset(b, False);
    }
    else if (b->flags & b_Colorset)
    {
      SetRectangleBackground(
	Dpy, MyWindow, x+f, y+f, BW-2*f, BH-2*f, &Colorset[b->colorset],
	Pdepth, NormalGC);
    }
    else if (!(b->flags&b_IconBack) && !(b->flags&b_IconParent) &&
	     !(b->flags&b_Swallow) && !(b->flags&b_ColorsetParent))
    {
      XFillRectangle(Dpy,MyWindow,NormalGC,x+f,y+f,BW-2*f,BH-2*f);
    }
    else if (clean == 2)
    {
      XClearArea(Dpy, MyWindow, x+f, y+f, BW-2*f, BH-2*f, False);
    }
  }

  /* ----------------------------------------------------------------------- */

  if(b->flags&b_Title)
  {
    if (Ffont)
    {
      gcv.foreground = fc;
      gcm = GCForeground;
      if (Ffont->font)
      {
	gcv.font = Ffont->font->fid;
	gcm |= GCFont;
      }
      XChangeGC(Dpy,NormalGC,gcm,&gcv);
      DrawTitle(b,MyWindow,NormalGC);
    }
  } /* title */
  else if ((b->flags & b_Panel) && (b->panel_flags.panel_indicator))
  {
    XGCValues gcv;
    int ix, iy, iw, ih;
    int is;

    /* draw the panel indicator, but not if there is a title */
    if (b->slide_direction != SLIDE_GEOMETRY && b->indicator_size != 0 &&
        (b->indicator_size & 1) == 0)
    {
      /* make sure we have an odd number */
      b->indicator_size--;
    }
    is = b->indicator_size;

    gcv.foreground = hc;
    XChangeGC(Dpy,NormalGC,GCForeground,&gcv);
    gcv.foreground = sc;
    XChangeGC(Dpy,ShadowGC,GCForeground,&gcv);

    GetInternalSize(b, &ix, &iy, &iw, &ih);
    if (is != 0)
    {
      /* limit to user specified size */
      if (is < iw)
      {
	ix += (iw - is) / 2;
	iw = is;
      }
      if (is < ih)
      {
	iy += (ih - is) / 2;
	ih = is;
      }
    }

    if (b->slide_direction == SLIDE_GEOMETRY)
    {
      if (ih < iw)
      {
	ix += (iw - ih) / 2;
	iw = ih;
      }
      else if (iw < ih)
      {
	iy += (ih - iw) / 2;
	ih = iw;
      }
      RelieveRectangle(
	Dpy, MyWindow, ix, iy, iw - 1, ih - 1, NormalGC, ShadowGC, 1);
    }
    else
    {
      char dir = b->slide_direction;

      if (rev)
      {
	switch (dir)
	{
	case SLIDE_UP:
	  dir = SLIDE_DOWN;
	  break;
	case SLIDE_DOWN:
	  dir = SLIDE_UP;
	  break;
	case SLIDE_LEFT:
	  dir = SLIDE_RIGHT;
	  break;
	case SLIDE_RIGHT:
	  dir = SLIDE_LEFT;
	  break;
	}
      }
      DrawTrianglePattern(
	Dpy, MyWindow, NormalGC, ShadowGC, None, ix, iy, iw, ih, 0, dir, 1, 0,
	0);
    }
  } /* panel indicator */
}

/**
*** DrawTitle()
*** Writes out title.
**/
void DrawTitle(button_info *b,Window win,GC gc)
{
  int BH;
  int ix,iy,iw,ih;
  FlocaleFont *Ffont=buttonFont(b);
  int justify=buttonJustify(b);
  int l,i,xpos;
  char *s;
  int just=justify&b_TitleHoriz; /* Left, center, right */

  BH = buttonHeight(b);

  GetInternalSize(b,&ix,&iy,&iw,&ih);

  /* ----------------------------------------------------------------------- */

  if(!(b->flags&b_Title) || !Ffont)
    return;

  /* If a title is to be shown, truncate it until it fits */
  if(justify&b_Horizontal && !(b->flags & b_Right))
  {
    if(b->flags&b_Icon)
    {
      ix+=b->icon->width+buttonXPad(b);
      iw-=b->icon->width+buttonXPad(b);
    }
    else if ((b->flags & b_Swallow) && buttonSwallowCount(b)==3)
    {
      ix += b->icon_w+buttonXPad(b);
      iw -= b->icon_w+buttonXPad(b);
    }
  }

  s = b->title;
  l = strlen(s);
  i = FlocaleTextWidth(Ffont,s,l);

  if(i>iw)
  {
    if(just==2)
    {
      while(i>iw && *s)
	i=FlocaleTextWidth(Ffont,++s,--l);
    }
    else /* Left or center - cut off its tail */
    {
      while(i>iw && l>0)
	i=FlocaleTextWidth(Ffont,s,--l);
    }
  }
  if(just==0 || ((justify&b_Horizontal) && (b->flags&b_Right))) /* Left */
    xpos=ix;
  else if(just==2) /* Right */
    xpos=max(ix,ix+iw-i);
  else /* Centered, I guess */
    xpos=ix+(iw-i)/2;

  if(*s && l>0 && BH>=Ffont->height) /* Clip it somehow? */
  {
    FlocaleWinString *FwinString;

    FlocaleAllocateWinString(&FwinString);
    FwinString->str = s;
    FwinString->win = win;
    FwinString->gc = gc;
    FwinString->x = xpos;
    /* If there is more than the title, put it at the bottom */
    /* Unless stack flag is set, put it to the right of icon */
    if((b->flags&b_Icon ||
	((buttonSwallowCount(b)==3) && (b->flags&b_Swallow))) &&
       !(justify&b_Horizontal))
    {
      FwinString->y = iy+ih-Ffont->descent;
      /* Shrink the space available for icon/window */
      ih-=Ffont->height;
    }
    /* Or else center vertically */
    else
    {
      FwinString->y = iy + (ih+ Ffont->ascent - Ffont->descent)/2;
    }
    FlocaleDrawString(Dpy, Ffont, FwinString, 0);
  }
}
