#include "Tools.h"


/***********************************************/
/* Fonction pour MiniScroll                      */
/***********************************************/
void InitMiniScroll(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;
 int i;

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

 /* La taille du widget est fixe */
 xobj->width=19;
 xobj->height=34;
 xobj->win=XCreateWindow(xobj->display,*xobj->ParentWin,
		xobj->x,xobj->y,xobj->width,xobj->height,1,
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
 if (xobj->value2>xobj->value3)
 {
  i=xobj->value2;
  xobj->value2=xobj->value3;
  xobj->value3=i;
 }
 if ((xobj->value<xobj->value2)||(xobj->value>xobj->value3))
  xobj->value=xobj->value2;
}

void DestroyMiniScroll(struct XObj *xobj)
{
 XFreeFont(xobj->display,xobj->xfont);
 XFreeGC(xobj->display,xobj->gc);
 XDestroyWindow(xobj->display,xobj->win);
}

void DrawMiniScroll(struct XObj *xobj)
{

 DrawReliefRect(-1,-1,xobj->width+2,xobj->height+2,xobj,xobj->TabColor[li].pixel,
 		xobj->TabColor[shad].pixel,xobj->TabColor[black].pixel,-1);

 /* Dessin de la fleche du haut */
 DrawArrowN(xobj,3,3,0);
 DrawArrowS(xobj,3,18,0);

}

void EvtMouseMiniScroll(struct XObj *xobj,XButtonEvent *EvtButton)
{
 static XEvent event;
 int x1,y1,x2,y2;
 Window Win1,Win2;
 unsigned int modif;
 int Pos=0;
 struct timeval *tv;
 long tus,ts;
 
 do
 {
  XQueryPointer(xobj->display,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
  /* Determiner l'option courante */
  y2=y2-xobj->y;
  x2=x2-xobj->x;
  if ((x2>0)&&(x2<xobj->width)&&(y2>0)&&(y2<xobj->height/2))
  {
   if (Pos==1)
   {
    tv=(struct timeval*)calloc(1,sizeof(struct timeval));
    gettimeofday(tv,NULL);
    tus=tv->tv_usec;
    ts=tv->tv_sec;
    while (((tv->tv_usec-tus)+(tv->tv_sec-ts)*1000000)<16667*8)
     gettimeofday(tv,NULL);
    free(tv);
   }
   else
   {
    DrawArrowN(xobj,3,3,1);
    Pos=1;
   }
   xobj->value++;
   if (xobj->value>xobj->value3)
    xobj->value=xobj->value2;
   SendMsg(xobj,SingleClic);
   
  }
  else if ((x2>0)&&(x2<xobj->width)&&(y2>xobj->height/2)&&(y2<xobj->height))
  {
   if (Pos==-1)
   {
    tv=(struct timeval*)calloc(1,sizeof(struct timeval));
    gettimeofday(tv,NULL);
    tus=tv->tv_usec;
    ts=tv->tv_sec;
    while (((tv->tv_usec-tus)+(tv->tv_sec-ts)*1000000)<16667*8)
     gettimeofday(tv,NULL);
    free(tv);
   }
   else
   {
    DrawArrowS(xobj,3,18,1);
    Pos=-1;
   }
   xobj->value--;
   if (xobj->value<xobj->value2)
    xobj->value=xobj->value3;
   SendMsg(xobj,SingleClic);
  }
  else if (Pos!=0)
  {
   Pos=0;
   DrawArrowN(xobj,3,3,0);
   DrawArrowS(xobj,3,18,0);
  }
 }
 while (!XCheckTypedEvent(xobj->display,ButtonRelease,&event));
 DrawArrowN(xobj,3,3,0);
 DrawArrowS(xobj,3,18,0);
}

void EvtKeyMiniScroll(struct XObj *xobj,XKeyEvent *EvtKey)
{
}

void ProcessMsgMiniScroll(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}








