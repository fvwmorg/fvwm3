/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified
 * by Rob Nation
 ****************************************************************************/
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/

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


/**********************************************************************
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 **********************************************************************/
#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/FScreen.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "commands.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "defaults.h"
#include "add_window.h"
#include "events.h"
#include "module_interface.h"
#include "stack.h"
#include "update.h"
#include "style.h"
#include "icons.h"
#include "gnome.h"
#include "ewmh.h"
#include "focus.h"
#include "placement.h"
#include "geometry.h"
#include "session.h"
#include "move_resize.h"
#include "borders.h"
#include "colormaps.h"
#include "decorations.h"

char NoName[] = "Untitled"; /* name if no name in XA_WM_NAME */
char NoClass[] = "NoClass"; /* Class if no res_class in class hints */
char NoResource[] = "NoResource"; /* Class if no res_name in class hints */
long isIconicState = 0;
Bool PPosOverride = False;
Bool isIconifiedByParent = False;

void FetchWmProtocols(FvwmWindow *);
void GetWindowSizeHints(FvwmWindow *);
/***********************************************************************/

Bool setup_window_structure(
  FvwmWindow **ptmp_win, Window w, FvwmWindow *ReuseWin)
{
  FvwmWindow save_state;
  FvwmWindow *savewin = NULL;

  memset(&save_state, '\0', sizeof(FvwmWindow));

  /*
      Allocate space for the FvwmWindow struct, or reuse an
      old one (on Recapture).
  */
  if (ReuseWin == NULL)
  {
    *ptmp_win = (FvwmWindow *)safemalloc(sizeof(FvwmWindow));
    if (*ptmp_win == (FvwmWindow *)0)
    {
      return False;
    }
  }
  else
  {
    *ptmp_win = ReuseWin;
    savewin = &save_state;
    memcpy(savewin, ReuseWin, sizeof(FvwmWindow));
  }

  /*
    RBW - 1999/05/28 - modify this when we implement the preserving of
    various states across a Recapture. The Destroy function in misc.c may
    also need tweaking, depending on what you want to preserve.
    For now, just zap any old information, except the desk.
  */
  memset(*ptmp_win, '\0', sizeof(FvwmWindow));
  (*ptmp_win)->w = w;
  if (savewin != NULL)
  {
    (*ptmp_win)->Desk = savewin->Desk;
    SET_SHADED(*ptmp_win, IS_SHADED(savewin));
    SET_PLACED_WB3(*ptmp_win,IS_PLACED_WB3(savewin));
    SET_PLACED_BY_FVWM(*ptmp_win, IS_PLACED_BY_FVWM(savewin));
    SET_HAS_EWMH_WM_ICON_HINT(*ptmp_win, HAS_EWMH_WM_ICON_HINT(savewin));
    (*ptmp_win)->ewmh_mini_icon_width = savewin->ewmh_mini_icon_width;
    (*ptmp_win)->ewmh_mini_icon_height = savewin->ewmh_mini_icon_height;
    (*ptmp_win)->ewmh_icon_width = savewin->ewmh_icon_width;
    (*ptmp_win)->ewmh_icon_height = savewin->ewmh_icon_height;
    (*ptmp_win)->ewmh_hint_desktop = savewin->ewmh_hint_desktop;
    /* restore ewmh state */
    EWMH_SetWMState(savewin, True);
    SET_HAS_EWMH_INIT_WM_DESKTOP(
      *ptmp_win, HAS_EWMH_INIT_WM_DESKTOP(savewin));
    SET_HAS_EWMH_INIT_FULLSCREEN_STATE(
      *ptmp_win, HAS_EWMH_INIT_FULLSCREEN_STATE(savewin));
    SET_HAS_EWMH_INIT_HIDDEN_STATE(
      *ptmp_win, HAS_EWMH_INIT_HIDDEN_STATE(savewin));
    SET_HAS_EWMH_INIT_MAXHORIZ_STATE(
      *ptmp_win, HAS_EWMH_INIT_MAXHORIZ_STATE(savewin));
    SET_HAS_EWMH_INIT_MAXVERT_STATE(
      *ptmp_win, HAS_EWMH_INIT_MAXVERT_STATE(savewin));
    SET_HAS_EWMH_INIT_SHADED_STATE(
      *ptmp_win, HAS_EWMH_INIT_SHADED_STATE(savewin));
    SET_HAS_EWMH_INIT_STICKY_STATE(
      *ptmp_win, HAS_EWMH_INIT_STICKY_STATE(savewin));
  }

  (*ptmp_win)->cmap_windows = (Window *)NULL;
#ifdef MINI_ICONS
  (*ptmp_win)->mini_pixmap_file = NULL;
  (*ptmp_win)->mini_icon = NULL;
#endif

  return True;
}

void setup_window_name_count(FvwmWindow *tmp_win)
{
  FvwmWindow *t;
  int count = 0;
  Bool done = False;

  if (!tmp_win->name)
    done = True;

  if (tmp_win->icon_name && strcmp(tmp_win->name, tmp_win->icon_name) == 0)
    count = tmp_win->icon_name_count;

  while (!done)
  {
    done = True;
    for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if (t == tmp_win)
	continue;
      if ((t->name && strcmp(t->name, tmp_win->name) == 0 &&
	  t->name_count == count) ||
	  (t->icon_name && strcmp(t->icon_name, tmp_win->name) == 0 &&
	   t->icon_name_count == count))
      {
	count++;
	done = False;
      }
    }
  }
  tmp_win->name_count = count;
}

static void setup_icon_name_count(FvwmWindow *tmp_win)
{
  FvwmWindow *t;
  int count = 0;
  Bool done = False;

  if (!tmp_win->icon_name)
    done = True;

  if (tmp_win->name && strcmp(tmp_win->name, tmp_win->icon_name) == 0)
    count = tmp_win->name_count;

  while (!done)
  {
    done = True;
    for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if (t == tmp_win)
	continue;
      if ((t->icon_name && strcmp(t->icon_name, tmp_win->icon_name) == 0 &&
	  t->icon_name_count == count) ||
	  (t->name && strcmp(t->name, tmp_win->icon_name) == 0 &&
	   t->name_count == count))
      {
	count++;
	done = False;
      }
    }
  }
  tmp_win->icon_name_count = count;
}

void setup_visible_name(FvwmWindow *tmp_win, Bool is_icon)
{
  char *ext_name;
  char *name;
  int count;
  int len;

  if (tmp_win == NULL)
    return; /* should never happen */

  if (is_icon)
  {
    if (tmp_win->visible_icon_name != NULL &&
	tmp_win->visible_icon_name != tmp_win->icon_name &&
	tmp_win->visible_icon_name != tmp_win->name &&
	tmp_win->visible_icon_name != NoName)
    {
      free(tmp_win->visible_icon_name);
      tmp_win->visible_icon_name = NULL;
    }
    name = tmp_win->icon_name;
    setup_icon_name_count(tmp_win);
    count = tmp_win->icon_name_count;
  }
  else
  {
    if (tmp_win->visible_name != NULL &&
	tmp_win->visible_name != tmp_win->name &&
	tmp_win->visible_name != NoName)
    {
      free(tmp_win->visible_name);
      tmp_win->visible_name = NULL;
    }
    name = tmp_win->name;
    setup_window_name_count(tmp_win);
    count = tmp_win->name_count;
  }

  if (name == NULL)
  {
    return; /* should never happen */
  }

  if (count > 998)
    count = 998;

  if (count != 0 &&
      ((is_icon && USE_INDEXED_ICON_NAME(tmp_win)) ||
       (!is_icon && USE_INDEXED_WINDOW_NAME(tmp_win))))
  {
    len = strlen(name);
    count++;
    ext_name = (char *)safemalloc(len + 6);
    sprintf(ext_name,"%s (%d)", name, count);
  }
  else
    ext_name = name;

  if (is_icon)
    tmp_win->visible_icon_name = ext_name;
  else
    tmp_win->visible_name = ext_name;
}

void setup_window_name(FvwmWindow *tmp_win)
{
  if (!EWMH_WMName(tmp_win, NULL, NULL, 0))
  {
    tmp_win->name = NoName;
    MULTIBYTE_CODE(tmp_win->name_list = NULL);
    FlocaleGetNameProperty(XGetWMName, dpy, tmp_win->w,
			   MULTIBYTE_ARG(&(tmp_win->name_list))
			   &(tmp_win->name));
  }

  if (debugging)
    fvwm_msg(DBG,"setup_window_name","Assigned name %s'",tmp_win->name);
}

static void setup_class_and_resource(FvwmWindow *tmp_win)
{
  /* removing NoClass change for now... */
  tmp_win->class.res_name = NoResource;
  tmp_win->class.res_class = NoClass;
  XGetClassHint(dpy, tmp_win->w, &tmp_win->class);
  if (tmp_win->class.res_name == NULL)
    tmp_win->class.res_name = NoResource;
  if (tmp_win->class.res_class == NULL)
    tmp_win->class.res_class = NoClass;
  FetchWmProtocols (tmp_win);
  FetchWmColormapWindows (tmp_win);
  if(!(XGetWindowAttributes(dpy, tmp_win->w, &(tmp_win->attr))))
    tmp_win->attr.colormap = Pcmap;
}

void setup_wm_hints(FvwmWindow *tmp_win)
{
  tmp_win->wmhints = XGetWMHints(dpy, tmp_win->w);
  set_focus_model(tmp_win);
}

static void destroy_window_font(FvwmWindow *tmp_win)
{
  if (IS_WINDOW_FONT_LOADED(tmp_win) && !USING_DEFAULT_WINDOW_FONT(tmp_win) &&
      tmp_win->title_font != Scr.DefaultFont)
  {
    FlocaleUnloadFont(dpy, tmp_win->title_font);
  }
  SET_WINDOW_FONT_LOADED(tmp_win, 0);
  /* Fall back to default font. There are some race conditions when a window
   * is destroyed and recaptured where an invalid font might be accessed
   * otherwise. */
  tmp_win->title_font = Scr.DefaultFont;
  SET_USING_DEFAULT_WINDOW_FONT(tmp_win, 1);
}

void setup_window_font(
  FvwmWindow *tmp_win, window_style *pstyle, Bool do_destroy)
{
  int height;

  /* get rid of old font */
  if (do_destroy)
  {
    destroy_window_font(tmp_win);
    /* destroy_window_font resets the IS_WINDOW_FONT_LOADED flag */
  }
  /* load new font */
  if (!IS_WINDOW_FONT_LOADED(tmp_win))
  {
    if (SFHAS_WINDOW_FONT(*pstyle) && SGET_WINDOW_FONT(*pstyle) &&
	(tmp_win->title_font =
	 FlocaleLoadFont(dpy, SGET_WINDOW_FONT(*pstyle), "FVWM")))
    {
      SET_USING_DEFAULT_WINDOW_FONT(tmp_win, 0);
    }
    else
    {
      /* no explicit font or failed to load, use default font instead */
      tmp_win->title_font = Scr.DefaultFont;
      SET_USING_DEFAULT_WINDOW_FONT(tmp_win, 1);
    }
    SET_WINDOW_FONT_LOADED(tmp_win, 1);
  }

  /* adjust font offset according to height specified in title style */
  if (tmp_win->decor->title_height)
  {
    height = tmp_win->decor->title_height;
    tmp_win->title_text_y = tmp_win->title_font->ascent +
      (height - (tmp_win->title_font->height + EXTRA_TITLE_FONT_HEIGHT)) / 2;
    if (tmp_win->title_text_y < tmp_win->title_font->ascent)
      tmp_win->title_text_y = tmp_win->title_font->ascent;
    tmp_win->title_g.height = height;
    tmp_win->corner_width = height + tmp_win->boundary_width;
  }
  else
  {
    height = tmp_win->title_font->height;
    tmp_win->title_text_y = tmp_win->title_font->ascent;
    tmp_win->corner_width =
      height + EXTRA_TITLE_FONT_HEIGHT + tmp_win->boundary_width;
    tmp_win->title_g.height =
      tmp_win->title_font->height + EXTRA_TITLE_FONT_HEIGHT;
  }
  if (!HAS_TITLE(tmp_win))
  {
    tmp_win->title_g.height = 0;
  }
  tmp_win->title_top_height =
    (HAS_BOTTOM_TITLE(tmp_win)) ? 0 : tmp_win->title_g.height;

  return;
}

static void destroy_icon_font(FvwmWindow *tmp_win)
{
  if (IS_ICON_FONT_LOADED(tmp_win) && !USING_DEFAULT_ICON_FONT(tmp_win) &&
      tmp_win->icon_font != Scr.DefaultFont)
  {
    FlocaleUnloadFont(dpy, tmp_win->icon_font);
  }
  SET_ICON_FONT_LOADED(tmp_win, 0);
  /* Fall back to default font (see comment above). */
  tmp_win->icon_font = Scr.DefaultFont;
  SET_USING_DEFAULT_ICON_FONT(tmp_win, 1);
}

void setup_icon_font(
  FvwmWindow *tmp_win, window_style *pstyle, Bool do_destroy)
{
  int height;

  height = (IS_ICON_FONT_LOADED(tmp_win)) ? tmp_win->icon_font->height : 0;
  if (IS_ICON_SUPPRESSED(tmp_win) || HAS_NO_ICON_TITLE(tmp_win))
  {
    if (IS_ICON_FONT_LOADED(tmp_win))
    {
      destroy_icon_font(tmp_win);
      /* destroy_icon_font resets the IS_ICON_FONT_LOADED flag */
    }
    return;
  }
  /* get rid of old font */
  if (do_destroy && IS_ICON_FONT_LOADED(tmp_win))
  {
    destroy_icon_font(tmp_win);
    /* destroy_icon_font resets the IS_ICON_FONT_LOADED flag */
  }
  /* load new font */
  if (!IS_ICON_FONT_LOADED(tmp_win))
  {
    if (SFHAS_ICON_FONT(*pstyle) && SGET_ICON_FONT(*pstyle) &&
	(tmp_win->icon_font =
	 FlocaleLoadFont(dpy, SGET_ICON_FONT(*pstyle), "FVWM")))
    {
      SET_USING_DEFAULT_ICON_FONT(tmp_win, 0);
    }
    else
    {
      /* no explicit font or failed to load, use default font instead */
      tmp_win->icon_font = Scr.DefaultFont;
      SET_USING_DEFAULT_ICON_FONT(tmp_win, 1);
    }
    SET_ICON_FONT_LOADED(tmp_win, 1);
  }
  /* adjust y position of existing icons */
  if (height)
  {
    resize_icon_title_height(tmp_win, height - tmp_win->icon_font->height);
    /* this repositions the icon even if the window is not iconified */
    DrawIconWindow(tmp_win);
  }

  return;
}

void setup_style_and_decor(
  FvwmWindow *tmp_win, window_style *pstyle, short *buttons)
{
  /* first copy the static styles into the window struct */
  memcpy(&(FW_COMMON_FLAGS(tmp_win)), &(pstyle->flags.common),
         sizeof(common_flags_type));
  tmp_win->wShaped = None;
  if (FShapesSupported)
  {
    int i;
    unsigned int u;
    Bool b;
    int boundingShaped;

    /* suppress compiler warnings w/o shape extension */
    i = 0;
    u = 0;
    b = False;

    FShapeSelectInput(dpy, tmp_win->w, FShapeNotifyMask);
    if (FShapeQueryExtents(
      dpy, tmp_win->w, &boundingShaped, &i, &i, &u, &u, &b, &i, &i, &u, &u))
    {
      tmp_win->wShaped = boundingShaped;
    }
  }

  /*  Assume that we'll decorate */
  SET_HAS_BORDER(tmp_win, 1);

  SET_HAS_TITLE(tmp_win, 1);

#ifdef USEDECOR
  /* search for a UseDecor tag in the style */
  if (!IS_DECOR_CHANGED(tmp_win))
  {
    FvwmDecor *decor = &Scr.DefaultDecor;

    for (; decor; decor = decor->next)
    {
      if (StrEquals(SGET_DECOR_NAME(*pstyle), decor->tag))
      {
        tmp_win->decor = decor;
        break;
      }
    }
  }
  if (tmp_win->decor == NULL)
  {
    tmp_win->decor = &Scr.DefaultDecor;
  }
#endif

  GetMwmHints(tmp_win);
  GetOlHints(tmp_win);

  tmp_win->buttons = SIS_BUTTON_DISABLED(&pstyle->flags);
  SelectDecor(tmp_win, pstyle, buttons);

  if (IS_TRANSIENT(tmp_win) && !pstyle->flags.do_decorate_transient)
  {
      SET_HAS_BORDER(tmp_win, 0);
      SET_HAS_TITLE(tmp_win, 0);
  }
  /* set boundary width to zero for shaped windows */
  if (FHaveShapeExtension)
  {
    if (tmp_win->wShaped)
      tmp_win->boundary_width = 0;
  }

  /****** window colors ******/
  update_window_color_style(tmp_win, pstyle);
  update_window_color_hi_style(tmp_win, pstyle);

  /****** window shading ******/
  tmp_win->shade_anim_steps = pstyle->shade_anim_steps;

  /****** GNOME style hints ******/
  if (!SDO_IGNORE_GNOME_HINTS(pstyle->flags))
  {
    GNOME_GetStyle(tmp_win, pstyle);
  }

  return;
}

void setup_icon_boxes(FvwmWindow *tmp_win, window_style *pstyle)
{
  icon_boxes *ib;

  /* copy iconboxes ptr (if any) */
  if (SHAS_ICON_BOXES(&pstyle->flags))
  {
    tmp_win->IconBoxes = SGET_ICON_BOXES(*pstyle);
    for (ib = tmp_win->IconBoxes; ib; ib = ib->next)
    {
      ib->use_count++;
    }
  }
  else
  {
    tmp_win->IconBoxes = NULL;
  }
}

void destroy_icon_boxes(FvwmWindow *tmp_win)
{
  if (tmp_win->IconBoxes)
  {
    tmp_win->IconBoxes->use_count--;
    if (tmp_win->IconBoxes->use_count == 0 && tmp_win->IconBoxes->is_orphan)
    {
      /* finally destroy the icon box */
      free_icon_boxes(tmp_win->IconBoxes);
      tmp_win->IconBoxes = NULL;
    }
  }
}

void change_icon_boxes(FvwmWindow *tmp_win, window_style *pstyle)
{
  destroy_icon_boxes(tmp_win);
  setup_icon_boxes(tmp_win, pstyle);
}

void setup_layer(FvwmWindow *tmp_win, window_style *pstyle)
{
  FvwmWindow *tf;
  int layer;

  if (SUSE_LAYER(&pstyle->flags))
  {
    /* use layer from style */
    layer = SGET_LAYER(*pstyle);
  }
  else if ((tf = get_transientfor_fvwmwindow(tmp_win)) != NULL)
  {
    /* inherit layer from transientfor window */
    layer = get_layer(tf);
  }
  else
  {
    /* use default layer */
    layer = Scr.DefaultLayer;
  }
  set_default_layer(tmp_win, layer);
  set_layer(tmp_win, layer);

  return;
}

void setup_frame_size_limits(FvwmWindow *tmp_win, window_style *pstyle)
{
  if (SHAS_MAX_WINDOW_SIZE(&pstyle->flags))
  {
    tmp_win->max_window_width = SGET_MAX_WINDOW_WIDTH(*pstyle);
    tmp_win->max_window_height = SGET_MAX_WINDOW_HEIGHT(*pstyle);
  }
  else
  {
    tmp_win->max_window_width = DEFAULT_MAX_MAX_WINDOW_WIDTH;
    tmp_win->max_window_height = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
  }
}

Bool setup_window_placement(FvwmWindow *tmp_win, window_style *pstyle)
{
  int client_argc = 0;
  char **client_argv = NULL;
  XrmValue rm_value;
  int tmpno1 = -1, tmpno2 = -1, tmpno3 = -1, spargs = 0;
  /* Used to parse command line of clients for specific desk requests. */
  /* Todo: check for multiple desks. */
  XrmDatabase db = NULL;
  static XrmOptionDescRec table [] = {
    /* Want to accept "-workspace N" or -xrm "fvwm*desk:N" as options
     * to specify the desktop. I have to include dummy options that
     * are meaningless since Xrm seems to allow -w to match -workspace
     * if there would be no ambiguity. */
    {"-workspacf",      "*junk",        XrmoptionSepArg, (caddr_t) NULL},
    {"-workspace",	"*desk",	XrmoptionSepArg, (caddr_t) NULL},
    {"-xrn",		NULL,		XrmoptionResArg, (caddr_t) NULL},
    {"-xrm",		NULL,		XrmoptionResArg, (caddr_t) NULL},
  };

  /* Find out if the client requested a specific desk on the command line. */
  /*  RBW - 11/20/1998 - allow a desk of -1 to work.  */

  if (XGetCommand(dpy, tmp_win->w, &client_argv, &client_argc) &&
      client_argc > 0 && client_argv != NULL)
  {
    /* Get global X resources */
    MergeXResources(dpy, &db, False);

    /* command line takes precedence over all */
    MergeCmdLineResources(
      &db, table, 4, client_argv[0], &client_argc, client_argv, True);

    /* Now parse the database values: */
    if (GetResourceString(db, "desk", client_argv[0], &rm_value) &&
	rm_value.size != 0)
    {
      SGET_START_DESK(*pstyle) = atoi(rm_value.addr);
      /*  RBW - 11/20/1998  */
      if (SGET_START_DESK(*pstyle) > -1)
      {
	SSET_START_DESK(*pstyle, SGET_START_DESK(*pstyle) + 1);
      }
      pstyle->flags.use_start_on_desk = 1;
    }
    if (GetResourceString(db, "fvwmscreen", client_argv[0], &rm_value) &&
	rm_value.size != 0)
    {
      SSET_START_SCREEN(*pstyle, FScreenGetScreenArgument(rm_value.addr, 'c'));
      pstyle->flags.use_start_on_screen = 1;
    }
    if (GetResourceString(db, "page", client_argv[0], &rm_value) &&
	rm_value.size != 0)
    {
      spargs = sscanf (rm_value.addr, "%d %d %d", &tmpno1, &tmpno2, &tmpno3);
      switch (spargs)
      {
      case 1:
	pstyle->flags.use_start_on_desk = 1;
	SSET_START_DESK(*pstyle, (tmpno1 > -1) ? tmpno1 + 1 : tmpno1);
	break;
      case 2:
	pstyle->flags.use_start_on_desk = 1;
	SSET_START_PAGE_X(*pstyle, (tmpno1 > -1) ? tmpno1 + 1 : tmpno1);
	SSET_START_PAGE_Y(*pstyle, (tmpno2 > -1) ? tmpno2 + 1 : tmpno2);
	break;
      case 3:
	pstyle->flags.use_start_on_desk = 1;
	SSET_START_DESK(*pstyle, (tmpno1 > -1) ? tmpno1 + 1 : tmpno1);
	SSET_START_PAGE_X(*pstyle, (tmpno2 > -1) ? tmpno2 + 1 : tmpno2);
	SSET_START_PAGE_Y(*pstyle, (tmpno3 > -1) ? tmpno3 + 1 : tmpno3);
	break;
      default:
	break;
      }
    }

    XFreeStringList(client_argv);
    XrmDestroyDatabase(db);
  }

  return PlaceWindow(
      tmp_win, &pstyle->flags, SGET_START_DESK(*pstyle),
      SGET_START_PAGE_X(*pstyle), SGET_START_PAGE_Y(*pstyle),
      SGET_START_SCREEN(*pstyle), PLACE_INITIAL);
}

void setup_placement_penalty(FvwmWindow *tmp_win, window_style *pstyle)
{
  int i;

  if (!SHAS_PLACEMENT_PENALTY(&pstyle->flags))
  {
    SSET_NORMAL_PLACEMENT_PENALTY(*pstyle, 1);
    SSET_ONTOP_PLACEMENT_PENALTY(*pstyle, PLACEMENT_AVOID_ONTOP);
    SSET_ICON_PLACEMENT_PENALTY(*pstyle, PLACEMENT_AVOID_ICON);
    SSET_STICKY_PLACEMENT_PENALTY(*pstyle, PLACEMENT_AVOID_STICKY);
    SSET_BELOW_PLACEMENT_PENALTY(*pstyle, PLACEMENT_AVOID_BELOW);
    SSET_EWMH_STRUT_PLACEMENT_PENALTY(*pstyle, PLACEMENT_AVOID_EWMH_STRUT);
  }
  if (!SHAS_PLACEMENT_PERCENTAGE_PENALTY(&pstyle->flags))
  {
    SSET_99_PLACEMENT_PERCENTAGE_PENALTY(*pstyle, PLACEMENT_AVOID_COVER_99);
    SSET_95_PLACEMENT_PERCENTAGE_PENALTY(*pstyle, PLACEMENT_AVOID_COVER_95);
    SSET_85_PLACEMENT_PERCENTAGE_PENALTY(*pstyle, PLACEMENT_AVOID_COVER_85);
    SSET_75_PLACEMENT_PERCENTAGE_PENALTY(*pstyle, PLACEMENT_AVOID_COVER_75);
  }
  for(i = 0; i < 6; i++)
  {
    tmp_win->placement_penalty[i] = (*pstyle).placement_penalty[i];
  }
  for(i = 0; i < 4; i++)
  {
    tmp_win->placement_percentage_penalty[i] =
      (*pstyle).placement_percentage_penalty[i];
  }
}

void get_default_window_attributes(
  FvwmWindow *tmp_win, unsigned long *pvaluemask,
  XSetWindowAttributes *pattributes)
{
  if(Pdepth < 2)
  {
    *pvaluemask = CWBackPixmap;
    if(IS_STICKY(tmp_win))
      pattributes->background_pixmap = Scr.sticky_gray_pixmap;
    else
      pattributes->background_pixmap = Scr.light_gray_pixmap;
  }
  else
  {
    *pvaluemask = CWBackPixel;
    pattributes->background_pixel = tmp_win->colors.back;
    pattributes->background_pixmap = None;
  }

  *pvaluemask |= CWBackingStore | CWCursor | CWSaveUnder;
  pattributes->backing_store = NotUseful;
  pattributes->cursor = Scr.FvwmCursors[CRS_DEFAULT];
  pattributes->save_under = False;
}

void setup_frame_window(
  FvwmWindow *tmp_win)
{
  XSetWindowAttributes attributes;
  int valuemask = CWBackingStore | CWBackPixmap | CWEventMask | CWSaveUnder;

  attributes.backing_store = NotUseful;
  attributes.background_pixmap = None;
  attributes.event_mask = XEVMASK_FRAMEW;
  attributes.save_under = False;

  /* create the frame window, child of root, grandparent of client */
  tmp_win->frame = XCreateWindow(
      dpy, Scr.Root, tmp_win->frame_g.x, tmp_win->frame_g.y,
      tmp_win->frame_g.width, tmp_win->frame_g.height, 0, CopyFromParent,
      InputOutput, CopyFromParent, valuemask, &attributes);
  XSaveContext(dpy, tmp_win->w, FvwmContext, (caddr_t) tmp_win);
  XSaveContext(dpy, tmp_win->frame, FvwmContext, (caddr_t) tmp_win);
}

void setup_decor_window(
  FvwmWindow *tmp_win, int valuemask, XSetWindowAttributes *pattributes)
{
  /* stash attribute bits in case BorderStyle TiledPixmap overwrites */
  Pixmap TexturePixmap, TexturePixmapSave = pattributes->background_pixmap;

  valuemask |= CWBorderPixel | CWColormap | CWEventMask;

  pattributes->border_pixel = 0;
  pattributes->colormap = Pcmap;
  pattributes->event_mask = XEVMASK_DECORW;

  if (DFS_FACE_TYPE(GetDecor(tmp_win, BorderStyle.inactive.style)) ==
      TiledPixmapButton)
  {
    TexturePixmap = GetDecor(tmp_win,BorderStyle.inactive.u.p->picture);
    if (TexturePixmap)
    {
      pattributes->background_pixmap = TexturePixmap;
      valuemask = (valuemask & ~CWBackPixel) | CWBackPixmap;
    }
  }

  /* decor window, parent of all decorative subwindows */
  tmp_win->decor_w =
    XCreateWindow(
      dpy, tmp_win->frame, 0, 0, tmp_win->frame_g.width,
      tmp_win->frame_g.height, 0, Pdepth, InputOutput, Pvisual, valuemask,
      pattributes);
  XSaveContext(dpy, tmp_win->decor_w, FvwmContext, (caddr_t) tmp_win);

  /* restore background */
  pattributes->background_pixmap = TexturePixmapSave;
}

void setup_title_window(
  FvwmWindow *tmp_win, int valuemask, XSetWindowAttributes *pattributes)
{
  valuemask |= CWCursor | CWEventMask;
  pattributes->cursor = Scr.FvwmCursors[CRS_TITLE];
  pattributes->event_mask = XEVMASK_TITLEW;

  /* set up the title geometry - the height is already known from the decor */
  tmp_win->title_g.width = tmp_win->frame_g.width - 2*tmp_win->boundary_width;
  if(tmp_win->title_g.width < 1)
    tmp_win->title_g.width = 1;
  tmp_win->title_g.x = tmp_win->boundary_width + tmp_win->title_g.height + 1;
  tmp_win->title_g.y = tmp_win->boundary_width;

  tmp_win->title_w =
    XCreateWindow(
      dpy, tmp_win->decor_w, tmp_win->title_g.x, tmp_win->title_g.y,
      tmp_win->title_g.width, tmp_win->title_g.height, 0, CopyFromParent,
      InputOutput, CopyFromParent, valuemask, pattributes);
  XSaveContext(dpy, tmp_win->title_w, FvwmContext, (caddr_t) tmp_win);
}

void destroy_title_window(FvwmWindow *tmp_win, Bool do_only_delete_context)
{
  if (!do_only_delete_context)
  {
    XDestroyWindow(dpy, tmp_win->title_w);
    tmp_win->title_w = None;
  }
  XDeleteContext(dpy, tmp_win->title_w, FvwmContext);
  tmp_win->title_w = None;
}

void change_title_window(
  FvwmWindow *tmp_win, int valuemask, XSetWindowAttributes *pattributes)
{
  if (HAS_TITLE(tmp_win) && tmp_win->title_w == None)
    setup_title_window(tmp_win, valuemask, pattributes);
  else if (!HAS_TITLE(tmp_win) && tmp_win->title_w != None)
    destroy_title_window(tmp_win, False);
}

void setup_button_windows(
  FvwmWindow *tmp_win, int valuemask, XSetWindowAttributes *pattributes,
  short buttons)
{
  int i;
  Bool has_button;

  valuemask |= CWCursor | CWEventMask;
  pattributes->cursor = Scr.FvwmCursors[CRS_SYS];
  pattributes->event_mask = XEVMASK_BUTTONW;

  for (i = 0; i < NUMBER_OF_BUTTONS; i++)
  {
    has_button = (((!(i & 1) && i / 2 < Scr.nr_left_buttons) ||
		   ( (i & 1) && i / 2 < Scr.nr_right_buttons)) &&
		  (buttons & (1 << i)));
    if (tmp_win->button_w[i] == None && has_button)
    {
      tmp_win->button_w[i] =
	XCreateWindow(
	  dpy, tmp_win->decor_w, (i < NR_LEFT_BUTTONS) ?
	  (tmp_win->title_g.height * i) :
	  (tmp_win->title_g.width - tmp_win->title_g.height * (i+1)),
	  0, tmp_win->title_g.height, tmp_win->title_g.height, 0,
	  CopyFromParent, InputOutput, CopyFromParent, valuemask, pattributes);
      XSaveContext(dpy, tmp_win->button_w[i], FvwmContext, (caddr_t) tmp_win);
    }
    else if (tmp_win->button_w[i] != None && !has_button)
    {
      /* destroy the current button window */
      XDestroyWindow(dpy, tmp_win->button_w[i]);
      XDeleteContext(dpy, tmp_win->button_w[i], FvwmContext);
      tmp_win->button_w[i] = None;
    }
  }
}

void destroy_button_windows(FvwmWindow *tmp_win, Bool do_only_delete_context)
{
  int i;

  for(i = 0; i < NUMBER_OF_BUTTONS; i++)
  {
    if(tmp_win->button_w[i] != None)
    {
      if (!do_only_delete_context)
      {
	XDestroyWindow(dpy, tmp_win->button_w[i]);
	tmp_win->button_w[i] = None;
      }
      XDeleteContext(dpy, tmp_win->button_w[i], FvwmContext);
      tmp_win->button_w[i] = None;
    }
  }
}

void change_button_windows(
  FvwmWindow *tmp_win, int valuemask, XSetWindowAttributes *pattributes,
  short buttons)
{
  if (HAS_TITLE(tmp_win))
    setup_button_windows(
      tmp_win, valuemask, pattributes, buttons);
  else
    destroy_button_windows(tmp_win, False);
}


void setup_parent_window(FvwmWindow *tmp_win)
{
  XSetWindowAttributes attributes;
  int valuemask = CWBackingStore | CWBackPixmap | CWCursor | CWEventMask
  		  | CWSaveUnder;

  attributes.backing_store = NotUseful;
  attributes.background_pixmap = None;
  attributes.cursor = Scr.FvwmCursors[CRS_DEFAULT];
  attributes.event_mask = XEVMASK_PARENTW;
  attributes.save_under = False;

  /* This window is exactly the same size as the client for the
     benefit of some clients */
  tmp_win->Parent = XCreateWindow(
    dpy, tmp_win->frame, tmp_win->boundary_width,
    tmp_win->boundary_width + tmp_win->title_g.height,
    (tmp_win->frame_g.width - 2 * tmp_win->boundary_width),
    (tmp_win->frame_g.height - 2 * tmp_win->boundary_width -
     tmp_win->title_g.height),
    0, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);

  XSaveContext(dpy, tmp_win->Parent, FvwmContext, (caddr_t) tmp_win);
}

void setup_resize_handle_windows(FvwmWindow *tmp_win)
{
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  int i;

  /* sides and corners are input only */
  /* title and buttons maybe one day */
  valuemask = CWCursor | CWEventMask;
  attributes.event_mask = XEVMASK_BORDERW;
  if (HAS_BORDER(tmp_win))
  {
    /* Just dump the windows any old place and let SetupFrame take
     * care of the mess */
    for(i = 0; i < 4; i++)
    {
      attributes.cursor = Scr.FvwmCursors[CRS_TOP_LEFT+i];
      tmp_win->corners[i] =
	XCreateWindow(
	  dpy, tmp_win->decor_w, 0, 0, tmp_win->corner_width,
	  tmp_win->corner_width, 0, 0, InputOnly,
	  DefaultVisual(dpy, Scr.screen), valuemask, &attributes);
      attributes.cursor = Scr.FvwmCursors[CRS_TOP+i];
      tmp_win->sides[i] =
	XCreateWindow(
	  dpy, tmp_win->decor_w, 0, 0, tmp_win->boundary_width,
	  tmp_win->boundary_width, 0, 0, InputOnly,
	  DefaultVisual(dpy, Scr.screen), valuemask, &attributes);
      XSaveContext(dpy, tmp_win->sides[i], FvwmContext, (caddr_t) tmp_win);
      XSaveContext(dpy, tmp_win->corners[i], FvwmContext, (caddr_t) tmp_win);
    }
  }
}

void destroy_resize_handle_windows(
  FvwmWindow *tmp_win, Bool do_only_delete_context)
{
  int i;

  for(i = 0; i < 4 ; i++)
  {
    if (!do_only_delete_context)
    {
      XDestroyWindow(dpy, tmp_win->sides[i]);
      XDestroyWindow(dpy, tmp_win->corners[i]);
      tmp_win->sides[i] = None;
      tmp_win->corners[i] = None;
    }
    XDeleteContext(dpy, tmp_win->sides[i], FvwmContext);
    XDeleteContext(dpy, tmp_win->corners[i], FvwmContext);
  }
}

void change_resize_handle_windows(FvwmWindow *tmp_win)
{
  if (HAS_BORDER(tmp_win) && tmp_win->sides[0] == None)
    setup_resize_handle_windows(tmp_win);
  else if (!HAS_BORDER(tmp_win) && tmp_win->sides[0] != None)
    destroy_resize_handle_windows(tmp_win, False);
}

void setup_frame_stacking(FvwmWindow *tmp_win)
{
  int i;

  XMapSubwindows(dpy, tmp_win->decor_w);
  XLowerWindow(dpy, tmp_win->decor_w);
  /* Must lower all four corners since they may overlap the (sibling) title */
  for (i = 0; i < 4; i++)
  {
    if (tmp_win->corners[i] != None)
      XLowerWindow(dpy, tmp_win->corners[i]);
  }
}

void setup_auxiliary_windows(
  FvwmWindow *tmp_win, Bool setup_frame_and_parent, short buttons)
{
  unsigned long valuemask_save = 0;
  XSetWindowAttributes attributes;

  get_default_window_attributes(tmp_win, &valuemask_save, &attributes);

  /****** frame window ******/
  if (setup_frame_and_parent)
  {
    setup_frame_window(tmp_win);
    setup_decor_window(tmp_win, valuemask_save, &attributes);
    setup_parent_window(tmp_win);
  }

  /****** resize handle windows ******/
  setup_resize_handle_windows(tmp_win);

  /****** title window ******/
  if (HAS_TITLE(tmp_win))
  {
    setup_title_window(tmp_win, valuemask_save, &attributes);
    setup_button_windows(tmp_win, valuemask_save, &attributes, buttons);
  }

  /****** setup frame stacking order ******/
  setup_frame_stacking(tmp_win);
  XMapSubwindows (dpy, tmp_win->frame);
}

void setup_frame_attributes(
  FvwmWindow *tmp_win, window_style *pstyle)
{
  int i;
  XSetWindowAttributes xswa;

  /* Backing_store is controlled on the client, decor_w, title & buttons */
  switch (pstyle->flags.use_backing_store)
  {
  case BACKINGSTORE_DEFAULT:
	  xswa.backing_store = tmp_win->initial_backing_store;
	  break;
  case BACKINGSTORE_ON:
	  xswa.backing_store = Scr.use_backing_store;
	  break;
  case BACKINGSTORE_OFF:
  default:
	  xswa.backing_store = NotUseful;
	  break;
  }
  XChangeWindowAttributes(dpy, tmp_win->w, CWBackingStore, &xswa);
  if (pstyle->flags.use_backing_store == BACKINGSTORE_OFF)
  {
    xswa.backing_store = NotUseful;
  }
  XChangeWindowAttributes(dpy, tmp_win->decor_w, CWBackingStore, &xswa);
  if (HAS_TITLE(tmp_win))
  {
    XChangeWindowAttributes(dpy, tmp_win->title_w, CWBackingStore, &xswa);
    for (i = 0; i < NUMBER_OF_BUTTONS; i++)
    {
      if (tmp_win->button_w[i])
	XChangeWindowAttributes(dpy, tmp_win->button_w[i], CWBackingStore,
				&xswa);
    }
  }

  /* parent_relative is applied to the frame and the parent */
  xswa.background_pixmap = pstyle->flags.use_parent_relative
			   ? ParentRelative : None;
  XChangeWindowAttributes(dpy, tmp_win->frame, CWBackPixmap, &xswa);
  XChangeWindowAttributes(dpy, tmp_win->Parent, CWBackPixmap, &xswa);

  /* Save_under is only useful on the frame */
  xswa.save_under = pstyle->flags.do_save_under
		    ? Scr.flags.do_save_under : NotUseful;
  XChangeWindowAttributes(dpy, tmp_win->frame, CWSaveUnder, &xswa);
}

void destroy_auxiliary_windows(FvwmWindow *tmp_win,
			       Bool destroy_frame_and_parent)
{
  if (destroy_frame_and_parent)
  {
    XDestroyWindow(dpy, tmp_win->frame);
    XDeleteContext(dpy, tmp_win->frame, FvwmContext);
    XDeleteContext(dpy, tmp_win->decor_w, FvwmContext);
    XDeleteContext(dpy, tmp_win->Parent, FvwmContext);
    XDeleteContext(dpy, tmp_win->w, FvwmContext);
  }

  if (HAS_TITLE(tmp_win))
    destroy_title_window(tmp_win, True);
  if (HAS_TITLE(tmp_win))
    destroy_button_windows(tmp_win, True);
  if (HAS_BORDER(tmp_win))
    destroy_resize_handle_windows(tmp_win, True);
}

void change_auxiliary_windows(FvwmWindow *tmp_win, short buttons)
{
  unsigned long valuemask_save = 0;
  XSetWindowAttributes attributes;

  get_default_window_attributes(tmp_win, &valuemask_save, &attributes);
  change_title_window(tmp_win, valuemask_save, &attributes);
  change_button_windows(tmp_win, valuemask_save, &attributes, buttons);
  change_resize_handle_windows(tmp_win);
  setup_frame_stacking(tmp_win);
}

void increase_icon_hint_count(FvwmWindow *tmp_win)
{
  if (tmp_win->wmhints &&
      (tmp_win->wmhints->flags & (IconWindowHint | IconPixmapHint)))
  {
    switch (WAS_ICON_HINT_PROVIDED(tmp_win))
    {
    case ICON_HINT_NEVER:
      SET_WAS_ICON_HINT_PROVIDED(tmp_win, ICON_HINT_ONCE);
      break;
    case ICON_HINT_ONCE:
      SET_WAS_ICON_HINT_PROVIDED(tmp_win, ICON_HINT_MULTIPLE);
      break;
    case ICON_HINT_MULTIPLE:
    default:
      break;
    }
ICON_DBG((stderr,"icon hint count++ (%d) '%s'\n", (int)WAS_ICON_HINT_PROVIDED(tmp_win), tmp_win->name));
  }

  return;
}

static void setup_icon(FvwmWindow *tmp_win, window_style *pstyle)
{
  increase_icon_hint_count(tmp_win);
  /* find a suitable icon pixmap */
  if((tmp_win->wmhints) && (tmp_win->wmhints->flags & IconWindowHint))
  {
    if (SHAS_ICON(&pstyle->flags) &&
	SICON_OVERRIDE(pstyle->flags) == ICON_OVERRIDE)
    {
ICON_DBG((stderr,"si: iwh ignored '%s'\n", tmp_win->name));
      tmp_win->icon_bitmap_file = SGET_ICON_NAME(*pstyle);
    }
    else
    {
ICON_DBG((stderr,"si: using iwh '%s'\n", tmp_win->name));
      tmp_win->icon_bitmap_file = NULL;
    }
  }
  else if((tmp_win->wmhints) && (tmp_win->wmhints->flags & IconPixmapHint))
  {
    if (SHAS_ICON(&pstyle->flags) &&
	SICON_OVERRIDE(pstyle->flags) != NO_ICON_OVERRIDE)
    {
ICON_DBG((stderr,"si: iph ignored '%s'\n", tmp_win->name));
      tmp_win->icon_bitmap_file = SGET_ICON_NAME(*pstyle);
    }
    else
    {
ICON_DBG((stderr,"si: using iph '%s'\n", tmp_win->name));
      tmp_win->icon_bitmap_file = NULL;
    }
  }
  else if(SHAS_ICON(&pstyle->flags))
  {
    /* an icon was specified */
ICON_DBG((stderr,"si: using style '%s'\n", tmp_win->name));
    tmp_win->icon_bitmap_file = SGET_ICON_NAME(*pstyle);
  }
  else
  {
    /* use default icon */
ICON_DBG((stderr,"si: using default '%s'\n", tmp_win->name));
    tmp_win->icon_bitmap_file = Scr.DefaultIcon;
  }

  /* icon name */
  if (!EWMH_WMIconName(tmp_win, NULL, NULL, 0))
  {
    tmp_win->icon_name = NoName;
    MULTIBYTE_CODE(tmp_win->icon_name_list = NULL);
    FlocaleGetNameProperty(XGetWMIconName, dpy, tmp_win->w,
			   MULTIBYTE_ARG(&(tmp_win->icon_name_list))
			   &(tmp_win->icon_name));
  }
  if (tmp_win->icon_name == NoName)
  {
    tmp_win->icon_name = tmp_win->name;
    SET_WAS_ICON_NAME_PROVIDED(tmp_win, 0);
  }
  setup_visible_name(tmp_win, True);


  /* wait until the window is iconified and the icon window is mapped
   * before creating the icon window
   */
  tmp_win->icon_title_w = None;

  EWMH_SetVisibleName(tmp_win, True);
  BroadcastWindowIconNames(tmp_win, False, True);
  if (tmp_win->icon_bitmap_file != NULL &&
      tmp_win->icon_bitmap_file != Scr.DefaultIcon)
    BroadcastName(M_ICON_FILE,tmp_win->w,tmp_win->frame,
		  (unsigned long)tmp_win,tmp_win->icon_bitmap_file);
}

static void destroy_icon(FvwmWindow *tmp_win)
{
  free_window_names(tmp_win, False, True);
  if (tmp_win->icon_title_w)
  {
    if (IS_PIXMAP_OURS(tmp_win))
    {
      XFreePixmap(dpy, tmp_win->iconPixmap);
      if (tmp_win->icon_maskPixmap != None)
      {
	XFreePixmap(dpy, tmp_win->icon_maskPixmap);
      }
    }
    XDestroyWindow(dpy, tmp_win->icon_title_w);
    XDeleteContext(dpy, tmp_win->icon_title_w, FvwmContext);
  }
  if((IS_ICON_OURS(tmp_win))&&(tmp_win->icon_pixmap_w != None))
    XDestroyWindow(dpy, tmp_win->icon_pixmap_w);
  if(tmp_win->icon_pixmap_w != None)
  {
    XDeleteContext(dpy, tmp_win->icon_pixmap_w, FvwmContext);
  }
  clear_icon(tmp_win);
}

void change_icon(FvwmWindow *tmp_win, window_style *pstyle)
{
  destroy_icon(tmp_win);
  setup_icon(tmp_win, pstyle);
}

#ifdef MINI_ICONS
void setup_mini_icon(FvwmWindow *tmp_win, window_style *pstyle)
{
  if (SHAS_MINI_ICON(&pstyle->flags))
  {
    tmp_win->mini_pixmap_file = SGET_MINI_ICON_NAME(*pstyle);
  }
  else
  {
    tmp_win->mini_pixmap_file = NULL;
  }

  if (tmp_win->mini_pixmap_file)
  {
    tmp_win->mini_icon = CachePicture(dpy, Scr.NoFocusWin, NULL,
				      tmp_win->mini_pixmap_file,
				      Scr.ColorLimit);
    if (tmp_win->mini_icon != NULL)
    {
      BroadcastMiniIcon(M_MINI_ICON,
			tmp_win->w, tmp_win->frame, (unsigned long)tmp_win,
			tmp_win->mini_icon->width,
			tmp_win->mini_icon->height,
			tmp_win->mini_icon->depth,
			tmp_win->mini_icon->picture,
			tmp_win->mini_icon->mask,
			tmp_win->mini_pixmap_file);
    }
  }
  else
  {
    tmp_win->mini_icon = NULL;
  }

}

void destroy_mini_icon(FvwmWindow *tmp_win)
{
  if (tmp_win->mini_icon)
  {
    DestroyPicture(dpy, tmp_win->mini_icon);
    tmp_win->mini_icon = 0;
  }
}

void change_mini_icon(FvwmWindow *tmp_win, window_style *pstyle)
{
  Picture *old_mi = tmp_win->mini_icon;
  destroy_mini_icon(tmp_win);
  setup_mini_icon(tmp_win, pstyle);
  if (old_mi != NULL && tmp_win->mini_icon == 0)
  {
    /* this case is not handled in setup_mini_icon, so we must broadcast here
     * explicitly */
    BroadcastMiniIcon(M_MINI_ICON,
		      tmp_win->w, tmp_win->frame, (unsigned long)tmp_win,
			0, 0, 0, 0, 0, "");
  }
}
#endif

void setup_focus_policy(FvwmWindow *tmp_win)
{
  focus_grab_buttons(tmp_win, False);
}


void setup_key_and_button_grabs(FvwmWindow *tmp_win)
{
#ifdef BUGS_ARE_COOL
  /* dv (29-May-2001): If keys are grabbed separately for C_WINDOW and the other
   * contexts, new windows have problems wen bindings are removed.  Therefore,
   * grab all keys in a single pass through the list. */
  GrabAllWindowKeysAndButtons(dpy, tmp_win->Parent, Scr.AllBindings,
			      C_WINDOW, GetUnusedModifiers(), None, True);
  GrabAllWindowKeys(dpy, tmp_win->frame, Scr.AllBindings,
		    C_TITLE|C_RALL|C_LALL|C_SIDEBAR,
		    GetUnusedModifiers(), True);
#endif
  GrabAllWindowButtons(dpy, tmp_win->Parent, Scr.AllBindings,
		       C_WINDOW, GetUnusedModifiers(), None, True);
  GrabAllWindowKeys(dpy, tmp_win->frame, Scr.AllBindings,
		    C_TITLE|C_RALL|C_LALL|C_SIDEBAR|C_WINDOW,
		    GetUnusedModifiers(), True);
  setup_focus_policy(tmp_win);
  /* Special handling for MouseFocusClickRaises *only*. If a
   * MouseFocusClickRaises window was so raised, it is now ungrabbed. If it's
   * no longer on top of its layer because of the new window we just added, we
   * have to restore the grab so it can be raised again. */
  if (Scr.Ungrabbed && DO_RAISE_MOUSE_FOCUS_CLICK(Scr.Ungrabbed)
      && (HAS_SLOPPY_FOCUS(Scr.Ungrabbed) || HAS_MOUSE_FOCUS(Scr.Ungrabbed))
      && !is_on_top_of_layer(Scr.Ungrabbed))
  {
    focus_grab_buttons(Scr.Ungrabbed, False);
  }
}


/***********************************************************************
 *
 *  Procedure:
 *	AddWindow - add a new window to the fvwm list
 *
 ***********************************************************************/
FvwmWindow *AddWindow(Window w, FvwmWindow *ReuseWin, Bool is_menu)
{
  /* new fvwm window structure */
  register FvwmWindow *tmp_win = NULL;
  FvwmWindow *tmptmp_win = NULL;
  /* mask for create windows */
  unsigned long valuemask;
  /* attributes for create windows */
  XSetWindowAttributes attributes;
  /* area for merged styles */
  window_style style;
  /* used for faster access */
  style_flags *sflags;
  short buttons;
  extern FvwmWindow *colormap_win;
  extern Bool PPosOverride;
  int do_shade = 0;
  int do_maximize = 0;
  Bool used_sm = False;
  Bool do_resize_too = False;

  /****** init window structure ******/
  if (!setup_window_structure(&tmptmp_win, w, ReuseWin)) {
    fvwm_msg(ERR,"AddWindow","Bad return code from setup_window_structure");
    return NULL;
  }
  tmp_win = tmptmp_win;

  /****** safety check, window might disappear before we get to it ******/
  if(!PPosOverride &&
     XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
                  &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
  {
    free((char *)tmp_win);
    return NULL;
  }

  /****** Make sure the client window still exists.  We don't want to leave an
   * orphan frame window if it doesn't.  Since we now have the server
   * grabbed, the window can't disappear later without having been
   * reparented, so we'll get a DestroyNotify for it.  We won't have
   * gotten one for anything up to here, however. ******/
  MyXGrabServer(dpy);
  if (XGetGeometry(dpy, w, &JunkRoot, &JunkX, &JunkY,
                   &JunkWidth, &JunkHeight,
                   &JunkBW,  &JunkDepth) == 0)
  {
    free((char *)tmp_win);
    MyXUngrabServer(dpy);
    return NULL;
  }

  /****** window name ******/
  setup_window_name(tmp_win);
  setup_class_and_resource(tmp_win);
  setup_wm_hints(tmp_win);

  /* save a copy of the backing store attribute */
  tmp_win->initial_backing_store = tmp_win->attr.backing_store;

  /****** basic style and decor ******/
  /* If the window is in the NoTitle list, or is a transient, dont decorate it.
   * If its a transient, and DecorateTransients was specified, decorate anyway.
   */
  /* get merged styles */
  lookup_style(tmp_win, &style);
  sflags = SGET_FLAGS_POINTER(style);
  SET_TRANSIENT(
    tmp_win, !!XGetTransientForHint(dpy, tmp_win->w, &tmp_win->transientfor));
  if (is_menu)
  {
    SET_TEAR_OFF_MENU(tmp_win, 1);
  }
  tmp_win->decor = NULL;
  setup_style_and_decor(tmp_win, &style, &buttons);

  /****** fonts ******/
  setup_window_font(tmp_win, &style, False);
  setup_icon_font(tmp_win, &style, False);

  /***** visible window name ****/
  setup_visible_name(tmp_win, False);
  EWMH_SetVisibleName(tmp_win, False);

  /****** state setup ******/
  setup_icon_boxes(tmp_win, &style);
  SET_ICONIFIED(tmp_win, 0);
  SET_ICON_UNMAPPED(tmp_win, 0);
  SET_MAXIMIZED(tmp_win, 0);
  /*
   * Reparenting generates an UnmapNotify event, followed by a MapNotify.
   * Set the map state to FALSE to prevent a transition back to
   * WithdrawnState in HandleUnmapNotify.  Map state gets set corrected
   * again in HandleMapNotify.
   */
  SET_MAPPED(tmp_win, 0);

  /****** window list and stack ring ******/
  /* add the window to the end of the fvwm list */
  tmp_win->next = Scr.FvwmRoot.next;
  tmp_win->prev = &Scr.FvwmRoot;
  while (tmp_win->next != NULL)
  {
    tmp_win->prev = tmp_win->next;
    tmp_win->next = tmp_win->next->next;
  }
  /* tmp_win->prev points to the last window in the list, tmp_win->next is
   * NULL. Now fix the last window to point to tmp_win */
  tmp_win->prev->next = tmp_win;
  /*
   * RBW - 11/13/1998 - add it into the stacking order chain also.
   * This chain is anchored at both ends on Scr.FvwmRoot, there are
   * no null pointers.
   */
  add_window_to_stack_ring_after(tmp_win, &Scr.FvwmRoot);

  /****** calculate frame size ******/
  tmp_win->hints.win_gravity = NorthWestGravity;
  GetWindowSizeHints(tmp_win);

  /****** border width ******/
  tmp_win->old_bw = tmp_win->attr.border_width;
  XSetWindowBorderWidth(dpy, tmp_win->w, 0);

  /***** placement penalities *****/
  setup_placement_penalty(tmp_win, &style);
  /*
   * MatchWinToSM changes tmp_win->attr and the stacking order.
   * Thus it is important have this call *after* PlaceWindow and the
   * stacking order initialization.
   */
  used_sm = MatchWinToSM(tmp_win, &do_shade, &do_maximize);
  if (used_sm)
  {
    /* read the requested absolute geometry */
    gravity_translate_to_northwest_geometry_no_bw(
      tmp_win->hints.win_gravity, tmp_win, &tmp_win->normal_g,
      &tmp_win->normal_g);
    gravity_resize(
      tmp_win->hints.win_gravity, &tmp_win->normal_g,
      2 * tmp_win->boundary_width,
      2 * tmp_win->boundary_width + tmp_win->title_g.height);
    tmp_win->frame_g = tmp_win->normal_g;
    tmp_win->frame_g.x -= Scr.Vx;
    tmp_win->frame_g.y -= Scr.Vy;

    /****** calculate frame size ******/
    setup_frame_size_limits(tmp_win, &style);
    constrain_size(
      tmp_win, (unsigned int *)&tmp_win->frame_g.width,
      (unsigned int *)&tmp_win->frame_g.height, 0, 0, 0);

    /****** maximize ******/
    if (do_maximize)
    {
      SET_MAXIMIZED(tmp_win, 1);
      constrain_size(
	tmp_win, (unsigned int *)&tmp_win->max_g.width,
	(unsigned int *)&tmp_win->max_g.height, 0, 0, CS_UPDATE_MAX_DEFECT);
      get_relative_geometry(&tmp_win->frame_g, &tmp_win->max_g);
    }
    else
    {
      get_relative_geometry(&tmp_win->frame_g, &tmp_win->normal_g);
    }
  }
  else
  {
    if (IS_SHADED(tmp_win))
    {
      do_shade = 1;
      SET_SHADED(tmp_win, 0);
    }
    /* Tentative size estimate */
    tmp_win->frame_g.width = tmp_win->attr.width + 2 * tmp_win->boundary_width;
    tmp_win->frame_g.height = tmp_win->attr.height + tmp_win->title_g.height +
      2 * tmp_win->boundary_width;

    /****** calculate frame size ******/
    setup_frame_size_limits(tmp_win, &style);

    /****** layer ******/
    setup_layer(tmp_win, &style);

    /****** window placement ******/
    do_resize_too = setup_window_placement(tmp_win, &style);

    /* set up geometry */
    tmp_win->frame_g.x = tmp_win->attr.x;
    tmp_win->frame_g.y = tmp_win->attr.y;
    tmp_win->frame_g.width = tmp_win->attr.width + 2 * tmp_win->boundary_width;
    tmp_win->frame_g.height = tmp_win->attr.height + tmp_win->title_g.height
      + 2 * tmp_win->boundary_width;
    gravity_constrain_size(
      tmp_win->hints.win_gravity, tmp_win, &tmp_win->frame_g, 0);

    update_absolute_geometry(tmp_win);
  }

  /****** auxiliary window setup ******/
  setup_auxiliary_windows(tmp_win, True, buttons);

  /****** 'backing store' and 'save under' window setup ******/
  setup_frame_attributes(tmp_win, &style);

  /****** reparent the window ******/
  XReparentWindow(dpy, tmp_win->w, tmp_win->Parent, 0, 0);

  /****** select events ******/
  valuemask = CWEventMask | CWDontPropagate;
  if (IS_TEAR_OFF_MENU(tmp_win))
  {
    attributes.event_mask = XEVMASK_TEAR_OFF_MENUW;
  }
  else
  {
    attributes.event_mask = XEVMASK_CLIENTW;
  }
  attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
  XChangeWindowAttributes(dpy, tmp_win->w, valuemask, &attributes);
  /****** make sure the window is not destroyed when fvwm dies ******/
  if (!IS_TEAR_OFF_MENU(tmp_win))
  {
    /* menus were created by fvwm itself, don't add them to the save set */
    XAddToSaveSet(dpy, tmp_win->w);
  }

  /****** arrange the frame ******/
  ForceSetupFrame(tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y,
		  tmp_win->frame_g.width, tmp_win->frame_g.height,
		  True);

  /****** grab keys and buttons ******/
  setup_key_and_button_grabs(tmp_win);

  /****** place the window in the stack ring ******/
  if (!position_new_window_in_stack_ring(tmp_win, SDO_START_LOWERED(sflags)))
  {
    XWindowChanges xwc;
    xwc.sibling = get_next_window_in_stack_ring(tmp_win)->frame;
    xwc.stack_mode = Above;
    XConfigureWindow(dpy, tmp_win->frame, CWSibling|CWStackMode, &xwc);
  }

  /****** now we can sefely ungrab the server ******/
  MyXUngrabServer(dpy);

  /****** inform modules of new window ******/
  BroadcastConfig(M_ADD_WINDOW,tmp_win);
  BroadcastWindowIconNames(tmp_win, True, False);

  /* these are sent and broadcast before res_{class,name} for the benefit
   * of FvwmIconBox which can't handle M_ICON_FILE after M_RES_NAME */
  /****** icon and mini icon ******/
  /* migo (20-Jan-2000): the logic is to unset this flag on NULL values */
  SET_WAS_ICON_NAME_PROVIDED(tmp_win, 1);
  setup_icon(tmp_win, &style);
#ifdef MINI_ICONS
  setup_mini_icon(tmp_win, &style);
#endif

  BroadcastName(M_RES_CLASS,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->class.res_class);
  BroadcastName(M_RES_NAME,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->class.res_name);

  /****** stick window ******/
  if (IS_STICKY(tmp_win) && (!(tmp_win->hints.flags & USPosition) || used_sm))
  {
    /* If it's sticky and the user didn't ask for an explicit position, force
     * it on screen now.  Don't do that with USPosition because we have to
     * assume the user knows what he is doing.  This is necessary e.g. if we
     * want a sticky 'panel' in FvwmButtons but don't want to see when it's
     * mapped in the void. */
    if (!IsRectangleOnThisPage(&tmp_win->frame_g, Scr.CurrentDesk))
    {
      SET_STICKY(tmp_win, 0);
      handle_stick(&Event, tmp_win->frame, tmp_win, C_FRAME, "", 0, 1);
    }
  }

  /****** resize window ******/
  if(do_resize_too)
  {
    XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
		 Scr.MyDisplayHeight,
		 tmp_win->frame_g.x + (tmp_win->frame_g.width>>1),
		 tmp_win->frame_g.y + (tmp_win->frame_g.height>>1));
    Event.xany.type = ButtonPress;
    Event.xbutton.button = 1;
    Event.xbutton.x_root = tmp_win->frame_g.x + (tmp_win->frame_g.width>>1);
    Event.xbutton.y_root = tmp_win->frame_g.y + (tmp_win->frame_g.height>>1);
    Event.xbutton.x = (tmp_win->frame_g.width>>1);
    Event.xbutton.y = (tmp_win->frame_g.height>>1);
    Event.xbutton.subwindow = None;
    Event.xany.window = tmp_win->w;
    CMD_Resize(&Event , tmp_win->w, tmp_win, C_WINDOW, "", 0);
  }

  /****** window colormap ******/
  InstallWindowColormaps(colormap_win);

  /****** ewmh setup *******/
  EWMH_WindowInit(tmp_win);

  /****** gnome setup ******/
  /* set GNOME hints on the window from flags set on tmp_win */
  GNOME_SetHints(tmp_win);
  GNOME_SetLayer(tmp_win);
  GNOME_SetDesk(tmp_win);
  GNOME_SetWinArea(tmp_win);

  /****** windowshade ******/
  if (do_shade)
  {
    rectangle big_g;

    big_g = (IS_MAXIMIZED(tmp_win)) ? tmp_win->max_g : tmp_win->frame_g;
    get_shaded_geometry(tmp_win, &tmp_win->frame_g, &tmp_win->frame_g);
    XLowerWindow(dpy, tmp_win->Parent);
    SET_SHADED(tmp_win ,1);
    SetupFrame(
      tmp_win, tmp_win->frame_g.x, tmp_win->frame_g.y,
      tmp_win->frame_g.width, tmp_win->frame_g.height, False);
  }
  if (!XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY, &JunkWidth,
		    &JunkHeight, &JunkBW,  &JunkDepth))
  {
    /* The window has disappeared somehow.  For some reason we do not always
     * get a DestroyNotify on the window, so make sure it is destroyed. */
    destroy_window(tmp_win);
    tmp_win = NULL;
  }

  return tmp_win;
}


/***********************************************************************
 *
 *  Procedure:
 *	FetchWMProtocols - finds out which protocols the window supports
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/
void FetchWmProtocols (FvwmWindow *tmp)
{
  Atom *protocols = NULL, *ap;
  int i, n;
  Atom atype;
  int aformat;
  unsigned long bytes_remain,nitems;

  if(tmp == NULL) return;
  /* First, try the Xlib function to read the protocols.
   * This is what Twm uses. */
  if (XGetWMProtocols (dpy, tmp->w, &protocols, &n))
  {
    for (i = 0, ap = protocols; i < n; i++, ap++)
    {
      if (*ap == (Atom)_XA_WM_TAKE_FOCUS)
      {
	SET_WM_TAKES_FOCUS(tmp, 1);
	set_focus_model(tmp);
      }
      if (*ap == (Atom)_XA_WM_DELETE_WINDOW)
	SET_WM_DELETES_WINDOW(tmp, 1);
    }
    if (protocols)
      XFree((char *)protocols);
  }
  else
  {
    /* Next, read it the hard way. mosaic from Coreldraw needs to
     * be read in this way. */
    if ((XGetWindowProperty(dpy, tmp->w, _XA_WM_PROTOCOLS, 0L, 10L, False,
			    _XA_WM_PROTOCOLS, &atype, &aformat, &nitems,
			    &bytes_remain,
			    (unsigned char **)&protocols))==Success)
    {
      for (i = 0, ap = protocols; i < nitems; i++, ap++)
      {
	if (*ap == (Atom)_XA_WM_TAKE_FOCUS)
	{
	  SET_WM_TAKES_FOCUS(tmp, 1);
	  set_focus_model(tmp);
	}
	if (*ap == (Atom)_XA_WM_DELETE_WINDOW)
	  SET_WM_DELETES_WINDOW(tmp, 1);
      }
      if (protocols)
	XFree((char *)protocols);
    }
  }
  return;
}



/***********************************************************************
 *
 *  Procedure:
 *	GetWindowSizeHints - gets application supplied size info
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/

void GetWindowSizeHints(FvwmWindow *tmp)
{
  long supplied = 0;
  char *broken_cause ="";
  XSizeHints orig_hints;

  if (HAS_OVERRIDE_SIZE_HINTS(tmp) ||
      !XGetWMNormalHints(dpy, tmp->w, &tmp->hints, &supplied))
  {
    tmp->hints.flags = 0;
    memset(&orig_hints, 0, sizeof(orig_hints));
  }
  else
  {
    memcpy(&orig_hints, &tmp->hints, sizeof(orig_hints));
  }

  /* Beat up our copy of the hints, so that all important field are
   * filled in! */
  if (tmp->hints.flags & PResizeInc)
  {
    SET_SIZE_INC_SET(tmp, 1);
    if (tmp->hints.width_inc <= 0)
    {
      if (tmp->hints.width_inc < 0 ||
	  (tmp->hints.width_inc == 0 &&
	   (tmp->hints.flags & PMinSize) && (tmp->hints.flags & PMaxSize) &&
	   tmp->hints.min_width != tmp->hints.max_width))
      {
	broken_cause = "width_inc";
      }
      tmp->hints.width_inc = 1;
      SET_SIZE_INC_SET(tmp, 0);
    }
    if (tmp->hints.height_inc <= 0)
    {
      if (tmp->hints.height_inc < 0 ||
	  (tmp->hints.height_inc == 0 &&
	   (tmp->hints.flags & PMinSize) && (tmp->hints.flags & PMaxSize) &&
	   tmp->hints.min_height != tmp->hints.max_height))
      {
	if (!*broken_cause)
	  broken_cause = "height_inc";
      }
      tmp->hints.height_inc = 1;
      SET_SIZE_INC_SET(tmp, 0);
    }
  }
  else
  {
    SET_SIZE_INC_SET(tmp, 0);
    tmp->hints.width_inc = 1;
    tmp->hints.height_inc = 1;
  }

  if(tmp->hints.flags & PMinSize)
  {
    if (tmp->hints.min_width < 0 && !*broken_cause)
      broken_cause = "min_width";
    if (tmp->hints.min_height < 0 && !*broken_cause)
      broken_cause = "min_height";
  }
  else
  {
    if(tmp->hints.flags & PBaseSize)
    {
      tmp->hints.min_width = tmp->hints.base_width;
      tmp->hints.min_height = tmp->hints.base_height;
    }
    else
    {
      tmp->hints.min_width = 1;
      tmp->hints.min_height = 1;
    }
  }
  if (tmp->hints.min_width <= 0)
    tmp->hints.min_width = 1;
  if (tmp->hints.min_height <= 0)
    tmp->hints.min_height = 1;

  if(tmp->hints.flags & PMaxSize)
  {
    if(tmp->hints.max_width < tmp->hints.min_width)
    {
      tmp->hints.max_width = DEFAULT_MAX_MAX_WINDOW_WIDTH;
      if (!*broken_cause)
	broken_cause = "max_width";
    }
    if(tmp->hints.max_height < tmp->hints.min_height)
    {
      tmp->hints.max_height = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
      if (!*broken_cause)
	broken_cause = "max_height";
    }
  }
  else
  {
    tmp->hints.max_width = DEFAULT_MAX_MAX_WINDOW_WIDTH;
    tmp->hints.max_height = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
  }

  /*
   * ICCCM says that PMinSize is the default if no PBaseSize is given,
   * and vice-versa.
   */

  if(tmp->hints.flags & PBaseSize)
  {
    if (tmp->hints.base_width < 0)
    {
      tmp->hints.base_width = 0;
      if (!*broken_cause)
	broken_cause = "base_width";
    }
    if (tmp->hints.base_height < 0)
    {
      tmp->hints.base_height = 0;
      if (!*broken_cause)
	broken_cause = "base_height";
    }
    if ((tmp->hints.base_width > tmp->hints.min_width) ||
	(tmp->hints.base_height > tmp->hints.min_height))
    {
      /* In this case, doing the aspect ratio calculation
	 for window_size - base_size as prescribed by the
	 ICCCM is going to fail.
	 Resetting the flag disables the use of base_size
	 in aspect ratio calculation while it is still used
	 for grid sizing.
      */
      tmp->hints.flags &= ~PBaseSize;
#if 0
      /* Keep silent about this, since the Xlib manual actually
	 recommends making min <= base <= max ! */
      broken_cause = "";
#endif
    }
  }
  else
  {
    if(tmp->hints.flags & PMinSize)
    {
      tmp->hints.base_width = tmp->hints.min_width;
      tmp->hints.base_height = tmp->hints.min_height;
    }
    else
    {
      tmp->hints.base_width = 0;
      tmp->hints.base_height = 0;
    }
  }

  if(!(tmp->hints.flags & PWinGravity))
  {
    tmp->hints.win_gravity = NorthWestGravity;
  }

  if (tmp->hints.flags & PAspect)
  {
    /*
    ** check to make sure min/max aspect ratios look valid
    */
#define maxAspectX tmp->hints.max_aspect.x
#define maxAspectY tmp->hints.max_aspect.y
#define minAspectX tmp->hints.min_aspect.x
#define minAspectY tmp->hints.min_aspect.y


    /*
    ** The math looks like this:
    **
    **   minAspectX    maxAspectX
    **   ---------- <= ----------
    **   minAspectY    maxAspectY
    **
    ** If that is multiplied out, this must be satisfied:
    **
    **   minAspectX * maxAspectY <=  maxAspectX * minAspectY
    **
    ** So, what to do if this isn't met?  Ignoring it entirely
    ** seems safest.
    **
    */

    /* We also ignore people who put negative entries into
     * their aspect ratios. They deserve it.
     *
     * We cast to double here, since the values may be large.
     */
    if ((maxAspectX < 0) || (maxAspectY < 0) ||
        (minAspectX < 0) || (minAspectY < 0) ||
        (((double)minAspectX * (double)maxAspectY) >
         ((double)maxAspectX * (double)minAspectY)))
    {
      if (!*broken_cause)
	broken_cause = "aspect ratio";
      tmp->hints.flags &= ~PAspect;
      fvwm_msg (WARN, "GetWindowSizeHints",
		"%s window %#lx has broken aspect ratio: %d/%d - %d/%d\n",
		tmp->name, tmp->w, minAspectX, minAspectY, maxAspectX,
	  	maxAspectY);
    }
    else
    {
      /* protect against overflow */
      if ((maxAspectX > 65536) || (maxAspectY > 65536))
      {
	double ratio = (double) maxAspectX / (double) maxAspectY;
	if (ratio > 1.0)
	{
	  maxAspectX = 65536;
	  maxAspectY = 65536 / ratio;
	}
	else
	{
	  maxAspectX = 65536 * ratio;
	  maxAspectY = 65536;
	}
      }
      if ((minAspectX > 65536) || (minAspectY > 65536))
      {
	double ratio = (double) minAspectX / (double) minAspectY;
	if (ratio > 1.0)
	{
	  minAspectX = 65536;
	  minAspectY = 65536 / ratio;
	}
	else
	{
	  minAspectX = 65536 * ratio;
	  minAspectY = 65536;
	}
      }
    }
  }

  if (*broken_cause != 0)
  {
    fvwm_msg(WARN, "GetWindowSizeHints",
	     "%s window %#lx has broken (%s) size hints\n"
	     "Please send a bug report to the application owner,\n"
	     "\tyou may use fvwm-workers@fvwm.org as a reference.\n"
	     "hint override = %d\n"
	     "flags = %lx\n"
	     "min_width = %d, min_height = %d, "
	     "max_width = %d, max_height = %d\n"
	     "width_inc = %d, height_inc = %d\n"
	     "min_aspect = %d/%d, max_aspect = %d/%d\n"
	     "base_width = %d, base_height = %d\n"
	     "win_gravity = %d\n",
	     tmp->name, tmp->w, broken_cause,
	     HAS_OVERRIDE_SIZE_HINTS(tmp),
	     orig_hints.flags,
	     orig_hints.min_width, orig_hints.min_height,
	     orig_hints.max_width, orig_hints.max_height,
	     orig_hints.width_inc, orig_hints.height_inc,
	     orig_hints.min_aspect.x, orig_hints.min_aspect.y,
	     orig_hints.max_aspect.x, orig_hints.max_aspect.y,
	     orig_hints.base_width, orig_hints.base_height,
	     orig_hints.win_gravity);
  }
}

/**************************************************************************
 *
 * Releases dynamically allocated space used to store window/icon names
 *
 **************************************************************************/
void free_window_names(FvwmWindow *tmp, Bool nukename, Bool nukeicon)
{
  if (!tmp)
    return;

  if (nukename && tmp->visible_name &&
      tmp->visible_name != tmp->name &&
      tmp->visible_name != NoName)
  {
    free(tmp->visible_name);
    tmp->visible_name = NULL;
  }
  if (nukeicon && tmp->visible_icon_name &&
      tmp->visible_icon_name != tmp->name &&
      tmp->visible_icon_name != tmp->icon_name &&
      tmp->visible_icon_name != NoName)
  {
    free(tmp->visible_icon_name);
    tmp->visible_icon_name = NULL;
  }
  if (nukename && tmp->name)
  {
#ifdef CODE_WITH_LEAK_I_THINK
    if (tmp->name != tmp->icon_name && tmp->name != NoName)
      FlocaleFreeNameProperty(MULTIBYTE_ARG(&(tmp->name_list)) &(tmp->name));
#else
    if (tmp->icon_name == tmp->name)
      tmp->icon_name = NoName;
    if (tmp->visible_icon_name == tmp->name)
      tmp->visible_icon_name = tmp->icon_name;
    if (tmp->name != NoName)
    {
      FlocaleFreeNameProperty(MULTIBYTE_ARG(&(tmp->name_list)) &(tmp->name));
      tmp->visible_name = NULL;
    }
#endif
  }
  if (nukeicon && tmp->icon_name)
  {
#ifdef CODE_WITH_LEAK_I_THINK
    if ((tmp->name != tmp->icon_name || nukename) && tmp->icon_name != NoName)
    {
      FlocaleFreeNameProperty(MULTIBYTE_ARG(&(tmp->icon_name_list))
			      &(tmp->icon_name));
      tmp->visible_icon_name = NULL;
    }
#else
    if ((tmp->name != tmp->icon_name) && tmp->icon_name != NoName)
    {
      FlocaleFreeNameProperty(MULTIBYTE_ARG(&(tmp->icon_name_list))
			      &(tmp->icon_name));
      tmp->visible_icon_name = NULL;
    }
#endif
  }

  return;
}


static void adjust_fvwm_internal_windows(FvwmWindow *tmp_win)
{
  extern FvwmWindow *ButtonWindow;

  if (tmp_win == Scr.Hilite)
    Scr.Hilite = NULL;
  update_last_screen_focus_window(tmp_win);
  if (ButtonWindow == tmp_win)
    ButtonWindow = NULL;
  restore_focus_after_unmap(tmp_win, False);

  return;
}


/***************************************************************************
 *
 * Handles destruction of a window
 *
 ****************************************************************************/
void destroy_window(FvwmWindow *tmp_win)
{
  extern Bool PPosOverride;

  /*
   * Warning, this is also called by HandleUnmapNotify; if it ever needs to
   * look at the event, HandleUnmapNotify will have to mash the UnmapNotify
   * into a DestroyNotify.
   */
  if(!tmp_win)
    return;

  /****** remove from window list ******/

  /* first of all, remove the window from the list of all windows! */
  tmp_win->prev->next = tmp_win->next;
  if (tmp_win->next != NULL)
    tmp_win->next->prev = tmp_win->prev;

  /****** also remove it from the stack ring ******/

  /*
   * RBW - 11/13/1998 - new: have to unhook the stacking order chain also.
   * There's always a prev and next, since this is a ring anchored on
   * Scr.FvwmRoot
   */
  remove_window_from_stack_ring(tmp_win);

  /****** check if we have to delay window destruction ******/

  if (Scr.flags.is_executing_complex_function && !DO_REUSE_DESTROYED(tmp_win))
  {
    /* mark window for destruction */
    SET_SCHEDULED_FOR_DESTROY(tmp_win, 1);
    Scr.flags.is_window_scheduled_for_destroy = 1;
    /* this is necessary in case the application destroys the client window and
     * a new window is created with the saem window id */
    XDeleteContext(dpy, tmp_win->w, FvwmContext);
    /* unmap the the window to fake that it was already removed */
    if (IS_ICONIFIED(tmp_win))
    {
      if (tmp_win->icon_title_w)
	XUnmapWindow(dpy, tmp_win->icon_title_w);
      if(tmp_win->icon_pixmap_w != None)
	XUnmapWindow(dpy, tmp_win->icon_pixmap_w);
    }
    else
    {
      XUnmapWindow(dpy, tmp_win->frame);
    }
    adjust_fvwm_internal_windows(tmp_win);
    BroadcastPacket(M_DESTROY_WINDOW, 3,
		    tmp_win->w, tmp_win->frame, (unsigned long)tmp_win);
    EWMH_DestroyWindow(tmp_win);
    return;
  }

  /****** unmap the frame ******/

  XUnmapWindow(dpy, tmp_win->frame);

  if(!PPosOverride)
    XSync(dpy,0);

  /* already done above? */
  if (!IS_SCHEDULED_FOR_DESTROY(tmp_win))
  {
    /****** adjust fvwm internal windows and the focus ******/

    adjust_fvwm_internal_windows(tmp_win);

    /****** broadcast ******/

    BroadcastPacket(M_DESTROY_WINDOW, 3,
		    tmp_win->w, tmp_win->frame, (unsigned long)tmp_win);
    EWMH_DestroyWindow(tmp_win);
  }

  /****** adjust fvwm internal windows II ******/

  /* restore_focus_after_unmap takes care of Scr.pushed_window and colormap_win
   */
  if (tmp_win == Scr.Ungrabbed)
    Scr.Ungrabbed = NULL;

  /****** destroy auxiliary windows ******/

  destroy_auxiliary_windows(tmp_win, True);

  /****** destroy icon ******/

  destroy_icon_boxes(tmp_win);
  destroy_icon(tmp_win);

  /****** destroy mini icon ******/

#ifdef MINI_ICON
  destroy_mini_icon(tmp_win);
#endif

  /****** free strings ******/

  free_window_names(tmp_win, True, True);

  if (tmp_win->class.res_name && tmp_win->class.res_name != NoResource)
  {
    XFree ((char *)tmp_win->class.res_name);
    tmp_win->class.res_name = NoResource;
  }
  if (tmp_win->class.res_class && tmp_win->class.res_class != NoClass)
  {
    XFree ((char *)tmp_win->class.res_class);
    tmp_win->class.res_class = NoClass;
  }
  if (tmp_win->mwm_hints)
  {
    XFree((char *)tmp_win->mwm_hints);
    tmp_win->mwm_hints = NULL;
  }

  /****** free fonts ******/

  destroy_window_font(tmp_win);
  destroy_icon_font(tmp_win);

  /****** free wmhints ******/

  if (tmp_win->wmhints)
  {
    XFree ((char *)tmp_win->wmhints);
    tmp_win->wmhints = NULL;
  }

  /****** free colormap windows ******/

  if (tmp_win->cmap_windows != (Window *)NULL)
  {
    XFree((void *)tmp_win->cmap_windows);
    tmp_win->cmap_windows = NULL;
  }

  /****** throw away the structure ******/

  /*  Recapture reuses this struct, so don't free it.  */
  if (!DO_REUSE_DESTROYED(tmp_win))
  {
    free((char *)tmp_win);
  }

  /****** cleanup ******/

  if(!PPosOverride)
    XSync(dpy,0);

  return;
}




/***********************************************************************
 *
 *  Procedure:
 *	RestoreWithdrawnLocation
 *
 *  Puts windows back where they were before fvwm took over
 *
 ************************************************************************/
void RestoreWithdrawnLocation(
  FvwmWindow *tmp, Bool is_restart_or_recapture, Window parent)
{
  int w2,h2;
  unsigned int mask;
  XWindowChanges xwc;
  rectangle naked_g;
  rectangle unshaded_g;
  XSetWindowAttributes xswa;

  if(!tmp)
    return;

  get_unshaded_geometry(tmp, &unshaded_g);
  gravity_get_naked_geometry(
    tmp->hints.win_gravity, tmp, &naked_g, &unshaded_g);
  gravity_translate_to_northwest_geometry(
    tmp->hints.win_gravity, tmp, &naked_g, &naked_g);
  xwc.x = naked_g.x;
  xwc.y = naked_g.y;
  xwc.width = naked_g.width;
  xwc.height = naked_g.height;
  xwc.border_width = tmp->old_bw;
  mask = (CWX | CWY | CWWidth | CWHeight | CWBorderWidth);

  /* We can not assume that the window is currently on the screen.
   * Although this is normally the case, it is not always true.  The
   * most common example is when the user does something in an
   * application which will, after some amount of computational delay,
   * cause the window to be unmapped, but then switches screens before
   * this happens.  The XTranslateCoordinates call above will set the
   * window coordinates to either be larger than the screen, or negative.
   * This will result in the window being placed in odd, or even
   * unviewable locations when the window is remapped.  The following code
   * forces the "relative" location to be within the bounds of the display.
   *
   * gpw -- 11/11/93
   *
   * Unfortunately, this does horrendous things during re-starts,
   * hence the "if(restart)" clause (RN)
   *
   * Also, fixed so that it only does this stuff if a window is more than
   * half off the screen. (RN)
   */

  if (!is_restart_or_recapture)
  {
    /* Don't mess with it if its partially on the screen now */
    if (unshaded_g.x < 0 || unshaded_g.y < 0 ||
	unshaded_g.x >= Scr.MyDisplayWidth ||
	unshaded_g.y >= Scr.MyDisplayHeight)
    {
      w2 = (unshaded_g.width >> 1);
      h2 = (unshaded_g.height >> 1);
      if ( xwc.x < -w2 || xwc.x > Scr.MyDisplayWidth - w2)
      {
	xwc.x = xwc.x % Scr.MyDisplayWidth;
	if (xwc.x < -w2)
	  xwc.x += Scr.MyDisplayWidth;
      }
      if (xwc.y < -h2 || xwc.y > Scr.MyDisplayHeight - h2)
      {
	xwc.y = xwc.y % Scr.MyDisplayHeight;
	if (xwc.y < -h2)
	  xwc.y += Scr.MyDisplayHeight;
      }
    }
  }

  /* restore initial backing store setting on window */
  xswa.backing_store = tmp->initial_backing_store;
  XChangeWindowAttributes(dpy, tmp->w, CWBackingStore, &xswa);
  /* reparent to root window */
  XReparentWindow(
    dpy, tmp->w, (parent == None) ? Scr.Root : parent, xwc.x, xwc.y);

  if (IS_ICONIFIED(tmp) && !IS_ICON_SUPPRESSED(tmp))
  {
    if (tmp->icon_title_w)
      XUnmapWindow(dpy, tmp->icon_title_w);
    if (tmp->icon_pixmap_w)
      XUnmapWindow(dpy, tmp->icon_pixmap_w);
  }

  XConfigureWindow(dpy, tmp->w, mask, &xwc);
  if (!is_restart_or_recapture)
  {
    /* must be initial capture */
    XSync(dpy,0);
  }
}


/***********************************************************************
 *
 *  Procedure:
 *	Reborder - Removes fvwm border windows
 *
 ************************************************************************/
void Reborder(void)
{
  FvwmWindow *tmp;

  /* put a border back around all windows */
  MyXGrabServer (dpy);

  /* force reinstall */
  InstallWindowColormaps (&Scr.FvwmRoot);

  /* RBW - 05/15/1998
   * Grab the last window and work backwards: preserve stacking order on
   * restart. */
  for (tmp = get_prev_window_in_stack_ring(&Scr.FvwmRoot); tmp != &Scr.FvwmRoot;
       tmp = get_prev_window_in_stack_ring(tmp))
  {
    if (!IS_ICONIFIED(tmp) && Scr.CurrentDesk != tmp->Desk)
    {
      XMapWindow(dpy, tmp->w);
      SetMapStateProp(tmp, NormalState);
    }
    RestoreWithdrawnLocation (tmp, True, Scr.Root);
    XUnmapWindow(dpy,tmp->frame);
    XDestroyWindow(dpy,tmp->frame);
  }

  MyXUngrabServer (dpy);
  FOCUS_RESET();
  XSync(dpy,0);
}

/***********************************************************************
 *
 *  Procedure:
 *      CaptureOneWindow
 *      CaptureAllWindows
 *
 *   Decorates windows at start-up and during recaptures
 *
 ***********************************************************************/

static void CaptureOneWindow(
	FvwmWindow *fw, Window window, Window keep_on_top_win,
	Window parent_win, Bool is_recapture)
{
	Window w;
	unsigned long data[1];

	isIconicState = DontCareState;
	if (fw == NULL)
	{
		return;
	}
	if (IS_SCHEDULED_FOR_DESTROY(fw))
	{
		/* Fvwm might crash in complex functions if we really try to
		 * the dying window here because AddWindow() may fail and leave
		 * a destroyed window in Tmp_win.  Any, by the way, it is
		 * pretty useless to recapture a * window that will vanish in a
		 * moment. */
		return;
	}
	/* Grab the server to make sure the window does not die during the
	 * recapture. */
	MyXGrabServer(dpy);
	if (!XGetGeometry(dpy, fw->w, &JunkRoot, &JunkX, &JunkY, &JunkWidth,
			  &JunkHeight, &JunkBW,  &JunkDepth))
	{
		/* The window has already died, do not recapture it! */
		MyXUngrabServer(dpy);
		return;
	}
	if (XFindContext(dpy, window, FvwmContext, (caddr_t *)&fw) != XCNOENT)
	{
		Bool f = PPosOverride;
		Bool is_mapped = IS_MAPPED(fw);
		Bool is_menu;

		PPosOverride = True;
		if (IS_ICONIFIED(fw))
		{
			isIconicState = IconicState;
			isIconifiedByParent = IS_ICONIFIED_BY_PARENT(fw);
		}
		else
		{
			isIconicState = NormalState;
			isIconifiedByParent = 0;
			if (Scr.CurrentDesk != fw->Desk)
				SetMapStateProp(fw, NormalState);
		}
		data[0] = (unsigned long) fw->Desk;
		XChangeProperty (dpy, fw->w, _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32,
				 PropModeReplace, (unsigned char *) data, 1);

		/* are all these really needed ? */
		/* EWMH_SetWMDesktop(fw); */
		GNOME_SetHints(fw);
		GNOME_SetDesk(fw);
		GNOME_SetLayer(fw);
		GNOME_SetWinArea(fw);

		XSelectInput(dpy, fw->w, NoEventMask);
		w = fw->w;
		XUnmapWindow(dpy, fw->frame);
		RestoreWithdrawnLocation(fw, is_recapture, parent_win);
		SET_DO_REUSE_DESTROYED(fw, 1); /* RBW - 1999/03/20 */
		destroy_window(fw);
		Event.xmaprequest.window = w;
		if (is_recapture && fw != NULL && IS_TEAR_OFF_MENU(fw))
		{
			is_menu = True;
		}
		else
		{
			is_menu = False;
		}
		HandleMapRequestKeepRaised(keep_on_top_win, fw, is_menu);
		if (!fFvwmInStartup)
		{
			SET_MAP_PENDING(fw, 0);
			SET_MAPPED(fw, is_mapped);
		}
		/* Clean out isIconicState here, otherwise all new windos may
		 * start iconified. */
		isIconicState = DontCareState;
		/* restore previous value */
		PPosOverride = f;
	}
	MyXUngrabServer(dpy);

	return;
}

/* Put a transparent window all over the screen to hide what happens below. */
static void hide_screen(
	Bool do_hide, Window *ret_hide_win, Window *ret_parent_win)
{
	static Bool is_hidden = False;
	static Window hide_win = None;
	static Window parent_win = None;
	XSetWindowAttributes xswa;
	unsigned long valuemask;

	if (do_hide == is_hidden)
	{
		/* nothing to do */
		if (ret_hide_win)
		{
			*ret_hide_win = hide_win;
		}
		if (ret_parent_win)
		{
			*ret_parent_win = parent_win;
		}

		return;
	}
	is_hidden = do_hide;
	if (do_hide)
	{
		xswa.override_redirect = True;
		xswa.cursor = Scr.FvwmCursors[CRS_WAIT];
		valuemask = CWOverrideRedirect|CWCursor;
		hide_win = XCreateWindow(
			dpy, Scr.Root, 0, 0, Scr.MyDisplayWidth,
			Scr.MyDisplayHeight, 0, CopyFromParent, InputOutput,
			CopyFromParent, valuemask, &xswa);
		if (hide_win)
		{
			/* When recapturing, all windows are reparented to this
			 * window. If they are reparented to the root window,
			 * they will flash over the hide_win with XFree.  So
			 * reparent them to an unmapped window that looks like
			 * the root window. */
			parent_win = XCreateWindow(
				dpy, Scr.Root, 0, 0, Scr.MyDisplayWidth,
				Scr.MyDisplayHeight, 0, CopyFromParent,
				InputOutput, CopyFromParent, valuemask, &xswa);
			if (!parent_win)
			{
				XDestroyWindow(dpy, hide_win);
				hide_win = None;
			}
			else
			{
				XMapWindow(dpy, hide_win);
				XSync(dpy, 0);
			}
		}
	}
	else
	{
		if (hide_win != None)
		{
			XDestroyWindow(dpy, hide_win);
		}
		if (parent_win != None)
		{
			XDestroyWindow(dpy, parent_win);
		}
		XSync(dpy,0);
		hide_win = None;
		parent_win = None;
	}
	if (ret_hide_win)
	{
		*ret_hide_win = hide_win;
	}
	if (ret_parent_win)
	{
		*ret_parent_win = parent_win;
	}

	return;
}

/***********************************************************************
 *
 *  Procedure:
 *	MappedNotOverride - checks to see if we should really
 *		put a fvwm frame on the window
 *
 *  Returned Value:
 *	TRUE	- go ahead and frame the window
 *	FALSE	- don't frame the window
 *
 *  Inputs:
 *	w	- the window to check
 *
 ***********************************************************************/

static int MappedNotOverride(Window w)
{
	XWindowAttributes wa;
        Atom atype;
	int aformat;
	unsigned long nitems, bytes_remain;
	unsigned char *prop;

	isIconicState = DontCareState;

	if((w==Scr.NoFocusWin)||(!XGetWindowAttributes(dpy, w, &wa)))
		return False;

	if(XGetWindowProperty(
		   dpy,w,_XA_WM_STATE,0L,3L,False,_XA_WM_STATE,
		   &atype,&aformat,&nitems,&bytes_remain,&prop)==Success)
	{
		if(prop != NULL)
		{
			isIconicState = *(long *)prop;
			XFree(prop);
		}
	}
	if (wa.override_redirect == True)
	{
		XSelectInput(dpy, w, XEVMASK_ORW);
	}
	return (((isIconicState == IconicState) ||
		 (wa.map_state != IsUnmapped)) &&
		(wa.override_redirect != True));
}

void CaptureAllWindows(Bool is_recapture)
{
  int i,j;
  unsigned int nchildren;
  Window root, parent, *children;
  FvwmWindow *tmp;		/* temp fvwm window structure */

  MyXGrabServer(dpy);

  if(!XQueryTree(dpy, Scr.Root, &root, &parent, &children, &nchildren))
  {
    MyXUngrabServer(dpy);
    return;
  }

  PPosOverride = True;
  if (!(Scr.flags.windows_captured)) /* initial capture? */
  {
    /*
    ** weed out icon windows
    */
    for (i=0;i<nchildren;i++)
    {
      if (children[i])
      {
        XWMHints *wmhintsp = XGetWMHints (dpy, children[i]);
        if (wmhintsp)
        {
          if (wmhintsp->flags & IconWindowHint)
          {
            for (j = 0; j < nchildren; j++)
            {
              if (children[j] == wmhintsp->icon_window)
              {
                children[j] = None;
                break;
              }
            }
          }
          XFree ((char *) wmhintsp);
        }
      }
    }
    /*
    ** map all of the non-override, non-icon windows
    */
    for (i=0;i<nchildren;i++)
    {
      if (children[i] && MappedNotOverride(children[i]))
      {
        XUnmapWindow(dpy, children[i]);
        Event.xmaprequest.window = children[i];
        HandleMapRequestKeepRaised(None, NULL, False);
      }
    }
    Scr.flags.windows_captured = 1;
  }
  else /* must be recapture */
  {
    Window keep_on_top_win;
    Window parent_win;
    Window focus_w;
    FvwmWindow *t;

    t = get_focus_window();
    focus_w = (t) ? t->w : None;
    hide_screen(True, &keep_on_top_win, &parent_win);
    /* reborder all windows */
    for (i=0;i<nchildren;i++)
    {
      if (XFindContext(dpy, children[i], FvwmContext, (caddr_t *)&tmp)!=XCNOENT)
      {
        CaptureOneWindow(
	  tmp, children[i], keep_on_top_win, parent_win, is_recapture);
      }
    }
    hide_screen(False, NULL, NULL);
    /* find the window that had the focus and focus it again */
    if (focus_w)
    {
      for (t = Scr.FvwmRoot.next; t && t->w != focus_w; t = t->next)
	;
      if (t)
      {
	SetFocusWindow(t, False, True);
      }
    }
  }

  isIconicState = DontCareState;

  if(nchildren > 0)
    XFree((char *)children);

  /* after the windows already on the screen are in place,
   * don't use PPosition */
  PPosOverride = False;
  MyXUngrabServer(dpy);
}

static void do_recapture(F_CMD_ARGS, Bool fSingle)
{
	if (fSingle)
	{
		if (DeferExecution(eventp, &w, &tmp_win, &context, CRS_SELECT,
				   ButtonRelease))
		{
			return;
		}
	}
	MyXGrabServer(dpy);
	if (fSingle)
	{
		CaptureOneWindow(
			tmp_win, tmp_win->w, None, None, True);
	}
	else
	{
		CaptureAllWindows(True);
	}
	/* Throw away queued up events. We don't want user input during a
	 * recapture.  The window the user clicks in might disapper at the very
	 * same moment and the click goes through to the root window. Not good
	 */
	XAllowEvents(dpy, AsyncPointer, CurrentTime);
	discard_events(
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|\
		PointerMotionMask|EnterWindowMask|LeaveWindowMask|\
		KeyPressMask|KeyReleaseMask);
#ifdef DEBUG_STACK_RING
	verify_stack_ring_consistency();
#endif
	MyXUngrabServer(dpy);
}

void CMD_Recapture(F_CMD_ARGS)
{
	do_recapture(F_PASS_ARGS, False);
}

void CMD_RecaptureWindow(F_CMD_ARGS)
{
	do_recapture(F_PASS_ARGS, True);
}
