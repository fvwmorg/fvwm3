/*
 * Copyright (c) 2007 Nicholas Marriott <nicholas.marriott@gmail.com>
 * Copyright (c) 2019 Thomas Adam <thomas@fvwm.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/time.h>

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fvwmlib.h"
#include "log.h"

static FILE	*log_file;
static int	 log_level;

static void	 log_vwrite(const char *, const char *, va_list);

/* Set log level. */
void
log_set_level(int ll)
{
	log_level = ll;
}

/* Open logging to file. */
void
log_open(void)
{
	char	*path = "fvwm3-output.log";

	if (log_level == 0)
		return;
	log_close();

	log_file = fopen(path, "a");
	if (log_file == NULL)
		return;

	setvbuf(log_file, NULL, _IOLBF, 0);
}

/* Toggle logging. */
void
log_toggle(void)
{
	if (log_level == 0) {
		log_level = 1;
		log_open();
		fvwm_debug(NULL, "log opened (because of SIGUSR2)");
	} else {
		fvwm_debug(NULL, "log closed (because of SIGUSR2)");
		log_level = 0;
		log_close();
	}
}

/* Close logging. */
void
log_close(void)
{
	if (log_file != NULL)
		fclose(log_file);
	log_file = NULL;
}

/* Write a log message. */
static void
log_vwrite(const char *func, const char *msg, va_list ap)
{
	char		*fmt, *sep = ":";
	struct timeval	 tv;

	if (log_file == NULL)
		return;

	if (func == NULL) {
		func = "";
		sep = "";
	}

	if (vasprintf(&fmt, msg, ap) == -1)
		exit(1);

	gettimeofday(&tv, NULL);
	if (fprintf(log_file, "[%lld.%06d] %s%s %s", (long long)tv.tv_sec,
	    (int)tv.tv_usec, func, sep, fmt) == -1)
		exit(1);
	/* Compat: some callers from conversion of printf(stderr, ...) most
	 * likely add a newline.  But we don't want to double-up on newlines
	 * in output.  Add one if not present.
	 */
	if (fmt[strlen(fmt) - 1] != '\n') {
		if (fprintf(log_file, "\n") == -1)
			exit(1);
	}

	fflush(log_file);

	free(fmt);
}

/* Log a debug message. */
void
fvwm_debug(const char *func, const char *msg, ...)
{
	va_list	ap;

	va_start(ap, msg);
	log_vwrite(func, msg, ap);
	va_end(ap);
}
