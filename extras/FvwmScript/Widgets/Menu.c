#include "Tools.h"


/***********************************************/
/* Fonction pour Menu                      */
/***********************************************/
void InitMenu(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;
 int asc,desc,dir;
 XCharStruct struc;
 int i;
 char *Option;

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
 mask|=CWCursor;
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

 /* Size */
 Option=GetMenuTitle(xobj->title,0);
 xobj->width=XTextWidth(xobj->xfont,Option,strlen(Option))+6;
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 xobj->height=asc+desc+4;
 free(Option);

 /* Position */
 xobj->y=3;
 xobj->x=2;
 i=0;
 while (xobj!=tabxobj[i])
 {
  if (tabxobj[i]->TypeWidget==Menu)
  {
   Option=GetMenuTitle(tabxobj[i]->title,0);
   xobj->x=xobj->x+XTextWidth(tabxobj[i]->xfont,Option,strlen(Option))+10;
   free(Option);
  }
  i++;
 }
 XResizeWindow(xobj->display,xobj->win,xobj->width,xobj->height);
 XMoveWindow(xobj->display,xobj->win,xobj->x,xobj->y);
}

void DestroyMenu(struct XObj *xobj)
{
 XFreeFont(xobj->display,xobj->xfont);
 XFreeGC(xobj->display,xobj->gc);
 XDestroyWindow(xobj->display,xobj->win);
}

void DrawMenu(struct XObj *xobj)
{
 XSegment segm[2];
 int i;

 /* Si c'est le premier menu, on dessine la bar de menu */
 if (xobj->x==2)
 {
  for (i=0;i<2;i++)
  {
   segm[0].x1=i;
   segm[0].y1=i;
   segm[0].x2=x11base->size.width-i-1;
   segm[0].y2=i;

   segm[1].x1=i;
   segm[1].y1=i;
   segm[1].x2=i;
   segm[1].y2=xobj->height-i+5;
   XSetForeground(xobj->display,xobj->gc,xobj->TabColor[li].pixel);
   XDrawSegments(xobj->display,x11base->win,xobj->gc,segm,2);
 
   segm[0].x1=1+i;
   segm[0].y1=xobj->height-i+5;
   segm[0].x2=x11base->size.width-i-1;
   segm[0].y2=xobj->height-i+5;
  
   segm[1].x1=x11base->size.width-i-1;
   segm[1].y1=i;
   segm[1].x2=x11base->size.width-i-1;
   segm[1].y2=xobj->height-i+5;
   XSetForeground(xobj->display,xobj->gc,xobj->TabColor[shad].pixel);
   XDrawSegments(xobj->display,x11base->win,xobj->gc,segm,2);
  }

 }
 DrawIconStr(0,xobj,True);
}

void EvtMouseMenu(struct XObj *xobj,XButtonEvent *EvtButton)
{
 static XEvent event;
 unsigned int modif;
 int x1,x2,y1,y2,i,oldy;
 Window Win1,Win2,WinPop;
 char *str;
 int x,y,hOpt,yMenu,hMenu,wMenu;
 int oldvalue,newvalue;
 unsigned long mask;
 XSetWindowAttributes Attr;
 int asc,desc,dir;
 XCharStruct struc;
 fd_set in_fdset;

 DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
   xobj->TabColor[shad].pixel,xobj->TabColor[li].pixel,xobj->TabColor[black].pixel,-1);

 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 hOpt=asc+desc+10;
 xobj->value3=CountOption(xobj->title);
 hMenu=(xobj->value3-1)*hOpt;			/* Hauteur totale du menu */
 yMenu=xobj->y+xobj->height+1;
 wMenu=0;
 for (i=2;i<=xobj->value3;i++)
 {
  str=(char*)GetMenuTitle(xobj->title,i);
  if (wMenu<XTextWidth(xobj->xfont,str,strlen(str))+20)
   wMenu=XTextWidth(xobj->xfont,str,strlen(str))+20;
  free(str);
 }

 /* Creation de la fenetre menu */
 XTranslateCoordinates(xobj->display,*xobj->ParentWin,
 		XRootWindow(xobj->display,XDefaultScreen(xobj->display)),xobj->x,yMenu,&x,&y,&Win1);
 if (x<0) x=0;
 if (y<0) y=0;
 if (x+wMenu>XDisplayWidth(xobj->display,XDefaultScreen(xobj->display)))
 x=XDisplayWidth(xobj->display,XDefaultScreen(xobj->display))-wMenu;
 if (y+hMenu>XDisplayHeight(xobj->display,XDefaultScreen(xobj->display)))
 {
  y=y-hMenu-xobj->height;
 }

 mask=0;
 Attr.background_pixel=xobj->TabColor[back].pixel;
 mask|=CWBackPixel;
 Attr.cursor=XCreateFontCursor(xobj->display,XC_hand2); 
 mask|=CWCursor;		/* Curseur pour la fenetre */
 Attr.override_redirect=True;
 mask|=CWOverrideRedirect;
 WinPop=XCreateWindow(xobj->display,XRootWindow(xobj->display,XDefaultScreen(xobj->display)),
	x,y,wMenu-5,hMenu,0,CopyFromParent,InputOutput,CopyFromParent,mask,&Attr);
 XMapRaised(xobj->display,WinPop);

 /* Dessin du menu */
 DrawPMenu(xobj,WinPop,hOpt,2);
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
   if ((x2>0)&&(x2<wMenu)&&(y2>0)&&(y2<hMenu))
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
  FD_ZERO(&in_fdset);
  FD_SET(x_fd,&in_fdset);
  select(32, &in_fdset, NULL, NULL, NULL);
 }
 while (!XCheckTypedEvent(xobj->display,ButtonRelease,&event));
 XDestroyWindow(xobj->display,WinPop);
 if (newvalue!=0)
 {
  xobj->value=newvalue;
  SendMsg(xobj,SingleClic);
  xobj->value=0;
 }
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
 XFillRectangle(xobj->display,xobj->win,xobj->gc,0,0,xobj->width,xobj->height);
 DrawIconStr(0,xobj,True);
 for (i=0;i<nbobj;i++)
  tabxobj[i]->DrawObj(tabxobj[i]);

}

void EvtKeyMenu(struct XObj *xobj,XKeyEvent *EvtKey)
{
}

void ProcessMsgMenu(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}









