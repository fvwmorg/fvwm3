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

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "libs/Colorset.h"

extern unsigned long *PipeMask;                /* in module.c */

extern int nColorsets;	/* in libs/Colorset.c */

#define MODULE_CONFIG_DELIM ':'

struct moduleInfoList
{
  char *data;
  unsigned char alias_len;
  struct moduleInfoList *next;
};

struct moduleInfoList *modlistroot = NULL;

static struct moduleInfoList *AddToModList(char *tline);
static void SendConfigToModule(
  int module, const struct moduleInfoList *entry, char *match, int match_len);


/*
 * ModuleConfig handles commands starting with "*".
 *
 * Each command is added to a list from which modules  request playback
 * thru "sendconfig".
 *
 * Some modules request that module config commands be sent to them
 * as the commands are entered.  Send to modules that want it.
 */
void  ModuleConfig(char *action) {
  int module, end;
  struct moduleInfoList *new_entry;

  end = strlen(action) - 1;
  if (action[end] == '\n')
    action[end] = '\0';
  new_entry = AddToModList(action);              /* save for config request */
  for (module = 0; module < npipes; module++) /* look at all possible pipes */
  {
    if (PipeMask[module] & M_SENDCONFIG)    /* does module want config cmds */
    {
      extern char **pipeName;
      char *name = pipeName[module];
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
      extern char **pipeAlias;
      if (pipeAlias[module]) name = pipeAlias[module];
#endif
      SendConfigToModule(module, new_entry, CatString2("*", name), 0);
    }
  }
}

static struct moduleInfoList *AddToModList(char *tline)
{
  struct moduleInfoList *t, *prev, *this;
  char *rline = tline;
  char *alias_end = skipModuleAliasToken(tline + 1);

  /* Find end of list */
  t = modlistroot;
  prev = NULL;

  while(t != NULL)
  {
    prev = t;
    t = t->next;
  }

  this = (struct moduleInfoList *)safemalloc(sizeof(struct moduleInfoList));

  this->alias_len = 0;
  if (alias_end && alias_end[0] == MODULE_CONFIG_DELIM)
  {
    /* migo (01-Sep-2000): construct an old-style config line */
    char *conf_start = alias_end + 1;
    while (isspace(*conf_start)) conf_start++;
    *alias_end = '\0';
    rline = CatString2(tline, conf_start);
    *alias_end = MODULE_CONFIG_DELIM;
    this->alias_len = alias_end - tline;
  }

  this->data = (char *)safemalloc(strlen(rline)+1);
  strcpy(this->data, rline);

  this->next = NULL;
  if(prev == NULL)
  {
    modlistroot = this;
  }
  else
    prev->next = this;

  return this;
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
  char *alias_end;
  int alias_len = 0;

  while (isspace(*action)) action++;
  alias_end = skipModuleAliasToken(action);

  if (alias_end && alias_end[0] == MODULE_CONFIG_DELIM)
  {
    /* migo: construct an old-style config line */
    char *conf_start = alias_end + 1;
    while (isspace(*conf_start)) conf_start++;
    *alias_end = '\0';
    GetNextToken(conf_start, &conf_start);
    if (conf_start == NULL)
      return;
    info = stripcpy(CatString2(action, conf_start));
    *alias_end = MODULE_CONFIG_DELIM;
    alias_len = alias_end - action + 1;  /* +1 for a leading '*' */
    free(conf_start);
  }
  else
  {
    GetNextToken(action, &info);
    if (info == NULL)
      return;
  }

  current = modlistroot;
  prev = NULL;

  while (current != NULL)
  {
    GetNextToken(current->data, &mi);
    next = current->next;
    if ((!alias_len || !current->alias_len || alias_len == current->alias_len)
        && matchWildcards(info, mi+1))
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
  int n;
  char *ImagePath = GetImagePath();

  GetNextToken(action, &match);
  if (match) {
    match_len = strlen(match);
  }

  /* send desktop geometry */
  sprintf(msg2,"DesktopSize %d %d\n", Scr.VxMax / Scr.MyDisplayWidth + 1,
	  Scr.VyMax / Scr.MyDisplayHeight + 1);
  SendName(*Module,M_CONFIG_INFO,0,0,0,msg2);

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
  /* now dump the colorsets (0 first as others copy it) */
  for (n = 0; n < nColorsets; n++)
    SendName(*Module, M_CONFIG_INFO, 0, 0, 0, DumpColorset(n, &Colorset[n]));

  /* Dominik Vogt (8-Nov-1998): Scr.ClickTime patch to set ClickTime to
   * 'not at all' during InitFunction and RestartFunction. */
  sprintf(msg2,"ClickTime %d\n", (Scr.ClickTime < 0) ?
	  -Scr.ClickTime : Scr.ClickTime);
  SendName(*Module,M_CONFIG_INFO,0,0,0,msg2);
  sprintf(msg2,"MoveThreshold %d\n", Scr.MoveThreshold);
  SendName(*Module,M_CONFIG_INFO,0,0,0,msg2);

  for (t = modlistroot; t != NULL; t = t->next)
  {
    SendConfigToModule(*Module, t, match, match_len);
  }
  SendPacket(*Module,M_END_CONFIG_INFO,0,0,0,0,0,0,0,0);
  free(match);
}

static void SendConfigToModule(
  int module, const struct moduleInfoList *entry, char *match, int match_len)
{
  if (match)
  {
    if (match_len == 0)
      match_len = strlen(match);
    if (entry->alias_len > 0 && entry->alias_len != match_len)
      return;
    /* migo: this should be strncmp not strncasecmp probably. */
    if (strncasecmp(entry->data, match, match_len) != 0)
      return;
  }
  SendName(module, M_CONFIG_INFO, 0, 0, 0, entry->data);
}
