/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Tools.h"

/***********************************************/
/* Fonction d'ecriture en relief               */
/***********************************************/
#ifdef I18N_MB
void FakeDrawString(XFontSet FONTSET,
		Display *dpy,GC gc,Window win,int x,int y,char *str,
		int strl,unsigned long ForeC,unsigned long HiC,
		unsigned long BackC,int WithRelief)
#else
void DrawString(Display *dpy,GC gc,Window win,int x,int y,char *str,
		int strl,unsigned long ForeC,unsigned long HiC,
		unsigned long BackC,int WithRelief)
#endif
{
 if (WithRelief)
 {
  XSetBackground(dpy,gc,BackC);
  XSetForeground(dpy,gc,HiC);
#ifdef I18N_MB
  XmbDrawImageString(dpy,win,FONTSET,gc,x+1,y+1,str,strl);
#else
  XDrawImageString(dpy,win,gc,x+1,y+1,str,strl);
#endif
  XSetForeground(dpy,gc,ForeC);
#ifdef I18N_MB
  XmbDrawString(dpy,win,FONTSET,gc,x,y,str,strl);
#else
  XDrawString(dpy,win,gc,x,y,str,strl);
#endif
 }
 else
 {
  XSetBackground(dpy,gc,BackC);
  XSetForeground(dpy,gc,ForeC);
#ifdef I18N_MB
  XmbDrawImageString(dpy,win,FONTSET,gc,x,y+1,str,strl);
#else
  XDrawImageString(dpy,win,gc,x,y+1,str,strl);
#endif
 }
}


/**************************************************/
/* Retourne le titre de l'option id du menu       */
/**************************************************/
char* GetMenuTitle(char *str,int id)
{
 int i=1;
 int w=0;
 int w2=0;
 char* TempStr;

 while ((str[w+w2]!='\0')&&(str[w+w2]!='|'))
  w2++;

 while ((i<id)&&(str[w]!='\0'))
 {
  i++;
  if (str[w+w2]=='|') w2++;
  w=w+w2;
  w2=0;
  while ((str[w+w2]!='\0')&&(str[w+w2]!='|'))
   w2++;
 }
 TempStr=(char*)calloc(sizeof(char),w2+1);
 TempStr=strncpy(TempStr,&str[w],w2);
 return TempStr;
}

/***********************************************************/
/* Dessine le contenu de la fenetre du popup-menu          */
/***********************************************************/
void DrawPMenu(struct XObj *xobj,Window WinPop,int h,int StrtOpt)
{
 XSegment segm[2];
 unsigned int i;
 char *str;
 int x,y;
 unsigned int width,height;
 Window Root;
 int asc,desc,dir;
 XCharStruct struc;

 XGetGeometry(dpy,WinPop,&Root,&x,&y,&width,&height,&i,&i);
 for (i=0;i<2;i++)
 {
  segm[0].x1=i;
  segm[0].y1=i;
  segm[0].x2=width-i-1;
  segm[0].y2=i;

  segm[1].x1=i;
  segm[1].y1=i;
  segm[1].x2=i;
  segm[1].y2=height-i-1;
  XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
  XDrawSegments(dpy,WinPop,xobj->gc,segm,2);

  segm[0].x1=1+i;
  segm[0].y1=height-i-1;
  segm[0].x2=width-i-1;
  segm[0].y2=height-i-1;

  segm[1].x1=width-i-1;
  segm[1].y1=i;
  segm[1].x2=width-i-1;
  segm[1].y2=height-i-1;
  XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
  XDrawSegments(dpy,WinPop,xobj->gc,segm,2);
 }
 /* Ecriture des options */
 x=0;
 for (i=StrtOpt;i<=xobj->value3;i++)
 {
  str=(char*)GetMenuTitle(xobj->title,i);
  if (x<XTextWidth(xobj->xfont,str,strlen(str)))
   x=XTextWidth(xobj->xfont,str,strlen(str));
  free(str);
 }
 for (i=StrtOpt;i<=xobj->value3;i++)
 {
  str=(char*)GetMenuTitle(xobj->title,i);
  XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
  y=h*(i-StrtOpt)+asc+4;
  DrawString(dpy,xobj->gc,WinPop,width-x-8,y,str,
  		strlen(str),xobj->TabColor[fore],
		xobj->TabColor[li],xobj->TabColor[back],
		!xobj->flags[1]);
 free(str);
 }
}

/***********************************************************/
/* Dessine l'option active d'un menu                       */
/***********************************************************/
void SelectMenu(struct XObj *xobj,Window WinPop,int hOpt,int newvalue,int Show)
{
 XSegment segm[2];
 unsigned int i;
 int x,y;
 unsigned int width,height;
 Window Root;

 XGetGeometry(dpy,WinPop,&Root,&x,&y,&width,&height,&i,&i);
 y=hOpt*(newvalue-1);
 for (i=0;i<2;i++)
 {
  segm[0].x1=i+2;
  segm[0].y1=i+y+2;
  segm[0].x2=width-i-3;
  segm[0].y2=i+y+2;

  segm[1].x1=i+2;
  segm[1].y1=i+y+2;
  segm[1].x2=i+2;
  segm[1].y2=y+hOpt-4-i;
  if (Show)
   XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
  else
   XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
  XDrawSegments(dpy,WinPop,xobj->gc,segm,2);

  segm[0].x1=i+3;
  segm[0].y1=y-i-3+hOpt;
  segm[0].x2=width-i-3;
  segm[0].y2=y-i-3+hOpt;

  segm[1].x1=width-i-3;
  segm[1].y1=i+y+2;
  segm[1].x2=width-i-3;
  segm[1].y2=i+y-4+hOpt;
  if (Show)
   XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
  else
   XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
  XDrawSegments(dpy,WinPop,xobj->gc,segm,2);
 }
}

/**************************************************/
/* Compte le nombre d'option contenu dans un menu */
/**************************************************/
int CountOption(char *str)
{
 int i=1;
 int w=0;

 while (str[w]!='\0')
 {
  if (str[w]=='|') i++;
  w++;
 }

 return i;
}


/*****************************************/
/* Dessine l'icone et le titre du widget */
/*****************************************/
void DrawIconStr(int offset,struct XObj *xobj,int DoRedraw)
{
 int i,j;
 char *str;
 int asc,desc,dir;
 XCharStruct struc;

 XTextExtents(xobj->xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 if (DoRedraw)
 {
  XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
  XFillRectangle(dpy,xobj->win,xobj->gc,4,4,xobj->width-8,xobj->height-8);
 }

 if (xobj->iconPixmap==None)			/* Si l'icone n'existe pas */
 {
  str=GetMenuTitle(xobj->title,1);
  j=xobj->height/2+(asc+desc)/2+offset-3;
  i=(xobj->width-XTextWidth(xobj->xfont,str,strlen(str)))/2+offset;
  DrawString(dpy,xobj->gc,xobj->win,i,j,str,
	strlen(str),xobj->TabColor[fore],xobj->TabColor[li],
	xobj->TabColor[back],!xobj->flags[1]);
  free(str);
 }
 else					/* Si l'icone existe */
 {
  if (xobj->title!=NULL)
  {
   str=GetMenuTitle(xobj->title,1);
   if (strlen(str)!=0)
   {
    i=(xobj->width-XTextWidth(xobj->xfont,str,strlen(str)))/2+offset;
    j=((xobj->height - xobj->icon_h)/4)*3 + xobj->icon_h+offset+(asc+desc)/2-3;
    DrawString(dpy,xobj->gc,xobj->win,i,j,str,
	strlen(str),xobj->TabColor[fore],xobj->TabColor[li],
	xobj->TabColor[back],!xobj->flags[1]);
   }
   free(str);
  }
  /* Dessin de l'icone */
  if (xobj->icon_maskPixmap!=None)
   XSetClipMask(dpy,xobj->gc,xobj->icon_maskPixmap);
  j=(xobj->height - xobj->icon_h)/2+offset;
  i=(xobj->width - xobj->icon_w)/2+offset;
  XSetClipOrigin(dpy,xobj->gc,i,j);
  XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
  XCopyArea(dpy,xobj->iconPixmap,xobj->win,xobj->gc,0,0,
  						xobj->icon_w,xobj->icon_h,i,j);
  XSetClipMask(dpy,xobj->gc,None);
 }
}

/***********************************************/
/* Fonction de dessin d'un rectangle en relief */
/***********************************************/
void DrawReliefRect(int x,int y,int width,int height,struct XObj *xobj,
		unsigned int LiC, unsigned int ShadC,unsigned int ForeC,int RectIn)
{
 XSegment segm[2];
 int i;
 int j;

/* XSetForeground(dpy,xobj->gc,xobj->TabColor[back]);
 XFillRectangle(dpy,xobj->win,xobj->gc,x,y,width,height);*/
 width--;
 height--;

 for (i=1;i<3;i++)
 {
  j=-1-i;
  segm[0].x1=i+x;
  segm[0].y1=i+y;
  segm[0].x2=i+x;
  segm[0].y2=height+j+y+1;
  segm[1].x1=i+x;
  segm[1].y1=i+y;
  segm[1].x2=width+j+x+1;
  segm[1].y2=i+y;
  XSetForeground(dpy,xobj->gc,LiC);
  XDrawSegments(dpy,xobj->win,xobj->gc,segm,2);

  segm[0].x1=width+j+x+1;
  segm[0].y1=i+1+y;
  segm[0].x2=width+j+x+1;
  segm[0].y2=height+j+y+1;
  segm[1].x1=i+1+x;
  segm[1].y1=height+j+y+1;
  segm[1].x2=width+j+x+1;
  segm[1].y2=height+j+y+1;
  XSetForeground(dpy,xobj->gc,ShadC);
  XDrawSegments(dpy,xobj->win,xobj->gc,segm,2);
 }
 XSetForeground(dpy,xobj->gc,ForeC);
 if (RectIn==1)			/* Rectangle a l'interieur */
  XDrawRectangle(dpy,xobj->win,xobj->gc,x+3,y+3,
 		width-6,height-6);
 else if (RectIn==0)		/* Rectangle a l'exterieur */
  XDrawRectangle(dpy,xobj->win,xobj->gc,x,y,width,height);
 XSetForeground(dpy,xobj->gc,xobj->TabColor[fore]);
}

/***********************************************/
/* Calcul ascent de la police                  */
/***********************************************/
int GetAscFont(XFontStruct *xfont)
{
 int asc,desc,dir;
 XCharStruct struc;

 XTextExtents(xfont,"lp",strlen("lp"),&dir,&asc,&desc,&struc);
 return asc;
}

/***********************************************/
/* Insertion d'un str dans le titre d'un objet */
/***********************************************/
int InsertText(struct XObj *xobj,char *str,int SizeStr)
{
 int Size;
 int NewPos;
 int i;

 /* Insertion du caractere dans le titre */
 NewPos=xobj->value;
 Size=strlen(xobj->title);
 xobj->title=(char*)realloc(xobj->title,(1+SizeStr+Size)*sizeof(char));
 memmove(&xobj->title[NewPos+SizeStr],&xobj->title[NewPos],
			Size-NewPos+1);
 for (i=NewPos;i<NewPos+SizeStr;i++)
  xobj->title[i]=str[i-NewPos];
 NewPos=NewPos+SizeStr;
 return NewPos;
}

/******************************************************/
/* Lecture d'un morceau de texte de xobj->value à End */
/******************************************************/
char *GetText(struct XObj *xobj,int End)
{
 char *str;
 int a,b;

 if (End>xobj->value)
 {
  a=xobj->value;
  b=End;
 }
 else
 {
  b=xobj->value;
  a=End;
 }
 str=(char*)calloc(b-a+2,1);
 memcpy(str,&xobj->title[a],b-a);
 str[b-a+1]='\0';
 return str;
}

void UnselectAllTextField(struct XObj **txobj)
{
 int i;

 for (i=0;i<nbobj;i++)
  if (txobj[i]->TypeWidget==TextField)
   if (txobj[i]->value2!=txobj[i]->value)
   {
    txobj[i]->value2=txobj[i]->value;
    txobj[i]->DrawObj(txobj[i]);
    return;
   }
}

void SelectOneTextField(struct XObj *xobj)
{
 int i;

 for (i=0;i<nbobj;i++)
  if ((tabxobj[i]->TypeWidget==TextField)&&(xobj!=tabxobj[i]))
   if (tabxobj[i]->value2!=tabxobj[i]->value)
   {
    tabxobj[i]->value2=tabxobj[i]->value;
    tabxobj[i]->DrawObj(tabxobj[i]);
    return;
   }
}

/************************************************************/
/* Dessine une fleche direction nord                        */
/************************************************************/
void DrawArrowN(struct XObj *xobj,int x,int y,int Press)
{
 XSegment segm[4];

 segm[0].x1=5+x;
 segm[0].y1=1+y;
 segm[0].x2=0+x;
 segm[0].y2=12+y;
 segm[1].x1=5+x;
 segm[1].y1=3+y;
 segm[1].x2=1+x;
 segm[1].y2=12+y;
 if (Press==1)
  XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
 else
  XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
 XDrawSegments(dpy,xobj->win,xobj->gc,segm,2);

 segm[0].x1=2+x;
 segm[0].y1=11+y;
 segm[0].x2=11+x;
 segm[0].y2=11+y;
 segm[1].x1=1+x;
 segm[1].y1=12+y;
 segm[1].x2=12+x;
 segm[1].y2=12+y;

 segm[2].x1=6+x;
 segm[2].y1=0+y;
 segm[2].x2=12+x;
 segm[2].y2=12+y;
 segm[3].x1=6+x;
 segm[3].y1=2+y;
 segm[3].x2=10+x;
 segm[3].y2=11+y;
 if (Press==1)
  XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
 else
  XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
 XDrawSegments(dpy,xobj->win,xobj->gc,segm,4);
}

/************************************************************/
/* Dessine une fleche direction sud                         */
/************************************************************/
void DrawArrowS(struct XObj *xobj,int x,int y,int Press)
{
 XSegment segm[4];

 segm[0].x1=0+x;
 segm[0].y1=0+y;
 segm[0].x2=12+x;
 segm[0].y2=0+y;
 segm[1].x1=1+x;
 segm[1].y1=1+y;
 segm[1].x2=11+x;
 segm[1].y2=1+y;

 segm[2].x1=1+x;
 segm[2].y1=1+y;
 segm[2].x2=5+x;
 segm[2].y2=10+y;
 segm[3].x1=2+x;
 segm[3].y1=1+y;
 segm[3].x2=5+x;
 segm[3].y2=8+y;
 if (Press==1)
  XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
 else
  XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
 XDrawSegments(dpy,xobj->win,xobj->gc,segm,4);

 segm[0].x1=6+x;
 segm[0].y1=11+y;
 segm[0].x2=12+x;
 segm[0].y2=1+y;
 segm[1].x1=6+x;
 segm[1].y1=10+y;
 segm[1].x2=10+x;
 segm[1].y2=2+y;
 if (Press==1)
  XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
 else
  XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
 XDrawSegments(dpy,xobj->win,xobj->gc,segm,2);
}

void DrawArrowE(struct XObj *xobj,int x,int y,int Press)
{
 XSegment segm[4];

 segm[0].x1=12+x;
 segm[0].y1=6+y;
 segm[0].x2=1+x;
 segm[0].y2=12+y;
 segm[1].x1=10+x;
 segm[1].y1=6+y;
 segm[1].x2=2+x;
 segm[1].y2=10+y;
 if (Press==1)
  XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
 else
  XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
 XDrawSegments(dpy,xobj->win,xobj->gc,segm,2);

 segm[0].x1=0+x;
 segm[0].y1=0+y;
 segm[0].x2=0+x;
 segm[0].y2=12+y;
 segm[1].x1=1+x;
 segm[1].y1=0+y;
 segm[1].x2=1+x;
 segm[1].y2=11+y;

 segm[2].x1=0+x;
 segm[2].y1=0+y;
 segm[2].x2=11+x;
 segm[2].y2=5+y;
 segm[3].x1=0+x;
 segm[3].y1=1+y;
 segm[3].x2=9+x;
 segm[3].y2=5+y;
 if (Press==1)
  XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
 else
  XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
 XDrawSegments(dpy,xobj->win,xobj->gc,segm,4);
}

void DrawArrowW(struct XObj *xobj,int x,int y,int Press)
{
 XSegment segm[4];

 segm[0].x1=2+x;
 segm[0].y1=5+y;
 segm[0].x2=12+x;
 segm[0].y2=0+y;
 segm[1].x1=4+x;
 segm[1].y1=5+y;
 segm[1].x2=10+x;
 segm[1].y2=2+y;
 if (Press==1)
  XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
 else
  XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
 XDrawSegments(dpy,xobj->win,xobj->gc,segm,2);

 segm[0].x1=0+x;
 segm[0].y1=6+y;
 segm[0].x2=12+x;
 segm[0].y2=12+y;
 segm[1].x1=2+x;
 segm[1].y1=6+y;
 segm[1].x2=11+x;
 segm[1].y2=10+y;

 segm[2].x1=12+x;
 segm[2].y1=1+y;
 segm[2].x2=12+x;
 segm[2].y2=12+y;
 segm[3].x1=11+x;
 segm[3].y1=2+y;
 segm[3].x2=11+x;
 segm[3].y2=11+y;
 if (Press==1)
  XSetForeground(dpy,xobj->gc,xobj->TabColor[li]);
 else
  XSetForeground(dpy,xobj->gc,xobj->TabColor[shad]);
 XDrawSegments(dpy,xobj->win,xobj->gc,segm,4);
 }

int PtInRect(XPoint pt,XRectangle rect)
{
 return ((pt.x>=rect.x)&&(pt.y>=rect.y)&&
		(pt.x<=rect.x+rect.width)&&(pt.y<=rect.y+rect.height));
}

/* Arret pendant t*1/60 de secondes */
void Wait(int t)
{
 struct timeval *tv;
 long tus,ts;

 tv=(struct timeval*)calloc(1,sizeof(struct timeval));
 gettimeofday(tv,NULL);
 tus=tv->tv_usec;
 ts=tv->tv_sec;
 while (((tv->tv_usec-tus)+(tv->tv_sec-ts)*1000000)<16667*t)
  gettimeofday(tv,NULL);
 free(tv);
}

int IsItDoubleClic(struct XObj *xobj)
{
 XEvent Event;
 XFlush(dpy);
 Wait(12);
 return (XCheckTypedEvent(dpy,ButtonPress,&Event));
}
