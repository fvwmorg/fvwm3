/* Author: Dietmar Maurer <dm@vlsivie.tuwien.ac.at> */
/* Description:

   This widget works like a normal menu_item, but you can insert a
   arbitrary widget (most often a pixmap widget), which is displayed
   at the left side. The advantage is that indentation is handled the
   same way as GTK does.

   (i.e if you create a menu with a gtk_check_menu_item, all normal
   menu_items are automatically indented by GTK - so if you use a normal
   menu_item to display pixmaps at the left side, the pixmaps will be
   indented, which is not what you want. This widget solves the problem)

   */

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

#ifndef __GTK_MENU_PIXMAP_ITEM_H__
#define __GTK_MENU_PIXMAP_ITEM_H__


#include <gtk/gtkpixmap.h>
#include <gtk/gtkmenuitem.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_PIXMAP_MENU_ITEM           (gtk_pixmap_menu_item_get_type ())
#define GTK_PIXMAP_MENU_ITEM(obj)           (GTK_CHECK_CAST ((obj), GTK_TYPE_PIXMAP_MENU_ITEM, GtkPixmapMenuItem))
#define GTK_PIXMAP_MENU_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_PIXMAP_MENU_ITEM, GtkPixmapMenuItemClass))
#define GTK_IS_PIXMAP_MENU_ITEM(obj)        (GTK_CHECK_TYPE ((obj), GTK_TYPE_PIXMAP_MENU_ITEM))
#define GTK_IS_PIXMAP_MENU_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_PIXMAP_MENU_ITEM))


typedef struct _GtkPixmapMenuItem       GtkPixmapMenuItem;
typedef struct _GtkPixmapMenuItemClass  GtkPixmapMenuItemClass;

struct _GtkPixmapMenuItem
{
  GtkMenuItem menu_item;

  GtkWidget *pixmap;
};

struct _GtkPixmapMenuItemClass
{
  GtkMenuItemClass parent_class;
};


GtkType    gtk_pixmap_menu_item_get_type      (void);
GtkWidget* gtk_pixmap_menu_item_new           (void);
void       gtk_pixmap_menu_item_set_pixmap    (GtkPixmapMenuItem *menu_item,
					       GtkWidget *pixmap);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_PIXMAP_MENU_ITEM_H__ */
