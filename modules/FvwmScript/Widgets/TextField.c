/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#include "Tools.h"

/***********************************************/
/* Fonction for TextField		       */
/***********************************************/
void InitTextField(struct XObj *xobj)
{
 unsigned long mask;
 XSetWindowAttributes Attr;
 int i;

 /* Enregistrement des couleurs et de la police */
 /* colors and fonts */
 if (xobj->colorset >= 0) {
  xobj->TabColor[fore] = Colorset[xobj->colorset].fg;
  xobj->TabColor[back] = Colorset[xobj->colorset].bg;
  xobj->TabColor[hili] = Colorset[xobj->colorset].hilite;
  xobj->TabColor[shad] = Colorset[xobj->colorset].shadow;
 } else {
  xobj->TabColor[fore] = GetColor(xobj->forecolor);
  xobj->TabColor[back] = GetColor(xobj->backcolor);
  xobj->TabColor[hili] = GetColor(xobj->hilicolor);
  xobj->TabColor[shad] = GetColor(xobj->shadcolor);
 }

 mask=0;
 Attr.cursor=XCreateFontCursor(dpy,XC_xterm);
 mask|=CWCursor;		/* Curseur pour la fenetre / window cursor */
 Attr.background_pixel=xobj->TabColor[back];
 mask|=CWBackPixel;

 xobj->win=XCreateWindow(dpy,*xobj->ParentWin,
		xobj->x,xobj->y,xobj->width,xobj->height,0,
		CopyFromParent,InputOutput,CopyFromParent,
		mask,&Attr);
 xobj->gc=fvwmlib_XCreateGC(dpy,xobj->win,0,NULL);
 XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);

 if ((xobj->Ffont = FlocaleLoadFont(dpy, xobj->font, ScriptName)) == NULL)
 {
   fprintf(stderr, "%s: Couldn't load font. Exiting!\n", ScriptName);
   exit(1);
 }
 if (xobj->Ffont->font != NULL)
   XSetFont(dpy, xobj->gc, xobj->Ffont->font->fid);

 XSetLineAttributes(dpy,xobj->gc,1,LineSolid,CapRound,JoinMiter);
 /* value2 représente la fin de la zone selectionnee */
 /* value2 gives the end of the selected zone */
 if (xobj->value>strlen(xobj->title))
  xobj->value=strlen(xobj->title);
 xobj->value2=xobj->value;
 /* left position of the visible title */
 xobj->value3=0;

 /* Redimensionnement du widget */
 /* widget resizing */
 xobj->height= xobj->Ffont->height + 10;
 i = FlocaleTextWidth(xobj->Ffont,xobj->title,strlen(xobj->title))+40;
 if (xobj->width<i)
  xobj->width=i;
 XResizeWindow(dpy,xobj->win,xobj->width,xobj->height);
 if (xobj->colorset >= 0)
   SetWindowBackground(dpy, xobj->win, xobj->width, xobj->height,
		       &Colorset[xobj->colorset], Pdepth,
		       xobj->gc, True);
 XSelectInput(dpy, xobj->win, ExposureMask);
}

void DestroyTextField(struct XObj *xobj)
{
 FlocaleUnloadFont(dpy,xobj->Ffont);
 XFreeGC(dpy,xobj->gc);
 XDestroyWindow(dpy,xobj->win);
}

/* Dessin du curseur du texte */
/* text cursor drawing */
void DrawPointTxt(struct XObj *xobj,unsigned int pixel)
{
#define dec 2
 int x,y;
 XSegment segm[2];

 x = FlocaleTextWidth(xobj->Ffont, xobj->title + xobj->value3,
	      xobj->value - xobj->value3)+5;
 y = xobj->Ffont->ascent + 5;

 segm[0].x1=x;
 segm[0].y1=y;
 segm[0].x2=x-dec;
 segm[0].y2=y+dec;
 segm[1].x1=x;
 segm[1].y1=y;
 segm[1].x2=x+dec;
 segm[1].y2=y+dec;
 XSetForeground(dpy,xobj->gc,pixel);
 XDrawSegments(dpy,xobj->win,xobj->gc,segm,2);
}

/* Dessin du contenu du champs texte */
/* text field drawing */
void DrawTextField(struct XObj *xobj)
{
 int x1,y1;
 int x2,l;
 int nl=0;
 int right=0;

 y1 = xobj->Ffont->ascent;
 l=strlen(xobj->title);
 if (xobj->value>l)
  xobj->value=l;
 if (xobj->value2>l)
  xobj->value2=l;
 DrawReliefRect(0,0,xobj->width,xobj->height,xobj,shad,hili);
 XClearArea(dpy,xobj->win,2,2,xobj->width-4,xobj->height-4,False);
 XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
 /* calcul du premier caractere visible */
 /* computation of the first visible character */
 while (l-nl >= 1 &&
	 FlocaleTextWidth(xobj->Ffont,xobj->title + nl,
			  xobj->value - nl) > (xobj->width-10))
   { nl++; }
 if (nl>xobj->value3) { xobj->value3 = nl; }
 else if (xobj->value3>xobj->value) { xobj->value3--; }
 /* calcul de la longueur du titre visible */
 /* computation of the length of the visible title */
 while ( l-xobj->value3-right >= 1 &&
	 FlocaleTextWidth(xobj->Ffont,xobj->title + xobj->value3,
			  l - xobj->value3 - right) > (xobj->width-10))
 { right++; }
 FwinString->win = xobj->win;
 FwinString->gc = xobj->gc;
 FwinString->x = 5;
 FwinString->y = y1+5;
 FwinString->str = xobj->title + xobj->value3;
 FwinString->len = strlen(xobj->title) - xobj->value3-right;
 FlocaleDrawString(dpy, xobj->Ffont, FwinString, FWS_HAVE_LENGTH);
#if 0
 XmbDrawString(dpy,xobj->win,xobj->xfontset,xobj->gc,5,y1+5,
	       xobj->title + xobj->value3,
	       strlen(xobj->title) - xobj->value3-right);
#endif

 /* Dessin de la zone selectionnee */
 /* selected zone drawing */
 XSetFunction(dpy,xobj->gc,GXinvert);
 if (xobj->value2>xobj->value)		/* Curseur avant la souris */
 {
  x1=FlocaleTextWidth(xobj->Ffont,&xobj->title[xobj->value3],
		      xobj->value - xobj->value3);
  x2=FlocaleTextWidth(xobj->Ffont,&xobj->title[xobj->value],
		      xobj->value2 - xobj->value);
 }
 else		/* Curseur apres la souris / cursor after the mouse */
 {
  x1=FlocaleTextWidth(xobj->Ffont,&xobj->title[xobj->value3],
		      xobj->value2 - xobj->value3);
  x2=FlocaleTextWidth(xobj->Ffont,&xobj->title[xobj->value2],
		      xobj->value - xobj->value2);
 }
 XFillRectangle(dpy,xobj->win,xobj->gc,x1+5,7,x2,y1+xobj->Ffont->descent-2);
 XSetFunction(dpy,xobj->gc,GXcopy);

 /* Dessin du point d'insertion */
 /* insertion point drawing */
 DrawPointTxt(xobj,xobj->TabColor[fore]);
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
 unsigned char *donnees=(unsigned char *)"";
 XRectangle rect;

 /* On deplace le curseur a la position de la souris */
 /* On recupere la position de la souris */
 /* We move the cursor at mouse position and we get the mouse position */
 switch (EvtButton->button)
 {
  case Button1:
    XQueryPointer(dpy,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
    x2=x2-xobj->x;
    PosCurs=0;
    while ((PosCurs<strlen(xobj->title+xobj->value3))&&
	   (x2>FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,PosCurs)+8))
     PosCurs++;
    DrawPointTxt(xobj,xobj->TabColor[back]);
    xobj->value=PosCurs+xobj->value3;
    xobj->value2=PosCurs+xobj->value3;
    DrawPointTxt(xobj,xobj->TabColor[fore]);
    DrawTextField(xobj);

    while (ButPress)
    {
     XNextEvent(dpy, &event);
     switch (event.type)
     {
      case MotionNotify:
       XQueryPointer(dpy,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
       x2=x2-xobj->x;
       PosCurs=0;
       while ((PosCurs<strlen(xobj->title+xobj->value3))&&
	      (x2 >
	       FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,PosCurs)+8))
	PosCurs++;
       /* Limitation de la zone de dessin */
       /* limitation of the drawing zone */
       if (PosCurs>xobj->value2)
       {
	rect.x=
	  FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,xobj->value2);
	rect.y=0;
	rect.width=
	  FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,PosCurs+1)
	  -rect.x+1;
	rect.height=xobj->height;
	xobj->value2=PosCurs+xobj->value3;
	DrawTextField(xobj);
       }
       else
       if (PosCurs<xobj->value2)
       {
	rect.x=FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,PosCurs)-1;
	rect.y=0;
	rect.width=FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,
			      xobj->value2+1) - rect.x+2;
	rect.height=xobj->height;
	xobj->value2=PosCurs+xobj->value3;
	DrawTextField(xobj);
       }

      break;
      case ButtonRelease:
       ButPress=0;
      break;
     }
    }
    /* Enregistrement de la selection dans le presse papier */
    /* Le programme devient proprietaire de la selection */
    /* selection stuff: get the selection */
    if (xobj->value!=xobj->value2)
    {
     str=(char*)GetText(xobj,xobj->value2);
     for (i=0;i<=7;i++)
      XStoreBuffer(dpy,str,strlen(str),i);
     Scrapt=(char*)realloc((void*)Scrapt,(strlen(str)+2)*sizeof(char));
     Scrapt=strcpy(Scrapt,str);
     free(str);
     x11base->HaveXSelection=True;
     XSetSelectionOwner(dpy,XA_PRIMARY,x11base->win,EvtButton->time);
     SelectOneTextField(xobj);
    }
   break;

   case Button2:			/* Colle le texte */
     /* Si l'application possede pas la selection, elle la demande */
     /* sinon elle lit son presse papier */
     /* read the selection */
    if (!x11base->HaveXSelection)
    {
      /* Demande de la selection */
      /* ask for the selection */
     XConvertSelection(dpy,XA_PRIMARY,XA_STRING,propriete,*xobj->ParentWin,
			EvtButton->time);
     while (!(XCheckTypedEvent(dpy,SelectionNotify,&event)))
      ;
     if (event.xselection.property!=None)
      if (event.xselection.selection==XA_PRIMARY)
      {
       XGetWindowProperty(dpy,event.xselection.requestor,
			  event.xselection.property,0,8192,False,
			  event.xselection.target,&type,&format,
			  &longueur,&octets_restant,&donnees);
       if (longueur>0)
       {
	Scrapt=(char*)realloc((void*)Scrapt,(longueur+1)*sizeof(char));
	Scrapt=strcpy(Scrapt,(char *)donnees);
	XDeleteProperty(dpy,event.xselection.requestor,
			event.xselection.property);
	XFree(donnees);
       }
      }
     }
    SizeBuf=strlen(Scrapt);
    if (SizeBuf>0)
    {
     NewPos=InsertText(xobj,Scrapt,SizeBuf);
     DrawPointTxt(xobj,xobj->TabColor[back]);
     xobj->value=NewPos;
     xobj->value2=NewPos;
     DrawPointTxt(xobj,xobj->TabColor[fore]);
     DrawTextField(xobj);
     SendMsg(xobj,SingleClic);
    }
    break;

   case Button3:		/* Appuie sur le troisieme bouton */
    XQueryPointer(dpy,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
    x2=x2-xobj->x;
    PosCurs=0;
    while ((PosCurs<strlen(xobj->title))&&
	   (x2>FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,PosCurs)+8))
     PosCurs++;
    if ((PosCurs<xobj->value) && (xobj->value<xobj->value2))
      xobj->value=xobj->value2;
    if ((PosCurs>xobj->value) && (xobj->value>xobj->value2))
      xobj->value=xobj->value2;
    xobj->value2=PosCurs+xobj->value3;
    DrawTextField(xobj);

    while (ButPress)
    {
     XNextEvent(dpy, &event);
     switch (event.type)
     {
      case MotionNotify:
       XQueryPointer(dpy,*xobj->ParentWin,&Win1,&Win2,&x1,&y1,&x2,&y2,&modif);
       x2=x2-xobj->x;
       while ((PosCurs<strlen(xobj->title))&&
	      (x2 >
	       FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,PosCurs)+8))
	PosCurs++;
       if (PosCurs>xobj->value2)
       {
	rect.x=
	  FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,xobj->value2);
	rect.y=0;
	rect.width=
	  FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,PosCurs+1)
	  -rect.x+1;
	rect.height=xobj->height;
	xobj->value2=PosCurs;
	DrawTextField(xobj);
       }
       else
       if (PosCurs<xobj->value2)
       {
	rect.x=FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,PosCurs)-1;
	rect.y=0;
	rect.width=FlocaleTextWidth(xobj->Ffont,xobj->title+xobj->value3,
			      xobj->value2+1)-rect.x+2;
	rect.height=xobj->height;
	xobj->value2=PosCurs+xobj->value3;
	DrawTextField(xobj);
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
      XStoreBuffer(dpy,str,strlen(str),i);
     Scrapt=(char*)realloc((void*)Scrapt,(strlen(str)+2)*sizeof(char));
     Scrapt=strcpy(Scrapt,str);
     free(str);
     x11base->HaveXSelection=True;
     XSetSelectionOwner(dpy,XA_PRIMARY,x11base->win,EvtButton->time);
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
    /* a normal character: insertion in the title */
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
  XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
  x2=FlocaleTextWidth(xobj->Ffont,xobj->title,strlen(xobj->title));
  XFillRectangle(dpy,xobj->win,xobj->gc,x2+4,4,xobj->width-x2-8,
		 xobj->height-8);
  xobj->value=NewPos;
  xobj->value2=NewPos;
  DrawPointTxt(xobj,xobj->TabColor[fore]);
  DrawTextField(xobj);
 }
 free(carks);
}

void ProcessMsgTextField(struct XObj *xobj,unsigned long type,unsigned long *body)
{
}
