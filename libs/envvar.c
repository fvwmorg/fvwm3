/**************************************************************************
 *
 *  FILE            envvar.c
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
 **************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fvwmlib.h"

#ifndef NULL
#define NULL 0
#endif

/**************************************************************************
 *                                                                        *
 *                       P R I V A T E    D A T A                         *
 *                                                                        *
 **************************************************************************/

/**************************************************************************
 *                                                                        *
 *                   P R I V A T E    F U N C T I O N S                   *
 *                                                                        *
 **************************************************************************/

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
	idx = l;
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
	li = maxstrlen - idx - 1;
    while (li--)
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
	if (*s == '$' && (isalpha(*next) || *next == '_' || *next == '{')) {
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
 *  NAME          getEnv
 *
 *  FUNCTION      Look up environment variable.
 *
 *  INPUT         name    name of environment variable to look up. This
 *                        may include $ and { }.
 *
 *  RETURNS       The variable contents, or "" if not found.
 *
 */
static const char *getEnv(const char *name)
{
    static char *empty = "";
    char   *ret, *tmp, *p, *p2;

    if ((tmp = strdup(name)) == NULL)
	return empty;  /* better than no test at all. */
    p = tmp;
    if (*p == '$')
	++p;
    if (*p == '{') {
	++p;
	if ((p2 = strchr(p, '}')) != NULL)
	    *p2 = '\0';
    }
    if ((ret = getenv(p)) == NULL)
	ret = empty;
    free(tmp);
    return ret;
}



/**************************************************************************
 *                                                                        *
 *                    P U B L I C    F U N C T I O N S                    *
 *                                                                        *
 **************************************************************************/

/*-------------------------------------------------------------------------
 *
 *  NAME          envExpand
 *
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
    char  *var, *s2, save;
    const char *env;
    int   len, ret = 0;

    s2 = s;
    while ((var = findEnvVar(s2, &len)) != NULL) {
	++ret;
	save = var[len];
	var[len] = '\0';
	env = getEnv(var);
	var[len] = save;
	strDel(s, var - s, len);
	strIns(s, env, var - s, maxstrlen);
	s2 = var + strlen(env);
    }
    return ret;
}



/*-------------------------------------------------------------------------
 *
 *  NAME          envDupExpand
 *
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
    char  *var, *ret, save;
    const char *env, *s2;
    int   len, slen, elen, bufflen;

    /*
     *  calculate length needed.
     */
    s2 = s;
    slen = strlen(s);
    bufflen = slen + 1 + extra;
    while ((var = findEnvVar(s2, &len)) != NULL) {
	save = var[len];
	var[len] = '\0';
	env = getEnv(var);
	var[len] = save;
	elen = strlen(env);
	/* need to make a buffer the maximum possible size, else we
	 * may get trouble while expanding. */
	bufflen += len > elen ? len : elen;
	s2 = var + len;
    }
    if (bufflen < slen + 1)
	bufflen = slen + 1;

    ret = safemalloc(bufflen);

    /*
     *  now do the real expansion.
     */
    strcpy(ret, s);
    envExpand(ret, bufflen - extra);

    return ret;
}
