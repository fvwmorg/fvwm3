
/*
 * FvwmGtk - gtk menus and dialogs for fvwm2
 *
 * Copyright (c) 1999 Matthias Clasen <clasen@mathematik.uni-freiburg.de>
 *
 * This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include "gtkpixmapmenuitem.h"

#include "fvwm/module.h"
#include "libs/fvwmlib.h"
#include "libs/ModParse.h"
#include "libs/Picture.h"

int Fvwm_fd[2];
char *MyName;
int MyNameLen;
char *image_path;
GSList *radio_group;
char *radio_name;
GHashTable *widgets;
GtkWidget *current;
unsigned long context;
char *notebook_label;
int collecting_window_list;
int window_list_button;


/*
  General overview:

  All widgets (menus and dialogs) are stored
  in one hash table, widgets. 
  They are built up piecemeal, the widget 
  currently being built up is current.
  For dialogs, current may also point to
  a subwidget of the dialog.
  dialog contain diffent kinds of widgets:
  
  * data widgets which return some value
  these all have a name and store their value
  as data on the top-level dialog under their name
    
  * action widgets like buttons. these have
  a list of commands which are sent to fvwm or
  executed by FvwmGtk itself (eg close). 
  The commands are stored as data on the action 
  widget under the name "values", as a NULL-terminated
  char*-array.

  A command can contain references to other widgets
  values in the form $(name). These references are
  resolved recursively, ie the values can again
  contain references.

  The resolution of the references is done by
  splitting the string into a list of passive strings
  and variable references and splicing in sublists
  for the variable references until only passive strings
  remain.
 */

/*********************************
 *
 *           Dialogs
 *
 *********************************/

void 
open_dialog (int argc, char **argv)
{
  GtkWidget *item;
  char *name;

  g_return_if_fail (argc >= 2);

  item = g_hash_table_lookup (widgets, argv[0]);
  if (item == NULL) 
    {
      item = gtk_window_new (GTK_WINDOW_DIALOG);
      gtk_widget_set_name (item, argv[0]);
      name = gtk_widget_get_name (item);
      gtk_widget_ref (item);
      g_hash_table_insert (widgets, name, item);
     
      gtk_window_set_title (GTK_WINDOW (item), argv[1]);
      gtk_window_set_wmclass (GTK_WINDOW (item), argv[1], "FvwmGtk");
      gtk_object_set_data 
	(GTK_OBJECT (item), "values", 
	 (gpointer) g_hash_table_new (g_str_hash, g_str_equal)); 
      gtk_signal_connect 
	(GTK_OBJECT (item), "delete_event",
	 GTK_SIGNAL_FUNC (gtk_widget_hide), (gpointer) current);
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
update_value (GtkWidget *w);

void 
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
    }
  notebook_label = strdup (argv[0]);
}

void 
dialog_end_notebook (int argc, char **argv)
{
  g_return_if_fail (current != NULL); 

  if (notebook_label)
    {
      free (notebook_label);
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



typedef struct str_struct
{
  struct str_struct *next;
  char *s;
  int is_var;
} str;

/* split string val into a list of str's, each containing 
   an ordinary substring or a variable reference of the form $(bla). 
   The list is appended to pre, the last list entry is returned.
*/
str *
split_string (char *val, str *pre)
{
  char *s, *t;
  char *v = val;
  str *curr = pre;
  
  while (v && *v) 
    {
      s = strstr (v, "$(");
      if (s) 
	{
	  t = strstr (s, ")");
	}

      if (s && t) 
	{
	  /* append the part before $( */
	  curr->next = (str *) safemalloc (sizeof (str));
	  curr = curr->next;
	  *s = '\0';
	  curr->s = strdup (v);
	  curr->is_var = 0;
	  *s = '$';
	  
	  /* append the variable reference, silently omit the ) */
	  curr->next = (str *) safemalloc (sizeof (str));
	  curr = curr->next;
	  curr->next = NULL;
	  *t = '\0';
	  curr->s = strdup (s + 2);    
	  curr->is_var = 1;
	  *t = ')';
	  
	  v = t + 1;
	}
      else
	{
	  /* append the part after the last variable reference */
	  curr->next = (str *) safemalloc (sizeof (str));
	  curr = curr->next;
	  curr->next = NULL;
	  curr->s = strdup (v);
	  curr->is_var = 0;
	  v = NULL;
	}
    }
  return curr;
}

char *
combine_string (str *p)
{
  str *r, *next;
  int l;
  char *res;
  
  for (l = 0, r = p; r != NULL; r = r->next)
    {
      l += strlen (r->s);
    }
  res = (char *) safemalloc (l * sizeof (char));
  *res = '\0';
  for (r = p; r != NULL; r = next)
    {
      strcat (res, r->s);
      next = r->next;
      free (r);
    }
  return res;
}

void
debug_list (char *t, str *p)
{
  str *r;

  for (r = p; r != NULL; r = r->next)
    {
      if (r->is_var)
	{
	  fprintf (stderr, "\t$(%s)\n", r->s);
	}
      else
	{
	  fprintf (stderr, "\t\"%s\"\n", r->s);
	}
    }
  fprintf(stderr, "----\n");
}

char *
recursive_replace (GtkWidget *d, char *val)
{
  str *head, *r, *tail, *next;
  char *nval;

  head = (str *) safemalloc (sizeof (str));
  head->is_var = 0;
  head->s = "";
  head->next = NULL;

  split_string (val, head);
  
  debug_list ("main list", head);
  for (r = head; r->next != NULL; r = r->next)
    {
      next = r->next;
      if (next->is_var)
	{
	  nval = (char *) gtk_object_get_data (GTK_OBJECT (d), next->s);
	  if (nval)
	    {
	      tail = split_string (nval, r);
	      tail->next = next->next;
	      free (next);
	      debug_list ("main list", head);
	    }
	  else 
	    {
	      next->is_var = 0;
	    }
	}
    }
  return combine_string (head);
}

void 
send_values (GtkWidget *w)
{
  GtkWidget *dialog;
  char **vals;
  char *c;
  int i;

  dialog = gtk_widget_get_toplevel (w);
  for (vals = (char **) gtk_object_get_data (GTK_OBJECT(w), "return_values");
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
	      system (val);
	    } 
	  else
	    {
	      SendText (Fvwm_fd, val, context);
	    }
	}
      free (val);
    }
}

gpointer 
widget_get_value (GtkWidget *w)
{
  static char buf[100];

  if (GTK_IS_SCALE (w))
    {
      snprintf (buf, sizeof (buf), "%.*f", 
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
      snprintf (buf, sizeof (buf), "rgb:%0.4lx/%0.4lx/%0.4lx", 
		red, green, blue);
      return buf;
    }
  else if (GTK_IS_ENTRY (w))
    {
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
void 
update_value (GtkWidget *w)
{
  char *val;

  val = widget_get_value (w);
  if (val) 
    {
      gtk_object_set_data_full (GTK_OBJECT (gtk_widget_get_toplevel (w)), 
				gtk_widget_get_name (w), strdup (val), free);
    }
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
      vals[i] = strdup(argv[i + 1]);
    }
  vals[i] = NULL;
  gtk_object_set_data (GTK_OBJECT(item), "return_values", 
		       (gpointer) vals); 
  gtk_signal_connect 
    (GTK_OBJECT(item), "clicked",
     GTK_SIGNAL_FUNC(send_values), (gpointer) item);
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
  gtk_object_set_data (GTK_OBJECT (item), "on-value", strdup (argv[2]));
  gtk_object_set_data (GTK_OBJECT (item), "off-value", strdup (argv[3]));
  if (argc >= 5 && strcasecmp (argv[4], "on") == 0)
    {
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (item), TRUE);
    }
  else
    {
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (item), FALSE);
    }
  gtk_signal_connect 
    (GTK_OBJECT(item), "toggled",
     GTK_SIGNAL_FUNC(update_value), (gpointer) item);

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
  if (argc >= 2)
    {
      gtk_entry_set_text (GTK_ENTRY (item), argv[1]);
    }
  gtk_signal_connect 
    (GTK_OBJECT(item), "changed",
     GTK_SIGNAL_FUNC(update_value), (gpointer) item);
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
  gtk_widget_set_name (item, radio_name);
  gtk_object_set_data (GTK_OBJECT (item), "on-value", strdup (argv[1]));
  if (argc >= 3 && strcasecmp (argv[2], "on") == 0)
    {
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (item), TRUE);
    }
  else
    {
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (item), FALSE);
    }
  gtk_signal_connect 
    (GTK_OBJECT(item), "toggled",
     GTK_SIGNAL_FUNC(update_value), (gpointer) item);

  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);  
}

void
dialog_start_radiogroup (int argc, char **argv)
{
  g_return_if_fail (argc >= 1);

  if (radio_name)
    {
      free (radio_name);
    }
  radio_name = strdup (argv[0]); 
  radio_group = NULL;
}

void
dialog_end_radiogroup (int argc, char **argv)
{
  if (radio_name)
    {
      free (radio_name);
    }
  radio_group = NULL;
}

void
dialog_color (int argc, char **argv)
{
  GtkWidget *item;
  XColor color;

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
     GTK_SIGNAL_FUNC (update_value), (gpointer) item);  
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
  int vertical = 0;
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
     GTK_SIGNAL_FUNC (update_value), (gpointer) item);
  gtk_widget_set_name (item, argv[0]);
  if (argc >= 8 + i) 
    {
      gtk_scale_set_digits (GTK_SCALE (item), atoi (argv[i + 7]));
    }
  gtk_widget_show (item);
  add_to_dialog (item, argc, argv);  
}

/*********************************
 *
 *           Menus
 *
 *********************************/

void 
send_item (char *s)
{
  SendText (Fvwm_fd, s, context);
}

GtkWidget *
find_or_create_menu (char *name)
{
  GtkWidget *menu;
  char *name_copy;

  menu = g_hash_table_lookup (widgets, name);

  if (menu == NULL) 
    {
      menu = gtk_menu_new ();
      gtk_widget_set_name (menu, name);
      name_copy = gtk_widget_get_name (menu);
      gtk_widget_ref (menu);
      g_hash_table_insert (widgets, name_copy, menu);
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

void
open_menu (int argc, char **argv)
{
  GtkWidget *item;
  char *name;

  g_return_if_fail (argc >= 1);

  current = find_or_create_menu (argv[0]);
}

GtkWidget *
menu_item_new_with_pixmap_and_label (char *file, char *label)
{
  GtkWidget *item;
  GtkWidget *accel_label;
  GtkWidget *icon;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  char *path;

  path = findImageFile (file, image_path, R_OK);

  if (path == NULL) 
    {
      item = gtk_menu_item_new ();
    } 
  else 
    {
      item = gtk_pixmap_menu_item_new ();
      pixmap = gdk_pixmap_create_from_xpm (NULL, &mask, NULL, path);
      icon = gtk_pixmap_new (pixmap, mask);
      gtk_pixmap_menu_item_set_pixmap (GTK_PIXMAP_MENU_ITEM (item), icon);
      gtk_widget_show (icon);
      
      free (path);
    }

  accel_label = gtk_accel_label_new (label);
  gtk_misc_set_alignment (GTK_MISC (accel_label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (item), accel_label);
  gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (accel_label), item);
  gtk_widget_show (accel_label);
  
  return item;
}

/*
 Add a title to the current menu.
 */
void 
menu_title (int argc, char **argv)
{
  GtkWidget *item;

  g_return_if_fail (argc >= 1);

  if (argc >= 2)
    {
      item = menu_item_new_with_pixmap_and_label (argv[1], argv[0]);
    }
  else
    {
      item = menu_item_new_with_pixmap_and_label ("", argv[0]);
    }
  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
  /* 
     This is a hack stolen from the Gnome panel. There ought
     to be a better way.
  */
  gtk_signal_connect 
    (GTK_OBJECT(item), "select",
     GTK_SIGNAL_FUNC(gtk_menu_item_deselect), NULL);
}

/*
   Add a separator to the current menu.
 */
void
menu_separator (int argc, char **argv)
{
  GtkWidget *item;
  
  item = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
  gtk_widget_set_sensitive (item, 0);
}

/*
   Add an item to the current menu
 */
void
menu_item (int argc, char **argv)
{
  GtkWidget *item;
  
  g_return_if_fail (argc >= 2);

  if (argc >= 3)
    {
      item = menu_item_new_with_pixmap_and_label (argv[2], argv[0]);
    }
  else
    {
      item = menu_item_new_with_pixmap_and_label ("", argv[0]);
    }
  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
  gtk_signal_connect_object 
    (GTK_OBJECT (item), "activate",
     GTK_SIGNAL_FUNC (send_item), (gpointer) strdup (argv[1])); 
}

/*
   Add a submenu to the current menu.
 */
void
menu_submenu (int argc, char **argv)
{
  GtkWidget *item;
  int i;
  
  if (argc >= 3)
    {
      item = menu_item_new_with_pixmap_and_label (argv[2], argv[0]);
    }
  else
    {
      item = menu_item_new_with_pixmap_and_label ("", argv[0]);
    }
  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), 
			     find_or_create_menu (argv[1])); 
}

void
destroy (int argc, char **argv)
{
  GtkWidget *w;

  g_return_if_fail (argc >= 1);

  w = g_hash_table_lookup (widgets, argv[0]);
  if (w != NULL)
    { 
      if (GTK_IS_MENU (w))
        {
	  fprintf(stderr, "destroying menu %s\n", argv[0]);
	}
      else if (GTK_IS_WINDOW (w))
	{
	  fprintf(stderr, "destroying dialog %s\n", argv[0]);
	}
      else 
	{
	  fprintf(stderr, "destroying unknown %s\n", argv[0]);
	}
      fprintf(stderr, "1\n");
      g_hash_table_remove (widgets, argv[0]);
      fprintf(stderr, "2\n");
      gtk_widget_unref (w);
      fprintf(stderr, "3\n");
      if (current == w) 
        {
          current = NULL;
        }
      fprintf(stderr, "4\n");
      gtk_widget_destroy (w);
      fprintf(stderr, "5\n");
    }
}


void
separator (int argc, char **argv)
{
  if (GTK_IS_MENU (current))
    {
      menu_separator (argc, argv);
    }
  else
    {
      dialog_separator (argc, argv);
    }
}


void
window_list (int argc, char **argv)
{
  GtkWidget *item;
  char **opts;
  int i;

  g_return_if_fail (argc >= 1);
  
  item = find_or_create_menu (argv[0]);
  opts = (char **) safemalloc (argc * sizeof (char *));
  for (i = 0; i < argc - 1; i++) 
    {
      opts[i] = strdup (argv[i + 1]);
    }
  opts[i] = NULL;
  gtk_object_set_data (GTK_OBJECT (item), "window_list", (gpointer) opts);
}

void
start_window_list (char *name, int button, char **opts)
{
  destroy (1, &name);
  open_menu (1, &name);
  gtk_object_set_data (GTK_OBJECT (current), "window_list", (gpointer) opts);
  collecting_window_list = TRUE;
  window_list_button = button;
  SendText (Fvwm_fd, "Send_WindowList", 0);
}

char *table[] = {
  "Box",
  "Button",
  "CheckButton",
  "Color",
  "Destroy",
  "Dialog",
  "EndBox",
  "EndFrame",
  "EndNotebook",
  "EndRadioGroup",
  "Entry",
  "Frame",
  "Item",
  "Label",
  "Menu",
  "Notebook",
  "RadioButton",
  "RadioGroup",
  "Scale",
  "Separator",
  "Submenu",
  "Title",
  "WindowList"
};

void (*handler[])(int, char**) = {
  dialog_start_box,
  dialog_button,
  dialog_checkbutton,
  dialog_color,
  destroy,
  open_dialog,
  dialog_end_something,
  dialog_end_something,
  dialog_end_notebook,
  dialog_end_radiogroup,
  dialog_entry,
  dialog_start_frame,
  menu_item,
  dialog_label,
  open_menu,
  dialog_notebook,
  dialog_radiobutton,
  dialog_start_radiogroup,
  dialog_scale,
  separator,
  menu_submenu,
  menu_title,
  window_list
};

void 
parse_arguments (char **line, int *argc, char ***argv)
{
  char *tmp[100];
  int i;

  for (i = 0; i < 100 ; i++) 
    {
      tmp[i] = GetArgument (line);
      if (!tmp[i]) break;
    }
  *argc = i;
  
  *argv = (char**) safemalloc (i * sizeof (char *));
  for (i = 0; i < *argc; i++)
    {
      (*argv)[i] = tmp[i]; 
    }
}

void
widget_not_found (char *name)
{
  GtkWidget *dialog, *box, *item;
  char buf[200];	

  SendText (Fvwm_fd, "Beep", 0);
  dialog = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title (GTK_WINDOW (dialog), "Error");
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "Error", "FvwmGtk");
      
  box = gtk_vbox_new (FALSE, 10);
  gtk_container_add (GTK_CONTAINER (dialog), box);
  gtk_container_border_width (GTK_CONTAINER (dialog), 30);
  snprintf (buf, sizeof (buf), "No such menu or dialog: %s", name); 
  item = gtk_label_new (buf);
  gtk_box_pack_start (GTK_BOX (box), item, FALSE, FALSE, 5);
  item = gtk_button_new_with_label ("OK");
  gtk_box_pack_start (GTK_BOX (box), item, FALSE, FALSE, 5);
  gtk_signal_connect_object 
    (GTK_OBJECT (item), "clicked",
     GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) dialog); 
  gtk_widget_show_all (box);
  gtk_widget_show (dialog);
}

void 
ParseConfigLine (char *buf) 
{
  int argc;
  char **argv;
  char *p;
  char **e;

  if (buf[strlen(buf)-1] == '\n') 
    {
      buf[strlen(buf)-1] = '\0';	
    }
  if (strncasecmp (buf, MyName, MyNameLen) == 0) 
    {
      fprintf (stderr, "Found line for me: %s\n", buf);
      p = buf + MyNameLen;
      if (e = FindToken (p, table, char*)) 
	{ 
	  p += strlen (*e);	
	  parse_arguments (&p, &argc, &argv); 
	  handler[e - (char**)table] (argc, argv);
	} 
      else 
	{
	  fprintf (stderr, "%s: unknown command: %s\n", MyName + 1, buf);
	}
    }
  else if (strncasecmp (buf, "ImagePath", 9) == 0) 
    {
      fprintf (stderr, "Found ImagePath: %s\n", buf);
      if (image_path != NULL) 
	{
	  free (image_path);
	}
      image_path = strdup (buf+9);
    }
}

void 
ParseOptions (void) 
{
  char *buf;
  
  while (GetConfigLine (Fvwm_fd, &buf), buf != NULL) 
    {
      ParseConfigLine(buf);
    } 
} 

void 
ProcessMessage (unsigned long type, 
	        unsigned long timestamp, 
		unsigned long *body)
{
  int button = 0;
  char name[128];
  GtkWidget *widget;
  char *wl_args[2];
  char buf[200];
  char **opts;

  SendText (Fvwm_fd, "UNLOCK", 0);
  switch (type) 
    {
    case M_STRING:
      context = body[0]; /* this is tmp_win->w */
      sscanf ((char*) (&body[3]), "%128s %d", name, &button);
      widget = g_hash_table_lookup (widgets, name);
      if (widget == NULL)
	{
          widget_not_found (name);
	}
      else if (GTK_IS_MENU (widget))
	{
          opts = (char **) gtk_object_get_data (GTK_OBJECT (widget), 
						"window_list");
          if (opts)
            {
              start_window_list (name, button, opts);
            }
          else
            {
	      gtk_menu_popup (GTK_MENU (widget), NULL, NULL, NULL, NULL, 
			      button, timestamp);
	    }    
        }
      else if (GTK_IS_WINDOW (widget)) 
	{
	  gtk_widget_show (GTK_WIDGET (widget));
	}
      break;
    case M_CONFIG_INFO:
      ParseConfigLine ((char*) (&body[3]));
      break;
    case M_WINDOW_NAME:
      if (collecting_window_list)
	{
	  snprintf (buf, sizeof(buf), "WindowListFunc %#lx", body[0]);
	  wl_args[0] = (char*) (&body[3]);
	  wl_args[1] = buf;
	  menu_item (2, wl_args);
	}
      break;
    case M_END_WINDOWLIST:
      if (collecting_window_list)
	{
          collecting_window_list = FALSE;
          gtk_menu_popup (GTK_MENU (current), NULL, NULL, NULL, NULL, 
	    	          window_list_button, timestamp);
        }
      break;
    }
}

void 
ReadFvwmPipe (gpointer data, int source, GdkInputCondition cond)
{
  unsigned long header[HEADER_SIZE],*body;

  if (ReadFvwmPacket (source, header, &body) > 0)
    {
      ProcessMessage (header[1], header[3], body);
      /*      free (body); */
    }
}

void 
DeadPipe (int nons)
{
  exit (1);
}

int 
main (int argc, char **argv)
{
  char *s, rcfile[200];
  
  if (s = strrchr (argv[0], '/'))
    {
      s++;
    }
  else
    {
      s = argv[0];
    }
  if (argc == 7)
    {
      s = argv[6];
    }
  MyNameLen = strlen(s) + 1;
  MyName = safemalloc (MyNameLen + 1);
  *MyName = '*';
  strcpy (MyName + 1, s);

  image_path = NULL;

  Fvwm_fd[0] = atoi (argv[1]);
  Fvwm_fd[1] = atoi (argv[2]);

  gtk_init (&argc, &argv);
  snprintf (rcfile, 200, "%s/.%src", getenv ("HOME"), MyName + 1);
  gtk_rc_parse (rcfile);

  widgets = g_hash_table_new (g_str_hash, g_str_equal);
  current = NULL;
  notebook_label = NULL;
  radio_group = NULL;
  radio_name = NULL;
  collecting_window_list = 0;

  ParseOptions ();
  
  SetMessageMask (Fvwm_fd, 
		  M_STRING |
		  M_LOCKONSEND |
		  M_CONFIG_INFO |
		  M_SENDCONFIG |
		  M_WINDOW_NAME |
		  M_END_WINDOWLIST);

  gtk_input_add_full (Fvwm_fd[1], GDK_INPUT_READ, ReadFvwmPipe, 
                      NULL, NULL, NULL);
  gtk_main ();
  return 0;
}


