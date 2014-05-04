/* -*-c-*- */
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

/*
 * System.c: code for dealing with various OS system call variants
 */

#include "config.h"
#include "fvwmlib.h"
#include "envvar.h"
#include "System.h"
#include "Strings.h"

#if HAVE_UNAME
#include <sys/utsname.h>
#endif
#include "libs/fvwm_sys_stat.h"

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif


/*
 * just in case...
 */
#ifndef FD_SETSIZE
#define FD_SETSIZE 2048
#endif


fd_set_size_t fvwmlib_max_fd = (fd_set_size_t)-9999999;

fd_set_size_t GetFdWidth(void)
{
#if HAVE_SYSCONF
	return min(sysconf(_SC_OPEN_MAX),FD_SETSIZE);
#else
	return min(getdtablesize(),FD_SETSIZE);
#endif
}

void fvwmlib_init_max_fd(void)
{
	fvwmlib_max_fd = GetFdWidth();

	return;
}


/* return a string indicating the OS type (i.e. "Linux", "SINIX-D", ... ) */
int getostype(char *buf, int max)
{
#if HAVE_UNAME
	struct utsname sysname;

	if (uname( &sysname ) >= 0)
	{
		buf[0] = '\0';
		strncat(buf, sysname.sysname, max);
		return 0;
	}
#endif
	strcpy (buf, "");
	return -1;
}


/*
 * Set a colon-separated path, with environment variable expansions,
 * and expand '+' to be the value of the previous path.
 */
void setPath(char **p_path, const char *newpath, int free_old_path)
{
	char *oldpath = *p_path;
	int oldlen = strlen(oldpath);
	char *stripped_path = stripcpy(newpath);
	int found_plus = strchr(newpath, '+') != NULL;

	/* Leave room for the old path, if we find a '+' in newpath */
	*p_path = envDupExpand(stripped_path, found_plus ? oldlen - 1 : 0);
	free(stripped_path);

	if (found_plus)
	{
		char *p = strchr(*p_path, '+');
		memmove(p + oldlen, p + 1, strlen(p + 1) + 1);
		memmove(p, oldpath, oldlen);
	}

	if (free_old_path)
	{
		free(oldpath);
	}
}


/*
 *
 * Find the specified file somewhere along the given path.
 *
 * There is a possible race condition here:  We check the file and later
 * do something with it.  By then, the file might not be accessible.
 * Oh well.
 *
 */
#include <stdio.h>
char *searchPath(
	const char *pathlist, const char *filename, const char *suffix,
	int type)
{
	char *path;
	int filename_len;
	int maxpath_len;

	if (filename == NULL || *filename == 0)
	{
		return NULL;
	}

	if (pathlist == NULL || *pathlist == 0)
	{
		/* use pwd if no path list is given */
		pathlist = ".";
	}

	filename_len = strlen(filename);
	maxpath_len = (pathlist) ? strlen(pathlist) : 0;
	maxpath_len += (suffix) ? strlen(suffix) : 0;

	/* +1 for extra / and +1 for null termination */
	path = xmalloc(maxpath_len + filename_len + 2);
	*path = '\0';

	if (*filename == '/')
	{
		/* No search if filename begins with a slash */
		strcpy(path, filename);

		/* test if the path is accessable -- the module code assumes
		 * this is done */
		if (access(filename, type) == 0)
		{
			return path;
		}

		/* the file is not accessable (don't test suffixes with full
		 * path), return NULL */
		free(path);

		return NULL;
	}

	/* Search each element of the pathlist for the file */
	while (pathlist && *pathlist)
	{
		char *path_end = strchr(pathlist, ':');
		char *curr_end;

		if (path_end != NULL)
		{
			strncpy(path, pathlist, path_end - pathlist);
			path[path_end - pathlist] = '\0';
		}
		else
		{
			strcpy(path, pathlist);
		}

		/* handle specially the path extention using semicolon */
		curr_end = strchr(path, ';');
		if (curr_end != NULL)
		{
			char *dot = strrchr(filename, '.');
			int filebase_len;
			/* count a leading nil in newext_len too */
			int newext_len = path + strlen(path) - curr_end;
			if (dot != NULL)
			{
				filebase_len = dot - filename;
			}
			else
			{
				filebase_len = filename_len;
			}
			*(curr_end++) = '/';
			memmove(curr_end + filebase_len, curr_end, newext_len);
			strncpy(curr_end, filename, filebase_len);
		}
		else
		{
			strcat(path, "/");
			strcat(path, filename);
		}

		if (access(path, type) == 0)
		{
			return path;
		}

		if (suffix && *suffix != '\0') {
			strcat(path, suffix);
			if (access(path, type) == 0)
			{
				return path;
			}
		}

		/* Point to next element of the path */
		if (path_end == NULL)
		{
			break;
		}
		else
		{
			pathlist = path_end + 1;
		}
	}

	/* Hmm, couldn't find the file.  Return NULL */
	free(path);
	return NULL;
}

/*
 * void setFileStamp(FileStamp *stamp, const char *name);
 * Bool isFileStampChanged(const FileStamp *stamp, const char *name);
 *
 * An interface for verifying cached files.
 * The first function associates a file stamp with file (named by name).
 * The second function returns True or False in case the file was changed
 * from the time the stamp was associated.
 *
 * FileStamp can be a structure; try to save memory by evaluating a checksum.
 */

FileStamp getFileStamp(const char *name)
{
	static struct stat buf;

	if (!name || stat(name, &buf))
	{
		return 0;
	}
	return ((FileStamp)buf.st_mtime << 13) + (FileStamp)buf.st_size;
}

void setFileStamp(FileStamp *stamp, const char *name)
{
	*stamp = getFileStamp(name);
}

Bool isFileStampChanged(const FileStamp *stamp, const char *name)
{
	return *stamp != getFileStamp(name);
}

#ifdef HAVE_SAFETY_MKSTEMP
int fvwm_mkstemp (char *TEMPLATE)
{
	return mkstemp(TEMPLATE);
}
#else

/* This function is an adaptation of the mkstemp() function included
 * in the Gnu C Library, version 2.2.2. The Gnu C library, in turn,
 * adapted the function from OpenBSD.
 * Michael Han <mikehan@mikehan.com> */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "ftime.h"

#define __set_errno(val) errno = (val)

/* These are the characters used in temporary filenames.  */
static const char letters[] =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

/* Generate a unique temporary file name from TEMPLATE.
   The last six characters of TEMPLATE must be "XXXXXX";
   they are replaced with a string that makes the filename unique.
   Then open the file and return a fd. */

int fvwm_mkstemp (char *template)
{
	int len;
	char *XXXXXX;
	static unsigned long value;
	struct timeval tv;
	int count, fd = -1;
	int save_errno = errno;

	len = strlen (template);
	if (len < 6 || strcmp (&template[len - 6], "XXXXXX"))
	{
		__set_errno (EINVAL);
		return -1;
	}

	/* This is where the Xs start.  */
	XXXXXX = &template[len - 6];

	/* Get some more or less random data.  */
	gettimeofday (&tv, NULL);
	value = ((unsigned long) tv.tv_usec << 16) ^ tv.tv_sec ^ getpid ();

	for (count = 0; count < TMP_MAX; value = 7777, count)
	{
		unsigned long v = value;

		/* Fill in the random bits.  */
		XXXXXX[0] = letters[v % 62];
		v /= 62;
		XXXXXX[1] = letters[v % 62];
		v /= 62;
		XXXXXX[2] = letters[v % 62];
		v /= 62;
		XXXXXX[3] = letters[v % 62];
		v /= 62;
		XXXXXX[4] = letters[v % 62];
		v /= 62;
		XXXXXX[5] = letters[v % 62];

		fd = open(
			template, O_RDWR | O_CREAT | O_EXCL,
			FVWM_S_IRUSR | FVWM_S_IWUSR);
		if (fd >= 0)
		{
			__set_errno (save_errno);
			return fd;
		}
		else if (errno != EEXIST)
		{
			return -1;
		}
	}

	/* We got out of the loop because we ran out of combinations to try.  */
	__set_errno (EEXIST);
	return -1;
}

#endif /* HAVE_SAFETY_MKSTEMP */
