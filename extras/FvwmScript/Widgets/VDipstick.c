#include "Tools.h"


/***********************************************/
/* Fonction pour VDipstick                     */
/* Création d'une jauge verticale              */
/* plusieurs options                           */
/***********************************************/
void InitVDipstick(struct XObj *xobj)
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

 /* Minimum size */
 if (xobj->width<11) 
  xobj->width=11;
 if (xobj->height<30) 
  xobj->height=30;

 mask=0;
 Attr.background_pixel=x11base->TabColor[back].pixel;
 mask|=CWBackPixel;
 xobj->win=XCreateWindow(xobj->display,*xobj->ParentWin,
		xobj->x,xobj->y,xobj->width,xobj->height,0,
		CopyFromParent,InputOutput,CopyFromParent,
		mask,&Attr);
 xobj->gc=XCreateGC(xobj->display,xobj->win,0,NULL);
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[fore].pixel);
 XSetBackground(xobj->display,xobj->gc,x11base->TabColor[back].pixel);
 XSetLineAttributes(xobj->display,xobj->gc,1,LineSolid,CapRound,JoinMiter);

 if (xobj->value2>xobj->value3)
  xobj->value3=xobj->value2+50;
 if (xobj->value<xobj->value2)
  xobj->value=xobj->value2;
 if (xobj->value>xobj->value3)
  xobj->value=xobj->value3;
}

void DestroyVDipstick(struct XObj *xobj)
{
 XFreeGC(xobj->display,xobj->gc);
 XDestroyWindow(xobj->display,xobj->win);
}

void DrawVDipstick(struct XObj *xobj)
{
 int  i,j;

 i=(xobj->height-4)*(xobj->value-xobj->value2)/(xobj->value3-xobj->value2);
 j=xobj->height-i;

 DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
		x11base->TabColor[shad].pixel,x11base->TabColor[li].pixel,
		x11base->TabColor[black].pixel,-1);
 if (i!=0)
 {
  DrawReliefRect(2,j-2,xobj->width-4,i,xobj,
		xobj->TabColor[li].pixel,xobj->TabColor[shad].pixel,
		xobj->TabColor[black].pixel,-1);
  XSetForeground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
  XFillRectangle(xobj->display,xobj->win,xobj->gc,5,j+1,xobj->width-10,i-6);
 }

}

void EvtMouseVDipstick(struct XObj *xobj,XButtonEvent *EvtButton)
{
}

void EvtKeyVDipstick(struct XObj *xobj,XKeyEvent *EvtKey)
{
}


void ProcessMsgVDipstick(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
