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
#include <stdint.h>

#include "config.h"
#include "log.h"
#include "safemalloc.h"
#include "mcomms.h"

uint64_t m_find_bit(const char *);

static const struct {
	uint64_t	 bits;
	const char	*name;
} all_mcomm_types[] = {
	{ MCOMMS_NEW_WINDOW, "new-window" },
	{ 0, NULL },
};

uint64_t
m_find_bit(const char *name)
{
	int i = 0;

	for (i = 0; all_mcomm_types[i].name != NULL; i++) {
		if (strcmp(all_mcomm_types[i].name, name) == 0) {
			return all_mcomm_types[i].bits;
		}
	}
	fprintf(stderr, "%s: Invalid module interest: %s\n", __func__, name);
	return 0;
}

uint64_t
m_register_interest(int *fd, const char *me)
{
	char		*me1;
	const char	*component;
	uint64_t	 m_bits = 0;

	if (me == NULL)
		return 0;
	me1 = fxstrdup(me);

	while ((component = strsep(&me1, " ")) != NULL)
		m_bits |= m_find_bit(component);

	free(me1);
	return m_bits;
}
