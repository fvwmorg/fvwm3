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

#ifndef _DEBUG_
#define _DEBUG_

#ifdef DEBUG
void DB_WI_WINDOWS(char *label, FvwmWindow *fw);
void DB_WI_SUBWINS(char *label, FvwmWindow *fw);
void DB_WI_FRAMEWINS(char *label, FvwmWindow *fw);
void DB_WI_BUTTONWINS(char *label, FvwmWindow *fw);
void DB_WI_FRAME(char *label, FvwmWindow *fw);
void DB_WI_ICON(char *label, FvwmWindow *fw);
void DB_WI_SIZEHINTS(char *label, FvwmWindow *fw);
void DB_WI_TITLE(char *label, FvwmWindow *fw);
void DB_WI_BORDER(char *label, FvwmWindow *fw);
void DB_WI_XWINATTR(char *label, FvwmWindow *fw);
void DB_WI_ALL(char *label, FvwmWindow *fw);
#else
#define DB_WI_WINDOWS(x,y)
#define DB_WI_SUBWINS(x,y)
#define DB_WI_FRAMEWINS(x,y)
#define DB_WI_BUTTONWINS(x,y)
#define DB_WI_FRAME(x,y)
#define DB_WI_ICON(x,y)
#define DB_WI_SIZEHINTS(x,y)
#define DB_WI_TITLE(x,y)
#define DB_WI_BORDER(x,y)
#define DB_WI_XWINATTR(x,y)
#define DB_WI_ALL(x,y)
#endif

/* Set up heap debugging library dmalloc.  */

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#endif /* _DEBUG_ */
