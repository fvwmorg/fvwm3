/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

/*****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/


/***********************************************************************
 *
 * fvwm menu code
 *
 ***********************************************************************/
/* #define FVWM_DEBUG_MSGS */
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <X11/keysym.h>

#include <libs/fvwmlib.h>
#include "fvwm.h"
#include "externs.h"
#include "events.h"
#include "menus.h"
#include "cursor.h"
#include "functions.h"
#include "repeat.h"
#include "borders.h"
#include "misc.h"
#include "screen.h"
#include "colors.h"
#include "colormaps.h"
#include "decorations.h"
#include "libs/Colorset.h"
#include "defaults.h"
#include "libs/FScreen.h"

/* IMPORTANT NOTE: Do *not* use any constant numbers in this file. All values
 * have to be #defined in the section below or defaults.h to ensure full
 * control over the menus. */

/* used in float to int arithmetic */
#define ROUNDING_ERROR_TOLERANCE  0.005


/***************************************************************
 * typedefs
 ***************************************************************/

typedef struct
{
  unsigned int keystate;
  unsigned int keycode;
  Time timestamp;
} double_keypress;

/* patch to pass the last popups position hints to popup_func */
typedef struct
{
  struct
  {
    unsigned is_last_menu_pos_hints_valid : 1;
    unsigned do_ignore_pos_hints : 1;
  } flags;
  MenuPosHints pos_hints;
} saved_pos_hints;

/***************************************************************
 * statics
 ***************************************************************/

/* This global is saved and restored every time a function is called that
 * might modify them, so we can safely let it live outside a function. */
static saved_pos_hints last_saved_pos_hints =
{
  { False, False },
  { 0, 0, 0.0, 0.0, 0, 0, False, False, False }
};

/***************************************************************
 * externals
 ***************************************************************/

/* This external is safe. It's written only during startup. */
extern XContext MenuContext;

/* We need to use this structure to make sure the other parts of fvwm work with
 * the most recent event. */
extern XEvent Event;

/* This one is only read, never written */
extern Time lastTimestamp;


/***************************************************************
 * forward declarations
 ***************************************************************/

static void draw_separator(Window, GC,GC,int, int,int,int,int);
static void draw_underline(MenuRoot *mr, GC gc, int x, int y, char *txt,
			   int coffset);
static void MenuInteraction(
  MenuParameters *pmp, MenuReturn *pmret, double_keypress *pdkp,
  Bool *pdo_warp_to_item);
static int pop_menu_up(
  MenuRoot **pmenu, MenuParameters *pmp, MenuRoot *parent_menu,
  MenuItem *parent_item,
  FvwmWindow **pfw, int *pcontext, int x, int y, Bool prefer_left_submenu,
  Bool do_warp_to_item, MenuOptions *pops, Bool *ret_overlap,
  Bool *pdo_warp_to_title, Window popdown_window);
static void pop_menu_down(MenuRoot **pmr, MenuParameters *pmp);
static void pop_menu_down_and_repaint_parent(
  MenuRoot **pmr, Bool *fSubmenuOverlaps, MenuParameters *pmp);
static void get_popup_options(
  MenuParameters *pmp, MenuItem *mi, MenuOptions *pops);
static void paint_item(MenuRoot *mr, MenuItem *mi, FvwmWindow *fw,
		       Bool do_redraw_menu_border);
static void paint_menu(MenuRoot *, XEvent *, FvwmWindow *fw);
static void select_menu_item(
  MenuRoot *mr, MenuItem *mi, Bool select, FvwmWindow *fw);
static MenuRoot *copy_menu_root(MenuRoot *mr);
static void make_menu(MenuRoot *mr, MenuParameters *pmp);
static void make_menu_window(MenuRoot *mr);
static void get_xy_from_position_hints(
  MenuPosHints *ph, int width, int height, int context_width,
  Bool do_reverse_x, int *ret_x, int *ret_y);
static MenuRoot *FindPopup(char *popup_name);
static void merge_continuation_menus(MenuRoot *mr);


/***************************************************************
 *
 * subroutines
 *
 ***************************************************************/

/***************************************************************
 * functions dealing with coordinates
 ***************************************************************/

static int item_middle_y_offset(MenuRoot *mr, MenuItem *mi)
{
  int r;
  if (!mi)
    return MST_BORDER_WIDTH(mr);
  r = (MI_IS_SELECTABLE(mi)) ? MST_RELIEF_THICKNESS(mr) : 0;
  return MI_Y_OFFSET(mi) + (MI_HEIGHT(mi) + r) / 2;
}

static int menu_middle_x_offset(MenuRoot *mr)
{
  return MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) / 2;
}

static Bool pointer_in_active_item_area(int x_offset, MenuRoot *mr)
{
  if (MST_USE_LEFT_SUBMENUS(mr))
  {
    return (x_offset <=
	    MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) -
	    MR_ITEM_WIDTH(mr) * MENU_POPUP_NOW_RATIO);
  }
  else
  {
    return (x_offset >=
	    MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) * MENU_POPUP_NOW_RATIO);
  }
}

static Bool pointer_in_passive_item_area(int x_offset, MenuRoot *mr)
{
  if (MST_USE_LEFT_SUBMENUS(mr))
  {
    return (x_offset >=
	    MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) * MENU_POPUP_NOW_RATIO);
  }
  else
  {
    return (x_offset <=
	    MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) -
	    MR_ITEM_WIDTH(mr) * MENU_POPUP_NOW_RATIO);
  }
}

static int get_left_popup_x_position(MenuRoot *mr, MenuRoot *submenu, int x)
{
  if (MST_USE_LEFT_SUBMENUS(mr))
  {
    return (x - MST_POPUP_OFFSET_ADD(mr) - MR_WIDTH(submenu) +
	    MR_WIDTH(mr) * (100 - MST_POPUP_OFFSET_PERCENT(mr))/100);
  }
  else
  {
    return (x - MR_WIDTH(submenu) + MST_BORDER_WIDTH(mr));
  }
}

static int get_right_popup_x_position(MenuRoot *mr, MenuRoot *submenu, int x)
{
  if (MST_USE_LEFT_SUBMENUS(mr))
  {
    return (x + MR_WIDTH(mr) - MST_BORDER_WIDTH(mr));
  }
  else
  {
    return (x + MR_WIDTH(mr) * MST_POPUP_OFFSET_PERCENT(mr) / 100 +
	    MST_POPUP_OFFSET_ADD(mr));
  }
}

static void get_prefered_popup_position(
  MenuRoot *mr, MenuRoot *submenu, int *px, int *py,
  Bool *pprefer_left_submenus)
{
  int menu_x, menu_y;
  MenuItem *mi = NULL;

  if (!XGetGeometry(dpy, MR_WINDOW(mr), &JunkRoot, &menu_x, &menu_y,
		    &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth))
  {
    *px = 0;
    *py = 0;
    *pprefer_left_submenus = False;
    fvwm_msg(ERR, "get_prefered_popup_position",
	     "can't get geometry of menu %s", MR_NAME(mr));
    return;
  }
  /* set the direction flag */
  *pprefer_left_submenus =
    (MR_HAS_POPPED_UP_LEFT(mr) ||
     (MST_USE_LEFT_SUBMENUS(mr) &&
      !MR_HAS_POPPED_UP_RIGHT(mr)));
  /* get the x position */
  if (*pprefer_left_submenus)
  {
    *px = get_left_popup_x_position(mr, submenu, menu_x);
  }
  else
  {
    *px = get_right_popup_x_position(mr, submenu, menu_x);
  }
  /* get the y position */
  if (MR_SELECTED_ITEM(mr))
  {
    mi = MR_SELECTED_ITEM(mr);
  }
  if (mi)
  {
    *py = menu_y + MI_Y_OFFSET(mi) -
      MST_BORDER_WIDTH(mr) + MST_RELIEF_THICKNESS(mr);
  }
  else
  {
    *py = menu_y;
  }
}


/***************************************************************
 * warping functions
 ***************************************************************/

static void warp_pointer_to_title(MenuRoot *mr)
{
  XWarpPointer(dpy, 0, MR_WINDOW(mr), 0, 0, 0, 0, menu_middle_x_offset(mr),
	       item_middle_y_offset(mr, MR_FIRST_ITEM(mr)));
}

static MenuItem *warp_pointer_to_item(MenuRoot *mr, MenuItem *mi,
				      Bool do_skip_title)
{
  if (do_skip_title)
  {
    while (MI_NEXT_ITEM(mi) != NULL &&
	   (MI_IS_SEPARATOR(mi) || MI_IS_TITLE(mi)))
    {
      /* skip separators and titles until the first 'real' item is found */
      mi = MI_NEXT_ITEM(mi);
    }
  }
  if (mi == NULL)
    mi = MR_LAST_ITEM(mr);
  if (mi == NULL)
    return mi;
  XWarpPointer(dpy, 0, MR_WINDOW(mr), 0, 0, 0, 0,
	       menu_middle_x_offset(mr), item_middle_y_offset(mr, mi));
  return mi;
}

/***************************************************************
 * menu animation functions
 ***************************************************************/

/* Undo the animation of a menu */
static void animated_move_back(MenuRoot *mr, Bool do_warp_pointer)
{
  int act_x;
  int act_y;

  if (MR_XANIMATION(mr) == 0)
    return;
  if (XGetGeometry(dpy, MR_WINDOW(mr), &JunkRoot, &act_x, &act_y,
		   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth))
  {
    /* move it back */
    AnimatedMoveOfWindow(
      MR_WINDOW(mr) ,act_x, act_y, act_x - MR_XANIMATION(mr), act_y,
      do_warp_pointer, -1, NULL);
    MR_XANIMATION(mr) = 0;
  }

  return;
}


/***************************************************************
 * functions dealing with menu items and indices
 ***************************************************************/

/***********************************************************************
 * find_entry()
 *
 * Returns the menu item the pointer is over and optionally the offset
 * from the left side of the menu entry (if px_offset is != NULL) and
 * the MenuRoot the pointer is over (if pmr is != NULL).
 ***********************************************************************/
static MenuItem *find_entry(
  int *px_offset /*NULL means don't return this value */,
  MenuRoot **pmr /*NULL means don't return this value */,
  /* values passed in from caller it XQueryPointer was already called there */
  Window p_child, int p_rx, int p_ry)
{
  MenuItem *mi;
  MenuRoot *mr;
  int root_x, root_y;
  int x, y;
  Window Child;
  int r;

  /* x_offset returns the x offset of the pointer in the found menu item */
  if (px_offset)
    *px_offset = 0;

  if (pmr)
    *pmr = NULL;

  if (p_rx < 0)
  {
    if (!XQueryPointer(dpy, Scr.Root, &JunkRoot, &Child,
		       &root_x, &root_y, &JunkX, &JunkY, &JunkMask))
    {
      return NULL;
    }
  }
  else
  {
    root_x = p_rx;
    root_y = p_ry;
    Child = p_child;
  }
  if (XFindContext(dpy, Child, MenuContext, (caddr_t *)&mr) == XCNOENT)
  {
    return NULL;
  }

  if (pmr)
    *pmr = mr;
  /* now get position in that child window */
  if (!XTranslateCoordinates(
	dpy, Scr.Root, Child, root_x, root_y, &x, &y, &JunkChild))
  {
    return NULL;
  }

  r = MST_RELIEF_THICKNESS(mr);
  /* look for the entry that the mouse is in */
  for (mi = MR_FIRST_ITEM(mr); mi; mi = MI_NEXT_ITEM(mi))
  {
    int a;
    int b;

    a = (MI_PREV_ITEM(mi) && MI_IS_SELECTABLE(MI_PREV_ITEM(mi))) ? r / 2 : 0;
    if (!MI_IS_SELECTABLE(mi))
      b = 0;
    else if (MI_NEXT_ITEM(mi) && MI_IS_SELECTABLE(MI_NEXT_ITEM(mi)))
      b = r / 2;
    else
      b = r;
    if (y >= MI_Y_OFFSET(mi) - a &&
	y < MI_Y_OFFSET(mi) + MI_HEIGHT(mi) + b)
    {
      break;
    }
  }
  if (x < MR_ITEM_X_OFFSET(mr) ||
      x >= MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) - 1)
  {
    mi = NULL;
  }

  if (mi && px_offset)
    *px_offset = x;

  return mi;
}


/***************************************************************
 * Returns the position of the item in the menu, but counts
 * only items that can be selected (i.e. nor separators or
 * titles). The count begins with 0.
 ***************************************************************/
static int get_selectable_item_index(
  MenuRoot *mr, MenuItem *miTarget, int *ret_sections)
{
  int i = 0;
  int s = 0;
  MenuItem *mi;
  Bool last_selectable = False;

  for (mi = MR_FIRST_ITEM(mr); mi && mi != miTarget; mi = MI_NEXT_ITEM(mi))
  {
    if (!MI_IS_SEPARATOR(mi) && !MI_IS_TITLE(mi))
    {
      i++;
      last_selectable = True;
    }
    else if (last_selectable)
    {
      s++;
      last_selectable = False;
    }
  }
  if (ret_sections)
  {
    *ret_sections = s;
  }
  if (mi == miTarget)
  {
    return i;
  }
  return -1;
}

static MenuItem *get_selectable_item_from_index(MenuRoot *mr, int index)
{
  int i = -1;
  MenuItem *mi;
  MenuItem *miLastOk = NULL;

  for (mi = MR_FIRST_ITEM(mr); mi && (i < index || miLastOk == NULL);
       mi=MI_NEXT_ITEM(mi))
  {
    if (!MI_IS_SEPARATOR(mi) && !MI_IS_TITLE(mi))
    {
      miLastOk = mi;
      i++;
    }
  }
  return miLastOk;
}

static MenuItem *get_selectable_item_from_section(MenuRoot *mr, int section)
{
  int i = 0;
  MenuItem *mi;
  MenuItem *miLastOk = NULL;
  Bool last_selectable = False;

  for (mi = MR_FIRST_ITEM(mr); mi && (i <= section || miLastOk == NULL);
       mi=MI_NEXT_ITEM(mi))
  {
    if (!MI_IS_SEPARATOR(mi) && !MI_IS_TITLE(mi))
    {
      if (!last_selectable)
      {
	miLastOk = mi;
	last_selectable = True;
      }
    }
    else if (last_selectable)
    {
      i++;
      last_selectable = False;
    }
  }
  return miLastOk;
}

static int get_selectable_item_count(MenuRoot *mr, int *ret_sections)
{
  int count;

  count = get_selectable_item_index(mr, MR_LAST_ITEM(mr), ret_sections);
  if (MR_LAST_ITEM(mr) && MI_IS_SELECTABLE(MR_LAST_ITEM(mr)))
    count++;
  return count;
}



/***************************************************************
 * functions dealing with submenus
 ***************************************************************/

/* Search for a submenu that was popped up by the given item in the given
 * instance of the menu. */
static MenuRoot *seek_submenu_instance(MenuRoot *parent_menu,
				       MenuItem *parent_item)
{
  MenuRoot *mr;

  for (mr = Menus.all; mr != NULL; mr = MR_NEXT_MENU(mr))
  {
    if (MR_PARENT_MENU(mr) == parent_menu &&
	MR_PARENT_ITEM(mr) == parent_item)
    {
      /* here it is */
      break;
    }
  }
  return mr;
}

static Bool is_submenu_mapped(MenuRoot *parent_menu, MenuItem *parent_item)
{
  XWindowAttributes win_attribs;
  MenuRoot *mr;

  mr = seek_submenu_instance(parent_menu, parent_item);
  if (mr == NULL)
    return False;

  if (MR_WINDOW(mr) == None)
    return False;
  if (!XGetWindowAttributes(dpy, MR_WINDOW(mr), &win_attribs))
    return False;
  return (win_attribs.map_state == IsViewable);
}

/* Returns the menu root that a given menu item pops up */
static MenuRoot *mr_popup_for_mi(MenuRoot *mr, MenuItem *mi)
{
  char *menu_name;
  MenuRoot *menu = NULL;

  /* This checks if mi is != NULL too */
  if (!mi || !MI_IS_POPUP(mi))
    return NULL;

  /* first look for a menu that is aleady mapped */
  menu = seek_submenu_instance(mr, mi);
  if (menu)
    return menu;

  /* just look past "Popup " in the action, and find that menu root */
  menu_name = PeekToken(SkipNTokens(MI_ACTION(mi), 1), NULL);
  menu = FindPopup(menu_name);
  return menu;
}

/* FindPopup expects a token as the input. Make sure you have used
 * GetNextToken before passing a menu name to remove quotes (if necessary) */
static MenuRoot *FindPopup(char *popup_name)
{
  MenuRoot *mr;

  if (popup_name == NULL)
    return NULL;

  mr = Menus.all;
  while (mr != NULL)
  {
    if (MR_NAME(mr) != NULL)
    {
      if (mr == MR_ORIGINAL_MENU(mr) && StrEquals(popup_name, MR_NAME(mr)))
      {
	return mr;
      }
    }
    mr = MR_NEXT_MENU(mr);
  }
  return NULL;

}


/***************************************************************
 * functions
 ***************************************************************/

/* Make sure the menu is properly rebuilt when the style or the menu has
 * changed. */
static void update_menu(MenuRoot *mr, MenuParameters *pmp)
{
  int sw;
  int sh;
  Bool has_screen_size_changed = False;
  fscreen_scr_arg fscr;

  if (MST_IS_UPDATED(mr))
  {
    /* The menu style has changed. */
    MenuRoot *menu;

    for (menu = Menus.all; menu; menu = MR_NEXT_MENU(menu))
    {
      if (MR_STYLE(menu) == MR_STYLE(mr))
      {
	/* Mark all other menus with the same style as changed. */
	MR_IS_UPDATED(menu) = 1;
      }
    }
    MST_IS_UPDATED(mr) = 0;
  }
  fscr.xypos.x = pmp->screen_origin_x;
  fscr.xypos.y = pmp->screen_origin_y;
  FScreenGetScrRect(&fscr, FSCREEN_XYPOS, &JunkX, &JunkY, &sw, &sh);
  if (sw != MR_SCREEN_WIDTH(mr) || sh != MR_SCREEN_HEIGHT(mr))
  {
    has_screen_size_changed = True;
    MR_SCREEN_WIDTH(mr) = (unsigned short)sw;
    MR_SCREEN_HEIGHT(mr) = (unsigned short)sh;
  }
  if (MR_IS_UPDATED(mr) || has_screen_size_changed)
  {
    /* The menu or the screen dimensions have changed. We have to re-make it. */
    make_menu(mr, pmp);
  }

  return;
}

/****************************************************************************
 *
 * Initiates a menu pop-up
 *
 * fStick = True  = sticky menu, stays up on initial button release.
 * fStick = False = transient menu, drops on initial release.
 *
 * eventp = 0: menu opened by mouse, do not warp
 * eventp > 1: root menu opened by keypress with 'Menu', warp pointer and
 *	       allow 'double-keypress'.
 * eventp = 1: menu opened by keypress, warp but forbid 'double-keypress'
 *	       this should always be used except in the call in 'staysup_func'
 *	       in builtin.c
 *
 * Returns one of MENU_NOP, MENU_ERROR, MENU_ABORTED, MENU_DONE
 ***************************************************************************/
void do_menu(MenuParameters *pmp, MenuReturn *pmret)
{
  extern XEvent Event;

  int x;
  int y;
  Bool fFailedPopup = False;
  Bool fWasAlreadyPopped = False;
  Bool key_press;
  Time t0 = lastTimestamp;
  XEvent tmpevent;
  double_keypress dkp;
  /* must be saved before launching parallel menus (by using the dynamic
   * actions). */
  static Bool do_warp_to_title = False;
  /* don't save these ones, we want them to work even within recursive menus
   * popped up by dynamic actions. */
  static int indirect_depth = 0;
  static int x_start;
  static int y_start;
  static Bool has_mouse_moved = False;
  int scr_x, scr_y, scr_w, scr_h;

  pmret->rc = MENU_NOP;
  if (pmp->flags.is_sticky && !pmp->flags.is_submenu)
  {
    XCheckTypedEvent(dpy, ButtonPressMask, &tmpevent);
  }
  if (pmp->menu == NULL)
  {
    pmret->rc = MENU_ERROR;
    return;
  }
  key_press = (pmp->eventp && (pmp->eventp == (XEvent *)1 ||
			       pmp->eventp->type == KeyPress));

  /* Try to pick a root-relative optimal x,y to
   * put the mouse right on the title w/o warping */
  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x, &y, &JunkX, &JunkY, &JunkMask);
  /* Save these-- we want to warp back here if this is a top level
     menu brought up by a keystroke */
  if (!pmp->flags.is_submenu)
  {
    pmp->flags.is_invoked_by_key_press = key_press;
    pmp->flags.is_first_root_menu = !indirect_depth;
  }
  else
  {
    pmp->flags.is_first_root_menu = 0;
  }
  if (!pmp->flags.is_submenu && indirect_depth == 0)
  {
    if (key_press)
    {
      x_start = x;
      y_start = y;
    }
    else
    {
      x_start = -1;
      y_start = -1;
    }
    pmp->screen_origin_x = pmp->pops->pos_hints.screen_origin_x;
    pmp->screen_origin_y = pmp->pops->pos_hints.screen_origin_y;
  }
  /* Figure out where we should popup, if possible */
  if (!pmp->flags.is_already_mapped)
  {
    Bool prefer_left_submenus = False;

    /* Make sure we are using the latest style and menu layout. */
    update_menu(pmp->menu, pmp);

    if (pmp->flags.is_submenu)
    {
      /* this is a submenu popup */
      get_prefered_popup_position(
	pmp->parent_menu, pmp->menu, &x, &y, &prefer_left_submenus);
    }
    else
    {
      fscreen_scr_arg fscr;

      /* we're a top level menu */
      has_mouse_moved = False;
      if(!GrabEm(CRS_MENU, GRAB_MENU))
      {
	/* GrabEm specifies the cursor to use */
	XBell(dpy, 0);
	pmret->rc = MENU_ABORTED;
	return;
      }
      /* Make the menu appear under the pointer rather than warping */
      fscr.xypos.x = x;
      fscr.xypos.y = y;
      FScreenGetScrRect(&fscr, FSCREEN_XYPOS, &scr_x, &scr_y, &scr_w, &scr_h);
      x -= menu_middle_x_offset(pmp->menu);
      y -= item_middle_y_offset(pmp->menu, MR_FIRST_ITEM(pmp->menu));
      if (x < scr_x)
      {
	x = scr_x;
      }
      if (y < scr_y)
      {
	y = scr_y;
      }
    }

    /* pop_menu_up may move the x,y to make it fit on screen more nicely */
    /* it might also move parent_menu out of the way */
    if (!pop_menu_up(
      &(pmp->menu), pmp, pmp->parent_menu, NULL, pmp->pTmp_win, pmp->pcontext,
      x, y, prefer_left_submenus, key_press /*warp*/, pmp->pops, NULL,
      &do_warp_to_title, None))
    {
      fFailedPopup = True;
      XBell(dpy, 0);
      UngrabEm(GRAB_MENU);
      pmret->rc = MENU_ERROR;
      return;
    }
  }
  else
  {
    fWasAlreadyPopped = True;
    if (key_press)
    {
      warp_pointer_to_item(pmp->menu, MR_FIRST_ITEM(pmp->menu),
			   True /* skip Title */);
    }
  }
  do_warp_to_title = False;

  /* Remember the key that popped up the root menu. */
  if (pmp->flags.is_submenu)
  {
    dkp.timestamp = 0;
  }
  else
  {
    if (pmp->eventp && pmp->eventp != (XEvent *)1)
    {
      /* we have a real key event */
      dkp.keystate = pmp->eventp->xkey.state;
      dkp.keycode = pmp->eventp->xkey.keycode;
    }
    dkp.timestamp = (key_press && pmp->flags.has_default_action) ? t0 : 0;
  }
  if(!pmp->flags.is_submenu && indirect_depth == 0)
  {
    /* we need to grab the keyboard so we are sure no key presses are lost */
    MyXGrabKeyboard(dpy);
  }
  if (!fFailedPopup)
  {
    if (!pmp->flags.is_submenu)
    {
      XSelectInput(dpy, Scr.NoFocusWin, XEVMASK_MENUNFW);
    }
    MenuInteraction(pmp, pmret, &dkp, &do_warp_to_title);
    if (!pmp->flags.is_submenu)
    {
      XSelectInput(dpy, Scr.NoFocusWin, XEVMASK_NOFOCUSW);
    }
  }
  else
  {
    pmret->rc = MENU_ABORTED;
  }
  if (!fWasAlreadyPopped)
  {
    /* popping down the menu may destroy the menu via the dynamic popdown
     * action! */
    pop_menu_down(&pmp->menu, pmp);
  }
  pmp->flags.is_menu_from_frame_or_window_or_titlebar = False;
  XFlush(dpy);

  if (!pmp->flags.is_submenu && x_start >= 0 && y_start >= 0 &&
      pmret->flags.is_key_press)
  {
    /* warp pointer back to where invoked if this was brought up
     * with a keypress and we're returning from a top level menu,
     * and a button release event didn't end it */
    XWarpPointer(dpy, 0, Scr.Root, 0, 0, Scr.MyDisplayWidth,
		 Scr.MyDisplayHeight, x_start, y_start);
    if (Event.type == KeyPress)
    {
      Event.xkey.x_root = x_start;
      Event.xkey.y_root = y_start;
    }
  }

  dkp.timestamp = 0;
  if(!pmp->flags.is_submenu)
  {
    UngrabEm(GRAB_MENU);
    WaitForButtonsUp(True);
    if (pmret->rc == MENU_DONE)
    {
      if (pmp->ret_paction && *(pmp->ret_paction))
      {
	exec_func_args_type efa;

	indirect_depth++;
	memset(&efa, 0, sizeof(efa));
	efa.eventp = &Event;
	efa.tmp_win = pmp->button_window;
	efa.action = *(pmp->ret_paction);
	efa.context = *(pmp->pcontext);
	efa.module = -1;
	efa.flags.do_save_tmpwin = 1;
	execute_function(&efa);
	indirect_depth--;
	free(*(pmp->ret_paction));
	*(pmp->ret_paction) = NULL;
      }
      last_saved_pos_hints.flags.do_ignore_pos_hints = False;
      last_saved_pos_hints.flags.is_last_menu_pos_hints_valid = False;
    }
    if (indirect_depth == 0)
    {
      last_saved_pos_hints.flags.do_ignore_pos_hints = False;
      last_saved_pos_hints.flags.is_last_menu_pos_hints_valid = False;
    }
  }
  if(!pmp->flags.is_submenu && indirect_depth == 0)
  {
    /* release the keyboard when the last menu closes */
    MyXUngrabKeyboard(dpy);
  }

  return;
}


/***********************************************************************
 * Procedure
 *	menuShortcuts() - Menu keyboard processing
 *
 * Function called from MenuInteraction instead of Keyboard_Shortcuts()
 * when a KeyPress event is received.  If the key is alphanumeric,
 * then the menu is scanned for a matching hot key.  Otherwise if
 * it was the escape key then the menu processing is aborted.
 * If none of these conditions are true, then the default processing
 * routine is called.
 * TKP - uses XLookupString so that keypad numbers work with windowlist
 ***********************************************************************/
typedef enum
{
  SA_NONE = 0,
  SA_ENTER,
  SA_LEAVE,
  SA_MOVE_ITEMS,
  SA_FIRST,
  SA_LAST,
  SA_CONTINUE,
  SA_WARPBACK,
  SA_SELECT,
#ifdef TEAR_OFF_MENUS
  SA_TEAROFF,
#endif
  SA_ABORT
} shortcut_action;

static void menuShortcuts(
  MenuRoot *mr, MenuParameters *pmp, MenuReturn *pmret, XEvent *event,
  MenuItem **pmiCurrent, double_keypress *pdkp)
{
  int fControlKey = event->xkey.state & ControlMask? True : False;
  int fShiftedKey = event->xkey.state & ShiftMask? True: False;
  int fMetaKey = event->xkey.state & Mod1Mask? True: False;
  KeySym keysym;
  char ckeychar = 0;
  int ikeychar;
  MenuItem *newItem = NULL;
  MenuItem *miCurrent = pmiCurrent ? *pmiCurrent : NULL;
  int index;
  int mx;
  int my;
  int menu_x;
  int menu_y;
  unsigned int menu_width;
  unsigned int menu_height;
  int items_to_move;
  Bool fSkipSection = False;
  shortcut_action saction = SA_NONE;

  if (event->type == KeyRelease)
  {
    /* This function is only called with a KeyRelease event if the user released
     * the 'select' key (s)he configured. */
    pmret->rc = MENU_SELECTED;
    return;
  }

  /*** handle double-keypress ***/

  if (pdkp->timestamp &&
      lastTimestamp - pdkp->timestamp < MST_DOUBLE_CLICK_TIME(pmp->menu) &&
      event->xkey.state == pdkp->keystate &&
      event->xkey.keycode == pdkp->keycode)
  {
    *pmiCurrent = NULL;
    pmret->rc = MENU_DOUBLE_CLICKED;
    return;
  }
  pdkp->timestamp = 0;

  /*** find out the key ***/

  /* Is it okay to treat keysym-s as Ascii?
   * No, because the keypad numbers don't work. Use XlookupString */
  index = XLookupString(&(event->xkey), &ckeychar, 1, &keysym, NULL);
  ikeychar = (int)ckeychar;

  /*** Try to match hot keys ***/

  /* Need isascii here - isgraph might coredump! */
  if (index == 1 && isascii(ikeychar) && isgraph(ikeychar) &&
      fControlKey == False && fMetaKey == False)
  {
    /* allow any printable character to be a keysym, but be sure control
       isn't pressed */
    MenuItem *mi;
    MenuItem *mi1;
    int key;
    int countHotkey = 0;      /* Added by MMH mikehan@best.com 2/7/99 */

    /* if this is a letter set it to lower case */
    if (isupper(ikeychar))
      ikeychar = tolower(ikeychar) ;

    /* MMH mikehan@best.com 2/7/99
     * Multiple hotkeys per menu
     * Search menu for matching hotkey;
     * remember how many we found and where we found it */
    mi = ( miCurrent == NULL || miCurrent == MR_LAST_ITEM(mr)) ?
      MR_FIRST_ITEM(mr) : MI_NEXT_ITEM(miCurrent);
    mi1 = mi;
    do
    {
      if (MI_HAS_HOTKEY(mi) && !MI_IS_TITLE(mi) &&
	  (!MI_IS_HOTKEY_AUTOMATIC(mi) || MST_USE_AUTOMATIC_HOTKEYS(mr)))
      {
	key = (MI_LABEL(mi)[(int)MI_HOTKEY_COLUMN(mi)])[MI_HOTKEY_COFFSET(mi)];
	key = tolower(key);
	if ( ikeychar == key )
	{
	  if ( ++countHotkey == 1 )
	    newItem = mi;
	}
      }
      mi = (mi == MR_LAST_ITEM(mr)) ? MR_FIRST_ITEM(mr) : MI_NEXT_ITEM(mi);
    }
    while (mi != mi1);

    /* For multiple instances of a single hotkey, just move the selection */
    if (countHotkey > 1)
    {
      *pmiCurrent = newItem;
      pmret->rc = MENU_NEWITEM;
      return;
    }
    /* Do things the old way for unique hotkeys in the menu */
    else if (countHotkey == 1)
    {
      *pmiCurrent = newItem;
      if (newItem && MI_IS_POPUP(newItem))
	pmret->rc = MENU_POPUP;
      else
	pmret->rc = MENU_SELECTED;
      return;
    }
    /* MMH mikehan@best.com 2/7/99 */
  }

  /*** now determine the action to take ***/

  items_to_move = 0;
  pmret->rc = MENU_NOP;

  switch(keysym)
  {
  case XK_Escape:
  case XK_Delete:
  case XK_KP_Separator:
    /* abort */
    saction = SA_ABORT;
    break;

  case XK_space:
  case XK_Return:
  case XK_KP_Enter:
    /* select menu item */
    saction = SA_SELECT;
    break;

  case XK_Insert:
  case XK_KP_0:
    /* move to last entry of menu ('More...' if this exists) and try to enter
     * the menu.  Otherwise try to enter the current submenu */
    saction = (MR_CONTINUATION_MENU(mr) != NULL) ? SA_CONTINUE : SA_ENTER;
    break;

  case XK_Left:
  case XK_KP_4:
  case XK_h: /* vi left */
    /* leave or enter a submenu */
    saction = (MST_USE_LEFT_SUBMENUS(mr)) ? SA_ENTER : SA_LEAVE;
    break;

  case XK_Right:
  case XK_KP_6:
  case XK_l: /* vi right */
    /* enter or leave a submenu */
    saction = (MST_USE_LEFT_SUBMENUS(mr)) ? SA_LEAVE : SA_ENTER;
    break;

  case XK_b: /* back */
    /* leave menu */
    saction = SA_LEAVE;
    break;

  case XK_f: /* forward */
    /* enter menu */
    saction = SA_ENTER;
    break;

  case XK_Page_Up:
  case XK_KP_9:
    items_to_move = -5;
    saction = SA_MOVE_ITEMS;
    break;

  case XK_Page_Down:
  case XK_KP_3:
    items_to_move = 5;
    saction = SA_MOVE_ITEMS;
    break;

  case XK_Up:
  case XK_KP_8:
  case XK_k: /* vi up */
  case XK_p: /* prior */
    if (fShiftedKey && !fControlKey && !fMetaKey)
    {
      saction = SA_FIRST;
      break;
    }
    else if (fControlKey)
      items_to_move = -5;
    else if (fMetaKey)
    {
      items_to_move = -1;
      fSkipSection = True;
    }
    else
      items_to_move = -1;
    saction = SA_MOVE_ITEMS;
    break;

  case XK_Down:
  case XK_KP_2:
  case XK_j: /* vi down */
  case XK_n: /* next */
    if (fShiftedKey && !fControlKey && !fMetaKey)
    {
      saction = SA_LAST;
      break;
    }
    else if (fControlKey)
      items_to_move = 5;
    else if (fMetaKey)
    {
      items_to_move = 1;
      fSkipSection = True;
    }
    else
      items_to_move = 1;
    saction = SA_MOVE_ITEMS;
    break;

  case XK_Tab:
#ifdef XK_XKB_KEYS
  case XK_ISO_Left_Tab:
#endif
    /* Tab added mostly for Winlist */
    items_to_move = 1;
    switch (fMetaKey + 2 * fControlKey)
    {
    case 0:
    case 1:
      /* tab, alt-tab */
      break;
    case 2:
      /* ctrl-tab */
      fSkipSection = True;
      break;
    case 3:
      /* ctrl-meta-tab */
      items_to_move = 5;
      break;
    default:
      break;
    }
    if (fShiftedKey)
    {
      /* shift reverses direction */
      items_to_move = -items_to_move;
    }
    saction = SA_MOVE_ITEMS;
    break;

  case XK_Home:
  case XK_KP_7:
    saction = SA_FIRST;
    break;

  case XK_End:
  case XK_KP_1:
    saction = SA_LAST;
    break;

#ifdef TEAR_OFF_MENUS
  case XK_BackSpace:
fprintf(stderr,"menu torn off\n");
    saction = SA_TEAROFF;
    break;
#endif

  default:
    break;
  }

  if (!miCurrent &&
      (saction == SA_ENTER || saction == SA_MOVE_ITEMS || saction == SA_SELECT))
  {
    if (XGetGeometry(dpy, MR_WINDOW(mr), &JunkRoot, &menu_x, &menu_y,
		      &menu_width, &menu_height, &JunkBW, &JunkDepth))
    {
      XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &mx, &my, &JunkX, &JunkY, &JunkMask);
      if (my < menu_y + MST_BORDER_WIDTH(mr))
	saction = SA_FIRST;
      else if (my > menu_y + menu_height - MST_BORDER_WIDTH(mr))
	saction = SA_LAST;
      else
	saction = SA_WARPBACK;
    }
    else
    {
      saction = SA_FIRST;
      fvwm_msg(
	ERR, "menuShortcuts", "can't get geometry of menu %s", MR_NAME(mr));
    }
  }

  /*** execute the necessary actions ***/

  switch (saction)
  {
  case SA_ENTER:
    if (miCurrent && MI_IS_POPUP(miCurrent))
      pmret->rc = MENU_POPUP;
    else
      pmret->rc = MENU_NOP;
    break;

  case SA_LEAVE:
    pmret->rc = MENU_POPDOWN;
    break;

  case SA_FIRST:
    if ((*pmiCurrent = get_selectable_item_from_index(mr,0)) != NULL)
      pmret->rc = MENU_NEWITEM;
    else
      pmret->rc = MENU_NOP;
    break;

  case SA_LAST:
    index = get_selectable_item_count(mr, NULL);
    if (index > 0)
    {
      *pmiCurrent = get_selectable_item_from_index(mr, index);
      pmret->rc = (*pmiCurrent) ? MENU_NEWITEM : MENU_NOP;
    }
    else
    {
      pmret->rc = MENU_NOP;
    }
    break;

  case SA_MOVE_ITEMS:
    /* Need isascii here - isgraph might coredump! */
    if (isascii(keysym) && isgraph(keysym))
      fControlKey = False; /* don't use control modifier
			      for j or n, since those might
			      be shortcuts too-- C-j, C-n will
			      always work to do a single down */
    if (fSkipSection)
    {
      int section;
      int count;

      get_selectable_item_count(mr, &count);
      get_selectable_item_index(mr, miCurrent, &section);
      section += items_to_move;
      if (section < 0)
	section = count;
      else if (section > count)
	section = 0;
      index = section;
    }
    else if (items_to_move < 0)
    {
      index = get_selectable_item_index(mr, miCurrent, NULL);
      if (index == 0)
	/* wraparound */
	index = get_selectable_item_count(mr, NULL);
      else
      {
	index += items_to_move;
      }
    }
    else
    {
      index = get_selectable_item_index(mr, miCurrent, NULL) + items_to_move;
      /* correct for the case that we're between items */
      if (MI_IS_SEPARATOR(miCurrent) || MI_IS_TITLE(miCurrent))
	index--;
    }
    if (fSkipSection)
    {
      newItem = get_selectable_item_from_section(mr, index);
    }
    else
    {
      newItem = get_selectable_item_from_index(mr, index);
      if (items_to_move > 0)
      {
	if (newItem == miCurrent)
	  newItem = get_selectable_item_from_index(mr, 0);
      }
    }
    if (newItem)
    {
      *pmiCurrent = newItem;
      pmret->rc = MENU_NEWITEM;
    }
    else
    {
      pmret->rc = MENU_NOP;
    }
    break;

  case SA_CONTINUE:
    *pmiCurrent = MR_LAST_ITEM(mr);
    if (*pmiCurrent && MI_IS_POPUP(*pmiCurrent))
    {
      /* enter the submenu */
      pmret->rc = MENU_POPUP;
    }
    else
    {
      /* do nothing */
      *pmiCurrent = miCurrent;
      pmret->rc = MENU_NOP;
    }
    break;

  case SA_WARPBACK:
    /* Warp the pointer back into the menu. */
    XWarpPointer(dpy, 0, MR_WINDOW(mr), 0, 0, 0, 0,
		 menu_middle_x_offset(mr), my - menu_y);
    *pmiCurrent = find_entry(NULL, NULL, None, -1, -1);
    pmret->rc = MENU_NEWITEM;
    break;

  case SA_SELECT:
    pmret->rc = MENU_SELECTED;
    return;

  case SA_ABORT:
    pmret->rc = MENU_ABORTED;
    return;

#ifdef TEAR_OFF_MENUS
  case SA_TEAROFF:
    pmret->rc = MENU_TEAR_OFF;
    return;
#endif

  case SA_NONE:
  default:
    pmret->rc = MENU_NOP;
    break;
  }

  return;
}


static Bool is_double_click(
  Time t0, MenuItem *mi, MenuParameters *pmp, MenuReturn *pmret,
  double_keypress *pdkp, Bool has_mouse_moved)
{
  if (Event.type == KeyPress)
    return False;
  if (lastTimestamp - t0 >= MST_DOUBLE_CLICK_TIME(pmp->menu))
    return False;
  if (has_mouse_moved)
    return False;
  if (!pmp->flags.has_default_action &&
      (mi && mi == MR_FIRST_ITEM(pmp->menu) && MI_IS_SELECTABLE(mi)))
    return False;
  if (pmp->flags.is_submenu)
    return False;
  if (pmp->flags.is_invoked_by_key_press && pdkp->timestamp == 0)
    return False;
  return True;
}


/***********************************************************************
 *
 *  Procedure:
 *	Interacts with user to Update menu display and start new submenus,
 *	return to old sub menus, etc.
 *  Input
 *	sticks = 0, transient style menu, drops on button release
 *	sticks = 1, sticky style, stays up on a click (i.e. initial release
 *		    comes soon after initial press.
 *  Returns:
 *	MENU_ERROR on error condition
 *	MENU_SUBMENU_DONE on return from submenu to parent menu
 *	MENU_DONE on completion by non-button-release (e.g. keyboard)
 *	MENU_ABORT on abort of menu by non-button-release (e.g. keyboard)
 *	MENU_SELECTED on item selection -- returns action * in *ret_paction
 *
 ***********************************************************************/
static void MenuInteraction(
  MenuParameters *pmp, MenuReturn *pmret, double_keypress *pdkp,
  Bool *pdo_warp_to_title)
{
  extern XEvent Event;
  MenuItem *mi = NULL;
  MenuItem *tmi;
  MenuItem *miRemovedSubmenu = NULL;
  MenuRoot *mrMi = NULL;
  MenuRoot *tmrMi = NULL;
  MenuRoot *mrPopup = NULL;
  MenuRoot *mrMiPopup = NULL;
  MenuRoot *mrNeedsPainting = NULL;
  MenuRoot *mrPopdown = NULL;

  int x_init = 0;
  int y_init = 0;
  int x_offset = 0;
  int popdown_delay_10ms = 0;
  int popup_delay_10ms = 0;
  Time t0 = lastTimestamp;
  MenuOptions mops;
  exec_func_args_type efa;
  struct
  {
    unsigned do_popup_immediately : 1;
    /* used for delay popups, to just popup the menu */
    unsigned do_popup_now : 1;
    unsigned do_popdown_now : 1;
    /* used for keystrokes, to popup and move to that menu */
    unsigned do_popup_and_warp : 1;
    unsigned do_force_reposition : 1;
    unsigned do_force_popup : 1;
    unsigned do_popdown : 1;
    unsigned do_popup : 1;
    unsigned do_menu : 1;
    unsigned do_recycle_event : 1;
    unsigned do_propagate_event_into_submenu : 1;
    unsigned has_mouse_moved : 1;
    unsigned is_off_menu_allowed : 1;
    unsigned is_key_press : 1;
    unsigned is_item_entered_by_key_press : 1;
    unsigned is_motion_faked : 1;
    unsigned is_popped_up_by_timeout : 1;
    unsigned is_pointer_in_active_item_area : 1;
    unsigned is_motion_first : 1;
    unsigned is_release_first : 1;
    unsigned is_submenu_mapped : 1;
    unsigned was_item_unposted : 1;
  } flags;
  /* Can't make this a member of the flags as we have to take its address. */
  Bool does_submenu_overlap = False;
  Bool does_popdown_submenu_overlap = False;

  pmret->rc = MENU_NOP;
  memset(&flags, 0, sizeof(flags));
  flags.do_force_reposition = 1;
  memset(&mops, 0, sizeof(mops));
  /* remember where the pointer was so we can tell if it has moved */
  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x_init, &y_init, &JunkX, &JunkY, &JunkMask);

  while (True)
  {
    flags.do_popup_and_warp = False;
    flags.do_popup_now = False;
    flags.do_popdown_now = False;
    flags.do_propagate_event_into_submenu = False;
    flags.is_key_press = False;
    if (pmp->event_propagate_to_submenu)
    {
      /* handle an event that was passed in from the parent menu */
      memcpy(&Event, pmp->event_propagate_to_submenu, sizeof(XEvent));
      pmp->event_propagate_to_submenu = NULL;
    }
    else if (flags.do_recycle_event)
    {
      flags.is_popped_up_by_timeout = False;
      flags.do_recycle_event = 0;
      if (pmp->menu != pmret->menu)
      {
	/* the event is for a previous menu, just close this one */
	pmret->rc = MENU_PROPAGATE_EVENT;
	goto DO_RETURN;
      }
      if (Event.type == KeyPress)
      {
	/* since the pointer has been warped since the key was pressed, fake a
	 * different key press position */
	XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		      &Event.xkey.x_root, &Event.xkey.y_root,
		      &JunkX, &JunkY, &JunkMask);
	mi = MR_SELECTED_ITEM(pmp->menu);
      }
    } /* flags.do_recycle_event */
    else
    {
      if (flags.do_force_reposition)
      {
	Event.type = MotionNotify;
	Event.xmotion.time = lastTimestamp;
	flags.is_motion_faked = True;
	flags.do_force_reposition = False;
	flags.is_popped_up_by_timeout = False;
      }
      else if (!XCheckMaskEvent(dpy,ExposureMask,&Event))
      {
	/* handle exposure events first */
	if (flags.do_force_popup || flags.is_pointer_in_active_item_area ||
	    MST_POPDOWN_DELAY(pmp->menu) > 0 ||
	    (MST_POPUP_DELAY(pmp->menu) > 0 && !flags.is_popped_up_by_timeout))
	{
	  while (!XPending(dpy) || !XCheckMaskEvent(dpy, XEVMASK_MENU, &Event))
	  {
	    Bool is_popup_timed_out =
	      (MST_POPUP_DELAY(pmp->menu) > 0 &&
	       popup_delay_10ms++ >= MST_POPUP_DELAY(pmp->menu) + 1);
	    Bool is_popdown_timed_out =
	      (MST_POPDOWN_DELAY(pmp->menu) > 0 && mrPopdown &&
	       popdown_delay_10ms++ >= MST_POPDOWN_DELAY(pmp->menu) + 1);
	    Bool do_fake_motion = False;

	    if (is_popup_timed_out)
	    {
	      popup_delay_10ms = MST_POPUP_DELAY(pmp->menu);
	    }
	    if (is_popdown_timed_out)
	    {
	      popdown_delay_10ms = MST_POPDOWN_DELAY(pmp->menu);
	    }
	    if (flags.do_force_popup ||
		(is_popup_timed_out &&
		 (is_popdown_timed_out || flags.is_item_entered_by_key_press ||
		  !mrPopdown || MST_DO_POPDOWN_IMMEDIATELY(pmp->menu))))
	    {
	      flags.do_popup_now = True;
	      flags.do_force_popup = False;
	      do_fake_motion = True;
	      flags.is_popped_up_by_timeout = True;
	      is_popdown_timed_out = True;
	    }
	    if ((mrPopdown || mrPopup) &&
		(is_popdown_timed_out || MST_DO_POPDOWN_IMMEDIATELY(pmp->menu)))
	    {
	      MenuRoot *m = (mrPopdown) ? mrPopdown : mrPopup;

	      if (!m || mi != MR_PARENT_ITEM(m))
	      {
		flags.do_popdown_now = True;
		do_fake_motion = True;
		if (!MST_DO_POPUP_IMMEDIATELY(pmp->menu) &&
		    !MST_DO_POPDOWN_IMMEDIATELY(pmp->menu) &&
		    MST_POPUP_DELAY(pmp->menu) <= MST_POPDOWN_DELAY(pmp->menu)
		    && popup_delay_10ms == popdown_delay_10ms)
		{
		  flags.do_popup_now = True;
		  flags.is_popped_up_by_timeout = True;
		}
		if (flags.is_pointer_in_active_item_area)
		{
		  flags.do_popup_now = True;
		  flags.is_popped_up_by_timeout = True;
		}
	      }
	    }
	    if (flags.do_popup_now && mi == miRemovedSubmenu &&
		!flags.is_key_press)
	    {
	      /* prevent popping up the menu again with RemoveSubemenus */
	      flags.do_popup_now = False;
	      flags.do_force_popup = False;
	      do_fake_motion = flags.do_popdown_now;
	      flags.is_popped_up_by_timeout = False;
	    }

	    if (do_fake_motion)
	    {
	      /* fake a motion event, and set flags.do_popup_now */
	      Event.type = MotionNotify;
	      Event.xmotion.time = lastTimestamp;
	      flags.is_motion_faked = True;
	      break;
	    }
	    usleep(10000 /* 10 ms*/);
	  }
	}
	else
	{
	  /* block until there is an event */
	  XMaskEvent(dpy, XEVMASK_MENU, &Event);
	  flags.is_popped_up_by_timeout = False;
	}
      }
      else
      {
	flags.is_popped_up_by_timeout = False;
      }
    } /* !flags.do_recycle_event */

    flags.is_pointer_in_active_item_area = False;
    StashEventTime(&Event);
    if (Event.type == MotionNotify)
    {
      /* discard any extra motion events before a release */
      while ((XCheckMaskEvent(dpy,ButtonMotionMask|ButtonReleaseMask,
			     &Event))&&(Event.type != ButtonRelease))
      {
	/* nop */
      }
    }

    pmret->rc = MENU_NOP;
    switch(Event.type)
    {
    case ButtonRelease:
      mi = find_entry(&x_offset, &mrMi, None, -1, -1);
      /* hold the menu up when the button is released
       * for the first time if released OFF of the menu */
      if(pmp->flags.is_sticky && !flags.is_motion_first)
      {
	flags.is_release_first = True;
	pmp->flags.is_sticky = False;
	continue;
	/* break; */
      }
      flags.was_item_unposted = 0;
      if (pmret->flags.is_menu_posted && mrMi != NULL)
      {
	if (pmret->flags.do_unpost_submenu)
	{
	  pmret->flags.do_unpost_submenu = 0;
	  pmret->flags.is_menu_posted = 0;
	  /* just ignore the event */
	  continue;
	}
	else if (mi && MI_IS_POPUP(mi) && mrMi == pmp->menu)
	{
	  /* post menu - done below */
	}
	else if (MR_PARENT_ITEM(pmp->menu) && mi == MR_PARENT_ITEM(pmp->menu))
	{
	  /* propagate back to parent menu and unpost current menu */
	  pmret->flags.do_unpost_submenu = 1;
	  pmret->rc = MENU_PROPAGATE_EVENT;
	  pmret->menu = mrMi;
	  goto DO_RETURN;
	}
	else if (mrMi != pmp->menu && mrMi != mrPopup)
	{
	  /* unpost and propagate back to ancestor */
	  pmret->flags.is_menu_posted = 0;
	  pmret->rc = MENU_PROPAGATE_EVENT;
	  pmret->menu = mrMi;
	  goto DO_RETURN;
	}
	else if (mrPopup && mrMi == mrPopup)
	{
	  /* unpost and propagate into submenu */
	  flags.was_item_unposted = 1;
	  flags.do_propagate_event_into_submenu = True;
	  break;
	}
	else
	{
	  /* simply unpost the menu */
	  pmret->flags.is_menu_posted = 0;
	}
      }
      if (is_double_click(t0, mi, pmp, pmret, pdkp, flags.has_mouse_moved))
      {
	pmret->rc = MENU_DOUBLE_CLICKED;
	goto DO_RETURN;
      }
      pmret->rc = (mi) ? MENU_SELECTED : MENU_ABORTED;
      if (pmret->rc == MENU_SELECTED && mi &&
	  MI_IS_POPUP(mi) && !MST_DO_POPUP_AS_ROOT_MENU(pmp->menu))
      {
	if (flags.was_item_unposted)
	{
	  pmret->flags.is_menu_posted = 0;
	  pmret->rc = MENU_UNPOST;
	  pmret->menu = NULL;
	  pmret->parent_menu = pmp->menu;
	  flags.do_popup_now = False;
	}
	else
	{
	  pmret->flags.is_menu_posted = 1;
	  pmret->rc = MENU_POST;
	  pmret->menu = NULL;
	  pmret->parent_menu = pmp->menu;
	  flags.do_popup_now = True;
	  if ((mrPopup || mrPopdown) && mi != MR_SELECTED_ITEM(pmp->menu))
	  {
	    flags.do_popdown_now = True;
	  }
	}
	break;
      }
      pdkp->timestamp = 0;
      goto DO_RETURN;

    case ButtonPress:
      /* if the first event is a button press allow the release to
       * select something */
      pmp->flags.is_sticky = False;
      continue;

    case VisibilityNotify:
      continue;

    case KeyRelease:
      if (Event.xkey.keycode != MST_SELECT_ON_RELEASE_KEY(pmp->menu))
	continue;
      /* fall through to KeyPress */

    case KeyPress:
      /* Handle a key press events to allow mouseless operation */
      flags.is_key_press = True;
      x_offset = menu_middle_x_offset(pmp->menu);

      /* if there is a posted menu we may have to move back into a previous
       * menu or possibly ignore the mouse position */
      if (pmret->flags.is_menu_posted)
      {
	MenuItem *l_mi;
	MenuRoot *l_mrMi;
	int l_x_offset;

	pmret->flags.is_menu_posted = 0;
	l_mi = find_entry(&l_x_offset, &l_mrMi, None, -1, -1);
	if (l_mrMi != NULL)
	{
	  if (pmp->menu != l_mrMi)
	  {
	    /* unpost the menu and propagate the event to the correct menu */
	    pmret->rc = MENU_PROPAGATE_EVENT;
	    pmret->menu = l_mrMi;
	    goto DO_RETURN;
	  }
	}
	mi = MR_SELECTED_ITEM(pmp->menu);
	Event.xkey.x_root = x_offset;
	Event.xkey.y_root = item_middle_y_offset(pmp->menu, mi);
      }

      /* now handle the actual key press */
      menuShortcuts(pmp->menu, pmp, pmret, &Event, &mi, pdkp);
      if (pmret->rc != MENU_NOP)
      {
	/* using a key 'unposts' the posted menu */
	pmret->flags.is_menu_posted = 0;
      }
      switch (pmret->rc)
      {
      case MENU_SELECTED:
	if (mi && MI_IS_POPUP(mi) && !MST_DO_POPUP_AS_ROOT_MENU(pmp->menu))
	{
	  pmret->rc = MENU_POPUP;
	  break;
	}
	goto DO_RETURN;
      case MENU_POPDOWN:
      case MENU_ABORTED:
      case MENU_DOUBLE_CLICKED:
#ifdef TEAR_OFF_MENUS
      case MENU_TEAR_OFF:
#endif
	goto DO_RETURN;
      case MENU_NEWITEM:
      case MENU_POPUP:
	if (mrMi == NULL)
	{
	  /* Set the MenuRoot of the current item in case we have just warped to
	   * the menu from the void or unposted a popup menu. */
	  mrMi = pmp->menu;
	}
	/*tmrMi = mrMi;*/
	break;
      default:
	break;
      }
      /* now warp to the new menu item, if any */
      if (pmret->rc == MENU_NEWITEM && mi)
      {
	warp_pointer_to_item(mrMi, mi, False);
      }
      if (pmret->rc == MENU_POPUP && mi && MI_IS_POPUP(mi))
      {
	flags.do_popup_and_warp = True;
	break;
      }
      break;

    case MotionNotify:
    {
      int p_rx = -1;
      int p_ry = -1;
      Window p_child = None;

      if (flags.has_mouse_moved == False)
      {
	XQueryPointer(dpy, Scr.Root, &JunkRoot, &p_child,
		      &p_rx, &p_ry, &JunkX, &JunkY, &JunkMask);
	if (p_rx - x_init > Scr.MoveThreshold ||
	    x_init - p_rx > Scr.MoveThreshold ||
	    p_ry - y_init > Scr.MoveThreshold ||
	    y_init - p_ry > Scr.MoveThreshold)
	{
	  /* global variable remember that this isn't just
	   * a click any more since the pointer moved */
	  flags.has_mouse_moved = True;
	}
      }
      mi = find_entry(&x_offset, &mrMi, p_child, p_rx, p_ry);
      if (pmret->flags.is_menu_posted && mrMi != pmret->menu)
      {
	/* ignore mouse movement outside a posted menu */
	mrMi = NULL;
	mi = NULL;
      }
      if (!flags.is_release_first && !flags.is_motion_faked &&
	  flags.has_mouse_moved)
	flags.is_motion_first = True;
      flags.is_motion_faked = False;
      break;
    }

    case Expose:
      /* grab our expose events, let the rest go through */
      if((XFindContext(dpy, Event.xany.window, MenuContext,
		       (caddr_t *)&mrNeedsPainting) != XCNOENT))
      {
	flush_accumulate_expose(Event.xany.window, &Event);
	paint_menu(mrNeedsPainting, &Event, (*pmp->pTmp_win));
      }
      /* we want to dispatch this too so window decorations get redrawn
       * after being obscured by menus. */
      /* We need to preserve the Tmp_win here! Note that handling an Expose
       * event will never invalidate the Tmp_win. */
      DispatchEvent(True);
      continue;

    default:
      DispatchEvent(False);
      break;
    } /* switch (Event.type) */


    /************************************************************************
     * Now handle new menu items, whether it is from a keypress or a pointer
     * motion event.
     ************************************************************************/

    pmret->flags.do_unpost_submenu = 0;
    if (mi)
    {
      /* we're on a menu item */
      flags.is_off_menu_allowed = False;
      if (mrMi == pmp->menu && mi != miRemovedSubmenu)
      {
	miRemovedSubmenu = NULL;
      }
      if (mrMi != pmp->menu && mrMi != mrPopup && mrMi != mrPopdown)
      {
	/* we're on an item from a prior menu */
	if (mrMi != MR_PARENT_MENU(pmp->menu))
	{
	  /* the event is for a previous menu, just close this one */
	  pmret->rc = MENU_PROPAGATE_EVENT;
	  pmret->menu = mrMi;
	}
	else
	{
	  pmret->rc = MENU_POPDOWN;
	}
	pdkp->timestamp = 0;
	goto DO_RETURN;
      }

      /* see if we're on a new item of this menu */
      if (mi != MR_SELECTED_ITEM(pmp->menu) && mrMi == pmp->menu)
      {
	flags.is_item_entered_by_key_press = flags.is_key_press;
	popup_delay_10ms = 0;
	/* we're on the same menu, but a different item,
	 * so we need to unselect the old item */
	if (MR_SELECTED_ITEM(pmp->menu))
	{
	  /* something else was already selected on this menu */
	  /* We have to pop down the menu before unselecting the item in case
	   * we are using gradient menus. The recalled image would paint over
	   * the submenu. */
	  if (mrPopup && mrPopup != mrPopdown)
	  {
	    mrPopdown = mrPopup;
	    popdown_delay_10ms = 0;
	    does_popdown_submenu_overlap = does_submenu_overlap;
	    mrPopup = NULL;
	  }
	  select_menu_item(pmp->menu, MR_SELECTED_ITEM(pmp->menu), False,
			   (*pmp->pTmp_win));
	}
	/* highlight the new item; sets MR_SELECTED_ITEM(pmp->menu), too */
	select_menu_item(pmp->menu, mi, True, (*pmp->pTmp_win));
      } /* new item of the same menu */
      else if (mi != MR_SELECTED_ITEM(pmp->menu) && mrMi && mrMi == mrPopdown)
      {
	/* we're on the popup menu of a different menu item of this menu */
	mi = MR_PARENT_ITEM(mrPopdown);
	mrPopup = mrPopdown;
	mrPopdown = NULL;
	does_submenu_overlap = does_popdown_submenu_overlap;
	select_menu_item(pmp->menu, mi, True, (*pmp->pTmp_win));
      }
      mrMiPopup = mr_popup_for_mi(pmp->menu, mi);

      /* check what has to be done with the item */
      flags.do_popdown = False;
      flags.do_popup = False;
      flags.do_menu = False;
      flags.is_submenu_mapped = False;

      if (mrMi == mrPopup)
      {
	/* must make current popup menu a real menu */
	flags.do_menu = True;
	flags.is_submenu_mapped = True;
      }
      else if (pmret->rc == MENU_POST)
      {
	/* must create a real menu and warp into it */
	if (mrPopup == NULL || mrPopup != mrMiPopup)
	{
	  flags.do_popup = True;
	}
	flags.do_menu = True;
	flags.is_submenu_mapped = True;
      }
      else if (flags.do_popup_and_warp)
      {
	/* must create a real menu and warp into it */
	if (mrPopup == NULL || mrPopup != mrMiPopup)
	{
	  flags.do_popup = True;
	}
	else
	{
	  XRaiseWindow(dpy, MR_WINDOW(mrPopup));
	  warp_pointer_to_item(mrPopup, MR_FIRST_ITEM(mrPopup), True);
	  flags.do_menu = True;
	  flags.is_submenu_mapped = True;
	}
      }
      else if (mi && MI_IS_POPUP(mi))
      {
	if (Event.type == ButtonPress &&
	    is_double_click(t0, mi, pmp, pmret, pdkp, flags.has_mouse_moved))
	{
	  pmret->rc = MENU_DOUBLE_CLICKED;
	  goto DO_RETURN;
	}
	else if (!mrPopup || mrPopup != mrMiPopup || flags.do_popup_and_warp)
	{
	  Bool do_it_now = False;

	  if (flags.do_popup_now)
	  {
	    do_it_now = True;
	  }
	  else if (MST_DO_POPUP_IMMEDIATELY(pmp->menu) &&
		   mi != miRemovedSubmenu)
	  {
	    if (flags.is_key_press ||
		MST_DO_POPDOWN_IMMEDIATELY(pmp->menu) || !mrPopdown)
	    {
	      do_it_now = True;
	    }
	  }
	  else if (pointer_in_active_item_area(x_offset, mrMi))
	  {
	    if (flags.is_key_press || mi == miRemovedSubmenu ||
		MST_DO_POPDOWN_IMMEDIATELY(pmp->menu) || !mrPopdown)
	    {
	      do_it_now = True;
	    }
	    else
	    {
	      flags.is_pointer_in_active_item_area = True;
	    }
	  }
	  if (do_it_now)
	  {
	    miRemovedSubmenu = NULL;
	    /* must create a new menu or popup */
	    if (mrPopup == NULL || mrPopup != mrMiPopup)
	    {
	      if (flags.do_popup_now)
		flags.do_popup = True;
	      else
	      {
		/* pop up in next pass through loop */
		flags.do_force_popup = True;
	      }
	    }
	    else if (flags.do_popup_and_warp)
	    {
	      warp_pointer_to_item(mrPopup, MR_FIRST_ITEM(mrPopup), True);
	    }
	  }
	}
      } /* else if (mi && MI_IS_POPUP(mi)) */
      if (flags.do_popdown_now)
      {
	flags.do_popdown = True;
	flags.do_popdown_now = False;
      }
      else if (flags.do_popup)
      {
	if (mrPopup && mrPopup != mrMiPopup)
	{
	  flags.do_popdown = True;
	  mrPopdown = mrPopup;
	  popdown_delay_10ms = 0;
	  does_popdown_submenu_overlap = does_submenu_overlap;
	}
	else if (mrPopdown && mrPopdown != mrMiPopup)
	{
	  flags.do_popdown = True;
	}
	else if (mrPopdown && mrPopdown == mrMiPopup)
	{
	  mrPopup = mrPopdown;
	  does_submenu_overlap = does_popdown_submenu_overlap;
	  mrPopdown = NULL;
	  flags.do_popup = False;
	  flags.do_popdown = False;
	}
      }
      if (flags.do_popdown && !flags.do_popup)
      {
	/* popdown previous popup */
	if (mrPopdown)
	{
	  pop_menu_down_and_repaint_parent(
	    &mrPopdown, &does_popdown_submenu_overlap, pmp);
	  MR_SUBMENU_ITEM(pmp->menu) = NULL;
	  if (mrPopup == mrPopdown)
	    mrPopup = NULL;
	  mrPopdown = NULL;
	}
	flags.do_popdown = False;
      } /* if (flags.do_popdown && !flags.do_popup) */
      if (flags.do_popup)
      {
	if (!MR_IS_PAINTED(pmp->menu))
	{
	  /* draw the parent menu if it is not already drawn */
	  flush_expose(MR_WINDOW(pmp->menu));
	  paint_menu(pmp->menu, NULL, (*pmp->pTmp_win));
	}
	/* get pos hints for item's action */
	get_popup_options(pmp, mi, &mops);
	if (mrMi == pmp->menu && mrMiPopup == NULL && MI_IS_POPUP(mi) &&
	    MR_MISSING_SUBMENU_FUNC(pmp->menu))
	{
	  /* We're on a submenu item, but the submenu does not exist. The
	   * user defined missing_submenu_action may create it. */
	  Bool is_complex_function;
	  Bool f;
	  Bool is_busy_grabbed = False;
	  Time t;
	  char *menu_name;
	  char *action;
	  extern XEvent Event;
	  char *missing_action = MR_MISSING_SUBMENU_FUNC(pmp->menu);

	  f = *pdo_warp_to_title;
	  t = lastTimestamp;
	  menu_name = PeekToken(SkipNTokens(MI_ACTION(mi), 1), NULL);
	  if (!menu_name)
	  {
	    menu_name = "";
	  }
	  is_complex_function = (FindFunction(missing_action) != NULL);
	  if (is_complex_function)
	  {
	    char *action_ptr;
	    action =
	      safemalloc(
		strlen("Function") + 3 +
		strlen(missing_action) * 2 + 3 +
		strlen(menu_name) * 2 + 1);
	    strcpy(action, "Function ");
	    action_ptr = action + strlen(action);
	    action_ptr = QuoteString(action_ptr, missing_action);
	    *action_ptr++ = ' ';
	    action_ptr = QuoteString(action_ptr, menu_name);
	  }
	  else
	  {
	    action = MR_MISSING_SUBMENU_FUNC(pmp->menu);
	  }
	  if (Scr.BusyCursor & BUSY_DYNAMICMENU)
	    is_busy_grabbed = GrabEm(CRS_WAIT, GRAB_BUSYMENU);
	  /* Execute the action */
	  memset(&efa, 0, sizeof(efa));
	  efa.eventp = &Event;
	  efa.tmp_win = *(pmp->pTmp_win);
	  efa.action = action;
	  efa.context = *(pmp->pcontext);
	  efa.module = -2;
	  efa.flags.exec = FUNC_DONT_EXPAND_COMMAND;
	  efa.flags.do_save_tmpwin = 1;
	  execute_function(&efa);
	  if (is_complex_function)
	    free(action);
	  if (is_busy_grabbed)
	    UngrabEm(GRAB_BUSYMENU);
	  /* restore the stuff we saved */
	  lastTimestamp = t;
	  *pdo_warp_to_title = f;
	  if (!check_if_fvwm_window_exists(*(pmp->pTmp_win)))
	  {
	    *(pmp->pTmp_win) = NULL;
	    *(pmp->pcontext) = 0;
	  }
	  /* Let's see if the menu exists now. */
	  mrMiPopup = mr_popup_for_mi(pmp->menu, mi);
	} /* run MISSING_SUBMENU_FUNCTION */
	mrPopup = mrMiPopup;
	if (!mrPopup)
	{
	  flags.do_menu = False;
	  pmret->flags.is_menu_posted = 0;
	}
	else
	{
	  /* create a popup menu */
	  if (!is_submenu_mapped(pmp->menu, mi))
	  {
	    /* We want to pop prepop menus so it doesn't *have* to be
	       unpopped; do_menu pops down any menus it pops up, but we
	       want to be able to popdown w/o actually removing the menu */
	    int x;
	    int y;
	    Bool prefer_left_submenus;

	    /* Make sure we are using the latest style and menu layout. */
	    update_menu(mrPopup, pmp);

	    get_prefered_popup_position(
	      pmp->menu, mrPopup, &x, &y, &prefer_left_submenus);
	    /* Note that we don't care if popping up the menu works. If it
	     * doesn't we'll catch it below. */
	    pop_menu_up(
	      &mrPopup, pmp, pmp->menu, mi, pmp->pTmp_win, pmp->pcontext, x, y,
	      prefer_left_submenus, flags.do_popup_and_warp, &mops,
	      &does_submenu_overlap, pdo_warp_to_title,
	      (mrPopdown) ? MR_WINDOW(mrPopdown) : None);
	    if (mrPopup == NULL)
	    {
	      /* the menu deleted itself when execution the dynamic popup
	       * action */
	      pmret->rc = MENU_ERROR;
	      goto DO_RETURN;
	    }
	    MR_SUBMENU_ITEM(pmp->menu) = mi;
	  }
	} /* else (mrPopup) */
	if (flags.do_popdown)
	{
	  if (mrPopdown)
	  {
	    if (mrPopdown != mrPopup)
	    {
	      pop_menu_down_and_repaint_parent(
		&mrPopdown, &does_popdown_submenu_overlap, pmp);
	    }
	    mrPopdown = NULL;
	  }
	  flags.do_popdown = False;
	}
	if (mrPopup)
	{
	  if (MR_PARENT_MENU(mrPopup) == pmp->menu)
	  {
	    mi = find_entry(NULL, &mrMi, None, -1, -1);
	    if (mi && mrMi == mrPopup)
	    {
	      flags.do_menu = True;
	      flags.is_submenu_mapped = True;
	    }
	  }
	  else
	  {
	    /* This menu must be already mapped somewhere else, so ignore
	     * it completely. This can only happen if we have reached the
	     * maximum allowed number of menu copies. */
	    flags.do_menu = False;
	    flags.do_popdown = False;
	    mrPopup = NULL;
	  }
	} /* if (mrPopup) */
      } /* if (flags.do_popup) */
      /* remember the 'posted' menu */
      if (pmret->flags.is_menu_posted && mrPopup != NULL &&
	  pmret->menu == NULL)
      {
	pmret->menu = mrPopup;
      }
      if (flags.do_menu)
      {
	{
	  MenuParameters mp;
	  XEvent e;

	  memset(&mp, 0, sizeof(mp));
	  mp.menu = mrPopup;
	  mp.parent_menu = pmp->menu;
	  mp.parent_item = mi;
	  mp.pTmp_win = pmp->pTmp_win;
	  mp.button_window = pmp->button_window;
	  mp.pcontext = pmp->pcontext;
	  mp.flags.has_default_action = False;
	  mp.flags.is_menu_from_frame_or_window_or_titlebar = False;
	  mp.flags.is_sticky = False;
	  mp.flags.is_submenu = True;
	  mp.flags.is_already_mapped = flags.is_submenu_mapped;
	  mp.eventp = (flags.do_popup_and_warp) ? (XEvent *)1 : NULL;
	  mp.pops = &mops;
	  mp.ret_paction = pmp->ret_paction;
	  mp.screen_origin_x = pmp->screen_origin_x;
	  mp.screen_origin_y = pmp->screen_origin_y;
	  if (flags.do_propagate_event_into_submenu)
	  {
	    memcpy(&e, &Event, sizeof(XEvent));
	    mp.event_propagate_to_submenu = &e;
	  }
	  else
	  {
	    mp.event_propagate_to_submenu = NULL;
	  }

	  /* recursively do the new menu we've moved into */
	  do_menu(&mp, pmret);
	}

	flags.do_propagate_event_into_submenu = False;
	if (pmret->rc == MENU_PROPAGATE_EVENT)
	{
	  flags.do_recycle_event = 1;
	  continue;
	}
	if (IS_MENU_RETURN(pmret->rc))
	{
	  pdkp->timestamp = 0;
	  goto DO_RETURN;
	}
	if (MST_DO_UNMAP_SUBMENU_ON_POPDOWN(pmp->menu) &&
	    pmret->flags.is_key_press)
	{
	  miRemovedSubmenu = MR_PARENT_ITEM(mrPopup);
	  pop_menu_down_and_repaint_parent(
	    &mrPopup, &does_submenu_overlap, pmp);
	  MR_SUBMENU_ITEM(pmp->menu) = NULL;
	  if (mrPopup == mrPopdown)
	    mrPopdown = NULL;
	  mrPopup = NULL;
	}
	if (pmret->rc == MENU_POPDOWN)
	{
	  popup_delay_10ms = 0;
	  flags.do_force_reposition = True;
	}
      } /* if (flags.do_menu) */

      /* Now check whether we can animate the current popup menu back to the
       * original place to unobscure the current menu;	this happens only when
       * using animation */
      if (mrPopup && MR_XANIMATION(mrPopup) &&
	  (tmi = find_entry(NULL, &tmrMi, None, -1, -1))  &&
	  (tmi == MR_SELECTED_ITEM(pmp->menu) || tmrMi != pmp->menu))
      {
	animated_move_back(mrPopup, False);
      }
      /* now check whether we should animate the current real menu
	 over to the right to unobscure the prior menu; only a very
	 limited case where this might be helpful and not too disruptive */

      if (mrPopup == NULL && pmp->parent_menu != NULL &&
	  MR_XANIMATION(pmp->menu) != 0 &&
	  pointer_in_passive_item_area(x_offset, mrMi))
      {
	/* we have to see if we need menu to be moved */
	animated_move_back(pmp->menu, True);
	  if (mrPopdown)
	  {
	    if (mrPopdown != mrPopup)
	    {
	      pop_menu_down_and_repaint_parent(
		&mrPopdown, &does_popdown_submenu_overlap, pmp);
	    }
	    mrPopdown = NULL;
	  }
	  flags.do_popdown = False;
      }

    } /* if (mi) */
    else
    {
      /* moved off menu, deselect selected item... */
      if (MR_SELECTED_ITEM(pmp->menu) &&
	  flags.is_off_menu_allowed == False && !pmret->flags.is_menu_posted)
      {
	if (mrPopup)
	{
	  int x, y, mx, my;
	  unsigned int mw, mh;
	  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
			&x, &y, &JunkX, &JunkY, &JunkMask);
	  if (XGetGeometry(dpy, MR_WINDOW(pmp->menu), &JunkRoot, &mx, &my,
			   &mw, &mh, &JunkBW, &JunkDepth) &&
	      ((!MR_IS_LEFT(mrPopup)  && x < mx) ||
	       (!MR_IS_RIGHT(mrPopup) && x > mx + mw) ||
	       (!MR_IS_UP(mrPopup)    && y < my) ||
	       (!MR_IS_DOWN(mrPopup)  && y > my + mh)))
	  {
	    select_menu_item(
	      pmp->menu, MR_SELECTED_ITEM(pmp->menu), False, (*pmp->pTmp_win));
	    pop_menu_down_and_repaint_parent(
	      &mrPopup, &does_submenu_overlap, pmp);
	    MR_SUBMENU_ITEM(pmp->menu) = NULL;
	    if (mrPopup == mrPopdown)
	      mrPopdown = NULL;
	    mrPopup = NULL;
	  }
	  else if (x < mx || x >= mx + mw || y < my || y >= my + mh)
	  {
	    /* pointer is outside the menu but do not pop down	*/
	    flags.is_off_menu_allowed = True;
	  }
	  else
	  {
	    /* Pointer is still in the menu. Postpone the decision if we have
	     * to pop down. */
	  }
	} /* if (mrPopup) */
	else
	{
	  select_menu_item(pmp->menu, MR_SELECTED_ITEM(pmp->menu), False,
			   (*pmp->pTmp_win));
	}
      } /* if (MR_SELECTED_ITEM(mpmp->enu)) */
    } /* else (!mi) */
    XFlush(dpy);
  } /* while (True) */

  DO_RETURN:
  if (mrPopdown)
  {
    pop_menu_down_and_repaint_parent(
      &mrPopdown, &does_popdown_submenu_overlap, pmp);
    MR_SUBMENU_ITEM(pmp->menu) = NULL;
  }
  if (pmret->rc == MENU_SELECTED &&
      is_double_click(t0, mi, pmp, pmret, pdkp, flags.has_mouse_moved))
  {
    pmret->rc = MENU_DOUBLE_CLICKED;
  }

  switch (pmret->rc)
  {
  case MENU_POPDOWN:
  case MENU_PROPAGATE_EVENT:
  case MENU_DOUBLE_CLICKED:
    if (MR_SELECTED_ITEM(pmp->menu))
      select_menu_item(pmp->menu, MR_SELECTED_ITEM(pmp->menu), False,
		       (*pmp->pTmp_win));
    if (flags.is_key_press && pmret->rc != MENU_DOUBLE_CLICKED)
    {
      if (!pmp->flags.is_submenu)
      {
	/* abort a root menu rather than pop it down */
	pmret->rc = MENU_ABORTED;
      }
      if (pmp->parent_menu && MR_SELECTED_ITEM(pmp->parent_menu))
      {
	warp_pointer_to_item(pmp->parent_menu,
			     MR_SELECTED_ITEM(pmp->parent_menu), False);
	tmi = find_entry(NULL, &tmrMi, None, -1, -1);
	if (pmp->parent_menu != tmrMi && MR_XANIMATION(pmp->menu) == 0)
	{
	  /* Warping didn't take us to the correct menu, i.e. the spot we want
	   * to warp to is obscured. So raise our window first. */
	  XRaiseWindow(dpy, MR_WINDOW(pmp->parent_menu));
	}
      }
    }
    break;

  case MENU_SELECTED:
    /* save action to execute so that the menu may be destroyed now */
    if (pmp->ret_paction)
      *pmp->ret_paction = (mi) ? safestrdup(MI_ACTION(mi)) : NULL;
    pmret->rc = MENU_DONE;
    if (pmp->ret_paction && *pmp->ret_paction && mi && MI_IS_POPUP(mi))
    {
      get_popup_options(pmp, mi, &mops);
      if (mops.flags.do_select_in_place)
      {
	MenuRoot *submenu;
	XWindowAttributes win_attribs;

	last_saved_pos_hints.flags.is_last_menu_pos_hints_valid = True;
	last_saved_pos_hints.pos_hints.x_offset = 0;
	last_saved_pos_hints.pos_hints.x_factor = 0;
	last_saved_pos_hints.pos_hints.context_x_factor = 0;
	last_saved_pos_hints.pos_hints.y_factor = 0;
	last_saved_pos_hints.pos_hints.is_relative = False;
	last_saved_pos_hints.pos_hints.is_menu_relative = False;
	submenu = mr_popup_for_mi(pmp->menu, mi);
	if (submenu &&
	    MR_WINDOW(submenu) != None &&
	    XGetWindowAttributes(dpy, MR_WINDOW(submenu), &win_attribs) &&
	    win_attribs.map_state == IsViewable &&
	    XGetGeometry(dpy, MR_WINDOW(submenu), &JunkRoot,
			 &last_saved_pos_hints.pos_hints.x,
			 &last_saved_pos_hints.pos_hints.y,
			 &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth))
	{
	  /* The submenu is mapped, just take its position and put it in the
	   * position hints. */
	}
	else if (mops.flags.has_poshints)
	{
	  last_saved_pos_hints.pos_hints = mops.pos_hints;
	}
	else
	{
	  Bool dummy;

	  get_prefered_popup_position(
	    pmp->menu, submenu, &last_saved_pos_hints.pos_hints.x,
	    &last_saved_pos_hints.pos_hints.y, &dummy);
	}
	if (mops.flags.do_warp_on_select)
	{
	  *pdo_warp_to_title = True;
	}
      } /* select in place */
      else
      {
	last_saved_pos_hints.flags.do_ignore_pos_hints = True;
      } /* mops.flags.do_select_in_place */
      last_saved_pos_hints.pos_hints.screen_origin_x = pmp->screen_origin_x;
      last_saved_pos_hints.pos_hints.screen_origin_y = pmp->screen_origin_y;
    } /* a menu was selected */
    break;
#ifdef TEAR_OFF_MENUS
  case MENU_TEAR_OFF:
fprintf(stderr,"got MENU_TEAR_OFF\n");
    AddWindow(MR_WINDOW(pmp->menu), NULL);
    break;
#endif
  default:
    break;
  }

  if (mrPopup)
  {
    pop_menu_down_and_repaint_parent(&mrPopup, &does_submenu_overlap, pmp);
    MR_SUBMENU_ITEM(pmp->menu) = NULL;
  }
  pmret->flags.is_key_press = flags.is_key_press;

  return;
}

static int do_menus_overlap(
  MenuRoot *mr, int x, int y, int width, int height, int h_tolerance,
  int v_tolerance, int s_tolerance, Bool allow_popup_offset_tolerance)
{
  int prior_x, prior_y, x_overlap;
  unsigned int prior_width, prior_height;

  if (mr == NULL)
    return 0;
  if (!XGetGeometry(dpy,MR_WINDOW(mr), &JunkRoot,&prior_x,&prior_y,
		    &prior_width,&prior_height, &JunkBW, &JunkDepth))
  {
    return 0;
  }
  x_overlap = 0;
  if (allow_popup_offset_tolerance)
  {
    if (MST_POPUP_OFFSET_ADD(mr) < 0)
      s_tolerance = -MST_POPUP_OFFSET_ADD(mr);
    if (MST_USE_LEFT_SUBMENUS(mr))
      prior_x += (100 - MST_POPUP_OFFSET_PERCENT(mr)) / 100;
    else
      prior_width *= (float)(MST_POPUP_OFFSET_PERCENT(mr)) / 100.0;
  }
  if (MST_USE_LEFT_SUBMENUS(mr))
  {
    int t = s_tolerance;
    s_tolerance = h_tolerance;
    h_tolerance = t;
  }
  if (y < prior_y + prior_height - v_tolerance &&
      prior_y < y + height - v_tolerance &&
      x < prior_x + prior_width - s_tolerance &&
      prior_x < x + width - h_tolerance)
  {
    x_overlap = x - prior_x;
    if (x <= prior_x)
    {
      x_overlap--;
    }
  }
  return x_overlap;
}

/***********************************************************************
 *
 *  Procedure:
 *	pop_menu_up - pop up a pull down menu
 *
 *  Inputs:
 *	x, y	  - location of upper left of menu
 *	do_warp_to_item - warp pointer to the first item after title
 *	pops	  - pointer to the menu options for new menu
 *
 ***********************************************************************/
static int pop_menu_up(
  MenuRoot **pmenu, MenuParameters *pmp, MenuRoot *parent_menu,
  MenuItem *parent_item,
  FvwmWindow **pfw, int *pcontext, int x, int y, Bool prefer_left_submenu,
  Bool do_warp_to_item, MenuOptions *pops, Bool *ret_overlap,
  Bool *pdo_warp_to_title, Window popdown_window)
{
  Bool do_warp_to_title = False;
  int x_overlap = 0;
  int x_clipped_overlap;
  MenuRoot *mrMi;
  MenuRoot *mr;
  FvwmWindow *fw;
  int context;
  int bw = 0;
  int bwp = 0;
  int prev_x;
  int prev_y;
  unsigned int prev_width;
  unsigned int prev_height;
  int scr_x, scr_y, scr_w, scr_h;

  mr = *pmenu;
  if (!mr || (MR_MAPPED_COPIES(mr) > 0 && MR_COPIES(mr) >= MAX_MENU_COPIES))
  {
    *pdo_warp_to_title = False;
    return False;
  }

  /***************************************************************
   * handle dynamic menu actions
   ***************************************************************/

  /* First of all, execute the popup action (if defined). */
  if (MR_POPUP_ACTION(mr))
  {
    char *menu_name;
    saved_pos_hints pos_hints;
    Bool f;
    Bool is_busy_grabbed = False;
    Time t;
    exec_func_args_type efa;
    extern XEvent Event;
    int mapped_copies = MR_MAPPED_COPIES(mr);

    /* save variables that we still need but that may be overwritten */
    menu_name = safestrdup(MR_NAME(mr));
    pos_hints = last_saved_pos_hints;
    f = *pdo_warp_to_title;
    t = lastTimestamp;
    if (Scr.BusyCursor & BUSY_DYNAMICMENU)
      is_busy_grabbed = GrabEm(CRS_WAIT, GRAB_BUSYMENU);
    /* Execute the action */
    memset(&efa, 0, sizeof(efa));
    efa.eventp = &Event;
    efa.tmp_win = *pfw;
    efa.action = MR_POPUP_ACTION(mr);
    efa.context = *pcontext;
    efa.module = -2;
    efa.flags.do_save_tmpwin = 1;
    efa.flags.exec = FUNC_DONT_EXPAND_COMMAND;
    execute_function(&efa);
    if (is_busy_grabbed)
      UngrabEm(GRAB_BUSYMENU);
    /* restore the stuff we saved */
    lastTimestamp = t;
    *pdo_warp_to_title = f;
    last_saved_pos_hints = pos_hints;
    /* See if the window has been deleted */
    if (!check_if_fvwm_window_exists(*pfw))
    {
      *pfw = NULL;
      *pcontext = 0;
    }
    if (mapped_copies == 0)
    {
      /* Now let's see if the menu still exists. It may have been destroyed and
       * recreated, so we have to look for a menu with the saved name.
       * FindPopup() will always return the original menu, not one of its
       * copies, so below logic would fail miserably if used with a menu copy.
       * On the other hand, menu copies can't be deleted within a dynamic popup
       * action, so just ignore this case. */
      *pmenu = FindPopup(menu_name);
      if (menu_name)
      {
	free(menu_name);
      }
      mr = *pmenu;
#if 0
      if (mr)
      {
	make_menu(mr, pmp);
      }
#endif
    }
  }
#if 1
  if (mr)
  {
    update_menu(mr, pmp);
  }
#endif
  if (mr == NULL || MR_FIRST_ITEM(mr) == NULL || MR_ITEMS(mr) == 0)
  {
    /* The menu deleted itself or all its items or it has been empty from the
     * start. */
    *pdo_warp_to_title = False;
    return False;
  }
  fw = *pfw;
  if (fw && !check_if_fvwm_window_exists(fw))
    fw = NULL;
  context = *pcontext;

  /***************************************************************
   * Create a new menu instance (if necessary)
   ***************************************************************/

  if (MR_MAPPED_COPIES(mr) > 0)
  {
    /* create a new instance of the menu */
    *pmenu = copy_menu_root(*pmenu);
    if (!*pmenu)
      return False;
    make_menu_window(*pmenu);
    mr = *pmenu;
  }

  /***************************************************************
   * Evaluate position hints
   ***************************************************************/

  /* calculate position from position hints if available */
  if (pops->flags.has_poshints &&
      !last_saved_pos_hints.flags.do_ignore_pos_hints)
  {
    get_xy_from_position_hints(
      &(pops->pos_hints), MR_WIDTH(mr), MR_HEIGHT(mr),
      (parent_menu) ? MR_WIDTH(parent_menu) : 0, prefer_left_submenu, &x, &y);
  }

  /***************************************************************
   * Initialise new menu
   ***************************************************************/

  MR_PARENT_MENU(mr) = parent_menu;
  MR_PARENT_ITEM(mr) = parent_item;
  MR_IS_PAINTED(mr) = 0;
  MR_IS_LEFT(mr) = 0;
  MR_IS_RIGHT(mr) = 0;
  MR_IS_UP(mr) = 0;
  MR_IS_DOWN(mr) = 0;
  MR_XANIMATION(mr) = 0;
  InstallFvwmColormap();

  /***************************************************************
   * Handle popups from button clicks on buttons in the title bar,
   * or the title bar itself. Position hints override this.
   ***************************************************************/

  if (!pops->flags.has_poshints && fw && parent_menu == NULL &&
      pmp->flags.is_first_root_menu)
  {
    int old_y = y;
    int cx;
    int cy;
    Bool has_context;

    if (HAS_BOTTOM_TITLE(fw))
    {
      y = fw->frame_g.y + fw->frame_g.height -
	fw->boundary_width - fw->title_g.height - MR_HEIGHT(mr);
      cy = fw->frame_g.y - fw->boundary_width - fw->title_g.height / 2;
    }
    else
    {
      y = fw->frame_g.y + fw->boundary_width + fw->title_g.height;
      cy = y - fw->title_g.height / 2;
    }
    has_context = 0;
    if (context&C_LALL)
    {
      x = fw->frame_g.x + fw->boundary_width +
	ButtonPosition(context, fw) * fw->title_g.height;
      cx = x + fw->title_g.height / 2;
      has_context = 1;
    }
    else if (context&C_RALL)
    {
      x = fw->frame_g.x + fw->frame_g.width -
	fw->boundary_width - ButtonPosition(context, fw) *
	fw->title_g.height - MR_WIDTH(mr);
      cx = x + fw->title_g.height / 2;
      has_context = 1;
    }
    else if (context&C_TITLE)
    {
      if (x < fw->frame_g.x + fw->title_g.x)
      {
	x = fw->frame_g.x + fw->title_g.x;
      }
      if (x + MR_WIDTH(mr) >
	  fw->frame_g.x + fw->title_g.x + fw->title_g.width)
      {
	x = fw->frame_g.x + fw->title_g.x + fw->title_g.width - MR_WIDTH(mr);
      }
      cx = x;
      has_context = 1;
    }
    else
    {
      y = old_y;
    }
    if (has_context)
    {
      fscreen_scr_arg fscr;

      pops->pos_hints.has_screen_origin = True;
      fscr.xypos.x = cx;
      fscr.xypos.y = cy;
      if (FScreenGetScrRect(
	    &fscr, FSCREEN_XYPOS, &JunkX, &JunkY, &JunkWidth, &JunkHeight))
      {
	/* use current cx/cy */
      }
      else if (XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
			     &cx, &cy, &JunkX, &JunkY, &JunkMask))
      {
	/* use pointer's position */
      }
      else
      {
	cx = -1;
	cy = -1;
      }
      pops->pos_hints.screen_origin_x = cx;
      pops->pos_hints.screen_origin_y = cy;
    }
  } /* if (pops->flags.has_poshints) */

  /***************************************************************
   * Clip to screen
   ***************************************************************/

  if (parent_menu)
  {
    bw = MST_BORDER_WIDTH(mr);
    bwp = MST_BORDER_WIDTH(parent_menu);
    x_overlap =
      do_menus_overlap(parent_menu, x, y, MR_WIDTH(mr), MR_HEIGHT(mr),
		       bwp, bwp, bwp, True);
  }

  {
    fscreen_scr_arg fscr;

    fscr.xypos.x = pops->pos_hints.screen_origin_x;
    fscr.xypos.y = pops->pos_hints.screen_origin_y;
    /* clip to screen */
    FScreenClipToScreen(
      &fscr, FSCREEN_XYPOS, &x, &y, MR_WIDTH(mr), MR_HEIGHT(mr));
    /* "this" screen is defined -- so get its coords for future reference */
    FScreenGetScrRect(&fscr, FSCREEN_XYPOS, &scr_x, &scr_y, &scr_w, &scr_h);
  }

  /***************************************************************
   * Calculate position and animate menus
   ***************************************************************/

  if (parent_menu == NULL ||
      !XGetGeometry(dpy, MR_WINDOW(parent_menu), &JunkRoot, &prev_x, &prev_y,
		    &prev_width,&prev_height, &JunkBW, &JunkDepth))
  {
    MR_HAS_POPPED_UP_LEFT(mr) = 0;
    MR_HAS_POPPED_UP_RIGHT(mr) = 0;
  }
  else
  {
    int left_x;
    int right_x;
    Bool use_left_submenus = MST_USE_LEFT_SUBMENUS(mr);

    /* check if menus overlap */
    x_clipped_overlap =
      do_menus_overlap(parent_menu, x, y, MR_WIDTH(mr), MR_HEIGHT(mr),
		       bwp, bwp, bwp, True);
    if (x_clipped_overlap &&
	(!pops->flags.has_poshints || pops->pos_hints.is_relative == False ||
	 x_overlap == 0))
    {
      /* menus do overlap, but do not reposition if overlap was caused by
       * relative positioning hints */
      left_x = get_left_popup_x_position(parent_menu, mr, prev_x);
      right_x = get_right_popup_x_position(parent_menu, mr, prev_x);
      if (use_left_submenus)
      {
	if (left_x + MR_WIDTH(mr) < prev_x + bwp)
	  left_x = prev_x + bwp - MR_WIDTH(mr);
      }
      else
      {
	if (right_x > prev_x + prev_width - bwp)
	  right_x = prev_x + prev_width - bwp;
      }

      /*
       * Animate parent menu
       */

      if (MST_IS_ANIMATED(mr))
      {
	/* animate previous out of the way */
	int a_left_x;
	int a_right_x;
	int end_x;

	if (use_left_submenus)
	{
	  if (prev_x - left_x < MR_WIDTH(mr))
	    a_right_x = x + (prev_x - left_x);
	  else
	    a_right_x = x + MR_WIDTH(mr) - bw;
	  a_left_x = x - MR_WIDTH(parent_menu);
	}
	else
	{
	  if (right_x - prev_x < prev_width)
	    a_left_x = x + (prev_x - right_x);
	  else
	    a_left_x = x - prev_width + bw;
	  a_right_x = x + MR_WIDTH(mr);
	}
	if (prefer_left_submenu)
	{
	  /* popup menu is left of old menu, try to move prior menu right */
          if (a_right_x + prev_width <= scr_x + scr_w)
	    end_x = a_right_x;
	  else if (a_left_x >= scr_x)
	    end_x = a_left_x;
	  else
            end_x = scr_x + scr_w - prev_width;
	}
	else
	{
	  /* popup menu is right of old menu, try to move prior menu left */
	  if (a_left_x >= scr_x)
	    end_x = a_left_x;
	  else if (a_right_x + prev_width <= scr_x + scr_w)
	    end_x = a_right_x;
	  else
	    end_x = scr_x;
	}
	if (end_x == a_left_x || end_x == 0)
	{
	  MR_HAS_POPPED_UP_LEFT(mr) = 0;
	  MR_HAS_POPPED_UP_RIGHT(mr) = 1;
	}
	else
	{
	  MR_HAS_POPPED_UP_LEFT(mr) = 1;
	  MR_HAS_POPPED_UP_RIGHT(mr) = 0;
	}
	MR_XANIMATION(parent_menu) += end_x - prev_x;
	AnimatedMoveOfWindow(MR_WINDOW(parent_menu),prev_x,prev_y,end_x,prev_y,
			     True, -1, NULL);
      } /* if (MST_IS_ANIMATED(mr)) */

      /*
       * Try the other side of the parent menu
       */

      else if (!pops->flags.is_fixed)
      {
	Bool may_use_left;
	Bool may_use_right;
	Bool do_use_left;
	Bool use_left_as_last_resort;

	use_left_as_last_resort =
	  (left_x > scr_x + scr_w - right_x - MR_WIDTH(mr));
	may_use_left = (left_x >= scr_x);
	may_use_right = (right_x + MR_WIDTH(mr) <= scr_x + scr_w);
	if (!may_use_left && !may_use_right)
	{
	  /* If everything goes wrong, put the submenu on the side with more
	   * free space. */
	  do_use_left = use_left_as_last_resort;
	}
	else if (may_use_left && (prefer_left_submenu || !may_use_right))
	  do_use_left = True;
	else
	  do_use_left = False;
	x = (do_use_left) ? left_x : right_x;
	MR_HAS_POPPED_UP_LEFT(mr) = do_use_left;
	MR_HAS_POPPED_UP_RIGHT(mr) = !do_use_left;

	/* Force the menu onto the screen, but leave at least
	 * PARENT_MENU_FORCE_VISIBLE_WIDTH pixels of the parent menu visible */
	if (x + MR_WIDTH(mr) > scr_x + scr_w)
	{
	  int d = x + MR_WIDTH(mr) - (scr_x + scr_w);
	  int c;

	  if (prev_width >= PARENT_MENU_FORCE_VISIBLE_WIDTH)
	    c = prev_x + PARENT_MENU_FORCE_VISIBLE_WIDTH;
	  else
	    c = prev_x + prev_width;

	  if (x - c >= d || x <= prev_x)
	    x -= d;
	  else if (x > c)
	    x = c;
	}
        if (x < scr_x)
	{
	  int c = prev_width - PARENT_MENU_FORCE_VISIBLE_WIDTH;

	  if (c < 0)
	    c = 0;
	  if (scr_x - x > c)
	    x += c;
	  else
	    x = scr_x;
	}
      } /* else if (non-overlapping menu style) */
    } /* if (x_clipped_overlap && ...) */
    else
    {
      MR_HAS_POPPED_UP_LEFT(mr) = prefer_left_submenu;
      MR_HAS_POPPED_UP_RIGHT(mr) = !prefer_left_submenu;
    }

    if (x < prev_x)
      MR_IS_LEFT(mr) = 1;
    if (x + MR_WIDTH(mr) > prev_x + prev_width)
      MR_IS_RIGHT(mr) = 1;
    if (y < prev_y)
      MR_IS_UP(mr) = 1;
    if (y + MR_HEIGHT(mr) > prev_y + prev_height)
      MR_IS_DOWN(mr) = 1;
    if (!MR_IS_LEFT(mr) && !MR_IS_RIGHT(mr))
    {
      MR_IS_LEFT(mr) = 1;
      MR_IS_RIGHT(mr) = 1;
    }
  } /* if (parent_menu) */

  /***************************************************************
   * Pop up the menu
   ***************************************************************/

  XMoveWindow(dpy, MR_WINDOW(mr), x, y);
  XMapRaised(dpy, MR_WINDOW(mr));
  if (popdown_window)
    XUnmapWindow(dpy, popdown_window);
  XFlush(dpy);
  MR_MAPPED_COPIES(mr)++;
  MST_USAGE_COUNT(mr)++;
  if (ret_overlap)
  {
    *ret_overlap =
      do_menus_overlap(parent_menu, x, y, MR_WIDTH(mr), MR_HEIGHT(mr),
		       0, 0, 0, False) ? True : False;
  }

  /***************************************************************
   * Warp the pointer
   ***************************************************************/

  if (!do_warp_to_item)
  {
    MenuItem *mi;

    mi = find_entry(NULL, &mrMi, None, -1, -1);
    if (mi && mrMi == mr && mi != MR_FIRST_ITEM(mrMi))
    {
      /* pointer is on an item of the popup */
      if (MST_DO_WARP_TO_TITLE(mr))
      {
	/* warp pointer if not on a root menu */
	do_warp_to_title = True;
      }
    }
  } /* if (!do_warp_to_item) */

  if (pops->flags.do_not_warp)
  {
    do_warp_to_title = False;
  }
  else if (pops->flags.do_warp_title)
  {
    do_warp_to_title = True;
  }
  if (*pdo_warp_to_title)
  {
    do_warp_to_title = True;
    *pdo_warp_to_title = False;
  }
  if (do_warp_to_item)
  {
    /* also warp */
    MR_SELECTED_ITEM(mr) = NULL;
    warp_pointer_to_item(mr, MR_FIRST_ITEM(mr), True /* skip Title */);
    select_menu_item(mr, MR_SELECTED_ITEM(mr), True, fw);
  }
  else if(do_warp_to_title)
  {
    /* Warp pointer to middle of top line, since we don't
     * want the user to come up directly on an option */
    warp_pointer_to_title(mr);
  }
  return True;
}

/* Set the selected-ness state of the menuitem passed in */
static void select_menu_item(
  MenuRoot *mr, MenuItem *mi, Bool select, FvwmWindow *fw)
{
  if (select == True && MR_SELECTED_ITEM(mr) != NULL &&
      MR_SELECTED_ITEM(mr) != mi)
  {
    select_menu_item(mr, MR_SELECTED_ITEM(mr), False, fw);
  }
  else if (select == False && MR_SELECTED_ITEM(mr) == NULL)
    return;
  else if (select == True && MR_SELECTED_ITEM(mr) == mi)
    return;
  if (!MI_IS_SELECTABLE(mi))
    return;

  if (select == False)
    MI_WAS_DESELECTED(mi) = True;

  if (!MST_HAS_MENU_CSET(mr))
  {
    switch (MST_FACE(mr).type)
    {
    case GradientMenu:
      if (select == True)
      {
	int iy;
	int ih;
	unsigned int mw;
	XEvent e;

	if (!MR_IS_PAINTED(mr))
	{
	  flush_expose(MR_WINDOW(mr));
	  paint_menu(mr, NULL, fw);
	}
	iy = MI_Y_OFFSET(mi);
	ih = MI_HEIGHT(mi) +
	  (MI_IS_SELECTABLE(mi) ? MST_RELIEF_THICKNESS(mr) : 0);
	if (iy < 0)
	{
	  ih += iy;
	  iy = 0;
	}
	mw = MR_WIDTH(mr) - 2 * MST_BORDER_WIDTH(mr);
	if (iy + ih > MR_HEIGHT(mr))
	  ih = MR_HEIGHT(mr) - iy;
	/* grab image */
	MR_STORED_ITEM(mr).stored =
	  XCreatePixmap(dpy, Scr.NoFocusWin, mw, ih, Pdepth);
	XSetGraphicsExposures(dpy, MST_MENU_GC(mr), True);
	XSync(dpy, 0);
	while (XCheckTypedEvent(dpy, NoExpose, &e))
	  ;
	XCopyArea(
	  dpy, MR_WINDOW(mr), MR_STORED_ITEM(mr).stored,
	  MST_MENU_GC(mr), MST_BORDER_WIDTH(mr), iy, mw, ih, 0, 0);
	XSync(dpy, 0);
	if (XCheckTypedEvent(dpy, NoExpose, &e))
	{
	  MR_STORED_ITEM(mr).y = iy;
	  MR_STORED_ITEM(mr).width = mw;
	  MR_STORED_ITEM(mr).height = ih;
	}
	else
	{
	  XFreePixmap(dpy, MR_STORED_ITEM(mr).stored);
	  MR_STORED_ITEM(mr).stored = None;
	  MR_STORED_ITEM(mr).width = 0;
	  MR_STORED_ITEM(mr).height = 0;
	  MR_STORED_ITEM(mr).y = 0;
	}
	XSetGraphicsExposures(dpy, MST_MENU_GC(mr), False);
      }
      else if (select == False && MR_STORED_ITEM(mr).width != 0)
      {
	/* ungrab image */
	XCopyArea(dpy, MR_STORED_ITEM(mr).stored, MR_WINDOW(mr),
		  MST_MENU_GC(mr), 0, 0,
		  MR_STORED_ITEM(mr).width, MR_STORED_ITEM(mr).height,
		  MST_BORDER_WIDTH(mr), MR_STORED_ITEM(mr).y);

	if (MR_STORED_ITEM(mr).stored != None)
	  XFreePixmap(dpy, MR_STORED_ITEM(mr).stored);
	MR_STORED_ITEM(mr).stored = None;
	MR_STORED_ITEM(mr).width = 0;
	MR_STORED_ITEM(mr).height = 0;
	MR_STORED_ITEM(mr).y = 0;
      }
      else if (select == False &&
	       (MST_FACE(mr).gradient_type == D_GRADIENT ||
		MST_FACE(mr).gradient_type == B_GRADIENT))
      {
	XEvent e;
	Picture *sidePic = NULL;

	e.type = Expose;
	e.xexpose.x = MST_BORDER_WIDTH(mr);
	e.xexpose.y = MI_Y_OFFSET(mi);
	e.xexpose.width = MR_WIDTH(mr) - 2 * MST_BORDER_WIDTH(mr);
	e.xexpose.height = MI_HEIGHT(mi) +
	  (MI_IS_SELECTABLE(mi) ? MST_RELIEF_THICKNESS(mr) : 0);;
	if (MR_SIDEPIC(mr))
	  sidePic = MR_SIDEPIC(mr);
	else if (MST_SIDEPIC(mr))
	  sidePic = MST_SIDEPIC(mr);
	if (sidePic)
	{
	  e.xexpose.width -= sidePic->width;
	  if (MR_SIDEPIC_X_OFFSET(mr) == MST_BORDER_WIDTH(mr))
	    e.xexpose.x += sidePic->width;
	}
	MR_SELECTED_ITEM(mr) = (select) ? mi : NULL;
	paint_menu(mr, &e, fw);
      }
      break;
    default:
      if (MR_STORED_ITEM(mr).width != 0)
      {
	if (MR_STORED_ITEM(mr).stored != None)
	  XFreePixmap(dpy, MR_STORED_ITEM(mr).stored);
	MR_STORED_ITEM(mr).stored = None;
	MR_STORED_ITEM(mr).width = 0;
	MR_STORED_ITEM(mr).height = 0;
	MR_STORED_ITEM(mr).y = 0;
      }
      break;
    } /* switch */
  } /* if */

  if (fw && !check_if_fvwm_window_exists(fw))
    fw = NULL;
  MR_SELECTED_ITEM(mr) = (select) ? mi : NULL;
  paint_item(mr, mi, fw, False);
}

/***********************************************************************
 *
 *  Procedure:
 *	pop_menu_down - unhighlight the current menu selection and
 *		      take down the menus
 *
 *	mr     - menu to pop down; this pointer is invalid after the function
 *		 returns. Don't use it anymore!
 *	parent - the menu that has spawned mr (may be NULL). this is
 *		 used to see if mr was spawned by itself on some level.
 *		 this is a hack to allow specifying 'Popup foo' within
 *		 menu foo. You must use the MenuRoot that is currently
 *		 being processed here. DO NOT USE MR_PARENT_MENU(mr) here!
 *
 ***********************************************************************/
static void pop_menu_down(MenuRoot **pmr, MenuParameters *pmp)
{
  MenuItem *mi;
  assert(*pmr);

  memset(&(MR_DYNAMIC_FLAGS(*pmr)), 0, sizeof(MR_DYNAMIC_FLAGS(*pmr)));
  XUnmapWindow(dpy, MR_WINDOW(*pmr));
  MR_MAPPED_COPIES(*pmr)--;
  MST_USAGE_COUNT(*pmr)--;
  UninstallFvwmColormap();
  XFlush(dpy);
  if (*(pmp->pcontext) & (C_WINDOW | C_FRAME | C_TITLE | C_SIDEBAR))
    pmp->flags.is_menu_from_frame_or_window_or_titlebar = True;
  else
    pmp->flags.is_menu_from_frame_or_window_or_titlebar = False;
  if ((mi = MR_SELECTED_ITEM(*pmr)) != NULL)
  {
    select_menu_item(*pmr, mi, False, (*pmp->pTmp_win));
  }

  if (MR_COPIES(*pmr) > 1)
  {
    /* delete this instance of the menu */
    DestroyMenu(*pmr, False, False);
  }
  else if (MR_POPDOWN_ACTION(*pmr))
  {
    /* Finally execute the popdown action (if defined). */
    saved_pos_hints pos_hints;
    exec_func_args_type efa;
    Time t;
    extern XEvent Event;

    /* save variables that we still need but that may be overwritten */
    pos_hints = last_saved_pos_hints;
    t = lastTimestamp;
    /* Execute the action */
    memset(&efa, 0, sizeof(efa));
    efa.eventp = &Event;
    efa.tmp_win = (*pmp->pTmp_win);
    efa.action = MR_POPDOWN_ACTION(*pmr);
    efa.context = *(pmp->pcontext);
    efa.module = -2;
    efa.flags.do_save_tmpwin = 1;
    efa.flags.exec = FUNC_DONT_EXPAND_COMMAND;
    execute_function(&efa);
    /* restore the stuff we saved */
    last_saved_pos_hints = pos_hints;
    lastTimestamp = t;
    if (!check_if_fvwm_window_exists(*(pmp->pTmp_win)))
    {
      *(pmp->pTmp_win) = NULL;
      *(pmp->pcontext) = 0;
    }
  }

  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	pop_menu_down_and_repaint_parent - Pops down a menu and repaints the
 *	overlapped portions of the parent menu. This is done only if
 *	*fSubmenuOverlaps is True. *fSubmenuOverlaps is set to False
 *	afterwards.
 *
 ***********************************************************************/
static void pop_menu_down_and_repaint_parent(
  MenuRoot **pmr, Bool *fSubmenuOverlaps, MenuParameters *pmp)
{
  MenuRoot *parent = MR_PARENT_MENU(*pmr);
  Window win;
  XEvent event;
  int mr_x;
  int mr_y;
  unsigned int mr_width;
  unsigned int mr_height;
  int parent_x;
  int parent_y;
  unsigned int parent_width;
  unsigned int parent_height;

  if (*fSubmenuOverlaps && parent)
  {
    /* popping down the menu may destroy the menu via the dynamic popdown
     * action! Thus we must not access *pmr afterwards. */
    win = MR_WINDOW(*pmr);

    /* Create a fake event to pass into paint_menu */
    event.type = Expose;
    if (!XGetGeometry(dpy, win, &JunkRoot, &mr_x, &mr_y,
		      &mr_width, &mr_height, &JunkBW, &JunkDepth) ||
	!XGetGeometry(dpy, MR_WINDOW(parent), &JunkRoot, &parent_x, &parent_y,
		       &parent_width, &parent_height, &JunkBW, &JunkDepth))
    {
      pop_menu_down(pmr, pmp);
      paint_menu(parent, NULL, (*pmp->pTmp_win));
    }
    else
    {
      pop_menu_down(pmr, pmp);
      event.xexpose.x = mr_x - parent_x;
      event.xexpose.width = mr_width;
      if (event.xexpose.x < 0)
      {
	event.xexpose.width += event.xexpose.x;
	event.xexpose.x = 0;
      }
      if (event.xexpose.x + event.xexpose.width > parent_width)
      {
	event.xexpose.width = parent_width - event.xexpose.x;
      }
      event.xexpose.y = mr_y - parent_y;
      event.xexpose.height = mr_height;
      if (event.xexpose.y < 0)
      {
	event.xexpose.height += event.xexpose.y;
	event.xexpose.y = 0;
      }
      if (event.xexpose.y + event.xexpose.height > parent_height)
      {
	event.xexpose.height = parent_height - event.xexpose.y;
      }
      flush_accumulate_expose(MR_WINDOW(parent), &event);
      paint_menu(parent, &event, (*pmp->pTmp_win));
    }
  }
  else
  {
    /* popping down the menu may destroy the menu via the dynamic popdown
     * action! Thus we must not access *pmr afterwards. */
    pop_menu_down(pmr, pmp);
  }
  *fSubmenuOverlaps = False;
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	paint_item - draws a single entry in a popped up menu
 *
 *	mr - the menu instance that holds the menu item
 *	mi - the menu item to redraw
 *	fw - the FvwmWindow structure to check against allowed functions
 *
 ***********************************************************************/
static void paint_item(MenuRoot *mr, MenuItem *mi, FvwmWindow *fw,
		       Bool do_redraw_menu_border)
{
  int y_offset;
  int text_y;
  int y_height;
  int x;
  int y;
  GC ShadowGC, ReliefGC, currentGC;
  short relief_thickness = MST_RELIEF_THICKNESS(mr);
  Bool is_item_selected;
  int i;
  int sx1;
  int sx2;

  if (!mi)
    return;
  is_item_selected = (mi == MR_SELECTED_ITEM(mr));

  y_offset = MI_Y_OFFSET(mi);
  y_height = MI_HEIGHT(mi);
  if (MI_IS_SELECTABLE(mi))
  {
    text_y = y_offset + MR_ITEM_TEXT_Y_OFFSET(mr);
  }
  else
  {
    text_y = y_offset + MST_PSTDFONT(mr)->y + MST_TITLE_GAP_ABOVE(mr);
  }
  /* center text vertically if the pixmap is taller */
  if (MI_PICTURE(mi))
  {
    text_y += MI_PICTURE(mi)->height;
  }
  for (i = 0; i < MAX_MENU_ITEM_MINI_ICONS; i++)
  {
    y = 0;
    if (MI_MINI_ICON(mi)[i])
    {
      if (MI_MINI_ICON(mi)[i]->height > y)
	y = MI_MINI_ICON(mi)[i]->height;
    }
    y -= MST_PSTDFONT(mr)->height;
    if (y > 1)
    {
      text_y += y / 2;
    }
  }

  ShadowGC = MST_MENU_SHADOW_GC(mr);
  if(Pdepth<2)
    ReliefGC = MST_MENU_SHADOW_GC(mr);
  else
    ReliefGC = MST_MENU_RELIEF_GC(mr);

  /***************************************************************
   * Hilight the item.
   ***************************************************************/

  /* Hilight or clear the background. */
  if (is_item_selected && MST_DO_HILIGHT(mr))
  {
    /* Hilight the background. */
    if (MR_HILIGHT_WIDTH(mr) - 2 * relief_thickness > 0)
    {
      XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
      XFillRectangle(dpy, MR_WINDOW(mr), MST_MENU_ACTIVE_BACK_GC(mr),
		     MR_HILIGHT_X_OFFSET(mr) + relief_thickness,
		     y_offset + relief_thickness,
		     MR_HILIGHT_WIDTH(mr) - 2 * relief_thickness,
		     y_height - relief_thickness);
    }
  }
  else if (MI_WAS_DESELECTED(mi) &&
	   (relief_thickness > 0 || MST_DO_HILIGHT(mr)) &&
	   (MST_FACE(mr).type != GradientMenu || MST_HAS_MENU_CSET(mr)))
  {
    int d = 0;
    if (MI_PREV_ITEM(mi) && MR_SELECTED_ITEM(mr) == MI_PREV_ITEM(mi))
    {
      /* Don't paint over the hilight relief. */
      d = relief_thickness;
    }
    /* Undo the hilighting. */
    XClearArea(dpy, MR_WINDOW(mr), MR_ITEM_X_OFFSET(mr), y_offset + d,
	       MR_ITEM_WIDTH(mr), y_height + relief_thickness - d, 0);
  }

  MI_WAS_DESELECTED(mi) = False;
  /* Hilight 3D */
  if (is_item_selected && relief_thickness > 0)
  {
    GC rgc = ReliefGC;
    GC sgc = ShadowGC;
    if (MST_HAS_ACTIVE_CSET(mr))
    {
      rgc = MST_MENU_ACTIVE_RELIEF_GC(mr);
      sgc = MST_MENU_ACTIVE_SHADOW_GC(mr);
    }
    if (MST_IS_ITEM_RELIEF_REVERSED(mr))
    {
      GC tgc = rgc;

      /* swap gcs for reversed relief */
      rgc = sgc;
      sgc = tgc;
    }
    if (MR_HILIGHT_WIDTH(mr) - 2 * relief_thickness > 0)
    {
      /* The relief reaches down into the next item, hence the value for the
       * second y coordinate: MI_HEIGHT(mi) + 1 */
      RelieveRectangle(
	dpy, MR_WINDOW(mr),  MR_HILIGHT_X_OFFSET(mr), y_offset,
	MR_HILIGHT_WIDTH(mr) - 1, MI_HEIGHT(mi) - 1 + relief_thickness,
	rgc, sgc, relief_thickness);
    }
  }


  /***************************************************************
   * Draw the item itself.
   ***************************************************************/

  /* Calculate the separator offsets. */
  if (MST_HAS_LONG_SEPARATORS(mr))
  {
    sx1 = MR_ITEM_X_OFFSET(mr) + relief_thickness;
    sx2 = MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) - 1 - relief_thickness;
  }
  else
  {
    sx1 = MR_ITEM_X_OFFSET(mr) + relief_thickness +
      MENU_SEPARATOR_SHORT_X_OFFSET;
    sx2 = MR_ITEM_X_OFFSET(mr) + MR_ITEM_WIDTH(mr) - 1 -
      relief_thickness - MENU_SEPARATOR_SHORT_X_OFFSET;
  }
  if (MI_IS_SEPARATOR(mi))
  {
    if (sx1 < sx2)
    {
      /* It's a separator. */
      draw_separator(MR_WINDOW(mr), ShadowGC, ReliefGC,
		     sx1, y_offset + y_height - MENU_SEPARATOR_HEIGHT,
		     sx2, y_offset + y_height - MENU_SEPARATOR_HEIGHT, 1);
      /* Nothing else to do. */
    }
    return;
  }
  else if(MI_IS_TITLE(mi))
  {
    /* Separate the title. */
    if (MST_TITLE_UNDERLINES(mr) > 0 && mi != MR_FIRST_ITEM(mr))
    {
      int add = (MI_IS_SELECTABLE(MI_PREV_ITEM(mi))) ? relief_thickness : 0;

      text_y += MENU_SEPARATOR_HEIGHT + add;
      y = y_offset + add;
      if (sx1 < sx2)
	draw_separator(MR_WINDOW(mr), ShadowGC, ReliefGC, sx1, y, sx2, y, 1);
    }
    /* Underline the title. */
    switch (MST_TITLE_UNDERLINES(mr))
    {
    case 0:
      break;
    case 1:
      if(MI_NEXT_ITEM(mi) != NULL)
      {
	y = y_offset + y_height - MENU_SEPARATOR_HEIGHT;
	draw_separator(MR_WINDOW(mr), ShadowGC, ReliefGC, sx1, y, sx2, y, 1);
      }
      break;
    default:
      for (i = MST_TITLE_UNDERLINES(mr); i-- > 0; )
      {
	y = y_offset + y_height - 1 - i * MENU_UNDERLINE_HEIGHT;
	XDrawLine(dpy, MR_WINDOW(mr), ShadowGC, sx1, y, sx2, y);
      }
      break;
    }
  }

  /* Note: it's ok to pass a NULL label to check_if_function_allowed. */
  if(check_if_function_allowed(MI_FUNC_TYPE(mi), fw, False, MI_LABEL(mi)[0]))
  {
    currentGC = (is_item_selected) ?
      MST_MENU_ACTIVE_GC(mr) : MST_MENU_GC(mr);
    if (MST_DO_HILIGHT(mr) &&
	!MST_HAS_ACTIVE_FORE(mr) &&
	!MST_HAS_ACTIVE_CSET(mr) &&
	is_item_selected)
    {
      /* Use a lighter color for highlighted windows menu items if the
       * background is hilighted */
      currentGC = MST_MENU_RELIEF_GC(mr);
    }
  }
  else
  {
    /* should be a shaded out word, not just re-colored. */
    currentGC = MST_MENU_STIPPLE_GC(mr);
  }


  /***************************************************************
   * Draw the labels.
   ***************************************************************/

  for (i = MAX_MENU_ITEM_LABELS; i-- > 0; )
  {
    if (MI_LABEL(mi)[i] && *(MI_LABEL(mi)[i]))
    {
#ifdef I18N_MB
      XmbDrawString(dpy, MR_WINDOW(mr), MST_PSTDFONT(mr)->fontset,
		    currentGC, MI_LABEL_OFFSET(mi)[i],
		    text_y, MI_LABEL(mi)[i], MI_LABEL_STRLEN(mi)[i]);
#else
      XDrawString(dpy, MR_WINDOW(mr),
		  currentGC, MI_LABEL_OFFSET(mi)[i],
		  text_y, MI_LABEL(mi)[i], MI_LABEL_STRLEN(mi)[i]);
#endif
    }
    if (MI_HAS_HOTKEY(mi) && !MI_IS_TITLE(mi) &&
	(!MI_IS_HOTKEY_AUTOMATIC(mi) || MST_USE_AUTOMATIC_HOTKEYS(mr)) &&
	MI_HOTKEY_COLUMN(mi) == i)
    {
      /* pete@tecc.co.uk: If the item has a hot key, underline it */
      draw_underline(mr, currentGC, MI_LABEL_OFFSET(mi)[i], text_y,
		     MI_LABEL(mi)[i], MI_HOTKEY_COFFSET(mi));
    }
  }


  /***************************************************************
   * Draw the submenu triangle.
   ***************************************************************/

  if(MI_IS_POPUP(mi))
  {
    y = y_offset + (y_height - MENU_TRIANGLE_HEIGHT + relief_thickness) / 2;
    DrawTrianglePattern(
      dpy, MR_WINDOW(mr),
      ReliefGC, ShadowGC, (is_item_selected) ? ReliefGC : MST_MENU_GC(mr),
      MR_TRIANGLE_X_OFFSET(mr), y, MENU_TRIANGLE_WIDTH, MENU_TRIANGLE_HEIGHT, 0,
      (MR_IS_LEFT_TRIANGLE(mr)) ? 'l' : 'r',
      MST_HAS_TRIANGLE_RELIEF(mr), !MST_HAS_TRIANGLE_RELIEF(mr),
      is_item_selected);
  }

  /***************************************************************
   * Draw the item picture.
   ***************************************************************/

  if(MI_PICTURE(mi))
  {
    x = menu_middle_x_offset(mr) - MI_PICTURE(mi)->width / 2;
    y = y_offset + ((MI_IS_SELECTABLE(mi)) ? relief_thickness : 0);

    if(MI_PICTURE(mi)->depth == Pdepth) /* pixmap */
    {
      Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
      Globalgcv.clip_mask = MI_PICTURE(mi)->mask;
      Globalgcv.clip_x_origin = x;
      Globalgcv.clip_y_origin = y;
      XChangeGC(dpy, ReliefGC, Globalgcm, &Globalgcv);
      XCopyArea(dpy, MI_PICTURE(mi)->picture, MR_WINDOW(mr), ReliefGC,
		0, 0, MI_PICTURE(mi)->width, MI_PICTURE(mi)->height, x, y);
      Globalgcm = GCClipMask;
      Globalgcv.clip_mask = None;
      XChangeGC(dpy, ReliefGC, Globalgcm, &Globalgcv);
    }
    else
    {
      XCopyPlane(dpy, MI_PICTURE(mi)->picture, MR_WINDOW(mr), currentGC,
		 0, 0, MI_PICTURE(mi)->width, MI_PICTURE(mi)->height, x, y, 1);
    }
  }

 /***************************************************************
  * Draw the mini icons.
  ***************************************************************/

  for (i = 0; i < MAX_MENU_ITEM_MINI_ICONS; i++)
  {
    int k;

    /* We need to reverse the mini icon order for left submenu style. */
    k = (MST_USE_LEFT_SUBMENUS(mr)) ?
      MAX_MENU_ITEM_MINI_ICONS - 1 - i : i;

    if (MI_MINI_ICON(mi)[i])
    {
      if (MI_PICTURE(mi))
      {
	y = y_offset + MI_HEIGHT(mi) - MI_MINI_ICON(mi)[i]->height;
      }
      else
      {
	y = y_offset +
	  (MI_HEIGHT(mi) + ((MI_IS_SELECTABLE(mi)) ? relief_thickness : 0) -
	   MI_MINI_ICON(mi)[i]->height) / 2;
      }
      Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
      Globalgcv.clip_x_origin = MR_ICON_X_OFFSET(mr)[k];
      Globalgcv.clip_y_origin = y;
      if(MI_MINI_ICON(mi)[i]->depth == Pdepth)
      {
	/* pixmap */
	Globalgcv.clip_mask = MI_MINI_ICON(mi)[i]->mask;
	XChangeGC(dpy,currentGC,Globalgcm,&Globalgcv);
	XCopyArea(
	  dpy, MI_MINI_ICON(mi)[i]->picture, MR_WINDOW(mr), currentGC,
	  0, 0, MI_MINI_ICON(mi)[i]->width, MI_MINI_ICON(mi)[i]->height,
	  MR_ICON_X_OFFSET(mr)[k], y);
      }
      else
      {
	/* monochrome bitmap */
	Globalgcv.clip_mask = MI_MINI_ICON(mi)[i]->picture;
	XChangeGC(dpy,currentGC,Globalgcm,&Globalgcv);
	XCopyPlane(
	  dpy, MI_MINI_ICON(mi)[i]->picture, MR_WINDOW(mr), currentGC,
	  0, 0, MI_MINI_ICON(mi)[i]->width, MI_MINI_ICON(mi)[i]->height,
	  MR_ICON_X_OFFSET(mr)[k], y, 1);
      }
      Globalgcm = GCClipMask;
      Globalgcv.clip_mask = None;
      XChangeGC(dpy, currentGC, Globalgcm, &Globalgcv);
    }
  }

  return;
}

/************************************************************
 *
 * Draws a picture on the left side of the menu
 *
 ************************************************************/
static void paint_side_pic(MenuRoot *mr, XEvent *pevent)
{
  GC ReliefGC, TextGC;
  Picture *sidePic;
  int ys;
  int yt;
  int h;
  int bw = MST_BORDER_WIDTH(mr);

  if (MR_SIDEPIC(mr))
    sidePic = MR_SIDEPIC(mr);
  else if (MST_SIDEPIC(mr))
    sidePic = MST_SIDEPIC(mr);
  else
    return;

  if(Pdepth < 2)
  {
    ReliefGC = MST_MENU_SHADOW_GC(mr);
  }
  else
  {
    ReliefGC = MST_MENU_RELIEF_GC(mr);
  }
  TextGC = MST_MENU_GC(mr);

  if(MR_HAS_SIDECOLOR(mr))
  {
    Globalgcv.foreground = MR_SIDECOLOR(mr);
  }
  else if (MST_HAS_SIDE_COLOR(mr))
  {
    Globalgcv.foreground = MST_SIDE_COLOR(mr);
  }
  if (MR_HAS_SIDECOLOR(mr) || MST_HAS_SIDE_COLOR(mr))
  {
    Globalgcm = GCForeground;
    XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
    XFillRectangle(dpy, MR_WINDOW(mr), Scr.ScratchGC1,
		   MR_SIDEPIC_X_OFFSET(mr), bw,
		   sidePic->width, MR_HEIGHT(mr) - 2 * bw);
  }

  if (sidePic->height > MR_HEIGHT(mr) - 2 * bw)
  {
    h = MR_HEIGHT(mr) - 2 * bw;
    ys = sidePic->height - h;
    yt = bw;
  }
  else
  {
    h = sidePic->height;
    ys = 0;
    yt = MR_HEIGHT(mr) - bw - sidePic->height;
  }

  if(sidePic->depth == Pdepth) /* pixmap */
  {
    Globalgcv.clip_mask = sidePic->mask;
    Globalgcv.clip_x_origin = MR_SIDEPIC_X_OFFSET(mr);
    Globalgcv.clip_y_origin = yt - ys;
    XChangeGC(dpy, ReliefGC, GCClipMask | GCClipXOrigin | GCClipYOrigin,
	      &Globalgcv);
    XCopyArea(dpy, sidePic->picture, MR_WINDOW(mr), ReliefGC,
	      0, ys, sidePic->width, h, MR_SIDEPIC_X_OFFSET(mr), yt);
    Globalgcv.clip_mask = None;
    XChangeGC(dpy, ReliefGC, GCClipMask, &Globalgcv);
  }
  else
  {
    XCopyPlane(dpy, sidePic->picture, MR_WINDOW(mr), TextGC,
	       0, ys, sidePic->width, h, MR_SIDEPIC_X_OFFSET(mr), yt, 1);
  }
}



/****************************************************************************
 * Procedure:
 *	draw_underline() - Underline a character in a string (pete@tecc.co.uk)
 *
 * Calculate the pixel offsets to the start of the character position we
 * want to underline and to the next character in the string.  Shrink by
 * one pixel from each end and the draw a line that long two pixels below
 * the character...
 *
 ****************************************************************************/
static void draw_underline(MenuRoot *mr, GC gc, int x, int y, char *txt,
			   int coffset)
{
  int off1 = XTextWidth(MST_PSTDFONT(mr)->font, txt, coffset);
  int off2 = XTextWidth(MST_PSTDFONT(mr)->font, txt + coffset, 1) - 1 + off1;
  XDrawLine(dpy, MR_WINDOW(mr), gc, x + off1, y + 2, x + off2, y + 2);
}

/****************************************************************************
 *
 *  Draws two horizontal lines to form a separator
 *
 ****************************************************************************/
static void draw_separator(Window w, GC TopGC, GC BottomGC, int x1, int y1,
			   int x2, int y2, int extra_off)
{
  XDrawLine(dpy, w, TopGC   , x1,	    y1,	 x2,	      y2);
  XDrawLine(dpy, w, BottomGC, x1-extra_off, y1+1,x2+extra_off,y2+1);
}

/***********************************************************************
 *
 *  Procedure:
 *	paint_menu - draws the entire menu
 *
 ***********************************************************************/
static void paint_menu(MenuRoot *mr, XEvent *pevent, FvwmWindow *fw)
{
  MenuItem *mi;
  MenuStyle *ms = MR_STYLE(mr);
  XRectangle bounds;
  int bw = MST_BORDER_WIDTH(mr);
  Picture *p;
  int width, height, x, y;
  Pixmap pmap;
  GC pmapgc;
  XGCValues gcv;
  unsigned long gcm = GCLineWidth;
  gcv.line_width = 3;

  if (fw && !check_if_fvwm_window_exists(fw))
    fw = NULL;
  if (MR_IS_PAINTED(mr) && pevent &&
      (pevent->xexpose.x >= MR_WIDTH(mr) - bw ||
       pevent->xexpose.x + pevent->xexpose.width <= bw ||
       pevent->xexpose.y >= MR_HEIGHT(mr) - bw ||
       pevent->xexpose.y + pevent->xexpose.height <= bw))
  {
    /* Only the border was obscured. Redraw it centrally instead of redrawing
     * several menu items. */
    RelieveRectangle(dpy, MR_WINDOW(mr), 0, 0, MR_WIDTH(mr) - 1,
		     MR_HEIGHT(mr) - 1, (Pdepth < 2)
					? MST_MENU_SHADOW_GC(mr)
					: MST_MENU_RELIEF_GC(mr),
		     MST_MENU_SHADOW_GC(mr), bw);
    return;
  }
  MR_IS_PAINTED(mr) = 1;
  if (ms && ST_HAS_MENU_CSET(ms))
  {
    if (MR_IS_BACKGROUND_SET(mr) == False)
    {
      SetWindowBackground(
	dpy, MR_WINDOW(mr), MR_WIDTH(mr), MR_HEIGHT(mr),
	&Colorset[ST_CSET_MENU(ms)], Pdepth, ST_MENU_GC(ms),
	True);
      MR_IS_BACKGROUND_SET(mr) = True;
    }
  }
  else if (ms)
  {
    register int type;

    type = ST_FACE(ms).type;
    switch(type)
    {
    case SolidMenu:
      XSetWindowBackground(dpy, MR_WINDOW(mr), MST_FACE(mr).u.back);
      flush_expose(MR_WINDOW(mr));
      XClearWindow(dpy,MR_WINDOW(mr));
      break;
    case GradientMenu:
      bounds.x = bw;
      bounds.y = bw;
      bounds.width = MR_WIDTH(mr) - bw;
      bounds.height = MR_HEIGHT(mr) - bw;

      /* H, V, D and B gradients are optimized and have their own code here. */
      switch (ST_FACE(ms).gradient_type)
      {
      case H_GRADIENT:
	if (MR_IS_BACKGROUND_SET(mr) == False)
	{
	  register int i;
	  register int dw;

	  pmap = XCreatePixmap(dpy, MR_WINDOW(mr), MR_WIDTH(mr),
			       DEFAULT_MENU_GRADIENT_PIXMAP_THICKNESS, Pdepth);
	  pmapgc = fvwmlib_XCreateGC(dpy, pmap, gcm, &gcv);
	  dw = (float) (bounds.width / ST_FACE(ms).u.grad.npixels) + 1;
	  for (i = 0; i < ST_FACE(ms).u.grad.npixels; i++)
	  {
	    unsigned short x = i * bounds.width/ST_FACE(ms).u.grad.npixels;
	    XSetForeground(dpy, pmapgc, ST_FACE(ms).u.grad.pixels[i]);
	    XFillRectangle(dpy, pmap, pmapgc, x, 0, dw,
			   DEFAULT_MENU_GRADIENT_PIXMAP_THICKNESS);
	  }
	  XSetWindowBackgroundPixmap(dpy, MR_WINDOW(mr), pmap);
	  XFreeGC(dpy,pmapgc);
	  XFreePixmap(dpy,pmap);
	  MR_IS_BACKGROUND_SET(mr) = True;
	}
	XClearWindow(dpy, MR_WINDOW(mr));
	break;
      case V_GRADIENT:
	if (MR_IS_BACKGROUND_SET(mr) == False)
	{
	  register int i;
	  register int dh;
	  static unsigned int best_tile_width = 0;
	  unsigned int junk;

	  if (best_tile_width == 0)
	  {
	    if (!XQueryBestTile(
	      dpy, Scr.screen, DEFAULT_MENU_GRADIENT_PIXMAP_THICKNESS,
	      DEFAULT_MENU_GRADIENT_PIXMAP_THICKNESS, &best_tile_width, &junk))
	    {
	      /* call failed, use default and risk a screwed up tile */
	      best_tile_width = DEFAULT_MENU_GRADIENT_PIXMAP_THICKNESS;
	    }
	  }
	  pmap = XCreatePixmap(dpy, MR_WINDOW(mr), best_tile_width,
			       MR_HEIGHT(mr), Pdepth);
	  pmapgc = fvwmlib_XCreateGC(dpy, pmap, gcm, &gcv);
	  dh = (float) (bounds.height / ST_FACE(ms).u.grad.npixels) + 1;
	  for (i = 0; i < ST_FACE(ms).u.grad.npixels; i++)
	  {
	    unsigned short y = i * bounds.height/ST_FACE(ms).u.grad.npixels;
	    XSetForeground(dpy, pmapgc, ST_FACE(ms).u.grad.pixels[i]);
	    XFillRectangle(dpy, pmap, pmapgc, 0, y, best_tile_width, dh);
	  }
	  XSetWindowBackgroundPixmap(dpy, MR_WINDOW(mr), pmap);
	  XFreeGC(dpy,pmapgc);
	  XFreePixmap(dpy,pmap);
	  MR_IS_BACKGROUND_SET(mr) = True;
	}
	XClearWindow(dpy, MR_WINDOW(mr));
	break;
      case D_GRADIENT:
      case B_GRADIENT:
	{
	  register int i = 0, numLines;
	  int cindex = -1;
	  XRectangle r;
	  Picture *sidePic = NULL;
	  int bw = MST_BORDER_WIDTH(mr);

	  if (MR_SIDEPIC(mr))
	    sidePic = MR_SIDEPIC(mr);
	  else if (MST_SIDEPIC(mr))
	    sidePic = MST_SIDEPIC(mr);
	  if (pevent)
	  {

	    r.x = pevent->xexpose.x;
	    r.y = pevent->xexpose.y;
	    r.width = pevent->xexpose.width;
	    r.height = pevent->xexpose.height;
	  }
	  else
	  {
	    r.x = bw;
	    r.y = bw;
	    r.width = MR_WIDTH(mr) - 2 * bw;
	    r.height = MR_HEIGHT(mr) - 2 * bw;
	    if (sidePic)
	    {
	      r.width -= sidePic->width;
	      if (MR_SIDEPIC_X_OFFSET(mr) == bw)
		r.x += sidePic->width;
	    }
	  }
	  XSetClipRectangles(dpy, Scr.TransMaskGC, 0, 0, &r, 1, Unsorted);
	  numLines = MR_WIDTH(mr) + MR_HEIGHT(mr) - 2 * bw;
	  for(i = 0; i < numLines; i++)
	  {
	    if((int)(i * ST_FACE(ms).u.grad.npixels / numLines) > cindex)
	    {
	      /* pick the next colour (skip if necc.) */
	      cindex = i * ST_FACE(ms).u.grad.npixels / numLines;
	      XSetForeground(dpy, Scr.TransMaskGC,
			     ST_FACE(ms).u.grad.pixels[cindex]);
	    }
	    if (ST_FACE(ms).gradient_type == D_GRADIENT)
	      XDrawLine(dpy, MR_WINDOW(mr), Scr.TransMaskGC, 0, i, i, 0);
	    else /* B_GRADIENT */
	      XDrawLine(dpy, MR_WINDOW(mr), Scr.TransMaskGC,
			0, MR_HEIGHT(mr) - 1 - i, i, MR_HEIGHT(mr) - 1);
	  }
	}
	XSetClipMask(dpy, Scr.TransMaskGC, None);
	break;
      default:
	if (MR_IS_BACKGROUND_SET(mr) == False)
	{
	  unsigned int g_width;
	  unsigned int g_height;

	  /* let library take care of all other gradients */
	  pmap = XCreatePixmap(dpy, MR_WINDOW(mr), MR_WIDTH(mr), MR_HEIGHT(mr),
			       Pdepth);
	  pmapgc = fvwmlib_XCreateGC(dpy, pmap, gcm, &gcv);

	  /* find out the size the pixmap should be */
	  CalculateGradientDimensions(
	    dpy, MR_WINDOW(mr), ST_FACE(ms).u.grad.npixels,
	    ST_FACE(ms).gradient_type, &g_width, &g_height);
	  /* draw the gradient directly into the window */
	  CreateGradientPixmap(
	    dpy, MR_WINDOW(mr), pmapgc, ST_FACE(ms).gradient_type,
	    g_width, g_height, ST_FACE(ms).u.grad.npixels,
	    ST_FACE(ms).u.grad.pixels, pmap,
	    bw, bw, MR_WIDTH(mr) - bw, MR_HEIGHT(mr) - bw, NULL);
	  XSetWindowBackgroundPixmap(dpy, MR_WINDOW(mr), pmap);
	  XFreeGC(dpy, pmapgc);
	  XFreePixmap(dpy, pmap);
	  MR_IS_BACKGROUND_SET(mr) = True;
	}
	XClearWindow(dpy, MR_WINDOW(mr));
	break;
      }
      break;
    case PixmapMenu:
      p = ST_FACE(ms).u.p;

      width = MR_WIDTH(mr) - 2 * bw;
      height = MR_HEIGHT(mr) - 2 * bw;

#if 0
      /* these flags are never set at the moment */
      x = border;
      if (ST_FACE(ms)Style & HOffCenter)
      {
	if (ST_FACE(ms)Style & HRight)
	  x += (int)(width - p->width);
      }
      else
	x += (int)(width - p->width) / 2;

      y = border;
      if (ST_FACE(ms)Style & VOffCenter)
      {
	if (ST_FACE(ms)Style & VBottom)
	  y += (int)(height - p->height);
      }
      else...
#else
      y = (int)(height - p->height) / 2;
      x = (int)(width - p->width) / 2;
#endif

      if (x < bw)
	x = bw;
      if (y < bw)
	y = bw;
      if (width > p->width)
	width = p->width;
      if (height > p->height)
	height = p->height;
      if (width > MR_WIDTH(mr) - x - bw)
	width = MR_WIDTH(mr) - x - bw;
      if (height > MR_HEIGHT(mr) - y - bw)
	height = MR_HEIGHT(mr) - y - bw;

      XSetClipMask(dpy, Scr.TransMaskGC, p->mask);
      XSetClipOrigin(dpy, Scr.TransMaskGC, x, y);
      XCopyArea(dpy, p->picture, MR_WINDOW(mr), Scr.TransMaskGC,
		bw, bw, width, height, x, y);
      break;

   case TiledPixmapMenu:
     XSetWindowBackgroundPixmap(dpy, MR_WINDOW(mr),
				ST_FACE(ms).u.p->picture);
     flush_expose(MR_WINDOW(mr));
     XClearWindow(dpy,MR_WINDOW(mr));
     break;
    } /* switch(type) */
  } /* if (ms) */

  /* draw the relief */
  RelieveRectangle(dpy, MR_WINDOW(mr), 0, 0, MR_WIDTH(mr) - 1,
		   MR_HEIGHT(mr) - 1, (Pdepth < 2)
				      ? MST_MENU_SHADOW_GC(mr)
				      : MST_MENU_RELIEF_GC(mr),
		   MST_MENU_SHADOW_GC(mr), bw);

  for (mi = MR_FIRST_ITEM(mr); mi != NULL; mi = MI_NEXT_ITEM(mi))
  {
    /* be smart about handling the expose, redraw only the entries
     * that we need to */
    if((MST_FACE(mr).type != SolidMenu &&
	MST_FACE(mr).type != SimpleMenu &&
	!MST_HAS_MENU_CSET(mr)) ||
       pevent == NULL ||
       (pevent->xexpose.y < (MI_Y_OFFSET(mi) + MI_HEIGHT(mi)) &&
	(pevent->xexpose.y + pevent->xexpose.height) > MI_Y_OFFSET(mi)))
    {
      paint_item(mr, mi, fw, True);
    }
  }
  paint_side_pic(mr, pevent);
  XSync(dpy, 0);

  return;
}


static void FreeMenuItem(MenuItem *mi)
{
  short i;

  if (!mi)
    return;
  for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
  {
    if (MI_LABEL(mi)[i] != NULL)
      free(MI_LABEL(mi)[i]);
  }
  if (MI_ACTION(mi) != NULL)
    free(MI_ACTION(mi));
  if(MI_PICTURE(mi))
    DestroyPicture(dpy,MI_PICTURE(mi));
  for (i = 0; i < MAX_MENU_ITEM_MINI_ICONS; i++)
  {
    if(MI_MINI_ICON(mi)[i])
      DestroyPicture(dpy, MI_MINI_ICON(mi)[i]);
  }
  free(mi);
}

Bool DestroyMenu(MenuRoot *mr, Bool do_recreate, Bool is_command_request)
{
  MenuItem *mi,*tmp2;
  MenuRoot *tmp, *prev;

  if (mr == NULL)
    return False;

  /* seek menu in master list */
  tmp = Menus.all;
  prev = NULL;
  while (tmp && tmp != mr)
  {
    prev = tmp;
    tmp = MR_NEXT_MENU(tmp);
  }
  if (tmp != mr)
  {
    /* no such menu */
    return False;
  }

  if (MR_MAPPED_COPIES(mr) > 0 && (is_command_request || MR_COPIES(mr) == 1))
  {
    /* can't destroy a menu while in use */
    fvwm_msg(ERR,"DestroyMenu", "Menu %s is in use", MR_NAME(mr));
    return False;
  }

  if (MR_COPIES(mr) > 1 && MR_ORIGINAL_MENU(mr) == mr)
  {
    MenuRoot *m;
    MenuRoot *new_orig;

    /* find a new 'original' menu */
    for (m = Menus.all, new_orig = NULL; m; m = MR_NEXT_MENU(m))
    {
      if (m != mr && MR_ORIGINAL_MENU(m) == mr)
      {
	if (new_orig == NULL)
	{
	  new_orig = m;
	}
	MR_ORIGINAL_MENU(m) = new_orig;
      }
    }
    MR_ORIGINAL_MENU(mr) = new_orig;
    /* now dump old original menu */
  }

  MR_COPIES(mr)--;
  if (MR_STORED_ITEM(mr).stored)
    XFreePixmap(dpy, MR_STORED_ITEM(mr).stored);
  if (MR_COPIES(mr) > 0)
  {
    do_recreate = False;
  }
  else
  {
    /* free all items */
    mi = MR_FIRST_ITEM(mr);
    while (mi != NULL)
    {
      tmp2 = MI_NEXT_ITEM(mi);
      FreeMenuItem(mi);
      mi = tmp2;
    }
    if (do_recreate)
    {
      /* just dump the menu items but keep the menu itself */
      MR_COPIES(mr)++;
      MR_FIRST_ITEM(mr) = NULL;
      MR_LAST_ITEM(mr) = NULL;
      MR_SELECTED_ITEM(mr) = NULL;
      MR_CONTINUATION_MENU(mr) = NULL;
      MR_PARENT_MENU(mr) = NULL;
      MR_ITEMS(mr) = 0;
      memset(&(MR_STORED_ITEM(mr)), 0 , sizeof(MR_STORED_ITEM(mr)));
      MR_IS_UPDATED(mr) = 1;
      return True;
    }
  }

  /* unlink menu from list */
  if (prev == NULL)
    Menus.all = MR_NEXT_MENU(mr);
  else
    MR_NEXT_MENU(prev) = MR_NEXT_MENU(mr);
  XDestroyWindow(dpy,MR_WINDOW(mr));
  XDeleteContext(dpy, MR_WINDOW(mr), MenuContext);

  if (MR_COPIES(mr) == 0)
  {
    if (MR_POPUP_ACTION(mr))
      free(MR_POPUP_ACTION(mr));
    if (MR_POPDOWN_ACTION(mr))
      free(MR_POPDOWN_ACTION(mr));
    if (MR_MISSING_SUBMENU_FUNC(mr))
      free(MR_MISSING_SUBMENU_FUNC(mr));
    free(MR_NAME(mr));
    if (MR_SIDEPIC(mr))
      DestroyPicture(dpy, MR_SIDEPIC(mr));
    free(mr->s);
  }

  free(mr->d);
  free(mr);

  return True;
}

void CMD_DestroyMenu(F_CMD_ARGS)
{
  MenuRoot *mr;
  MenuRoot *mrContinuation;
  Bool do_recreate = False;
  char *token;

  token = PeekToken(action, &action);
  if (!token)
    return;
  if (StrEquals(token, "recreate"))
  {
    do_recreate = True;
    token = PeekToken(action, NULL);
  }
  mr = FindPopup(token);
  if (Scr.last_added_item.type == ADDED_MENU)
    set_last_added_item(ADDED_NONE, NULL);
  while (mr)
  {
    /* save continuation before destroy */
    mrContinuation = MR_CONTINUATION_MENU(mr);
    if (!DestroyMenu(mr, do_recreate, True))
    {
      return;
    }
    /* Don't recreate the continuations */
    do_recreate = False;
    mr = mrContinuation;
  }
  return;
}


/****************************************************************************
 *
 * Merge menu continuations back into the original menu.
 * Called by make_menu().
 *
 ****************************************************************************/
static void merge_continuation_menus(MenuRoot *mr)
{
  /* merge menu continuations into one menu again - needed when changing the
   * font size of a long menu. */
  while (MR_CONTINUATION_MENU(mr) != NULL)
  {
    MenuRoot *cont = MR_CONTINUATION_MENU(mr);

    /* link first item of continuation to item before 'more...' */
    MI_NEXT_ITEM(MI_PREV_ITEM(MR_LAST_ITEM(mr))) = MR_FIRST_ITEM(cont);
    MI_PREV_ITEM(MR_FIRST_ITEM(cont)) = MI_PREV_ITEM(MR_LAST_ITEM(mr));
    FreeMenuItem(MR_LAST_ITEM(mr));
    MR_LAST_ITEM(mr) = MR_LAST_ITEM(cont);
    MR_CONTINUATION_MENU(mr) = MR_CONTINUATION_MENU(cont);
    /* fake an empty menu so that DestroyMenu does not destroy the items. */
    MR_FIRST_ITEM(cont) = NULL;
    DestroyMenu(cont, False, False);
  }
  return;
}

/****************************************************************************
 *
 * Extract interesting values from the item format string that are needed by
 * the size_menu_... functions.
 *
 ****************************************************************************/
static void calculate_item_sizes(MenuSizingParameters *msp)
{
  MenuItem *mi;
  unsigned short w;
  int i;
  int j;

  memset(&(msp->max), 0, sizeof(msp->max));
  /* Calculate the widths for all columns of all items. */
  for (mi = MR_FIRST_ITEM(msp->menu); mi != NULL; mi = MI_NEXT_ITEM(mi))
  {
    if (MI_IS_POPUP(mi))
    {
      msp->max.triangle_width = MENU_TRIANGLE_WIDTH;
    }
    else if (MI_IS_TITLE(mi) && !MI_HAS_PICTURE(mi))
    {
      Bool is_formatted = False;

      /* titles stretch over the whole menu width, so count the maximum
       * separately if the title is unformatted. */
      for (j = 1; j < MAX_MENU_ITEM_LABELS; j++)
      {
	if (MI_LABEL(mi)[j] != NULL)
	{
	  is_formatted = True;
	  break;
	}
	else
	{
	  MI_LABEL_OFFSET(mi)[j] = 0;
	}
      }
      if (!is_formatted && MI_LABEL(mi)[0] != NULL)
      {
	MI_LABEL_STRLEN(mi)[0] = strlen(MI_LABEL(mi)[0]);
	w = XTextWidth(MST_PSTDFONT(msp->menu)->font, MI_LABEL(mi)[0],
		       MI_LABEL_STRLEN(mi)[0]);
	MI_LABEL_OFFSET(mi)[0] = w;
	MI_IS_TITLE_CENTERED(mi) = True;
	if (msp->max.title_width < w)
	  msp->max.title_width = w;
	continue;
      }
    }

    for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
    {
      if (MI_LABEL(mi)[i])
      {
	MI_LABEL_STRLEN(mi)[i] = strlen(MI_LABEL(mi)[i]);
	w = XTextWidth(MST_PSTDFONT(msp->menu)->font, MI_LABEL(mi)[i],
		       MI_LABEL_STRLEN(mi)[i]);
	MI_LABEL_OFFSET(mi)[i] = w;
	if (msp->max.label_width[i] < w)
	  msp->max.label_width[i] = w;
      }
    }
    if (MI_PICTURE(mi) && msp->max.picture_width < MI_PICTURE(mi)->width)
    {
      msp->max.picture_width = MI_PICTURE(mi)->width;
    }
    for (i = 0; i < MAX_MENU_ITEM_MINI_ICONS; i++)
    {
      int k;

      /* We need to reverse the mini icon order for left submenu style. */
      k = (MST_USE_LEFT_SUBMENUS(msp->menu)) ?
	MAX_MENU_ITEM_MINI_ICONS - 1 - i : i;

      if (MI_MINI_ICON(mi)[i] &&
	  msp->max.icon_width[k] < MI_MINI_ICON(mi)[i]->width)
      {
	msp->max.icon_width[k] = MI_MINI_ICON(mi)[i]->width;
      }
    }
  } /* for (mi = MR_FIRST_ITEM(msp->menu); mi != NULL; mi = MI_NEXT_ITEM(mi)) */

  if (MR_SIDEPIC(msp->menu))
  {
    msp->max.sidepic_width = MR_SIDEPIC(msp->menu)->width;
  }
  else if (MST_SIDEPIC(msp->menu))
  {
    msp->max.sidepic_width = MST_SIDEPIC(msp->menu)->width;
  }

  return;
}

/****************************************************************************
 *
 * Calculate the positions of the columns in the menu.
 * Called by make_menu().
 *
 ****************************************************************************/
static void size_menu_horizontally(MenuSizingParameters *msp)
{
  MenuItem *mi;
  Bool sidepic_is_left = True;
  int total_width;
  int sidepic_space = 0;
  unsigned short label_offset[MAX_MENU_ITEM_LABELS];
  char lcr_column[MAX_MENU_ITEM_LABELS];
  int i;
  int d;
  short relief_thickness = MST_RELIEF_THICKNESS(msp->menu);
  unsigned short *item_order[MAX_MENU_ITEM_LABELS + MAX_MENU_ITEM_MINI_ICONS +
			    1 /* triangle */ + 2 /* relief markers */];
  short used_objects = 0;
  short left_objects = 0;
  short right_objects = 0;

  memset(item_order, 0, sizeof(item_order));
  for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
    lcr_column[i] = 'l';

  /* Now calculate the offsets for the columns. */
  {
    int x;
    unsigned char icons_placed = 0;
    Bool sidepic_placed = False;
    Bool triangle_placed = False;
    Bool relief_begin_placed = False;
    Bool relief_end_placed = False;
    char *format;
    Bool first = True;
    Bool done = False;
    Bool is_last_object_left = True;
    unsigned char columns_placed = 0;
    int relief_gap = 0;
    int gap_left;
    int gap_right;
    int chars;

    format = MST_ITEM_FORMAT(msp->menu);
    if (!format)
    {
      format = (MST_USE_LEFT_SUBMENUS(msp->menu)) ?
	DEFAULT_LEFT_MENU_ITEM_FORMAT : DEFAULT_MENU_ITEM_FORMAT;
    }
    /* Place the individual items off the menu in case they are not set in the
     * format string. */
    for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
    {
      label_offset[i] = 2 * Scr.MyDisplayWidth;
    }

    x = MST_BORDER_WIDTH(msp->menu);
    while (*format && !done)
    {
      switch (*format)
      {
      case '%':
	format++;
	chars = 0;
	gap_left = 0;
	gap_right = 0;

	/* Insert a gap of %d pixels. */
	if (sscanf(format, "%d.%d%n", &gap_left, &gap_right, &chars) >= 2 ||
	    (sscanf(format, "%d.%n", &gap_left, &chars) >= 1 && chars > 0) ||
	    sscanf(format, "%d%n", &gap_left, &chars) >= 1 ||
	    sscanf(format, ".%d%n", &gap_right, &chars) >= 1)
	{
	  if (gap_left > MR_SCREEN_WIDTH(msp->menu) ||
	      gap_left < -MR_SCREEN_WIDTH(msp->menu))
	  {
	    gap_left = 0;
	  }
	  if (gap_right > MR_SCREEN_HEIGHT(msp->menu) ||
	      gap_right < -MR_SCREEN_HEIGHT(msp->menu))
	  {
	    gap_right = 0;
	  }
	  /* Skip the number. */
	  format += chars;
	}
	else if (*format == '.')
	{
	  /* Skip a dot without values */
	  format++;
	}
	if (!*format)
	  break;
	switch (*format)
	{
	case 'l':
	case 'c':
	case 'r':
	  /* A left, center or right aligned column. */
	  if (columns_placed < MAX_MENU_ITEM_LABELS)
	  {
	    if (msp->max.label_width[columns_placed] > 0)
	    {
	      lcr_column[columns_placed] = *format;
	      x += gap_left;
	      label_offset[columns_placed] = x;
	      x += msp->max.label_width[columns_placed] + gap_right;
	      item_order[used_objects++] = &(label_offset[columns_placed]);
	      if (is_last_object_left && (*format == 'l'))
		left_objects++;
	      else
	      {
		is_last_object_left = False;
		if (*format == 'r')
		  right_objects++;
		else
		  right_objects = 0;
	      }
	    }
	    columns_placed++;
	  }
	  break;
	case 's':
	  /* the sidepic */
	  if (!sidepic_placed)
	  {
	    sidepic_placed = True;
	    if (msp->max.sidepic_width > 0)
	    {
	      x += gap_left;
	      if (!first)
	      {
		/* Stop parsing immediately. %s must be first or last in the
		 * format string. */
		sidepic_is_left = False;
		done = True;
	      }
	      MR_SIDEPIC_X_OFFSET(msp->menu) = x;
	      if (first)
	      {
		sidepic_is_left = True;
	      }
	      sidepic_space = msp->max.sidepic_width +
		((sidepic_is_left) ? gap_left : gap_right);
	      x += msp->max.sidepic_width + gap_right;
	    }
	  }
	  break;
	case 'i':
	  /* a mini icon */
	  if (icons_placed < MAX_MENU_ITEM_MINI_ICONS)
	  {
	    if (msp->max.icon_width[icons_placed] > 0)
	    {
	      x += gap_left;
	      MR_ICON_X_OFFSET(msp->menu)[icons_placed] = x;
	      x += msp->max.icon_width[icons_placed] + gap_right;
	      item_order[used_objects++] =
		&(MR_ICON_X_OFFSET(msp->menu)[icons_placed]);
	      if (is_last_object_left)
		left_objects++;
	      else
		right_objects++;
	      icons_placed++;
	    }
	  }

	  break;
	case '|':
	  if (!relief_begin_placed)
	  {
	    relief_begin_placed = True;
	    x += gap_left;
	    MR_HILIGHT_X_OFFSET(msp->menu) = x;
	    x += relief_thickness + gap_right;
	    relief_gap += gap_right;
	    item_order[used_objects++] = &(MR_HILIGHT_X_OFFSET(msp->menu));
	    if (is_last_object_left)
	      left_objects++;
	    else
	      right_objects = 0;
	  }
	  else if (!relief_end_placed)
	  {
	    relief_end_placed = True;
	    x += relief_thickness + gap_left;
	    /* This is a hack: for now we record the x coordinate of the
	     * end of the hilight area, but later we'll place the width in
	     * here. */
	    MR_HILIGHT_WIDTH(msp->menu) = x;
	    x += gap_right;
	    relief_gap += gap_left;
	    item_order[used_objects++] = &(MR_HILIGHT_WIDTH(msp->menu));
	    right_objects++;
	  }
	  break;
	case '>':
	case '<':
	  /* the triangle for popup menus */
	  if (!triangle_placed)
	  {
	    triangle_placed = True;
	    if (msp->max.triangle_width > 0)
	    {
	      x += gap_left;
	      MR_TRIANGLE_X_OFFSET(msp->menu) = x;
	      MR_IS_LEFT_TRIANGLE(msp->menu) = (*format == '<');
	      x += msp->max.triangle_width + gap_right;
	      item_order[used_objects++] = &(MR_TRIANGLE_X_OFFSET(msp->menu));
	      if (is_last_object_left && *format == '<')
		left_objects++;
	      else
	      {
		is_last_object_left = False;
		right_objects++;
	      }
	    }
	  }
	  break;
	case 'p':
	  /* Simply add a gap. */
	  x += gap_right + gap_left;
	  break;
	case '\t':
	  x +=
	    MENU_TAB_WIDTH * XTextWidth(MST_PSTDFONT(msp->menu)->font, " ", 1);
	  break;
	case ' ':
	  /* Advance the x position. */
	  x += XTextWidth(MST_PSTDFONT(msp->menu)->font, format, 1);
	  break;
	default:
	  /* Ignore unknown characters. */
	  break;
	} /* switch (*format) */
	break;
      case '\t':
	x += MENU_TAB_WIDTH * XTextWidth(MST_PSTDFONT(msp->menu)->font, " ", 1);
	break;
      case ' ':
	/* Advance the x position. */
	x += XTextWidth(MST_PSTDFONT(msp->menu)->font, format, 1);
	break;
      default:
	/* Ignore unknown characters. */
	break;
      } /* switch (*format) */
      format++;
      first = False;
    } /* while (*format) */
    /* stored for vertical sizing */
    msp->used_item_labels = columns_placed;
    msp->used_mini_icons = icons_placed;

    /* Hide unplaced parts of the menu. */
    if (!sidepic_placed)
    {
      MR_SIDEPIC_X_OFFSET(msp->menu) = 2 * MR_SCREEN_WIDTH(msp->menu);
    }
    for (i = icons_placed; i < MAX_MENU_ITEM_MINI_ICONS; i++)
    {
      MR_ICON_X_OFFSET(msp->menu)[i] = 2 * MR_SCREEN_WIDTH(msp->menu);
    }
    if (!triangle_placed)
    {
      MR_TRIANGLE_X_OFFSET(msp->menu) = 2 * MR_SCREEN_WIDTH(msp->menu);
    }
    msp->flags.is_popup_indicator_used = triangle_placed;

    total_width = x - MST_BORDER_WIDTH(msp->menu);
    d = (sidepic_space + 2 * relief_thickness +
	 max(msp->max.title_width, msp->max.picture_width)) - total_width;
    if (d > 0)
    {
      int m = 1 - left_objects;
      int n = 1 + used_objects - left_objects - right_objects;

      /* The title is larger than all menu items. Stretch the gaps between the
       * items up to the total width of the title. */
      for (i = 0; i < used_objects; i++)
      {
	if (i >= left_objects)
	{
	  if (i >= used_objects - right_objects)
	  {
	    /* Right aligned item. */
	    *(item_order[i]) += d;
	  }
	  else
	  {
#if 0
	    /* Neither left nor right aligned item. */
	    *(item_order[i]) += d / 2;
#else
	    /* Neither left nor right aligned item. Divide the overhead gap
	     * evenly between the items. */

	    *(item_order[i]) += d * (m + i) / n;
#endif
	  }
	}
      }
      total_width += d;
      if (!sidepic_is_left)
      {
	MR_SIDEPIC_X_OFFSET(msp->menu) += d;
      }
    } /* if (d > 0) */
    MR_WIDTH(msp->menu) = total_width + 2 * MST_BORDER_WIDTH(msp->menu);
    MR_ITEM_WIDTH(msp->menu) = total_width - sidepic_space;
    MR_ITEM_X_OFFSET(msp->menu) = MST_BORDER_WIDTH(msp->menu);
    if (sidepic_is_left)
    {
      MR_ITEM_X_OFFSET(msp->menu) += sidepic_space;
    }
    if (!relief_begin_placed)
    {
      MR_HILIGHT_X_OFFSET(msp->menu) = MR_ITEM_X_OFFSET(msp->menu);
    }
    if (relief_end_placed)
    {
      MR_HILIGHT_WIDTH(msp->menu) =
	MR_HILIGHT_WIDTH(msp->menu) - MR_HILIGHT_X_OFFSET(msp->menu);
    }
    else
    {
      MR_HILIGHT_WIDTH(msp->menu) =
	MR_ITEM_WIDTH(msp->menu) + MR_ITEM_X_OFFSET(msp->menu) -
	MR_HILIGHT_X_OFFSET(msp->menu);
    }
  }

  /* Now calculate the offsets for the individual labels. */
  for (mi = MR_FIRST_ITEM(msp->menu); mi != NULL; mi = MI_NEXT_ITEM(mi))
  {
    for (i = 0; i < MAX_MENU_ITEM_LABELS; i++)
    {
      if (MI_LABEL(mi)[i])
      {
	if (!MI_IS_TITLE(mi) || !MI_IS_TITLE_CENTERED(mi))
	{
	  switch (lcr_column[i])
	  {
	  case 'l':
	    MI_LABEL_OFFSET(mi)[i] = label_offset[i];
	    break;
	  case 'c':
	    MI_LABEL_OFFSET(mi)[i] = label_offset[i] +
	      (msp->max.label_width[i] - MI_LABEL_OFFSET(mi)[i]) / 2;
	    break;
	  case 'r':
	    MI_LABEL_OFFSET(mi)[i] = label_offset[i] + msp->max.label_width[i] -
	      MI_LABEL_OFFSET(mi)[i];
	    break;
	  }
	}
	else
	{
	  /* This is a centered title item (indicated by negative width). */
	  MI_LABEL_OFFSET(mi)[i] =
	    menu_middle_x_offset(msp->menu) - MI_LABEL_OFFSET(mi)[i] / 2;
	}
      }
    } /* for */
  } /* for */

  return;
}


/****************************************************************************
 *
 * Calculate the positions of the columns in the menu.
 * Called by make_menu().
 *
 ****************************************************************************/
static Bool size_menu_vertically(MenuSizingParameters *msp)
{
  MenuItem *mi;
  int y;
  int cItems;
  short relief_thickness = MST_RELIEF_THICKNESS(msp->menu);
  short simple_entry_height;
  int i;
  Bool has_continuation_menu = False;

  MR_ITEM_TEXT_Y_OFFSET(msp->menu) =
    MST_PSTDFONT(msp->menu)->y + relief_thickness +
    MST_ITEM_GAP_ABOVE(msp->menu);
  simple_entry_height =	MST_PSTDFONT(msp->menu)->height +
    MST_ITEM_GAP_ABOVE(msp->menu) + MST_ITEM_GAP_BELOW(msp->menu);

  /* mi_prev trails one behind mi, since we need to move that
     into a newly-made menu if we run out of space */
  y = MST_BORDER_WIDTH(msp->menu);
  for (cItems = 0, mi = MR_FIRST_ITEM(msp->menu); mi != NULL;
       mi = MI_NEXT_ITEM(mi), cItems++)
  {
    Bool last_item_has_relief =
      (MI_PREV_ITEM(mi)) ? MI_IS_SELECTABLE(MI_PREV_ITEM(mi)) : False;
    Bool has_mini_icon = False;
    int separator_height;

    separator_height = (last_item_has_relief) ?
      MENU_SEPARATOR_HEIGHT + relief_thickness : MENU_SEPARATOR_TOTAL_HEIGHT;
    MI_Y_OFFSET(mi) = y;
    if (MI_IS_TITLE(mi) && MI_IS_TITLE_CENTERED(mi))
    {
      MI_HEIGHT(mi) = MST_PSTDFONT(msp->menu)->height +
	MST_TITLE_GAP_ABOVE(msp->menu) + MST_TITLE_GAP_BELOW(msp->menu);
    }
    else if (MI_IS_SEPARATOR(mi))
    {
      /* Separator */
      MI_HEIGHT(mi) = separator_height;
    }
    else
    {
      /* Normal text entry or an entry with a sub menu triangle */
      if ((MI_HAS_TEXT(mi) && msp->used_item_labels) ||
	  (MI_IS_POPUP(mi) && msp->flags.is_popup_indicator_used))
      {
	MI_HEIGHT(mi) = simple_entry_height + relief_thickness;
      }
      else
      {
	MI_HEIGHT(mi) =
	  MST_ITEM_GAP_ABOVE(msp->menu) + MST_ITEM_GAP_BELOW(msp->menu) +
	  relief_thickness;
      }
    }
    if (MI_IS_TITLE(mi))
    {
      /* add space for the underlines */
      switch (MST_TITLE_UNDERLINES(msp->menu))
      {
      case 0:
	if (last_item_has_relief)
	  MI_HEIGHT(mi) += relief_thickness;
	break;
      case 1:
	if (mi != MR_FIRST_ITEM(msp->menu))
	{
	  /* Space to draw the separator plus a gap above */
	  MI_HEIGHT(mi) += separator_height;
	}
	if (MI_NEXT_ITEM(mi) != NULL)
	{
	  /* Space to draw the separator */
	  MI_HEIGHT(mi) += MENU_SEPARATOR_HEIGHT;
	}
	break;
      default:
	/* Space to draw n underlines. */
	MI_HEIGHT(mi) +=
	  MENU_UNDERLINE_HEIGHT * MST_TITLE_UNDERLINES(msp->menu);
	if (last_item_has_relief)
	  MI_HEIGHT(mi) += relief_thickness;
	break;
      }
    }
    for (i = 0; i < msp->used_mini_icons; i++)
    {
      if (MI_MINI_ICON(mi)[i])
      {
	has_mini_icon = True;
      }
      if (MI_MINI_ICON(mi)[i] &&
	  MI_HEIGHT(mi) < MI_MINI_ICON(mi)[i]->height + relief_thickness)
      {
	MI_HEIGHT(mi) = MI_MINI_ICON(mi)[i]->height + relief_thickness;
      }
    }
    if (MI_PICTURE(mi))
    {
      if ((MI_HAS_TEXT(mi) && msp->used_item_labels) || has_mini_icon)
	MI_HEIGHT(mi) += MI_PICTURE(mi)->height;
      else
	MI_HEIGHT(mi) = MI_PICTURE(mi)->height + relief_thickness;
    }
    y += MI_HEIGHT(mi);
    /* this item would have to be the last item, or else
     * we need to add a "More..." entry pointing to a new menu */
    if (y + MST_BORDER_WIDTH(msp->menu) +
	((MI_IS_SELECTABLE(mi)) ? relief_thickness : 0)
	> MR_SCREEN_HEIGHT(msp->menu))
    {
      /* Item does not fit on screen anymore. */
      Bool does_fit = False;
      char *t;
      char *tempname;
      MenuRoot *menuContinuation;

      /* Remove items form the menu until it fits (plus a 'More' entry). */
      while (MI_PREV_ITEM(mi) != NULL)
      {
	/* Remove current item. */
	y -= MI_HEIGHT(mi);
	mi = MI_PREV_ITEM(mi);
	cItems--;
	if (y + MST_BORDER_WIDTH(msp->menu) + simple_entry_height +
	    2 * relief_thickness <= MR_SCREEN_HEIGHT(msp->menu))
	{
	  /* ok, it fits now */
	  does_fit = True;
	  break;
	}
      }
      if (!does_fit || MI_PREV_ITEM(mi) == NULL)
      {
	fvwm_msg(ERR, "size_menu_vertically",
		 "Menu entry does not fit on screen");
	/* leave a coredump */
	abort();
	exit(1);
      }

      t = EscapeString(MR_NAME(msp->menu), "\"", '\\');
      tempname = (char *)safemalloc((10 + strlen(t)) * sizeof(char));
      strcpy(tempname, "Popup \"");
      strcat(tempname, t);
      strcat(tempname, "$\"");
      free(t);
      /* NewMenuRoot inserts at the head of the list of menus
	 but, we need it at the end */
      /* (Give it just the name, which is 6 chars past the action
	 since strlen("Popup ")==6 ) */
      t = (char *)safemalloc(strlen(MR_NAME(msp->menu)) + 2);
      strcpy(t, MR_NAME(msp->menu));
      strcat(t, "$");
      menuContinuation = NewMenuRoot(t);
      free(t);
      MR_CONTINUATION_MENU(msp->menu) = menuContinuation;

      /* Now move this item and the remaining items into the new menu */
      MR_FIRST_ITEM(menuContinuation) = MI_NEXT_ITEM(mi);
      MR_LAST_ITEM(menuContinuation) = MR_LAST_ITEM(msp->menu);
      MR_ITEMS(menuContinuation) = MR_ITEMS(msp->menu) - cItems;
      MI_PREV_ITEM(MI_NEXT_ITEM(mi)) = NULL;

      /* mi_prev is now the last item in the parent menu */
      MR_LAST_ITEM(msp->menu) = mi;
      MR_ITEMS(msp->menu) = cItems;
      MI_NEXT_ITEM(mi) = NULL;

      /* use the same style for the submenu */
      MR_STYLE(menuContinuation) = MR_STYLE(msp->menu);
      MR_IS_LEFT_TRIANGLE(menuContinuation) = MR_IS_LEFT_TRIANGLE(msp->menu);
      /* migo: propagate missing_submenu_func */
      if (MR_MISSING_SUBMENU_FUNC(msp->menu))
      {
	MR_MISSING_SUBMENU_FUNC(menuContinuation) =
	  safestrdup(MR_MISSING_SUBMENU_FUNC(msp->menu));
      }
      /* don't propagate sidepic, sidecolor, popup- and popdown actions */

      /* And add the entry pointing to the new menu */
      AddToMenu(msp->menu, "More&...", tempname,
		False /* no pixmap scan */, False);
      free(tempname);
      has_continuation_menu = True;
    }
  } /* for */
  /* The menu may be empty here! */
  if (MR_LAST_ITEM(msp->menu) != NULL &&
      MI_IS_SELECTABLE(MR_LAST_ITEM(msp->menu)))
    y += relief_thickness;
  MR_HEIGHT(msp->menu) = y + MST_BORDER_WIDTH(msp->menu);

  return has_continuation_menu;
}


/****************************************************************************
 *
 * Creates the window for the menu.
 *
 ****************************************************************************/
static void make_menu_window(MenuRoot *mr)
{
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  unsigned int w;
  unsigned int h;

  valuemask = CWBackPixel | CWEventMask | CWCursor | CWColormap
    | CWBorderPixel | CWSaveUnder;
  attributes.border_pixel = 0;
  attributes.colormap = Pcmap;
  attributes.background_pixel = (MST_HAS_MENU_CSET(mr)) ?
    Colorset[MST_CSET_MENU(mr)].bg : MST_MENU_COLORS(mr).back;
  attributes.event_mask = XEVMASK_MENUW;
  attributes.cursor = Scr.FvwmCursors[CRS_MENU];
  attributes.save_under = True;

  if (MR_WINDOW(mr) != None)
    XDestroyWindow(dpy,MR_WINDOW(mr));
  w = MR_WIDTH(mr);
  if (w == 0)
    w = 1;
  h = MR_HEIGHT(mr);
  if (h == 0)
    h = 1;
  MR_WINDOW(mr) = XCreateWindow(dpy, Scr.Root, 0, 0, w, h,
				(unsigned int) 0, Pdepth, InputOutput,
				Pvisual, valuemask, &attributes);
  XSaveContext(dpy,MR_WINDOW(mr),MenuContext,(caddr_t)mr);
}


/****************************************************************************
 *
 * Generates the window for a menu
 *
 ****************************************************************************/
static void make_menu(MenuRoot *mr, MenuParameters *pmp)
{
  MenuSizingParameters msp;
  Bool has_continuation_menu = False;

#if 0
  if (!Scr.flags.windows_captured)
  {
    return;
  }
#endif
  if (MR_MAPPED_COPIES(mr) > 0)
  {
    return;
  }

  merge_continuation_menus(mr);
  do
  {
    memset(&msp, 0, sizeof(MenuSizingParameters));
    msp.menu = mr;
    calculate_item_sizes(&msp);
    /* Call size_menu_horizontally first because it calculated some values
     * used by size_menu_vertically. */
    size_menu_horizontally(&msp);
    has_continuation_menu = size_menu_vertically(&msp);
    /* repeat this step if the menu was split */
  } while (has_continuation_menu);

  MR_XANIMATION(mr) = 0;
  memset(&(MR_DYNAMIC_FLAGS(mr)), 0, sizeof(MR_DYNAMIC_FLAGS(mr)));

  /* create a new window for the menu */
  make_menu_window(mr);
  MR_IS_UPDATED(mr) = 0;

  return;
}


/***********************************************************************
 * Procedure:
 *	scanForHotkeys - Look for hotkey markers in a MenuItem
 *							(pete@tecc.co.uk)
 *
 * Inputs:
 *	it	- MenuItem to scan
 *	column	- The column number in which to look for a hotkey.
 *
 ***********************************************************************/
static void scanForHotkeys(MenuItem *it, int column)
{
  char *start, *txt;

  start = MI_LABEL(it)[column];	/* Get start of string	*/
  for (txt = start; *txt != '\0'; txt++)
  {
    /* Scan whole string	*/
    if (*txt == '&')
    {		/* A hotkey marker?			*/
      if (txt[1] == '&')
      {	/* Just an escaped &			*/
	char *tmp;		/* Copy the string down over it	*/
	for (tmp = txt; *tmp != '\0';
	     tmp++) tmp[0] = tmp[1];
	continue;		/* ...And skip to the key char		*/
      }
      else
      {
	/* It's a hot key marker - work out the offset value */
	MI_HOTKEY_COFFSET(it) = txt - start;
	MI_HOTKEY_COLUMN(it) = column;
	MI_HAS_HOTKEY(it) = (txt[1] != '\0');
	MI_IS_HOTKEY_AUTOMATIC(it) = 0;
	for (; *txt != '\0'; txt++)
	{
	  /* Copy down.. */
	  txt[0] = txt[1];
	}
	return;			/* Only one hotkey per item...	*/
      }
    }
  }
  return;
}


/* Side picture support: this scans for a color int the menu name
   for colorization */
static void scanForColor(char *instring, Pixel *p, Bool *flag, char identifier)
{
  char *tstart, *txt, *save_instring, *name;
  int i;

  *flag = False;

  /* save instring in case can't find pixmap */
  save_instring = (char *)safemalloc(strlen(instring)+1);
  name = (char *)safemalloc(strlen(instring)+1);
  strcpy(save_instring,instring);

  /* Scan whole string	      */
  for (txt = instring; *txt != '\0'; txt++)
  {
    /* A hotkey marker? */
    if (*txt == identifier)
    {
      /* Just an escaped '^'  */
      if (txt[1] == identifier)
      {
	char *tmp;		  /* Copy the string down over it */
	for (tmp = txt; *tmp != '\0'; tmp++) tmp[0] = tmp[1];
	continue;	  /* ...And skip to the key char	  */
      }
      /* It's a hot key marker - work out the offset value	    */
      tstart = txt;
	txt++;
	i=0;
	while((*txt != identifier)&&(*txt != '\0'))
	{
	  name[i] = *txt;
	  txt++;
	  i++;
	}
	name[i] = 0;

	*p = GetColor(name);
	*flag = True;

	if(*txt != '\0')
	  txt++;
	while(*txt != '\0')
	  *tstart++ = *txt++;
	*tstart = 0;
	break;
    }
  }
  free(name);
  free(save_instring);
  return;
}

static Bool scanForPixmap(char *instring, Picture **p, char identifier)
{
  char *tstart, *txt, *name;
  int i;
  Picture *pp;

  *p = NULL;
  if (!instring)
  {
    return False;
  }
  name = (char *)safemalloc(strlen(instring)+1);

  /* Scan whole string	*/
  for (txt = instring; *txt != '\0'; txt++)
  {
    /* A hotkey marker? */
    if (*txt == identifier)
    {
      /* Just an escaped &	*/
      if (txt[1] == identifier)
      {
	char *tmp;		/* Copy the string down over it	*/
	for (tmp = txt; *tmp != '\0'; tmp++)
	  tmp[0] = tmp[1];
	continue;		/* ...And skip to the key char		*/
      }
      /* It's a hot key marker - work out the offset value		*/
      tstart = txt;
      txt++;
      i=0;
      while ((*txt != identifier)&&(*txt != '\0'))
      {
	name[i] = *txt;
	txt++;
	i++;
      }
      name[i] = 0;

      /* Next, check for a color pixmap */
      pp = CachePicture(dpy, Scr.NoFocusWin, NULL, name, Scr.ColorLimit);
      if (*txt != '\0')
	txt++;
      while (*txt != '\0')
      {
	*tstart++ = *txt++;
      }
      *tstart = 0;
      if (pp)
	*p = pp;
      else
	fvwm_msg(WARN, "scanForPixmap",
		 "Couldn't load image from %s", name);
      break;
    }
  }

  free(name);
  return (*p != NULL);
}

/* FollowMenuContinuations
 * Given an menu root, return the menu root to add to by
 * following continuation links until there are no more
 */
MenuRoot *FollowMenuContinuations(MenuRoot *mr, MenuRoot **pmrPrior )
{
  *pmrPrior = NULL;
  while ((mr != NULL) &&
	 (MR_CONTINUATION_MENU(mr) != NULL))
  {
    *pmrPrior = mr;
    mr = MR_CONTINUATION_MENU(mr);
  }
  return mr;
}

/***********************************************************************
 *
 *  Procedure:
 *	AddToMenu - add an item to a root menu
 *
 *  Returned Value:
 *	(MenuItem *)
 *
 *  Inputs:
 *	menu	- pointer to the root menu to add the item
 *	item	- the text to appear in the menu
 *	action	- the string to possibly execute
 *
 * ckh - need to add boolean to say whether or not to expand for pixmaps,
 *	 so built in window list can handle windows w/ * and % in title.
 *
 ***********************************************************************/
void AddToMenu(MenuRoot *mr, char *item, char *action, Bool fPixmapsOk,
	       Bool fNoPlus)
{
  MenuItem *tmp;
  char *start;
  char *end;
  char *token = NULL;
  char *option = NULL;
  int i;
  short current_mini_icon = 0;

  if (MR_MAPPED_COPIES(mr) > 0)
  {
    /* whoa, we can't handle *everything* */
    return;
  }
  if ((item == NULL || *item == 0) && (action == NULL || *action == 0) &&
      fNoPlus)
    return;
  /* empty items screw up our menu when painted, so we replace them with a
   * separator */
  if (item == NULL)
    item = "";

  /***************************************************************
   * Handle dynamic actions
   ***************************************************************/

  if (StrEquals(item, "DynamicPopupAction"))
  {
    if (MR_POPUP_ACTION(mr))
      free(MR_POPUP_ACTION(mr));
    if (!action || *action == 0)
      MR_POPUP_ACTION(mr) = NULL;
    else
      MR_POPUP_ACTION(mr) = stripcpy(action);
    return;
  }
  else if (StrEquals(item, "DynamicPopdownAction"))
  {
    if (MR_POPDOWN_ACTION(mr))
      free(MR_POPDOWN_ACTION(mr));
    if (!action || *action == 0)
      MR_POPDOWN_ACTION(mr) = NULL;
    else
      MR_POPDOWN_ACTION(mr) = stripcpy(action);
    return;
  }
  else if (StrEquals(item, "MissingSubmenuFunction"))
  {
    if (MR_MISSING_SUBMENU_FUNC(mr))
      free(MR_MISSING_SUBMENU_FUNC(mr));
    if (!action || *action == 0)
      MR_MISSING_SUBMENU_FUNC(mr) = NULL;
    else
      MR_MISSING_SUBMENU_FUNC(mr) = stripcpy(action);
    return;
  }

  /***************************************************************
   * Parse the action
   ***************************************************************/

  if (action == NULL || *action == 0)
    action = "Nop";
  GetNextToken(GetNextToken(action, &token), &option);
  tmp = (MenuItem *)safemalloc(sizeof(MenuItem));
  memset(tmp, 0, sizeof(MenuItem));
  if (MR_FIRST_ITEM(mr) == NULL)
  {
    MR_FIRST_ITEM(mr) = tmp;
    MR_LAST_ITEM(mr) = tmp;
    MI_NEXT_ITEM(tmp) = NULL;
    MI_PREV_ITEM(tmp) = NULL;
  }
  else if (StrEquals(token, "title") && option && StrEquals(option, "top"))
  {
    if (MI_IS_TITLE(MR_FIRST_ITEM(mr)))
    {
      if (MR_FIRST_ITEM(mr) == MR_LAST_ITEM(mr))
	MR_LAST_ITEM(mr) = tmp;
      MI_PREV_ITEM(MI_NEXT_ITEM(MR_FIRST_ITEM(mr))) = tmp;
      MI_NEXT_ITEM(tmp) = MI_NEXT_ITEM(MR_FIRST_ITEM(mr));
      FreeMenuItem(MR_FIRST_ITEM(mr));
    }
    else
    {
      MI_PREV_ITEM(MR_FIRST_ITEM(mr)) = tmp;
      MI_NEXT_ITEM(tmp) = MR_FIRST_ITEM(mr);
    }
    MI_PREV_ITEM(tmp) = NULL;
    MR_FIRST_ITEM(mr) = tmp;
  }
  else
  {
    MI_NEXT_ITEM(MR_LAST_ITEM(mr)) = tmp;
    MI_PREV_ITEM(tmp) = MR_LAST_ITEM(mr);
    MR_LAST_ITEM(mr) = tmp;
  }
  if (token)
    free(token);
  if (option)
    free(option);

  MI_ACTION(tmp) = stripcpy(action);

  /***************************************************************
   * Parse the labels
   ***************************************************************/

  start = item;
  end = item;
  for (i = 0; i < MAX_MENU_ITEM_LABELS; i++, start = end)
  {
    /* Read label up to next tab. */
    if (*end)
    {
      if (i < MAX_MENU_ITEM_LABELS - 1)
      {
	while (*end && *end != '\t')
	  /* seek next tab or end of string */
	  end++;
      }
      else
      {
	/* remove all tabs in last label */
	while (*end)
	{
	  if (*end == '\t')
	    *end = ' ';
	  end++;
	}
      }
      /* Copy the label. */
      MI_LABEL(tmp)[i] = safemalloc(end - start + 1);
      strncpy(MI_LABEL(tmp)[i], start, end - start);
      (MI_LABEL(tmp)[i])[end - start] = 0;
      if (*end == '\t')
      {
	/* skip the tab */
	end++;
      }
    }
    else
    {
      MI_LABEL(tmp)[i] = NULL;
    }

    /* Parse the label. */
    if (MI_LABEL(tmp)[i] != NULL)
    {
      if (fPixmapsOk)
      {
	if(!MI_PICTURE(tmp))
	{
	  if (scanForPixmap(MI_LABEL(tmp)[i], &MI_PICTURE(tmp), '*'))
	    MI_HAS_PICTURE(tmp) = True;
	}
	while (current_mini_icon < MAX_MENU_ITEM_MINI_ICONS)
	{
	  if (scanForPixmap(
	    MI_LABEL(tmp)[i], &(MI_MINI_ICON(tmp)[current_mini_icon]), '%'))
	  {
	    current_mini_icon++;
	    MI_HAS_PICTURE(tmp) = True;
	  }
	  else
	    break;
	}
      }
      if (!MI_HAS_HOTKEY(tmp))
      {
	/* pete@tecc.co.uk */
	scanForHotkeys(tmp, i);

	if (!MI_HAS_HOTKEY(tmp) && *MI_LABEL(tmp)[i] != 0)
	{
	  MI_HOTKEY_COFFSET(tmp) = 0;
	  MI_HOTKEY_COLUMN(tmp) = i;
	  MI_HAS_HOTKEY(tmp) = 1;
	  MI_IS_HOTKEY_AUTOMATIC(tmp) = 1;
	}
      }
      if (*(MI_LABEL(tmp)[i]))
      {
	MI_HAS_TEXT(tmp) = True;
      }
      else
      {
	free(MI_LABEL(tmp)[i]);
	MI_LABEL(tmp)[i] = NULL;
      }
    }
    MI_LABEL_STRLEN(tmp)[i] =
      (MI_LABEL(tmp)[i]) ? strlen(MI_LABEL(tmp)[i]) : 0;
  } /* for */

  /***************************************************************
   * Set the type flags
   ***************************************************************/

  find_func_type(MI_ACTION(tmp), &(MI_FUNC_TYPE(tmp)), NULL);
  switch (MI_FUNC_TYPE(tmp))
  {
  case F_POPUP:
    MI_IS_POPUP(tmp) = True;
  case F_WINDOWLIST:
  case F_STAYSUP:
    MI_IS_MENU(tmp) = True;
    break;
  case F_TITLE:
    MI_IS_TITLE(tmp) = True;
    break;
  default:
    break;
  }
  MI_IS_SEPARATOR(tmp) = (!MI_HAS_TEXT(tmp) && !MI_HAS_PICTURE(tmp));
  if (MI_IS_SEPARATOR(tmp))
  {
    /* An empty title is handled like a separator. */
    MI_IS_TITLE(tmp) = False;
  }
  MI_IS_SELECTABLE(tmp) =
    ((MI_HAS_TEXT(tmp) || MI_HAS_PICTURE(tmp)) && !MI_IS_TITLE(tmp));


  /***************************************************************
   * misc stuff
   ***************************************************************/

  MR_ITEMS(mr)++;
  MR_IS_UPDATED(mr) = 1;
}

/***********************************************************************
 *
 *  Procedure:
 *	NewMenuRoot - create a new menu root
 *
 *  Returned Value:
 *	(MenuRoot *)
 *
 *  Inputs:
 *	name	- the name of the menu root
 *
 ***********************************************************************/
MenuRoot *NewMenuRoot(char *name)
{
  MenuRoot *mr;
  Bool flag;

  mr = (MenuRoot *)safemalloc(sizeof(MenuRoot));
  mr->s = (MenuRootStatic *)safemalloc(sizeof(MenuRootStatic));
  mr->d = (MenuRootDynamic *)safemalloc(sizeof(MenuRootDynamic));

  memset(mr->s, 0, sizeof(MenuRootStatic));
  memset(mr->d, 0, sizeof(MenuRootDynamic));
  MR_NEXT_MENU(mr) = Menus.all;
  MR_NAME(mr) = stripcpy(name);
  MR_WINDOW(mr) = None;
  scanForPixmap(MR_NAME(mr), &MR_SIDEPIC(mr), '@');
  scanForColor(MR_NAME(mr), &MR_SIDECOLOR(mr), &flag,'^');
  MR_HAS_SIDECOLOR(mr) = flag;
  MR_STYLE(mr) = Menus.DefaultStyle;
  MR_ORIGINAL_MENU(mr) = mr;
  MR_COPIES(mr) = 1;
  MR_IS_UPDATED(mr) = 1;

  Menus.all = mr;
  return mr;
}

/***********************************************************************
 *
 *  Procedure:
 *	NewMenuRoot - creates a new instance of an existing menu
 *
 *  Returned Value:
 *	(MenuRoot *)
 *
 *  Inputs:
 *	menu	- the MenuRoot structure of the existing menu
 *
 ***********************************************************************/
static MenuRoot *copy_menu_root(MenuRoot *mr)
{
  MenuRoot *tmp;

  if (!mr || MR_COPIES(mr) >= MAX_MENU_COPIES)
    return NULL;
  tmp = (MenuRoot *)safemalloc(sizeof(MenuRoot));
  tmp->d = (MenuRootDynamic *)safemalloc(sizeof(MenuRootDynamic));
  memset(tmp->d, 0, sizeof(MenuRootDynamic));
  tmp->s = mr->s;

  MR_COPIES(mr)++;
  MR_ORIGINAL_MENU(tmp) = MR_ORIGINAL_MENU(mr);
  MR_CONTINUATION_MENU(tmp) = MR_CONTINUATION_MENU(mr);
  MR_NEXT_MENU(tmp) = MR_NEXT_MENU(mr);
  MR_NEXT_MENU(mr) = tmp;
  MR_WINDOW(tmp) = None;
  memset(&(MR_DYNAMIC_FLAGS(tmp)), 0, sizeof(MR_DYNAMIC_FLAGS(tmp)));

  return tmp;
}


void SetMenuCursor(Cursor cursor)
{
  MenuRoot *mr;

  mr = Menus.all;
  for (mr = Menus.all; mr; mr = MR_NEXT_MENU(mr))
    if (MR_WINDOW(mr))
      XDefineCursor(dpy, MR_WINDOW(mr), cursor);
}

/*
 * ------------------------ Menu and Popup commands --------------------------
 */

static void menu_func(F_CMD_ARGS, Bool fStaysUp)
{
  MenuRoot *menu;
  char *ret_action = NULL;
  MenuOptions mops;
  char *menu_name = NULL;
  XEvent *teventp;
  MenuParameters mp;
  MenuReturn mret;
  FvwmWindow *fw;
  int tc;
  extern FvwmWindow *Tmp_win;
  extern FvwmWindow *ButtonWindow;
  extern int Context;

  memset(&mops, 0, sizeof(mops));
  memset(&mret, 0, sizeof(MenuReturn));
  action = GetNextToken(action,&menu_name);
  action = get_menu_options(action, w, tmp_win, eventp, NULL, NULL, &mops);
  while (action && *action && isspace((unsigned char)*action))
    action++;
  if (action && *action == 0)
    action = NULL;
  menu = FindPopup(menu_name);
  if(menu == NULL)
  {
    if(menu_name)
    {
      fvwm_msg(ERR,"menu_func","No such menu %s",menu_name);
      free(menu_name);
    }
    return;
  }
  if (menu_name &&
      set_repeat_data(menu_name, (fStaysUp) ? REPEAT_MENU : REPEAT_POPUP,NULL))
    free(menu_name);

  if (!action && eventp && eventp->type == KeyPress)
    teventp = (XEvent *)1;
  else
    teventp = eventp;

  memset(&mp, 0, sizeof(mp));
  mp.menu = menu;
  mp.parent_menu = NULL;
  mp.parent_item = NULL;
  fw = (tmp_win != None) ? tmp_win : Tmp_win;
  mp.pTmp_win = &fw;
  mp.button_window = ButtonWindow;
  tc = Context;
  mp.pcontext = &tc;
  mp.flags.has_default_action = (action != NULL);
  mp.flags.is_menu_from_frame_or_window_or_titlebar = False;
  mp.flags.is_sticky = fStaysUp;
  mp.flags.is_submenu = False;
  mp.flags.is_already_mapped = False;
  mp.eventp = teventp;
  mp.pops = &mops;
  mp.ret_paction = &ret_action;
  mp.event_propagate_to_submenu = NULL;

  do_menu(&mp, &mret);
  if (mret.rc == MENU_DOUBLE_CLICKED && action)
  {
    old_execute_function(F_PASS_EXEC_ARGS, 0, NULL);
  }
}

/* the function for the "Popup" command */
void CMD_Popup(F_CMD_ARGS)
{
  menu_func(F_PASS_ARGS, False);
}

/* the function for the "Menu" command */
void CMD_Menu(F_CMD_ARGS)
{
  menu_func(F_PASS_ARGS, True);
}

/*
 * -------------------------- Menu style commands ----------------------------
 */

static void FreeMenuFace(Display *dpy, MenuFace *mf)
{
  switch (mf->type)
  {
  case GradientMenu:
    /* - should we check visual is not TrueColor before doing this?
     *
     *		  XFreeColors(dpy, PictureCMap,
     *			      ms->u.grad.pixels, ms->u.grad.npixels,
     *			      AllPlanes); */
    free(mf->u.grad.pixels);
    mf->u.grad.pixels = NULL;
    break;

  case PixmapMenu:
  case TiledPixmapMenu:
    if (mf->u.p)
      DestroyPicture(dpy, mf->u.p);
    mf->u.p = NULL;
    break;
  case SolidMenu:
    FreeColors(&mf->u.back, 1);
  default:
    break;
  }
  mf->type = SimpleMenu;
}

/*****************************************************************************
 *
 * Reads a menu face line into a structure (veliaa@rpi.edu)
 *
 ****************************************************************************/
static Boolean ReadMenuFace(char *s, MenuFace *mf, int verbose)
{
  char *style;
  char *token;
  char *action = s;

  s = GetNextToken(s, &style);
  if (style && strncasecmp(style, "--", 2) == 0)
  {
    free(style);
    return True;
  }

  FreeMenuFace(dpy, mf);
  mf->type = SimpleMenu;

  /* determine menu style */
  if (!style)
    return True;
  else if (StrEquals(style,"Solid"))
  {
    s = GetNextToken(s, &token);
    if (token)
    {
      mf->type = SolidMenu;
      mf->u.back = GetColor(token);
      free(token);
    }
    else
    {
      if(verbose)
	fvwm_msg(ERR, "ReadMenuFace", "no color given for Solid face type: %s",
		 action);
      free(style);
      return False;
    }
  }

  else if (StrEquals(style+1, "Gradient"))
  {
    char **s_colors;
    int npixels, nsegs, *perc;
    Pixel *pixels;

    if (!IsGradientTypeSupported(style[0]))
      return False;

    /* translate the gradient string into an array of colors etc */
    npixels = ParseGradient(s, NULL, &s_colors, &perc, &nsegs);
    if (npixels <= 0)
      return False;
    /* grab the colors */
    pixels = AllocAllGradientColors(s_colors, perc, nsegs, npixels);
    if (pixels == None)
      return False;

    mf->u.grad.pixels = pixels;
    mf->u.grad.npixels = npixels;
    mf->type = GradientMenu;
    mf->gradient_type = toupper(style[0]);
  }

  else if (StrEquals(style,"Pixmap") || StrEquals(style,"TiledPixmap"))
  {
    s = GetNextToken(s, &token);
    if (token)
    {
      mf->u.p = CachePicture(dpy, Scr.NoFocusWin, NULL,
			     token, Scr.ColorLimit);
      if (mf->u.p == NULL)
      {
	if(verbose)
	  fvwm_msg(ERR, "ReadMenuFace", "couldn't load pixmap %s", token);
	free(token);
	free(style);
	return False;
      }
      free(token);
      mf->type = (StrEquals(style,"TiledPixmap")) ?
	TiledPixmapMenu : PixmapMenu;
    }
    else
    {
      if(verbose)
	fvwm_msg(ERR, "ReadMenuFace", "missing pixmap name for style %s",
		 style);
	free(style);
	return False;
    }
  }

  else
  {
    if(verbose)
      fvwm_msg(ERR, "ReadMenuFace", "unknown style %s: %s", style, action);
    free(style);
    return False;
  }

  free(style);
  return True;
}

static MenuStyle *FindMenuStyle(char *name)
{
  MenuStyle *ms = Menus.DefaultStyle;

  while(ms)
  {
     if(strcasecmp(ST_NAME(ms),name)==0)
       return ms;
     ms = ST_NEXT_STYLE(ms);
  }
  return NULL;
}

static void FreeMenuStyle(MenuStyle *ms)
{
  MenuRoot *mr;
  MenuStyle *before = Menus.DefaultStyle;

  if (!ms)
    return;
  mr = Menus.all;
  while(mr)
  {
    if(MR_STYLE(mr) == ms)
      MR_STYLE(mr) = Menus.DefaultStyle;
    mr = MR_NEXT_MENU(mr);
  }
  if(ST_MENU_GC(ms))
    XFreeGC(dpy, ST_MENU_GC(ms));
  if(ST_MENU_ACTIVE_GC(ms))
    XFreeGC(dpy, ST_MENU_ACTIVE_GC(ms));
  if(ST_MENU_ACTIVE_BACK_GC(ms))
    XFreeGC(dpy, ST_MENU_ACTIVE_BACK_GC(ms));
  if(ST_MENU_ACTIVE_RELIEF_GC(ms))
    XFreeGC(dpy, ST_MENU_ACTIVE_RELIEF_GC(ms));
  if(ST_MENU_ACTIVE_SHADOW_GC(ms))
    XFreeGC(dpy, ST_MENU_ACTIVE_SHADOW_GC(ms));
  if(ST_MENU_RELIEF_GC(ms))
    XFreeGC(dpy, ST_MENU_RELIEF_GC(ms));
  if(ST_MENU_STIPPLE_GC(ms))
    XFreeGC(dpy, ST_MENU_STIPPLE_GC(ms));
  if(ST_MENU_SHADOW_GC(ms))
    XFreeGC(dpy, ST_MENU_SHADOW_GC(ms));
  if (ST_SIDEPIC(ms))
    DestroyPicture(dpy, ST_SIDEPIC(ms));
  if (ST_HAS_SIDE_COLOR(ms) == 1)
    FreeColors(&ST_SIDE_COLOR(ms), 1);
  if (ST_PSTDFONT(ms) && ST_PSTDFONT(ms) != &Scr.DefaultFont)
  {
    FreeFvwmFont(dpy, ST_PSTDFONT(ms));
    free(ST_PSTDFONT(ms));
  }
  if (ST_ITEM_FORMAT(ms))
    free(ST_ITEM_FORMAT(ms));


  while(ST_NEXT_STYLE(before) != ms)
    /* Not too many checks, may segfaults in race conditions */
    before = ST_NEXT_STYLE(before);

  ST_NEXT_STYLE(before) = ST_NEXT_STYLE(ms);
  free(ST_NAME(ms));
  free(ms);
}

void CMD_DestroyMenuStyle(F_CMD_ARGS)
{
  MenuStyle *ms = NULL;
  char *name = NULL;
  MenuRoot *mr;

  name = PeekToken(action, NULL);
  if (name == NULL)
  {
    fvwm_msg(ERR,"DestroyMenuStyle", "needs one parameter");
    return;
  }

  ms = FindMenuStyle(name);
  if(ms == NULL)
  {
    return;
  }
  else if (ms == Menus.DefaultStyle)
  {
    fvwm_msg(ERR,"DestroyMenuStyle", "cannot destroy default menu style. "
	     "To reset the default menu style use\n  %s", DEFAULT_MENU_STYLE);
    return;
  }
  else if (ST_USAGE_COUNT(ms) != 0)
  {
    fvwm_msg(ERR,"DestroyMenuStyle", "menu style %s is in use", name);
    return;
  }
  else
  {
    FreeMenuFace(dpy, &ST_FACE(ms));
    FreeMenuStyle(ms);
  }
  for (mr = Menus.all; mr; mr = MR_NEXT_MENU(mr))
  {
    if (MR_STYLE(mr) == ms)
    {
      MR_STYLE(mr) = Menus.DefaultStyle;
      MR_IS_UPDATED(mr) = 1;
    }
  }
}

static void UpdateMenuStyle(MenuStyle *ms)
{
  XGCValues gcv;
  unsigned long gcm;
  Pixel menu_fore;
  Pixel menu_back;
  Pixel relief_fore;
  Pixel relief_back;
  Pixel active_fore;
  Pixel active_back;
  Pixel active_relief_fore;
  Pixel active_relief_back;
  colorset_struct *menu_cs = &Colorset[ST_CSET_MENU(ms)];
  colorset_struct *active_cs = &Colorset[ST_CSET_ACTIVE(ms)];
  colorset_struct *greyed_cs = &Colorset[ST_CSET_GREYED(ms)];

  if (ST_USAGE_COUNT(ms) != 0)
  {
    fvwm_msg(ERR,"UpdateMenuStyle", "menu style %s is in use", ST_NAME(ms));
    return;
  }
  ST_IS_UPDATED(ms) = 1;

  /* calculate colors based on foreground */
  if (!ST_HAS_ACTIVE_FORE(ms))
    ST_MENU_ACTIVE_COLORS(ms).fore = ST_MENU_COLORS(ms).fore;

  /* calculate colors based on background */
  if (!ST_HAS_ACTIVE_BACK(ms))
    ST_MENU_ACTIVE_COLORS(ms).back = ST_MENU_COLORS(ms).back;
  if (!ST_HAS_STIPPLE_FORE(ms))
    ST_MENU_STIPPLE_COLORS(ms).fore = ST_MENU_COLORS(ms).back;
  if(Pdepth > 2)
  {
    /* if not black and white */
    ST_MENU_RELIEF_COLORS(ms).back = GetShadow(ST_MENU_COLORS(ms).back);
    ST_MENU_RELIEF_COLORS(ms).fore = GetHilite(ST_MENU_COLORS(ms).back);
  }
  else
  {
    /* black and white */
    ST_MENU_RELIEF_COLORS(ms).back = GetColor(DEFAULT_SHADOW_COLOR);
    ST_MENU_RELIEF_COLORS(ms).fore = GetColor(DEFAULT_HILIGHT_COLOR);
  }
  ST_MENU_STIPPLE_COLORS(ms).back = ST_MENU_COLORS(ms).back;

  /* calculate some pixel values for convenience reasons */
  if (ST_HAS_MENU_CSET(ms))
  {
    menu_fore = menu_cs->fg;
    menu_back = menu_cs->bg;
    relief_fore = menu_cs->hilite;
    relief_back = menu_cs->shadow;
    active_relief_fore = menu_cs->hilite;
    active_relief_back = menu_cs->shadow;
  }
  else
  {
    menu_fore = ST_MENU_COLORS(ms).fore;
    menu_back = ST_MENU_COLORS(ms).back;
    relief_fore = ST_MENU_RELIEF_COLORS(ms).fore;
    relief_back = ST_MENU_RELIEF_COLORS(ms).back;
    active_relief_fore = ST_MENU_RELIEF_COLORS(ms).fore;
    active_relief_back = ST_MENU_RELIEF_COLORS(ms).back;
  }
  if (ST_HAS_ACTIVE_CSET(ms))
  {
    active_fore = active_cs->fg;
    active_back = active_cs->bg;
    active_relief_fore = active_cs->hilite;
    active_relief_back = active_cs->shadow;
  }
  else
  {
    active_fore = (ST_HAS_ACTIVE_FORE(ms)) ?
      ST_MENU_ACTIVE_COLORS(ms).fore : menu_fore;
    active_back = (ST_HAS_ACTIVE_BACK(ms)) ?
      ST_MENU_ACTIVE_COLORS(ms).back : menu_back;
  }

  /* make GC's */
  gcm = GCFunction|GCFont|GCLineWidth|GCForeground|GCBackground;
  gcv.font = ST_PSTDFONT(ms)->font->fid;
  gcv.function = GXcopy;
  gcv.line_width = 0;

  /* update relief gc */
  gcv.foreground = relief_fore;
  gcv.background = relief_back;
  if (ST_MENU_RELIEF_GC(ms))
    XChangeGC(dpy, ST_MENU_RELIEF_GC(ms), gcm, &gcv);
  else
    ST_MENU_RELIEF_GC(ms) = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  /* update shadow gc */
  gcv.foreground = relief_back;
  gcv.background = relief_fore;
  if (ST_MENU_SHADOW_GC(ms))
    XChangeGC(dpy, ST_MENU_SHADOW_GC(ms), gcm, &gcv);
  else
    ST_MENU_SHADOW_GC(ms) = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  /* update active gc */
  gcv.foreground = active_fore;
  gcv.background = active_back;
  if(ST_MENU_ACTIVE_GC(ms))
    XChangeGC(dpy, ST_MENU_ACTIVE_GC(ms), gcm, &gcv);
  else
    ST_MENU_ACTIVE_GC(ms) = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  /* update active relief gc */
  gcv.foreground = active_relief_fore;
  gcv.background = active_relief_back;
  if(ST_MENU_ACTIVE_RELIEF_GC(ms))
    XChangeGC(dpy, ST_MENU_ACTIVE_RELIEF_GC(ms), gcm, &gcv);
  else
    ST_MENU_ACTIVE_RELIEF_GC(ms) =
      fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  /* update active shadow gc */
  gcv.foreground = active_relief_back;
  gcv.background = active_relief_fore;
  if(ST_MENU_ACTIVE_SHADOW_GC(ms))
    XChangeGC(dpy, ST_MENU_ACTIVE_SHADOW_GC(ms), gcm, &gcv);
  else
    ST_MENU_ACTIVE_SHADOW_GC(ms) =
      fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  /* update active back gc */
  gcv.foreground = active_back;
  gcv.background = active_fore;
  if (ST_MENU_ACTIVE_BACK_GC(ms))
    XChangeGC(dpy, ST_MENU_ACTIVE_BACK_GC(ms), gcm, &gcv);
  else
    ST_MENU_ACTIVE_BACK_GC(ms) =
      fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  /* update menu gc */
  gcv.foreground = menu_fore;
  gcv.background = menu_back;
  if(ST_MENU_GC(ms))
    XChangeGC(dpy, ST_MENU_GC(ms), gcm, &gcv);
  else
    ST_MENU_GC(ms) = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  /* update stipple gc */
  if(Pdepth < 2)
  {
    gcv.fill_style = FillStippled;
    gcv.stipple = Scr.gray_bitmap;
    gcm|=GCStipple|GCFillStyle; /* no need to reset fg/bg, FillStipple wins */
  }
  else if (ST_HAS_GREYED_CSET(ms))
  {
    gcv.foreground = greyed_cs->fg;
    gcv.background = greyed_cs->bg;
  }
  else
  {
    gcv.foreground = ST_MENU_STIPPLE_COLORS(ms).fore;
    gcv.background = ST_MENU_STIPPLE_COLORS(ms).back;
  }
  if (ST_MENU_STIPPLE_GC(ms))
    XChangeGC(dpy, ST_MENU_STIPPLE_GC(ms), gcm, &gcv);
  else
    ST_MENU_STIPPLE_GC(ms) = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
}

void UpdateAllMenuStyles(void)
{
  MenuStyle *ms;

  for (ms = Menus.DefaultStyle; ms; ms = ST_NEXT_STYLE(ms))
  {
    UpdateMenuStyle(ms);
  }
}

void UpdateMenuColorset(int cset)
{
  MenuStyle *ms;

  for (ms = Menus.DefaultStyle; ms; ms = ST_NEXT_STYLE(ms))
  {
    if ((ST_HAS_MENU_CSET(ms) && ST_CSET_MENU(ms) == cset) ||
	(ST_HAS_ACTIVE_CSET(ms) && ST_CSET_ACTIVE(ms) == cset) ||
	(ST_HAS_GREYED_CSET(ms) && ST_CSET_GREYED(ms) == cset))
    {
      UpdateMenuStyle(ms);
    }
  }
}

static void parse_vertical_spacing_line(
  char *args, signed char *above, signed char *below,
  signed char above_default, signed char below_default)
{
  int val[2];

  if (GetIntegerArguments(args, NULL, val, 2) != 2 ||
      val[0] < MIN_VERTICAL_SPACING || val[0] > MAX_VERTICAL_SPACING ||
      val[1] < MIN_VERTICAL_SPACING || val[1] > MAX_VERTICAL_SPACING)
  {
    /* illegal or missing parameters, return to default */
    *above = above_default;
    *below = below_default;
    return;
  }
  *above = val[0];
  *below = val[1];
  return;
}

static int GetMenuStyleIndex(char *option)
{
  char *optlist[] = {
    "fvwm", "mwm", "win",
    "Foreground", "Background", "Greyed",
    "HilightBack", "HilightBackOff",
    "ActiveFore", "ActiveForeOff",
    "Hilight3DThick", "Hilight3DThin", "Hilight3DOff",
    "Animation", "AnimationOff",
    "Font",
    "MenuFace",
    "PopupDelay", "PopupOffset",
    "TitleWarp", "TitleWarpOff",
    "TitleUnderlines0", "TitleUnderlines1", "TitleUnderlines2",
    "SeparatorsLong", "SeparatorsShort",
    "TrianglesSolid", "TrianglesRelief",
    "PopupImmediately", "PopupDelayed",
    "DoubleClickTime",
    "SidePic", "SideColor",
    "PopupAsRootmenu", "PopupAsSubmenu",
    "RemoveSubmenus", "HoldSubmenus",
    "SubmenusRight", "SubmenusLeft",
    "BorderWidth",
    "Hilight3DThickness",
    "ItemFormat",
    "AutomaticHotkeys", "AutomaticHotkeysOff",
    "VerticalItemSpacing",
    "VerticalTitleSpacing",
    "MenuColorset", "ActiveColorset", "GreyedColorset",
    "SelectOnRelease",
    "PopdownImmediately", "PopdownDelayed",
    "PopdownDelay",
    NULL
  };
  return GetTokenIndex(option, optlist, 0, NULL);
}

static void NewMenuStyle(F_CMD_ARGS)
{
  char *name;
  char *option = NULL;
  char *optstring = NULL;
  char *nextarg;
  char *args = NULL;
  char *arg1;
  MenuStyle *ms;
  MenuStyle *tmpms;
  Bool is_initialised = True;
  Bool has_gc_changed = False;
  Bool is_default_style = False;
  int val[2];
  int n;
  FvwmFont new_font;
  int i;
  KeyCode keycode;

  action = GetNextToken(action, &name);
  if (!name)
  {
    fvwm_msg(ERR,"NewMenuStyle", "error in %s style specification",action);
    return;
  }

  ms = FindMenuStyle(name);
  if (ms && ST_USAGE_COUNT(ms) != 0)
  {
    fvwm_msg(ERR,"NewMenuStyle", "menu style %s is in use", name);
    return;
  }
  tmpms = (MenuStyle *)safemalloc(sizeof(MenuStyle));
  memset(tmpms, 0, sizeof(MenuStyle));
  if (ms)
  {
    /* copy the structure over our temporary menu face. */
    memcpy(tmpms, ms, sizeof(MenuStyle));
    if (ms == Menus.DefaultStyle)
      is_default_style = True;
    free(name);
  }
  else
  {
    ST_NAME(tmpms) = name;
    is_initialised = False;
  }
  ST_IS_UPDATED(tmpms) = 1;

  /* Parse the options. */
  while (!is_initialised || (action && *action))
  {
    if (!is_initialised)
    {
      /* some default configuration goes here for the new menu style */
      ST_MENU_COLORS(tmpms).back = GetColor(DEFAULT_BACK_COLOR);
      ST_MENU_COLORS(tmpms).fore = GetColor(DEFAULT_FORE_COLOR);
      ST_PSTDFONT(tmpms) = &Scr.DefaultFont;
      ST_FACE(tmpms).type = SimpleMenu;
      ST_HAS_ACTIVE_FORE(tmpms) = 0;
      ST_HAS_ACTIVE_BACK(tmpms) = 0;
      ST_DO_POPUP_AS_ROOT_MENU(tmpms) = 0;
      ST_DOUBLE_CLICK_TIME(tmpms) = DEFAULT_MENU_CLICKTIME;
      ST_POPUP_DELAY(tmpms) = DEFAULT_POPUP_DELAY;
      ST_POPDOWN_DELAY(tmpms) = DEFAULT_POPDOWN_DELAY;
      has_gc_changed = True;
      option = "fvwm";
    }
    else
    {
      /* Read next option specification (delimited by a comma or \0). */
      args = action;
      action = GetQuotedString(action, &optstring, ",", NULL, NULL, NULL);
      if (!optstring)
	break;

      args = GetNextToken(optstring, &option);
      if (!option)
      {
	free(optstring);
	break;
      }
      nextarg = GetNextToken(args, &arg1);
    }

    switch((i = GetMenuStyleIndex(option)))
    {
    case 0: /* fvwm */
    case 1: /* mwm */
    case 2: /* win */
      if (i == 0)
      {
	ST_POPUP_OFFSET_PERCENT(tmpms) = 67;
	ST_POPUP_OFFSET_ADD(tmpms) = 0;
	ST_DO_POPUP_IMMEDIATELY(tmpms) = 0;
	ST_DO_WARP_TO_TITLE(tmpms) = 1;
	ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = 0;
	ST_RELIEF_THICKNESS(tmpms) = 1;
	ST_TITLE_UNDERLINES(tmpms) = 1;
	ST_HAS_LONG_SEPARATORS(tmpms) = 0;
	ST_HAS_TRIANGLE_RELIEF(tmpms) = 1;
	ST_DO_HILIGHT(tmpms) = 0;
      }
      else if (i == 1)
      {
	ST_POPUP_OFFSET_PERCENT(tmpms) = 100;
	ST_POPUP_OFFSET_ADD(tmpms) = -DEFAULT_MENU_BORDER_WIDTH - 1;
	ST_DO_POPUP_IMMEDIATELY(tmpms) = 1;
	ST_DO_WARP_TO_TITLE(tmpms) = 0;
	ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = 0;
	ST_RELIEF_THICKNESS(tmpms) = 2;
	ST_TITLE_UNDERLINES(tmpms) = 2;
	ST_HAS_LONG_SEPARATORS(tmpms) = 1;
	ST_HAS_TRIANGLE_RELIEF(tmpms) = 1;
	ST_DO_HILIGHT(tmpms) = 0;
      }
      else /* i == 2 */
      {
	ST_POPUP_OFFSET_PERCENT(tmpms) = 100;
	ST_POPUP_OFFSET_ADD(tmpms) = -DEFAULT_MENU_BORDER_WIDTH - 3;
	ST_DO_POPUP_IMMEDIATELY(tmpms) = 1;
	ST_DO_WARP_TO_TITLE(tmpms) = 0;
	ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = 1;
	ST_RELIEF_THICKNESS(tmpms) = 0;
	ST_TITLE_UNDERLINES(tmpms) = 1;
	ST_HAS_LONG_SEPARATORS(tmpms) = 0;
	ST_HAS_TRIANGLE_RELIEF(tmpms) = 0;
	ST_DO_HILIGHT(tmpms) = 1;
      }

      /* common settings */
      ST_BORDER_WIDTH(tmpms) = DEFAULT_MENU_BORDER_WIDTH;
      ST_ITEM_GAP_ABOVE(tmpms) = DEFAULT_MENU_ITEM_TEXT_Y_OFFSET;
      ST_ITEM_GAP_BELOW(tmpms) = DEFAULT_MENU_ITEM_TEXT_Y_OFFSET2;
      ST_TITLE_GAP_ABOVE(tmpms) = DEFAULT_MENU_TITLE_TEXT_Y_OFFSET;
      ST_TITLE_GAP_BELOW(tmpms) = DEFAULT_MENU_TITLE_TEXT_Y_OFFSET2;
      ST_USE_LEFT_SUBMENUS(tmpms) = 0;
      ST_IS_ANIMATED(tmpms) = 0;
      ST_USE_AUTOMATIC_HOTKEYS(tmpms) = 0;
      FreeMenuFace(dpy, &ST_FACE(tmpms));
      ST_FACE(tmpms).type = SimpleMenu;
      if (ST_PSTDFONT(tmpms) && ST_PSTDFONT(tmpms) != &Scr.DefaultFont)
      {
	FreeFvwmFont(dpy, ST_PSTDFONT(tmpms));
	free(ST_PSTDFONT(tmpms));
      }
      ST_PSTDFONT(tmpms) = &Scr.DefaultFont;
      has_gc_changed = True;
      if (ST_HAS_SIDE_COLOR(tmpms) == 1)
      {
	FreeColors(&ST_SIDE_COLOR(tmpms), 1);
	ST_HAS_SIDE_COLOR(tmpms) = 0;
      }
      ST_HAS_SIDE_COLOR(tmpms) = 0;
      if (ST_SIDEPIC(tmpms))
      {
	DestroyPicture(dpy, ST_SIDEPIC(tmpms));
	ST_SIDEPIC(tmpms) = NULL;
      }

      if (is_initialised == False)
      {
	/* now begin the real work */
	is_initialised = True;
	continue;
      }
      break;

    case 3: /* Foreground */
      FreeColors(&ST_MENU_COLORS(tmpms).fore, 1);
      if (arg1)
	ST_MENU_COLORS(tmpms).fore = GetColor(arg1);
      else
	ST_MENU_COLORS(tmpms).fore = GetColor(DEFAULT_FORE_COLOR);
      has_gc_changed = True;
      break;

    case 4: /* Background */
      FreeColors(&ST_MENU_COLORS(tmpms).back, 1);
      if (arg1)
	ST_MENU_COLORS(tmpms).back = GetColor(arg1);
      else
	ST_MENU_COLORS(tmpms).back = GetColor(DEFAULT_BACK_COLOR);
      has_gc_changed = True;
      break;

    case 5: /* Greyed */
      if (ST_HAS_STIPPLE_FORE(tmpms))
	FreeColors(&ST_MENU_STIPPLE_COLORS(tmpms).fore, 1);
      if (arg1 == NULL)
      {
	ST_HAS_STIPPLE_FORE(tmpms) = 0;
      }
      else
      {
	ST_MENU_STIPPLE_COLORS(tmpms).fore = GetColor(arg1);
	ST_HAS_STIPPLE_FORE(tmpms) = 1;
      }
      has_gc_changed = True;
      break;

    case 6: /* HilightBack */
      if (ST_HAS_ACTIVE_BACK(tmpms))
	FreeColors(&ST_MENU_ACTIVE_COLORS(tmpms).back, 1);
      if (arg1 == NULL)
      {
	ST_HAS_ACTIVE_BACK(tmpms) = 0;
      }
      else
      {
	ST_MENU_ACTIVE_COLORS(tmpms).back = GetColor(arg1);
	ST_HAS_ACTIVE_BACK(tmpms) = 1;
      }
      ST_DO_HILIGHT(tmpms) = 1;
      has_gc_changed = True;
      break;

    case 7: /* HilightBackOff */
      ST_DO_HILIGHT(tmpms) = 0;
      has_gc_changed = True;
      break;

    case 8: /* ActiveFore */
      if (ST_HAS_ACTIVE_FORE(tmpms))
	FreeColors(&ST_MENU_ACTIVE_COLORS(tmpms).fore, 1);
      if (arg1 == NULL)
      {
	ST_HAS_ACTIVE_FORE(tmpms) = 0;
      }
      else
      {
	ST_MENU_ACTIVE_COLORS(tmpms).fore = GetColor(arg1);
	ST_HAS_ACTIVE_FORE(tmpms) = 1;
      }
      has_gc_changed = True;
      break;

    case 9: /* ActiveForeOff */
      ST_HAS_ACTIVE_FORE(tmpms) = 0;
      has_gc_changed = True;
      break;

    case 10: /* Hilight3DThick */
      ST_RELIEF_THICKNESS(tmpms) = 2;
      break;

    case 11: /* Hilight3DThin */
      ST_RELIEF_THICKNESS(tmpms) = 1;
      break;

    case 12: /* Hilight3DOff */
      ST_RELIEF_THICKNESS(tmpms) = 0;
      break;

    case 13: /* Animation */
      ST_IS_ANIMATED(tmpms) = 1;
      break;

    case 14: /* AnimationOff */
      ST_IS_ANIMATED(tmpms) = 0;
      break;

    case 15: /* Font */
      if (arg1 && !LoadFvwmFont(dpy, arg1, &new_font))
      {
	fvwm_msg(ERR, "NewMenuStyle",
		 "Couldn't load font '%s' or 'fixed'\n", arg1);
	break;
      }
      if (ST_PSTDFONT(tmpms) && ST_PSTDFONT(tmpms) != &Scr.DefaultFont)
      {
	FreeFvwmFont(dpy, ST_PSTDFONT(tmpms));
	free(ST_PSTDFONT(tmpms));
      }
      if (arg1 == NULL)
      {
	/* reset to screen font */
	ST_PSTDFONT(tmpms) = &Scr.DefaultFont;
      }
      else
      {
	ST_PSTDFONT(tmpms) = (FvwmFont *)safemalloc(sizeof(FvwmFont));
	*ST_PSTDFONT(tmpms) = new_font;
      }
      has_gc_changed = True;
      break;

    case 16: /* MenuFace */
      while (args && *args != '\0' && isspace((unsigned char)*args))
	args++;
      ReadMenuFace(args, &ST_FACE(tmpms), True);
      break;

    case 17: /* PopupDelay */
      if (GetIntegerArguments(args, NULL, val, 1) == 0 || *val < 0)
	ST_POPUP_DELAY(tmpms) = DEFAULT_POPUP_DELAY;
      else
	ST_POPUP_DELAY(tmpms) = (*val+9)/10;
      break;

    case 18: /* PopupOffset */
      if ((n = GetIntegerArguments(args, NULL, val, 2)) == 0)
      {
	fvwm_msg(ERR,"NewMenuStyle",
		 "PopupOffset requires one or two arguments");
      }
      else
      {
	ST_POPUP_OFFSET_ADD(tmpms) = val[0];
	if (n == 2 && val[1] <= 100 && val[1] >= 0)
	  ST_POPUP_OFFSET_PERCENT(tmpms) = val[1];
	else
	  ST_POPUP_OFFSET_PERCENT(tmpms) = 100;
      }
      break;

    case 19: /* TitleWarp */
      ST_DO_WARP_TO_TITLE(tmpms) = 1;
      break;

    case 20: /* TitleWarpOff */
      ST_DO_WARP_TO_TITLE(tmpms) = 0;
      break;

    case 21: /* TitleUnderlines0 */
      ST_TITLE_UNDERLINES(tmpms) = 0;
      break;

    case 22: /* TitleUnderlines1 */
      ST_TITLE_UNDERLINES(tmpms) = 1;
      break;

    case 23: /* TitleUnderlines2 */
      ST_TITLE_UNDERLINES(tmpms) = 2;
      break;

    case 24: /* SeparatorsLong */
      ST_HAS_LONG_SEPARATORS(tmpms) = 1;
      break;

    case 25: /* SeparatorsShort */
      ST_HAS_LONG_SEPARATORS(tmpms) = 0;
      break;

    case 26: /* TrianglesSolid */
      ST_HAS_TRIANGLE_RELIEF(tmpms) = 0;
      break;

    case 27: /* TrianglesRelief */
      ST_HAS_TRIANGLE_RELIEF(tmpms) = 1;
      break;

    case 28: /* PopupImmediately */
      ST_DO_POPUP_IMMEDIATELY(tmpms) = 1;
      break;

    case 29: /* PopupDelayed */
      ST_DO_POPUP_IMMEDIATELY(tmpms) = 0;
      break;

    case 30: /* DoubleClickTime */
      if (GetIntegerArguments(args, NULL, val, 1) == 0 || *val < 0)
	ST_DOUBLE_CLICK_TIME(tmpms) = DEFAULT_MENU_CLICKTIME;
      else
	ST_DOUBLE_CLICK_TIME(tmpms) = *val;
      break;

    case 31: /* SidePic */
      if (ST_SIDEPIC(tmpms))
      {
	DestroyPicture(dpy, ST_SIDEPIC(tmpms));
	ST_SIDEPIC(tmpms) = NULL;
      }
      if (arg1)
      {
	ST_SIDEPIC(tmpms) = CachePicture(dpy, Scr.NoFocusWin, NULL,
					   arg1, Scr.ColorLimit);
	if (!ST_SIDEPIC(tmpms))
	  fvwm_msg(WARN, "NewMenuStyle", "Couldn't find pixmap %s", arg1);
      }
      break;

    case 32: /* SideColor */
      if (ST_HAS_SIDE_COLOR(tmpms) == 1)
      {
	FreeColors(&ST_SIDE_COLOR(tmpms), 1);
	ST_HAS_SIDE_COLOR(tmpms) = 0;
      }
      if (arg1)
      {
	ST_SIDE_COLOR(tmpms) = GetColor(arg1);
	ST_HAS_SIDE_COLOR(tmpms) = 1;
      }
      break;

    case 33: /* PopupAsRootmenu */
      ST_DO_POPUP_AS_ROOT_MENU(tmpms) = 1;
      break;

    case 34: /* PopupAsSubmenu */
      ST_DO_POPUP_AS_ROOT_MENU(tmpms) = 0;
      break;

    case 35: /* RemoveSubmenus */
      ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = 1;
      break;

    case 36: /* HoldSubmenus */
      ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = 0;
      break;

    case 37: /* SubmenusRight */
      ST_USE_LEFT_SUBMENUS(tmpms) = 0;
      break;

    case 38: /* SubmenusLeft */
      ST_USE_LEFT_SUBMENUS(tmpms) = 1;
      break;

    case 39: /* BorderWidth */
      if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
	  *val < 0 || *val > MAX_MENU_BORDER_WIDTH)
	ST_BORDER_WIDTH(tmpms) = DEFAULT_MENU_BORDER_WIDTH;
      else
	ST_BORDER_WIDTH(tmpms) = *val;
      break;

    case 40: /* Hilight3DThickness */
      if (GetIntegerArguments(args, NULL, val, 1) > 0)
      {
	if (*val < 0)
	{
	  *val = -*val;
	  ST_IS_ITEM_RELIEF_REVERSED(tmpms) = 1;
	}
	else
	{
	  ST_IS_ITEM_RELIEF_REVERSED(tmpms) = 0;
	}
	if (*val > MAX_MENU_ITEM_RELIEF_THICKNESS)
	  *val = MAX_MENU_ITEM_RELIEF_THICKNESS;
	ST_RELIEF_THICKNESS(tmpms) = *val;
      }
      break;

    case 41: /* ItemFormat */
      if (ST_ITEM_FORMAT(tmpms))
      {
	free(ST_ITEM_FORMAT(tmpms));
	ST_ITEM_FORMAT(tmpms) = NULL;
      }
      if (arg1)
      {
	ST_ITEM_FORMAT(tmpms) = safestrdup(arg1);
      }
      break;

    case 42: /* AutomaticHotkeys */
      ST_USE_AUTOMATIC_HOTKEYS(tmpms) = 1;
      break;

    case 43: /* AutomaticHotkeysOff */
      ST_USE_AUTOMATIC_HOTKEYS(tmpms) = 0;
      break;

    case 44: /* VerticalItemSpacing */
      parse_vertical_spacing_line(
	args, &ST_ITEM_GAP_ABOVE(tmpms), &ST_ITEM_GAP_BELOW(tmpms),
	DEFAULT_MENU_ITEM_TEXT_Y_OFFSET, DEFAULT_MENU_ITEM_TEXT_Y_OFFSET2);
      break;

    case 45: /* VerticalTitleSpacing */
      parse_vertical_spacing_line(
	args, &ST_TITLE_GAP_ABOVE(tmpms), &ST_TITLE_GAP_BELOW(tmpms),
	DEFAULT_MENU_TITLE_TEXT_Y_OFFSET, DEFAULT_MENU_TITLE_TEXT_Y_OFFSET2);
      break;
    case 46: /* MenuColorset */
      if (GetIntegerArguments(args, NULL, val, 1) == 0 || *val < 0)
	ST_HAS_MENU_CSET(tmpms) = 0;
      else
      {
	ST_HAS_MENU_CSET(tmpms) = 1;
	ST_CSET_MENU(tmpms) = *val;
	AllocColorset(*val);
      }
      has_gc_changed = True;
      break;
    case 47: /* ActiveColorset */
      if (GetIntegerArguments(args, NULL, val, 1) == 0 || *val < 0)
      {
	ST_HAS_ACTIVE_CSET(tmpms) = 0;
      }
      else
      {
	ST_HAS_ACTIVE_CSET(tmpms) = 1;
	ST_CSET_ACTIVE(tmpms) = *val;
	AllocColorset(*val);
      }
      has_gc_changed = True;
      break;

    case 48: /* GreyedColorset */
      if (GetIntegerArguments(args, NULL, val, 1) == 0 || *val < 0)
	ST_HAS_GREYED_CSET(tmpms) = 0;
      else
      {
	ST_HAS_GREYED_CSET(tmpms) = 1;
	ST_CSET_GREYED(tmpms) = *val;
	AllocColorset(*val);
      }
      has_gc_changed = True;
      break;

    case 49: /* SelectOnRelease */
      keycode = 0;
      if (arg1)
      {
	keycode = XKeysymToKeycode(dpy, FvwmStringToKeysym(dpy, arg1));
      }
      ST_SELECT_ON_RELEASE_KEY(tmpms) = keycode;
      break;

    case 50: /* PopdownImmediately */
      ST_DO_POPDOWN_IMMEDIATELY(tmpms) = 1;
      break;

    case 51: /* PopdownDelayed */
      ST_DO_POPDOWN_IMMEDIATELY(tmpms) = 0;
      break;

    case 52: /* PopdownDelay */
      if (GetIntegerArguments(args, NULL, val, 1) == 0 || *val < 0)
	ST_POPDOWN_DELAY(tmpms) = DEFAULT_POPDOWN_DELAY;
      else
	ST_POPDOWN_DELAY(tmpms) = (*val+9)/10;
      break;

#if 0
    case 99: /* PositionHints */
      /* to be implemented */
      break;
#endif

    default:
      fvwm_msg(ERR,"NewMenuStyle", "unknown option '%s'", option);
      break;
    } /* switch */

    if (option)
    {
      free(option);
      option = NULL;
    }
    free(optstring);
    optstring = NULL;
    if (arg1)
    {
      free(arg1);
      arg1 = NULL;
    }
  } /* while */

  if (has_gc_changed)
  {
    UpdateMenuStyle(tmpms);
  } /* if (has_gc_changed) */

  if(Menus.DefaultStyle == NULL)
  {
    /* First MenuStyle MUST be the default style */
    Menus.DefaultStyle = tmpms;
    ST_NEXT_STYLE(tmpms) = NULL;
  }
  else if (ms)
  {
    /* copy our new menu face over the old one */
    memcpy(ms, tmpms, sizeof(MenuStyle));
    free(tmpms);
  }
  else
  {
    MenuStyle *before = Menus.DefaultStyle;

    /* add a new menu face to list */
    ST_NEXT_STYLE(tmpms) = NULL;
    while(ST_NEXT_STYLE(before))
      before = ST_NEXT_STYLE(before);
    ST_NEXT_STYLE(before) = tmpms;
  }

  return;
}

void CMD_CopyMenuStyle(F_CMD_ARGS)
{
  char *origname = NULL;
  char *destname = NULL;
  char *buffer;
  MenuStyle *origms;
  MenuStyle *destms;

  origname = PeekToken(action, &action);
  if (origname == NULL)
  {
    fvwm_msg(ERR,"CopyMenuStyle", "need two arguments");
    return;
  }

  origms = FindMenuStyle(origname);
  if (!origms)
  {
    fvwm_msg(ERR,"CopyMenuStyle", "%s: no such menu style", origname);
    return;
  }

  destname = PeekToken(action, &action);
  if (destname == NULL)
  {
    fvwm_msg(ERR,"CopyMenuStyle", "need two arguments");
    return;
  }

  if (action && *action)
  {
    fvwm_msg(ERR,"CopyMenuStyle", "too many arguments");
    return;
  }

  destms = FindMenuStyle(destname);
  if (!destms)
  {
    /* create destms menu style */
    buffer = (char *)safemalloc(strlen(destname) + 3);
    sprintf(buffer,"\"%s\"",destname);
    action = buffer;
    NewMenuStyle(F_PASS_ARGS);
    free(buffer);
    destms = FindMenuStyle(destname);
    if (!destms)
    {
      /* this must never happen */
      fvwm_msg(ERR,"CopyMenuStyle", "impossible to create %s menu style",
	       destname);
      return;
    }
  }

  if (strcasecmp("*",destname) == 0)
  {
    fvwm_msg(ERR,"CopyMenuStyle", "You cannot copy on the default menu style");
    return;
  }
  if (strcasecmp(ST_NAME(origms),destname) == 0)
  {
    fvwm_msg(ERR,"CopyMenuStyle", "%s and %s identify the same menu style",
	     ST_NAME(origms),destname);
    return;
  }

  if (ST_USAGE_COUNT(destms) != 0)
  {
    fvwm_msg(ERR,"CopyMenuStyle", "menu style %s is in use", destname);
    return;
  }

  /* Copy origms to destms, be award of all pointers in the MenuStyle
     strcture. Use  the same order as in NewMenuStyle */

  /* menu colors */
  FreeColors(&ST_MENU_COLORS(destms).fore, 1);
  FreeColors(&ST_MENU_COLORS(destms).back, 1);
  memcpy(&ST_MENU_COLORS(destms),&ST_MENU_COLORS(origms),sizeof(ColorPair));

  /* Greyed */
  if (ST_HAS_STIPPLE_FORE(destms))
    FreeColors(&ST_MENU_STIPPLE_COLORS(destms).fore, 1);
  ST_HAS_STIPPLE_FORE(destms) = ST_HAS_STIPPLE_FORE(origms);
  if (ST_HAS_STIPPLE_FORE(origms))
    memcpy(&ST_MENU_STIPPLE_COLORS(destms).fore,
	   &ST_MENU_STIPPLE_COLORS(origms).fore, sizeof(Pixel));

  /* HilightBack */
  if (ST_HAS_ACTIVE_BACK(destms))
    FreeColors(&ST_MENU_ACTIVE_COLORS(destms).back, 1);
  ST_HAS_ACTIVE_BACK(destms) = ST_HAS_ACTIVE_BACK(origms);
  if (ST_HAS_ACTIVE_BACK(origms))
    memcpy(&ST_MENU_ACTIVE_COLORS(destms).back,
	   &ST_MENU_ACTIVE_COLORS(origms).back, sizeof(Pixel));

  ST_DO_HILIGHT(destms) = ST_DO_HILIGHT(origms);

  /* ActiveFore */
  if (ST_HAS_ACTIVE_FORE(destms))
    FreeColors(&ST_MENU_ACTIVE_COLORS(destms).fore, 1);
  ST_HAS_ACTIVE_FORE(destms) = ST_HAS_ACTIVE_FORE(origms);
  if (ST_HAS_ACTIVE_FORE(origms))
    memcpy(&ST_MENU_ACTIVE_COLORS(destms).fore,
	   &ST_MENU_ACTIVE_COLORS(origms).fore, sizeof(Pixel));

  /* Hilight3D */
  ST_RELIEF_THICKNESS(destms) = ST_RELIEF_THICKNESS(origms);
  /* Animation */
  ST_IS_ANIMATED(destms) = ST_IS_ANIMATED(origms);

  /* font */
  if (ST_PSTDFONT(destms) && ST_PSTDFONT(destms) != &Scr.DefaultFont)
  {
    FreeFvwmFont(dpy, ST_PSTDFONT(destms));
    free(ST_PSTDFONT(destms));
  }
  if (ST_PSTDFONT(origms) && ST_PSTDFONT(origms) != &Scr.DefaultFont)
  {
    unsigned long value = 0;
    char *orig_font_name;
    FvwmFont new_font;
    Bool loaded = 0;

    if (XGetFontProperty(ST_PSTDFONT(origms)->font,XA_FONT,&value))
    {
      orig_font_name = XGetAtomName(dpy, value);
      if (orig_font_name != NULL)
      {
	if (!LoadFvwmFont(dpy, orig_font_name, &new_font))
	{
	  fvwm_msg(ERR, "CopyMenuStyle",
		   "Couldn't load font '%s' or 'fixed'\n", orig_font_name);
	}
	else
	{
	  loaded = 1;
	}
	XFree(orig_font_name);
      }
    }
    if (loaded)
    {
      ST_PSTDFONT(destms) = (FvwmFont *)safemalloc(sizeof(FvwmFont));
      *ST_PSTDFONT(destms) = new_font;
    }
    else
    {
      ST_PSTDFONT(destms) = &Scr.DefaultFont;
      fvwm_msg(ERR, "CopyMenuStyle",
	       "Unable to find origin font name: Using Default Font\n");
    }
  }
  else
    ST_PSTDFONT(destms) = &Scr.DefaultFont;

  /* MenuFace */
  FreeMenuFace(dpy, &ST_FACE(destms));
  ST_FACE(destms).type = SimpleMenu;
  switch (ST_FACE(origms).type)
  {
  case SolidMenu:
    memcpy(&ST_FACE(destms).u.back,&ST_FACE(origms).u.back,sizeof(Pixel));
    ST_FACE(destms).type = SolidMenu;
    break;
  case GradientMenu:
    ST_FACE(destms).u.grad.pixels =
      (Pixel *)safemalloc(sizeof(Pixel) * ST_FACE(origms).u.grad.npixels);
    memcpy(ST_FACE(destms).u.grad.pixels,ST_FACE(origms).u.grad.pixels,
	   sizeof(Pixel) * ST_FACE(origms).u.grad.npixels);
    ST_FACE(destms).u.grad.npixels = ST_FACE(origms).u.grad.npixels;
    ST_FACE(destms).type = GradientMenu;
    ST_FACE(destms).gradient_type = ST_FACE(origms).gradient_type;
    break;
  case PixmapMenu:
  case TiledPixmapMenu:
    ST_FACE(destms).u.p = CachePicture(dpy, Scr.NoFocusWin, NULL,
			     ST_FACE(origms).u.p->name, Scr.ColorLimit);
    memcpy(&ST_FACE(destms).u.back,&ST_FACE(origms).u.back,sizeof(Pixel));
    ST_FACE(destms).type = ST_FACE(origms).type;
    break;
  default:
    break;
  }

  /* PopupDelay */
  ST_POPUP_DELAY(destms) = ST_POPUP_DELAY(origms);
  /* PopupOffset */
  ST_POPUP_OFFSET_PERCENT(destms) = ST_POPUP_OFFSET_PERCENT(origms);
  /* TitleWarp */
  ST_DO_WARP_TO_TITLE(destms) = ST_DO_WARP_TO_TITLE(origms);
  /* TitleUnderlines */
  ST_TITLE_UNDERLINES(destms) = ST_TITLE_UNDERLINES(origms);
  /* Separators */
  ST_HAS_LONG_SEPARATORS(destms) = ST_HAS_LONG_SEPARATORS(origms);
  /* Triangles */
  ST_HAS_TRIANGLE_RELIEF(destms) = ST_HAS_TRIANGLE_RELIEF(origms);
  /* PopupDelayed */
  ST_DO_POPUP_IMMEDIATELY(destms) = ST_DO_POPUP_IMMEDIATELY(origms);
  /* DoubleClickTime */
  ST_DOUBLE_CLICK_TIME(destms) = ST_DOUBLE_CLICK_TIME(origms);

  /* SidePic */
  if (ST_SIDEPIC(destms))
  {
    DestroyPicture(dpy, ST_SIDEPIC(destms));
    ST_SIDEPIC(destms) = NULL;
  }
  if (ST_SIDEPIC(origms))
    ST_SIDEPIC(destms) = CachePicture(dpy, Scr.NoFocusWin, NULL,
				      ST_SIDEPIC(origms)->name,
				      Scr.ColorLimit);

  /* side color */
  if (ST_HAS_SIDE_COLOR(destms) == 1)
    FreeColors(&ST_SIDE_COLOR(destms), 1);
  ST_HAS_SIDE_COLOR(destms) = ST_HAS_SIDE_COLOR(origms);
  if (ST_HAS_SIDE_COLOR(origms) == 1)
    memcpy(&ST_SIDE_COLOR(destms),&ST_SIDE_COLOR(origms),sizeof(Pixel));

  /* PopupAsRootmenu */
  ST_DO_POPUP_AS_ROOT_MENU(destms) = ST_DO_POPUP_AS_ROOT_MENU(origms);
  /* RemoveSubmenus */
  ST_DO_UNMAP_SUBMENU_ON_POPDOWN(destms) =
    ST_DO_UNMAP_SUBMENU_ON_POPDOWN(origms);
  /* SubmenusRight */
  ST_USE_LEFT_SUBMENUS(destms) = ST_USE_LEFT_SUBMENUS(origms);
  /* BorderWidth */
  ST_BORDER_WIDTH(destms) = ST_BORDER_WIDTH(origms);
  /* Hilight3DThickness */
  ST_IS_ITEM_RELIEF_REVERSED(destms) = ST_IS_ITEM_RELIEF_REVERSED(origms);

  /* ItemFormat */
  if (ST_ITEM_FORMAT(destms))
  {
    free(ST_ITEM_FORMAT(destms));
    ST_ITEM_FORMAT(destms) = NULL;
  }
  if (ST_ITEM_FORMAT(origms))
    ST_ITEM_FORMAT(destms) = safestrdup(ST_ITEM_FORMAT(origms));

  /* AutomaticHotkeys */
  ST_USE_AUTOMATIC_HOTKEYS(destms) = ST_USE_AUTOMATIC_HOTKEYS(origms);
  /* Item and Title Spacing */
  ST_ITEM_GAP_ABOVE(destms) =  ST_ITEM_GAP_ABOVE(origms);
  ST_ITEM_GAP_BELOW(destms) = ST_ITEM_GAP_BELOW(origms);
  ST_TITLE_GAP_ABOVE(destms) = ST_TITLE_GAP_ABOVE(origms);
  ST_TITLE_GAP_BELOW(destms) = ST_TITLE_GAP_BELOW(origms);
  /* MenuColorset */
  ST_HAS_MENU_CSET(destms) = ST_HAS_MENU_CSET(origms);
  ST_CSET_MENU(destms) = ST_CSET_MENU(origms);
  /* ActiveColorset */
  ST_HAS_ACTIVE_CSET(destms) = ST_HAS_ACTIVE_CSET(origms);
  ST_CSET_ACTIVE(destms) = ST_CSET_ACTIVE(origms);
  /* MenuColorset */
  ST_HAS_GREYED_CSET(destms) = ST_HAS_GREYED_CSET(origms);
  ST_CSET_GREYED(destms) = ST_CSET_GREYED(origms);
  /* SelectOnRelease */
  ST_SELECT_ON_RELEASE_KEY(destms) = ST_SELECT_ON_RELEASE_KEY(origms);
  /* PopdownImmediately */
  ST_DO_POPDOWN_IMMEDIATELY(destms) = ST_DO_POPDOWN_IMMEDIATELY(origms);
  /* PopdownDelay */
  ST_POPDOWN_DELAY(destms) = ST_POPDOWN_DELAY(destms);

  UpdateMenuStyle(destms);
}

static void OldMenuStyle(F_CMD_ARGS)
{
  char *buffer, *rest;
  char *fore, *back, *stipple, *font, *style, *animated;

  rest = GetNextToken(action,&fore);
  rest = GetNextToken(rest,&back);
  rest = GetNextToken(rest,&stipple);
  rest = GetNextToken(rest,&font);
  rest = GetNextToken(rest,&style);
  rest = GetNextToken(rest,&animated);

  if(!fore || !back || !stipple || !font || !style)
  {
    fvwm_msg(ERR,"OldMenuStyle", "error in %s style specification", action);
  }
  else
  {
    buffer = (char *)safemalloc(strlen(action) + 100);
    sprintf(buffer,
	    "* \"%s\", Foreground \"%s\", Background \"%s\", "
	    "Greyed \"%s\", Font \"%s\", \"%s\"",
	    style, fore, back, stipple, font,
	    (animated && StrEquals(animated, "anim")) ?
	    "Animation" : "AnimationOff");
    action = buffer;
    NewMenuStyle(F_PASS_ARGS);
    free(buffer);
  }

  if(fore)
    free(fore);
  if(back)
    free(back);
  if(stipple)
    free(stipple);
  if(font)
    free(font);
  if(style)
    free(style);
  if(animated)
    free(animated);
}

void CMD_MenuStyle(F_CMD_ARGS)
{
  char *option;

  GetNextSimpleOption(SkipNTokens(action, 1), &option);
  if (option == NULL || GetMenuStyleIndex(option) != -1)
    NewMenuStyle(F_PASS_ARGS);
  else
    OldMenuStyle(F_PASS_ARGS);
  if (option)
    free(option);
  return;
}

/* This is called by the window list function */
void change_mr_menu_style(MenuRoot *mr, char *stylename)
{
  MenuStyle *ms = NULL;

  ms = FindMenuStyle(stylename);
  if(ms == NULL)
    return;
  if (MR_MAPPED_COPIES(mr) != 0)
  {
    fvwm_msg(ERR,"ChangeMenuStyle", "menu %s is in use", MR_NAME(mr));
  }
  else
  {
    MR_STYLE(mr) = ms;
    MR_IS_UPDATED(mr) = 1;
  }
}

void CMD_ChangeMenuStyle(F_CMD_ARGS)
{
  char *name = NULL;
  char *menuname = NULL;
  MenuStyle *ms = NULL;
  MenuRoot *mr = NULL;

  name = PeekToken(action, &action);
  if (name == NULL)
  {
    fvwm_msg(ERR,"ChangeMenuStyle", "needs at least two parameters");
    return;
  }

  ms = FindMenuStyle(name);
  if(ms == NULL)
  {
    fvwm_msg(ERR,"ChangeMenuStyle", "cannot find style %s", name);
    return;
  }

  menuname = PeekToken(action, &action);
  while(menuname && *menuname)
  {
    mr = FindPopup(menuname);
    if(mr == NULL)
    {
      fvwm_msg(ERR,"ChangeMenuStyle", "cannot find menu %s", menuname);
      break;
    }
    if (MR_MAPPED_COPIES(mr) != 0)
    {
      fvwm_msg(ERR,"ChangeMenuStyle", "menu %s is in use", menuname);
    }
    else
    {
      MR_STYLE(mr) = ms;
      MR_IS_UPDATED(mr) = 1;
    }
    menuname = PeekToken(action, &action);
  }
}

void CMD_AddToMenu(F_CMD_ARGS)
{
  MenuRoot *mr;
  MenuRoot *mrPrior;
  char *token, *rest,*item;

  token = PeekToken(action, &rest);
  if (!token)
    return;
  mr = FindPopup(token);
  if (mr && MR_MAPPED_COPIES(mr) != 0)
  {
    fvwm_msg(ERR,"add_item_to_menu", "menu %s is in use", token);
    return;
  }
  mr = FollowMenuContinuations(FindPopup(token), &mrPrior);
  if (mr == NULL)
    mr = NewMenuRoot(token);

  /* Set + state to last menu */
  set_last_added_item(ADDED_MENU, mr);

  rest = GetNextToken(rest, &item);
  AddToMenu(mr, item, rest, True /* pixmap scan */, True);
  if (item)
    free(item);

  return;
}

void add_another_menu_item(char *action)
{
  MenuRoot *mr;
  MenuRoot *mrPrior;
  char *rest,*item;

  mr = FollowMenuContinuations(Scr.last_added_item.item, &mrPrior);
  if(mr == NULL)
    return;
  if (MR_MAPPED_COPIES(mr) != 0)
  {
    fvwm_msg(ERR,"add_another_menu_item", "menu is in use");
    return;
  }
  rest = GetNextToken(action,&item);
  AddToMenu(mr, item,rest,True /* pixmap scan */, False);
  if (item)
    free(item);

  return;
}

/*
 * ----------------------------- Position hints -------------------------------
 */

static int float_to_int_with_tolerance(float f)
{
  int low;

  if (f < 0)
    low = (int)(f - ROUNDING_ERROR_TOLERANCE);
  else
    low = (int)(f + ROUNDING_ERROR_TOLERANCE);
  if ((int)f != low)
    return low;
  else
    return (int)(f);
}

static void get_xy_from_position_hints(
  MenuPosHints *ph, int width, int height, int context_width,
  Bool do_reverse_x, int *ret_x, int *ret_y)
{
  float x_add;
  float y_add;

  *ret_x = ph->x;
  *ret_y = ph->y;
  if (ph->is_menu_relative)
  {
    if (do_reverse_x)
    {
      *ret_x -= ph->x_offset;
      x_add =
	width * (-1.0 - ph->x_factor) +
	ph->menu_width * (1.0 - ph->context_x_factor);
    }
    else
    {
      *ret_x += ph->x_offset;
      x_add =
	width * ph->x_factor +
	ph->menu_width * ph->context_x_factor;
    }
    y_add = height * ph->y_factor;
  }
  else
  {
    x_add = width * ph->x_factor;
    y_add = height * ph->y_factor;
  }
  *ret_x += float_to_int_with_tolerance(x_add);
  *ret_y += float_to_int_with_tolerance(y_add);
}

/*****************************************************************************
 * Used by get_menu_options
 *
 * The vars are named for the x-direction, but this is used for both x and y
 *****************************************************************************/
static char *get_one_menu_position_argument(
  char *action, int x, int w, int *pFinalX, int *x_offset, float *width_factor,
  float *context_width_factor, Bool *is_menu_relative)
{
  char *token, *orgtoken, *naction;
  char c;
  int val;
  int chars;
  float fval;
  float factor = (float)w/100;
  float x_add = 0;

  naction = GetNextToken(action, &token);
  if (token == NULL)
    return action;
  orgtoken = token;
  *pFinalX = x;
  *x_offset = 0;
  *width_factor = 0.0;
  *context_width_factor = 0.0;
  if (sscanf(token,"o%d%n", &val, &chars) >= 1)
  {
    fval = val;
    token += chars;
    x_add += fval*factor;
    *width_factor -= fval / 100.0;
    *context_width_factor += fval / 100.0;
  }
  else if (token[0] == 'c')
  {
    token++;
    x_add += ((float)w) / 2.0;
    *width_factor -= 0.5;
    *context_width_factor += 0.5;
  }
  while (*token != 0)
  {
    if (sscanf(token,"%d%n", &val, &chars) >= 1)
    {
      fval = (float)val;
      token += chars;
      if (sscanf(token,"%c", &c) == 1)
      {
	switch (c)
	{
	case 'm':
	  token++;
	  *width_factor += fval / 100.0;
	  *is_menu_relative = True;
	  break;
	case 'p':
	  token++;
	  x_add += val;
	  *x_offset += val;
	  break;
	default:
	  x_add += fval * factor;
	  *context_width_factor += fval / 100.0;
	  break;
	}
      }
      else
      {
	x_add += fval * factor;
	*context_width_factor += fval / 100.0;
      }
    }
    else
    {
      naction = action;
      break;
    }
  }
  *pFinalX += float_to_int_with_tolerance(x_add);
  free(orgtoken);
  return naction;
}

/*****************************************************************************
 * get_menu_options is used for Menu, Popup and WindowList
 * It parses strings matching
 *
 *   [ [context-rectangle] x y ] [special-options] [other arguments]
 *
 * and returns a pointer to the first part of the input string that doesn't
 * match this syntax.
 *
 * See documentation for a detailed description.
 ****************************************************************************/
char *get_menu_options(
  char *action, Window w, FvwmWindow *tmp_win, XEvent *e, MenuRoot *mr,
  MenuItem *mi, MenuOptions *pops)
{
  char *tok = NULL;
  char *naction = action;
  char *taction;
  int x;
  int y;
  int button;
  int gflags;
  unsigned int width;
  unsigned int height;
  int dummy_int;
  float dummy_float;
  Bool dummy_flag;
  Window context_window = None;
  Bool fHasContext, fUseItemOffset, fRectangleContext, fXineramaRoot;
  Bool fValidPosHints =
    last_saved_pos_hints.flags.is_last_menu_pos_hints_valid;
  /* If this is set we may want to reverse the position hints, so don't sum up
   * the totals right now. This is useful for the SubmenusLeft style. */

  fXineramaRoot = False;
  last_saved_pos_hints.flags.is_last_menu_pos_hints_valid = False;
  if (pops == NULL)
  {
    fvwm_msg(ERR,"get_menu_options","no MenuOptions pointer passed");
    return action;
  }

  taction = action;
  memset(&(pops->flags), 0, sizeof(pops->flags));
  pops->flags.has_poshints = 0;
  if (!action || *action == 0)
  {
    if (!pops->pos_hints.has_screen_origin)
    {
      if (!GetLocationFromEventOrQuery(
	    dpy, None, e, &pops->pos_hints.screen_origin_x,
	    &pops->pos_hints.screen_origin_y))
      {
	pops->pos_hints.screen_origin_x = 0;
	pops->pos_hints.screen_origin_y = 0;
      }
    }
  }
  while (action != NULL && *action != 0)
  {
    /* ^ just to be able to jump to end of loop without 'goto' */
    gflags = NoValue;
    pops->pos_hints.is_relative = False;
    pops->pos_hints.menu_width = 0;
    pops->pos_hints.is_menu_relative = False;
    /* parse context argument (if present) */
    naction = GetNextToken(taction, &tok);
    if (!tok)
    {
      /* no context string */
      fHasContext = False;
      break;
    }

    pops->pos_hints.is_relative = True; /* set to False for absolute hints! */
    fUseItemOffset = False;
    fHasContext = True;
    fRectangleContext = False;
    if (StrEquals(tok, "context"))
    {
      if (mi && mr)
	context_window = MR_WINDOW(mr);
      else if (tmp_win)
      {
	if (IS_ICONIFIED(tmp_win))
	  context_window = tmp_win->icon_pixmap_w;
	else
	  context_window = tmp_win->frame;
      }
      else
	context_window = w;
    }
    else if (StrEquals(tok,"menu"))
    {
      if (mr)
      {
	context_window = MR_WINDOW(mr);
	pops->pos_hints.is_menu_relative = True;
	pops->pos_hints.menu_width = MR_WIDTH(mr);
      }
    }
    else if (StrEquals(tok,"item"))
    {
      if (mi && mr)
      {
	context_window = MR_WINDOW(mr);
	fUseItemOffset = True;
	pops->pos_hints.is_menu_relative = True;
	pops->pos_hints.menu_width = MR_WIDTH(mr);
      }
    }
    else if (StrEquals(tok,"icon"))
    {
      if (tmp_win && IS_ICONIFIED(tmp_win))
	context_window = tmp_win->icon_pixmap_w;
    }
    else if (StrEquals(tok,"window"))
    {
      if (tmp_win && !IS_ICONIFIED(tmp_win))
	context_window = tmp_win->frame;
    }
    else if (StrEquals(tok,"interior"))
    {
      if (tmp_win && !IS_ICONIFIED(tmp_win))
	context_window = tmp_win->w;
    }
    else if (StrEquals(tok,"title"))
    {
      if (tmp_win)
      {
	if (IS_ICONIFIED(tmp_win))
	  context_window = tmp_win->icon_w;
	else
	  context_window = tmp_win->title_w;
      }
    }
    else if (strncasecmp(tok,"button",6) == 0)
    {
      if (sscanf(&(tok[6]),"%d",&button) != 1 ||
		 tok[6] == '+' || tok[6] == '-' || button < 0 || button > 9)
      {
	fHasContext = False;
      }
      else if (tmp_win && !IS_ICONIFIED(tmp_win))
      {
	button = BUTTON_INDEX(button);
	context_window = tmp_win->button_w[button];
      }
    }
    else if (StrEquals(tok,"root"))
    {
      context_window = Scr.Root;
      pops->pos_hints.is_relative = False;
    }
    else if (StrEquals(tok,"xineramaroot"))
    {
      context_window = Scr.Root;
      pops->pos_hints.is_relative = False;
      fXineramaRoot = True;
    }
    else if (StrEquals(tok,"mouse"))
    {
      context_window = None;
    }
    else if (StrEquals(tok,"rectangle"))
    {
      int flags;
      /* parse the rectangle */
      free(tok);
      naction = GetNextToken(naction, &tok);
      if (tok == NULL)
      {
	fvwm_msg(ERR,"get_menu_options","missing rectangle geometry");
	if (!pops->pos_hints.has_screen_origin)
	{
	  /* xinerama: emergency fallback */
	  pops->pos_hints.has_screen_origin = 1;
	  pops->pos_hints.screen_origin_x = 0;
	  pops->pos_hints.screen_origin_y = 0;
	}
	return action;
      }
      flags = FScreenParseGeometry(tok, &x, &y, &width, &height);
      if ((flags & (XValue | YValue)) != (XValue | YValue))
      {
	free(tok);
	fvwm_msg(ERR,"get_menu_options","invalid rectangle geometry");
	if (!pops->pos_hints.has_screen_origin)
	{
	  /* xinerama: emergency fallback */
	  pops->pos_hints.has_screen_origin = 1;
	  pops->pos_hints.screen_origin_x = 0;
	  pops->pos_hints.screen_origin_y = 0;
	}
	return action;
      }
      if (!(flags & WidthValue))
      {
	width = 1;
      }
      if (!(flags & HeightValue))
      {
	height = 1;
      }
      /* These calculations shouldn't be affected by Xinerama support logic
       * since geometry specifications are for "global" screen */
      if (flags & XNegative)
	x = Scr.MyDisplayWidth - x - width;
      if (flags & YNegative)
	y = Scr.MyDisplayHeight - y - height;
      pops->pos_hints.is_relative = False;
      fRectangleContext = True;
    }
    else if (StrEquals(tok,"this"))
    {
      MenuRoot *dummy_mr;

      context_window = w;
      if (XFindContext(dpy, w, MenuContext, (caddr_t *)&dummy_mr) != XCNOENT)
      {
	if (mr)
	{
	  pops->pos_hints.is_menu_relative = True;
	  pops->pos_hints.menu_width = MR_WIDTH(mr);
	}
      }
    }
    else
    {
      /* no context string */
      fHasContext = False;
    }

    if (tok)
      free(tok);
    if (fHasContext)
      taction = naction;
    else
      naction = action;

    if (fRectangleContext)
    {
      if (!pops->pos_hints.has_screen_origin)
      {
	/* xinerama: use global screen as reference */
	pops->pos_hints.has_screen_origin = 1;
	pops->pos_hints.screen_origin_x = -1;
	pops->pos_hints.screen_origin_y = -1;
      }
      /* nothing else to do */
    }
    else if (!fHasContext || !context_window ||
	     !XGetGeometry(dpy, context_window, &JunkRoot, &JunkX, &JunkY,
			   &width, &height, &JunkBW, &JunkDepth) ||
	     !XTranslateCoordinates(
	       dpy, context_window, Scr.Root, 0, 0, &x, &y, &JunkChild))
    {
      /* no window or could not get geometry */
      XQueryPointer(dpy,Scr.Root, &JunkRoot, &JunkChild, &x, &y, &JunkX, &JunkY,
		    &JunkMask);
      width = height = 1;
      if (!pops->pos_hints.has_screen_origin)
      {
	/* xinerama: use screen with pinter as reference */
	pops->pos_hints.has_screen_origin = 1;
	pops->pos_hints.screen_origin_x = x;
	pops->pos_hints.screen_origin_y = y;
      }
    }
    else
    {
      /* we have a context window */
      if (fUseItemOffset)
      {
	y += MI_Y_OFFSET(mi);
	height = MI_HEIGHT(mi);
      }
      if (!pops->pos_hints.has_screen_origin)
      {
	pops->pos_hints.has_screen_origin = 1;
	if (fXineramaRoot)
	{
	  /* use whole screen */
	  pops->pos_hints.screen_origin_x = -1;
	  pops->pos_hints.screen_origin_y = -1;
	}
	else if (context_window == Scr.Root)
	{
	  /* xinerama: use screen that contains the window center as reference
	   */
	  if (!GetLocationFromEventOrQuery(
		dpy, context_window, e, &pops->pos_hints.screen_origin_x,
		&pops->pos_hints.screen_origin_y))
	  {
	    pops->pos_hints.screen_origin_x = 0;
	    pops->pos_hints.screen_origin_y = 0;
	  }
	  else
	  {
	    fscreen_scr_arg fscr;

	    fscr.xypos.x = pops->pos_hints.screen_origin_x;
	    fscr.xypos.y = pops->pos_hints.screen_origin_y;
	    FScreenGetScrRect(&fscr, FSCREEN_XYPOS, &x, &y, &width, &height);
	  }
	}
	else
	{
	  /* xinerama: use screen that contains the window center as reference
	   */
	  pops->pos_hints.screen_origin_x = JunkX + width / 2;
	  pops->pos_hints.screen_origin_y = JunkY + height / 2;
	}
      }
    }

    /* parse position arguments */
    taction = get_one_menu_position_argument(
      naction, x, width, &(pops->pos_hints.x), &(pops->pos_hints.x_offset),
      &(pops->pos_hints.x_factor), &(pops->pos_hints.context_x_factor),
      &pops->pos_hints.is_menu_relative);
    if (pops->pos_hints.is_menu_relative)
    {
      pops->pos_hints.x = x;
      if (pops->pos_hints.menu_width == 0 && mr)
      {
	pops->pos_hints.menu_width = MR_WIDTH(mr);
      }
    }
    naction = get_one_menu_position_argument(
      taction, y, height, &(pops->pos_hints.y), &dummy_int,
      &(pops->pos_hints.y_factor), &dummy_float, &dummy_flag);
    if (naction == taction)
    {
      /* argument is missing or invalid */
      if (fHasContext)
	fvwm_msg(ERR,"get_menu_options","invalid position arguments");
      naction = action;
      taction = action;
      break;
    }
    taction = naction;
    pops->flags.has_poshints = 1;
    if (fValidPosHints == True && pops->pos_hints.is_relative == True)
    {
      pops->pos_hints = last_saved_pos_hints.pos_hints;
    }
    /* we want to do this only once */
    break;
  } /* while */

  if (!pops->flags.has_poshints && fValidPosHints)
  {
    pops->flags.has_poshints = 1;
    pops->pos_hints = last_saved_pos_hints.pos_hints;
    pops->pos_hints.is_relative = False;
  }

  action = naction;
  /* to keep Purify silent */
  pops->flags.do_select_in_place = 0;
  /* parse additional options */
  while (naction && *naction)
  {
    naction = GetNextToken(action, &tok);
    if (!tok)
      break;
    if (StrEquals(tok, "WarpTitle"))
    {
      pops->flags.do_warp_title = 1;
      pops->flags.do_not_warp = 0;
    }
    else if (StrEquals(tok, "NoWarp"))
    {
      pops->flags.do_warp_title = 0;
      pops->flags.do_not_warp = 1;
    }
    else if (StrEquals(tok, "Fixed"))
    {
      pops->flags.is_fixed = 1;
    }
    else if (StrEquals(tok, "SelectInPlace"))
    {
      pops->flags.do_select_in_place = 1;
    }
    else if (StrEquals(tok, "SelectWarp"))
    {
      pops->flags.do_warp_on_select = 1;
    }
    else
    {
      free (tok);
      break;
    }
    action = naction;
    free (tok);
  }
  if (!pops->flags.do_select_in_place)
  {
    pops->flags.do_warp_on_select = 0;
  }

  return action;
}


/* Returns the menu options for the menu that a given menu item pops up */
static void get_popup_options(
  MenuParameters *pmp, MenuItem *mi, MenuOptions *pops)
{
  if (!mi)
    return;
  pops->flags.has_poshints = 0;
  pops->pos_hints.has_screen_origin = True;
  pops->pos_hints.screen_origin_x = pmp->screen_origin_x;
  pops->pos_hints.screen_origin_y = pmp->screen_origin_y;
  /* just look past "Popup <name>" in the action */
  get_menu_options(
    SkipNTokens(MI_ACTION(mi), 2), MR_WINDOW(pmp->menu), NULL, NULL, pmp->menu,
    mi, pops);
}
