/*
   File:		ModParse.c

   Modifications:

   Changed 08/30/98 by Dan Espen:
   - Copied from FvwmEvent/Parse.c.  Set up for general use by modules.
   Still to be done, try to integrate with Parse.c which is used by fvwm
   itself for parsing.
*/


#include "fvwmlib.h"
#include "ModParse.h"

/*
** PeekArgument: returns next token from string, leaving string intact
**            (you should free returned string later)
**
** notes - quotes and brakets deonte blocks, which are returned MINUS
**         the start & end delimiters.  Delims within block may be escaped
**         with a backslash \ (which is removed when returned).  Backslash
**         must be escaped too, if you want one... (\\).  Arguments may be up
**         to MAX_TOKEN_LENGTH in size.
*/


char *PeekArgument(const char *pstr)
{
  char *tok=NULL;
  const char *p;
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
** GetArgument: destructively rips next token from string, returning it
**           (you should free returned string later)
*/
char *GetArgument(char **pstr)
{
  char *tok ;

  if (!pstr || !*pstr || !(tok=PeekArgument(*pstr)))
    return NULL;			/* *pstr=NULL; ???? */

  /* skip tok and following whitespace/separators in pstr & DON'T realloc */
  EatWS(*pstr);
  *pstr += strlen(tok);			/* != actual size (quote-collapsing) */
  EatWS(*pstr);

  if (!**pstr) 
      *pstr=NULL;			/* change \0 to NULL */

  return tok;
}

/*
** CmpArgument: does case-insensitive compare on next token in string, leaving
**           string intact (return code like strcmp)
*/
int CmpArgument(const char *pstr,char *tok)
{
  int rc=0;
  char *ntok=PeekArgument(pstr);
  if (ntok)
  {
    rc = strcasecmp(tok,ntok);
    free(ntok);
  }
  return rc;
}

/*
** MatchArgument: does case-insensitive compare on next token in string, leaving
**             string intact (returns true if matches, false otherwise)
*/
int MatchArgument(const char *pstr,char *tok)
{
  int rc=0;
  char *ntok=PeekArgument(pstr);
  if (ntok)
  {
    rc = (strcasecmp(tok,ntok)==0);
    free(ntok);
  }
  return rc;
}


#if 0
/*
** GetNextArgument: equiv interface of old parsing routine, for ease of transition
*/
char *GetNextArgument(char *indata,char **token)
{
  char *nindata=indata;

  *token = PeekArgument(indata);

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
char *GetNextArgument(char *indata,char **token)
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
	      /* Skip over escaped text, ie \" or \space " */
	      if((*t == '\\')&&(*(t+1) != 0))
		t++;
	      t++;
	    }
	  if(*t == '"')
	    t++;
	}
      else
	{
	  /* Skip over escaped text, ie \" or \space " */
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
	      /* Skip over escaped text, ie \" or \space " */
	      if((*start == '\\')&&(*(start+1) != 0))
		start++;
	      *text++ = *start++;
	    }
	  if(*start == '"')
	    start++;
	}
      else
	{
	  /* Skip over escaped text, ie \" or \space " */
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



/*
   function:		MatchToken
   description:	        matches one word
   returns:		pointer to delimiter character
                        NULL if no match
*/

#if 0					/* supported in 2.0.47b */
const char *MatchToken(register const char *s, register const char *w)
{
    if (s==NULL) return NULL;
    assert(w!=NULL);	/* the token may not be NULL -> design flaw */

    while (*w && (*s==*w ||
#ifdef WORD_IS_UPPERCASE
		            isupper(*s) && _toupper(*s)==*w
#else
		            toupper(*s)==toupper(*w)
#endif
		  ))
	s++,w++;

    if (*w=='\0' &&			          /* end of word and */
	(*s=='\0' || ispunct(*s) || isspace(*s))) /* same in string */
	return s;				  /* return endptr */
    else 
	return NULL;				  /* no match */
}
#endif

/*
   function:		CmpToken
   description:	        compare 1st word of s to 1st word of w
   returns:		< 0  if s < t
                        = 0  if s = t
                        > 0  if s > t

   Note arguments are not declares register, so the function can be 
   used with the bsearch() <search.h> function of the c library.

*/

int XCmpToken(char *s, char **t)
{
    register char *w=*t;

    if (w==NULL) return 1;		/* non existant word */
    if (s==NULL) return -1;		/* non existant string */

    while (*w && (*s==*w ||
#ifdef WORD_IS_UPPERCASE
		            isupper(*s) && _toupper(*s)==*w
#else
		            toupper(*s)==toupper(*w)
#endif
		  ))
	s++,w++;

    if ((*s=='\0' && (ispunct(*w) || isspace(*w)))||
	(*w=='\0' && (ispunct(*s) || isspace(*s))) )
	return 0;			/* 1st word equal */
    else 
	return toupper(*s)-toupper(*w);	/* smaller/greater */
}

