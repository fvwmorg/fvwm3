/* -*-c-*- */
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
#include "libs/Rectangles.h"
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
  if (b->parent->c->flags&b_Colorset || b->parent->c->flags&b_ColorsetParent)
    b->flags|=b_ColorsetParent;

  Ffont = buttonFont(b);

  GetInternalSize(b,&ix,&iy,&iw,&ih);

  /* At this point iw,ih,ix and iy should be correct. Now all we have to do is
     place title and iconwin in their proper positions */

  /* For now, use the old routine in icons.h for buttons with icons */
  if(b->flags&b_Icon && !(b->flags&b_IconAlpha))
  {
    ConfigureIconWindow(b, NULL);
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
***
*** draw can be:
*** DRAW_RELIEF: draw only the relief
*** DRAW_CLEAN:  draw the relief, the foreground bg if any and if this
*** the case draw the title and the icon if b_IconAlpha. This is safe
*** but the button background may be not repaint (if the bg of the button
*** come from a parent button).
*** DRAW_ALL: as above but draw the title and the icon if b_IconAlpha in
*** any case. WARRNING: the title and the icon (b_IconAlpha) must be cleaned:
*** if the button has a bg this is the case, but if the bg of the button
*** come from a parent button this is not the case and xft title and alpha
*** icons will be not draw correctly.
*** So DRAW_ALL is ok only when you draw buttons recursively.
*** DRAW_FORCE: draw the button and its parent fg bg. Use this only if
*** you need to draw an individual button (not in a recursive drawing)
***
*** If pev is != NULL the expose member give to use the rectangle on
*** which we want to draw.
***
**/
void RedrawButton(button_info *b, int draw, XEvent *pev)
{
	int i,j,k,BH,BW;
	int f,of,x,y,px,py;
	int ix,iy,iw,ih;
	FlocaleFont *Ffont=buttonFont(b);
	XGCValues gcv;
	int rev = 0;
	int rev_xor = 0;
	Pixel fc;
	Pixel bc;
	Pixel hc;
	Pixel sc;
	int cset;
	XRectangle clip;
	Bool clean = False;
	Bool cleaned = False;
	Bool clear_bg = False;

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

	/* This probably isn't the place for this, but it seems to work here
	 * and not elsewhere, so... */
	if((b->flags & b_Swallow) && (buttonSwallowCount(b)==3) &&
	   b->IconWin!=None)
	{
		XSetWindowBorderWidth(Dpy,b->IconWin,0);
	}

	if (draw == DRAW_FORCE)
	{
		button_info *tmpb;

		if (buttonBackgroundButton(b, &tmpb))
		{
			/* found the button which holds the bg */
			if (b == tmpb)
			{
				/* ok the button will be cleaned now */
				clean = True;
			}
			else
			{
				XEvent e;

				cleaned = True;
				e.xexpose.x = ix;
				e.xexpose.y = iy;
				e.xexpose.width = iw;
				e.xexpose.height = ih;

				RedrawButton(tmpb, DRAW_CLEAN, &e);
			}
		}
		else
		{
			/* bg of the button is the true bg */
			clean = True;
			clear_bg = True;
		}
	}

	/* ------------------------------------------------------------------ */

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

	if (b->flags&b_Action)
	{
		/* If this is a Desk button that takes you to here.. */
		int n;

		for (n = 0; n < 4; n++)
		{
			if (b->action[n])
			{
				if (strncasecmp(
					b->action[n], "GotoDeskAndPage", 15)
				    == 0)
				{
					k = sscanf(&b->action[n][15], "%d", &i);
					if (k == 1 && i == new_desk)
					{
						rev=1;
					}
					break;
				}
				else if (strncasecmp(
					b->action[n], "Desk", 4) == 0)
				{
					k = sscanf(
						&b->action[n][4], "%d%d", &i,
						&j);
					if (k == 2 && i == 0 && j == new_desk)
					{
						rev=1;
					}
					break;
				}
				else if (strncasecmp(
					b->action[n], "GotoDesk", 8) == 0)
				{
					k = sscanf(
						&b->action[n][8], "%d%d", &i,
						&j);
					if (k == 2 && i == 0 && j == new_desk)
					{
						rev=1;
					}
					break;
				}
			}
		}
	}

	/* ------------------------------------------------------------------ */
	of = f;
	f=abs(f);

	if (draw == DRAW_CLEAN)
	{
		clean = True;
	}
	else if (draw == DRAW_ALL)
	{
		/* this means that the button is clean or it will be
		 * cleaned now */
		cleaned = True;
		clean = True;
	}

	if(clean && BW>2*f && BH>2*f)
	{
		Bool do_draw = True;

		gcv.foreground = bc;
		XChangeGC(Dpy,NormalGC,GCForeground,&gcv);
		clip.x = x+f;
		clip.y = y+f;
		clip.width = BW-2*f;
		clip.height = BH-2*f;

		do_draw = (!pev ||
			   frect_get_intersection(
				   x+f, y+f, BW-2*f, BH-2*f,
				   pev->xexpose.x, pev->xexpose.y,
				   pev->xexpose.width, pev->xexpose.height,
				   &clip));

		if(b->flags&b_Container)
		{
			if (b->c->flags & b_Colorset)
			{
				if (do_draw)
				{
					cleaned = True;
					SetClippedRectangleBackground(
						Dpy, MyWindow,
						x+f, y+f, BW-2*f, BH-2*f, &clip,
						&Colorset[b->c->colorset],
						Pdepth, NormalGC);
				}
			}
			else if (!(b->c->flags&b_IconParent) &&
				 !(b->c->flags&b_ColorsetParent))
			{
				int x1 = x + f;
				int y1 = y + f;
				int w1 = px;
				int h1 = py;
				int w2 = w1;
				int h2 = h1;
				int w = BW - 2 * f;
				int h = BH - 2 * f;
				int lx = x1, ly = y1, lh = 0, lw = 0;

				w2 += iw - b->c->width;
				h2 += ih - b->c->height;

				if(w1)
				{
					lw = w1;
					lh = h;
				}
				if(w2)
				{
					lx = x1+w-w2;
					lw = w2;
					lh = h;
				}
				if(h1)
				{
					lw = w;
					lh = h1;
				}
				if(h2)
				{
					ly = y1+h-h2;
					lw = w;
					lh = h2;
				}
				clip.x = lx;
				clip.y = ly;
				clip.width = lw;
				clip.height = lh;
				if ((lw>0 && lh >0) &&
				    (!pev || frect_get_intersection(
					lx, ly, lw, lh,
					pev->xexpose.x, pev->xexpose.y,
					pev->xexpose.width, pev->xexpose.height,
					&clip)))
				{
					XFillRectangle(
						Dpy,MyWindow,NormalGC,
						clip.x, clip.y,
						clip.width,clip.height);
				}
			}
		} /* container */
		else if (buttonSwallowCount(b) == 3 && (b->flags & b_Swallow) &&
			 b->flags&b_Colorset)
		{
			/* Set the back color of the buttons for shaped apps
			 * (olicha 00-03-09) and also for transparent modules */
			if (do_draw)
			{
				cleaned = True;
				SetClippedRectangleBackground(
					Dpy, MyWindow, x+f, y+f, BW-2*f, BH-2*f,
					&clip, &Colorset[b->colorset],
					Pdepth, NormalGC);
			}
			if (!(b->flags & b_FvwmModule))
			{
				change_swallowed_window_colorset(b, False);
			}
			else
			{
				/* module */
			}
		}
		else if (b->flags & b_Colorset)
		{
			if (do_draw)
			{
				cleaned = True;
				SetClippedRectangleBackground(
					Dpy, MyWindow, x+f, y+f, BW-2*f, BH-2*f,
					&clip,
					&Colorset[b->colorset],
					Pdepth,
					NormalGC);
			}
		}
		else if (!(b->flags&b_IconBack) && !(b->flags&b_IconParent) &&
			 !(b->flags&b_Swallow) && !(b->flags&b_ColorsetParent))
		{
			if (do_draw)
			{
				cleaned = True;
				XFillRectangle(
					Dpy,MyWindow,NormalGC,
					clip.x,clip.y,clip.width,clip.height);
			}
		}
		else if (clear_bg ||
			 (pev && !buttonBackgroundButton(b,NULL) &&
			  ((b->flags&b_Title && Ffont && Ffont->fftf.fftfont) ||
			   (b->flags&b_Icon && b->flags&b_IconAlpha))))
		{
			/* some times we need to clear the real bg.
			 * The pev expose rectangle can be bigger than the real
			 * exposed part (as we rectangle flush and pev can
			 * be a fake event) so we need to clear with xft font
			 * and icons with alpha */
			if (do_draw)
			{
				cleaned = True;
				XClearArea(
					Dpy, MyWindow, clip.x, clip.y,
					clip.width, clip.height, False);
			}
		}
	}

	/* ------------------------------------------------------------------ */

	if(cleaned && (b->flags&b_Title))
	{
		DrawTitle(b,MyWindow,NormalGC,pev,False);
	}

	if (!(b->flags&b_Title) && (b->flags & b_Panel) &&
	    (b->panel_flags.panel_indicator))
	{
		XGCValues gcv;
		int ix, iy, iw, ih;
		int is;

		/* draw the panel indicator, but not if there is a title */
		/* FIXEME: and if we have an icon */
		if (b->slide_direction != SLIDE_GEOMETRY &&
		    b->indicator_size != 0 && (b->indicator_size & 1) == 0)
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
				Dpy, MyWindow, ix, iy, iw - 1, ih - 1,
				NormalGC, ShadowGC, 1);
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
				Dpy, MyWindow, NormalGC, ShadowGC, None,
				ix, iy, iw, ih, 0, dir, 1, 0, 0);
		}
	} /* panel indicator */

	/* redraw icons with alpha because there are drawn on the foreground */
	if(cleaned && (b->flags&b_Icon) && (b->flags & b_IconAlpha))
	{
		DrawForegroundIcon(b, pev);
	}

	/* relief */
	RelieveButton(MyWindow,of,x,y,BW,BH,hc,sc,rev ^ rev_xor);
}

/**
*** Writes out title.
**/
void DrawTitle(
	button_info *b,Window win,GC gc, XEvent *pev, Bool do_not_modify_fg)
{
	int BH;
	int ix,iy,iw,ih;
	FlocaleFont *Ffont=buttonFont(b);
	int justify=buttonJustify(b);
	int l,i,xpos;
	char *s;
	int just=justify&b_TitleHoriz; /* Left, center, right */
	XGCValues gcv;
	unsigned long gcm;
	int cset;
	XRectangle clip;
	Region region = None;

	BH = buttonHeight(b);

	GetInternalSize(b,&ix,&iy,&iw,&ih);

	/* ------------------------------------------------------------------ */

	if(!(b->flags&b_Title) || !Ffont)
	{
		return;
	}

	cset = buttonColorset(b);
	gcm = 0;
	if (do_not_modify_fg == False)
	{
		gcm |= GCForeground;
		if (cset >= 0)
		{
			gcv.foreground = Colorset[cset].fg;
		}
		else
		{
			gcv.foreground = buttonFore(b);
		}
	}
	if (Ffont->font)
	{
		gcv.font = Ffont->font->fid;
		gcm |= GCFont;
	}
	XChangeGC(Dpy,gc,gcm,&gcv);

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
			{
				i=FlocaleTextWidth(Ffont,++s,--l);
			}
		}
		else /* Left or center - cut off its tail */
		{
			while(i>iw && l>0)
			{
				i=FlocaleTextWidth(Ffont,s,--l);
			}
		}
	}
	if(just==0 || ((justify&b_Horizontal) && (b->flags&b_Right))) /* Left */
	{
		xpos=ix;
	}
	else if(just==2) /* Right */
	{
		xpos=max(ix,ix+iw-i);
	}
	else /* Centered, I guess */
	{
		xpos=ix+(iw-i)/2;
	}

	if(*s && l>0 && BH>=Ffont->height) /* Clip it somehow? */
	{
		FlocaleWinString FwinString;
		int cset;

		memset(&FwinString, 0, sizeof(FwinString));
		FwinString.str = s;
		FwinString.win = win;
		FwinString.gc = gc;
		cset = buttonColorset(b);
		if (cset >= 0)
		{
			FwinString.colorset = &Colorset[cset];
			FwinString.flags.has_colorset = 1;
		}
		FwinString.x = xpos;
		/* If there is more than the title, put it at the bottom */
		/* Unless stack flag is set, put it to the right of icon */
		if((b->flags&b_Icon ||
		    ((buttonSwallowCount(b)==3) && (b->flags&b_Swallow))) &&
		   !(justify&b_Horizontal))
		{
			FwinString.y = iy+ih-Ffont->descent;
			/* Shrink the space available for icon/window */
			ih-=Ffont->height;
		}
		/* Or else center vertically */
		else
		{
			FwinString.y =
				iy + (ih+ Ffont->ascent - Ffont->descent)/2;
		}

		clip.x = FwinString.x;
		clip.y = FwinString.y - Ffont->ascent;
		clip.width = i;
		clip.height = Ffont->height;
		if (pev)
		{
			if (!frect_get_intersection(
				FwinString.x, FwinString.y - Ffont->ascent,
				i, Ffont->height,
				pev->xexpose.x, pev->xexpose.y,
				pev->xexpose.width, pev->xexpose.height,
				&clip))
			{
				return;
			}
		}
		XSetClipRectangles(
			Dpy, FwinString.gc, 0, 0, &clip, 1, Unsorted);
		region = XCreateRegion();
		XUnionRectWithRegion (&clip, region, region);
		FwinString.flags.has_clip_region = True;
		FwinString.clip_region = region;
		if (0 && Ffont->fftf.fftfont != NULL)
		{
			XClearArea(
				Dpy, win,
				clip.x, clip.y, clip.width, clip.height,
				False);
		}
		FlocaleDrawString(Dpy, Ffont, &FwinString, 0);
		XSetClipMask(Dpy, FwinString.gc, None);
		if (region)
		{
			XDestroyRegion(region);
		}
	}
}
