#include "Tools.h"


/***********************************************/
/* Fonction for TextField                      */
/***********************************************/
void InitTextField(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;
 int i;
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
 Attr.cursor=XCreateFontCursor(xobj->display,XC_xterm);
 mask|=CWCursor;		/* Curseur pour la fenetre */
 Attr.background_pixel=xobj->TabColor[back].pixel;
 mask|=CWBackPixel;

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
 /* value2 représente la fin de la zone selectionnee */
 if (xobj->value>strlen(xobj->title))
  xobj->value=strlen(xobj->title);
 xobj->value2=xobj->value;

 /* Redimensionnement du widget */
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 xobj->height=asc+desc+10;
 i=XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title))+40;
 if (xobj->width<i)
  xobj->width=i;
 XResizeWindow(xobj->display,xobj->win,xobj->width,xobj->height);

}

void DestroyTextField(struct XObj *xobj)
{
 XFreeFont(xobj->display,xobj->xfont);
 XFreeGC(xobj->display,xobj->gc);
 XDestroyWindow(xobj->display,xobj->win);
}

/* Dessin du curseur du texte */
void DrawPointTxt(struct XObj *xobj,unsigned int color)
{
 #define dec 2
 int x,y;
 XSegment segm[2];
 int asc,desc,dir;
 XCharStruct struc;

 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 x=XTextWidth(xobj->xfont,xobj->title,xobj->value)+5;
 y=asc+5;

 segm[0].x1=x;
 segm[0].y1=y;
 segm[0].x2=x-dec;
 segm[0].y2=y+dec;
 segm[1].x1=x;
 segm[1].y1=y;
 segm[1].x2=x+dec;
 segm[1].y2=y+dec;
 XSetForeground(xobj->display,xobj->gc,color);
 XDrawSegments(xobj->display,xobj->win,xobj->gc,segm,2);
}

/* Dessin du contenu du champs texte */
void DrawTextField(struct XObj *xobj)
{
 int x1,y1;
 int x2,l;
 int desc,dir,asc;
 XCharStruct struc;

 l=strlen(xobj->title);
 if (xobj->value>l)
  xobj->value=l;
 if (xobj->value2>l)
  xobj->value2=l;
 DrawReliefRect(0,0,xobj->width,xobj->height,xobj,
	xobj->TabColor[shad].pixel,xobj->TabColor[li].pixel,xobj->TabColor[black].pixel,1);
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
 XFillRectangle(xobj->display,xobj->win,xobj->gc,3,3,xobj->width-6,xobj->height-6);
 XSetForeground(xobj->display,xobj->gc,xobj->TabColor[fore].pixel);
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&y1,&desc,&struc);
 XDrawImageString(xobj->display,xobj->win,xobj->gc,5,y1+5,xobj->title,strlen(xobj->title));

 /* Dessin de la zone selectionnee */
 XSetFunction(xobj->display,xobj->gc,GXinvert);
 if (xobj->value2>xobj->value)		/* Curseur avant la souris */
 {
  x1=XTextWidth(xobj->xfont,&xobj->title[0],xobj->value);
  x2=XTextWidth(xobj->xfont,&xobj->title[xobj->value],xobj->value2-xobj->value);
 }
 else		/* Curseur apres la souris */
 {
  x1=XTextWidth(xobj->xfont,&xobj->title[0],xobj->value2);
  x2=XTextWidth(xobj->xfont,&xobj->title[xobj->value2],xobj->value-xobj->value2);
 }
 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 XFillRectangle(xobj->display,xobj->win,xobj->gc,x1+5,7,x2,y1+desc-2);
 XSetFunction(xobj->display,xobj->gc,GXcopy);

 /* Dessin du point d'insertion */
 DrawPointTxt(xobj,xobj->TabColor[fore].pixel);
}

void EvtMouseTextField(struct XObj *xobj,XButtonEvent *EvtButton)
{
 unsigned int modif;
 int x1,x2,y1,y2,i;
 Window Win1,Win2;
 int PosCurs=0;
 int SizeBuf;
 char *str;
 int NewPos;
 Atom type;
 XEvent event;
 int ButPress=1;
 int format;
 unsigned long longueur,octets_restant;
 unsigned char *donnees="";
 XRectangle rect;

 /* On deplace le curseur a la position de la souris */
 /* On recupere la position de la souris */
 switch (EvtButton->button)
 {
  case Button1:
    XQueryPointer(xobj->display,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
    x2=x2-xobj->x;
    PosCurs=0;
    while ((PosCurs<strlen(xobj->title))&&(x2>XTextWidth(xobj->xfont,xobj->title,PosCurs)+8))
     PosCurs++;
    DrawPointTxt(xobj,xobj->TabColor[back].pixel);
    xobj->value=PosCurs;
    xobj->value2=PosCurs;
    DrawPointTxt(xobj,xobj->TabColor[fore].pixel);
    DrawTextField(xobj);

    while (ButPress)
    {
     XNextEvent(xobj->display, &event);
     switch (event.type)
     {
      case MotionNotify:
       XQueryPointer(xobj->display,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
       x2=x2-xobj->x;
       PosCurs=0;
       while ((PosCurs<strlen(xobj->title))&&(x2>XTextWidth(xobj->xfont,xobj->title,PosCurs)+8))
     	PosCurs++;
       /* Limitation de la zone de dessin */
       if (PosCurs>xobj->value2)
       {
        rect.x=XTextWidth(xobj->xfont,xobj->title,xobj->value2);
        rect.y=0;
        rect.width=XTextWidth(xobj->xfont,xobj->title,PosCurs+1)-rect.x+1;
        rect.height=xobj->height;
        XSetClipRectangles(xobj->display,xobj->gc,0,0,&rect,1,Unsorted);
        xobj->value2=PosCurs;
        DrawTextField(xobj);
        XSetClipMask(xobj->display,xobj->gc,None);
       }
       else
       if (PosCurs<xobj->value2)
       {
        rect.x=XTextWidth(xobj->xfont,xobj->title,PosCurs)-1;
        rect.y=0;
        rect.width=XTextWidth(xobj->xfont,xobj->title,xobj->value2+1)-rect.x+2;
        rect.height=xobj->height;
        XSetClipRectangles(xobj->display,xobj->gc,0,0,&rect,1,Unsorted);
        xobj->value2=PosCurs;
        DrawTextField(xobj);
        XSetClipMask(xobj->display,xobj->gc,None);
       }

      break;
      case ButtonRelease:
       ButPress=0;
      break;
     }
    }
    XSetClipMask(xobj->display,xobj->gc,None);
    /* Enregistrement de la selection dans le presse papier */
    /* Le programme devient proprietaire de la selection */
    if (xobj->value!=xobj->value2)
    {
     str=(char*)GetText(xobj,xobj->value2);
     for (i=0;i<=7;i++)
      XStoreBuffer(xobj->display,str,strlen(str),i);
     Scrapt=(char*)realloc((void*)Scrapt,(strlen(str)+2)*sizeof(char));
     Scrapt=strcpy(Scrapt,str);
     free(str);
     x11base->HaveXSelection=True;
     XSetSelectionOwner(x11base->display,XA_PRIMARY,x11base->win,EvtButton->time);
     SelectOneTextField(xobj);
    }
   break;

   case Button2:			/* Colle le texte */
    /* Si l'application possede pas la selection, elle la demande */
    /* sinon elle lit son presse papier */
    if (!x11base->HaveXSelection)
    {
     /* Demande de la selection */
     XConvertSelection(xobj->display,XA_PRIMARY,XA_STRING,propriete,*xobj->ParentWin,
    			EvtButton->time);
     while (!(XCheckTypedEvent(xobj->display,SelectionNotify,&event)))
      ;
     if (event.xselection.property!=None)
      if (event.xselection.selection==XA_PRIMARY)
      {
       XGetWindowProperty(xobj->display,event.xselection.requestor,event.xselection.property,0,
      		8192,False,event.xselection.target,&type,&format,&longueur,&octets_restant,
      		&donnees);
       if (longueur>0)
       {
        Scrapt=(char*)realloc((void*)Scrapt,(longueur+1)*sizeof(char));
        Scrapt=strcpy(Scrapt,donnees);
        XDeleteProperty(xobj->display,event.xselection.requestor,event.xselection.property);
        XFree(donnees);
       }
      }
     }
    SizeBuf=strlen(Scrapt);
    if (SizeBuf>0)
    {
     NewPos=InsertText(xobj,Scrapt,SizeBuf);
     DrawPointTxt(xobj,xobj->TabColor[back].pixel);
     xobj->value=NewPos;
     xobj->value2=NewPos;
     DrawPointTxt(xobj,xobj->TabColor[fore].pixel);
     DrawTextField(xobj);
     SendMsg(xobj,SingleClic);
    }
    break;

   case Button3:		/* Appuie sur le troisieme bouton */
    XQueryPointer(xobj->display,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
    x2=x2-xobj->x;
    PosCurs=0;
    while ((PosCurs<strlen(xobj->title))&&
 		(x2>XTextWidth(xobj->xfont,xobj->title,PosCurs)+8))
     PosCurs++;
    if ((PosCurs<xobj->value) && (xobj->value<xobj->value2))
      xobj->value=xobj->value2;
    if ((PosCurs>xobj->value) && (xobj->value>xobj->value2))
      xobj->value=xobj->value2;
    xobj->value2=PosCurs;
    DrawTextField(xobj);

    while (ButPress)
    {
     XNextEvent(xobj->display, &event);
     switch (event.type)
     {
      case MotionNotify:
       XQueryPointer(xobj->display,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
       x2=x2-xobj->x;
       while ((PosCurs<strlen(xobj->title))&&(x2>XTextWidth(xobj->xfont,xobj->title,PosCurs)+8))
     	PosCurs++;
       if (PosCurs>xobj->value2)
       {
        rect.x=XTextWidth(xobj->xfont,xobj->title,xobj->value2);
        rect.y=0;
        rect.width=XTextWidth(xobj->xfont,xobj->title,PosCurs+1)-rect.x+1;
        rect.height=xobj->height;
        XSetClipRectangles(xobj->display,xobj->gc,0,0,&rect,1,Unsorted);
        xobj->value2=PosCurs;
        DrawTextField(xobj);
        XSetClipMask(xobj->display,xobj->gc,None);
       }
       else
       if (PosCurs<xobj->value2)
       {
        rect.x=XTextWidth(xobj->xfont,xobj->title,PosCurs)-1;
        rect.y=0;
        rect.width=XTextWidth(xobj->xfont,xobj->title,xobj->value2+1)-rect.x+2;
        rect.height=xobj->height;
        XSetClipRectangles(xobj->display,xobj->gc,0,0,&rect,1,Unsorted);
        xobj->value2=PosCurs;
        DrawTextField(xobj);
        XSetClipMask(xobj->display,xobj->gc,None);
       }
       PosCurs=0;
      break;
      case ButtonRelease:
       ButPress=0;
      break;
     }
    }
    if (xobj->value!=xobj->value2)
    {
     str=(char*)GetText(xobj,xobj->value2);
     for (i=0;i<=7;i++)
      XStoreBuffer(xobj->display,str,strlen(str),i);
     Scrapt=(char*)realloc((void*)Scrapt,(strlen(str)+2)*sizeof(char));
     Scrapt=strcpy(Scrapt,str);
     free(str);
     x11base->HaveXSelection=True;
     XSetSelectionOwner(x11base->display,XA_PRIMARY,x11base->win,EvtButton->time);
    }
   break;
 }
}

void EvtKeyTextField(struct XObj *xobj,XKeyEvent *EvtKey)
{
 int i,x2;
 char car[11];
 char* carks2;
 char* carks = NULL;
 KeySym ks;
 int Size;
 int NewPos;

 /* Recherche du charactere */
 i=XLookupString(EvtKey,car,10,&ks,NULL);
 NewPos=xobj->value;
 car[i]='\0';
 carks2=XKeysymToString(ks);

 if (carks2!=NULL)
 {
  carks=(char*)calloc(sizeof(char),30);
  sprintf(carks,"%s",carks2);
  if (strcmp(carks,"Right")==0)
  {
   NewPos++;
   if (NewPos>strlen(xobj->title))
    NewPos=strlen(xobj->title);
  }
  else if (strcmp(carks,"Left")==0)
  {
   NewPos--;
   if (NewPos<0)
    NewPos=0;
  }
  else if (strcmp(carks,"Return")==0)
  {
   ;
  }
  else if ((strcmp(carks,"Delete")==0)||(strcmp(carks,"BackSpace")==0))
  {
   if (NewPos>0)
   {
    Size=strlen(xobj->title);
    memmove(&xobj->title[NewPos-1],&xobj->title[NewPos],
			Size-NewPos+1);
    xobj->title=(char*)realloc(xobj->title,(Size)*sizeof(char));
    NewPos--;
    SendMsg(xobj,SingleClic);
   }
  }
  else if (i!=0)	/* Cas d'un caractere normal */
  {
   /* Insertion du caractere dans le titre */
   Size=strlen(xobj->title);
   xobj->title=(char*)realloc(xobj->title,(2+Size)*sizeof(char));
   memmove(&xobj->title[NewPos+1],&xobj->title[NewPos],
			Size-NewPos+1);
   xobj->title[NewPos]=car[0];
   NewPos++;
   SendMsg(xobj,SingleClic);
  }

 }

 if ((xobj->value!=NewPos)||(xobj->value2!=NewPos))
 {
  XSetForeground(xobj->display,xobj->gc,xobj->TabColor[back].pixel);
  x2=XTextWidth(xobj->xfont,xobj->title,strlen(xobj->title));
  XFillRectangle(xobj->display,xobj->win,xobj->gc,x2+4,4,xobj->width-x2-8,xobj->height-8);
  xobj->value=NewPos;
  xobj->value2=NewPos;
  DrawPointTxt(xobj,xobj->TabColor[fore].pixel);
  DrawTextField(xobj);
 }
 free(carks);
}

void ProcessMsgTextField(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}



