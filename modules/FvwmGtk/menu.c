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

#include <stdio.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkprivate.h>
#include "gtkpixmapmenuitem.h"
#ifdef GDK_IMLIB
#include <gdk_imlib.h>
#endif

#include "libs/fvwmlib.h"
#include "libs/Module.h"
#include "libs/InitPicture.h"


extern int fvwm_fd[2];
extern char *image_path;
extern GHashTable *widgets;
extern GtkWidget *current;
extern GtkWidget *attach_to_toplevel;
extern unsigned long context;
extern int icon_w, icon_h;


static void
send_item (GtkWidget *item, char *s)
{
  SendText (fvwm_fd, s, context);
}


static void
detacher (GtkWidget *attach, GtkMenu *menu)
{
}


GtkWidget*
find_or_create_menu (char *name)
{
  GtkWidget *menu;
  char *name_copy;

  menu = g_hash_table_lookup (widgets, name);

  if (menu == NULL)
    {
      menu = gtk_menu_new ();
      gtk_menu_attach_to_widget (GTK_MENU (menu),
				 attach_to_toplevel, detacher);
      gtk_widget_set_name (menu, name);
      name_copy = gtk_widget_get_name (menu);
      gtk_object_ref (GTK_OBJECT (menu));
      gtk_object_sink (GTK_OBJECT (menu));
      g_hash_table_insert (widgets, name_copy, menu);
      gtk_object_ref (GTK_OBJECT (menu));
    }
  if (GTK_IS_MENU (menu))
    {
      return menu;
    }
  else
    {
      fprintf (stderr, "%s is not a menu\n", name);
      return NULL;
    }
}


/* from gtkcauldron.c */
/* result must be g_free'd */
static gchar *
convert_label_with_ampersand (const gchar * _label,
			      gint * accelerator_key,
			      gint * underbar_pos)
{
    gchar *p;
    gchar *label = g_strdup (_label);
    for (p = label;; p++) {
	if (!*p)
	    break;
	if (!p[1])
	    break;
	if (*p == '&') {
	    memmove (p, p + 1, strlen (p));
	    if (*p == '&')	/* use && for an actual & */
		continue;
	    *underbar_pos = (unsigned long) p - (unsigned long) label;
	    *accelerator_key = *p;
	    return label;
	}
    }
    return label;
}


/* from gtkcauldron.c */
static gchar *
create_label_pattern (gchar * label, gint underbar_pos)
{
    gchar *pattern;
    pattern = g_strdup (label);
    memset (pattern, ' ', strlen (label));
    if (underbar_pos < strlen (pattern) && underbar_pos >= 0)
	pattern[underbar_pos] = '_';
    return pattern;
}


/* from gnome-app-helper.c */
static GtkAccelGroup *
get_menu_accel_group (GtkMenuShell *menu_shell)
{
	GtkAccelGroup *ag;

	ag = gtk_object_get_data (GTK_OBJECT (menu_shell),
			"gnome_menu_accel_group");

	if (!ag) {
		ag = gtk_accel_group_new ();
		gtk_accel_group_attach (ag, GTK_OBJECT (menu_shell));
		gtk_object_set_data (GTK_OBJECT (menu_shell),
				"gnome_menu_accel_group", ag);
	}

	return ag;
}


GtkWidget *
menu_item_new_with_pixmap_and_label (char *file, char *l_label, char *r_label)
{
  GtkWidget *item, *label, *icon, *box;
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;
  char *path;
  gint accel_key = 0, underbar_pos = -1;
  gchar *pattern, *converted_label;
#ifdef GDK_IMLIB
  GdkImlibImage *im;
  int w, h;
#endif

  path = findImageFile (file ? file : "", image_path, R_OK);

  if (path == NULL)
    {
      item = gtk_menu_item_new ();
    }
  else
    {
#ifdef GDK_IMLIB
      im = gdk_imlib_load_image (path);
      if (im)
	{
	  if (icon_w == 0 || icon_h == 0)
	    {
	      w = im->rgb_width;
	      h = im->rgb_height;
	    }
	  else
	    {
	      w = icon_w;
	      h = icon_h;
	    }
	  gdk_imlib_render (im, w, h);
	  pixmap = gdk_imlib_move_image (im);
	  mask = gdk_imlib_move_mask (im);
	}
#else
      pixmap = gdk_pixmap_create_from_xpm (NULL, &mask, NULL, path);
#endif
      if (pixmap)
	{
	  item = gtk_pixmap_menu_item_new ();
	  icon = gtk_pixmap_new (pixmap, mask);
	  gtk_pixmap_menu_item_set_pixmap (GTK_PIXMAP_MENU_ITEM (item), icon);
	  gtk_widget_show (icon);
	}
      else
	{
	  item = gtk_menu_item_new ();
	}

      free (path);
    }

  converted_label = convert_label_with_ampersand (l_label, &accel_key,
						  &underbar_pos);
  label = gtk_label_new (converted_label);
  if (underbar_pos != -1)
    {
      GtkAccelGroup *accel_group =
	get_menu_accel_group (GTK_MENU_SHELL (current));

      pattern = create_label_pattern (converted_label, underbar_pos);
      gtk_label_set_pattern (GTK_LABEL (label), pattern);

      gtk_widget_add_accelerator (item, "activate_item",
				  accel_group, accel_key, 0, 0);
    }
  g_free (converted_label);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  if (r_label)
    {
      box = gtk_hbox_new (0, 0);
      gtk_box_pack_start (GTK_BOX (box), label, 0, 1, 0);
      label = gtk_label_new (r_label);
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_box_pack_end (GTK_BOX (box), label, 0, 1, 0);
      gtk_container_add (GTK_CONTAINER (item), box);
      gtk_widget_show_all (box);
    }
  else
    {
      gtk_container_add (GTK_CONTAINER (item), label);
      gtk_widget_show (label);
    }

  return item;
}


void
open_menu (int argc, char **argv)
{
  g_return_if_fail (argc > 0);

  current = find_or_create_menu (argv[0]);
}


void
menu_title (int argc, char **argv)
{
  GtkWidget *item;

  g_return_if_fail (argc > 0);

  item = menu_item_new_with_pixmap_and_label
    (argc > 1 ? argv[1] : NULL, argv[0], argc > 2 ? argv[2] : NULL);
  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
  /*
     This is a hack stolen from the Gnome panel. There ought
     to be a better way.
  */
  gtk_signal_connect
    (GTK_OBJECT (item), "select",
     GTK_SIGNAL_FUNC (gtk_menu_item_deselect), NULL);
}


void
menu_separator (int argc, char **argv)
{
  GtkWidget *item;

  item = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
  gtk_widget_set_sensitive (item, 0);
}


void
menu_item (int argc, char **argv)
{
  GtkWidget *item;

  g_return_if_fail (argc > 1);

  item = menu_item_new_with_pixmap_and_label
    (argc > 2 ? argv[2] : NULL, argv[0], argc > 3 ? argv[3] : NULL);
  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
  gtk_signal_connect
    (GTK_OBJECT (item), "activate",
     GTK_SIGNAL_FUNC (send_item), safestrdup (argv[1]));
}


void
menu_tearoff_item (int argc, char **argv)
{
  GtkWidget *item;

  item = gtk_tearoff_menu_item_new ();

  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
}


void
menu_submenu (int argc, char **argv)
{
  GtkWidget *item, *submenu;

  item = menu_item_new_with_pixmap_and_label
    (argc > 2 ? argv[2] : NULL, argv[0], NULL);
  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
  submenu = find_or_create_menu (argv[1]);

  gtk_menu_detach (GTK_MENU (submenu));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
}


