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

#include "Tools.h"


/*******************************************************/
/* Fonction pour MiniScroll / Functions for MiniScroll */
/*******************************************************/
void InitMiniScroll(struct XObj *xobj)
{
  unsigned long mask;
  XSetWindowAttributes Attr;
  int i;

  /* Enregistrement des couleurs / colors */
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

  /* La taille du widget est fixe / The widget size is fixed */
  xobj->width = 19;
  xobj->height = 34;
  xobj->win=XCreateWindow(dpy, *xobj->ParentWin,
			  xobj->x, xobj->y, xobj->width, xobj->height, 0,
			  CopyFromParent, InputOutput, CopyFromParent,
			  mask, &Attr);
  xobj->gc=fvwmlib_XCreateGC(dpy, xobj->win, 0, NULL);
  if (xobj->colorset >= 0)
    SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
			&Colorset[xobj->colorset], Pdepth,
			xobj->gc, True);
  XSetForeground(dpy, xobj->gc, xobj->TabColor[fore]);
  XSetLineAttributes(dpy, xobj->gc, 1, LineSolid, CapRound, JoinMiter);
  if (xobj->value2>xobj->value3)
  {
    i = xobj->value2;
    xobj->value2 = xobj->value3;
    xobj->value3 = i;
  }
  if ((xobj->value < xobj->value2) || (xobj->value > xobj->value3))
    xobj->value = xobj->value2;
  XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyMiniScroll(struct XObj *xobj)
{
  XFreeGC(dpy,xobj->gc);
  XDestroyWindow(dpy,xobj->win);
}

void DrawMiniScroll(struct XObj *xobj)
{

  DrawReliefRect(-1, -1, xobj->width+2, xobj->height+2, xobj, hili, shad);

  DrawArrowN(xobj, 3, 3, 0);  /* fleche du haut / top arrow    */
  DrawArrowS(xobj, 3, 18, 0); /* fleche du bas  / bottom arrow */
}

void EvtMouseMiniScroll(struct XObj *xobj, XButtonEvent *EvtButton)
{
  static XEvent event;
  int x1,y1,x2,y2;
  Window Win1,Win2;
  unsigned int modif;
  int Pos = 0;
  struct timeval *tv;
  long tus,ts;
  int count = 0;

  do
  {
    if (count == 1)
    {
      /* emulate auto repeat delay */
      tv = (struct timeval*)calloc(1,sizeof(struct timeval));
      gettimeofday(tv,NULL);
      tus = tv->tv_usec;
      ts = tv->tv_sec;
      while (((tv->tv_usec-tus)+(tv->tv_sec-ts)*1000000)<16667*16)
        gettimeofday(tv,NULL);
      free(tv);
      count++;
      continue;
    }
    XQueryPointer(dpy, *xobj->ParentWin, &Win1, &Win2,
		  &x1, &y1, &x2, &y2, &modif);
    /* Determiner l'option courante */
    y2 = y2 - xobj->y;
    x2 = x2 - xobj->x;
    if ((x2 > 0) && (x2 < xobj->width) && (y2 > 0) && (y2 < xobj->height/2))
    {
      /* top arrow */
      if (Pos == 1)
      {
	tv = (struct timeval*)calloc(1,sizeof(struct timeval));
	gettimeofday(tv,NULL);
	tus = tv->tv_usec;
	ts = tv->tv_sec;
	while (((tv->tv_usec-tus)+(tv->tv_sec-ts)*1000000)<16667*8)
	  gettimeofday(tv,NULL);
	free(tv);
      }
      else
      {
	DrawArrowN(xobj, 3, 3, 1);
	Pos = 1;
      }
      xobj->value++;
      if (xobj->value > xobj->value3)
	xobj->value = xobj->value2;
      SendMsg(xobj,SingleClic);
    }
    else if ( ( x2 >0) && (x2 < xobj->width) && (y2 > xobj->height/2) && 
	      (y2 <xobj->height))
    {
      if (Pos == -1)
      {
	tv = (struct timeval*)calloc(1, sizeof(struct timeval));
	gettimeofday(tv,NULL);
	tus = tv->tv_usec;
	ts = tv->tv_sec;
	while (((tv->tv_usec-tus)+(tv->tv_sec-ts)*1000000)<16667*8)
	  gettimeofday(tv,NULL);
	free(tv);
      }
      else
      {
	DrawArrowS(xobj, 3, 18, 1);
	Pos = -1;
      }
      xobj->value--;
      if (xobj->value < xobj->value2)
	xobj->value = xobj->value3;
      SendMsg(xobj,SingleClic);
    }
    else if (Pos!=0)
    {
      Pos = 0;
      DrawArrowN(xobj, 3, 3, 0);
      DrawArrowS(xobj, 3, 18, 0);
    }
    count++;
  }
  while (!XCheckTypedEvent(dpy, ButtonRelease, &event) && EvtButton != NULL);
  DrawArrowN(xobj, 3, 3, 0);
  DrawArrowS(xobj, 3, 18, 0);
}

void EvtKeyMiniScroll(struct XObj *xobj, XKeyEvent *EvtKey)
{
  KeySym ks;
  unsigned char buf[10];
  
  XLookupString(EvtKey, (char *)buf, sizeof(buf), &ks, NULL);
  if (ks == XK_Return) {
    EvtMouseMiniScroll(xobj, NULL);
  }
  else if (ks == XK_Up) {
    xobj->value++;
    if (xobj->value>xobj->value3)
      xobj->value=xobj->value2;
    SendMsg(xobj,SingleClic);
  }
  else if (ks == XK_Down) {
    xobj->value--;
    if (xobj->value<xobj->value2)
      xobj->value=xobj->value3;
    SendMsg(xobj,SingleClic);
  }
}

void ProcessMsgMiniScroll(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
