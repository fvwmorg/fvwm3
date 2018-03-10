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

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "fio.h"

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- interface functions ------------------------ */

ssize_t fvwm_send(int s, const void *buf, size_t len, int flags)
{
	int rc;
	size_t offset;
	const char *data;

	data = buf;
	offset = 0;
	do
	{
		rc = send(s, (char *)data + offset, len - offset, flags);
		if (rc > 0)
		{
			offset += rc;
		}
	} while ((rc > 0 && (offset < len)) || (rc == -1 && errno == EINTR));

	return rc;
}

ssize_t fvwm_recv(int s, void *buf, size_t len, int flags)
{
	int rc;

	do
	{
		rc = recv(s, buf, len, flags);
	} while (rc == -1 && errno == EINTR);

	return rc;
}
