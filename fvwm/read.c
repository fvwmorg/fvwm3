/****************************************************************************
 * This module is all original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/
#include "../configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

extern Boolean debugging;

char *fvwm_file = NULL;

struct moduleInfoList
{
  char *data;
  struct moduleInfoList *next;
};

struct moduleInfoList *modlistroot = NULL;

int numfilesread = 0;

static int last_read_failed=0;

#ifdef FVWMRC /* FVWMRC should be .fvwm2rc or .fvwmrc */
static const char *read_system_rc_cmd="Read system"FVWMRC;
#else
static const char *read_system_rc_cmd="Read system.fvwm2rc";
#endif

void AddToModList(char *tline);

extern void StartupStuff(void);

/*
** func to do actual read/piperead work
*/
static void ReadSubFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                        unsigned long context, char *action,int* Module,
                        int piperead)
{
  char *filename= NULL,*Home, *home_file, *ofilename = NULL;
  char *rest,*tline,line[1000];
  int HomeLen;
  FILE *fd;
  int thisfileno;
  extern XEvent Event;

  thisfileno = numfilesread;
  numfilesread++;

/*  fvwm_msg(INFO,piperead?"PipeRead":"Read","action == '%s'",action); */

  rest = GetNextToken(action,&ofilename);
  if(ofilename == NULL)
  {
    fvwm_msg(ERR,piperead?"PipeRead":"Read","missing parameter");
    last_read_failed = 1;
    return;
  }

  filename = ofilename;
/*  fvwm_msg(INFO,piperead?"PipeRead":"Read","trying '%s'",filename); */

  if (piperead)
    fd = popen(filename,"r");
  else
    fd = fopen(filename,"r");

  if (!piperead)
  {
    if((fd == NULL)&&(ofilename[0] != '/'))
    {
      /* find the home directory to look in */
      Home = getenv("HOME");
      if (Home == NULL)
        Home = "./";
      HomeLen = strlen(Home);
      home_file = safemalloc(HomeLen + strlen(ofilename)+3);
      strcpy(home_file,Home);
      strcat(home_file,"/");
      strcat(home_file,ofilename);
      filename = home_file;
      fd = fopen(filename,"r");      
    }
    if((fd == NULL)&&(ofilename[0] != '/'))
    {
      if((filename != NULL)&&(filename!= ofilename))
        free(filename);
      /* find the home directory to look in */
      Home = FVWMDIR;
      HomeLen = strlen(Home);
      home_file = safemalloc(HomeLen + strlen(ofilename)+3);
      strcpy(home_file,Home);
      strcat(home_file,"/");
      strcat(home_file,ofilename);
      filename = home_file;
      fd = fopen(filename,"r");      
    }
  }

  if(fd == NULL)
  {
    fvwm_msg(ERR,
             piperead?"PipeRead":"Read",
             piperead?"command '%s' not run":"file '%s' not found in $HOME or "FVWMDIR,
             ofilename);
    if((ofilename != filename)&&(filename != NULL))
    {
      free(filename);
      filename = NULL;
    }
    if(ofilename != NULL)
    {
      free(ofilename);
      ofilename = NULL;
    }
    last_read_failed = 1;
    return;
  }
  if((ofilename != NULL)&&(filename!= ofilename))
    free(ofilename);
  fcntl(fileno(fd), F_SETFD, 1);
  if(fvwm_file != NULL)
    free(fvwm_file);
  fvwm_file = filename;

  tline = fgets(line,(sizeof line)-1,fd);
  while(tline != (char *)0)
  {
    int l;
    while(tline && (l=strlen(line))<sizeof(line) &&
          line[l-1]=='\n' && line[l-2]=='\\')
    {
      tline = fgets(line+l-2,sizeof(line)-l,fd);
    }
    tline=line;
    while(isspace(*tline))tline++;
    if (debugging)
    {
      fvwm_msg(DBG,"ReadSubFunc","about to exec: '%s'",tline);
    }
    /* should these next checks be moved into ExecuteFunction? */
    if((strlen(&tline[0])>1)&&(tline[0]!='#')&&(tline[0]!='*'))
      ExecuteFunction(tline,tmp_win,eventp,context,*Module);
    if(tline[0] == '*')
      AddToModList(tline);
    tline = fgets(line,(sizeof line)-1,fd);
  }

  if (piperead)
    pclose(fd);
  else
    fclose(fd);
  last_read_failed = 0;
}

void ReadFile(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  int this_read = numfilesread;

  if (debugging)
  {
    fvwm_msg(DBG,"ReadFile","about to attempt '%s'",action);
  }

  ReadSubFunc(eventp,junk,tmp_win,context,action,Module,0);

  if (last_read_failed && this_read == 0)
  {
    fvwm_msg(INFO,"Read","trying to read system rc file");
    ExecuteFunction((char *)read_system_rc_cmd,NULL,&Event,C_ROOT,-1);
  }

  if (this_read == 0)
  {
    if (debugging)
    {
      fvwm_msg(DBG,"ReadFile","about to call startup functions");
    }
    StartupStuff();
  }
}

void PipeRead(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  int this_read = numfilesread;

  if (debugging)
  {
    fvwm_msg(DBG,"PipeRead","about to attempt '%s'",action);
  }

  ReadSubFunc(eventp,junk,tmp_win,context,action,Module,1);

  if (last_read_failed && this_read == 0)
  {
    fvwm_msg(INFO,"PipeRead","trying to read system rc file");
    ExecuteFunction((char *)read_system_rc_cmd,NULL,&Event,C_ROOT,-1);
  }

  if (this_read == 0)
  {
    if (debugging)
    {
      fvwm_msg(DBG,"PipeRead","about to call startup functions");
    }
    StartupStuff();
  }
}

void AddToModList(char *tline)
{
  struct moduleInfoList *t, *prev, *this;

  /* Find end of list */
  t = modlistroot;
  prev = NULL;

  while(t != NULL)
  {
    prev = t;
    t = t->next;
  }
  
  this = (struct moduleInfoList *)safemalloc(sizeof(struct moduleInfoList));
  this->data = (char *)safemalloc(strlen(tline)+1);
  this->next = NULL;
  strcpy(this->data, tline);  
  if(prev == NULL)
  {
    modlistroot = this;
  }
  else
    prev->next = this;
}
      
/* interface function for AddToModList */
void AddModConfig(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module)
{
  AddToModList( action );
}

/**************************************************************/
/* delete from module configuration                           */
/**************************************************************/
void DestroyModConfig(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                      unsigned long context, char *action,int* Module)
{
  struct moduleInfoList *this, *that, *prev;
  char *info;   /* info to be deleted - may contain wildcards */
  char *mi;

  action = GetNextToken(action,&info); 
  if( info == NULL )
  {
    return;
  }

  this = modlistroot;
  prev = NULL;

  while(this != NULL)
  {
    GetNextToken( this->data, &mi);
    that = this->next;
    if( matchWildcards(info, mi+1) )
    {
      free(this->data);
      free(this);
      if( prev )
      {
        prev->next = that;
      }
      else
      {
        modlistroot = that;
      }
    }
    else
    {
      prev = this;
    }
    this = that;
  }
}

void SendDataToModule(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	      unsigned long context, char *action, int *Module)
{
  struct moduleInfoList *t;
  char *message,msg2[32];
  extern char *IconPath;
#ifdef XPM
  extern char *PixmapPath;
#endif

  if (IconPath && strlen(IconPath))
  {
    message=safemalloc(strlen(IconPath)+11);
    sprintf(message,"IconPath %s\n",IconPath);
    SendName(*Module,M_CONFIG_INFO,0,0,0,message);
    free(message);
  }
#ifdef XPM
  if (PixmapPath && strlen(PixmapPath))
  {
    message=safemalloc(strlen(PixmapPath)+13);
    sprintf(message,"PixmapPath %s\n",PixmapPath);
    SendName(*Module,M_CONFIG_INFO,0,0,0,message);
    sprintf(message,"ColorLimit %d\n",Scr.ColorLimit);
    SendName(*Module,M_CONFIG_INFO,0,0,0,message);
    free(message);
  }
#endif
  sprintf(msg2,"ClickTime %d\n",Scr.ClickTime);
  SendName(*Module,M_CONFIG_INFO,0,0,0,msg2);

  t = modlistroot;
  while(t != NULL)
  {
    SendName(*Module,M_CONFIG_INFO,0,0,0,t->data);
    t = t->next;
  }  
  SendPacket(*Module,M_END_CONFIG_INFO,0,0,0,0,0,0,0,0);
}
