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

/*#define FVWM_DEBUG_MSGS 1*/
/*#define FVWM_DEBUG_DEVEL 1*/

#include "config.h"

#include <stdio.h>
#include <pwd.h>
#include <signal.h>

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "add_window.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "session.h"
#include "module_interface.h"
#include "stack.h"
#include "icccm2.h"
#include "virtual.h"
#include "geometry.h"
#ifdef SESSION
#include <X11/SM/SMlib.h>
#endif

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
  int                 width_defect_max, height_defect_max;
  int                 max_x_offset, max_y_offset;
  int                 desktop;
  int                 layer;
  int                 used;
  int                 gravity;
  window_flags        flags;
}
Match;

#ifdef SESSION
int sm_fd = -1;
static char *previous_sm_client_id = NULL;
static char *sm_client_id = NULL;
static Bool sent_save_done = 0;
#endif

static int num_match = 0;
static Match *matches = NULL;
static Bool does_file_version_match = False;

extern Bool Restarting;

static
char *duplicate(const char *s)
{
  int l;
  char *r;

  if (!s) return NULL;
  l = strlen(s);
  r = (char *) safemalloc (sizeof(char)*(l+1));
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
static int
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
  fprintf(f, "  [MISC] %i %i %i\n",
	  Scr.ClickTime, Scr.ColormapFocus, Scr.ColorLimit);
  fprintf(f, "  [STYLE] %i %i\n", Scr.gs.EmulateMWM, Scr.gs.EmulateWIN);
  return 1;
}

static Bool doPreserveState = True;

#ifdef SESSION
extern char **g_argv;
extern int g_argc;

SmcConn sm_conn = NULL;
static char *realStateFilename = NULL;
static Bool goingToRestart = False;

static void
setSmProperties (SmcConn sm_conn, char *filename, char hint);

static void setRealStateFilename(char *filename)
{
  if (realStateFilename)
    free(realStateFilename);
  realStateFilename = safestrdup(filename);
}

static char *getUniqueStateFilename(void)
{
  const char *path = getenv("SM_SAVE_DIR");
  struct passwd *pwd;

  if (!path) {
    pwd = getpwuid(getuid());
    path = pwd->pw_dir;
  }
  return tempnam(path, ".fs-");
}

#else
#define setRealStateFilename(filename)
#endif /* SESSION */

void
LoadGlobalState(char *filename)
{
  FILE               *f;
  char                s[4096], s1[4096];
  /* char s2[256]; */
  int i1, i2, i3, i4, i5, i6;

  if (!does_file_version_match)
    return;
  if (!filename || !*filename)
    return;
  if ((f = fopen(filename, "r")) == NULL)
    return;

  while (fgets(s, sizeof(s), f))
  {
    i1 = 0; i2 = 0; i3 = 0; i4 = 0; i5 = 0; i6 = 0;
    sscanf(s, "%4000s", s1);
#ifdef SESSION
    /* If we are restarting, [REAL_STATE_FILENAME] points
     * to the file containing the true session state. */
    if (!strcmp(s1, "[REAL_STATE_FILENAME]"))
    {
    /* migo: temporarily (?) moved to LoadWindowStates (trick for gnome-session)
      sscanf(s, "%*s %s", s2);
      setSmProperties (sm_conn, s2, SmRestartIfRunning);
      setRealStateFilename(s2);
    */
    }
    else
#endif /* SESSION */
    if (!strcmp(s1, "[DESKTOP]"))
    {
      sscanf(s, "%*s %i", &i1);
      goto_desk(i1);
    }
    else if (!strcmp(s1, "[VIEWPORT]"))
    {
      sscanf(s, "%*s %i %i %i %i", &i1, &i2, &i3, &i4);
    /* migo: we don't want to lose DeskTopSize in configurations,
     * and it does not work well anyways - Gnome is not updated
      Scr.VxMax = i3;
      Scr.VyMax = i4;
    */
      MoveViewport(i1, i2, True);
    }
#if 0
    /* migo (08-Dec-1999): we don't want to eliminate .fvwm2rc for now */
    else if (/*!Restarting*/ 0)
    {
      /* Matthias: We don't want to restore too much state if
       * we are restarting, since that would make restarting
       * useless for rereading changed rc files. */
      if (!strcmp(s1, "[SCROLL]"))
      {
        sscanf(s, "%*s %i %i %i %i %i %i", &i1, &i2, &i3, &i4, &i5, &i6);
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
        sscanf(s, "%*s %i %i %i", &i1, &i2, &i3);
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
}

static char *
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

static char *
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

char *get_version_string()
{
  /* migo (14-Mar-2001): it is better to manually update a version string
   * in the stable branch, othervise saving sessions becomes useless */
  /*return CatString3(VERSION, ", ",__DATE__);*/

  return "2.4-4";
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
    return False;
  if ((f = fopen(filename, "r")) == NULL)
    return False;

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
	  "State file version (%s) does not match the current version (%s)\n"
	  "State file will be ignored", s1, current_v);
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

char *unspace_string(const char *str)
{
  static const char *spaces = " \t\n";
  char *tr_str = CatString2(str, NULL);
  int i;

  if (!tr_str)
    return NULL;

  for (i = 0; i < strlen(spaces); i++)
  {
    char *ptr = tr_str;
    while ((ptr = strchr(ptr, spaces[i])) != NULL)
      *(ptr++) = '_';
  }
  return tr_str;
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
  int i;

  for (ewin = get_next_window_in_stack_ring(&Scr.FvwmRoot);
       ewin != &Scr.FvwmRoot;
       ewin = get_next_window_in_stack_ring(ewin))
  {
    Bool is_icon_sticky;

    if (!XGetGeometry(dpy, ewin->w, &JunkRoot, &JunkX, &JunkY, &JunkWidth,
		      &JunkHeight, &JunkBW, &JunkDepth))
    {
      /* Don't save the state of windows that already died (i.e. modules)!
       */
      continue;
    }
    is_icon_sticky = (IS_STICKY(ewin) ||
		     (IS_ICONIFIED(ewin) && IS_ICON_STICKY(ewin)));
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
        fprintf(f, "  [RES_NAME] %s\n", ewin->class.res_name);
      if (ewin->class.res_name)
        fprintf(f, "  [RES_CLASS] %s\n", ewin->class.res_class);
      if (ewin->name)
        fprintf(f, "  [WM_NAME] %s\n", ewin->name);

      wm_command = NULL;
      wm_command_count = 0;
      if (XGetCommand(dpy, ewin->w, &wm_command, &wm_command_count) &&
	  wm_command && wm_command_count > 0)
      {
        fprintf(f, "  [WM_COMMAND] %i", wm_command_count);
        for (i = 0; i < wm_command_count; i++)
          fprintf(f, " %s", unspace_string(wm_command[i]));
        fprintf(f, "\n");
      }
      if (wm_command)
      {
        XFreeStringList(wm_command);
	wm_command = NULL;
      }
    } /* !window_role */

    gravity_get_naked_geometry(
      ewin->hints.win_gravity, ewin, &save_g, &ewin->normal_g);
    if (IS_STICKY(ewin))
    {
      save_g.x -= Scr.Vx;
      save_g.y -= Scr.Vy;
    }
    fprintf(
      f, "  [GEOMETRY] %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i\n",
      save_g.x, save_g.y, save_g.width, save_g.height,
      ewin->max_g.x, ewin->max_g.y, ewin->max_g.width, ewin->max_g.height,
      ewin->max_g_defect.width, ewin->max_g_defect.height,
      ewin->icon_g.x + ((!is_icon_sticky) ? Scr.Vx : 0),
      ewin->icon_g.y + ((!is_icon_sticky) ? Scr.Vy : 0),
      ewin->hints.win_gravity,
      ewin->max_offset.x, ewin->max_offset.y);
    fprintf(f, "  [DESK] %i\n", ewin->Desk);
    fprintf(f, "  [LAYER] %i\n", get_layer(ewin));
    fprintf(f, "  [FLAGS] ");
    for (i = 0; i < sizeof(window_flags); i++)
      fprintf(f, "%02x ", (int)(((unsigned char *)&(ewin->flags))[i]));
    fprintf(f, "\n");
  }
  return 1;
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
  int i, pos, pos1;
  unsigned long w;

  if (!VerifyVersionInfo(filename))
    return;

  if (!filename || !*filename)
    return;

  setRealStateFilename(filename);

  if ((f = fopen(filename, "r")) == NULL)
    return;

#ifdef FVWM_DEBUG_MSGS
  fprintf(stderr, "\tLoading %s\n", filename);
#endif
#ifdef FVWM_DEBUG_DEVEL
  system(CatString3("mkdir -p /tmp/fs-load; cp ", filename, " /tmp/fs-load"));
#endif

  while (fgets(s, sizeof(s), f))
  {
    sscanf(s, "%4000s", s1);
#ifdef SESSION
    /* migo: temporarily */
    if (!strcmp(s1, "[REAL_STATE_FILENAME]"))
    {
      sscanf(s, "%*s %s", s1);
      setSmProperties (sm_conn, s1, SmRestartIfRunning);
      setRealStateFilename(s1);
    } else
#endif /* SESSION */
    if (!strcmp(s1, "[CLIENT]"))
    {
      sscanf(s, "%*s %lx", &w);
      num_match++;
      matches =
	(Match *)saferealloc((void *)matches, sizeof(Match) * num_match);
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
      memset(&(matches[num_match - 1].flags), 0,
	     sizeof(window_flags));
      matches[num_match - 1].used = 0;
    }
    else if (!strcmp(s1, "[GEOMETRY]"))
    {
      sscanf(s, "%*s %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
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
      sscanf(s, "%*s %[^\n]", s1);
      matches[num_match - 1].client_id = duplicate(s1);
    }
    else if (!strcmp(s1, "[WINDOW_ROLE]"))
    {
      sscanf(s, "%*s %[^\n]", s1);
      matches[num_match - 1].window_role = duplicate(s1);
    }
    else if (!strcmp(s1, "[RES_NAME]"))
    {
      sscanf(s, "%*s %[^\n]", s1);
      matches[num_match - 1].res_name = duplicate(s1);
    }
    else if (!strcmp(s1, "[RES_CLASS]"))
    {
      sscanf(s, "%*s %[^\n]", s1);
      matches[num_match - 1].res_class = duplicate(s1);
    }
    else if (!strcmp(s1, "[WM_NAME]"))
    {
      sscanf(s, "%*s %[^\n]", s1);
      matches[num_match - 1].wm_name = duplicate(s1);
    }
    else if (!strcmp(s1, "[WM_COMMAND]"))
    {
      sscanf(s, "%*s %i%n",
	&matches[num_match - 1].wm_command_count, &pos);
      matches[num_match - 1].wm_command =
	(char **) safemalloc (matches[num_match - 1].wm_command_count *
			      sizeof (char *));
      for (i = 0; i < matches[num_match - 1].wm_command_count; i++)
      {
        sscanf (s+pos, "%s%n", s1, &pos1);
        pos += pos1;
        matches[num_match - 1].wm_command[i] = duplicate (s1);
      }
    }
  }
  fclose(f);
}

/* This complicated logic is from twm, where it is explained */

#define xstreq(a,b) ((!a && !b) || (a && b && (strcmp(a,b)==0)))

static Bool matchWin(FvwmWindow *w, Match *m)
{
  char *client_id = NULL;
  char *window_role = NULL;
  char **wm_command = NULL;
  int wm_command_count = 0, i;
  int found;

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
          (IS_NAME_CHANGED(w) || IS_NAME_CHANGED(m) ||
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

	  if (!XGetCommand (dpy, w->w, &wm_command, &wm_command_count))
	  {
	    wm_command = NULL;
	    wm_command_count = 0;
	  }
          if (wm_command_count == m->wm_command_count)
          {
            for (i = 0; i < wm_command_count; i++)
            {
              if (strcmp(unspace_string(wm_command[i]), m->wm_command[i]) != 0)
                break;
            }

            if (i == wm_command_count)
            {
              /* migo (21/Oct/1999): on restarts compare window ids too */
              if (!Restarting || w->w == m->win)
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

#ifdef FVWM_DEBUG_DEVEL
  fprintf(stderr, "\twin(%s, %s, %s, %d)\n[%d]\tmat(%s, %s, %s, %d)\n\n",
	  w->class.res_name, w->class.res_class, w->name, IS_NAME_CHANGED(w),
	  found,
	  m->res_name, m->res_class, m->wm_name, IS_NAME_CHANGED(m));
#endif
  return found;
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
MatchWinToSM(FvwmWindow *ewin, int *do_shade, int *do_max)
{
  int i;

  if (!does_file_version_match)
    return False;
  for (i = 0; i < num_match; i++)
  {
    if (!matches[i].used && matchWin(ewin, &matches[i]))
    {
      matches[i].used = 1;
      if (!Restarting)
      {
	/* We don't want to restore too much state if
	   we are restarting, since that would make restarting
	   useless for rereading changed rc files. */
	SET_DO_SKIP_WINDOW_LIST(
	  ewin, DO_SKIP_WINDOW_LIST(&(matches[i])));
	SET_ICON_SUPPRESSED(ewin, IS_ICON_SUPPRESSED(&(matches[i])));
	SET_HAS_NO_ICON_TITLE(ewin, HAS_NO_ICON_TITLE(&(matches[i])));
	SET_LENIENT(ewin, IS_LENIENT(&(matches[i])));
	SET_ICON_STICKY(ewin, IS_ICON_STICKY(&(matches[i])));
	SET_DO_SKIP_ICON_CIRCULATE(
	  ewin, DO_SKIP_ICON_CIRCULATE(&(matches[i])));
	SET_DO_SKIP_SHADED_CIRCULATE(
	  ewin, DO_SKIP_SHADED_CIRCULATE(&(matches[i])));
	SET_DO_SKIP_CIRCULATE(ewin, DO_SKIP_CIRCULATE(&(matches[i])));
	SET_FOCUS_MODE(ewin, GET_FOCUS_MODE(&(matches[i])));
	if (matches[i].wm_name)
	{
	  if (ewin->name)
	  {
	    free(ewin->name);
	  }
	  ewin->name = matches[i].wm_name;
	}
      }
      SET_PLACED_WB3(ewin,IS_PLACED_WB3(&(matches[i])));
      SET_PLACED_BY_FVWM(ewin,IS_PLACED_BY_FVWM(&(matches[i])));
      *do_shade = IS_SHADED(&(matches[i]));
      *do_max = IS_MAXIMIZED(&(matches[i]));
      SET_ICON_MOVED(ewin, IS_ICON_MOVED(&(matches[i])));
      if (IS_ICONIFIED(&(matches[i])))
      {
	/*
	  ICON_MOVED is necessary to make fvwm use icon_[xy]_loc
	  for icon placement
	*/
	SET_DO_START_ICONIC(ewin, 1);
	if (!IS_ICON_MOVED(ewin))
	{
	  /* only temporary to initially place the icon */
	  SET_ICON_MOVED(ewin, 1);
	  SET_DELETE_ICON_MOVED(ewin, 1);
	}
      }
      else
      {
	SET_DO_START_ICONIC(ewin, 0);
      }

      ewin->normal_g.x = matches[i].x;
      ewin->normal_g.y = matches[i].y;
      ewin->normal_g.width = matches[i].w;
      ewin->normal_g.height = matches[i].h;
      ewin->max_g.x = matches[i].x_max;
      ewin->max_g.y = matches[i].y_max;
      ewin->max_g.width = matches[i].w_max;
      ewin->max_g.height = matches[i].h_max;
      ewin->max_g_defect.width = matches[i].width_defect_max;
      ewin->max_g_defect.height = matches[i].height_defect_max;
      ewin->max_offset.x = matches[i].max_x_offset;
      ewin->max_offset.y = matches[i].max_y_offset;
      SET_STICKY(ewin, IS_STICKY(&(matches[i])));
      ewin->Desk = (IS_STICKY(ewin)) ? Scr.CurrentDesk : matches[i].desktop;
      set_layer(ewin, matches[i].layer);

      /* this is not enough to fight fvwms attempts to
	 put icons on the current page */
      ewin->icon_g.x = matches[i].icon_x;
      ewin->icon_g.y = matches[i].icon_y;
      if (!IS_STICKY(&(matches[i])) &&
	  !(IS_ICONIFIED(&(matches[i])) && IS_ICON_STICKY(&(matches[i]))))
      {
	ewin->icon_g.x -= Scr.Vx;
	ewin->icon_g.y -= Scr.Vy;
      }

      return True;
    }
  }

  return False;
}

static int saveStateFile(char *filename)
{
  FILE *f;
  int success;

  if (!filename || !*filename)
    return 0;
  if ((f = fopen(filename, "w")) == NULL)
    return 0;

  fprintf(f, "# This file is generated by fvwm."
	  " It stores global and window states.\n");
  fprintf(f, "# Normally, you must never delete this file,"
	  " it will be auto-deleted.\n\n");

#ifdef SESSION
  if (goingToRestart)
  {
    fprintf(f, "[REAL_STATE_FILENAME] %s\n", realStateFilename);
    goingToRestart = False;  /* not needed */
  }
#endif

  success = doPreserveState?
    SaveVersionInfo(f) && SaveWindowStates(f) && SaveGlobalState(f):
    1;
  doPreserveState = True;
  if (fclose(f) != 0)
    return 0;

#ifdef FVWM_DEBUG_DEVEL
  system(CatString3("mkdir -p /tmp/fs-save; cp ", filename, " /tmp/fs-save"));
#endif
#ifdef FVWM_DEBUG_MSGS
  fprintf(stderr, "\tSaving %s\n", filename);
#endif

  return success;
}

void
RestartInSession (char *filename, Bool isNative, Bool _doPreserveState)
{
  doPreserveState = _doPreserveState;
#ifdef SESSION
  if (sm_conn && isNative)
  {
    goingToRestart = True;

    saveStateFile(filename);
    setSmProperties(sm_conn, filename, SmRestartImmediately);

    MoveViewport(0, 0, False);
    Reborder();

    CloseICCCM2();
    XCloseDisplay(dpy);

    SmcCloseConnection(sm_conn, 0, NULL);

#ifdef FVWM_DEBUG_MSGS
    fprintf(stderr, "[FVWM]: Exiting, now SM must restart us.\n");
#endif
    /* Close all my pipes */
    ClosePipes();

    exit(0); /* let the SM restart us */
  }
#endif
  saveStateFile(filename);
  /* return and let Done restart us */
}

#ifdef SESSION
void SetClientID(char *client_id)
{
  previous_sm_client_id = client_id;
}

static void
callback_save_yourself2(SmcConn sm_conn, SmPointer client_data)
{
  Bool success = 0;
  char *filename = getUniqueStateFilename();

#ifdef FVWM_DEBUG_MSGS
  fprintf(stderr, "\t[callback_save_yourself2]\n");
#endif

  success = saveStateFile(filename);
  if (success)
  {
    setSmProperties(sm_conn, filename, SmRestartIfRunning);
    setRealStateFilename(filename);
  }
  free(filename);

  SmcSaveYourselfDone (sm_conn, success);
  sent_save_done = 1;
}

static void
setSmProperties (SmcConn sm_conn, char *filename, char hint)
{
  SmProp prop1, prop2, prop3, prop4, prop5, prop6, *props[6];
  SmPropValue prop1val, prop2val, prop3val, prop4val;
/*#ifdef XSM_BUGGY_DISCARD_COMMAND*/
  SmPropValue prop6val;
/*#endif*/
  struct passwd *pwd;
  char *user_id;
  int numVals, i, priority = 30;
  Bool xsmDetected;

#ifdef FVWM_DEBUG_MSGS
  fprintf(stderr, "\t[setSmProperties] state filename: %s%s\n", filename,
    sm_conn? "": " - not connected");
#endif

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
    }
    else if (strcmp (g_argv[i], "-s") != 0)
    {
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

/*
 * migo (15-Jul-1999): IMHO, XSM_BUGGY_DISCARD_COMMAND must be deleted at all.
 * Is there a better way to detect xsm, then requiring to set environment?
 */
  xsmDetected =
#ifdef XSM_BUGGY_DISCARD_COMMAND
    1
#else
    StrEquals(getenv("SESSION_MANAGER_NAME"), "xsm")
#endif
  ;

  /*#ifdef XSM_BUGGY_DISCARD_COMMAND*/
  if (xsmDetected) {
    /* the protocol spec says that the discard command
       should be LISTofARRAY8 on posix systems, but xsm
       demands that it be ARRAY8.
    */
    char *discardCommand = alloca((10 + strlen(filename)) * sizeof(char));
    sprintf (discardCommand, "rm -f '%s'", filename);
    prop6.type = SmARRAY8;
    prop6.num_vals = 1;
    prop6.vals = &prop6val;
    prop6val.value = (SmPointer) discardCommand;
    prop6val.length = strlen (discardCommand);
    /*#else*/
  } else {
    prop6.type = SmLISTofARRAY8;
    prop6.num_vals = 3;
    prop6.vals = (SmPropValue *) malloc (3 * sizeof (SmPropValue));
    prop6.vals[0].value = "rm";
    prop6.vals[0].length = 2;
    prop6.vals[1].value = "-f";
    prop6.vals[1].length = 2;
    prop6.vals[2].value = filename;
    prop6.vals[2].length = strlen (filename);
    /*#endif*/
  }

  props[0] = &prop1;
  props[1] = &prop2;
  props[2] = &prop3;
  props[3] = &prop4;
  props[4] = &prop5;
  props[5] = &prop6;

  SmcSetProperties (sm_conn, 6, props);

  free ((char *) prop5.vals);
/*#ifndef XSM_BUGGY_DISCARD_COMMAND*/
  if (!xsmDetected) free ((char *) prop6.vals);
/*#endif*/
}

/* ------------------------------------------------------------------------ */
static void
callback_save_yourself(SmcConn sm_conn, SmPointer client_data,
		       int save_style, Bool shutdown, int interact_style,
		       Bool fast)
{
#ifdef FVWM_DEBUG_MSGS
  fprintf(stderr, "\t[callback_save_yourself] (save=%d, shut=%d, intr=%d, fast=%d)\n", save_style, shutdown, interact_style, fast);
#endif

  if (!SmcRequestSaveYourselfPhase2(sm_conn, callback_save_yourself2, NULL))
  {
    SmcSaveYourselfDone (sm_conn, False);
    sent_save_done = 1;
  }
  else
    sent_save_done = 0;
}

/* ------------------------------------------------------------------------ */
static void
callback_die(SmcConn sm_conn, SmPointer client_data)
{
#ifdef FVWM_DEBUG_MSGS
  fprintf(stderr, "\t[callback_die]\n");
#endif

  SmcCloseConnection(sm_conn, 0, NULL);
  sm_fd = -1;

  if (master_pid != getpid())
    kill(master_pid, SIGTERM);
  Done(0, NULL);
}

/* ------------------------------------------------------------------------ */
static void
callback_save_complete(SmcConn sm_conn, SmPointer client_data)
{
#ifdef FVWM_DEBUG_MSGS
  fprintf(stderr, "\t[callback_save_complete]\n");
#endif
}

/* ------------------------------------------------------------------------ */
static void
callback_shutdown_cancelled(SmcConn sm_conn, SmPointer client_data)
{
#ifdef FVWM_DEBUG_MSGS
  fprintf(stderr, "\t[callback_shutdown_cancelled]\n");
#endif

  if (!sent_save_done)
  {
    SmcSaveYourselfDone(sm_conn, False);
    sent_save_done = 1;
  }
}

/* the following is taken from xsm */

static IceIOErrorHandler prev_handler;

/* ------------------------------------------------------------------------ */
static void
MyIoErrorHandler (IceConn ice_conn)
{
  if (prev_handler)
    (*prev_handler) (ice_conn);
}

/* ------------------------------------------------------------------------ */
static void
InstallIOErrorHandler (void)
{
  IceIOErrorHandler default_handler;

  prev_handler = IceSetIOErrorHandler (NULL);
  default_handler = IceSetIOErrorHandler (MyIoErrorHandler);
  if (prev_handler == default_handler)
    prev_handler = NULL;
}

/* ------------------------------------------------------------------------ */
void
SessionInit(void)
{
  char error_string_ret[4096] = "";
  static SmPointer context;
  SmcCallbacks callbacks;

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
			      &callbacks, previous_sm_client_id, &sm_client_id,
			      4096, error_string_ret);

  if (!sm_conn)
    {
      /*
	Don't annoy users which don't use a session manager
      */
      if (previous_sm_client_id)
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
      setInitFunctionName(0, "SessionInitFunction");
      setInitFunctionName(1, "SessionRestartFunction");
      setInitFunctionName(2, "SessionExitFunction");
    }
}

/* ------------------------------------------------------------------------ */
void
ProcessICEMsgs(void)
{
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
}
#endif


/* ------------------------------------------------------------------------ */
/*
 * Fvwm Function implementation: QuitSession, SaveSession, SaveQuitSession.
 * migo (15-Jun-1999): I am not sure this is right.
 * The session mabager must implement SmsSaveYourselfRequest (xsm doesn't?).
 * Alternative implementation may use unix signals, but this does not work
 * with all session managers (also must suppose that SM runs locally).
 */

#ifdef SESSION
int getSmPid(void)
{
  const char *sessionManagerVar = getenv("SESSION_MANAGER");
  const char *smPidStrPtr;
  int smPid = 0;

  if (!sessionManagerVar) return 0;
  smPidStrPtr = strchr(sessionManagerVar, ',');
  if (!smPidStrPtr) return 0;

  while (smPidStrPtr > sessionManagerVar && isdigit(*(--smPidStrPtr)))
    ;
  while (isdigit(*(++smPidStrPtr))) {
    smPid = smPid * 10 + *smPidStrPtr - '0';
  }
  return smPid;
}
#endif

/*
 * quitSession - hopefully shutdowns the session
 */
Bool quitSession(void)
{
#ifdef SESSION
  if (!sm_conn) return False;

  SmcRequestSaveYourself(
    sm_conn,
    SmSaveLocal,
    True,               /* shutdown */
    SmInteractStyleAny,
    False,              /* fast */
    True                /* global */
  );
  return True;

/* migo: xsm does not support RequestSaveYourself, but supports signals: */
/*
  int smPid = getSmPid();
  if (!smPid) return False;
  return kill(smPid, SIGTERM) == 0? True: False;
*/
#else
  return False;
#endif
}

/*
 * saveSession - hopefully saves the session
 */
Bool saveSession(void)
{
#ifdef SESSION
  if (!sm_conn) return False;

  SmcRequestSaveYourself(
    sm_conn,
    SmSaveBoth,
    False,              /* shutdown */
    SmInteractStyleAny,
    False,              /* fast */
    True                /* global */
  );
  return True;

/* migo: xsm does not support RequestSaveYourself, but supports signals: */
/*
  int smPid = getSmPid();
  if (!smPid) return False;
  return kill(smPid, SIGUSR1) == 0? True: False;
*/
#else
  return False;
#endif
}

/*
 * saveQuitSession - hopefully saves and shutdowns the session
 */
Bool saveQuitSession(void)
{
#ifdef SESSION
  if (!sm_conn) return False;

  SmcRequestSaveYourself(
    sm_conn,
    SmSaveBoth,
    True,               /* shutdown */
    SmInteractStyleAny,
    False,              /* fast */
    True                /* global */
  );
  return True;

/* migo: xsm does not support RequestSaveYourself, but supports signals: */
/*
  if (saveSession() == False) return False;
  sleep(3);  / * doesn't work anyway * /
  if (quitSession() == False) return False;
  return True;
*/
#else
  return False;
#endif
}

void CMD_QuitSession(F_CMD_ARGS)
{
  quitSession();
}

void CMD_SaveSession(F_CMD_ARGS)
{
  saveSession();
}

void CMD_SaveQuitSession(F_CMD_ARGS)
{
  saveQuitSession();
}
