/* -*-c-*- */
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
