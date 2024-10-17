/* -*-c-*- */
/*
 * FvwmRearrange.c -- fvwm module to arrange windows
 *
 * Copyright (C) 1996, 1997, 1998, 1999 Andrew T. Veliath
 *
 * Version 1.0
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 *
 * Combined FvwmTile and FvwmCascade to FvwmRearrange module.
 * 9-Nov-1998 Dominik Vogt
 */
#include "config.h"

#include "libs/ftime.h"
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h>
#endif

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <X11/Xlib.h>

#include "fvwm/fvwm.h"
#include "libs/log.h"
#include "libs/Module.h"
#include "libs/Parse.h"
#include "libs/System.h"
#include "libs/fvwmlib.h"
#include "libs/vpacket.h"

#define MIN_CELL_SIZE 10

enum window_order_methods
{
	WIN_ORDER_DEFAULT = 0,
	WIN_ORDER_NAME,
	WIN_ORDER_ICON,
	WIN_ORDER_CLASS,
	WIN_ORDER_RESOURCE,
	WIN_ORDER_XY,
	WIN_ORDER_YX,
	WIN_ORDER_HW,
	WIN_ORDER_WH,
};

typedef struct window_item
{
	Window		    w, frame;
	int		    x, y;
	int		    th, bw;
	int		    width, height;
	int		    min_w, min_h;
	int		    max_w, max_h;
	int		    base_w, base_h;
	int		    inc_w, inc_h;
	char		   *name;
	char		   *icon_name;
	char		   *class;
	char		   *resource;
	window_flags	    flags;
	struct window_item *prev, *next;
} window_item, *window_list;

/* vars */
Display		  *dpy;
static ModuleArgs *module;
int		   fd[2];
fd_set_size_t	   fd_width;
window_list	   wins = NULL, wins_tail = NULL;
int		   wins_count = 0;
FILE		  *console;

static void	   free_window_list(window_list);

/* Computation variables */
int box_x, box_y, box_w, box_h; /* Box dimensions. */
int usr_l = 0, usr_r = 0;	/* User adjustments to box. */
int usr_t = 0, usr_b = 0;
int inc_x = 0, inc_y = 0;	/* FvwmCascade increment sizes. */
int c_width = 0, c_height = 0;	/* FvwmCascade window sizes. */
int max_n	 = 0;		/* FvwmTile maximum rows or cols. */
int current_desk = 0;
int win_order = WIN_ORDER_DEFAULT;

/* Switches */
int FvwmTile	 = 1;
int FvwmCascade	 = 0;
int auto_tile	 = 1;
int horizontal	 = 0;
int fill_start	 = 0;
int fill_end	 = 0;
int inc_equal	 = 0;
int flatx	 = 0;
int flaty	 = 0;
int raise_window = 1;
int resize	 = 1;
int nostretchx	 = 0;
int nostretchy	 = 0;
int do_maximize	 = 0;
int do_animate	 = 0;
int skip_list	 = 0;
int sticky_page	 = 0;
int sticky_desk	 = 0;
int transients	 = 0;
int maximized	 = 1;
int titled	 = 1;
int desk	 = 0;
int reversed	 = 0;
int do_ewmhiwa	 = 0;

/* Monitor to act on. */
struct monitor *mon;
int is_global = 0;

void
free_resources(void)
{
	free_window_list(wins);

	if (console != stderr)
		fclose(console);
}

RETSIGTYPE
DeadPipe(int sig)
{
	free_resources();

	exit(0);
	SIGNAL_RETURN;
}

void
insert_window_list(window_item *wi_new)
{
	if (wins == NULL) {
		/* First item, initialize list. */
		wi_new->next = wi_new->prev = NULL;
		wins = wins_tail = wi_new;
	} else {
		/* Insert item at end. */
		wi_new->prev = wins_tail;
		wi_new->next = NULL;
		wins_tail->next = wi_new;
		wins_tail = wi_new;
	}
}

/* Returns negative if a < b, 0 if a = b, and positive if a > b. */
int
compare_window_items(window_item *a, window_item *b)
{
	switch (win_order)
	{
		case WIN_ORDER_NAME:
			return strcmp(a->name, b->name);
		case WIN_ORDER_ICON:
			return strcmp(a->icon_name, b->icon_name);
		case WIN_ORDER_CLASS:
			return strcmp(a->class, b->class);
		case WIN_ORDER_RESOURCE:
			return strcmp(a->resource, b->resource);
		case WIN_ORDER_XY:
			if (a->x < b->x || (a->x == b->x && a->y < b->y))
				return -1;
			else if (a->x == b->x && a->y == b->y)
				return 0;
			else
				return 1;
		case WIN_ORDER_YX:
			if (a->y < b->y || (a->y == b->y && a->x < b->x))
				return -1;
			else if (a->x == b->x && a->y == b->y)
				return 0;
			else
				return 1;
		case WIN_ORDER_HW:
			if (a->height < b->height || (a->height == b->height
			    && a->width < b->width))
				return -1;
			else if (a->height == b->height
				 && a->width == b->width)
				return 0;
			else
				return 1;
		case WIN_ORDER_WH:
			if (a->width < b->width || (a->width == b->width
			    && a->height < b->height))
				return -1;
			else if (a->width == b->width
				 && a->height == b->height)
				return 0;
			else
				return 1;
		default:
			/* Shouldn't happen. */
			return 0;
	}
}

void
sort_window_items(void)
{
	window_item *a, *b, *cur;

	/* Nothing to sort. */
	if (wins == NULL || win_order == WIN_ORDER_DEFAULT)
		return;

	/* Insertion short. Simple but not efficient, O(n^2),
	 * but window lists are small enough it shouldn't matter.
	 */
	cur = wins->next;
	while (cur) {
		a = cur->prev;
		b = cur;
		cur = cur->next;

		/* Find first element smaller than b */
		while (a && compare_window_items(a, b) > 0)
			a = a->prev;

		if (a == NULL) {
			/* Put b at start of list. */
			b->prev->next = b->next;
			if (b->next == NULL)
				wins_tail = b->prev;
			else
				b->next->prev = b->prev;
			b->prev = NULL;
			b->next = wins;
			wins->prev = b;
			wins = b;
		} else if (a->next != b) {
			/* Put b between a and a->next */
			b->prev->next = b->next;
			if (b->next == NULL)
				wins_tail = b->prev;
			else
				b->next->prev = b->prev;
			a->next->prev = b;
			b->next = a->next;
			b->prev = a;
			a->next = b;
		}
	}
}

void
free_window_list(window_list wl)
{
	window_item *q;

	while (wl) {
		q  = wl;
		wl = wl->next;
		free(q->name);
		free(q->icon_name);
		free(q->class);
		free(q->resource);
		free(q);
	}
}

int
is_suitable_window(unsigned long *body)
{
	XWindowAttributes	xwa;
	struct ConfigWinPacket *cfgpacket = (void *)body;
	struct monitor	       *m = monitor_by_output(cfgpacket->monitor_id);

	if (m == NULL) {
		fprintf(console, "FvwmRearrange: MONITOR WAS NULL\n");
		return 0;
	}
	if (!is_global && strcmp(mon->si->name, m->si->name) != 0)
		return 0;

	if (!skip_list && DO_SKIP_WINDOW_LIST(cfgpacket))
		return 0;

	if (!maximized && IS_MAXIMIZED(cfgpacket))
		return 0;

	if (!titled && !HAS_TITLE(cfgpacket))
		return 0;

	if (!sticky_page && IS_STICKY_ACROSS_PAGES(cfgpacket))
		return 0;

	if (!sticky_desk && IS_STICKY_ACROSS_DESKS(cfgpacket))
		return 0;

	if (!XGetWindowAttributes(dpy, cfgpacket->w, &xwa))
		return 0;

	if (xwa.map_state != IsViewable)
		return 0;

	if (!IS_MAPPED(cfgpacket))
		return 0;

	if (IS_ICONIFIED(cfgpacket))
		return 0;

	if (desk) {
		if ((int)cfgpacket->desk != current_desk)
			return 0;
	} else {
		/* Only select windows that intersect box. */
		int x = (int)cfgpacket->frame_x;
		int y = (int)cfgpacket->frame_y;
		int w = (int)cfgpacket->frame_width;
		int h = (int)cfgpacket->frame_height;
		if (x >= box_x + box_w || x + w <= box_x
		    || y >= box_y + box_h || y + h <= box_y)
			return 0;
	}

	if (!transients && IS_TRANSIENT(cfgpacket))
		return 0;

	return 1;
}

void
process_name(unsigned long type, unsigned long *body)
{
	window_item *wi = wins;

	while (wi != NULL && wi->w != body[0])
		wi = wi->next;

	if (wi == NULL)
		return;

	switch(type) {
		case M_RES_NAME:
			free(wi->resource);
			wi->resource = fxstrdup((char *)(&body[3]));
			break;
		case M_RES_CLASS:
			free(wi->class);
			wi->class = fxstrdup((char *)(&body[3]));
			break;
		case M_ICON_NAME:
			free(wi->icon_name);
			wi->icon_name = fxstrdup((char *)(&body[3]));
			break;
		case M_WINDOW_NAME:
			free(wi->name);
			wi->name = fxstrdup((char *)(&body[3]));
			break;
	}
}

int
get_window(void)
{
	FvwmPacket	       *packet;
	struct ConfigWinPacket *cfgpacket;
	int			last = 1;
	fd_set			infds;

	FD_ZERO(&infds);
	FD_SET(fd[1], &infds);
	select(fd_width, SELECT_FD_SET_CAST & infds, 0, 0, NULL);

	if ((packet = ReadFvwmPacket(fd[1])) == NULL)
		DeadPipe(0);
	else {
		cfgpacket = (struct ConfigWinPacket *)packet->body;
		switch (packet->type &= ~M_EXTENDED_MSG) {
		case M_CONFIGURE_WINDOW:
			if (is_suitable_window(packet->body)) {
				window_item *wi = fxmalloc(sizeof(window_item));
				wi->w		= cfgpacket->w;
				wi->x		= cfgpacket->frame_x;
				wi->y		= cfgpacket->frame_y;
				wi->frame	= cfgpacket->frame;
				wi->th		= cfgpacket->title_height;
				wi->bw		= cfgpacket->border_width;
				wi->width	= cfgpacket->frame_width;
				wi->height	= cfgpacket->frame_height;
				wi->base_w	= cfgpacket->hints_base_width;
				wi->base_h	= cfgpacket->hints_base_height;
				wi->min_w	= cfgpacket->hints_min_width;
				wi->min_h	= cfgpacket->hints_min_height;
				wi->max_w	= cfgpacket->hints_max_width;
				wi->max_h	= cfgpacket->hints_max_height;
				wi->inc_w	= cfgpacket->hints_width_inc;
				wi->inc_h	= cfgpacket->hints_height_inc;
				wi->flags	= cfgpacket->flags;
				wi->name	= NULL;
				wi->icon_name	= NULL;
				wi->class	= NULL;
				wi->resource	= NULL;
				insert_window_list(wi);
				++wins_count;
			}
			break;

		case M_RES_NAME:
		case M_RES_CLASS:
		case M_ICON_NAME:
		case M_WINDOW_NAME:
			process_name(packet->type, packet->body);
			break;

		case M_END_WINDOWLIST:
			last = 0;
			break;

		default:
			fprintf(console,
			    "%s: internal inconsistency: unknown message "
			    "0x%08x\n",
			    module->name, (int)packet->type);
			break;
		}
	}
	return last;
}

void
wait_configure(window_item *wi)
{
	int found = 0;

	/** Uh, what's the point of the select() here?? **/
	fd_set infds;
	FD_ZERO(&infds);
	FD_SET(fd[1], &infds);
	select(fd_width, SELECT_FD_SET_CAST & infds, 0, 0, NULL);

	while (!found) {
		FvwmPacket *packet = ReadFvwmPacket(fd[1]);
		if (packet == NULL)
			DeadPipe(0);
		if (packet->type == M_CONFIGURE_WINDOW
		    && (Window)(packet->body[1]) == wi->frame)
			found = 1;
	}
}

int
atopixel(char *s, int length)
{
	int l = strlen(s);

	if (l < 1)
		return 0;

	if (s[l - 1] == 'p') {
		char s2[24];
		strcpy(s2, s);
		s2[strlen(s2) - 1] = 0;
		return atoi(s2);
	}
	return (atoi(s) * length) / 100;
}

int
frame_size(window_item *wi, int is_width)
{
	int length = 2 * wi->bw;
	if ((is_width && HAS_VERTICAL_TITLE(wi))
	    || (!is_width && !HAS_VERTICAL_TITLE(wi)))
		length += wi->th;
	return length;
}

void
move_resize_raise_window(window_item *wi, int x, int y, int w, int h)
{
	static char msg[78];
	const char *ewmhiwa = do_ewmhiwa ? "ewmhiwa" : "";
	int orig_w = wi->width - frame_size(wi, 1);
	int orig_h = wi->height - frame_size(wi, 0);
	int do_wait = 1;

	if (x == wi->x && y == wi->y && (!resize ||
	    (w == orig_w && h == orig_h))) {
		/* Window is the same size/position. Do nothing. */
		do_wait = 0;
	} else if (resize) {
		const char *function =
		    do_maximize ? "ResizeMoveMaximize" : "ResizeMove";
		snprintf(msg, sizeof(msg), "%s %dp %dp %up %up %s",
		    function, w, h, x, y, ewmhiwa);
		SendText(fd, msg, wi->frame);
	} else {
		const char *function = do_maximize
					   ? "ResizeMoveMaximize keep keep"
				       : do_animate ? "AnimatedMove"
						    : "Move";
		snprintf(
		    msg, sizeof(msg), "%s %up %up %s", function, x, y, ewmhiwa);
		SendText(fd, msg, wi->frame);
	}

	if (raise_window)
		SendText(fd, "Raise", wi->frame);

	if (do_wait)
		wait_configure(wi);
}

int
get_hint_size(int val, int base, int min, int max, int inc)
{
	if (val < min)
		val = min;
	if (val > max)
		val = max;
	if (inc > 1) {
		val = base + ((val - base) / inc) * inc;
		if (val < min)
			val += inc;
	}

	return val;
}

void
size_hints_adjustment(window_item *wi, int *w, int *h)
{
	/* Subtract borders/title to get size of window. */
	*w -= frame_size(wi, 1);
	*h -= frame_size(wi, 0);

	if (HAS_OVERRIDE_SIZE_HINTS(wi))
		return;

	/* Apply size hints */
	*w = get_hint_size(*w, wi->base_w, wi->min_w, wi->max_w, wi->inc_w);
	*h = get_hint_size(*h, wi->base_h, wi->min_h, wi->max_h, wi->inc_h);
}

void
size_win_to_cell(window_item *wi, int x, int y, int cell_w, int cell_h,
		int extra_w, int extra_h)
{
		int w = cell_w, h = cell_h;

		if (x < extra_w) {
			w++;
			x = box_x + x * cell_w + x;
		} else {
			x = box_x + x * cell_w + extra_w;
		}
		if (y < extra_h) {
			h++;
			y = box_y + y * cell_h + y;
		} else {
			y = box_y + y * cell_h + extra_h;
		}

		if (nostretchx && w > wi->width)
			w = wi->width;
		if (nostretchy && h > wi->height)
			h = wi->height;

		size_hints_adjustment(wi, &w, &h);
		move_resize_raise_window(wi, x, y, w, h);
}

window_item
*fill_cells(window_item *wi, int n, int rows, int cols,
	int cell_w, int cell_h, int extra_w, int extra_h)
{
	int i, extra_wins, cell_l, extra_l;

	if (horizontal) {
		extra_wins = wins_count % rows;
		cell_l = box_h / extra_wins;
		extra_l = box_h % extra_wins;
	} else {
		extra_wins = wins_count % cols;
		cell_l = box_w / extra_wins;
		extra_l = box_w % extra_wins;
	}

	for (i = 0; i < extra_wins; i++) {
		if (horizontal)
			size_win_to_cell(wi, n, i,
				cell_w, cell_l, extra_w, extra_l);
		else
			size_win_to_cell(wi, i, n,
				cell_l, cell_h, extra_l, extra_h);

		wi = reversed ? wi->prev : wi->next;
	}
	return wi;
}

void
tile_windows(void)
{
	int cols = 1, rows = 1, cell_w, cell_h, extra_w, extra_h, n = 0;
	int x, y, extra_cells;
	window_item *wi = reversed ? wins_tail : wins;

	/* Compute grid dimensions. */
	if (auto_tile) {
		if (max_n > 0) {
			cols += max_n;
			if (cols > wins_count)
				cols = wins_count;
		}
		if (!fill_end)
			fill_start = 1;

		/* Loop to find the desired size. */
		while (cols * rows < wins_count) {
			cols++;
			if (cols * rows < wins_count)
				rows++;
		}
	} else if (max_n > 0 && max_n < wins_count) {
		rows = (wins_count - 1) / max_n + 1;
		cols = max_n;
	} else {
		cols = wins_count;
	}
	if (horizontal) { /* Swap rows and columns. */
		int tmp = cols;
		cols = rows;
		rows = tmp;
	}

	/* Compute single cell size. */
	cell_w  = box_w / cols;
	cell_h  = box_h / rows;
	/* Extra space to spread across initial cells. */
	extra_w = box_w % cols;
	extra_h = box_h % rows;

	/* Do nothing if cells are too small. */
	if (cell_w < MIN_CELL_SIZE || cell_h < MIN_CELL_SIZE) {
		fprintf(console,
			"FvwmRearrange: Cell size to small. Aborting!");
		free_resources();
		exit(1);
	}

	/* Fill empty cells at start of grid. */
	extra_cells = cols * rows - wins_count;
	if (fill_start && extra_cells > 0) {
		wi = fill_cells(wi, 0, rows, cols,
			cell_w, cell_h, extra_w, extra_h);
		n += horizontal ? rows : cols;
	}

	/* Loop through windows to compute their location and sizes. */
	while (wi) {
		if (horizontal) {
			x = n / rows;
			y = n % rows;
			if (fill_end && extra_cells > 0 && x + 1 == cols)
				break;
		} else {
			x = n % cols;
			y = n / cols;
			if (fill_end && extra_cells > 0 && y + 1 == rows)
				break;
		}
		size_win_to_cell(wi, x, y, cell_w, cell_h, extra_w, extra_h);

		wi = reversed ? wi->prev : wi->next;
		n++;
	}

	/* Fill empty cells at end of grid. */
	if (fill_end && extra_cells > 0) {
		n = horizontal ? cols - 1 : rows - 1;
		wi = fill_cells(wi, n, rows, cols, cell_w, cell_h, 0, 0);
	}
}

void
cascade_windows(void)
{
	int x = box_x, y = box_y, w, h;
	window_item *wi = reversed ? wins_tail : wins;

	while (wi) {
		if (resize) {
			/* Default size is 75% of the bounding box. */
			w = c_width > 0 ? c_width : 3 * box_w / 4;
			h = c_height > 0 ? c_height : 3 * box_h / 4;

			if (x + w > box_x + box_w)
				w = box_x + box_w - x;
			if (y + h > box_y + box_h)
				h = box_y + box_h - y;
			if (nostretchx && w > wi->width)
				w = wi->width;
			if (nostretchy && h > wi->height)
				h = wi->height;
			size_hints_adjustment(wi, &w, &h);
		}
		move_resize_raise_window(wi, x, y, w, h);

		/* Increment x and y to offset next window. */
		w = flatx ? inc_x : inc_x + frame_size(wi, 1) - wi->bw;
		h = flaty ? inc_y : inc_y + frame_size(wi, 0) - wi->bw;
		if (inc_equal) {
			if (h < w)
				h = w;
			else
				w = h;
		}
		x += w;
		y += h;
		wi = reversed ? wi->prev : wi->next;
	}
}

void
parse_args(int argc, char *argv[], int argi)
{
	int nsargc = 0;
	for (; argi < argc; ++argi) {
		/* Tiling options */
		if (StrEquals(argv[argi], "-auto_tile")) {
			FvwmTile    = 1;
			FvwmCascade = 0;
			auto_tile   = 1;
		} else if (StrEquals(argv[argi], "-tile")) {
			FvwmTile    = 1;
			FvwmCascade = 0;
			auto_tile   = 0;
		} else if (StrEquals(argv[argi], "-max_n")
			   && ((argi + 1) < argc)) {
			max_n = atoi(argv[++argi]);
		} else if (StrEquals(argv[argi], "-swap")
			   || StrEquals(argv[argi], "-h")) {
			horizontal = 1;
		} else if (StrEquals(argv[argi], "-fill_start")) {
			fill_start = 1;
		} else if (StrEquals(argv[argi], "-fill_end")) {
			fill_end = 1;

		/* Cascading options */
		} else if (StrEquals(argv[argi], "-cascade")) {
			FvwmCascade = 1;
			FvwmTile    = 0;
		} else if (StrEquals(argv[argi], "-cascadew")
			   && ((argi + 1) < argc)) {
			c_width = ++argi;
		} else if (StrEquals(argv[argi], "-cascadeh")
			   && ((argi + 1) < argc)) {
			c_height = ++argi;
		} else if (StrEquals(argv[argi], "-inc_equal")) {
			inc_equal = 1;
		} else if (StrEquals(argv[argi], "-incx")
			   && ((argi + 1) < argc)) {
			inc_x = ++argi;
		} else if (StrEquals(argv[argi], "-incy")
			   && ((argi + 1) < argc)) {
			inc_y = ++argi;
		} else if (StrEquals(argv[argi], "-flatx")) {
			flatx = 1;
		} else if (StrEquals(argv[argi], "-flaty")) {
			flaty = 1;

		/* General options */
		} else if (StrEquals(argv[argi], "-screen")
			   && ((argi + 1) < argc)) {
			mon = monitor_resolve_name(argv[++argi]);
			/* Ignore hints for global monitor. */
			if (mon == monitor_get_global())
				do_ewmhiwa = is_global = 1;
		} else if (StrEquals(argv[argi], "-noraise")) {
			raise_window = 0;
		} else if (StrEquals(argv[argi], "-maximize")) {
			do_maximize = 1;
		} else if (StrEquals(argv[argi], "-animate")) {
			do_animate = 1;
		} else if (StrEquals(argv[argi], "-ewmhiwa")) {
			do_ewmhiwa = 1;

		/* Resizing options */
		} else if (StrEquals(argv[argi], "-noresize")) {
			resize = 0;
		} else if (StrEquals(argv[argi], "-nostretch")) {
			nostretchx = nostretchy = 1;
		} else if (StrEquals(argv[argi], "-nostretchx")) {
			nostretchx = 1;
		} else if (StrEquals(argv[argi], "-nostretchy")) {
			nostretchy = 1;

		/* Filtering options */
		} else if (StrEquals(argv[argi], "-all")) {
			sticky_page = sticky_desk = transients = skip_list = 1;
		} else if (StrEquals(argv[argi], "-some")) {
			maximized = titled = 0;
		} else if (StrEquals(argv[argi], "-skiplist")) {
			skip_list = 1;
		} else if (StrEquals(argv[argi], "-sticky")) {
			sticky_page = sticky_desk = 1;
		} else if (StrEquals(argv[argi], "-sticky_page")) {
			sticky_page = 1;
		} else if (StrEquals(argv[argi], "-sticky_desk")) {
			sticky_desk = 1;
		} else if (StrEquals(argv[argi], "-transient")) {
			transients = 1;
		} else if (StrEquals(argv[argi], "-no_maximized")) {
			maximized = 0;
		} else if (StrEquals(argv[argi], "-no_titled")) {
			titled = 0;
		} else if (StrEquals(argv[argi], "-desk")) {
			desk = 1;
		} else if (StrEquals(argv[argi], "-ewmhiwa")) {
			do_ewmhiwa = 1;

		/* Window ordering */
		} else if (StrEquals(argv[argi], "-order_name")) {
			win_order = WIN_ORDER_NAME;
		} else if (StrEquals(argv[argi], "-order_icon")) {
			win_order = WIN_ORDER_ICON;
		} else if (StrEquals(argv[argi], "-order_class")) {
			win_order = WIN_ORDER_CLASS;
		} else if (StrEquals(argv[argi], "-order_resource")) {
			win_order = WIN_ORDER_RESOURCE;
		} else if (StrEquals(argv[argi], "-order_xy")) {
			win_order = WIN_ORDER_XY;
		} else if (StrEquals(argv[argi], "-order_yx")) {
			win_order = WIN_ORDER_YX;
		} else if (StrEquals(argv[argi], "-order_hw")) {
			win_order = WIN_ORDER_HW;
		} else if (StrEquals(argv[argi], "-order_wh")) {
			win_order = WIN_ORDER_WH;
		} else if (StrEquals(argv[argi], "-reverse")) {
			reversed = 1;

		/* Bounding box */
		} else {
			if (argv[argi][0] == '-' || ++nsargc > 4) {
				fprintf(console,
				    "%s: ignoring unknown arg %s\n",
				    module->name, argv[argi]);
				continue;
			}
			if (nsargc == 1) {
				usr_l = argi;
			} else if (nsargc == 2) {
				usr_t = argi;
			} else if (nsargc == 3) {
				usr_r = argi;
			} else if (nsargc == 4) {
				usr_b = argi;
			}
		}
	}
}

void
update_sizes(char *argv[])
{
	int left = 0, top = 0;

	/* Adjust box size first. */
	if (usr_l > 0)
		left = atopixel(argv[usr_l], box_w);
	if (usr_t > 0)
		top = atopixel(argv[usr_t], box_h);
	if (usr_r > 0)
		box_w = atopixel(argv[usr_r], box_w);
	if (usr_b > 0)
		box_h = atopixel(argv[usr_b], box_h);
	box_x += left;
	box_y += top;
	box_w -= left;
	box_h -= top;

	/* Cascade increments, based off size of resulting bounding box. */
	if (inc_x > 0)
		inc_x = atopixel(argv[inc_x], box_w);
	if (inc_y > 0)
		inc_y = atopixel(argv[inc_y], box_h);
	if (c_width > 0)
		c_width = atopixel(argv[c_width], box_w);
	if (c_height > 0)
		c_height = atopixel(argv[c_height], box_h);
}

int
main(int argc, char *argv[])
{
	char  match[128];
	char *config_line;

	console = fopen("/dev/console", "w");
	if (!console)
		console = stderr;

	module = ParseModuleArgs(argc, argv, 0);
	if (module == NULL) {
		fprintf(console,
		    "FvwmRearrange: module should be executed by fvwm only\n");
		exit(-1);
	}

	fd[0] = module->to_fvwm;
	fd[1] = module->from_fvwm;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(console, "%s: couldn't open display %s\n", module->name,
		    XDisplayName(NULL));
		exit(-1);
	}
	signal(SIGPIPE, DeadPipe);

	FScreenInit(dpy);
	fd_width = GetFdWidth();

	/* Need to parse args first so we know what monitor to use. */
	parse_args(module->user_argc, module->user_argv, 0);
	if (mon == NULL)
		mon = monitor_get_current();

	box_x = mon->si->x;
	box_y = mon->si->y;
	box_w = mon->si->w;
	box_h = mon->si->h;

	strcpy(match, "*");
	strcat(match, module->name);
	InitGetConfigLine(fd, match);
	for (GetConfigLine(fd, &config_line); config_line != NULL;
	     GetConfigLine(fd, &config_line)) {
		char *token, *next, *mname;
		int   dummy, bs_top, bs_bottom, bs_left, bs_right;

		token = PeekToken(config_line, &next);
		if (!StrEquals(token, "Monitor"))
			continue;

		config_line = GetNextToken(next, &mname);

		/* Skip unless we found the correct monitor or using global
		 * monitor for the full desk, so we need to set current_desk.
		 */
		if (!StrEquals(mname, mon->si->name)
		    && !(is_global && desk))
			continue;

		sscanf(config_line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d",
		    &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy,
		    &current_desk, &dummy, &dummy, &bs_left, &bs_right, &bs_top,
		    &bs_bottom);
		if (!do_ewmhiwa) {
			box_x += bs_left;
			box_y += bs_top;
			box_w -= bs_left + bs_right;
			box_h -= bs_top + bs_bottom;
		}
	}

	/* Update sizes now that monitor + working area dimensions are known */
	update_sizes(module->user_argv);

	SetMessageMask(fd,
		M_CONFIGURE_WINDOW
		| M_END_WINDOWLIST
		| M_WINDOW_NAME
		| M_ICON_NAME
		| M_RES_CLASS
		| M_RES_NAME
	);

	SendText(fd, "Send_WindowList", 0);

	/* Tell fvwm we're running */
	SendFinishedStartupNotification(fd);

	while (get_window()) /* */
		;

	sort_window_items();
	if (wins_count) {
		if (FvwmCascade)
			cascade_windows();
		else /* FvwmTile */
			tile_windows();
	}

	free_resources();

	return 0;
}
