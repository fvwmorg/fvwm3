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

#include <stdlib.h>
#include <stdio.h>

#include "safemalloc.h"

static void alloc_failed(char c, int length)
{
  fprintf(stderr, "%calloc of %d bytes failed. Exiting\n", c, length);
  exit(1);
}


/***********************************************************************
 *
 *  Procedure:
 *	safemalloc - mallocs specified space or exits if there's a
 *		     problem
 *
 ***********************************************************************/
char *safemalloc(int length)
{
  char *ptr;

  if(length <= 0)
    length = 1;

  ptr = malloc(length);
  if(ptr == (char *)0)
  {
    /* doesn't return */
    alloc_failed('m', length);
  }
  return ptr;
}


/***********************************************************************
 *
 *  Procedure:
 *	safecalloc - mallocs specified space or exits if there's a
 *		     problem
 *
 ***********************************************************************/
char *safecalloc(int num, int length)
{
  char *ptr;

  ptr = calloc(num, length);
  if(ptr == (char *)0)
  {
    /* doesn't return */
    alloc_failed('c', length);
  }
  return ptr;
}
