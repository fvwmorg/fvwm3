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

#include "config.h"

#include <stdio.h>

#include "wild.h"

/*
 *      Does `string' match `pattern'? '*' in pattern matches any sub-string
 *      (including the null string) '?' matches any single char. For use
 *      by filenameforall. Note that '*' matches across directory boundaries
 *
 *      This code donated by  Paul Hudson <paulh@harlequin.co.uk>
 *      It is public domain, no strings attached. No guarantees either.
 *
 */
int matchWildcards(const char *pattern, const char *string)
{
	if(string == NULL)
	{
		if(pattern == NULL)
			return 1;
		else if(strcmp(pattern,"*")==0)
			return 1;
		else
			return 0;
	}
	if(pattern == NULL)
		return 1;

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
			/* see if the rest of the pattern matches any trailing
			 * substring of the string. */
			pattern += 1;
			if (*pattern == 0)
			{
				return 1; /* trailing * must match rest */
			}
			while (*string)
			{
				if (matchWildcards(pattern,string))
				{
					return 1;
				}
				string++;
			}
			return 0;
		}
		else
		{
			if (*pattern == '\\')
			{
				/* has strange, but harmless effects if the
				 * last character is a '\\' */
				pattern ++;
			}
			if  (*pattern++ != *string++)
			{
				return 0;
			}
		}
	}
	if((*pattern == 0)&&(*string == 0))
		return 1;
	if((*string == 0)&&(strcmp(pattern,"*")==0))
		return 1;

	return 0;
}

