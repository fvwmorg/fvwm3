/****************************************************************************
 *
 * This is all original code by Robert Nation
 * which reads motif mwm window manager
 * hints from a window, and makes necessary adjustments for fvwm.
 *
 * Definitions of the hint structure and the constants are courtesy of
 * mitnits@bgumail.bgu.ac.il (Roman Mitnitski ), who sent this note,
 * after conferring with a friend at the OSF:
 * Hi, Rob.
 *
 *I'm happy to announce, that you can use motif public
 *headers in any way you can... I just got the letter from
 *my friend, it says literally:
 *
 *>    Hi.
 *>
 *> Yes, you can use motif public header files, in particular
 *> because there is NO limitation on inclusion of this files
 *> in your programms....Also, no one can put copyright to the NUMBERS
 *> (I mean binary flags for decorations) or DATA STRUCTURES
 *> (I mean little structure used by motif to pass description
 *> of the decorations to the mwm). Call it another name, if you are
 *> THAT MUCH concerned.
 *>
 *> You can even use the little piece of code I've passed to you -
 *> we are talking about 10M distribution against two pages of code.
 *> Don't be silly.
 *>
 *> Best wishes.
 *> Eli.
 *
 *
 ****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fvwm.h"
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "screen.h"
#include "parse.h"
#include "menus.h"
#include "lang-strings.h"


extern Atom _XA_MwmAtom;

/* Motif  window hints */
typedef struct
{
    CARD32      flags;
    CARD32      functions;
    CARD32      decorations;
    INT32       inputMode;
} PropMotifWmHints;

typedef PropMotifWmHints        PropMwmHints;

/* Motif window hints */
#define MWM_HINTS_FUNCTIONS           (1L << 0)
#define MWM_HINTS_DECORATIONS         (1L << 1)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL                 (1L << 0)
#define MWM_DECOR_BORDER              (1L << 1)
#define MWM_DECOR_RESIZEH             (1L << 2)
#define MWM_DECOR_TITLE               (1L << 3)
#define MWM_DECOR_MENU                (1L << 4)
#define MWM_DECOR_MINIMIZE            (1L << 5)
#define MWM_DECOR_MAXIMIZE            (1L << 6)

#define PROP_MOTIF_WM_HINTS_ELEMENTS  4
#define PROP_MWM_HINTS_ELEMENTS       PROP_MOTIF_WM_HINTS_ELEMENTS

/* bit definitions for OL hints; I just
 *  made these up, OL stores hints as atoms */
#define OL_DECOR_CLOSE                (1L << 0)
#define OL_DECOR_RESIZEH              (1L << 1)
#define OL_DECOR_HEADER               (1L << 2)
#define OL_DECOR_ICON_NAME            (1L << 3)
#define OL_DECOR_ALL                  (OL_DECOR_CLOSE | OL_DECOR_RESIZEH | OL_DECOR_HEADER | OL_DECOR_ICON_NAME)

extern FvwmWindow *Tmp_win;

/****************************************************************************
 *
 * Reads the property MOTIF_WM_HINTS
 *
 *****************************************************************************/
void GetMwmHints(FvwmWindow *t)
{
  int actual_format;
  Atom actual_type;
  unsigned long nitems, bytesafter;

  if(XGetWindowProperty (dpy, t->w, _XA_MwmAtom, 0L, 20L, False,
			 _XA_MwmAtom, &actual_type, &actual_format, &nitems,
			 &bytesafter,(unsigned char **)&t->mwm_hints)==Success)
    {
      if(nitems >= PROP_MOTIF_WM_HINTS_ELEMENTS)
	{
	  return;
	}
    }

  t->mwm_hints = NULL;
}

/****************************************************************************
 *
 * Reads the openlook properties _OL_WIN_ATTR, _OL_DECOR_ADD, _OL_DECOR_DEL
 *
 * _OL_WIN_ATTR - the win_type field is the either the first atom if the
 * property has length three or the second atom if the property has
 * length five.  It can be any of:
 *     _OL_WT_BASE (no changes)
 *     _OL_WT_CMD (no minimize decoration)
 *     _OL_WT_HELP (no minimize, maximize or resize handle decorations)
 *     _OL_WT_NOTICE (no minimize, maximize, system menu, resize handle
 *         or titlebar decorations)
 *     _OL_WT_OTHER (no minimize, maximize, system menu, resize handle
 *         or titlebar decorations)
 * In addition, if the _OL_WIN_ATTR property is in the three atom format
 * or if the type is _OL_WT_OTHER, then the icon name is not displayed
 * (same behavior as olvwm).
 *
 * _OL_DECOR_ADD or _OL_DECOR_DEL - indivdually add or remove minimize
 * button (_OL_DECOR_CLOSE), resize handles (_OL_DECOR_RESIZE), title bar
 * (_OL_DECOR_HEADER), or icon name (_OL_DECOR_ICON_NAME).
 *
 * The documentation for the Open Look hints was taken from "Advanced X
 * Window Application Programming", Eric F. Johnson and Kevin Reichard
 * (M&T Books), and the olvwm source code (available at ftp.x.org in
 * /R5contrib).
 *****************************************************************************/
void GetOlHints(FvwmWindow *t)
{
  int actual_format;
  Atom actual_type;
  unsigned long nitems, bytesafter;
  Atom *hints;
  int i;
  Atom win_type;

  t->ol_hints = OL_DECOR_ALL;

  if (XGetWindowProperty (dpy, t->w, _XA_OL_WIN_ATTR, 0L, 20L, False,
                          _XA_OL_WIN_ATTR, &actual_type, &actual_format, &nitems,
                          &bytesafter,(unsigned char **)&hints)==Success)
    {
      if (nitems > 0)
        {
          if (nitems == 3)
            win_type = hints[0];
          else
            win_type = hints[1];

	  /* got this from olvwm and sort of mapped it to
	   * FVWM/MWM hints */
          if (win_type == _XA_OL_WT_BASE)
	    t->ol_hints = OL_DECOR_ALL;
          else if (win_type == _XA_OL_WT_CMD)
	    t->ol_hints = OL_DECOR_ALL & ~OL_DECOR_CLOSE;
          else if (win_type == _XA_OL_WT_HELP)
	    t->ol_hints = OL_DECOR_ALL & ~(OL_DECOR_CLOSE | OL_DECOR_RESIZEH);
          else if (win_type == _XA_OL_WT_NOTICE)
	    t->ol_hints = OL_DECOR_ALL & ~(OL_DECOR_CLOSE | OL_DECOR_RESIZEH |
	        OL_DECOR_HEADER | OL_DECOR_ICON_NAME);
          else if (win_type == _XA_OL_WT_OTHER)
	    t->ol_hints = 0;
          else
	    t->ol_hints = OL_DECOR_ALL;

	  if (nitems == 3)
	    t->ol_hints &= ~OL_DECOR_ICON_NAME;
	}

      if (hints)
        XFree (hints);
    }

  if(XGetWindowProperty (dpy, t->w, _XA_OL_DECOR_ADD, 0L, 20L, False,
			 XA_ATOM, &actual_type, &actual_format, &nitems,
			 &bytesafter,(unsigned char **)&hints)==Success)
    {
      for (i = 0; i < nitems; i++) {
	if (hints[i] == _XA_OL_DECOR_CLOSE)
            t->ol_hints |= OL_DECOR_CLOSE;
	else if (hints[i] == _XA_OL_DECOR_RESIZE)
            t->ol_hints |= OL_DECOR_RESIZEH;
	else if (hints[i] == _XA_OL_DECOR_HEADER)
            t->ol_hints |= OL_DECOR_HEADER;
	else if (hints[i] == _XA_OL_DECOR_ICON_NAME)
            t->ol_hints |= OL_DECOR_ICON_NAME;
      }
      if (hints)
        XFree (hints);
    }

  if(XGetWindowProperty (dpy, t->w, _XA_OL_DECOR_DEL, 0L, 20L, False,
			 XA_ATOM, &actual_type, &actual_format, &nitems,
			 &bytesafter,(unsigned char **)&hints)==Success)
    {
      for (i = 0; i < nitems; i++) {
	if (hints[i] == _XA_OL_DECOR_CLOSE)
            t->ol_hints &= ~OL_DECOR_CLOSE;
	else if (hints[i] == _XA_OL_DECOR_RESIZE)
            t->ol_hints &= ~OL_DECOR_RESIZEH;
	else if (hints[i] == _XA_OL_DECOR_HEADER)
            t->ol_hints &= ~OL_DECOR_HEADER;
	else if (hints[i] == _XA_OL_DECOR_ICON_NAME)
            t->ol_hints &= ~OL_DECOR_ICON_NAME;
      }
      if (hints)
        XFree (hints);
    }

}


/****************************************************************************
 *
 * Interprets the property MOTIF_WM_HINTS, sets decoration and functions
 * accordingly
 *
 *****************************************************************************/
void SelectDecor(FvwmWindow *t, unsigned long tflags, int border_width,
		 int resize_width)
{
  int decor,i;
  PropMwmHints *prop;

  if(!(tflags & BW_FLAG))
    border_width = Scr.NoBoundaryWidth;

  if(!(tflags & NOBW_FLAG))
    resize_width = Scr.BoundaryWidth;

  for(i=0;i<5;i++)
    {
      t->left_w[i] = 1;
      t->right_w[i] = 1;
    }

  decor = MWM_DECOR_ALL;
  t->functions = MWM_FUNC_ALL;
  if(t->mwm_hints)
    {
      prop = (PropMwmHints *)t->mwm_hints;
      if(tflags & MWM_DECOR_FLAG)
	if(prop->flags & MWM_HINTS_DECORATIONS)
	  decor = prop->decorations;
      if(tflags & MWM_FUNCTIONS_FLAG)
	if(prop->flags & MWM_HINTS_FUNCTIONS)
	  t->functions = prop->functions;
    }

  /* functions affect the decorations! if the user says
   * no iconify function, then the iconify button doesn't show
   * up. */
  if(t->functions & MWM_FUNC_ALL)
    {
      /* If we get ALL + some other things, that means to use
       * ALL except the other things... */
      t->functions &= ~MWM_FUNC_ALL;
      t->functions = (MWM_FUNC_RESIZE | MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE |
		   MWM_FUNC_MAXIMIZE | MWM_FUNC_CLOSE) & (~(t->functions));
    }
  if((tflags & MWM_FUNCTIONS_FLAG) && (t->flags & TRANSIENT))
    {
      t->functions &= ~(MWM_FUNC_MAXIMIZE|MWM_FUNC_MINIMIZE);
    }

  if(decor & MWM_DECOR_ALL)
    {
      /* If we get ALL + some other things, that means to use
       * ALL except the other things... */
      decor &= ~MWM_DECOR_ALL;
      decor = (MWM_DECOR_BORDER | MWM_DECOR_RESIZEH | MWM_DECOR_TITLE |
	       MWM_DECOR_MENU | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE)
	& (~decor);
    }

  /* now remove any functions specified in the OL hints */
  if (tflags & OL_DECOR_FLAG)
    {
      if (!(t->ol_hints & OL_DECOR_CLOSE))
        t->functions &= ~MWM_FUNC_MINIMIZE;
      if (!(t->ol_hints & OL_DECOR_RESIZEH))
        t->functions &= ~(MWM_FUNC_RESIZE | MWM_FUNC_MAXIMIZE);
      if (!(t->ol_hints & OL_DECOR_HEADER))
        t->functions &= ~(MWM_DECOR_MENU | MWM_FUNC_MINIMIZE |
                          MWM_FUNC_MAXIMIZE | MWM_DECOR_TITLE);
      if (!(t->ol_hints & OL_DECOR_ICON_NAME))
	t->flags |= NOICON_TITLE;
    }

  /* Now I have the un-altered decor and functions, but with the
   * ALL attribute cleared and interpreted. I need to modify the
   * decorations that are affected by the functions */
  if(!(t->functions & MWM_FUNC_RESIZE))
    decor &= ~MWM_DECOR_RESIZEH;
  /* MWM_FUNC_MOVE has no impact on decorations. */
  if(!(t->functions & MWM_FUNC_MINIMIZE))
    decor &= ~MWM_DECOR_MINIMIZE;
  if(!(t->functions & MWM_FUNC_MAXIMIZE))
    decor &= ~MWM_DECOR_MAXIMIZE;
  /* MWM_FUNC_CLOSE has no impact on decorations. */

  /* This rule is implicit, but its easier to deal with if
   * I take care of it now */
  if(decor & (MWM_DECOR_MENU| MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE))
    decor |= MWM_DECOR_TITLE;

  /* Selected the mwm-decor field, now trim down, based on
   * .fvwmrc entries */
  if ((tflags & NOTITLE_FLAG)||
      ((!(tflags & DECORATE_TRANSIENT_FLAG)) && (t->flags & TRANSIENT)))
    decor &= ~MWM_DECOR_TITLE;

  if ((tflags & NOBORDER_FLAG)||
      ((!(tflags&DECORATE_TRANSIENT_FLAG)) && (t->flags & TRANSIENT)))
    decor &= ~MWM_DECOR_RESIZEH;

  if((tflags & MWM_DECOR_FLAG) && (t->flags & TRANSIENT))
    {
      decor &= ~(MWM_DECOR_MAXIMIZE|MWM_DECOR_MINIMIZE);
    }

#ifdef SHAPE
  if (ShapesSupported)
  {
    if(t->wShaped)
      decor &= ~(BORDER|MWM_DECOR_RESIZEH);
  }
#endif
  /* Assume no decorations, and build up */
  t->flags &= ~(BORDER|TITLE);
  if(tflags & MWM_BORDER_FLAG)
    t->flags |= MWMBorders;
  if(tflags & MWM_BUTTON_FLAG)
    t->flags |= MWMButtons;
  if(tflags & MWM_OVERRIDE_FLAG)
    t->flags |= HintOverride;
  t->boundary_width = 0;
  t->corner_width = 0;
  t->title_height = 0;

  if(decor & MWM_DECOR_BORDER)
    {
      /* A narrow border is displayed (5 pixels - 2 relief, 1 top,
       * (2 shadow) */
      t->boundary_width = border_width;
    }
  if(decor & MWM_DECOR_TITLE)
    {
      /*  A title barm with no buttons in it
       * window gets a 1 pixel wide black border. */
      t->flags |= TITLE;
      t->title_height = GetDecor(t,TitleHeight);
    }
  if(decor & MWM_DECOR_RESIZEH)
    {
      /* A wide border, with corner tiles is desplayed
       * (10 pixels - 2 relief, 2 shadow) */
      t->flags |= BORDER;
      t->boundary_width = resize_width;
      t->corner_width = GetDecor(t,TitleHeight) + t->boundary_width;
    }
  if(!(decor & MWM_DECOR_MENU))
    {
      /*  title-bar menu button omitted
       * window gets 1 pixel wide black border */
      /* disable any buttons with the MWMDecorMenu flag */
      int i;
      for (i = 0; i < 5; ++i) {
        if (GetDecor(t,left_buttons[i].flags)&MWMDecorMenu)
          t->left_w[i] = None;
        if (GetDecor(t,right_buttons[i].flags)&MWMDecorMenu)
          t->right_w[i] = None;
      }
    }
  if(!(decor & MWM_DECOR_MINIMIZE))
    {
      /* title-bar + iconify button, no menu button.
       * window gets 1 pixel wide black border */
      /* disable any buttons with the MWMDecorMinimize flag */
      int i;
      for (i = 0; i < 5; ++i) {
        if (GetDecor(t,left_buttons[i].flags)&MWMDecorMinimize)
          t->left_w[i] = None;
        if (GetDecor(t,right_buttons[i].flags)&MWMDecorMinimize)
          t->right_w[i] = None;
      }
    }
  if(!(decor & MWM_DECOR_MAXIMIZE))
    {
      /* title-bar + maximize button, no menu button, no iconify.
       * window has 1 pixel wide black border */
      /* disable any buttons with the MWMDecorMaximize flag */
      int i;
      for (i = 0; i < 5; ++i) {
        if (GetDecor(t,left_buttons[i].flags)&MWMDecorMaximize)
          t->left_w[i] = None;
        if (GetDecor(t,right_buttons[i].flags)&MWMDecorMaximize)
          t->right_w[i] = None;
      }
    }
  if (t->buttons & BUTTON1) t->left_w[0]=None;
  if (t->buttons & BUTTON3) t->left_w[1]=None;
  if (t->buttons & BUTTON5) t->left_w[2]=None;
  if (t->buttons & BUTTON7) t->left_w[3]=None;
  if (t->buttons & BUTTON9) t->left_w[4]=None;

  if (t->buttons & BUTTON2) t->right_w[0]=None;
  if (t->buttons & BUTTON4) t->right_w[1]=None;
  if (t->buttons & BUTTON6) t->right_w[2]=None;
  if (t->buttons & BUTTON8) t->right_w[3]=None;
  if (t->buttons & BUTTON10)t->right_w[4]=None;

  t->nr_left_buttons = Scr.nr_left_buttons;
  t->nr_right_buttons = Scr.nr_right_buttons;

  for(i=0;i<Scr.nr_left_buttons;i++)
    if(t->left_w[i] == None)
      t->nr_left_buttons--;

  for(i=0;i<Scr.nr_right_buttons;i++)
    if(t->right_w[i] == None)
      t->nr_right_buttons--;

  if(tflags & MWM_BORDER_FLAG)
    t->bw = 0;
  else if(t->boundary_width <= 0)
    {
      t->boundary_width = 0;
      t->bw = 0;
    }
  else
    {
      t->bw = BW;
      t->boundary_width = t->boundary_width - 1;
    }
  if(t->title_height > 0)
    t->title_height += t->bw;
  if(t->boundary_width == 0)
    t->flags &= ~BORDER;
}

/*
** seemed kind of silly to have check_allowed_function and
** check_allowed_function2 partially overlapping in their checks, so I
** combined them here and made them wrapper functions instead.
*/
static int check_if_function_allowed(int function,
                                     FvwmWindow *t,
                                     MenuItem *mi)
{
  if (t) /* should always be ok */
  {
    if (!mi) /* no menu item, must be exec check, so allow overrides */
    {
      if(t->flags & HintOverride)
        return 1;
    }

    switch(function)
    {
      case F_DELETE:
        if (!(t->flags & DoesWmDeleteWindow))
          return 0;
        /* fall through to close clause */
      case F_CLOSE:
        if (!(t->functions & MWM_FUNC_CLOSE))
          return 0;
        break;
      case F_DESTROY: /* shouldn't destroy always be allowed??? */
        if (!(t->functions & MWM_FUNC_CLOSE))
          return 0;
        break;
      case F_RESIZE:
        if (!(t->functions & MWM_FUNC_RESIZE))
          return 0;
        break;
      case F_ICONIFY:
        if ((!(t->flags & ICONIFIED))&&
            (!(t->functions & MWM_FUNC_MINIMIZE)))
          return 0;
        break;
      case F_MAXIMIZE:
        if (!(t->functions & MWM_FUNC_MAXIMIZE))
          return 0;
        break;
      case F_MOVE:
        /* Move is a funny hint. Keeps it out of the menu, but you're
         * still allowed to move. */
        if((!(t->functions & MWM_FUNC_MOVE))&&mi)
          return 0;
        break;
      case F_FUNCTION:
        /* Hard part! What to do now? */
        /* Hate to do it, but for lack of a better idea,
         * check based on the menu entry name */
        /* Complex functions are a little tricky, ignore them if no menu item*/
        if (mi && mi->item)
        {
          if((!(t->functions & MWM_FUNC_MOVE))&&
             (StrEquals(mi->item,MOVE_STRING)))
            return 0;
          if((!(t->functions & MWM_FUNC_RESIZE))&&
             (StrEquals(mi->item,RESIZE_STRING1)))
            return 0;
          if((!(t->functions & MWM_FUNC_RESIZE))&&
             (StrEquals(mi->item,RESIZE_STRING2)))
            return 0;
          if((!(t->functions & MWM_FUNC_MINIMIZE))&&
             (!(t->flags & ICONIFIED))&&
             (StrEquals(mi->item,MINIMIZE_STRING)))
            return 0;
          if((!(t->functions & MWM_FUNC_MINIMIZE))&&
             (StrEquals(mi->item,MINIMIZE_STRING2)))
            return 0;
          if((!(t->functions & MWM_FUNC_MAXIMIZE))&&
             (StrEquals(mi->item,MAXIMIZE_STRING)))
            return 0;
          if((!(t->functions & MWM_FUNC_CLOSE))&&
             (StrEquals(mi->item,CLOSE_STRING1)))
            return 0;
          if((!(t->functions & MWM_FUNC_CLOSE))&&
             (StrEquals(mi->item,CLOSE_STRING2)))
            return 0;
          if((!(t->functions & MWM_FUNC_CLOSE))&&
             (StrEquals(mi->item,CLOSE_STRING3)))
            return 0;
          if((!(t->functions & MWM_FUNC_CLOSE))&&
             (StrEquals(mi->item,CLOSE_STRING4)))
            return 0;
        }
        break;
      default:
        break;
    } /* end of switch */
  } /* end of if */

  /* if we fell through, just return a 1 */
  return 1;
}

/****************************************************************************
 *
 * Checks the function described in menuItem mi, and sees if it
 * is an allowed function for window Tmp_Win,
 * according to the motif way of life.
 *
 * This routine is used to determine whether or not to grey out menu items.
 *
 ****************************************************************************/
int check_allowed_function(MenuItem *mi)
{
  return check_if_function_allowed(mi->func_type,Tmp_win,mi);
}

/****************************************************************************
 *
 * Checks the function "function", and sees if it
 * is an allowed function for window t,  according to the motif way of life.
 * This routine is used to decide if we should refuse to perform a function.
 *
 ****************************************************************************/
int check_allowed_function2(int function, FvwmWindow *t)
{
  return check_if_function_allowed(function,t,NULL);
}



