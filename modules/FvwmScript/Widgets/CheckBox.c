/* -*-c-*- */
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include "libs/fvwmlib.h"
#include "libs/ColorUtils.h"
#include "libs/Graphics.h"
#include "Tools.h"

/*
 * Fonction pour CheckBox / functions for CheckBox
 */
void InitCheckBox(struct XObj *xobj)
{
  unsigned long mask;
  XSetWindowAttributes Attr;

  /* Enregistrement des couleurs et de la police / fonts and colors */
  if (xobj->colorset >= 0) {
    xobj->TabColor[fore] = Colorset[xobj->colorset].fg;
    xobj->TabColor[back] = Colorset[xobj->colorset].bg;
    xobj->TabColor[hili] = Colorset[xobj->colorset].hilite;
    xobj->TabColor[shad] = Colorset[xobj->colorset].shadow;
  } else {
    xobj->TabColor[fore] = GetColor(xobj->forecolor);
    xobj->TabColor[back] = GetColor(xobj->backcolor);
    xobj->TabColor[hili] = GetColor(xobj->hilicolor);
    xobj->TabColor[shad] = GetColor(xobj->shadcolor);
  }

  mask = 0;
  Attr.background_pixel = xobj->TabColor[back];
  mask |= CWBackPixel;
  Attr.cursor = XCreateFontCursor(dpy,XC_hand2);
  mask |= CWCursor;  /* Curseur pour la fenetre / Cursor for the window */

  xobj->win = XCreateWindow(dpy, *xobj->ParentWin,
		xobj->x, xobj->y, xobj->width, xobj->height,0,
		CopyFromParent, InputOutput, CopyFromParent,
		mask, &Attr);
  xobj->gc = fvwmlib_XCreateGC(dpy,xobj->win,0,NULL);
  XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);

  if ((xobj->Ffont = FlocaleLoadFont(dpy, xobj->font, ScriptName)) == NULL)
  {
    fprintf(stderr, "%s: Couldn't load font. Exiting!\n", ScriptName);
    exit(1);
  }
  if (xobj->Ffont->font != NULL)
    XSetFont(dpy, xobj->gc, xobj->Ffont->font->fid);

  XSetLineAttributes(dpy, xobj->gc, 1, LineSolid, CapRound, JoinMiter);

  /* Redimensionnement du widget / resize the widget */
  xobj->height = xobj->Ffont->height + 5;
  xobj->width =
    FlocaleTextWidth(xobj->Ffont, xobj->title, strlen(xobj->title)) + 30;
  XResizeWindow(dpy, xobj->win, xobj->width, xobj->height);
  if (xobj->colorset >= 0)
    SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
			&Colorset[xobj->colorset], Pdepth,
			xobj->gc, True);
  XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyCheckBox(struct XObj *xobj)
{
  FlocaleUnloadFont(dpy, xobj->Ffont);
  XFreeGC(dpy,xobj->gc);
  XDestroyWindow(dpy,xobj->win);
}

void DrawCheckBox(struct XObj *xobj, XEvent *evp)
{
  XSegment segm[2];

  /* Dessin du rectangle arrondi / drawing of the round rectangle */
  DrawReliefRect(0, xobj->Ffont->ascent - 11, xobj->height, xobj->height,
		 xobj, hili, shad);

  /* Calcul de la position de la chaine de charactere */
  /* Compute the position of string */
  MyDrawString(dpy, xobj, xobj->win, 23, xobj->Ffont->ascent, xobj->title,
	       fore, hili, back, !xobj->flags[1], NULL, evp);
  /* Dessin de la croix / draw the cross */
  if (xobj->value)
  {
    XSetLineAttributes(dpy, xobj->gc, 2, LineSolid, CapProjecting, JoinMiter);
    segm[0].x1 = 5;
    segm[0].y1 = 5;
    segm[0].x2 = xobj->height - 6;
    segm[0].y2 = xobj->height - 6;
    segm[1].x1 = 5;
    segm[1].y1 = xobj->height - 6;
    segm[1].x2 = xobj->height - 6;
    segm[1].y2 = 5;
    XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);
    XDrawSegments(dpy, xobj->win, xobj->gc, segm, 2);
    XSetLineAttributes(dpy, xobj->gc, 1, LineSolid, CapRound, JoinMiter);
  }
}

void EvtMouseCheckBox(struct XObj *xobj, XButtonEvent *EvtButton)
{
  static XEvent event;
  int End = 1;
  unsigned int modif;
  int x1,x2,y1,y2;
  Window Win1,Win2;
  Window WinBut = 0;
  int In = 0;
  XSegment segm[2];

  while (End)
  {
    FNextEvent(dpy, &event);
    switch (event.type)
    {
    case EnterNotify:
      FQueryPointer(dpy, *xobj->ParentWin,
		    &Win1, &Win2, &x1, &y1, &x2, &y2, &modif);
      if (WinBut == 0)
      {
	WinBut = Win2;
	/* Mouse on button */
	DrawReliefRect(0, xobj->Ffont->ascent - 11, xobj->height,
		       xobj->height, xobj, shad, hili);
	In = 1;
      }
      else
      {
	if (Win2 == WinBut)
	{
	  /* Mouse on button */
	  DrawReliefRect(0, xobj->Ffont->ascent - 11, xobj->height, xobj->height,
			 xobj, shad, hili);
	  In = 1;
	}
	else if (In)
	{
	  In = 0;
	  /* Mouse not on button */
	  DrawReliefRect(0, xobj->Ffont->ascent - 11, xobj->height, xobj->height,
			 xobj, hili, shad);
	}
      }
      break;
    case LeaveNotify:
      FQueryPointer(dpy, *xobj->ParentWin,
		    &Win1, &Win2, &x1, &y1, &x2, &y2, &modif);
      if (Win2 == WinBut)
      {
	In = 1;
	/* Mouse on button */
	DrawReliefRect(0, xobj->Ffont->ascent - 11, xobj->height, xobj->height,
		       xobj, shad, hili);
      }
      else if (In)
      {
	/* Mouse not on button */
	DrawReliefRect(0, xobj->Ffont->ascent - 11, xobj->height, xobj->height,
		       xobj, hili, shad);
	In = 0;
      }
      break;
    case ButtonRelease:
      End = 0;
      /* Mouse not on button */
      if (In)
      {
	/* Envoie d'un message vide de type SingleClic pour un clique souris */
	xobj->value = !xobj->value;
	DrawReliefRect(0, xobj->Ffont->ascent -11, xobj->height, xobj->height,
		       xobj, hili, shad);
	SendMsg(xobj,SingleClic);
      }
      if (xobj->value)
      {
	XSetLineAttributes(dpy, xobj->gc, 2,
			   LineSolid, CapProjecting, JoinMiter);
	segm[0].x1 = 5;
	segm[0].y1 = 5;
	segm[0].x2 = xobj->height - 6;
	segm[0].y2 = xobj->height - 6;
	segm[1].x1 = 5;
	segm[1].y1 = xobj->height - 6;
	segm[1].x2 = xobj->height - 6;
	segm[1].y2 = 5;
	XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);
	XDrawSegments(dpy, xobj->win, xobj->gc, segm, 2);
	XSetLineAttributes(dpy, xobj->gc, 1, LineSolid, CapRound, JoinMiter);
      }
      else
      {
	XClearArea(dpy, xobj->win, 4, 4, xobj->height-8, xobj->height-8, False);
      }
      break;
    }
  }
}

void EvtKeyCheckBox(struct XObj *xobj, XKeyEvent *EvtKey)
{
  KeySym ks;
  unsigned char buf[10];
  XSegment segm[2];

  XLookupString(EvtKey, (char *)buf, sizeof(buf), &ks, NULL);
  if (ks == XK_Return) {
    xobj->value = !xobj->value;
    DrawReliefRect(0, xobj->Ffont->ascent - 11, xobj->height,
		   xobj->height, xobj, hili, shad);
    SendMsg(xobj,SingleClic);
    if (xobj->value)
    {
      XSetLineAttributes(dpy, xobj->gc, 2,
			 LineSolid, CapProjecting, JoinMiter);
      segm[0].x1 = 5;
      segm[0].y1 = 5;
      segm[0].x2 = xobj->height - 6;
      segm[0].y2 = xobj->height - 6;
      segm[1].x1 = 5;
      segm[1].y1 = xobj->height - 6;
      segm[1].x2 = xobj->height - 6;
      segm[1].y2 = 5;
      XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);
      XDrawSegments(dpy, xobj->win, xobj->gc, segm, 2);
      XSetLineAttributes(dpy, xobj->gc, 1, LineSolid, CapRound, JoinMiter);
    }
    else
    {
      XClearArea(dpy, xobj->win, 4, 4, xobj->height-8, xobj->height-8, False);
    }
  }
}


void ProcessMsgCheckBox(struct XObj *xobj, unsigned long type, unsigned long *body)
{
}
