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
 DrawReliefRect(x,y,w,h,xobj,xobj->TabColor[li].pixel,xobj->TabColor[shad].pixel,
 		xobj->TabColor[black].pixel,-1);
 segm.x1=x+15;
 segm.y1=y+3;
 segm.x2=x+15;
 segm.y2=y+h-3;
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[shad].pixel);
 XDrawSegments(xobj->display,xobj->win,xobj->gc,&segm,1);
 segm.x1=x+16;
 segm.y1=y+3;
 segm.x2=x+16;
 segm.y2=y+h-3;
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[li].pixel);
 XDrawSegments(xobj->display,xobj->win,xobj->gc,&segm,1);
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[fore].pixel);
 
 sprintf(str,"%d",xobj->value);
 x=x+15-(XTextWidth(xobj->xfont,str,strlen(str))/2);
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 y=y-desc-4;
 DrawString(xobj->display,xobj->gc,xobj->win,x,y,str,
	strlen(str),xobj->TabColor[fore].pixel,
	xobj->TabColor[li].pixel,xobj->TabColor[back].pixel,
	!xobj->flags[1]);

}

void HideThumbH(struct XObj *xobj)
{
 int x,y;
 int asc,desc,dir;
 XCharStruct struc;

 x=4+(xobj->width-36)*(xobj->value-xobj->value2)/(xobj->value3-xobj->value2);
 y=xobj->height/2-8;
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
 XFillRectangle(xobj->display,xobj->win,xobj->gc,x,y,28,16);
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 XFillRectangle(xobj->display,xobj->win,xobj->gc,x-asc
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
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->forecolor,&xobj->TabColor[fore]); 
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->backcolor,&xobj->TabColor[back]);
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->licolor,&xobj->TabColor[li]); 
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->shadcolor,&xobj->TabColor[shad]);
 MyAllocNamedColor(xobj->display,*xobj->colormap,"#000000",&xobj->TabColor[black]);
 MyAllocNamedColor(xobj->display,*xobj->colormap,"#FFFFFF",&xobj->TabColor[white]);

 mask=0;
 Attr.background_pixel=xobj->TabColor[back].pixel;
 mask|=CWBackPixel;
 Attr.cursor=XCreateFontCursor(xobj->display,XC_hand2); 
 mask|=CWCursor;		/* Curseur pour la fenetre */

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
 XResizeWindow(xobj->display,xobj->win,xobj->width,xobj->height);
}

void DestroyHScrollBar(struct XObj *xobj)
{
 XFreeFont(xobj->display,xobj->xfont);
 XFreeGC(xobj->display,xobj->gc);
 XDestroyWindow(xobj->display,xobj->win);
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
 DrawReliefRect(x,y,w,h,xobj,xobj->TabColor[shad].pixel,xobj->TabColor[li].pixel,
 		xobj->TabColor[black].pixel,1);
 DrawThumbH(xobj);
 
 /* Ecriture des valeurs */
 sprintf(str,"%d",xobj->value2);
 x=4;
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 y=y+asc+h;
 DrawString(xobj->display,xobj->gc,xobj->win,x,y,str,
	strlen(str),xobj->TabColor[fore].pixel,
	xobj->TabColor[li].pixel,xobj->TabColor[back].pixel,
	!xobj->flags[1]);
 sprintf(str,"%d",xobj->value3);
 x=w-XTextWidth(xobj->xfont,str,strlen(str))-4;
 DrawString(xobj->display,xobj->gc,xobj->win,x,y,str,
	strlen(str),xobj->TabColor[fore].pixel,
	xobj->TabColor[li].pixel,xobj->TabColor[back].pixel,
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
  XQueryPointer(xobj->display,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
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
 while (!XCheckTypedEvent(xobj->display,ButtonRelease,&event));
}

void EvtKeyHScrollBar(struct XObj *xobj,XKeyEvent *EvtKey)
{
}

void ProcessMsgHScrollBar(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}














