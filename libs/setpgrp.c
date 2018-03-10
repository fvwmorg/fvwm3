/* -*-c-*- */
/* Copyright (C) 2002  Dominik Vogt */
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
** setpgrp.c:
** Provides a portable replacement for setpgrp
*/

#include "config.h"
#include <unistd.h>
#include "setpgrp.h"

int fvwm_setpgrp(void)
{
	int rc;

#ifdef HAVE_SETPGID
	rc = setpgid(0, 0);
#else

#  ifdef HAVE_SETPGRP
#    ifdef SETPGRP_VOID
	rc = setpgrp();
#    else
	rc = setpgrp(0, 0);
#    endif
#  else
	/* neither setpgrp nor setpgid - just do nothing */
	rc = 0;
#  endif
#endif

	return rc;
}
