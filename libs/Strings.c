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

/*
** Strings.c: various routines for dealing with strings
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fvwmlib.h"

/************************************************************************
 *
 * Concatentates 3 strings
 *
 *************************************************************************/
char CatS[256];

char *CatString3(char *a, char *b, char *c)
{
  int len = 0;

  if(a != NULL)
    len += strlen(a);
  if(b != NULL)
    len += strlen(b);
  if(c != NULL)
    len += strlen(c);

  if (len > 255)
    return NULL;

  if(a == NULL)
    CatS[0] = 0;
  else
    strcpy(CatS, a);
  if(b != NULL)
    strcat(CatS, b);
  if(c != NULL)
    strcat(CatS, c);
  return CatS;
}

/***************************************************************************
 * A simple routine to copy a string, stripping spaces and mallocing
 * space for the new string
 ***************************************************************************/
void CopyString(char **dest, char *source)
{
  int len;
  char *start;

  if (source == NULL)
    {
      *dest = NULL;
      return;
    }
  while(((isspace(*source))&&(*source != '\n'))&&(*source != 0))
  {
    source++;
  }
  len = 0;
  start = source;
  while((*source != '\n')&&(*source != 0))
  {
    len++;
    source++;
  }

  source--;
  while((isspace(*source))&&(*source != 0)&&(len >0))
  {
    len--;
    source--;
  }
  *dest = safemalloc(len+1);
  strncpy(*dest,start,len);
  (*dest)[len]=0;
}

/****************************************************************************
 *
 * Copies a string into a new, malloc'ed string
 * Strips leading spaces and trailing spaces and new lines
 *
 ****************************************************************************/
char *stripcpy( const char *source )
{
    const char* tmp;
    char* ptr;
    int len;

    if(source == NULL)
	return NULL;

    while(isspace(*source))
	source++;
    len = strlen(source);
    tmp = source + len -1;

    while( (tmp >= source) && ((isspace(*tmp)) || (*tmp == '\n')) )
    {
	tmp--;
	len--;
    }
    ptr = safemalloc(len+1);
    strncpy(ptr,source,len);
    ptr[len]=0;
    return ptr;
}

int StrEquals(char *s1,char *s2)
{
  if (!s1 && !s2)
    return 1;
  if (!s1 || !s2)
    return 0;
  return (strcasecmp(s1,s2)==0);
}
