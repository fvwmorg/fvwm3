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

static void DrawTrianglePattern(Window,GC,GC,GC,GC,int,int,int,int,char);
static void DrawSeparator(Window, GC,GC,int, int,int,int,int);
static void DrawUnderline(MenuRoot *mr, GC gc, int x, int y,
			  char *txt, int off);
static MenuStatus MenuInteraction(MenuRoot *menu,MenuRoot *menuPrior,
				  MenuItem **pmiExecuteAction,int cmenuDeep,
				  Bool fSticks);
static void WarpPointerToTitle(MenuRoot *menu);
static MenuItem *MiWarpPointerToItem(MenuItem *mi, Bool fSkipTitle);
static void PopDownMenu(MenuRoot *mr);
static void PopDownAndRepaintParent(MenuRoot *mr, Bool *fSubmenuOverlaps);
static int DoMenusOverlap(MenuRoot *mr, int x, int y, int width, int height,
			  Bool fTolerant);
static Bool FPopupMenu(MenuRoot *menu, MenuRoot *menuPrior, int x, int y,
		       Bool fWarpItem, MenuOptions *pops, Bool *ret_overlap);
static void GetPreferredPopupPosition(MenuRoot *mr, int *x, int *y);
static int PopupPositionOffset(MenuRoot *mr);
static void GetPopupOptions(MenuItem *mi, MenuOptions *pops);
static void paint_menu_item(MenuItem *mi);
static void PaintMenu(MenuRoot *, XEvent *);
static void SetMenuItemSelected(MenuItem *mi, Bool f);
static MenuRoot *MrPopupForMi(MenuItem *mi);
static int ButtonPosition(int context, FvwmWindow * t);
static Bool FMenuMapped(MenuRoot *menu);

Bool menuFromFrameOrWindowOrTitlebar = FALSE;
static Bool mouse_moved = FALSE;

extern int Context,Button;
extern FvwmWindow *ButtonWindow, *Tmp_win;
extern XEvent Event;
extern XContext MenuContext;

static FvwmWindow *my_Tmp_win=NULL;

/* dirty patch to pass the last popups position hints to popup_func */
MenuPosHints lastMenuPosHints;
Bool fLastMenuPosHintsValid = FALSE;
static Bool fIgnorePosHints = FALSE;
static Bool fWarpPointerToTitle = FALSE;

/* patch to allow double-keypress (like doubleclick) */
static unsigned int dkp_keystate;
static unsigned int dkp_keycode;
static Time dkp_timestamp;

#define IS_TITLE_MENU_ITEM(mi) (((mi)?((mi)->func_type==F_TITLE):FALSE))
#define IS_POPUP_MENU_ITEM(mi) (((mi)?((mi)->func_type==F_POPUP):FALSE))
#define IS_SEPARATOR_MENU_ITEM(mi) ((mi)?(mi)->fIsSeparator:FALSE)
#define IS_REAL_SEPARATOR_MENU_ITEM(mi) ((mi)?((mi)->fIsSeparator && StrEquals((mi)->action,"nop") && !((mi)->strlen) && !((mi)->strlen2)):FALSE)
#define IS_LABEL_MENU_ITEM(mi) ((mi)?((mi)->fIsSeparator && StrEquals((mi)->action,"nop") && ((mi)->strlen || ((mi)->strlen2))):FALSE)
#define MENU_MIDDLE_OFFSET(menu) ((menu)->xoffset + ((menu)->width - (menu)->xoffset)/2)
#define IS_LEFT_MENU(menu) ((menu)->flags.f.is_left)
#define IS_RIGHT_MENU(menu) ((menu)->flags.f.is_right)
#define IS_UP_MENU(menu) ((menu)->flags.f.is_up)
#define IS_DOWN_MENU(menu) ((menu)->flags.f.is_down)

/****************************************************************************
 *
 * Initiates a menu pop-up
 *
 * fStick = TRUE  = sticky menu, stays up on initial button release.
 * fStick = FALSE = transient menu, drops on initial release.
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
MenuStatus do_menu(MenuRoot *menu, MenuRoot *menuPrior,
		   MenuItem **pmiExecuteAction,int cmenuDeep,Bool fStick,
		   XEvent *eventp, MenuOptions *pops)
{
  MenuStatus retval=MENU_NOP;
  int x,y;
  static int x_start, y_start;
  Bool fFailedPopup = FALSE;
  Bool fWasAlreadyPopped = FALSE;
  Bool key_press;
  Bool fDoubleClick = FALSE;
  Time t0 = lastTimestamp;
  extern Time lastTimestamp;
  static int cindirectDeep = 0;
  XEvent tmpevent;

  if (cmenuDeep == 0 && cindirectDeep == 0)
    my_Tmp_win = Tmp_win;
  DBUG("do_menu","called");

  if (fStick && cmenuDeep == 0)
    XCheckTypedEvent(dpy, ButtonPressMask, &tmpevent);
  key_press = (eventp && (eventp == (XEvent *)1 || eventp->type == KeyPress));
  /* this condition could get ugly */
  if(menu == NULL || menu->flags.f.is_in_use) {
    /* DBUG("do_menu","menu->flags.f.is_in_use for %s -- returning",
	 menu->name); */
    return MENU_ERROR;
  }

  /* Try to pick a root-relative optimal x,y to
     put the mouse right on the title w/o warping */
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x, &y, &JunkX, &JunkY, &JunkMask);
  /* Save these-- we want to warp back here if this is a top level
     menu brought up by a keystroke */
  if (cmenuDeep == 0 && cindirectDeep == 0) {
    if (key_press) {
      x_start = x;
      y_start = y;
    } else {
      x_start = -1;
      y_start = -1;
    }
  }
  /* calculate position from positioning hints */
  if ((pops->flags.f.has_poshints) && !fIgnorePosHints) {
    pops->pos_hints.x += pops->pos_hints.x_factor * menu->width;
    pops->pos_hints.y += pops->pos_hints.y_factor * menu->height;
  }
  /* Figure out where we should popup, if possible */
  if (!FMenuMapped(menu)) {
    if(cmenuDeep > 0) {
      /* this is a submenu popup */
      assert(menuPrior);
      if ((pops->flags.f.has_poshints) && !fIgnorePosHints) {
	x = pops->pos_hints.x;
	y = pops->pos_hints.y;
      } else {
	GetPreferredPopupPosition(menuPrior, &x, &y);
      }
    } else {
      /* we're a top level menu */
      mouse_moved = FALSE;
      if(!GrabEm(MENU)) { /* GrabEm specifies the cursor to use */
	XBell(dpy, 0);
	return MENU_ABORTED;
      }
      if ((pops->flags.f.has_poshints) && !fIgnorePosHints) {
	x = pops->pos_hints.x;
	y = pops->pos_hints.y;
      } else {
	/* Make the menu appear under the pointer rather than warping */
	x -= MENU_MIDDLE_OFFSET(menu);
	y -= menu->ms->look.EntryHeight/2 + 2;
      }
    }

    /* FPopupMenu may move the x,y to make it fit on screen more nicely */
    /* it might also move menuPrior out of the way */
    if (!FPopupMenu (menu, menuPrior, x, y, key_press /*warp*/, pops, NULL)) {
      fFailedPopup = TRUE;
      XBell (dpy, 0);
    }
  }
  else {
    fWasAlreadyPopped = TRUE;
    if (key_press) MiWarpPointerToItem(menu->first, TRUE /* skip Title */);
  }
  fWarpPointerToTitle = FALSE;

  menu->flags.f.is_in_use = TRUE;
  /* Remember the key that popped up the root menu. */
  if (!(cmenuDeep++)) {
    if (eventp && eventp != (XEvent *)1) {
      /* we have a real key event */
      dkp_keystate = eventp->xkey.state;
      dkp_keycode = eventp->xkey.keycode;
    }
    dkp_timestamp = (key_press) ? t0 : 0;
  }
  if (!fFailedPopup)
    retval = MenuInteraction(menu,menuPrior,pmiExecuteAction,cmenuDeep,fStick);
  else
    retval = MENU_ABORTED;
  cmenuDeep--;
  menu->flags.f.is_in_use = FALSE;

  if (!fWasAlreadyPopped)
    PopDownMenu(menu);
  /* FIX: this global is bad */
  menuFromFrameOrWindowOrTitlebar = FALSE;
  XFlush(dpy);

  if (cmenuDeep == 0 && x_start >= 0 && y_start >= 0 &&
      IS_MENU_BUTTON(retval)) {
    /* warp pointer back to where invoked if this was brought up
       with a keypress and we're returning from a top level menu,
       and a button release event didn't end it */
    XWarpPointer(dpy, 0, Scr.Root, 0, 0,
		 Scr.MyDisplayWidth, Scr.MyDisplayHeight,x_start, y_start);
  }

  if (lastTimestamp-t0 < Menus.DoubleClickTime && !mouse_moved &&
      (!key_press || dkp_timestamp != 0)) {
    /* dkp_timestamp is non-zero if a double-keypress occured! */
    fDoubleClick = TRUE;
  }
  dkp_timestamp = 0;
  if(cmenuDeep == 0) {
    UngrabEm();
    WaitForButtonsUp();
    if (retval == MENU_DONE || retval == MENU_DONE_BUTTON) {
      if (pmiExecuteAction && *pmiExecuteAction && !fDoubleClick) {
	cindirectDeep++;
	ExecuteFunction(
	  (*pmiExecuteAction)->action,ButtonWindow, &Event,Context, -1,
	  EXPAND_COMMAND);
	cindirectDeep--;
      }
      fIgnorePosHints = FALSE;
      fLastMenuPosHintsValid = FALSE;
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
MenuItem *FindEntry(int *px_offset /*NULL means don't return this value */)
{
  MenuItem *mi;
  MenuRoot *mr;
  int root_x, root_y;
  int x,y;
  Window Child;

  /* x_offset returns the x offset of the pointer in the found menu item */
  if (px_offset)
    *px_offset = 0;

  XQueryPointer( dpy, Scr.Root, &JunkRoot, &Child,
		&root_x,&root_y, &JunkX, &JunkY, &JunkMask);
  if (XFindContext (dpy, Child,MenuContext,(caddr_t *)&mr)==XCNOENT) {
    return NULL;
  }

  /* now get position in that child window */
  XQueryPointer( dpy, Child, &JunkRoot, &JunkChild,
		&root_x,&root_y, &x, &y, &JunkMask);

  /* look for the entry that the mouse is in */
  for(mi=mr->first; mi; mi=mi->next)
    if(y>=mi->y_offset && y<=mi->y_offset+mi->y_height)
      break;
  if(x<mr->xoffset || x>mr->width+2)
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
  return (mr->width * mr->ms->feel.PopupOffsetPercent / 100 +
	  mr->ms->feel.PopupOffsetAdd);
}

static
void GetPreferredPopupPosition(MenuRoot *mr, int *px, int *py)
{
  int menu_x, menu_y;
  XGetGeometry(dpy,mr->w,&JunkRoot,&menu_x,&menu_y,
	       &JunkWidth,&JunkHeight,&JunkBW,&JunkDepth);
  *px = menu_x + PopupPositionOffset(mr);
  *py = menu_y;
  if(mr->selected) {
    /* *py = mr->selected->y_offset + menu_y - (mr->ms->look.EntryHeight/2); */
    *py = mr->selected->y_offset + menu_y;
  }
}


static
int IndexFromMi(MenuItem *miTarget)
{
  int i = 0;
  MenuRoot *mr = miTarget->mr;
  MenuItem *mi = mr->first;
  for (; mi && mi != miTarget; mi = mi->next) {
    if (!IS_TITLE_MENU_ITEM(mi) && !IS_SEPARATOR_MENU_ITEM(mi))
      i++;
  }
  if (mi == miTarget) {
    /* DBUG("IndexFromMi","%s = %d",miTarget->item,i); */
    return i;
  }
  return -1;
}

static
Bool FMenuMapped(MenuRoot *menu)
{
  XWindowAttributes win_attribs;
  XGetWindowAttributes(dpy,menu->w,&win_attribs);
  return (menu->w == None) ? False : (win_attribs.map_state == IsViewable);
}

static
MenuItem *MiFromMenuIndex(MenuRoot *mr, int index)
{
  int i = -1;
  MenuItem *mi = mr->first;
  MenuItem *miLastOk = NULL;
  for (; mi && (i < index || miLastOk == NULL); mi=mi->next) {
    if (!IS_TITLE_MENU_ITEM(mi) && !IS_SEPARATOR_MENU_ITEM(mi)) {
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
  return IndexFromMi(mr->last);
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
MenuStatus menuShortcuts(MenuRoot *menu,XEvent *Event,MenuItem **pmiCurrent)
{
  int fControlKey = Event->xkey.state & ControlMask? TRUE : FALSE;
  int fShiftedKey = Event->xkey.state & ShiftMask? TRUE: FALSE;
  KeySym keysym;
  char keychar;
  MenuItem *newItem = NULL;
  MenuItem *miCurrent = pmiCurrent?*pmiCurrent:NULL;
  int index;

  /* handle double-keypress */
  if (dkp_timestamp &&
      lastTimestamp-dkp_timestamp < Menus.DoubleClickTime &&
      Event->xkey.state == dkp_keystate && Event->xkey.keycode == dkp_keycode){
    *pmiCurrent = NULL;
    return MENU_SELECTED;
  }
  dkp_timestamp = 0;
  /* Is it okay to treat keysym-s as Ascii? */
  /* No, because the keypad numbers don't work. Use XlookupString */
  index = XLookupString(&(Event->xkey), &keychar, 1, &keysym, NULL);
  /* Try to match hot keys */
  /* Need isascii here - isgraph might coredump! */
  if (index == 1 && isascii((int)keychar) && isgraph((int)keychar) &&
      fControlKey == FALSE) {
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
    mi = ( miCurrent == NULL || miCurrent == menu->last) ?
      menu->first : miCurrent->next;
    mi1 = mi;
    do
    {
      key = tolower( mi->chHotkey );
      if ( keychar == key )
      {
	if ( ++countHotkey == 1 )
	  newItem = mi;

      }
      mi = (mi == menu->last) ? menu->first : mi->next;
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
      if ( IS_POPUP_MENU_ITEM( newItem ) )
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
      if (IS_POPUP_MENU_ITEM(miCurrent))
	return MENU_POPUP;
      break;

    case XK_Up:
    case XK_KP_8:
    case XK_k: /* vi up */
    case XK_p: /* prior */
      if (!miCurrent) {
        if ((*pmiCurrent = MiFromMenuIndex(menu,0)) != NULL)
	  return MENU_NEWITEM;
	else
	  return MENU_NOP;
      }
      /* Need isascii here - isgraph might coredump! */
      if (isascii(keysym) && isgraph(keysym))
	fControlKey = FALSE; /* don't use control modifier
				for k or p, since those might
				be shortcuts too-- C-k, C-p will
				always work to do a single up */
      index = IndexFromMi(miCurrent);
      if (index == 0)
	/* wraparound */
	index = CmiFromMenu(miCurrent->mr);
      else if (fShiftedKey)
	index = 0;
      else {
	index -= (fControlKey?5:1);
      }
      newItem = MiFromMenuIndex(miCurrent->mr,index);
      if (newItem) {
	*pmiCurrent = newItem;
	return MENU_NEWITEM;
      } else
	return MENU_NOP;
      break;

    case XK_Down:
    case XK_KP_2:
    case XK_j: /* vi down */
    case XK_n: /* next */
      if (!miCurrent) {
        if ((*pmiCurrent = menu->last) != NULL)
	  return MENU_NEWITEM;
	else
	  return MENU_NOP;
      }
      /* Need isascii here - isgraph might coredump! */
      if (isascii(keysym) && isgraph(keysym))
	fControlKey = FALSE; /* don't use control modifier
				for j or n, since those might
				be shortcuts too-- C-j, C-n will
				always work to do a single down */
      if (fShiftedKey)
	index = CmiFromMenu(miCurrent->mr);
      else {
	index = IndexFromMi(miCurrent) + (fControlKey?5:1);
	/* correct for the case that we're between items */
	if (IS_SEPARATOR_MENU_ITEM(miCurrent) ||
	    IS_TITLE_MENU_ITEM(miCurrent))
	  index--;
      }
      newItem = MiFromMenuIndex(miCurrent->mr,index);
      if (newItem == miCurrent)
	newItem = MiFromMenuIndex(miCurrent->mr,0);
      if (newItem) {
	*pmiCurrent = newItem;
	return MENU_NEWITEM;
      } else
	return MENU_NOP;
      break;

    case XK_Page_Up:
      if ((*pmiCurrent = MiFromMenuIndex(menu,0)) != NULL)
	return MENU_NEWITEM;
      else
	return MENU_NOP;
      break;

    case XK_Page_Down:
      if ((*pmiCurrent = menu->last) != NULL)
	return MENU_NEWITEM;
      else
	return MENU_NOP;
      break;

      /* Nothing special --- Allow other shortcuts */
    default:
      /* There are no useful shortcuts, so don't do that.
       * (Dominik Vogt, 11-Nov-1998)
       * Keyboard_shortcuts(Event, NULL, ButtonRelease); */
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
 *      MENU_SELECTED on item selection -- returns MenuItem * in
 *                    *pmiExecuteAction
 *
 ***********************************************************************/
static
MenuStatus MenuInteraction(MenuRoot *menu,MenuRoot *menuPrior,
			   MenuItem **pmiExecuteAction,int cmenuDeep,
			   Bool fSticks)
{
  Bool fPopupImmediately;
  MenuItem *mi = NULL, *tmi;
  MenuRoot *mrPopup = NULL;
  MenuRoot *mrMiPopup = NULL;
  MenuRoot *mrNeedsPainting = NULL;
  Bool fDoPopupNow = FALSE; /* used for delay popups, to just popup the menu */
  Bool fPopupAndWarp = FALSE; /* used for keystrokes, to popup and move to
			       * that menu */
  Bool fKeyPress = FALSE;
  Bool fForceReposition = TRUE;
  int x_init = 0, y_init = 0;
  int x_offset = 0;
  MenuStatus retval = MENU_NOP;
  int c10msDelays = 0;
  MenuOptions mops;
  Bool fOffMenuAllowed = FALSE;
  Bool fPopdown = FALSE;
  Bool fPopup   = FALSE;
  Bool fDoMenu  = FALSE;
  Bool fMotionFirst = FALSE;
  Bool fReleaseFirst = FALSE;
  Bool fFakedMotion = FALSE;
  Bool fSubmenuOverlaps = False;

  mops.flags.allflags = 0;
  fPopupImmediately = (menu->ms->feel.f.PopupImmediately &&
		       (Menus.PopupDelay10ms > 0));

  /* remember where the pointer was so we can tell if it has moved */
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		 &x_init, &y_init, &JunkX, &JunkY, &JunkMask);

  while (TRUE) {
      fPopupAndWarp = FALSE;
      fDoPopupNow = FALSE;
      fKeyPress = FALSE;
      if (fForceReposition) {
	Event.type = MotionNotify;
	Event.xmotion.time = lastTimestamp;
	fFakedMotion = TRUE;
	fForceReposition = FALSE;
      } else if (!XCheckMaskEvent(dpy,ExposureMask,&Event)) {
	/* handle exposure events first */
	if (Menus.PopupDelay10ms > 0) {
	  while (XCheckMaskEvent(dpy,
				 ButtonPressMask|ButtonReleaseMask|
				 ExposureMask|KeyPressMask|
				 VisibilityChangeMask|ButtonMotionMask,
				 &Event) == FALSE) {
	    usleep(10000 /* 10 ms*/);
	    if (c10msDelays++ == Menus.PopupDelay10ms) {
	      DBUG("MenuInteraction","Faking motion");
	      /* fake a motion event, and set fDoPopupNow */
	      Event.type = MotionNotify;
	      Event.xmotion.time = lastTimestamp;
	      fFakedMotion = TRUE;
	      fDoPopupNow = TRUE;
	      break;
	    }
	  }
	} else {  /* block until there is an event */
	  XMaskEvent(dpy,
		     ButtonPressMask|ButtonReleaseMask|ExposureMask |
		     KeyPressMask|VisibilityChangeMask|ButtonMotionMask,
		     &Event);
	}
      }
      /*DBUG("MenuInteraction","mrPopup=%s",mrPopup?mrPopup->name:"(none)");*/

      StashEventTime(&Event);
      if (Event.type == MotionNotify) {
	/* discard any extra motion events before a release */
	while((XCheckMaskEvent(dpy,ButtonMotionMask|ButtonReleaseMask,
			       &Event))&&(Event.type != ButtonRelease))
	  ;
      }

      retval = 0;
      switch(Event.type)
	{
	case ButtonRelease:
	  mi = FindEntry(&x_offset);
	  /* hold the menu up when the button is released
	     for the first time if released OFF of the menu */
	  if(fSticks && !fMotionFirst) {
	    fReleaseFirst = TRUE;
	    fSticks = FALSE;
	    continue;
	    /* break; */
	  }
	  retval = mi?MENU_SELECTED:MENU_ABORTED;
	  if (retval == MENU_SELECTED && IS_POPUP_MENU_ITEM(mi) &&
	      !menu->ms->feel.f.PopupAsRootmenu)
	    {
	      retval = MENU_POPUP;
	      fDoPopupNow = TRUE;
	      break;
	    }
	  dkp_timestamp = 0;
	  goto DO_RETURN;

	case ButtonPress:
	  /* if the first event is a button press allow the release to
	     select something */
	  fSticks = FALSE;
	  continue;

	case VisibilityNotify:
	  continue;

	case KeyPress:
	  /* Handle a key press events to allow mouseless operation */
	  fKeyPress = TRUE;
	  x_offset = 0;
	  retval = menuShortcuts(menu,&Event,&mi);
	  if (retval == MENU_SELECTED && IS_POPUP_MENU_ITEM(mi) &&
	      !menu->ms->feel.f.PopupAsRootmenu)
	    retval = MENU_POPUP;
	  if (retval == MENU_POPDOWN ||
	      retval == MENU_ABORTED ||
	      retval == MENU_SELECTED)
	    goto DO_RETURN;
	  /* now warp to the new menu-item, if any */
	  if (mi && mi != FindEntry(NULL)) {
	    MiWarpPointerToItem(mi,FALSE);
	    /* DBUG("MenuInteraction","Warping on keystroke to %s",mi->item);*/
	  }
	  if (retval == MENU_POPUP && IS_POPUP_MENU_ITEM(mi)) {
	    fPopupAndWarp = TRUE;
	    DBUG("MenuInteraction","fPopupAndWarp = TRUE");
	    break;
	  }
	  break;

	case MotionNotify:
	  if (mouse_moved == FALSE) {
	    int x_root, y_root;
	    XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
			   &x_root,&y_root, &JunkX, &JunkY, &JunkMask);
	    if(x_root-x_init > 3 || x_init-x_root > 3 ||
	       y_root-y_init > 3 || y_init-y_root > 3) {
	      /* global variable remember that this isn't just
		 a click any more since the pointer moved */
	      mouse_moved = TRUE;
	    }
	  }
	  mi = FindEntry(&x_offset);
	  if (!fReleaseFirst && !fFakedMotion && mouse_moved)
	    fMotionFirst = TRUE;
	  fFakedMotion = FALSE;
	  break;

	case Expose:
	  /* grab our expose events, let the rest go through */
	  if((XFindContext(dpy, Event.xany.window,MenuContext,
			   (caddr_t *)&mrNeedsPainting)!=XCNOENT)) {
	    flush_expose(Event.xany.window);
	    PaintMenu(mrNeedsPainting,&Event);
	  }
	  /* continue; */ /* instead of continuing, we want to
	     dispatch this too by letting it fall
	     through so window decorations get redrawn
	     after being obscured by menus */
	  DispatchEvent();
	  continue;

	default:
	  DispatchEvent();
	  break;
	} /* switch (Event.type) */

      /* Now handle new menu items, whether it is from a keypress or
	 a pointer motion event */
      if (mi) {
	/* we're on a menu item */
	fOffMenuAllowed = FALSE;
	mrMiPopup = MrPopupForMi(mi);
	if (mi->mr != menu && mi->mr != mrPopup && mi->mr != mrMiPopup) {
	  /* we're on an item from a prior menu */
	  /* DBUG("MenuInteraction","menu %s: returning popdown",menu->name);*/
	  retval = MENU_POPDOWN;
	  dkp_timestamp = 0;
	  goto DO_RETURN;
	}

	/* see if we're on a new item of this menu */
	if (mi != menu->selected && mi->mr == menu) {
	  c10msDelays = 0;
	  /* we're on the same menu, but a different item,
	     so we need to unselect the old item */
	  if (menu->selected) {
	    /* something else was already selected on this menu */
	    if (mrPopup) {
	      PopDownAndRepaintParent(mrPopup, &fSubmenuOverlaps);
	      mrPopup = NULL;
	    }
	    /* We have to pop down the menu before unselecting the item in case
	     * we are using gradient menus. The recalled image would paint over
	     * the submenu. */
	    SetMenuItemSelected(menu->selected,FALSE);
	  } else {
	    /* DBUG("MenuInteraction","Menu %s had nothing else selected",
	       menu->name); */
	  }
	  /* do stuff to highlight the new item; this
	     sets menu->selected, too */
	  SetMenuItemSelected(mi,TRUE);
	} /* new item of the same menu */

	/* check what has to be done with the item */
	fPopdown = FALSE;
	fPopup   = FALSE;
	fDoMenu  = FALSE;

	if (mi->mr == mrPopup) {
	  /* must make current popup menu a real menu */
	  fDoMenu = TRUE;
	}
	else if (fPopupAndWarp) {
	  /* must create a real menu and warp into it */
	  if (mrPopup == NULL || mrPopup != mrMiPopup) {
	    fPopup = TRUE;
	  } else {
	    XRaiseWindow(dpy, mrPopup->w);
	    MiWarpPointerToItem(mrPopup->first,TRUE);
	    fDoMenu = TRUE;
	  }
	}
	else if (IS_POPUP_MENU_ITEM(mi)) {
	  if (x_offset >= mi->mr->width*3/4 || fDoPopupNow ||
	      fPopupImmediately) {
	    /* must create a new menu or popup */
	    if (mrPopup == NULL || mrPopup != mrMiPopup)
	      fPopup = TRUE;
	    else if (fPopupAndWarp)
	      MiWarpPointerToItem(mrPopup->first,TRUE);
	  }
	} /* else if (IS_POPUP_MENU_ITEM(mi)) */
	if (fPopup && mrPopup && mrPopup != mrMiPopup) {
	  /* must remove previous popup first */
	  fPopdown = TRUE;
	}

	if (fPopdown) {
	  DBUG("MenuInteraction","Popping down");
	  /* popdown previous popup */
	  if (mrPopup) {
	    PopDownAndRepaintParent(mrPopup, &fSubmenuOverlaps);
	  }
	  mrPopup = NULL;
	  fPopdown = FALSE;
	}
	if (fPopup) {
	  DBUG("MenuInteraction","Popping up");
          /* get pos hints for item's action */
          GetPopupOptions(mi,&mops);
	  mrPopup = mrMiPopup;
	  if (!mrPopup) {
	    fDoMenu = FALSE;
	    fPopdown = FALSE;
	  } else {
	    /* create a popup menu */
	    if (!FMenuMapped(mrPopup)) {
	      /* We want to pop prepop menus so it doesn't *have* to be
		 unpopped; do_menu pops down any menus it pops up, but we
		 want to be able to popdown w/o actually removing the menu */
	      int x, y;
	      if (mops.flags.f.has_poshints) {
		x = mops.pos_hints.x + mops.pos_hints.x_factor*mrPopup->width;
		y = mops.pos_hints.y + mops.pos_hints.y_factor*mrPopup->height;
	      } else {
		GetPreferredPopupPosition(menu,&x,&y);
	      }
	      FPopupMenu(mrPopup, menu, x, y, fPopupAndWarp, &mops,
			 &fSubmenuOverlaps);
	    }
	    if (mrPopup->mrDynamicPrev == menu) {
	      mi = FindEntry(NULL);
	      if (mi && mi->mr == mrPopup) {
		fDoMenu = TRUE;
		fPopdown = !menu->ms->feel.f.PopupImmediately;
	      }
	    }
	    else {
	      /* This menu must be already mapped somewhere else, so ignore
	       * it completely. */
	      fDoMenu = FALSE;
	      fPopdown = FALSE;
	      mrPopup = NULL;
	    }
	  } /* if (!mrPopup) */
	} /* if (fPopup) */
	if (fDoMenu) {
	  /* recursively do the new menu we've moved into */
	  retval = do_menu(mrPopup,menu,pmiExecuteAction,cmenuDeep,FALSE,
			   (fPopupAndWarp) ? (XEvent *)1 : NULL, &mops);
	  if (IS_MENU_RETURN(retval)) {
	    dkp_timestamp = 0;
	    goto DO_RETURN;
	  }
	  if (fPopdown || !menu->ms->feel.f.PopupImmediately) {
	    PopDownAndRepaintParent(mrPopup, &fSubmenuOverlaps);
	    mrPopup = NULL;
	  }
	  if (retval == MENU_POPDOWN) {
	    c10msDelays = 0;
	    fForceReposition = TRUE;
	  }
	} /* if (fDoMenu) */

	/* Now check whether we can animate the current popup menu
	   over to the right to unobscure the current menu;  this
	   happens only when using animation */
	tmi = FindEntry(NULL);
	if (mrPopup && mrPopup->xanimation && tmi &&
	    (tmi == menu->selected || tmi->mr != menu)) {
	  int x_popup, y_popup;
	  DBUG("MenuInteraction","Moving the popup menu back over");
	  XGetGeometry(dpy, mrPopup->w, &JunkRoot, &x_popup, &y_popup,
		       &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	  /* move it back */
	  AnimatedMoveOfWindow(mrPopup->w,x_popup,y_popup,
			       x_popup-mrPopup->xanimation, y_popup,
			       FALSE /* no warp ptr */,-1,NULL);
	  mrPopup->xanimation = 0;
	}
	/* now check whether we should animate the current real menu
	   over to the right to unobscure the prior menu; only a very
	   limited case where this might be helpful and not too disruptive */
	if (mrPopup == NULL && menuPrior != NULL && menu->xanimation != 0 &&
	    x_offset < menu->width/4) {
	  int x_menu, y_menu;
	  DBUG("MenuInteraction","Moving the menu back over");
	  /* we have to see if we need menu to be moved */
	  XGetGeometry( dpy, menu->w, &JunkRoot, &x_menu, &y_menu,
			&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	  /* move it back */
	  AnimatedMoveOfWindow(menu->w,x_menu,y_menu,
			       x_menu - menu->xanimation,y_menu,
			       TRUE /* warp ptr */,-1,NULL);
	  menu->xanimation = 0;
	}

      } /* if (mi) */
      else {
        /* moved off menu, deselect selected item... */
        if (menu->selected) {
          SetMenuItemSelected(menu->selected,FALSE);
          if (mrPopup && fOffMenuAllowed == FALSE) {
	    int x, y, mx, my;
	    unsigned int mw, mh;
	    XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
			   &x, &y, &JunkX, &JunkY, &JunkMask);
	    XGetGeometry( dpy, menu->w, &JunkRoot, &mx, &my,
			  &mw, &mh, &JunkBW, &JunkDepth);
	    if ((!IS_LEFT_MENU(mrPopup)  && x < mx)    ||
		(!IS_RIGHT_MENU(mrPopup) && x > mx+mw) ||
		(!IS_UP_MENU(mrPopup)    && y < my)    ||
		(!IS_DOWN_MENU(mrPopup)  && y > my+mh)) {
	      PopDownAndRepaintParent(mrPopup, &fSubmenuOverlaps);
	      mrPopup = NULL;
	    } else {
	      fOffMenuAllowed = TRUE;
	    }
	  } /* if (mrPopup && fOffMenuAllowed == FALSE) */
        } /* if (menu->selected) */
      } /* else (!mi) */
      XFlush(dpy);
    } /* while (TRUE) */

  DO_RETURN:
  if (mrPopup) {
    PopDownAndRepaintParent(mrPopup, &fSubmenuOverlaps);
  }
  if (retval == MENU_POPDOWN) {
    if (menu->selected)
      SetMenuItemSelected(menu->selected,FALSE);
    if (fKeyPress) {
      if (cmenuDeep == 1)
	/* abort a root menu rather than pop it down */
	retval = MENU_ABORTED;
      if (menuPrior && menuPrior->selected) {
	MiWarpPointerToItem(menuPrior->selected, FALSE);
	if (menuPrior->selected != FindEntry(NULL) &&
	    menu->xanimation == 0) {
	  XRaiseWindow(dpy, menuPrior->w);
	}
      }
    }
    /* DBUG("MenuInteraction","Prior menu has %s selected",
       menuPrior?(menuPrior->selected?
       menuPrior->selected->item:"(no selected item)"):"(no prior menu)"); */
  } else if (retval == MENU_SELECTED) {
    /* DBUG("MenuInteraction","Got MENU_SELECTED for menu %s, on item %s",
       menu->name,mi->item); */
    *pmiExecuteAction = mi;
    retval = MENU_ADD_BUTTON_IF(fKeyPress,MENU_DONE);
  }
  if ((retval == MENU_DONE || retval == MENU_DONE_BUTTON) &&
      pmiExecuteAction && *pmiExecuteAction && (*pmiExecuteAction)->action) {
    switch ((*pmiExecuteAction)->func_type)
    {
    case F_POPUP:
    case F_STAYSUP:
    case F_WINDOWLIST:
      GetPopupOptions(mi, &mops);
      if (!(mops.flags.f.select_in_place)) {
	fIgnorePosHints = TRUE;
      } else {
	if (mops.flags.f.has_poshints) {
	  lastMenuPosHints = mops.pos_hints;
	} else {
	  GetPreferredPopupPosition(
	    menu, &lastMenuPosHints.x, &lastMenuPosHints.y);
	  lastMenuPosHints.x_factor = 0;
	  lastMenuPosHints.y_factor = 0;
	  lastMenuPosHints.fRelative = FALSE;
	}
	fLastMenuPosHintsValid = TRUE;
	if (mops.flags.f.select_warp) {
	  fWarpPointerToTitle = TRUE;
	}
      } /* else (mops.flags.f.select_in_place) */
      break;
    default:
      break;
    }
  } /* ((retval == MENU_DONE ||... */
  return MENU_ADD_BUTTON_IF(fKeyPress,retval);
}

static
void WarpPointerToTitle(MenuRoot *menu)
{
  int y = menu->ms->look.EntryHeight/2 + 2;
  int x = MENU_MIDDLE_OFFSET(menu);
  XWarpPointer(dpy, 0, menu->w, 0, 0, 0, 0, x, y);
}

static
MenuItem *MiWarpPointerToItem(MenuItem *mi, Bool fSkipTitle)
{
  MenuRoot *menu = mi->mr;
  int y;
  int x = MENU_MIDDLE_OFFSET(menu);

  if (fSkipTitle && IS_TITLE_MENU_ITEM(mi) &&
      /* also don't skip if there is no next item */
      mi->next != NULL) {
    mi = mi->next;
    /* Shouldn't ever have a separator right after a title, but
       lets skip one if we do, anyway */
    if (IS_SEPARATOR_MENU_ITEM(mi) && mi->next != NULL)
      mi = mi->next;
  }

  y = mi->y_offset + menu->ms->look.EntryHeight/2;
  XWarpPointer(dpy, 0, menu->w, 0, 0, 0, 0, x, y);
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
      if (mr->ms->feel.PopupOffsetAdd < 0)
	tolerance1 -= mr->ms->feel.PopupOffsetAdd;
      tolerance2 = 4;
    }
  else
    {
      tolerance1 = 1;
      tolerance2 = 1;
    }
  XGetGeometry(dpy,mr->w,&JunkRoot,&prior_x,&prior_y,
	       &prior_width,&prior_height,&JunkBW,&JunkDepth);
  x_overlap = 0;
  if (fTolerant) {
    /* Don't use multiplier if doing an intolerant check */
    prior_width *= (float)(mr->ms->feel.PopupOffsetPercent) / 100.0;
  }
  if (y <= prior_y + prior_height - tolerance2 &&
      prior_y <= y + height - tolerance2 &&
      x <= prior_x + prior_width - tolerance1 &&
      prior_x <= x + width - tolerance2) {
    x_overlap = x - prior_x;
    if (x <= prior_x) {
      x_overlap--;
    }
  }
  return x_overlap;
}

/***********************************************************************
 *
 *  Procedure:
 *	FPopupMenu - pop up a pull down menu
 *
 *  Inputs:
 *	menu	  - the root pointer of the menu to pop up
 *	x, y	  - location of upper left of menu
 *      fWarpItem - warp pointer to the first item after title
 *      pops      - pointer to the menu options for new menu
 *
 ***********************************************************************/
static
Bool FPopupMenu(MenuRoot *menu, MenuRoot *menuPrior, int x, int y,
		Bool fWarpItem, MenuOptions *pops, Bool *ret_overlap)
{
  Bool fWarpTitle = FALSE;
  int x_overlap, x_clipped_overlap;
  MenuItem *mi = NULL;

  DBUG("FPopupMenu","called");
  if ((!menu)||(menu->w == None)||(menu->items == 0)||
      (menu->flags.f.is_in_use)) {
    fWarpPointerToTitle = FALSE;
    return False;
  }
  menu->mrDynamicPrev = menuPrior;
  menu->flags.f.painted = 0;
  menu->flags.f.is_left = 0;
  menu->flags.f.is_right = 0;
  menu->flags.f.is_up = 0;
  menu->flags.f.is_down = 0;
  menu->xanimation = 0;

  /*  RepaintAlreadyReversedMenuItems(menu); */

  InstallRootColormap();

  /* First handle popups from button clicks on buttons in the title bar,
     or the title bar itself. Position hints override this. */
  if (!(pops->flags.f.has_poshints)) {
    if((Tmp_win)&&(menuPrior == NULL)&&(Context&C_LALL))
      {
	y = Tmp_win->frame_y+Tmp_win->boundary_width+Tmp_win->title_height+1;
	x = Tmp_win->frame_x + Tmp_win->boundary_width +
	  ButtonPosition(Context,Tmp_win)*Tmp_win->title_height+1;
      }
    if((Tmp_win)&&(menuPrior == NULL)&&(Context&C_RALL))
      {
	y = Tmp_win->frame_y+Tmp_win->boundary_width+Tmp_win->title_height+1;
	x = Tmp_win->frame_x +Tmp_win->frame_width - Tmp_win->boundary_width-
	  ButtonPosition(Context,Tmp_win)*Tmp_win->title_height-menu->width+1;
      }
    if((Tmp_win)&&(menuPrior == NULL)&&(Context&C_TITLE))
      {
	y = Tmp_win->frame_y+Tmp_win->boundary_width+Tmp_win->title_height+1;
	if(x < Tmp_win->frame_x + Tmp_win->title_x)
	  x = Tmp_win->frame_x + Tmp_win->title_x;
	if((x + menu->width) >
	   (Tmp_win->frame_x + Tmp_win->title_x +Tmp_win->title_width))
	  x = Tmp_win->frame_x + Tmp_win->title_x +Tmp_win->title_width-
	    menu->width +1;
      }
  } /* if (pops->flags.f.has_poshints) */
  x_overlap = DoMenusOverlap(menuPrior, x, y, menu->width, menu->height, True);
  /* clip to screen */
  if (x + menu->width > Scr.MyDisplayWidth - 2)
    x = Scr.MyDisplayWidth - 2 - menu->width;
  if (y + menu->height > Scr.MyDisplayHeight)
    y = Scr.MyDisplayHeight - menu->height;
  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;

  if (menuPrior != NULL) {
    int prev_x, prev_y, left_x, right_x;
    unsigned int prev_width, prev_height;
    int x_offset;

    /* try to find a better place */
    XGetGeometry(dpy,menuPrior->w,&JunkRoot,&prev_x,&prev_y,
		 &prev_width,&prev_height,&JunkBW,&JunkDepth);

    /* check if menus overlap */
    x_clipped_overlap = DoMenusOverlap(menuPrior, x, y, menu->width,
				       menu->height, True);
    if (x_clipped_overlap &&
	(!(pops->flags.f.has_poshints) ||
	 pops->pos_hints.fRelative == FALSE || x_overlap == 0)) {
      /* menus do overlap, but do not reposition if overlap was caused by
	 relative positioning hints */
      Bool fDefaultLeft;
      Bool fEmergencyLeft;

      x_offset = prev_width * menuPrior->ms->feel.PopupOffsetPercent / 100 +
	menuPrior->ms->feel.PopupOffsetAdd;
      left_x = prev_x - menu->width + 2;
      right_x = prev_x + x_offset;
      if (x_offset > prev_width - 2)
	right_x = prev_x + prev_width - 2;
      if (x + menu->width < prev_x + right_x)
	fDefaultLeft = TRUE;
      else
	fDefaultLeft = FALSE;
      fEmergencyLeft = (prev_x > Scr.MyDisplayWidth - right_x) ? TRUE : FALSE;

      if (menu->ms->feel.f.Animated) {
	/* animate previous out of the way */
	int left_x, right_x, end_x;

	left_x = x - x_offset;
	if (x_offset >= prev_width)
	  left_x = x - x_offset + 3;
	right_x = x + menu->width;
	if (fDefaultLeft) {
	  /* popup menu is left of old menu, try to move prior menu right */
	  if (right_x + prev_width <=  Scr.MyDisplayWidth - 2)
	    end_x = right_x;
	  else if (left_x >= 0)
	    end_x = left_x;
	  else
	    end_x = Scr.MyDisplayWidth - 2 - prev_width;
	} else {
	  /* popup menu is right of old menu, try to move prior menu left */
	  if (left_x >= 0)
	    end_x = left_x;
	  else if (right_x + prev_width <=  Scr.MyDisplayWidth - 2)
	    end_x = right_x;
	  else
	    end_x = 0;
	}
	menuPrior->xanimation += end_x - prev_x;
	AnimatedMoveOfWindow(menuPrior->w,prev_x,prev_y,end_x,prev_y,
			     TRUE, -1, NULL);
      } /* if (menu->ms->feel.f.Animated) */
      else if (prev_x + x_offset > x + 3 && x_offset + 3 > -menu->width &&
	       !(pops->flags.f.fixed)) {
	Bool fLeftIsOK = FALSE;
	Bool fRightIsOK = FALSE;
	Bool fUseLeft = FALSE;

	if (left_x >= 0)
	  fLeftIsOK = TRUE;
	if (right_x + menu->width < Scr.MyDisplayWidth - 2)
	  fRightIsOK = TRUE;
	if (!fLeftIsOK && !fRightIsOK)
	  fUseLeft = fEmergencyLeft;
	else if (fLeftIsOK && (fDefaultLeft || !fRightIsOK))
	  fUseLeft = TRUE;
	else
	  fUseLeft = FALSE;
	x = (fUseLeft) ? left_x : right_x;
	/* force the menu onto the screen; prefer to have the left border
	 * visible if the menu is wider than the screen. But leave at least
	 * 20 pixels of the parent menu visible */

	if (x + menu->width >= Scr.MyDisplayWidth - 2)
	{
	  int d = x + menu->width - Scr.MyDisplayWidth + 3;
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
      menu->flags.f.is_left = 1;
    if (x + menu->width > prev_x + prev_width)
      menu->flags.f.is_right = 1;
    if (y < prev_y)
      menu->flags.f.is_up = 1;
    if (y + menu->height > prev_y + prev_height)
      menu->flags.f.is_down = 1;
    if (!menu->flags.f.is_left && !menu->flags.f.is_right)
    {
      menu->flags.f.is_left = 1;
      menu->flags.f.is_right = 1;
    }
  } /* if (menuPrior) */

  /* popup the menu */
  XMoveWindow(dpy, menu->w, x, y);
  XMapRaised(dpy, menu->w);
  XFlush(dpy);
  if (ret_overlap) {
    *ret_overlap =
      DoMenusOverlap(menuPrior, x, y, menu->width, menu->height, False) ?
      True : False;
  }

  if (!fWarpItem) {
    mi = FindEntry(NULL);
    if (mi && mi->mr == menu && mi != mi->mr->first) {
      /* pointer is on an item of the popup */
      if (menu->ms->feel.f.TitleWarp) {
	/* warp pointer if not on a root menu and MWM/WIN menu style */
	fWarpTitle = TRUE;
      }
    }
  } /* if (!fWarpItem) */

  if (pops->flags.f.no_warp) {
    fWarpTitle = FALSE;
  } else if (pops->flags.f.warp_title) {
    fWarpTitle = TRUE;
  }
  if (fWarpPointerToTitle) {
    fWarpTitle = TRUE;
    fWarpPointerToTitle = FALSE;
  }
  if (fWarpItem) {
    /* also warp */
    DBUG("FPopupMenu","Warping to item");
    menu->selected = MiWarpPointerToItem(menu->first, TRUE /* skip Title */);
    SetMenuItemSelected(
      MiWarpPointerToItem(menu->first, TRUE /* skip Title */),TRUE);
  } else if(fWarpTitle) {
    /* Warp pointer to middle of top line, since we don't
     * want the user to come up directly on an option */
    DBUG("FPopupMenu","Warping to title");
    WarpPointerToTitle(menu);
  }
  return True;
}

/* Set the selected-ness state of the menuitem passed in */
static
void SetMenuItemSelected(MenuItem *mi, Bool f)
{
  if (f == True && mi->mr->selected != NULL && mi->mr->selected != mi)
    SetMenuItemSelected(mi->mr->selected, False);
  if (f == False && mi->mr->selected == NULL)
    return;

  if (mi->state == f)
    return;

#ifdef GRADIENT_BUTTONS
  switch (mi->mr->ms->look.face.type)
  {
  case HGradMenu:
  case VGradMenu:
  case DGradMenu:
  case BGradMenu:
    if (f == True)
    {
      int iy, ih;
      unsigned int mw, mh;

      if (!mi->mr->flags.f.painted)
      {
	PaintMenu(mi->mr, NULL);
	flush_expose(mi->mr->w);
      }
      iy = mi->y_offset - 2;
      ih = mi->y_height + 4;
      if (iy < 0)
      {
	ih += iy;
	iy = 0;
      }
      XGetGeometry(dpy, mi->mr->w, &JunkRoot, &JunkX, &JunkY,
		   &mw, &mh, &JunkBW, &JunkDepth);
      if (iy + ih > mh)
	ih = mh - iy;
      /* grab image */
      mi->mr->stored_item.stored = XCreatePixmap(dpy, Scr.NoFocusWin, mw, ih,
						 Scr.depth);
      XCopyArea(dpy, mi->mr->w, mi->mr->stored_item.stored,
		mi->mr->ms->look.MenuGC, 0, iy, mw, ih, 0, 0);
      mi->mr->stored_item.y = iy;
      mi->mr->stored_item.width = mw;
      mi->mr->stored_item.height = ih;
    }
    else if (f == False && mi->mr->stored_item.width != 0)
    {
      /* ungrab image */

	XCopyArea(dpy, mi->mr->stored_item.stored, mi->mr->w,
		  mi->mr->ms->look.MenuGC, 0,0, mi->mr->stored_item.width,
		  mi->mr->stored_item.height, 0, mi->mr->stored_item.y);

        XFreePixmap(dpy, mi->mr->stored_item.stored);
        mi->mr->stored_item.width = 0;
        mi->mr->stored_item.height = 0;
        mi->mr->stored_item.y = 0;

    }
    break;
  default:
    if (mi->mr->stored_item.width != 0)
    {
      XFreePixmap(dpy, mi->mr->stored_item.stored);
      mi->mr->stored_item.width = 0;
      mi->mr->stored_item.height = 0;
      mi->mr->stored_item.y = 0;
    }
    break;
  }
#endif

  mi->state = f;
  mi->mr->selected = (f) ? mi : NULL;
  paint_menu_item(mi);
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
    if(mr->name != NULL)
      if(strcasecmp(popup_name,mr->name)== 0)
      {
        return mr;
      }
    mr = mr->next;
  }
  return NULL;

}

/* Returns a menu root that a given menu item pops up */
static
MenuRoot *MrPopupForMi(MenuItem *mi)
{
  char *menu_name;
  MenuRoot *tmp = NULL;

  /* This checks if mi is != NULL too */
  if (!IS_POPUP_MENU_ITEM(mi))
    return NULL;
  /* just look past "Popup " in the action, and find that menu root */
  GetNextToken(SkipNTokens(mi->action, 1), &menu_name);
  tmp = FindPopup(menu_name);
  if (menu_name != NULL)
    free(menu_name);
  return tmp;
}

/* Returns the menu options for the menu that a given menu item pops up */
static
void GetPopupOptions(MenuItem *mi, MenuOptions *pops)
{
  if (!mi)
    return;
  pops->flags.f.has_poshints = 0;
  /* just look past "Popup <name>" in the action */
  GetMenuOptions(SkipNTokens(mi->action, 2), mi->mr->w, NULL, mi, pops);
}


/***********************************************************************
 *
 *  Procedure:
 *	PopDownMenu - unhighlight the current menu selection and
 *                    take down the menus
 *
 *      mr     - menu to pop down
 *      parent - the menu that has spawned mr (may be NULL). this is
 *               used to see if mr was spawned by itself on some level.
 *               this is a hack to allow specifying 'Popup foo' within
 *               menu foo. You must use the MenuRoot that is currently
 *               being processed here. DO NOT USE mr->mrDynamicPrev here!
 *
 ***********************************************************************/
static
void PopDownMenu(MenuRoot *mr)
{
  MenuItem *mi;
  assert(mr);

  mr->flags.allflags = 0;
  XUnmapWindow(dpy, mr->w);

  UninstallRootColormap();
  XFlush(dpy);
  /* FIX: Context and menuFromFrameOrWindowOrTitlebar should really
     be passed around, and not global */
  if (Context & (C_WINDOW | C_FRAME | C_TITLE | C_SIDEBAR))
    menuFromFrameOrWindowOrTitlebar = TRUE;
  else
    menuFromFrameOrWindowOrTitlebar = FALSE;
  if ((mi = mr->selected) != NULL) {
    SetMenuItemSelected(mi,FALSE);
  }

  /* DBUG("PopDownMenu","popped down %s",mr->name); */
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
static void PopDownAndRepaintParent(MenuRoot *mr, Bool *fSubmenuOverlaps)
{
  MenuRoot *parent = mr->mrDynamicPrev;
  XEvent event;
  int mr_y;
  unsigned int mr_height;
  int parent_y;

  if (*fSubmenuOverlaps && parent)
  {
    XGetGeometry(dpy, mr->w, &JunkRoot, &JunkX, &mr_y,
		 &JunkWidth, &mr_height, &JunkBW, &JunkDepth);
    XGetGeometry(dpy, parent->w, &JunkRoot, &JunkX, &parent_y,
		 &JunkWidth, &JunkWidth, &JunkBW, &JunkDepth);
    PopDownMenu(mr);
    /* Create a fake event to pass into PaintMenu */
    event.type = Expose;
    event.xexpose.y = mr_y - parent_y;
    event.xexpose.height = mr_height;
    PaintMenu(parent, &event);
    flush_expose(parent->w);
  }
  else
  {
    PopDownMenu(mr);
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
 ***********************************************************************/
static
void paint_menu_item(MenuItem *mi)
{
  int y_offset,text_y,d, y_height,y,x;
  GC ShadowGC, ReliefGC, currentGC;
  MenuRoot *mr = mi->mr;
  char th = mr->ms->look.ReliefThickness;
  Bool fClear = False;
#ifdef GRADIENT_BUTTONS
  Bool fGradient;

  switch (mi->mr->ms->look.face.type)
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
  text_y = y_offset + mi->mr->ms->look.pStdFont->y;
  /* center text vertically if the pixmap is taller */
  if(mi->picture)
    text_y+=mi->picture->height;
  if (mi->lpicture)
  {
    y = mi->lpicture->height - mi->mr->ms->look.pStdFont->height;
    if (y>1)
      text_y += y/2;
  }

  ShadowGC = mr->ms->look.MenuShadowGC;
  if(Scr.depth<2)
    ReliefGC = mr->ms->look.MenuShadowGC;
  else
    ReliefGC = mr->ms->look.MenuReliefGC;

  /* Hilight background */
  if (mr->ms->look.f.Hilight) {
    if (mi->state && (!mi->fIsSeparator) &&
	(((*mi->item)!=0) || mi->picture || mi->lpicture)) {
      int d = (th == 2 && mi->prev && mi->prev->state) ? 1 : 0;

      XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
      XFillRectangle(dpy, mr->w, mr->ms->look.MenuActiveBackGC, mr->xoffset+3,
		       y_offset + d, mr->width - mr->xoffset-6, y_height - d);
    } else if (th == 0) {
#ifdef GRADIENT_BUTTONS
      if (!fGradient)
#endif
	XClearArea(dpy,mr->w,mr->xoffset+3,y_offset,mr->width -mr->xoffset-6,
		   y_height,0);
    } else {
      fClear = True;
    }
    if (th == 0) {
      RelieveHalfRectangle(mr->w, 0, y_offset-1, mr->width, y_height+3,
			   ReliefGC, ShadowGC);
    }
  }

  if (mr->ms->look.ReliefThickness > 0 && (fClear || !mr->ms->look.f.Hilight)){
    /* background was already painted above? */
#ifdef GRADIENT_BUTTONS
    if (!fGradient)
#endif
    {

      if (th == 2 && mi->prev && mi->prev->state)
	XClearArea(dpy, mr->w,mr->xoffset,y_offset+1,mr->width,y_height-1,0);
      else
	XClearArea(dpy, mr->w, mr->xoffset, y_offset - th + 1, mr->width,
		   y_height + 2*(th - 1), 0);
    }
  }



  /* Hilight 3D */
  if (mr->ms->look.ReliefThickness > 0) {
    int sw = 0;

    if (mr->sidePic != NULL)
      sw = mr->sidePic->width + 5;
    else if (mr->ms->look.sidePic != NULL)
      sw = mr->ms->look.sidePic->width + 5;

    if ((mi->state)&&(!mi->fIsSeparator)&&
	(((*mi->item)!=0) || mi->picture || mi->lpicture)) {
      RelieveRectangle(dpy,mr->w, mr->xoffset + th + 1, y_offset,
		       mr->width - 2*(th + 1) - sw - 1, mi->y_height - 1,
		       ReliefGC,ShadowGC,1);
      if (th == 2) {
	RelieveRectangle(dpy,mr->w, mr->xoffset + 2, y_offset - 1,
			 mr->width - 4 - sw - 1, mi->y_height + 2 - 1,
			 ReliefGC,ShadowGC,1);
      }
    }

    RelieveHalfRectangle(mr->w, 0, y_offset - th + 1, mr->width,
			 y_height + 2*th - 1, ReliefGC, ShadowGC);
  }


  /* Draw the shadows for the absolute outside of the menus
     This stuff belongs in here, not in PaintMenu, since we only
     want to redraw it when we have too (i.e. on expose event) */

  /* Top of the menu */
  if(mi == mr->first)
    DrawSeparator(mr->w,ReliefGC,ReliefGC,0,0, mr->width-1,0,-1);

  /* Botton of the menu */
  if(mi->next == NULL)
    DrawSeparator(mr->w,ShadowGC,ShadowGC,1,mr->height-2,
                  mr->width-1, mr->height-2,1);

  if(IS_TITLE_MENU_ITEM(mi))
    {
      if(mr->ms->look.TitleUnderlines == 2)
	{
	  text_y += HEIGHT_EXTRA/2;
	  XDrawLine(dpy, mr->w, ShadowGC, mr->xoffset+2, y_offset+y_height-2,
		    mr->width-3, y_offset+y_height-2);
	  XDrawLine(dpy, mr->w, ShadowGC, mr->xoffset+2, y_offset+y_height-4,
		    mr->width-3, y_offset+y_height-4);
	}
      else if(mr->ms->look.TitleUnderlines == 1)
	{
	  if(mi->next != NULL)
	    {
	      DrawSeparator(mr->w,ShadowGC,ReliefGC,mr->xoffset+5,
			    y_offset+y_height-3,
			    mr->width-6, y_offset+y_height-3,1);
	    }
	  if(mi != mr->first)
	    {
	      text_y += HEIGHT_EXTRA_TITLE/2;
	      DrawSeparator(mr->w,ShadowGC,ReliefGC,mr->xoffset+5, y_offset+1,
			    mr->width-6, y_offset+1,1);
	    }
	}
    }
  else
    text_y += HEIGHT_EXTRA/2;

  /* see if it's an actual separator (titles are also separators) */
  if(mi->fIsSeparator && !IS_TITLE_MENU_ITEM(mi) && !IS_LABEL_MENU_ITEM(mi))
    {
      int d = (mr->ms->look.f.LongSeparators) ? 4 : 6;

      DrawSeparator(mr->w,ShadowGC,ReliefGC,mr->xoffset+5-d,
		    y_offset-1+HEIGHT_SEPARATOR/2,
		    mr->width-d,y_offset-1+HEIGHT_SEPARATOR/2,1);
    }
  if(mi->next == NULL)
    DrawSeparator(mr->w,ShadowGC,ShadowGC,mr->xoffset+1,mr->height-2,
		  mr->width-2, mr->height-2,1);
  if(mi == mr->first)
    DrawSeparator(mr->w,ReliefGC,ReliefGC,mr->xoffset,0, mr->width-1,0,-1);

  if(!mi || check_if_function_allowed(mi->func_type,my_Tmp_win,False,mi->item))
  {
    if(mi->state && !IS_TITLE_MENU_ITEM(mi))
      currentGC = mr->ms->look.MenuActiveGC;
    else
      currentGC = mr->ms->look.MenuGC;
    if (mr->ms->look.f.Hilight && !mr->ms->look.f.hasActiveFore &&
	mi->state && mi->fIsSeparator == FALSE)
      /* Use a lighter color for highlighted windows menu items for win mode */
      currentGC = mr->ms->look.MenuReliefGC;
  }
  else
    /* should be a shaded out word, not just re-colored. */
    currentGC = mr->ms->look.MenuStippleGC;

  if(*mi->item)
    XDrawString(dpy, mr->w,currentGC,mi->x+mr->xoffset,text_y, mi->item,
		mi->strlen);
  if(mi->strlen2>0)
    XDrawString(dpy, mr->w,currentGC,mi->x2+mr->xoffset,text_y, mi->item2,
		mi->strlen2);

  /* pete@tecc.co.uk: If the item has a hot key, underline it */
  if (mi->hotkey > 0)
    DrawUnderline(mr, currentGC,mr->xoffset+mi->x,text_y,mi->item,
		  mi->hotkey - 1);
  if (mi->hotkey < 0)
   DrawUnderline(mr, currentGC,mr->xoffset+mi->x2,text_y,mi->item2,
		 -1 - mi->hotkey);

  d=(mr->ms->look.EntryHeight-7)/2;
  if(mi->func_type == F_POPUP) {
    if(mi->state)
      DrawTrianglePattern(mr->w, ShadowGC, ReliefGC, ShadowGC, ReliefGC,
			  mr->width-13, y_offset+d-1, mr->width-7,
			  y_offset+d+7, mr->ms->look.f.TriangleRelief);
    else
      DrawTrianglePattern(mr->w, ReliefGC, ShadowGC, ReliefGC,
			  mr->ms->look.MenuGC,
			  mr->width-13, y_offset+d-1, mr->width-7,
			  y_offset+d+7, mr->ms->look.f.TriangleRelief);
  }

 if(mi->picture)
    {
      x = (mr->width - mi->picture->width)/2;
      if(mi->lpicture && x < mr->width0 + 5)
	x = mr->width0+5;

      if(mi->picture->depth > 0) /* pixmap? */
	{
	  Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
	  Globalgcv.clip_mask = mi->picture->mask;
	  Globalgcv.clip_x_origin= x;
	  Globalgcv.clip_y_origin = y_offset+1;
	  XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
	  XCopyArea(dpy,mi->picture->picture,mr->w,ReliefGC, 0, 0,
		    mi->picture->width, mi->picture->height,
		    x,y_offset+1);
	  Globalgcm = GCClipMask;
	  Globalgcv.clip_mask = None;
	  XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
	}
      else
	{
	  XCopyPlane(dpy,mi->picture->picture,mr->w,
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
	  Globalgcv.clip_x_origin= lp_offset + mr->xoffset;
	  Globalgcv.clip_y_origin = y;

	  XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
	  XCopyArea(dpy,mi->lpicture->picture,mr->w,ReliefGC,0,0,
		    mi->lpicture->width, mi->lpicture->height,
		    lp_offset + mr->xoffset,y);
	  Globalgcm = GCClipMask;
	  Globalgcv.clip_mask = None;
	  XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
	}
      else
	{
	  XCopyPlane(dpy,mi->lpicture->picture,mr->w,
		     currentGC,0,0,mi->lpicture->width,mi->lpicture->height,
		     lp_offset + mr->xoffset,y,1);
	}
    }
  return;
}

/************************************************************
 *
 * Draws a picture on the left side of the menu
 *
 ************************************************************/

void PaintSidePic(MenuRoot *mr)
{
  GC ReliefGC, TextGC;
  Picture *sidePic;

  if (mr->sidePic)
    sidePic = mr->sidePic;
  else if (mr->ms->look.sidePic)
    sidePic = mr->ms->look.sidePic;
  else
    return;

  if(Scr.depth<2)
    ReliefGC = mr->ms->look.MenuShadowGC;
  else
    ReliefGC = mr->ms->look.MenuReliefGC;
  TextGC = mr->ms->look.MenuGC;

  if(mr->flags.f.colorize)
    Globalgcv.foreground = mr->sideColor;
  else if (mr->ms->look.f.hasSideColor)
    Globalgcv.foreground = mr->ms->look.sideColor;
  if (mr->flags.f.colorize || mr->ms->look.f.hasSideColor) {
    Globalgcm = GCForeground;
    XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
    XFillRectangle(dpy, mr->w, Scr.ScratchGC1, 3, 3,
                   sidePic->width, mr->height - 6);
  }

  if(sidePic->depth > 0) /* pixmap? */
    {
      Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
      Globalgcv.clip_mask = sidePic->mask;
      Globalgcv.clip_x_origin = 3;
      Globalgcv.clip_y_origin = mr->height - sidePic->height -3;

      XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
      XCopyArea(dpy, sidePic->picture, mr->w,
                ReliefGC, 0, 0,
                sidePic->width, sidePic->height,
                Globalgcv.clip_x_origin, Globalgcv.clip_y_origin);
      Globalgcm = GCClipMask;
      Globalgcv.clip_mask = None;
      XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
    } else {
      XCopyPlane(dpy, sidePic->picture, mr->w,
                 TextGC, 0, 0,
                 sidePic->width, sidePic->height,
                 1, mr->height - sidePic->height, 1);
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
  int off1 = XTextWidth(mr->ms->look.pStdFont->font, txt, posn);
  int off2 = XTextWidth(mr->ms->look.pStdFont->font, txt, posn + 1) - 1;
  XDrawLine(dpy, mr->w, gc, x + off1, y + 2, x + off2, y + 2);
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
  if (u-m > m-b) {
    u=2*m-b;
  } else if (u-m < m-b) {
    b=2*m-u;
  }

  /* make (r-l)/(m-b) or its inverse be a whole number */
  if (r-l > m-b) {
    r=((r-l)/(m-b))*(m-b)+l;
  } else if (r-l < m-b) {
    r=(m-b)/((((m-b)-1)/(r-l))+1)+l;
  }

  if (!relief) {
    /* solid triangle */
    XPoint points[3];
    points[0].x = l; points[0].y = u;
    points[1].x = l; points[1].y = b;
    points[2].x = r; points[2].y = m;
    XFillPolygon(dpy, w, gc, points, 3, Convex, CoordModeOrigin);
  } else {
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
void PaintMenu(MenuRoot *mr, XEvent *pevent)
{
  MenuItem *mi;
  MenuStyle *ms = mr->ms;
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

  mr->flags.f.painted = 1;
  if( ms )
    {
      type = ms->look.face.type;
      switch(type)
      {
      case SolidMenu:
	XSetWindowBackground(dpy, mr->w, mr->ms->look.face.u.back);
	flush_expose(mr->w);
	XClearWindow(dpy,mr->w);
	break;
#ifdef GRADIENT_BUTTONS
      case HGradMenu:
      case VGradMenu:
      case DGradMenu:
      case BGradMenu:
        bounds.x = 2; bounds.y = 2;
        bounds.width = mr->width - 5;
        bounds.height = mr->height;

        if (type == HGradMenu) {
	  if (mr->flags.f.is_backgroundset == False)
	  {
	  register int i = 0;
	  register int dw;

             pmap = XCreatePixmap(dpy, mr->w, mr->width, 5, Scr.depth);
	     pmapgc = XCreateGC(dpy, pmap, gcm, &gcv);

	     bounds.width = mr->width;
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
	     XSetWindowBackgroundPixmap(dpy, mr->w, pmap);
	     XFreeGC(dpy,pmapgc);
	     XFreePixmap(dpy,pmap);
	     mr->flags.f.is_backgroundset = True;
	  }
	  XClearWindow(dpy, mr->w);
        }
        else if (type == VGradMenu)
        {
	  if (mr->flags.f.is_backgroundset == False)
	  {
	  register int i = 0;
	  register int dh = bounds.height / ms->look.face.u.grad.npixels + 1;

             pmap = XCreatePixmap(dpy, mr->w, 5, mr->height, Scr.depth);
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
	     XSetWindowBackgroundPixmap(dpy, mr->w, pmap);
	     XFreeGC(dpy,pmapgc);
	     XFreePixmap(dpy,pmap);
	     mr->flags.f.is_backgroundset = True;
	  }
	  XClearWindow(dpy, mr->w);
        }
        else /* D or BGradient */
        {
	  register int i = 0, numLines;
	  int cindex = -1;

	  XSetClipMask(dpy, Scr.TransMaskGC, None);
          numLines = mr->width + mr->height - 1;
          for(i = 0; i < numLines; i++)
          {
            if((int)(i * ms->look.face.u.grad.npixels / numLines) > cindex)
            {
              /* pick the next colour (skip if necc.) */
              cindex = i * ms->look.face.u.grad.npixels / numLines;
              XSetForeground(dpy, Scr.TransMaskGC, ms->look.face.u.grad.pixels[cindex]);
            }
            if (type == DGradMenu)
              XDrawLine(dpy, mr->w, Scr.TransMaskGC,
	                0, i, i, 0);
	    else /* BGradient */
	      XDrawLine(dpy, mr->w, Scr.TransMaskGC,
	                0, mr->height - 1 - i, i, mr->height - 1);
	  }
        }
        break;
#endif  /* GRADIENT_BUTTONS */
#ifdef PIXMAP_BUTTONS
    case PixmapMenu:
      p = ms->look.face.u.p;

      border = 0;
      width = mr->width - border * 2; height = mr->height - border * 2;

#if 0
      /* these flags are never set at the moment */
      x = border;
      if (ms->look.FaceStyle & HOffCenter) {
	if (ms->look.FaceStyle & HRight)
	  x += (int)(width - p->width);
      } else
      x += (int)(width - p->width) / 2;

      y = border;
      if (ms->look.FaceStyle & VOffCenter) {
	if (ms->look.FaceStyle & VBottom)
	  y += (int)(height - p->height);
      } else
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
      if (width > mr->width - x - border)
	width = mr->width - x - border;
      if (height > mr->height - y - border)
	height = mr->height - y - border;

      XSetClipMask(dpy, Scr.TransMaskGC, p->mask);
      XSetClipOrigin(dpy, Scr.TransMaskGC, x, y);
      XCopyArea(dpy, p->picture, mr->w, Scr.TransMaskGC,
		0, 0, width, height, x, y);
      break;

   case TiledPixmapMenu:
     XSetWindowBackgroundPixmap(dpy, mr->w, ms->look.face.u.p->picture);
     flush_expose(mr->w);
     XClearWindow(dpy,mr->w);
     break;
#endif /* PIXMAP_BUTTONS */
    }
  }

  for (mi = mr->first; mi != NULL; mi = mi->next)
  {
    /* be smart about handling the expose, redraw only the entries
     * that we need to */
    if( (mr->ms->look.face.type != SolidMenu &&
	 mr->ms->look.face.type != SimpleMenu) || pevent == NULL ||
	(pevent->xexpose.y < (mi->y_offset + mi->y_height) &&
	 (pevent->xexpose.y + pevent->xexpose.height) > mi->y_offset))
    {
      paint_menu_item(mi);
    }
  }

  PaintSidePic(mr);
  XSync(dpy, 0);
  return;
}


void FreeMenuItem(MenuItem *mi)
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


void DestroyMenu(MenuRoot *mr)
{
  MenuItem *mi,*tmp2;
  MenuRoot *tmp, *prev;

  if(mr == NULL)
    return;

  tmp = Menus.all;
  prev = NULL;
  while((tmp != NULL)&&(tmp != mr))
    {
      prev = tmp;
      tmp = tmp->next;
    }
  if(tmp != mr)
    return;

  if (mr->flags.f.is_in_use)
  {
    fvwm_msg(ERR,"DestroyMenu", "Menu %s is in use", mr->name);
    return;
  }

  if(prev == NULL)
    Menus.all = mr->next;
  else
    prev->next = mr->next;

  free(mr->name);
  XDestroyWindow(dpy,mr->w);
  XDeleteContext(dpy, mr->w, MenuContext);

  if (mr->sidePic)
    DestroyPicture(dpy, mr->sidePic);

#if 0
  /* Hey, we can't just destroy the menu face here. Another menu may need it */
  if (mr->ms != Scr.DefaultMenuStyle && mr->ms) /* I'm a bit paranoid about
                                                  segfaults :) */
  {
    XFreeGC(dpy,mr->ms->look.MenuReliefGC);
    XFreeGC(dpy,mr->ms->look.MenuShadowGC);
    XFreeGC(dpy,mr->ms->look.MenuActiveGC);
    XFreeGC(dpy,mr->ms->look.MenuGC);
    free( mr->ms );
  }
#endif

  /* need to free the window list ? */
  mi = mr->first;
  while(mi != NULL)
    {
      tmp2 = mi->next;
      FreeMenuItem(mi);
      mi = tmp2;
    }
  free(mr);
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
      mr = mr->next;
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
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  int y,width;
  int cItems;

  if(!Scr.flags.windows_captured)
    return;

  /* merge menu continuations into one menu again - needed when changing the
   * font size of a long menu. */
  while (mr->continuation != NULL)
    {
      MenuRoot *cont = mr->continuation;

      if (mr->first == mr->last)
	{
	  fvwm_msg(ERR, "MakeMenu", "BUG: Menu contains only continuation");
	  break;
	}
      /* link first item of continuation to item before 'more...' */
      mr->last->prev->next = cont->first;
      cont->first->prev = mr->last->prev;
      FreeMenuItem(mr->last);
      mr ->last = cont->last;
      mr->continuation = cont->continuation;
      /* fake an empty menu so that DestroyMenu does not destroy the items. */
      cont->first = NULL;
      DestroyMenu(cont);
    }

  mr->width = 0;
  mr->width2 = 0;
  mr->width0 = 0;
  for (cur = mr->first; cur != NULL; cur = cur->next)
    {
      width = XTextWidth(mr->ms->look.pStdFont->font, cur->item, cur->strlen);
      if(cur->picture && width < cur->picture->width)
	width = cur->picture->width;
      if(cur->func_type == F_POPUP)
	width += 15;
      if (width <= 0)
	width = 1;
      if (width > mr->width)
	mr->width = width;

      width = XTextWidth(mr->ms->look.pStdFont->font, cur->item2,cur->strlen2);
      if (width < 0)
	width = 0;
      if (width > mr->width2)
	mr->width2 = width;
      if((width==0)&&(cur->strlen2>0))
	mr->width2 = 1;

      if(cur->lpicture)
	if(mr->width0 < (cur->lpicture->width+3))
	  mr->width0 = cur->lpicture->width+3;
    }

  /* lets first size the window accordingly */
  mr->width += 10;
  if(mr->width2 > 0)
    mr->width += 5;

  /* cur_prev trails one behind cur, since we need to move that
     into a newly-made menu if we run out of space */
  for (y=2, cItems = 0, cur = mr->first, cur_prev = NULL;
       cur != NULL;
       cur_prev = cur, cur = cur->next, cItems++)
    {
      cur->mr = mr;
      cur->y_offset = y;
      cur->x = 5+mr->width0;
      if(IS_TITLE_MENU_ITEM(cur))
	{
	  width = XTextWidth(mr->ms->look.pStdFont->font, cur->item,
			     cur->strlen);
	  /* Title */
	  if(cur->strlen2  == 0)
	    cur->x = (mr->width+mr->width2+mr->width0 - width) / 2;

	  if((cur->strlen > 0)||(cur->strlen2>0))
	    {
	      if(mr->ms->look.TitleUnderlines == 2)
		cur->y_height = mr->ms->look.EntryHeight + HEIGHT_EXTRA_TITLE;
	      else
		{
		  if((cur == mr->first)||(cur->next == NULL))
		    cur->y_height=mr->ms->look.EntryHeight-HEIGHT_EXTRA+1+
		      (HEIGHT_EXTRA_TITLE/2);
		  else
		    cur->y_height = mr->ms->look.EntryHeight-HEIGHT_EXTRA +1+
		      HEIGHT_EXTRA_TITLE;
		}
	    }
	  else {
	    cur->y_height = HEIGHT_SEPARATOR;
	  }
	  /* Titles are separators, too */
	  cur->fIsSeparator = TRUE;
	}
      else if((cur->strlen==0)&&(cur->strlen2 == 0)&&
	      /* added check for NOP to distinguish from items with no text,
	       * only pixmap */
              StrEquals(cur->action,"nop")) {
	/* Separator */
	cur->y_height = HEIGHT_SEPARATOR;
	cur->fIsSeparator = TRUE;
	}
      else {
	/* Normal text entry */
	cur->fIsSeparator = FALSE;
        if ((cur->strlen==0)&&(cur->strlen2 == 0))
          cur->y_height = HEIGHT_EXTRA;
        else
          cur->y_height = mr->ms->look.EntryHeight;
	}
      if(cur->picture)
	cur->y_height += cur->picture->height;
      if(cur->lpicture && cur->y_height < cur->lpicture->height+4)
	cur->y_height = cur->lpicture->height+4;
      y += cur->y_height;
      if(mr->width2 == 0)
	{
	  cur->x2 = cur->x;
	}
      else
	{
	  cur->x2 = mr->width -5 + mr->width0;
	}
      /* this item would have to be the last item, or else
	 we need to add a "More..." entry pointing to a new menu */
      if (y+mr->ms->look.EntryHeight > Scr.MyDisplayHeight &&
	  cur->next != NULL)
	{
	  char *szMenuContinuationActionAndName;
	  MenuRoot *menuContinuation;

	  if (mr->continuation != NULL) {
	    fvwm_msg(ERR, "MakeMenu",
		     "Confused-- expected continuation to be null");
	    break;
	  }
	  szMenuContinuationActionAndName =
	    (char *) safemalloc((8+strlen(mr->name))*sizeof(char));
	  strcpy(szMenuContinuationActionAndName,"Popup ");
	  strcat(szMenuContinuationActionAndName, mr->name);
	  strcat(szMenuContinuationActionAndName,"$");
	  /* NewMenuRoot inserts at the head of the list of menus
	     but, we need it at the end */
	  /* (Give it just the name, which is 6 chars past the action
	     since strlen("Popup ")==6 ) */
	  menuContinuation = NewMenuRoot(szMenuContinuationActionAndName+6);
	  mr->continuation = menuContinuation;

	  /* Now move this item and the remaining items into the new menu */
	  cItems--;
	  menuContinuation->first = cur;
	  menuContinuation->last = mr->last;
	  menuContinuation->items = mr->items - cItems;
	  cur->prev = NULL;

	  /* cur_prev is now the last item in the current menu */
	  mr->last = cur_prev;
	  mr->items = cItems;
	  cur_prev->next = NULL;

	  /* Go back one, so that this loop will process the new item */
	  y -= cur->y_height;
	  cur = cur_prev;

	  /* And add the entry pointing to the new menu */
	  AddToMenu(mr,"More&...",szMenuContinuationActionAndName,
		    FALSE /* no pixmap scan */, FALSE);
	  MakeMenu(menuContinuation);
	  free(szMenuContinuationActionAndName);
	}
    } /* for */
  mr->flags.f.is_in_use = 0;
  /* allow two pixels for top border */
  mr->height = y + ((mr->ms->look.ReliefThickness == 1) ? 2 : 3);
  mr->flags.allflags = 0;
  mr->xanimation = 0;


  valuemask = CWBackPixel | CWEventMask | CWCursor | CWColormap
             | CWBorderPixel | CWSaveUnder;
  attributes.border_pixel = 0;
  attributes.colormap = Scr.cmap;
  attributes.background_pixel = mr->ms->look.MenuColors.back;
  attributes.event_mask = (ExposureMask | EnterWindowMask);
  attributes.cursor = Scr.FvwmCursors[MENU];
  attributes.save_under = TRUE;
  if(mr->w != None)
    XDestroyWindow(dpy,mr->w);

  mr->xoffset = 0;
  if(mr->sidePic) {
    mr->xoffset = mr->sidePic->width + 5;
  }
  else if (mr->ms->look.sidePic) {
    mr->xoffset = mr->ms->look.sidePic->width + 5;
  }

  mr->width = mr->width0 + mr->width + mr->width2 + mr->xoffset;
  mr->flags.f.is_backgroundset = False;

  mr->w = XCreateWindow (dpy, Scr.Root, 0, 0, (unsigned int) (mr->width),
			 (unsigned int) mr->height, (unsigned int) 0,
			 Scr.depth, InputOutput, Scr.viz, valuemask,
			 &attributes);
  XSaveContext(dpy,mr->w,MenuContext,(caddr_t)mr);

  return;
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
char scanForHotkeys(MenuItem *it, int which)
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
void scanForColor(char *instring, Pixel *p, Bool *c, char identifier)
{
  char *tstart, *txt, *save_instring, *name;
  int i;

  *c = False;

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
        *c = True;

        if(*txt != '\0')txt++;
        while(*txt != '\0')
          {
            *tstart++ = *txt++;
          }
        *tstart = 0;
	break;
      }
    }
  free(name);
  free(save_instring);
  return;
}

void scanForPixmap(char *instring, Picture **p, char identifier)
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
	 (mr->continuation != NULL))
    {
    *pmrPrior = mr;
    mr = mr->continuation;
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
  if (action == NULL || *action == 0)
    action = "Nop";
  GetNextToken(GetNextToken(action, &token), &option);

  tmp = (MenuItem *)safemalloc(sizeof(MenuItem));
  tmp->chHotkey = '\0';
  tmp->next = NULL;
  tmp->mr = menu; /* this gets updated in MakeMenu if we split the menu
		     because it's too large vertically */
  if (menu->first == NULL)
    {
      menu->first = tmp;
      menu->last = tmp;
      tmp->prev = NULL;
    }
  else if (StrEquals(token, "title") && option && StrEquals(option, "top"))
    {
      if (menu->first->action)
	{
	  GetNextToken(menu->first->action, &token2);
	}
      if (StrEquals(token2, "title"))
	{
	  tmp->next = menu->first->next;
	  FreeMenuItem(menu->first);
	}
      else
	{
	  tmp->next = menu->first;
	}
      if (token2)
	free(token2);
      tmp->prev = NULL;
      if (menu->first == NULL)
	menu->last = tmp;
      menu->first = tmp;
    }
  else
    {
      menu->last->next = tmp;
      tmp->prev = menu->last;
      menu->last = tmp;
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
      if (tmp->hotkey == 0) {
        char ch = scanForHotkeys(tmp, -1);	/* pete@tecc.co.uk */
	if (ch != '\0')
	  tmp->chHotkey = ch;
      }
      tmp->strlen2 = strlen(tmp->item2);
    }
  else
    tmp->strlen2 = 0;

  tmp->action = stripcpy(action);
  tmp->state = 0;
  find_func_type(tmp->action, &(tmp->func_type), NULL);
  tmp->item_num = menu->items++;
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

  tmp = (MenuRoot *) safemalloc(sizeof(MenuRoot));

  tmp->flags.allflags = 0;
  tmp->first = NULL;
  tmp->last = NULL;
  tmp->selected = NULL;
#ifdef GRADIENT_BUTTONS
  tmp->stored_item.width = 0;
  tmp->stored_item.height = 0;
  tmp->stored_item.y = 0;
#endif
  tmp->next  = Menus.all;
  tmp->continuation = NULL;
  tmp->mrDynamicPrev = NULL;
  tmp->name = stripcpy(name);
  tmp->w = None;
  tmp->height = 0;
  tmp->width = 0;
  tmp->width2 = 0;
  tmp->width0 = 0;
  tmp->items = 0;
  tmp->sidePic = NULL;
  scanForPixmap(tmp->name, &tmp->sidePic, '@');
  scanForColor(tmp->name, &tmp->sideColor, &flag,'^');
  tmp->flags.f.colorize = flag;
  tmp->xoffset = 0;
  tmp->ms = Menus.DefaultStyle;
  tmp->xanimation = 0;

  Menus.all = tmp;
  return (tmp);
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

  if (context&C_RALL) {
    for(i=0;i<Scr.nr_right_buttons;i++)
      {
	if(t->right_w[i]) {
	  buttons++;
	}
	/* is this the button ? */
	if (((1<<i)*C_R1) & context)
	  return(buttons);
      }
  } else {
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
  for (mr = Menus.all; mr; mr = mr->next)
    if (mr->w)
      XDefineCursor(dpy, mr->w, cursor);
}

/*
 * ------------------------ Menu and Popup commands --------------------------
 */

static void menu_func(F_CMD_ARGS, Bool fStaysUp)
{
  extern int menuFromFrameOrWindowOrTitlebar;
  MenuRoot *menu;
  MenuItem *miExecuteAction = NULL;
  MenuOptions mops;
  char *menu_name = NULL;
  XEvent *teventp;

  mops.flags.allflags = 0;
  action = GetNextToken(action,&menu_name);
  action = GetMenuOptions(action,w,tmp_win,NULL,&mops);
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
  if (menu_name && set_repeat_data(menu_name,
				   (fStaysUp) ? REPEAT_MENU : REPEAT_POPUP,
				   NULL))
    free(menu_name);
  menuFromFrameOrWindowOrTitlebar = FALSE;

  if (!action && eventp && eventp->type == KeyPress)
    teventp = (XEvent *)1;
  else
    teventp = eventp;
  if ((do_menu(menu, NULL, &miExecuteAction, 0, fStaysUp, teventp, &mops) ==
       MENU_DOUBLE_CLICKED) && action)
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
    if(mr->ms == ms)
      mr->ms = Menus.DefaultStyle;
    mr = mr->next;
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
  if (ms->look.f.hasSideColor == 1)
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
  for (mr = Menus.all; mr; mr = mr->next)
  {
    if (mr->ms == ms)
      mr->ms = Menus.DefaultStyle;
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
  if (!ms->look.f.hasActiveFore)
    ms->look.MenuActiveColors.fore=ms->look.MenuColors.fore;

  /* calculate colors based on background */
  if (!ms->look.f.hasActiveBack)
    ms->look.MenuActiveColors.back = ms->look.MenuColors.back;
  if (!ms->look.f.hasStippleFore)
    ms->look.MenuStippleColors.fore = ms->look.MenuColors.back;
  if(Scr.depth > 2) {                 /* if not black and white */
    ms->look.MenuRelief.back = GetShadow(ms->look.MenuColors.back);
    ms->look.MenuRelief.fore = GetHilite(ms->look.MenuColors.back);
  } else {                              /* black and white */
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

  gcv.foreground = (ms->look.f.hasActiveBack) ?
    ms->look.MenuActiveColors.back : ms->look.MenuRelief.back;
  gcv.background = (ms->look.f.hasActiveFore) ?
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
	  tmpms->look.f.hasActiveFore = 0;
	  tmpms->look.f.hasActiveBack = 0;
	  tmpms->feel.f.PopupAsRootmenu = 0;
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
	if (i == 0) {
	  tmpms->feel.PopupOffsetPercent = 67;
	  tmpms->feel.PopupOffsetAdd = 0;
	  tmpms->feel.f.PopupImmediately = 0;
	  tmpms->feel.f.TitleWarp = 1;
	  tmpms->look.ReliefThickness = 1;
	  tmpms->look.TitleUnderlines = 1;
	  tmpms->look.f.LongSeparators = 0;
	  tmpms->look.f.TriangleRelief = 1;
	  tmpms->look.f.Hilight = 0;
	} else if (i == 1) {
	  tmpms->feel.PopupOffsetPercent = 100;
	  tmpms->feel.PopupOffsetAdd = -3;
	  tmpms->feel.f.PopupImmediately = 1;
	  tmpms->feel.f.TitleWarp = 0;
	  tmpms->look.ReliefThickness = 2;
	  tmpms->look.TitleUnderlines = 2;
	  tmpms->look.f.LongSeparators = 1;
	  tmpms->look.f.TriangleRelief = 1;
	  tmpms->look.f.Hilight = 0;
	} else /* i == 2 */ {
	  tmpms->feel.PopupOffsetPercent = 100;
	  tmpms->feel.PopupOffsetAdd = -5;
	  tmpms->feel.f.PopupImmediately = 1;
	  tmpms->feel.f.TitleWarp = 0;
	  tmpms->look.ReliefThickness = 0;
	  tmpms->look.TitleUnderlines = 1;
	  tmpms->look.f.LongSeparators = 0;
	  tmpms->look.f.TriangleRelief = 0;
	  tmpms->look.f.Hilight = 1;
	}

	/* common settings */
	tmpms->feel.f.Animated = 0;
	FreeMenuFace(dpy, &tmpms->look.face);
	tmpms->look.face.type = SimpleMenu;
	if (tmpms->look.pStdFont && tmpms->look.pStdFont != &Scr.StdFont)
	{
	  XFreeFont(dpy, tmpms->look.pStdFont->font);
	  free(tmpms->look.pStdFont);
	}
	tmpms->look.pStdFont = &Scr.StdFont;
	gc_changed = True;
	if (tmpms->look.f.hasSideColor == 1)
	{
	  FreeColors(&tmpms->look.sideColor, 1);
	  tmpms->look.f.hasSideColor = 0;
	}
	tmpms->look.f.hasSideColor = 0;
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
	if (tmpms->look.f.hasStippleFore)
	  FreeColors(&tmpms->look.MenuStippleColors.fore, 1);
	if (arg1 == NULL)
	{
	  tmpms->look.f.hasStippleFore = 0;
	}
	else
	{
	  tmpms->look.MenuStippleColors.fore = GetColor(arg1);
	  tmpms->look.f.hasStippleFore = 1;
	}
	gc_changed = True;
	break;

      case 6: /* HilightBack */
	if (tmpms->look.f.hasActiveBack)
	  FreeColors(&tmpms->look.MenuActiveColors.back, 1);
	if (arg1 == NULL)
	{
	  tmpms->look.f.hasActiveBack = 0;
	}
	else
	{
	  tmpms->look.MenuActiveColors.back = GetColor(arg1);
	  tmpms->look.f.hasActiveBack = 1;
	}
	tmpms->look.f.Hilight = 1;
	gc_changed = True;
	break;

      case 7: /* HilightBackOff */
	tmpms->look.f.Hilight = 0;
	gc_changed = True;
	break;

      case 8: /* ActiveFore */
	if (tmpms->look.f.hasActiveFore)
	  FreeColors(&tmpms->look.MenuActiveColors.fore, 1);
	if (arg1 == NULL)
	{
	  tmpms->look.f.hasActiveFore = 0;
	}
	else
	{
	  tmpms->look.MenuActiveColors.fore = GetColor(arg1);
	  tmpms->look.f.hasActiveFore = 1;
	}
	gc_changed = True;
	break;

      case 9: /* ActiveForeOff */
	tmpms->look.f.hasActiveFore = 0;
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
	tmpms->feel.f.Animated = 1;
	break;

      case 14: /* AnimationOff */
	tmpms->feel.f.Animated = 0;
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
	tmpms->feel.f.TitleWarp = 1;
	break;

      case 20: /* TitleWarpOff */
	tmpms->feel.f.TitleWarp = 0;
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
	tmpms->look.f.LongSeparators = 1;
	break;

      case 25: /* SeparatorsShort */
	tmpms->look.f.LongSeparators = 0;
	break;

      case 26: /* TrianglesSolid */
	tmpms->look.f.TriangleRelief = 0;
	break;

      case 27: /* TrianglesRelief */
	tmpms->look.f.TriangleRelief = 1;
	break;

      case 28: /* PopupImmediately */
	tmpms->feel.f.PopupImmediately = 1;
	break;

      case 29: /* PopupDelayed */
	tmpms->feel.f.PopupImmediately = 0;
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
	if (tmpms->look.f.hasSideColor == 1)
	{
	  FreeColors(&tmpms->look.sideColor, 1);
	  tmpms->look.f.hasSideColor = 0;
	}
	if (arg1)
	{
	  tmpms->look.sideColor = GetColor(arg1);
	  tmpms->look.f.hasSideColor = 1;
	}
	break;

      case 33: /* PopupAsRootmenu */
	tmpms->feel.f.PopupAsRootmenu = 1;
	break;

      case 34: /* PopupAsSubmenu */
	tmpms->feel.f.PopupAsRootmenu = 0;
	break;

#if 0
      case 35: /* PositionHints */
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
    mr->ms = ms;
    MakeMenu(mr);
    free(menuname);
    action = GetNextToken(action,&menuname);
  }
}

void destroy_menu(F_CMD_ARGS)
{
  MenuRoot *mr;
  MenuRoot *mrContinuation;

  char *token;

  GetNextToken(action,&token);
  if (!token)
    return;
  mr = FindPopup(token);
  if (Scr.last_added_item.type == ADDED_MENU)
    set_last_added_item(ADDED_NONE, NULL);
  free(token);
  while (mr)
  {
    mrContinuation = mr->continuation; /* save continuation before destroy */
    DestroyMenu(mr);
    mr = mrContinuation;
  }
  return;
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
  AddToMenu(mr, item,rest,TRUE /* pixmap scan */, TRUE);
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
  AddToMenu(mr, item,rest,TRUE /* pixmap scan */, FALSE);
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
  if (sscanf(token,"o%d%n", &val, &chars) >= 1) {
    token += chars;
    *pFinalX += val*factor;
    *width_factor -= val/100;
  } else if (token[0] == 'c') {
    token++;
    *pFinalX += w/2;
    *width_factor -= 0.5;
  }
  while (*token != 0) {
    if (sscanf(token,"%d%n", &val, &chars) >= 1) {
      token += chars;
      if (sscanf(token,"%c", &c) == 1) {
	if (c == 'm') {
	  token++;
	  *width_factor += val/100;
	} else if (c == 'p') {
	  token++;
	  *pFinalX += val;
	} else {
	  *pFinalX += val*factor;
	}
      } else {
	*pFinalX += val*factor;
      }
    } else {
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
		     MenuItem *mi, MenuOptions *pops)
{
  char *tok = NULL, *naction = action, *taction;
  int x, y, button, gflags;
  unsigned int width, height;
  Window context_window = 0;
  Bool fHasContext, fUseItemOffset;
  Bool fValidPosHints = fLastMenuPosHintsValid;

  fLastMenuPosHintsValid = FALSE;
  if (pops == NULL) {
    fvwm_msg(ERR,"GetMenuOptions","no MenuOptions pointer passed");
    return action;
  }

  taction = action;
  pops->flags.allflags = 0;
  pops->flags.f.has_poshints = 0;
  while (action != NULL) {
    /* ^ just to be able to jump to end of loop without 'goto' */
    gflags = NoValue;
    pops->pos_hints.fRelative = FALSE;
    /* parse context argument (if present) */
    naction = GetNextToken(taction, &tok);
    if (!tok) {
      /* no context string */
      fHasContext = FALSE;
      break;
    }

    pops->pos_hints.fRelative = TRUE; /* set to FALSE for absolute hints! */
    fUseItemOffset = FALSE;
    fHasContext = TRUE;
    if (StrEquals(tok, "context")) {
      if (mi && mi->mr) context_window = mi->mr->w;
      else if (tmp_win) {
	if (IS_ICONIFIED(tmp_win))
	  context_window=tmp_win->icon_pixmap_w;
	else
	  context_window = tmp_win->frame;
      }
      else
	context_window = w;
      pops->pos_hints.fRelative = TRUE;
    } else if (StrEquals(tok,"menu")) {
      if (mi && mi->mr) context_window = mi->mr->w;
    } else if (StrEquals(tok,"item")) {
      if (mi && mi->mr) {
	context_window = mi->mr->w;
	fUseItemOffset = TRUE;
      }
    } else if (StrEquals(tok,"icon")) {
      if (tmp_win) context_window = tmp_win->icon_pixmap_w;
    } else if (StrEquals(tok,"window")) {
      if (tmp_win) context_window = tmp_win->frame;
    } else if (StrEquals(tok,"interior")) {
      if (tmp_win) context_window = tmp_win->w;
    } else if (StrEquals(tok,"title")) {
      if (tmp_win) {
	if (IS_ICONIFIED(tmp_win))
	  context_window = tmp_win->icon_w;
	else
	  context_window = tmp_win->title_w;
      }
    } else if (strncasecmp(tok,"button",6) == 0) {
      if (sscanf(&(tok[6]),"%d",&button) != 1 ||
		 tok[6] == '+' || tok[6] == '-' || button < 0 || button > 9) {
	fHasContext = FALSE;
      } else if (tmp_win) {
	if (button == 0) button = 10;
	if (button & 0x01) context_window = tmp_win->left_w[button/2];
	else context_window = tmp_win->right_w[button/2-1];
      }
    } else if (StrEquals(tok,"root")) {
      context_window = Scr.Root;
      pops->pos_hints.fRelative = FALSE;
    } else if (StrEquals(tok,"mouse")) {
      context_window = 0;
    } else if (StrEquals(tok,"rectangle")) {
      int flags;
      /* parse the rectangle */
      free(tok);
      naction = GetNextToken(taction, &tok);
      if (tok == NULL) {
	fvwm_msg(ERR,"GetMenuOptions","missing rectangle geometry");
	return action;
      }
      flags = XParseGeometry(tok, &x, &y, &width, &height);
      if ((flags & AllValues) != AllValues) {
	free(tok);
	fvwm_msg(ERR,"GetMenuOptions","invalid rectangle geometry");
	return action;
      }
      if (flags & XNegative) x = Scr.MyDisplayWidth - x - width;
      if (flags & YNegative) y = Scr.MyDisplayHeight - y - height;
      pops->pos_hints.fRelative = FALSE;
    } else if (StrEquals(tok,"this")) {
      context_window = w;
    } else {
      /* no context string */
      fHasContext = FALSE;
    }

    if (tok)
      free(tok);
    if (fHasContext)
      taction = naction;
    else naction = action;

    if (!context_window || !fHasContext
	|| !XGetGeometry(dpy, context_window, &JunkRoot, &JunkX, &JunkY,
			 &width, &height, &JunkBW, &JunkDepth)
	|| !XTranslateCoordinates(
	  dpy, context_window, Scr.Root, 0, 0, &x, &y, &JunkChild)) {
      /* now window or could not get geometry */
      XQueryPointer(dpy,Scr.Root,&JunkRoot,&JunkChild,&x,&y,&JunkX,&JunkY,
		    &JunkMask);
      width = height = 1;
    } else if (fUseItemOffset) {
      y += mi->y_offset;
      height = mi->y_height;
    }

    /* parse position arguments */
    taction = GetOneMenuPositionArgument(
      naction, x, width, &(pops->pos_hints.x), &(pops->pos_hints.x_factor));
    naction = GetOneMenuPositionArgument(
      taction, y, height, &(pops->pos_hints.y), &(pops->pos_hints.y_factor));
    if (naction == taction) {
      /* argument is missing or invalid */
      if (fHasContext)
	fvwm_msg(ERR,"GetMenuOptions","invalid position arguments");
      naction = action;
      taction = action;
      break;
    }
    taction = naction;
    pops->flags.f.has_poshints = 1;
    if (fValidPosHints == TRUE && pops->pos_hints.fRelative == TRUE) {
      pops->pos_hints = lastMenuPosHints;
    }
    /* we want to do this only once */
    break;
  } /* while (1) */

  if (!pops->flags.f.has_poshints && fValidPosHints) {
    DBUG("GetMenuOptions","recycling position hints");
    pops->flags.f.has_poshints = 1;
    pops->pos_hints = lastMenuPosHints;
    pops->pos_hints.fRelative = FALSE;
  }

  action = naction;
  /* to keep Purify silent */
  pops->flags.f.select_in_place = 0;
  /* parse additional options */
  while (naction && *naction) {
    naction = GetNextToken(action, &tok);
    if (!tok)
      break;
    if (StrEquals(tok, "WarpTitle")) {
      pops->flags.f.warp_title = 1;
      pops->flags.f.no_warp = 0;
    } else if (StrEquals(tok, "NoWarp")) {
      pops->flags.f.warp_title = 0;
      pops->flags.f.no_warp = 1;
    } else if (StrEquals(tok, "Fixed")) {
      pops->flags.f.fixed = 1;
    } else if (StrEquals(tok, "SelectInPlace")) {
      pops->flags.f.select_in_place = 1;
    } else if (StrEquals(tok, "SelectWarp")) {
      pops->flags.f.select_warp = 1;
    } else {
      free (tok);
      break;
    }
    action = naction;
    free (tok);
  }
  if (!pops->flags.f.select_in_place) {
    pops->flags.f.select_warp = 0;
  }

  return action;
}
