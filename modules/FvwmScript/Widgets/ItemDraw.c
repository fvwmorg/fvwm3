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
/* Fonction pour ItemDraw                      */
/***********************************************/
void InitItemDraw(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;
 int minHeight,minWidth;
 int asc,desc,dir;
 XCharStruct struc;

 /* Enregistrement des couleurs et de la police */
 xobj->TabColor[fore] = GetColor(xobj->forecolor);
 xobj->TabColor[back] = GetColor(xobj->backcolor);
 xobj->TabColor[li] = GetColor(xobj->licolor);
 xobj->TabColor[shad] = GetColor(xobj->shadcolor);
 xobj->TabColor[black] = GetColor("#000000");
 xobj->TabColor[white] = GetColor("#FFFFFF");

 mask=0;
 Attr.background_pixel=xobj->TabColor[back];
 mask|=CWBackPixel;

 xobj->win=XCreateWindow(dpy,*xobj->ParentWin,
		xobj->x,xobj->y,xobj->width,xobj->height,0,
		CopyFromParent,InputOutput,CopyFromParent,
		mask,&Attr);
 xobj->gc=XCreateGC(dpy,xobj->win,0,NULL);
 XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
 XSetBackground(dpy,xobj->gc,xobj->TabColor[back]);
 if ((xobj->xfont=XLoadQueryFont(dpy,xobj->font))==NULL)
   fprintf(stderr,"Can't load font %s\n",xobj->font);
 else
  XSetFont(dpy,xobj->gc,xobj->xfont->fid);

 XSetLineAttributes(dpy,xobj->gc,1,LineSolid,CapRound,JoinMiter);

 /* Redimensionnement du widget */
 if (xobj->icon==NULL)
 {
  if (strlen(xobj->title)!=0)
  {
   XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
   minHeight=asc+desc+2;
   minWidth=XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title))+2;
   if (xobj->height < minHeight)
     xobj->height = minHeight;
   if (xobj->width < minWidth)
     xobj->width = minWidth;
  }
 }
 else if (strlen(xobj->title)==0)
 {
  if (xobj->height<xobj->icon_h)
   xobj->height=xobj->icon_h;
  if (xobj->width<xobj->icon_w)
   xobj->width=xobj->icon_w;
 }
 else
 {
  if (xobj->icon_w>XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title))+2)
  {
   if (xobj->width<xobj->icon_w)
    xobj->width=xobj->icon_w;
  }
  else
   xobj->width=XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title))+2;
  xobj->height=xobj->icon_h+2*(asc+desc+15);
 }
 XResizeWindow(dpy,xobj->win,xobj->width,xobj->height);
}

void DestroyItemDraw(struct XObj *xobj)
{
 XFreeFont(dpy,xobj->xfont);
 XFreeGC(dpy,xobj->gc);
 XDestroyWindow(dpy,xobj->win);
}

void DrawItemDraw(struct XObj *xobj)
{

 /* Calcul de la position de la chaine de charactere */
 XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
 XFillRectangle(dpy,xobj->win,xobj->gc,0,0,xobj->width,xobj->height);
 DrawIconStr(0,xobj,False);
}

void EvtMouseItemDraw(struct XObj *xobj,XButtonEvent *EvtButton)
{
}

void EvtKeyItemDraw(struct XObj *xobj,XKeyEvent *EvtKey)
{
}

void ProcessMsgItemDraw(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}








