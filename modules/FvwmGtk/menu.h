/* This program is free software; you can redistribute it and/or modify
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

#ifndef _MENU_H
#define _MENU_H

GtkWidget *menu_item_new_with_pixmap_and_label (char *file,
						char *l_label,
						char *r_label);
GtkWidget *find_or_create_menu (char *name);
void open_menu (int argc, char **argv);
void menu_title (int argc, char **argv);
void menu_separator (int argc, char **argv);
void menu_item (int argc, char **argv);
void menu_tearoff_item (int argc, char **argv);
void menu_submenu (int argc, char **argv);


#endif /* _MENU_H */
