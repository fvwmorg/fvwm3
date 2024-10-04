/* -*-c-*- */
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include <time.h>

#include "types.h"
#include "libs/fvwmsignal.h"
#include "libs/ftime.h"
#include "libs/FGettext.h"
#include "libs/FEvent.h"
#include "libs/Bindings.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "libs/modifiers.h"
#include "libs/Module.h"
#include "libs/ColorUtils.h"
#include "libs/Strings.h"
#include "libs/log.h"
#ifdef HAVE_GETPWUID
#  include <pwd.h>
#endif

extern int fd[2];
extern Window ref;

void (*TabCom[29]) (int NbArg,long *TabArg);
char *(*TabFunc[27]) (int *NbArg, long *TabArg);
int (*TabComp[7]) (char *arg1,char *arg2);

extern Display *dpy;
extern int screen;
extern X11base *x11base;
extern int grab_serve;
extern struct XObj *tabxobj[1000];
extern Binding *BindingsList;
extern void LoadIcon(struct XObj *xobj);

extern int nbobj;
extern char **TabVVar;
extern int TabIdObj[1001];
extern char *ScriptName;
extern ModuleArgs *module;
extern TypeBuffSend BuffSend;
extern Atom propriete;
extern char *LastString;
char *FvwmUserDir = NULL;
char *BufCom;
char Command[256]="None";
time_t TimeCom=0;

/*
 * Utilities
 */
void setFvwmUserDir(void)
{
  char *home_dir;

  if (FvwmUserDir != NULL)
    return;

  /* Figure out user's home directory */
  home_dir = getenv("HOME");
#ifdef HAVE_GETPWUID
  if (home_dir == NULL) {
    struct passwd* pw = getpwuid(getuid());
    if (pw != NULL)
      home_dir = fxstrdup(pw->pw_dir);
  }
#endif
  if (home_dir == NULL)
    home_dir = "/"; /* give up and use root dir */

  /* Figure out where to read and write user's data files. */
  FvwmUserDir = getenv("FVWM_USERDIR");
  if (FvwmUserDir == NULL)
  {
    xasprintf(&FvwmUserDir, "%s/.fvwm", home_dir);
  }
}

/*
 * Functions to compare 2 integers
 */
static int Inf(char *arg1,char *arg2)
{
  int an1,an2;
  an1=atoi(arg1);
  an2=atoi(arg2);
  return (an1<an2);
}

static int InfEq(char *arg1,char *arg2)
{
  int an1,an2;
  an1=atoi(arg1);
  an2=atoi(arg2);
  return (an1<=an2);
}

static int Equal(char *arg1,char *arg2)
{
  int n;

  n = atoi(arg1);
  n = atoi(arg2);
  (void)n;
  return (strcmp(arg1,arg2)==0);
}

static int SupEq(char *arg1,char *arg2)
{
  int an1,an2;
  an1=atoi(arg1);
  an2=atoi(arg2);
  return (an1>=an2);
}

static int Sup(char *arg1,char *arg2)
{
  int an1,an2;
  an1=atoi(arg1);
  an2=atoi(arg2);
  return (an1>an2);
}

static int Diff(char *arg1,char *arg2)
{
  int n;

  n = atoi(arg1);
  n = atoi(arg2);
  (void)n;
  return (strcmp(arg1,arg2)!=0);
}

/*
 * Funtion that returns an argument value
 */
static char *CalcArg (long *TabArg,int *Ix)
{
  char *TmpStr;
  int i;

  if (TabArg[*Ix]>100000)       /* Number coding case */
  {
    i = (int)TabArg[*Ix] - 200000;
    xasprintf(&TmpStr,"%d",i);
  }
  else if (TabArg[*Ix] < -200000)/* Comparison fuction ID case */
  {
   i = TabArg[*Ix]+250000;
   xasprintf(&TmpStr,"%d",i);
  }
  else if (TabArg[*Ix] < -100000)       /* Function ID case */
  {
    TmpStr = TabFunc[TabArg[*Ix]+150000](Ix,TabArg);
  }
  else                          /* Variable case */
  {
    TmpStr=fxstrdup(TabVVar[TabArg[*Ix]]);
  }
  return (TmpStr);
}

/*
 * Functions to get an object properties
 */

/* GetValue */
static char *FuncGetValue(int *NbArg, long *TabArg)
{
  char *tmp;
  long Id;

  (*NbArg)++;              /* GetValue has one single argument */
  tmp = CalcArg(TabArg,NbArg);
  Id = atoi(tmp);
  free(tmp);
  xasprintf(&tmp,"%d",tabxobj[TabIdObj[Id]]->value);
  return tmp;
}

/* GetMinValue */
static char *FuncGetMinValue(int *NbArg, long *TabArg)
{
  char *tmp;
  long Id;

  (*NbArg)++;              /* GetValue has one single argument */
  tmp = CalcArg(TabArg,NbArg);
  Id = atoi(tmp);
  free(tmp);
  xasprintf(&tmp,"%d",tabxobj[TabIdObj[Id]]->value2);
  return tmp;
}

/* GetMaxValue */
static char *FuncGetMaxValue(int *NbArg, long *TabArg)
{
  char *tmp;
  long Id;

  (*NbArg)++;         /* GetValue has one single argument */
  tmp = CalcArg(TabArg,NbArg);
  Id = atoi(tmp);
  free(tmp);
  xasprintf(&tmp,"%d",tabxobj[TabIdObj[Id]]->value3);
  return tmp;
}

/* GetFore */
static char *FuncGetFore(int *NbArg, long *TabArg)
{
  char *tmp;
  long Id;
  XColor color;

  (*NbArg)++;           /* GetValue has one single argument */
  tmp = CalcArg(TabArg,NbArg);
  Id = atoi(tmp);
  free(tmp);
  color.pixel = tabxobj[TabIdObj[Id]]->TabColor[fore];
  XQueryColor(dpy, Pcmap, &color);
  xasprintf(&tmp, "%02x%02x%02x",
	  color.red >> 8, color.green >> 8, color.blue >> 8);
  return tmp;
}

/* GetBack */
static char *FuncGetBack(int *NbArg, long *TabArg)
{
  char *tmp;
  long Id;
  XColor color;

  (*NbArg)++;              /* GetValue has one single argument */
  tmp = CalcArg(TabArg,NbArg);
  Id = atoi(tmp);
  free(tmp);
  color.pixel = tabxobj[TabIdObj[Id]]->TabColor[back];
  XQueryColor(dpy, Pcmap, &color);
  xasprintf(&tmp, "%02x%02x%02x",
	  color.red >> 8, color.green >> 8, color.blue >> 8);
  return tmp;
}

/* GetHili */
static char *FuncGetHili(int *NbArg, long *TabArg)
{
  char *tmp;
  long Id;
  XColor color;

  (*NbArg)++;             /* GetValue has one single argument */
  tmp = CalcArg(TabArg,NbArg);
  Id = atoi(tmp);
  free(tmp);
  color.pixel = tabxobj[TabIdObj[Id]]->TabColor[hili];
  XQueryColor(dpy, Pcmap, &color);
  xasprintf(&tmp, "%02x%02x%02x",
	  color.red >> 8, color.green >> 8, color.blue >> 8);
  return tmp;
}

/* GetShad */
static char *FuncGetShad(int *NbArg, long *TabArg)
{
  char *tmp;
  long Id;
  XColor color;

  (*NbArg)++;             /* GetValue has one single argument */
  tmp = CalcArg(TabArg,NbArg);
  Id = atoi(tmp);
  free(tmp);
  color.pixel = tabxobj[TabIdObj[Id]]->TabColor[shad];
  XQueryColor(dpy, Pcmap, &color);
  xasprintf(&tmp, "%02x%02x%02x",
	  color.red >> 8, color.green >> 8, color.blue >> 8);
  return tmp;
}

/* Returns an object title */
static char *FuncGetTitle(int *NbArg, long *TabArg)
{
  char *tmp;
  long Id;

  (*NbArg)++;
  tmp = CalcArg(TabArg,NbArg);
  Id = atoi(tmp);
  free(tmp);
  if (TabIdObj[Id] != -1)
    tmp=fxstrdup(tabxobj[TabIdObj[Id]]->title);
  else
  {
    fvwm_debug(__func__,
               "[%s][GetTitle]: <<WARNING>> Widget %d doesn't exist\n",
               ScriptName, (int)Id);
    tmp=fxcalloc(1, sizeof(char));
    tmp[0]='\0';
  }
  return tmp;
}

/* Returns a command output */
static char *FuncGetOutput(int *NbArg, long *TabArg)
{
  char *cmndbuf;
  char *str;
  int line,index,i=2,j=0,k,NewWord;
  FILE *f;
  int maxsize=32000;

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

  if ((strcmp(Command,cmndbuf)) || ((time(NULL)-TimeCom) > 1) || (TimeCom == 0))
  {
    if ((f = popen(cmndbuf,"r")) == NULL)
    {
      fvwm_debug(__func__, "[%s][GetOutput]: can't run %s\n",ScriptName,
                 cmndbuf);
      str = fxcalloc(sizeof(char), 10);
      free(cmndbuf);
      return str;
    }
    else
    {
      int n;

      if (strcmp(Command,"None"))
	free(BufCom);
      BufCom = fxcalloc(sizeof(char), maxsize);
      n = fread(BufCom,1,maxsize,f);
      (void)n;
      pclose(f);
      strncpy(Command,cmndbuf,255);
      TimeCom=time(NULL);
    }
  }

  /* Search for the line */
  while ((i <= line) && (BufCom[j] != '\0'))
  {
    j++;
    if (BufCom[j] == '\n')
      i++;
  }

  /* Search for the word */
  if (index != -1)
  {
    if (i != 2) j++;
    i = 1;
    NewWord = 0;
    while ((i < index) && (BufCom[j] != '\n') && (BufCom[j] != '\0'))
    {
      j++;
      if (BufCom[j] == ' ')
      {
	if (NewWord)
	{
	  i++;
	  NewWord = 0;
	}
      }
      else
	if (!NewWord) NewWord = 1;
    }
    str = fxcalloc(sizeof(char), 255);
    sscanf(&BufCom[j],"%s",str);
  }
  else          /* Reading the full line */
  {
    if (i != 2) j++;
    k=j;
    while ((BufCom[k] != '\n') && (BufCom[k] != '\0'))
      k++;
    str = fxcalloc(sizeof(char), k - j + 1);
    memmove(str,&BufCom[j],k-j);
    str[k-j]='\0';
  }

  free(cmndbuf);
  return str;
}

/* Converting from decimal to hexadecimal */
static char *FuncNumToHex(int *NbArg, long *TabArg)
{
  char *str;
  int value,nbchar;
  int i,j;

  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  value = atoi(str);
  free(str);
  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  nbchar = atoi(str);
  free(str);

  j = xasprintf(&str,"%X",value);
  if (j < nbchar)
  {
    memmove(&str[nbchar-j],str,j);
    for (i=0; i < (nbchar-j); i++)
      str[i]='0';
  }

  return str;
}

/* Converting from hexadecimal to decimal */
static char *FuncHexToNum(int *NbArg, long *TabArg)
{
  char *str,*str2;
  int k;

  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  if (str[0] == '#')
    memmove(str,&str[1],strlen(str));
  k = (int)strtol(str,NULL,16);
  free(str);

  xasprintf(&str2,"%d",k);
  return str2;
}

/* + */
static char *FuncAdd(int *NbArg, long *TabArg)
{
  char *str;
  int val1,val2;

  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  val1 = atoi(str);
  free(str);
  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  val2 = atoi(str);
  free(str);
  xasprintf(&str,"%d",val1+val2);
  return str;
}

/* * */
static char *FuncMult(int *NbArg, long *TabArg)
{
  char *str;
  int val1,val2;

  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  val1 = atoi(str);
  free(str);
  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  val2 = atoi(str);
  free(str);
  xasprintf(&str,"%d",val1*val2);
  return str;
}

/* / */
static char *FuncDiv(int *NbArg, long *TabArg)
{
  char *str;
  int val1,val2;

  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  val1 = atoi(str);
  free(str);
  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  val2 = atoi(str);
  free(str);
  xasprintf(&str,"%d",val1/val2);
  return str;
}

/* % */
static char *RemainderOfDiv(int *NbArg, long *TabArg)
{
#ifndef HAVE_DIV
  return fxstrdup("Unsupported function: div");
#else
  char *str;
  int val1,val2;
  div_t res;

  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  val1 = atoi(str);
  free(str);
  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  val2 = atoi(str);
  free(str);
  res = div(val1,val2);
  xasprintf(&str,"%d",res.rem);
  return str;
#endif
}


/* StrCopy */
static char *FuncStrCopy(int *NbArg, long *TabArg)
{
  char *str,*strsrc;
  int i1,i2;

  (*NbArg)++;
  strsrc = CalcArg(TabArg,NbArg);
  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  i1 = atoi(str);
  if (i1 < 1) i1 = 1;
  free(str);
  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  i2 = atoi(str);
  if (i2 < 1) i2 = 1;
  free(str);

  if ((i1 <= i2) &&( i1 <= strlen(strsrc)))
  {
    if (i2 > strlen(strsrc))
      i2=strlen(strsrc);
    str = fxcalloc(1, i2 - i1 + 2);
    memmove(str,&strsrc[i1-1],i2 - i1 + 1);
  }
  else
  {
    str = fxcalloc(1, 1);
  }

 free(strsrc);
 return str;
}

/* Launching a script with pipe */
static char *LaunchScript (int *NbArg,long *TabArg)
{
  char *arg,*execstr,*str,*scriptarg,*scriptname;
  unsigned long leng;
  int i;
  Atom MyAtom;

  /* Reading the arguments */
  (*NbArg)++;
  arg = CalcArg(TabArg,NbArg);

  str = fxcalloc(100, sizeof(char));

 /* Computing the son script name */
  x11base->TabScriptId[x11base->NbChild+2] =
    fxcalloc(strlen(x11base->TabScriptId[1]) + 4, sizeof(char));

  if (x11base->NbChild<98)
  {
    i=16;
    do
    {
      sprintf(x11base->TabScriptId[x11base->NbChild + 2],"%s%x",
	      x11base->TabScriptId[1],i);
      MyAtom = XInternAtom(dpy,x11base->TabScriptId[x11base->NbChild + 2],False);
      i++;
    }
    while (XGetSelectionOwner(dpy,MyAtom)!=None);
  }
  else
  {
    fvwm_debug(__func__, "[%s][LaunchScript]: Too many launched script\n",
               ScriptName);
    sprintf(str,"-1");
    return str;
  }

  /* Building the command */
  scriptname = fxcalloc(sizeof(char), 100);
  sscanf(arg,"%s",scriptname);
  scriptarg = fxcalloc(sizeof(char), strlen(arg));
  scriptarg = (char*)strncpy(scriptarg, &arg[strlen(scriptname)],
			     strlen(arg) - strlen(scriptname));
  xasprintf(&execstr,"%s %s %s %s",module->name,scriptname,
	  x11base->TabScriptId[x11base->NbChild + 2],scriptarg);
  free(scriptname);
  free(scriptarg);
  free(arg);

  /* Sending the command */
  {
    int n;

    n = write(fd[0], &ref, sizeof(Window));
    (void)n;
    leng = strlen(execstr);
    n = write(fd[0], &leng, sizeof(unsigned long));
    n = write(fd[0], execstr, leng);
    leng = 1;
    n = write(fd[0], &leng, sizeof(unsigned long));
  }
  free(execstr);

  /* Returning the son ID */
  sprintf(str,"%d",x11base->NbChild+2);
  x11base->NbChild++;
  return str;
}

/* GetScriptFather */
static char *GetScriptFather (int *NbArg,long *TabArg)
{
  return fxstrdup("0");
}

/* GetTime */
static char *GetTime (int *NbArg,long *TabArg)
{
  char *str;
  time_t t;

  t = time(NULL);
  xasprintf(&str,"%lld",(long long)t-x11base->BeginTime);
  return str;
}

/* GetScriptArg */
static char *GetScriptArg (int *NbArg,long *TabArg)
{
  char *str;
  int val1;

  (*NbArg)++;
  str = CalcArg(TabArg,NbArg);
  val1 = atoi(str);
  free(str);

  if (x11base->TabArg[val1] != NULL)
  {
    str = fxcalloc(strlen(x11base->TabArg[val1]) + 1, sizeof(char));
    str = strcpy(str,x11base->TabArg[val1]);
  }
  else
  {
    str = fxcalloc(1, sizeof(char));
    str = strcpy(str,"");
  }

 return str;
}

/* ReceivFromScript */
static char *ReceivFromScript (int *NbArg,long *TabArg)
{
  char *arg,*msg;
  int send;
  Atom AReceiv,ASend,type;
  static XEvent event;
  unsigned long longueur,octets_restant;
  unsigned char *donnees = (unsigned char *)"";
  int format;
  int NbEssai = 0;

  (*NbArg)++;
  arg = CalcArg(TabArg,NbArg);
  send = (int)atoi(arg);
  free(arg);

  msg = fxstrdup("No message");

  /* Get atoms */
  AReceiv = XInternAtom(dpy,x11base->TabScriptId[1],True);
  if (AReceiv == None)
  {
    fvwm_debug(__func__,
               "[%s][ReceivFromScript]: <<WARNING>> Error with atome\n",
               ScriptName);
    return msg;
  }

  if ( (send >=0 ) && (send < 99))
  {
    if (x11base->TabScriptId[send]!=NULL)
    {
      ASend=XInternAtom(dpy,x11base->TabScriptId[send],True);
      if (ASend==None)
	fvwm_debug(__func__,
                   "[%s][ReceivFromScript]: <<WARNING>> Error with atome\n",
                   ScriptName);
    }
    else
      return msg;
  }
  else
    return msg;

  /* Get message */
  XConvertSelection(dpy,ASend,AReceiv,propriete,x11base->win,CurrentTime);
  while ((!FCheckTypedEvent(dpy,SelectionNotify,&event)) && (NbEssai < 10))
  {
    usleep(1);
    NbEssai++;
  }
  if (event.xselection.property!=None)
    if (event.xselection.selection==ASend)
    {
      XGetWindowProperty(dpy,event.xselection.requestor,
			 event.xselection.property,
			 0L,8192L,False,event.xselection.target,&type,&format,
			 &longueur,&octets_restant,&donnees);
      if (longueur > 0)
      {
	msg = fxrealloc((void *)msg, (longueur + 1) * sizeof(char),
                       sizeof((void *)msg));
	msg = strcpy(msg,(char *)donnees);
	XDeleteProperty(dpy,event.xselection.requestor,
			event.xselection.property);
	XFree(donnees);
      }
    }
  return msg;
}

/* GetPid */
static char *FuncGetPid(int *NbArg,long *TabArg)
{
  char *str;
  pid_t pid;

  pid = getpid();
  xasprintf(&str,"%d",pid);
  return str;
}

/*  SendMsgAndGet */
#define IN_FIFO_NBR_OF_TRY 200
#define IN_FIFO_TIMEOUT    100000 /* usec: 0.1 sec (20 sec) */
#define OUT_FIFO_NBR_OF_TRY  400
#define OUT_FIFO_TIMEOUT     50000 /* usec: 0.05 sec (20 sec) */
static char *FuncSendMsgAndGet(int *NbArg,long *TabArg)
{
  char *com_name, *cmd, *tmp, *out_fifo, *in_fifo, *str;
  char *buf = NULL;
  int maxsize=32000;
  int err = 0;
  int filedes = 0;
  int i,l,j,lock;
  FILE *f;
  struct timeval timeout;

  /* communication name */
  (*NbArg)++;
  com_name=CalcArg(TabArg,NbArg);
  /* the command sent to the receiver */
  (*NbArg)++;
  cmd=CalcArg(TabArg,NbArg);
  /* 0: no answer (so no locking) from the receiver  *
   * 1: real send and get mode                       */
  (*NbArg)++;
  tmp=CalcArg(TabArg,NbArg);
  lock=atoi(tmp);
  free(tmp);

  setFvwmUserDir();
  xasprintf(&in_fifo,"%s/.tmp-com-in-%s",FvwmUserDir,com_name);

  /* unlock the receiver, wait IN_FIFO_TIMEOUT * IN_FIFO_NBR_OF_TRY so that *
   * the receiver has the time to go in its communication loop at startup   *
   * or if it do an other job than waiting for a cmd                        */
  i = 0;
  while(1)
  {
    if (access(in_fifo,W_OK)  == 0)
    {
      if ((filedes = open(in_fifo,O_WRONLY)) == 0)
      {
	fvwm_debug(__func__,
                   "[%s][GetMsgAndGet]: <<WARNING>> cannot open the in fifo %s\n",
                   ScriptName,in_fifo);
	err = 1;
	break;
      }
      i = 0;
      l = strlen(cmd);
      while(l > i) {
	if ((j = write(filedes, cmd+i, l-i)) == -1) {
	  fvwm_debug(__func__,
                     "[%s][GetMsgAndGet]: <<WARNING>> write error on %s\n",
                     ScriptName,in_fifo);
	  err = 1;
	}
	i += j;
      }
      close(filedes);
      break;
    }
    else
    {
      timeout.tv_sec = 0;
      timeout.tv_usec = IN_FIFO_TIMEOUT;
      select(None,NULL,NULL,NULL,&timeout);
      i++;
      if (i > IN_FIFO_NBR_OF_TRY)
      {
	fvwm_debug(__func__,
                   "[%s][GetMsgAndGet]: <<WARNING>> No in fifo %s for communication %s\n",
                   ScriptName,in_fifo,com_name);
	close(filedes);
	err = 1;
	break;
      }
    }
  }
  free(in_fifo);

  /* no msg to read from the receiver (or an error occur): return */
  if (!lock || err)
  {
    free(cmd);
    free(com_name);
    return fxstrdup(err ? "0" : "1");
  }

  /* get the answer from the receiver.                              *
   * we wait OUT_FIFO_TIMEOUT * OUT_FIFO_NBR_OF_TRY for this answer */
  xasprintf(&out_fifo,"%s/.tmp-com-out-%s",FvwmUserDir,com_name);
  i = 0;
  while(1)
  {
    if (access(out_fifo,R_OK)  == 0)
    {
      if ((f = fopen (out_fifo, "r")) != NULL)
      {
	int n;

	buf=fxcalloc(sizeof(char), maxsize);
	n = fread(buf,1,maxsize,f);
	(void)n;
	fclose (f);
      }
      else
      {
	fvwm_debug(__func__,
                   "[%s][GetMsgAndGet]: <<WARNING>> cannot open the out fifo %s\n",
                   ScriptName,out_fifo);
	err = 1;
      }
      break;
    }
    else
    {
      timeout.tv_sec = 0;
      timeout.tv_usec = OUT_FIFO_TIMEOUT;
      select(None,NULL,NULL,NULL,&timeout);
      i++;
      if (i > OUT_FIFO_NBR_OF_TRY)
      {
	fvwm_debug(__func__,
                   "[%s][GetMsgAndGet]: <<WARNING>>: No out fifo %s for communication %s\n",
                   ScriptName,out_fifo,com_name);
	err = 1;
	break;
      }
    }
  }

  free(cmd);
  free(com_name);
  free(out_fifo);

  if (err)
  {
    free(buf);
    return fxstrdup("0");
  }
  l = strlen(buf);
  str=fxcalloc(l + 1, sizeof(char));
  memmove(str,&buf[0],l);
  free(buf);
  str[l]='\0';
  return str;
}

/* Parse */
static char *FuncParse(int *NbArg,long *TabArg)
{
  char *string,*str;
  char num[4];
  int index,toklen,l;
  int i = 0;
  int step = 1;
  int start = 0;
  int end = 0;

  /* the string to parse */
  (*NbArg)++;
  string=CalcArg(TabArg,NbArg);
  /* the substring to return */
  (*NbArg)++;
  str=CalcArg(TabArg,NbArg);
  index=atoi(str);
  free(str);

  l = strlen(string);
  while(i < l+4) {
    memmove(num,&string[i],4);
    toklen = atoi(num);
    if (step == index)
    {
      start = i+4;
      end = start+toklen;
      break;
    }
    step++;
    i += toklen + 4;
  }

  if (end > l || end <= start)
  {
    str=fxcalloc(1, sizeof(char));
    str[0] ='\0';
  }
  else
  {
    str=fxcalloc(end - start + 1, sizeof(char));
    memmove(str,&string[start],end-start);
    str[end-start] ='\0';
  }
  free(string);
  return str;
}

/* GetLastString */
static char *FuncGetLastString(int *NbArg,long *TabArg)
{
  char *str;

  if (LastString == NULL) {
    str = fxcalloc(1, sizeof(char));
    str[0] ='\0';
  }
  else
  {
    CopyString(&str,LastString);
  }
  return str;
}

/*
 * Object Commands
 */

/* Exec */
static void Exec (int NbArg,long *TabArg)
{
  unsigned long leng = 0;
  char *execstr;
  char *tempstr;
  int i;

  for (i=0; i < NbArg; i++)
    leng += strlen(CalcArg(TabArg,&i));

  if (leng >= 998)
  {
    fvwm_debug(__func__, "[%s][Do]: too long command %i chars max 998\n",
               ScriptName,(int)leng);
    return;
  }

  execstr = fxcalloc(1, leng + 1);
  for (i=0; i < NbArg; i++)
  {
    tempstr = CalcArg(TabArg,&i);
    execstr = strcat(execstr,tempstr);
    free(tempstr);
  }

  {
    int n;

    n = write(fd[0], &ref, sizeof(Window));
    (void)n;
    leng = strlen(execstr);
    n = write(fd[0], &leng, sizeof(unsigned long));
    n = write(fd[0], execstr, leng);
    leng = 1;
    n = write(fd[0], &leng, sizeof(unsigned long));
  }
  free(execstr);
}

/* Hide */
static void HideObj (int NbArg,long *TabArg)
{
  char *arg[1];
  int IdItem;
  int i = 0;

  arg[0] = CalcArg(TabArg,&i);
  IdItem = TabIdObj[atoi(arg[0])];


  tabxobj[IdItem]->flags[0] = True;
  /* We hide the window */
  XUnmapWindow(dpy,tabxobj[IdItem]->win);
  free(arg[0]);
}

/* Show */
static void ShowObj (int NbArg,long *TabArg)
{
  char *arg[1];
  int IdItem;
  int i = 0;

  arg[0] = CalcArg(TabArg,&i);
  IdItem = TabIdObj[atoi(arg[0])];

  tabxobj[IdItem]->flags[0] = False;
  XMapWindow(dpy,tabxobj[IdItem]->win);
  tabxobj[IdItem]->DrawObj(tabxobj[IdItem], NULL);
  free(arg[0]);
}

/* ChangeValue */
static void ChangeValue (int NbArg,long *TabArg)
{
  int i = 0;
  char *arg[2];

  arg[0] =CalcArg(TabArg,&i);
  i++;
  arg[1] =CalcArg(TabArg,&i);

  tabxobj[TabIdObj[atoi(arg[0])]]->value = atoi(arg[1]);
  /* Redraw the object to refresh it */
  if (tabxobj[TabIdObj[atoi(arg[0])]]->TypeWidget != SwallowExec)
    XClearWindow(dpy, tabxobj[TabIdObj[atoi(arg[0])]]->win);
  tabxobj[TabIdObj[atoi(arg[0])]]->DrawObj(tabxobj[TabIdObj[atoi(arg[0])]],NULL);
  free(arg[0]);
  free(arg[1]);
}

/* ChangeValueMax */
static void ChangeValueMax (int NbArg,long *TabArg)
{
  int i=0;
  char *arg[2];
  int j;

  arg[0] = CalcArg(TabArg,&i);
  j = atoi(arg[0]);
  i++;
  arg[1] = CalcArg(TabArg,&i);

  tabxobj[TabIdObj[j]]->value3 = atoi(arg[1]);
  /* Redraw the object to refresh it */
  if (tabxobj[TabIdObj[j]]->value > tabxobj[TabIdObj[j]]->value3)
  {
    tabxobj[TabIdObj[j]]->value = atoi(arg[1]);
    if (tabxobj[TabIdObj[j]]->TypeWidget != SwallowExec)
      XClearWindow(dpy, tabxobj[TabIdObj[j]]->win);
    tabxobj[TabIdObj[j]]->DrawObj(tabxobj[TabIdObj[j]],NULL);
  }
  free(arg[0]);
  free(arg[1]);
}

/* ChangeValueMin */
static void ChangeValueMin (int NbArg,long *TabArg)
{
  int i=0;
  char *arg[2];
  int j;

  arg[0] = CalcArg(TabArg,&i);
  i++;
  arg[1] = CalcArg(TabArg,&i);
  j = atoi(arg[0]);

  tabxobj[TabIdObj[j]]->value2 = atoi(arg[1]);
  /* Redraw the object to refresh it */
  if (tabxobj[TabIdObj[j]]->value < tabxobj[TabIdObj[j]]->value2)
  {
    tabxobj[TabIdObj[j]]->value = atoi(arg[1]);
    if (tabxobj[TabIdObj[j]]->TypeWidget != SwallowExec)
      XClearWindow(dpy, tabxobj[TabIdObj[j]]->win);
    tabxobj[TabIdObj[j]]->DrawObj(tabxobj[TabIdObj[j]],NULL);
  }
  free(arg[0]);
  free(arg[1]);
}

/* ChangePosition */
static void ChangePos (int NbArg,long *TabArg)
{
  int i=0;
  char *arg[3];
  int an[3];
  int IdItem;

  arg[0] = CalcArg(TabArg,&i);
  i++;
  arg[1] = CalcArg(TabArg,&i);
  i++;
  arg[2] = CalcArg(TabArg,&i);

  IdItem = TabIdObj[atoi(arg[0])];
  for (i=1; i<3; i++)
    an[i] = atoi(arg[i]);
  tabxobj[IdItem]->x = an[1];
  tabxobj[IdItem]->y = an[2];
  XMoveWindow(dpy,tabxobj[IdItem]->win,an[1],an[2]);

  free(arg[0]);
  free(arg[1]);
  free(arg[2]);
}

/* ChangeFont */
static void ChangeFont (int NbArg,long *TabArg)
{
  int i=0;
  char *arg[2];
  int IdItem;
  FlocaleFont *Ffont;

  arg[0] = CalcArg(TabArg,&i);
  i++;
  arg[1] = CalcArg(TabArg,&i);
  IdItem = TabIdObj[atoi(arg[0])];
  free(tabxobj[IdItem]->font);
  tabxobj[IdItem]->font = fxstrdup(arg[1]);

  if ((Ffont =
       FlocaleLoadFont(dpy, tabxobj[IdItem]->font, ScriptName)) == NULL) {
    fvwm_debug(__func__, "[%s][ChangeFont]: Couldn't load font %s\n",
               ScriptName,tabxobj[IdItem]->font);
  }
  else
  {
    FlocaleUnloadFont(dpy, tabxobj[IdItem]->Ffont);
    tabxobj[IdItem]->Ffont = Ffont;
    if (Ffont->font != NULL)
      XSetFont(dpy,tabxobj[IdItem]->gc,tabxobj[IdItem]->Ffont->font->fid);
  }

  if (tabxobj[IdItem]->TypeWidget != SwallowExec)
    XClearWindow(dpy, tabxobj[IdItem]->win);
  tabxobj[IdItem]->DrawObj(tabxobj[IdItem],NULL);
  free(arg[0]);
  free(arg[1]);
}

/* ChangeSize */
static void ChangeSize (int NbArg,long *TabArg)
{
  int i=0;
  char *arg[3];
  int an[3];
  int IdItem;

  arg[0] = CalcArg(TabArg,&i);
  i++;
  arg[1] = CalcArg(TabArg,&i);
  i++;
  arg[2] = CalcArg(TabArg,&i);

  IdItem = TabIdObj[atoi(arg[0])];
  for (i=1; i<3; i++) {
    an[i] = atoi(arg[i]);
  }
  tabxobj[IdItem]->width = an[1];
  tabxobj[IdItem]->height = an[2];
  XResizeWindow(dpy,tabxobj[IdItem]->win,an[1],an[2]);
  tabxobj[IdItem]->DrawObj(tabxobj[IdItem],NULL);
  free(arg[0]);
  free(arg[1]);
  free(arg[2]);
}

/* ChangeTitle */
static void ChangeTitle (int NbArg,long *TabArg)
{
  int i=0;
  char *arg[2];
  int IdItem;

  arg[0] = CalcArg(TabArg,&i);
  i++;
  arg[1] = CalcArg(TabArg,&i);
  IdItem = TabIdObj[atoi(arg[0])];
  free(tabxobj[IdItem]->title);
  tabxobj[IdItem]->title=fxstrdup(arg[1]);
  if (tabxobj[IdItem]->TypeWidget != SwallowExec)
    XClearWindow(dpy, tabxobj[IdItem]->win);
  tabxobj[IdItem]->DrawObj(tabxobj[IdItem],NULL);
  free(arg[0]);
  free(arg[1]);
}

/* ChangeLocaleTitle */
static void ChangeLocaleTitle (int NbArg,long *TabArg)
{
  int i=0;
  char *arg[2];
  int IdItem;

  arg[0] = CalcArg(TabArg,&i);
  i++;
  arg[1] = CalcArg(TabArg,&i);
  IdItem = TabIdObj[atoi(arg[0])];
  free(tabxobj[IdItem]->title);
  tabxobj[IdItem]->title=fxstrdup(FGettext(arg[1]));
  if (tabxobj[IdItem]->TypeWidget != SwallowExec)
    XClearWindow(dpy, tabxobj[IdItem]->win);
  tabxobj[IdItem]->DrawObj(tabxobj[IdItem],NULL);
  free(arg[0]);
  free(arg[1]);
}

/* ChangeIcon */
static void ChangeIcon (int NbArg,long *TabArg)
{
	int i=0;
	char *arg[2];
	int IdItem;

	arg[0] = CalcArg(TabArg,&i);
	i++;
	arg[1] = CalcArg(TabArg,&i);
	IdItem = TabIdObj[atoi(arg[0])];
	if (tabxobj[IdItem]->icon)
	{
		free(tabxobj[IdItem]->icon);
	}
	tabxobj[IdItem]->icon = fxstrdup(arg[1]);
	LoadIcon(tabxobj[IdItem]);
	if (tabxobj[IdItem]->TypeWidget != SwallowExec)
	{
		XClearWindow(dpy, tabxobj[IdItem]->win);
	}
	tabxobj[IdItem]->DrawObj(tabxobj[IdItem],NULL);
	free(arg[0]);
	free(arg[1]);
}

/* ChangeForeColor */
static void ChangeForeColor (int NbArg,long *TabArg)
{
  int i=0;
  char *arg[2];
  int IdItem;

  arg[0] = CalcArg(TabArg,&i);
  i++;
  arg[1] = CalcArg(TabArg,&i);
  IdItem = TabIdObj[atoi(arg[0])];

  /* Free color */
  if (tabxobj[IdItem]->colorset < 0)
    PictureFreeColors(
	    dpy,Pcmap,(void*)(&(tabxobj[IdItem])->TabColor[fore]),1,0, True);

  if (tabxobj[IdItem]->forecolor)
  {
	  free(tabxobj[IdItem]->forecolor);
  }
  tabxobj[IdItem]->forecolor= fxstrdup(arg[1]);

  tabxobj[IdItem]->TabColor[fore] = GetColor(tabxobj[IdItem]->forecolor);
  if (tabxobj[IdItem]->colorset >= 0) {
    tabxobj[IdItem]->TabColor[back] = GetColor(tabxobj[IdItem]->backcolor);
    tabxobj[IdItem]->TabColor[hili] = GetColor(tabxobj[IdItem]->hilicolor);
    tabxobj[IdItem]->TabColor[shad] = GetColor(tabxobj[IdItem]->shadcolor);
  }
  tabxobj[IdItem]->colorset = -1;

  if (tabxobj[IdItem]->TypeWidget != SwallowExec)
    XClearWindow(dpy, tabxobj[IdItem]->win);
  tabxobj[IdItem]->DrawObj(tabxobj[IdItem],NULL);

  free(arg[0]);
  free(arg[1]);

}

/* ChangeBackColor */
static void ChangeBackColor (int NbArg,long *TabArg)
{
  int i=0;
  char *arg[2];
  int IdItem;

  arg[0] = CalcArg(TabArg,&i);
  i++;
  arg[1] = CalcArg(TabArg,&i);
  IdItem = TabIdObj[atoi(arg[0])];

  /* Free color */
  if (tabxobj[IdItem]->colorset < 0)
    PictureFreeColors(
	    dpy,Pcmap,(void*)(&(tabxobj[IdItem])->TabColor[back]),1,0,True);

  if (tabxobj[IdItem]->backcolor)
  {
	  free(tabxobj[IdItem]->backcolor);
  }
  tabxobj[IdItem]->backcolor= fxstrdup(arg[1]);

  tabxobj[IdItem]->TabColor[back] = GetColor(tabxobj[IdItem]->backcolor);
  if (tabxobj[IdItem]->colorset >= 0) {
    tabxobj[IdItem]->TabColor[fore] = GetColor(tabxobj[IdItem]->forecolor);
    tabxobj[IdItem]->TabColor[hili] = GetColor(tabxobj[IdItem]->hilicolor);
    tabxobj[IdItem]->TabColor[shad] = GetColor(tabxobj[IdItem]->shadcolor);
  }
  tabxobj[IdItem]->colorset = -1;

  if (tabxobj[IdItem]->TypeWidget != SwallowExec) {
    XSetWindowBackground(dpy,
			 tabxobj[IdItem]->win,
			 tabxobj[IdItem]->TabColor[back]);
    XClearWindow(dpy, tabxobj[IdItem]->win);
  }
  tabxobj[IdItem]->DrawObj(tabxobj[IdItem],NULL);

  free(arg[0]);
  free(arg[1]);
}

/* ChangeMainColorset */
static void ChangeMainColorset (int i)
{
 /* Free color */
  if (x11base->colorset < 0) {
    PictureFreeColors(dpy,Pcmap,&x11base->TabColor[fore],1,0,True);
    PictureFreeColors(dpy,Pcmap,&x11base->TabColor[back],1,0,True);
    PictureFreeColors(dpy,Pcmap,&x11base->TabColor[hili],1,0,True);
    PictureFreeColors(dpy,Pcmap,&x11base->TabColor[shad],1,0,True);
  }
  x11base->colorset = i;
  AllocColorset(i);
  x11base->TabColor[fore] = Colorset[i].fg;
  x11base->TabColor[back] = Colorset[i].bg;
  x11base->TabColor[hili] = Colorset[i].hilite;
  x11base->TabColor[shad] = Colorset[i].shadow;
  SetWindowBackground(dpy, x11base->win,x11base->size.width,x11base->size.height,
		      &Colorset[i], Pdepth, x11base->gc, True);
}

/* ChangeMainColorset */
static void ChangeColorset (int NbArg,long *TabArg)
{
  int i=0;
  char *arg[2];
  int IdItem;

  arg[0] = CalcArg(TabArg,&i);
  i++;
  arg[1] = CalcArg(TabArg,&i);
  if (atoi(arg[0]) == 0) {
    ChangeMainColorset(atoi(arg[1]));
    free(arg[0]);
    free(arg[1]);
    return;
  }
  IdItem = TabIdObj[atoi(arg[0])];

  /* Free color */
  if (tabxobj[IdItem]->colorset < 0) {
    PictureFreeColors(dpy,Pcmap,&tabxobj[IdItem]->TabColor[fore],1,0,True);
    PictureFreeColors(dpy,Pcmap,&tabxobj[IdItem]->TabColor[back],1,0,True);
    PictureFreeColors(dpy,Pcmap,&tabxobj[IdItem]->TabColor[hili],1,0,True);
    PictureFreeColors(dpy,Pcmap,&tabxobj[IdItem]->TabColor[shad],1,0,True);
  }
  sscanf(arg[1], "%d", &i);
  tabxobj[IdItem]->colorset = i;
  AllocColorset(i);
  tabxobj[IdItem]->TabColor[fore] = Colorset[i].fg;
  tabxobj[IdItem]->TabColor[back] = Colorset[i].bg;
  tabxobj[IdItem]->TabColor[hili] = Colorset[i].hilite;
  tabxobj[IdItem]->TabColor[shad] = Colorset[i].shadow;

  if (tabxobj[IdItem]->TypeWidget != SwallowExec) {
    SetWindowBackground(dpy, tabxobj[IdItem]->win, tabxobj[IdItem]->width,
			tabxobj[IdItem]->height, &Colorset[i], Pdepth,
			tabxobj[IdItem]->gc, False);
    XClearWindow(dpy, tabxobj[IdItem]->win);
  }
  tabxobj[IdItem]->DrawObj(tabxobj[IdItem],NULL);

  free(arg[0]);
  free(arg[1]);
}

/* ChangeWindowTitle */
static void ChangeWindowTitle(int NbArg,long * TabArg){

  char *arg;
  int tmpVal=NbArg-1;

  arg=CalcArg(TabArg,&tmpVal);
  XChangeProperty(
	  dpy, x11base->win, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
	  (unsigned char*)arg, strlen(arg));
  free(arg);
}


/* ChangeWindowTitleFromArg */
static void ChangeWindowTitleFromArg(int NbArg,long * TabArg){

  char *arg;
  int argVal;
  int tmpVal=NbArg-1;

  arg=CalcArg(TabArg,&tmpVal);
  argVal = atoi(arg);
  free(arg);

  if(x11base->TabArg[argVal]!=NULL){
    arg =  fxcalloc(strlen(x11base->TabArg[argVal]) + 1, sizeof(char));
    arg = strcpy(arg,x11base->TabArg[argVal]);
  }else{
    arg =  fxcalloc(1, sizeof(char));
    arg = strcpy(arg,"");
  }

  XChangeProperty(
	  dpy, x11base->win, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
	  (unsigned char*)arg, strlen(arg));
  free(arg);
}


/* SetVar */
static void SetVar (int NbArg,long *TabArg)
{
  int i;
  char *str,*tempstr;

  str=fxcalloc(sizeof(char), 1);
  for (i=1; i<NbArg; i++)
  {
    tempstr = CalcArg(TabArg,&i);
    str = fxrealloc((void *)str,
                   sizeof(char) * (1 + strlen(str) + strlen(tempstr)),
                   sizeof((void *)str));
    str = strcat(str,tempstr);
    free(tempstr);
  }

  free(TabVVar[TabArg[0]]);
  TabVVar[TabArg[0]] = str;
}

/* SendSign */
static void SendSign (int NbArg,long *TabArg)
{
  int i=0;
  char *arg[2];
  int IdItem;
  int TypeMsg;

  arg[0] = CalcArg(TabArg,&i);
  i++;
  arg[1] = CalcArg(TabArg,&i);
  IdItem = TabIdObj[atoi(arg[0])];
  TypeMsg = atoi(arg[1]);
  SendMsg(tabxobj[IdItem],TypeMsg);
  free(arg[0]);
  free(arg[1]);
}

/* MyWarpPointer */
static void MyWarpPointer(int NbArg,long *TabArg)
{
  int i=0;
  char *arg;
  int IdItem;

  arg=CalcArg(TabArg,&i);
  IdItem= TabIdObj[atoi(arg)];
  /* Move pointer toward object */
  FWarpPointer(dpy, None, tabxobj[IdItem]->win, 0, 0, 0, 0,
	       tabxobj[IdItem]->width / 2, tabxobj[IdItem]->height + 10);
  free(arg);
}

/* Quit */
void Quit (int NbArg,long *TabArg)
{
  exit(0);
}

/* IfThen */
static void IfThen (int NbArg,long *TabArg)
{
  char *arg[10];
  int i,j;
  int CurrArg=0;
  int IdFuncComp = 0;

  /* Checking the condition */
  for (j=0; j<NbArg-2; j++)
  {
    if (TabArg[j] > 100000)     /* Number coding case */
    {
      i = (int)TabArg[j] - 200000;
      xasprintf(&arg[CurrArg],"%d",i);
      CurrArg++;
    }
    else if (TabArg[j] < -200000)/* Comparison function ID case */
    {
      IdFuncComp = TabArg[j] + 250000;
    }
    else if (TabArg[j] < -100000)       /* Function ID case */
    {
      arg[CurrArg] = TabFunc[TabArg[j]+150000](&j,TabArg);
      CurrArg++;
    }
    else                                /* Variable case */
    {
      arg[CurrArg] = fxstrdup(TabVVar[TabArg[j]]);
      CurrArg++;
    }
  }

  /* Comparing the arguments */
  if (TabComp[IdFuncComp](arg[0],arg[1]))
    ExecBloc((Bloc*)TabArg[NbArg-2]);
  else if (TabArg[NbArg-1]!=0)
    ExecBloc((Bloc*)TabArg[NbArg-1]);

  free(arg[0]);
  free(arg[1]);
}

/** Loop Instruction **/
static void Loop (int NbArg,long *TabArg)
{
  char *arg[2];
  int limit[2];
  int i;
  int CurrArg=0;

  /* First argument is a variable */
  /* Variable size is tuned for storing a number */
  TabVVar[TabArg[0]] = fxrealloc(TabVVar[TabArg[0]], sizeof(char) * 10,
                                sizeof(TabVVar[TabArg[0]]));
  /* Compute the 2 remaining arguments */
  for (i=1; i<NbArg; i++)
  {
    if (TabArg[i] > 100000)     /* Number coding case */
    {
      int x;
      x = (int)TabArg[i] - 200000;
      xasprintf(&arg[CurrArg],"%d",x);
    }
    else if (TabArg[i] < -100000)       /* Function ID case */
    {
      arg[CurrArg] = TabFunc[TabArg[i]+150000](&i,TabArg);
    }
    else                                /* Variable case */
    {
      arg[CurrArg] = fxstrdup(TabVVar[TabArg[i]]);
    }
    CurrArg++;
  }
  limit[0] = atoi(arg[0]);
  limit[1] = atoi(arg[1]);
  if (limit[0] < limit[1])
  {
    for (i=limit[0]; i<=limit[1]; i++)
    {
      /* Refeshing the variable */
      sprintf(TabVVar[TabArg[0]],"%d",i);
      ExecBloc((Bloc*)TabArg[NbArg-1]);
    }
  }
  else
  {
    for (i=limit[0]; i<=limit[1]; i++)
    {
      sprintf(TabVVar[TabArg[0]],"%d",i);
      ExecBloc((Bloc*)TabArg[NbArg-1]);
    }
  }

 free(arg[0]);
 free(arg[1]);
}

/** While Instruction **/
static void While (int NbArg,long *TabArg)
{
  char *arg[3],*str;
  int i;
  int Loop=1;
  int IdFuncComp;

  while (Loop)
  {
    i = 0;
    arg[0] = CalcArg(TabArg,&i);
    i++;
    str = CalcArg(TabArg,&i);
    IdFuncComp = atoi(str);
    free(str);
    i++;
    arg[1] = CalcArg(TabArg,&i);

    Loop=TabComp[IdFuncComp](arg[0],arg[1]);
    if (Loop) ExecBloc((Bloc*)TabArg[NbArg-1]);
    free(arg[0]);
    free(arg[1]);
  }
}

/* WriteToFile */
static void WriteToFile (int NbArg,long *TabArg)
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

  arg[0] = CalcArg(TabArg,&i);
  arg[1]=fxcalloc(1, 256);
  for (i=1; i<NbArg; i++)
  {
    tempstr = CalcArg(TabArg,&i);
    arg[1] = strcat(arg[1],tempstr);
    free(tempstr);
  }
  if (arg[1][strlen(arg[1])-1] != '\n')
  {
    i = strlen(arg[1]);
    arg[1] = fxrealloc(arg[1], strlen(arg[1]) + 2, sizeof(arg[1]));
    arg[1][i] = '\n';
    arg[1][i+1] = '\0';
  }

  strlcpy(StrEnd,"#end\n",sizeof(StrEnd));
  snprintf(StrBegin,sizeof(StrBegin),"#%s,",ScriptName);

  buf=fxcalloc(1, maxsize);

  if (arg[0][0] != '/')
  {
    file = fxstrdup(arg[0]);
    home = getenv("HOME");
    free(arg[0]);
    xasprintf(&arg[0],"%s/%s",home,file);
    free(file);
  }
  f = fopen(arg[0],"a+");
  fseek(f, 0, SEEK_SET);
  size = fread(buf, 1 ,maxsize,f);
  while(((strncmp(StrBegin, &buf[CurrPos], strlen(StrBegin))) !=0) &&
	(CurrPos<size))
  {
    CurrPos++;
  }
  if (CurrPos==size)
  {
    size_t l = strlen(buf);
    if (l < maxsize)
      snprintf(buf + l, maxsize - l, "\n%s%d\n%s%s\n",StrBegin,getpid(),arg[1],StrEnd);
  }
  else
  {
    sscanf(&buf[CurrPos+strlen(StrBegin)],"%d",&OldPID);
    if (OldPID == getpid())
    {
      snprintf(str,sizeof(str),"%d\n",OldPID);
      while(((strncmp(StrEnd,&buf[CurrPos],strlen(StrEnd))) != 0) &&
	    (CurrPos<size))
      {
	CurrPos++;
      }
      memmove(&buf[CurrPos+strlen(arg[1])],&buf[CurrPos],strlen(buf)-CurrPos);
      memmove(&buf[CurrPos],arg[1],strlen(arg[1]));
    }
    else          /* Replacing old commands */
    {
      CurrPos = CurrPos+strlen(StrBegin);
      CurrPos2 = CurrPos;
      while(((strncmp(StrEnd,&buf[CurrPos2],strlen(StrEnd))) != 0) &&
	    (CurrPos2<size))
      {
	CurrPos2++;
      }
      snprintf(str,sizeof(str),"%d\n%s",getpid(),arg[1]);
      memmove(&buf[CurrPos+strlen(str)], &buf[CurrPos2], strlen(buf)-CurrPos2);
      buf[strlen(buf)-((CurrPos2-CurrPos)-strlen(str))] = '\0';
      memmove(&buf[CurrPos],str,strlen(str));
    }
  }

  fclose(f);
  f= fopen(arg[0],"w");
  if (f == NULL)
  {
    fvwm_debug(__func__, "[%s][WriteToFile]: Unable to open file %s\n",
               ScriptName,arg[0]);
    return;
  }
  fwrite(buf,1,strlen(buf),f);
  fclose(f);

  free(arg[0]);
  free(arg[1]);
}

/* SendToScript */
static void SendToScript (int NbArg,long *TabArg)
{
  char *tempstr,*Msg,*R;
  int dest;
  int j=0;
  Atom myatom;

  /* Compute Receiver */
  tempstr=CalcArg(TabArg,&j);
  dest=(int)atoi(tempstr);
  free(tempstr);

  /* Compute what's inside */
  Msg=fxcalloc(256, sizeof(char));
  for (j=1;j<NbArg;j++)
  {
    tempstr=CalcArg(TabArg,&j);
    Msg=fxrealloc((void *)Msg, strlen(Msg) + strlen(tempstr) + 1,
                 sizeof((void *)Msg));
    Msg=strcat(Msg,tempstr);
    free(tempstr);
  }

  /* Compute Receiver */
  R=fxstrdup(x11base->TabScriptId[dest]);
  myatom=XInternAtom(dpy,R,True);

  if ((BuffSend.NbMsg<40)&&(XGetSelectionOwner(dpy,myatom)!=None))
  {
    /* Storing into message buffer */
    BuffSend.TabMsg[BuffSend.NbMsg].Msg=Msg;
    /* Storing into receiver buffer */
    BuffSend.TabMsg[BuffSend.NbMsg].R=R;
    /* Storing the message */
    BuffSend.NbMsg++;

    /* Waking the receiver up */
    XConvertSelection(dpy,
		      XInternAtom(dpy,x11base->TabScriptId[dest],True),
		      propriete, propriete, x11base->win, CurrentTime);
  }
  else
  {
    fvwm_debug(__func__, "[%s][SendToScript]: Too many messages sended\n",
               ScriptName);
    free(Msg);
  }
}

/* Key */
static void Key (int NbArg,long *TabArg)
{
  char *key_string,*in_modifier,*action,*str,*tmp,*widget,*sig;
  int modifier;
  int error = 0;
  int i,j=0;
  KeySym keysym = NoSymbol;

  /* the key */
  key_string = CalcArg(TabArg,&j);
  /* the modifier */
  j++;
  in_modifier = CalcArg(TabArg,&j);
  /* the widget */
  j++;
  widget = CalcArg(TabArg,&j);
  /* the signal */
  j++;
  sig = CalcArg(TabArg,&j);

  /* the string */
  str = fxcalloc(256,sizeof(char));
  for (i=j+1; i<NbArg; i++)
  {
    tmp = CalcArg(TabArg,&i);
    str = fxrealloc((void*)str, strlen(str) + strlen(tmp) + 1, sizeof(str));
    str = strcat(str, tmp);
    free(tmp);
  }

  xasprintf(&tmp, "%s %s", widget, sig);
  xasprintf(&action, "%s %s", tmp, str);

  free(sig);
  free(widget);
  free(str);
  free(tmp);

  keysym = FvwmStringToKeysym(dpy, key_string);
  if (keysym == 0)
  {
    fvwm_debug(__func__, "[%s][Key]: No such key: %s", ScriptName, key_string);
    error = 1;
  }

  if (modifiers_string_to_modmask(in_modifier, &modifier)) {
    fvwm_debug(__func__, "[%s][Key]: bad modifier: %s\n",
               ScriptName,in_modifier);
    error = 1;
  }

  if (error) {
    free(key_string);
    free(in_modifier);
    free(action);
    return;
  }

  AddBinding(
	  dpy, &BindingsList, BIND_KEYPRESS, 0, keysym,
	  key_string, modifier, C_WINDOW, (void *)action, NULL, NULL);
  free(key_string);
  free(in_modifier);
}

/*
 * GetText Support
 */

static char *FuncGettext(int *NbArg,long *TabArg)
{
	char *str;
	char *string;

	(*NbArg)++;
	str = CalcArg(TabArg,NbArg);

	string = FGettextCopy(str);
	if (str && string != str)
	{
		free(str);
	}
	return (char *)string;
}

/*
 * Initializing TabCom and TabFunc
 */
void InitCom(void)
{
  /* Command */
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
  TabCom[17]=MyWarpPointer;
  TabCom[18]=WriteToFile;
  TabCom[19]=ChangeBackColor;
  TabCom[21]=ChangeValueMax;
  TabCom[22]=ChangeValueMin;
  TabCom[23]=SendToScript;
  TabCom[24]=ChangeColorset;
  TabCom[25]=Key;
  TabCom[26]=ChangeLocaleTitle;
  TabCom[27]=ChangeWindowTitle;
  TabCom[28]=ChangeWindowTitleFromArg;

  /* Function */
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
  TabFunc[16]=FuncGetFore;
  TabFunc[17]=FuncGetBack;
  TabFunc[18]=FuncGetHili;
  TabFunc[19]=FuncGetShad;
  TabFunc[20]=FuncGetMinValue;
  TabFunc[21]=FuncGetMaxValue;
  TabFunc[22]=FuncGetPid;
  TabFunc[23]=FuncSendMsgAndGet;
  TabFunc[24]=FuncParse;
  TabFunc[25]=FuncGetLastString;
  TabFunc[26]=FuncGettext;

  /* Comparison Function */
  TabComp[1]=Inf;
  TabComp[2]=InfEq;
  TabComp[3]=Equal;
  TabComp[4]=SupEq;
  TabComp[5]=Sup;
  TabComp[6]=Diff;
}
