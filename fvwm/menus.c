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

/*****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/* Almost a complete rewrite by
   Greg J. Badros, Apr 6, Apr 16-19, 1997
   gjb@cs.washington.edu

   Fixed bugs w/ Escape leaving a menu item highlighted the next
      time it pops up
   Added Checking for reused hotkeys
   Added Win-like drawing code (motivated by fvwm95)
   Splitting of long (i.e. tall) menus into "More..." popup menus
   Integrated LEFT_MENUS compile-time option as a bug fix
      and added new menu style MWMLeft to builtin.c
   Drastically Improved handling of keyboard-movement keystrokes --
      moves one item at a time
   Animated animated menus (along with animated move)
   */

/* German Gomez Garcia, Nov 1998
   german@pinon.ccu.uniovi.es

   Implemented new menu style definition, allowing multiple definitios and
   gradients and pixmaps 'ala' ButtonStyle. See doc/README.styles for more
   info.  */


/***********************************************************************
 *
 * fvwm menu code
 *
 ***********************************************************************/
/* #define FVWM_DEBUG_MSGS */
#include "config.h"

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <X11/keysym.h>
#include <sys/types.h>
#include <sys/time.h>

#include "fvwm.h"
#include "events.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "repeat.h"
#include "screen.h"

/* prevent core dumping */
#define MAX_MENU_COPIES 4

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

/* This global is saved and restored every time a function is called that
 * might modify them, so we can safely let it live outside a function. */
static saved_pos_hints last_saved_pos_hints =
{
  { False, False },
  { 0, 0, 0.0, 0.0, False }
};


/* This external is safe. It's written only during startup. */
extern XContext MenuContext;

/* We need to use this structure to make sure the other parts of fvwm work with
 * the most recent event. */
extern XEvent Event;

/* This one is only read, never written */
extern Time lastTimestamp;

#define IS_TITLE_ITEM(mi) ((mi)->flags.is_title)
#define IS_POPUP_ITEM(mi) ((mi)->flags.is_popup)
#define IS_MENU_ITEM(mi) ((mi)->flags.is_menu)
#define IS_SEPARATOR_ITEM(mi) ((mi)->flags.is_separator)


static void DrawTrianglePattern(Window,GC,GC,GC,GC,int,int,int,int,char);
static void DrawSeparator(Window, GC,GC,int, int,int,int,int);
static void DrawUnderline(MenuRoot *mr, GC gc, int x, int y,
			  char *txt, int off);
static MenuStatus MenuInteraction(MenuParameters *pmp, double_keypress *pdkp,
				  Bool *pfWarpToItem, Bool *pfMouseMoved);
static void WarpPointerToTitle(MenuRoot *menu);
static MenuItem *MiWarpPointerToItem(MenuRoot *mr, MenuItem *mi,
				     Bool fSkipTitle);
static int PopUpMenu(MenuRoot **pmenu, MenuRoot *parent_menu, FvwmWindow **pfw,
		     int *pcontext, int x, int y, Bool fWarpToItem,
		     MenuOptions *pops, Bool *ret_overlap,
		     Bool *pfWarpToTitle);
static void PopDownMenu(MenuRoot **pmr, MenuParameters *pmp);
static void PopDownAndRepaintParent(MenuRoot **pmr, Bool *fSubmenuOverlaps,
				    MenuParameters *pmp);
static int DoMenusOverlap(MenuRoot *mr, int x, int y, int width, int height,
			  Bool fTolerant);
static void GetPreferredPopupPosition(MenuRoot *mr, int *x, int *y);
static int PopupPositionOffset(MenuRoot *mr);
static void GetPopupOptions(MenuRoot *mr, MenuItem *mi, MenuOptions *pops);
static void paint_menu_item(MenuRoot *mr, MenuItem *mi, FvwmWindow *fw);
static void PaintMenu(MenuRoot *, XEvent *, FvwmWindow *fw);
static void SelectMenuItem(MenuRoot *mr, MenuItem *mi, Bool select,
			   FvwmWindow *fw);
static MenuRoot *MrPopupForMi(MenuRoot *mr, MenuItem *mi);
static int ButtonPosition(int context, FvwmWindow * t);
static Bool IsSubmenuMapped(MenuRoot *parent_menu, MenuItem *parent_item);
static void MakeMenuWindow(MenuRoot *mr);
static MenuRoot *CopyMenuRoot(MenuRoot *menu);



static int menu_middle_offset(MenuRoot *menu)
{
  return MR_XOFFSET(menu) + (MR_WIDTH(menu) - MR_XOFFSET(menu))/2;
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
 *             allow 'double-keypress'.
 * eventp = 1: menu opened by keypress, warp but forbid 'double-keypress'
 *             this should always be used except in the call in 'staysup_func'
 *             in builtin.c
 *
 * Returns one of MENU_NOP, MENU_ERROR, MENU_ABORTED, MENU_DONE
 ***************************************************************************/
MenuStatus do_menu(MenuParameters *pmp)
{
  extern XEvent Event;

  MenuStatus retval = MENU_NOP;
  int x,y;
  Bool fFailedPopup = False;
  Bool fWasAlreadyPopped = False;
  Bool key_press;
  Bool fDoubleClick = False;
  Time t0 = lastTimestamp;
  XEvent tmpevent;
  double_keypress dkp;

  /* must be saved before launching parallel menus (by using the dynamic
   * actions). */
  static Bool fWarpToTitle = False;
  /* don't save these ones, we want them to work even within recursive menus
   * popped up by dynamic actions. */
  static int cindirectDeep = 0;
  static int x_start, y_start;
  static Bool fMouseMoved = False;

  DBUG("do_menu","called");

  if (pmp->flags.is_sticky && !pmp->flags.is_submenu)
    XCheckTypedEvent(dpy, ButtonPressMask, &tmpevent);
  key_press = (pmp->eventp && (pmp->eventp == (XEvent *)1 ||
			       pmp->eventp->type == KeyPress));
  if(pmp->menu == NULL)
  {
    return MENU_ERROR;
  }

  /* Try to pick a root-relative optimal x,y to
     put the mouse right on the title w/o warping */
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x, &y, &JunkX, &JunkY, &JunkMask);
  /* Save these-- we want to warp back here if this is a top level
     menu brought up by a keystroke */
  if (!pmp->flags.is_submenu && cindirectDeep == 0)
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
  }
  /* Figure out where we should popup, if possible */
  if (!pmp->flags.is_submenu || !IsSubmenuMapped(pmp->parent_menu,
						 pmp->parent_item))
  {
    if(pmp->flags.is_submenu)
    {
      /* this is a submenu popup */
      GetPreferredPopupPosition(pmp->parent_menu, &x, &y);
    }
    else
    {
      /* we're a top level menu */
      fMouseMoved = False;
      if(!GrabEm(MENU))
      {
	/* GrabEm specifies the cursor to use */
	XBell(dpy, 0);
	return MENU_ABORTED;
      }
      /* Make the menu appear under the pointer rather than warping */
      x -= menu_middle_offset(pmp->menu);
      y -= MR_STYLE(pmp->menu)->look.EntryHeight/2 + 2;
    }

    /* PopUpMenu may move the x,y to make it fit on screen more nicely */
    /* it might also move parent_menu out of the way */
    if (!PopUpMenu(&(pmp->menu), pmp->parent_menu, pmp->pTmp_win,
		   pmp->pcontext, x, y, key_press /*warp*/, pmp->pops, NULL,
		   &fWarpToTitle))
    {
      fFailedPopup = True;
      XBell(dpy, 0);
      UngrabEm();
      return MENU_ERROR;
    }
  }
  else
  {
    fWasAlreadyPopped = True;
    if (key_press)
      MiWarpPointerToItem(pmp->menu, MR_FIRST_ITEM(pmp->menu),
			  True /* skip Title */);
  }
  fWarpToTitle = False;

  /* Remember the key that popped up the root menu. */
  if (!pmp->flags.is_submenu)
  {
    if (pmp->eventp && pmp->eventp != (XEvent *)1)
    {
      /* we have a real key event */
      dkp.keystate = pmp->eventp->xkey.state;
      dkp.keycode = pmp->eventp->xkey.keycode;
    }
    dkp.timestamp = (key_press) ? t0 : 0;
  }
  if (!fFailedPopup)
    retval = MenuInteraction(pmp, &dkp, &fWarpToTitle, &fMouseMoved);
  else
    retval = MENU_ABORTED;
  if (!fWasAlreadyPopped)
    PopDownMenu(&pmp->menu, pmp);
  pmp->flags.is_menu_from_frame_or_window_or_titlebar = False;
  XFlush(dpy);

  if (!pmp->flags.is_submenu && x_start >= 0 && y_start >= 0 &&
      IS_MENU_BUTTON(retval))
  {
    /* warp pointer back to where invoked if this was brought up
       with a keypress and we're returning from a top level menu,
       and a button release event didn't end it */
    XWarpPointer(dpy, 0, Scr.Root, 0, 0,
		 Scr.MyDisplayWidth, Scr.MyDisplayHeight,x_start, y_start);
  }

  if (lastTimestamp-t0 < Menus.DoubleClickTime && !fMouseMoved &&
      (!key_press || (dkp.timestamp != 0 && !pmp->flags.is_submenu)))
  {
    /* dkp.timestamp is non-zero if a double-keypress occured! */
    fDoubleClick = True;
  }
  dkp.timestamp = 0;
  if(!pmp->flags.is_submenu)
  {
    UngrabEm();
    WaitForButtonsUp();
    if (retval == MENU_DONE || retval == MENU_DONE_BUTTON)
    {
      if (pmp->ret_paction && *(pmp->ret_paction) && !fDoubleClick)
      {
	cindirectDeep++;
	ExecuteFunctionSaveTmpWin(*(pmp->ret_paction),pmp->button_window,
				  &Event, *(pmp->pcontext), -1,
				  EXPAND_COMMAND);
	cindirectDeep--;
	free(*(pmp->ret_paction));
        *(pmp->ret_paction) = NULL;
      }
      last_saved_pos_hints.flags.do_ignore_pos_hints = False;
      last_saved_pos_hints.flags.is_last_menu_pos_hints_valid = False;
    }
  }

  if (fDoubleClick)
    retval = MENU_DOUBLE_CLICKED;
  return retval;
}


/***********************************************************************
 *
 *  Procedure:
 *	Updates menu display to reflect the highlighted item
 *
 * Return value is a menu item
 ***********************************************************************/
static
MenuItem *FindEntry(int *px_offset /*NULL means don't return this value */,
		    MenuRoot **pmr /*NULL means don't return this value */)
{
  MenuItem *mi;
  MenuRoot *mr;
  int root_x, root_y;
  int x,y;
  Window Child;

  /* x_offset returns the x offset of the pointer in the found menu item */
  if (px_offset)
    *px_offset = 0;

  if (pmr)
    *pmr = NULL;

  XQueryPointer( dpy, Scr.Root, &JunkRoot, &Child,
		&root_x,&root_y, &JunkX, &JunkY, &JunkMask);
  if (XFindContext(dpy, Child, MenuContext, (caddr_t *)&mr) == XCNOENT)
  {
    return NULL;
  }

  if (pmr)
      *pmr = mr;
  /* now get position in that child window */
  XQueryPointer( dpy, Child, &JunkRoot, &JunkChild,
		&root_x,&root_y, &x, &y, &JunkMask);

  /* look for the entry that the mouse is in */
  for(mi=MR_FIRST_ITEM(mr); mi; mi=mi->next)
    if(y>=mi->y_offset && y<=mi->y_offset+mi->y_height)
      break;
  if(x < MR_XOFFSET(mr) || x > MR_WIDTH(mr)+2)
    mi = NULL;

  if (mi && px_offset)
    *px_offset = x;

  return mi;
}

/* return the appropriate x offset from the prior menu to
   use as the location of a popup menu */
static
int PopupPositionOffset(MenuRoot *mr)
{
  return (MR_WIDTH(mr) * MR_STYLE(mr)->feel.PopupOffsetPercent / 100 +
	  MR_STYLE(mr)->feel.PopupOffsetAdd);
}

static
void GetPreferredPopupPosition(MenuRoot *mr, int *px, int *py)
{
  int menu_x, menu_y;
  XGetGeometry(dpy,MR_WINDOW(mr),&JunkRoot,&menu_x,&menu_y,
	       &JunkWidth,&JunkHeight,&JunkBW,&JunkDepth);
  *px = menu_x + PopupPositionOffset(mr);
  *py = menu_y;
  if(MR_SELECTED_ITEM(mr))
  {
    /* *py = MR_SELECTED_ITEM(mr)->y_offset + menu_y - (MR_STYLE(mr)->look.EntryHeight/2); */
    *py = MR_SELECTED_ITEM(mr)->y_offset + menu_y;
  }
}


static
int IndexFromMi(MenuRoot *mr, MenuItem *miTarget)
{
  int i = 0;
  MenuItem *mi = MR_FIRST_ITEM(mr);

  for (; mi && mi != miTarget; mi = mi->next)
  {
    if (!IS_SEPARATOR_ITEM(mi))
      i++;
  }
  if (mi == miTarget)
  {
    return i;
  }
  return -1;
}

/* Search for a submenu that was popped up by the given item in the given
 * instance of the menu. */
static
MenuRoot *SeekSubmenuInstance(MenuRoot *parent_menu, MenuItem *parent_item)
{
  MenuRoot *menu;

  for (menu = Menus.all; menu != NULL; menu = MR_NEXT_MENU(menu))
  {
    if (MR_PARENT_MENU(menu) == parent_menu &&
	MR_PARENT_ITEM(menu) == parent_item)
      /* here it is */
      break;
  }
  return menu;
}

static
Bool IsSubmenuMapped(MenuRoot *parent_menu, MenuItem *parent_item)
{
  XWindowAttributes win_attribs;
  MenuRoot *menu;

  menu = SeekSubmenuInstance(parent_menu, parent_item);
  if (menu == NULL)
    return False;

  XGetWindowAttributes(dpy, MR_WINDOW(menu), &win_attribs);
  return (MR_WINDOW(menu) == None) ?
    False : (win_attribs.map_state == IsViewable);
}

static
MenuItem *MiFromMenuIndex(MenuRoot *mr, int index)
{
  int i = -1;
  MenuItem *mi = MR_FIRST_ITEM(mr);
  MenuItem *miLastOk = NULL;
  for (; mi && (i < index || miLastOk == NULL); mi=mi->next)
  {
    if (!IS_SEPARATOR_ITEM(mi))
    {
      miLastOk = mi;
      i++;
    }
  }
  /* DBUG("MiFromMenuIndex","%d = %s",index,miLastOk->item); */
  return miLastOk;
}

static
int CmiFromMenu(MenuRoot *mr)
{
  return IndexFromMi(mr, MR_LAST_ITEM(mr));
}


/***********************************************************************
 * Procedure
 * 	menuShortcuts() - Menu keyboard processing
 *
 * Function called from MenuInteraction instead of Keyboard_Shortcuts()
 * when a KeyPress event is received.  If the key is alphanumeric,
 * then the menu is scanned for a matching hot key.  Otherwise if
 * it was the escape key then the menu processing is aborted.
 * If none of these conditions are true, then the default processing
 * routine is called.
 * TKP - uses XLookupString so that keypad numbers work with windowlist
 ***********************************************************************/
static
MenuStatus menuShortcuts(MenuRoot *menu, XEvent *event, MenuItem **pmiCurrent,
			 double_keypress *pdkp)
{
  int fControlKey = event->xkey.state & ControlMask? True : False;
  int fShiftedKey = event->xkey.state & ShiftMask? True: False;
  KeySym keysym;
  char keychar;
  MenuItem *newItem = NULL;
  MenuItem *miCurrent = pmiCurrent ? *pmiCurrent : NULL;
  int index;

  /* handle double-keypress */
  if (pdkp->timestamp &&
      lastTimestamp-pdkp->timestamp < Menus.DoubleClickTime &&
      event->xkey.state == pdkp->keystate &&
      event->xkey.keycode == pdkp->keycode)
  {
    *pmiCurrent = NULL;
    return MENU_SELECTED;
  }
  pdkp->timestamp = 0;
  /* Is it okay to treat keysym-s as Ascii? */
  /* No, because the keypad numbers don't work. Use XlookupString */
  index = XLookupString(&(event->xkey), &keychar, 1, &keysym, NULL);
  /* Try to match hot keys */
  /* Need isascii here - isgraph might coredump! */
  if (index == 1 && isascii((int)keychar) && isgraph((int)keychar) &&
      fControlKey == False)
  {
    /* allow any printable character to be a keysym, but be sure control
       isn't pressed */
    MenuItem *mi;
    MenuItem *mi1;
    char key;
    int countHotkey = 0;      /* Added by MMH mikehan@best.com 2/7/99 */

    /* if this is a letter set it to lower case */
    if (isupper(keychar))
      keychar = tolower((int)keychar) ;

    /* MMH mikehan@best.com 2/7/99
     * Multiple hotkeys per menu
     * Search menu for matching hotkey;
     * remember how many we found and where we found it */
    mi = ( miCurrent == NULL || miCurrent == MR_LAST_ITEM(menu)) ?
      MR_FIRST_ITEM(menu) : miCurrent->next;
    mi1 = mi;
    do
    {
      key = tolower( mi->chHotkey );
      if ( keychar == key )
      {
	if ( ++countHotkey == 1 )
	  newItem = mi;

      }
      mi = (mi == MR_LAST_ITEM(menu)) ? MR_FIRST_ITEM(menu) : mi->next;
    }
    while (mi != mi1);

    /* For multiple instances of a single hotkey, just move the selection */
    if ( countHotkey > 1 )
    {
      *pmiCurrent = newItem;
      return MENU_NEWITEM;
    }
    /* Do things the old way for unique hotkeys in the menu */
    else if ( countHotkey == 1  )
    {
      *pmiCurrent = newItem;
      if (newItem && IS_POPUP_ITEM(newItem))
	return MENU_POPUP;
      else
	return MENU_SELECTED;
    }
    /* MMH mikehan@best.com 2/7/99 */
  }

  switch(keysym)		/* Other special keyboard handling	*/
    {
    case XK_Escape:		/* Escape key pressed. Abort		*/
      return MENU_ABORTED;
      break;

    case XK_Return:
    case XK_KP_Enter:
      return MENU_SELECTED;
      break;

    case XK_Left:
    case XK_KP_4:
    case XK_b: /* back */
    case XK_h: /* vi left */
      return MENU_POPDOWN;
      break;

    case XK_Right:
    case XK_KP_6:
    case XK_f: /* forward */
    case XK_l: /* vi right */
      if (miCurrent && IS_POPUP_ITEM(miCurrent))
	return MENU_POPUP;
      break;

    case XK_Up:
    case XK_KP_8:
    case XK_k: /* vi up */
    case XK_p: /* prior */
      if (!miCurrent)
      {
        if ((*pmiCurrent = MiFromMenuIndex(menu,0)) != NULL)
	  return MENU_NEWITEM;
	else
	  return MENU_NOP;
      }
      /* Need isascii here - isgraph might coredump! */
      if (isascii(keysym) && isgraph(keysym))
	fControlKey = False; /* don't use control modifier
				for k or p, since those might
				be shortcuts too-- C-k, C-p will
				always work to do a single up */
      index = IndexFromMi(menu, miCurrent);
      if (index == 0)
	/* wraparound */
	index = CmiFromMenu(menu);
      else if (fShiftedKey)
	index = 0;
      else
      {
	index -= (fControlKey?5:1);
      }
      newItem = MiFromMenuIndex(menu,index);
      if (newItem)
      {
	*pmiCurrent = newItem;
	return MENU_NEWITEM;
      }
      else
	return MENU_NOP;
      break;

    case XK_Down:
    case XK_KP_2:
    case XK_j: /* vi down */
    case XK_n: /* next */
      if (!miCurrent)
      {
        if ((*pmiCurrent = MR_LAST_ITEM(menu)) != NULL)
	  return MENU_NEWITEM;
	else
	  return MENU_NOP;
      }
      /* Need isascii here - isgraph might coredump! */
      if (isascii(keysym) && isgraph(keysym))
	fControlKey = False; /* don't use control modifier
				for j or n, since those might
				be shortcuts too-- C-j, C-n will
				always work to do a single down */
      if (fShiftedKey)
	index = CmiFromMenu(menu);
      else
      {
	index = IndexFromMi(menu, miCurrent) + (fControlKey ? 5 : 1);
	/* correct for the case that we're between items */
	if (IS_SEPARATOR_ITEM(miCurrent))
	  index--;
      }
      newItem = MiFromMenuIndex(menu,index);
      if (newItem == miCurrent)
	newItem = MiFromMenuIndex(menu,0);
      if (newItem)
      {
	*pmiCurrent = newItem;
	return MENU_NEWITEM;
      }
      else
	return MENU_NOP;
      break;

    case XK_Page_Up:
      if ((*pmiCurrent = MiFromMenuIndex(menu,0)) != NULL)
	return MENU_NEWITEM;
      else
	return MENU_NOP;
      break;

    case XK_Page_Down:
      if ((*pmiCurrent = MR_LAST_ITEM(menu)) != NULL)
	return MENU_NEWITEM;
      else
	return MENU_NOP;
      break;

      /* Nothing special --- Allow other shortcuts */
    default:
      /* There are no useful shortcuts, so don't do that.
       * (Dominik Vogt, 11-Nov-1998)
       * Keyboard_shortcuts(event, NULL, ButtonRelease); */
      break;
    }

  return MENU_NOP;
}

/***********************************************************************
 *
 *  Procedure:
 *	Interacts with user to Update menu display and start new submenus,
 *      return to old sub menus, etc.
 *  Input
 *      sticks = 0, transient style menu, drops on button release
 *      sticks = 1, sticky style, stays up on a click (i.e. initial release
 *                  comes soon after initial press.
 *  Returns:
 *      MENU_ERROR on error condition
 *      MENU_SUBMENU_DONE on return from submenu to parent menu
 *      MENU_DONE on completion by non-button-release (e.g. keyboard)
 *      MENU_DONE_BUTTON on completion by button release
 *      MENU_ABORT on abort of menu by non-button-release (e.g. keyboard)
 *      MENU_ABORT_BUTTON on aborting by button release
 *      MENU_SELECTED on item selection -- returns action * in *ret_paction
 *
 ***********************************************************************/
static MenuStatus MenuInteraction(MenuParameters *pmp, double_keypress *pdkp,
				  Bool *pfWarpToTitle, Bool *pfMouseMoved)
{
  extern XEvent Event;
  Bool fPopupImmediately;
  MenuItem *mi = NULL;
  MenuItem *tmi;
  MenuRoot *mrMi = NULL;
  MenuRoot *tmrMi = NULL;
  MenuRoot *mrPopup = NULL;
  MenuRoot *mrMiPopup = NULL;
  MenuRoot *mrNeedsPainting = NULL;
  Bool fDoPopupNow = False; /* used for delay popups, to just popup the menu */
  Bool fPopupAndWarp = False; /* used for keystrokes, to popup and move to
			       * that menu */
  Bool fKeyPress = False;
  Bool fForceReposition = True;
  int x_init = 0, y_init = 0;
  int x_offset = 0;
  MenuStatus retval = MENU_NOP;
  int c10msDelays = 0;
  MenuOptions mops;
  Bool fOffMenuAllowed = False;
  Bool fPopdown = False;
  Bool fPopup   = False;
  Bool fDoMenu  = False;
  Bool fMotionFirst = False;
  Bool fReleaseFirst = False;
  Bool fFakedMotion = False;
  Bool fSubmenuOverlaps = False;

  memset(&(mops.flags), 0, sizeof(mops.flags));
  fPopupImmediately = (MR_STYLE(pmp->menu)->feel.flags.do_popup_immediately &&
		       (Menus.PopupDelay10ms > 0));

  /* remember where the pointer was so we can tell if it has moved */
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		 &x_init, &y_init, &JunkX, &JunkY, &JunkMask);

  while (True)
  {
      fPopupAndWarp = False;
      fDoPopupNow = False;
      fKeyPress = False;
      if (fForceReposition)
      {
	Event.type = MotionNotify;
	Event.xmotion.time = lastTimestamp;
	fFakedMotion = True;
	fForceReposition = False;
      }
      else if (!XCheckMaskEvent(dpy,ExposureMask,&Event))
      {
	/* handle exposure events first */
	if (Menus.PopupDelay10ms > 0)
	{
	  while (XCheckMaskEvent(dpy,
				 ButtonPressMask|ButtonReleaseMask|
				 ExposureMask|KeyPressMask|
				 VisibilityChangeMask|ButtonMotionMask,
				 &Event) == False)
	  {
	    usleep(10000 /* 10 ms*/);
	    if (c10msDelays++ == Menus.PopupDelay10ms)
	    {
	      DBUG("MenuInteraction","Faking motion");
	      /* fake a motion event, and set fDoPopupNow */
	      Event.type = MotionNotify;
	      Event.xmotion.time = lastTimestamp;
	      fFakedMotion = True;
	      fDoPopupNow = True;
	      break;
	    }
	  }
	}
	else
	{
	  /* block until there is an event */
	  XMaskEvent(dpy,
		     ButtonPressMask|ButtonReleaseMask|ExposureMask |
		     KeyPressMask|VisibilityChangeMask|ButtonMotionMask,
		     &Event);
	}
      }
      /*DBUG("MenuInteraction","mrPopup=%s",mrPopup?mrPopup->name:"(none)");*/

      StashEventTime(&Event);
      if (Event.type == MotionNotify)
      {
	/* discard any extra motion events before a release */
	while((XCheckMaskEvent(dpy,ButtonMotionMask|ButtonReleaseMask,
			       &Event))&&(Event.type != ButtonRelease))
	  ;
      }

      retval = 0;
      switch(Event.type)
	{
	case ButtonRelease:
	  mi = FindEntry(&x_offset, &mrMi);
	  /* hold the menu up when the button is released
	     for the first time if released OFF of the menu */
	  if(pmp->flags.is_sticky && !fMotionFirst)
	  {
	    fReleaseFirst = True;
	    pmp->flags.is_sticky = False;
	    continue;
	    /* break; */
	  }
	  retval = (mi) ? MENU_SELECTED : MENU_ABORTED;
	  if (retval == MENU_SELECTED && mi && IS_POPUP_ITEM(mi) &&
	      !MR_STYLE(pmp->menu)->feel.flags.do_popup_as_root_menu)
	  {
	      retval = MENU_POPUP;
	      fDoPopupNow = True;
	      break;
	  }
	  pdkp->timestamp = 0;
	  goto DO_RETURN;

	case ButtonPress:
	  /* if the first event is a button press allow the release to
	     select something */
	  pmp->flags.is_sticky = False;
	  continue;

	case VisibilityNotify:
	  continue;

	case KeyPress:
	  /* Handle a key press events to allow mouseless operation */
	  fKeyPress = True;
	  x_offset = 0;
	  retval = menuShortcuts(pmp->menu, &Event, &mi, pdkp);
	  if (retval == MENU_SELECTED && mi && IS_POPUP_ITEM(mi) &&
	      !MR_STYLE(pmp->menu)->feel.flags.do_popup_as_root_menu)
	    retval = MENU_POPUP;
	  if (retval == MENU_POPDOWN ||
	      retval == MENU_ABORTED ||
	      retval == MENU_SELECTED)
	    goto DO_RETURN;
	  /* now warp to the new menu-item, if any */
	  if (mi && (mi != FindEntry(NULL, &tmrMi) || pmp->menu != tmrMi))
	  {
	    MiWarpPointerToItem(tmrMi, mi, False);
	    /* DBUG("MenuInteraction","Warping on keystroke to %s",mi->item);*/
	  }
	  if (retval == MENU_POPUP && mi && IS_POPUP_ITEM(mi))
	  {
	    fPopupAndWarp = True;
	    DBUG("MenuInteraction","fPopupAndWarp = True");
	    break;
	  }
	  break;

	case MotionNotify:
	  if (*pfMouseMoved == False)
	  {
	    int x_root, y_root;
	    XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
			   &x_root,&y_root, &JunkX, &JunkY, &JunkMask);
	    if(x_root-x_init > Scr.MoveThreshold ||
	       x_init-x_root > Scr.MoveThreshold ||
	       y_root-y_init > Scr.MoveThreshold ||
	       y_init-y_root > Scr.MoveThreshold)
	    {
	      /* global variable remember that this isn't just
		 a click any more since the pointer moved */
	      *pfMouseMoved = True;
	    }
	  }
	  mi = FindEntry(&x_offset, &mrMi);
	  if (!fReleaseFirst && !fFakedMotion && *pfMouseMoved)
	    fMotionFirst = True;
	  fFakedMotion = False;
	  break;

	case Expose:
	  /* grab our expose events, let the rest go through */
	  if((XFindContext(dpy, Event.xany.window,MenuContext,
			   (caddr_t *)&mrNeedsPainting)!=XCNOENT))
	  {
	    flush_expose(Event.xany.window);
	    PaintMenu(mrNeedsPainting, &Event, (*pmp->pTmp_win));
	  }
	  /* we want to dispatch this too so window decorations get redrawn
	   * after being obscured by menus. */
	  /* We need to preserve the Tmp_win here! */
	  DispatchEvent(True);
	  continue;

	default:
	  DispatchEvent(False);
	  break;
	} /* switch (Event.type) */

      /* Now handle new menu items, whether it is from a keypress or
	 a pointer motion event */
      if (mi)
      {
	/* we're on a menu item */
	fOffMenuAllowed = False;
	mrMiPopup = MrPopupForMi(pmp->menu, mi);
	if (mrMi != pmp->menu && mrMi != mrPopup && mrMi != mrMiPopup)
	{
	  /* we're on an item from a prior menu */
	  /* DBUG("MenuInteraction","menu %s: returning popdown",
	     MR_NAME(pmp->menu));*/
	  retval = MENU_POPDOWN;
	  pdkp->timestamp = 0;
	  goto DO_RETURN;
	}

	/* see if we're on a new item of this menu */
	if (mi != MR_SELECTED_ITEM(pmp->menu) && mrMi == pmp->menu)
	{
	  c10msDelays = 0;
	  /* we're on the same menu, but a different item,
	     so we need to unselect the old item */
	  if (MR_SELECTED_ITEM(pmp->menu))
	  {
	    /* something else was already selected on this menu */
	    if (mrPopup)
	    {
	      PopDownAndRepaintParent(&mrPopup, &fSubmenuOverlaps, pmp);
	      mrPopup = NULL;
	    }
	    /* We have to pop down the menu before unselecting the item in case
	     * we are using gradient menus. The recalled image would paint over
	     * the submenu. */
	    SelectMenuItem(pmp->menu, MR_SELECTED_ITEM(pmp->menu), False,
			   (*pmp->pTmp_win));
	  }
	  else
	  {
	    /* DBUG("MenuInteraction","Menu %s had nothing else selected",
	       MR_NAME(menu)); */
	  }
	  /* do stuff to highlight the new item; this
	     sets MR_SELECTED_ITEM(menu), too */
	  SelectMenuItem(pmp->menu, mi, True, (*pmp->pTmp_win));
	} /* new item of the same menu */

	/* check what has to be done with the item */
	fPopdown = False;
	fPopup   = False;
	fDoMenu  = False;

	if (mrMi == mrPopup)
	{
	  /* must make current popup menu a real menu */
	  fDoMenu = True;
	}
	else if (fPopupAndWarp)
	{
	  /* must create a real menu and warp into it */
	  if (mrPopup == NULL || mrPopup != mrMiPopup)
	  {
	    fPopup = True;
	  }
	  else
	  {
	    XRaiseWindow(dpy, MR_WINDOW(mrPopup));
	    MiWarpPointerToItem(mrPopup, MR_FIRST_ITEM(mrPopup), True);
	    fDoMenu = True;
	  }
	}
	else if (mi && IS_POPUP_ITEM(mi))
	{
	  if (x_offset >= MR_WIDTH(mrMi)*3/4 || fDoPopupNow ||
	      fPopupImmediately)
	  {
	    /* must create a new menu or popup */
	    if (mrPopup == NULL || mrPopup != mrMiPopup)
	    {
	      fPopup = True;
	    }
	    else if (fPopupAndWarp)
	    {
	      MiWarpPointerToItem(mrPopup, MR_FIRST_ITEM(mrPopup), True);
	    }
	  }
	} /* else if (mi && IS_POPUP_ITEM(mi)) */
	if (fPopup && mrPopup && mrPopup != mrMiPopup)
	{
	  /* must remove previous popup first */
	  fPopdown = True;
	}

	if (fPopdown)
	{
	  DBUG("MenuInteraction","Popping down");
	  /* popdown previous popup */
	  if (mrPopup)
	  {
	    PopDownAndRepaintParent(&mrPopup, &fSubmenuOverlaps, pmp);
	    mrPopup = NULL;
	  }
	  fPopdown = False;
	}
	if (fPopup)
	{
	  DBUG("MenuInteraction","Popping up");
          /* get pos hints for item's action */
          GetPopupOptions(pmp->menu, mi, &mops);
	  mrPopup = mrMiPopup;
	  if (!mrPopup)
	  {
	    fDoMenu = False;
	    fPopdown = False;
	  }
	  else
	  {
	    /* create a popup menu */
	    if (!IsSubmenuMapped(pmp->menu, mi))
	    {
	      /* We want to pop prepop menus so it doesn't *have* to be
		 unpopped; do_menu pops down any menus it pops up, but we
		 want to be able to popdown w/o actually removing the menu */
	      int x, y;
	      GetPreferredPopupPosition(pmp->menu,&x,&y);
	      PopUpMenu(&mrPopup, pmp->menu, pmp->pTmp_win, pmp->pcontext, x,
			y, fPopupAndWarp, &mops, &fSubmenuOverlaps,
			pfWarpToTitle);
	      if (mrPopup == NULL)
	      {
		/* the menu deleted itself when execution the dynamic popup
		 * action */
		retval = MENU_ERROR;
		goto DO_RETURN;
	      }
	    }
	    if (MR_PARENT_MENU(mrPopup) == pmp->menu)
	    {
	      mi = FindEntry(NULL, &mrMi);
	      if (mi && mrMi == mrPopup)
	      {
		fDoMenu = True;
		fPopdown =
		  !MR_STYLE(pmp->menu)->feel.flags.do_popup_immediately;
	      }
	    }
	    else
	    {
	      /* This menu must be already mapped somewhere else, so ignore
	       * it completely. */
	      fDoMenu = False;
	      fPopdown = False;
	      mrPopup = NULL;
	    }
	  } /* if (!mrPopup) */
	} /* if (fPopup) */
	if (fDoMenu)
	{
	  MenuParameters mp;

	  mp.menu = mrPopup;
	  mp.parent_menu = pmp->menu;
	  mp.parent_item = mi;
	  mp.pTmp_win = pmp->pTmp_win;
	  mp.button_window = pmp->button_window;
	  mp.pcontext = pmp->pcontext;
	  mp.flags.is_menu_from_frame_or_window_or_titlebar = False;
	  mp.flags.is_sticky = False;
	  mp.flags.is_submenu = True;
	  mp.eventp = (fPopupAndWarp) ? (XEvent *)1 : NULL;
	  mp.pops = &mops;
	  mp.ret_paction = pmp->ret_paction;

	  /* recursively do the new menu we've moved into */
	  retval = do_menu(&mp);
	  if (IS_MENU_RETURN(retval))
	  {
	    pdkp->timestamp = 0;
	    goto DO_RETURN;
	  }
	  XRaiseWindow(dpy, MR_WINDOW(pmp->menu));
	  if ((fPopdown ||
	       !MR_STYLE(pmp->menu)->feel.flags.do_popup_immediately) &&
	      MR_STYLE(pmp->menu)->feel.flags.do_unmap_submenu_on_popdown)
	  {
	    PopDownAndRepaintParent(&mrPopup, &fSubmenuOverlaps, pmp);
	    mrPopup = NULL;
	  }
	  if (retval == MENU_POPDOWN)
	  {
	    c10msDelays = 0;
	    fForceReposition = True;
	  }
	} /* if (fDoMenu) */

	/* Now check whether we can animate the current popup menu
	   over to the right to unobscure the current menu;  this
	   happens only when using animation */
	tmi = FindEntry(NULL, &tmrMi);
	if (mrPopup && MR_XANIMATION(mrPopup) && tmi &&
	    (tmi == MR_SELECTED_ITEM(pmp->menu) || tmrMi != pmp->menu))
	{
	  int x_popup, y_popup;
	  DBUG("MenuInteraction","Moving the popup menu back over");
	  XGetGeometry(dpy, MR_WINDOW(mrPopup), &JunkRoot, &x_popup, &y_popup,
		       &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	  /* move it back */
	  AnimatedMoveOfWindow(MR_WINDOW(mrPopup),x_popup,y_popup,
			       x_popup-MR_XANIMATION(mrPopup), y_popup,
			       False /* no warp ptr */,-1,NULL);
	  MR_XANIMATION(mrPopup) = 0;
	}
	/* now check whether we should animate the current real menu
	   over to the right to unobscure the prior menu; only a very
	   limited case where this might be helpful and not too disruptive */
	if (mrPopup == NULL && pmp->parent_menu != NULL &&
	    MR_XANIMATION(pmp->menu) != 0 &&
	    x_offset < MR_WIDTH(pmp->menu)/4)
	{
	  int x_menu, y_menu;
	  DBUG("MenuInteraction","Moving the menu back over");
	  /* we have to see if we need menu to be moved */
	  XGetGeometry( dpy, MR_WINDOW(pmp->menu), &JunkRoot, &x_menu, &y_menu,
			&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	  /* move it back */
	  AnimatedMoveOfWindow(MR_WINDOW(pmp->menu),x_menu,y_menu,
			       x_menu - MR_XANIMATION(pmp->menu),y_menu,
			       True /* warp ptr */,-1,NULL);
	  MR_XANIMATION(pmp->menu) = 0;
	}

      } /* if (mi) */
      else {
        /* moved off menu, deselect selected item... */
        if (MR_SELECTED_ITEM(pmp->menu))
	{
          SelectMenuItem(pmp->menu, MR_SELECTED_ITEM(pmp->menu), False,
			 (*pmp->pTmp_win));
          if (mrPopup && fOffMenuAllowed == False)
	  {
	    int x, y, mx, my;
	    unsigned int mw, mh;
	    XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
			   &x, &y, &JunkX, &JunkY, &JunkMask);
	    XGetGeometry( dpy, MR_WINDOW(pmp->menu), &JunkRoot, &mx, &my,
			  &mw, &mh, &JunkBW, &JunkDepth);
	    if ((!MR_IS_LEFT(mrPopup)  && x < mx)    ||
		(!MR_IS_RIGHT(mrPopup) && x > mx+mw) ||
		(!MR_IS_UP(mrPopup)    && y < my)    ||
		(!MR_IS_DOWN(mrPopup)  && y > my+mh))
	    {
	      PopDownAndRepaintParent(&mrPopup, &fSubmenuOverlaps, pmp);
	      mrPopup = NULL;
	    }
	    else
	    {
	      fOffMenuAllowed = True;
	    }
	  } /* if (mrPopup && fOffMenuAllowed == False) */
        } /* if (MR_SELECTED_ITEM(menu)) */
      } /* else (!mi) */
      XFlush(dpy);
    } /* while (True) */

  DO_RETURN:
  if (mrPopup)
  {
    PopDownAndRepaintParent(&mrPopup, &fSubmenuOverlaps, pmp);
    mrPopup = NULL;
  }
  if (retval == MENU_POPDOWN)
  {
    if (MR_SELECTED_ITEM(pmp->menu))
      SelectMenuItem(pmp->menu, MR_SELECTED_ITEM(pmp->menu), False,
		     (*pmp->pTmp_win));
    if (fKeyPress)
    {
      if (!pmp->flags.is_submenu)
	/* abort a root menu rather than pop it down */
	retval = MENU_ABORTED;
      if (pmp->parent_menu && MR_SELECTED_ITEM(pmp->parent_menu))
      {
	MiWarpPointerToItem(pmp->parent_menu,
			    MR_SELECTED_ITEM(pmp->parent_menu), False);
	tmi = FindEntry(NULL, &tmrMi);
	if ((MR_SELECTED_ITEM(pmp->parent_menu) != tmi ||
	     pmp->parent_menu != tmrMi) &&
	    MR_XANIMATION(pmp->menu) == 0)
	{
	  XRaiseWindow(dpy, MR_WINDOW(pmp->parent_menu));
	}
      }
    }
    /*DBUG("MenuInteraction","Prior menu has %s selected",
	 pmp->parent_menu?(MR_SELECTED_ITEM(pmp->parent_menu) ?
			   MR_SELECTED_ITEM(pmp->parent_menu)->item :
			   "(no selected item)") :
	 "(no prior menu)"); */
  }
  else if (retval == MENU_SELECTED)
  {
    /* DBUG("MenuInteraction","Got MENU_SELECTED for menu %s, on item %s",
       MR_NAME(pmp->menu),mi->item); */
    /* save action to execute so that the menu may be destroyed now */
    if (pmp->ret_paction)
      *pmp->ret_paction = (mi) ? strdup(mi->action) : NULL;
    retval = MENU_ADD_BUTTON_IF(fKeyPress,MENU_DONE);
    if (pmp->ret_paction && *pmp->ret_paction && mi)
    {
      if (IS_MENU_ITEM(mi))
      {
	GetPopupOptions(pmp->menu, mi, &mops);
	if (!(mops.flags.select_in_place))
	{
	  last_saved_pos_hints.flags.do_ignore_pos_hints = True;
	}
	else
	{
	  if (mops.flags.has_poshints)
	  {
	    last_saved_pos_hints.pos_hints = mops.pos_hints;
	  }
	  else
	  {
	    GetPreferredPopupPosition(
	      pmp->menu, &last_saved_pos_hints.pos_hints.x,
	      &last_saved_pos_hints.pos_hints.y);
	    last_saved_pos_hints.pos_hints.x_factor = 0;
	    last_saved_pos_hints.pos_hints.y_factor = 0;
	    last_saved_pos_hints.pos_hints.fRelative = False;
	  }
	  last_saved_pos_hints.flags.is_last_menu_pos_hints_valid = True;
	  if (mops.flags.select_warp)
	  {
	    *pfWarpToTitle = True;
	  }
	} /* else (mops.flags.select_in_place) */
      }
    } /* ((retval == MENU_DONE ||... */
  }
  return MENU_ADD_BUTTON_IF(fKeyPress,retval);
}

static
void WarpPointerToTitle(MenuRoot *menu)
{
  int y = MR_STYLE(menu)->look.EntryHeight/2 + 2;
  int x = menu_middle_offset(menu);
  XWarpPointer(dpy, 0, MR_WINDOW(menu), 0, 0, 0, 0, x, y);
}

static
MenuItem *MiWarpPointerToItem(MenuRoot *mr, MenuItem *mi, Bool fSkipTitle)
{
  int y;
  int x = menu_middle_offset(mr);

  if (fSkipTitle)
  {
    while (mi->next != NULL && IS_SEPARATOR_ITEM(mi))
      /* skip separators and titles until the first 'real' item is found */
      mi = mi->next;
  }

  y = mi->y_offset + MR_STYLE(mr)->look.EntryHeight/2;
  XWarpPointer(dpy, 0, MR_WINDOW(mr), 0, 0, 0, 0, x, y);
  return mi;
}

static
int DoMenusOverlap(MenuRoot *mr, int x, int y, int width, int height,
		   Bool fTolerant)
{
  int prior_x, prior_y, x_overlap;
  unsigned int prior_width, prior_height;
  int tolerance1;
  int tolerance2;

  if (mr == NULL)
    return 0;

  if (fTolerant)
    {
      tolerance1 = 3;
      if (MR_STYLE(mr)->feel.PopupOffsetAdd < 0)
	tolerance1 -= MR_STYLE(mr)->feel.PopupOffsetAdd;
      tolerance2 = 4;
    }
  else
    {
      tolerance1 = 1;
      tolerance2 = 1;
    }
  XGetGeometry(dpy,MR_WINDOW(mr),&JunkRoot,&prior_x,&prior_y,
	       &prior_width,&prior_height,&JunkBW,&JunkDepth);
  x_overlap = 0;
  if (fTolerant)
  {
    /* Don't use multiplier if doing an intolerant check */
    prior_width *= (float)(MR_STYLE(mr)->feel.PopupOffsetPercent) / 100.0;
  }
  if (y <= prior_y + prior_height - tolerance2 &&
      prior_y <= y + height - tolerance2 &&
      x <= prior_x + prior_width - tolerance1 &&
      prior_x <= x + width - tolerance2)
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
 *	PopUpMenu - pop up a pull down menu
 *
 *  Inputs:
 *	menu	  - the root pointer of the menu to pop up
 *	x, y	  - location of upper left of menu
 *      fWarpToItem - warp pointer to the first item after title
 *      pops      - pointer to the menu options for new menu
 *
 ***********************************************************************/
static
Bool PopUpMenu(MenuRoot **pmenu, MenuRoot *parent_menu, FvwmWindow **pfw,
	       int *pcontext, int x, int y, Bool fWarpToItem,
	       MenuOptions *pops, Bool *ret_overlap, Bool *pfWarpToTitle)
{
  Bool fWarpTitle = False;
  int x_overlap;
  int x_clipped_overlap;
  MenuItem *mi = NULL;
  MenuRoot *mrMi;
  MenuRoot *menu;
  FvwmWindow *fw;
  int context;

  DBUG("PopUpMenu","called");
  menu = *pmenu;
  if (!menu || MR_WINDOW(menu) == None ||
      (MR_MAPPED_COPIES(menu) > 0 && MR_COPIES(menu) >= MAX_MENU_COPIES))
  {
    *pfWarpToTitle = False;
    return False;
  }

  /* First of all, execute the popup action (if defined). */
  if (MR_DYNAMIC(menu).popup_action)
  {
    char *menu_name;
    saved_pos_hints pos_hints;
    Bool f;
    Time t;
    extern XEvent Event;

    /* save variables that we still need but that may be overwritten */
    menu_name = strdup(MR_NAME(menu));
    pos_hints = last_saved_pos_hints;
    f = *pfWarpToTitle;
    t = lastTimestamp;
    /* need to ungrab the mouse during function call */
    UngrabEm();
    /* Execute the action */
    ExecuteFunctionSaveTmpWin(MR_DYNAMIC(menu).popup_action, *pfw, &Event,
			      *pcontext, -2, DONT_EXPAND_COMMAND);
    /* restore the stuff we saved */
    lastTimestamp = t;
    *pfWarpToTitle = f;
    last_saved_pos_hints = pos_hints;
    /* See if the window has been deleted */
    if (!check_if_fvwm_window_exists(*pfw))
    {
      *pfw = NULL;
      *pcontext = 0;
    }
    /* Now let's see if the menu still exists. It may have been destroyed and
     * recreated, so we have to look for a menu with the saved name. */
    *pmenu = FindPopup(menu_name);
    free(menu_name);
    menu = *pmenu;
    /* grab the mouse again */
    if(!GrabEm(MENU))
    {
      /* FIXME: If this happens in a submenu we're pissed. All kinds of
       * unpleasant things might happen. */
      free(menu_name);
      return False;
    }
    if (menu)
      MakeMenu(menu);
  }
  fw = *pfw;
  context = *pcontext;

  if(menu == NULL || MR_FIRST_ITEM(menu) == NULL || MR_ITEMS(menu) == 0)
  {
    /* The menu deleted itself or all its items or it has been empty from the
     * start. */
    *pfWarpToTitle = False;
    return False;
  }

  if (MR_MAPPED_COPIES(menu) > 0)
  {
    /* create a new instance of the menu */
    *pmenu = CopyMenuRoot(*pmenu);
    if (!*pmenu)
      return False;
    MakeMenuWindow(*pmenu);
    menu = *pmenu;
  }

  /* calculate position from positioning hints if available */
  if (pops->flags.has_poshints &&
      !last_saved_pos_hints.flags.do_ignore_pos_hints)
  {
    x = pops->pos_hints.x + pops->pos_hints.x_factor * MR_WIDTH(menu);
    y = pops->pos_hints.y + pops->pos_hints.y_factor * MR_HEIGHT(menu);
  }

  MR_PARENT_MENU(menu) = parent_menu;
  MR_PARENT_ITEM(menu) = (parent_menu) ? MR_SELECTED_ITEM(parent_menu) : NULL;
  MR_IS_PAINTED(menu) = 0;
  MR_IS_LEFT(menu) = 0;
  MR_IS_RIGHT(menu) = 0;
  MR_IS_UP(menu) = 0;
  MR_IS_DOWN(menu) = 0;
  MR_XANIMATION(menu) = 0;

  /*  RepaintAlreadyReversedMenuItems(menu); */

  InstallRootColormap();

  /* First handle popups from button clicks on buttons in the title bar,
     or the title bar itself. Position hints override this. */
  if (!(pops->flags.has_poshints))
  {
    if((fw)&&(parent_menu == NULL)&&(context&C_LALL))
      {
	y = fw->frame_g.y + fw->boundary_width +
	  fw->title_g.height + 1;
	x = fw->frame_g.x + fw->boundary_width +
	  ButtonPosition(context,fw)*fw->title_g.height + 1;
      }
    if((fw)&&(parent_menu == NULL)&&(context&C_RALL))
      {
	y = fw->frame_g.y + fw->boundary_width +
	  fw->title_g.height + 1;
	x = fw->frame_g.x + fw->frame_g.width -
	  fw->boundary_width - ButtonPosition(context,fw) *
	  fw->title_g.height-MR_WIDTH(menu)+1;
      }
    if((fw)&&(parent_menu == NULL)&&(context&C_TITLE))
      {
	y = fw->frame_g.y + fw->boundary_width
	  + fw->title_g.height + 1;
	if(x < fw->frame_g.x + fw->title_g.x)
	  x = fw->frame_g.x + fw->title_g.x;
	if((x + MR_WIDTH(menu)) >
	   (fw->frame_g.x + fw->title_g.x +fw->title_g.width))
	  x = fw->frame_g.x + fw->title_g.x +fw->title_g.width-
	    MR_WIDTH(menu) +1;
      }
  } /* if (pops->flags.has_poshints) */
  x_overlap = DoMenusOverlap(parent_menu, x, y, MR_WIDTH(menu), MR_HEIGHT(menu),
			     True);
  /* clip to screen */
  if (x + MR_WIDTH(menu) > Scr.MyDisplayWidth - 2)
    x = Scr.MyDisplayWidth - 2 - MR_WIDTH(menu);
  if (y + MR_HEIGHT(menu) > Scr.MyDisplayHeight)
    y = Scr.MyDisplayHeight - MR_HEIGHT(menu);
  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;

  if (parent_menu != NULL)
  {
    int prev_x, prev_y, left_x, right_x;
    unsigned int prev_width, prev_height;
    int x_offset;

    /* try to find a better place */
    XGetGeometry(dpy,MR_WINDOW(parent_menu),&JunkRoot,&prev_x,&prev_y,
		 &prev_width,&prev_height,&JunkBW,&JunkDepth);

    /* check if menus overlap */
    x_clipped_overlap = DoMenusOverlap(parent_menu, x, y, MR_WIDTH(menu),
				       MR_HEIGHT(menu), True);
    if (x_clipped_overlap &&
	(!(pops->flags.has_poshints) ||
	 pops->pos_hints.fRelative == False || x_overlap == 0))
    {
      /* menus do overlap, but do not reposition if overlap was caused by
	 relative positioning hints */
      Bool fDefaultLeft;
      Bool fEmergencyLeft;

      x_offset = prev_width * MR_STYLE(parent_menu)->feel.PopupOffsetPercent
	/ 100 + MR_STYLE(parent_menu)->feel.PopupOffsetAdd;
      left_x = prev_x - MR_WIDTH(menu) + 2;
      right_x = prev_x + x_offset;
      if (x_offset > prev_width - 2)
	right_x = prev_x + prev_width - 2;
      if (x + MR_WIDTH(menu) < prev_x + right_x)
	fDefaultLeft = True;
      else
	fDefaultLeft = False;
      fEmergencyLeft = (prev_x > Scr.MyDisplayWidth - right_x) ? True : False;

      if (MR_STYLE(menu)->feel.flags.is_animated)
      {
	/* animate previous out of the way */
	int left_x, right_x, end_x;

	left_x = x - x_offset;
	if (x_offset >= prev_width)
	  left_x = x - x_offset + 3;
	right_x = x + MR_WIDTH(menu);
	if (fDefaultLeft)
	{
	  /* popup menu is left of old menu, try to move prior menu right */
	  if (right_x + prev_width <=  Scr.MyDisplayWidth - 2)
	    end_x = right_x;
	  else if (left_x >= 0)
	    end_x = left_x;
	  else
	    end_x = Scr.MyDisplayWidth - 2 - prev_width;
	}
	else
	{
	  /* popup menu is right of old menu, try to move prior menu left */
	  if (left_x >= 0)
	    end_x = left_x;
	  else if (right_x + prev_width <=  Scr.MyDisplayWidth - 2)
	    end_x = right_x;
	  else
	    end_x = 0;
	}
	MR_XANIMATION(parent_menu) += end_x - prev_x;
	AnimatedMoveOfWindow(MR_WINDOW(parent_menu),prev_x,prev_y,end_x,prev_y,
			     True, -1, NULL);
      } /* if (MR_STYLE(menu)->feel.flags.is_animated) */
      else if (prev_x + x_offset > x + 3 && x_offset + 3 > -MR_WIDTH(menu) &&
	       !(pops->flags.fixed))
      {
	Bool fLeftIsOK = False;
	Bool fRightIsOK = False;
	Bool fUseLeft = False;

	if (left_x >= 0)
	  fLeftIsOK = True;
	if (right_x + MR_WIDTH(menu) < Scr.MyDisplayWidth - 2)
	  fRightIsOK = True;
	if (!fLeftIsOK && !fRightIsOK)
	  fUseLeft = fEmergencyLeft;
	else if (fLeftIsOK && (fDefaultLeft || !fRightIsOK))
	  fUseLeft = True;
	else
	  fUseLeft = False;
	x = (fUseLeft) ? left_x : right_x;
	/* force the menu onto the screen; prefer to have the left border
	 * visible if the menu is wider than the screen. But leave at least
	 * 20 pixels of the parent menu visible */

	if (x + MR_WIDTH(menu) >= Scr.MyDisplayWidth - 2)
	{
	  int d = x + MR_WIDTH(menu) - Scr.MyDisplayWidth + 3;
	  int c;

	  if (prev_width >= 20)
	    c = prev_x + 20;
	  else
	    c = prev_x + prev_width;

	  if (x - c >= d || x <= prev_x)
	    x -= d;
	  else if (x > c)
	    x = c;
	}
	if (x < 0)
	{
	  int c = prev_width - 20;

	  if (c < 0)
	    c = 0;
	  if (-x > c)
	    x += c;
	  else
	    x = 0;
	}
      } /* else if (non-overlapping menu style) */
    } /* if (x_clipped_overlap && ...) */

    if (x < prev_x)
      MR_IS_LEFT(menu) = 1;
    if (x + MR_WIDTH(menu) > prev_x + prev_width)
      MR_IS_RIGHT(menu) = 1;
    if (y < prev_y)
      MR_IS_UP(menu) = 1;
    if (y + MR_HEIGHT(menu) > prev_y + prev_height)
      MR_IS_DOWN(menu) = 1;
    if (!MR_IS_LEFT(menu) && !MR_IS_RIGHT(menu))
    {
      MR_IS_LEFT(menu) = 1;
      MR_IS_RIGHT(menu) = 1;
    }
  } /* if (parent_menu) */

  /* popup the menu */
  XMoveWindow(dpy, MR_WINDOW(menu), x, y);
  XMapRaised(dpy, MR_WINDOW(menu));
  XFlush(dpy);
  MR_MAPPED_COPIES(menu)++;
  if (ret_overlap)
  {
    *ret_overlap =
      DoMenusOverlap(parent_menu, x, y, MR_WIDTH(menu), MR_HEIGHT(menu), False)?
      True : False;
  }

  if (!fWarpToItem)
  {
    mi = FindEntry(NULL, &mrMi);
    if (mi && mrMi == menu && mi != MR_FIRST_ITEM(mrMi))
    {
      /* pointer is on an item of the popup */
      if (MR_STYLE(menu)->feel.flags.do_title_warp)
      {
	/* warp pointer if not on a root menu and MWM/WIN menu style */
	fWarpTitle = True;
      }
    }
  } /* if (!fWarpToItem) */

  if (pops->flags.no_warp)
  {
    fWarpTitle = False;
  }
  else if (pops->flags.warp_title)
  {
    fWarpTitle = True;
  }
  if (*pfWarpToTitle)
  {
    fWarpTitle = True;
    *pfWarpToTitle = False;
  }
  if (fWarpToItem)
  {
    /* also warp */
    DBUG("PopUpMenu","Warping to item");
    MR_SELECTED_ITEM(menu) =
      MiWarpPointerToItem(menu, MR_FIRST_ITEM(menu), True /* skip Title */);
    SelectMenuItem(menu, MR_SELECTED_ITEM(menu), True, fw);
  }
  else if(fWarpTitle)
  {
    /* Warp pointer to middle of top line, since we don't
     * want the user to come up directly on an option */
    DBUG("PopUpMenu","Warping to title");
    WarpPointerToTitle(menu);
  }
  return True;
}

/* Set the selected-ness state of the menuitem passed in */
static
void SelectMenuItem(MenuRoot *mr, MenuItem *mi, Bool select, FvwmWindow *fw)
{
  if (select == True && MR_SELECTED_ITEM(mr) != NULL &&
      MR_SELECTED_ITEM(mr) != mi)
    SelectMenuItem(mr, MR_SELECTED_ITEM(mr), False, fw);
  else if (select == False && MR_SELECTED_ITEM(mr) == NULL)
    return;
  else if (select == True && MR_SELECTED_ITEM(mr) == mi)
    return;

#ifdef GRADIENT_BUTTONS
  switch (MR_STYLE(mr)->look.face.type)
  {
  case HGradMenu:
  case VGradMenu:
  case DGradMenu:
  case BGradMenu:
    if (select == True)
    {
      int iy, ih;
      unsigned int mw, mh;

      if (!MR_IS_PAINTED(mr))
      {
	PaintMenu(mr, NULL, fw);
	flush_expose(MR_WINDOW(mr));
      }
      iy = mi->y_offset - 2;
      ih = mi->y_height + 4;
      if (iy < 0)
      {
	ih += iy;
	iy = 0;
      }
      XGetGeometry(dpy, MR_WINDOW(mr), &JunkRoot, &JunkX, &JunkY,
		   &mw, &mh, &JunkBW, &JunkDepth);
      if (iy + ih > mh)
	ih = mh - iy;
      /* grab image */
      MR_STORED_ITEM(mr).stored = XCreatePixmap(dpy, Scr.NoFocusWin, mw,
						    ih, Scr.depth);
      XCopyArea(dpy, MR_WINDOW(mr), MR_STORED_ITEM(mr).stored,
		MR_STYLE(mr)->look.MenuGC, 0, iy, mw, ih, 0, 0);
      MR_STORED_ITEM(mr).y = iy;
      MR_STORED_ITEM(mr).width = mw;
      MR_STORED_ITEM(mr).height = ih;
    }
    else if (select == False && MR_STORED_ITEM(mr).width != 0)
    {
      /* ungrab image */

	XCopyArea(dpy, MR_STORED_ITEM(mr).stored, MR_WINDOW(mr),
		  MR_STYLE(mr)->look.MenuGC, 0, 0,
		  MR_STORED_ITEM(mr).width, MR_STORED_ITEM(mr).height,
		  0, MR_STORED_ITEM(mr).y);

        XFreePixmap(dpy, MR_STORED_ITEM(mr).stored);
        MR_STORED_ITEM(mr).width = 0;
        MR_STORED_ITEM(mr).height = 0;
        MR_STORED_ITEM(mr).y = 0;

    }
    break;
  default:
    if (MR_STORED_ITEM(mr).width != 0)
    {
      XFreePixmap(dpy, MR_STORED_ITEM(mr).stored);
      MR_STORED_ITEM(mr).width = 0;
      MR_STORED_ITEM(mr).height = 0;
      MR_STORED_ITEM(mr).y = 0;
    }
    break;
  }
#endif

  MR_SELECTED_ITEM(mr) = (select) ? mi : NULL;
  paint_menu_item(mr, mi, fw);
}

/* FindPopup expects a token as the input. Make sure you have used
 * GetNextToken before passing a menu name to remove quotes (if necessary) */
MenuRoot *FindPopup(char *popup_name)
{
  MenuRoot *mr;

  if(popup_name == NULL)
    return NULL;

  mr = Menus.all;
  while(mr != NULL)
  {
    if(MR_NAME(mr) != NULL)
    {
      if(mr == MR_ORIGINAL_MENU(mr) &&
	 strcasecmp(popup_name, MR_NAME(mr)) == 0)
      {
        return mr;
      }
    }
    mr = MR_NEXT_MENU(mr);
  }
  return NULL;

}

/* Returns a menu root that a given menu item pops up */
static
MenuRoot *MrPopupForMi(MenuRoot *mr, MenuItem *mi)
{
  char *menu_name;
  MenuRoot *menu = NULL;

  /* This checks if mi is != NULL too */
  if (!mi || !IS_POPUP_ITEM(mi))
    return NULL;

  /* first look for a menu that is aleady mapped */
  menu = SeekSubmenuInstance(mr, mi);
  if (menu)
    return menu;

  /* just look past "Popup " in the action, and find that menu root */
  GetNextToken(SkipNTokens(mi->action, 1), &menu_name);
  menu = FindPopup(menu_name);
  if (menu_name != NULL)
    free(menu_name);
  return menu;
}

/* Returns the menu options for the menu that a given menu item pops up */
static
void GetPopupOptions(MenuRoot *mr, MenuItem *mi, MenuOptions *pops)
{
  if (!mi)
    return;
  pops->flags.has_poshints = 0;
  /* just look past "Popup <name>" in the action */
  GetMenuOptions(SkipNTokens(mi->action, 2), MR_WINDOW(mr), NULL, mr,
		 mi, pops);
}


/***********************************************************************
 *
 *  Procedure:
 *	PopDownMenu - unhighlight the current menu selection and
 *                    take down the menus
 *
 *      mr     - menu to pop down; this pointer is invalid after the function
 *               returns. Don't use it anymore!
 *      parent - the menu that has spawned mr (may be NULL). this is
 *               used to see if mr was spawned by itself on some level.
 *               this is a hack to allow specifying 'Popup foo' within
 *               menu foo. You must use the MenuRoot that is currently
 *               being processed here. DO NOT USE MR_PARENT_MENU(mr) here!
 *
 ***********************************************************************/
static
void PopDownMenu(MenuRoot **pmr, MenuParameters *pmp)
{
  MenuItem *mi;
  assert(*pmr);

  memset(&(MR_TEMP_FLAGS(*pmr)), 0, sizeof(MR_TEMP_FLAGS(*pmr)));
  XUnmapWindow(dpy, MR_WINDOW(*pmr));
  MR_MAPPED_COPIES(*pmr)--;

  UninstallRootColormap();
  XFlush(dpy);
  if (*(pmp->pcontext) & (C_WINDOW | C_FRAME | C_TITLE | C_SIDEBAR))
    pmp->flags.is_menu_from_frame_or_window_or_titlebar = True;
  else
    pmp->flags.is_menu_from_frame_or_window_or_titlebar = False;
  if ((mi = MR_SELECTED_ITEM(*pmr)) != NULL)
  {
    SelectMenuItem(*pmr, mi, False, (*pmp->pTmp_win));
  }

  /* DBUG("PopDownMenu","popped down %s",MR_NAME(*pmr)); */
  if (MR_COPIES(*pmr) > 1)
  {
    /* delete this instance of the menu */
    DestroyMenu(*pmr, False);
  }
  else if (MR_DYNAMIC(*pmr).popdown_action)
  {
    /* Finally execute the popdown action (if defined). */
    saved_pos_hints pos_hints;
    Time t;
    extern XEvent Event;

    /* save variables that we still need but that may be overwritten */
    pos_hints = last_saved_pos_hints;
    t = lastTimestamp;
    /* need to ungrab the mouse during function call */
    UngrabEm();
    /* Execute the action */
    ExecuteFunctionSaveTmpWin(MR_DYNAMIC(*pmr).popdown_action,
			      (*pmp->pTmp_win), &Event, *(pmp->pcontext), -2,
			      DONT_EXPAND_COMMAND);
    /* restore the stuff we saved */
    last_saved_pos_hints = pos_hints;
    lastTimestamp = t;
    /* grab the mouse again */
    GrabEm(MENU);
    /* FIXME: If the grab fails in a submenu we're pissed. All kinds of
     * unpleasant things might happen. */
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
 *      PopDownAndRepaintParent - Pops down a menu and repaints the
 *      overlapped portions of the parent menu. This is done only if
 *      *fSubmenuOverlaps is True. *fSubmenuOverlaps is set to False
 *      afterwards.
 *
 ***********************************************************************/
static void PopDownAndRepaintParent(MenuRoot **pmr, Bool *fSubmenuOverlaps,
				    MenuParameters *pmp)
{
  MenuRoot *parent = MR_PARENT_MENU(*pmr);
  XEvent event;
  int mr_y;
  unsigned int mr_height;
  int parent_y;

  if (*fSubmenuOverlaps && parent)
  {
    XGetGeometry(dpy, MR_WINDOW(*pmr), &JunkRoot, &JunkX, &mr_y,
		 &JunkWidth, &mr_height, &JunkBW, &JunkDepth);
    XGetGeometry(dpy, MR_WINDOW(parent), &JunkRoot, &JunkX, &parent_y,
		 &JunkWidth, &JunkWidth, &JunkBW, &JunkDepth);
    PopDownMenu(pmr, pmp);
    /* Create a fake event to pass into PaintMenu */
    event.type = Expose;
    event.xexpose.y = mr_y - parent_y;
    event.xexpose.height = mr_height;
    PaintMenu(parent, &event, (*pmp->pTmp_win));
    flush_expose(MR_WINDOW(parent));
  }
  else
  {
    PopDownMenu(pmr, pmp);
  }
  *fSubmenuOverlaps = False;
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	RelieveHalfRectangle - add relief lines to the sides only of a
 *      rectangular window
 *
 ***********************************************************************/
static
void RelieveHalfRectangle(Window win,int x,int y,int w,int h,
			  GC Hilite,GC Shadow)
{
  XDrawLine(dpy, win, Hilite, x, y-1, x, h+y);
  XDrawLine(dpy, win, Hilite, x+1, y, x+1, h+y-1);

  XDrawLine(dpy, win, Shadow, w+x-1, y-1, w+x-1, h+y);
  XDrawLine(dpy, win, Shadow, w+x-2, y, w+x-2, h+y-1);
}


/***********************************************************************
 *
 *  Procedure:
 *      paint_menu_item - draws a single entry in a popped up menu
 *
 *      mr - the menu instance that holds the menu item
 *      mi - the menu item to redraw
 *      fw - the FvwmWindow structure to check against allowed functions
 *
 ***********************************************************************/
static
void paint_menu_item(MenuRoot *mr, MenuItem *mi, FvwmWindow *fw)
{
  int y_offset,text_y,d, y_height,y,x;
  GC ShadowGC, ReliefGC, currentGC;
  char th = MR_STYLE(mr)->look.ReliefThickness;
  Bool fClear = False;
#ifdef GRADIENT_BUTTONS
  Bool fGradient;
  Bool fSelected;
  int sw = 0;

  if (!mi)
    return;

  fSelected = (mi == MR_SELECTED_ITEM(mr)) ? True : False;

  if (MR_SIDEPIC(mr) != NULL)
    sw = MR_SIDEPIC(mr)->width + 5;
  else if (MR_STYLE(mr)->look.sidePic != NULL)
    sw = MR_STYLE(mr)->look.sidePic->width + 5;

  switch (MR_STYLE(mr)->look.face.type)
  {
  case HGradMenu:
  case VGradMenu:
  case DGradMenu:
  case BGradMenu:
    fGradient = True;
    break;
  default:
    fGradient = False;
    break;
  }
#endif

  y_offset = mi->y_offset;
  y_height = mi->y_height;
  text_y = y_offset + MR_STYLE(mr)->look.pStdFont->y;
  /* center text vertically if the pixmap is taller */
  if(mi->picture)
    text_y+=mi->picture->height;
  if (mi->lpicture)
  {
    y = mi->lpicture->height - MR_STYLE(mr)->look.pStdFont->height;
    if (y>1)
      text_y += y/2;
  }

  ShadowGC = MR_STYLE(mr)->look.MenuShadowGC;
  if(Scr.depth<2)
    ReliefGC = MR_STYLE(mr)->look.MenuShadowGC;
  else
    ReliefGC = MR_STYLE(mr)->look.MenuReliefGC;

  /* Hilight background */
  if (MR_STYLE(mr)->look.flags.do_hilight)
  {
    if (fSelected && !IS_SEPARATOR_ITEM(mi) &&
	(((*mi->item)!=0) || mi->picture || mi->lpicture))
    {
      XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
      XFillRectangle(dpy, MR_WINDOW(mr), MR_STYLE(mr)->look.MenuActiveBackGC,
		     MR_XOFFSET(mr)+3, y_offset,
		     MR_WIDTH(mr) - MR_XOFFSET(mr)-6, y_height);
    }
    else if (th == 0)
    {
#ifdef GRADIENT_BUTTONS
      if (!fGradient)
#endif
	XClearArea(dpy,MR_WINDOW(mr),MR_XOFFSET(mr)+3,y_offset,
		   MR_WIDTH(mr) - MR_XOFFSET(mr)-6, y_height,0);
    }
    else
    {
      fClear = True;
    }
    if (th == 0)
    {
      RelieveHalfRectangle(MR_WINDOW(mr), 0, y_offset-1, MR_WIDTH(mr),
			   y_height+3, ReliefGC, ShadowGC);
    }
  }

  if (MR_STYLE(mr)->look.ReliefThickness > 0 &&
      (fClear || !MR_STYLE(mr)->look.flags.do_hilight))
  {
    /* background was already painted above? */
#ifdef GRADIENT_BUTTONS
    if (!fGradient)
#endif
    {

      if (th == 2 && mi->prev && mi->prev == MR_SELECTED_ITEM(mr))
	XClearArea(dpy, MR_WINDOW(mr),MR_XOFFSET(mr),y_offset+1,
		   MR_WIDTH(mr),y_height-1,0);
      else
	XClearArea(dpy, MR_WINDOW(mr), MR_XOFFSET(mr), y_offset - th + 1,
		   MR_WIDTH(mr), y_height + 2*(th - 1), 0);
    }
  }



  /* Hilight 3D */
  if (MR_STYLE(mr)->look.ReliefThickness > 0)
  {
    if ((fSelected)&&(!IS_SEPARATOR_ITEM(mi))&&
	(((*mi->item)!=0 || mi->strlen2) || mi->picture || mi->lpicture))
    {
      RelieveRectangle(dpy,MR_WINDOW(mr), MR_XOFFSET(mr) + th + 1, y_offset,
		       MR_WIDTH(mr) - 2*(th + 1) - sw - 1, mi->y_height - 1,
		       ReliefGC,ShadowGC,1);
      if (th == 2)
      {
	RelieveRectangle(dpy,MR_WINDOW(mr), MR_XOFFSET(mr) + 2, y_offset - 1,
			 MR_WIDTH(mr) - 4 - sw - 1, mi->y_height + 2 - 1,
			 ReliefGC,ShadowGC,1);
      }
    }

    RelieveHalfRectangle(MR_WINDOW(mr), 0, y_offset - th + 1, MR_WIDTH(mr),
			 y_height + 2*th - 1, ReliefGC, ShadowGC);
  }


  /* Draw the shadows for the absolute outside of the menus
     This stuff belongs in here, not in PaintMenu, since we only
     want to redraw it when we have too (i.e. on expose event) */

  /* Top of the menu */
  if(mi == MR_FIRST_ITEM(mr))
    DrawSeparator(MR_WINDOW(mr),ReliefGC,ReliefGC,0,0, MR_WIDTH(mr)-1,0,-1);

  /* Botton of the menu */
  if(mi->next == NULL)
    DrawSeparator(MR_WINDOW(mr),ShadowGC,ShadowGC,1,MR_HEIGHT(mr)-2,
                  MR_WIDTH(mr)-1, MR_HEIGHT(mr)-2,1);

  if(IS_TITLE_ITEM(mi))
  {
    if(MR_STYLE(mr)->look.TitleUnderlines == 2)
    {
      text_y += HEIGHT_EXTRA/2;
      XDrawLine(dpy, MR_WINDOW(mr), ShadowGC, MR_XOFFSET(mr)+2,
		y_offset+y_height-2, MR_WIDTH(mr)-3, y_offset+y_height-2);
      XDrawLine(dpy, MR_WINDOW(mr), ShadowGC, MR_XOFFSET(mr)+2,
		y_offset+y_height-4, MR_WIDTH(mr)-3, y_offset+y_height-4);
    }
    else if(MR_STYLE(mr)->look.TitleUnderlines == 1)
    {
      if(mi->next != NULL)
      {
	DrawSeparator(MR_WINDOW(mr),ShadowGC,ReliefGC,MR_XOFFSET(mr)+5,
		      y_offset+y_height-3,
		      MR_WIDTH(mr)-6, y_offset+y_height-3,1);
      }
      if(mi != MR_FIRST_ITEM(mr))
      {
	text_y += HEIGHT_EXTRA_TITLE/2;
	DrawSeparator(MR_WINDOW(mr),ShadowGC,ReliefGC,MR_XOFFSET(mr)+5,
		      y_offset+1, MR_WIDTH(mr)-6, y_offset+1,1);
      }
    }
  }
  else
    text_y += HEIGHT_EXTRA/2;

  /* see if it's an actual separator (titles are also separators) */
  if (IS_SEPARATOR_ITEM(mi) && !IS_TITLE_ITEM(mi))
  {
      int d = (MR_STYLE(mr)->look.flags.has_long_separators) ? 4 : 6;

      DrawSeparator(MR_WINDOW(mr),ShadowGC,ReliefGC,MR_XOFFSET(mr)+5-d,
		    y_offset-1+HEIGHT_SEPARATOR/2,
		    MR_WIDTH(mr)-d,y_offset-1+HEIGHT_SEPARATOR/2,1);
  }
  if(mi->next == NULL)
    DrawSeparator(MR_WINDOW(mr),ShadowGC,ShadowGC,MR_XOFFSET(mr)+1,
		  MR_HEIGHT(mr)-2, MR_WIDTH(mr)-2, MR_HEIGHT(mr)-2,1);
  if(mi == MR_FIRST_ITEM(mr))
    DrawSeparator(MR_WINDOW(mr),ReliefGC,ReliefGC,MR_XOFFSET(mr),0,
		  MR_WIDTH(mr)-1,0,-1);

  if(check_if_function_allowed(mi->func_type,fw,False,mi->item))
  {
    if(fSelected && !IS_TITLE_ITEM(mi))
      currentGC = MR_STYLE(mr)->look.MenuActiveGC;
    else
      currentGC = MR_STYLE(mr)->look.MenuGC;
    if (MR_STYLE(mr)->look.flags.do_hilight &&
	!MR_STYLE(mr)->look.flags.has_active_fore &&
	fSelected && IS_SEPARATOR_ITEM(mi) == False)
      /* Use a lighter color for highlighted windows menu items for win mode */
      currentGC = MR_STYLE(mr)->look.MenuReliefGC;
  }
  else
    /* should be a shaded out word, not just re-colored. */
    currentGC = MR_STYLE(mr)->look.MenuStippleGC;

  if(*mi->item)
    XDrawString(dpy, MR_WINDOW(mr),currentGC,mi->x+MR_XOFFSET(mr),text_y,
		mi->item, mi->strlen);
  if(mi->strlen2>0)
    XDrawString(dpy, MR_WINDOW(mr),currentGC,
		mi->x2 + MR_WIDTH(mr) + MR_XOFFSET(mr) - MR_WIDTH3(mr) - 5 -
		sw, text_y, mi->item2, mi->strlen2);

  /* pete@tecc.co.uk: If the item has a hot key, underline it */
  if (mi->hotkey > 0)
    DrawUnderline(mr, currentGC,MR_XOFFSET(mr)+mi->x,text_y,mi->item,
		  mi->hotkey - 1);
  if (mi->hotkey < 0)
   DrawUnderline(mr, currentGC,
		 MR_XOFFSET(mr) + MR_WIDTH(mr) + mi->x2 - MR_WIDTH3(mr) - 5-sw,
		 text_y, mi->item2, -1 - mi->hotkey);

  d=(MR_STYLE(mr)->look.EntryHeight-7)/2;
  if(IS_POPUP_ITEM(mi))
  {
    if(fSelected)
      DrawTrianglePattern(MR_WINDOW(mr), ShadowGC, ReliefGC, ShadowGC,ReliefGC,
			  MR_WIDTH(mr)-13, y_offset+d-1, MR_WIDTH(mr)-7,
			  y_offset+d+7,
			  MR_STYLE(mr)->look.flags.has_triangle_relief);
    else
      DrawTrianglePattern(MR_WINDOW(mr), ReliefGC, ShadowGC, ReliefGC,
			  MR_STYLE(mr)->look.MenuGC,
			  MR_WIDTH(mr)-13, y_offset+d-1, MR_WIDTH(mr)-7,
			  y_offset+d+7,
			  MR_STYLE(mr)->look.flags.has_triangle_relief);
  }

 if(mi->picture)
    {
      x = (MR_WIDTH(mr) - mi->picture->width)/2;
      if(mi->lpicture && x < MR_WIDTH0(mr) + 5)
	x = MR_WIDTH0(mr)+5;

      if(mi->picture->depth > 0) /* pixmap? */
	{
	  Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
	  Globalgcv.clip_mask = mi->picture->mask;
	  Globalgcv.clip_x_origin= x;
	  Globalgcv.clip_y_origin = y_offset+1;
	  XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
	  XCopyArea(dpy,mi->picture->picture,MR_WINDOW(mr),ReliefGC, 0, 0,
		    mi->picture->width, mi->picture->height,
		    x,y_offset+1);
	  Globalgcm = GCClipMask;
	  Globalgcv.clip_mask = None;
	  XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
	}
      else
	{
	  XCopyPlane(dpy,mi->picture->picture,MR_WINDOW(mr),
		     currentGC,0,0,mi->picture->width,mi->picture->height,
		     x,y_offset+1,1);
	}
    }

  if(mi->lpicture)
    {
      int lp_offset = 6;
      if(mi->picture && *mi->item != 0)
	y = y_offset + mi->y_height - mi->lpicture->height-1;
      else
	y = y_offset + mi->y_height/2 - mi->lpicture->height/2;
      if(mi->lpicture->depth > 0) /* pixmap? */
	{
	  Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
	  Globalgcv.clip_mask = mi->lpicture->mask;
	  Globalgcv.clip_x_origin= lp_offset + MR_XOFFSET(mr);
	  Globalgcv.clip_y_origin = y;

	  XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
	  XCopyArea(dpy,mi->lpicture->picture,MR_WINDOW(mr),ReliefGC,0,0,
		    mi->lpicture->width, mi->lpicture->height,
		    lp_offset + MR_XOFFSET(mr),y);
	  Globalgcm = GCClipMask;
	  Globalgcv.clip_mask = None;
	  XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
	}
      else
	{
	  XCopyPlane(dpy,mi->lpicture->picture,MR_WINDOW(mr),
		     currentGC,0,0,mi->lpicture->width,mi->lpicture->height,
		     lp_offset + MR_XOFFSET(mr),y,1);
	}
    }
  return;
}

/************************************************************
 *
 * Draws a picture on the left side of the menu
 *
 ************************************************************/

static void PaintSidePic(MenuRoot *mr)
{
  GC ReliefGC, TextGC;
  Picture *sidePic;

  if (MR_SIDEPIC(mr))
    sidePic = MR_SIDEPIC(mr);
  else if (MR_STYLE(mr)->look.sidePic)
    sidePic = MR_STYLE(mr)->look.sidePic;
  else
    return;

  if(Scr.depth<2)
    ReliefGC = MR_STYLE(mr)->look.MenuShadowGC;
  else
    ReliefGC = MR_STYLE(mr)->look.MenuReliefGC;
  TextGC = MR_STYLE(mr)->look.MenuGC;

  if(MR_HAS_SIDECOLOR(mr))
    Globalgcv.foreground = MR_SIDECOLOR(mr);
  else if (MR_STYLE(mr)->look.flags.has_side_color)
    Globalgcv.foreground = MR_STYLE(mr)->look.sideColor;
  if (MR_HAS_SIDECOLOR(mr) || MR_STYLE(mr)->look.flags.has_side_color)
  {
    Globalgcm = GCForeground;
    XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
    XFillRectangle(dpy, MR_WINDOW(mr), Scr.ScratchGC1, 3, 3,
                   sidePic->width, MR_HEIGHT(mr) - 6);
  }

  if(sidePic->depth > 0) /* pixmap? */
  {
    Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
    Globalgcv.clip_mask = sidePic->mask;
    Globalgcv.clip_x_origin = 3;
    Globalgcv.clip_y_origin = MR_HEIGHT(mr) - sidePic->height -3;

    XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
    XCopyArea(dpy, sidePic->picture, MR_WINDOW(mr),
	      ReliefGC, 0, 0,
	      sidePic->width, sidePic->height,
	      Globalgcv.clip_x_origin, Globalgcv.clip_y_origin);
    Globalgcm = GCClipMask;
    Globalgcv.clip_mask = None;
    XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
  }
  else
  {
    XCopyPlane(dpy, sidePic->picture, MR_WINDOW(mr),
	       TextGC, 0, 0,
	       sidePic->width, sidePic->height,
	       1, MR_HEIGHT(mr) - sidePic->height, 1);
  }
}



/****************************************************************************
 * Procedure:
 *	DrawUnderline() - Underline a character in a string (pete@tecc.co.uk)
 *
 * Calculate the pixel offsets to the start of the character position we
 * want to underline and to the next character in the string.  Shrink by
 * one pixel from each end and the draw a line that long two pixels below
 * the character...
 *
 ****************************************************************************/
static
void  DrawUnderline(MenuRoot *mr, GC gc, int x, int y, char *txt, int posn)
{
  int off1 = XTextWidth(MR_STYLE(mr)->look.pStdFont->font, txt, posn);
  int off2 = XTextWidth(MR_STYLE(mr)->look.pStdFont->font, txt, posn + 1) - 1;
  XDrawLine(dpy, MR_WINDOW(mr), gc, x + off1, y + 2, x + off2, y + 2);
}
/****************************************************************************
 *
 *  Draws two horizontal lines to form a separator
 *
 ****************************************************************************/
static
void DrawSeparator(Window w, GC TopGC, GC BottomGC,int x1,int y1,int x2,int y2,
		   int extra_off)
{
  XDrawLine(dpy, w, TopGC   , x1,           y1,  x2,          y2);
  XDrawLine(dpy, w, BottomGC, x1-extra_off, y1+1,x2+extra_off,y2+1);
}

/****************************************************************************
 *
 *  Draws a little Triangle pattern within a window
 *
 ****************************************************************************/
static
void DrawTrianglePattern(Window w,GC GC1,GC GC2,GC GC3,GC gc,int l,int u,
			 int r,int b, char relief)
{
  int m;

  m = (u + b)/2;

  /**************************************************************************
   *
   * anti-ugliness patch by Adam Rice, wysiwyg@glympton.airtime.co.uk,
   * January 28th 1998.
   *
   **************************************************************************/

  /* ensure vertical symmetry */
  if (u-m > m-b)
  {
    u=2*m-b;
  }
  else if (u-m < m-b)
  {
    b=2*m-u;
  }

  /* make (r-l)/(m-b) or its inverse be a whole number */
  if (r-l > m-b)
  {
    r=((r-l)/(m-b))*(m-b)+l;
  }
  else if (r-l < m-b)
  {
    r=(m-b)/((((m-b)-1)/(r-l))+1)+l;
  }

  if (!relief)
  {
    /* solid triangle */
    XPoint points[3];
    points[0].x = l; points[0].y = u;
    points[1].x = l; points[1].y = b;
    points[2].x = r; points[2].y = m;
    XFillPolygon(dpy, w, gc, points, 3, Convex, CoordModeOrigin);
  }
  else
  {
    /* relief triangle */
    XDrawLine(dpy,w,GC1,l,u,l,b);
    XDrawLine(dpy,w,GC2,l,b,r,m);
    XDrawLine(dpy,w,GC3,r,m,l,u);
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	PaintMenu - draws the entire menu
 *
 ***********************************************************************/
void PaintMenu(MenuRoot *mr, XEvent *pevent, FvwmWindow *fw)
{
  MenuItem *mi;
  MenuStyle *ms = MR_STYLE(mr);
  register int type;
  XRectangle bounds;

#ifdef PIXMAP_BUTTONS
  Picture *p;
  int border = 0;
  int width, height, x, y;
#endif

#ifdef GRADIENT_BUTTONS
  Pixmap pmap;
  GC	 pmapgc;
  XGCValues gcv;
  unsigned long gcm = 0;
  gcv.line_width=3;
  gcm = GCLineWidth;
#endif

  MR_IS_PAINTED(mr) = 1;
  if( ms )
    {
      type = ms->look.face.type;
      switch(type)
      {
      case SolidMenu:
	XSetWindowBackground(dpy, MR_WINDOW(mr),
			     MR_STYLE(mr)->look.face.u.back);
	flush_expose(MR_WINDOW(mr));
	XClearWindow(dpy,MR_WINDOW(mr));
	break;
#ifdef GRADIENT_BUTTONS
      case HGradMenu:
      case VGradMenu:
      case DGradMenu:
      case BGradMenu:
        bounds.x = 2; bounds.y = 2;
        bounds.width = MR_WIDTH(mr) - 5;
        bounds.height = MR_HEIGHT(mr);

        if (type == HGradMenu)
	{
	  if (MR_IS_BACKGROUND_SET(mr) == False)
	  {
	  register int i = 0;
	  register int dw;

             pmap = XCreatePixmap(dpy, MR_WINDOW(mr), MR_WIDTH(mr), 5,
				  Scr.depth);
	     pmapgc = XCreateGC(dpy, pmap, gcm, &gcv);

	     bounds.width = MR_WIDTH(mr);
	  dw= (float) bounds.width / ms->look.face.u.grad.npixels + 1;
	  while (i < ms->look.face.u.grad.npixels)
          {
	    unsigned short x = i * bounds.width / ms->look.face.u.grad.npixels;
	       XSetForeground(dpy, pmapgc,
			   ms->look.face.u.grad.pixels[i++ ]);
	       XFillRectangle(dpy, pmap, pmapgc,
			      x, 0,
			      dw, 5);
	     }
	     XSetWindowBackgroundPixmap(dpy, MR_WINDOW(mr), pmap);
	     XFreeGC(dpy,pmapgc);
	     XFreePixmap(dpy,pmap);
	     MR_IS_BACKGROUND_SET(mr) = True;
	  }
	  XClearWindow(dpy, MR_WINDOW(mr));
        }
        else if (type == VGradMenu)
        {
	  if (MR_IS_BACKGROUND_SET(mr) == False)
	  {
	  register int i = 0;
	  register int dh = bounds.height / ms->look.face.u.grad.npixels + 1;

             pmap = XCreatePixmap(dpy, MR_WINDOW(mr), 5, MR_HEIGHT(mr),
				  Scr.depth);
	     pmapgc = XCreateGC(dpy, pmap, gcm, &gcv);

	  while (i < ms->look.face.u.grad.npixels)
          {
	    unsigned short y = i*bounds.height / ms->look.face.u.grad.npixels;
	       XSetForeground(dpy, pmapgc,
			   ms->look.face.u.grad.pixels[i++]);
	       XFillRectangle(dpy, pmap, pmapgc,
			      0, y,
			      5, dh);
	     }
	     XSetWindowBackgroundPixmap(dpy, MR_WINDOW(mr), pmap);
	     XFreeGC(dpy,pmapgc);
	     XFreePixmap(dpy,pmap);
	     MR_IS_BACKGROUND_SET(mr) = True;
	  }
	  XClearWindow(dpy, MR_WINDOW(mr));
        }
        else /* D or BGradient */
        {
	  register int i = 0, numLines;
	  int cindex = -1;

	  XSetClipMask(dpy, Scr.TransMaskGC, None);
          numLines = MR_WIDTH(mr) + MR_HEIGHT(mr) - 1;
          for(i = 0; i < numLines; i++)
          {
            if((int)(i * ms->look.face.u.grad.npixels / numLines) > cindex)
            {
              /* pick the next colour (skip if necc.) */
              cindex = i * ms->look.face.u.grad.npixels / numLines;
              XSetForeground(dpy, Scr.TransMaskGC,
			     ms->look.face.u.grad.pixels[cindex]);
            }
            if (type == DGradMenu)
              XDrawLine(dpy, MR_WINDOW(mr), Scr.TransMaskGC,
	                0, i, i, 0);
	    else /* BGradient */
	      XDrawLine(dpy, MR_WINDOW(mr), Scr.TransMaskGC,
	                0, MR_HEIGHT(mr) - 1 - i, i, MR_HEIGHT(mr) - 1);
	  }
        }
        break;
#endif  /* GRADIENT_BUTTONS */
#ifdef PIXMAP_BUTTONS
    case PixmapMenu:
      p = ms->look.face.u.p;

      border = 0;
      width = MR_WIDTH(mr) - border * 2; height = MR_HEIGHT(mr) - border * 2;

#if 0
      /* these flags are never set at the moment */
      x = border;
      if (ms->look.FaceStyle & HOffCenter)
      {
	if (ms->look.FaceStyle & HRight)
	  x += (int)(width - p->width);
      }
      else
	x += (int)(width - p->width) / 2;

      y = border;
      if (ms->look.FaceStyle & VOffCenter)
      {
	if (ms->look.FaceStyle & VBottom)
	  y += (int)(height - p->height);
      }
      else...
#else
      y = border + (int)(height - p->height) / 2;
      x = border + (int)(width - p->width) / 2;
#endif

      if (x < border)
	x = border;
      if (y < border)
	y = border;
      if (width > p->width)
	width = p->width;
      if (height > p->height)
	height = p->height;
      if (width > MR_WIDTH(mr) - x - border)
	width = MR_WIDTH(mr) - x - border;
      if (height > MR_HEIGHT(mr) - y - border)
	height = MR_HEIGHT(mr) - y - border;

      XSetClipMask(dpy, Scr.TransMaskGC, p->mask);
      XSetClipOrigin(dpy, Scr.TransMaskGC, x, y);
      XCopyArea(dpy, p->picture, MR_WINDOW(mr), Scr.TransMaskGC,
		0, 0, width, height, x, y);
      break;

   case TiledPixmapMenu:
     XSetWindowBackgroundPixmap(dpy, MR_WINDOW(mr), ms->look.face.u.p->picture);
     flush_expose(MR_WINDOW(mr));
     XClearWindow(dpy,MR_WINDOW(mr));
     break;
#endif /* PIXMAP_BUTTONS */
    }
  }

  for (mi = MR_FIRST_ITEM(mr); mi != NULL; mi = mi->next)
  {
    /* be smart about handling the expose, redraw only the entries
     * that we need to */
    if( (MR_STYLE(mr)->look.face.type != SolidMenu &&
	 MR_STYLE(mr)->look.face.type != SimpleMenu) || pevent == NULL ||
	(pevent->xexpose.y < (mi->y_offset + mi->y_height) &&
	 (pevent->xexpose.y + pevent->xexpose.height) > mi->y_offset))
    {
      paint_menu_item(mr, mi, fw);
    }
  }

  PaintSidePic(mr);
  XSync(dpy, 0);
  return;
}


static void FreeMenuItem(MenuItem *mi)
{
  if (!mi)
    return;
  if (mi->item != NULL)
    free(mi->item);
  if (mi->item2 != NULL)
    free(mi->item2);
  if (mi->action != NULL)
    free(mi->action);
  if(mi->picture)
    DestroyPicture(dpy,mi->picture);
  if(mi->lpicture)
    DestroyPicture(dpy,mi->lpicture);
  free(mi);
}

void DestroyMenu(MenuRoot *mr, Bool recreate)
{
  MenuItem *mi,*tmp2;
  MenuRoot *tmp, *prev;

  if(mr == NULL)
    return;

  /* seek menu in master list */
  tmp = Menus.all;
  prev = NULL;
  while((tmp != NULL)&&(tmp != mr))
  {
    prev = tmp;
    tmp = MR_NEXT_MENU(tmp);
  }
  if(tmp != mr)
    /* no such menu */
    return;

  if (MR_MAPPED_COPIES(mr) > 0 && MR_COPIES(mr) == 1)
  {
    /* can't destroy a menu while in use */
    fvwm_msg(ERR,"DestroyMenu", "Menu %s is in use", MR_NAME(mr));
    return;
  }

  if (MR_COPIES(mr) > 1 && MR_ORIGINAL_MENU(mr) == mr)
  {
    MenuRootDynamic *temp_mrd;
    MenuRoot *temp_mr;

    tmp = Menus.all;
    prev = NULL;
    while (tmp && MR_ORIGINAL_MENU(tmp) != mr)
    {
      prev = tmp;
      tmp = MR_NEXT_MENU(tmp);
    }
    /* build one new valid original menu structure from both menus */
    temp_mrd = mr->d;
    mr->d = tmp->d;
    tmp->d = temp_mrd;
    temp_mr = mr;
    mr = tmp;
    tmp = temp_mr;
    /* update the context to point to the new MenuRoot */
    XDeleteContext(dpy, MR_WINDOW(tmp), MenuContext);
    XSaveContext(dpy, MR_WINDOW(tmp), MenuContext, (caddr_t)tmp);
    /* now dump the parts of mr we don't need any longer */
  }

#ifdef GRADIENT_BUTTONS
      if (MR_STORED_ITEM(mr).stored)
	XFreePixmap(dpy, MR_STORED_ITEM(mr).stored);
#endif

  MR_COPIES(mr)--;
  if (MR_COPIES(mr) > 0)
  {
    recreate = False;
    /* recursively destroy the menu continuations */
    if (MR_CONTINUATION_MENU(mr))
      DestroyMenu(MR_CONTINUATION_MENU(mr), False);
  }
  else
  {
    /* free all items */
    mi = MR_FIRST_ITEM(mr);
    while(mi != NULL)
    {
      tmp2 = mi->next;
      FreeMenuItem(mi);
      mi = tmp2;
    }
    if (recreate)
    {
      /* just dump the menu items but keep the menu itself */
      MR_FIRST_ITEM(mr) = NULL;
      MR_LAST_ITEM(mr) = NULL;
      MR_SELECTED_ITEM(mr) = NULL;
      MR_CONTINUATION_MENU(mr) = NULL;
      MR_PARENT_MENU(mr) = NULL;
      MR_ITEMS(mr) = 0;
#ifdef GRADIENT_BUTTONS
      memset(&(MR_STORED_ITEM(mr)), 0 , sizeof(MR_STORED_ITEM(mr)));
#endif
      MakeMenu(mr);
      return;
    }
  }

  /* unlink menu from list */
  if(prev == NULL)
    Menus.all = MR_NEXT_MENU(mr);
  else
    MR_NEXT_MENU(prev) = MR_NEXT_MENU(mr);
  XDestroyWindow(dpy,MR_WINDOW(mr));
  XDeleteContext(dpy, MR_WINDOW(mr), MenuContext);

  if (MR_COPIES(mr) == 0)
  {
    if (MR_DYNAMIC(mr).popup_action)
      free(MR_DYNAMIC(mr).popup_action);
    if (MR_DYNAMIC(mr).popdown_action)
      free(MR_DYNAMIC(mr).popdown_action);
    free(MR_NAME(mr));
    if (MR_SIDEPIC(mr))
      DestroyPicture(dpy, MR_SIDEPIC(mr));
    free(mr->s);
  }

  free(mr->d);
  free(mr);
}

void destroy_menu(F_CMD_ARGS)
{
  MenuRoot *mr;
  MenuRoot *mrContinuation;
  Bool recreate = False;

  char *token;

  token = PeekToken(action, &action);
  if (!token)
    return;
  if (StrEquals(token, "recreate"))
  {
    recreate = True;
    token = PeekToken(action, NULL);
  }
  mr = FindPopup(token);
  if (Scr.last_added_item.type == ADDED_MENU)
    set_last_added_item(ADDED_NONE, NULL);
  while (mr)
  {
    /* save continuation before destroy */
    mrContinuation = MR_CONTINUATION_MENU(mr);
    DestroyMenu(mr, recreate);
    /* Don't recreate the continuations */
    recreate = False;
    mr = mrContinuation;
  }
  return;
}


/****************************************************************************
 *
 * Generates the windows for all menus
 *
 ****************************************************************************/
void MakeMenus(void)
{
  MenuRoot *mr;

  mr = Menus.all;
  while(mr != NULL)
  {
    MakeMenu(mr);
    mr = MR_NEXT_MENU(mr);
  }
}

/****************************************************************************
 *
 * Generates the window for a menu
 *
 ****************************************************************************/
void MakeMenu(MenuRoot *mr)
{
  MenuItem *cur;
  MenuItem *cur_prev;
  int y,width,title_width;
  int cItems;

  if(!Scr.flags.windows_captured)
    return;

  /* merge menu continuations into one menu again - needed when changing the
   * font size of a long menu. */
  while (MR_CONTINUATION_MENU(mr) != NULL)
  {
    MenuRoot *cont = MR_CONTINUATION_MENU(mr);

    if (MR_FIRST_ITEM(mr) == MR_LAST_ITEM(mr))
    {
      fvwm_msg(ERR, "MakeMenu", "BUG: Menu contains only continuation");
      break;
    }
    /* link first item of continuation to item before 'more...' */
    MR_LAST_ITEM(mr)->prev->next = MR_FIRST_ITEM(cont);
    MR_FIRST_ITEM(cont)->prev = MR_LAST_ITEM(mr)->prev;
    FreeMenuItem(MR_LAST_ITEM(mr));
    MR_LAST_ITEM(mr) = MR_LAST_ITEM(cont);
    MR_CONTINUATION_MENU(mr) = MR_CONTINUATION_MENU(cont);
    /* fake an empty menu so that DestroyMenu does not destroy the items. */
    MR_FIRST_ITEM(cont) = NULL;
    DestroyMenu(cont, False);
  }

  title_width = 0;
  MR_WIDTH0(mr) = 0;
  MR_WIDTH(mr) = 0;
  MR_WIDTH2(mr) = 0;
  MR_WIDTH3(mr) = 0;
  for (cur = MR_FIRST_ITEM(mr); cur != NULL; cur = cur->next)
  {
    int tw = 0;
    if(IS_POPUP_ITEM(cur))
      MR_WIDTH3(mr) = 15;

    width = XTextWidth(MR_STYLE(mr)->look.pStdFont->font, cur->item,
		       cur->strlen);
    if(cur->picture && width < cur->picture->width)
      width = cur->picture->width;
    if (width <= 0)
      width = 1;
    if (IS_TITLE_ITEM(cur))
    {
      /* titles stretch over the whole menu width, so count the maximum
       * separately */
      tw = width;
    }

    if (tw)
    {
      if (cur->strlen == 0)
      {
	if (tw > title_width)
	  title_width = tw;
	continue;
      }
      else
      {
	width = tw;
      }
    }
    if (width > MR_WIDTH(mr))
      MR_WIDTH(mr) = width;
    width = XTextWidth(MR_STYLE(mr)->look.pStdFont->font, cur->item2,
		       cur->strlen2);
    if (width < 0)
      width = 0;
    cur->x2 = -width;
    if (width > MR_WIDTH2(mr))
      MR_WIDTH2(mr) = width;
    if((width==0)&&(cur->strlen2>0))
      MR_WIDTH2(mr) = 1;

    if(cur->lpicture)
      if(MR_WIDTH0(mr) < (cur->lpicture->width+3))
	MR_WIDTH0(mr) = cur->lpicture->width+3;
  }

  /* lets first size the window accordingly */
  if(MR_WIDTH2(mr) > 0)
    MR_WIDTH(mr) += 5;
  if (MR_WIDTH0(mr) + MR_WIDTH(mr) + MR_WIDTH2(mr) + MR_WIDTH3(mr)
      < title_width)
    /* make it wide enough for the title */
    MR_WIDTH(mr) = title_width - MR_WIDTH0(mr) - MR_WIDTH2(mr) - MR_WIDTH3(mr);
  MR_WIDTH(mr) += 10;

  /* cur_prev trails one behind cur, since we need to move that
     into a newly-made menu if we run out of space */
  for (y=2, cItems = 0, cur = MR_FIRST_ITEM(mr), cur_prev = NULL;
       cur != NULL;
       cur_prev = cur, cur = cur->next, cItems++)
  {
    cur->y_offset = y;
    cur->x = 5+MR_WIDTH0(mr);
    if(IS_TITLE_ITEM(cur))
    {
      width = XTextWidth(MR_STYLE(mr)->look.pStdFont->font, cur->item,
			 cur->strlen);
      /* Title */
      if(cur->strlen2  == 0)
	cur->x = (MR_WIDTH(mr)+MR_WIDTH2(mr)+MR_WIDTH0(mr) - width) / 2;

      if((cur->strlen > 0)||(cur->strlen2>0))
      {
	if(MR_STYLE(mr)->look.TitleUnderlines == 2)
	  cur->y_height = MR_STYLE(mr)->look.EntryHeight +
	    HEIGHT_EXTRA_TITLE;
	else
	{
	  if((cur == MR_FIRST_ITEM(mr))||(cur->next == NULL))
	    cur->y_height=MR_STYLE(mr)->look.EntryHeight-HEIGHT_EXTRA +
	      1 + (HEIGHT_EXTRA_TITLE/2);
	  else
	    cur->y_height = MR_STYLE(mr)->look.EntryHeight -
	      HEIGHT_EXTRA + 1 + HEIGHT_EXTRA_TITLE;
	}
      }
      else
      {
	cur->y_height = HEIGHT_SEPARATOR;
      }
      /* Titles are separators, too */
    }
    else if((cur->strlen==0)&&(cur->strlen2 == 0)&&
	    /* added check for NOP to distinguish from items with no text,
	     * only pixmap */
	    StrEquals(cur->action,"nop"))
    {
      /* Separator */
      cur->y_height = HEIGHT_SEPARATOR;
      cur->flags.is_separator = True;
    }
    else
    {
      /* Normal text entry */
      if ((cur->strlen==0)&&(cur->strlen2 == 0))
	cur->y_height = HEIGHT_EXTRA;
      else
	cur->y_height = MR_STYLE(mr)->look.EntryHeight;
    }
    if(cur->picture)
      cur->y_height += cur->picture->height;
    if(cur->lpicture && cur->y_height < cur->lpicture->height+4)
      cur->y_height = cur->lpicture->height+4;
    y += cur->y_height;
    if(MR_WIDTH2(mr) == 0)
    {
#if 0
      cur->x2 = cur->x;
#endif
    }
    else
    {
#if 0
      cur->x2 = MR_WIDTH(mr) -5 + MR_WIDTH0(mr);
#endif
    }
    /* this item would have to be the last item, or else
       we need to add a "More..." entry pointing to a new menu */
    if (y+MR_STYLE(mr)->look.EntryHeight > Scr.MyDisplayHeight &&
	cur->next != NULL)
    {
      char *szMenuContinuationActionAndName;
      MenuRoot *menuContinuation;

      if (MR_CONTINUATION_MENU(mr) != NULL)
      {
	fvwm_msg(ERR, "MakeMenu",
		 "Confused-- expected continuation to be null");
	break;
      }
      szMenuContinuationActionAndName =
	(char *)safemalloc((8+strlen(MR_NAME(mr)))*sizeof(char));
      strcpy(szMenuContinuationActionAndName,"Popup ");
      strcat(szMenuContinuationActionAndName, MR_NAME(mr));
      strcat(szMenuContinuationActionAndName,"$");
      /* NewMenuRoot inserts at the head of the list of menus
	 but, we need it at the end */
      /* (Give it just the name, which is 6 chars past the action
	 since strlen("Popup ")==6 ) */
      menuContinuation = NewMenuRoot(szMenuContinuationActionAndName+6);
      MR_CONTINUATION_MENU(mr) = menuContinuation;
      MR_IS_CONTINUATION_MENU(menuContinuation) = True;

      /* Now move this item and the remaining items into the new menu */
      cItems--;
      MR_FIRST_ITEM(menuContinuation) = cur;
      MR_LAST_ITEM(menuContinuation) = MR_LAST_ITEM(mr);
      MR_ITEMS(menuContinuation) = MR_ITEMS(mr) - cItems;
      cur->prev = NULL;

      /* cur_prev is now the last item in the current menu */
      MR_LAST_ITEM(mr) = cur_prev;
      MR_ITEMS(mr) = cItems;
      cur_prev->next = NULL;

      /* Go back one, so that this loop will process the new item */
      y -= cur->y_height;
      cur = cur_prev;

      /* And add the entry pointing to the new menu */
      AddToMenu(mr,"More&...",szMenuContinuationActionAndName,
		False /* no pixmap scan */, False);
      MakeMenu(menuContinuation);
      free(szMenuContinuationActionAndName);
    }
  } /* for */
  /* allow two pixels for top border */
  MR_HEIGHT(mr) = y + ((MR_STYLE(mr)->look.ReliefThickness == 1) ? 2 : 3);
  memset(&(MR_TEMP_FLAGS(mr)), 0, sizeof(MR_TEMP_FLAGS(mr)));
  MR_XANIMATION(mr) = 0;

  MR_XOFFSET(mr) = 0;
  if(MR_SIDEPIC(mr))
  {
    MR_XOFFSET(mr) = MR_SIDEPIC(mr)->width + 5;
  }
  else if (MR_STYLE(mr)->look.sidePic)
  {
    MR_XOFFSET(mr) = MR_STYLE(mr)->look.sidePic->width + 5;
  }

  MR_WIDTH(mr) = MR_WIDTH0(mr) + MR_WIDTH(mr) + MR_WIDTH2(mr) + MR_WIDTH3(mr) +
    MR_XOFFSET(mr);
  MR_IS_BACKGROUND_SET(mr) = False;

  /* create a new window for the menu */
  MakeMenuWindow(mr);

  return;
}

static void MakeMenuWindow(MenuRoot *mr)
{
  unsigned long valuemask;
  XSetWindowAttributes attributes;

  valuemask = CWBackPixel | CWEventMask | CWCursor | CWColormap
    | CWBorderPixel | CWSaveUnder;
  attributes.border_pixel = 0;
  attributes.colormap = Scr.cmap;
  attributes.background_pixel = MR_STYLE(mr)->look.MenuColors.back;
  attributes.event_mask = (ExposureMask | EnterWindowMask);
  attributes.cursor = Scr.FvwmCursors[MENU];
  attributes.save_under = True;

  if(MR_WINDOW(mr) != None)
    XDestroyWindow(dpy,MR_WINDOW(mr));
  MR_WINDOW(mr) = XCreateWindow(dpy, Scr.Root, 0, 0,
				(unsigned int) (MR_WIDTH(mr)),
				(unsigned int) MR_HEIGHT(mr),
				(unsigned int) 0, Scr.depth, InputOutput,
				Scr.viz, valuemask, &attributes);
  XSaveContext(dpy,MR_WINDOW(mr),MenuContext,(caddr_t)mr);
}


/***********************************************************************
 * Procedure:
 *	scanForHotkeys - Look for hotkey markers in a MenuItem
 * 							(pete@tecc.co.uk)
 *
 * Inputs:
 *	it	- MenuItem to scan
 * 	which 	- +1 to look in it->item1 and -1 to look in it->item2.
 *
 ***********************************************************************/
static char scanForHotkeys(MenuItem *it, int which)
{
  char *start, *txt;

  start = (which > 0) ? it->item : it->item2;	/* Get start of string	*/
  for (txt = start; *txt != '\0'; txt++)
    {
      /* Scan whole string	*/
      if (*txt == '&')
	{		/* A hotkey marker?			*/
	  if (txt[1] == '&')
	    {	/* Just an escaped &			*/
	      char *tmp;		/* Copy the string down over it	*/
	      for (tmp = txt; *tmp != '\0'; tmp++) tmp[0] = tmp[1];
	      continue;		/* ...And skip to the key char		*/
	    }
	  else {
	    char ch = txt[1];
	    /* It's a hot key marker - work out the offset value */
	    it->hotkey = (1 + (txt - start)) * which;
	    for (; *txt != '\0'; txt++)
	      /* Copy down.. */
	      txt[0] = txt[1];
	    return ch;			/* Only one hotkey per item...	*/
	    }
	}
    }
  it->hotkey = 0;		/* No hotkey found.  Set offset to zero	*/
  return '\0';
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

  /* Scan whole string        */
  for (txt = instring; *txt != '\0'; txt++)
    {
      /* A hotkey marker? */
      if (*txt == identifier)
      {
        /* Just an escaped '^'  */
        if (txt[1] == identifier)
          {
            char *tmp;                /* Copy the string down over it */
            for (tmp = txt; *tmp != '\0'; tmp++) tmp[0] = tmp[1];
            continue;         /* ...And skip to the key char          */
          }
        /* It's a hot key marker - work out the offset value          */
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

static void scanForPixmap(char *instring, Picture **p, char identifier)
{
  char *tstart, *txt, *name;
  int i;
  Picture *pp;
#ifdef UGLY_WHEN_PIXMAPS_MISSING
  char *save_instring;
#endif

  if (!instring)
    {
      *p = NULL;
      return;
    }

#ifdef UGLY_WHEN_PIXMAPS_MISSING
  /* save instring in case can't find pixmap */
  save_instring = (char *)safemalloc(strlen(instring)+1);
  strcpy(save_instring,instring);
#endif
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
	  while((*txt != identifier)&&(*txt != '\0'))
	    {
	      name[i] = *txt;
	      txt++;
	      i++;
	    }
	  name[i] = 0;

	  /* Next, check for a color pixmap */
	  pp=CachePicture(dpy,Scr.NoFocusWin,NULL,name,
			  Scr.ColorLimit);
	  if(*txt != '\0')
	    txt++;
	  while(*txt != '\0')
	    {
	      *tstart++ = *txt++;
	    }
	  *tstart = 0;
	  if (pp)
	    *p = pp;
	  else
#ifdef UGLY_WHEN_PIXMAPS_MISSING
            strcpy(instring,save_instring);
#else
  	    fvwm_msg(WARN,"scanForPixmap","Couldn't find pixmap %s",name);
#endif
	  break;
	}
    }

  free(name);
#ifdef UGLY_WHEN_PIXMAPS_MISSING
  free(save_instring);
#endif
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
 *       so built in window list can handle windows w/ * and % in title.
 *
 ***********************************************************************/
void AddToMenu(MenuRoot *menu, char *item, char *action, Bool fPixmapsOk,
	       Bool fNoPlus)
{
  MenuItem *tmp;
  char *start,*end;
  char *token = NULL;
  char *token2 = NULL;
  char *option = NULL;

  if ((item == NULL || *item == 0) && (action == NULL || *action == 0) &&
      fNoPlus)
    return;
  /* empty items screw up our menu when painted, so we replace them with a
   * separator */
  if (item == NULL)
    item = "";
  if (StrEquals(item, "DynamicPopupAction"))
  {
    if (MR_DYNAMIC(menu).popup_action)
      free(MR_DYNAMIC(menu).popup_action);
    if (!action || *action == 0)
      MR_DYNAMIC(menu).popup_action = NULL;
    else
      MR_DYNAMIC(menu).popup_action = stripcpy(action);
    return;
  }
  else if (StrEquals(item, "DynamicPopdownAction"))
  {
    if (MR_DYNAMIC(menu).popdown_action)
      free(MR_DYNAMIC(menu).popdown_action);
    if (!action || *action == 0)
      MR_DYNAMIC(menu).popdown_action = NULL;
    else
      MR_DYNAMIC(menu).popdown_action = stripcpy(action);
    return;
  }

  if (action == NULL || *action == 0)
    action = "Nop";
  GetNextToken(GetNextToken(action, &token), &option);
  tmp = (MenuItem *)safemalloc(sizeof(MenuItem));
  tmp->chHotkey = '\0';
  tmp->next = NULL;
  if (MR_FIRST_ITEM(menu) == NULL)
    {
      MR_FIRST_ITEM(menu) = tmp;
      MR_LAST_ITEM(menu) = tmp;
      tmp->prev = NULL;
    }
  else if (StrEquals(token, "title") && option && StrEquals(option, "top"))
    {
      if (MR_FIRST_ITEM(menu)->action)
	{
	  GetNextToken(MR_FIRST_ITEM(menu)->action, &token2);
	}
      if (StrEquals(token2, "title"))
	{
	  tmp->next = MR_FIRST_ITEM(menu)->next;
	  FreeMenuItem(MR_FIRST_ITEM(menu));
	}
      else
	{
	  tmp->next = MR_FIRST_ITEM(menu);
	}
      if (token2)
	free(token2);
      tmp->prev = NULL;
      if (MR_FIRST_ITEM(menu) == NULL)
	MR_LAST_ITEM(menu) = tmp;
      MR_FIRST_ITEM(menu) = tmp;
    }
  else
    {
      MR_LAST_ITEM(menu)->next = tmp;
      tmp->prev = MR_LAST_ITEM(menu);
      MR_LAST_ITEM(menu) = tmp;
    }
  if (token)
    free(token);
  if (option)
    free(option);
  tmp->picture=NULL;
  tmp->lpicture=NULL;

  /* skip leading spaces */
  /*while(isspace(*item)&&(item != NULL))
    item++;*/
  /* up to first tab goes in "item" field */
  start = item;
  end=item;
  while((*end != '\t')&&(*end != 0))
    end++;
  tmp->item = safemalloc(end-start+1);
  strncpy(tmp->item,start,end-start);
  tmp->item[end-start] = 0;
  tmp->item2 = NULL;
  if(*end=='\t')
    {
      start = end+1;
      while(*start == '\t')
	start++;
      end = start;
      while(*end != 0)
	end++;
      if(end > start)
	{
	  char *s;

	  tmp->item2 = safemalloc(end-start+1);
	  strncpy(tmp->item2,start,end-start);
	  tmp->item2[end-start] = 0;
	  s = tmp->item2;
	  while (*s)
	    {
	      if (*s == '\t')
		*s = ' ';
	      s++;
	    }
	}
    }

  if (item != (char *)0)
    {
      char ch;
      if (fPixmapsOk)
      {
        scanForPixmap(tmp->item,&tmp->picture,'*');
        scanForPixmap(tmp->item,&tmp->lpicture,'%');
      }
      ch = scanForHotkeys(tmp, 1);	/* pete@tecc.co.uk */
      if (ch != '\0')
        tmp->chHotkey = ch;
      tmp->strlen = strlen(tmp->item);
    }
  else
    tmp->strlen = 0;

  if (tmp->item2 != (char *)0)
    {
      if (fPixmapsOk)
      {
        if(!tmp->picture)
          scanForPixmap(tmp->item2,&tmp->picture,'*');
        if(!tmp->lpicture)
          scanForPixmap(tmp->item2,&tmp->lpicture,'%');
      }
      if (tmp->hotkey == 0)
      {
        char ch = scanForHotkeys(tmp, -1);	/* pete@tecc.co.uk */
	if (ch != '\0')
	  tmp->chHotkey = ch;
      }
      tmp->strlen2 = strlen(tmp->item2);
    }
  else
    tmp->strlen2 = 0;

  tmp->action = stripcpy(action);
  find_func_type(tmp->action, &(tmp->func_type), NULL);
  memset(&(tmp->flags), 0, sizeof(tmp->flags));
  switch (tmp->func_type)
  {
  case F_POPUP:
    tmp->flags.is_popup = True;
  case F_WINDOWLIST:
  case F_STAYSUP:
    tmp->flags.is_menu = True;
    break;
  case F_TITLE:
    tmp->flags.is_separator = True;
    tmp->flags.is_title = True;
    break;
  default:
    if(tmp->strlen == 0 && tmp->strlen2 == 0 && StrEquals(tmp->action,"nop"))
      /* check for NOP to distinguish from items with no text, only pixmap */
      tmp->flags.is_separator = True;
    break;
  }

  MR_ITEMS(menu)++;
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
  MenuRoot *tmp;
  Bool flag;

  tmp = (MenuRoot *)safemalloc(sizeof(MenuRoot));
  tmp->s = (MenuRootStatic *)safemalloc(sizeof(MenuRootStatic));
  tmp->d = (MenuRootDynamic *)safemalloc(sizeof(MenuRootDynamic));

  memset(tmp->s, 0, sizeof(MenuRootStatic));
  memset(tmp->d, 0, sizeof(MenuRootDynamic));
  MR_NEXT_MENU(tmp) = Menus.all;
  MR_NAME(tmp) = stripcpy(name);
  MR_WINDOW(tmp) = None;
  scanForPixmap(MR_NAME(tmp), &MR_SIDEPIC(tmp), '@');
  scanForColor(MR_NAME(tmp), &MR_SIDECOLOR(tmp), &flag,'^');
  MR_HAS_SIDECOLOR(tmp) = flag;
  MR_STYLE(tmp) = Menus.DefaultStyle;
  MR_ORIGINAL_MENU(tmp) = tmp;
  MR_COPIES(tmp) = 1;

  Menus.all = tmp;
  return tmp;
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
static MenuRoot *CopyMenuRoot(MenuRoot *menu)
{
  MenuRoot *tmp;

  if (!menu || MR_COPIES(menu) >= MAX_MENU_COPIES)
    return NULL;
  tmp = (MenuRoot *)safemalloc(sizeof(MenuRoot));
  tmp->d = (MenuRootDynamic *)safemalloc(sizeof(MenuRootDynamic));
  memset(tmp->d, 0, sizeof(MenuRootDynamic));
  tmp->s = menu->s;

  MR_COPIES(menu)++;
  MR_ORIGINAL_MENU(tmp) = MR_ORIGINAL_MENU(menu);
  MR_CONTINUATION_MENU(tmp) = CopyMenuRoot(MR_CONTINUATION_MENU(menu));
  MR_NEXT_MENU(tmp) = MR_NEXT_MENU(menu);
  MR_NEXT_MENU(menu) = tmp;
  MR_WINDOW(tmp) = None;

  return tmp;
}



/***********************************************************************
 * change by KitS@bartley.demon.co.uk to correct popups off title buttons
 *
 *  Procedure:
 *ButtonPosition - find the actual position of the button
 *                 since some buttons may be disabled
 *
 *  Returned Value:
 *The button count from left or right taking in to account
 *that some buttons may not be enabled for this window
 *
 *  Inputs:
 *      context - context as per the global Context
 *      t       - the window (FvwmWindow) to test against
 *
 ***********************************************************************/
int ButtonPosition(int context, FvwmWindow * t)
{
  int i;
  int buttons = -1;

  if (context&C_RALL)
  {
    for(i=0;i<Scr.nr_right_buttons;i++)
    {
      if(t->right_w[i])
      {
	buttons++;
      }
      /* is this the button ? */
      if (((1<<i)*C_R1) & context)
	return(buttons);
    }
  }
  else
  {
    for(i=0;i<Scr.nr_left_buttons;i++)
    {
      if(t->left_w[i])
      {
	buttons++;
      }
      /* is this the button ? */
      if (((1<<i)*C_L1) & context)
	return(buttons);
    }
  }
  /* you never know... */
  return 0;
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
  FvwmWindow *fw;
  int tc;
  extern FvwmWindow *Tmp_win;
  extern FvwmWindow *ButtonWindow;
  extern int Context;

  memset(&(mops.flags), 0, sizeof(mops.flags));
  action = GetNextToken(action,&menu_name);
  action = GetMenuOptions(action, w, tmp_win, NULL, NULL, &mops);
  while (action && *action && isspace(*action))
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

  mp.menu = menu;
  mp.parent_menu = NULL;
  mp.parent_item = NULL;
  fw = Tmp_win;
  mp.pTmp_win = &fw;
  mp.button_window = ButtonWindow;
  tc = Context;
  mp.pcontext = &tc;
  mp.flags.is_menu_from_frame_or_window_or_titlebar = False;
  mp.flags.is_sticky = fStaysUp;
  mp.flags.is_submenu = False;
  mp.eventp = teventp;
  mp.pops = &mops;
  mp.ret_paction = &ret_action;

  if (do_menu(&mp) == MENU_DOUBLE_CLICKED && action)
  {
    ExecuteFunction(action,tmp_win,eventp,context,*Module,EXPAND_COMMAND);
  }
}

/* the function for the "Popup" command */
void popup_func(F_CMD_ARGS)
{
  menu_func(eventp, w, tmp_win, context, action, Module, False);
}

/* the function for the "Menu" command */
void staysup_func(F_CMD_ARGS)
{
  menu_func(eventp, w, tmp_win, context, action, Module, True);
}

/*
 * -------------------------- Menu style commands ----------------------------
 */

static void FreeMenuFace(Display *dpy, MenuFace *mf)
{
  switch (mf->type)
  {
#ifdef GRADIENT_BUTTONS
  case HGradMenu:
  case VGradMenu:
  case DGradMenu:
    /* - should we check visual is not TrueColor before doing this?
     *
     *            XFreeColors(dpy, PictureCMap,
     *                        ms->u.grad.pixels, ms->u.grad.npixels,
     *                        AllPlanes); */
    free(mf->u.grad.pixels);
    mf->u.grad.pixels = NULL;
    break;
#endif

#ifdef PIXMAP_BUTTONS
  case PixmapMenu:
  case TiledPixmapMenu:
    if (mf->u.p)
      DestroyPicture(dpy, mf->u.p);
    mf->u.p = NULL;
    break;
#endif
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

#ifdef GRADIENT_BUTTONS
  else if (StrEquals(style,"HGradient") || StrEquals(style, "VGradient") ||
	   StrEquals(style,"DGradient") || StrEquals(style, "BGradient"))
  {
    char *item, **s_colors;
    int npixels, nsegs, i, sum, perc[128];
    Pixel *pixels;
    char gtype = style[0];

    s = GetNextToken(s, &item);
    if (!item)
    {
      if(verbose)
	fvwm_msg(ERR, "ReadMenuFace",
		 "expected number of colors to allocate in gradient");
      free(style);
      return False;
    }
    npixels = atoi(item);
    free(item);

    s = GetNextToken(s, &item);
    if (!item)
    {
      if(verbose)
	fvwm_msg(ERR, "ReadMenuFace", "incomplete gradient style");
      free(style);
      return False;
    }

    if (!(isdigit(*item)))
    {
      s_colors = (char **)safemalloc(sizeof(char *) * 2);
      nsegs = 1;
      s_colors[0] = item;
      s = GetNextToken(s, &s_colors[1]);
      if (!s_colors[1])
      {
	if(verbose)
	  fvwm_msg(ERR, "ReadMenuFace", "incomplete gradient style");
	free(s_colors);
	free(item);
	free(style);
	return False;
      }
      perc[0] = 100;
    }
    else
    {
      nsegs = atoi(item);
      free(item);
      if (nsegs < 1)
	nsegs = 1;
      if (nsegs > 128)
	nsegs = 128;
      s_colors = (char **)safemalloc(sizeof(char *) * (nsegs + 1));
      for (i = 0; i <= nsegs; ++i)
      {
	s = GetNextToken(s, &s_colors[i]);
	if (i < nsegs)
	{
	  s = GetNextToken(s, &item);
	  perc[i] = (item) ? atoi(item) : 0;
	  if (item)
	    free(item);
	}
      }
    }

    for (i = 0, sum = 0; i < nsegs; ++i)
      sum += perc[i];

    if (sum != 100)
    {
      if(verbose)
	fvwm_msg(ERR,"ReadMenuFace", "multi gradient lenghts must sum to 100");
      for (i = 0; i <= nsegs; ++i)
	if (s_colors[i])
	  free(s_colors[i]);
      free(style);
      free(s_colors);
      return False;
    }

    if (npixels < 2)
      npixels = 2;
    if (npixels > 128)
      npixels = 128;

    pixels = AllocNonlinearGradient(s_colors, perc, nsegs, npixels);
    for (i = 0; i <= nsegs; ++i)
      if (s_colors[i])
	free(s_colors[i]);
    free(s_colors);

    if (!pixels)
    {
      if(verbose)
	fvwm_msg(ERR, "ReadMenuFace", "couldn't create gradient");
      free(style);
      return False;
    }

    mf->u.grad.pixels = pixels;
    mf->u.grad.npixels = npixels;

    switch (gtype)
    {
    case 'h':
    case 'H':
      mf->type = HGradMenu;
      break;
    case 'v':
    case 'V':
      mf->type = VGradMenu;
      break;
    case 'd':
    case 'D':
      mf->type = DGradMenu;
      break;
    default:
      mf->type = BGradMenu;
      break;
    }
  }
#endif /* GRADIENT_BUTTONS */

#ifdef PIXMAP_BUTTONS
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
#endif /* PIXMAP_BUTTONS */

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
     if(strcasecmp(ms->name,name)==0)
       return ms;
     ms = ms->next;
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
  if(ms->look.MenuGC)
    XFreeGC(dpy, ms->look.MenuGC);
  if(ms->look.MenuActiveGC)
    XFreeGC(dpy, ms->look.MenuActiveGC);
  if(ms->look.MenuActiveBackGC)
    XFreeGC(dpy, ms->look.MenuActiveBackGC);
  if(ms->look.MenuReliefGC)
    XFreeGC(dpy, ms->look.MenuReliefGC);
  if(ms->look.MenuStippleGC)
    XFreeGC(dpy, ms->look.MenuStippleGC);
  if(ms->look.MenuShadowGC)
    XFreeGC(dpy, ms->look.MenuShadowGC);
  if (ms->look.sidePic)
    DestroyPicture(dpy, ms->look.sidePic);
  if (ms->look.flags.has_side_color == 1)
    FreeColors(&ms->look.sideColor, 1);

  while(before->next != ms)
    /* Not too many checks, may segfaults in race conditions */
    before = before->next;

  before->next = ms->next;
  free(ms->name);
  free(ms);
}

void DestroyMenuStyle(F_CMD_ARGS)
{
  MenuStyle *ms = NULL;
  char *name = NULL;
  MenuRoot *mr;

  action = GetNextToken(action,&name);
  if (name == NULL)
  {
    fvwm_msg(ERR,"DestroyMenuStyle", "needs one parameter");
    return;
  }

  ms = FindMenuStyle(name);
  if(ms == NULL)
    fvwm_msg(ERR,"DestroyMenuStyle", "cannot find style %s", name);
  else if (ms == Menus.DefaultStyle)
    fvwm_msg(ERR,"DestroyMenuStyle", "cannot destroy Default Menu Face");
  else
  {
    FreeMenuFace(dpy, &ms->look.face);
    FreeMenuStyle(ms);
    MakeMenus();
  }
  free(name);
  for (mr = Menus.all; mr; mr = MR_NEXT_MENU(mr))
  {
    if (MR_STYLE(mr) == ms)
      MR_STYLE(mr) = Menus.DefaultStyle;
  }
  MakeMenus();
}

static void UpdateMenuStyle(MenuStyle *ms)
{
  XGCValues gcv;
  unsigned long gcm;

  if (ms->look.pStdFont && ms->look.pStdFont != &Scr.StdFont)
    {
      ms->look.pStdFont->y = ms->look.pStdFont->font->ascent;
      ms->look.pStdFont->height =
	ms->look.pStdFont->font->ascent +
	ms->look.pStdFont->font->descent;
    }
  ms->look.EntryHeight =
    ms->look.pStdFont->height + HEIGHT_EXTRA;

  /* calculate colors based on foreground */
  if (!ms->look.flags.has_active_fore)
    ms->look.MenuActiveColors.fore=ms->look.MenuColors.fore;

  /* calculate colors based on background */
  if (!ms->look.flags.has_active_back)
    ms->look.MenuActiveColors.back = ms->look.MenuColors.back;
  if (!ms->look.flags.has_stipple_fore)
    ms->look.MenuStippleColors.fore = ms->look.MenuColors.back;
  if(Scr.depth > 2)
  {                 /* if not black and white */
    ms->look.MenuRelief.back = GetShadow(ms->look.MenuColors.back);
    ms->look.MenuRelief.fore = GetHilite(ms->look.MenuColors.back);
  }
  else
  {                              /* black and white */
    ms->look.MenuRelief.back = GetColor("black");
    ms->look.MenuRelief.fore = GetColor("white");
  }
  ms->look.MenuStippleColors.back = ms->look.MenuColors.back;

  /* make GC's */
  gcm = GCFunction|GCFont|GCLineWidth|GCForeground|GCBackground;
  gcv.font = ms->look.pStdFont->font->fid;
  gcv.function = GXcopy;
  gcv.line_width = 0;

  gcv.foreground = ms->look.MenuRelief.fore;
  gcv.background = ms->look.MenuRelief.back;
  if(ms->look.MenuReliefGC)
    XChangeGC(dpy, ms->look.MenuReliefGC, gcm, &gcv);
  else
    ms->look.MenuReliefGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcv.foreground = ms->look.MenuRelief.back;
  gcv.background = ms->look.MenuRelief.fore;
  if(ms->look.MenuShadowGC)
    XChangeGC(dpy, ms->look.MenuShadowGC, gcm, &gcv);
  else
    ms->look.MenuShadowGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcv.foreground = (ms->look.flags.has_active_back) ?
    ms->look.MenuActiveColors.back : ms->look.MenuRelief.back;
  gcv.background = (ms->look.flags.has_active_fore) ?
    ms->look.MenuActiveColors.fore : ms->look.MenuRelief.fore;
  if(ms->look.MenuActiveBackGC)
    XChangeGC(dpy, ms->look.MenuActiveBackGC, gcm, &gcv);
  else
    ms->look.MenuActiveBackGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcv.foreground = ms->look.MenuColors.fore;
  gcv.background = ms->look.MenuColors.back;
  if(ms->look.MenuGC)
    XChangeGC(dpy, ms->look.MenuGC, gcm, &gcv);
  else
    ms->look.MenuGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  gcv.foreground = ms->look.MenuActiveColors.fore;
  gcv.background = ms->look.MenuActiveColors.back;
  if(ms->look.MenuActiveGC)
    XChangeGC(dpy, ms->look.MenuActiveGC, gcm, &gcv);
  else
    ms->look.MenuActiveGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);

  if(Scr.depth < 2)
  {
    gcv.fill_style = FillStippled;
    gcv.stipple = Scr.gray_bitmap;
    gcm|=GCStipple|GCFillStyle; /* no need to reset fg/bg, FillStipple wins */
  }
  else
  {
    gcv.foreground = ms->look.MenuStippleColors.fore;
    gcv.background = ms->look.MenuStippleColors.back;
  }
  if(ms->look.MenuStippleGC)
    XChangeGC(dpy, ms->look.MenuStippleGC, gcm, &gcv);
  else
    ms->look.MenuStippleGC = XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
}

void UpdateAllMenuStyles(void)
{
  MenuStyle *ms;

  for (ms = Menus.DefaultStyle; ms; ms = ms->next)
  {
    if (ms->look.pStdFont == &Scr.StdFont)
      ms->look.EntryHeight = Scr.StdFont.height + HEIGHT_EXTRA;
    UpdateMenuStyle(ms);
  }
  MakeMenus();
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
  Bool gc_changed = False;
  Bool is_default_style = False;
  int val[2];
  int n;
  XFontStruct *xfs = NULL;
  int i;

  action = GetNextToken(action, &name);
  if (!name)
    {
      fvwm_msg(ERR,"NewMenuStyle", "error in %s style specification",action);
      return;
    }

  tmpms = (MenuStyle *)safemalloc(sizeof(MenuStyle));
  memset(tmpms, 0, sizeof(MenuStyle));
  ms = FindMenuStyle(name);
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
      tmpms->name = name;
      is_initialised = False;
    }

  /* Parse the options. */
  while (action && *action)
    {
      if (is_initialised == False)
	{
	  /* some default configuration goes here for the new menu style */
	  tmpms->look.MenuColors.back = GetColor("white");
	  tmpms->look.MenuColors.fore = GetColor("black");
	  tmpms->look.pStdFont = &Scr.StdFont;
	  tmpms->look.face.type = SimpleMenu;
	  tmpms->look.flags.has_active_fore = 0;
	  tmpms->look.flags.has_active_back = 0;
	  tmpms->feel.flags.do_popup_as_root_menu = 0;
	  gc_changed = True;
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
	  tmpms->feel.PopupOffsetPercent = 67;
	  tmpms->feel.PopupOffsetAdd = 0;
	  tmpms->feel.flags.do_popup_immediately = 0;
	  tmpms->feel.flags.do_title_warp = 1;
	  tmpms->feel.flags.do_unmap_submenu_on_popdown = 1;
	  tmpms->look.ReliefThickness = 1;
	  tmpms->look.TitleUnderlines = 1;
	  tmpms->look.flags.has_long_separators = 0;
	  tmpms->look.flags.has_triangle_relief = 1;
	  tmpms->look.flags.do_hilight = 0;
	}
	else if (i == 1)
	{
	  tmpms->feel.PopupOffsetPercent = 100;
	  tmpms->feel.PopupOffsetAdd = -3;
	  tmpms->feel.flags.do_popup_immediately = 1;
	  tmpms->feel.flags.do_title_warp = 0;
	  tmpms->feel.flags.do_unmap_submenu_on_popdown = 1;
	  tmpms->look.ReliefThickness = 2;
	  tmpms->look.TitleUnderlines = 2;
	  tmpms->look.flags.has_long_separators = 1;
	  tmpms->look.flags.has_triangle_relief = 1;
	  tmpms->look.flags.do_hilight = 0;
	}
	else /* i == 2 */
	{
	  tmpms->feel.PopupOffsetPercent = 100;
	  tmpms->feel.PopupOffsetAdd = -5;
	  tmpms->feel.flags.do_popup_immediately = 1;
	  tmpms->feel.flags.do_title_warp = 0;
	  tmpms->feel.flags.do_unmap_submenu_on_popdown = 0;
	  tmpms->look.ReliefThickness = 0;
	  tmpms->look.TitleUnderlines = 1;
	  tmpms->look.flags.has_long_separators = 0;
	  tmpms->look.flags.has_triangle_relief = 0;
	  tmpms->look.flags.do_hilight = 1;
	}

	/* common settings */
	tmpms->feel.flags.is_animated = 0;
	FreeMenuFace(dpy, &tmpms->look.face);
	tmpms->look.face.type = SimpleMenu;
	if (tmpms->look.pStdFont && tmpms->look.pStdFont != &Scr.StdFont)
	{
	  XFreeFont(dpy, tmpms->look.pStdFont->font);
	  free(tmpms->look.pStdFont);
	}
	tmpms->look.pStdFont = &Scr.StdFont;
	gc_changed = True;
	if (tmpms->look.flags.has_side_color == 1)
	{
	  FreeColors(&tmpms->look.sideColor, 1);
	  tmpms->look.flags.has_side_color = 0;
	}
	tmpms->look.flags.has_side_color = 0;
	if (tmpms->look.sidePic)
	{
	  DestroyPicture(dpy, tmpms->look.sidePic);
	  tmpms->look.sidePic = NULL;
	}

	if (is_initialised == False)
	{
	  /* now begin the real work */
	  is_initialised = True;
	  continue;
	}
	break;

      case 3: /* Foreground */
	FreeColors(&tmpms->look.MenuColors.fore, 1);
	if (arg1)
	  tmpms->look.MenuColors.fore = GetColor(arg1);
	else
	  tmpms->look.MenuColors.fore = GetColor("black");
	gc_changed = True;
	break;

      case 4: /* Background */
	FreeColors(&tmpms->look.MenuColors.back, 1);
	if (arg1)
	  tmpms->look.MenuColors.back = GetColor(arg1);
	else
	  tmpms->look.MenuColors.back = GetColor("grey");
	gc_changed = True;
	break;

      case 5: /* Greyed */
	if (tmpms->look.flags.has_stipple_fore)
	  FreeColors(&tmpms->look.MenuStippleColors.fore, 1);
	if (arg1 == NULL)
	{
	  tmpms->look.flags.has_stipple_fore = 0;
	}
	else
	{
	  tmpms->look.MenuStippleColors.fore = GetColor(arg1);
	  tmpms->look.flags.has_stipple_fore = 1;
	}
	gc_changed = True;
	break;

      case 6: /* HilightBack */
	if (tmpms->look.flags.has_active_back)
	  FreeColors(&tmpms->look.MenuActiveColors.back, 1);
	if (arg1 == NULL)
	{
	  tmpms->look.flags.has_active_back = 0;
	}
	else
	{
	  tmpms->look.MenuActiveColors.back = GetColor(arg1);
	  tmpms->look.flags.has_active_back = 1;
	}
	tmpms->look.flags.do_hilight = 1;
	gc_changed = True;
	break;

      case 7: /* HilightBackOff */
	tmpms->look.flags.do_hilight = 0;
	gc_changed = True;
	break;

      case 8: /* ActiveFore */
	if (tmpms->look.flags.has_active_fore)
	  FreeColors(&tmpms->look.MenuActiveColors.fore, 1);
	if (arg1 == NULL)
	{
	  tmpms->look.flags.has_active_fore = 0;
	}
	else
	{
	  tmpms->look.MenuActiveColors.fore = GetColor(arg1);
	  tmpms->look.flags.has_active_fore = 1;
	}
	gc_changed = True;
	break;

      case 9: /* ActiveForeOff */
	tmpms->look.flags.has_active_fore = 0;
	gc_changed = True;
	break;

      case 10: /* Hilight3DThick */
	tmpms->look.ReliefThickness = 2;
	break;

      case 11: /* Hilight3DThin */
	tmpms->look.ReliefThickness = 1;
	break;

      case 12: /* Hilight3DOff */
	tmpms->look.ReliefThickness = 0;
	break;

      case 13: /* Animation */
	tmpms->feel.flags.is_animated = 1;
	break;

      case 14: /* AnimationOff */
	tmpms->feel.flags.is_animated = 0;
	break;

      case 15: /* Font */
	if (arg1 && (xfs = GetFontOrFixed(dpy, arg1)) == NULL)
	{
	  fvwm_msg(ERR,"NewMenuStyle",
		   "Couldn't load font '%s' or 'fixed'\n", arg1);
	  break;
	}
	if (tmpms->look.pStdFont && tmpms->look.pStdFont != &Scr.StdFont)
	{
	  if (tmpms->look.pStdFont->font)
	    XFreeFont(dpy, tmpms->look.pStdFont->font);
	  free(tmpms->look.pStdFont);
	}
	if (arg1 == NULL)
	{
	  /* reset to screen font */
	  tmpms->look.pStdFont = &Scr.StdFont;
	}
	else
	{
	  tmpms->look.pStdFont = (MyFont *)safemalloc(sizeof(MyFont));
	  tmpms->look.pStdFont->font = xfs;
	}
	gc_changed = True;
	break;

      case 16: /* MenuFace */
	while (args && *args != '\0' && isspace(*args))
	  args++;
	ReadMenuFace(args, &tmpms->look.face, True);
	break;

      case 17: /* PopupDelay */
	if (GetIntegerArguments(args, NULL, val, 1) == 0 || *val < 0)
	  Menus.PopupDelay10ms = DEFAULT_POPUP_DELAY;
	else
	  Menus.PopupDelay10ms = (*val+9)/10;
	if (!is_default_style)
	{
	  fvwm_msg(WARN, "NewMenuStyle",
		   "PopupDelay applied to style '%s' will affect all menus",
		   tmpms->name);
	}
	break;

      case 18: /* PopupOffset */
	if ((n = GetIntegerArguments(args, NULL, val, 2)) == 0)
	{
	  fvwm_msg(ERR,"NewMenuStyle",
		   "PopupOffset requires one or two arguments");
	}
	else
	{
	  tmpms->feel.PopupOffsetAdd = val[0];
	  if (n == 2 && val[1] <= 100 && val[1] >= 0)
	    tmpms->feel.PopupOffsetPercent = val[1];
	  else
	    tmpms->feel.PopupOffsetPercent = 100;
	}
	break;

      case 19: /* TitleWarp */
	tmpms->feel.flags.do_title_warp = 1;
	break;

      case 20: /* TitleWarpOff */
	tmpms->feel.flags.do_title_warp = 0;
	break;

      case 21: /* TitleUnderlines0 */
	tmpms->look.TitleUnderlines = 0;
	break;

      case 22: /* TitleUnderlines1 */
	tmpms->look.TitleUnderlines = 1;
	break;

      case 23: /* TitleUnderlines2 */
	tmpms->look.TitleUnderlines = 2;
	break;

      case 24: /* SeparatorsLong */
	tmpms->look.flags.has_long_separators = 1;
	break;

      case 25: /* SeparatorsShort */
	tmpms->look.flags.has_long_separators = 0;
	break;

      case 26: /* TrianglesSolid */
	tmpms->look.flags.has_triangle_relief = 0;
	break;

      case 27: /* TrianglesRelief */
	tmpms->look.flags.has_triangle_relief = 1;
	break;

      case 28: /* PopupImmediately */
	tmpms->feel.flags.do_popup_immediately = 1;
	break;

      case 29: /* PopupDelayed */
	tmpms->feel.flags.do_popup_immediately = 0;
	break;

      case 30: /* DoubleClickTime */
	if (GetIntegerArguments(args, NULL, val, 1) == 0 || *val < 0)
	  Menus.DoubleClickTime = DEFAULT_MENU_CLICKTIME;
	else
	  Menus.DoubleClickTime = *val;
	if (!is_default_style)
	{
	  fvwm_msg(WARN, "NewMenuStyle",
		   "DoubleClickTime for style '%s' will affect all menus",
		   tmpms->name);
	}
	break;

      case 31: /* SidePic */
	if (tmpms->look.sidePic)
	{
	  DestroyPicture(dpy, tmpms->look.sidePic);
	  tmpms->look.sidePic = NULL;
	}
	if (arg1)
	{
	    tmpms->look.sidePic = CachePicture(dpy, Scr.NoFocusWin, NULL,
					       arg1, Scr.ColorLimit);
	  if (!tmpms->look.sidePic)
	    fvwm_msg(WARN, "NewMenuStyle", "Couldn't find pixmap %s", arg1);
	}
	break;

      case 32: /* SideColor */
	if (tmpms->look.flags.has_side_color == 1)
	{
	  FreeColors(&tmpms->look.sideColor, 1);
	  tmpms->look.flags.has_side_color = 0;
	}
	if (arg1)
	{
	  tmpms->look.sideColor = GetColor(arg1);
	  tmpms->look.flags.has_side_color = 1;
	}
	break;

      case 33: /* PopupAsRootmenu */
	tmpms->feel.flags.do_popup_as_root_menu = 1;
	break;

      case 34: /* PopupAsSubmenu */
	tmpms->feel.flags.do_popup_as_root_menu = 0;
	break;

      case 35: /* RemoveSubmenus */
	tmpms->feel.flags.do_unmap_submenu_on_popdown = 1;
	break;

      case 36: /* HoldSubmenus */
	tmpms->feel.flags.do_unmap_submenu_on_popdown = 0;
	break;

#if 0
      case 37: /* PositionHints */
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

  if (gc_changed)
    {
      UpdateMenuStyle(tmpms);
    } /* if (gc_changed) */

  if(Menus.DefaultStyle == NULL)
    {
      /* First MenuStyle MUST be the default style */
      Menus.DefaultStyle = tmpms;
      tmpms->next = NULL;
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
      tmpms->next = NULL;
      while(before->next)
	before = before->next;
      before->next = tmpms;
    }

  MakeMenus();

  return;
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
	      "* %s, Foreground %s, Background %s, Greyed %s, Font %s, %s",
	      style, fore, back, stipple, font,
	      (animated && StrEquals(animated, "anim")) ?
	        "Animation" : "AnimationOff");
      NewMenuStyle(eventp, w, tmp_win, context, buffer, Module);
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


void SetMenuStyle(F_CMD_ARGS)
{
  char *option;

  GetNextSimpleOption(SkipNTokens(action, 1), &option);
  if (option == NULL || GetMenuStyleIndex(option) != -1)
    NewMenuStyle(eventp, w, tmp_win, context, action, Module);
  else
    OldMenuStyle(eventp, w, tmp_win, context, action, Module);
  if (option)
    free(option);
  return;
}


void ChangeMenuStyle(F_CMD_ARGS)
{
  char *name = NULL, *menuname = NULL;
  MenuStyle *ms = NULL;
  MenuRoot *mr = NULL;


  action = GetNextToken(action,&name);
  if (name == NULL)
  {
    fvwm_msg(ERR,"ChangeMenuStyle", "needs at least two parameters");
    return;
  }

  ms = FindMenuStyle(name);
  if(ms == NULL)
  {
    fvwm_msg(ERR,"ChangeMenuStyle", "cannot find style %s", name);
    free(name);
    return;
  }
  free(name);

  action = GetNextToken(action,&menuname);
  while(menuname && *menuname)
  {
    mr = FindPopup(menuname);
    if(mr == NULL)
    {
      fvwm_msg(ERR,"ChangeMenuStyle", "cannot find menu %s", menuname);
      free(menuname);
      break;
    }
    MR_STYLE(mr) = ms;
    MakeMenu(mr);
    free(menuname);
    action = GetNextToken(action,&menuname);
  }
}

void add_item_to_menu(F_CMD_ARGS)
{
  MenuRoot *mr;
  MenuRoot *mrPrior;
  char *token, *rest,*item;

  rest = GetNextToken(action,&token);
  if (!token)
    return;
  mr = FollowMenuContinuations(FindPopup(token),&mrPrior);
  if(mr == NULL)
    mr = NewMenuRoot(token);

  /* Set + state to last menu */
  set_last_added_item(ADDED_MENU, mr);

  rest = GetNextToken(rest,&item);
  AddToMenu(mr, item,rest,True /* pixmap scan */, True);
  if (item)
    free(item);
  /* These lines are correct! We must not release token if the string is
     empty.  WHY??
   * It cannot be NULL! GetNextToken never returns an empty string! */
  if (*token)
    free(token);

  MakeMenu(mr);
  return;
}

void add_another_menu_item(char *action)
{
  MenuRoot *mr;
  MenuRoot *mrPrior;
  char *rest,*item;

  mr = FollowMenuContinuations(Scr.last_added_item.item ,&mrPrior);
  if(mr == NULL)
    return;
  rest = GetNextToken(action,&item);
  AddToMenu(mr, item,rest,True /* pixmap scan */, False);
  if (item)
    free(item);
  MakeMenu(mr);
}


/*****************************************************************************
 * Used by GetMenuOptions
 *
 * The vars are named for the x-direction, but this is used for both x and y
 *****************************************************************************/
static
char *GetOneMenuPositionArgument(char *action,int x,int w,int *pFinalX,
				 float *width_factor)
{
  char *token, *orgtoken, *naction;
  char c;
  int val;
  int chars;
  float factor = (float)w/100;

  naction = GetNextToken(action, &token);
  if (token == NULL)
    return action;
  orgtoken = token;
  *pFinalX = x;
  *width_factor = 0;
  if (sscanf(token,"o%d%n", &val, &chars) >= 1)
  {
    token += chars;
    *pFinalX += val*factor;
    *width_factor -= val/100;
  }
  else if (token[0] == 'c')
  {
    token++;
    *pFinalX += w/2;
    *width_factor -= 0.5;
  }
  while (*token != 0)
  {
    if (sscanf(token,"%d%n", &val, &chars) >= 1)
    {
      token += chars;
      if (sscanf(token,"%c", &c) == 1)
      {
	if (c == 'm')
	{
	  token++;
	  *width_factor += val/100;
	}
	else if (c == 'p')
	{
	  token++;
	  *pFinalX += val;
	}
	else
	{
	  *pFinalX += val*factor;
	}
      }
      else
      {
	*pFinalX += val*factor;
      }
    }
    else
    {
      naction = action;
      break;
    }
  }
  free(orgtoken);
  return naction;
}

/*****************************************************************************
 * GetMenuOptions is used for Menu, Popup and WindowList
 * It parses strings matching
 *
 *   [ [context-rectangle] x y ] [special-options] [other arguments]
 *
 * and returns a pointer to the first part of the input string that doesn't
 * match this syntax.
 *
 * See documentation for a detailed description.
 ****************************************************************************/
char *GetMenuOptions(char *action, Window w, FvwmWindow *tmp_win,
		     MenuRoot *mr, MenuItem *mi, MenuOptions *pops)
{
  char *tok = NULL, *naction = action, *taction;
  int x, y, button, gflags;
  unsigned int width, height;
  Window context_window = 0;
  Bool fHasContext, fUseItemOffset;
  Bool fValidPosHints =
    last_saved_pos_hints.flags.is_last_menu_pos_hints_valid;

  last_saved_pos_hints.flags.is_last_menu_pos_hints_valid = False;
  if (pops == NULL)
  {
    fvwm_msg(ERR,"GetMenuOptions","no MenuOptions pointer passed");
    return action;
  }

  taction = action;
  memset(&(pops->flags), 0, sizeof(pops->flags));
  pops->flags.has_poshints = 0;
  while (action != NULL)
  {
    /* ^ just to be able to jump to end of loop without 'goto' */
    gflags = NoValue;
    pops->pos_hints.fRelative = False;
    /* parse context argument (if present) */
    naction = GetNextToken(taction, &tok);
    if (!tok)
    {
      /* no context string */
      fHasContext = False;
      break;
    }

    pops->pos_hints.fRelative = True; /* set to False for absolute hints! */
    fUseItemOffset = False;
    fHasContext = True;
    if (StrEquals(tok, "context"))
    {
      if (mi && mr)
	context_window = MR_WINDOW(mr);
      else if (tmp_win)
      {
	if (IS_ICONIFIED(tmp_win))
	  context_window=tmp_win->icon_pixmap_w;
	else
	  context_window = tmp_win->frame;
      }
      else
	context_window = w;
      pops->pos_hints.fRelative = True;
    }
    else if (StrEquals(tok,"menu"))
    {
      if (mi && mr)
	context_window = MR_WINDOW(mr);
    }
    else if (StrEquals(tok,"item"))
    {
      if (mi && mr)
      {
	context_window = MR_WINDOW(mr);
	fUseItemOffset = True;
      }
    }
    else if (StrEquals(tok,"icon"))
    {
      if (tmp_win)
	context_window = tmp_win->icon_pixmap_w;
    }
    else if (StrEquals(tok,"window"))
    {
      if (tmp_win)
	context_window = tmp_win->frame;
    }
    else if (StrEquals(tok,"interior"))
    {
      if (tmp_win)
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
      else if (tmp_win)
      {
	if (button == 0)
	  button = 10;
	if (button & 0x01)
	  context_window = tmp_win->left_w[button/2];
	else
	  context_window = tmp_win->right_w[button/2-1];
      }
    }
    else if (StrEquals(tok,"root"))
    {
      context_window = Scr.Root;
      pops->pos_hints.fRelative = False;
    }
    else if (StrEquals(tok,"mouse"))
    {
      context_window = 0;
    }
    else if (StrEquals(tok,"rectangle"))
    {
      int flags;
      /* parse the rectangle */
      free(tok);
      naction = GetNextToken(taction, &tok);
      if (tok == NULL)
      {
	fvwm_msg(ERR,"GetMenuOptions","missing rectangle geometry");
	return action;
      }
      flags = XParseGeometry(tok, &x, &y, &width, &height);
      if ((flags & AllValues) != AllValues)
      {
	free(tok);
	fvwm_msg(ERR,"GetMenuOptions","invalid rectangle geometry");
	return action;
      }
      if (flags & XNegative) x = Scr.MyDisplayWidth - x - width;
      if (flags & YNegative) y = Scr.MyDisplayHeight - y - height;
      pops->pos_hints.fRelative = False;
    }
    else if (StrEquals(tok,"this"))
    {
      context_window = w;
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

    if (!context_window || !fHasContext
	|| !XGetGeometry(dpy, context_window, &JunkRoot, &JunkX, &JunkY,
			 &width, &height, &JunkBW, &JunkDepth)
	|| !XTranslateCoordinates(
	  dpy, context_window, Scr.Root, 0, 0, &x, &y, &JunkChild))
    {
      /* now window or could not get geometry */
      XQueryPointer(dpy,Scr.Root,&JunkRoot,&JunkChild,&x,&y,&JunkX,&JunkY,
		    &JunkMask);
      width = height = 1;
    }
    else if (fUseItemOffset)
    {
      y += mi->y_offset;
      height = mi->y_height;
    }

    /* parse position arguments */
    taction = GetOneMenuPositionArgument(
      naction, x, width, &(pops->pos_hints.x), &(pops->pos_hints.x_factor));
    naction = GetOneMenuPositionArgument(
      taction, y, height, &(pops->pos_hints.y), &(pops->pos_hints.y_factor));
    if (naction == taction)
    {
      /* argument is missing or invalid */
      if (fHasContext)
	fvwm_msg(ERR,"GetMenuOptions","invalid position arguments");
      naction = action;
      taction = action;
      break;
    }
    taction = naction;
    pops->flags.has_poshints = 1;
    if (fValidPosHints == True && pops->pos_hints.fRelative == True)
    {
      pops->pos_hints = last_saved_pos_hints.pos_hints;
    }
    /* we want to do this only once */
    break;
  } /* while (1) */

  if (!pops->flags.has_poshints && fValidPosHints)
  {
    DBUG("GetMenuOptions","recycling position hints");
    pops->flags.has_poshints = 1;
    pops->pos_hints = last_saved_pos_hints.pos_hints;
    pops->pos_hints.fRelative = False;
  }

  action = naction;
  /* to keep Purify silent */
  pops->flags.select_in_place = 0;
  /* parse additional options */
  while (naction && *naction)
  {
    naction = GetNextToken(action, &tok);
    if (!tok)
      break;
    if (StrEquals(tok, "WarpTitle"))
    {
      pops->flags.warp_title = 1;
      pops->flags.no_warp = 0;
    }
    else if (StrEquals(tok, "NoWarp"))
    {
      pops->flags.warp_title = 0;
      pops->flags.no_warp = 1;
    }
    else if (StrEquals(tok, "Fixed"))
    {
      pops->flags.fixed = 1;
    }
    else if (StrEquals(tok, "SelectInPlace"))
    {
      pops->flags.select_in_place = 1;
    }
    else if (StrEquals(tok, "SelectWarp"))
    {
      pops->flags.select_warp = 1;
    }
    else
    {
      free (tok);
      break;
    }
    action = naction;
    free (tok);
  }
  if (!pops->flags.select_in_place)
  {
    pops->flags.select_warp = 0;
  }

  return action;
}
