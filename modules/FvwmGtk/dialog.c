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
#include <gdk/gdkprivate.h>

#include "libs/fvwmlib.h"
#include "libs/Module.h"

#include "menu.h"
#include "expand.h"

extern int fvwm_fd[2];
extern GHashTable *widgets;
extern GtkWidget *current;
extern unsigned long context;

GSList *radio_group = NULL;
char *group_name = NULL;
GtkWidget *menu = NULL;
int option_menu_items = 0;
int option_menu_active_item = 0;
char *notebook_label = NULL;


gpointer
widget_get_value (GtkWidget *w)
{
  static char buf[100];

  if (GTK_IS_OPTION_MENU (w))
    {
      GtkWidget *menu = GTK_OPTION_MENU (w)->menu;
      GtkWidget *item;

      if (menu == NULL)
	return NULL;
      item = gtk_menu_get_active (GTK_MENU (menu));
      if (item)
	{
	  return gtk_object_get_data (GTK_OBJECT (item), "value");
	}
      else
	{
	  return NULL;
	}
    }
  else if (GTK_IS_SCALE (w))
    {
      g_snprintf (buf, sizeof (buf), "%.*f",
		  GTK_RANGE (w)->digits,
		  gtk_range_get_adjustment (GTK_RANGE (w))->value);
      return buf;
    }
  else if (GTK_IS_COLOR_SELECTION (w))
    {
      double rgb[3];
      unsigned long red, green, blue;

      gtk_color_selection_get_color (GTK_COLOR_SELECTION (w), rgb);
      red   = rgb[0] * 65536;
      if (red >= 65536)
	{
	  red = 65535;
	}
      green = rgb[1] * 65536;
      if (green >= 65536)
	{
	  green = 65535;
	}
      blue  = rgb[2] * 65536;
      if (blue >= 65536)
	{
	  blue = 65535;
	}
      g_snprintf (buf, sizeof (buf), "rgb:%.4lx/%.4lx/%.4lx",
		red, green, blue);
      return buf;
    }
  else if (GTK_IS_ENTRY (w))
    {
      /* this also catches spin buttons */
      return gtk_entry_get_text (GTK_ENTRY (w));
    }
  else if (GTK_IS_TOGGLE_BUTTON (w))
    {
      if (GTK_TOGGLE_BUTTON (w)->active)
	{
	  return gtk_object_get_data (GTK_OBJECT (w), "on-value");
	}
      else
	{
	  return gtk_object_get_data (GTK_OBJECT (w), "off-value");
	}
    }

  return NULL;
}


/* generic callback added to all data widgets */
static void
update_value (GtkWidget *w)
{
  char *val;

  val = widget_get_value (w);
  if (val)
    {
      gtk_object_set_data_full (GTK_OBJECT (gtk_widget_get_toplevel (w)),
	gtk_widget_get_name (w), safestrdup (val), free);
    }
}


static void
add_to_dialog (GtkWidget *w, int argc, char **argv)
{
  int i;
  gboolean expand = FALSE;
  gboolean fill = FALSE;
  gboolean focus = FALSE;
  gboolean def = FALSE;
  gboolean can_def = FALSE;
  unsigned int padding = 0;
  GtkWidget *dialog;

  dialog = gtk_widget_get_toplevel (w);
  for (i = argc - 1; i >= 0; i--)
    {
      if (strcmp (argv[i], "--") == 0)
	{
	  for (i++; i < argc; i++)
	    {
	      if (strcasecmp (argv[i], "default") == 0)
		{
		  can_def = TRUE;
		  def = TRUE;
		}
	      else if (strcasecmp (argv[i], "can-default") == 0)
		{
		  can_def = TRUE;
		}
	      else if (strcasecmp (argv[i], "focus") == 0)
		{
		  focus = TRUE;
		}
	      else if (strcasecmp (argv[i], "expand") == 0)
		{
		  expand = TRUE;
		}
	      else if (strcasecmp (argv[i], "fill") == 0)
		{
		  fill = TRUE;
		}
	      else
		{
		  padding = atoi (argv[i]);
		}
	    }
	  break;
	}
    }
  if (GTK_IS_NOTEBOOK (current))
    {
      gtk_notebook_append_page (GTK_NOTEBOOK (current), w,
				gtk_label_new (notebook_label));
    }
  else if (GTK_IS_BOX (current))
    {
      gtk_box_pack_start (GTK_BOX (current), w, expand, fill, padding);
    }
  else
    {
      gtk_container_add (GTK_CONTAINER (current), w);
    }
  if (focus)
    {
      gtk_widget_grab_focus (w);
    }
  if (can_def)
    {
      GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    }
  if (def)
    {
      gtk_widget_grab_default (w);
    }
  update_value (w);
}


void
send_values (GtkWidget *w)
{
  GtkWidget *dialog;
  char **vals;

  dialog = gtk_widget_get_toplevel (w);
  for (vals = (char **) gtk_object_get_data (GTK_OBJECT (w), "return_values");
       *vals; vals++)
    {
      char *val = recursive_replace (dialog, *vals);
      if (strcasecmp (val, "close") == 0)
	{
	  gtk_widget_hide (dialog);
	}
      else
	{
	  if (val[0] == '!')
	    {
	      system (val + 1);
	    }
	  else
	    {
	      SendText (fvwm_fd, val, context);
	    }
	}
      free (val);
    }
}

void
open_dialog (int argc, char **argv)
{
  GtkWidget *item;
  char *name;

  g_return_if_fail (argc >= 2);

  item = g_hash_table_lookup (widgets, argv[0]);
  if (item)
    {
      name = gtk_widget_get_name (item);
    }
  else
    {
      item = gtk_window_new (GTK_WINDOW_DIALOG);
      gtk_widget_set_name (item, argv[0]);
      name = gtk_widget_get_name (item);
      g_hash_table_insert (widgets, name, item);
      gtk_object_ref (GTK_OBJECT (item));

      gtk_window_set_title (GTK_WINDOW (item), argv[1]);
      gtk_window_set_wmclass (GTK_WINDOW (item), argv[1], "FvwmGtk");
      gtk_object_set_data
	(GTK_OBJECT (item), "values",
	 g_hash_table_new (g_str_hash, g_str_equal));
      gtk_signal_connect
	(GTK_OBJECT (item), "delete_event",
	 GTK_SIGNAL_FUNC (gtk_widget_hide), current);
      if (argc >= 3 && strcasecmp ("center", argv[2]) == 0)
	{
	  gtk_window_position (GTK_WINDOW (item), GTK_WIN_POS_CENTER);
	}
      else
	{
	  gtk_window_position (GTK_WINDOW (item), GTK_WIN_POS_MOUSE);
	}
      if (argc >= 4)
	{
	  int border_width;

	  border_width = atoi (argv[3]);

	  gtk_container_border_width (GTK_CONTAINER (item), border_width);
	}
    }
  if (GTK_IS_MENU (item))
    {
      fprintf (stderr, "%s is a menu\n", name);
    }
  current = item;
}


/* this applies to rows, columns and frames */
void
dialog_end_something (int argc, char **argv)
{
  g_return_if_fail (current != NULL);
  current = GTK_WIDGET (current)->parent;
}


void
dialog_label (int argc, char **argv)
{
  GtkWidget *item;

  g_return_if_fail (argc >= 1);

  item = gtk_label_new (argv[0]);
  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);
}


void
dialog_notebook (int argc, char **argv)
{
  g_return_if_fail (argc >= 1);

  if (!GTK_IS_NOTEBOOK (current))
    {
      GtkWidget *nb = gtk_notebook_new ();
      gtk_widget_show (nb);
      add_to_dialog (nb, argc, argv);
      current = nb;
    }
  if (notebook_label)
    {
      free (notebook_label);
      notebook_label = NULL;
    }
  notebook_label = safestrdup (argv[0]);
}


void
dialog_end_notebook (int argc, char **argv)
{
  g_return_if_fail (current != NULL);

  if (notebook_label)
    {
      free (notebook_label);
      notebook_label = NULL;
    }
  current = GTK_WIDGET (current)->parent;
}


void
dialog_start_frame (int argc, char **argv)
{
  GtkWidget *item;

  g_return_if_fail (argc >= 1);

  item = gtk_frame_new (argv[0]);
  if (argc >= 2)
    {
      int border_width;

      border_width = atoi (argv[1]);

      gtk_container_border_width (GTK_CONTAINER (item), border_width);
    }
  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);
  current = item;
}


void
dialog_button (int argc, char **argv)
{
  GtkWidget *item;
  char **vals;
  int i;

  g_return_if_fail (argc >= 2);

  item = gtk_button_new_with_label (argv[0]);

  vals = (char **) safemalloc (argc * sizeof (char *));
  for (i = 0; i < argc - 1; i++)
    {
      if (strcmp (argv[i+1], "--") == 0)
	{
	  break;
	}
      vals[i] = safestrdup(argv[i + 1]);
    }
  vals[i] = NULL;
  gtk_object_set_data (GTK_OBJECT (item), "return_values", vals);
  gtk_signal_connect
    (GTK_OBJECT (item), "clicked",
     GTK_SIGNAL_FUNC (send_values), item);
  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);
}


void
dialog_checkbutton (int argc, char **argv)
{
  GtkWidget *item;

  g_return_if_fail (argc >= 4);

  item = gtk_check_button_new_with_label (argv[1]);
  gtk_widget_set_name (item, argv[0]);
  gtk_object_set_data (GTK_OBJECT (item), "on-value", safestrdup (argv[2]));
  gtk_object_set_data (GTK_OBJECT (item), "off-value", safestrdup (argv[3]));
  if (argc >= 5 && strcasecmp (argv[4], "on") == 0)
    {
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (item), TRUE);
    }
  else
    {
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (item), FALSE);
    }
  gtk_signal_connect
    (GTK_OBJECT (item), "toggled",
     GTK_SIGNAL_FUNC (update_value), item);

  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);
}


void
dialog_entry (int argc, char **argv)
{
  GtkWidget *item;

  g_return_if_fail (argc >= 1);
  item = gtk_entry_new ();
  gtk_widget_set_name (item, argv[0]);
  if (argc >= 2 && strcmp(argv[1], "--") != 0)
    {
      gtk_entry_set_text (GTK_ENTRY (item), argv[1]);
    }
  gtk_signal_connect
    (GTK_OBJECT (item), "changed",
     GTK_SIGNAL_FUNC (update_value), item);
  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);
}


void
dialog_radiobutton (int argc, char **argv)
{
  GtkWidget *item;

  g_return_if_fail (argc >= 2);

  item = gtk_radio_button_new_with_label (radio_group, argv[0]);
  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (item));
  gtk_widget_set_name (item, group_name);
  gtk_object_set_data (GTK_OBJECT (item), "on-value", safestrdup (argv[1]));
  if (argc >= 3 && strcasecmp (argv[2], "on") == 0)
    {
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (item), TRUE);
    }
  else
    {
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (item), FALSE);
    }
  gtk_signal_connect
    (GTK_OBJECT (item), "toggled",
     GTK_SIGNAL_FUNC (update_value), item);

  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);
}


void
dialog_start_radiogroup (int argc, char **argv)
{
  g_return_if_fail (argc >= 1);

  if (group_name)
    {
      free (group_name);
    }
  group_name = safestrdup (argv[0]);
  radio_group = NULL;
}


void
dialog_end_radiogroup (int argc, char **argv)
{
  if (group_name)
    {
      free (group_name);
      group_name = NULL;
    }
  radio_group = NULL;
}


void
dialog_color (int argc, char **argv)
{
  GtkWidget *item;

  g_return_if_fail (argc >= 1);

  item = gtk_color_selection_new ();
  gtk_widget_set_name (item, argv[0]);
  if (argc >= 2)
    {
      XColor color;
      /* FIXME: get meaningful colormap */
      if (XParseColor (gdk_display,
		       DefaultColormap (gdk_display, gdk_screen),
		       argv[1],
		       &color))
	{
	  double rgb[3];
	  rgb[0] = (double) color.red   / 65536.0;
	  rgb[1] = (double) color.green / 65536.0;
	  rgb[2] = (double) color.blue  / 65536.0;
	  gtk_color_selection_set_color (GTK_COLOR_SELECTION (item), rgb);
	}
    }
  gtk_signal_connect
    (GTK_OBJECT (item), "color_changed",
     GTK_SIGNAL_FUNC (update_value), item);
  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);
}


void
dialog_start_box (int argc, char **argv)
{
  GtkWidget *item;
  int vertical = 0;
  int homogeneous = 0;
  int spacing = 0;
  unsigned int border_width = 0;
  int i;
  gboolean read_spacing = TRUE;

  for (i = 0; i < argc; i++)
    {
      if (strcmp (argv[i], "--") == 0)
	{
	  break;
	}
      else if (strcasecmp (argv[i], "homogeneous") == 0)
	{
	  homogeneous = 1;
	}
      else if (strcasecmp (argv[i], "vertical") == 0)
	{
	  vertical = 1;
	}
      else if (read_spacing)
	{
	  spacing = atoi (argv[i]);
	  read_spacing = FALSE;
	}
      else
	{
	  border_width = atoi (argv[i]);
	}
    }

  if (vertical)
    {
      item = gtk_vbox_new (homogeneous, spacing);
    }
  else
    {
      item = gtk_hbox_new (homogeneous, spacing);
    }
  gtk_container_border_width (GTK_CONTAINER (item), border_width);
  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);
  current = item;
}


void
dialog_separator (int argc, char **argv)
{
  GtkWidget *item;

  if (GTK_IS_HBOX (current))
    {
      item = gtk_vseparator_new ();
    }
  else
    {
      item = gtk_hseparator_new ();
    }
  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);
}


void
dialog_scale (int argc, char **argv)
{
  unsigned int vertical = 0;
  unsigned int digits = 0;
  GtkWidget *item;
  GtkObject *adj;
  int i;

  g_return_if_fail (argc >= 7);

  i = 0;
  if (argc >= 8)
    {
      if (strcasecmp (argv[1], "vertical") == 0)
	{
	  vertical = 1;
	  i = 1;
	}
      else if (strcasecmp (argv[1], "horizontal") == 0)
	{
	  vertical = 0;
	  i = 1;
	}
    }

  if (argc >= 8 + i)
    {
      digits = atoi (argv[i + 7]);
    }

  adj = gtk_adjustment_new (atof (argv[i + 1]), atof (argv[i + 2]),
			    atof (argv[i + 3]), atof (argv[i + 4]),
			    atof (argv[i + 5]), atof (argv[i + 6]));
  if (vertical)
    {
      item = gtk_vscale_new (GTK_ADJUSTMENT (adj));
    }
  else
    {
      item = gtk_hscale_new (GTK_ADJUSTMENT (adj));
    }
  gtk_signal_connect_object
    (GTK_OBJECT (adj), "value_changed",
     GTK_SIGNAL_FUNC (update_value), GTK_OBJECT (item));
  gtk_widget_set_name (item, argv[0]);
  gtk_scale_set_digits (GTK_SCALE (item), digits);
  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);
}


void
dialog_spinbutton (int argc, char **argv)
{
  GtkWidget *item;
  GtkObject *adj;
  unsigned int digits = 0;

  g_return_if_fail (argc >= 8);

  if (argc >= 9)
    {
      digits = atoi (argv[8]);
    }

  adj = gtk_adjustment_new (atof (argv[1]), atof (argv[2]),
			    atof (argv[3]), atof (argv[4]),
			    atof (argv[5]), atof (argv[6]));

  item = gtk_spin_button_new (GTK_ADJUSTMENT (adj), atof (argv[7]), digits);
  gtk_signal_connect_object
    (GTK_OBJECT (adj), "value_changed",
     GTK_SIGNAL_FUNC (update_value), GTK_OBJECT (item));
  gtk_widget_set_name (item, argv[0]);
  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);
}


void
dialog_start_option_menu (int argc, char **argv)
{
  GtkWidget *item;
  g_return_if_fail (argc >= 1);

  option_menu_active_item = -1;
  option_menu_items = 0;
  item = gtk_option_menu_new ();
  gtk_widget_set_name (item, argv[0]);
  menu = gtk_menu_new ();
  gtk_widget_show (menu);
  add_to_dialog (item, argc, argv);
  current = item;
}


void
dialog_end_option_menu (int argc, char **argv)
{
  gtk_signal_connect_object
    (GTK_OBJECT (menu), "deactivate",
     GTK_SIGNAL_FUNC (update_value), GTK_OBJECT (current));
  gtk_option_menu_set_menu (GTK_OPTION_MENU (current), menu);
  menu = NULL;
  if (option_menu_active_item > 0)
   {
     gtk_option_menu_set_history (GTK_OPTION_MENU (current),
				  option_menu_active_item);
   }
  update_value (current);
  gtk_widget_show (current);
  current = current->parent;
}


void
dialog_option_menu_item (int argc, char **argv)
{
  GtkWidget *item;

  g_return_if_fail (argc >= 2);

  item = menu_item_new_with_pixmap_and_label ("", argv[0], NULL);

  gtk_object_set_data (GTK_OBJECT (item), "value", safestrdup (argv[1]));
  gtk_widget_show (item);

  if (argc >= 3 && strcasecmp (argv[2], "on") == 0)
    {
       option_menu_active_item = option_menu_items;
    }

  gtk_menu_append (GTK_MENU (menu), item);
  option_menu_items++;
}


