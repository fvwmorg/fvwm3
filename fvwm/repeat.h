/* Copyright (C) 1999  Dominik Vogt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef _REPEAT_
#define _REPEAT_

typedef enum
{
  REPEAT_NONE = 0,
  REPEAT_COMMAND,
/* I think we don't need all these
  REPEAT_BUILTIN,
  REPEAT_FUNCTION,
  REPEAT_TOP_FUNCTION,
  REPEAT_MODULE,
*/
  REPEAT_MENU,
  REPEAT_POPUP,
  REPEAT_PAGE,
  REPEAT_DESK,
  REPEAT_DESK_AND_PAGE,
  REPEAT_FVWM_WINDOW
} repeat_type;

extern char *repeat_last_function;
extern char *repeat_last_complex_function;
extern char *repeat_last_builtin_function;
extern char *repeat_last_module;
/*
extern char *repeat_last_top_function;
extern char *repeat_last_menu;
extern FvwmWindow *repeat_last_fvwm_window;
*/

Bool set_repeat_data(void *data, repeat_type type, const func_type *builtin);

#endif /* _REPEAT_ */
