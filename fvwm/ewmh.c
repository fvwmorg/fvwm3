/* Copyright (C) 2001  Olivier Chapuis
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ************************************************************************
 * An implementation of the Extended Window Manager Hints version 1.0 and 1.1
 *
 * http://www.freedesktop.org/standards/wm-spec.html
 *
 * version 1.2 is on the road, but I wait it become official
 * ************************************************************************/

#include "config.h"

#ifdef HAVE_EWMH

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "cursor.h"
#include "functions.h"
#include "misc.h"
#include "screen.h"
#include "update.h"
#include "stack.h"
#include "style.h"
#include "externs.h"
#include "ewmh.h"
#include "ewmh_intern.h"

typedef struct kst_item
{
  Window w;
  struct  kst_item *next;
} KstItem;


Atom XA_UTF8_STRING = None;
KstItem *ewmh_KstWinList = NULL;

/* ************************************************************************* *
 * Default
 * ************************************************************************* */
ewmhInfo ewmhc =
{
  4,            /* NumberOfDesktops */
  0,            /* MaxDesktops limit the number od desktops*/
  0,            /* CurrentNumberOfDesktops */
  False,        /* NeedsToCheckDesk */
  {0, 0, 0, 0}, /* BaseStrut */
};

/* ************************************************************************* *
 * The ewmh atoms lists
 * ************************************************************************* */
#define ENTRY(name, type, func) \
 {name, None, type, func}

#define NET_SUPPORTED 1
#define NET_NOT_SUPPORTED 0
#define IS_NET_ATOM_SUPPORTED(x) !!(x->flags)

/* EWMH_ATOM_LIST_CLIENT_ROOT
 * net atoms that can be send (Client Message) by a client and do not
 * need a window to operate */
ewmh_atom ewmh_atom_client_root[] =
{
  ENTRY("_NET_CURRENT_DESKTOP",    XA_CARDINAL, ewmh_CurrentDesktop),
  ENTRY("_NET_DESKTOP_GEOMETRY",   XA_CARDINAL, ewmh_DesktopGeometry),
  ENTRY("_NET_DESKTOP_VIEWPORT",   XA_CARDINAL, ewmh_DesktopViewPort),
  ENTRY("_NET_NUMBER_OF_DESKTOPS", XA_CARDINAL, ewmh_NumberOfDesktops),
  {NULL,0,0,0}
};

/* EWMH_ATOM_LIST_CLIENT_WIN
 * net atoms that can be send (Client Message) by a client and do need
 * window to operate */
ewmh_atom ewmh_atom_client_win[] =
{
  ENTRY("_NET_ACTIVE_WINDOW", XA_WINDOW,   ewmh_ActiveWindow),
  ENTRY("_NET_CLOSE_WINDOW",  XA_WINDOW,   ewmh_CloseWindow),
  ENTRY("_NET_WM_DESKTOP",    XA_CARDINAL, ewmh_WMDesktop),
  ENTRY("_NET_WM_MOVERESIZE", XA_WINDOW,   ewmh_MoveResize),
  ENTRY("_NET_WM_STATE",      XA_ATOM,     ewmh_WMState),
  {NULL,0,0,0}
};

/* EWMH_ATOM_LIST_WM_STATE
 * the different wm state, a client can ask to add, remove or toggle
 * via a _NET_WM_STATE Client message. fvwm2 must maintain these states
 * too */
ewmh_atom ewmh_atom_wm_state[] =
{
  ENTRY("_NET_WM_STATE_MAXIMIZED_HORIZ", XA_ATOM,   ewmh_WMStateMaxHoriz),
  ENTRY("_NET_WM_STATE_MAXIMIZED_HORZ",  XA_ATOM,   ewmh_WMStateMaxHoriz),
  ENTRY("_NET_WM_STATE_MAXIMIZED_VERT",  XA_ATOM,   ewmh_WMStateMaxVert),
  ENTRY("_NET_WM_STATE_MODAL",           XA_ATOM,   ewmh_WMStateModal),
  ENTRY("_NET_WM_STATE_SHADED",          XA_ATOM,   ewmh_WMStateShaded),
  ENTRY("_NET_WM_STATE_SKIP_PAGER",      XA_ATOM,   ewmh_WMStateListSkip),
  ENTRY("_NET_WM_STATE_SKIP_TASKBAR",    XA_ATOM,   ewmh_WMStateListSkip),
  ENTRY("_NET_WM_STATE_STAYS_ON_TOP",    XA_ATOM,   ewmh_WMStateStaysOnTop),
  ENTRY("_NET_WM_STATE_STICKY",          XA_ATOM,   ewmh_WMStateSticky),
  {NULL,0,0,0}
};
#define EWMH_NUMBER_OF_STATE sizeof(ewmh_atom_wm_state)/sizeof(ewmh_atom) - 1

/* EWMH ATOM_LIST_WINDOW_TYPE: the various window type */
ewmh_atom ewmh_atom_window_type[] =
{
  ENTRY("_NET_WM_WINDOW_TYPE_DESKTOP", XA_ATOM, ewmh_HandleDesktop),
  ENTRY("_NET_WM_WINDOW_TYPE_DIALOG",  XA_ATOM, ewmh_HandleDialog),
  ENTRY("_NET_WM_WINDOW_TYPE_DOCK",    XA_ATOM, ewmh_HandleDock),
  ENTRY("_NET_WM_WINDOW_TYPE_MENU",    XA_ATOM, ewmh_HandleMenu),
  ENTRY("_NET_WM_WINDOW_TYPE_NORMAL",  XA_ATOM, ewmh_HandleNormal),
  ENTRY("_NET_WM_WINDOW_TYPE_TOOLBAR", XA_ATOM, ewmh_HandleToolBar),
  {NULL,0,0,0}
};

/* EWMH ATOM_LIST_FIXED_PROPERTY
 * property that have a window at startup and which should not change */
ewmh_atom ewmh_atom_fixed_property[] =
{
  ENTRY("_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", XA_WINDOW, None),
  ENTRY("_NET_WM_HANDLED_ICON",               XA_ATOM,   None),
  ENTRY("_NET_WM_PID",                        XA_ATOM,   None),
  ENTRY("_NET_WM_WINDOW_TYPE",                XA_ATOM,   None),
  {NULL,0,0,0}
};

/* EWMH ATOM_LIST_PROPERTY_NOTIFY
 * properties of a window which is updated by the window via a PropertyNotify
 * event */
ewmh_atom ewmh_atom_property_notify[] =
{
  ENTRY("_NET_WM_ICON",          XA_CARDINAL, ewmh_WMIcon),
  ENTRY("_NET_WM_ICON_GEOMETRY", XA_CARDINAL, ewmh_WMIconGeometry),
  ENTRY("_NET_WM_ICON_NAME",     None,        EWMH_WMIconName),
  ENTRY("_NET_WM_NAME",          None,        EWMH_WMName),
  ENTRY("_NET_WM_STRUT",         XA_CARDINAL, ewmh_WMStrut),
  {NULL,0,0,0}
};

/* EWMH_ATOM_LIST_FVWM_ROOT: root atom that should be maintained by fvwm only */
ewmh_atom ewmh_atom_fvwm_root[] =
{
  ENTRY("_KDE_NET_SYSTEM_TRAY_WINDOWS", XA_WINDOW,   None),
  ENTRY("_NET_CLIENT_LIST",             XA_WINDOW,   None),
  ENTRY("_NET_CLIENT_LIST_STACKING",    XA_WINDOW,   None),
  ENTRY("_NET_SUPPORTED",               XA_ATOM,     None),
  ENTRY("_NET_SUPPORTING_WM_CHECK",     XA_WINDOW,   None),
  ENTRY("_NET_VIRTUAL_ROOTS",           XA_WINDOW,   None),
  ENTRY("_NET_WORKAREA",                XA_CARDINAL, None),
  {NULL,0,0,0}
};

/* EWMH_ATOM_LIST_FVWM_WIN: window atom that should be maintained by fvwm
 * only */
ewmh_atom ewmh_atom_fvwm_win[] =
{
  ENTRY("_KDE_NET_WM_FRAME_STRUT",    XA_CARDINAL, None),
  ENTRY("_NET_WM_ICON_VISIBLE_NAME",  None,        None),
  ENTRY("_NET_WM_VISIBLE_NAME",       None,        None),
  {NULL,0,0,0}
};


#define NUMBER_OF_ATOM_LISTS 8
#define L_ENTRY(x,y) {x,y,sizeof(y)/sizeof(ewmh_atom)}
ewmh_atom_list atom_list[] =
{
  L_ENTRY(EWMH_ATOM_LIST_CLIENT_ROOT,       ewmh_atom_client_root),
  L_ENTRY(EWMH_ATOM_LIST_CLIENT_WIN,        ewmh_atom_client_win),
  L_ENTRY(EWMH_ATOM_LIST_WM_STATE,          ewmh_atom_wm_state),
  L_ENTRY(EWMH_ATOM_LIST_WINDOW_TYPE,       ewmh_atom_window_type),
  L_ENTRY(EWMH_ATOM_LIST_FIXED_PROPERTY,    ewmh_atom_fixed_property),
  L_ENTRY(EWMH_ATOM_LIST_PROPERTY_NOTIFY,   ewmh_atom_property_notify),
  L_ENTRY(EWMH_ATOM_LIST_FVWM_ROOT,         ewmh_atom_fvwm_root),
  L_ENTRY(EWMH_ATOM_LIST_FVWM_WIN,          ewmh_atom_fvwm_win),
  L_ENTRY(EWMH_ATOM_LIST_END,               NULL)
};

/* ************************************************************************* *
 * Atoms utilities
 * ************************************************************************* */

static int compare(const void *a, const void *b)
{
  return (strcmp((char *)a, ((ewmh_atom *)b)->name));
}

ewmh_atom *get_ewmh_atom_by_name(const char *atom_name,
				 ewmh_atom_list_name list_name)
{
  ewmh_atom *a = NULL;
  Bool done = 0;
  int i = 0;

  while(!done && atom_list[i].name != EWMH_ATOM_LIST_END)
  {
    if (atom_list[i].name == list_name || list_name == EWMH_ATOM_LIST_ALL)
    {
       a = (ewmh_atom *)bsearch (atom_name,
				 atom_list[i].list,
				 atom_list[i].size - 1,
				 sizeof(ewmh_atom),
				 compare);
      if (a != NULL || list_name != EWMH_ATOM_LIST_ALL)
	done = 1;
    }
    i++;
  }
  return a;
}

ewmh_atom *ewmh_GetEwmhAtomByAtom(Atom atom, ewmh_atom_list_name list_name)
{
  Bool done = 0;
  int i = 0;

  while(!done && atom_list[i].name != EWMH_ATOM_LIST_END)
  {
    if (atom_list[i].name == list_name || list_name == EWMH_ATOM_LIST_ALL)
    {
      ewmh_atom *list = atom_list[i].list;
      while(list->name != NULL)
      {
	if (atom == list->atom)
	  return list;
	list++;
      }
      if (atom_list[i].name == list_name)
	return NULL;
    }
    i++;
  }
  return NULL;
}

void ewmh_ChangeProperty(Window w,
			 const char *atom_name,
			 ewmh_atom_list_name list, 
			 unsigned char *data,
			 unsigned int length)
{
  ewmh_atom *a;
  int format = 32;

  if ((a = get_ewmh_atom_by_name(atom_name, list)) != NULL)
  {
    if (a->atom_type == XA_UTF8_STRING)
      format = 8;
    XChangeProperty(
      dpy, w, a->atom, a->atom_type , format, PropModeReplace,
      data, length);
  }
}

void ewmh_DeleteProperty(Window w,
			 const char *atom_name,
			 ewmh_atom_list_name list)
{
  ewmh_atom *a;

  if ((a = get_ewmh_atom_by_name(atom_name, list)) != NULL)
  {
    XDeleteProperty(dpy, w, a->atom);
  }
}

void *atom_get(Window win, Atom to_get, Atom type, unsigned int *size)
{
  unsigned char	*retval;
  Atom	type_ret;
  unsigned long	 bytes_after, num_ret;
  long length;
  int  format_ret;
  void *data;

  retval = NULL;
  length = 0x7fffffff;
  XGetWindowProperty(dpy, win, to_get, 0,
		     length,
		     False, type,
		     &type_ret,
		     &format_ret,
		     &num_ret,
		     &bytes_after,
		     &retval);

  if ((retval) && (num_ret > 0) && (format_ret > 0))
  {
    data = safemalloc(num_ret * (format_ret >> 3));
    if (data)
      memcpy(data, retval, num_ret * (format_ret >> 3));
    XFree(retval);
    *size = num_ret * (format_ret >> 3);
    return data;
  }
  return NULL;
}

void *ewmh_AtomGetByName(Window win, const char *atom_name,
			 ewmh_atom_list_name list, unsigned int *size)
{
  ewmh_atom *a;
  void *data = NULL;

  if ((a = get_ewmh_atom_by_name(atom_name, list)) != NULL)
  {
    data = atom_get(win, a->atom, a->atom_type, size);
  }

  return data;
}

/* ************************************************************************* *
 *  client_root: here the client is fvwm2
 * ************************************************************************* */
int check_desk(void)
{
  int d = -1;
  FvwmWindow *t;

  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    if (!IS_STICKY(t))
      d = max(d,t->Desk);
  }
  return d;
}

void EWMH_SetCurrentDesktop(void)
{
  CARD32 val;

  val = Scr.CurrentDesk;

  if (val < 0 || (val >= ewmhc.MaxDesktops && ewmhc.MaxDesktops != 0))
    return;

  if (val >= ewmhc.CurrentNumberOfDesktops ||
      (ewmhc.NumberOfDesktops != ewmhc.CurrentNumberOfDesktops &&
       val < ewmhc.CurrentNumberOfDesktops))
    EWMH_SetNumberOfDesktops();

  ewmh_ChangeProperty(Scr.Root,"_NET_CURRENT_DESKTOP",
		      EWMH_ATOM_LIST_CLIENT_ROOT,
		      (unsigned char *)&val, 1);
}

void EWMH_SetNumberOfDesktops(void)
{
  CARD32 val;

  if (ewmhc.CurrentNumberOfDesktops < ewmhc.NumberOfDesktops)
    ewmhc.CurrentNumberOfDesktops = ewmhc.NumberOfDesktops;

  if (ewmhc.CurrentNumberOfDesktops > ewmhc.NumberOfDesktops ||
      ewmhc.NeedsToCheckDesk)
  {
    int d = check_desk();

    ewmhc.NeedsToCheckDesk = False;
    if (d >= ewmhc.MaxDesktops && ewmhc.MaxDesktops != 0)
      d = 0;
    ewmhc.CurrentNumberOfDesktops = max(ewmhc.NumberOfDesktops, d+1);
  }

  if (Scr.CurrentDesk >= ewmhc.CurrentNumberOfDesktops &&
      (Scr.CurrentDesk < ewmhc.MaxDesktops || ewmhc.MaxDesktops == 0))
    ewmhc.CurrentNumberOfDesktops = Scr.CurrentDesk + 1;

  val = (CARD32)ewmhc.CurrentNumberOfDesktops;
  ewmh_ChangeProperty(Scr.Root, "_NET_NUMBER_OF_DESKTOPS",
		      EWMH_ATOM_LIST_CLIENT_ROOT,
		      (unsigned char *)&val, 1);
  ewmh_SetWorkArea();
}

/* ************************************************************************* *
 *  client_win: here the client is fvwm2
 * ************************************************************************* */
void EWMH_SetActiveWindow(Window w)
{
  ewmh_ChangeProperty(Scr.Root, "_NET_ACTIVE_WINDOW", EWMH_ATOM_LIST_CLIENT_WIN,
		      (unsigned char *)&w, 1);
}

void EWMH_SetWMDesktop(FvwmWindow *fwin)
{
  CARD32 desk = fwin->Desk;
  
  if (IS_STICKY(fwin))
  {
    desk = 0xFFFFFFFE;
  }
  else if (desk >= ewmhc.CurrentNumberOfDesktops)
  {
    ewmhc.NeedsToCheckDesk = True;
    EWMH_SetNumberOfDesktops();
  }
  ewmh_ChangeProperty(fwin->w, "_NET_WM_DESKTOP", EWMH_ATOM_LIST_CLIENT_WIN,
		      (unsigned char *)&desk, 1);
}

/* ************************************************************************* *
 *  fvwm2 must maintain the _NET_WM_STATE
 * ************************************************************************* */

void EWMH_SetWMState(FvwmWindow *fwin)
{
  Atom wm_state[EWMH_NUMBER_OF_STATE];
  int i = 0;
  ewmh_atom *list = ewmh_atom_wm_state;

  while(list->name != NULL)
  {
    if (list->action(fwin, NULL, NULL))
      wm_state[i++] = list->atom;
    list++;
  }

  if (i > 0)
    ewmh_ChangeProperty(fwin->w, "_NET_WM_STATE", EWMH_ATOM_LIST_CLIENT_WIN,
			(unsigned char *)wm_state, i);
  else
    ewmh_DeleteProperty(fwin->w, "_NET_WM_STATE", EWMH_ATOM_LIST_CLIENT_WIN);
}

/* ************************************************************************* *
 *  fvwm_root
 * ************************************************************************* */

/*** kde system tray ***/

void add_kst_item(Window w)
{
  KstItem *t,**prev;

  t = ewmh_KstWinList;
  prev = &ewmh_KstWinList;

  while(t != NULL) {
    prev = &(t->next);
    t = t->next;
  }

  *prev = (KstItem *)safemalloc(sizeof(KstItem));
  (*prev)->w = w;
  (*prev)->next = NULL;
}

void delete_kst_item(Window w)
{
  KstItem *t,**prev;

  t = ewmh_KstWinList;
  prev = &ewmh_KstWinList;
  while((t!= NULL)&&(t->w != w))
  {
    prev = &(t->next);
    t = t->next;
  }

  if (t == NULL)
    return;

  if(prev != NULL)
    *prev = t->next;

  free(t);
}

void set_kde_sys_tray(void)
{
  Window *wins = NULL;
  KstItem *t;
  int i = 0, nbr = 0;

  t = ewmh_KstWinList;
  while(t != NULL) {
    nbr++;
    t = t->next;
  }

  if (nbr > 0)
    wins = (Window *)safemalloc(sizeof(Window) * nbr);

  t = ewmh_KstWinList;
  while (t != NULL)
  {
    wins[i++] = t->w;
    t = t->next;
  }

  ewmh_ChangeProperty(Scr.Root,"_KDE_NET_SYSTEM_TRAY_WINDOWS",
		      EWMH_ATOM_LIST_FVWM_ROOT,
		      (unsigned char *)wins,i);
  if (wins != NULL)
    free(wins);
}

void ewmh_AddToKdeSysTray(FvwmWindow *fwin)
{
  unsigned int size = 0;
  Atom *val;
  KstItem *t;

  val = ewmh_AtomGetByName(fwin->w, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR",
			   EWMH_ATOM_LIST_FIXED_PROPERTY, &size);

  if (val == NULL)
    return;
  free(val);

  t = ewmh_KstWinList;
  while(t != NULL && t->w != fwin->w)
    t = t->next;

  if (t != NULL)
    return; /* already in the list */

  add_kst_item(fwin->w);
  set_kde_sys_tray();
}

#if 0
/* not used at present time */
void ewmh_FreeKdeSysTray(void)
{
  KstItem *t;

  t = ewmh_KstWinList;
  while(t != NULL)
  {
    XSelectInput(dpy, t->w, NoEventMask);
    delete_kst_item(t->w);
    t = ewmh_KstWinList;
  }
  set_kde_sys_tray();
}
#endif

int EWMH_IsKdeSysTrayWindow(Window w)
{
  KstItem *t;

  t = ewmh_KstWinList;
  while(t != NULL && t->w != w)
    t = t->next;

  if (t == NULL)
    return 0;
  return 1;
}

void EWMH_ManageKdeSysTray(Window w, Bool is_destroy)
{
  KstItem *t;

  t = ewmh_KstWinList;
  while(t != NULL && t->w != w)
    t = t->next;

  if (t == NULL)
    return;

  if (!is_destroy)
  {
    XSelectInput(dpy, w, StructureNotifyMask);
  }
  else
  {
    XSelectInput(dpy, t->w, NoEventMask);
    delete_kst_item(w);
    set_kde_sys_tray();
  }
}

/**** Client lists ****/

void EWMH_SetClientList()
{
  Window *wl = NULL;
  FvwmWindow *t;
  int nbr = 0;
  int i = 0;

  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    nbr++;
  if (nbr != 0)
  {
    wl = (Window *)safemalloc(sizeof (Window) * nbr);
    for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
      wl[i++] = t->w;
  }
  ewmh_ChangeProperty(Scr.Root,"_NET_CLIENT_LIST", EWMH_ATOM_LIST_FVWM_ROOT,
		      (unsigned char *)wl, nbr);
  if (wl != NULL)
    free (wl);
}

void EWMH_SetClientListStacking()
{
  Window *wl = NULL;
  FvwmWindow *t;
  int nbr = 0;
  int i = 0;

  for (t = Scr.FvwmRoot.stack_next; t != &Scr.FvwmRoot; t = t->stack_next)
    nbr++;
  if (nbr != 0)
  {
    wl = (Window *)safemalloc(sizeof (Window) * nbr);
    for (t = Scr.FvwmRoot.stack_next; t != &Scr.FvwmRoot; t = t->stack_next)
      wl[i++] = t->w;
  }
  ewmh_ChangeProperty(Scr.Root,"_NET_CLIENT_LIST_STACKING",
		      EWMH_ATOM_LIST_FVWM_ROOT, (unsigned char *)wl, i);
  if (wl != NULL)
    free (wl);
}

/**** Working Area stuff ****/

void ewmh_SetWorkArea(void)
{
  CARD32 val[256][4]; /* no more than 256 desktops */
  int i = 0;

  while(i < ewmhc.NumberOfDesktops && i < 256)
  {
    val[i][0] = Scr.work_area.x;
    val[i][1] = Scr.work_area.y;
    val[i][2] = Scr.work_area.width;
    val[i][3] = Scr.work_area.height;
    i++;
  }
  ewmh_ChangeProperty(Scr.Root, "_NET_WORKAREA", EWMH_ATOM_LIST_FVWM_ROOT,
		      (unsigned char *)&val, i*4);
}

void ewmh_ComputeAndSetWorkArea(void)
{
  int left = ewmhc.BaseStrut.left;
  int right = ewmhc.BaseStrut.right;
  int top = ewmhc.BaseStrut.top;
  int bottom = ewmhc.BaseStrut.bottom;
  int x,y,width,height;
  FvwmWindow *t;

  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    left = max(left, t->strut.left);
    right = max(right, t->strut.right);
    top = max(top, t->strut.top);
    bottom = max(bottom, t->strut.bottom);
  }

  x = left;
  y = top;
  width = Scr.MyDisplayWidth - (left + right);
  height = Scr.MyDisplayHeight - (top + bottom);
  
  if (Scr.work_area.x != x || Scr.work_area.y != y ||
      Scr.work_area.width != width || Scr.work_area.height != height)
  {
    Scr.work_area.x = x;
    Scr.work_area.y = y;
    Scr.work_area.width = width; 
    Scr.work_area.height = height;
    ewmh_SetWorkArea();
  }

}

void ewmh_HandleDynamicWorkArea(void)
{
  int dyn_left = ewmhc.BaseStrut.left;
  int dyn_right = ewmhc.BaseStrut.right;
  int dyn_top = ewmhc.BaseStrut.top;
  int dyn_bottom = ewmhc.BaseStrut.bottom;
  int x,y,width,height;
  FvwmWindow *t;

  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    dyn_left = max(dyn_left, t->dyn_strut.left);
    dyn_right = max(dyn_right, t->dyn_strut.right);
    dyn_top = max(dyn_top, t->dyn_strut.top);
    dyn_bottom = max(dyn_bottom, t->dyn_strut.bottom);
  }

  x = dyn_left;
  y = dyn_top;
  width = Scr.MyDisplayWidth - (dyn_left + dyn_right);
  height = Scr.MyDisplayHeight - (dyn_top + dyn_bottom);
  
  if (Scr.dyn_work_area.x != x || Scr.dyn_work_area.y != y ||
      Scr.dyn_work_area.width != width || Scr.dyn_work_area.height != height)
  {
    Scr.dyn_work_area.x = x;
    Scr.dyn_work_area.y = y;
    Scr.dyn_work_area.width = width; 
    Scr.dyn_work_area.height = height;
    /* here we may update the maximized window ...etc */
  }
}

void EWMH_GetWorkAreaIntersection(FvwmWindow *fwin,
				  int *x, int *y, int *w, int *h, int func)
{
  int nx,ny,nw,nh;
  int area_x = Scr.work_area.x;
  int area_y = Scr.work_area.y;
  int area_w = Scr.work_area.width;
  int area_h = Scr.work_area.height;
  Bool is_dynamic = False;

  switch(func)
  {
    case F_MAXIMIZE:
      is_dynamic = True;
      break;
    default:
      break;
  }

  if (is_dynamic)
  {
    area_x = Scr.dyn_work_area.x;
    area_y = Scr.dyn_work_area.y;
    area_w = Scr.dyn_work_area.width;
    area_h = Scr.dyn_work_area.height;
  }

  nx = max(*x, area_x);
  ny = max(*y, area_y);
  nw = min(*x + *w, area_x + area_w) - nx;
  nh = min(*y + *h, area_y + area_h) - ny;

  *x = nx;
  *y = ny;
  *w = nw;
  *h = nh;
}

/* ************************************************************************* *
 *  fvwm_win
 * ************************************************************************* */
void EWMH_SetFrameStrut(FvwmWindow *fwin)
{
  CARD32 val[4];
  int border_width = fwin->boundary_width;

  /* left */
  val[0] = border_width;
  /* right */
  val[1] = border_width;
  /* top */
  val[2] = border_width + 
    ((HAS_TITLE(fwin) && !HAS_BOTTOM_TITLE(fwin))? fwin->title_g.height : 0);
  /* bottom */
  val[3] = border_width +
    ((HAS_TITLE(fwin) && HAS_BOTTOM_TITLE(fwin))? fwin->title_g.height : 0);

  ewmh_ChangeProperty(fwin->w, "_KDE_NET_WM_FRAME_STRUT",
		      EWMH_ATOM_LIST_FVWM_WIN, (unsigned char *)&val, 4);
}

/* ************************************************************************* *
 * Window types
 * ************************************************************************* */

int ewmh_HandleDesktop(EWMH_CMD_ARGS)
{
  if (Scr.EwmhDesktop != NULL && Scr.EwmhDesktop->w != fwin->w)
  {
    fvwm_msg(WARN,"ewmh_HandleDesktop",
	"An Desktop application (0x%lx) already run! This can cause problem\n",
	 Scr.EwmhDesktop->w);
    /* what to do ? */
  }

  Scr.EwmhDesktop = fwin;

  SSET_LAYER(*style, 0);
  style->flags.use_layer = 1;
  style->flag_mask.use_layer = 1;

  SFSET_IS_STICKY(*style, 1);
  SMSET_IS_STICKY(*style, 1);
  SCSET_IS_STICKY(*style, 1);

  SFSET_IS_FIXED(*style, 1);
  SMSET_IS_FIXED(*style, 1);
  SCSET_IS_FIXED(*style, 1);

  SFSET_DO_WINDOW_LIST_SKIP(*style, 1);
  SMSET_DO_WINDOW_LIST_SKIP(*style, 1);
  SCSET_DO_WINDOW_LIST_SKIP(*style, 1);

  SFSET_DO_CIRCULATE_SKIP(*style, 1);
  SMSET_DO_CIRCULATE_SKIP(*style, 1);
  SCSET_DO_CIRCULATE_SKIP(*style, 1);

  /* No border */
  SSET_BORDER_WIDTH(*style, 0);
  style->flags.has_border_width = 1;
  style->flag_mask.has_border_width = 1;
  style->change_mask.has_border_width = 1;

  SSET_HANDLE_WIDTH(*style, 0);
  style->flags.has_handle_width = 1;
  style->flag_mask.has_handle_width = 1;
  style->change_mask.has_handle_width = 1;

  /* no title */
  style->flags.has_no_title = 1;
  style->flag_mask.has_no_title = 1;
  style->change_mask.has_no_title = 1;

  /* ClickToFocus, I do not think we should use NeverFocus */
  SFSET_FOCUS_MODE(*style, FOCUS_CLICK);
  SMSET_FOCUS_MODE(*style, FOCUS_MASK);
  SCSET_FOCUS_MODE(*style, FOCUS_MASK);
  SFSET_DO_GRAB_FOCUS_WHEN_CREATED(*style, 1);
  SMSET_DO_GRAB_FOCUS_WHEN_CREATED(*style, 1);
  SCSET_DO_GRAB_FOCUS_WHEN_CREATED(*style, 1);

  /* ClickToFocusPassesClick */
  SFSET_DO_NOT_PASS_CLICK_FOCUS_CLICK(*style, 0);
  SMSET_DO_NOT_PASS_CLICK_FOCUS_CLICK(*style, 1);
  SCSET_DO_NOT_PASS_CLICK_FOCUS_CLICK(*style, 1);

  /* not useful */
  SFSET_DO_NOT_RAISE_CLICK_FOCUS_CLICK(*style, 1);
  SMSET_DO_NOT_RAISE_CLICK_FOCUS_CLICK(*style, 1);
  SCSET_DO_NOT_RAISE_CLICK_FOCUS_CLICK(*style, 1);

  return 1;
}

int ewmh_HandleDialog(EWMH_CMD_ARGS)
{
  return 0;
}

int ewmh_HandleDock(EWMH_CMD_ARGS)
{
  SFSET_IS_STICKY(*style, 1);
  SMSET_IS_STICKY(*style, 1);
  SCSET_IS_STICKY(*style, 1);

  SFSET_DO_WINDOW_LIST_SKIP(*style, 1);
  SMSET_DO_WINDOW_LIST_SKIP(*style, 1);
  SCSET_DO_WINDOW_LIST_SKIP(*style, 1);

  SFSET_DO_CIRCULATE_SKIP(*style, 1);
  SMSET_DO_CIRCULATE_SKIP(*style, 1);
  SCSET_DO_CIRCULATE_SKIP(*style, 1);

  /* no title ? MWM hints should be used by the app but ... */

  return 1;
}

int ewmh_HandleMenu(EWMH_CMD_ARGS)
{
  SFSET_IS_STICKY(*style, 1);
  SMSET_IS_STICKY(*style, 1);
  SCSET_IS_STICKY(*style, 1);

  SFSET_DO_WINDOW_LIST_SKIP(*style, 1);
  SMSET_DO_WINDOW_LIST_SKIP(*style, 1);
  SCSET_DO_WINDOW_LIST_SKIP(*style, 1);

  SFSET_DO_CIRCULATE_SKIP(*style, 1);
  SMSET_DO_CIRCULATE_SKIP(*style, 1);
  SCSET_DO_CIRCULATE_SKIP(*style, 1);

  /* no title */
  style->flags.has_no_title = 1;
  style->flag_mask.has_no_title = 1;
  style->change_mask.has_no_title = 1;
  
  SSET_BORDER_WIDTH(*style, 0);
  style->flags.has_border_width = 1;
  style->flag_mask.has_border_width = 1;
  style->change_mask.has_border_width = 1;

  SSET_HANDLE_WIDTH(*style, 0);
  style->flags.has_handle_width = 1;
  style->flag_mask.has_handle_width = 1;
  style->change_mask.has_handle_width = 1;

  return 1;
}

int ewmh_HandleNormal(EWMH_CMD_ARGS)
{
  return 0;
}

int ewmh_HandleToolBar(EWMH_CMD_ARGS)
{
  SFSET_IS_STICKY(*style, 1);
  SMSET_IS_STICKY(*style, 1);
  SCSET_IS_STICKY(*style, 1);

  SFSET_DO_WINDOW_LIST_SKIP(*style, 1);
  SMSET_DO_WINDOW_LIST_SKIP(*style, 1);
  SCSET_DO_WINDOW_LIST_SKIP(*style, 1);

  SFSET_DO_CIRCULATE_SKIP(*style, 1);
  SMSET_DO_CIRCULATE_SKIP(*style, 1);
  SCSET_DO_CIRCULATE_SKIP(*style, 1);

  /* no title ? MWM hints should be used by the app but ... */
  
  return 1;
}

void ewmh_HandleWindowType(FvwmWindow *fwin, window_style *style)
{
  Atom *val;
  ewmh_atom *list = ewmh_atom_window_type;
  unsigned int size = 0;

  fwin->ewmh_window_type = 0;
  val = ewmh_AtomGetByName(fwin->w, "_NET_WM_WINDOW_TYPE",
			   EWMH_ATOM_LIST_FIXED_PROPERTY, &size);

  if (val == NULL)
    return;

  /* we support only one window type: the first one */
  while(list->name != NULL)
  {
    if (list->atom == val[0])
    {
      list->action(fwin, NULL, style);
      fwin->ewmh_window_type = list->atom;
    }
     list++;
  }
  free(val);
}

/* ************************************************************************* *
 * a workaround for ksmserver exit windows
 * ************************************************************************* */
int ksmserver_workarround(FvwmWindow *fwin)
{

  if (fwin->name != NULL && fwin->class.res_name != NULL &&
      fwin->icon_name != NULL && fwin->class.res_class != NULL &&
      strcmp(fwin->name,"ksmserver") == 0 &&
      strcmp(fwin->class.res_class,"ksmserver") == 0 &&
      strcmp(fwin->icon_name,"ksmserver") == 0 &&
      strcmp(fwin->class.res_name,"unnamed") == 0)
  {
    int layer = 0;
    if (IS_TRANSIENT(fwin))
      layer = Scr.TopLayer + 2;
    else
      layer = Scr.TopLayer + 1;
    
    new_layer(fwin, layer);
    return 1;
  }
  return 0;
}

/* ************************************************************************* *
 * Window Initialisation / Destroy
 * ************************************************************************* */

void EWMH_GetStyle(FvwmWindow *fwin, window_style *style)
{
  ewmh_HandleWindowType(fwin, style);
  ewmh_WMState(fwin, NULL, style);
}

/* see also EWMH_WMName and EWMH_WMIconName in add_window */
void EWMH_WindowInit(FvwmWindow *fwin)
{
  EWMH_DLOG("Init window 0x%lx",fwin->w);
  EWMH_SetWMState(fwin);
  EWMH_SetWMDesktop(fwin);
  EWMH_SetFrameStrut(fwin);
  ewmh_WMStrut(fwin, NULL, NULL);
  ewmh_WMIconGeometry(fwin, NULL, NULL);
  ewmh_AddToKdeSysTray(fwin);
  if (IS_EWMH_DESKTOP(fwin->w))
    return;
  if (ksmserver_workarround(fwin))
    return;
  ewmh_WMIcon(fwin, NULL, NULL);
  EWMH_DLOG("window 0x%lx initialised",fwin->w);
}

/* a window are going to be destroyed */
void EWMH_DestroyWindow(FvwmWindow *fwin)
{
  if (IS_EWMH_DESKTOP(fwin->w))
    Scr.EwmhDesktop = NULL;
  if (fwin->Desk >= ewmhc.NumberOfDesktops)
    ewmhc.NeedsToCheckDesk = True;
}

/* a window has been destroyed */
void EWMH_WindowDestroyed(void)
{
  EWMH_SetClientList();
  if (ewmhc.NeedsToCheckDesk)
    EWMH_SetNumberOfDesktops();
  ewmh_ComputeAndSetWorkArea();
  ewmh_HandleDynamicWorkArea();
}

/* ************************************************************************* *
 * Init Stuff 
 * ************************************************************************* */

int set_all_atom_in_list(ewmh_atom *list)
{
  int l = 0;

  while(list->name != NULL)
  {
    list->atom = XInternAtom(dpy,list->name,False);
    if (list->atom_type == None)
      list->atom_type = XA_UTF8_STRING;
    l++;
    list++;
  }
  return l;
}


void set_net_supported(int l)
{
  Atom *supported;
  int i, k = 0;

  supported = (Atom *)safemalloc(l*sizeof(Atom));
  for(i=0; i < NUMBER_OF_ATOM_LISTS; i++)
  {
    ewmh_atom *list = atom_list[i].list;
    while(list->name != NULL)
    {
      supported[k++] = list->atom;
      list++;
    }
  }

  ewmh_ChangeProperty(Scr.Root, "_NET_SUPPORTED", EWMH_ATOM_LIST_FVWM_ROOT,
		      (unsigned char *)supported, k);
  free(supported);
}

void clean_up(void)
{
  ewmh_ChangeProperty(Scr.Root,"_KDE_NET_SYSTEM_TRAY_WINDOWS",
		      EWMH_ATOM_LIST_FVWM_ROOT, NULL, 0);
}

void EWMH_Init(void)
{
  int i;
  int supported_count = 0;
  CARD32 val;
  XTextProperty text;
  CARD32 utf_name[5];
  char *names[1];

  /* initialisation of all the atoms */
  XA_UTF8_STRING = XInternAtom(dpy,"UTF8_STRING",False);
  for(i=0; i < NUMBER_OF_ATOM_LISTS; i++)
  {
    supported_count += set_all_atom_in_list(atom_list[i].list);
  }

  /* the laws that we respect */
  set_net_supported(supported_count);

  /*  use the Scr.NoFocusWin as the WM_CHECK window */
  val = Scr.NoFocusWin;
  ewmh_ChangeProperty(Scr.Root, "_NET_SUPPORTING_WM_CHECK",
		      EWMH_ATOM_LIST_FVWM_ROOT, (unsigned char *)&val, 1);
  ewmh_ChangeProperty(Scr.NoFocusWin, "_NET_SUPPORTING_WM_CHECK",
		      EWMH_ATOM_LIST_FVWM_ROOT, (unsigned char *)&val, 1);

  names[0] = "FVWM";
  if (XStringListToTextProperty(names, 1, &text))
  {
    XSetWMName(dpy, Scr.NoFocusWin, &text);
    XFree(text.value);
  }

  /* FVWM in UTF8 */
  utf_name[0] = 0x46;
  utf_name[1] = 0x56;
  utf_name[2] = 0x57;
  utf_name[3] = 0x4D;
  utf_name[4] = 0x0;

  ewmh_ChangeProperty(Scr.NoFocusWin, "_NET_WM_NAME",
		      EWMH_ATOM_LIST_PROPERTY_NOTIFY,
		      (unsigned char *)&utf_name, 5);

  clean_up();

  EWMH_SetCurrentDesktop();
  EWMH_SetNumberOfDesktops();
  EWMH_SetClientList();
  EWMH_SetClientListStacking();
}

/* ************************************************************************* *
 * Exit Stuff 
 * ************************************************************************* */
void EWMH_ExitStuff(void)
{
  FvwmWindow *t;

  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    if (HAS_EWMH_WM_ICON_HINT(t) == EWMH_FVWM_ICON)
      EWMH_DeleteWmIcon(t, True, True);
  }

}

#ifdef EWMH_DEBUG
void EWMH_DLOG(char *msg, ...)
{
  va_list args;
  clock_t time_val, time_taken;
  static clock_t start_time = 0;
  static clock_t prev_time = 0;
  struct tms not_used_tms;
  char buffer[200];                     /* oversized */
  time_t mytime;
  struct tm *t_ptr;

  time(&mytime);
  t_ptr = localtime(&mytime);
  if (start_time == 0) {
    prev_time = start_time =
      (unsigned int)times(&not_used_tms); /* get clock ticks */
  }
  time_val = (unsigned int)times(&not_used_tms); /* get clock ticks */
  time_taken = time_val - prev_time;
  prev_time = time_val;
  sprintf(buffer, "%.2d:%.2d:%.2d %6ld",
          t_ptr->tm_hour, t_ptr->tm_min, t_ptr->tm_sec, time_taken);

  fprintf(stderr,"EWMH DEBUG: ");
  va_start(args,msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fprintf(stderr,"\n");
  fprintf(stderr,"            [time]: %s\n",buffer);
}
#endif

#endif /* HAVE_EWMH */
