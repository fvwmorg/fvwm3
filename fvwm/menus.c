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

/* Dominik Vogt, Sep 1998
   dominik_vogt@hp.com

   Implemented position hints for menus and fixed a lot of bugs
   (some cosmetic bugs and two or three coredumps). Rewrote large parts
   of MenuInteraction and FPopupMenus.
   */

/* German Gomez Garcia, Nov 1998
   german@pinon.ccu.uniovi.es

   Implemented new menu style definition, allowing multiple definitios and
   gradients and pixmaps 'ala' ButtonStyle. See doc/README.styles for more
   info.  */

/* Dominik Vogt, Dec 1998
   dominik_vogt@hp.com

   Rewrite of the MenuStyle code for cleaner parsing and better
   configurability. */


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
#include "menus.h"
#include "misc.h"
#include "parse.h"
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
static void PaintEntry(MenuItem *mi);
static void PaintMenu(MenuRoot *, XEvent *);
static void SetMenuItemSelected(MenuItem *mi, Bool f);
static MenuRoot *MrPopupForMi(MenuItem *mi);
static int ButtonPosition(int context, FvwmWindow * t);
static Bool FMenuMapped(MenuRoot *menu);

Bool menuFromFrameOrWindowOrTitlebar = FALSE;
Bool fShowPopupTimedout = FALSE;
Bool mouse_moved = FALSE;

extern int Context,Button;
extern FvwmWindow *ButtonWindow, *Tmp_win;
extern XEvent Event;
extern XContext MenuContext;

static FvwmWindow *s_Tmp_win=NULL;

/* dirty patch to pass the last popups position hints to popup_func */
MenuPosHints lastMenuPosHints;
Bool fLastMenuPosHintsValid = FALSE;
static Bool fIgnorePosHints = FALSE;
static Bool fWarpPointerToTitle = FALSE;

/* patch to allow double-keypress (like doubleclick) */
unsigned int dkp_keystate;
unsigned int dkp_keycode;
Time dkp_timestamp;

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

  DBUG("do_menu","called");

  key_press = (eventp && (eventp == (XEvent *)1 || eventp->type == KeyPress));
  /* this condition could get ugly */
  if(menu == NULL || menu->in_use) {
    /* DBUG("do_menu","menu->in_use for %s -- returning",menu->name); */
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

  menu->in_use = TRUE;
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
  menu->in_use = FALSE;

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

  if (lastTimestamp-t0 < Scr.menus.DoubleClickTime && !mouse_moved &&
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
	  (*pmiExecuteAction)->action,ButtonWindow, &Event,Context, -1);
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
  MenuItem *newItem;
  MenuItem *miCurrent = pmiCurrent?*pmiCurrent:NULL;
  int index;

  /* handle double-keypress */
  if (dkp_timestamp &&
      lastTimestamp-dkp_timestamp < Scr.menus.DoubleClickTime &&
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
    char key;
    /* Search menu for matching hotkey */
    for (mi = menu->first; mi; mi = mi->next) {
      key = tolower(mi->chHotkey);
      if (keychar == key) {
	*pmiCurrent = mi;
	if (IS_POPUP_MENU_ITEM(mi))
	  return MENU_POPUP;
	else
	  return MENU_SELECTED;
      }
    }
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
        if ((*pmiCurrent = menu->last) != NULL)
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
        if ((*pmiCurrent = MiFromMenuIndex(menu,0)) != NULL)
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

      /* Nothing special --- Allow other shortcuts */
    default:
      /* There are no useful shortcuts, so don't do that.
       * (Dominik Vogt, 11-Nov-1998)
       * Keyboard_shortcuts(Event, NULL, ButtonRelease); */
      break;
    }

  return MENU_NOP;
}

#define MICRO_S_FOR_10MS 10000

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
		       (Scr.menus.PopupDelay10ms > 0));

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
	if (Scr.menus.PopupDelay10ms > 0) {
	  while (XCheckMaskEvent(dpy,
				 ButtonPressMask|ButtonReleaseMask|
				 ExposureMask|KeyPressMask|
				 VisibilityChangeMask|ButtonMotionMask,
				 &Event) == FALSE) {
	    usleep(MICRO_S_FOR_10MS);
	    if (c10msDelays++ == Scr.menus.PopupDelay10ms) {
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
			       &Event))&&(Event.type != ButtonRelease));
      }

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
	  s_Tmp_win = Tmp_win; /* BUT we need to save Tmp_win first
				  because DispatchEvent changes
				  Tmp_win and messes up our calls to
				  check_allowed_function for stippling
				  disallowed menu items */

	default:
	  DispatchEvent();
	  if (s_Tmp_win)
	    {
	      Tmp_win = s_Tmp_win; /* restore Tmp_win */
	      s_Tmp_win = NULL;
	      continue; /* now 'continue' if event was an expose */
	    }
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
    prior_width *= mr->ms->feel.PopupOffsetPercent / 100;
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
Bool FPopupMenu (MenuRoot *menu, MenuRoot *menuPrior, int x, int y,
		 Bool fWarpItem, MenuOptions *pops, Bool *ret_overlap)
{
  Bool fWarpTitle = FALSE;
  int x_overlap, x_clipped_overlap;
  MenuItem *mi = NULL;

  DBUG("FPopupMenu","called");
  if ((!menu)||(menu->w == None)||(menu->items == 0)||(menu->in_use)) {
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
      int mw, mh;

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
      mi->mr->stored_item.stored = XCreatePixmap(dpy, Scr.Root, mw, ih,
						 Scr.d_depth);
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
  PaintEntry(mi);
}

/* Returns a menu root that a given menu item pops up */
static
MenuRoot *MrPopupForMi(MenuItem *mi)
{
  char *menu_name = NULL;
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
  int mr_height;
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
 *	RelieveRectangle - add relief lines to a rectangular window
 *
 ***********************************************************************/
static
void RelieveRectangle(Window win,int x,int y,int w, int h,GC Hilite,GC Shadow)
{
  XDrawLine(dpy, win, Hilite, x, y, w+x-1, y);
  XDrawLine(dpy, win, Hilite, x, y, x, h+y-1);

  XDrawLine(dpy, win, Shadow, x, h+y-1, w+x-1, h+y-1);
  XDrawLine(dpy, win, Shadow, w+x-1, y, w+x-1, h+y-1);
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
 *      PaintEntry - draws a single entry in a popped up menu
 *
 ***********************************************************************/
static
void PaintEntry(MenuItem *mi)
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
  if(Scr.d_depth<2)
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
      RelieveRectangle(mr->w, mr->xoffset + th + 1, y_offset,
		       mr->width - 2*(th + 1) - sw, mi->y_height,
		       ReliefGC,ShadowGC);
      if (th == 2) {
	RelieveRectangle(mr->w, mr->xoffset + 2, y_offset - 1,
			 mr->width - 4 - sw, mi->y_height + 2,
			 ReliefGC,ShadowGC);
      }
    }

    RelieveHalfRectangle(mr->w, 0, y_offset - th + 1, mr->width,
			 y_height + 2*(th - 1), ReliefGC, ShadowGC);
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
                  mr->width-2, mr->height-2,1);

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
      int d = (mr->ms->look.f.LongSeparators) ? 3 : 0;

      DrawSeparator(mr->w,ShadowGC,ReliefGC,mr->xoffset+5-d,
		    y_offset-1+HEIGHT_SEPARATOR/2,
		    mr->width-6+d,y_offset-1+HEIGHT_SEPARATOR/2,1);
    }
  if(mi->next == NULL)
    DrawSeparator(mr->w,ShadowGC,ShadowGC,mr->xoffset+1,mr->height-2,
		  mr->width-2, mr->height-2,1);
  if(mi == mr->first)
    DrawSeparator(mr->w,ReliefGC,ReliefGC,mr->xoffset,0, mr->width-1,0,-1);

  if(check_allowed_function(mi))
  {
    if(mi->state && !IS_TITLE_MENU_ITEM(mi))
      currentGC = mr->ms->look.MenuActiveGC;
    else
      currentGC = mr->ms->look.MenuGC;
  }
  else
    /* should be a shaded out word, not just re-colored. */
    currentGC = mr->ms->look.MenuStippleGC;

  if (mr->ms->look.f.Hilight && !mr->ms->look.f.hasActiveFore &&
      mi->state && mi->fIsSeparator == FALSE)
    /* Use a lighter color for highlighted windows menu items for win mode */
    currentGC = mr->ms->look.MenuReliefGC;

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

  if(Scr.d_depth<2)
    ReliefGC = mr->ms->look.MenuShadowGC;
  else
    ReliefGC = mr->ms->look.MenuReliefGC;
  TextGC = mr->ms->look.MenuGC;

  if(mr->colorize)
    Globalgcv.foreground = mr->sideColor;
  else if (mr->ms->look.f.hasSideColor)
    Globalgcv.foreground = mr->ms->look.sideColor;
  if (mr->colorize || mr->ms->look.f.hasSideColor) {
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
	  if (mr->backgroundset == False)
	  {
	  register int i = 0;
	  register int dw;

             pmap = XCreatePixmap(dpy, Scr.Root, mr->width, 5, Scr.d_depth);
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
	     mr->backgroundset = True;
	  }
	  XClearWindow(dpy, mr->w);
        }
        else if (type == VGradMenu)
        {
	  if (mr->backgroundset == False)
	  {
	  register int i = 0;
	  register int dh = bounds.height / ms->look.face.u.grad.npixels + 1;

             pmap = XCreatePixmap(dpy, Scr.Root, 5, mr->height, Scr.d_depth);
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
	     mr->backgroundset = True;
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
      PaintEntry(mi);
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

  tmp = Scr.menus.all;
  prev = NULL;
  while((tmp != NULL)&&(tmp != mr))
    {
      prev = tmp;
      tmp = tmp->next;
    }
  if(tmp != mr)
    return;

  if(prev == NULL)
    Scr.menus.all = mr->next;
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

  mr = Scr.menus.all;
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

  if((mr->func != F_POPUP)||(!(Scr.flags & WindowsCaptured)))
    return;

  /* merge menu continuations into one menu again - needed when changing the
   * font size of a long menu. */
  while (mr->continuation != NULL)
    {
      MenuRoot *cont = mr->continuation;

      if (mr->first == mr->last)
	{
	  fvwm_msg(ERR, "MakeMenu", "BUG: Menu contains only contionuation");
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
	  menuContinuation = NewMenuRoot(szMenuContinuationActionAndName+6,
					 False);
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
  mr->in_use = 0;
  /* allow two pixels for top border */
  mr->height = y + ((mr->ms->look.ReliefThickness == 1) ? 2 : 3);
  mr->flags.allflags = 0;
  mr->xanimation = 0;

#ifndef NO_SAVEUNDERS
  valuemask = (CWBackPixel | CWEventMask | CWCursor | CWSaveUnder);
#else
  valuemask = (CWBackPixel | CWEventMask | CWCursor);
#endif
  attributes.background_pixel = mr->ms->look.MenuColors.back;
  attributes.event_mask = (ExposureMask | EnterWindowMask);
  attributes.cursor = Scr.FvwmCursors[MENU];
#ifndef NO_SAVEUNDERS
  attributes.save_under = TRUE;
#endif
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
  mr->backgroundset = False;

  mr->w = XCreateWindow (dpy, Scr.Root, 0, 0, (unsigned int) (mr->width),
			 (unsigned int) mr->height, (unsigned int) 0,
			 CopyFromParent, (unsigned int) InputOutput,
			 (Visual *) CopyFromParent,
			 valuemask, &attributes);
  XSaveContext(dpy,mr->w,MenuContext,(caddr_t)mr);

  return;
}

/* FHotKeyUsedBefore
 * Return true if any item already in the menu has
 * used the given hotkey already
 * This means that it doesn't check the last element of the menu
 */
int FHotKeyUsedBefore(MenuRoot *menu, char ch) {
  int f = FALSE;
  MenuItem *currentMenuItem = menu->first;
  /* we want to stop just before the last item in the menu */
  while (currentMenuItem != 0 && currentMenuItem->next != 0)
    {
    if (currentMenuItem->chHotkey == ch)
      {
      f = TRUE;
      break;
      }
    currentMenuItem = currentMenuItem->next;
    }
  return f;
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
	    for (; *txt != '\0'; txt++) txt[0] = txt[1];/* Copy down..	*/
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
  extern char *IconPath;
  extern char *PixmapPath;
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
	  pp=CachePicture(dpy,Scr.Root,IconPath,PixmapPath,name,
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
 *	func	- the numeric function
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

  if ((item == NULL || *item == 0) && fNoPlus)
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
	{
	if (FHotKeyUsedBefore(menu,ch))
	  {
	    fvwm_msg(WARN, "AddToMenu",
		     "Hotkey %c is reused in menu %s; second binding ignored.",
		     ch, menu->name);
	    tmp->hotkey = 0;
	  }
	else
	  {
	    tmp->chHotkey = ch;
	  }
	}
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
	{
	  if (FHotKeyUsedBefore(menu,ch))
	  {
	    fvwm_msg(WARN, "AddToMenu",
		     "Hotkey %c is reused in menu %s; second binding ignored.",
		     ch, menu->name);
	    tmp->hotkey = 0;
	  }
	  else
	  {
	    tmp->chHotkey = ch;
	  }
	}
      }
      tmp->strlen2 = strlen(tmp->item2);
    }
  else
    tmp->strlen2 = 0;

  tmp->action = stripcpy(action);
  tmp->state = 0;
  find_func_type(tmp->action, &(tmp->func_type), &(tmp->func_needs_window));
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
 *      fFunction - True if we should set func to F_FUNCTION,
 *                  F_POPUP otherwise
 *
 ***********************************************************************/
MenuRoot *NewMenuRoot(char *name, Bool fFunction)
{
  MenuRoot *tmp;

  tmp = (MenuRoot *) safemalloc(sizeof(MenuRoot));

  tmp->first = NULL;
  tmp->last = NULL;
  tmp->selected = NULL;
#ifdef GRADIENT_BUTTONS
  tmp->stored_item.width = 0;
  tmp->stored_item.height = 0;
  tmp->stored_item.y = 0;
#endif
  tmp->next  = Scr.menus.all;
  tmp->continuation = NULL;
  tmp->mrDynamicPrev = NULL;
  tmp->name = stripcpy(name);
  tmp->w = None;
  tmp->height = 0;
  tmp->width = 0;
  tmp->width2 = 0;
  tmp->width0 = 0;
  tmp->items = 0;
  tmp->in_use = 0;
  tmp->func = (fFunction) ? F_FUNCTION : F_POPUP;
  tmp->sidePic = NULL;
  scanForPixmap(tmp->name, &tmp->sidePic, '@');
  scanForColor(tmp->name, &tmp->sideColor, &tmp->colorize,'^');
  tmp->xoffset = 0;
  tmp->ms = Scr.menus.DefaultStyle;
  tmp->flags.allflags = 0;
  tmp->xanimation = 0;

  Scr.menus.all = tmp;
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
