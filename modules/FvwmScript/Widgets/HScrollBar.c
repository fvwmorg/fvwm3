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
 * Fonction pour HScrollBar
 */
void DrawThumbH(struct XObj *xobj, XEvent *evp)
{
  int x,y,w,h;
  XSegment segm;
  char str[20];

  x = 2 +
    (xobj->width - 36)*(xobj->value - xobj->value2) /
    (xobj->value3 - xobj->value2);
  y = xobj->height/2 - 11;
  w = 32;
  h = 22;
  DrawReliefRect(x, y, w, h, xobj, hili, shad);
  segm.x1 = x + 15;
  segm.y1 = y + 3;
  segm.x2 = x + 15;
  segm.y2 = y +h - 3;
  XSetForeground(dpy, xobj->gc, xobj->TabColor[shad]);
  XDrawSegments(dpy, xobj->win, xobj->gc, &segm, 1);
  segm.x1 = x + 16;
  segm.y1 = y + 3;
  segm.x2 = x+16;
  segm.y2 = y + h-3;
  XSetForeground(dpy, xobj->gc, xobj->TabColor[hili]);
  XDrawSegments(dpy, xobj->win, xobj->gc, &segm, 1);
  XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);

  sprintf(str, "%d", xobj->value);
  x = x + 15 - (FlocaleTextWidth(xobj->Ffont, str, strlen(str)) / 2);
  y = y - xobj->Ffont->descent - 4;
  MyDrawString(dpy, xobj, xobj->win, x, y, str, fore, hili, back,
	     !xobj->flags[1], NULL, evp);
}

void HideThumbH(struct XObj *xobj)
{
  int x,y;

  x = 2 +
    (xobj->width - 36) * (xobj->value - xobj->value2) /
    (xobj->value3-xobj->value2);
  y = xobj->height/2 - 11;
  XClearArea(dpy, xobj->win, x, y, 32, 22, False);
  /* clear "text" */
  XClearArea(dpy, xobj->win, 0, 0, xobj->width, xobj->height/2-15, False);
}

void InitHScrollBar(struct XObj *xobj)
{
  unsigned long mask;
  XSetWindowAttributes Attr;
  int i;
  char str[20];

  /* Enregistrement des couleurs et de la police */
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
  mask |= CWCursor;             /* Curseur pour la fenetre */

  xobj->win = XCreateWindow(dpy, *xobj->ParentWin,
			    xobj->x, xobj->y, xobj->width, xobj->height, 0,
			    CopyFromParent, InputOutput, CopyFromParent,
			    mask, &Attr);
  xobj->gc = fvwmlib_XCreateGC(dpy, xobj->win, 0, NULL);
  XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);
  XSetBackground(dpy, xobj->gc, xobj->TabColor[back]);

  if ((xobj->Ffont = FlocaleLoadFont(dpy, xobj->font, ScriptName)) == NULL)
  {
    fprintf(stderr, "%s: Couldn't load font. Exiting!\n", ScriptName);
    exit(1);
  }
  if (xobj->Ffont->font != NULL)
    XSetFont(dpy, xobj->gc, xobj->Ffont->font->fid);

  XSetLineAttributes(dpy, xobj->gc, 1, LineSolid, CapRound, JoinMiter);

  if ((xobj->value3 - xobj->value2) <= 0)
    xobj->value3 = xobj->value2 + 10;
  if (!((xobj->value >= xobj->value2) && (xobj->value <= xobj->value3)))
    xobj->value = xobj->value2;
  xobj->height = xobj->Ffont->height * 2 + 30;
  sprintf(str, "%d", xobj->value2);
  i = FlocaleTextWidth(xobj->Ffont, str, strlen(str));
  sprintf(str, "%d", xobj->value3);
  i = FlocaleTextWidth(xobj->Ffont, str, strlen(str)) + i + 20;
  if (xobj->width<i)
    xobj->width = i;
  XResizeWindow(dpy, xobj->win, xobj->width, xobj->height);
  if (xobj->colorset >= 0)
    SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
			&Colorset[xobj->colorset], Pdepth,
			xobj->gc, True);
  XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyHScrollBar(struct XObj *xobj)
{
  FlocaleUnloadFont(dpy,xobj->Ffont);
  XFreeGC(dpy,xobj->gc);
  XDestroyWindow(dpy,xobj->win);
}

void DrawHScrollBar(struct XObj *xobj, XEvent *evp)
{
  int x,y,w,h;
  char str[20];

  /* Calcul de la taille de l'ascenseur */
  x = 0;
  y = xobj->height/2-13;
  w = xobj->width;
  h = 26;
  DrawThumbH(xobj, evp);
  DrawReliefRect(x, y, w, h, xobj, shad, hili);
  /* Ecriture des valeurs */
  sprintf(str, "%d", xobj->value2);
  x = 4;
  y = y + xobj->Ffont->ascent + h;
  MyDrawString(dpy, xobj, xobj->win, x, y, str, fore, hili, back,
	       !xobj->flags[1], NULL, evp);
  sprintf(str, "%d", xobj->value3);
  x = w - FlocaleTextWidth(xobj->Ffont, str, strlen(str)) - 4;
  MyDrawString(dpy, xobj, xobj->win, x, y, str, fore, hili, back,
	       !xobj->flags[1], NULL, evp);
}

void EvtMouseHScrollBar(struct XObj *xobj, XButtonEvent *EvtButton)
{
  static XEvent event;
  int oldx = 0;
  int oldvalue = -1;
  int newvalue;
  int x1,y1,x2,y2;
  Window Win1,Win2;
  unsigned int modif;
  fd_set in_fdset;

  do
  {
    /* On suit les mouvements de la souris */
    FQueryPointer(dpy, *xobj->ParentWin, &Win1, &Win2, &x1, &y1, &x2, &y2, &modif);
    x2 = x2 - xobj->x;
    if (x2 < 15)
      x2 = 15;
    if (x2 > xobj->width-21)
      x2 = xobj->width - 21;
    if (oldx!=x2)
    {
      oldx = x2;
      /* calcule de xobj->value */
      newvalue = (x2-15)*xobj->width / (xobj->width - 36) *
	(xobj->value3 - xobj->value2) / (xobj->width) + xobj->value2;
      if (newvalue!=oldvalue)
      {
	HideThumbH(xobj);
	xobj->value = newvalue;
	DrawThumbH(xobj, NULL);
	oldvalue = newvalue;
	SendMsg(xobj,SingleClic);
	XSync(dpy, 0);
	usleep(10000);
      }
    }
    FD_ZERO(&in_fdset);
    FD_SET(x_fd, &in_fdset);
    select(32, SELECT_FD_SET_CAST &in_fdset, NULL, NULL, NULL);
  }
  while (!FCheckTypedEvent(dpy, ButtonRelease, &event) && EvtButton != NULL);
}

void EvtKeyHScrollBar(struct XObj *xobj, XKeyEvent *EvtKey)
{
  KeySym ks;
  unsigned char buf[10];

  XLookupString(EvtKey, (char *)buf, sizeof(buf), &ks, NULL);
  if (ks == XK_Left && xobj->value > 0) {
    HideThumbH(xobj);
    xobj->value--;
    DrawThumbH(xobj, NULL);
    SendMsg(xobj,SingleClic);
  }
  else if (ks == XK_Right &&
	   xobj->value <
	   xobj->width*(xobj->value3-xobj->value2)/(xobj->width) + xobj->value2) {
    HideThumbH(xobj);
    xobj->value++;
    DrawThumbH(xobj, NULL);
    SendMsg(xobj,SingleClic);
  }
  else if (ks == XK_Return) {
    EvtMouseHScrollBar(xobj, NULL);
  }
}

void ProcessMsgHScrollBar(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
