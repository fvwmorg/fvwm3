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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ---------------------------- included header files ----------------------- */
#include "config.h"

#include "FGettext.h"
#include "locale.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

int FGettextInitOk = 0;
char *FGDefaultDir = NULL;
char *FGDefaultDomain = NULL;
char *FGPath = NULL;

/* ---------------------------- interface functions ------------------------- */

void FGettextInit(const char *domain, const char *dir, const char *module)
{
	if (!HaveNLSSupport)
	{
		return;
	}
	setlocale (LC_MESSAGES, "");

	FGDefaultDir = bindtextdomain (domain, dir);
	FGDefaultDomain = textdomain (domain);
	FGettextInitOk = 1;
}

const char *FGettext(char *str)
{
	if (!HaveNLSSupport || !FGettextInitOk)
	{
		return str;
	}
	return gettext(str);
}


void FGettextSetLocalePath(char *path)
{

}
