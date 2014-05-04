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
#include <stdint.h>
#include <err.h>
#include <sys/param.h>
#include <stdint.h>
#include "safemalloc.h"

void *
xmalloc(size_t length)
{
	void	*ptr;

	if (length == 0)
		errx(1, "malloc: zero size.");

	if ((ptr = malloc(length)) == NULL)
		err(1, "malloc: couldn't allocate");

	return (ptr);
}

void *
xcalloc(size_t num, size_t length)
{
	void	*ptr;

	if (num == 0 || length == 0)
		errx(1, "calloc:  zero size");

	if (SIZE_MAX / num < length)
		errx(1, "num * length > SIZE_MAX");

	if ((ptr = calloc(num, length)) == NULL)
		err(1, "calloc:  couldn't allocate");

	return (ptr);
}

void *
xrealloc(void *oldptr, size_t nmemb, size_t size)
{
	size_t	 newsize = nmemb * size;
	void	*newptr;

	if (newsize == 0)
		errx(1, "zero size");
	if (SIZE_MAX / nmemb < size)
		errx(1, "nmemb * size > SIZE_MAX");
	if ((newptr = realloc(oldptr, newsize)) == NULL)
		err(1, "xrealloc failed");

	return (newptr);
}

char *
xstrdup(const char *s)
{
	char	*ptr;
	size_t	 len;

	len = strlen(s) + 1;
	ptr = xmalloc(len);

	strlcpy(ptr, s, len);

	return (ptr);
}
