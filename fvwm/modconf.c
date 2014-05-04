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

/*
 *
 * code for processing module configuration commands
 *
 * Modification History
 *
 * Created 09/23/98 by Dan Espen:
 *
 * - Some of this logic used to reside in "read.c", preceeded by a
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
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/wild.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "module_list.h"
#include "module_interface.h"
#include "colorset.h"
#include "libs/FScreen.h"

extern int nColorsets;  /* in libs/Colorset.c */

/* do not send ColorLimit, it is not used anymore but maybe by non
 * "official" modules */
#define DISABLE_COLORLIMIT_CONFIG_INFO

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
	fmodule *module, const struct moduleInfoList *entry, char *match,
	int match_len);


/*
 * ModuleConfig handles commands starting with "*".
 *
 * Each command is added to a list from which modules  request playback
 * thru "sendconfig".
 *
 * Some modules request that module config commands be sent to them
 * as the commands are entered.  Send to modules that want it.
 */
void ModuleConfig(char *action)
{
	int end;
	fmodule_list_itr moditr;
	fmodule *module;
	struct moduleInfoList *new_entry;

	end = strlen(action) - 1;
	if (action[end] == '\n')
		action[end] = '\0';
	/* save for config request */
	new_entry = AddToModList(action);
	/* look at all possible pipes */
	module_list_itr_init(&moditr);
	while ( (module = module_list_itr_next(&moditr)) != NULL)
	{
		if (IS_MESSAGE_SELECTED(module, M_SENDCONFIG))
		{
			/* module wants config cmds */
			char *name = MOD_NAME(module);
			if (MOD_ALIAS(module))
			{
				name = MOD_ALIAS(module);
			}
			SendConfigToModule(
				module, new_entry, CatString2("*", name), 0);
		}
	}

	return;
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

	this = xmalloc(sizeof(struct moduleInfoList));

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

	this->data = xmalloc(strlen(rline)+1);
	strcpy(this->data, rline);

	this->next = NULL;
	if(prev == NULL)
	{
		modlistroot = this;
	}
	else
	{
		prev->next = this;
	}

	return this;
}

/*
 * delete from module configuration
 */
void CMD_DestroyModuleConfig(F_CMD_ARGS)
{
	struct moduleInfoList *current, *next, *prev;
	char *info;   /* info to be deleted - may contain wildcards */
	char *mi;
	char *alias_end;
	int alias_len = 0;

	while (isspace(*action))
	{
		action++;
	}
	alias_end = skipModuleAliasToken(action);

	if (alias_end && alias_end[0] == MODULE_CONFIG_DELIM)
	{
		/* migo: construct an old-style config line */
		char *conf_start = alias_end + 1;
		while (isspace(*conf_start)) conf_start++;
		*alias_end = '\0';
		GetNextToken(conf_start, &conf_start);
		if (conf_start == NULL)
		{
			return;
		}
		info = stripcpy(CatString2(action, conf_start));
		*alias_end = MODULE_CONFIG_DELIM;
		/* +1 for a leading '*' */
		alias_len = alias_end - action + 1;
		free(conf_start);
	}
	else
	{
		GetNextToken(action, &info);
		if (info == NULL)
		{
			return;
		}
	}

	current = modlistroot;
	prev = NULL;

	while (current != NULL)
	{
		GetNextToken(current->data, &mi);
		next = current->next;
		if ((!alias_len || !current->alias_len ||
		     alias_len == current->alias_len) &&
		    matchWildcards(info, mi+1))
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
		{
			free(mi);
		}
	}
	free(info);

	return;
}

static void send_xinerama_state(fmodule *module)
{
	SendName(module, M_CONFIG_INFO, 0, 0, 0, FScreenGetConfiguration());

	return;
}

static void send_desktop_names(fmodule *module)
{
	DesktopsInfo *d;
	char *name;

	for (d = Scr.Desktops->next; d != NULL; d = d->next)
	{
		if (d->name != NULL)
		{
			/* TA:  FIXME!  xasprintf() */
			name = xmalloc(strlen(d->name) + 44);
			sprintf(name,"DesktopName %d %s", d->desk, d->name);
			SendName(module, M_CONFIG_INFO, 0, 0, 0, name);
			free(name);
		}
	}

	return;
}

static void send_desktop_geometry(fmodule *module)
{
	char msg[64];

	sprintf(msg, "DesktopSize %d %d\n", Scr.VxMax / Scr.MyDisplayWidth + 1,
		Scr.VyMax / Scr.MyDisplayHeight + 1);
	SendName(module, M_CONFIG_INFO, 0, 0, 0, msg);

	return;
}

static void send_image_path(fmodule *module)
{
	char *msg;
	char *ImagePath = PictureGetImagePath();

	if (ImagePath && *ImagePath != 0)
	{
		/* TA:  FIXME!  xasprintf() */
		msg = xmalloc(strlen(ImagePath) + 12);
		sprintf(msg, "ImagePath %s\n", ImagePath);
		SendName(module, M_CONFIG_INFO, 0, 0, 0, msg);
		free(msg);
	}

	return;
}

static void send_color_limit(fmodule *module)
{
#ifndef DISABLE_COLORLIMIT_CONFIG_INFO
	char msg[64];

	sprintf(msg, "ColorLimit %d\n", Scr.ColorLimit);
	SendName(module, M_CONFIG_INFO, 0, 0, 0, msg);
#endif
	return;
}

static void send_colorsets(fmodule *module)
{
	int n;

	/* dump the colorsets (0 first as others copy it) */
	for (n = 0; n < nColorsets; n++)
	{
		SendName(
			module, M_CONFIG_INFO, 0, 0, 0,
			DumpColorset(n, &Colorset[n]));
	}

	return;
}

static void send_click_time(fmodule *module)
{
	char msg[64];

	/* Dominik Vogt (8-Nov-1998): Scr.ClickTime patch to set ClickTime to
	 * 'not at all' during InitFunction and RestartFunction. */
	sprintf(msg,"ClickTime %d\n", (Scr.ClickTime < 0) ?
		-Scr.ClickTime : Scr.ClickTime);
	SendName(module, M_CONFIG_INFO, 0, 0, 0, msg);

	return;
}

static void send_move_threshold(fmodule *module)
{
	char msg[64];

	sprintf(msg, "MoveThreshold %d\n", Scr.MoveThreshold);
	SendName(module, M_CONFIG_INFO, 0, 0, 0, msg);

	return;
}

void send_ignore_modifiers(fmodule *module)
{
	char msg[64];

	sprintf(msg, "IgnoreModifiers %d\n", GetUnusedModifiers());
	SendName(module, M_CONFIG_INFO, 0, 0, 0, msg);

	return;
}

void CMD_Send_ConfigInfo(F_CMD_ARGS)
{
	struct moduleInfoList *t;
	/* matching criteria for module cmds */
	char *match;
	/* get length once for efficiency */
	int match_len = 0;
	fmodule *mod = exc->m.module;

	send_desktop_geometry(mod);
	/* send ImagePath and ColorLimit first */
	send_image_path(mod);
	send_color_limit(mod);
	send_xinerama_state(mod);
	send_colorsets(mod);
	send_click_time(mod);
	send_move_threshold(mod);
	match = PeekToken(action, &action);
	if (match)
	{
		match_len = strlen(match);
	}
	for (t = modlistroot; t != NULL; t = t->next)
	{
		SendConfigToModule(mod, t, match, match_len);
	}
	send_desktop_names(mod);
	send_ignore_modifiers(mod);
	SendPacket(
		mod, M_END_CONFIG_INFO, (long)0, (long)0, (long)0, (long)0,
		(long)0, (long)0, (long)0, (long)0);

	return;
}

static void SendConfigToModule(
	fmodule *module, const struct moduleInfoList *entry, char *match,
	int match_len)
{
	if (match)
	{
		if (match_len == 0)
		{
			match_len = strlen(match);
		}
		if (entry->alias_len > 0 && entry->alias_len != match_len)
		{
			return;
		}
		/* migo: this should be strncmp not strncasecmp probably. */
		if (strncasecmp(entry->data, match, match_len) != 0)
		{
			return;
		}
	}
	SendName(module, M_CONFIG_INFO, 0, 0, 0, entry->data);

	return;
}
