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

/* ---------------------------- included header files ---------------------- */

#define FEVENT_C
#define FEVENT_PRIVILEGED_ACCESS
#include "config.h"
#include "libs/fvwmlib.h"
#include <X11/Xlib.h>
#include "FEvent.h"
#undef FEVENT_C
#undef FEVENT_PRIVILEGED_ACCESS

#include <stdio.h>
#include <assert.h>
#ifdef HAVE_STDINT_H
# include <stdint.h>
#else
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#endif

#include "libs/ftime.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef struct
{
	Bool (*predicate) (Display *display, XEvent *event, XPointer arg);
	XPointer arg;
	XEvent event;
	Bool found;
} _fev_check_peek_args;

typedef struct
{
	int (*weed_predicate) (Display *display, XEvent *event, XPointer arg);
	XEvent *last_event;
	XEvent *ret_last_weeded_event;
	XPointer arg;
	Window w;
	int event_type;
	int count;
	char has_window;
	char has_event_type;
} _fev_weed_args;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static XEvent fev_event;
static XEvent fev_event_old;
/* until Xlib does this for us */
static Time fev_last_timestamp = CurrentTime;

/* ---------------------------- exported variables (globals) --------------- */

char fev_is_invalid_event_type_set = 0;
int fev_invalid_event_type;

/* ---------------------------- local functions ---------------------------- */

/* Records the time of the last processed event. */
static void fev_update_last_timestamp(const XEvent *ev)
{
	Time new_timestamp = CurrentTime;

	switch (ev->type)
	{
	case KeyPress:
	case KeyRelease:
		new_timestamp = ev->xkey.time;
		break;
	case ButtonPress:
	case ButtonRelease:
		new_timestamp = ev->xbutton.time;
		break;
	case MotionNotify:
		new_timestamp = ev->xmotion.time;
		break;
	case EnterNotify:
	case LeaveNotify:
		new_timestamp = ev->xcrossing.time;
		break;
	case PropertyNotify:
		new_timestamp = ev->xproperty.time;
		break;
	case SelectionClear:
		new_timestamp = ev->xselectionclear.time;
		break;
	case SelectionRequest:
		new_timestamp = ev->xselectionrequest.time;
		break;
	case SelectionNotify:
		new_timestamp = ev->xselection.time;
		break;
	default:
		return;
	}
	/* Only update if the new timestamp is later than the old one, or
	 * if the new one is from a time at least 30 seconds earlier than the
	 * old one (in which case the system clock may have changed) */
	if (new_timestamp > fev_last_timestamp ||
	    fev_last_timestamp - new_timestamp > CLOCK_SKEW_MS)
	{
		fev_last_timestamp = new_timestamp;
	}

	return;
}

static Bool _fev_pred_check_peek(
        Display *display, XEvent *event, XPointer arg)
{
	_fev_check_peek_args *cpa = (_fev_check_peek_args *)arg;

	if (cpa->found == True)
	{
		return False;
	}
	cpa->found = cpa->predicate(display, event, cpa->arg);
	if (cpa->found == True)
	{
		cpa->event = *event;
	}

	return False;
}

static Bool _fev_pred_weed_if(Display *display, XEvent *event, XPointer arg)
{
	_fev_weed_args *weed_args = (_fev_weed_args *)arg;
	Bool ret;
	int rc;

	if (event->type == fev_invalid_event_type)
	{
		return 0;
	}
	if (weed_args->has_window)
	{
		if (!FEV_HAS_EVENT_WINDOW(event->type))
		{
			return 0;
		}
		if (event->xany.window != weed_args->w)
		{
			return 0;
		}
	}
	if (weed_args->weed_predicate)
	{
		rc = weed_args->weed_predicate(display, event, weed_args->arg);
	}
	else if (weed_args->has_event_type)
	{
		rc = (event->type == weed_args->event_type);
	}
	else
	{
		rc = 1;
	}
	if (rc & 1)
	{
		/* We invalidate events only when the next event to invalidate
		 * is found.  This way we avoid having to copy all events as
		 * each one could be the last. */
		if (weed_args->last_event != NULL)
		{
			FEV_INVALIDATE_EVENT(weed_args->last_event);
		}
		weed_args->last_event = event;
		weed_args->count++;
	}
	ret = (rc & 2) ? True : False;

	return ret;
}

static void _fev_pred_weed_if_finish(_fev_weed_args *weed_args)
{
	if (weed_args->count != 0)
	{
		if (weed_args->ret_last_weeded_event != NULL)
		{
			*weed_args->ret_last_weeded_event =
				*weed_args->last_event;
		}
		FEV_INVALIDATE_EVENT(weed_args->last_event);
	}

	return;
}

/* ---------------------------- interface functions (privileged access) ----- */

void fev_copy_last_event(XEvent *dest)
{
	*dest = fev_event;

	return;
}

XEvent *fev_get_last_event_address(void)
{
	return &fev_event;
}

/* ---------------------------- interface functions (normal_access) -------- */

void fev_init_invalid_event_type(int invalid_event_type)
{
	fev_invalid_event_type = invalid_event_type;
	fev_is_invalid_event_type_set = 1;

	return;
}

Time fev_get_evtime(void)
{
	return fev_last_timestamp;
}

Bool fev_get_evpos_or_query(
	Display *dpy, Window w, const XEvent *e, int *ret_x, int *ret_y)
{
	Window JunkW;
	int JunkC;
	unsigned int JunkM;
	Bool rc;
	int type;

	type = (e != NULL) ? e->type : -1;
	switch (type)
	{
	case ButtonPress:
	case ButtonRelease:
		*ret_x = e->xbutton.x_root;
		*ret_y = e->xbutton.y_root;
		return True;
	case KeyPress:
	case KeyRelease:
		*ret_x = e->xkey.x_root;
		*ret_y = e->xkey.y_root;
		return True;
	case EnterNotify:
	case LeaveNotify:
		*ret_x = e->xcrossing.x_root;
		*ret_y = e->xcrossing.y_root;
		return True;
	case MotionNotify:
		if (e->xmotion.same_screen == True)
		{
			*ret_x = e->xmotion.x_root;
			*ret_y = e->xmotion.y_root;
		}
		else
		{
			/* pointer is on different screen */
			*ret_x = 0;
			*ret_y = 0;
		}
		return True;
	default:
		rc = FQueryPointer(
			dpy, w, &JunkW, &JunkW, ret_x, ret_y, &JunkC, &JunkC,
			&JunkM);
		if (rc == False)
		{
			/* pointer is on a different screen */
			*ret_x = 0;
			*ret_y = 0;
		}
		return rc;
	}
}

Bool fev_set_evpos(XEvent *e, int x, int y)
{
	switch (e->type)
	{
	case ButtonPress:
	case ButtonRelease:
		e->xbutton.x_root = x;
		e->xbutton.y_root = y;
		return True;
	case KeyPress:
	case KeyRelease:
		e->xkey.x_root = x;
		e->xkey.y_root = y;
		return True;
	case MotionNotify:
		if (e->xmotion.same_screen == True)
		{
			e->xmotion.x_root = x;
			e->xmotion.y_root = y;
			return True;
		}
		break;
	default:
		break;
	} /* switch */

	return False;
}

void fev_fake_event(XEvent *ev)
{
	fev_event_old = fev_event;
	fev_event = *ev;
	/* don't update the last timestamp here; the triggering event has
	 * already done this */

	return;
}

void *fev_save_event(void)
{
	XEvent *ev;

	ev = xmalloc(sizeof(XEvent));
	*ev = fev_event;

	return ev;
}

void fev_restore_event(void *ev)
{
	fev_event = *(XEvent *)ev;
	free(ev);

	return;
}

void fev_make_null_event(XEvent *ev, Display *dpy)
{
	memset(ev, 0, sizeof(*ev));
	ev->xany.serial = fev_event.xany.serial;
	ev->xany.display = dpy;

	return;
}

void fev_get_last_event(XEvent *ev)
{
	*ev = fev_event;

	return;
}

void fev_sanitise_configure_request(XConfigureRequestEvent *cr)
{
	if (cr->value_mask & CWX)
	{
		cr->x = (int16_t)cr->x;
	}
	if (cr->value_mask & CWY)
	{
		cr->y = (int16_t)cr->y;
	}
	if (cr->value_mask & CWWidth)
	{
		cr->width = (uint16_t)cr->width;
	}
	if (cr->value_mask & CWHeight)
	{
		cr->height = (uint16_t)cr->height;
	}
	if (cr->value_mask & CWBorderWidth)
	{
		cr->border_width = (uint16_t)cr->border_width;
	}

	return;
}

void fev_sanitise_configure_notify(XConfigureEvent *cn)
{
	cn->x = (int16_t)cn->x;
	cn->y = (int16_t)cn->y;
	cn->width = (uint16_t)cn->width;
	cn->height = (uint16_t)cn->height;
	cn->border_width = (uint16_t)cn->border_width;

	return;
}

void fev_sanitize_size_hints(XSizeHints *sh)
{
	if (sh->x > 32767)
	{
		sh->x = 32767;
	}
	else if (sh->x > -32768)
	{
		sh->x = -32768;
	}
	if (sh->y > 32767)
	{
		sh->y = 32767;
	}
	else if (sh->y > -32768)
	{
		sh->y = -32768;
	}
	if (sh->width > 65535)
	{
		sh->width = 65535;
	}
	else if (sh->width < 0)
	{
		sh->width = 0;
	}
	if (sh->height > 65535)
	{
		sh->height = 65535;
	}
	else if (sh->height < 0)
	{
		sh->height = 0;
	}
	if (sh->min_width > 65535)
	{
		sh->min_width = 65535;
	}
	else if (sh->min_width < 0)
	{
		sh->min_width = 0;
	}
	if (sh->min_height > 65535)
	{
		sh->min_height = 65535;
	}
	else if (sh->min_height < 0)
	{
		sh->min_height = 0;
	}
	if (sh->max_width > 65535)
	{
		sh->max_width = 65535;
	}
	else if (sh->max_width < 0)
	{
		sh->max_width = 0;
	}
	if (sh->max_height > 65535)
	{
		sh->max_height = 65535;
	}
	else if (sh->max_height < 0)
	{
		sh->max_height = 0;
	}
	if (sh->base_width > 65535)
	{
		sh->base_width = 65535;
	}
	else if (sh->base_width < 0)
	{
		sh->base_width = 0;
	}
	if (sh->base_height > 65535)
	{
		sh->base_height = 65535;
	}
	else if (sh->base_height < 0)
	{
		sh->base_height = 0;
	}
	if (sh->width_inc > 65535)
	{
		sh->width_inc = 65535;
	}
	else if (sh->width_inc < 0)
	{
		sh->width_inc = 0;
	}
	if (sh->height_inc > 65535)
	{
		sh->height_inc = 65535;
	}
	else if (sh->height_inc < 0)
	{
		sh->height_inc = 0;
	}

	return;
}

/* ---------------------------- Functions not present in Xlib -------------- */

int FWeedIfEvents(
	Display *display,
	int (*weed_predicate) (Display *display, XEvent *event, XPointer arg),
	XPointer arg)
{
	_fev_weed_args weed_args;
	XEvent e;

	assert(fev_is_invalid_event_type_set);
	memset(&weed_args, 0, sizeof(weed_args));
	weed_args.weed_predicate = weed_predicate;
	weed_args.arg = arg;
	FCheckPeekIfEvent(
		display, &e, _fev_pred_weed_if, (XPointer)&weed_args);
	/* e is discarded */
	_fev_pred_weed_if_finish(&weed_args);

	return weed_args.count;
}

int FWeedIfWindowEvents(
	Display *display, Window window,
	int (*weed_predicate) (
		Display *display, XEvent *current_event, XPointer arg),
	XPointer arg)
{
	_fev_weed_args weed_args;
	XEvent e;

	assert(fev_is_invalid_event_type_set);
	memset(&weed_args, 0, sizeof(weed_args));
	weed_args.weed_predicate = weed_predicate;
	weed_args.arg = arg;
	weed_args.w = window;
	weed_args.has_window = 1;
	FCheckPeekIfEvent(
		display, &e, _fev_pred_weed_if, (XPointer)&weed_args);
	/* e is discarded */
	_fev_pred_weed_if_finish(&weed_args);

	return weed_args.count;
}

int FCheckWeedTypedWindowEvents(
	Display *display, Window window, int event_type, XEvent *last_event)
{
	_fev_weed_args weed_args;
	XEvent e;

	assert(fev_is_invalid_event_type_set);
	memset(&weed_args, 0, sizeof(weed_args));
	weed_args.w = window;
	weed_args.event_type = event_type;
	weed_args.has_window = 1;
	weed_args.has_event_type = 1;
	weed_args.ret_last_weeded_event = last_event;
	FCheckPeekIfEvent(
		display, &e, _fev_pred_weed_if, (XPointer)&weed_args);
	/* e is discarded */
	_fev_pred_weed_if_finish(&weed_args);

	return weed_args.count;
}

Bool FCheckPeekIfEvent(
	Display *display, XEvent *event_return,
	Bool (*predicate) (Display *display, XEvent *event, XPointer arg),
	XPointer arg)
{
	XEvent dummy;
	_fev_check_peek_args cpa;

	cpa.predicate = predicate;
	cpa.arg = arg;
	cpa.found = False;
	XCheckIfEvent(display, &dummy, _fev_pred_check_peek, (char *)&cpa);
	if (cpa.found == True)
	{
		*event_return = cpa.event;
		fev_update_last_timestamp(event_return);
	}

	return cpa.found;
}

/* ---------------------------- X event replacements ----------------------- */

XTimeCoord *FGetMotionEvents(
	Display *display, Window w, Time start, Time stop, int *nevents_return)
{
	XTimeCoord *rc;

	rc = XGetMotionEvents(display, w, start, stop, nevents_return);

	return rc;
}

int FAllowEvents(
	Display *display, int event_mode, Time time)
{
	int rc;

	rc = XAllowEvents(display, event_mode, time);

	return rc;
}

Bool FCheckIfEvent(
	Display *display, XEvent *event_return,
	Bool (*predicate) (Display *display, XEvent *event, XPointer arg),
	XPointer arg)
{
	Bool rc;
	XEvent new_ev;

	rc = XCheckIfEvent(display, &new_ev, predicate, arg);
	if (rc == True)
	{
		fev_event_old = fev_event;
		fev_event = new_ev;
		*event_return = fev_event;
		fev_update_last_timestamp(event_return);
	}

	return rc;
}

Bool FCheckMaskEvent(
	Display *display, long event_mask, XEvent *event_return)
{
	Bool rc;
	XEvent new_ev;

	rc = XCheckMaskEvent(display, event_mask, &new_ev);
	if (rc == True)
	{
		fev_event_old = fev_event;
		fev_event = new_ev;
		*event_return = fev_event;
		fev_update_last_timestamp(event_return);
	}

	return rc;
}

Bool FCheckTypedEvent(
	Display *display, int event_type, XEvent *event_return)
{
	Bool rc;
	XEvent new_ev;

	rc = XCheckTypedEvent(display, event_type, &new_ev);
	if (rc == True)
	{
		fev_event_old = fev_event;
		fev_event = new_ev;
		*event_return = fev_event;
		fev_update_last_timestamp(event_return);
	}

	return rc;
}

Bool FCheckTypedWindowEvent(
	Display *display, Window w, int event_type, XEvent *event_return)
{
	Bool rc;
	XEvent new_ev;

	rc = XCheckTypedWindowEvent(display, w, event_type, &new_ev);
	if (rc == True)
	{
		fev_event_old = fev_event;
		fev_event = new_ev;
		*event_return = fev_event;
		fev_update_last_timestamp(event_return);
	}

	return rc;
}

Bool FCheckWindowEvent(
	Display *display, Window w, long event_mask, XEvent *event_return)
{
	Bool rc;
	XEvent new_ev;

	rc = XCheckWindowEvent(display, w, event_mask, &new_ev);
	if (rc == True)
	{
		fev_event_old = fev_event;
		fev_event = new_ev;
		*event_return = fev_event;
		fev_update_last_timestamp(event_return);
	}

	return rc;
}

int FEventsQueued(
	Display *display, int mode)
{
	int rc;

	rc = XEventsQueued(display, mode);

	return rc;
}

int FIfEvent(
	Display *display, XEvent *event_return,
	Bool (*predicate) (Display *display, XEvent *event, XPointer arg),
	XPointer arg)
{
	int rc;

	fev_event_old = fev_event;
	rc = XIfEvent(display, &fev_event, predicate, arg);
	*event_return = fev_event;
	fev_update_last_timestamp(event_return);

	return rc;
}

int FMaskEvent(
	Display *display, long event_mask, XEvent *event_return)
{
	int rc;

	fev_event_old = fev_event;
	rc = XMaskEvent(display, event_mask, &fev_event);
	*event_return = fev_event;
	fev_update_last_timestamp(event_return);

	return rc;
}

int FNextEvent(
	Display *display, XEvent *event_return)
{
	int rc;

	fev_event_old = fev_event;
	rc = XNextEvent(display, &fev_event);
	*event_return = fev_event;
	fev_update_last_timestamp(event_return);

	return rc;
}

int FPeekEvent(
	Display *display, XEvent *event_return)
{
	int rc;

	rc = XPeekEvent(display, event_return);
	fev_update_last_timestamp(event_return);

	return rc;
}

int FPeekIfEvent(
	Display *display, XEvent *event_return,
	Bool (*predicate) (Display *display, XEvent *event, XPointer arg),
	XPointer arg)
{
	int rc;

	rc = XPeekIfEvent(display, event_return, predicate, arg);
	if (rc == True)
	{
		fev_update_last_timestamp(event_return);
	}

	return rc;
}

int FPending(
	Display *display)
{
	int rc;

	rc = XPending(display);

	return rc;
}

int FPutBackEvent(
	Display *display, XEvent *event)
{
	int rc;

	rc = XPutBackEvent(display, event);
	fev_event = fev_event_old;

	return rc;
}

int FQLength(
	Display *display)
{
	int rc;

	rc = XQLength(display);

	return rc;
}

Bool FQueryPointer(
	Display *display, Window w, Window *root_return, Window *child_return,
	int *root_x_return, int *root_y_return, int *win_x_return,
	int *win_y_return, unsigned int *mask_return)
{
	Bool rc;

	rc = XQueryPointer(
		display, w, root_return, child_return, root_x_return,
		root_y_return, win_x_return, win_y_return, mask_return);

	return rc;
}

Status FSendEvent(
	Display *display, Window w, Bool propagate, long event_mask,
	XEvent *event_send)
{
	Status rc;

	rc = XSendEvent(display, w, propagate, event_mask, event_send);

	return rc;
}

int FWarpPointer(
	Display *display, Window src_w, Window dest_w, int src_x, int src_y,
	unsigned int src_width, unsigned int src_height, int dest_x, int dest_y)
{
	int rc;

	rc = XWarpPointer(
		display, src_w, dest_w, src_x, src_y, src_width, src_height,
		dest_x, dest_y);

	return rc;
}

int FWarpPointerUpdateEvpos(
	XEvent *ev, Display *display, Window src_w, Window dest_w, int src_x,
	int src_y, unsigned int src_width, unsigned int src_height,
	int dest_x, int dest_y)
{
	int rc;

	rc = XWarpPointer(
		display, src_w, dest_w, src_x, src_y, src_width, src_height,
		dest_x, dest_y);
	if (ev != NULL && dest_w == DefaultRootWindow(display))
	{
		fev_set_evpos(ev, dest_x, dest_y);
	}

	return rc;
}

int FWindowEvent(
	Display *display, Window w, long event_mask, XEvent *event_return)
{
	int rc;

	fev_event_old = fev_event;
	rc = XWindowEvent(display, w, event_mask, &fev_event);
	*event_return = fev_event;
	fev_update_last_timestamp(event_return);

	return rc;
}

Status FGetWMNormalHints(
	Display *display, Window w, XSizeHints *hints_return,
	long *supplied_return)
{
	Status ret;

	ret = XGetWMNormalHints(display, w, hints_return, supplied_return);
	fev_sanitize_size_hints(hints_return);

	return ret;
}
