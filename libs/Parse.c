/*
** Parse.c: routines for parsing in fvwm & modules
*/

#include <stdio.h>
#include <string.h>
#include "fvwmlib.h"

/*
** PeekToken: returns next token from string, leaving string intact
**            (you should free returned string later)
**
** notes - quotes and brakets deonte blocks, which are returned MINUS
**         the start & end delimiters.  Delims within block may be escaped
**         with a backslash \ (which is removed when returned).  Backslash
**         must be escaped too, if you want one... (\\).  Tokens may be up
**         to MAX_TOKEN_LENGTH in size.
*/
char *PeekToken(const char *pstr)
{
  char *tok=NULL,*p;
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
** GetToken: destructively rips next token from string, returning it
**           (you should free returned string later)
*/
char *GetToken(char **pstr)
{
  char *new_pstr=NULL,*tok,*ws;

  if (!pstr || !(tok=PeekToken(*pstr)))
    return NULL;

  /* skip leading WS, tok, and following WS/separators in pstr & realloc */
  ws = *pstr;
  EatWS(ws);
  ws += strlen(tok);
  EatWS(ws);

  if (*ws)
    new_pstr = strdup(ws);

  free(*pstr);
  *pstr = new_pstr;

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
    rc = mystrcasecmp(tok,ntok);
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
    rc = (mystrcasecmp(tok,ntok)==0);
    free(ntok);
  }
  return rc;
}

/*
** NukeToken: removes next token from string
*/
void NukeToken(char **pstr)
{
  char *foo;
  foo = GetToken(pstr);
  free(foo);
}

#if 0
/*
** GetNextToken: equiv interface of old parsing routine, for ease of transition
*/
char *GetNextToken(char *indata,char **token)
{
  char *nindata=indata;

  *token = PeekToken(indata);

  if (*token)
    nindata+=strlen(*token);
  EatWS(nindata);

  return nindata;
}
#else
/****************************************************************************
 *
 * Gets the next "word" of input from char string indata.
 * "word" is a string with no spaces, or a qouted string.
 * Return value is ptr to indata,updated to point to text after the word
 * which is extracted.
 * token is the extracted word, which is copied into a malloced
 * space, and must be freed after use. 
 *
 **************************************************************************/
char *GetNextToken(char *indata,char **token)
{ 
  char *t,*start, *end, *text;

  t = indata;
  if(t == NULL)
    {
      *token = NULL;
      return NULL;
    }
  while(isspace(*t)&&(*t != 0))t++;
  start = t;
  while(!isspace(*t)&&(*t != 0))
    {
      /* Check for qouted text */
      if(*t == '"')
	{
	  t++;
	  while((*t != '"')&&(*t != 0))
	    {
	      /* Skip over escaped text, ie \" or \space */
	      if((*t == '\\')&&(*(t+1) != 0))
		t++;
	      t++;
	    }
	  if(*t == '"')
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

  while(start < end)
    {
      /* Check for qouted text */
      if(*start == '"')
	{	
	  start++;
	  while((*start != '"')&&(*start != 0))
	    {
	      /* Skip over escaped text, ie \" or \space */
	      if((*start == '\\')&&(*(start+1) != 0))
		start++;
	      *text++ = *start++;
	    }
	  if(*start == '"')
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

  return end;
}
#endif /* 0 */

