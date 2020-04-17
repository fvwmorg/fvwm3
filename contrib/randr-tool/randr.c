/* rand.c -- fvwm3 diagnostic tool to monitor for RandR changes. */
/*
 * Released under the same licence as FVWM.  Sometime in 2020 under a pandemic
 * cloud.
 *
 * Author:  Thomas Adam <thomas@fvwm.org>
 *
 * To compile (install librandr-dev and libx11-dev):
 *
 * gcc ./randr.c -lXrandr -lX11 -ggdb -O0 -o randr
 *
 * Information is spat to stdout/stderr.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/randr.h>
#include <sys/queue.h>

struct screen_info {
	int 	 w;
	int 	 h;
	int 	 x;
	int 	 y;

	char 	*name;
	bool 	 is_primary;
	bool 	 is_disabled;
	bool 	 wants_update;
};

struct monitor {
	struct screen_info 	 *si;
	TAILQ_ENTRY(monitor) entry;
};
TAILQ_HEAD(monitors, monitor);

struct monitors		 monitor_q;

struct monitor 		*monitor_new(void);
struct monitor 		*monitor_by_output_info(const char *, XRRMonitorInfo *);
struct screen_info	*screen_info_new(void);
struct screen_info	*screen_info_by_name(const char *);

Display 	*dpy = NULL;
Window 		 root;
XRRScreenChangeNotifyEvent *sce;
int 		 event_base, error_base;
int 		 major, minor;
int 		 screen = -1;
XEvent 		 event;

bool is_randr_newest(void);
void scan_for_monitors(void);

struct monitor *
monitor_new(void)
{
	struct monitor 	*m;

	m = calloc(1, sizeof *m);

	return (m);
}

struct monitor *
monitor_by_output_info(const char *name, XRRMonitorInfo *rrm)
{
	struct monitor 	 *m, *m_ret = NULL;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		if (strcmp(m->si->name, name) == 0) {
			m_ret = m;
			break;
		}
	}

	if (m_ret != NULL) {
		return (m_ret);
	}

	return (NULL);
}

struct screen_info *
screen_info_by_name(const char *name)
{
	struct screen_info 	*si = NULL;
	struct monitor 		*m, *m2 = NULL;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		if (strcmp(m->si->name, name) == 0) {
			m2 = m;
			break;
		}
	}

	if (m2 != NULL)
		si = m2->si;

	return (si);
}

struct screen_info *
screen_info_new(void)
{
	struct screen_info 	*si;

	si = calloc(1, sizeof *si);

	return (si);
}

bool is_randr_newest(void)
{
	if ((RANDR_MAJOR * 100) + RANDR_MINOR < 105) {
		fprintf(stderr, "Error: randr v1.5+ required\n");
		return false;
	}
	return true;
}

void
poll_x(void)
{
	struct monitor 	*m;

	screen = DefaultScreen(dpy);
	for (;;) {
		XNextEvent(dpy, (XEvent *)&event);

		/* update Xlib's knowledge of the event */
		XRRUpdateConfiguration(&event);

		switch (event.type - event_base) {
		case RRScreenChangeNotify:
		{
			struct screen_info 	*si;
			XRRScreenResources *sres;
			XRROutputInfo *oinfo;
			int i;
			sce = (XRRScreenChangeNotifyEvent *) &event;
			sres = XRRGetScreenResources(sce->display, sce->root);
			
			if (sres == NULL)
				break;

			scan_for_monitors();

			for (i = 0; i < sres->noutput; ++i) {
				oinfo = XRRGetOutputInfo(sce->display, sres,
					    sres->outputs[i]);

				si = screen_info_by_name(oinfo->name);
				
				if (si != NULL) {
					switch (oinfo->connection) {
					case RR_Disconnected:
						si->is_disabled = true;
						break;
					case RR_Connected:
						si->is_disabled = false;
						break;
					default:
						break;
					}
				}
			}

			TAILQ_FOREACH(m, &monitor_q, entry) {
				printf("monitor changed: %s (primary: %s) (%s) "
					"{x: %d, y: %d, w: %d, h: %d}\n",
					m->si->name,
					m->si->is_primary ? "true" : "false",
					m->si->is_disabled ? "disconnected" : "connected",
					m->si->x, m->si->y, m->si->w, m->si->h);
			}
			XRRFreeScreenResources(sres);
			XRRFreeOutputInfo(oinfo);
			break;
		}
		case RRNotify_OutputChange:
		{
			XRROutputChangeNotifyEvent *oce =
				(XRROutputChangeNotifyEvent *)&event;
			XRRScreenConfiguration *sc;
			XRRScreenResources *sres;
			XRROutputInfo *oinfo;
			XRRScreenSize *sizes;
			Rotation current_rotation;
			int nsize;

			sres = XRRGetScreenResources(oce->display, oce->window);
			sc = XRRGetScreenInfo(oce->display, oce->window);
			oinfo = XRRGetOutputInfo(oce->display, sres, oce->output);
			XRRConfigCurrentConfiguration(sc, &current_rotation);
			sizes = XRRConfigSizes(sc, &nsize);
			fprintf(stderr, "%s (%s) %d %dx%d\n",
				oinfo->name,
				oinfo->connection == RR_Connected ?
				  "connected" : "disconnected",
				0,
				sizes[0].width,
				sizes[0].height
			);
			XRRFreeScreenResources(sres);
			XRRFreeOutputInfo(oinfo);
			break;
		}
		}
	}
}

void
scan_for_monitors(void)
{
	XRRMonitorInfo		*rrm;
	struct monitor 		*m;
    	int			i, n = 0;

	rrm = XRRGetMonitors(dpy, root, false, &n);
	if (n == -1) {
		fprintf(stderr, "get monitors failed\n");
		exit(1);
	}

	for (i = 0; i < n; i++) {

		char *name = XGetAtomName(dpy, rrm[i].name);

		if (name == NULL)
			name = "";

		if ((m = monitor_by_output_info(name, &rrm[i])) == NULL) {
			m = monitor_new();
			m->si = screen_info_new();
			m->si->name = strdup(name);

			TAILQ_INSERT_TAIL(&monitor_q, m, entry);

			printf("New monitor: %s\n", m->si->name);
		}

		m->si->x = rrm[i].x;
		m->si->y = rrm[i].y;
		m->si->w = rrm[i].width;
		m->si->h = rrm[i].height;
		m->si->is_primary = rrm[i].primary > 0;

	}
}

int
main(void)
{
	struct monitor 	*m;

	if ((dpy = XOpenDisplay(NULL)) == NULL) {
		fprintf(stderr, "Couldn't open display\n");
		return (1);
	}

	printf("Starting...\n");

	TAILQ_INIT(&monitor_q);

	root = RootWindow(dpy, DefaultScreen(dpy));
	if (!XRRQueryExtension(dpy, &event_base, &error_base) ||
        	!XRRQueryVersion (dpy, &major, &minor))
    	{
		fprintf (stderr, "RandR extension missing\n");
		return (1);
	}
	if (!is_randr_newest())
		return(1);


	XSelectInput (dpy, root, StructureNotifyMask);
	XRRSelectInput(dpy, DefaultRootWindow(dpy),
			RRScreenChangeNotifyMask | RROutputChangeNotifyMask);

	scan_for_monitors();

	poll_x();

	return (0);
}
