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
#include <signal.h>
#include <ctype.h>
#include <sys/socket.h>

#include "fvwm.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "libs/Picture.h"
#include "libs/Colorset.h"

extern unsigned long *PipeMask;                /* in module.c */

extern Boolean debugging;

struct moduleInfoList
{
  char *data;
  struct moduleInfoList *next;
};

struct moduleInfoList *modlistroot = NULL;

void AddToModList(char *tline);         /* prototypes */


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
  char *message, msg2[32];
  char *match;                          /* matching criteria for module cmds */
  int match_len = 0;                    /* get length once for efficiency */
  int n = nColorsets;
  char *ImagePath = GetImagePath();
  char *buf;

  GetNextToken(action, &match);
  if (match) {
    match_len = strlen(match);
  }

  /* send ImagePath and ColorLimit first */
  if (ImagePath && strlen(ImagePath))
  {
    message=safemalloc(strlen(ImagePath)+12);
    sprintf(message,"ImagePath %s\n",ImagePath);
    SendName(*Module,M_CONFIG_INFO,0,0,0,message);
    free(message);
  }
#ifdef XPM
  {
    sprintf(msg2,"ColorLimit %d\n",Scr.ColorLimit);
    SendName(*Module,M_CONFIG_INFO,0,0,0,msg2);
  }
#endif
  /* now dump the colorsets (in reverse order to minimize mallocing lots */
  buf = DumpColorset(n);
  while (n--)
    SendName(*Module, M_CONFIG_INFO, 0, 0, 0, buf);

  /* Dominik Vogt (8-Nov-1998): Scr.ClickTime patch to set ClickTime to
   * 'not at all' during InitFunction and RestartFunction. */
  sprintf(msg2,"ClickTime %d\n", (Scr.ClickTime < 0) ?
	  -Scr.ClickTime : Scr.ClickTime);
  SendName(*Module,M_CONFIG_INFO,0,0,0,msg2);
  sprintf(msg2,"MoveThreshold %d\n", Scr.MoveThreshold);
  SendName(*Module,M_CONFIG_INFO,0,0,0,msg2);

  for (t = modlistroot; t != NULL; t = t->next) {
    if (match && strncasecmp(t->data,match,match_len)) {
      continue;
    }
    SendName(*Module,M_CONFIG_INFO,0,0,0,t->data);
  }
  SendPacket(*Module,M_END_CONFIG_INFO,0,0,0,0,0,0,0,0);
  free(match);
}
