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


/***********************************************/
/* Fonction pour HDipstick                     */
/* Création d'une jauge horizontale            */
/* plusieurs options                           */
/***********************************************/
void InitHDipstick(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;

 /* Enregistrement des couleurs et de la police */
 xobj->TabColor[fore] = GetColor(xobj->forecolor);
 xobj->TabColor[back] = GetColor(xobj->backcolor);
 xobj->TabColor[li] = GetColor(xobj->licolor);
 xobj->TabColor[shad] = GetColor(xobj->shadcolor);
 xobj->TabColor[black] = GetColor("#000000");
 xobj->TabColor[white] = GetColor("#FFFFFF");

 /* Minimum size */
 if (xobj->width<30)
  xobj->width=30;
 if (xobj->height<11)
  xobj->height=11;

 mask=0;
 Attr.background_pixel=x11base->TabColor[back];
 mask|=CWBackPixel;
 xobj->win=XCreateWindow(dpy,*xobj->ParentWin,
		xobj->x,xobj->y,xobj->width,xobj->height,0,
		CopyFromParent,InputOutput,CopyFromParent,
		mask,&Attr);
 xobj->gc=XCreateGC(dpy,xobj->win,0,NULL);
 XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
 XSetBackground(dpy,xobj->gc,x11base->TabColor[back]);
 XSetLineAttributes(dpy,xobj->gc,1,LineSolid,CapRound,JoinMiter);

 if (xobj->value2>xobj->value3)
  xobj->value3=xobj->value2+50;
 if (xobj->value<xobj->value2)
  xobj->value=xobj->value2;
 if (xobj->value>xobj->value3)
  xobj->value=xobj->value3;
}

void DestroyHDipstick(struct XObj *xobj)
{
 XFreeGC(dpy,xobj->gc);
 XDestroyWindow(dpy,xobj->win);
}

void DrawHDipstick(struct XObj *xobj)
{
 int  i;

 i=(xobj->width-4)*(xobj->value-xobj->value2)/(xobj->value3-xobj->value2);

 DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
		x11base->TabColor[shad],x11base->TabColor[li],
		x11base->TabColor[black],-1);
 if (i!=0)
 {
  DrawReliefRect(2,2,i,xobj->height-4,xobj,
		xobj->TabColor[li],xobj->TabColor[shad],
		xobj->TabColor[black],-1);
  XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
  XFillRectangle(dpy,xobj->win,xobj->gc,5,5,i-6,xobj->height-10);
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
