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
 * Fonction pour HDipstick
 * Création d'une jauge horizontale
 * plusieurs options
 */
void InitHDipstick(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;

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

 /* Minimum size */
 if (xobj->width<30)
  xobj->width=30;
 if (xobj->height<11)
  xobj->height=11;

 mask=0;
 Attr.background_pixel=xobj->TabColor[back];
 mask|=CWBackPixel;
 xobj->win=XCreateWindow(dpy,*xobj->ParentWin,
		xobj->x,xobj->y,xobj->width,xobj->height,0,
		CopyFromParent,InputOutput,CopyFromParent,
		mask,&Attr);
 xobj->gc=fvwmlib_XCreateGC(dpy,xobj->win,0,NULL);
 if (xobj->colorset >= 0)
   SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
		       &Colorset[xobj->colorset], Pdepth,
		       xobj->gc, True);

 XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
 XSetLineAttributes(dpy,xobj->gc,1,LineSolid,CapRound,JoinMiter);

 if (xobj->value2>xobj->value3)
  xobj->value3=xobj->value2+50;
 if (xobj->value<xobj->value2)
  xobj->value=xobj->value2;
 if (xobj->value>xobj->value3)
  xobj->value=xobj->value3;

 XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyHDipstick(struct XObj *xobj)
{
 XFreeGC(dpy,xobj->gc);
 XDestroyWindow(dpy,xobj->win);
}

void DrawHDipstick(struct XObj *xobj, XEvent *evp)
{
 int  i;

 i=(xobj->width-4)*(xobj->value-xobj->value2)/(xobj->value3-xobj->value2);

 DrawReliefRect(0,0,xobj->width,xobj->height,xobj,shad,hili);
 if (i!=0)
 {
  DrawReliefRect(2,2,i,xobj->height-4,xobj,hili,shad);
  XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
  XFillRectangle(dpy,xobj->win,xobj->gc,4,4,i-4,xobj->height-8);
 }

}

void EvtMouseHDipstick(struct XObj *xobj,XButtonEvent *EvtButton)
{
}

void EvtKeyHDipstick(struct XObj *xobj,XKeyEvent *EvtKey)
{
}


void ProcessMsgHDipstick(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
