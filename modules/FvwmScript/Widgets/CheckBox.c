#include "Tools.h"

/***********************************************/
/* Fonction pour CheckBox                      */
/***********************************************/
void InitCheckBox(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;
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
 
 /* Redimensionnement du widget */
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 xobj->height=asc+desc+5;
 xobj->width=XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title))+30;
 XResizeWindow(xobj->display,xobj->win,xobj->width,xobj->height);
}

void DestroyCheckBox(struct XObj *xobj)
{
 XFreeFont(xobj->display,xobj->xfont);
 XFreeGC(xobj->display,xobj->gc);
 XDestroyWindow(xobj->display,xobj->win);
}

void DrawCheckBox(struct XObj *xobj)
{
 XSegment segm[2];
 int asc,desc,dir;
 XCharStruct struc;

 /* Dessin du rectangle arrondi */
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 DrawReliefRect(0,asc-11,15,15,xobj,xobj->TabColor[li].pixel,
 		xobj->TabColor[shad].pixel,xobj->TabColor[black].pixel,0);

 /* Calcul de la position de la chaine de charactere */
 DrawString(xobj->display,xobj->gc,xobj->win,23,
	asc,xobj->title,
	strlen(xobj->title),xobj->TabColor[fore].pixel,
	xobj->TabColor[li].pixel,xobj->TabColor[back].pixel,
	!xobj->flags[1]);
 /* Dessin de la croix */
 if (xobj->value)
 {
  XSetLineAttributes(xobj->display,xobj->gc,2,LineSolid,CapProjecting,JoinMiter);
  segm[0].x1=5;
  segm[0].y1=5+asc-11;
  segm[0].x2=9;
  segm[0].y2=9+asc-11;
  segm[1].x1=5;
  segm[1].y1=9+asc-11;
  segm[1].x2=9;
  segm[1].y2=5+asc-11;
  XSetForeground(xobj->display,xobj->gc,xobj->TabColor[black].pixel);
  XDrawSegments(xobj->display,xobj->win,xobj->gc,segm,2);
  XSetLineAttributes(xobj->display,xobj->gc,1,LineSolid,CapRound,JoinMiter);
 }
}

void EvtMouseCheckBox(struct XObj *xobj,XButtonEvent *EvtButton)
{
 static XEvent event;
 int End=1;
 unsigned int modif;
 int x1,x2,y1,y2;
 Window Win1,Win2;
 Window WinBut=0;
 int In;
 XSegment segm[2];
 int asc,desc,dir;
 XCharStruct struc;

 while (End)
 {
  XNextEvent(xobj->display, &event);
  switch (event.type)
    {
      case EnterNotify:
	   XQueryPointer(xobj->display,*xobj->ParentWin,
		&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
	   if (WinBut==0)
	   {
	    WinBut=Win2;
	    /* Mouse on button */
	     XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
             DrawReliefRect(0,asc-11,15,15,xobj,xobj->TabColor[shad].pixel,
            		xobj->TabColor[li].pixel,xobj->TabColor[black].pixel,0);
	    In=1;
	   }
	   else
	   {
	    if (Win2==WinBut)
	    {
	    /* Mouse on button */
             DrawReliefRect(0,asc-11,15,15,xobj,xobj->TabColor[shad].pixel,
             		xobj->TabColor[li].pixel,xobj->TabColor[black].pixel,0);
	     In=1;
	    }
	    else if (In)
	    {
	     In=0;
	     /* Mouse not on button */
             DrawReliefRect(0,asc-11,15,15,xobj,xobj->TabColor[li].pixel,
             		xobj->TabColor[shad].pixel,xobj->TabColor[black].pixel,0);
	    }
	   }
	  break;
      case LeaveNotify:
	   XQueryPointer(xobj->display,*xobj->ParentWin,
		&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
	   if (Win2==WinBut)
	   {
	    In=1;
	    /* Mouse on button */
            DrawReliefRect(0,asc-11,15,15,xobj,xobj->TabColor[shad].pixel,
            	xobj->TabColor[li].pixel,xobj->TabColor[black].pixel,0);
	   }
	   else if (In)
	   {
	    /* Mouse not on button */
            DrawReliefRect(0,asc-11,15,15,xobj,xobj->TabColor[li].pixel,
            	xobj->TabColor[shad].pixel,xobj->TabColor[black].pixel,0);
	    In=0;
	   }
	  break;
      case ButtonRelease:
	   End=0;
	   /* Mouse not on button */
	   if (In)
	   {
	    /* Envoie d'un message vide de type SingleClic pour un clique souris */
	    xobj->value=!xobj->value;
            DrawReliefRect(0,asc-11,15,15,xobj,xobj->TabColor[li].pixel,
            	xobj->TabColor[shad].pixel,xobj->TabColor[black].pixel,0);
	    SendMsg(xobj,SingleClic);
	   }
	   if (xobj->value)
	   {
  	    XSetLineAttributes(xobj->display,xobj->gc,2,LineSolid,CapProjecting,JoinMiter);
	    segm[0].x1=5;
	    segm[0].y1=5+asc-11;
	    segm[0].x2=9;
	    segm[0].y2=9+asc-11;
	    segm[1].x1=5;
	    segm[1].y1=9+asc-11;
	    segm[1].x2=9;
	    segm[1].y2=5+asc-11;
	    XSetForeground(xobj->display,xobj->gc,xobj->TabColor[black].pixel);
	    XDrawSegments(xobj->display,xobj->win,xobj->gc,segm,2);
	    XSetLineAttributes(xobj->display,xobj->gc,1,LineSolid,CapRound,JoinMiter);
	   }
	   else
	   {
	    XSetForeground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
	    XFillRectangle(xobj->display,xobj->win,xobj->gc,3,asc-8,9,9);
	   }
	  break;
     }
 }
}

void EvtKeyCheckBox(struct XObj *xobj,XKeyEvent *EvtKey)
{
}


void ProcessMsgCheckBox(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}





