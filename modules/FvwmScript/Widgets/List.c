#include "Tools.h"

#define BdWidth 2		/* Border width */
#define SbWidth 15		/* ScrollBar width */

/***********************************************/
/* Fonction pour Liste                         */
/***********************************************/
void InitList(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;
 int asc,desc,dir;
 XCharStruct struc;
 int minw,minh,resize=0;
 int NbVisCell,NbCell;

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
 XSetLineAttributes(xobj->display,xobj->gc,1,LineSolid,CapRound,JoinMiter);
 if ((xobj->xfont=XLoadQueryFont(xobj->display,xobj->font))==NULL)
   fprintf(stderr,"Can't load font %s\n",xobj->font);
 else
  XSetFont(xobj->display,xobj->gc,xobj->xfont->fid);

 /* Calcul de la taille du widget */
 /* Taille minimum: une ligne ou ascenseur visible */
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 minh=8+3*BdWidth+3*(asc+desc+3);
 if (xobj->height<minh)
 { 
  xobj->height=minh;
  resize=1;
 }
 minw=12+3*BdWidth+SbWidth+75;
 if (xobj->width<minw) 
 {
  xobj->width=minw;
  resize=1;
 }
 if (resize)
  XResizeWindow(xobj->display,xobj->win,xobj->width,xobj->height);

 /* Calcul de la premiere cellule visible */
 NbVisCell=(xobj->height-2-3*BdWidth)/(asc+desc+3);
 NbCell=CountOption(xobj->title);
 if (NbCell>NbVisCell)
 {
  if (xobj->value2>(NbCell-NbVisCell+1))
   xobj->value2=NbCell-NbVisCell+1;
 if ((xobj->value2<1)||(xobj->value2<0))
  xobj->value2=1;
 }
 else
  xobj->value2=1;
}

void DrawVSbList(struct XObj *xobj,int NbCell,int NbVisCell,int press)
{
 XRectangle r;
 int PosTh,SizeTh;

 r.y=2+BdWidth;
 r.x=xobj->width-(6+BdWidth)-SbWidth;
 r.height=xobj->height-r.y-2*BdWidth;
 r.width=SbWidth+4;
 DrawReliefRect(r.x,r.y,r.width,r.height,xobj,xobj->TabColor[shad].pixel,
		xobj->TabColor[li].pixel,xobj->TabColor[fore].pixel,-1);

 /* Calcul du rectangle pour les fleches*/
 r.x=r.x+2;
 r.y=r.y+2;
 r.width=r.width-4;
 r.height=r.height-4;

 /* Dessin de la fleche haute */
 DrawArrowN(xobj,r.x+1,r.y+1,press==1);
 DrawArrowS(xobj,r.x+1,r.y+r.height-14,press==2);

 /* Calcul du rectangle pour le pouce*/
 r.y=r.y+13;
 r.height=r.height-26;

 /* Effacement */
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
 XFillRectangle(xobj->display,xobj->win,xobj->gc,r.x+1,r.y+1,r.width-2,r.height-2);

 /* Dessin du pouce */
 if (NbVisCell<NbCell)
  SizeTh=(NbVisCell*(r.height-8)/NbCell)+8;
 else
  SizeTh=r.height;
 PosTh=(xobj->value2-1)*(r.height-8)/NbCell;
 DrawReliefRect(r.x,r.y+PosTh,r.width,SizeTh,xobj,xobj->TabColor[li].pixel,
		xobj->TabColor[shad].pixel,xobj->TabColor[fore].pixel,-1);
}

void DrawCellule(struct XObj *xobj,int NbCell,int NbVisCell,int HeightCell,int asc)
{
 XRectangle r;
 char *Title;
 int i;

 r.x=4+BdWidth;
 r.y=r.x;
 r.width=xobj->width-r.x-10-2*BdWidth-SbWidth;
 r.height=xobj->height-r.y-4-2*BdWidth;

 /* Dessin des cellules */ 
 XSetClipRectangles(xobj->display,xobj->gc,0,0,&r,1,Unsorted);
 for (i=xobj->value2;i<xobj->value2+NbVisCell;i++)
 {
  Title=GetMenuTitle(xobj->title,i);
  if (strlen(Title)!=0)
   if (xobj->value==i)
   {
    XSetForeground(xobj->display,xobj->gc,xobj->TabColor[shad].pixel);
    XFillRectangle(xobj->display,xobj->win,xobj->gc,r.x+2,r.y+(i-xobj->value2)*HeightCell+2,
		xobj->width-2,HeightCell-2);
    DrawString(xobj->display,xobj->gc,xobj->win,5+r.x,(i-xobj->value2)*HeightCell+asc+2+r.y,Title,
		strlen(Title),xobj->TabColor[fore].pixel,xobj->TabColor[back].pixel,
		xobj->TabColor[shad].pixel,!xobj->flags[1]);
   }
   else
   {
    XSetForeground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
    XFillRectangle(xobj->display,xobj->win,xobj->gc,r.x+2,r.y+(i-xobj->value2)*HeightCell+2,
		xobj->width-2,HeightCell-2);
    DrawString(xobj->display,xobj->gc,xobj->win,5+r.x,(i-xobj->value2)*HeightCell+asc+2+r.y,Title,
		strlen(Title),xobj->TabColor[fore].pixel,xobj->TabColor[li].pixel,
		xobj->TabColor[back].pixel,!xobj->flags[1]);
   }
  else
  {
   XSetForeground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
   XFillRectangle(xobj->display,xobj->win,xobj->gc,r.x+2,r.y+(i-xobj->value2)*HeightCell+2,
	xobj->width-2,HeightCell-2);
  }
 }
 XSetClipMask(xobj->display,xobj->gc,None); 
}

void DestroyList(struct XObj *xobj)
{
 XFreeGC(xobj->display,xobj->gc);
 XDestroyWindow(xobj->display,xobj->win);
}

void DrawList(struct XObj *xobj)
{
 int asc,desc,dir,HeightCell;
 XCharStruct struc;
 int NbVisCell,NbCell;
 XRectangle r;

 /* Dessin du contour */
 DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
	xobj->TabColor[li].pixel,xobj->TabColor[shad].pixel,xobj->TabColor[fore].pixel,-1);

 /* Dessin du contour de la liste */
 r.x=2+BdWidth;
 r.y=r.x;
 r.width=xobj->width-r.x-4-2*BdWidth-SbWidth;
 r.height=xobj->height-r.y-2*BdWidth;
 DrawReliefRect(r.x,r.y,r.width,r.height,xobj,xobj->TabColor[shad].pixel,
	xobj->TabColor[li].pixel,xobj->TabColor[fore].pixel,-1);

 /* Calcul du nombre de cellules visibles */
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 HeightCell=asc+desc+3;
 NbVisCell=r.height/HeightCell;
 NbCell=CountOption(xobj->title);
 if (NbCell>NbVisCell)
 {
  if (xobj->value2>(NbCell-NbVisCell+1))
   xobj->value2=NbCell-NbVisCell+1;
 if ((xobj->value2<1)||(xobj->value2<0))
  xobj->value2=1;
 }
 else
  xobj->value2=1;
 

 /* Dessin des cellules */
 DrawCellule(xobj,NbCell,NbVisCell,HeightCell,asc);

 /* Dessin de l'ascenseur vertical */
 DrawVSbList(xobj,NbCell,NbVisCell,0);

}

void EvtMouseList(struct XObj *xobj,XButtonEvent *EvtButton)
{
 XRectangle rect,rectT;
 XPoint pt;
 XCharStruct struc;
 int x1,y1,x2,y2;
 Window Win1,Win2;
 unsigned int modif;
 int In=1;
 static XEvent event;
 int asc,desc,dir;
 int NbVisCell,NbCell,HeightCell,NPosCell,PosMouse;
 fd_set in_fdset;

 pt.x=EvtButton->x-xobj->x;
 pt.y=EvtButton->y-xobj->y;

 /* Clic dans une cellule */
 rect.x=4+BdWidth;
 rect.y=rect.x;
 rect.width=xobj->width-rect.x-10-2*BdWidth-SbWidth;
 rect.height=xobj->height-rect.y-4-2*BdWidth;
 if(PtInRect(pt,rect))
 {
  /* Determination de la cellule */
  pt.y=pt.y-rect.y;
  XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
  NPosCell=xobj->value2+(pt.y/(asc+desc+3));
  if (NPosCell>CountOption(xobj->title))
   NPosCell=0;
  if (NPosCell!=xobj->value)
  {
   xobj->value=NPosCell;
   DrawList(xobj);
  }
/*  if (IsItDoubleClic(xobj))
   SendMsg(xobj,DoubleClic);
  else*/
   SendMsg(xobj,SingleClic);
  return ;
 }
 
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 HeightCell=asc+desc+3;
 NbVisCell=(xobj->height-6-BdWidth)/HeightCell;
 NbCell=CountOption(xobj->title);

 /* Clic fleche haute asc vertical */
 rect.y=5+BdWidth;
 rect.x=xobj->width-(6+BdWidth)-SbWidth+3;
 rect.height=12;
 rect.width=12;
 if(PtInRect(pt,rect))
 {
  DrawVSbList(xobj,NbCell,NbVisCell,1);
  do
  {
   XQueryPointer(xobj->display,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
   pt.y=y2-xobj->y;
   pt.x=x2-xobj->x;
   if (PtInRect(pt,rect))
   {
    if (In)
    {
     Wait(8);
     xobj->value2--;
     if (xobj->value2<1)
      xobj->value2=1;
     else
     {
      DrawCellule(xobj,NbCell,NbVisCell,HeightCell,asc);
      DrawVSbList(xobj,NbCell,NbVisCell,1);
     }
    }
    else
    {
     In=1;
     DrawVSbList(xobj,NbCell,NbVisCell,1);
      xobj->value2--;
     if (xobj->value2<1)
      xobj->value2=1;
     else
      DrawCellule(xobj,NbCell,NbVisCell,HeightCell,asc);
    }
   }
   else
   {
    if (In)
    {
     In=0;
     DrawVSbList(xobj,NbCell,NbVisCell,0);
    }
   }
   FD_ZERO(&in_fdset);
   FD_SET(x_fd,&in_fdset);
   select(32, &in_fdset, NULL, NULL, NULL);
  }
  while (!XCheckTypedEvent(xobj->display,ButtonRelease,&event));
  DrawVSbList(xobj,NbCell,NbVisCell,0);
  return;
 }

 /* Clic flache basse asc vertical */
 rect.y=xobj->height-2*BdWidth-16;
 if(PtInRect(pt,rect))
 {
  DrawVSbList(xobj,NbCell,NbVisCell,2);
  do
  {
   XQueryPointer(xobj->display,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
   pt.y=y2-xobj->y;
   pt.x=x2-xobj->x;
   if (PtInRect(pt,rect))
   {
    if (In)
    {
     Wait(8);
     if (xobj->value2<=NbCell-NbVisCell)
     {
      xobj->value2++;
      DrawCellule(xobj,NbCell,NbVisCell,HeightCell,asc);
      DrawVSbList(xobj,NbCell,NbVisCell,2);
     }
    }
    else
    {
     In=1;
     DrawVSbList(xobj,NbCell,NbVisCell,2);
     if (xobj->value2<=NbCell-NbVisCell)
     {
      xobj->value2++;
      DrawCellule(xobj,NbCell,NbVisCell,HeightCell,asc);
     }
    }
   }
   else
   {
    if (In)
    {
     In=0;
     DrawVSbList(xobj,NbCell,NbVisCell,0);
    }
   }
   FD_ZERO(&in_fdset);
   FD_SET(x_fd,&in_fdset);
   select(32, &in_fdset, NULL, NULL, NULL);
  }
  while (!XCheckTypedEvent(xobj->display,ButtonRelease,&event));
  DrawVSbList(xobj,NbCell,NbVisCell,0);
  return;
 }

 /* clic sur la zone pouce de l'ascenseur de l'ascenseur */
 rect.y=17+BdWidth;
 rect.x=xobj->width-(6+BdWidth)-SbWidth+2;
 rect.height=xobj->height-rect.y-19-2*BdWidth;
 rect.width=SbWidth;
 if(PtInRect(pt,rect))
 {
  /* Clic dans le pouce */
  rectT.x=rect.x;
  rectT.y=rect.y+(xobj->value2-1)*(rect.height-8)/NbCell;
  if (NbVisCell<NbCell)
   rectT.height=NbVisCell*(rect.height-8)/NbCell+8;
  rectT.width=rect.width;
  if(PtInRect(pt,rectT))
  {
   PosMouse=pt.y-rectT.y-HeightCell/2+2;
   do
   {
    XQueryPointer(xobj->display,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
    /* Calcul de l'id de la premiere cellule */
    pt.y=y2-xobj->y-PosMouse;
    NPosCell=(pt.y-rect.y)*NbCell/(rect.height);
    
    if (NPosCell<1) NPosCell=1;
    if (NbCell>NbVisCell)
    {
     if (NPosCell>(NbCell-NbVisCell+1))
      NPosCell=NbCell-NbVisCell+1;
    }
    else
     NPosCell=1;
    if (xobj->value2!=NPosCell)
    {
     xobj->value2=NPosCell;
     DrawCellule(xobj,NbCell,NbVisCell,HeightCell,asc);
     DrawVSbList(xobj,NbCell,NbVisCell,0);
    }
    FD_ZERO(&in_fdset);
    FD_SET(x_fd,&in_fdset);
    select(32, &in_fdset, NULL, NULL, NULL);
   }
   while (!XCheckTypedEvent(xobj->display,ButtonRelease,&event));
  }
  else if (pt.y<rectT.y)
  {
   NPosCell=xobj->value2-NbVisCell;
   if (NPosCell<1) NPosCell=1;
   if (xobj->value2!=NPosCell)
   {
    xobj->value2=NPosCell;
    DrawCellule(xobj,NbCell,NbVisCell,HeightCell,asc);
    DrawVSbList(xobj,NbCell,NbVisCell,0);
   }
  }
  else if (pt.y>rectT.y+rectT.height)
  {
   NPosCell=xobj->value2+NbVisCell;
   if (NbCell>NbVisCell)
   {
    if (NPosCell>(NbCell-NbVisCell+1))
     NPosCell=NbCell-NbVisCell+1;
   }
   else NPosCell=1;
   if (xobj->value2!=NPosCell)
   {
    xobj->value2=NPosCell;
    DrawCellule(xobj,NbCell,NbVisCell,HeightCell,asc);
    DrawVSbList(xobj,NbCell,NbVisCell,0);
   }
  }
 } 
}

void EvtKeyList(struct XObj *xobj,XKeyEvent *EvtKey)
{
}


void ProcessMsgList(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}


