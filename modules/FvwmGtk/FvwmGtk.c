
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
#include <gdk/gdk.h>
#include <gdk/gdkprivate.h>
#include "gtkpixmapmenuitem.h"

#ifdef IMLIB
#include <gdk_imlib.h>
#endif

#include "fvwm/module.h"
#include "libs/fvwmlib.h"
#include "libs/Picture.h"

int fvwm_fd[2];
char *my_name;
int my_name_len;
char *image_path;
GSList *radio_group;
char *group_name;
GHashTable *widgets;
GtkWidget *current;
GtkWidget *menu;
int option_menu_items, option_menu_active_item;
unsigned long context;
char *notebook_label;
int collecting_window_list;
int window_list_button;
GtkWidget *attach_to_toplevel;
int icon_w, icon_h;

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
      notebook_label = NULL;
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
  
  for (l = 1, r = p; r != NULL; r = r->next)
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
  for (r = head; r->next != NULL; r = r->next)
    {
      next = r->next;
      if (next->is_var)
	{
	  nval = (char *) gtk_object_get_data (GTK_OBJECT (d), next->s);
	  if (nval)
	    {
              /* this changes r->next, thus freeing next is safe */
	      tail = split_string (nval, r);
	      tail->next = next->next;
              free (next);
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

gpointer 
widget_get_value (GtkWidget *w)
{
  static char buf[100];

  if (GTK_IS_OPTION_MENU (w))
    {
      GtkWidget *item;

      item = gtk_menu_get_active (GTK_MENU (GTK_OPTION_MENU (w)->menu));
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
  if (argc >= 2)
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
  group_name = strdup (argv[0]); 
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

GtkWidget*
menu_item_new_with_pixmap_and_label (char *x, char *y);

void
dialog_option_menu_item (int argc, char **argv)
{
  GtkWidget *item;

  g_return_if_fail (argc >= 2);

  item = menu_item_new_with_pixmap_and_label ("", argv[0]);

  gtk_object_set_data (GTK_OBJECT (item), "value", strdup (argv[1]));
  gtk_widget_show (item);

  if (argc >= 3 && strcasecmp (argv[2], "on") == 0)
    {
       option_menu_active_item = option_menu_items;
    }

  gtk_menu_append (GTK_MENU (menu), item);
  option_menu_items++;
}


/*********************************
 *
 *           Menus
 *
 *********************************/

void 
send_item (GtkWidget *item, char *s)
{
  SendText (fvwm_fd, s, context);
}

void
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

void
open_menu (int argc, char **argv)
{
  g_return_if_fail (argc >= 1);

  current = find_or_create_menu (argv[0]);
}

/* from gtkcauldron.c */
/* result must be g_free'd */
gchar *
convert_label_with_ampersand (const gchar * _label, gint * accelerator_key, gint * underbar_pos)
{
    gchar *p;
    gchar *label = g_strdup (_label);
    for (p = label;; p++) {
	if (!*p)
	    break;
	if (!p[1])
	    break;
	if (*p == '&') {
	    memcpy (p, p + 1, strlen (p));
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
gchar *
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
GtkAccelGroup *
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
menu_item_new_with_pixmap_and_label (char *file, char *_label)
{
  GtkWidget *item;
  GtkWidget *label;
  GtkWidget *icon;
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;
  char *path;
  gint accel_key = 0, underbar_pos = -1;
  gchar *pattern, *converted_label;
#ifdef IMLIB
  GdkImlibImage *im;
  int w, h;
#endif

  path = findImageFile (file, image_path, R_OK);

  if (path == NULL) 
    {
      item = gtk_menu_item_new ();
    } 
  else 
    {
#ifdef IMLIB
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
  
  converted_label = convert_label_with_ampersand (_label, &accel_key,
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
  gtk_container_add (GTK_CONTAINER (item), label);
  gtk_widget_show (label);
  
  return item;
}

/*
 Add a title to the current menu.
 */
void 
menu_title (int argc, char **argv)
{
  GtkWidget *item;
  char *file;

  g_return_if_fail (argc >= 1);

  if (argc >= 2)
    {
      file = argv[1];
    }
  else
    {
      file = "";
    }

  item = menu_item_new_with_pixmap_and_label (file, argv[0]);
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
  char *file;
  
  g_return_if_fail (argc >= 2);

  if (argc >= 3)
    {
      file = argv[2];
    }
  else 
    {
      file = "";
    }
  item = menu_item_new_with_pixmap_and_label (file, argv[0]);
  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
  gtk_signal_connect 
    (GTK_OBJECT (item), "activate",
     GTK_SIGNAL_FUNC (send_item), strdup (argv[1])); 
}

void
menu_tearoff_item (int argc, char **argv)
{
  GtkWidget *item;
  
  item = gtk_tearoff_menu_item_new ();

  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
}

/*
   Add a submenu to the current menu.
 */
void
menu_submenu (int argc, char **argv)
{
  GtkWidget *item, *submenu;
  char *file;

  if (argc >= 3)
    {
      file = argv[2];
    }
  else 
    {
      file = "";
    }

  item = menu_item_new_with_pixmap_and_label (file, argv[0]);
  gtk_menu_append (GTK_MENU (current), item);
  gtk_widget_show (item);
  submenu = find_or_create_menu (argv[1]);

  gtk_menu_detach (GTK_MENU (submenu));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu); 
}

void
destroy (int argc, char **argv)
{
  GtkWidget *w;

  g_return_if_fail (argc >= 1);

  w = g_hash_table_lookup (widgets, argv[0]);
  if (w != NULL)
    { 
      if (gtk_widget_get_toplevel (current) == w) 
        {
          current = NULL;
        }

      g_hash_table_remove (widgets, argv[0]);
      gtk_object_unref (GTK_OBJECT (w));
      gtk_widget_destroy (w);
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
item (int argc, char **argv)
{
  if (GTK_IS_MENU (current))
    {
      menu_item (argc, argv);
    }
  else
    {
      dialog_option_menu_item (argc, argv);
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
  gtk_object_set_data (GTK_OBJECT (item), "window_list", opts);
}

void
start_window_list (char *name, int button, char **opts)
{
  destroy (1, &name);
  open_menu (1, &name);
  gtk_object_set_data (GTK_OBJECT (current), "window_list", opts);
  collecting_window_list = TRUE;
  window_list_button = button;
  SendText (fvwm_fd, "Send_WindowList", 0);
}

void
parse_rc_file (int argc, char **argv)
{
  g_return_if_fail (argc >= 1);

  gtk_rc_parse (argv[0]);
  
}

void 
icon_size (int argc, char **argv)
{
  if (argc < 2)
    {
      icon_w = 0;
      icon_h = 0;
    }
  else 
    {
      icon_w = atoi (argv[0]);
      icon_h = atoi (argv[1]);
    }
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
  "EndOptionMenu",
  "EndRadioGroup",
  "Entry",
  "Frame",
  "IconSize",
  "Item",
  "Label",
  "Menu",
  "Notebook",
  "OptionMenu",
  "RadioButton",
  "RadioGroup",
  "RCFile",
  "Scale",
  "Separator",
  "SpinButton",
  "Submenu",
  "Tearoff",
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
  dialog_end_option_menu,
  dialog_end_radiogroup,
  dialog_entry,
  dialog_start_frame,
  icon_size,
  item,
  dialog_label,
  open_menu,
  dialog_notebook,
  dialog_start_option_menu,
  dialog_radiobutton,
  dialog_start_radiogroup,
  parse_rc_file,
  dialog_scale,
  separator,
  dialog_spinbutton,
  menu_submenu,
  menu_tearoff_item,
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
      *line = GetNextSimpleOption( *line, &tmp[i] );
      if (!tmp[i]) break;
    }
  *argc = i;
  
  *argv = (char **) safemalloc (i * sizeof (char *));
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

  SendText (fvwm_fd, "Beep", 0);
  dialog = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title (GTK_WINDOW (dialog), "Error");
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "Error", "FvwmGtk");
      
  box = gtk_vbox_new (FALSE, 10);
  gtk_container_add (GTK_CONTAINER (dialog), box);
  gtk_container_border_width (GTK_CONTAINER (dialog), 30);
  g_snprintf (buf, sizeof (buf), "No such menu or dialog: %s", name); 
  item = gtk_label_new (buf);
  gtk_box_pack_start (GTK_BOX (box), item, FALSE, FALSE, 5);
  item = gtk_button_new_with_label ("OK");
  gtk_box_pack_start (GTK_BOX (box), item, FALSE, FALSE, 5);
  gtk_signal_connect_object 
    (GTK_OBJECT (item), "clicked",
     GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (dialog)); 
  gtk_widget_show_all (box);
  gtk_widget_show (dialog);
}

void 
parse_config_line (char *buf) 
{
  int argc;
  char **argv;
  char *p;
  char **e;

  if (buf[strlen(buf)-1] == '\n') 
    {
      buf[strlen(buf)-1] = '\0';	
    }
  if (strncasecmp (buf, my_name, my_name_len) == 0) 
    {
      p = buf + my_name_len;
      if ((e = FindToken (p, table, char*)))
	{ 
	  p += strlen (*e);	
	  parse_arguments (&p, &argc, &argv); 
	  handler[e - (char**)table] (argc, argv);
	} 
      else 
	{
	  fprintf (stderr, "%s: unknown command: %s\n", my_name + 1, buf);
	}
    }
  else if (strncasecmp (buf, "ImagePath", 9) == 0) 
    {
      if (image_path) 
	{
	  free (image_path);
	}
      image_path = stripcpy (buf + 9);
    }
}

void 
parse_options (void) 
{
  char *buf;
  
  while (GetConfigLine (fvwm_fd, &buf), buf != NULL) 
    {
      parse_config_line (buf);
    } 
} 

void 
process_message (unsigned long type, 
		 unsigned long timestamp, 
		 unsigned long *body)
{
  int button = 0;
  char name[128];
  GtkWidget *widget;
  char *wl_args[2];
  char buf[200];
  char **opts;

  SendText (fvwm_fd, "UNLOCK", 0);
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
      parse_config_line ((char*) (&body[3]));
      break;
    case M_WINDOW_NAME:
      if (collecting_window_list)
	{
	  g_snprintf (buf, sizeof (buf), "WindowListFunc %#lx", body[0]);
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
read_fvwm_pipe (gpointer data, int source, GdkInputCondition cond)
{
  unsigned long header[HEADER_SIZE], *body;

  if (ReadFvwmPacket (source, header, &body) > 0)
    {
      process_message (header[1], header[3], body);
      free (body);
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
  
  if ((s = strrchr (argv[0], '/')))
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
  my_name_len = strlen (s) + 1;
  my_name = safemalloc (my_name_len + 1);
  *my_name = '*';
  strcpy (my_name + 1, s);

  image_path = NULL;

  fvwm_fd[0] = atoi (argv[1]);
  fvwm_fd[1] = atoi (argv[2]);

#ifdef IMLIB
  gdk_init (&argc, &argv);
  gdk_imlib_init ();
  gtk_widget_push_visual (gdk_imlib_get_visual ());
  gtk_widget_push_colormap (gdk_imlib_get_colormap ());
#endif
  gtk_init (&argc, &argv);

  widgets = g_hash_table_new (g_str_hash, g_str_equal);
  current = NULL;
  notebook_label = NULL;
  radio_group = NULL;
  group_name = NULL;
  collecting_window_list = 0;
  attach_to_toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  icon_w = icon_h = 0;

  parse_options ();
  
  SetMessageMask (fvwm_fd, 
		  M_STRING |
		  M_LOCKONSEND |
		  M_CONFIG_INFO |
		  M_SENDCONFIG |
		  M_WINDOW_NAME |
		  M_END_WINDOWLIST);

  gtk_input_add_full (fvwm_fd[1], GDK_INPUT_READ, read_fvwm_pipe, 
                      NULL, NULL, NULL);
  gtk_main ();
  return 0;
}

