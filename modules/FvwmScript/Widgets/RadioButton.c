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
/* Fonction pour RadioButton                    */
/***********************************************/
void InitRadioButton(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;
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
 Attr.cursor=XCreateFontCursor(dpy,XC_hand2);
 mask|=CWCursor;		/* Curseur pour la fenetre */
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
  {
   fprintf(stderr,"Can't load font %s\n",xobj->font);
  }
 else
  XSetFont(dpy,xobj->gc,xobj->xfont->fid);

 XSetLineAttributes(dpy,xobj->gc,1,LineSolid,CapRound,JoinMiter);

 /* Redimensionnement du widget */
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 xobj->height=asc+desc+5;
 xobj->width=XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title))+20;
 XResizeWindow(dpy,xobj->win,xobj->width,xobj->height);

}

void DestroyRadioButton(struct XObj *xobj)
{
 XFreeFont(dpy,xobj->xfont);
 XFreeGC(dpy,xobj->gc);
 XDestroyWindow(dpy,xobj->win);
}

void DrawRadioButton(struct XObj *xobj)
{
 int i,j;

 j=xobj->height/2+3;
 i=16;
 /* Dessin du cercle arrondi */
 XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
 XDrawArc(dpy,xobj->win,xobj->gc,1,j-11,11,11,45*64,180*64);
 XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
 XDrawArc(dpy,xobj->win,xobj->gc,1,j-11,11,11,225*64,180*64);
 XSetForeground(dpy,xobj->gc,xobj->TabColor[white]);
 XFillArc(dpy,xobj->win,xobj->gc,2,j-10,9,9,0*64,360*64);
 XSetForeground(dpy,xobj->gc,xobj->TabColor[black]);
 XDrawArc(dpy,xobj->win,xobj->gc,2,j-10,9,9,0*64,360*64);
 if (xobj->value)
 {
  XSetForeground(dpy,xobj->gc,xobj->TabColor[black]);
  XFillArc(dpy,xobj->win,xobj->gc,3,j-9,7,7,0*64,360*64);
 }

 /* Calcul de la position de la chaine de charactere */
 DrawString(dpy,xobj->gc,xobj->win,i,j,xobj->title,
	strlen(xobj->title),xobj->TabColor[fore],
	xobj->TabColor[li],xobj->TabColor[back],
	!xobj->flags[1]);
}

void EvtMouseRadioButton(struct XObj *xobj,XButtonEvent *EvtButton)
{
 static XEvent event;
 int End=1;
 unsigned int modif;
 int x1,x2,y1,y2,i,j;
 Window Win1,Win2;
 Window WinBut=0;
 int In = 0;

 j=xobj->height/2+3;
 i=(xobj->width-XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title)))/2;

 while (End)
 {
  XNextEvent(dpy, &event);
  switch (event.type)
    {
      case EnterNotify:
	   XQueryPointer(dpy,*xobj->ParentWin,
		&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
	   if (WinBut==0)
	   {
	    WinBut=Win2;
	    /* Mouse on button */
	    XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
	    XFillArc(dpy,xobj->win,xobj->gc,3,j-9,7,7,0*64,360*64);
	    In=1;
	   }
	   else
	   {
	    if (Win2==WinBut)
	    {
	    /* Mouse on button */
	     XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
	     XFillArc(dpy,xobj->win,xobj->gc,3,j-9,7,7,0*64,360*64);
	     In=1;
	    }
	    else if (In)
	    {
	     In=0;
	     /* Mouse not on button */
	     XSetForeground(dpy,xobj->gc,xobj->TabColor[white]);
	     XFillArc(dpy,xobj->win,xobj->gc,3,j-9,7,7,0*64,360*64);
	    }
	   }
	  break;
      case LeaveNotify:
	   XQueryPointer(dpy,*xobj->ParentWin,
		&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
	   if (Win2==WinBut)
	   {
	    In=1;
	    /* Mouse on button */
	    XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
	    XFillArc(dpy,xobj->win,xobj->gc,3,j-9,7,7,0*64,360*64);
	   }
	   else if (In)
	   {
	    /* Mouse not on button */
	    XSetForeground(dpy,xobj->gc,xobj->TabColor[white]);
	    XFillArc(dpy,xobj->win,xobj->gc,3,j-9,7,7,0*64,360*64);
	    In=0;
	   }
	  break;
      case ButtonRelease:
	   End=0;
	   /* Mouse not on button */
	   if (In)
	   {
	    /* Envoie d'un message vide de type SingleClic pour un clique souris */
	    xobj->value=1;
	    XSetForeground(dpy,xobj->gc,xobj->TabColor[black]);
	    XFillArc(dpy,xobj->win,xobj->gc,3,j-9,7,7,0*64,360*64);
	    SendMsg(xobj,SingleClic);
	   }
	   else if (xobj->value)
	   {
	    XSetForeground(dpy,xobj->gc,xobj->TabColor[black]);
	    XFillArc(dpy,xobj->win,xobj->gc,3,j-9,7,7,0*64,360*64);
	   }
	  break;
     }
 }
}

void EvtKeyRadioButton(struct XObj *xobj,XKeyEvent *EvtKey)
{
}

void ProcessMsgRadioButton(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}






