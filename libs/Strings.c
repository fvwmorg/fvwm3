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
#include "config.h"

#include <ctype.h>

#include "Strings.h"
#include "safemalloc.h"


#define CHUNK_SIZE 256


char* CatString3(const char *a, const char *b, const char *c)
{
    static char* buffer = NULL;
    static int buffer_len = 0;

    int len = 0;
    if (a != NULL) len += strlen(a);
    if (b != NULL) len += strlen(b);
    if (c != NULL) len += strlen(c);

    /* Expand buffer to fit string, to a multiple of CHUNK_SIZE */
    if (len > buffer_len) {
	if ( buffer ) free( buffer );

	buffer_len = CHUNK_SIZE * (1 + len / CHUNK_SIZE);
	buffer = safemalloc( buffer_len );
    }

    buffer[0] = 0;
    if (a != NULL) strcat( buffer, a );
    if (b != NULL) strcat( buffer, b );
    if (c != NULL) strcat( buffer, c );

    return buffer;
}

#undef CHUNK_SIZE


void CopyString(char **dest, const char *source)
{
    int len;
    const char *start;

    if (source == NULL) {
	*dest = NULL;
	return;
    }

    /* set 'start' to the first character of the string,
       skipping over spaces, but not newlines
       (newline terminates the string) */

    while ( isspace((unsigned char)*source) && (*source != '\n') )
	source++;
    start = source;

    /* set 'len' to the length of the string, ignoring
       trailing spaces */

    len = 0;
    while ( (*source != '\n') && (*source != 0) )
    {
	len++;
	source++;
    }
    source--;

    while( len > 0 && isspace((unsigned char)*source) )
    {
	len--;
	source--;
    }

  *dest = safemalloc(len+1);
  strncpy(*dest,start,len);
  (*dest)[len]=0;
}


void CopyStringWithQuotes(char **dest, const char *src)
{
	while (src && src[0] == ' ')
	{
		src++;
	}
	if (src && src[0] == '"')
	{
		int len;

		src++;
		CopyString(dest, src);
		len = strlen(*dest);
		if (len > 0 && (*dest)[len - 1] == '"')
		{
			(*dest)[len - 1] = '\0';
		}
	}
	else
	{
		CopyString(dest, src);
	}
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

    while(isspace((unsigned char)*source))
	source++;
    len = strlen(source);
    tmp = source + len -1;

    while( (tmp >= source) && ((isspace((unsigned char)*tmp)) ||
			       (*tmp == '\n')) )
    {
	tmp--;
	len--;
    }
    ptr = safemalloc(len+1);
    if (len) {
      strncpy(ptr,source,len);
    }
    ptr[len]=0;
    return ptr;
}


int StrEquals( const char *s1, const char *s2 )
{
    if (s1 == NULL && s2 == NULL) return 1;
    if (s1 == NULL || s2 == NULL) return 0;
    return strcasecmp(s1,s2) == 0;
}


int StrHasPrefix( const char* string, const char* prefix )
{
    if ( prefix == NULL ) return 1;
    if ( string == NULL ) return 0;
    return strncasecmp( string, prefix, strlen(prefix) ) == 0;
}


/****************************************************************************
 *
 * Adds single quotes arround the string and escapes single quotes with
 * backslashes.  The result is placed in the given dest, not allocated.
 * The end of destination, i.e. pointer to '\0' is returned.
 * You should allocate dest yourself, at least strlen(source) * 2 + 3.
 *
 ****************************************************************************/
char *QuoteString(char *dest, const char *source)
{
  int i = 0;
  *dest++ = '\'';
  for(i = 0; source[i]; i++)
  {
    if (source[i] == '\'')
      *dest++ = '\\';
    *dest++ = source[i];
  }
  *dest++ = '\'';
  *dest = '\0';
  return dest;
}
