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
/* Fonction pour HScrollBar                    */
/***********************************************/
void DrawThumbH(struct XObj *xobj)
{
 int x,y,w,h;
 XSegment segm;
 char str[20];
 int asc,desc,dir;
 XCharStruct struc;

 x=3+(xobj->width-36)*(xobj->value-xobj->value2)/(xobj->value3-xobj->value2);
 y=xobj->height/2-9;
 w=30;
 h=18;
 DrawReliefRect(x,y,w,h,xobj,xobj->TabColor[li],xobj->TabColor[shad],
 		xobj->TabColor[black],-1);
 segm.x1=x+15;
 segm.y1=y+3;
 segm.x2=x+15;
 segm.y2=y+h-3;
 XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
 XDrawSegments(dpy,xobj->win,xobj->gc,&segm,1);
 segm.x1=x+16;
 segm.y1=y+3;
 segm.x2=x+16;
 segm.y2=y+h-3;
 XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
 XDrawSegments(dpy,xobj->win,xobj->gc,&segm,1);
 XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);

 sprintf(str,"%d",xobj->value);
 x=x+15-(XTextWidth(xobj->xfont,str,strlen(str))/2);
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 y=y-desc-4;
 DrawString(dpy,xobj->gc,xobj->win,x,y,str,
	strlen(str),xobj->TabColor[fore],
	xobj->TabColor[li],xobj->TabColor[back],
	!xobj->flags[1]);

}

void HideThumbH(struct XObj *xobj)
{
 int x,y;
 int asc,desc,dir;
 XCharStruct struc;

 x=4+(xobj->width-36)*(xobj->value-xobj->value2)/(xobj->value3-xobj->value2);
 y=xobj->height/2-8;
 XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
 XFillRectangle(dpy,xobj->win,xobj->gc,x,y,28,16);
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 XFillRectangle(dpy,xobj->win,xobj->gc,x-asc
 			-desc,y-asc-10,90,asc+desc+2);
}

void InitHScrollBar(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;
 int i;
 int asc,desc,dir;
 XCharStruct struc;
 char str[20];

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
 Attr.cursor=XCreateFontCursor(dpy,XC_hand2);
 mask|=CWCursor;		/* Curseur pour la fenetre */

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

 if ((xobj->value3-xobj->value2)<=0)
  xobj->value3=xobj->value2+10;
 if (!((xobj->value>=xobj->value2)&&(xobj->value<=xobj->value3)))
  xobj->value=xobj->value2;
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 xobj->height=(asc+desc)*2+30;
 sprintf(str,"%d",xobj->value2);
 i=XTextWidth(xobj->xfont,str,strlen(str));
 sprintf(str,"%d",xobj->value3);
 i=XTextWidth(xobj->xfont,str,strlen(str))+i+20;
 if (xobj->width<i)
  xobj->width=i;
 XResizeWindow(dpy,xobj->win,xobj->width,xobj->height);
}

void DestroyHScrollBar(struct XObj *xobj)
{
 XFreeFont(dpy,xobj->xfont);
 XFreeGC(dpy,xobj->gc);
 XDestroyWindow(dpy,xobj->win);
}

void DrawHScrollBar(struct XObj *xobj)
{
 int x,y,w,h;
 char str[20];
 int asc,desc,dir;
 XCharStruct struc;

 /* Calcul de la taille de l'ascenseur */
 x=0;
 y=xobj->height/2-12;
 w=xobj->width;
 h=24;
 DrawReliefRect(x,y,w,h,xobj,xobj->TabColor[shad],xobj->TabColor[li],
 		xobj->TabColor[black],1);
 DrawThumbH(xobj);

 /* Ecriture des valeurs */
 sprintf(str,"%d",xobj->value2);
 x=4;
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 y=y+asc+h;
 DrawString(dpy,xobj->gc,xobj->win,x,y,str,
	strlen(str),xobj->TabColor[fore],
	xobj->TabColor[li],xobj->TabColor[back],
	!xobj->flags[1]);
 sprintf(str,"%d",xobj->value3);
 x=w-XTextWidth(xobj->xfont,str,strlen(str))-4;
 DrawString(dpy,xobj->gc,xobj->win,x,y,str,
	strlen(str),xobj->TabColor[fore],
	xobj->TabColor[li],xobj->TabColor[back],
	!xobj->flags[1]);
}

void EvtMouseHScrollBar(struct XObj *xobj,XButtonEvent *EvtButton)
{
 static XEvent event;
 int x,y,w,h;
 int oldx=0;
 int oldvalue=-1;
 int newvalue;
 int x1,y1,x2,y2;
 Window Win1,Win2;
 unsigned int modif;

 x=3+((xobj->width-36)*xobj->value)/(xobj->value3-xobj->value2);
 y=xobj->height/2-9;
 w=30;
 h=18;


 do
 {
  /* On suit les mouvements de la souris */
  XQueryPointer(dpy,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
  x2=x2-xobj->x;
  if (x2<15) x2=15;
  if (x2>xobj->width-21) x2=xobj->width-21;
  if (oldx!=x2)
  {
   oldx=x2;
   /* calcule de xobj->value */
   newvalue=(x2-15)*xobj->width/(xobj->width-36)*(xobj->value3-xobj->value2)/(xobj->width)+xobj->value2;
   if (newvalue!=oldvalue)
   {
    HideThumbH(xobj);
    xobj->value=newvalue;
    DrawThumbH(xobj);
    oldvalue=newvalue;
    SendMsg(xobj,SingleClic);
   }
  }
 }
 while (!XCheckTypedEvent(dpy,ButtonRelease,&event));
}

void EvtKeyHScrollBar(struct XObj *xobj,XKeyEvent *EvtKey)
{
}

void ProcessMsgHScrollBar(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}














