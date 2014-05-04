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
/*
  This file is strongly based on the corresponding files from
  twm and enlightenment.
*/

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#ifdef HAVE_GETPWUID
#include <pwd.h>
#endif
#include <signal.h>
#include <fcntl.h>
#include <X11/Xatom.h>

#include "libs/fvwmlib.h"
#include "libs/FSMlib.h"
#include "libs/Strings.h"
#include "libs/System.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "add_window.h"
#include "misc.h"
#include "screen.h"
#include "session.h"
#include "module_list.h"
#include "module_interface.h"
#include "stack.h"
#include "icccm2.h"
#include "virtual.h"
#include "geometry.h"
#include "move_resize.h"
#include "infostore.h"

/* ---------------------------- local definitions -------------------------- */

/*#define FVWM_SM_DEBUG_PROTO*/
/*#define FVWM_SM_DEBUG_WINMATCH*/
#define FVWM_SM_DEBUG_FILES

/* ---------------------------- local macros ------------------------------- */

#define xstreq(a,b) ((!a && !b) || (a && b && (strcmp(a,b)==0)))

/* ---------------------------- imports ------------------------------------ */

extern Bool Restarting;
extern int master_pid;
extern char **g_argv;
extern int g_argc;

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef struct _match
{
	unsigned long win;
	char *client_id;
	char *res_name;
	char *res_class;
	char *window_role;
	char *wm_name;
	int wm_command_count;
	char **wm_command;
	int x, y, w, h, icon_x, icon_y;
	int x_max, y_max, w_max, h_max;
	int width_defect_max, height_defect_max;
	int max_x_offset, max_y_offset;
	int desktop;
	int layer;
	int default_layer;
	int placed_by_button;
	int used;
	int gravity;
	unsigned long ewmh_hint_desktop;
	window_flags flags;
} Match;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static char *previous_sm_client_id = NULL;
static char *sm_client_id = NULL;
static Bool sent_save_done = 0;
static char *real_state_filename = NULL;
static Bool going_to_restart = False;
static FIceIOErrorHandler prev_handler;
static FSmcConn sm_conn = NULL;

static int num_match = 0;
static Match *matches = NULL;
static Bool does_file_version_match = False;
static Bool do_preserve_state = True;

/* ---------------------------- exported variables (globals) --------------- */

int sm_fd = -1;

/* ---------------------------- local functions ---------------------------- */

static
char *duplicate(const char *s)
{
	int l;
	char *r;

	/* TA:  FIXME!  Use xasprintf() */

	if (!s) return NULL;
	l = strlen(s);
	r = xmalloc (sizeof(char)*(l+1));
	strncpy(r, s, l+1);

	return r;
}

static char *get_version_string(void)
{
	/* migo (14-Mar-2001): it is better to manually update a version string
	 * in the stable branch, otherwise saving sessions becomes useless */
	return CatString3(VERSION, ", ", __DATE__);
	/* return "2.6-0"; */
}

static char *unspace_string(const char *str)
{
	static const char *spaces = " \t\n";
	char *tr_str = CatString2(str, NULL);
	int i;

	if (!tr_str)
	{
		return NULL;
	}
	for (i = 0; i < strlen(spaces); i++)
	{
		char *ptr = tr_str;
		while ((ptr = strchr(ptr, spaces[i])) != NULL)
		{
			*(ptr++) = '_';
		}
	}

	return tr_str;
}

/*
 * It is a bit ugly to have a separate file format for
 * config files and session save files. The proper way
 * to do this may be to extend the config file format
 * to allow the specification of everything we need
 * to save here. Then the option "-restore xyz" could
 * be replaced by "-f xyz".
 */
static int
SaveGlobalState(FILE *f)
{
	fprintf(f, "[GLOBAL]\n");
	fprintf(f, "  [DESKTOP] %i\n", Scr.CurrentDesk);
	fprintf(f, "  [VIEWPORT] %i %i %i %i\n",
		Scr.Vx, Scr.Vy, Scr.VxMax, Scr.VyMax);
	fprintf(f, "  [SCROLL] %i %i %i %i %i\n",
		Scr.EdgeScrollX, Scr.EdgeScrollY, Scr.ScrollDelay,
		!!(Scr.flags.do_edge_wrap_x), !!(Scr.flags.do_edge_wrap_y));
	fprintf(f, "  [MISC] %i %i %i\n",
		Scr.ClickTime, Scr.ColormapFocus, Scr.ColorLimit);
	fprintf(
		f, "  [STYLE] %i %i\n", Scr.gs.do_emulate_mwm,
		Scr.gs.do_emulate_win);

	if (get_metainfo_length() > 0) {
		MetaInfo *mi = get_metainfo(), *mi_i;

		fprintf(f, "  [INFOSTORE]\n");
		for (mi_i = mi; mi_i; mi_i = mi_i->next) {
			fprintf(f, "    [KEY] %s\n", mi_i->key);
			fprintf(f, "    [VALUE] %s\n", mi_i->value);
		}
	}


	return 1;
}

static void set_real_state_filename(char *filename)
{
	if (!SessionSupport)
	{
		return;
	}
	if (real_state_filename)
	{
		free(real_state_filename);
	}
	real_state_filename = xstrdup(filename);

	return;
}

static char *get_unique_state_filename(void)
{
	const char *path = getenv("SM_SAVE_DIR");
	char *filename;
	int fd;

	if (!SessionSupport)
	{
		return NULL;
	}

	if (!path)
	{
		path = getenv ("HOME");
	}

#ifdef HAVE_GETPWUID
	if (!path)
	{
		struct passwd *pwd;

		pwd = getpwuid(getuid());
		if (pwd)
		{
			path = pwd->pw_dir;
		}
	}
#endif
	if (!path)
	{
		return NULL;
	}
	filename = xstrdup(CatString2(path, "/.fs-XXXXXX"));
	fd = fvwm_mkstemp(filename);
	if (fd == -1)
	{
		free (filename);
		filename = NULL;
	}
	else
	{
		close (fd);
	}

	return filename;
}

static char *
GetWindowRole(Window window)
{
	XTextProperty tp;

	if (XGetTextProperty (dpy, window, &tp, _XA_WM_WINDOW_ROLE))
	{
		if (tp.encoding == XA_STRING && tp.format == 8 &&
		    tp.nitems != 0)
		{
			return ((char *) tp.value);
		}
	}
	if (XGetTextProperty (dpy, window, &tp, _XA_WINDOW_ROLE))
	{
		if (tp.encoding == XA_STRING && tp.format == 8 &&
		    tp.nitems != 0)
		{
			return ((char *) tp.value);
		}
	}

	return NULL;
}

static char *
GetClientID(FvwmWindow *fw)
{
	char *client_id = NULL;
	Window client_leader = None;
	Window window;
	XTextProperty tp;
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char *prop = NULL;

	window = FW_W(fw);

	if (XGetWindowProperty(
		    dpy, window, _XA_WM_CLIENT_LEADER, 0L, 1L, False,
		    AnyPropertyType, &actual_type, &actual_format, &nitems,
		    &bytes_after, &prop) == Success)
	{
		if (actual_type == XA_WINDOW && actual_format == 32 &&
		    nitems == 1 && bytes_after == 0)
		{
			client_leader = (Window)(*(long *)prop);
		}
	}

	if (!client_leader && fw->wmhints &&
	    (fw->wmhints->flags & WindowGroupHint))
	{
		client_leader = fw->wmhints->window_group;
	}

	if (client_leader)
	{
		if (
			XGetTextProperty(
				dpy, client_leader, &tp, _XA_SM_CLIENT_ID))
		{
			if (tp.encoding == XA_STRING && tp.format == 8 &&
			    tp.nitems != 0)
			{
				client_id = (char *) tp.value;
			}
		}
	}

	if (prop)
	{
		XFree (prop);
	}

	return client_id;
}

/*
**  Verify the current fvwm version with the version that stroed the state file.
**  No state will be restored if versions don't match.
*/
static Bool VerifyVersionInfo(char *filename)
{
	FILE *f;
	char s[4096], s1[4096];

	if (!filename || !*filename)
	{
		return False;
	}
	if ((f = fopen(filename, "r")) == NULL)
	{
		return False;
	}

	while (fgets(s, sizeof(s), f))
	{
		sscanf(s, "%4000s", s1);
		if (!strcmp(s1, "[FVWM_VERSION]"))
		{
			char *current_v = get_version_string();
			sscanf(s, "%*s %[^\n]", s1);
			if (strcmp(s1, current_v) == 0)
			{
				does_file_version_match = True;
			}
			else
			{
				fvwm_msg(
					ERR, "VerifyVersionInfo",
					"State file version (%s) does not"
					" match the current version (%s), "
					"state file is ignored.", s1,
					current_v);
				break;
			}
		}
	}
	fclose(f);

	return does_file_version_match;
}

static int
SaveVersionInfo(FILE *f)
{
	fprintf(f, "[FVWM_VERSION] %s\n", get_version_string());

	return 1;
}

static int
SaveWindowStates(FILE *f)
{
	char *client_id;
	char *window_role;
	char **wm_command;
	int wm_command_count;
	FvwmWindow *ewin;
	rectangle save_g;
	rectangle ig;
	int i;
	int layer;

	for (ewin = get_next_window_in_stack_ring(&Scr.FvwmRoot);
	     ewin != &Scr.FvwmRoot;
	     ewin = get_next_window_in_stack_ring(ewin))
	{
		Bool is_icon_sticky_across_pages;

		if (!XGetGeometry(
			    dpy, FW_W(ewin), &JunkRoot, &JunkX, &JunkY,
			    (unsigned int*)&JunkWidth,
			    (unsigned int*)&JunkHeight,
			    (unsigned int*)&JunkBW,
			    (unsigned int*)&JunkDepth))
		{
			/* Don't save the state of windows that already died
			 * (i.e. modules)! */
			continue;
		}
		is_icon_sticky_across_pages =
			is_window_sticky_across_pages(ewin);

		wm_command = NULL;
		wm_command_count = 0;

		client_id = GetClientID(ewin);
		if (!client_id)
		{
			/* no client id, some session manager do not manage
			 * such client ... this can cause problem */
			if (XGetCommand(
				    dpy, FW_W(ewin), &wm_command,
				    &wm_command_count) &&
			    wm_command && wm_command_count > 0)
			{
				/* ok */
			}
			else
			{
				/* No client id and no WM_COMMAND, the client
				 * cannot be managed by the sessiom manager
				 * skip it! */
				/* TA:  20110611 - But actually, this breaks
				 * those applications which don't set the
				 * WM_COMMAND XAtom anymore.  The ICCCM
				 * deprecated this at version 2.0 -- and its
				 * lack of existence here shouldn't be a
				 * problem.  Let newer session managers handle
				 * the error if it even matters.
				 */
				if (!Restarting)
				{
					if (wm_command)
					{
						XFreeStringList(wm_command);
						wm_command = NULL;
					}
					/* TA: 20110611 - But see above.  We
					 * no longer skip clients who don't
					 * set this legacy field.
					 */
					/* continue; */
				}
			}
		}

		fprintf(f, "[CLIENT] %lx\n", FW_W(ewin));
		if (client_id)
		{
			fprintf(f, "  [CLIENT_ID] %s\n", client_id);
			XFree(client_id);
		}

		window_role = GetWindowRole(FW_W(ewin));
		if (window_role)
		{
			fprintf(f, "  [WINDOW_ROLE] %s\n", window_role);
			XFree(window_role);
		}
		if (client_id && window_role)
		{
			/* we have enough information */
		}
		else
		{
			if (ewin->class.res_class)
			{
				fprintf(f, "  [RES_NAME] %s\n",
					ewin->class.res_name);
			}
			if (ewin->class.res_name)
			{
				fprintf(f, "  [RES_CLASS] %s\n",
					ewin->class.res_class);
			}
			if (ewin->name.name)
			{
				fprintf(f, "  [WM_NAME] %s\n",
					ewin->name.name);
			}

			if (wm_command && wm_command_count > 0)
			{
				fprintf(f, "  [WM_COMMAND] %i",
					wm_command_count);
				for (i = 0; i < wm_command_count; i++)
				{
					fprintf(f, " %s",
						unspace_string(wm_command[i]));
				}
				fprintf(f, "\n");
			}
		} /* !window_role */

		if (wm_command)
		{
			XFreeStringList(wm_command);
			wm_command = NULL;
		}

		gravity_get_naked_geometry(
			ewin->hints.win_gravity, ewin, &save_g,
			&ewin->g.normal);
		if (IS_STICKY_ACROSS_PAGES(ewin))
		{
			save_g.x -= Scr.Vx;
			save_g.y -= Scr.Vy;
		}
		get_visible_icon_geometry(ewin, &ig);
		fprintf(
			f, "  [GEOMETRY] %i %i %i %i %i %i %i %i %i %i %i %i"
			" %i %i %i\n",
			save_g.x, save_g.y, save_g.width, save_g.height,
			ewin->g.max.x, ewin->g.max.y, ewin->g.max.width,
			ewin->g.max.height, ewin->g.max_defect.width,
			ewin->g.max_defect.height,
			ig.x + ((!is_icon_sticky_across_pages) ? Scr.Vx : 0),
			ig.y + ((!is_icon_sticky_across_pages) ? Scr.Vy : 0),
			ewin->hints.win_gravity,
			ewin->g.max_offset.x, ewin->g.max_offset.y);
		fprintf(f, "  [DESK] %i\n", ewin->Desk);
		/* set the layer to the default layer if the layer has been
		 * set by an ewmh hint */
		layer = get_layer(ewin);
		if (layer == ewin->ewmh_hint_layer && layer > 0)
		{
			layer = Scr.DefaultLayer;
		}
		fprintf(f, "  [LAYER] %i %i\n", layer, ewin->default_layer);
		fprintf(f, "  [PLACED_BY_BUTTON] %i\n", ewin->placed_by_button);
		fprintf(f, "  [EWMH_DESKTOP] %lu\n", ewin->ewmh_hint_desktop);
		fprintf(f, "  [FLAGS] ");
		for (i = 0; i < sizeof(window_flags); i++)
		{
			fprintf(f, "%02x ",
				(int)(((unsigned char *)&(ewin->flags))[i]));
		}
		fprintf(f, "\n");
	}
	return 1;
}

/* This complicated logic is from twm, where it is explained */
static Bool matchWin(FvwmWindow *w, Match *m)
{
	char *client_id = NULL;
	char *window_role = NULL;
	char **wm_command = NULL;
	int wm_command_count = 0, i;
	int found;

	found = 0;
	client_id = GetClientID(w);

	if (Restarting)
	{
		if (FW_W(w) == m->win)
		{
			found = 1;
		}
	}
	else if (xstreq(client_id, m->client_id))
	{

		/* client_id's match */

		window_role = GetWindowRole(FW_W(w));

		if (client_id && (window_role || m->window_role))
		{
			/* We have or had a window role, base decision on it */
			found = xstreq(window_role, m->window_role);
		}
		else if (xstreq(w->class.res_name, m->res_name) &&
			 xstreq(w->class.res_class, m->res_class) &&
			 (IS_NAME_CHANGED(w) || IS_NAME_CHANGED(m) ||
			  xstreq(w->name.name, m->wm_name)))
		{
			if (client_id)
			{
				/* If we have a client_id, we don't
				 * compare WM_COMMAND, since it will be
				 * different. */
				found = 1;
			}
			else
			{
				/* for non-SM-aware clients we also
				 * compare WM_COMMAND */
				if (!XGetCommand(
					    dpy, FW_W(w), &wm_command,
					    &wm_command_count))
				{
					wm_command = NULL;
					wm_command_count = 0;
				}
				if (wm_command_count == m->wm_command_count)
				{
					for (i = 0; i < wm_command_count; i++)
					{
						if (strcmp(unspace_string(
								   wm_command[i]),
							   m->wm_command[i])!=0)
						{
							break;
						}
					}

					if (i == wm_command_count)
					{
						/* migo (21/Oct/1999):
						 * on restarts compare
						 * window ids too */
						/* But if we restart we only need
						 * to compare window ids
						 * olicha (2005-01-06) */
						found = 1;
					}
				} /* if (wm_command_count ==... */
			} /* else if res_class, res_name and wm_name agree */
		} /* else no window roles */
	} /* if client_id's agree */

#ifdef FVWM_SM_DEBUG_WINMATCH
	fprintf(stderr,
		"\twin(%s, %s, %s, %s, %s,",
		w->class.res_name, w->class.res_class, w->name.name,
		(client_id)? client_id:"(null)",
		(window_role)? window_role:"(null)");
	if (wm_command)
	{
		for (i = 0; i < wm_command_count; i++)
		{
			fprintf(stderr," %s", wm_command[i]);
		}
		fprintf(stderr,",");
	}
	else
	{
		fprintf(stderr," no_wmc,");
	}
	fprintf(stderr," %d)", IS_NAME_CHANGED(w));
	fprintf(stderr,"\n[%d]", found);
	fprintf(stderr,
		"\tmat(%s, %s, %s, %s, %s,",
		m->res_name, m->res_class, m->wm_name,
		(m->client_id)?m->client_id:"(null)",
		(m->window_role)?m->window_role:"(null)");
	if (m->wm_command)
	{
		for (i = 0; i < m->wm_command_count; i++)
		{
			fprintf(stderr," %s", m->wm_command[i]);
		}
		fprintf(stderr,",");
	}
	else
	{
		fprintf(stderr," no_wmc,");
	}
	fprintf(stderr," %d)\n\n", IS_NAME_CHANGED(m));
#endif

	if (client_id)
	{
		XFree(client_id);
	}
	if (window_role)
	{
		XFree(window_role);
	}
	if (wm_command)
	{
		XFreeStringList (wm_command);
	}

	return found;
}

static int save_state_file(char *filename)
{
	FILE *f;
	int success;

	if (!filename || !*filename)
	{
		return 0;
	}
	if ((f = fopen(filename, "w")) == NULL)
	{
		return 0;
	}

	fprintf(f, "# This file is generated by fvwm."
		" It stores global and window states.\n");
	fprintf(f, "# Normally, you must never delete this file,"
		" it will be auto-deleted.\n\n");

	if (SessionSupport && going_to_restart)
	{
		fprintf(f, "[REAL_STATE_FILENAME] %s\n", real_state_filename);
		going_to_restart = False;  /* not needed */
	}

	success = do_preserve_state
		? SaveVersionInfo(f) && SaveWindowStates(f) && SaveGlobalState(f)
		: 1;
	do_preserve_state = True;
	if (fclose(f) != 0)
		return 0;

#ifdef FVWM_SM_DEBUG_FILES
	system(CatString3(
		       "mkdir -p /tmp/fs-save; cp ", filename,
		       " /tmp/fs-save"));
#endif
#if defined(FVWM_SM_DEBUG_PROTO) || defined(FVWM_SM_DEBUG_FILES)
	fprintf(stderr, "[FVWM_SMDEBUG] Saving %s\n", filename);
#endif

	return success;
}

static void
set_sm_properties(FSmcConn sm_conn, char *filename, char hint)
{
	FSmProp prop1, prop2, prop3, prop4, prop5, prop6, prop7, *props[7];
	FSmPropValue prop1val, prop2val, prop3val, prop4val, prop7val;
	struct passwd *pwd;
	char *user_id;
	char screen_num[32];
	int numVals, i, priority = 30;
	Bool is_xsm_detected = False;

	if (!SessionSupport)
	{
		return;
	}

#ifdef FVWM_SM_DEBUG_PROTO
	fprintf(stderr, "[FVWM_SMDEBUG][set_sm_properties] state filename: %s%s\n",
		filename ? filename : "(null)", sm_conn ? "" : " - not connected");
#endif

	if (!sm_conn)
	{
		return;
	}

	pwd = getpwuid (getuid());
	user_id = pwd->pw_name;

	prop1.name = FSmProgram;
	prop1.type = FSmARRAY8;
	prop1.num_vals = 1;
	prop1.vals = &prop1val;
	prop1val.value = g_argv[0];
	prop1val.length = strlen (g_argv[0]);

	prop2.name = FSmUserID;
	prop2.type = FSmARRAY8;
	prop2.num_vals = 1;
	prop2.vals = &prop2val;
	prop2val.value = (FSmPointer) user_id;
	prop2val.length = strlen (user_id);

	prop3.name = FSmRestartStyleHint;
	prop3.type = FSmCARD8;
	prop3.num_vals = 1;
	prop3.vals = &prop3val;
	prop3val.value = (FSmPointer) &hint;
	prop3val.length = 1;

	prop4.name = "_GSM_Priority";
	prop4.type = FSmCARD8;
	prop4.num_vals = 1;
	prop4.vals = &prop4val;
	prop4val.value = (FSmPointer) &priority;
	prop4val.length = 1;

	sprintf(screen_num, "%d", (int)Scr.screen);

	prop5.name = FSmCloneCommand;
	prop5.type = FSmLISTofARRAY8;
	prop5.vals = (FSmPropValue *)malloc((g_argc + 2) * sizeof (FSmPropValue));
	numVals = 0;
	for (i = 0; i < g_argc; i++)
	{
		if (strcmp (g_argv[i], "-clientId") == 0 ||
		    strcmp (g_argv[i], "-restore") == 0 ||
		    strcmp (g_argv[i], "-d") == 0 ||
		    (strcmp (g_argv[i], "-s") == 0 && i+1 < g_argc &&
		     g_argv[i+1][0] != '-'))
		{
			i++;
		}
		else if (strcmp (g_argv[i], "-s") != 0)
		{
			prop5.vals[numVals].value = (FSmPointer) g_argv[i];
			prop5.vals[numVals++].length = strlen (g_argv[i]);
		}
	}

	prop5.vals[numVals].value = (FSmPointer) "-s";
	prop5.vals[numVals++].length = 2;

	prop5.vals[numVals].value = (FSmPointer) screen_num;
	prop5.vals[numVals++].length = strlen (screen_num);


	prop5.num_vals = numVals;

	if (filename)
	{
		prop6.name = FSmRestartCommand;
		prop6.type = FSmLISTofARRAY8;

		prop6.vals = (FSmPropValue *)malloc(
			(g_argc + 6) * sizeof (FSmPropValue));

		numVals = 0;

		for (i = 0; i < g_argc; i++)
		{
			if (strcmp (g_argv[i], "-clientId") == 0 ||
			    strcmp (g_argv[i], "-restore") == 0 ||
			    strcmp (g_argv[i], "-d") == 0 ||
			    (strcmp (g_argv[i], "-s") == 0 && i+1 < g_argc &&
			     g_argv[i+1][0] != '-'))
			{
				i++;
			}
			else if (strcmp (g_argv[i], "-s") != 0)
			{
				prop6.vals[numVals].value =
					(FSmPointer) g_argv[i];
				prop6.vals[numVals++].length =
					strlen (g_argv[i]);
			}
		}

		prop6.vals[numVals].value = (FSmPointer) "-s";
		prop6.vals[numVals++].length = 2;

		prop6.vals[numVals].value = (FSmPointer) screen_num;
		prop6.vals[numVals++].length = strlen (screen_num);

		prop6.vals[numVals].value = (FSmPointer) "-clientId";
		prop6.vals[numVals++].length = 9;

		prop6.vals[numVals].value = (FSmPointer) sm_client_id;
		prop6.vals[numVals++].length = strlen (sm_client_id);

		prop6.vals[numVals].value = (FSmPointer) "-restore";
		prop6.vals[numVals++].length = 8;

		prop6.vals[numVals].value = (FSmPointer) filename;
		prop6.vals[numVals++].length = strlen (filename);

		prop6.num_vals = numVals;

		prop7.name = FSmDiscardCommand;

		is_xsm_detected = StrEquals(getenv("SESSION_MANAGER_NAME"), "xsm");

		if (is_xsm_detected)
		{
			/* the protocol spec says that the discard command
			   should be LISTofARRAY8 on posix systems, but xsm
			   demands that it be ARRAY8.
			*/
			char *discardCommand = alloca(
				(10 + strlen(filename)) * sizeof(char));
			sprintf (discardCommand, "rm -f '%s'", filename);
			prop7.type = FSmARRAY8;
			prop7.num_vals = 1;
			prop7.vals = &prop7val;
			prop7val.value = (FSmPointer) discardCommand;
			prop7val.length = strlen (discardCommand);
		}
		else
		{
			prop7.type = FSmLISTofARRAY8;
			prop7.num_vals = 3;
			prop7.vals =
				(FSmPropValue *) malloc (
					3 * sizeof (FSmPropValue));
			prop7.vals[0].value = "rm";
			prop7.vals[0].length = 2;
			prop7.vals[1].value = "-f";
			prop7.vals[1].length = 2;
			prop7.vals[2].value = filename;
			prop7.vals[2].length = strlen (filename);
		}
	}

	props[0] = &prop1;
	props[1] = &prop2;
	props[2] = &prop3;
	props[3] = &prop4;
	props[4] = &prop5;
	SUPPRESS_UNUSED_VAR_WARNING(props);
	if (filename)
	{
		props[5] = &prop6;
		props[6] = &prop7;
		FSmcSetProperties (sm_conn, 7, props);

		free ((char *) prop6.vals);
		if (!is_xsm_detected)
		{
			free ((char *) prop7.vals);
		}
	}
	else
	{
		FSmcSetProperties (sm_conn, 5, props);
	}
	free ((char *) prop5.vals);
}

static void
callback_save_yourself2(FSmcConn sm_conn, FSmPointer client_data)
{
	Bool success = 0;
	char *filename;


	if (!SessionSupport)
	{
		return;
	}

	filename = get_unique_state_filename();
#ifdef FVWM_SM_DEBUG_PROTO
	fprintf(stderr, "[FVWM_SMDEBUG][callback_save_yourself2]\n");
#endif

	success = save_state_file(filename);
	if (success)
	{
		set_sm_properties(sm_conn, filename, FSmRestartIfRunning);
		set_real_state_filename(filename);
	}
	free(filename);

	FSmcSaveYourselfDone (sm_conn, success);
	sent_save_done = 1;
}

static void
callback_save_yourself(FSmcConn sm_conn, FSmPointer client_data,
		       int save_style, Bool shutdown, int interact_style,
		       Bool fast)
{

	if (!SessionSupport)
	{
		return;
	}

#ifdef FVWM_SM_DEBUG_PROTO
	fprintf(stderr, "[FVWM_SMDEBUG][callback_save_yourself] "
		"(save=%d, shut=%d, intr=%d, fast=%d)\n",
		save_style, shutdown, interact_style, fast);
#endif

	if (save_style == FSmSaveGlobal)
	{
		/* nothing to do */
#ifdef FVWM_SM_DEBUG_PROTO
		fprintf(stderr, "[FVWM_SMDEBUG][callback_save_yourself] "
		"Global Save type ... do nothing\n");
#endif
		FSmcSaveYourselfDone (sm_conn, True);
		sent_save_done = 1;
		return;

	}
#ifdef FVWM_SM_DEBUG_PROTO
	fprintf(stderr, "[FVWM_SMDEBUG][callback_save_yourself] "
		"Both or Local save type, going to phase 2 ...");
#endif
	if (!FSmcRequestSaveYourselfPhase2(
		    sm_conn, callback_save_yourself2, NULL))
	{
		FSmcSaveYourselfDone (sm_conn, False);
		sent_save_done = 1;
#ifdef FVWM_SM_DEBUG_PROTO
		fprintf(stderr, " failed!\n");
#endif
	}
	else
	{
#ifdef FVWM_SM_DEBUG_PROTO
		fprintf(stderr, " OK\n");
#endif
		sent_save_done = 0;
	}

	return;
}

static void
callback_die(FSmcConn sm_conn, FSmPointer client_data)
{

	if (!SessionSupport)
	{
		return;
	}

#ifdef FVWM_SM_DEBUG_PROTO
	fprintf(stderr, "[FVWM_SMDEBUG][callback_die]\n");
#endif

	if (FSmcCloseConnection(sm_conn, 0, NULL) != FSmcClosedNow)
	{
		/* go a head any way ? */
	}
	sm_fd = -1;

	if (master_pid != getpid())
	{
		kill(master_pid, SIGTERM);
	}
	Done(0, NULL);
}

static void
callback_save_complete(FSmcConn sm_conn, FSmPointer client_data)
{
	if (!SessionSupport)
	{
		return;
	}
#ifdef FVWM_SM_DEBUG_PROTO
	fprintf(stderr, "[FVWM_SMDEBUG][callback_save_complete]\n");
#endif

	return;
}

static void
callback_shutdown_cancelled(FSmcConn sm_conn, FSmPointer client_data)
{
	if (!SessionSupport)
	{
		return;
	}

#ifdef FVWM_SM_DEBUG_PROTO
	fprintf(stderr, "[FVWM_SMDEBUG][callback_shutdown_cancelled]\n");
#endif

	if (!sent_save_done)
	{
		FSmcSaveYourselfDone(sm_conn, False);
		sent_save_done = 1;
	}

	return;
}

/* the following is taken from xsm */
static void
MyIoErrorHandler(FIceConn ice_conn)
{
	if (!SessionSupport)
	{
		return;
	}

	if (prev_handler)
	{
		(*prev_handler) (ice_conn);
	}

	return;
}

static void
InstallIOErrorHandler(void)
{
	FIceIOErrorHandler default_handler;

	if (!SessionSupport)
	{
		return;
	}

	prev_handler = FIceSetIOErrorHandler (NULL);
	default_handler = FIceSetIOErrorHandler (MyIoErrorHandler);
	if (prev_handler == default_handler)
	{
		prev_handler = NULL;
	}

	return;
}

/* ---------------------------- interface functions ------------------------ */

void
LoadGlobalState(char *filename)
{
	FILE *f;
	char s[4096], s1[4096];
	/* char s2[256]; */
	char *is_key = NULL, *is_value = NULL;
	int n, i1, i2, i3, i4;

	if (!does_file_version_match)
	{
		return;
	}
	if (!filename || !*filename)
	{
		return;
	}
	if ((f = fopen(filename, "r")) == NULL)
	{
		return;
	}

	while (fgets(s, sizeof(s), f))
	{
		n = 0;

		i1 = 0;
		i2 = 0;
		i3 = 0;
		i4 = 0;

		sscanf(s, "%4000s%n", s1, &n);
		/* If we are restarting, [REAL_STATE_FILENAME] points
		 * to the file containing the true session state. */
		if (SessionSupport && !strcmp(s1, "[REAL_STATE_FILENAME]"))
		{
			/* migo: temporarily (?) moved to
			   LoadWindowStates (trick for gnome-session)
			   sscanf(s, "%*s %s", s2);
			   set_sm_properties(sm_conn, s2, FSmRestartIfRunning);
			   set_real_state_filename(s2);
			*/
		}
		else if (!strcmp(s1, "[DESKTOP]"))
		{
			sscanf(s, "%*s %i", &i1);
			goto_desk(i1);
		}
		else if (!strcmp(s1, "[VIEWPORT]"))
		{
			sscanf(s, "%*s %i %i %i %i", &i1, &i2, &i3,
			       &i4);
			/* migo: we don't want to lose DeskTopSize in
			 * configurations, and it does not work well
			 * anyways - Gnome is not updated
			 Scr.VxMax = i3;
			 Scr.VyMax = i4;
			*/
			MoveViewport(i1, i2, True);
		}
		else if (!strcmp(s1, "[KEY]"))
		{
			char *s2;
			s2 = s + n;
			if (*s2 != 0)
			{
				s2++;
			}
			sscanf(s2, "%[^\n]", s1);
			is_key = xstrdup(s1);
		}
		else if (!strcmp(s1, "[VALUE]"))
		{
			char *s2;
			s2 = s + n;
			if (*s2 != 0)
			{
				s2++;
			}
			sscanf(s2, "%[^\n]", s1);
			is_value = xstrdup(s1);

			fprintf(stderr, "GOT: %s -> %s\n", is_key, is_value);

			if (is_key != NULL && is_value != NULL) {
				fprintf(stderr, "INSERTING: %s -> %s\n",
					is_key, is_value);
				insert_metainfo(is_key, is_value);
			}

			free(is_key);
			free(is_value);
			is_key = is_value = NULL;
		}
#if 0
		/* migo (08-Dec-1999): we don't want to eliminate config yet */
		else if (/*!Restarting*/ 0)
		{
			/* Matthias: We don't want to restore too much
			 * state if we are restarting, since that
			 * would make restarting useless for rereading
			 * changed rc files. */
			if (!strcmp(s1, "[SCROLL]"))
			{
				sscanf(s, "%*s %i %i %i %i ", &i1,
				       &i2, &i3, &i4);
				Scr.EdgeScrollX = i1;
				Scr.EdgeScrollY = i2;
				Scr.ScrollDelay = i3;
				if (i4)
				{
					Scr.flags.edge_wrap_x = 1;
				}
				else
				{
					Scr.flags.edge_wrap_x = 0;
				}
				if (i3)
				{
					Scr.flags.edge_wrap_y = 1;
				}
				else
				{
					Scr.flags.edge_wrap_y = 0;
				}
			}
			else if (!strcmp(s1, "[MISC]"))
			{
				sscanf(s, "%*s %i %i %i", &i1, &i2,
				       &i3);
				Scr.ClickTime = i1;
				Scr.ColormapFocus = i2;
				Scr.ColorLimit = i3;
			}
			else if (!strcmp(s1, "[STYLE]"))
			{
				sscanf(s, "%*s %i %i", &i1, &i2);
				Scr.gs.EmulateMWM = i1;
				Scr.gs.EmulateWIN = i2;
			}
		}
#endif
	}
	fclose(f);

	return;
}

void
DisableRestoringState(void)
{
	num_match = 0;

	return;
}

void
LoadWindowStates(char *filename)
{
	FILE *f;
	char s[4096], s1[4096];
	char *s2;
	int i, pos, pos1;
	unsigned long w;
	int n;

	if (!VerifyVersionInfo(filename))
	{
		return;
	}
	if (!filename || !*filename)
	{
		return;
	}
	set_real_state_filename(filename);
	if ((f = fopen(filename, "r")) == NULL)
	{
		return;
	}

#if defined(FVWM_SM_DEBUG_PROTO) ||  defined(FVWM_SM_DEBUG_FILES)
	fprintf(stderr, "[FVWM_SMDEBUG] Loading %s\n", filename);
#endif
#ifdef FVWM_SM_DEBUG_FILES
	system(CatString3(
		       "mkdir -p /tmp/fs-load; cp ", filename,
		       " /tmp/fs-load"));
#endif

	while (fgets(s, sizeof(s), f))
	{
		n = 0;
		sscanf(s, "%4000s%n", s1, &n);
		if (!SessionSupport /* migo: temporarily */ &&
		    !strcmp(s1, "[REAL_STATE_FILENAME]"))
		{
			sscanf(s, "%*s %s", s1);
			set_sm_properties(sm_conn, s1, FSmRestartIfRunning);
			set_real_state_filename(s1);
		}
		else if (!strcmp(s1, "[CLIENT]"))
		{
			sscanf(s, "%*s %lx", &w);
			num_match++;
			matches = xrealloc(
				(void *)matches, sizeof(Match), num_match);
			matches[num_match - 1].win = w;
			matches[num_match - 1].client_id = NULL;
			matches[num_match - 1].res_name = NULL;
			matches[num_match - 1].res_class = NULL;
			matches[num_match - 1].window_role = NULL;
			matches[num_match - 1].wm_name = NULL;
			matches[num_match - 1].wm_command_count = 0;
			matches[num_match - 1].wm_command = NULL;
			matches[num_match - 1].x = 0;
			matches[num_match - 1].y = 0;
			matches[num_match - 1].w = 100;
			matches[num_match - 1].h = 100;
			matches[num_match - 1].x_max = 0;
			matches[num_match - 1].y_max = 0;
			matches[num_match - 1].w_max = Scr.MyDisplayWidth;
			matches[num_match - 1].h_max = Scr.MyDisplayHeight;
			matches[num_match - 1].width_defect_max = 0;
			matches[num_match - 1].height_defect_max = 0;
			matches[num_match - 1].icon_x = 0;
			matches[num_match - 1].icon_y = 0;
			matches[num_match - 1].desktop = 0;
			matches[num_match - 1].layer = 0;
			matches[num_match - 1].default_layer = 0;
			memset(&(matches[num_match - 1].flags), 0,
			       sizeof(window_flags));
			matches[num_match - 1].used = 0;
		}
		else if (!strcmp(s1, "[GEOMETRY]"))
		{
			sscanf(s, "%*s %i %i %i %i %i %i %i %i %i %i %i %i"
			       " %i %i %i",
			       &(matches[num_match - 1].x),
			       &(matches[num_match - 1].y),
			       &(matches[num_match - 1].w),
			       &(matches[num_match - 1].h),
			       &(matches[num_match - 1].x_max),
			       &(matches[num_match - 1].y_max),
			       &(matches[num_match - 1].w_max),
			       &(matches[num_match - 1].h_max),
			       &(matches[num_match - 1].width_defect_max),
			       &(matches[num_match - 1].height_defect_max),
			       &(matches[num_match - 1].icon_x),
			       &(matches[num_match - 1].icon_y),
			       &(matches[num_match - 1].gravity),
			       &(matches[num_match - 1].max_x_offset),
			       &(matches[num_match - 1].max_y_offset));
		}
		else if (!strcmp(s1, "[DESK]"))
		{
			sscanf(s, "%*s %i",
			       &(matches[num_match - 1].desktop));
		}
		else if (!strcmp(s1, "[LAYER]"))
		{
			sscanf(s, "%*s %i %i",
			       &(matches[num_match - 1].layer),
			       &(matches[num_match - 1].default_layer));
		}
		else if (!strcmp(s1, "[PLACED_BY_BUTTON]"))
		{
			sscanf(s, "%*s %i",
			       &(matches[num_match - 1].placed_by_button));
		}
		else if (!strcmp(s1, "[EWMH_DESKTOP]"))
		{
			sscanf(s, "%*s %lu",
			       &(matches[num_match - 1].ewmh_hint_desktop));
		}
		else if (!strcmp(s1, "[FLAGS]"))
		{
			char *ts = s;

			/* skip [FLAGS] */
			while (*ts != ']')
			{
				ts++;
			}
			ts++;

			for (i = 0; i < sizeof(window_flags); i++)
			{
				unsigned int f;
				sscanf(ts, "%02x ", &f);
				((unsigned char *)&
				 (matches[num_match-1].flags))[i] = f;
				ts += 3;
			}
		}
		else if (!strcmp(s1, "[CLIENT_ID]"))
		{
			s2 = s + n;
			if (*s2 != 0)
			{
				s2++;
			}
			sscanf(s2, "%[^\n]", s1);
			matches[num_match - 1].client_id = duplicate(s1);
		}
		else if (!strcmp(s1, "[WINDOW_ROLE]"))
		{
			s2 = s + n;
			if (*s2 != 0)
			{
				s2++;
			}
			sscanf(s2, "%[^\n]", s1);
			matches[num_match - 1].window_role = duplicate(s1);
		}
		else if (!strcmp(s1, "[RES_NAME]"))
		{
			s2 = s + n;
			if (*s2 != 0)
			{
				s2++;
			}
			sscanf(s2, "%[^\n]", s1);
			matches[num_match - 1].res_name = duplicate(s1);
		}
		else if (!strcmp(s1, "[RES_CLASS]"))
		{
			s2 = s + n;
			if (*s2 != 0)
			{
				s2++;
			}
			sscanf(s2, "%[^\n]", s1);
			matches[num_match - 1].res_class = duplicate(s1);
		}
		else if (!strcmp(s1, "[WM_NAME]"))
		{
			s2 = s + n;
			if (*s2 != 0)
			{
				s2++;
			}
			sscanf(s2, "%[^\n]", s1);
			matches[num_match - 1].wm_name = duplicate(s1);
		}
		else if (!strcmp(s1, "[WM_COMMAND]"))
		{
			sscanf(s, "%*s %i%n",
			       &matches[num_match - 1].wm_command_count, &pos);
			matches[num_match - 1].wm_command = (char **)
				xmalloc(
					matches[num_match - 1].
					wm_command_count * sizeof (char *));
			for (i = 0;
			     i < matches[num_match - 1].wm_command_count; i++)
			{
				sscanf (s+pos, "%s%n", s1, &pos1);
				pos += pos1;
				matches[num_match - 1].wm_command[i] =
					duplicate (s1);
			}
		}
	}
	fclose(f);

	return;
}

/*
  This routine (potentially) changes the flags STARTICONIC,
  MAXIMIZED, WSHADE and STICKY and the Desk and
  attr.x, .y, .width, .height entries. It also changes the
  stack_before pointer to return information about the
  desired stacking order. It expects the stacking order
  to be set up correctly beforehand!
*/
Bool
MatchWinToSM(
	FvwmWindow *ewin, mwtsm_state_args *ret_state_args,
	initial_window_options_t *win_opts)
{
	int i;

	if (!does_file_version_match)
	{
		return False;
	}
	for (i = 0; i < num_match; i++)
	{
		if (!matches[i].used && matchWin(ewin, &matches[i]))
		{
			matches[i].used = 1;

			if (!Restarting)
			{
				/* We don't want to restore too much state if
				   we are restarting, since that would make
				   * restarting useless for rereading changed
				   * rc files. */
				SET_DO_SKIP_WINDOW_LIST(
					ewin,
					DO_SKIP_WINDOW_LIST(&(matches[i])));
				SET_ICON_SUPPRESSED(
					ewin,
					IS_ICON_SUPPRESSED(&(matches[i])));
				SET_HAS_NO_ICON_TITLE(
					ewin, HAS_NO_ICON_TITLE(&(matches[i])));
				FPS_LENIENT(
					FW_FOCUS_POLICY(ewin),
					FP_IS_LENIENT(FW_FOCUS_POLICY(
							      &(matches[i]))));
				SET_ICON_STICKY_ACROSS_PAGES(
					ewin, IS_ICON_STICKY_ACROSS_PAGES(
						&(matches[i])));
				SET_ICON_STICKY_ACROSS_DESKS(
					ewin, IS_ICON_STICKY_ACROSS_DESKS(
						&(matches[i])));
				SET_DO_SKIP_ICON_CIRCULATE(
					ewin, DO_SKIP_ICON_CIRCULATE(
						&(matches[i])));
				SET_DO_SKIP_SHADED_CIRCULATE(
					ewin, DO_SKIP_SHADED_CIRCULATE(
						&(matches[i])));
				SET_DO_SKIP_CIRCULATE(
					ewin, DO_SKIP_CIRCULATE(&(matches[i])));
				memcpy(
					&FW_FOCUS_POLICY(ewin),
					&FW_FOCUS_POLICY(&matches[i]),
					sizeof(focus_policy_t));
				if (matches[i].wm_name)
				{
					free_window_names(ewin, True, False);
					ewin->name.name = matches[i].wm_name;
					setup_visible_names(ewin, 1);
				}
			}
			SET_NAME_CHANGED(ewin,IS_NAME_CHANGED(&(matches[i])));
			SET_PLACED_BY_FVWM(
				ewin, IS_PLACED_BY_FVWM(&(matches[i])));
			ret_state_args->do_shade = IS_SHADED(&(matches[i]));
			ret_state_args->used_title_dir_for_shading =
				USED_TITLE_DIR_FOR_SHADING(&(matches[i]));
			ret_state_args->shade_dir = SHADED_DIR(&(matches[i]));
			ret_state_args->do_max = IS_MAXIMIZED(&(matches[i]));
			SET_USER_STATES(ewin, GET_USER_STATES(&(matches[i])));
			SET_ICON_MOVED(ewin, IS_ICON_MOVED(&(matches[i])));
			if (IS_ICONIFIED(&(matches[i])))
			{
				/*
				  ICON_MOVED is necessary to make fvwm use
				  icon_[xy]_loc for icon placement
				*/
				win_opts->initial_state = IconicState;
				win_opts->flags.use_initial_icon_xy = 1;
				win_opts->initial_icon_x = matches[i].icon_x;
				win_opts->initial_icon_y = matches[i].icon_y;
				if (!IS_STICKY_ACROSS_PAGES(&(matches[i])) &&
				    !(IS_ICONIFIED(&(matches[i])) &&
				      IS_ICON_STICKY_ACROSS_PAGES(
					      &(matches[i]))))
				{
					win_opts->initial_icon_x -= Scr.Vx;
					win_opts->initial_icon_y -= Scr.Vy;
				}
			}
			ewin->g.normal.x = matches[i].x;
			ewin->g.normal.y = matches[i].y;
			ewin->g.normal.width = matches[i].w;
			ewin->g.normal.height = matches[i].h;
			ewin->g.max.x = matches[i].x_max;
			ewin->g.max.y = matches[i].y_max;
			ewin->g.max.width = matches[i].w_max;
			ewin->g.max.height = matches[i].h_max;
			ewin->g.max_defect.width = matches[i].width_defect_max;
			ewin->g.max_defect.height =
				matches[i].height_defect_max;
			ewin->g.max_offset.x = matches[i].max_x_offset;
			ewin->g.max_offset.y = matches[i].max_y_offset;
			SET_STICKY_ACROSS_PAGES(
				ewin, IS_STICKY_ACROSS_PAGES(&(matches[i])));
			SET_STICKY_ACROSS_DESKS(
				ewin, IS_STICKY_ACROSS_DESKS(&(matches[i])));
			ewin->Desk = (IS_STICKY_ACROSS_DESKS(ewin)) ?
				Scr.CurrentDesk : matches[i].desktop;
			set_layer(ewin, matches[i].layer);
			set_default_layer(ewin, matches[i].default_layer);
			ewin->placed_by_button = matches[i].placed_by_button;
			/* Note: the Modal, skip pager, skip taskbar and
			 * "stacking order" state are not restored here: there
			 * are restored in EWMH_ExitStuff */
			ewin->ewmh_hint_desktop = matches[i].ewmh_hint_desktop;
			SET_HAS_EWMH_INIT_WM_DESKTOP(
				ewin, HAS_EWMH_INIT_WM_DESKTOP(&(matches[i])));
			SET_HAS_EWMH_INIT_HIDDEN_STATE(
				ewin, HAS_EWMH_INIT_HIDDEN_STATE(
					&(matches[i])));
			SET_HAS_EWMH_INIT_MAXHORIZ_STATE(
				ewin, HAS_EWMH_INIT_MAXHORIZ_STATE(
					&(matches[i])));
			SET_HAS_EWMH_INIT_MAXVERT_STATE(
				ewin, HAS_EWMH_INIT_MAXVERT_STATE(
					&(matches[i])));
			SET_HAS_EWMH_INIT_SHADED_STATE(
				ewin, HAS_EWMH_INIT_SHADED_STATE(
					&(matches[i])));
			SET_HAS_EWMH_INIT_STICKY_STATE(
				ewin, HAS_EWMH_INIT_STICKY_STATE(
					&(matches[i])));
			return True;
		}
	}

	return False;
}

void
RestartInSession (char *filename, Bool is_native, Bool _do_preserve_state)
{
	do_preserve_state = _do_preserve_state;

	if (SessionSupport && sm_conn && is_native)
	{
		going_to_restart = True;

		save_state_file(filename);
		set_sm_properties(sm_conn, filename, FSmRestartImmediately);

		MoveViewport(0, 0, False);
		Reborder();

		CloseICCCM2();
		XCloseDisplay(dpy);

		if ((!FSmcCloseConnection(sm_conn, 0, NULL)) != FSmcClosedNow)
		{
			/* go a head any way ? */
		}

#ifdef  FVWM_SM_DEBUG_PROTO
		fprintf(stderr, "[FVWM_SMDEBUG]: Exiting, now SM must "
			"restart us.\n");
#endif
		/* Close all my pipes */
		module_kill_all();

		exit(0); /* let the SM restart us */
	}

	save_state_file(filename);
	/* return and let Done restart us */

	return;
}

void SetClientID(char *client_id)
{
	if (!SessionSupport)
	{
		return;
	}
	previous_sm_client_id = client_id;

	return;
}

void
SessionInit(void)
{
	char error_string_ret[4096] = "";
	FSmPointer context;
	FSmcCallbacks callbacks;

	if (!SessionSupport)
	{
		/* -Wall fixes */
		MyIoErrorHandler(NULL);
		callback_save_yourself2(NULL, NULL);
		return;
	}

	InstallIOErrorHandler();

	callbacks.save_yourself.callback = callback_save_yourself;
	callbacks.die.callback = callback_die;
	callbacks.save_complete.callback = callback_save_complete;
	callbacks.shutdown_cancelled.callback = callback_shutdown_cancelled;
	callbacks.save_yourself.client_data =
		callbacks.die.client_data =
		callbacks.save_complete.client_data =
		callbacks.shutdown_cancelled.client_data = (FSmPointer) NULL;
	SUPPRESS_UNUSED_VAR_WARNING(context);
	sm_conn = FSmcOpenConnection(
		NULL, &context, FSmProtoMajor, FSmProtoMinor,
		FSmcSaveYourselfProcMask | FSmcDieProcMask |
		FSmcSaveCompleteProcMask | FSmcShutdownCancelledProcMask,
		&callbacks, previous_sm_client_id, &sm_client_id, 4096,
		error_string_ret);
	if (!sm_conn)
	{
		/*
		  Don't annoy users which don't use a session manager
		*/
		if (previous_sm_client_id)
		{
			fvwm_msg(
				ERR, "SessionInit",
				"While connecting to session manager:\n%s.",
				error_string_ret);
		}
		sm_fd = -1;
#ifdef FVWM_SM_DEBUG_PROTO
		fprintf(stderr,"[FVWM_SMDEBUG] No SM connection\n");
#endif
	}
	else
	{
		sm_fd = FIceConnectionNumber(FSmcGetIceConnection(sm_conn));
#ifdef FVWM_SM_DEBUG_PROTO
		fprintf(stderr,"[FVWM_SMDEBUG] Connectecd to a SM\n");
#endif
		set_init_function_name(0, "SessionInitFunction");
		set_init_function_name(1, "SessionRestartFunction");
		set_init_function_name(2, "SessionExitFunction");
		/* basically to restet our restart style hint after a
		 * restart */
		set_sm_properties(sm_conn, NULL, FSmRestartIfRunning);
	}

	return;
}

void
ProcessICEMsgs(void)
{
	FIceProcessMessagesStatus status;

	if (!SessionSupport)
	{
		return;
	}

#ifdef FVWM_SM_DEBUG_PROTO
	fprintf(stderr,"[FVWM_SMDEBUG][ProcessICEMsgs] %i\n", (int)sm_fd);
#endif
	if (sm_fd < 0)
	{
		return;
	}
	status = FIceProcessMessages(FSmcGetIceConnection(sm_conn), NULL, NULL);
	if (status == FIceProcessMessagesIOError)
	{
		fvwm_msg(ERR, "ProcessICEMSGS",
			 "Connection to session manager lost\n");
		sm_conn = NULL;
		sm_fd = -1;
	}

	return;
}

/*
 * Fvwm Function implementation: QuitSession, SaveSession, SaveQuitSession.
 * migo (15-Jun-1999): I am not sure this is right.
 * The session mabager must implement SmsSaveYourselfRequest (xsm doesn't?).
 * Alternative implementation may use unix signals, but this does not work
 * with all session managers (also must suppose that SM runs locally).
 */
int get_sm_pid(void)
{
	const char *session_manager_var = getenv("SESSION_MANAGER");
	const char *sm_pid_str_ptr;
	int sm_pid = 0;

	if (!SessionSupport)
	{
		return 0;
	}

	if (!session_manager_var)
	{
		return 0;
	}
	sm_pid_str_ptr = strchr(session_manager_var, ',');
	if (!sm_pid_str_ptr)
	{
		return 0;
	}
	while (sm_pid_str_ptr > session_manager_var && isdigit(*(--sm_pid_str_ptr)))
	{
		/* nothing */
	}
	while (isdigit(*(++sm_pid_str_ptr)))
	{
		sm_pid = sm_pid * 10 + *sm_pid_str_ptr - '0';
	}

	return sm_pid;
}

/*
 * quit_session - hopefully shutdowns the session
 */
Bool quit_session(void)
{
	if (!SessionSupport)
	{
		return False;
	}
	if (!sm_conn)
	{
		return False;
	}

	FSmcRequestSaveYourself(
		sm_conn, FSmSaveLocal, True /* shutdown */, FSmInteractStyleAny,
		False /* fast */, True /* global */);
	return True;

	/* migo: xsm does not support RequestSaveYourself, but supports
	 * signals: */
	/*
	  int sm_pid = get_sm_pid();
	  if (!sm_pid) return False;
	  return kill(sm_pid, SIGTERM) == 0 ? True : False;
	*/
}

/*
 * save_session - hopefully saves the session
 */
Bool save_session(void)
{
	if (!SessionSupport)
	{
		return False;
	}
	if (!sm_conn)
	{
		return False;
	}

	FSmcRequestSaveYourself(
		sm_conn, FSmSaveBoth, False /* shutdown */, FSmInteractStyleAny,
		False /* fast */, True /* global */);
	return True;

	/* migo: xsm does not support RequestSaveYourself, but supports
	 * signals: */
	/*
	  int sm_pid = get_sm_pid();
	  if (!sm_pid) return False;
	  return kill(sm_pid, SIGUSR1) == 0 ? True : False;
	*/
}

/*
 * save_quit_session - hopefully saves and shutdowns the session
 */
Bool save_quit_session(void)
{
	if (!SessionSupport)
	{
		return False;
	}

	if (!sm_conn)
	{
		return False;
	}

	FSmcRequestSaveYourself(
		sm_conn, FSmSaveBoth, True /* shutdown */, FSmInteractStyleAny,
		False /* fast */, True /* global */);
	return True;

	/* migo: xsm does not support RequestSaveYourself, but supports
	 * signals: */
	/*
	  if (save_session() == False) return False;
	  sleep(3);  / * doesn't work anyway * /
	  if (quit_session() == False) return False;
	  return True;
	*/
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_QuitSession(F_CMD_ARGS)
{
	quit_session();

	return;
}

void CMD_SaveSession(F_CMD_ARGS)
{
	save_session();

	return;
}

void CMD_SaveQuitSession(F_CMD_ARGS)
{
	save_quit_session();

	return;
}
