#include "Tools.h"

extern int fd[2];

/***********************************************/
/* Fonction pour Swallow                       */
/***********************************************/

void DrawRelief(struct XObj *xobj)
{
 XSegment segm[2];
 int i;
 
 if (xobj->value!=0)
 {
  for (i=1;i<4;i++)
  {
   segm[0].x1=xobj->x-i;
   segm[0].y1=xobj->y-i;
   segm[0].x2=xobj->x+xobj->width+i-2;
   segm[0].y2=xobj->y-i;
   segm[1].x1=xobj->x-i;
   segm[1].y1=xobj->y-i;
   segm[1].x2=xobj->x-i;
   segm[1].y2=xobj->y+xobj->height+i-2;
   if (xobj->value==-1)
    XSetForeground(xobj->display,xobj->gc,xobj->TabColor[shad].pixel);
   else
    XSetForeground(xobj->display,xobj->gc,xobj->TabColor[li].pixel);
   XDrawSegments(xobj->display,*xobj->ParentWin,xobj->gc,segm,2);
 
   segm[0].x1=xobj->x-i;
   segm[0].y1=xobj->y+xobj->height+i-1;
   segm[0].x2=xobj->x+xobj->width+i-1;
   segm[0].y2=xobj->y+xobj->height+i-1;
   segm[1].x1=xobj->x+xobj->width+i-1;
   segm[1].y1=xobj->y-i;
   segm[1].x2=xobj->x+xobj->width+i-1;
   segm[1].y2=xobj->y+xobj->height+i-1;
   if (xobj->value==-1)
    XSetForeground(xobj->display,xobj->gc,xobj->TabColor[li].pixel);
   else
    XSetForeground(xobj->display,xobj->gc,xobj->TabColor[shad].pixel);
   XDrawSegments(xobj->display,*xobj->ParentWin,xobj->gc,segm,2);
  }
 }

}

void InitSwallow(struct XObj *xobj)
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

 mask=0;
 xobj->win=XCreateWindow(xobj->display,*xobj->ParentWin,
		-1000,-1000,xobj->width,xobj->height,0,
		CopyFromParent,InputOutput,CopyFromParent,
		mask,&Attr);

 /* Redimensionnement du widget */
 if (xobj->height<30)
  xobj->height=30;
 if (xobj->width<30) 
  xobj->width=30;

 if (xobj->swallow!=NULL)
 {
  SendText(fd,xobj->swallow,0);
 }
 else
  fprintf(stderr,"Error\n");

}

void DestroySwallow(struct XObj *xobj)
{
 XSetCloseDownMode(xobj->display,DestroyAll);
 /* Arrete le programme swallow */
 if (xobj->win!=None)
  XKillClient(xobj->display, xobj->win);
}

void DrawSwallow(struct XObj *xobj)
{
 DrawRelief(xobj);
}

void EvtMouseSwallow(struct XObj *xobj,XButtonEvent *EvtButton)
{
}

void EvtKeySwallow(struct XObj *xobj,XKeyEvent *EvtKey)
{
}

/* Recupere le pointeur de la fenetre Swallow */
void CheckForHangon(struct XObj *xobj,unsigned long *body)
{
  char *cbody;

  cbody=(char*)calloc(100,sizeof(char));
  sprintf(cbody,"%s",(char *)&body[3]);
  if(strcmp(cbody,xobj->title)==0)
  {
   xobj->win = (Window)body[0];
   free(xobj->title);
   xobj->title=(char*)calloc(sizeof(char),20);
   sprintf(xobj->title,"No window");
   XUnmapWindow(xobj->display,xobj->win);
   XSetWindowBorderWidth(xobj->display,xobj->win,0);
  }
  free(cbody);
}

void swallow(struct XObj *xobj,unsigned long *body)
{

 if(xobj->win == (Window)body[0])
 {
  XReparentWindow(xobj->display,xobj->win,*xobj->ParentWin,xobj->x,xobj->y);
  XResizeWindow(xobj->display,xobj->win,xobj->width,xobj->height);
  XMapWindow(xobj->display,xobj->win);
 }
}

void ProcessMsgSwallow(struct XObj *xobj,unsigned long type,unsigned long *body)
{
 switch(type)
 {
  case M_MAP:
   swallow(xobj,body);
  break;
  case M_RES_NAME:
  break;
  case M_RES_CLASS:
  break;
  case M_WINDOW_NAME:
    CheckForHangon(xobj,body);
  break;
  default:
  break;
 }
}






