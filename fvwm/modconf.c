/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/*
 * ********************************************************************
 *
 * code for processing module configuration commands
 *
 * Modification History
 *
 * Created 09/23/98 by Dan Espen:
 *
 * - Some  of  this Logic   used to reside  in  "read.c",  preceeded by a
 * comment about whether it belonged there.  Time tells.
 *
 * - Added  logic to  execute module config  commands by  passing to  the
 * modules that want "active command pipes".
 *
 * ********************************************************************
 */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

unsigned long *PipeMask;                /* in module.c */

extern Boolean debugging;

struct moduleInfoList
{
  char *data;
  struct moduleInfoList *next;
};

struct moduleInfoList *modlistroot = NULL;

void AddToModList(char *tline);         /* prototypes */
extern void StartupStuff(void);


/*
 * ModuleConfig handles commands starting with "*".
 *
 * Each command is added to a list from which modules  request playback
 * thru "sendconfig".
 *
 * Some modules request that module config commands be sent to them
 * as the commands are entered.  Send to modules that want it.
 */
void  ModuleConfig(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context,
                   char *action, int *Module) {
  int module;
  AddToModList(action);                 /* save for config request */
  for (module=0;module<npipes;module++) {/* look at all possible pipes */
    if (PipeMask[module] & M_SENDCONFIG) { /* does module want config cmds */
      SendName(module,M_CONFIG_INFO,0,0,0,action); /* send cmd to module */
    }
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
/* dje, this doesn't seem to be used? */
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
  struct moduleInfoList *current, *next, *prev;
  char *info;   /* info to be deleted - may contain wildcards */
  char *mi;

  GetNextToken(action, &info);
  if( info == NULL )
  {
    return;
  }

  current = modlistroot;
  prev = NULL;

  while(current != NULL)
  {
    GetNextToken( current->data, &mi);
    next = current->next;
    if( matchWildcards(info, mi+1) )
    {
      free(current->data);
      free(current);
      if( prev )
      {
        prev->next = next;
      }
      else
      {
        modlistroot = next;
      }
    }
    else
    {
      prev = current;
    }
    current = next;
    if (mi)
      free(mi);
  }
  free(info);
}

void SendDataToModule(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	      unsigned long context, char *action, int *Module)
{
  struct moduleInfoList *t;
  char *message,msg2[32];
  extern char *IconPath;
  extern char *PixmapPath;

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
  /* Dominik Vogt (8-Nov-1998): Scr.ClickTime patch to set ClickTime to
   * 'not at all' during InitFunction and RestartFunction. */
  sprintf(msg2,"ClickTime %d\n", (Scr.ClickTime < 0) ?
	  -Scr.ClickTime : Scr.ClickTime);
  SendName(*Module,M_CONFIG_INFO,0,0,0,msg2);

  t = modlistroot;
  while(t != NULL)
  {
    SendName(*Module,M_CONFIG_INFO,0,0,0,t->data);
    t = t->next;
  }
  SendPacket(*Module,M_END_CONFIG_INFO,0,0,0,0,0,0,0,0);
}
