#include "config.h"

#include <X11/Xlib.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "libs/fvwmlib.h"

#include "menu.h"
#include "windowlist.h"


extern GtkWidget *current;
extern GHashTable *window_list_entries;
extern long current_desk;


static void
window_list_item (gpointer key, gpointer value, gpointer data)
{
  unsigned long win = *(unsigned long *) key;
  window_list_entry *wle = (window_list_entry *) value;
  window_list_options *opts;
  char *argv[4];
  char cmd[200], geo[200];
  
  if (wle->skip)
    {
      return;
    }

  opts = (window_list_options *) 
    gtk_object_get_data (GTK_OBJECT (current), "window_list");

  if ((opts->one_desk && (opts->desk != wle->desk)) ||
      (opts->current_desk && (current_desk != wle->desk)) ||
      (opts->omit_iconified && wle->iconified) ||
      (opts->omit_sticky && wle->sticky) ||
      (opts->omit_normal && !wle->iconified && !wle->sticky))
    {
      return;
    }

  g_snprintf (cmd, sizeof (cmd), "WindowListFunc %#lx", win);
  g_snprintf (geo, sizeof (geo), "  %s%d:%dx%d%s%d%s%d%s%s",
	      wle->iconified ? "(" : "",
	      wle->desk, wle->width, wle->height, 
	      (wle->x >= 0) ? "+" : "", wle->x, 
	      (wle->y >= 0) ? "+" : "", wle->y, 
	      wle->sticky ? " S" : "",
	      wle->iconified ? ")" : "");

  argv[0] = (opts->use_icon_name ? wle->icon_name : wle->name); 
  argv[1] = cmd;
  argv[2] = ((opts->no_mini_icon || !wle->mini_icon) ? "" : wle->mini_icon);
  argv[3] = geo;
  
  menu_item (4, argv);
}


void
window_list (int argc, char **argv)
{
  GtkWidget *item;
  window_list_options *opts;
  int i;

  g_return_if_fail (argc >= 1);
  
  item = find_or_create_menu (argv[0]);
  opts = (window_list_options *) safemalloc (sizeof (window_list_options));
  memset (opts, '\0', sizeof (window_list_options));

  for (i = 1; i < argc; i++) 
    {
      if (strcasecmp (argv[i], "NoGeometry") == 0)
	{
	  opts->no_geometry = 1;
	}
      else if (strcasecmp (argv[i], "Desk") == 0)
	{
	  if (i+1 < argc)
	    {
	      opts->one_desk = 1;
	      opts->desk = atoi (argv[++i]);
	    }
	}
      else if (strcasecmp (argv[i], "CurrentDesk") == 0)
	{
	  opts->current_desk = 1;
	}
      else if (strcasecmp (argv[i], "UseIconName") == 0)
	{
	  opts->use_icon_name = 1;
	}
      else if (strcasecmp (argv[i], "NoMiniIcon") == 0)
	{
	  opts->no_mini_icon = 1;
	}
      else if (strcasecmp (argv[i], "Icons") == 0)
	{
	  opts->omit_iconified = 0;
	}
      else if (strcasecmp (argv[i], "NoIcons") == 0)
	{
	  opts->omit_iconified = 1;
	}
      else if (strcasecmp (argv[i], "OnlyIcons") == 0)
	{
	  opts->omit_iconified = 0;
	  opts->omit_sticky = 1;
	  opts->omit_normal = 1;
	}
      else if (strcasecmp (argv[i], "Sticky") == 0)
	{
	  opts->omit_sticky = 0;
	}
      else if (strcasecmp (argv[i], "NoSticky") == 0)
	{
	  opts->omit_sticky = 1;
	}
      else if (strcasecmp (argv[i], "OnlySticky") == 0)
	{
	  opts->omit_sticky = 0;
	  opts->omit_iconified = 1;
	  opts->omit_normal = 1;
	}
      else if (strcasecmp (argv[i], "Normal") == 0)
	{
	  opts->omit_normal = 0;
	}
      else if (strcasecmp (argv[i], "NoNormal") == 0)
	{
	  opts->omit_normal = 1;
	}
      else if (strcasecmp (argv[i], "OnlyNormal") == 0)
	{
	  opts->omit_normal = 0;
	  opts->omit_iconified = 1;
	  opts->omit_sticky = 1;
	}
	
    }
  gtk_object_set_data (GTK_OBJECT (item), "window_list", opts);
}


window_list_entry *
lookup_window_list_entry (unsigned long w)
{
  window_list_entry *wle;

  wle = g_hash_table_lookup (window_list_entries, &w);
  if (!wle)
    {
      wle = (window_list_entry *) malloc (sizeof (window_list_entry));
      wle->w = w;
      wle->name = NULL;
      wle->icon_name = NULL;
      wle->mini_icon = NULL;
      wle->desk = 0;
      wle->layer = 0;
      wle->iconified = 0;
      wle->sticky = 0;
      wle->skip = 0;
      wle->x = 0;
      wle->y = 0;
      wle->width = 0;
      wle->height = 0;

      g_hash_table_insert (window_list_entries, &(wle->w), wle);
    }

  return wle;
}


void
construct_window_list (void)
{
  g_hash_table_foreach (window_list_entries, window_list_item, NULL);
}


