#include "Tools.h"

/***********************************************/
/***********************************************/
/* Fonction pour PopupMenu                     */
/***********************************************/
/***********************************************/



void InitPopupMenu(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;
 int i;
 char *str;
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
 xobj->value3=CountOption(xobj->title);
 if (xobj->value>xobj->value3)
  xobj->value=xobj->value3;
 if (xobj->value<1)
  xobj->value=1;
  
 /* Redimensionnement du widget */
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 xobj->height=asc+desc+12;
 xobj->width=30;
 for (i=1;i<=xobj->value3;i++)
 {
  str=(char*)GetMenuTitle(xobj->title,i);
  if (xobj->width<XTextWidth(xobj->xfont,str,strlen(str))+34)
   xobj->width=XTextWidth(xobj->xfont,str,strlen(str))+34;
  free(str);
 }
 XResizeWindow(xobj->display,xobj->win,xobj->width,xobj->height);
}

void DestroyPopupMenu(struct XObj *xobj)
{
 XFreeFont(xobj->display,xobj->xfont);
 XFreeGC(xobj->display,xobj->gc);
 XDestroyWindow(xobj->display,xobj->win);
}

void DrawPopupMenu(struct XObj *xobj)
{
 XSegment segm[4];
 char* str;
 int x,y;
 int asc,desc,dir;
 XCharStruct struc;
 
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
   xobj->TabColor[li].pixel,xobj->TabColor[shad].pixel,xobj->TabColor[black].pixel,0);
   
 /* Dessin de la fleche */
 segm[0].x1=7;
 segm[0].y1=asc;
 segm[0].x2=19;
 segm[0].y2=asc;
 segm[1].x1=8;
 segm[1].y1=asc;
 segm[1].x2=13;
 segm[1].y2=5+asc;
 segm[2].x1=6;
 segm[2].y1=asc-1;
 segm[2].x2=19;
 segm[2].y2=0+asc-1;
 segm[3].x1=7;
 segm[3].y1=asc;
 segm[3].x2=12;
 segm[3].y2=5+asc;
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[shad].pixel);
 XDrawSegments(xobj->display,xobj->win,xobj->gc,segm,4);
 segm[0].x1=17;
 segm[0].y1=asc+1;
 segm[0].x2=13;
 segm[0].y2=5+asc;
 segm[1].x1=19;
 segm[1].y1=asc;
 segm[1].x2=14;
 segm[1].y2=5+asc;
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[li].pixel);
 XDrawSegments(xobj->display,xobj->win,xobj->gc,segm,2);

 /* Dessin du titre du popup menu */
 str=(char*)GetMenuTitle(xobj->title,xobj->value);
 x=25;
 y=asc+5;
 DrawString(xobj->display,xobj->gc,xobj->win,x,y,str,
	strlen(str),xobj->TabColor[fore].pixel,xobj->TabColor[li].pixel,
	xobj->TabColor[back].pixel,
	!xobj->flags[1]);

 free(str);
}

void EvtMousePopupMenu(struct XObj *xobj,XButtonEvent *EvtButton)
{
 static XEvent event;
 int x,y,hOpt,yMenu,hMenu;
 int x1,y1,x2,y2,oldy;
 int oldvalue,newvalue;
 Window Win1,Win2,WinPop;
 unsigned int modif;
 unsigned long mask;
 XSetWindowAttributes Attr;
 int asc,desc,dir;
 XCharStruct struc;
 
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 hOpt=asc+desc+10;
 yMenu=xobj->y-((xobj->value-1)*hOpt);
 hMenu=xobj->value3*hOpt;
 
 /* Creation de la fenetre menu */
 XTranslateCoordinates(xobj->display,*xobj->ParentWin,
 		XRootWindow(xobj->display,XDefaultScreen(xobj->display)),xobj->x,yMenu,&x,&y,&Win1);
 if (x<0) x=0;
 if (y<0) y=0;
 if (x+xobj->width>XDisplayWidth(xobj->display,XDefaultScreen(xobj->display)))
  x=XDisplayWidth(xobj->display,XDefaultScreen(xobj->display))-xobj->width;
 if (y+hMenu>XDisplayHeight(xobj->display,XDefaultScreen(xobj->display)))
  y=XDisplayHeight(xobj->display,XDefaultScreen(xobj->display))-hMenu;
 mask=0;
 Attr.background_pixel=xobj->TabColor[back].pixel;
 mask|=CWBackPixel;
 Attr.cursor=XCreateFontCursor(xobj->display,XC_hand2); 
 mask|=CWCursor;		/* Curseur pour la fenetre */
 Attr.override_redirect=True;
 mask|=CWOverrideRedirect;
 WinPop=XCreateWindow(xobj->display,XRootWindow(xobj->display,XDefaultScreen(xobj->display)),
 	x,y,xobj->width-2,hMenu,1,
 	CopyFromParent,InputOutput,CopyFromParent,mask,&Attr);
 XMapRaised(xobj->display,WinPop);

  /* Dessin du menu */
 DrawPMenu(xobj,WinPop,hOpt,1);
 do
 {
  XQueryPointer(xobj->display,XRootWindow(xobj->display,XDefaultScreen(xobj->display)),
  			&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
  /* Determiner l'option courante */
  y2=y2-y;
  x2=x2-x;
  {
   oldy=y2;
   /* calcule de xobj->value */
   if ((x2>0)&&(x2<xobj->width)&&(y2>0)&&(y2<hMenu))
    newvalue=y2/hOpt+1;
   else
    newvalue=0;
   if (newvalue!=oldvalue)
   {
    SelectMenu(xobj,WinPop,hOpt,oldvalue,0);
    SelectMenu(xobj,WinPop,hOpt,newvalue,1);
    oldvalue=newvalue;
   }
  } 
 }
 while (!XCheckTypedEvent(xobj->display,ButtonRelease,&event));
 XDestroyWindow(xobj->display,WinPop);
 if (newvalue!=0)
 {
  xobj->value=newvalue;
  SendMsg(xobj,SingleClic);
 }
 xobj->DrawObj(xobj);
}


void EvtKeyPopupMenu(struct XObj *xobj,XKeyEvent *EvtKey)
{
}

void ProcessMsgPopupMenu(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}














