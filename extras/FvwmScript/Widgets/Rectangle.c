#include "Tools.h"


/***********************************************/
/* Fonction pour Rectangle                      */
/***********************************************/
void InitRectangle(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;

 /* Enregistrement des couleurs et de la police */
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->forecolor,&xobj->TabColor[fore]); 
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->backcolor,&xobj->TabColor[back]);
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->licolor,&xobj->TabColor[li]); 
 MyAllocNamedColor(xobj->display,*xobj->colormap,xobj->shadcolor,&xobj->TabColor[shad]);
 MyAllocNamedColor(xobj->display,*xobj->colormap,"#000000",&xobj->TabColor[black]);
 MyAllocNamedColor(xobj->display,*xobj->colormap,"#FFFFFF",&xobj->TabColor[white]);

 xobj->gc=XCreateGC(xobj->display,*xobj->ParentWin,0,NULL);
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[fore].pixel);
 XSetBackground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
 XSetLineAttributes(xobj->display,xobj->gc,1,LineSolid,CapRound,JoinMiter);
 
 mask=0;
 xobj->win=XCreateWindow(xobj->display,*xobj->ParentWin,
		-1000,-1000,xobj->width,xobj->height,0,
		CopyFromParent,InputOutput,CopyFromParent,
		mask,&Attr);
}

void DestroyRectangle(struct XObj *xobj)
{
 XFreeGC(xobj->display,xobj->gc);
 XDestroyWindow(xobj->display,xobj->win);
}

void DrawRectangle(struct XObj *xobj)
{
 XSegment segm[4];
 
 segm[0].x1=xobj->x;
 segm[0].y1=xobj->y;
 segm[0].x2=xobj->width+xobj->x-1;
 segm[0].y2=xobj->y;
  
 segm[1].x1=xobj->x;
 segm[1].y1=xobj->y;
 segm[1].x2=xobj->x;
 segm[1].y2=xobj->height+xobj->y-1;

 segm[2].x1=2+xobj->x;
 segm[2].y1=xobj->height-2+xobj->y;
 segm[2].x2=xobj->width-2+xobj->x;
 segm[2].y2=xobj->height-2+xobj->y;

 segm[3].x1=xobj->width-2+xobj->x;
 segm[3].y1=2+xobj->y;
 segm[3].x2=xobj->width-2+xobj->x;
 segm[3].y2=xobj->height-2+xobj->y;

 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[shad].pixel);
 XDrawSegments(xobj->display,*xobj->ParentWin,xobj->gc,segm,4);


 segm[0].x1=1+xobj->x;
 segm[0].y1=1+xobj->y;
 segm[0].x2=xobj->width-1+xobj->x;
 segm[0].y2=1+xobj->y;
  
 segm[1].x1=1+xobj->x;
 segm[1].y1=1+xobj->y;
 segm[1].x2=1+xobj->x;
 segm[1].y2=xobj->height-1+xobj->y;

 segm[2].x1=1+xobj->x;
 segm[2].y1=xobj->height-1+xobj->y;
 segm[2].x2=xobj->width-1+xobj->x;
 segm[2].y2=xobj->height-1+xobj->y;

 segm[3].x1=xobj->width-1+xobj->x;
 segm[3].y1=1+xobj->y;
 segm[3].x2=xobj->width-1+xobj->x;
 segm[3].y2=xobj->height-1+xobj->y;

 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[li].pixel);
 XDrawSegments(xobj->display,*xobj->ParentWin,xobj->gc,segm,4);

}

void EvtMouseRectangle(struct XObj *xobj,XButtonEvent *EvtButton)
{
}

void EvtKeyRectangle(struct XObj *xobj,XKeyEvent *EvtKey)
{
}


void ProcessMsgRectangle(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
