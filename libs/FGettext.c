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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ---------------------------- included header files ----------------------- */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "safemalloc.h"
#include "Strings.h"
#include "Parse.h"
#include "envvar.h"

#include "FGettext.h"
#include "locale.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

typedef struct _FGettextPath
{
	struct _FGettextPath *next;
	char *domain;
	char *dir;
} FGettextPath;

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static int FGettextInitOk = 0;
static char *FGDefaultDir = NULL;
static char *FGDefaultDomain = NULL;
static const char *FGModuleName = NULL;

static FGettextPath *FGPath = NULL;
static FGettextPath *FGLastPath = NULL;

/* ---------------------------- interface functions ------------------------- */

static
void fgettext_add_one_path(char *path, int position)
{
	char *dir,*domain;
	FGettextPath *fgpath, *tmp, *prevpath;
	int count = 1;

	if (!HaveNLSSupport)
	{
		return;
	}

	domain = GetQuotedString(path, &dir, ";", NULL, NULL, NULL);
	if (!dir || dir[0] != '/')
	{
		CopyString(&dir, FGDefaultDir);
	}
	if (!domain || domain[0] == '\0')
	{
		domain = FGDefaultDomain;
	}

	tmp = (FGettextPath *)safemalloc(sizeof(FGettextPath));
	tmp->dir = dir;
	tmp->next = NULL;
	CopyString(&tmp->domain, domain);

	if (FGPath == NULL)
	{
		FGPath = tmp;
		return;
	}
	fgpath = FGPath;
	prevpath = FGPath;
	while(fgpath->next != NULL && count != position)
	{
		count++;
		prevpath = fgpath;
		fgpath = fgpath->next;
	}
	if (fgpath->next == NULL)
	{
		/* end */
		fgpath->next = tmp;
	}
	else
	{
		tmp->next = fgpath;
		if (fgpath == FGPath)
		{
			/* first */
			FGPath = tmp;
		}
		else
		{
			/* middle */
			prevpath->next = tmp;
		}
	}
}

static
void fgettext_free_path(void)
{
	FGettextPath *fgpath = FGPath;
	FGettextPath *tmp;

	if (!HaveNLSSupport)
	{
		return;
	}

	while(fgpath != NULL)
	{
		if (fgpath->domain)
		{
			free(fgpath->domain);
		}
		if (fgpath->dir)
		{
			free(fgpath->dir);
		}
		tmp = fgpath;
		fgpath = fgpath->next;
		free(tmp);
	}
	FGPath = NULL;
}

/* ---------------------------- interface functions ------------------------- */

void FGettextInit(const char *domain, const char *dir, const char *module)
{
	if (!HaveNLSSupport)
	{
		return;
	}
	setlocale (LC_MESSAGES, "");

	FGDefaultDir = bindtextdomain (domain, dir);
	FGDefaultDomain = textdomain (domain);
	FGModuleName = module;
	FGPath = (FGettextPath *)safemalloc(sizeof(FGettextPath));
	CopyString(&FGPath->domain, domain);
	CopyString(&FGPath->dir, dir);
	FGPath->next = NULL;
	FGLastPath = FGPath;
	FGettextInitOk = 1;
}

const char *FGettext(char *str)
{
	FGettextPath *fgpath = FGPath;
	const char *s, *dummy;

	if (!HaveNLSSupport || !FGettextInitOk || FGPath == NULL || str == NULL)
	{
		return str;
	}

	if (FGPath != FGLastPath)
	{
		dummy = bindtextdomain (FGPath->domain, FGPath->dir);
		dummy = textdomain (FGPath->domain);
		FGLastPath = FGPath;
	}
	s = gettext(str);
	if (s != str)
	{
		return s;
	}
	fgpath = fgpath->next;
	while(fgpath != NULL)
	{
		dummy = bindtextdomain (fgpath->domain, fgpath->dir);
		dummy = textdomain (fgpath->domain);
		FGLastPath = fgpath;
		s = gettext(str);
		if (s != str)
		{
			return s;
		}
		fgpath = fgpath->next;
	}
	return str;
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
		fgettext_free_path();
		FGPath = (FGettextPath *)safemalloc(sizeof(FGettextPath));
		CopyString(&FGPath->domain, FGDefaultDomain);
		CopyString(&FGPath->dir, FGDefaultDir);
		FGPath->next = NULL;
		FGLastPath = NULL;
		return;
	}

	exp_path = envDupExpand(path, 0);

	if (StrEquals(exp_path,"None"))
	{
		fgettext_free_path();
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
	    fgettext_free_path();
	}
	while(after && *after)
	{
		after = GetQuotedString(after, &p, ":", NULL, NULL, NULL);
		if (p && *p)
		{
			fgettext_add_one_path(p,0);
		}
		if (p)
		{
			free(p);
		}
	}
	count = 1;
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
	FGettextPath *fgpath = FGPath;

	if (!HaveNLSSupport || !FGettextInitOk)
	{
		return;
	}

	fprintf(stderr,"FVWM NLS gettext path:\n");
	while(fgpath != NULL)
	{
		fprintf(
			stderr,"  dir: %s, domain: %s\n",
			fgpath->dir, fgpath->domain);
		fgpath = fgpath->next;
	}
}
