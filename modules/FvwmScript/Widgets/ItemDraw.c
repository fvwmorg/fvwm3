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

 Attr.background_pixel=xobj->TabColor[back];
 mask=CWBackPixel;
 
 xobj->win=XCreateWindow(dpy,*xobj->ParentWin,
		xobj->x,xobj->y,xobj->width,xobj->height,0,
		CopyFromParent,InputOutput,CopyFromParent,
		mask,&Attr);
 xobj->gc=XCreateGC(dpy,xobj->win,0,NULL);
 XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
 if ((xobj->xfont=XLoadQueryFont(dpy,xobj->font))==NULL)
   fprintf(stderr,"Can't load font %s\n",xobj->font);
 else
  XSetFont(dpy,xobj->gc,xobj->xfont->fid);

 XSetLineAttributes(dpy,xobj->gc,1,LineSolid,CapRound,JoinMiter);

 /* Redimensionnement du widget */
 if (strlen(xobj->title)!=0)
   XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 else {
   asc = 0;
   desc = 0;
 }
   
 if (xobj->icon==NULL)
 {
  if (strlen(xobj->title)!=0)
  {
   /* title but no icon */
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
  /* icon but no title */
  if (xobj->height<xobj->icon_h)
   xobj->height=xobj->icon_h;
  if (xobj->width<xobj->icon_w)
   xobj->width=xobj->icon_w;
 }
 else
 {
  /* title and icon */
  if (xobj->icon_w>XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title))+2)
  {
  /* icon is wider than the title */
   if (xobj->width<xobj->icon_w)
    xobj->width=xobj->icon_w;
  }
  else
   /* title is wider than icon */
   if (xobj->width<XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title))+2)
    xobj->width=XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title))+2;
  xobj->height=xobj->icon_h+asc+desc+2;
 }
 XResizeWindow(dpy,xobj->win,xobj->width,xobj->height);
 if (xobj->colorset >= 0)
   SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
		       &Colorset[xobj->colorset], Pdepth,
		       xobj->gc, True);
 XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyItemDraw(struct XObj *xobj)
{
 XFreeFont(dpy,xobj->xfont);
 XFreeGC(dpy,xobj->gc);
 XDestroyWindow(dpy,xobj->win);
}

void DrawItemDraw(struct XObj *xobj)
{ 
  XClearArea(dpy,xobj->win,0,0,xobj->width,xobj->height,False);
  XClearWindow(dpy,xobj->win);
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
