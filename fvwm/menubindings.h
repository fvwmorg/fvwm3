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

#ifndef _MENU_BINDINGS_
#define _MENU_BINDINGS_

int menu_binding(
	Display *dpy, binding_t type, int button, KeySym keysym,
	int context, int modifier, char *action, char *menuStyle);
Binding *menu_binding_is_mouse(XEvent* event, int context);
Binding *menu_binding_is_key(XEvent* event, int context);
void menu_bindings_startup_complete(void);

#endif /* _MENU_BINDINGS_ */
