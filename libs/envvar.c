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
 *  MODULE OF       fvwm
 *
 *  DESCRIPTION     Routines to expand environment-variables into strings.
 *                  Will understand both $ENV and ${ENV} -type variables.
 *
 *  WRITTEN BY      Sverre H. Huseby
 *                  sverrehu@ifi.uio.no
 *
 *  CREATED         1995/10/3
 *
 *  UPDATED         migo - 21/Jun/1999 - added getFirstEnv, some changes
 *
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"
#include <stdio.h>
#include <ctype.h>

#include "fvwmlib.h"
#include "envvar.h"

/* ---------------------------- local definitions -------------------------- */

#ifdef HAVE_UNSETENV
#define FHaveUnsetenv 1
#else
#define unsetenv(x) do { } while (0)
#define FHaveUnsetenv 0
#endif

/* ---------------------------- local macros ------------------------------- */

#define ENV_LIST_INC 10
#ifndef NULL
#define NULL 0
#endif

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef struct
{
	char *var;
	char *env;
} env_list_item;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

/*-------------------------------------------------------------------------
 *
 *  NAME          strDel
 *
 *  FUNCTION      Delete characters from a string.
 *
 *  INPUT         s       the string to delete characters from.
 *                idx     index of first character to delete.
 *                n       number of characters to delete.
 *
 *  OUTPUT        s       string with characters deleted.
 *
 *  DESCRIPTION   Deletes characters from a string by moving following
 *                characters back.
 *
 */
static void strDel(char *s, int idx, int n)
{
	int  l;
	char *p;

	if (idx >= (l = strlen(s)))
		return;
	if (idx + n > l)
		n = l - idx;
	s += idx;
	p = s + n;
	do {
		*s++ = *p;
	} while (*p++);
}

/*-------------------------------------------------------------------------
 *
 *  NAME          strIns
 *
 *  FUNCTION      Insert a string into a string.
 *
 *  INPUT         s       the string to insert into.
 *                ins     the string to insert.
 *                idx     index of where to insert the string.
 *                maxstrlen     max length of s, including '\0'.
 *
 *  OUTPUT        s       string with characters inserted.
 *
 *  DESCRIPTION   The insertion will be done even if the string gets to
 *                long, but characters will be sacrificed at the end of s.
 *                The string is always '\0'-terminated.
 *
 */
static void strIns(char *s, const char *ins, int idx, int maxstrlen)
{
	int  l, li, move;
	char *p1, *p2;

	if (idx > (l = strlen(s)))
	{
		idx = l;
	}
	li = strlen(ins);
	move = l - idx + 1; /* include '\0' in move */
	p1 = s + l;
	p2 = p1 + li;
	while (p2 >= s + maxstrlen) {
		--p1;
		--p2;
		--move;
	}
	while (move-- > 0)
		*p2-- = *p1--;
	p1 = s + idx;
	if (idx + li >= maxstrlen)
	{
		li = maxstrlen - idx - 1;
	}
	while (li-- > 0)
		*p1++ = *ins++;
	s[maxstrlen - 1] = '\0';
}

/*-------------------------------------------------------------------------
 *
 *  NAME          findEnvVar
 *
 *  FUNCTION      Find first environment variable in a string.
 *
 *  INPUT         s       the string to scan.
 *
 *  OUTPUT        len     length of variable, including $ and { }.
 *
 *  RETURNS       Pointer to the $ that introduces the variable, or NULL
 *                if no variable is found.
 *
 *  DESCRIPTION   Searches for matches like $NAME and ${NAME}, where NAME is
 *                a sequence of characters, digits and underscores, of which
 *                the first can not be a digit.
 *
 *  NOTE          This function will only return `legal' variables. There
 *                may be $'s in the string that are not followed by what
 *                is considered a legal variable name introducer. Such
 *                occurrences are skipped.
 *
 */
static char *findEnvVar(const char *s, int *len)
{
	int   brace = 0;
	char  *ret = NULL;
	const char *next;

	if (!s)
		return NULL;
	while (*s) {
		next = s + 1;
		if (*s == '$' &&
		    (isalpha(*next) || *next == '_' || *next == '{')) {
			ret = (char *) s++;
			if (*s == '{') {
				brace = 1;
				++s;
			}
			while (*s && (isalnum(*s) || *s == '_'))
				++s;
			*len = s - ret;
			if (brace) {
				if (*s == '}') {
					++*len;
					break;
				}
				ret = NULL;
			} else
				break;
		}
		++s;
	}
	return ret;
}

/*-------------------------------------------------------------------------
 *
 *  FUNCTION      Look up environment variable.
 *
 *  INPUT         name    name of environment variable to look up. This
 *                        may include $ and { }.
 *                len     length for environment variable name (0 - ignore).
 *
 *  RETURNS       The variable contents, or "" if not found.
 *
 */
static const char *getEnv(const char *name, int len)
{
	static char *empty = "";
	char   *ret = NULL, *tmp, *p, *p2;

	if ((tmp = xstrdup(name)) == NULL)
		return empty;  /* better than no test at all. */
	p = tmp;
	if (*p == '$')
		++p;
	if (*p == '{') {
		++p;
		if ((p2 = strchr(p, '}')) != NULL)
			*p2 = '\0';
	}
	if (len > 0 && len < strlen(tmp)) tmp[len] = '\0';
	if ((ret = getenv(p)) == NULL)
		ret = empty;
	free(tmp);
	return ret;
}

/* ---------------------------- interface functions ------------------------ */

/*
 *  FUNCTION      Expand environment variables in a string.
 *
 *  SYNOPSIS      #include "envvar.h"
 *                int envExpand(char *s, int maxstrlen);
 *
 *  INPUT         s       string to expand environment variables in.
 *                maxstrlen     max length of string, including '\0'.
 *
 *  OUTPUT        s       the string with environment variables expanded.
 *
 *  RETURNS       Number of changes done.
 *
 *  NOTES         A non-existing variable is substituted with the empty
 *                string.
 *
 */
int envExpand(char *s, int maxstrlen)
{
	char  *var, *s2;
	const char *env;
	int   len, ret = 0;

	s2 = s;
	while ((var = findEnvVar(s2, &len)) != NULL) {
		++ret;
		env = getEnv(var, len);
		strDel(s, var - s, len);
		strIns(s, env, var - s, maxstrlen);
		s2 = var + strlen(env);
	}
	return ret;
}

/*
 *  FUNCTION      Expand environment variables into a new string.
 *
 *  SYNOPSIS      #include "envvar.h"
 *                char *envDupExpand(const char *s, int extra);
 *
 *  INPUT         s       string to expand environment variables in.
 *                extra   number of extra bytes to allocate in the
 *                        string, in addition to the string contents
 *                        and the terminating '\0'.
 *
 *  RETURNS       A dynamically allocated string with environment
 *                variables expanded.
 *                Use free() to deallocate the buffer when it is no
 *                longer needed.
 *                NULL is returned if there is not enough memory.
 *
 *  NOTES         A non-existing variable is substituted with the empty
 *                string.
 *
 */
char *envDupExpand(const char *s, int extra)
{
	char  *var, *ret;
	const char *env, *s2;
	int   len, slen, elen, bufflen;

	/*
	 *  calculate length needed.
	 */
	s2 = s;
	slen = strlen(s);
	bufflen = slen + 1 + extra;
	while ((var = findEnvVar(s2, &len)) != NULL) {
		env = getEnv(var, len);
		elen = strlen(env);
		/* need to make a buffer the maximum possible size, else we
		 * may get trouble while expanding. */
		bufflen += len > elen ? len : elen;
		s2 = var + len;
	}
	if (bufflen < slen + 1)
		bufflen = slen + 1;

	ret = xmalloc(bufflen);
	/* TA:  FIXME!  xasprintf() */

	/*
	 *  now do the real expansion.
	 */
	strcpy(ret, s);
	envExpand(ret, bufflen - extra);

	return ret;
}

/*
 *  FUNCTION      Search for the first environment variable and return
 *                its contents and coordinates in the given string.
 *
 *  INPUT         s       the string to scan.
 *                        may include $ and { } that introduce variable.
 *
 *  OUTPUT        beg     index in the string of matching $.
 *                end     index in the string, first after matching var.
 *
 *  RETURNS       The variable contents; "" if env variable has legal name,
 *                but does not exist; or NULL if no env variables found.
 *                Returned constant string must not be deallocated.
 *
 *  NOTE          This function will only return `legal' variables. There
 *                may be $'s in the string that are not followed by what
 *                is considered a legal variable name introducer. Such
 *                occurrences are skipped.
 *                If nothing is found returns NULL and sets beg and end to 0.
 *
 *  EXAMPLE       getFirstEnv("echo $HOME/.fvwm/config", &beg, &end)
 *                returns "/home/username" and beg=5, end=10.
 *
 */
const char* getFirstEnv(const char *s, int *beg, int *end)
{
	char *var;
	const char *env;
	int len;

	*beg = *end = 0;
	if ((var = findEnvVar(s, &len)) == NULL) return NULL;
	env = getEnv(var, len);

	*beg = var - s;
	*end = *beg + len;

	return env;
}

/* If env is NULL, var is removed from the environment list */
static void add_to_envlist(char *var, char *env)
{
	static env_list_item *env_list = NULL;
	static unsigned int env_len = 0;
	static unsigned int env_len_allocated = 0;
	unsigned int i;

	/* find string in list */
	if (env_list && env_len)
	{
		for (i = 0; i < env_len; i++)
		{
			if (strcmp(var, env_list[i].var) != 0)
			{
				continue;
			}
			/* found it - replace old string */
			free(env_list[i].var);
			free(env_list[i].env);
			if (env == NULL)
			{
				/* delete */
				env_len--;
				env_list[i].var =
					env_list[env_len].var;
				env_list[i].env =
					env_list[env_len].env;
			}
			else
			{
				/* replace */
				env_list[i].var = var;
				env_list[i].env = env;
			}

			return;
		}
	}
	if (env == NULL)
	{
		return;
	}
	/* not found */
	if (env_list == NULL)
	{
		/* list is still empty */
		env_len_allocated = ENV_LIST_INC;
		env_list = xcalloc(sizeof(env_list_item), env_len_allocated);
	}
	else if (env_len >= env_len_allocated && env != NULL)
	{
		/* need more memory */
		env_len_allocated = env_len + ENV_LIST_INC;
		env_list = xrealloc((void *)env_list, (env_len_allocated),
				sizeof(env_list_item));
	}
	env_list[env_len].var = var;
	env_list[env_len].env = env;
	env_len++;

	return;
}

/* This function keeps a list of all strings that were set in the environment.
 * If a variable is written again, the old memory is freed.  This function
 * should be called instead of putenv().
 *
 *   var - environement variable name
 *   env - environment string ("variable=value")
 *
 * Both arguments are copied internally and should be freed after calling this
 * function.
 */
void flib_putenv(char *var, char *env)
{
	char *s;

	s = xstrdup(var);
	var = s;
	s = xstrdup(env);
	env = s;
	putenv(env);
	add_to_envlist(var, env);

	return;
}

void flib_unsetenv(const char *name)
{
	if (FHaveUnsetenv)
	{
		unsetenv(name);
	}
	else
	{
		int rc;

		/* try putenv without '=' */
		rc = putenv((char *)name);
		if (rc == 0 || getenv(name) != NULL)
		{
			/* failed, write empty string */
			flib_putenv((char *)name, "");
			return;
		}
	}
	add_to_envlist((char *)name, NULL);

	return;
}
