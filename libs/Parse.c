/*
** Parse.c: routines for parsing in fvwm & modules
*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fvwmlib.h"

/* If the string s begins with a quote chracter SkipQuote returns a pointer
 * to the first unquoted character or to the final '\0'. If it does not, a
 * pointer to the next character in the string is returned.
 * There are three possible types of quoting: a backslash quotes the next
 * character only. Long quotes like " " or ' ' quoting everything in
 * between and quote pairs like ( ) or { }.
 *
 * precedence:
 *
 * 1) Backslashes are honoured always, even inside long or pair quotes.
 * 2) long quotes do quote quoting pair characters but not simple quotes. All
 *    long quotes can quote all other types of long quotes).
 * 3) pair quotes none of the above. Text between a pair of quotes is treated
 *    as a single token.
 *
 * qlong   - string of long quoted (defaults to "'` )
 * qstart  - string of pair quote start characters (defaults to empty string)
 * qend    - string of pair quote end characters (defaults to empty string)
 *
 * The defaults are used if NULL is passed for the corresponding string.
 */
char *SkipQuote(char *s, const char *qlong, const char *qstart,
		const char *qend)
{
  char *t;

  if (s == NULL || *s == 0)
    return s;
  if (!qlong)
    qlong = "\"'`";
  if (!qstart)
    qstart = "";
  if (!qend)
    qend = "";

  if (*s == '\\' && s[1] != 0)
    return s+2;
  else if (*qlong && (t = strchr(qlong, *s)))
    {
      char c = *t;

      s++;
      while(*s && *s != c)
	{
	  /* Skip over escaped text, ie \quote */
	  if(*s == '\\' && *(s+1) != 0)
	    s++;
	  s++;
	}
      if(*s == c)
	s++;
      return s;
    }
  else if (*qstart && (t = strchr(qstart, *s)))
    {
      char c = *((t - qstart) + qend);

      while (*s && *s != c)
	s = SkipQuote(s, qlong, "", "");
      return (*s == *t) ? ++s : s;
    }
  else
    return ++s;
}

/* Returns a string up to the first character from the string delims in a
 * malloc'd area just like GetNextToken. Quotes are not removed from the
 * returned string. The returned string is stored in *sout, the return value
 * of this call is a pointer to the first character after the delimiter or
 * to the terminating '\0'. Quoting is handled like in SkipQuote. */
char *GetQuotedString(char *sin, char **sout, const char *delims,
		      const char *qlong, const char *qstart, const char *qend)
{
  char *t = sin;
  unsigned int len;

  if (!sout || !sin)
    return NULL;

  while (*t && !strchr(delims, *t))
    t = SkipQuote(t, qlong, qstart, qend);
  len = t - sin;
  *sout = (char *)safemalloc(len + 1);
  memcpy(*sout, sin, len);
  (*sout)[len] = 0;
  if (*t)
    t++;

  return t;
}

/* SkipSpaces: returns a pointer to the first character in indata that is
 * neither a whitespace character nor contained in the string 'spaces'. snum
 * is the number of characters in 'spaces'. You must not pass a NULL pointer
 * in indata. */
char *SkipSpaces(char *indata, char *spaces, int snum)
{
  while (*indata != 0 && (isspace((unsigned char)*indata) ||
			  (snum && strchr(spaces, *indata))))
    indata++;
  return indata;
}

/* Copies a token beginning at src to a previously allocated area dest. dest
 * must be large enough to hold the token. Leading whitespaces cause the token
 * to be NULL. */
char *CopyToken(char *src, char *dest, char *spaces, int snum, char *delims,
		int dnum, char *out_delim)
{
  while ( (*src != 0) && !(isspace((unsigned char)*src) ||
			   (snum && strchr(spaces, *src)) ||
			   (dnum && strchr(delims, *src))))
    {
      /* Check for qouted text */
      if (IsQuote(*src))
	{
	  char c = *src;

	  src++;
	  while((*src != c)&&(*src != 0))
	    {
	      *(dest++) = *src;
	      /* Skip over escaped text, ie \quote */
	      if((*src == '\\')&&(*(src+1) != 0))
		*(dest++) = *(++src);
	      src++;
	    }
	  if(*src == c)
	    src++;
	}
      else
	{
	  *(dest++) = *src;
	  /* Skip over escaped text, ie \" or \space */
	  if((*src == '\\')&&(*(src+1) != 0))
	    *(dest++) = *(++src);
	  src++;
	}
    }
  if (out_delim)
    *out_delim = *src;
  *dest = 0;
  if (*src != 0)
    src++;
  return src;
}


/*
** DoPeekToken: returns next token from string, leaving string intact
**              (you must not free returned string)
**
** For a description of the parameters see DoGetNextToken below. DoPeekToken
** is a bit faster.
*/
char *DoPeekToken(char *indata, char **token, char *spaces, char *delims,
		  char *out_delim)
{
  char *end;
  int snum;
  int dnum;
  char tmptok[MAX_TOKEN_LENGTH];

  snum = (spaces) ? strlen(spaces) : 0;
  dnum = (delims) ? strlen(delims) : 0;
  if(indata == NULL)
    {
      if (out_delim)
	*out_delim = '\0';
      *token = NULL;
      return NULL;
    }
  indata = SkipSpaces(indata, spaces, snum);
  end = CopyToken(indata, tmptok, spaces, snum, delims, dnum, out_delim);

  if (tmptok[0] == 0)
    *token = NULL;
  else
    *token = tmptok;
  return end;
}

char *PeekToken(char *indata, char **token)
{
  return DoPeekToken(indata, token, NULL, NULL, NULL);
}

/* Tries to seek up to n tokens in indata. Returns the number of tokens
 * actually found (up to a maximum of n). */
int CheckNTokens(char *indata, unsigned int n)
{
  unsigned int i;
  char *token;

  for (i = 0; i < n; i++)
    {
      indata = PeekToken(indata, &token);
      if (token == NULL)
	break;
    }
  return i;
}

/*
** MatchToken: does case-insensitive compare on next token in string, leaving
**             string intact (returns true if matches, false otherwise)
*/
int MatchToken(char *pstr,char *tok)
{
  int rc=0;
  char *ntok;

  DoPeekToken(pstr, &ntok, NULL, NULL, NULL);
  if (ntok)
    rc = (strcasecmp(tok,ntok)==0);
  return rc;
}

/*
** NukeToken: removes next token from string
*/
void NukeToken(char **pstr)
{
  char *tok;
  char *next;
  char *temp = NULL;

  next = GetNextToken(*pstr, &tok);
  if (next != NULL);
    temp = strdup(next);
  if (pstr && *pstr)
    free(*pstr);
  *pstr = temp;
  if (tok)
    free(tok);
}

/****************************************************************************
 *
 * Gets the next "word" of input from string indata.
 * "word" is a string with no spaces, or a qouted string.
 * Return value is ptr to indata,updated to point to text after the word
 * which is extracted.
 * token is the extracted word, which is copied into a malloced
 * space, and must be freed after use. DoGetNextToken *never* returns an
 * empty string or token. If the token consists only of whitespace or
 * delimiters, the returned token is NULL instead. If out_delim is given,
 * the character ending the string is returned therein.
 *
 * spaces = string of characters to treat as spaces
 * delims = string of characters delimiting token
 *
 * Use "spaces" and "delims" to define additional space/delimiter
 * characters (spaces are skipped before a token, delimiters are not).
 *
 **************************************************************************/
char *DoGetNextToken(char *indata, char **token, char *spaces, char *delims,
		     char *out_delim)
{
  char *tmptok;
  char *end;

  end = DoPeekToken(indata, &tmptok, spaces, delims, out_delim);

  if (tmptok == NULL)
    *token = NULL;
  else
    *token = strdup(tmptok);

  return end;
}

char *GetNextToken(char *indata, char **token)
{
  return DoGetNextToken(indata, token, NULL, NULL, NULL);
}

char *GetNextOption(char *indata, char **token)
{
  return DoGetNextToken(indata, token, ",", NULL, NULL);
}

char *SkipNTokens(char *indata, unsigned int n)
{
  char *junk;

  for ( ; n > 0 ; n--)
    indata = PeekToken(indata, &junk);
  return indata;
}

/*
 *
 * convenience functions
 *
 */

/****************************************************************************
 *
 * Works like GetNextToken, but with the following differences:
 *
 * If *indata begins with a "*" followed by the string module_name,
 * it returns the string following directly after module_name as the
 * new token. Otherwise NULL is returned.
 * e.g. GetModuleResource("*FvwmPagerGeometry", &token, "FvwmPager")
 * returns "Geometry" in token.
 *
 **************************************************************************/
char *GetModuleResource(char *indata, char **resource, char *module_name)
{
  char *tmp;
  char *next;

  if (!module_name)
    {
      *resource = NULL;
      return indata;
    }
  next = GetNextToken(indata, &tmp);
  if (!tmp)
    return next;

  if (tmp[0] != '*' || strncasecmp(tmp+1, module_name, strlen(module_name)))
    {
      *resource = NULL;
      return indata;
    }
  CopyString(resource, tmp+1+strlen(module_name));
  free(tmp);
  return next;
}

/****************************************************************************
 *
 * This function uses GetNextToken to parse action for up to num integer
 * arguments. The number of values actually found is returned.
 * If ret_action is non-NULL, a pointer to the next token is returned there.
 * The suffixlist parameter points to a string off possible suffixes for the
 * integer values. The index of the matched suffix is returned in
 * ret_suffixnum (0 = no suffix, 1 = first suffix in suffixlist ...).
 *
 **************************************************************************/
int GetSuffixedIntegerArguments(char *action, char **ret_action, int retvals[],
				int num, char *suffixlist, int ret_suffixnum[])
{
  int i;
  int j;
  char *token;
  int suffixes;

  suffixes = (suffixlist != NULL) ? strlen(suffixlist) : 0;
  for (i = 0; i < num && action; i++)
  {
    action = GetNextToken(action, &token);
    if (token == NULL)
      break;
    if (sscanf(token, "%d", &(retvals[i])) != 1)
      break;
    if (suffixes != 0 && ret_suffixnum != NULL)
      {
	int len;
	char c;

	len = strlen(token) - 1;
	c = token[len];
	if (isupper(c))
	  c = tolower(c);
	for (j = 0; j < suffixes; j++)
	  {
	    char c2 = suffixlist[j];

	    if (isupper(c2))
	      c2 = tolower(c2);
	    if (c == c2)
	      {
		ret_suffixnum[i] = j+1;
		break;
	      }
	  }
	if (j == suffixes)
	  ret_suffixnum[i] = 0;
      }
    free(token);
    token = NULL;
  }
  if (token)
    free(token);
  if (ret_action != NULL)
    *ret_action = action;

  return i;
}

/****************************************************************************
 *
 * This function uses GetNextToken to parse action for up to num integer
 * arguments. The number of values actually found is returned.
 * If ret_action is non-NULL, a pointer to the next token is returned there.
 *
 **************************************************************************/
int GetIntegerArguments(char *action, char **ret_action, int retvals[],int num)
{
  GetSuffixedIntegerArguments(action, ret_action, retvals, num, NULL, NULL);
}


/***************************************************************************
 *
 * This function tries to match a token with a list of strings and returns
 * the position of token in the array or -1 if no match is found. The last
 * entry in the list must be NULL.
 *
 * len = 0 : only exact matches
 * len < 0 : match, if token begins with one of the strings in list
 * len > 0 : match, if the first len characters do match
 *
 * if next is non-NULL, *next will be set to point to the first character
 * in token after the match.
 *
 **************************************************************************/
int GetTokenIndex(char *token, char *list[], int len, char **next)
{
  int i;
  int l;
  int k;

  if (!token || !list)
    {
      if (next)
	*next = NULL;
      return -1;
    }
  l = (len) ? len : strlen(token);
  for (i = 0; list[i] != NULL; i++)
    {
      k = strlen(list[i]);
      if (len < 0)
	l = k;
      if (len == 0 && k != l)
	continue;
      if (!strncasecmp(token, list[i], l))
	break;
    }
  if (next)
    {
      *next = (list[i]) ? token + l : token;
    }

  return (list[i]) ? i : -1;
}

/***************************************************************************
 *
 * This function does roughly the same as GetTokenIndex but reads the
 * token from string action with GetNextToken. The index is returned
 * in *index. The return value is a pointer to the character after the
 * token (just like the return value of GetNextToken).
 *
 **************************************************************************/
char *GetNextTokenIndex(char *action, char *list[], int len, int *index)
{
  char *token;
  char *next;

  if (!index)
    return action;

  next = GetNextToken(action, &token);
  if (!token)
    {
      *index = -1;
      return action;
    }
  *index = GetTokenIndex(token, list, len, NULL);
  free(token);

  return (*index == -1) ? action : next;
}


int GetRectangleArguments(char *action, int *width, int *height)
{
  char *token;
  int n;

  GetNextToken(action, &token);
  if (!token)
    return 0;
  /* now try MxN style number, specifically for DeskTopSize: */
  n = sscanf(token, "%d%*c%d", width, height);
  free(token);

  return (n == 2) ? 2 : 0;
}

/* unit_io is input as well as output. If action has a postfix 'p' or 'P',
 * *unit_io is set to 100, otherwise it is left untouched. */
int GetOnePercentArgument(char *action, int *value, int *unit_io)
{
  unsigned int len;
  char *token;
  int n;

  *value = 0;
  if (!action)
    return 0;
  GetNextToken(action, &token);
  if (!token)
    return 0;

  len = strlen(token);
  /* token never contains an empty string, so this is ok */
  if (token[len - 1] == 'p' || token[len - 1] == 'P')
  {
    *unit_io = 100;
    token[len - 1] = '\0';
  }
  n = sscanf(token, "%d", value);

  free(token);
  return n;
}


int GetTwoPercentArguments(char *action, int *val1, int *val2, int *val1_unit,
			   int *val2_unit)
{
  char *tok1, *tok2;
  int n = 0;

  *val1 = 0;
  *val2 = 0;

  action = GetNextToken(action, &tok1);
  if (!tok1)
    return 0;
  GetNextToken(action, &tok2);
  if (GetOnePercentArgument(tok2, val2, val2_unit) == 1 &&
      GetOnePercentArgument(tok1, val1, val1_unit) == 1)
  {
    free(tok1);
    free(tok2);
    return 2;
  }

  /* now try MxN style number, specifically for DeskTopSize: */
  n = GetRectangleArguments(tok1, val1, val2);
  free(tok1);
  if (tok2)
    free(tok2);
  return n;
}

/* Parses the next token in action and returns 1 if it is "yes", "true", "y",
 * "t" or "on", zero if it is "no", "false", "n", "f" or "off" and -1 if it is
 * "toggle". A pointer to the first character in action behind the token is
 * returned through ret_action in this case. ret_action may be NULL. If the
 * token matches none of these strings the default_ret value is returned and
 * the action itself is passed back in ret_action. If the no_toggle flag is
 * non-zero, the "toggle" string is handled as no match. */
int ParseToggleArgument(char *action, char **ret_action, int default_ret,
			char no_toggle)
{
  int index;
  int rc;
  char *token;
  char *next;
  char *optlist[] = {
    "toggle",
    "yes", "no",
    "true", "false",
    "on", "off",
    "t", "f",
    "y", "n",
    NULL
  };

  next = GetNextToken(action, &token);
  index = GetTokenIndex(token, optlist, 0, NULL);
  if (index == 0 && no_toggle == 0)
  {
    /* toggle requested explicitly */
    rc = -1;
  }
  else if (index == -1 || (index == 0 && no_toggle))
  {
    /* nothing selected, use default and don't modify action */
    rc = default_ret;
    next = action;
  }
  else
  {
    /* odd numbers denote True, even numbers denote False */
    rc = (index & 1);
  }

  if (ret_action)
    *ret_action = next;
  if (token)
    free(token);
  return rc;
}
