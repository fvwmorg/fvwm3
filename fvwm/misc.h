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


/************************************************************************
 * ReapChildren - wait() for all dead child processes
 ************************************************************************/

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

/* Start of function prototype area. */

void ReapChildren(void);

/* bits for GrabPointerState */
#define GRAB_NONE 0           /* the cursor is not grabed */
#define GRAB_NORMAL (1<<0)    /* DeferExecution, Move, Resize, ... */
#define GRAB_BUSY (1<<1)      /* BusyCursor stuff */
#define GRAB_MENU (1<<2)      /* a menus.c grabing */
#define GRAB_BUSYMENU (1<<3)  /* Allows menus.c to regrab the cursor */
#define GRAB_STARTUP (1<<4)   /* Startup busy cursor */

Bool GrabEm(int cursor, int grab_context);
void UngrabEm(int ungrab_context);

void WaitForButtonsUp(Bool do_handle_expose);
int GetTwoArguments(char *action, int *val1, int *val2, int *val1_unit,
		    int *val2_unit);
void NewFontAndColor(Font newfont, Pixel color, Pixel backcolor);
void Keyboard_shortcuts(XEvent *, FvwmWindow*, int);
Bool check_if_fvwm_window_exists(FvwmWindow *fw);
int truncate_to_multiple (int x, int m);
Bool move_into_rectangle(rectangle *move_rec, rectangle *target_rec);
Bool IsRectangleOnThisPage(rectangle *rec, int desk);
Bool IntersectsInterval(int x1, int width1, int x2, int width2);

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

#endif /* MISC_H */
