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

#ifndef MISC_H
#define MISC_H

/* MB stuff: rename XmbTextEscapement() and XFreeFont() */
#ifdef I18N_MB
#ifdef __STDC__
#define XTextWidth(x,y,z)	XmbTextEscapement(x ## set,y,z)
#define XFreeFont(x,y)		XFreeFontSet(x,y ## set)
#else
#define XTextWidth(x,y,z)	XmbTextEscapement(x/**/set,y,z)
#define XFreeFont(x,y)		XFreeFontSet(x,y/**/set)
#endif
#endif

#define GRAB_ALL      0       /* sum of all grabs */
#define GRAB_STARTUP  1       /* Startup busy cursor */
#define GRAB_NORMAL   2       /* DeferExecution, Move, Resize, ... */
#define GRAB_MENU     3       /* a menus.c grabing */
#define GRAB_BUSY     4       /* BusyCursor stuff */
#define GRAB_BUSYMENU 5       /* Allows menus.c to regrab the cursor */
#define GRAB_PASSIVE  6       /* Override of passive grab, only prevents grab
			       * to be released too early */
#define GRAB_MAXVAL   7       /* last GRAB macro + 1 */


/* Start of function prototype area. */

Bool GrabEm(int cursor, int grab_context);
Bool UngrabEm(int ungrab_context);

int GetTwoArguments(char *action, int *val1, int *val2, int *val1_unit,
		    int *val2_unit);
void NewFontAndColor(Font newfont, Pixel color, Pixel backcolor);
void Keyboard_shortcuts(
  XEvent *Event, FvwmWindow *fw, int *x_defect, int *y_defect, int ReturnEvent);
Bool check_if_fvwm_window_exists(FvwmWindow *fw);
int truncate_to_multiple (int x, int m);
Bool move_into_rectangle(rectangle *move_rec, rectangle *target_rec);
Bool IsRectangleOnThisPage(rectangle *rec, int desk);
Bool IntersectsInterval(int x1, int width1, int x2, int width2);
Bool intersect_xrectangles(XRectangle *r1, XRectangle *r2);

/*
** message levels for fvwm_msg:
*/
typedef enum
{
  DBG = 0,
  ECHO,
  INFO,
  WARN,
  ERR
} fvwm_msg_type;
void fvwm_msg(fvwm_msg_type type, char *id, char *msg, ...);

/* needed in misc.h */
typedef enum {
  ADDED_NONE = 0,
  ADDED_MENU,
#ifdef USEDECOR
  ADDED_DECOR,
#endif
  ADDED_FUNCTION
} last_added_item_type;
void set_last_added_item(last_added_item_type type, void *item);

#ifdef ICON_DEBUG
#define ICON_DBG(X) \
  fprintf X;
#else
#define ICON_DBG(X)
#endif

#endif /* MISC_H */
