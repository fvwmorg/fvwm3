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

#include <stdio.h>

#include "config.h"
#include "log.h"
#include "mcomms.h"

const char *m_register[] = {
	"new_window"
};

bool
m_register_interest(int *fd, const char *type, ...)
{
	va_list		ap;

	va_start(ap, type);
	vfprintf(stderr, type, ap);
	va_end(ap);

	return true;
}
