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
    

/***********************************************************************
 *
 * fvwm menu code
 *
 ***********************************************************************/
/* #define FVWM_DEBUG_MSGS */
#include "../configure.h"

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

static void DrawTrianglePattern(Window,GC,GC,GC,GC,int,int,int,int);
static void DrawSeparator(Window, GC,GC,int, int,int,int,int);
static void DrawUnderline(Window w, GC gc, int x, int y, char *txt, int off);
static MenuStatus MenuInteraction(MenuRoot *menu,MenuRoot *menuPrior, MenuItem **pmiExecuteAction,
			   int cmenuDeep,Bool fSticks);
static void WarpPointerToTitle(MenuRoot *menu);
static MenuItem *MiWarpPointerToItem(MenuItem *mi, Bool fSkipTitle);
static void PopDownMenu(MenuRoot *mr);
static Bool FPopupMenu(MenuRoot *menu, MenuRoot *menuPrior, int x, int y, Bool fWarpItem);
static void GetPreferredPopupPosition(MenuRoot *mr, int *x, int *y);
static int PopupPositionOffset(MenuRoot *mr);
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

#define IS_TITLE_MENU_ITEM(mi) (((mi)?((mi)->func_type==F_TITLE):FALSE))
#define IS_POPUP_MENU_ITEM(mi) (((mi)?((mi)->func_type==F_POPUP):FALSE))
#define IS_SEPARATOR_MENU_ITEM(mi) ((mi)?(mi)->fIsSeparator:FALSE)
#define IS_REAL_SEPARATOR_MENU_ITEM(mi) ((mi)?((mi)->fIsSeparator && StrEquals(mi->action,"nop") && !mi->strlen && !mi->strlen2):FALSE)
#define IS_LABEL_MENU_ITEM(mi) ((mi)?((mi)->fIsSeparator && StrEquals(mi->action,"nop") && (mi->strlen || mi->strlen2)):FALSE)
#define MENU_MIDDLE_OFFSET(menu) (menu->xoffset + (menu->width - menu->xoffset)/2)

/****************************************************************************
 *
 * Initiates a menu pop-up
 *
 * Style = 1 = sticky menu, stays up on initial button release.
 * Style = 0 = transient menu, drops on initial release.
 *
 * Returns one of MENU_NOP, MENU_ERROR, MENU_ABORTED, MENU_DONE
 ***************************************************************************/
MenuStatus do_menu(MenuRoot *menu, MenuRoot *menuPrior, MenuItem **pmiExecuteAction,
		   int cmenuDeep,Bool fStick, Bool key_press )
{
  MenuStatus retval=MENU_NOP;
  int x,y;
  int x_start = -1,y_start = -1;
  Bool fFailedPopup = FALSE;
  Bool fWasAlreadyPopped = FALSE;
  Time t0;
  extern Time lastTimestamp;

  DBUG("do_menu","called");

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
  if (key_press && cmenuDeep == 0) {
    x_start = x;
    y_start = y;
  }

  /* Figure out where we should popup, if possible */
  if (!FMenuMapped(menu)) {
    if(cmenuDeep > 0) {
      /* this is a submenu popup */
      assert(menuPrior);
      GetPreferredPopupPosition(menuPrior, &x, &y);
    } else {
      /* we're a top level menu */
      mouse_moved = FALSE;
      t0 = lastTimestamp;
      if(!GrabEm(MENU)) { /* GrabEm specifies the cursor to use */
	XBell(dpy,Scr.screen);
	return MENU_ABORTED;
      }
      /* Make the menu appear under the pointer rather than warping */
      x -= MENU_MIDDLE_OFFSET(menu);
      y -= Scr.EntryHeight/2;
    }

    /* FPopupMenu may move the x,y to make it fit on screen more nicely */
    /* it might also move menuPrior out of the way */
    if (!FPopupMenu (menu, menuPrior, x, y, key_press /* warp */)) {
      fFailedPopup = TRUE;
      XBell (dpy, Scr.screen);
    }
  }
  else {
    fWasAlreadyPopped = TRUE;
    if (key_press) MiWarpPointerToItem(menu->first, TRUE /* skip Title */);
  }

  menu->in_use = TRUE;
  cmenuDeep++;
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

  if(cmenuDeep == 0) {
    UngrabEm();
    WaitForButtonsUp();
    if (retval == MENU_DONE || retval == MENU_DONE_BUTTON)
      ExecuteFunction((*pmiExecuteAction)->action,ButtonWindow, &Event, Context,-1);
  }

  if (cmenuDeep == 0 &&
      x_start >= 0 && y_start >= 0 &&
      IS_MENU_BUTTON(retval)) {
    /* warp pointer back to where invoked if this was brought up
       with a keypress and we're returning from a top level menu, 
       and a button release event didn't end it */
    XWarpPointer(dpy, 0, Scr.Root, 0, 0,
		 Scr.MyDisplayWidth, Scr.MyDisplayHeight,
		 x_start, y_start);
  }

  if(((lastTimestamp-t0) < 3*Scr.ClickTime)&&(mouse_moved==FALSE))
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
MenuItem *FindEntry(int *px_offset /*NULL mans don't return this value */)
{
  MenuItem *mi;
  MenuRoot *mr;
  int root_x, root_y;
  int x,y;
  Window Child;

  /* x_offset returns the x offset of the pointer in the found menu item */
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
  if(x<mr->xoffset || x>mr->width)
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
  int x;
  if(USING_MWM_MENUS)
    x = mr->width - 3;
  else if (USING_WIN_MENUS)
    x = mr->width - 5;
  else /* fvwm menus */
    x = mr->width*2/3;
  return x;
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
    /* *py = mr->selected->y_offset + menu_y - (Scr.EntryHeight/2); */
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
  return (win_attribs.map_state == IsViewable);
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
 ***********************************************************************/
static
MenuStatus menuShortcuts(MenuRoot *menu,XEvent *Event,MenuItem **pmiCurrent)
{
  int fControlKey = Event->xkey.state & ControlMask? TRUE : FALSE;
  int fShiftedKey = Event->xkey.state & ShiftMask? TRUE: FALSE;
  KeySym keysym = XLookupKeysym(&Event->xkey,0);
  MenuItem *newItem;
  MenuItem *miCurrent = pmiCurrent?*pmiCurrent:NULL;
  int index;
  /* Is it okay to treat keysym-s as Ascii? */

  /* Try to match hot keys */
  if (isgraph(keysym) && fControlKey == FALSE) { 
    /* allow any printable character to be a keysym, but be sure control
       isn't pressed */
    MenuItem *mi;
    char key;
    /* Search menu for matching hotkey */
    for (mi = menu->first; mi; mi = mi->next) {
      key = tolower(mi->chHotkey);
      if (keysym == key) {
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
      return MENU_SELECTED;
      break;

    case XK_Left:
    case XK_b: /* back */
    case XK_h: /* vi left */
      return MENU_POPDOWN;
      break;
      
    case XK_Right:
    case XK_f: /* forward */
    case XK_l: /* vi right */
      if (IS_POPUP_MENU_ITEM(miCurrent))
	  return MENU_POPUP;
      break;
      
    case XK_Up:
    case XK_k: /* vi up */
    case XK_p: /* prior */
      if (!miCurrent) {
        if ((*pmiCurrent = menu->last) != NULL)
	  return MENU_NEWITEM;
	else
	  return MENU_NOP;
      }
      if (isgraph(keysym))
	  fControlKey = FALSE; /* don't use control modifier 
				  for k or p, since those might
				  be shortcuts too-- C-k, C-p will
				  always work to do a single up */
      if (fShiftedKey)
	index = 0;
      else {
	index = IndexFromMi(miCurrent) - (fControlKey?5:1);
      }
      newItem = MiFromMenuIndex(miCurrent->mr,index);
      if (newItem) {
	*pmiCurrent = newItem;
	return MENU_NEWITEM;
      } else 
	return MENU_NOP;
      break;

    case XK_Down:
    case XK_j: /* vi down */
    case XK_n: /* next */
      if (!miCurrent) {
        if ((*pmiCurrent = MiFromMenuIndex(menu,0)) != NULL)
	  return MENU_NEWITEM;
	else
	  return MENU_NOP;
      }
      if (isgraph(keysym))
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
      if (newItem) {
	*pmiCurrent = newItem;
	return MENU_NEWITEM;
      } else 
	return MENU_NOP;
      break;
      
      /* Nothing special --- Allow other shortcuts */
    default:
      Keyboard_shortcuts(Event,ButtonRelease);
      break;
    }
  
  return MENU_NOP;
}

int c10msDelaysBeforePopup = 15;
#define MICRO_S_FOR_10MS 10000
#define DELAY_POPUP_MENUS (USING_PREPOP_MENUS && (c10msDelaysBeforePopup > 0))

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
 *      MENU_SELECTED on item selection -- returns MenuItem * in *pmiExecuteAction
 *
 ***********************************************************************/
static
MenuStatus MenuInteraction(MenuRoot *menu,MenuRoot *menuPrior, MenuItem **pmiExecuteAction,
			   int cmenuDeep,Bool fSticks)
{
  Bool fPopupImmediately = USING_PREPOP_MENUS && !DELAY_POPUP_MENUS;
  MenuItem *mi = NULL;
  MenuRoot *mrPopup = NULL;
  MenuRoot *mrNeedsPainting = NULL;
  Bool fDoPopupNow = FALSE;  /* used for delay popups, to just popup the menu */
  Bool fPopupAndWarp = FALSE;  /* used for keystrokes, to popup and move to that menu */
  Bool fKeyPress = FALSE;
  Bool fForceReposition = TRUE;
  int x_init = 0, y_init = 0;
  int x_offset = 0;
  MenuStatus retval = MENU_NOP;
  int c10msDelays = 0;

  /* remember where the pointer was so we can tell if it has moved */
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		 &x_init, &y_init, &JunkX, &JunkY, &JunkMask);

  while (TRUE)
    {
      fPopupAndWarp = FALSE;
      fDoPopupNow = FALSE;
      fKeyPress = FALSE;
      if (fForceReposition) {
	Event.type = MotionNotify;
	Event.xmotion.time = lastTimestamp;
	fForceReposition = FALSE;
      } else if (DELAY_POPUP_MENUS) {
	while (XCheckMaskEvent(dpy, 
			       ButtonPressMask|ButtonReleaseMask|ExposureMask | 
			       KeyPressMask|VisibilityChangeMask|ButtonMotionMask, 
			       &Event) == FALSE) {
	  usleep(MICRO_S_FOR_10MS);
	  if (c10msDelays++ == c10msDelaysBeforePopup) {
	    DBUG("MenuInteraction","Faking motion");
	    /* fake a motion event, and set fDoPopupNow */
	    Event.type = MotionNotify;
	    Event.xmotion.time = lastTimestamp;
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
      /* DBUG("MenuInteraction","mrPopup=%s",mrPopup?mrPopup->name:"(none)"); */
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
	  if(fSticks) {
	    fSticks = FALSE;
            continue;
	    /* break; */
	  }
	  retval = mi?MENU_SELECTED:MENU_ABORTED;
	  goto DO_RETURN;

	case VisibilityNotify:
	case ButtonPress:
	  continue;

	case KeyPress:
	  /* Handle a key press events to allow mouseless operation */
	  fKeyPress = TRUE;
	  x_offset = 0;
	  retval = menuShortcuts(menu,&Event,&mi);
	  /* now warp to the new menu-item, if any */
	  if (mi) {
	    MiWarpPointerToItem(mi,FALSE);
	    /* DBUG("MenuInteraction","Warping on keystroke to %s",mi->item); */
	  }
	  if (retval == MENU_POPUP && IS_POPUP_MENU_ITEM(mi)) {
	    fPopupAndWarp = TRUE;
	    DBUG("MenuInteraction","fPopupAndWarp = TRUE");
	    break;
	  }
	  if (retval == MENU_POPDOWN ||
	      retval == MENU_ABORTED || 
	      retval == MENU_SELECTED)
	    goto DO_RETURN;
	  break;
	  
	case MotionNotify:
	  if (mouse_moved == FALSE) {
	    int x_root, y_root;
	    XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
			   &x_root,&y_root, &JunkX, &JunkY, &JunkMask);
	    if(((x_root-x_init > 3)||(x_init-x_root > 3))&&
	       ((y_root-y_init > 3)||(y_init-y_root > 3))) {
	      /* global variable remember that this isn't just
		 a click any more since the pointer moved */
	      mouse_moved = TRUE;
	    }
	  }
	  mi = FindEntry(&x_offset);
	  break;

	case Expose:
	  /* grab our expose events, let the rest go through */
	  if((XFindContext(dpy, Event.xany.window,MenuContext,
			   (caddr_t *)&mrNeedsPainting)!=XCNOENT)) {
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
	} /* switch */

      /* Now handle new menu items, whether it is from a keypress or
	 a pointer motion event */
      if (mi) {
	/* we're on a menu item */
	MenuRoot *mr = mi->mr;

	/* see if we're on a new item of this menu */
	if (mi != menu->selected && mi->mr == menu) {
	  c10msDelays = 0;
	  /* we're on the same menu, but a different item,
	     so we need to unselect the old item */
	  if (menu->selected) { 
	    /* something else was already selected on this menu */
	    SetMenuItemSelected(menu->selected,FALSE);
	    if (mrPopup) {
	      PopDownMenu(mrPopup);
	      mrPopup = NULL;
	    }
	  } else {
	    /* DBUG("MenuInteraction","Menu %s had nothing else selected",menu->name); */
	  }
	  /* do stuff to highlight the new item; this
	     sets menu->selected, too */
	  SetMenuItemSelected(mi,TRUE);
	}

	if (mr != menu && mr != mrPopup && mr != MrPopupForMi(mi)) {
	/* we're on an item from a prior menu */
	  /* DBUG("MenuInteraction","menu %s: returning popdown",menu->name); */
	  retval = MENU_POPDOWN;
	  goto DO_RETURN;
	} 
	if (mr == mrPopup || fPopupAndWarp ||
	    (!USING_PREPOP_MENUS && IS_POPUP_MENU_ITEM(mi) && x_offset >= mr->width*3/4)) {
	  /* we've moved into a new popup menu (or we need to pop it up and move into it */
	  if ((mr != mrPopup && !fPopupAndWarp) ||
	      (MrPopupForMi(mi) != mrPopup && fPopupAndWarp)) {
  	    /* DBUG("MenuInteraction","mr != mrPopup for menu %s",menu->name); */
	    if (mrPopup) PopDownMenu(mrPopup);
	    mrPopup = MrPopupForMi(mi);
	  }
	  /* DBUG("MenuInteraction","menu %s: doing menu %s",menu->name,mrPopup->name); */
	  if (USING_PREPOP_MENUS && !FMenuMapped(mrPopup)) {
	    /* We want to pop prepop menus so it doesn't *have* to be
	       unpopped; do_menu pops down any menus it pops up, but 
	       we want to be able to popdown w/o actually removing the menu */
	    int x, y;
	    GetPreferredPopupPosition(menu,&x,&y);
	    FPopupMenu(mrPopup,menu,x,y,FALSE);
	  }
	  /* recursively do the new menu we've moved into */
	  retval = do_menu(mrPopup,menu,pmiExecuteAction,cmenuDeep,FALSE,
			   fKeyPress || fPopupAndWarp);
	  if (IS_MENU_RETURN(retval))
	    goto DO_RETURN;
	  if (retval == MENU_POPDOWN) {
	    if (!USING_PREPOP_MENUS)
	      mrPopup = NULL; /* !USING_PREPOP_MENUS means we always need mrPopup == NULL */
	    c10msDelays = 0;
	    fForceReposition = TRUE;
	  }
	  /* DBUG("MenuInteraction","Returned an mrPopup = %s",
	       mrPopup?mrPopup->name:"(none)"); */
	}
	/* Now check whether we can animate the current popup menu 
	   over to the right to unobscure the current menu;  this
	   happens only when using animation */
	if (USING_ANIMATED_MENUS && mrPopup) {
	  int x, y, x_popup, y_popup;
	  int x_where_popup_should_be;
	  /* we have to see if we need menu to be moved */
	  XGetGeometry( dpy, mrPopup->w, &JunkRoot, &x_popup, &y_popup, 
			&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	  XGetGeometry( dpy, menu->w, &JunkRoot, &x, &y, 
			&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	  x_where_popup_should_be = x + PopupPositionOffset(menu);
	  if (x_popup < x_where_popup_should_be) {
	    /* move it back */
	    AnimatedMoveOfWindow(mrPopup->w,x_popup,y_popup,
				 x_where_popup_should_be,
				 y_popup,FALSE /* no warp ptr */,-1,NULL);
	  }
	}
	/* now check whether we should animate the current real menu 
	   over the the right to unobscure the prior menu; only a very
	   limited case where this might be helpful and not too disruptive */
	if (USING_ANIMATED_MENUS && !USING_PREPOP_MENUS && 
	    mrPopup == NULL && menuPrior != NULL &&
	    x_offset < menu->width/4) {
	  int x_prior, y_prior, x_menu, y_menu;
	  int x_where_menu_should_be;
	  DBUG("MenuInteraction","Considering moving the menu back over");
	  /* we have to see if we need menu to be moved */
	  XGetGeometry( dpy, menu->w, &JunkRoot, &x_menu, &y_menu, 
			&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	  XGetGeometry( dpy, menuPrior->w, &JunkRoot, &x_prior, &y_prior, 
			&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	  x_where_menu_should_be = x_prior + PopupPositionOffset(menuPrior);
	  if (x_menu < x_where_menu_should_be) {
	    /* move it back */
	    AnimatedMoveOfWindow(menu->w,x_menu,y_menu,
				 x_where_menu_should_be,
				 y_menu,TRUE /* warp ptr */,-1,NULL);
	  }
	}
	/* now check to see if we need to pop up a new menu without
	   moving into that popup */
	if (USING_PREPOP_MENUS &&
	    IS_POPUP_MENU_ITEM(mi) &&
	    (fDoPopupNow || 
	     (DELAY_POPUP_MENUS && x_offset >= mr->width*3/4) ||
	     (fPopupImmediately && x_offset >= 0))) {
	  int x, y;
	  mrPopup = MrPopupForMi(mi);
	  if (!FMenuMapped(mrPopup)) {
	    /* DBUG("MenuInteraction","menu %s is popping up for viewing %s",
		 menu->name,mrPopup->name); */
	    GetPreferredPopupPosition(menu,&x,&y);
	    FPopupMenu(mrPopup,menu,x,y,FALSE);
	  }
	}
      }
      else
      {
        /* moved off menu, deselect selected item... */
        if (menu->selected) { 
          /* something else was already selected on this menu */
          SetMenuItemSelected(menu->selected,FALSE);
          if (mrPopup) {
            PopDownMenu(mrPopup);
            mrPopup = NULL;
          }
        }
      }
      XFlush(dpy);
    } /* while */
  DO_RETURN:  
  if (mrPopup) {
    PopDownMenu(mrPopup);
    mrPopup = NULL;
  }
  if (retval == MENU_POPDOWN) {
    if (menu->selected)
      SetMenuItemSelected(menu->selected,FALSE);
    if (fKeyPress && menuPrior)
      MiWarpPointerToItem(menuPrior->selected, FALSE);
    /* DBUG("MenuInteraction","Prior menu has %s selected",menuPrior?(menuPrior->selected?
							      menuPrior->selected->item:"(no selected item)"):"(no prior menu)"); */
  } else if (retval == MENU_SELECTED) {
    /* DBUG("MenuInteraction","Got MENU_SELECTED for menu %s, on item %s",menu->name,mi->item); */
    *pmiExecuteAction = mi;
    retval = MENU_ADD_BUTTON_IF(fKeyPress,MENU_DONE);
  }
  return MENU_ADD_BUTTON_IF(fKeyPress,retval);
}

static 
void WarpPointerToTitle(MenuRoot *menu)
{
  int y = Scr.EntryHeight/2;
  int x = MENU_MIDDLE_OFFSET(menu);
  XWarpPointer(dpy, 0, menu->w, 0, 0, 0, 0, 
	       x, y);
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

  y = mi->y_offset + Scr.EntryHeight/2;
  XWarpPointer(dpy, 0, menu->w, 0, 0, 0, 0, x, y);
  return mi;
}

/***********************************************************************
 *
 *  Procedure:
 *	FPopupMenu - pop up a pull down menu
 *
 *  Inputs:
 *	menu	- the root pointer of the menu to pop up
 *	x, y	- location of upper left of menu
 *      center	- whether or not to center horizontally over position
 *
 ***********************************************************************/
static
Bool FPopupMenu (MenuRoot *menu, MenuRoot *menuPrior, 
		 int x, int y, Bool fWarpItem)
{
  Bool fWarpTitle = FALSE;
  if ((!menu)||(menu->w == None)||(menu->items == 0)||(menu->in_use))
    return False;

  /*  RepaintAlreadyReversedMenuItems(menu); */

  InstallRootColormap();

  /* First handle popups from button clicks on buttons in the title bar,
     or the title bar itself */
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
	ButtonPosition(Context,Tmp_win)*Tmp_win->title_height - menu->width+1;
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

  if (x + menu->width > Scr.MyDisplayWidth - 2) {
    /* does not fit horizontally */
    if (menuPrior == NULL) {
      /* Just move it if we're a top level menu */
      x = Scr.MyDisplayWidth - menu->width-2;
    } else if (USING_ANIMATED_MENUS) {
      /* animated the prev menu over */
      int startx, endx;
      int starty, endy;
      XGetGeometry(dpy,menuPrior->w,&JunkRoot,&startx,&starty,&JunkWidth,&JunkHeight,&JunkBW,&JunkDepth);
      endx = startx - (x + menu->width - (Scr.MyDisplayWidth-2));
      endy = starty;
      AnimatedMoveOfWindow(menuPrior->w,startx,starty,endx,endy, TRUE, -1, NULL);
      x = Scr.MyDisplayWidth - menu->width-2;
    } else if (USING_MWM_MENUS || USING_WIN_MENUS) {
      /* put this menu to the left of the previous one */
      int prev_x, prev_y;
      XGetGeometry(dpy,menuPrior->w,&JunkRoot,&prev_x,&prev_y,
		   &JunkWidth,&JunkHeight,&JunkBW,&JunkDepth);
      x = prev_x - menu->width + 2;
    } else {
      x = Scr.MyDisplayWidth - menu->width-2;
    }
  }

  if (y + menu->height > Scr.MyDisplayHeight-2) {
    /* does not fit vertically */
    y = Scr.MyDisplayHeight - menu->height-2;
    if (!USING_PREPOP_MENUS)
      fWarpTitle = TRUE;
  }

  /* clip to screen */
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  XMoveWindow(dpy, menu->w, x, y);
  XMapRaised(dpy, menu->w);

  if (fWarpItem) {
    /* also warp */
    DBUG("FPopupMenu","Warping");
    SetMenuItemSelected(
      MiWarpPointerToItem(menu->first, TRUE /* skip Title */),TRUE);
  } else if(fWarpTitle) {
    /* Warp pointer to middle of top line, since we don't
       want the user to come up directly on an option --
       Not with MWMMenus unless we're at the top level menu */
    WarpPointerToTitle(menu);
  }
  
  return True;
}


/* Set the selected-ness state of the menuitem passed in */
static
void SetMenuItemSelected(MenuItem *mi, Bool f)
{
  Bool fNeedsPainting = mi->state != f; /* paint only if it is changing */
  mi->state = f;
  mi->mr->selected = f?mi:NULL;
  if (fNeedsPainting)
    PaintEntry(mi);
}

/* Returns a menu root that a given menu item pops up */
static
MenuRoot *MrPopupForMi(MenuItem *mi)
{
  /* just look past "Popup " in the action, and find that menu root */
  if (mi)
    return FindPopup(&mi->action[5]);
  return NULL;
}


/***********************************************************************
 *
 *  Procedure:
 *	PopDownMenu - unhighlight the current menu selection and
 *		take down the menus
 *
 ***********************************************************************/
static
void PopDownMenu(MenuRoot *mr)
{
  MenuItem *mi;
  assert(mr);
  
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
}

/***************************************************************************
 * 
 * Wait for all mouse buttons to be released 
 * This can ease some confusion on the part of the user sometimes 
 * 
 * Discard superflous button events during this wait period.
 *
 ***************************************************************************/
void WaitForButtonsUp()
{
  Bool AllUp = False;
  XEvent JunkEvent;
  unsigned int mask;

  while(!AllUp)
    {
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &JunkX, &JunkY, &mask);    
      
      if((mask&
	  (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask))==0)
	AllUp = True;
    }
  XSync(dpy,0);
  while(XCheckMaskEvent(dpy,
			ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
			&JunkEvent))
    {
      StashEventTime (&JunkEvent);
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
    }

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

  y_offset = mi->y_offset;
  y_height = mi->y_height;
  text_y = y_offset + Scr.StdFont.y;
  /* center text vertically if the pixmap is taller */
  if(mi->picture)
    text_y+=mi->picture->height;
  if (mi->lpicture)
  {
    y = mi->lpicture->height - Scr.StdFont.height;
    if (y>1)
      text_y += y/2;
  }

  ShadowGC = Scr.MenuShadowGC;
  if(Scr.d_depth<2)
    ReliefGC = Scr.MenuShadowGC;
  else
    ReliefGC = Scr.MenuReliefGC;

  if(USING_MWM_MENUS) {
    if((!mi->prev)||(!mi->prev->state))
      XClearArea(dpy, mr->w,mr->xoffset,y_offset-1,mr->width,y_height+2,0);
    else
      XClearArea(dpy, mr->w,mr->xoffset,y_offset+1,mr->width,y_height-1,0);
    if ((mi->state)&&(!mi->fIsSeparator)&&
	(((*mi->item)!=0) || mi->picture || mi->lpicture))
      {
	/* thicker shadow border for selected button for motif */
	RelieveRectangle(mr->w, mr->xoffset+3, y_offset, mr->width-5, mi->y_height,
			 ReliefGC,ShadowGC);
	RelieveRectangle(mr->w, mr->xoffset+2, y_offset-1, mr->width-3, mi->y_height+2,
			 ReliefGC,ShadowGC);
      }
    RelieveHalfRectangle(mr->w, 0, y_offset-1, mr->width,
			 y_height+2, ReliefGC, ShadowGC);
  } else if (USING_WIN_MENUS) {
    if (mi->state && (!mi->fIsSeparator) &&
	(((*mi->item)!=0) || mi->picture || mi->lpicture)) {
      XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
      XFillRectangle(dpy, mr->w, Scr.MenuShadowGC, mr->xoffset+3, y_offset,
		     mr->width - mr->xoffset-6, y_height);
    } else {
      XClearArea(dpy,mr->w,mr->xoffset+3,y_offset,mr->width -mr->xoffset-6,y_height,0);
    }
    RelieveHalfRectangle(mr->w, 0, y_offset-1, mr->width,
			 y_height+3, ReliefGC, ShadowGC);
  } else {
    XClearArea(dpy, mr->w, mr->xoffset,y_offset,mr->width,y_height,0);
    if ((mi->state)&&(!mi->fIsSeparator)&&
	(((*mi->item)!=0)|| mi->picture || mi->lpicture ))
      RelieveRectangle(mr->w, mr->xoffset+2, y_offset, mr->width-4, mi->y_height,
		       ReliefGC,ShadowGC);
    RelieveHalfRectangle(mr->w, 0, y_offset, mr->width,
			 y_height, ReliefGC, ShadowGC);
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
      if(USING_MWM_MENUS)
	{
	  text_y += HEIGHT_EXTRA/2;
	  XDrawLine(dpy, mr->w, ShadowGC, mr->xoffset+2, y_offset+y_height-2,
		    mr->width-3, y_offset+y_height-2);
	  XDrawLine(dpy, mr->w, ShadowGC, mr->xoffset+2, y_offset+y_height-4,
		    mr->width-3, y_offset+y_height-4);
	}      
      else if (USING_WIN_MENUS || USING_FVWM_MENUS)
	{
	  if(mi->next != NULL)
	    {
	      DrawSeparator(mr->w,ShadowGC,ReliefGC,mr->xoffset+5, y_offset+y_height-3,
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
  /* see if it's am actual separator (titles are also separators) */
  if(mi->fIsSeparator && !IS_TITLE_MENU_ITEM(mi) && !IS_LABEL_MENU_ITEM(mi))
    {
      if(USING_MWM_MENUS)
	DrawSeparator(mr->w,ShadowGC,ReliefGC,mr->xoffset+2,y_offset-1+HEIGHT_SEPARATOR/2,
		      mr->width-3,y_offset-1+HEIGHT_SEPARATOR/2,0);
      else /* Fvwm and Win */
	DrawSeparator(mr->w,ShadowGC,ReliefGC,mr->xoffset+5,y_offset-1+HEIGHT_SEPARATOR/2,
		      mr->width-6,y_offset-1+HEIGHT_SEPARATOR/2,1);
    }
  if(mi->next == NULL) 
    DrawSeparator(mr->w,ShadowGC,ShadowGC,mr->xoffset+1,mr->height-2,
		  mr->width-2, mr->height-2,1);
  if(mi == mr->first) 
    DrawSeparator(mr->w,ReliefGC,ReliefGC,mr->xoffset,0, mr->width-1,0,-1);

  if(check_allowed_function(mi))
  /* if(check_allowed_function2(mi->func_type,Tmp_win)) */
    currentGC = Scr.MenuGC;
  else
    /* should be a shaded out word, not just re-colored. */
    currentGC = Scr.MenuStippleGC;    

  if (USING_WIN_MENUS && mi->state && (mi->fIsSeparator == FALSE))
    /* Use a lighter color for highlighted windows menu items for win mode */
    currentGC = ReliefGC;

  if(*mi->item)
    XDrawString(dpy, mr->w,currentGC,mi->x+mr->xoffset,text_y, mi->item, mi->strlen);
  if(mi->strlen2>0)
    XDrawString(dpy, mr->w,currentGC,mi->x2+mr->xoffset,text_y, mi->item2,mi->strlen2);

  /* pete@tecc.co.uk: If the item has a hot key, underline it */
  if (mi->hotkey > 0)
    DrawUnderline(mr->w, currentGC,mr->xoffset+mi->x,text_y,mi->item,mi->hotkey - 1);
  if (mi->hotkey < 0)
   DrawUnderline(mr->w, currentGC,mr->xoffset+mi->x2,text_y,mi->item2, -1 - mi->hotkey);

  d=(Scr.EntryHeight-7)/2;
  if(mi->func_type == F_POPUP)
    if(mi->state)
      DrawTrianglePattern(mr->w, ShadowGC, ReliefGC, ShadowGC, ReliefGC,
			  mr->width-d-8, y_offset+d-1, mr->width-d-1, y_offset+d+7);
    else
      DrawTrianglePattern(mr->w, ReliefGC, ShadowGC, ReliefGC, Scr.MenuGC,
			  mr->width-d-8, y_offset+d-1, mr->width-d-1, y_offset+d+7);

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
	  XCopyArea(dpy,mi->picture->picture,mr->w,ReliefGC,	0,0,
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
  if (mr->sidePic == NULL)
    return;

  if(Scr.d_depth<2)
    ReliefGC = Scr.MenuShadowGC; 
  else
    ReliefGC = Scr.MenuReliefGC;
  TextGC = Scr.MenuGC;

  if(mr->colorize) {
    Globalgcv.foreground = mr->sideColor;
    Globalgcm = GCForeground;
    XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
    XFillRectangle(dpy, mr->w, Scr.ScratchGC1, 3, 3,
                   mr->sidePic->width, mr->height - 6);
  }
    
  if(mr->sidePic->depth > 0) /* pixmap? */
    {
      Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
      Globalgcv.clip_mask = mr->sidePic->mask;
      Globalgcv.clip_x_origin = 3;
      Globalgcv.clip_y_origin = mr->height - mr->sidePic->height -3;
      
      XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
      XCopyArea(dpy, mr->sidePic->picture, mr->w,
                ReliefGC, 0, 0,
                mr->sidePic->width, mr->sidePic->height,
                Globalgcv.clip_x_origin, Globalgcv.clip_y_origin);
      Globalgcm = GCClipMask;
      Globalgcv.clip_mask = None;
      XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);    
    } else {
      XCopyPlane(dpy, mr->sidePic->picture, mr->w,
                 TextGC, 0, 0,
                 mr->sidePic->width, mr->sidePic->height,
                 1, mr->height - mr->sidePic->height, 1);
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
void  DrawUnderline(Window w, GC gc, int x, int y, char *txt, int posn) 
{
  int off1 = XTextWidth(Scr.StdFont.font, txt, posn);
  int off2 = XTextWidth(Scr.StdFont.font, txt, posn + 1) - 1;
  XDrawLine(dpy, w, gc, x + off1, y + 2, x + off2, y + 2);
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
void DrawTrianglePattern(Window w,GC GC1,GC GC2,GC GC3,GC gc,int l,int u,int r,int b)
{
  int m;

  m = (u + b)/2;

  if (USING_WIN_MENUS) {
      XPoint points[3];
      points[0].x = l; points[0].y = u;
      points[1].x = l; points[1].y = b;
      points[2].x = r; points[2].y = m;
      XFillPolygon(dpy, w, gc, points, 3, Convex, CoordModeOrigin);
  } else {
    /* Fvwm and MWM menus */
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
void PaintMenu(MenuRoot *mr, XEvent *e)
{
  MenuItem *mi;
  
  GC ReliefGC;

  if(Scr.d_depth<2)
    ReliefGC = Scr.MenuShadowGC;
  else
    ReliefGC = Scr.MenuReliefGC;

  for (mi = mr->first; mi != NULL; mi = mi->next)
    {
      /* be smart about handling the expose, redraw only the entries
       * that we need to
       */
      if (e->xexpose.y < (mi->y_offset + mi->y_height) &&
	  (e->xexpose.y + e->xexpose.height) > mi->y_offset)
	{
	  PaintEntry(mi);
	}
    }

  PaintSidePic(mr);
  XSync(dpy, 0);
  return;
}


void DestroyMenu(MenuRoot *mr)
{
  MenuItem *mi,*tmp2;
  MenuRoot *tmp, *prev;

  if(mr == NULL)
    return;

  tmp = Scr.AllMenus;
  prev = NULL;
  while((tmp != NULL)&&(tmp != mr))
    {
      prev = tmp;
      tmp = tmp->next;
    }
  if(tmp != mr)
    return;

  if(prev == NULL)
    Scr.AllMenus = mr->next;
  else
    prev->next = mr->next;

  XDestroyWindow(dpy,mr->w);
  XDeleteContext(dpy, mr->w, MenuContext);  
  
  if (mr->sidePic)
      DestroyPicture(dpy, mr->sidePic);

  /* need to free the window list ? */
  mi = mr->first;
  while(mi != NULL)
    {
      tmp2 = mi->next;
      if (mi->item != NULL) free(mi->item);
      if (mi->item2 != NULL) free(mi->item2);
      if (mi->action != NULL) free(mi->action);
      if(mi->picture)
	DestroyPicture(dpy,mi->picture);
      if(mi->lpicture)
	DestroyPicture(dpy,mi->lpicture);
      free(mi);
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

  mr = Scr.AllMenus;
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

  if((mr->func != F_POPUP)||(!Scr.flags & WindowsCaptured))
    return;

  /* allow two pixels for top border */
  mr->width = 0;
  mr->width2 = 0;
  mr->width0 = 0;
  for (cur = mr->first; cur != NULL; cur = cur->next)
    {
      width = XTextWidth(Scr.StdFont.font, cur->item, cur->strlen);
      if(cur->picture && width < cur->picture->width)
	width = cur->picture->width;
      if(cur->func_type == F_POPUP)
	width += 15;
      if (width <= 0)
	width = 1;
      if (width > mr->width)
	mr->width = width;

      width = XTextWidth(Scr.StdFont.font, cur->item2, cur->strlen2);
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
	  /* Title */
	  if(cur->strlen2  == 0)
	    cur->x = (mr->width+mr->width2+mr->width0
		      - XTextWidth(Scr.StdFont.font, cur->item,
					     cur->strlen))/2;
	  
	  if((cur->strlen > 0)||(cur->strlen2>0))
	    {
	      if(USING_MWM_MENUS)
		cur->y_height = Scr.EntryHeight + HEIGHT_EXTRA_TITLE;
	      else
		{
		  if((cur == mr->first)||(cur->next == NULL))
		    cur->y_height=Scr.EntryHeight-HEIGHT_EXTRA+1+
		      (HEIGHT_EXTRA_TITLE/2);
		  else
		    cur->y_height = Scr.EntryHeight -HEIGHT_EXTRA +1+
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
              StrEquals(cur->action,"nop")) { /* added check for NOP to distinguish from items with no text, only pixmap */
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
          cur->y_height = Scr.EntryHeight;
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
      if (y+Scr.EntryHeight > Scr.MyDisplayHeight &&
	  cur->next != NULL)
	{
	char *szMenuContinuationActionAndName = 
	  (char *) safemalloc((8+strlen(mr->name))*sizeof(char));
	MenuRoot *menuContinuation;
	if (mr->continuation != NULL) {
	  fvwm_msg(ERR,"MakeMenu","Confused-- expected continuation to be null");
	  break;
	}
	strcpy(szMenuContinuationActionAndName,"Popup ");
	strcat(szMenuContinuationActionAndName, mr->name);
	strcat(szMenuContinuationActionAndName,"$");
	/* NewMenuRoot inserts at the head of the list of menus 
	   but, we need it at the end */
	/* (Give it just the name, with is 6 chars passed the action
	   since strlen("Popup ")==6 ) */
	menuContinuation = NewMenuRoot(szMenuContinuationActionAndName+6,0);
	mr->continuation = menuContinuation;

	/* Now move this item and the remaining items into the new menu */
	cItems--;
	menuContinuation->first = cur;
	menuContinuation->last = mr->last;
	menuContinuation->items = mr->items - cItems;

	/* cur_prev is now the last item in the current menu */
	mr->last = cur_prev;
	mr->items = cItems;
	cur_prev->next = NULL;

	/* Go back one, so that this loop will process the new item */
	y -= cur->y_height;
	cur = cur_prev;

	/* And add the entry pointing to the new menu */
	AddToMenu(mr,"More&...",szMenuContinuationActionAndName,FALSE /* no pixmap scan */);
	MakeMenu(menuContinuation);

	}
    }
  mr->in_use = 0;
  mr->height = y+2;
  
  
#ifndef NO_SAVEUNDERS   
  valuemask = (CWBackPixel | CWEventMask | CWCursor | CWSaveUnder);
#else
  valuemask = (CWBackPixel | CWEventMask | CWCursor);
#endif
  attributes.background_pixel = Scr.MenuColors.back;
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

  mr->width = mr->width0 + mr->width + mr->width2 + mr->xoffset;

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
	    for (; *txt != '\0'; txt++) txt[0] = txt[1];	/* Copy down..	*/
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
  char *tstart, *txt, *save_instring;
  int i;
  char name[100];

  *c = False;

  /* save instring in case can't find pixmap */
  save_instring = (char *)safemalloc(strlen(instring)+1);
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
        while((*txt != identifier)&&(*txt != '\0')&&(i<99))
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
          free(save_instring);
        return;
      }
    }
}

void scanForPixmap(char *instring, Picture **p, char identifier) 
{
  char *tstart, *txt, *save_instring;
  int i;
  Picture *pp;
  char name[100];
  extern char *IconPath;
#ifdef XPM
  extern char *PixmapPath;
#endif

  /* save instring in case can't find pixmap */
  save_instring = (char *)safemalloc(strlen(instring)+1);
#ifdef UGLY_WHEN_PIXMAPS_MISSING
  strcpy(save_instring,instring);
#endif

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
	      for (tmp = txt; *tmp != '\0'; tmp++) tmp[0] = tmp[1];
	      continue;		/* ...And skip to the key char		*/
	    }
	  /* It's a hot key marker - work out the offset value		*/
	  tstart = txt;
	  txt++;
	  i=0;
	  while((*txt != identifier)&&(*txt != '\0')&&(i<99))
	    {
	      name[i] = *txt;
	      txt++;
	      i++;
	    }
	  name[i] = 0;

	  /* Next, check for a color pixmap */
#ifdef XPM
	  pp=CachePicture(dpy,Scr.Root,IconPath,PixmapPath,name,Scr.ColorLimit);
#else
	  pp=CachePicture(dpy,Scr.Root,IconPath,IconPath,name,Scr.ColorLimit);
#endif
	  if(*txt != '\0')txt++;
	  while(*txt != '\0')
	    {
	      *tstart++ = *txt++;
	    }
	  *tstart = 0;
#ifdef UGLY_WHEN_PIXMAPS_MISSING
          if(!pp)
            strcpy(instring,save_instring);
	  else
	    *p=pp;
#else
	  if (pp) 
	      *p=pp;
	  else {
  	    fvwm_msg(WARN,"scanForPixmap","Couldn't find pixmap %s",name);
	  }
#endif
          free(save_instring);
	  return;
	}
    }
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
void AddToMenu(MenuRoot *menu, char *item, char *action, Bool fPixmapsOk)
{
  MenuItem *tmp;
  char *start,*end;

  if(item == NULL)
     return;

  tmp = (MenuItem *)safemalloc(sizeof(MenuItem));
  tmp->chHotkey = '\0';
  tmp->next = NULL;
  tmp->mr = menu; /* this gets updated in MakeMenu if we split the menu
		     because it's too large vertically */
  if (menu->first == NULL)
    {
      menu->first = tmp;
      tmp->prev = NULL;
    }
  else
    {
      menu->last->next = tmp;
      tmp->prev = menu->last;
    }
  menu->last = tmp;
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
      while(*end != 0)
	end++;  
      if(end-start != 0)
	{
	  tmp->item2 = safemalloc(end-start+1);
	  strncpy(tmp->item2,start,end-start);
	  tmp->item2[end-start] = 0;
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
	  fvwm_msg(WARN,"AddToMenu","Hotkey %c is reused in menu %s; second binding ignored.",ch,
		   menu->name);
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
	    fvwm_msg(WARN,"AddToMenu","Hotkey %c is reused in menu %s; second binding ignored.",ch,
		     menu->name);
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
  tmp->func_type = find_func_type(tmp->action);
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
 *      fFunction - non-zero if we shoot set func to F_FUNCTION, 
 *                  F_POPUP otherwise
 *
 ***********************************************************************/
MenuRoot *NewMenuRoot(char *name,int fFunction)
{
  MenuRoot *tmp;
  
  tmp = (MenuRoot *) safemalloc(sizeof(MenuRoot));
  if(fFunction != 0)
    tmp->func = F_FUNCTION;
  else
    tmp->func = F_POPUP;

  tmp->name = stripcpy(name);
  tmp->first = NULL;
  tmp->last = NULL;
  tmp->selected = NULL;
  tmp->items = 0;
  tmp->width = 0;
  tmp->width2 = 0;
  tmp->w = None;
  tmp->next  = Scr.AllMenus;
  tmp->continuation = NULL;
  tmp->sidePic = NULL;
  scanForPixmap(tmp->name, &tmp->sidePic, '@');
  if (tmp->sidePic != NULL) {
    /* DBUG("NewMenuRoot","Got side pixmap, name = %s\n",tmp->name); */
  }
  scanForColor(tmp->name,&tmp->sideColor,&tmp->colorize,'^');
  Scr.AllMenus = tmp;
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
