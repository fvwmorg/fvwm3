#include "types.h"

extern int fd[2];
extern Window ref;

void (*TabCom[30]) (int NbArg,long *TabArg);
char *(*TabFunc[20]) (int *NbArg, long *TabArg);
int (*TabComp[15]) (char *arg1,char *arg2);

extern X11base *x11base;
extern int grab_serve;
extern struct XObj *tabxobj[100];
extern void LoadIcon(struct XObj *xobj);

extern int nbobj;
extern char **TabVVar;
extern int TabIdObj[101];
extern char *ScriptName;
extern char *ModuleName;
extern TypeBuffSend BuffSend;
extern Atom propriete;

char *BufCom;
char Command[255]="None";
time_t TimeCom=0;

/*************************************************************/
/* Ensemble de fonction de comparaison de deux entiers       */
/*************************************************************/
int Inf(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (an1<an2);
}

int InfEq(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (an1<=an2);
}

int Equal(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (strcmp(arg1,arg2)==0);
}

int SupEq(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (an1>=an2);
}

int Sup(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (an1>an2);
}

int Diff(char *arg1,char *arg2)
{
 int an1,an2;
 an1=atoi(arg1);
 an2=atoi(arg2);
 return (strcmp(arg1,arg2)!=0);
}

/*****************************************************/
/* Fonction qui retourne la valeur d'un argument     */
/*****************************************************/
char *CalcArg (long *TabArg,int *Ix)
{
 char *TmpStr;
 int i;

 if (TabArg[*Ix]>100000)	/* Cas du codage d'un nombre */
 {
  i=(int)TabArg[*Ix]-200000;
  TmpStr=(char*)calloc(1,sizeof(char)*10);
  sprintf(TmpStr,"%d",i);
 }
 else if (TabArg[*Ix]<-200000)/* Cas d'un id de fonction de comparaison */
 {
  i=TabArg[*Ix]+250000;
  TmpStr=(char*)calloc(1,sizeof(char)*10);
  sprintf(TmpStr,"%d",i);
 }
 else if (TabArg[*Ix]< -100000)	/* Cas d'un id de fonction */
 {
  TmpStr=TabFunc[TabArg[*Ix]+150000](Ix,TabArg);
 }
 else				/* Cas d'une variable */
 {
  TmpStr=strdup(TabVVar[TabArg[*Ix]]);
 }
 return (TmpStr);
}

/*************************************************************/
/* Ensemble des fonctions pour recuperer les prop d'un objet */
/*************************************************************/
char *FuncGetValue(int *NbArg, long *TabArg)
{
 char *tmp;
 long Id;

 (*NbArg)++;			/* La fonction GetValue n'a qu'un seul argument */
 tmp=CalcArg(TabArg,NbArg);
 Id=atoi(tmp);
 free(tmp);
 tmp=(char*)calloc(1,sizeof(char)*10);
 sprintf(tmp,"%d",tabxobj[TabIdObj[Id]]->value);
 return tmp;
}

/* Fonction qui retourne le titre d'un objet */
char *FuncGetTitle(int *NbArg, long *TabArg)
{
 char *tmp;
 long Id;

 (*NbArg)++;
 tmp=CalcArg(TabArg,NbArg);
 Id=atoi(tmp);
 free(tmp);
 if (TabIdObj[Id]!=-1)
  tmp=strdup(tabxobj[TabIdObj[Id]]->title);
 else
 {
  fprintf(stderr,"Widget %d doesn't exist\n",(int)Id);
  tmp=(char*)calloc(1,sizeof(char));
  tmp[0]='\0';
 }
 return tmp;
}

/* Fonction qui retourne la sortie d'une commande */
char *FuncGetOutput(int *NbArg, long *TabArg)
{
 char *cmndbuf;
 char *str;
 int line,index,i=2,j=0,k,NewWord;
 FILE *f;
 int maxsize=32000;
 int size;

 (*NbArg)++;		
 cmndbuf=CalcArg(TabArg,NbArg);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 line=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 index=atoi(str);
 free(str);
 
 if ((strcmp(Command,cmndbuf))||((time(NULL)-TimeCom)>1)||(TimeCom==0))
 {
  if ((f = popen(cmndbuf,"r")) == NULL)
  {
   fprintf(stderr,"%s: can't run %s\n",ScriptName,cmndbuf);
   str=(char*)calloc(sizeof(char),10);
   free(cmndbuf);
   return str;
  }
  else
  {
   if (strcmp(Command,"None"))
    free(BufCom);
   BufCom=(char*)calloc(sizeof(char),maxsize);
   size=fread(BufCom,1,maxsize,f);
   pclose(f);
   strcpy(Command,cmndbuf);
   TimeCom=time(NULL);
  }
 }
 
 /* Recherche de la ligne */
 while ((i<=line)&&(BufCom[j]!='\0'))
 {
  j++;
  if (BufCom[j]=='\n') i++;
 }
 
 /* Recherche du mot */
 if (index!=-1)
 {
  if (i!=2) j++;
  i=1;
  NewWord=0;
  while ((i<index)&&(BufCom[j]!='\n')&&(BufCom[j]!='\0'))
  {
   j++;
   if (BufCom[j]==' ')
   {
    if (NewWord)
    {
     i++;
     NewWord=0;
    }
   }
   else
    if (!NewWord) NewWord=1;
  }
  str=(char*)calloc(sizeof(char),255);
  sscanf(&BufCom[j],"%s",str);
 }
 else		/* Lecture de la ligne complete */
 {
  if (i!=2) j++;
  k=j;
  while ((BufCom[k]!='\n')&&(BufCom[k]!='\0'))
   k++;
  str=(char*)calloc(sizeof(char),k-j+1);
  memmove(str,&BufCom[j],k-j);
  str[k-j]='\0';
 }
  
 free(cmndbuf);
 return str;
}

/* Convertion decimal vers hexadecimal */
char *FuncNumToHex(int *NbArg, long *TabArg)
{
 char *str;
 int value,nbchar;
 int i,j;
 
 (*NbArg)++;		
 str=CalcArg(TabArg,NbArg);
 value=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 nbchar=atoi(str);
 free(str);
 
 str=(char*)calloc(1,nbchar+10);
 sprintf(str,"%X",value);
 j=strlen(str);
 if (j<nbchar)
 {
  memmove(&str[nbchar-j],str,j);
  for (i=0;i<(nbchar-j);i++)
   str[i]='0';
 }

 return str;
}

/* Convertion hexadecimal vers decimal */
char *FuncHexToNum(int *NbArg, long *TabArg)
{
 char *str,*str2;
 int k;
 
 (*NbArg)++;		
 str=CalcArg(TabArg,NbArg);
 if (str[0]=='#')
  memmove(str,&str[1],strlen(str));
 k=(int)strtol(str,NULL,16);
 free(str);
 
 str2=(char*)calloc(1,20);
 sprintf(str2,"%d",k);
 return str2;
}

char *FuncAdd(int *NbArg, long *TabArg)
{
 char *str;
 int val1,val2;
 
 (*NbArg)++;		
 str=CalcArg(TabArg,NbArg);
 val1=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val2=atoi(str);
 free(str);
 str=(char*)calloc(1,20);
 sprintf(str,"%d",val1+val2);
 return str;
}

char *FuncMult(int *NbArg, long *TabArg)
{
 char *str;
 int val1,val2;
 
 (*NbArg)++;		
 str=CalcArg(TabArg,NbArg);
 val1=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val2=atoi(str);
 free(str);
 str=(char*)calloc(1,20);
 sprintf(str,"%d",val1*val2);
 return str;
}

char *FuncDiv(int *NbArg, long *TabArg)
{
 char *str;
 int val1,val2;
 
 (*NbArg)++;		
 str=CalcArg(TabArg,NbArg);
 val1=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val2=atoi(str);
 free(str);
 str=(char*)calloc(1,20);
 sprintf(str,"%d",val1/val2);
 return str;
}

char *RemainderOfDiv(int *NbArg, long *TabArg)
{
 char *str;
 int val1,val2;
 div_t res;

 (*NbArg)++;		
 str=CalcArg(TabArg,NbArg);
 val1=atoi(str);
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 val2=atoi(str);
 free(str);
 str=(char*)calloc(1,20);
 res=div(val1,val2);
 sprintf(str,"%d",res.rem);
 return str;
}


char *FuncStrCopy(int *NbArg, long *TabArg)
{
 char *str,*strsrc;
 int i1,i2;
 
 (*NbArg)++;		
 strsrc=CalcArg(TabArg,NbArg);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 i1=atoi(str);
 if (i1<1) i1=1;
 free(str);
 (*NbArg)++;
 str=CalcArg(TabArg,NbArg);
 i2=atoi(str);
 if (i2<1) i2=1;
 free(str);
 
 if ((i1<=i2)&&(i1<=strlen(strsrc)))
 {
  if (i2>strlen(strsrc)) i2=strlen(strsrc);
  str=(char*)calloc(1,i2-i1+2);
  memmove(str,&strsrc[i1-1],i2-i1+1);
 }
 else
 {
  str=(char*)calloc(1,1);
 }

 free(strsrc);
 return str;
}

/* Lancement d'un script avec pipe */
char *LaunchScript (int *NbArg,long *TabArg)
{
 char *arg,*execstr,*str,*scriptarg,*scriptname;
 int leng,i;
 Atom MyAtom;

 /* Lecture des arguments */
 (*NbArg)++;		
 arg=CalcArg(TabArg,NbArg);

 str=(char*)calloc(100,sizeof(char));

 /* Calcul du nom du script fils */
 x11base->TabScriptId[x11base->NbChild+2]=(char*)calloc(strlen(x11base->TabScriptId[1])+4,sizeof(char));

 if (x11base->NbChild<98)
 {
  i=16;
  do
  {
   sprintf(x11base->TabScriptId[x11base->NbChild+2],"%s%x",x11base->TabScriptId[1],i);
   MyAtom=XInternAtom(x11base->display,x11base->TabScriptId[x11base->NbChild+2],False);
   i++;
  }
  while (XGetSelectionOwner(x11base->display,MyAtom)!=None);
 }
 else
 {
  fprintf(stderr,"Too many launched script\n");
  sprintf(str,"-1");
  return str;
 }

 /* Construction de la commande */
 execstr=(char*)calloc(strlen(ModuleName)+strlen(arg)+
	strlen(x11base->TabScriptId[x11base->NbChild+2])+5,sizeof(char));
 scriptname=(char*)calloc(sizeof(char),100);
 sscanf(arg,"%s",scriptname);
 scriptarg=(char*)calloc(sizeof(char),strlen(arg));
 scriptarg=(char*)strncpy(scriptarg,&arg[strlen(scriptname)],strlen(arg)-strlen(scriptname));
 sprintf(execstr,"%s %s %s %s",ModuleName,scriptname,
		x11base->TabScriptId[x11base->NbChild+2],scriptarg);
 free(scriptname);
 free(scriptarg);
 free(arg);

 /* Envoi de la commande */
 write(fd[0], &ref, sizeof(Window));
 leng = strlen(execstr);
 write(fd[0], &leng, sizeof(int));
 write(fd[0], execstr, leng);
 leng = 1;
 write(fd[0], &leng, sizeof(int));
 free(execstr);

 /* Retourne l'id du fils */
 sprintf(str,"%d",x11base->NbChild+2);
 x11base->NbChild++;
 return str;
}

char *GetScriptFather (int *NbArg,long *TabArg)
{
 char *str;

 str=(char*)calloc(10,sizeof(char));
 sprintf(str,"0");
 return str;
}

char *GetTime (int *NbArg,long *TabArg)
{
 char *str;
 time_t t;

 str=(char*)calloc(20,sizeof(char));
 t=time(NULL);
 sprintf(str,"%ld",(long)t-x11base->BeginTime);
 return str;
}

char *GetScriptArg (int *NbArg,long *TabArg)
{
 char *str;
 int val1;

 (*NbArg)++;		
 str=CalcArg(TabArg,NbArg);
 val1=atoi(str);
 free(str);

 str=(char*)calloc(strlen(x11base->TabArg[val1])+1,sizeof(char));
 str=strcpy(str,x11base->TabArg[val1]);

 return str;
}

char *ReceivFromScript (int *NbArg,long *TabArg)
{
 char *arg,*msg;
 int send;
 Atom AReceiv,ASend,type;
 static XEvent event;
 unsigned long longueur,octets_restant;
 unsigned char *donnees="";
 int format;
 int NbEssai=0;

 (*NbArg)++;		
 arg=CalcArg(TabArg,NbArg);
 send=(int)atoi(arg);
 free(arg);

 msg=(char*)calloc(256,sizeof(char));
 sprintf(msg,"No message");

 /* Recuperation des atomes */
 AReceiv=XInternAtom(x11base->display,x11base->TabScriptId[1],True);
 if (AReceiv==None)
 {
  fprintf(stderr,"Error with atome\n");
  return msg;
 }

 if ((send>=0)&&(send<99))
  if (x11base->TabScriptId[send]!=NULL)
  {
   ASend=XInternAtom(x11base->display,x11base->TabScriptId[send],True);
   if (ASend==None)
    fprintf(stderr,"Error with atome\n");
  }
  else
   return msg;
 else
  return msg;

 /* Recuperation du message */
 XConvertSelection(x11base->display,ASend,AReceiv,propriete,x11base->win,CurrentTime);
 while ((!XCheckTypedEvent(x11base->display,SelectionNotify,&event))&&(NbEssai<25000))
  NbEssai++;
 if (event.xselection.property!=None)
  if (event.xselection.selection==ASend)
  {
   XGetWindowProperty(x11base->display,event.xselection.requestor,event.xselection.property,0,
 	8192,False,event.xselection.target,&type,&format,&longueur,&octets_restant,
 	&donnees);
   if (longueur>0)
   {
    msg=(char*)realloc((void*)msg,(longueur+1)*sizeof(char));
    msg=strcpy(msg,donnees);
    XDeleteProperty(x11base->display,event.xselection.requestor,event.xselection.property);
    XFree(donnees);
   }
  }

 return msg;
}


/***********************************************/
/* Ensemble des commandes possible pour un obj */
/***********************************************/

void Exec (int NbArg,long *TabArg)
{
 int leng;
 char *execstr;
 char *tempstr;
 int i;

 execstr=(char*)calloc(1,256);
 for (i=0;i<NbArg;i++)
 {
  tempstr=CalcArg(TabArg,&i);
  execstr=strcat(execstr,tempstr);
  free(tempstr);
 }
 
 write(fd[0], &ref, sizeof(Window));
 leng = strlen(execstr);
 write(fd[0], &leng, sizeof(int));
 write(fd[0], execstr, leng);
 leng = 1;
 write(fd[0], &leng, sizeof(int));
 
 
 free(execstr);
}

void HideObj (int NbArg,long *TabArg)
{
 char *arg[1];
 int IdItem; 
 int i=0;

 arg[0]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];

 tabxobj[IdItem]->flags[0]=True;
 /* On cache la fentre pour la faire disparaitre */
 XUnmapWindow(x11base->display,tabxobj[IdItem]->win);
 free(arg[0]);
}

void ShowObj (int NbArg,long *TabArg)
{
 char *arg[1];
 int IdItem; 
 int i=0;

 arg[0]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];

 tabxobj[IdItem]->flags[0]=False;
 XMapWindow(x11base->display,tabxobj[IdItem]->win);
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);
 free(arg[0]);
}

void ChangeValue (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 
 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 
 tabxobj[TabIdObj[atoi(arg[0])]]->value=atoi(arg[1]);
 /* On redessine l'objet pour le mettre a jour */
 tabxobj[TabIdObj[atoi(arg[0])]]->DrawObj(tabxobj[TabIdObj[atoi(arg[0])]]);
 free(arg[0]);
 free(arg[1]);
}

void ChangeValueMax (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int j;
 
 arg[0]=CalcArg(TabArg,&i);
 j=atoi(arg[0]);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 
 tabxobj[TabIdObj[j]]->value3=atoi(arg[1]);
 /* On redessine l'objet pour le mettre a jour */
 if (tabxobj[TabIdObj[j]]->value>tabxobj[TabIdObj[j]]->value3)
 {
  tabxobj[TabIdObj[j]]->value=atoi(arg[1]);
  tabxobj[TabIdObj[j]]->DrawObj(tabxobj[TabIdObj[j]]);
 }
 free(arg[0]);
 free(arg[1]);
}

void ChangeValueMin (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int j;
 
 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 j=atoi(arg[0]);
 
 tabxobj[TabIdObj[j]]->value2=atoi(arg[1]);
 /* On redessine l'objet pour le mettre a jour */
 if (tabxobj[TabIdObj[j]]->value<tabxobj[TabIdObj[j]]->value2)
 {
  tabxobj[TabIdObj[j]]->value=atoi(arg[1]);
  tabxobj[TabIdObj[j]]->DrawObj(tabxobj[TabIdObj[j]]);
 }
 free(arg[0]);
 free(arg[1]);
}

void ChangePos (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[3];
 int an[3]; 
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 i++;
 arg[2]=CalcArg(TabArg,&i);

 IdItem= TabIdObj[atoi(arg[0])];
 for (i=1;i<3;i++)
  an[i]=atoi(arg[i]);
 tabxobj[IdItem]->x=an[1];
 tabxobj[IdItem]->y=an[2];
 XMoveWindow(x11base->display,tabxobj[IdItem]->win,an[1],an[2]);

 free(arg[0]);
 free(arg[1]);
 free(arg[2]);

}

void ChangeFont (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;
 XFontStruct *xfont;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];

 tabxobj[IdItem]->font=strdup(arg[1]);
 if ((xfont=XLoadQueryFont(tabxobj[IdItem]->display,tabxobj[IdItem]->font))==NULL)
  {
   fprintf(stderr,"Can't load font %s\n",tabxobj[IdItem]->font);
  }
 else
 {
  XFreeFont(tabxobj[IdItem]->display,tabxobj[IdItem]->xfont);
  tabxobj[IdItem]->xfont=xfont;
  XSetFont(tabxobj[IdItem]->display,tabxobj[IdItem]->gc,tabxobj[IdItem]->xfont->fid);
 }
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);
 free(arg[0]);
 free(arg[1]);
}

void ChangeSize (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[3];
 int an[3]; 
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 i++;
 arg[2]=CalcArg(TabArg,&i);

 IdItem= TabIdObj[atoi(arg[0])];
 for (i=1;i<3;i++)
  an[i]=atoi(arg[i]);
 tabxobj[IdItem]->width=an[1];
 tabxobj[IdItem]->height=an[2];
 XResizeWindow(x11base->display,tabxobj[IdItem]->win,an[1],an[2]);
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);
 free(arg[0]);
 free(arg[1]);
 free(arg[2]);
}

void ChangeTitle (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];

 tabxobj[IdItem]->title=strdup(arg[1]);
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);
 free(arg[0]);
 free(arg[1]);
}

void ChangeIcon (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];
/* if (tabxobj[IdItem]->icon!=NULL)
 {
  free(tabxobj[IdItem]->icon);
  if (tabxobj[IdItem]->iconPixmap!=None)
   XFreePixmap(tabxobj[IdItem]->display,tabxobj[IdItem]->iconPixmap);
  if (tabxobj[IdItem]->icon_maskPixmap!=None)
   XFreePixmap(tabxobj[IdItem]->display,tabxobj[IdItem]->icon_maskPixmap);
 }*/
 tabxobj[IdItem]->icon=strdup(arg[1]);
 LoadIcon(tabxobj[IdItem]);
 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);
 free(arg[0]);
 free(arg[1]);
}

void ChangeForeColor (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];

 /* Liberation de la couleur */
 XFreeColors(tabxobj[IdItem]->display,*(tabxobj[IdItem])->colormap,
		(void*)(&(tabxobj[IdItem])->TabColor[fore]),1,0);

 tabxobj[IdItem]->forecolor=(char*)calloc(100,sizeof(char));
 sprintf(tabxobj[IdItem]->forecolor,"%s",arg[1]);

 MyAllocNamedColor(tabxobj[IdItem]->display,*(tabxobj[IdItem])->colormap,
		tabxobj[IdItem]->forecolor,&(tabxobj[IdItem])->TabColor[fore]);

 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);

 free(arg[0]);
 free(arg[1]);

}

void ChangeBackColor (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];

 /* Liberation de la couleur */
 XFreeColors(tabxobj[IdItem]->display,*(tabxobj[IdItem])->colormap,
		(void*)(&(tabxobj[IdItem])->TabColor[back]),1,0);

 tabxobj[IdItem]->backcolor=(char*)calloc(100,sizeof(char));
 sprintf(tabxobj[IdItem]->backcolor,"%s",arg[1]);

 MyAllocNamedColor(tabxobj[IdItem]->display,*(tabxobj[IdItem])->colormap,
		tabxobj[IdItem]->backcolor,&(tabxobj[IdItem])->TabColor[back]);

 tabxobj[IdItem]->DrawObj(tabxobj[IdItem]);

 free(arg[0]);
 free(arg[1]);
}

void SetVar (int NbArg,long *TabArg)
{
 int i;
 char *str,*tempstr;
 
 str=(char*)calloc(sizeof(char),1);
 for (i=1;i<NbArg;i++)
 {
  tempstr=CalcArg(TabArg,&i);
  str=(char*)realloc((void*)str,sizeof(char)*(1+strlen(str)+strlen(tempstr)));
  str=strcat(str,tempstr);
  free(tempstr);
 }

 free(TabVVar[TabArg[0]]);
 TabVVar[TabArg[0]]=str;
}

void SendSign (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2];
 int IdItem;
 int TypeMsg;

 arg[0]=CalcArg(TabArg,&i);
 i++;
 arg[1]=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg[0])];
 TypeMsg=atoi(arg[1]);
 SendMsg(tabxobj[IdItem],TypeMsg);
 free(arg[0]);
 free(arg[1]);
}

void WarpPointer(int NbArg,long *TabArg)
{
 int i=0;
 char *arg;
 int IdItem;

 arg=CalcArg(TabArg,&i);
 IdItem= TabIdObj[atoi(arg)];
 /* Deplacement du pointeur sur l'objet */
 XWarpPointer(x11base->display,None,tabxobj[IdItem]->win,0,0,0,0,
	tabxobj[IdItem]->width/2,tabxobj[IdItem]->height+10);
 free(arg);
}

void Quit (int NbArg,long *TabArg)
{
 int i;
 static XEvent event;
 fd_set in_fdset;
 extern int x_fd;
 Atom MyAtom;
 int NbEssai=0;
 struct timeval tv;

#ifdef DEBUG			/* For debugging */
  XSync(x11base->display,0);
#endif

 /* On cache la fenetre */
 XUnmapWindow(x11base->display,x11base->win);
 XFlush(x11base->display);

 /* Le script ne possede plus la propriete */
 MyAtom=XInternAtom(x11base->display,x11base->TabScriptId[1],False);
 XSetSelectionOwner(x11base->display,MyAtom,x11base->root,CurrentTime);

 /* On verifie si tous les messages ont ete envoyes */
 while((BuffSend.NbMsg>0)&&(NbEssai<10000))
 {
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  FD_ZERO(&in_fdset);
  FD_SET(x_fd,&in_fdset);
  select(32, &in_fdset, NULL, NULL, &tv);
  if (FD_ISSET(x_fd, &in_fdset))
  {
   if (XCheckTypedEvent(x11base->display,SelectionRequest,&event))
    SendMsgToScript(event);
   else
    NbEssai++;
  }
 }
 XFlush(x11base->display);

 /* Attente de deux secondes afin d'etre sur que tous */
 /* les messages soient arrives a destination         */
 /* On quitte proprement le serveur X */
 for (i=0;i<nbobj;i++)
  tabxobj[i]->DestroyObj(tabxobj[i]);
 XFlush(x11base->display);
/* XSync(x11base->display,True);*/
 sleep(2);
 XFreeGC(x11base->display,x11base->gc);
 XFreeColormap(x11base->display,x11base->colormap);
 XDestroyWindow(x11base->display,x11base->win);
 XCloseDisplay(x11base->display);
 exit(0);
}

void IfThen (int NbArg,long *TabArg)
{
 char *arg[10];
 int i,j;
 int CurrArg=0;
 int IdFuncComp;

 /* Verification de la condition */
 for (j=0;j<NbArg-2;j++)
 {
  if (TabArg[j]>100000)	/* Cas du codage d'un nombre */
  {
   i=(int)TabArg[j]-200000;
   arg[CurrArg]=(char*)calloc(1,sizeof(char)*10);
   sprintf(arg[CurrArg],"%d",i);
   CurrArg++;
  }
  else if (TabArg[j]<-200000)/* Cas d'un id de fonction de comparaison */
  {
   IdFuncComp=TabArg[j]+250000;
  }
  else if (TabArg[j]<-100000)	/* Cas d'un id de fonction */
  {
   arg[CurrArg]=TabFunc[TabArg[j]+150000](&j,TabArg);
   CurrArg++;
  }
  else				/* Cas d'une variable */
  {
    arg[CurrArg]=strdup(TabVVar[TabArg[j]]);
    CurrArg++;
  }
 }

 /* Comparaison des arguments */
 if (TabComp[IdFuncComp](arg[0],arg[1]))
  ExecBloc((Bloc*)TabArg[NbArg-2]);
 else if (TabArg[NbArg-1]!=0)
  ExecBloc((Bloc*)TabArg[NbArg-1]);

 free(arg[0]);
 free(arg[1]);
}

/* Instruction boucle */
void Loop (int NbArg,long *TabArg)
{
 int IdVar;
 char *arg[2];
 int limit[2];
 int i;
 int CurrArg=0;

 /* le premier argument est une variable */
 IdVar=TabArg[0];
 /*On ajuste la taille de la var pour contenir un nombre */
 TabVVar[TabArg[0]]=(char*)realloc(TabVVar[TabArg[0]],sizeof(char)*10);
 /* Calcul des 2 autres arguments */
 for (i=1;i<NbArg-1;i++)
 {
  if (TabArg[i]>100000)	/* Cas du codage d'un nombre */
  {
   i=(int)TabArg[i]-200000;
   arg[CurrArg]=(char*)calloc(1,sizeof(char)*10);
   sprintf(arg[CurrArg],"%d",i);
  }
  else if (TabArg[i]<-100000)	/* Cas d'un id de fonction */
  {
   arg[CurrArg]=TabFunc[TabArg[i]+150000](&i,TabArg);
  }
  else				/* Cas d'une variable */
    arg[CurrArg]=strdup(TabVVar[TabArg[i]]);
  CurrArg++;
 }
 limit[0]=atoi(arg[0]);
 limit[1]=atoi(arg[1]);
 if (limit[0]<limit[1])
  for (i=limit[0];i<=limit[1];i++)
  {
   /* On met a jour la variable */
   sprintf(TabVVar[TabArg[0]],"%d",i);
   ExecBloc((Bloc*)TabArg[NbArg-1]);
  }
 else
  for (i=limit[0];i<=limit[1];i++)
  {
   sprintf(TabVVar[TabArg[0]],"%d",i);
   ExecBloc((Bloc*)TabArg[NbArg-1]);
  }

 free(arg[0]);
 free(arg[1]);
}

/* Instruction While */
void While (int NbArg,long *TabArg)
{
 char *arg[3],*str;
 int i;
 int Loop=1;
 int IdFuncComp;

 while (Loop)
 {
  i=0;
  arg[0]=CalcArg(TabArg,&i);
  i++;
  str=CalcArg(TabArg,&i);
  IdFuncComp=atoi(str);
  free(str);
  i++;
  arg[1]=CalcArg(TabArg,&i);

  Loop=TabComp[IdFuncComp](arg[0],arg[1]);
  if (Loop) ExecBloc((Bloc*)TabArg[NbArg-1]);
  free(arg[0]);
  free(arg[1]);
 }
}


void WriteToFile (int NbArg,long *TabArg)
{
 int i=0;
 char *arg[2],str[50],*tempstr,*home,*file;
 FILE *f;
 char StrBegin[100];
 char StrEnd[10];
 size_t  size;
 char *buf;
 int maxsize=32000;
 int CurrPos=0,CurrPos2;
 int OldPID;

 arg[0]=CalcArg(TabArg,&i);
 arg[1]=(char*)calloc(1,256);
 for (i=1;i<NbArg;i++)
 {
  tempstr=CalcArg(TabArg,&i);
  arg[1]=strcat(arg[1],tempstr);
  free(tempstr);
 }
 if (arg[1][strlen(arg[1])-1]!='\n')
 {
  i=strlen(arg[1]);
  arg[1]=(char*)realloc(arg[1],strlen(arg[1])+2);
  arg[1][i]='\n';
  arg[1][i+1]='\0';
 }
  
 sprintf(StrEnd,"#end\n");
 sprintf(StrBegin,"#%s,",ScriptName);
 
 buf=(char*)calloc(1,maxsize);
 
 if (arg[0][0]!='/')
 {
  file=strdup(arg[0]);
  home=getenv("HOME");
  arg[0]=(char*)realloc(arg[0],sizeof(char)*(strlen(arg[0])+4+strlen(home)));
  sprintf(arg[0],"%s/%s",home,file);
/*  free(home);*/		/* BUG */
  free(file);
 }
 f=fopen(arg[0],"a+");
 fseek(f,0,SEEK_SET);
 size=fread(buf,1,maxsize,f);
 while(((strncmp(StrBegin,&buf[CurrPos],strlen(StrBegin)))!=0)&&(CurrPos<size))
  CurrPos++;
 if (CurrPos==size)
 {
  sprintf(buf,"%s\n%s%d\n",buf,StrBegin,getpid());
  sprintf(buf,"%s%s",buf,arg[1]);
  sprintf(buf,"%s%s\n",buf,StrEnd);
 }
 else
 {
  sscanf(&buf[CurrPos+strlen(StrBegin)],"%d",&OldPID);
  if (OldPID==getpid())
  {
   sprintf(str,"%d\n",OldPID);
   while(((strncmp(StrEnd,&buf[CurrPos],strlen(StrEnd)))!=0)&&(CurrPos<size))
    CurrPos++;
   memmove(&buf[CurrPos+strlen(arg[1])],&buf[CurrPos],strlen(buf)-CurrPos);
   memmove(&buf[CurrPos],arg[1],strlen(arg[1]));
  }
  else				/* Remplacement des anciennes commandes */
  {
   CurrPos=CurrPos+strlen(StrBegin);
   CurrPos2=CurrPos;
   while(((strncmp(StrEnd,&buf[CurrPos2],strlen(StrEnd)))!=0)&&(CurrPos2<size))
    CurrPos2++;
   sprintf(str,"%d\n%s",getpid(),arg[1]);
   memmove(&buf[CurrPos+strlen(str)],&buf[CurrPos2],strlen(buf)-CurrPos2);
   buf[strlen(buf)-((CurrPos2-CurrPos)-strlen(str))]='\0';
   memmove(&buf[CurrPos],str,strlen(str));
  }
 }
 
 fclose(f);
 f=fopen(arg[0],"w");
 if (f==NULL)
 {
  fprintf(stderr,"Enable to open file %s\n",arg[0]);
  return;
 }
 fwrite(buf,1,strlen(buf),f);
 fclose(f);
 
 free(arg[0]);
 free(arg[1]);
}

void SendToScript (int NbArg,long *TabArg)
{
 char *tempstr,*Msg,*R;
 int dest;
 int j=0;
 Atom myatom;

 /* Calcul destinataire */
 tempstr=CalcArg(TabArg,&j);
 dest=(int)atoi(tempstr);
 free(tempstr);

 /* Calcul contenu */
 Msg=(char*)calloc(256,sizeof(char));
 for (j=1;j<NbArg;j++)
 {
  tempstr=CalcArg(TabArg,&j);
  Msg=(char*)realloc((void*)Msg,strlen(Msg)+strlen(tempstr)+1);
  Msg=strcat(Msg,tempstr);
  free(tempstr);
 }

 /* Calcul recepteur */
 R=(char*)calloc(strlen(x11base->TabScriptId[dest])+1,sizeof(char));
 sprintf(R,"%s",x11base->TabScriptId[dest]);
 myatom=XInternAtom(x11base->display,R,True);
 
 if ((BuffSend.NbMsg<40)&&(XGetSelectionOwner(x11base->display,myatom)!=None))
 {
  /* Enregistrement dans le buffer du message */
  BuffSend.TabMsg[BuffSend.NbMsg].Msg=Msg;
  /* Enregistrement dans le buffer du destinataire */
  BuffSend.TabMsg[BuffSend.NbMsg].R=R;
  /* Enregistrement du message */
  BuffSend.NbMsg++;

  /* Reveil du destinataire */
  XConvertSelection(x11base->display,XInternAtom(x11base->display,x11base->TabScriptId[dest],True)
			,propriete,propriete,x11base->win,CurrentTime);
 }
 else
 {
  fprintf(stderr,"Too many messages sended\n");
  free(Msg);
 }
}

/****************************************************/
/* Fonction d'initialisation de TabCom et TabFunc   */
/****************************************************/
void InitCom()
{
 /* commande */
 TabCom[1]=Exec;
 TabCom[2]=HideObj;
 TabCom[3]=ShowObj;
 TabCom[4]=ChangeValue;
 TabCom[5]=ChangePos;
 TabCom[6]=ChangeSize;
 TabCom[7]=ChangeIcon;
 TabCom[8]=ChangeTitle;
 TabCom[9]=ChangeFont;
 TabCom[10]=ChangeForeColor;
 TabCom[11]=SetVar;
 TabCom[12]=SendSign;
 TabCom[13]=Quit;
 TabCom[14]=IfThen;
 TabCom[15]=Loop;
 TabCom[16]=While;
 TabCom[17]=WarpPointer;
 TabCom[18]=WriteToFile;
 TabCom[19]=ChangeBackColor;
 TabCom[21]=ChangeValueMax;
 TabCom[22]=ChangeValueMin;
 TabCom[23]=SendToScript;

 /* Fonction */
 TabFunc[1]=FuncGetValue;
 TabFunc[2]=FuncGetTitle;
 TabFunc[3]=FuncGetOutput;
 TabFunc[4]=FuncNumToHex;
 TabFunc[5]=FuncHexToNum;
 TabFunc[6]=FuncAdd;
 TabFunc[7]=FuncMult;
 TabFunc[8]=FuncDiv;
 TabFunc[9]=FuncStrCopy;
 TabFunc[10]=LaunchScript;
 TabFunc[11]=GetScriptFather;
 TabFunc[12]=ReceivFromScript;
 TabFunc[13]=RemainderOfDiv;
 TabFunc[14]=GetTime;
 TabFunc[15]=GetScriptArg;

 /* Fonction de comparaison */
 TabComp[1]=Inf;
 TabComp[2]=InfEq;
 TabComp[3]=Equal;
 TabComp[4]=SupEq;
 TabComp[5]=Sup;
 TabComp[6]=Diff;

}
