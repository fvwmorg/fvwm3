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

/* ---------------------------- included header files ---------------------- */
#include "config.h"

#include <stdio.h>

#include "safemalloc.h"
#include "Strings.h"
#include "Parse.h"
#include "envvar.h"
#include "flist.h"

#include "FGettext.h"
#include "locale.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

#define FGP_DOMAIN(l) ((FGettextPath *)l->object)->domain
#define FGP_DIR(l)    ((FGettextPath *)l->object)->dir

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef struct
{
	char *domain;
	char *dir;
} FGettextPath;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static int FGettextInitOk = 0;
static char *FGDefaultDir = NULL;
static char *FGDefaultDomain = NULL;
static const char *FGModuleName = NULL;

static flist *FGPathList = NULL;
static FGettextPath *FGLastPath = NULL;

/* ---------------------------- interface functions ------------------------ */

static
void fgettext_add_one_path(char *path, int position)
{
	char *dir,*domain;
	FGettextPath *tmp;

	if (!HaveNLSSupport)
	{
		return;
	}

	domain = GetQuotedString(path, &dir, ";", NULL, NULL, NULL);
	if (!dir || dir[0] == '\0' || dir[0] != '/')
	{
		if (dir)
		{
			free(dir);
		}
		CopyString(&dir, FGDefaultDir);
	}
	if (!domain || domain[0] == '\0')
	{
		domain = FGDefaultDomain;
	}

	tmp = xmalloc(sizeof(FGettextPath));
	tmp->dir = dir;
	CopyString(&tmp->domain, domain);

	FGPathList = flist_insert_obj(FGPathList, tmp, position);
}

static void fgettext_free_fgpath_list(void)
{
	flist *l = FGPathList;

	if (!HaveNLSSupport)
	{
		return;
	}

	while(l != NULL)
	{
		if (l->object)
		{
			if (FGP_DOMAIN(l))
			{
				free(FGP_DOMAIN(l));
			}
			if (FGP_DIR(l))
			{
				free(FGP_DIR(l));
			}
			free(l->object);
		}
		l = l->next;
	}
	FGLastPath = NULL;
	FGPathList = flist_free_list(FGPathList);
}

/* ---------------------------- interface functions ------------------------ */

void FGettextInit(const char *domain, const char *dir, const char *module)
{
	const char *btd;
	const char *td;

	if (!HaveNLSSupport)
	{
		return;
	}
	setlocale (LC_MESSAGES, "");

	btd = bindtextdomain (domain, dir);
	td = textdomain (domain);
	if (!td || !btd)
	{
		fprintf(
			stderr,"[%s][FGettextInit]: <<ERROR>> "
			"gettext initialisation fail!\n",
			module);
		return;
	}
	FGModuleName = module;
	CopyString(&FGDefaultDir, btd);
	CopyString(&FGDefaultDomain, td);
	FGLastPath = xmalloc(sizeof(FGettextPath));
	CopyString(&FGLastPath->domain, td);
	CopyString(&FGLastPath->dir, btd);
	FGPathList = flist_append_obj(FGPathList, FGLastPath);
	FGettextInitOk = 1;
}

const char *FGettext(char *str)
{
	flist *l = FGPathList;
	const char *s;

	if (!HaveNLSSupport || !FGettextInitOk || FGPathList == NULL ||
	    str == NULL)
	{
		return str;
	}

	if (FGLastPath != l->object)
	{
		(void)bindtextdomain (FGP_DOMAIN(l), FGP_DIR(l));
		(void)textdomain (FGP_DOMAIN(l));
		FGLastPath = l->object;
	}
	s = gettext(str);
	if (s != str)
	{
		return s;
	}
	l = l->next;
	while(l != NULL)
	{
		(void)bindtextdomain (FGP_DOMAIN(l), FGP_DIR(l));
		(void)textdomain (FGP_DOMAIN(l));
		FGLastPath = l->object;
		s = gettext(str);
		if (s != str)
		{
			return s;
		}
		l = l->next;
	}
	return str;
}

char *FGettextCopy(char *str)
{
	const char *trans;
	char *r = NULL;

	trans = FGettext(str);
	if (trans != NULL)
	{
		CopyString(&r, trans);
	}
	return r;
}

void FGettextSetLocalePath(const char *path)
{
	char *exp_path = NULL;
	char *before = NULL;
	char *after, *p, *str;
	int count;

	if (!HaveNLSSupport || !FGettextInitOk)
	{
		return;
	}

	FGLastPath = NULL;

	if (path == NULL || path[0] == '\0')
	{
		fgettext_free_fgpath_list();
		FGLastPath = xmalloc(sizeof(FGettextPath));
		CopyString(&FGLastPath->domain, FGDefaultDomain);
		CopyString(&FGLastPath->dir, FGDefaultDir);
		FGPathList = flist_append_obj(FGPathList, FGLastPath);
		FGLastPath = NULL;
		return;
	}

	exp_path = envDupExpand(path, 0);

	if (StrEquals(exp_path,"None"))
	{
		fgettext_free_fgpath_list();
		goto bail;
	}

	after = GetQuotedString(exp_path, &before, "+", NULL, NULL, NULL);
	if ((after && strchr(after, '+')) || (before && strchr(before, '+')))
	{
		fprintf(
			stderr,"[%s][SetLocalePath]: "
			"To many '+' in locale path specification: %s\n",
			FGModuleName, path);
		goto bail;
	}
	if (!strchr(exp_path, '+'))
	{
	    fgettext_free_fgpath_list();
	}
	while(after && *after)
	{
		after = GetQuotedString(after, &p, ":", NULL, NULL, NULL);
		if (p && *p)
		{
			fgettext_add_one_path(p,-1);
		}
		if (p)
		{
			free(p);
		}
	}
	count = 0;
	str = before;
	while (str && *str)
	{
		str = GetQuotedString(str, &p, ":", NULL, NULL, NULL);
		if (p && *p)
		{
			fgettext_add_one_path(p,count);
			count++;
		}
		if (p)
		{
			free(p);
		}
	}
 bail:
	if (before)
	{
		free(before);
	}
	if (exp_path)
	{
		free(exp_path);
	}
}


void FGettextPrintLocalePath(int verbose)
{
	flist *l = FGPathList;

	if (!HaveNLSSupport || !FGettextInitOk)
	{
		return;
	}

	fprintf(stderr,"fvwm NLS gettext path:\n");
	while(l != NULL)
	{
		fprintf(
			stderr,"  dir: %s, domain: %s\n",
			FGP_DIR(l), FGP_DOMAIN(l));
		l = l->next;
	}
}
