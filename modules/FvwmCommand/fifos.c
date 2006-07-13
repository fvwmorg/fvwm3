/* -*-c-*- */
/*
 * Fvwm command input interface.
 *
 * Copyright 1998, Toshi Isogai.
 * Use this program at your own risk.
 * Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
 */

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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"
#include "FvwmCommand.h"

#define MAXHOSTNAME 32

char * fifos_get_default_name()
{
	char file_suffix[] = { 'R', 'C', 'M', '\0' };
	char *f_stem;
	char *dpy_name;
	char dpy_name_add[3];
	char *c;
	int i;
	struct stat stat_buf;
	char hostname[MAXHOSTNAME];
	int type_pos;
	uid_t owner;
	Bool is_path_valid;

	/* default name */
	dpy_name = getenv("DISPLAY");
	if (!dpy_name || *dpy_name == 0)
	{
		dpy_name = ":0.0";
	}
	if (strncmp(dpy_name, "unix:", 5) == 0)
	{
		dpy_name += 4;
	}
	dpy_name_add[0] = 0;
	c = strrchr(dpy_name, '.');
	i = 0;
	if (c != NULL)
	{
		if (*(c + 1) != 0)
		{
			for (c++, i = 0; isdigit(*c); c++, i++)
			{
				/* nothing */
			}
		}
		else
		{
			/* cut off trailing period */
			*c = 0;
		}
	}
	if (i == 0)
	{
		/* append screen number */
		strcpy(dpy_name_add, ".0");
	}
	f_stem = safemalloc(11 + strlen(F_NAME) + MAXHOSTNAME +
			    strlen(dpy_name) + strlen(dpy_name_add));

	if (
		(stat("/var/tmp", &stat_buf) == 0) &&
		(stat_buf.st_mode & S_IFDIR))
	{
		strcpy (f_stem, "/var/tmp/");
	}
	else
	{
		strcpy (f_stem, "/tmp/");
	}
	strcat(f_stem, F_NAME);

	/* Make it unique */
	if (!dpy_name[0] || ':' == dpy_name[0])
	{
		/* Put hostname before dpy if not there */
		gethostname(hostname, MAXHOSTNAME);
		strcat(f_stem, hostname);
	}
	strcat(f_stem, dpy_name);
	strcat(f_stem, dpy_name_add);

	/* Verify that all files are either non-symlinks owned by the current
	 * user or non-existing. If not: use FVWM_USERDIR as base instead. */

	type_pos = strlen(f_stem);
	owner = geteuid();
	is_path_valid = True;

	f_stem[type_pos+1] = 0;

	for (c = file_suffix; *c != 0 && is_path_valid; c++)
	{
		int rc;

		f_stem[type_pos] = *c;
		if (DO_USE_LSTAT)
		{
			rc = fvwm_lstat(f_stem, &stat_buf);
		}
		else
		{
			rc = stat(f_stem, &stat_buf);
		}
		if (rc == 0)
		{
			/* stat successful */
			if (
				stat_buf.st_uid != owner ||
				stat_buf.st_nlink > 1 ||
				S_ISDIR(stat_buf.st_mode) ||
				FVWM_S_ISLNK(stat_buf.st_mode) ||
				(stat_buf.st_mode & FVWM_S_IFLNK) != 0)
			{
				is_path_valid = False;
			}
			break;
		}
		else if (errno != ENOENT)
		{
			is_path_valid = False;
		}
	}
	f_stem[type_pos] = 0;
	if (!is_path_valid)
	{
		char *userdir;
		char *tailname;
		char *tmp;
		tmp = f_stem;

		if (f_stem[1] == 't') /* /tmp/ */
		{
			tailname = f_stem + 4;
		}
		else /* /var/tmp/ */
		{
			tailname = f_stem + 8;
		}

		userdir = getenv("FVWM_USERDIR");
		if (userdir == NULL)
		{
			free(tmp);
			return NULL;
		}
		f_stem = safemalloc(strlen(userdir) + strlen(tailname) + 1);
		strcpy(f_stem, userdir);
		strcat(f_stem, tailname);
		free(tmp);
	}

	return f_stem;
}
