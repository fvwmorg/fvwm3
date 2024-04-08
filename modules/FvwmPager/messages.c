/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"
#include <stdbool.h>
#include "libs/FScreen.h"
#include "libs/fvwmlib.h"
#include "libs/log.h"
#include "libs/Module.h"
#include "libs/Parse.h"
#include "fvwm/fvwm.h"
#include "libs/vpacket.h"
#include "FvwmPager.h"

static bool fp_new_block = false;

static PagerWindow *find_pager_window(Window target_w);
static void handle_config_win_package(PagerWindow *t,
				      ConfigWinPacket *cfgpacket);

static void process_add(unsigned long *body);
static void process_configure(unsigned long *body);
static void process_destroy(unsigned long *body);
static void process_monitor_focus(unsigned long *body);
static void process_focus(unsigned long *body);
static void process_new_page(unsigned long *body);
static void process_new_desk(unsigned long *body);
static void process_raise(unsigned long *body);
static void process_lower(unsigned long *body);
static void process_iconify(unsigned long *body);
static void process_deiconify(unsigned long *body, unsigned long length);
static void process_window_name(unsigned long *body,unsigned long type);
static void process_icon_name(unsigned long *body);
static void process_mini_icon(unsigned long *body);
static void process_restack(unsigned long *body, unsigned long length);
static void process_end(void);
static void process_property_change(unsigned long *body);
static void process_reply(unsigned long *body);
static void parse_monitor_line(char *tline);
static void parse_desktop_size_line(char *tline);
static void parse_desktop_configuration_line(char *tline);

/*
 *  Procedure:
 *      Process message - examines packet types, and takes appropriate action
 */
void process_message(FvwmPacket *packet)
{
	unsigned long type = packet->type;
	unsigned long length = packet->size;
	unsigned long* body = packet->body;

	switch (type)
	{
		case M_ADD_WINDOW:
		case M_CONFIGURE_WINDOW:
			process_configure(body);
			break;
		case M_DESTROY_WINDOW:
			process_destroy(body);
			break;
		case M_FOCUS_CHANGE:
			process_focus(body);
			break;
		case M_NEW_PAGE:
			process_new_page(body);
			break;
		case M_NEW_DESK:
			process_new_desk(body);
			break;
		case M_RAISE_WINDOW:
			process_raise(body);
			break;
		case M_LOWER_WINDOW:
			process_lower(body);
			break;
		case M_ICONIFY:
		case M_ICON_LOCATION:
			process_iconify(body);
			break;
		case M_DEICONIFY:
			process_deiconify(body, length);
			break;
		case M_RES_CLASS:
		case M_RES_NAME:
		case M_VISIBLE_NAME:
			process_window_name(body, type);
			break;
		case MX_VISIBLE_ICON_NAME:
			process_icon_name(body);
			break;
		case M_MINI_ICON:
			process_mini_icon(body);
			break;
		case M_END_WINDOWLIST:
			process_end();
			break;
		case M_RESTACK:
			process_restack(body, length);
			break;
		case M_CONFIG_INFO:
			process_config_info_line((char *)&body[3], false);
			break;
		case MX_PROPERTY_CHANGE:
			process_property_change(body);
			break;
		case MX_MONITOR_FOCUS:
			process_monitor_focus(body);
			break;
		case MX_REPLY:
			process_reply(body);
			break;
		case M_STRING: {
			char *this = (char *)&body[3];
			char *token;
			char *rest = GetNextToken(this, &token);

			if (rest != NULL && StrEquals(token, "Monitor")) {
				update_monitor_to_track(
					&monitor_to_track, rest, true);
			} else if (StrEquals(token, "CurrentMonitor")) {
				update_monitor_to_track(
					&current_monitor, rest, false);
			} else if (StrEquals(token, "DeskLabels")) {
				use_desk_label = true;
			} else if (StrEquals(token, "NoDeskLabels")) {
				use_desk_label = false;
			} else if (StrEquals(token, "MonitorLabels")) {
				use_monitor_label = true;
			} else if (StrEquals(token, "NoMonitorLabels")) {
				use_monitor_label = false;
			}
			else if (StrEquals(token, "CurrentDeskPerMonitor")) {
				CurrentDeskPerMonitor = true;
			} else if (StrEquals(token, "CurrentDeskGlobal")) {
				CurrentDeskPerMonitor = false;
			} else if (StrEquals(token, "IsShared")) {
				IsShared = true;
			} else if (StrEquals(token, "IsNotShared")) {
				IsShared = false;
			} else {
				break;
			}
			set_desk_size(true);
			ReConfigure();
			break;
		}
		default:
			/* ignore unknown packet */
			break;
	}
}

/* Convenience method to find target window. */
PagerWindow *find_pager_window(Window target_w)
{
	PagerWindow *t;

	t = Start;
	while(t != NULL && t->w != target_w)
		t = t->next;

	return t;
}

/*
 *  Procedure:
 *	handle_config_win_package - updates a PagerWindow
 *		with respect to a ConfigWinPacket
 */
void handle_config_win_package(PagerWindow *t,
			       ConfigWinPacket *cfgpacket)
{
	if (t->w != None && t->w != cfgpacket->w)
	{
		/* Should never happen */
		fvwm_debug(__func__,
			   "%s: Error: Internal window list corrupt\n",MyName);
		/* might be a good idea to exit here */
		return;
	}
	t->w = cfgpacket->w;
	t->frame = cfgpacket->frame;
	t->frame_x = cfgpacket->frame_x;
	t->frame_y = cfgpacket->frame_y;
	t->frame_width = cfgpacket->frame_width;
	t->frame_height = cfgpacket->frame_height;
	t->m = monitor_by_output(cfgpacket->monitor_id);

	t->desk = cfgpacket->desk;

	t->title_height = cfgpacket->title_height;
	t->border_width = cfgpacket->border_width;

	t->icon_w = cfgpacket->icon_w;
	t->icon_pixmap_w = cfgpacket->icon_pixmap_w;

	memcpy(&(t->flags), &(cfgpacket->flags), sizeof(cfgpacket->flags));
	memcpy(&(t->allowed_actions), &(cfgpacket->allowed_actions),
	       sizeof(cfgpacket->allowed_actions));

	/* These are used for windows if no pixel is set. */
	t->text = cfgpacket->TextPixel;
	t->back = cfgpacket->BackPixel;

	if (IS_ICONIFIED(t))
	{
		/* For new windows icon_x and icon_y will be zero until
		 * iconify message is recived
		 */
		t->x = t->icon_x;
		t->y = t->icon_y;
		t->width = t->icon_width;
		t->height = t->icon_height;
		if(IS_ICON_SUPPRESSED(t) || t->width == 0 || t->height == 0)
		{
			t->x = -32768;
			t->y = -32768;
		}
	}
	else
	{
		t->x = t->frame_x;
		t->y = t->frame_y;
		t->width = t->frame_width;
		t->height = t->frame_height;
	}
}

void process_add(unsigned long *body)
{
	PagerWindow *t, **prev;
	struct ConfigWinPacket  *cfgpacket = (void *)body;

	t = Start;
	prev = &Start;

	while(t != NULL)
	{
		if (t->w == cfgpacket->w)
		{
			/* it's already there, do nothing */
			return;
		}
		prev = &(t->next);
		t = t->next;
	}
	*prev = fxcalloc(1, sizeof(PagerWindow));
	handle_config_win_package(*prev, cfgpacket);
	AddNewWindow(*prev);

	return;
}

void process_configure(unsigned long *body)
{
	PagerWindow *t;
	struct ConfigWinPacket  *cfgpacket = (void *)body;
	bool is_new_desk;

	t = find_pager_window(cfgpacket->w);
	if (t == NULL)
	{
		process_add(body);
		return;
	}

	is_new_desk = (t->desk != cfgpacket->desk);
	handle_config_win_package(t, cfgpacket);
	if (is_new_desk)
		ChangeDeskForWindow(t, cfgpacket->desk);
	MoveResizePagerView(t, true);
}

void process_destroy(unsigned long *body)
{
	PagerWindow *t, **prev;
	Window target_w;

	target_w = body[0];
	t = Start;
	prev = &Start;
	while(t != NULL && t->w != target_w)
	{
		prev = &(t->next);
		t = t->next;
	}

	if(t != NULL)
	{
		if(prev != NULL)
			*prev = t->next;
			/* remove window from the chain */

		XDestroyWindow(dpy, t->PagerView);
		XDestroyWindow(dpy, t->IconView);
		if(FocusWin == t)
			FocusWin = NULL;

		free(t->res_class);
		free(t->res_name);
		free(t->window_name);
		free(t->icon_name);
		free(t->window_label);
		free(t);
	}
}

void process_monitor_focus(unsigned long *body)
{
	return;
}

void process_focus(unsigned long *body)
{
	PagerWindow *t = find_pager_window(body[0]);
	bool do_force_update = false;

	/* Update Pixels data. */
	if (Scr.focus_win_fg != body[3] || Scr.focus_win_bg != body[4])
		do_force_update = true;
	Scr.focus_win_fg = body[3];
	Scr.focus_win_bg = body[4];

	if (do_force_update || t != FocusWin) {
		PagerWindow *focus = FocusWin;
		FocusWin = t;

		update_window_background(focus);
		update_window_background(t);
	}
}

void process_new_page(unsigned long *body)
{
	bool do_reconfigure = false;
	int mon_num = body[7];
	struct monitor *m;
	struct fpmonitor *fp;

	m = monitor_by_output(mon_num);
	/* Don't allow monitor_by_output to fallback to RB_MIN. */
	if (m->si->rr_output != mon_num)
		return;

	/* Still need to update the monitors when tracking a single
	 * monitor, but don't need to reconfigure anything.
	 */
	if (monitor_to_track != NULL && m == monitor_to_track->m) {
		fp = monitor_to_track;
		do_reconfigure = true;
	} else {
		fp = fpmonitor_this(m);
		if (fp == NULL)
			return;
		if (monitor_to_track == NULL)
			do_reconfigure = true;
	}

	fp->virtual_scr.Vx = fp->m->virtual_scr.Vx = body[0];
	fp->virtual_scr.Vy = fp->m->virtual_scr.Vy = body[1];
	if (fp->m->virtual_scr.CurrentDesk != body[2]) {
		/* first handle the new desk */
		body[0] = body[2];
		body[1] = mon_num;
		process_new_desk(body);
	}
	if (fp->virtual_scr.VxPages != body[5] ||
	    fp->virtual_scr.VyPages != body[6])
	{
		fp->virtual_scr.VxPages = body[5];
		fp->virtual_scr.VyPages = body[6];
		fp->virtual_scr.VWidth =
			fp->virtual_scr.VxPages * fpmonitor_get_all_widths();
		fp->virtual_scr.VHeight =
			fp->virtual_scr.VyPages * fpmonitor_get_all_heights();
		fp->virtual_scr.VxMax =
			fp->virtual_scr.VWidth - fpmonitor_get_all_widths();
		fp->virtual_scr.VyMax =
			fp->virtual_scr.VHeight - fpmonitor_get_all_heights();
		ReConfigure();
	}

	if (do_reconfigure) {
		MovePage();
		MoveStickyWindows(true, false);
		update_window_background(FocusWin);
	}
}

void process_new_desk(unsigned long *body)
{
	int oldDesk, newDesk;
	int mon_num = body[1];
	struct monitor *mout;
	struct fpmonitor *fp;
	DeskStyle *style;

	mout = monitor_by_output(mon_num);
	/* Don't allow monitor_by_output to fallback to RB_MIN. */
	if (mout->si->rr_output != mon_num)
		return;

	fp = fpmonitor_this(mout);
	if (fp == NULL)
		return;

	/* Best to update the desk, even if the monitor isn't being
	 * tracked, as this could change.
	 */
	oldDesk = fp->m->virtual_scr.CurrentDesk;
	newDesk = fp->m->virtual_scr.CurrentDesk = (long)body[0];
	if (newDesk >= desk1 && newDesk <= desk2)
		Desks[newDesk - desk1].fp = fp;

	/* Only update FvwmPager's desk for the monitors being tracked.
	 * Exceptions:
	 *   + If CurrentDeskPerMonitor is set, always update desk.
	 *   + If current_monitor is set, only update if that monitor changes
	 *     and tracking is per-monitor or shared.
	 */
	if (!CurrentDeskPerMonitor && ((monitor_to_track != NULL &&
	    fp != monitor_to_track) ||
	    (current_monitor != NULL &&
	    monitor_to_track == NULL &&
	    fp != current_monitor &&
	    (monitor_mode == MONITOR_TRACKING_M ||
	    is_tracking_shared))))
	{
		/* Still need to update monitor location of other monitors. */
		if (current_monitor != NULL && oldDesk != newDesk)
			goto update_grid;
		return;
	}

	/* Update the current desk. */
	desk_i = newDesk;
	style = FindDeskStyle(newDesk);

	/* Keep monitors in sync when tracking is global. */
	if (monitor_mode == MONITOR_TRACKING_G)
		monitor_assign_virtual(fp->m);

	/* If always tracking current desk. Update Desks[0]. */
	if (fAlwaysCurrentDesk && oldDesk != newDesk)
	{
		PagerWindow *t;

		desk1 = desk2 = newDesk;
		for (t = Start; t != NULL; t = t->next)
		{
			if (t->desk == oldDesk || t->desk == newDesk)
				ChangeDeskForWindow(t, t->desk);
		}

		/* Update DeskStyle */
		Desks[0].style = style;
		update_desk_background(0);
		update_monitor_locations(0);
		update_monitor_backgrounds(0);
	}

	XStoreName(dpy, Scr.pager_w, style->label);
	XSetIconName(dpy, Scr.pager_w, style->label);

update_grid:
	MovePage();
	ReConfigureAll();
	draw_desk_grid(oldDesk - desk1);
	draw_desk_grid(newDesk - desk1);
	MoveStickyWindows(false, true);
	update_window_background(FocusWin);
}

void process_raise(unsigned long *body)
{
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

	XRaiseWindow(dpy, t->PagerView);
	XRaiseWindow(dpy, t->IconView);
}

void process_lower(unsigned long *body)
{
	struct fpmonitor *fp;
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

	XLowerWindow(dpy, t->PagerView);
	if (HilightDesks && t->desk >= desk1 && t->desk <= desk2) {
		TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
			XLowerWindow(dpy, fp->CPagerWin[t->desk - desk1]);
		}
	}
	XLowerWindow(dpy, t->IconView);
}

void process_iconify(unsigned long *body)
{
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

	t->frame = body[1];
	t->icon_x = body[3];
	t->icon_y = body[4];
	t->icon_width = body[5];
	t->icon_height = body[6];
	SET_ICONIFIED(t, true);
	if (IS_ICON_SUPPRESSED(t) || t->icon_width == 0 ||
	    t->icon_height == 0)
	{
		t->x = -32768;
		t->y = -32768;
	}
	else
	{
		t->x = t->icon_x;
		t->y = t->icon_y;
	}
	t->width = t->icon_width;
	t->height = t->icon_height;

	/* if iconifying main pager window turn balloons on or off */
	if (t->w == Scr.pager_w)
		Balloon.show = Balloon.show_in_icon;
	MoveResizePagerView(t, true);
}

void process_deiconify(unsigned long *body, unsigned long length)
{
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

	SET_ICONIFIED(t, false);
	if (length >= 11 + FvwmPacketHeaderSize)
	{
		t->frame_x = body[7];
		t->frame_y = body[8];
		t->frame_width = body[9];
		t->frame_height = body[10];
	}
	t->x = t->frame_x;
	t->y = t->frame_y;
	t->width = t->frame_width;
	t->height = t->frame_height;

	/* if deiconifying main pager window turn balloons on or off */
	if (t->w == Scr.pager_w)
		Balloon.show = Balloon.show_in_pager;

	MoveResizePagerView(t, true);
}

void process_window_name(unsigned long *body, unsigned long type)
{
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

	switch (type) {
		case M_RES_CLASS:
			free(t->res_class);
			CopyString(&t->res_class, (char *)(&body[3]));
			break;
		case M_RES_NAME:
			free(t->res_name);
			CopyString(&t->res_name, (char *)(&body[3]));
			break;
		case M_VISIBLE_NAME:
			free(t->window_name);
			CopyString(&t->window_name, (char *)(&body[3]));
			break;
	}

	/* repaint by clearing window */
	if (Scr.winFfont != NULL && t->icon_name != NULL &&
	    !(MiniIcons && t->mini_icon.picture))
	{
		XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
		XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
	}
	if (Balloon.is_mapped)
	{
		/* update balloons */
		UnmapBalloonWindow();
		MapBalloonWindow(t, Balloon.is_icon);
	}
}

void process_icon_name(unsigned long *body)
{
	PagerWindow *t = find_pager_window(body[0]);

	if (t == NULL)
		return;

	free(t->icon_name);
	CopyString(&t->icon_name, (char *)(&body[3]));
	/* repaint by clearing window */
	if (Scr.winFfont != NULL && t->icon_name != NULL &&
	    !(MiniIcons && t->mini_icon.picture))
	{
		XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
		XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
	}
}

void process_mini_icon(unsigned long *body)
{
	MiniIconPacket *mip = (MiniIconPacket *) body;
	PagerWindow *t = find_pager_window(mip->w);

	if (t == NULL)
		return;

	t->mini_icon.width   = mip->width;
	t->mini_icon.height  = mip->height;
	t->mini_icon.depth   = mip->depth;
	t->mini_icon.picture = mip->picture;
	t->mini_icon.mask    = mip->mask;
	t->mini_icon.alpha   = mip->alpha;
	/* repaint by clearing window */
	if (MiniIcons && t->mini_icon.picture) {
		XClearArea(dpy, t->PagerView, 0, 0, 0, 0, True);
		XClearArea(dpy, t->IconView, 0, 0, 0, 0, True);
	}
}

void process_restack(unsigned long *body, unsigned long length)
{
	PagerWindow *t;
	PagerWindow *pager_wins[length];
	Window *wins;
	int i, j, desk;

	wins = fxmalloc(length * sizeof (Window));

	/* Should figure out how to do this with a single loop of the list,
	 * not one per desk + icon window.
	 */

	/* first restack in the icon view */
	j = 0;
	for (i = 0; i < (length - FvwmPacketHeaderSize); i += 3)
	{
		t = find_pager_window(body[i]);
		if (t != NULL) {
			pager_wins[j] = t;
			wins[j++] = t->IconView;
		}
	}
	XRestackWindows(dpy, wins, j);
	length = j;

	/* now restack each desk separately, since they have separate roots */
	for (desk = 0; desk < ndesks; desk++)
	{
		j = 0;
		for (i = 0; i < length; i++)
		{
			t = pager_wins[i];
			if (t != NULL && t->desk == desk + desk1)
				wins[j++] = t->PagerView;
		}
		XRestackWindows(dpy, wins, j);
	}

	free(wins);
}

void process_end(void)
{
	unsigned int nchildren, i;
	Window root, parent, *children;
	PagerWindow *ptr;

	if (!XQueryTree(dpy, Scr.Root, &root, &parent, &children, &nchildren))
		return;

	for(i = 0; i < nchildren; i++)
	{
		ptr = Start;
		while(ptr != NULL)
		{
			if (ptr->frame == children[i] ||
			    ptr->icon_w == children[i] ||
			    ptr->icon_pixmap_w == children[i])
			{
				XRaiseWindow(dpy, ptr->PagerView);
				XRaiseWindow(dpy, ptr->IconView);
			}
			ptr = ptr->next;
		}
	}

	if (nchildren > 0)
		XFree((char *)children);
}

void process_property_change(unsigned long *body)
{
	if (body[0] == MX_PROPERTY_CHANGE_BACKGROUND)
	{
		if (((!Swallowed && body[2] == 0) ||
		    (Swallowed && body[2] == Scr.pager_w)))
			update_pr_transparent_windows();
	}
	else if  (body[0] == MX_PROPERTY_CHANGE_SWALLOW &&
		  body[2] == Scr.pager_w)
	{
		Swallowed = body[1];
	}
}

void process_reply(unsigned long *body)
{
	char *tline;
	tline = (char*)&(body[3]);

	if (strcmp(tline, "ScrollDone") == 0)
		HandleScrollDone();
}

/* Helper methods */
void set_desk_label(int desk, const char *label)
{
	DeskStyle *style = FindDeskStyle(desk);

	free(style->label);
	CopyString(&(style->label), label);
}

void parse_monitor_line(char *tline)
{
	int		  dx, dy, Vx, Vy, VxMax, VyMax, CurrentDesk;
	int		  scr_width, scr_height;
	int		  flags;
	char		 *mname;
	struct monitor	 *tm;
	struct fpmonitor *fp;

	tline = GetNextToken(tline, &mname);

	sscanf(tline, "%d %d %d %d %d %d %d %d %d %d", &flags,
	    &dx, &dy, &Vx, &Vy, &VxMax, &VyMax, &CurrentDesk,
	    &scr_width, &scr_height);

	monitor_refresh_module(dpy);

	tm = monitor_resolve_name(mname);
	if (tm == NULL)
		return;

	if (flags & MONITOR_DISABLED) {
		fp = fpmonitor_by_name(mname);
		if (fp != NULL) {
			bool disabled = fp->disabled;
			fpmonitor_disable(fp);
			if (!disabled)
				ReConfigure();
		}
		return;
	}

	if ((fp = fpmonitor_this(tm)) == NULL) {
		if (fp_new_block)
			return;
		fp_new_block = true;
		fp = fpmonitor_new(tm);
		fp->m->flags |= MONITOR_NEW;
	}

	/* This ensures that if monitor_to_track gets disconnected
	 * then reconnected, the pager can resume tracking it.
	 */
	if (preferred_monitor != NULL &&
	    strcmp(fp->m->si->name, preferred_monitor) == 0)
	{
		monitor_to_track = fp;
	}
	fp->disabled = false;
	fp->scr_width = scr_width;
	fp->scr_height = scr_height;
	fp->m->dx = dx;
	fp->m->dy = dy;
	fp->m->virtual_scr.Vx = fp->virtual_scr.Vx = Vx;
	fp->m->virtual_scr.Vy = fp->virtual_scr.Vy = Vy;
	fp->m->virtual_scr.VxMax = fp->virtual_scr.VxMax = VxMax;
	fp->m->virtual_scr.VyMax = fp->virtual_scr.VyMax = VyMax;
	fp->m->virtual_scr.CurrentDesk = CurrentDesk;

	fp->virtual_scr.VxMax = dx * fpmonitor_get_all_widths() - fpmonitor_get_all_widths();
	fp->virtual_scr.VyMax = dy * fpmonitor_get_all_heights() - fpmonitor_get_all_heights();
	if (fp->virtual_scr.VxMax < 0)
		fp->virtual_scr.VxMax = 0;
	if (fp->virtual_scr.VyMax < 0)
		fp->virtual_scr.VyMax = 0;
	fp->virtual_scr.VWidth = fp->virtual_scr.VxMax + fpmonitor_get_all_widths();
	fp->virtual_scr.VHeight = fp->virtual_scr.VyMax + fpmonitor_get_all_heights();
	fp->virtual_scr.VxPages = fp->virtual_scr.VWidth / fpmonitor_get_all_widths();
	fp->virtual_scr.VyPages = fp->virtual_scr.VHeight / fpmonitor_get_all_heights();

	if (fp->m != NULL && fp->m->flags & MONITOR_NEW) {
		fp->m->flags &= ~MONITOR_NEW;
		initialize_fpmonitor_windows(fp);
	}
	fp_new_block = false;
}

void parse_desktop_size_line(char *tline)
{
	int dx, dy;
	struct fpmonitor *m;

	sscanf(tline, "%d %d", &dx, &dy);

	TAILQ_FOREACH(m, &fp_monitor_q, entry) {
		m->virtual_scr.VxMax = dx * fpmonitor_get_all_widths() - fpmonitor_get_all_widths();
		m->virtual_scr.VyMax = dy * fpmonitor_get_all_heights() - fpmonitor_get_all_heights();
		if (m->virtual_scr.VxMax < 0)
			m->virtual_scr.VxMax = 0;
		if (m->virtual_scr.VyMax < 0)
			m->virtual_scr.VyMax = 0;
		m->virtual_scr.VWidth = m->virtual_scr.VxMax + fpmonitor_get_all_widths();
		m->virtual_scr.VHeight = m->virtual_scr.VyMax + fpmonitor_get_all_heights();
		m->virtual_scr.VxPages = m->virtual_scr.VWidth / fpmonitor_get_all_widths();
		m->virtual_scr.VyPages = m->virtual_scr.VHeight / fpmonitor_get_all_heights();
	}
}

void parse_desktop_configuration_line(char *tline)
{
		int mmode, is_shared;

		sscanf(tline, "%d %d", &mmode, &is_shared);

		if (mmode > 0) {
			monitor_mode = mmode;
			is_tracking_shared = is_shared;
		}
}

/* These methods are also used in parse_options in init_pager.c */
void process_config_info_line(char *line, bool is_init)
{
	char *token, *next;

	token = PeekToken(line, &next);
	if (StrEquals(token, "DesktopSize")) {
		parse_desktop_size_line(next);
	} else if (StrEquals(token, "DesktopConfiguration")) {
		parse_desktop_configuration_line(next);
	} else if (StrEquals(token, "Monitor")) {
		parse_monitor_line(next);
		if (!is_init)
			ReConfigure();
	} else if (StrEquals(token, "ImagePath")) {
			free(ImagePath);
			ImagePath = NULL;
			GetNextToken(next, &ImagePath);
	} else if (StrEquals(token, "DesktopName")) {
		int val;

		if (GetIntegerArguments(next, &next, &val, 1) <= 0)
			return;

		set_desk_label(val, (const char *)next);

		if (is_init)
			return;

		if (fAlwaysCurrentDesk)
		{
			struct fpmonitor *fp;

			TAILQ_FOREACH(fp, &fp_monitor_q, entry) {
				if (fp->m->virtual_scr.CurrentDesk == val)
					val = 0;
			}
		}
		else if ((val >= desk1) && (val <=desk2))
		{
			val = val - desk1;
		}
		draw_desk_grid(val);
	} else if (StrEquals(token, "Colorset")) {
		DeskStyle *style;
		int color = LoadColorset(next);

		if (is_init)
			return;

		TAILQ_FOREACH(style, &desk_style_q, entry) {
			if (style->cs == color || style->hi_cs == color)
			{
				update_desk_style_gcs(style);
				if (style->desk < desk1 ||
				    style->desk > desk2)
					continue;

				int i = style->desk - desk1;
				update_desk_background(i);
				update_monitor_backgrounds(i);
			}
			if (style->win_cs == color ||
			    style->focus_cs == color)
			{
				PagerWindow *t = Start;

				/* Only update if it wasn't updated previously. */
				if (style->cs != color && style->hi_cs != color)
					update_desk_style_gcs(style);

				while (t != NULL) {
					if (t->desk != style->desk) {
						t = t->next;
						continue;
					}
					update_window_background(t);
					update_window_decor(t);
					t = t->next;
				}
			}
		}
	}
}

void update_monitor_to_track(struct fpmonitor **fp_track,
				    char *name, bool update_prefered)
{
	struct fpmonitor *new_fp;

	name = SkipSpaces(name, NULL, 0);
	if (StrEquals(name, "none")) {
		*fp_track = NULL;
		if (update_prefered) {
			free(preferred_monitor);
			preferred_monitor = NULL;
		}
		return;
	}

	new_fp = fpmonitor_by_name(name);
	/* Fallback to current monitor, if monitor not already set. */
	if (new_fp != NULL)
		*fp_track = new_fp;
	else if (*fp_track == NULL)
		*fp_track = fpmonitor_this(NULL);
	if (update_prefered) {
		/* Set this even if no monitor matches given name.
		 * That monitor may get enabled later.
		 */
		free(preferred_monitor);
		preferred_monitor = fxstrdup(name);
	}
}
