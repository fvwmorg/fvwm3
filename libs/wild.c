#include <stdio.h>

#ifndef TRUE                    
#define TRUE	1
#define FALSE	0
#endif

/*****************************************************************************
 *	Does `string' match `pattern'? '*' in pattern matches any sub-string
 *	(including the null string) '?' matches any single char. For use
 *	by filenameforall. Note that '*' matches across directory boundaries
 *
 *      This code donated by  Paul Hudson <paulh@harlequin.co.uk>    
 *      It is public domain, no strings attached. No guarantees either.
 *
 *****************************************************************************/
int matchWildcards(char *pattern, char *string)
{
  if(string == NULL)
    {
      if(pattern == NULL)
	return TRUE;
      else if(strcmp(pattern,"*")==0)
	return TRUE;
      else
	return FALSE;
    }
  if(pattern == NULL)
    return TRUE;

  while (*string && *pattern)
    {
      if (*pattern == '?')
	{
	  /* match any character */
	  pattern += 1;
	  string += 1;
	}
      else if (*pattern == '*')
	{
	  /* see if the rest of the pattern matches any trailing substring
	     of the string. */
	  pattern += 1;
	  if (*pattern == 0)
	    {
	      return TRUE; /* trailing * must match rest */
	    }
	  while (*string)
	    {
	      if (matchWildcards(pattern,string))
		{
		  return TRUE;
		}
	      string++;
	    }
	  return FALSE;
	}
      else
	{
	  if (*pattern == '\\')
	    pattern ++;	   /* has strange, but harmless effects if the last
			      character is a '\\' */
	  if  (*pattern++ != *string++) 
	    {
	      return FALSE;
	    }
	}
    }
  if((*pattern == 0)&&(*string == 0))
    return TRUE;
  if((*string == 0)&&(strcmp(pattern,"*")==0))
    return TRUE;
  return FALSE;
}

