/* Memory management routines...
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/* Originally from OpenBSD */

#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <err.h>
#include <errno.h>
#include <sys/param.h>
#include <stdint.h>

#include "safemalloc.h"
#include "log.h"

void *
fxmalloc(size_t length)
{
	void	*ptr;

	if (length == 0)
		errx(1, "malloc: zero size.");

	if ((ptr = malloc(length)) == NULL)
		err(1, "malloc: couldn't allocate");

	return (ptr);
}

void *
fxcalloc(size_t num, size_t length)
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
fxrealloc(void *oldptr, size_t nmemb, size_t size)
{
	size_t	 newsize = nmemb * size;
	void	*newptr;

	if (newsize == 0)
		errx(1, "zero size");
	if (SIZE_MAX / nmemb < size)
		errx(1, "nmemb * size > SIZE_MAX");
	if ((newptr = realloc(oldptr, newsize)) == NULL)
		err(1, "fxrealloc failed");

	return (newptr);
}

char *
fxstrdup(const char *s)
{
	char	*ptr;
	size_t	 len;

	len = strlen(s) + 1;
	ptr = fxmalloc(len);

	strlcpy(ptr, s, len);

	return (ptr);
}

int
xasprintf(char **ret, const char *fmt, ...)
{
	va_list ap;
	int i;

	va_start(ap, fmt);
	i = xvasprintf(ret, fmt, ap);
	va_end(ap);

	return (i);
}

int
xvasprintf(char **ret, const char *fmt, va_list ap)
{
	int i;

	i = vasprintf(ret, fmt, ap);

	if (i < 0 || *ret == NULL) {
		fvwm_debug(__func__, "xasprintf: %s", strerror(errno));
		exit (1);
	}

	return (i);
}