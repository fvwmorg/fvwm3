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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include "FvwmIconMan.h"
#include "x.h"
#include "xmanager.h"

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/Module.h"
#include "libs/Parse.h"
#include "libs/Strings.h"

static WinData *fvwm_focus_win = NULL;

typedef struct {
	Ulong desknum;
} m_new_desk_data;

typedef struct {
	Ulong x, y, desknum;
} m_new_page_data;

typedef struct {
	Ulong app_id, frame_id, dbase_entry;
} m_minimal_data;

typedef struct {
	Ulong app_id, frame_id, dbase_entry;
	Ulong xpos, ypos, icon_width, icon_height;
} m_icon_data;

typedef struct {
	Ulong app_id, frame_id, dbase_entry;
	union {
		Ulong name_long[1];
		Uchar name[4];
	} name;
} m_name_data;

typedef struct {
	Ulong arg, toggle, id;
	Uchar str[4];
} m_property_data;

typedef struct {
	Ulong app_id, frame_id, dbase_entry;
	Ulong width, height, depth, picture, mask, alpha;
	union {
		Ulong name_long[1];
		Uchar name[4];
	} name;
} m_mini_icon_data;

typedef union {
	m_new_desk_data      new_desk_data;
	ConfigWinPacket      add_config_data;
	m_new_page_data      new_page_data;
	m_minimal_data       minimal_data;
	m_icon_data          icon_data;
	m_name_data          name_data;
	m_mini_icon_data     mini_icon_data;
	m_property_data      property_data;
} FvwmPacketBody;

/* only used by count_nonsticky_in_hashtab */
static WinManager *the_manager;

static int count_nonsticky_in_hashtab(void *arg)
{
	WinData *win = (WinData *)arg;
	WinManager *man = the_manager;

	if (!(IS_STICKY_ACROSS_DESKS(win) || IS_STICKY_ACROSS_PAGES(win)) &&
	    !(WINDATA_ICONIFIED(win) && (IS_ICON_STICKY_ACROSS_PAGES(win) ||
				 IS_ICON_STICKY_ACROSS_DESKS(win))) &&
	    win->complete && win->manager == man)
	{
		return 1;
	}

	return 0;
}

static void set_draw_mode(WinManager *man, int flag)
{
	int num;

	if (!man)
	{
		return;
	}

	if (man->we_are_drawing == 0 && flag)
	{
		draw_manager(man);
	}
	else if (man->we_are_drawing && !flag)
	{
		the_manager = man;
		num = accumulate_walk_hashtab(count_nonsticky_in_hashtab);
		ConsoleDebug(
			FVWM, "SetDrawMode on 0x%lx, num = %d\n",
			(unsigned long)man, num);

		if (num == 0)
		{
			return;
		}
		man->configures_expected = num;
	}
	man->we_are_drawing = flag;
}

static int drawing(WinManager *man)
{
	if (!man)
	{
		return 1;
	}

	return man->we_are_drawing;
}

static void got_configure(WinManager *man)
{
	if (man && !man->we_are_drawing)
	{
		man->configures_expected--;
		ConsoleDebug(
			FVWM, "got_configure on 0x%lx, num_expected now = %d\n",
			(unsigned long) man, man->configures_expected);
		if (man->configures_expected <= 0)
		{
			set_draw_mode(man, 1);
		}
	}
}

/* MMH <mikehan@best.com> 6/99
 * Following a gruesome hack this function has been hijacked.
 * The function is no longer about whether or not the window is in the
 * viewport, but whether or not the manager wants to display the window,
 * based on its current location.
 */
int win_in_viewport(WinData *win)
{
	return check_resolution(win->manager, win);
}


WinData *id_to_win(Ulong id)
{
	WinData *win;

	win = find_win_hashtab(id);
	if (win == NULL)
	{
		win = new_windata();
		win->app_id = id;
		win->app_id_set = 1;
		insert_win_hashtab(win);
	}

	return win;
}

static void set_win_configuration(WinData *win, FvwmPacketBody *body)
{
	win->desknum = body->add_config_data.desk;
	win->x = body->add_config_data.frame_x;
	win->y = body->add_config_data.frame_y;
	win->width = body->add_config_data.frame_width;
	win->height = body->add_config_data.frame_height;
	win->iconified =  IS_ICONIFIED(&(body->add_config_data));
	if (win == fvwm_focus_win)
	{
		win->state = FOCUS_CONTEXT;
	}
	else if (win->iconified)
	{
		win->state = ICON_CONTEXT;
	}
	else
	{
		win->state = PLAIN_CONTEXT;
	}
	win->geometry_set = 1;
	memcpy(&(win->flags), &(body->add_config_data.flags),
		sizeof(win->flags));
	{
		int wb_x = body->add_config_data.border_width;
		int wb_y = body->add_config_data.border_width;
		int t_w = 2*wb_x;
		int t_h = 2*wb_y;

		if (HAS_TITLE_DIR(win, DIR_N))
		{
			wb_y += body->add_config_data.title_height;
			t_h += body->add_config_data.title_height;
		}
		else if (HAS_TITLE_DIR(win, DIR_W))
		{
			wb_x += body->add_config_data.title_height;
			t_w += body->add_config_data.title_height;
		}
		if (HAS_TITLE_DIR(win, DIR_S))
		{
			t_h += body->add_config_data.title_height;
		}
		if (HAS_TITLE_DIR(win, DIR_E))
		{
			t_w += body->add_config_data.title_height;
		}
		win->real_g.x = win->x + wb_x;
		win->real_g.y = win->y + wb_y;
		win->real_g.width = win->width - t_w;
		win->real_g.height = win->height - t_h;
	}
}

static void handle_config_info(unsigned long *body)
{
	char *tline, *token, *rest;
	int color;
	extern void process_dynamic_config_line(char *line);

	tline = (char*)&(body[3]);
	token = PeekToken(tline, &rest);
	if (StrEquals(token, "Colorset"))
	{
		color = LoadColorset(rest);
		change_colorset(color);
	}
	else if (StrEquals(token, XINERAMA_CONFIG_STRING))
	{
		FScreenConfigureModule(rest);
	}
	else if (StrEquals(token, "IgnoreModifiers"))
	{
		sscanf(rest, "%d", &mods_unused);
	}
	else if (strncasecmp(tline, Module, ModuleLen) == 0)
	{
		process_dynamic_config_line(tline);
	}
}

static void configure_window(FvwmPacketBody *body)
{
	Ulong app_id = body->add_config_data.w;
	WinData *win;
	ConsoleDebug(FVWM, "configure_window: %ld\n", app_id);

	win = id_to_win(app_id);

	set_win_configuration(win, body);

	check_win_complete(win);
	check_in_window(win);
	got_configure(win->manager);
}

static void focus_change(FvwmPacketBody *body)
{
	Ulong app_id = body->minimal_data.app_id;
	WinData *win = id_to_win(app_id);

	ConsoleDebug(FVWM, "Focus Change\n");
	ConsoleDebug(FVWM, "\tID: %ld\n", app_id);

	if (fvwm_focus_win &&
	    fvwm_focus_win->button &&
	    fvwm_focus_win->manager->showonlyfocused)
	  delete_windows_button(fvwm_focus_win);

	if (fvwm_focus_win && win != fvwm_focus_win)
	{
		del_win_state(fvwm_focus_win, FOCUS_CONTEXT);
		if (fvwm_focus_win->manager &&
			fvwm_focus_win->manager->focus_button)
		{
			fvwm_focus_win->manager->focus_button = NULL;
		}
		fvwm_focus_win = NULL;
	}

	if (win->complete &&
		win->button &&
		win->manager &&
		win->manager->window_up &&
		win->manager->followFocus)
	{
		win->manager->focus_button = win->button;
	}
	add_win_state(win, FOCUS_CONTEXT);

	check_in_window(win);

	globals.focus_win = win;
	fvwm_focus_win = win;
	ConsoleDebug(FVWM, "leaving focus_change\n");
}

static void res_name(FvwmPacketBody *body)
{
	Ulong app_id = body->name_data.app_id;
	Uchar *name = body->name_data.name.name;
	WinData *win;

	ConsoleDebug(FVWM, "In res_name\n");

	win = id_to_win(app_id);

	copy_string(&win->resname, (char *)name);
	change_windows_manager(win);

	ConsoleDebug(FVWM, "Exiting res_name\n");
}

static void class_name(FvwmPacketBody *body)
{
	Ulong app_id = body->name_data.app_id;
	Uchar *name = body->name_data.name.name;
	WinData *win;

	ConsoleDebug(FVWM, "In class_name\n");

	win = id_to_win(app_id);

	copy_string(&win->classname, (char *)name);
	change_windows_manager(win);

	ConsoleDebug(FVWM, "Exiting class_name\n");
}

static void icon_name(FvwmPacketBody *body)
{
	WinData *win;
	Ulong app_id;
	Uchar *name = body->name_data.name.name;

	ConsoleDebug(FVWM, "In icon_name\n");

	app_id = body->name_data.app_id;

	win = id_to_win(app_id);

	if (win->iconname && !strcmp(win->iconname, (char *)name))
	{
		ConsoleDebug(
			FVWM, "No icon change: %s %s\n", win->iconname, name);
		return;
	}

	copy_string(&win->iconname, (char *)name);
	ConsoleDebug(FVWM, "new icon name: %s\n", win->iconname);

	ConsoleDebug(FVWM, "Exiting icon_name\n");
}

static void visible_icon_name(FvwmPacketBody *body)
{
	WinData *win;
	Ulong app_id;
	Uchar *name = body->name_data.name.name;

	ConsoleDebug(FVWM, "In visible_icon_name\n");

	app_id = body->name_data.app_id;

	win = id_to_win(app_id);

	if (win->visible_icon_name &&
		!strcmp(win->visible_icon_name, (char *)name))
	{
		ConsoleDebug(
			FVWM, "No icon change: %s %s\n", win->iconname, name);
		return;
	}

	copy_string(&win->visible_icon_name, (char *)name);
	ConsoleDebug(
		FVWM, "new visible icon name: %s\n", win->visible_icon_name);
	if (change_windows_manager(win) == 0 && win->button &&
		(win->manager->format_depend & ICON_NAME))
	{
		if (win->manager->sort)
		{
			resort_windows_button(win);
		}
	}

	ConsoleDebug(FVWM, "Exiting icon_name\n");
}

static void window_name(FvwmPacketBody *body)
{
	WinData *win;
	Ulong app_id;
	Uchar *name = body->name_data.name.name;

	ConsoleDebug(FVWM, "In window_name\n");

	app_id = body->name_data.app_id;

	win = id_to_win(app_id);

	/* This is necessary because bash seems to update the window title on
	   every keystroke regardless of whether anything changes */
	if (win->titlename && !strcmp(win->titlename, (char *)name))
	{
		ConsoleDebug(
			FVWM, "No name change: %s %s\n", win->titlename, name);
		return;
	}

	copy_string(&win->titlename, (char *)name);

	ConsoleDebug(FVWM, "Exiting window_name\n");
}

static void visible_name(FvwmPacketBody *body)
{
	WinData *win;
	Ulong app_id;
	Uchar *name = body->name_data.name.name;

	ConsoleDebug(FVWM, "In visible_name\n");

	app_id = body->name_data.app_id;

	win = id_to_win(app_id);

	/* This is necessary because bash seems to update the window title on
	   every keystroke regardless of whether anything changes */
	if (win->visible_name && !strcmp(win->visible_name, (char *)name))
	{
		ConsoleDebug(
			FVWM, "No visible name change: %s %s\n",
			win->visible_name, name);
		return;
	}

	copy_string(&win->visible_name, (char *)name);
	if (change_windows_manager(win) == 0 && win->button &&
		(win->manager->format_depend & TITLE_NAME))
	{
		if (win->manager->sort)
		{
			resort_windows_button(win);
		}
	}
	ConsoleDebug(FVWM, "Exiting visible_name\n");
}


static void new_window(FvwmPacketBody *body)
{
	WinData *win;

	win = id_to_win(body->add_config_data.w);
	memcpy(&(win->flags), &(body->add_config_data.flags), sizeof(win->flags));
	set_win_configuration(win, body);
	got_configure(win->manager);
	check_win_complete(win);
	check_in_window(win);
	/* FIXME: not perfect, the manager is not know yet */
	tips_cancel(win->manager);
}

static void destroy_window(FvwmPacketBody *body)
{
	WinData *win;
	Ulong app_id;

	app_id = body->minimal_data.app_id;
	win = id_to_win(app_id);
	if (win->manager)
	{
		tips_cancel(win->manager);
	}
	if (win == globals.focus_win)
	{
		globals.focus_win = NULL;
	}
	delete_win_hashtab(win);
	if (win->button)
	{
		ConsoleDebug(FVWM, "destroy_window: deleting windows_button\n");
		delete_windows_button(win);
	}
	if (win == fvwm_focus_win)
	{
		fvwm_focus_win = NULL;
	}
	free_windata(win);
}

static void mini_icon(FvwmPacketBody *body)
{
	Ulong app_id = body->mini_icon_data.app_id;
	WinData *win;

	if (!FMiniIconsSupported)
	{
		return;
	}
	win = id_to_win(app_id);
	set_win_picture(
		win, body->mini_icon_data.picture, body->mini_icon_data.mask,
		body->mini_icon_data.alpha, body->mini_icon_data.depth,
		body->mini_icon_data.width, body->mini_icon_data.height);


	ConsoleDebug(
		FVWM, "mini_icon: 0x%lx 0x%lx %dx%dx%d\n",
		(unsigned long) win->pic.picture, (unsigned long) win->pic.mask,
		win->pic.width, win->pic.height, win->pic.depth);
}

static void icon_location(FvwmPacketBody *body)
{
	Ulong app_id = body->minimal_data.app_id;
	WinData *win;

	win = id_to_win(app_id);
	if (win == NULL)
	{
		return;
	}
	win->icon_g.x = body->icon_data.xpos;
	win->icon_g.y = body->icon_data.ypos;
	win->icon_g.width = body->icon_data.icon_width;
	if (win->icon_g.width <= 0)
	{
		win->icon_g.width = 1;
	}
	win->icon_g.height = body->icon_data.icon_height;
	if (win->icon_g.height <= 0)
	{
		win->icon_g.height = 1;
	}
	check_in_window(win);
}

static void iconify(FvwmPacketBody *body, int dir)
{
	Ulong app_id = body->minimal_data.app_id;
	WinData *win;

	win = id_to_win(app_id);
	set_win_iconified(win, dir);
	icon_location(body);
	check_win_complete(win);
	check_in_window(win);
}

static void window_shade(FvwmPacketBody *body, int do_shade)
{
	Window w;
	WinManager *man;

	w = (Window)(body->minimal_data.app_id);
	man = find_windows_manager(w);
	if (man == NULL)
	{
		return;
	}
	if (do_shade)
	{
		man->flags.needs_resize_after_unshade = 0;
		man->flags.is_shaded = 1;
	}
	else
	{
		man->flags.is_shaded = 0;
		if (man->flags.needs_resize_after_unshade)
		{
			draw_manager(man);
			man->flags.needs_resize_after_unshade = 0;
		}
	}

	return;
}

/* only used by remanage_winlist */

static void update_win_in_hashtab(void *arg)
{
	WinData *p = (WinData *)arg;
	check_in_window(p);
}

void remanage_winlist(void)
{
	walk_hashtab(update_win_in_hashtab);
	draw_managers();
}

static void new_desk(FvwmPacketBody *body)
{
	globals.desknum = body->new_desk_data.desknum;
	remanage_winlist();
}

static void sendtomodule(FvwmPacketBody *body)
{
	extern void execute_function(char *);
	Uchar *string = body->name_data.name.name;

	ConsoleDebug(FVWM, "Got string: %s\n", string);

	execute_function((char *)string);
}

static void property_change(FvwmPacketBody *body)
{
	WinManager *man = NULL;
	int j;

	if (body->property_data.id != 0)
	{
		if (!(man = find_windows_manager(body->property_data.id)))
			return;
	}

	if (body->property_data.arg == MX_PROPERTY_CHANGE_BACKGROUND)
	{
		if (man != NULL &&
		    CSET_IS_TRANSPARENT_PR(man->colorsets[DEFAULT]))
		{
			recreate_background(man, DEFAULT);
			force_manager_redraw(man);
		}
		if (man != NULL)
		{
			return;
		}
		for (j = 0; j < globals.num_managers; j++)
		{
			man = &globals.managers[j];
			if (CSET_IS_TRANSPARENT_PR(man->colorsets[DEFAULT]))
			{
				recreate_background(man, DEFAULT);
				force_manager_redraw(man);
			}
		}
	}
	else if (body->property_data.arg == MX_PROPERTY_CHANGE_SWALLOW &&
		man != NULL)
	{
		man->swallowed = body->property_data.toggle;
		if (man->swallowed)
		{
			unsigned long u;

			if (sscanf((char *)body->property_data.str, "%lu",
				   &u) == 1)
			{
				man->swallower_win = (Window)u;
			}
			else
			{
				man->swallower_win = 0;
			}
		}
	}
}

static void ProcessMessage(Ulong type, FvwmPacketBody *body)
{
	int i;

	ConsoleDebug(FVWM, "fvwm message type: %ld\n", type);

	switch(type)
	{
	case M_CONFIG_INFO:
		ConsoleDebug(FVWM, "DEBUG::M_CONFIG_INFO\n");
		handle_config_info((unsigned long*)body);
		break;

	case M_CONFIGURE_WINDOW:
		ConsoleDebug(FVWM, "DEBUG::M_CONFIGURE_WINDOW\n");
		configure_window(body);
		break;

	case M_FOCUS_CHANGE:
		ConsoleDebug(FVWM, "DEBUG::M_FOCUS_CHANGE\n");
		focus_change(body);
		break;

	case M_RES_NAME:
		ConsoleDebug(FVWM, "DEBUG::M_RES_NAME\n");
		res_name(body);
		break;

	case M_RES_CLASS:
		ConsoleDebug(FVWM, "DEBUG::M_RES_CLASS\n");
		class_name(body);
		break;

	case M_MAP:
		ConsoleDebug(FVWM, "DEBUG::M_MAP\n");
		break;

	case M_ADD_WINDOW:
		ConsoleDebug(FVWM, "DEBUG::M_ADD_WINDOW\n");
		new_window(body);
		break;

	case M_DESTROY_WINDOW:
		ConsoleDebug(FVWM, "DEBUG::M_DESTROY_WINDOW\n");
		destroy_window(body);
		break;

	case M_MINI_ICON:
		ConsoleDebug(FVWM, "DEBUG::M_MINI_ICON\n");
		mini_icon(body);
		break;

	case M_WINDOW_NAME:
		ConsoleDebug(FVWM, "DEBUG::M_WINDOW_NAME\n");
		window_name(body);
		break;

	case M_VISIBLE_NAME:
		ConsoleDebug(FVWM, "DEBUG::M_VISIBLE_NAME\n");
		visible_name(body);
		break;

	case M_ICON_NAME:
		ConsoleDebug(FVWM, "DEBUG::M_ICON_NAME\n");
		icon_name(body);
		break;

	case MX_VISIBLE_ICON_NAME:
		ConsoleDebug(FVWM, "DEBUG::MX_VISIBLE_ICON_NAME\n");
		visible_icon_name(body);
		break;

	case M_DEICONIFY:
		ConsoleDebug(FVWM, "DEBUG::M_DEICONIFY\n");
		iconify(body, 0);
		SendUnlockNotification(fvwm_fd);
		break;

	case M_ICONIFY:
		ConsoleDebug(FVWM, "DEBUG::M_ICONIFY\n");
		iconify(body, 1);
		SendUnlockNotification(fvwm_fd);
		break;

	case M_ICON_LOCATION:
		icon_location(body);
		break;

	case M_END_WINDOWLIST:
		ConsoleDebug(FVWM, "DEBUG::M_END_WINDOWLIST\n");
		ConsoleDebug(
			FVWM, "+++++ End window list +++++\n");
		if (globals.focus_win && globals.focus_win->button)
		{
			globals.focus_win->manager->focus_button =
				globals.focus_win->button;
		}
		globals.got_window_list = 1;
		for (i = 0; i < globals.num_managers; i++)
		{
			create_manager_window(i);
		}
	  break;

	case M_NEW_DESK:
		ConsoleDebug(FVWM, "DEBUG::M_NEW_DESK\n");
		new_desk(body);
		break;

	case M_NEW_PAGE:
		ConsoleDebug(FVWM, "DEBUG::M_NEW_PAGE\n");
		if (globals.x == body->new_page_data.x &&
			globals.y == body->new_page_data.y &&
			globals.desknum == body->new_page_data.desknum)
		{
			ConsoleDebug(FVWM, "Useless NEW_PAGE received\n");
			break;
		}
		globals.x = body->new_page_data.x;
		globals.y = body->new_page_data.y;
		globals.desknum = body->new_page_data.desknum;
		if (fvwm_focus_win && fvwm_focus_win->manager &&
			fvwm_focus_win->manager->followFocus)
		{
			/* need to set the focus on a page change */
			add_win_state(fvwm_focus_win, FOCUS_CONTEXT);
		}
		for (i = 0; i < globals.num_managers; i++)
		{
			set_draw_mode(&globals.managers[i], 0);
		}
		break;

	case M_STRING:
		ConsoleDebug(FVWM, "DEBUG::M_STRING\n");
		sendtomodule(body);
		break;

	case MX_PROPERTY_CHANGE:
		ConsoleDebug(FVWM, "DEBUG::MX_PROPERTY_CHANGE\n");
		property_change(body);
		break;

	case M_WINDOWSHADE:
		window_shade(body, 1);
		break;

	case M_DEWINDOWSHADE:
		window_shade(body, 0);
		break;

	default:
		break;
	}

	check_managers_consistency();

	for (i = 0; i < globals.num_managers; i++)
	{
		if (drawing(&globals.managers[i]))
		{
			draw_manager(&globals.managers[i]);
		}
	}

	check_managers_consistency();
}

void ReadFvwmPipe(void)
{
	FvwmPacket* packet;

	PrintMemuse();
	ConsoleDebug(FVWM, "DEBUG: entering ReadFvwmPipe\n");

	if ((packet = ReadFvwmPacket(fvwm_fd[1])) == NULL)
	{
		exit(0);
	}
	else
	{
		ProcessMessage(packet->type, (FvwmPacketBody *)packet->body);
	}

	ConsoleDebug(FVWM, "DEBUG: leaving ReadFvwmPipe\n");
}
