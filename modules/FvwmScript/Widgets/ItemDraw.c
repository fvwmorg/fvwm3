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
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->forecolor,&xobj->TabColor[fore]);
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->backcolor,&xobj->TabColor[back]);
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->licolor,&xobj->TabColor[li]);
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->shadcolor,&xobj->TabColor[shad]);
 MyAllocNamedColor(xobj->display,*xobj->colormap,"#000000",&xobj->TabColor[black]);
 MyAllocNamedColor(xobj->display,*xobj->colormap,"#FFFFFF",&xobj->TabColor[white]);

 mask=0;
 Attr.background_pixel=xobj->TabColor[back].pixel;
 mask|=CWBackPixel;

 xobj->win=XCreateWindow(xobj->display,*xobj->ParentWin,
		xobj->x,xobj->y,xobj->width,xobj->height,0,
		CopyFromParent,InputOutput,CopyFromParent,
		mask,&Attr);
 xobj->gc=XCreateGC(xobj->display,xobj->win,0,NULL);
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[fore].pixel);
 XSetBackground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
 if ((xobj->xfont=XLoadQueryFont(xobj->display,xobj->font))==NULL)
   fprintf(stderr,"Can't load font %s\n",xobj->font);
 else
  XSetFont(xobj->display,xobj->gc,xobj->xfont->fid);

 XSetLineAttributes(xobj->display,xobj->gc,1,LineSolid,CapRound,JoinMiter);

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
 XResizeWindow(xobj->display,xobj->win,xobj->width,xobj->height);
}

void DestroyItemDraw(struct XObj *xobj)
{
 XFreeFont(xobj->display,xobj->xfont);
 XFreeGC(xobj->display,xobj->gc);
 XDestroyWindow(xobj->display,xobj->win);
}

void DrawItemDraw(struct XObj *xobj)
{

 /* Calcul de la position de la chaine de charactere */
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
 XFillRectangle(xobj->display,xobj->win,xobj->gc,0,0,xobj->width,xobj->height);
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








