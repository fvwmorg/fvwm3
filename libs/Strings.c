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
char *stripcpy(char *source)
{
  char *tmp,*ptr;
  int len;

  if(source == NULL)
    return NULL;

  while(isspace(*source))
    source++;
  len = strlen(source);
  tmp = source + len -1;
  while(((isspace(*tmp))||(*tmp == '\n'))&&(tmp >=source))
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
