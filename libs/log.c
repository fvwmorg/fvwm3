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
#include "getpwuid.h"
#include "defaults.h"
#include "log.h"

static char	*log_file_name;
static FILE	*log_file;
int	lib_log_level = 0;

void
set_log_file(char *name)
{
	free(log_file_name);
	log_file_name = NULL;
	if (name != NULL)
	{
		log_file_name = fxstrdup(name);
	}

}

/* Open logging to file. */
void
log_open(const char *fvwm_userdir)
{
	char *path, *file_name;
	char *expanded_path;

	if (lib_log_level == 0)
		return;
	/* determine file name or file path to use */
	file_name = log_file_name;
	if (file_name == NULL)
	{
		file_name = getenv("FVWM3_LOGFILE");
	}
	if (file_name == NULL)
	{
		file_name = FVWM3_LOGFILE_DEFAULT;
	}
	/* handle stderr logging */
	if (file_name[0] == '-' && file_name[1] == 0)
	{
		log_file = stderr;
		return;
	}
	/* handle file logging */
	expanded_path = expand_path(file_name);
	if (expanded_path[0] == '/')
	{
		path = expanded_path;
	}
	else
	{
		xasprintf(&path, "%s/%s", fvwm_userdir, expanded_path);
		free((char *)expanded_path);
	}

	log_close();

	if ((log_file = fopen(path, "a")) == NULL) {
		free(path);
		return;
	}

	setvbuf(log_file, NULL, _IOLBF, 0);

	free(path);
}

/* Toggle logging. */
void
log_toggle(const char *fvwm_userdir)
{
	if (lib_log_level == 0) {
		lib_log_level = 1;
		log_open(fvwm_userdir);
		fvwm_debug(NULL, "log opened (because of SIGUSR2)");
	} else {
		fvwm_debug(NULL, "log closed (because of SIGUSR2)");
		lib_log_level = 0;
		log_close();
	}
}

/* Close logging. */
void
log_close(void)
{
	if (log_file != NULL && log_file != stderr)
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
