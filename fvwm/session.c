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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 This file is strongly based on the corresponding files from
 twm and enlightenment.
 */
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <signal.h>
#include "fvwm.h"
#include <X11/Xatom.h>
#include "screen.h"
#include "misc.h"

extern int master_pid;


typedef struct _match
{
  unsigned long      win;
  char               *client_id;
  char               *res_name;
  char               *res_class;
  char               *window_role;
  char               *wm_name;
  int                wm_command_count;
  char               **wm_command;
  int                 x, y, w, h, icon_x, icon_y;
  int                 x_max, y_max, w_max, h_max;
  int                 desktop;
  int                 layer;
  int                 used;
  window_flags        flags;
}
Match;

int                 sm_fd = -1;

static char         *sm_client_id = NULL;
static int          num_match = 0;
static Match        *matches = NULL;
static Bool         sent_save_done = 0;

extern Bool Restarting;

static
char *duplicate(char *s)
{
  int l;
  char *r;

  if (!s) return NULL;
  l = strlen(s);
  r = (char *) malloc (sizeof(char)*(l+1));
  strncpy(r, s, l+1);
  return r;
}

/*
 * It is a bit ugly to have a separate file format for
 * config files and session save files. The proper way
 * to do this may be to extend the config file format
 * to allow the specification of everything we need
 * to save here. Then the option "-restore xyz" could
 * be replaced by "-f xyz".
 */
int
SaveGlobalState(FILE *f)
{
  fprintf(f, "[GLOBAL]\n");
  fprintf(f, "  [DESKTOP] %i\n", Scr.CurrentDesk);
  fprintf(f, "  [VIEWPORT] %i %i %i %i\n",
          Scr.Vx, Scr.Vy, Scr.VxMax, Scr.VyMax);
  fprintf(f, "  [SCROLL] %i %i %i %i %i %i\n",
	  Scr.EdgeScrollX, Scr.EdgeScrollY, Scr.ScrollResistance,
	  Scr.MoveResistance,
	  !!(Scr.flags.edge_wrap_x), !!(Scr.flags.edge_wrap_y));
  fprintf(f, "  [SNAP] %i %i %i %i\n",
	  Scr.SnapAttraction, Scr.SnapMode, Scr.SnapGridX, Scr.SnapGridY);
  fprintf(f, "  [MISC] %i %i %i %i %i %i %i %i %i\n",
	  Scr.ClickTime, Scr.ColormapFocus,
	  Scr.ColorLimit, Scr.go.SmartPlacementIsClever,
	  Scr.go.ClickToFocusPassesClick, Scr.go.ClickToFocusRaises,
	  Scr.go.MouseFocusClickRaises, Scr.go.StipledTitles,
	  Scr.go.WindowShadeScrolls);
  fprintf(f, "  [STYLE] %i %i %i %i %i %i\n",
	  Scr.go.ModifyUSP, Scr.go.CaptureHonorsStartsOnPage,
	  Scr.go.RecaptureHonorsStartsOnPage,
	  Scr.go.ActivePlacementHonorsStartsOnPage,
          Scr.gs.EmulateMWM, Scr.gs.EmulateWIN);
  return 1;
}

#ifdef SESSION
#include <X11/SM/SMlib.h>

extern char **g_argv;
extern int g_argc;

SmcConn sm_conn = NULL;
static char *last_used_filename = NULL;

static void 
set_sm_properties (SmcConn sm_conn, char *filename, char hint);

static int
save_session_state (SmcConn sm_conn, char *filename, FILE *f);

#endif /* SESSION */

void
LoadGlobalState(char *filename)
{
  FILE               *f;
  char                s[4096], s1[4096], s2[256];
  int i1, i2, i3, i4, i5, i6, i7, i8, i9;

  if (filename && (f = fopen(filename, "r")))
    {
      while (fgets(s, sizeof(s), f))
	{
	  i1 = 0; i2 = 0; i3 = 0; i4 = 0; i5 = 0; i6 = 0;
	  i7 = 0; i8 = 0;
	  sscanf(s, "%4000s", s1);
#ifdef SESSION
	  /* If we are restarting, [SESSION_STATE] points
	     to the file containing the true session state. */
	  if (!strcmp(s1, "[SESSION_STATE]"))
	    {
	      sscanf(s, "%*s %s", &s2);
	      set_sm_properties (sm_conn, s2, SmRestartIfRunning);
	      if (last_used_filename) free (last_used_filename);
	      last_used_filename = strdup (s2);
	    }
	  else
#endif /* SESSION */ 
	  if (!strcmp(s1, "[DESKTOP]"))
	    {
	      sscanf(s, "%*s %i", &i1);
	      changeDesks(i1);
	    }
	  else if (!strcmp(s1, "[VIEWPORT]"))
	    {
	      sscanf(s, "%*s %i %i %i %i", &i1, &i2, &i3, &i4);
	      Scr.VxMax = i3;
	      Scr.VyMax = i4;
	      MoveViewport(i1, i2, True);
	    }
	  else if (!strcmp(s1, "[SCROLL]"))
	    {
	      sscanf(s, "%*s %i %i %i %i %i %i", &i1, &i2, &i3, &i4, &i5,
		     &i6);
	      Scr.EdgeScrollX = i1;
	      Scr.EdgeScrollY = i2;
	      Scr.ScrollResistance = i3;
	      Scr.MoveResistance = i4;
	      if (i5)
		Scr.flags.edge_wrap_x = 1;
	      else
		Scr.flags.edge_wrap_x = 0;
	      if (i6)
		Scr.flags.edge_wrap_y = 1;
	      else
		Scr.flags.edge_wrap_y = 0;
	    }
	  else if (!strcmp(s1, "[SNAP]"))
	    {
	      sscanf(s, "%*s %i %i %i %i", &i1, &i2, &i3, &i4);
	      Scr.SnapAttraction = i1;
	      Scr.SnapMode = i2;
	      Scr.SnapGridX = i3;
	      Scr.SnapGridY = i4;
	    }
	  else if (!strcmp(s1, "[MISC]"))
	    {
	      sscanf(s, "%*s %i %i %i %i %i %i %i %i %i", &i1, &i2, &i3, &i4,
		     &i5, &i6, &i7, &i8, &i9);
	      Scr.ClickTime = i1;
	      Scr.ColormapFocus = i2;
	      Scr.ColorLimit = i3;
	      Scr.go.SmartPlacementIsClever = i4;
	      Scr.go.ClickToFocusPassesClick = i5;
	      Scr.go.ClickToFocusRaises = i6;
	      Scr.go.MouseFocusClickRaises = i7;
	      Scr.go.StipledTitles = i8;
	      Scr.go.WindowShadeScrolls = i9;
	    }
	  else if (!strcmp(s1, "[STYLE]"))
	    {
	      sscanf(s, "%*s %i %i %i %i %i %i", &i1, &i2, &i3, &i4, &i5, &i6);
	      Scr.go.ModifyUSP = i1;
	      Scr.go.CaptureHonorsStartsOnPage = i2;
	      Scr.go.RecaptureHonorsStartsOnPage = i3;
	      Scr.go.ActivePlacementHonorsStartsOnPage = i4;
	      Scr.gs.EmulateMWM = i5;
	      Scr.gs.EmulateWIN = i6;
	    }
	}
      fclose(f);
    }
}

char *
GetWindowRole(Window window)
{
  XTextProperty tp;

  if (XGetTextProperty (dpy, window, &tp, _XA_WM_WINDOW_ROLE))
    {
      if (tp.encoding == XA_STRING && tp.format == 8 && tp.nitems != 0)
	return ((char *) tp.value);
    }

  return NULL;
}

char *
GetClientID(Window window)
{
  char *client_id = NULL;
  Window client_leader;
  XTextProperty tp;
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *prop = NULL;

  if (XGetWindowProperty(dpy, window, _XA_WM_CLIENT_LEADER,
			 0L, 1L, False, AnyPropertyType, &actual_type,
			 &actual_format, &nitems, &bytes_after, &prop)
      == Success)
    {
      if (actual_type == XA_WINDOW && actual_format == 32 &&
	  nitems == 1 && bytes_after == 0)
	{
	  client_leader = *((Window *) prop);

	  if (XGetTextProperty (dpy, client_leader, &tp, _XA_SM_CLIENT_ID))
	    {
	      if (tp.encoding == XA_STRING &&
		  tp.format == 8 && tp.nitems != 0)
		client_id = (char *) tp.value;
	    }
	}

      if (prop)
	XFree (prop);
    }

  return client_id;
}

int
SaveWindowStates(FILE *f)
{
  char *client_id;
  char *window_role;
  char **wm_command;
  int wm_command_count;
  FvwmWindow *ewin;
  int i;

  for (ewin = Scr.FvwmRoot.stack_next; ewin != &Scr.FvwmRoot;
       ewin = ewin->stack_next)
    {
      fprintf(f, "[CLIENT] %lx\n", ewin->w);

      client_id = GetClientID(ewin->w);
      if (client_id)
	{
	  fprintf(f, "  [CLIENT_ID] %s\n", client_id);
	  XFree(client_id);
	}

      window_role = GetWindowRole(ewin->w);
      if (window_role)
	{
	  fprintf(f, "  [WINDOW_ROLE] %s\n", window_role);
	  XFree(window_role);
	}
      else
	{
	  if (ewin->class.res_class)
	    fprintf(f, "  [RES_NAME] %s\n", ewin->class.res_class);
	  if (ewin->class.res_name)
	    fprintf(f, "  [RES_CLASS] %s\n", ewin->class.res_name);
	  if (ewin->name)
	    fprintf(f, "  [WM_NAME] %s\n", ewin->name);

	  wm_command = NULL;
	  wm_command_count = 0;
	  XGetCommand (dpy, ewin->w, &wm_command, &wm_command_count);

	  if (wm_command && (wm_command_count > 0))
	    {
	      fprintf(f, "  [WM_COMMAND] %i", wm_command_count);
	      for (i = 0; i < wm_command_count; i++)
		fprintf(f, " %s", wm_command[i]);
	      fprintf(f, "\n");
	      XFreeStringList (wm_command);
	    }
	} /* !window_role */

      fprintf(f, "  [GEOMETRY] %i %i %i %i %i %i %i %i %i %i\n",
	      ewin->orig_x,
              ewin->orig_y,
	      ewin->orig_wd - 2*ewin->boundary_width ,
	      ewin->orig_ht - 2*ewin->boundary_width - ewin->title_height,
	      ewin->frame_x + Scr.Vx,
              ewin->frame_y + Scr.Vy,
              ewin->frame_width,
              ewin->maximized_ht,
	      ewin->icon_x_loc + Scr.Vx,
	      ewin->icon_y_loc + Scr.Vy);
      fprintf(f, "  [DESK] %i\n", ewin->Desk);
      fprintf(f, "  [LAYER] %i\n", ewin->layer);
      fprintf(f, "  [FLAGS] ");
      for (i = 0; i < sizeof(window_flags); i++)
	fprintf(f, "%02x ", (int)(((unsigned char *)&(ewin->flags))[i]));
      fprintf(f, "\n");
    }
  return 1;
}

void
LoadWindowStates(char *filename)
{
  FILE               *f;
  char                s[4096], s1[4096];
  int i, pos;
  unsigned long w;

  if (filename && (f = fopen (filename, "r")))
    {
      while (fgets(s, sizeof(s), f))
	{
	  sscanf(s, "%4000s", s1);
	  if (!strcmp(s1, "[CLIENT]"))
	    {
	      sscanf(s, "%*s %lx", &w);
	      num_match++;
	      matches = realloc(matches, sizeof(Match) * num_match);
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
	      matches[num_match - 1].icon_x = 0;
	      matches[num_match - 1].icon_y = 0;
	      matches[num_match - 1].desktop = 0;
	      matches[num_match - 1].layer = 0;
	      memset(&(matches[num_match - 1].flags), 0,
		     sizeof(window_flags));
	      matches[num_match - 1].used = 0;
	    }
	  else if (!strcmp(s1, "[GEOMETRY]"))
	    {
	      sscanf(s, "%*s %i %i %i %i %i %i %i %i %i %i",
		     &(matches[num_match - 1].x),
		     &(matches[num_match - 1].y),
		     &(matches[num_match - 1].w),
		     &(matches[num_match - 1].h),
		     &(matches[num_match - 1].x_max),
		     &(matches[num_match - 1].y_max),
		     &(matches[num_match - 1].w_max),
		     &(matches[num_match - 1].h_max),
		     &(matches[num_match - 1].icon_x),
		     &(matches[num_match - 1].icon_y));
	    }
	  else if (!strcmp(s1, "[DESK]"))
	    {
	      sscanf(s, "%*s %i",
		     &(matches[num_match - 1].desktop));
	    }
	  else if (!strcmp(s1, "[LAYER]"))
	    {
	      sscanf(s, "%*s %i",
		     &(matches[num_match - 1].layer));
	    }
	  else if (!strcmp(s1, "[FLAGS]"))
	    {
	      char *ts = s;

	      /* skip [FLAGS] */
	      while (*ts != ']')
		ts++;
	      ts++;

	      for (i = 0; i < sizeof(window_flags); i++)
		{
		    unsigned int f;
		    sscanf(ts, "%02x ", &f);
		    ((unsigned char *)&(matches[num_match-1].flags))[i]
		      = f;
		    ts += 3;
		}
	    }
	  else if (!strcmp(s1, "[CLIENT_ID]"))
	    {
	      sscanf(s, "%*s %4000s", s1);
	      matches[num_match - 1].client_id = duplicate(s1);
	    }
	  else if (!strcmp(s1, "[WINDOW_ROLE]"))
	    {
	      sscanf(s, "%*s %4000s", s1);
	      matches[num_match - 1].window_role = duplicate(s1);
	    }
	  else if (!strcmp(s1, "[RES_NAME]"))
	    {
	      sscanf(s, "%*s %4000s", s1);
	      matches[num_match - 1].res_name = duplicate(s1);
	    }
	  else if (!strcmp(s1, "[RES_CLASS]"))
	    {
	      sscanf(s, "%*s %4000s", s1);
	      matches[num_match - 1].res_class = duplicate(s1);
	    }
	  else if (!strcmp(s1, "[WM_NAME]"))
	    {
	      sscanf(s, "%*s %4000s", s1);
	      matches[num_match - 1].wm_name = duplicate(s1);
	    }
	  else if (!strcmp(s1, "[WM_COMMAND]"))
	    {
	      sscanf(s, "%*s %i", &matches[num_match - 1].wm_command_count);
	      matches[num_match - 1].wm_command =
		(char **) malloc (matches[num_match - 1].wm_command_count *
				  sizeof (char *));
	      pos = 0;
	      for (i = 0; i < matches[num_match - 1].wm_command_count; i++)
		{
		  sscanf (s+pos, "%s%n", s1, &pos);
		  matches[num_match - 1].wm_command[i] = duplicate (s1);
		}
	    }

	}
      fclose(f);
    }
}

/* This complicated logic is from twm, where it is explained */

#define xstreq(a,b) ((!a && !b) || (a && b && (strcmp(a,b)==0)))

Bool matchWin(FvwmWindow *w, Match *m)
{
  char *client_id = NULL;
  char *window_role = NULL;
  char **wm_command = NULL;
  int wm_command_count = 0, i;
  int found;


  if (Restarting)
    {
       /* simply match by window id */
       return (w->w == m->win);
    }

  found = 0;
  client_id = GetClientID(w->w);

  if (xstreq(client_id, m->client_id))
    {

      /* client_id's match */

      window_role = GetWindowRole(w->w);

      if (window_role || m->window_role)
        {
          /* We have or had a window role, base decision on it */

          found = xstreq(window_role, m->window_role);
        }
      else
	{
          /* Compare res_class, res_name and WM_NAME, unless the
             user has changed WM_NAME */

	  if (xstreq(w->class.res_name, m->res_name) &&
	      xstreq(w->class.res_class, m->res_class) &&
	      (IS_NAME_CHANGED(m) || IS_NAME_CHANGED(w) ||
	       xstreq(w->name, m->wm_name)))
            {
              if (client_id)
                {
                  /* If we have a client_id, we don't compare
                     WM_COMMAND, since it will be different. */
                  found = 1;
                }
              else
	        {
                  /* for non-SM-aware clients we also compare WM_COMMAND */
	          XGetCommand (dpy, w->w, &wm_command, &wm_command_count);

	          if (wm_command_count == m->wm_command_count)
		    {
		      for (i = 0; i < wm_command_count; i++)
		        {
		          if (strcmp (wm_command[i], m->wm_command[i]) != 0)
			    break;
		        }

                      if (i == wm_command_count)
                        {
                          found = 1;
                        }
		    } /* if (wm_command_count ==... */
		} /* else no client id */
	    } /* if res_class, res_name and wm_name agree */
	} /* else no window roles */
    } /* if client_id's agree */

  if (client_id)
    XFree (client_id);

  if (window_role)
    XFree (window_role);

  if (wm_command)
    XFreeStringList (wm_command);

  return found;
}

static int
my_modulo (int x, int m)
{
  return (x < 0) ? (m + (x % m)) : (x % m);
}

/*
  This routine (potentially) changes the flags STARTICONIC,
  MAXIMIZED, WSHADE and STICKY and the Desk and
  attr.x, .y, .width, .height entries. It also changes the
  stack_before pointer to return information about the
  desired stacking order. It expects the stacking order
  to be set up correctly beforehand!
*/
void
MatchWinToSM(FvwmWindow *ewin,
             int *x_max, int *y_max, int *w_max, int *h_max,
             int *do_shade, int *do_max)
{
  int                 i, j;
  FvwmWindow *t;

  for (i = 0; i < num_match; i++)
    {
      if (!matches[i].used && matchWin(ewin, &matches[i]))
	{

	  matches[i].used = 1;
	  *do_shade = IS_SHADED(&(matches[i]));
	  *do_max = IS_MAXIMIZED(&(matches[i]));
	  if (IS_ICONIFIED(&(matches[i]))) {
	    /*
	      ICON_MOVED is necessary to make fvwm use icon_[xy]_loc
	      for icon placement
	    */
	    SET_DO_START_ICONIC(ewin, 1);
	    SET_ICON_MOVED(ewin, 1);
	  } else {
	    SET_DO_START_ICONIC(ewin, 0);
	  }
	  SET_DO_SKIP_WINDOW_LIST(ewin, DO_SKIP_WINDOW_LIST(&(matches[i])));
	  SET_ICON_SUPPRESSED(ewin, IS_ICON_SUPPRESSED(&(matches[i])));
	  SET_HAS_NO_ICON_TITLE(ewin, HAS_NO_ICON_TITLE(&(matches[i])));
	  SET_LENIENT(ewin, IS_LENIENT(&(matches[i])));
	  SET_ICON_STICKY(ewin, IS_ICON_STICKY(&(matches[i])));
	  SET_DO_SKIP_ICON_CIRCULATE(ewin,
				     DO_SKIP_ICON_CIRCULATE(&(matches[i])));
	  SET_DO_SKIP_CIRCULATE(ewin, DO_SKIP_CIRCULATE(&(matches[i])));
	  SET_FOCUS_MODE(ewin, GET_FOCUS_MODE(&(matches[i])));
	  ewin->name = matches[i].wm_name;
	  /* this doesn't work very well if Vx/Vy is not on
	     a page boundary */
	  ewin->attr.x = matches[i].x - Scr.Vx;
	  ewin->attr.y = matches[i].y - Scr.Vy;

	  *x_max = matches[i].x_max - Scr.Vx;
	  *y_max = matches[i].y_max - Scr.Vy;
	  *w_max = matches[i].w_max;
	  *h_max = matches[i].h_max;

	  if (IS_STICKY(&(matches[i]))) {
	    SET_STICKY(ewin, 1);
	    /* force sticky windows on screen */
	    ewin->attr.x = my_modulo (ewin->attr.x, Scr.MyDisplayWidth);
	    ewin->attr.y = my_modulo (ewin->attr.y, Scr.MyDisplayHeight);
	    *x_max = my_modulo (*x_max, Scr.MyDisplayWidth);
	    *y_max = my_modulo (*y_max, Scr.MyDisplayHeight);
	  } else {
	    SET_STICKY(ewin, 0);
	    ewin->Desk = matches[i].desktop;
	  }

	  ewin->layer = matches[i].layer;

	  ewin->attr.width = matches[i].w;
	  ewin->attr.height = matches[i].h;

	  /* this is not enough to fight fvwms attempts to
	     put icons on the current page */
	  ewin->icon_x_loc = matches[i].icon_x - Scr.Vx;
	  ewin->icon_y_loc = matches[i].icon_y - Scr.Vy;

	  /* Find the window to stack this one below. */

	  for (j = i-1; j >= 0; j--) {

	    /* matches are sorted in stacking order */

	    if (matches[j].used) {

	      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next) {
		if (matchWin(t, &matches[j])) {
		  ewin->stack_next->stack_prev = ewin->stack_prev;
		  ewin->stack_prev->stack_next = ewin->stack_next;

		  ewin->stack_prev = t;
		  ewin->stack_next = t->stack_next;
		  ewin->stack_prev->stack_next = ewin;
		  ewin->stack_next->stack_prev = ewin;
		  return;
		}
	      }
	    }
	  }
	  return;
	}
    }
}

void
RestartInSession (char *filename)
{
  FILE *f = fopen (filename, "w");

#ifdef SESSION
  if (sm_conn)
    {
      if (last_used_filename)
	{
	  fprintf (f, "[SESSION_STATE] %s\n", last_used_filename);
	}
      save_session_state (sm_conn, filename, f);
      fclose (f);  
      set_sm_properties (sm_conn, filename, SmRestartImmediately);
      
      XSelectInput(dpy, Scr.Root, 0);
      XSync(dpy, 0);
      XCloseDisplay(dpy);
      
      SmcCloseConnection(sm_conn, 0, NULL);
      
      exit (0); /* let the SM restart us */
    }
#endif
  SaveWindowStates (f);
  SaveGlobalState (f);
  fclose (f); /* return and let Done restart us */
}

#ifdef SESSION
static void
callback_save_yourself2(SmcConn sm_conn, SmPointer client_data)
{
  char *path = NULL;
  char *filename = NULL;
  FILE *f = NULL;
  Bool success = 0;
  struct passwd *pwd;

  path = getenv ("SM_SAVE_DIR");
  if (!path)
    {
      pwd = getpwuid (getuid ());
      path = pwd->pw_dir;
    }

  filename = tempnam (path, ".fvwm");
  f = fopen (filename, "w");
  success = save_session_state (sm_conn, filename, f);
  fclose (f);

  SmcSaveYourselfDone (sm_conn, success);
  sent_save_done = 1;
}

static void
set_sm_properties (SmcConn sm_conn, char *filename, char hint)
{
  SmProp prop1, prop2, prop3, prop4, prop5, prop6, *props[6];
  SmPropValue prop1val, prop2val, prop3val, prop4val, prop5val, prop6val;
  char discardCommand[80];
  struct passwd *pwd;
  char *user_id;
  int numVals, i, priority = 30;

  if (!sm_conn) return;

  pwd = getpwuid (getuid());
  user_id = pwd->pw_name;

  prop1.name = SmProgram;
  prop1.type = SmARRAY8;
  prop1.num_vals = 1;
  prop1.vals = &prop1val;
  prop1val.value = g_argv[0];
  prop1val.length = strlen (g_argv[0]);
  
  prop2.name = SmUserID;
  prop2.type = SmARRAY8;
  prop2.num_vals = 1;
  prop2.vals = &prop2val;
  prop2val.value = (SmPointer) user_id;
  prop2val.length = strlen (user_id);
  
  prop3.name = SmRestartStyleHint;
  prop3.type = SmCARD8;
  prop3.num_vals = 1;
  prop3.vals = &prop3val;
  prop3val.value = (SmPointer) &hint;
  prop3val.length = 1;
  
  prop4.name = "_GSM_Priority";
  prop4.type = SmCARD8;
  prop4.num_vals = 1;
  prop4.vals = &prop4val;
  prop4val.value = (SmPointer) &priority;
  prop4val.length = 1;

  prop5.name = SmRestartCommand;
  prop5.type = SmLISTofARRAY8;

  prop5.vals = (SmPropValue *) malloc ((g_argc + 7) * sizeof (SmPropValue));

  numVals = 0;

  for (i = 0; i < g_argc; i++)
    {
      if (strcmp (g_argv[i], "-clientId") == 0 ||
	  strcmp (g_argv[i], "-restore") == 0 ||
	  strcmp (g_argv[i], "-d") == 0)
	{
	  i++;
	} else if (strcmp (g_argv[i], "-s") != 0) {
	  prop5.vals[numVals].value = (SmPointer) g_argv[i];
	  prop5.vals[numVals++].length = strlen (g_argv[i]);
	}
    }

  prop5.vals[numVals].value = (SmPointer) "-d";
  prop5.vals[numVals++].length = 2;

  prop5.vals[numVals].value = (SmPointer) XDisplayString (dpy);
  prop5.vals[numVals++].length = strlen (XDisplayString (dpy));

  prop5.vals[numVals].value = (SmPointer) "-s";
  prop5.vals[numVals++].length = 2;

  prop5.vals[numVals].value = (SmPointer) "-clientId";
  prop5.vals[numVals++].length = 9;

  prop5.vals[numVals].value = (SmPointer) sm_client_id;
  prop5.vals[numVals++].length = strlen (sm_client_id);

  prop5.vals[numVals].value = (SmPointer) "-restore";
  prop5.vals[numVals++].length = 8;

  prop5.vals[numVals].value = (SmPointer) filename;
  prop5.vals[numVals++].length = strlen (filename);

  prop5.num_vals = numVals;

  prop6.name = SmDiscardCommand;
#ifdef XSM_BUGGY_DISCARD_COMMAND
  /* the protocol spec says that the discard command
     should be LISTofARRAY8 on posix systems, but xsm
     demands that it be ARRAY8. 
  */
  sprintf (discardCommand, "rm -f %s", filename);
  prop6.type = SmARRAY8;
  prop6.num_vals = 1;
  prop6.vals = &prop6val;
  prop6val.value = (SmPointer) discardCommand;
  prop6val.length = strlen (discardCommand);
#else
  prop6.type = SmLISTofARRAY8;
  prop6.num_vals = 3;
  prop6.vals = (SmPropValue *) malloc (3 * sizeof (SmPropValue));
  prop6.vals[0].value = "rm";
  prop6.vals[0].length = 2;
  prop6.vals[1].value = "-f";
  prop6.vals[1].length = 2;
  prop6.vals[2].value = filename;
  prop6.vals[2].length = strlen (filename);
#endif

  props[0] = &prop1;
  props[1] = &prop2;
  props[2] = &prop3;
  props[3] = &prop4;
  props[4] = &prop5;
  props[5] = &prop6;
  
  SmcSetProperties (sm_conn, 6, props);

  free ((char *) prop5.vals);
#ifndef XSM_BUGGY_DISCARD_COMMAND
  free ((char *) prop6.vals);
#endif
}

static int
save_session_state (SmcConn sm_conn, char *filename, FILE *cfg_file)
{
  Bool success = 0;

  if (!filename || !cfg_file) return 0;

  success = (SaveWindowStates (cfg_file) && SaveGlobalState (cfg_file));

  set_sm_properties (sm_conn, filename, SmRestartIfRunning);
  if (last_used_filename) free (last_used_filename);
  last_used_filename = filename;

  return success;
}

static void
callback_save_yourself(SmcConn sm_conn, SmPointer client_data,
		       int save_style, Bool shutdown, int interact_style,
		       Bool fast)
{
  if (!SmcRequestSaveYourselfPhase2(sm_conn, callback_save_yourself2, NULL))
    {
      SmcSaveYourselfDone (sm_conn, False);
      sent_save_done = 1;
    }
  else
    sent_save_done = 0;
}

static void
callback_die(SmcConn sm_conn, SmPointer client_data)
{
  SmcCloseConnection(sm_conn, 0, NULL);
  sm_fd = -1;

  if (master_pid != getpid())
    kill(master_pid, SIGTERM);
  Done(0, NULL);
}

static void
callback_save_complete(SmcConn sm_conn, SmPointer client_data)
{
}

static void
callback_shutdown_cancelled(SmcConn sm_conn, SmPointer client_data)
{
  if (!sent_save_done)
    {
      SmcSaveYourselfDone(sm_conn, False);
      sent_save_done = 1;
    }
}

/* the following is taken from xsm */

static IceIOErrorHandler prev_handler;

static void
MyIoErrorHandler (ice_conn)

  IceConn ice_conn;

{
  if (prev_handler)
    (*prev_handler) (ice_conn);
}

static void
InstallIOErrorHandler (void)

{
  IceIOErrorHandler default_handler;

  prev_handler = IceSetIOErrorHandler (NULL);
  default_handler = IceSetIOErrorHandler (MyIoErrorHandler);
  if (prev_handler == default_handler)
    prev_handler = NULL;
}

#endif /* SESSION */

void
SessionInit(char *previous_client_id)
{
#ifdef SESSION
  char                error_string_ret[4096] = "";
  static SmPointer    context;
  SmcCallbacks        callbacks;

  InstallIOErrorHandler();

  callbacks.save_yourself.callback = callback_save_yourself;
  callbacks.die.callback = callback_die;
  callbacks.save_complete.callback = callback_save_complete;
  callbacks.shutdown_cancelled.callback = callback_shutdown_cancelled;

  callbacks.save_yourself.client_data =
    callbacks.die.client_data =
    callbacks.save_complete.client_data =
    callbacks.shutdown_cancelled.client_data = (SmPointer) NULL;

  sm_conn = SmcOpenConnection(NULL, &context, SmProtoMajor, SmProtoMinor,
			      SmcSaveYourselfProcMask | SmcDieProcMask |
			      SmcSaveCompleteProcMask |
			      SmcShutdownCancelledProcMask,
			      &callbacks, previous_client_id, &sm_client_id,
			      4096, error_string_ret);

  if (!sm_conn)
    {
      /*
	Don't annoy users which don't use a session manager
      */
      if (previous_client_id)
        {
	  fvwm_msg(ERR, "SessionInit",
		   "While connecting to session manager:\n%s.",
		   error_string_ret);
        }
      sm_fd = -1;
    }
  else
    {
      sm_fd = IceConnectionNumber(SmcGetIceConnection(sm_conn));
    }
#endif
}

void
ProcessICEMsgs(void)
{
#ifdef SESSION
  IceProcessMessagesStatus status;

  if (sm_fd < 0)
    return;
  status = IceProcessMessages(SmcGetIceConnection(sm_conn), NULL, NULL);
  if (status == IceProcessMessagesIOError)
    {
      fvwm_msg(ERR, "ProcessICEMSGS",
	       "Connection to session manager lost\n");
      sm_conn = NULL;
      sm_fd = -1;
    }
#endif
}

