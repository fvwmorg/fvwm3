/*
 * FvwmGtk - gtk menus and dialogs for fvwm
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

#include <X11/Xlib.h>

#ifdef NEED_GNOMESUPPORT_H
#include <gnome.h>
#else
#include <glib.h>
#include <gtk/gtk.h>
#endif

#ifdef GDK_IMLIB
#include <gdk_imlib.h>
#endif

#include <libs/Module.h>
#include <libs/fvwmlib.h>
#include <fvwm/fvwm.h>
#include <libs/vpacket.h>

#include "dialog.h"
#include "menu.h"
#include "windowlist.h"


int fvwm_fd[2] = { -1, -1 };
char *my_name = NULL;
int my_name_len = 0;
char *image_path = NULL;
GHashTable *widgets = NULL;
GtkWidget *current = NULL;
GtkWidget *attach_to_toplevel = NULL;
unsigned long context = 0;
int icon_w = 0;
int icon_h = 0;
long current_desk = 0;
GHashTable *window_list_entries = NULL;

/*
   General overview:

   All widgets (menus and dialogs) are stored in one hash table, widgets.
   They are built up piecemeal, the widget currently being built up is current.
   For dialogs, current may also point to a subwidget of the dialog.

   dialog contain diffent kinds of widgets:

   * data widgets which return some value these all have a name and store
   their value as data on the top-level dialog under their name

   * action widgets like buttons. These have a list of commands which are
   sent to fvwm or executed by FvwmGtk itself (eg close).
   The commands are stored as data on the action widget under the name
   "values", as a NULL-terminated char*-array.

   A command can contain references to other widgets values in the form
   $(name). These references are resolved recursively, ie the values can
   again contain references.

   The resolution of the references is done by splitting the string into a
   list of passive strings and variable references and splicing in sublists
   for the variable references until only passive strings remain.
 */


void destroy(int argc, char **argv)
{
	GtkWidget *w;

	g_return_if_fail(argc >= 1);

	w = g_hash_table_lookup(widgets, argv[0]);
	if (w != NULL)
	{
		if (gtk_widget_get_toplevel(current) == w)
		{
			current = NULL;
		}

		g_hash_table_remove(widgets, argv[0]);
		gtk_object_unref(GTK_OBJECT(w));
		gtk_widget_destroy(w);
	}
}


void separator(int argc, char **argv)
{
	if (GTK_IS_MENU(current))
	{
		menu_separator(argc, argv);
	}
	else
	{
		dialog_separator(argc, argv);
	}
}


void item(int argc, char **argv)
{
	if (GTK_IS_MENU(current))
	{
		  menu_item(argc, argv);
	}
	else
	{
		  dialog_option_menu_item(argc, argv);
	}
}


void parse_rc_file(int argc, char **argv)
{
	g_return_if_fail(argc >= 1);

	gtk_rc_parse(argv[0]);
}


void icon_size(int argc, char **argv)
{
	if (argc < 2)
	{
		icon_w = 0;
		icon_h = 0;
	}
	else
	{
		icon_w = atoi(argv[0]);
		icon_h = atoi(argv[1]);
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
	"WindowList",
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


void parse_arguments(char **line, int *argc, char ***argv)
{
	char *tmp[100];
	int i;

	for (i = 0; i < 100 ; i++)
	{
		*line = GetNextSimpleOption(*line, &tmp[i]);
		if (!tmp[i]) break;
	}
	*argc = i;

	*argv = (char **) safemalloc(i * sizeof(char *));
	for (i = 0; i < *argc; i++)
	{
		(*argv)[i] = tmp[i];
	}
}


void widget_not_found(char *name)
{
	GtkWidget *dialog, *box, *item;
	char buf[200];

	SendText(fvwm_fd, "Beep", 0);
	dialog = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_window_set_wmclass(GTK_WINDOW(dialog), "Error", "FvwmGtk");

	box = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(dialog), box);
	gtk_container_border_width(GTK_CONTAINER(dialog), 30);
	g_snprintf(buf, sizeof(buf), "No such menu or dialog: %s", name);
	item = gtk_label_new(buf);
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 5);
	item = gtk_button_new_with_label("OK");
	gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 5);
	gtk_signal_connect_object(
		GTK_OBJECT(item), "clicked",
		GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(box);
	gtk_widget_show(dialog);
}


void parse_config_line(char *buf)
{
	int argc;
	char **argv;
	char *p;
	char **e;

	if (buf[strlen(buf)-1] == '\n')
	{
		buf[strlen(buf)-1] = '\0';
	}
	if (strncasecmp(buf, my_name, my_name_len) == 0)
	{
		p = buf + my_name_len;
		if ((e = FindToken(p, table, char*)))
		{
			p += strlen(*e);
			parse_arguments(&p, &argc, &argv);
			handler[e - (char**)table](argc, argv);
		}
		else
		{
			fprintf(stderr, "%s: unknown command: %s\n",
				my_name + 1, buf);
		}
	}
	else if (strncasecmp(buf, "ImagePath", 9) == 0)
	{
		if (image_path)
		{
			free(image_path);
		}
		image_path = stripcpy(buf + 9);
	}
}


void parse_options(void)
{
	char *buf;

	InitGetConfigLine(fvwm_fd,my_name);   /* only my config lines needed */
	while (GetConfigLine(fvwm_fd, &buf), buf != NULL)
	{
		parse_config_line(buf);
	}
}


void process_message(
	unsigned long type, unsigned long timestamp, unsigned long *body)
{
	int button = 0;
	GtkWidget *widget;
	window_list_options *opts;

	char name[128];

	switch (type)
	{
	case M_STRING:
		SendUnlockNotification(fvwm_fd);
		context = body[0]; /* this is fw */
		sscanf((char*)(&body[3]), "%128s %d", name, &button);
		widget = g_hash_table_lookup(widgets, name);
		if (!widget)
		{
			widget_not_found(name);
		}
		else if (GTK_IS_MENU(widget))
		{
			opts = (window_list_options *)gtk_object_get_data(
				GTK_OBJECT(widget), "window_list");
			if (opts)
			{
				char *argv[1];

				argv[0] = name;
				destroy(1, argv);
				open_menu(1, argv);
				widget = current;
				gtk_object_set_data(
					GTK_OBJECT(current), "window_list",
					opts);
				construct_window_list();
			}

			gtk_menu_popup(
				GTK_MENU(widget), NULL, NULL, NULL, NULL,
				button, timestamp);
		}
		else if (GTK_IS_WINDOW(widget))
		{
			gtk_widget_show(GTK_WIDGET(widget));
		}
		break;
	case M_CONFIG_INFO:
		parse_config_line((char *)(&body[3]));
		break;
	case M_NEW_DESK:
		current_desk = (long)body[0];
		break;
	case M_ADD_WINDOW:
	case M_CONFIGURE_WINDOW:
		{
			struct ConfigWinPacket *cfg = (void *)body;
			window_list_entry *wle =
				lookup_window_list_entry(body[0]);

			wle->desk = cfg->desk;
			wle->layer = 0;
			wle->iconified = IS_ICONIFIED(cfg);
			wle->sticky = (IS_STICKY_ON_PAGE(cfg) ||
				       IS_STICKY_ON_DESK(cfg));
			wle->skip = DO_SKIP_WINDOW_LIST(cfg);
			wle->x = cfg->frame_x;
			wle->y = cfg->frame_y;
			wle->width = cfg->frame_width;
			wle->height = cfg->frame_height;
		}
		break;
	case M_DESTROY_WINDOW:
		g_hash_table_remove(window_list_entries, &(body[0]));
		break;
	case M_VISIBLE_NAME:
		{
			window_list_entry *wle =
				lookup_window_list_entry(body[0]);
			if (wle->name)
			{
				free(wle->name);
			}
			wle->name = safestrdup((char *)(&body[3]));
		}
		break;
	case MX_VISIBLE_ICON_NAME:
		{
			window_list_entry *wle =
				lookup_window_list_entry(body[0]);
			if (wle->icon_name)
			{
				free(wle->icon_name);
			}
			wle->icon_name = safestrdup((char*)(&body[3]));
		}
		break;
	case M_MINI_ICON:
		{

			MiniIconPacket *mip = (MiniIconPacket *)body;
			window_list_entry *wle =
				lookup_window_list_entry(mip->w);

			if (wle->mini_icon)
			{
				free(wle->mini_icon);
			}
			wle->mini_icon = mip->name[0] != '\0' ?
				safestrdup(mip->name) : NULL;
		}
		break;
	}
}


void read_fvwm_pipe(gpointer data, int source, GdkInputCondition cond)
{
	FvwmPacket* packet = ReadFvwmPacket(source);
	if (packet == NULL)
	{
		exit(0);
	}
	else
	{
		process_message(packet->type, packet->timestamp, packet->body);
	}
}

int main(int argc, char **argv)
{
	char *s;

	if ((s = strrchr(argv[0], '/')))
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
	my_name_len = strlen(s) + 1;
	my_name = safemalloc(my_name_len + 1);
	*my_name = '*';
	strcpy(my_name + 1, s);

	if ((argc != 6) && (argc != 7)) /* Now MyName is defined */
	{
		fprintf(stderr,
			"%s version %s should only be executed by fvwm!\n",
			my_name + 1, VERSION);
		exit(1);
	}

	fvwm_fd[0] = atoi(argv[1]);
	fvwm_fd[1] = atoi(argv[2]);

#ifdef GDK_IMLIB
	gdk_init(&argc, &argv);
	gdk_imlib_init();
	gtk_widget_push_visual(gdk_imlib_get_visual());
	gtk_widget_push_colormap(gdk_imlib_get_colormap());
#endif

#ifdef NEED_GNOMESUPPORT_H
	gnome_init("FvwmGtk", VERSION, argc, argv);
	gnome_client_disconnect(gnome_master_client());
#else
	gtk_init(&argc, &argv);
#endif

	widgets = g_hash_table_new(g_str_hash, g_str_equal);

	attach_to_toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_realize(attach_to_toplevel);

	window_list_entries = g_hash_table_new(g_int_hash, g_int_equal);

	parse_options();

	/* normal messages */
	SetMessageMask(
		fvwm_fd,
		M_STRING |
		M_CONFIG_INFO |
		M_SENDCONFIG |
		M_ADD_WINDOW |
		M_DESTROY_WINDOW |
		M_CONFIGURE_WINDOW |
		M_NEW_DESK |
		M_VISIBLE_NAME |
		M_MINI_ICON);
	/* extended messages */
	SetMessageMask(
		fvwm_fd,
		MX_VISIBLE_ICON_NAME);

	SendText(fvwm_fd, "Send_WindowList", 0);

	gtk_input_add_full(
		fvwm_fd[1], GDK_INPUT_READ | GDK_INPUT_EXCEPTION,
		read_fvwm_pipe, NULL, NULL, NULL);

	/* tell fvwm we're running */
	SendFinishedStartupNotification(fvwm_fd);

	/* tell fvwm we want to be lock on send for M_STRING Messages */
	SetSyncMask(fvwm_fd, M_STRING);

	gtk_main();
	return 0;
}

