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
#include <stdlib.h>
#include <string.h>

#include "safemalloc.h"
#include "getpwuid.h"

const char
*find_home_dir(void)
{
	struct passwd		*pw;
	const char		*home;

	home = getenv("HOME");
	if (home == NULL || *home == '\0') {
		pw = getpwuid(getuid());
		if (pw != NULL)
			home = pw->pw_dir;
		else
			home = NULL;
	}

	return (home);
}

const char *
expand_path(const char *path)
{
	char			*expanded, *name;
	const char		*end, *value, *home;

	home = find_home_dir();

	if (strncmp(path, "~/", 2) == 0) {
		if (home == NULL)
			return (NULL);
		xasprintf(&expanded, "%s%s", home, path + 1);
		return (expanded);
	}

	if (*path == '$') {
		end = strchr(path, '/');
		if (end == NULL)
			name = fxstrdup(path + 1);
		else
			name = strndup(path + 1, end - path - 1);
		value = getenv(name);
		free(name);
		if (value == NULL)
			return (NULL);
		if (end == NULL)
			end = "";
		xasprintf(&expanded, "%s%s", value, end);
		return (expanded);
	}

	return (fxstrdup(path));
}
