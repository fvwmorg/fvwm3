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

#include <stdio.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#include "wild.h"

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

