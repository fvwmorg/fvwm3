#include "Tools.h"

/***********************************************/
/***********************************************/
/* Fonction pour PushButton                    */
/***********************************************/
/***********************************************/
void InitPushButton(struct XObj *xobj)
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
 Attr.cursor=XCreateFontCursor(xobj->display,XC_hand2); 
 mask|=CWCursor;
 Attr.background_pixel=xobj->TabColor[back].pixel;
 mask|=CWBackPixel;


 /* Epaisseur de la fenetre = 0 */
 xobj->win=XCreateWindow(xobj->display,*xobj->ParentWin,
		xobj->x,xobj->y,xobj->width,xobj->height,0,
		CopyFromParent,InputOutput,CopyFromParent,
		mask,&Attr);
 xobj->gc=XCreateGC(xobj->display,xobj->win,0,NULL);
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[fore].pixel);
 XSetBackground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
 if ((xobj->xfont=XLoadQueryFont(xobj->display,xobj->font))==NULL)
  {
   fprintf(stderr,"Can't load font %s\n",xobj->font);
  }
 else
  XSetFont(xobj->display,xobj->gc,xobj->xfont->fid);
 XSetLineAttributes(xobj->display,xobj->gc,1,LineSolid,CapRound,JoinMiter);
 
 /* Redimensionnement du widget */
 str=(char*)GetMenuTitle(xobj->title,1);
 if (xobj->icon==NULL)
 {
  XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
  i=asc+desc+12;
  if (xobj->height<i)
   xobj->height=i;
  i=XTextWidth(xobj->xfont,str,strlen(str))+16;
  if (xobj->width<i)
   xobj->width=i;
 }
 else if (strlen(str)==0)
 {
  if (xobj->height<xobj->icon_h+10)
   xobj->height=xobj->icon_h+10;
  if (xobj->width<xobj->icon_w+10)
   xobj->width=xobj->icon_w+10;
 }
 else
 {
  if (xobj->icon_w+10>XTextWidth(xobj->xfont,str,strlen(str))+16)
   i=xobj->icon_w+10;
  else
   i=XTextWidth(xobj->xfont,str,strlen(str))+16;
  if (xobj->width<i)
   xobj->width=i;
  i=xobj->icon_h+2*(asc+desc+10);
  if (xobj->height<i)
   xobj->height=i;
 }
 XResizeWindow(xobj->display,xobj->win,xobj->width,xobj->height);
 xobj->value3=CountOption(xobj->title);
}

void DestroyPushButton(struct XObj *xobj)
{
 XFreeFont(xobj->display,xobj->xfont);
 XFreeGC(xobj->display,xobj->gc);
 XDestroyWindow(xobj->display,xobj->win);
}


void DrawPushButton(struct XObj *xobj)
{
 DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
   xobj->TabColor[li].pixel,xobj->TabColor[shad].pixel,xobj->TabColor[black].pixel,0);
 DrawIconStr(0,xobj,True);
}

void EvtMousePushButton(struct XObj *xobj,XButtonEvent *EvtButton)
{
 static XEvent event;
 int End=1;
 unsigned int modif;
 int x1,x2,y1,y2,i,j,oldy;
 Window Win1,Win2,WinPop;
 Window WinBut=0;
 int In;
 char *str;
 int x,y,hOpt,yMenu,hMenu,wMenu;
 int oldvalue,newvalue;
 unsigned long mask;
 XSetWindowAttributes Attr;
 int asc,desc,dir;
 XCharStruct struc;


 if (EvtButton->button==Button1)
 {
  j=xobj->height/2+3;
  i=(xobj->width-XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title)))/2;

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
	    DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
		xobj->TabColor[shad].pixel,xobj->TabColor[li].pixel,xobj->TabColor[black].pixel,0);
	    DrawIconStr(1,xobj,True);
	    In=1;
	   }
	   else
	   {
	    if (Win2==WinBut)
	    {
	     DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
		xobj->TabColor[shad].pixel,xobj->TabColor[li].pixel,xobj->TabColor[black].pixel,0);
	     DrawIconStr(1,xobj,True);
	     In=1;
	    }
	    else if (In)
	    {
	     In=0;
	     DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
		xobj->TabColor[li].pixel,xobj->TabColor[shad].pixel,xobj->TabColor[black].pixel,0);
	     DrawIconStr(0,xobj,True);
	    }
	   }
	  break;
      case LeaveNotify:
	   XQueryPointer(xobj->display,*xobj->ParentWin,
		&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
	   if (Win2==WinBut)
	   {
	    In=1;
	    DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
		xobj->TabColor[shad].pixel,xobj->TabColor[li].pixel,xobj->TabColor[black].pixel,0);
	    DrawIconStr(1,xobj,True);
	   }
	   else if (In)
	   {
	    DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
		xobj->TabColor[li].pixel,xobj->TabColor[shad].pixel,xobj->TabColor[black].pixel,0);
	    DrawIconStr(0,xobj,True);
	    In=0;
	   }
	  break;
      case ButtonRelease:
	   End=0;
	   DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
		xobj->TabColor[li].pixel,xobj->TabColor[shad].pixel,xobj->TabColor[black].pixel,0);
	   DrawIconStr(0,xobj,True);
	   if (In)
	   {
	    /* Envoie d'un message vide de type SingleClic pour un clique souris */
	    xobj->value=1;
	    SendMsg(xobj,SingleClic);
	    xobj->value=0;
	   }
	  break;
     }
   }
  }
  else if (EvtButton->button==Button3)		/* affichage du popup menu */
   if (xobj->value3>1)
   {
    XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
    hOpt=asc+desc+10;
    hMenu=(xobj->value3-1)*hOpt;			/* Hauteur totale du menu */
    yMenu=xobj->y+xobj->height;
    wMenu=0;
    for (i=2;i<=xobj->value3;i++)
    {
     str=(char*)GetMenuTitle(xobj->title,i);
     if (wMenu<XTextWidth(xobj->xfont,str,strlen(str))+34)
      wMenu=XTextWidth(xobj->xfont,str,strlen(str))+34;
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
     /*y=XDisplayHeight(xobj->display,XDefaultScreen(xobj->display))-hMenu;*/
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
 	x,y,wMenu-5,hMenu,1,
 	CopyFromParent,InputOutput,CopyFromParent,mask,&Attr);
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
  }
  while (!XCheckTypedEvent(xobj->display,ButtonRelease,&event));
  XDestroyWindow(xobj->display,WinPop);
  if (newvalue!=0)
  {
   xobj->value=newvalue;
   SendMsg(xobj,SingleClic);
   xobj->value=0;
  }
  xobj->DrawObj(xobj);

 }
}

void EvtKeyPushButton(struct XObj *xobj,XKeyEvent *EvtKey)
{
 char car[11];
 char *carks2;
 char *carks;
 KeySym ks;
 int Size;

 Size=XLookupString(EvtKey,car,10,&ks,NULL);
 car[Size]='\0';
 carks2=XKeysymToString(ks);
 if (carks2!=NULL)
 {
  carks=(char*)calloc(sizeof(char),30);
  sprintf(carks,"%s",carks2);
  if (strcmp(carks,"Return")==0)
  {
   xobj->value=1;
   SendMsg(xobj,SingleClic);
   xobj->value=0;
  }
  free(carks);
 }
}

void ProcessMsgPushButton(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}









