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

#ifndef _BINDINGS_
#define _BINDINGS_

int ParseBinding(
  Display *dpy, Binding **pblist, char *tline, BindingType type,
  int *nr_left_buttons, int *nr_right_buttons, unsigned char *buttons_grabbed,
  Bool do_ungrab_root);
void key_binding(F_CMD_ARGS);
void pointerkey_binding(F_CMD_ARGS);
void mouse_binding(F_CMD_ARGS);
STROKE_CODE(void stroke_binding(F_CMD_ARGS));
unsigned int MaskUsedModifiers(unsigned int in_modifiers);
unsigned int GetUnusedModifiers(void);
void ignore_modifiers(F_CMD_ARGS);

#endif /* _BINDINGS_ */
