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

#include "config.h"

#include <X11/Xlocale.h>

#ifdef I18N_MB

/* 
 * i18n X initialization 
 * category: the usual category LC_CTYPE, LC_CTIME, ...
 * modifier: "" or NULL if NULL XSetLocaleModifiers is not call
 * module: the name of the fvwm module that call the function for reporting
 * errors message
 *
 */
void FInitLocale(
		int category,
		const char *local,
		const char *modifier,
		const char *module
		);

#else /* !I18N_MB */

#define FInitLocale(x,y,z,t)

#endif /* I18N_MB */

/* 
 * Initialize the locale for !I18N_MB and get the user Charset for iconv
 * conversion.
 * module: should be the name of the fvwm module that call the function
 * for error messages.
 * This is probably not appropriate from an X point of view
 *
 */
void FInitCharset(const char *module);
