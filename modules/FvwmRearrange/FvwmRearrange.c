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
#include "libs/Module.h"
#include "libs/Parse.h"
#include "libs/System.h"
#include "libs/fvwmlib.h"
#include "libs/vpacket.h"

typedef struct window_item
{
	Window		    frame;
	int		    x, y;
	int		    th, bw;
	int		    width, height;
	int		    min_w, min_h;
	int		    max_w, max_h;
	int		    base_w, base_h;
	int		    inc_w, inc_h;
	struct window_item *prev, *next;
} window_item, *window_list;

/* vars */
Display		  *dpy;
int		   dx, dy;
int		   dwidth, dheight;
static ModuleArgs *module;
int		   fd[2];
fd_set_size_t	   fd_width;
window_list	   wins = NULL, wins_tail = NULL;
int		   wins_count = 0;
FILE		  *console;

static void	   Loop(int *);
static void	   process_message(unsigned long, unsigned long *);
static void	   free_window_list(window_list);

/* switches */
int ofsx = 0, ofsy = 0;
int maxw = 0, maxh = 0;
int maxx, maxy;
int untitled = 0, transients = 0;
int maximized	 = 0;
int all		 = 0;
int desk	 = 0;
int current_desk = 0;
int reversed = 0, raise_window = 1;
int resize	= 0;
int nostretch	= 0;
int sticky_page = 0;
int sticky_desk = 0;
int flatx = 0, flaty = 0;
int incx = 0, incy = 0;
int horizontal = 0;
int maxnum     = 0;

int do_maximize = 0;
int do_animate	= 0;
int do_ewmhiwa	= 0;
int is_global	= 0;

int FvwmTile	= 0;
int FvwmCascade = 1;

char	       *monitor_name = NULL;
struct monitor *mon;

RETSIGTYPE
DeadPipe(int sig)
{
	free_window_list(wins);

	if (console != stderr)
		fclose(console);

	free(monitor_name);

	exit(0);
	SIGNAL_RETURN;
}

RETSIGTYPE
HandleAlarm(int sig)
{
	DeadPipe(sig);
}

void
insert_window_list(window_list *wl, window_item *i)
{
	if (*wl) {
		if ((i->prev = (*wl)->prev))
			i->prev->next = i;
		i->next	    = *wl;
		(*wl)->prev = i;
	} else
		i->next = i->prev = NULL;
	*wl = i;
}

void
free_window_list(window_list wl)
{
	window_item *q;

	while (wl) {
		q  = wl;
		wl = wl->next;
		free(q);
	}
}

int
is_suitable_window(unsigned long *body)
{
	XWindowAttributes	xwa;
	struct ConfigWinPacket *cfgpacket = (void *)body;
	struct monitor	       *m = monitor_by_output(cfgpacket->monitor_id);
	char		       *m_name;

	if (m == NULL) {
		fprintf(stderr, "FvwmRearrange: MONITOR WAS NULL\n");
		return 0;
	}
	m_name = (char *)m->si->name;

	if ((DO_SKIP_WINDOW_LIST(cfgpacket)) && !all)
		return 0;

	if ((IS_MAXIMIZED(cfgpacket)) && !maximized)
		return 0;

	if ((IS_STICKY_ACROSS_PAGES(cfgpacket)) && !sticky_page)
		return 0;

	if ((IS_STICKY_ACROSS_DESKS(cfgpacket)) && !sticky_desk)
		return 0;

	if (!XGetWindowAttributes(dpy, cfgpacket->w, &xwa))
		return 0;

	if (xwa.map_state != IsViewable)
		return 0;

	if (!(IS_MAPPED(cfgpacket)))
		return 0;

	if (IS_ICONIFIED(cfgpacket))
		return 0;

	if (desk) {
		if (!is_global && (int)cfgpacket->desk != current_desk)
			return 0;
	} else {
		int x = (int)cfgpacket->frame_x, y = (int)cfgpacket->frame_y;
		int w = (int)cfgpacket->frame_width,
		    h = (int)cfgpacket->frame_height;
		if (x >= dx + dwidth || y >= dy + dheight || x + w <= dx
		    || y + h <= dy)
			return 0;
	}

	if (!(HAS_TITLE(cfgpacket)) && !untitled)
		return 0;

	if ((IS_TRANSIENT(cfgpacket)) && !transients)
		return 0;

	if (!is_global && strcmp(mon->si->name, m_name) != 0)
		return 0;

	return 1;
}

void
get_window(unsigned long *body)
{
	struct ConfigWinPacket *cfgpacket = (void *)body;

	if (is_suitable_window(body)) {
		window_item *wi = fxmalloc(sizeof(window_item));
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
		if (!wins_tail)
			wins_tail = wi;
		insert_window_list(&wins, wi);
		++wins_count;
	}
}

int
atopixel(char *s, unsigned long f)
{
	int l = strlen(s);
	if (l < 1)
		return 0;
	if (isalpha(s[l - 1])) {
		char s2[24];
		strcpy(s2, s);
		s2[strlen(s2) - 1] = 0;
		return atoi(s2);
	}
	return (atoi(s) * f) / 100;
}

void
move_resize_raise_window(window_item *wi, int x, int y, int w, int h)
{
	static char msg[78];
	const char *ewmhiwa = do_ewmhiwa ? "ewmhiwa" : "";
	int orig_w = wi->width - 2 * wi->bw;
	int orig_h = wi->height - 2 * wi->bw - wi->th;

	if (x == wi->x && y == wi->y && (!resize ||
	    (w == orig_w && h == orig_h))) {
		/* Window is in the same. Do nothing. */
		return;
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
	/* Apply size hints */
	*w = get_hint_size(*w, wi->base_w, wi->min_w, wi->max_w, wi->inc_w);
	*h = get_hint_size(*h, wi->base_h, wi->min_h, wi->max_h, wi->inc_h);
}

void
tile_windows(void)
{
	int	     cur_x = ofsx, cur_y = ofsy;
	int	     final_w = -1, final_h = -1;
	int	     wdiv, hdiv, i, j, count = 1;
	window_item *w = reversed ? wins_tail : wins;

	if (horizontal) {
		if ((maxnum > 0) && (maxnum < wins_count)) {
			count = wins_count / maxnum;
			if (wins_count % maxnum)
				++count;
			hdiv = (maxy - ofsy + 1) / maxnum;
		} else {
			maxnum = wins_count;
			hdiv   = (maxy - ofsy + 1) / wins_count;
		}
		wdiv = (maxx - ofsx + 1) / count;

		for (i = 0; w && (i < count); ++i) {
			for (j = 0; w && (j < maxnum); ++j) {
				int nw = wdiv - w->bw * 2;
				int nh = hdiv - w->bw * 2 - w->th;
				size_hints_adjustment(w, &nw, &nh);

				if (resize) {
					if (nostretch) {
						if (nw > w->width)
							nw = w->width;
						if (nh > w->height)
							nh = w->height;
					}
					final_w = (nw > 0) ? nw : w->width;
					final_h = (nh > 0) ? nh : w->height;
				}
				move_resize_raise_window(
				    w, cur_x, cur_y, final_w, final_h);

				cur_y += hdiv;
				w = reversed ? w->prev : w->next;
			}
			cur_x += wdiv;
			cur_y = ofsy;
		}
	} else {
		if ((maxnum > 0) && (maxnum < wins_count)) {
			count = wins_count / maxnum;
			if (wins_count % maxnum)
				++count;
			wdiv = (maxx - ofsx + 1) / maxnum;
		} else {
			maxnum = wins_count;
			wdiv   = (maxx - ofsx + 1) / wins_count;
		}
		hdiv = (maxy - ofsy + 1) / count;

		for (i = 0; w && (i < count); ++i) {
			for (j = 0; w && (j < maxnum); ++j) {
				int nw = wdiv - w->bw * 2;
				int nh = hdiv - w->bw * 2 - w->th;
				size_hints_adjustment(w, &nw, &nh);

				if (resize) {
					if (nostretch) {
						if (nw > w->width)
							nw = w->width;
						if (nh > w->height)
							nh = w->height;
					}
					final_w = (nw > 0) ? nw : w->width;
					final_h = (nh > 0) ? nh : w->height;
				}
				move_resize_raise_window(
				    w, cur_x, cur_y, final_w, final_h);

				cur_x += wdiv;
				w = reversed ? w->prev : w->next;
			}
			cur_x = ofsx;
			cur_y += hdiv;
		}
	}
}

void
cascade_windows(void)
{
	int	     cur_x = ofsx, cur_y = ofsy;
	int	     final_w = -1, final_h = -1;
	window_item *w = reversed ? wins_tail : wins;
	while (w) {
		unsigned long nw = 0, nh = 0;
		if (resize) {
			if (nostretch) {
				if (maxw && (w->width > maxw))
					nw = maxw;
				if (maxh && (w->height > maxh))
					nh = maxh;
			} else {
				nw = maxw;
				nh = maxh;
			}
			if (nw || nh) {
				final_w = nw ? nw : w->width;
				final_h = nh ? nh : w->height;
			}
		}
		move_resize_raise_window(w, cur_x, cur_y, final_w, final_h);

		if (!flatx)
			cur_x += w->bw;
		cur_x += incx;
		if (!flaty)
			cur_y += w->bw + w->th;
		cur_y += incy;
		w = reversed ? w->prev : w->next;
	}
}

void
parse_args(int argc, char *argv[], int argi)
{
	int nsargc = 0;
	/* parse args */
	for (; argi < argc; ++argi) {
		if (!strcmp(argv[argi], "-tile")) {
			FvwmTile    = 1;
			FvwmCascade = 0;
			resize	    = 1;
		} else if (!strcmp(argv[argi], "-cascade")) {
			FvwmCascade = 1;
			FvwmTile    = 0;
		} else if (!strcmp(argv[argi], "-u")) {
			untitled = 1;
		} else if (!strcmp(argv[argi], "-t")) {
			transients = 1;
		} else if (!strcmp(argv[argi], "-a")) {
			all = untitled = transients = maximized = 1;
			if (FvwmCascade) {
				sticky_page = 1;
				sticky_desk = 1;
			}
		} else if (!strcmp(argv[argi], "-r")) {
			reversed = 1;
		} else if (!strcmp(argv[argi], "-noraise")) {
			raise_window = 0;
		} else if (!strcmp(argv[argi], "-noresize")) {
			resize = 0;
		} else if (!strcmp(argv[argi], "-nostretch")) {
			nostretch = 1;
		} else if (!strcmp(argv[argi], "-desk")) {
			desk = 1;
		} else if (!strcmp(argv[argi], "-flatx")) {
			flatx = 1;
		} else if (!strcmp(argv[argi], "-flaty")) {
			flaty = 1;
		} else if (!strcmp(argv[argi], "-r")) {
			reversed = 1;
		} else if (!strcmp(argv[argi], "-h")) {
			horizontal = 1;
		} else if (!strcmp(argv[argi], "-m")) {
			maximized = 1;
		} else if (!strcmp(argv[argi], "-s")) {
			sticky_page = 1;
			sticky_desk = 1;
		} else if (!strcmp(argv[argi], "-sp")) {
			sticky_page = 1;
		} else if (!strcmp(argv[argi], "-sd")) {
			sticky_desk = 1;
		} else if (!strcmp(argv[argi], "-mn") && ((argi + 1) < argc)) {
			maxnum = atoi(argv[++argi]);
		} else if (!strcmp(argv[argi], "-resize")) {
			resize = 1;
		} else if (!strcmp(argv[argi], "-nostretch")) {
			nostretch = 1;
		} else if (!strcmp(argv[argi], "-incx")
			   && ((argi + 1) < argc)) {
			incx = ++argi;
		} else if (!strcmp(argv[argi], "-incy")
			   && ((argi + 1) < argc)) {
			incy = ++argi;
		} else if (!strcmp(argv[argi], "-ewmhiwa")) {
			do_ewmhiwa = 1;
		} else if (!strcmp(argv[argi], "-maximize")) {
			do_maximize = 1;
		} else if (!strcmp(argv[argi], "-nomaximize")) {
			do_maximize = 0;
		} else if (!strcmp(argv[argi], "-animate")) {
			do_animate = 1;
		} else if (!strcmp(argv[argi], "-noanimate")) {
			do_animate = 0;
		} else if (!strcmp(argv[argi], "-screen")
			   && ((argi + 1) < argc)) {
			monitor_name = fxstrdup(argv[++argi]);
		} else {
			if (++nsargc > 4) {
				fprintf(console,
				    "%s: ignoring unknown arg %s\n",
				    module->name, argv[argi]);
				continue;
			}
			if (nsargc == 1) {
				ofsx = argi;
			} else if (nsargc == 2) {
				ofsy = argi;
			} else if (nsargc == 3) {
				maxw = argi;
			} else if (nsargc == 4) {
				maxh = argi;
			}
		}
	}
}

void
update_sizes(char *argv[])
{
	if (incx > 0)
		incx = atopixel(argv[incx], dwidth);
	if (incy > 0)
		incy = atopixel(argv[incy], dheight);
	if (ofsx > 0)
		ofsx = atopixel(argv[ofsx], dwidth);
	if (ofsy > 0)
		ofsy = atopixel(argv[ofsy], dheight);
	if (maxw > 0)
		maxw = atopixel(argv[maxw], dwidth);
	if (maxh > 0)
		maxh = atopixel(argv[maxh], dheight);
	maxx = maxw;
	maxy = maxh;
	ofsx += dx;
	ofsy += dy;
	maxx += dx;
	maxy += dy;
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
		fvwm_debug(__func__,
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
	signal(SIGALRM, HandleAlarm);

	FScreenInit(dpy);
	fd_width = GetFdWidth();

	/* Need to parse args first so we know what monitor to use. */
	parse_args(module->user_argc, module->user_argv, 0);
	if (monitor_name && StrEquals(monitor_name, "g")) {
		is_global  = 1;
		do_ewmhiwa = 1; /* Ignore hints for global monitor. */
	}

	mon = monitor_resolve_name(monitor_name ? monitor_name : "c");
	if (mon == NULL)
		mon = monitor_get_current();

	dx	= mon->si->x;
	dy	= mon->si->y;
	dwidth	= mon->si->w;
	dheight = mon->si->h;

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
		if (!StrEquals(mname, mon->si->name))
			continue;

		sscanf(config_line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d",
		    &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy,
		    &current_desk, &dummy, &dummy, &bs_left, &bs_right, &bs_top,
		    &bs_bottom);
		if (!do_ewmhiwa) {
			dx += bs_left;
			dy += bs_top;
			dwidth -= bs_right;
			dheight -= bs_bottom;
		}
	}

	/* Update sizes now that dwidth and dheight are known */
	update_sizes(module->user_argv);

	if (FvwmTile) {
		if (maxx == dx)
			maxx = dx + dwidth;
		if (maxy == dy)
			maxy = dy + dheight;
	}

	SetMessageMask(fd, M_CONFIGURE_WINDOW | M_END_WINDOWLIST);

	SendText(fd, "Send_WindowList", 0);

	/* tell fvwm we're running */
	SendFinishedStartupNotification(fd);

	Loop(fd);

	return 0;
}

static void
Loop(int *fd)
{
	FvwmPacket	*packet = NULL;

	/* Ten seconds should be enough to tile all the windows.  This was
	 * tested on a desk which had 128 windows, and it took less than five
	 * seconds.
	 */
	alarm(10);

	while (1) {
		if ((packet = ReadFvwmPacket(fd[1])) == NULL)
			DeadPipe(0);
		process_message(packet->type, packet->body);
	}
}

void process_message(unsigned long type, unsigned long *body)
{
	switch(type)
	{
	case M_CONFIGURE_WINDOW:
		get_window(body);
		break;
	case M_END_WINDOWLIST:
		if (wins_count) {
			if (FvwmCascade)
				cascade_windows();
			else /* FvwmTile */
				tile_windows();
		}
		break;
	default:
		break;
	}
}