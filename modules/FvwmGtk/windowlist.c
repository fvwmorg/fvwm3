#include "config.h"

#include <stdio.h>
#include <stdlib.h>
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
window_list_item (window_list_entry *wle, window_list_options *opts)
{
  char *argv[4];
  char desk[10], cmd[200], geo[200];

  g_snprintf (cmd, sizeof (cmd), "%s %#lx", 
	      opts->function ? opts->function : "WindowListFunc",
	      wle->w);
  g_snprintf(desk, sizeof (desk), "%d", wle->desk);
  g_snprintf (geo, sizeof (geo), "  %s%s:%dx%d%s%d%s%d%s",
	      wle->iconified ? "(" : "",
	      wle->sticky ? "*" : desk,
	      wle->width, wle->height, 
	      (wle->x >= 0) ? "+" : "", wle->x, 
	      (wle->y >= 0) ? "+" : "", wle->y, 
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
      if (strcasecmp (argv[i], "Title") == 0)
	{
	  if (i+1 < argc)
	    {
	      if (opts->title)
		{
		  free (opts->title);
		  opts->title = NULL;
		}
	      i++;
	      if (strcmp (argv[i], "") != 0)
		{
		  opts->title = strdup (argv[i]);
		}
	    }
	  if (i+1 < argc)
	    {
	      if (opts->title_icon)
		{
		  free (opts->title_icon);
		  opts->title_icon = NULL;
		}
	      i++;
	      if (strcmp (argv[i], "") != 0)
		{
		  opts->title_icon = strdup (argv[i]);
		}
	    }
	  if (i+1 < argc)
	    {
	      if (opts->right_title)
		{
		  free (opts->right_title);
 		  opts->right_title = NULL;
		}
	      i++;
	      if (strcmp (argv[i], "") != 0)
		{
		  opts->right_title = strdup (argv[i]);
		}
	    }
	}
      else if (strcasecmp (argv[i], "Function") == 0)
	{
	  if (i+1 < argc)
	    {
	      if (opts->function)
		{
		  free (opts->function);
		}
	      opts->function = strdup (argv[++i]);
	    }
	}
      else if (strcasecmp (argv[i], "NoDeskSort") == 0)
	{
	  opts->sorting |= NO_DESK_SORT;
	}
      else if (strcasecmp (argv[i], "Alphabetic") == 0)
	{
	  opts->sorting |= SORT_ALPHABETIC;
	}
      else if (strcasecmp (argv[i], "NoGeometry") == 0)
	{
	  opts->no_geometry = 1;
	}
      else if (strcasecmp (argv[i], "Desk") == 0)
	{
	  if (i+1 < argc)
	    {
	      opts->one_desk = 1;
	      opts->desk = atoi (argv[++i]);
	      opts->sorting |= NO_DESK_SORT;
	    }
	}
      else if (strcasecmp (argv[i], "CurrentDesk") == 0)
	{
	  opts->one_desk = 1;
	  opts->current_desk = 1;
	  opts->sorting |= NO_DESK_SORT;
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
      else 
	{
	  if (opts->pattern) 
	    {
	      free (opts->pattern);
	    }
	  opts->pattern = strdup (argv[i]);
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

static window_list_entry **unsorted_window_list = NULL;
static int unsorted_window_list_entries = 0;

void 
append_unsorted (gpointer key, gpointer value, gpointer data)
{
  window_list_entry *wle = (window_list_entry *) value;
  window_list_options *opts = (window_list_options *) data;
  
  if ( !(wle->skip ||
	 (opts->one_desk && (opts->desk != wle->desk)) ||
	 (opts->current_desk && (current_desk != wle->desk)) ||
	 (opts->omit_iconified && wle->iconified) ||
	 (opts->omit_sticky && wle->sticky) ||
	 (opts->omit_normal && !wle->iconified && !wle->sticky) ||
	 (opts->pattern && 
	  !matchWildcards (opts->pattern, wle->icon_name) &&
	  !matchWildcards (opts->pattern, wle->name))) )
    {
      unsorted_window_list[unsorted_window_list_entries++] = wle;
    }
}

static int 
compare_desk (const window_list_entry **u1, const window_list_entry **u2)
{
  int r = (*u1)->desk - (*u2)->desk;
  return ((r != 0) ? r : (int) u1 - (int) u2);
}

static int 
compare_name (const window_list_entry **u1, const window_list_entry **u2)
{
  return strcasecmp((*u1)->name, (*u2)->name);
}

static int 
compare_icon_name (const window_list_entry **u1, const window_list_entry **u2)
{
  return strcasecmp((*u1)->icon_name, (*u2)->icon_name);
}

void
construct_window_list (void)
{
  window_list_options *opts;
  int i;

  unsorted_window_list = (window_list_entry **) 
    realloc (unsorted_window_list, 
	     g_hash_table_size (window_list_entries) * 
	     sizeof (window_list_entry *));
  
  unsorted_window_list_entries = 0;

  opts = (window_list_options *) 
    gtk_object_get_data (GTK_OBJECT (current), "window_list");

  g_hash_table_foreach (window_list_entries, append_unsorted, opts);

  if (opts->sorting & SORT_ALPHABETIC)
    {
      if (opts->use_icon_name)
	{
	  qsort (unsorted_window_list, unsorted_window_list_entries, 
		 sizeof(window_list_entry*), 
		 (int(*)(const void*,const void*))compare_icon_name);
	}
      else
	{
	  qsort (unsorted_window_list, unsorted_window_list_entries, 
		 sizeof(window_list_entry*), 
		 (int(*)(const void*,const void*))compare_name);
	}
    }
  if (!(opts->sorting & NO_DESK_SORT)) 
    {
      qsort (unsorted_window_list, unsorted_window_list_entries, 
	     sizeof(window_list_entry*), 
	     (int(*)(const void*,const void*))compare_desk);      
    }

  if (opts->title || opts->right_title)
    {
      char *argv[3];
      argv[0] = opts->title;
      argv[1] = opts->title_icon;
      argv[2] = opts->right_title;
      menu_title (3, argv);
      menu_separator (0, NULL);
    }

  for (i = 0; i < unsorted_window_list_entries; i++)
    {
      if (!(opts->sorting & NO_DESK_SORT) &&
	  (i > 0) && 
	  (unsorted_window_list[i-1]->desk != unsorted_window_list[i]->desk))
	{
	  menu_separator (0, NULL);
	}
      window_list_item (unsorted_window_list[i], opts);
    }
}


