/*
** Parse.c: routines for parsing in fvwm & modules
*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fvwmlib.h"

/*
** PeekToken: returns next token from string, leaving string intact
**            (you should free returned string later)
**
** notes - quotes and brakets denote blocks, which are returned MINUS
**         the start & end delimiters.  Delims within block may be escaped
**         with a backslash \ (which is removed when returned).  Backslash
**         must be escaped too, if you want one... (\\).  Tokens may be up
**         to MAX_TOKEN_LENGTH in size.
*/
char *PeekToken(const char *pstr)
{
  char *tok=NULL;
  const char* p;
  char bc=0,be=0,tmptok[MAX_TOKEN_LENGTH];
  int len=0;

  if (!pstr)
    return NULL;

  p=pstr;
  EatWS(p); /* skip leading space */
  if (*p)
  {
    if (IsQuote(*p) || IsBlockStart(*p)) /* quoted string or block start? */
    {
      bc = *p; /* save block start char */
      p++;
    }
    /* find end of token */
    while (*p && len < MAX_TOKEN_LENGTH)
    {
      /* first, check stop conditions based on block or normal token */
      if (bc)
      {
        if ((IsQuote(*p) && bc == *p) || IsBlockEnd(*p,bc))
        {
          be = *p;
          break;
        }
      }
      else /* normal token */
      {
        if (isspace(*p) || *p == ',')
          break;
      }

      if (*p == '\\' && *(p+1)) /* if \, copy next char verbatim */
        p++;
      tmptok[len] = *p;
      len++;
      p++;
    }

    /* sanity checks: */
    if (bc && !be) /* did we have block start, but not end? */
    {
      /* should yell about this */
      return NULL;
    }

    if (len)
    {
      tok = (char *)malloc(len+1);
      strncpy(tok,tmptok,len);
      tok[len]='\0';
    }
  }

  return tok;
}

/*
** CmpToken: does case-insensitive compare on next token in string, leaving
**           string intact (return code like strcmp)
*/
int CmpToken(const char *pstr,char *tok)
{
  int rc=0;
  char *ntok=PeekToken(pstr);
  if (ntok)
  {
    rc = strcasecmp(tok,ntok);
    free(ntok);
  }
  return rc;
}

/*
** MatchToken: does case-insensitive compare on next token in string, leaving
**             string intact (returns true if matches, false otherwise)
*/
int MatchToken(const char *pstr,char *tok)
{
  int rc=0;
  char *ntok=PeekToken(pstr);
  if (ntok)
  {
    rc = (strcasecmp(tok,ntok)==0);
    free(ntok);
  }
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

  if (next = GetNextToken(*pstr, &tok))
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
 * empty string. If the token consists only of whitespace or delimiters,
 * the returned token is NULL instead.
 *
 * spaces = string of characters to treat as spaces
 * delims = string of characters delimiting token
 *
 * Use "spaces" and "delims" to define additional space/delimiter
 * characters (spaces are skipped before a token, delimiters are not).
 *
 **************************************************************************/
char *DoGetNextToken(char *indata, char **token, char *spaces, char *delims)
{ 
  char *t,*start, *end, *text;
  int snum;
  int dnum;

  snum = (spaces) ? strlen(spaces) : 0;
  dnum = (delims) ? strlen(delims) : 0;
  t = indata;
  if(t == NULL)
    {
      *token = NULL;
      return NULL;
    }
  while ( (isspace(*t) || (snum && strchr(spaces, *t))) && (*t != 0) ) t++;
  start = t;
  while (!(isspace(*t) ||
	   (snum && strchr(spaces, *t)) || (dnum && strchr(delims, *t))) &&
	 (*t != 0) )
    {
      /* Check for qouted text */
      if (IsQuote(*t))
	{
	  char c = *t;

	  t++;
	  while((*t != c)&&(*t != 0))
	    {
	      /* Skip over escaped text, ie \" or \space */
	      if((*t == '\\')&&(*(t+1) != 0))
		t++;
	      t++;
	    }
	  if(*t == c)
	    t++;
	}
      else
	{
	  /* Skip over escaped text, ie \" or \space */
	  if((*t == '\\')&&(*(t+1) != 0))
	    t++;
	  t++;
	}
    }
  end = t;

  text = safemalloc(end-start+1);
  *token = text;

  /* copy token */
  while(start < end)
    {
      /* Check for qouted text */
      if(IsQuote(*start))
	{	
	  char c = *start;
	  start++;
	  while((*start != c)&&(*start != 0))
	    {
	      /* Skip over escaped text, ie \" or \space */
	      if((*start == '\\')&&(*(start+1) != 0))
		start++;
	      *text++ = *start++;
	    }
	  if(*start == c)
	    start++;
	}
      else
	{
	  /* Skip over escaped text, ie \" or \space */
	  if((*start == '\\')&&(*(start+1) != 0))
	    start++;
	  *text++ = *start++;
	}
    }
  *text = 0;
  if(*end != 0)
    end++;

  if (**token == 0)
    {
      free(*token);
      *token = NULL;
    }
  return end;
}

char *GetNextToken(char *indata,char **token)
{
  return DoGetNextToken(indata, token, NULL, NULL);
}

char *GetNextOption(char *indata,char **token)
{
  return DoGetNextToken(indata, token, ",", NULL);
}

char *SkipNTokens(char *indata, unsigned int n)
{
  char *tmp;
  char *junk;

  tmp = indata;
  for ( ; n > 0 ; n--)
    {
      tmp = GetNextToken(tmp, &junk);
      if (junk)
	free(junk);
    }
  return tmp;
}

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
 *
 **************************************************************************/
int GetIntegerArguments(char *action, char **ret_action, int retvals[],int num)
{
  int i;
  char *token;

  for (i = 0; i < num && action; i++)
    {
      action = GetNextToken(action, &token);
      if (token == NULL)
	break;
      if (sscanf(token, "%d", &(retvals[i])) != 1)
	break;
      free(token);
      token = NULL;
    }
  if (token)
    free(token);
  if (ret_action != NULL)
    *ret_action = action;

  return i;
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

  if (!token && !list)
    {
      if (next)
	*next = NULL;
      return -1;
    }
  l = (len) ? len : strlen(token);
  for (i = 0; list[i] != NULL; i++)
    {
      if (len < 0)
	l = strlen(list[i]);
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
 * token from ste string action with GetNextToken. The index is returned
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
