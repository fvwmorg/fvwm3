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

#include "config.h"

#include <stdio.h>
#include "fvwm.h"
#include "cursor.h"
#include "functions.h"
#include "libs/fvwmlib.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "cursor.h"
#include "menus.h"

static Cursor cursors[CRS_MAX];
static const unsigned int default_cursors[CRS_MAX] =
{
  None,
  XC_top_left_corner,      /* CRS_POSITION */
  XC_top_left_arrow,       /* CRS_TITLE */
  XC_top_left_arrow,       /* CRS_DEFAULT */
  XC_hand2,                /* CRS_SYS */
  XC_fleur,                /* CRS_MOVE */
  XC_sizing,               /* CRS_RESIZE */
  XC_watch,                /* CRS_WAIT */
  XC_top_left_arrow,       /* CRS_MENU */
  XC_crosshair,            /* CRS_SELECT */
  XC_pirate,               /* CRS_DESTROY */
  XC_top_side,             /* CRS_TOP */
  XC_right_side,           /* CRS_RIGHT */
  XC_bottom_side,          /* CRS_BOTTOM */
  XC_left_side,            /* CRS_LEFT */
  XC_top_left_corner,      /* CRS_TOP_LEFT */
  XC_top_right_corner,     /* CRS_TOP_RIGHT */
  XC_bottom_left_corner,   /* CRS_BOTTOM_LEFT */
  XC_bottom_right_corner,  /* CRS_BOTTOM_RIGHT */
  XC_top_side,             /* CRS_TOP_EDGE */
  XC_right_side,           /* CRS_RIGHT_EDGE */
  XC_bottom_side,          /* CRS_BOTTOM_EDGE */
  XC_left_side,            /* CRS_LEFT_EDGE */
  XC_left_ptr,             /* CRS_ROOT */
  XC_plus                  /* CRS_STROKE */
};

/***********************************************************************
 *
 *  Procedure:
 *	CreateCursors - Loads fvwm cursors
 *
 ***********************************************************************/
Cursor *CreateCursors(Display *dpy)
{
  int i;
  /* define cursors */
  cursors[0] = None;
  for (i = 1; i < CRS_MAX; i++)
  {
    cursors[i] = XCreateFontCursor(dpy, default_cursors[i]);
  }
  return cursors;
}


static void SafeDefineCursor(Window w, Cursor cursor)
{
  if (w)
    XDefineCursor(dpy,w,cursor);
}


/***********************************************************************
 *
 *  myCursorNameToIndex: return the number of a X11 cursor from its
 *  name, if not found return -1.
 *  Do not know if all the ifdef are useful, but since we do not use Xmu
 *  for portability ...
 ***********************************************************************/
static int myCursorNameToIndex (char *cursor_name)
{
  static const struct CursorNameIndex {
    const char	*name;
    unsigned int number;
  } cursor_table[] = {
#ifdef XC_arrow
    {"arrow", XC_arrow},
#endif
#ifdef XC_based_arrow_down
    {"based_arrow_down", XC_based_arrow_down},
#endif
#ifdef XC_based_arrow_up
    {"based_arrow_up", XC_based_arrow_up},
#endif
#ifdef XC_boat
    {"boat", XC_boat},
#endif
#ifdef XC_bogosity
    {"bogosity", XC_bogosity},
#endif
    {"bottom_left_corner", XC_bottom_left_corner},
    {"bottom_right_corner", XC_bottom_right_corner},
    {"bottom_side", XC_bottom_side},
#ifdef XC_bottom_tee
    {"bottom_tee", XC_bottom_tee},
#endif
#ifdef XC_box_spiral
    {"box_spiral", XC_box_spiral},
#endif
#ifdef XC_center_ptr
    {"center_ptr", XC_center_ptr},
#endif
#ifdef XC_circle
    {"circle", XC_circle},
#endif
#ifdef XC_clock
    {"clock", XC_clock},
#endif
#ifdef XC_coffee_mug
    {"coffee_mug", XC_coffee_mug},
#endif
#ifdef XC_cross
    {"cross", XC_cross},
#endif
#ifdef XC_cross_reverse
    {"cross_reverse", XC_cross_reverse},
#endif
    {"crosshair", XC_crosshair},
#ifdef XC_diamond_cross
    {"diamond_cross", XC_diamond_cross},
#endif
#ifdef XC_dot
    {"dot", XC_dot},
#endif
#ifdef XC_dotbox
    {"dotbox", XC_dotbox},
#endif
#ifdef XC_double_arrow
    {"double_arrow", XC_double_arrow},
#endif
#ifdef XC_draft_large
    {"draft_large", XC_draft_large},
#endif
#ifdef XC_draft_small
    {"draft_small", XC_draft_small},
#endif
#ifdef XC_draped_box
    {"draped_box", XC_draped_box},
#endif
#ifdef XC_exchange
    {"exchange", XC_exchange},
#endif
    {"fleur", XC_fleur},
#ifdef XC_gobbler
    {"gobbler",	XC_gobbler},
#endif
#ifdef XC_gumby
    {"gumby", XC_gumby},
#endif
#ifdef XC_hand1
    {"hand1", XC_hand1},
#endif
    {"hand2", XC_hand2},
#ifdef XC_heart
    {"heart", XC_heart},
#endif
#ifdef XC_icon
    {"icon", XC_icon},
#endif
#ifdef XC_iron_cross
    {"iron_cross", XC_iron_cross},
#endif
    {"left_ptr", XC_left_ptr},
    {"left_side", XC_left_side},
#ifdef XC_left_tee
    {"left_tee", XC_left_tee},
#endif
#ifdef XC_leftbutton
    {"leftbutton", XC_leftbutton},
#endif
#ifdef XC_ll_angle
    {"ll_angle", XC_ll_angle},
#endif
#ifdef XC_lr_angle
    {"lr_angle", XC_lr_angle},
#endif
#ifdef XC_man
    {"man", XC_man},
#endif
#ifdef XC_middlebutton
    {"middlebutton", XC_middlebutton},
#endif
#ifdef XC_mouse
    {"mouse", XC_mouse},
#endif
#ifdef XC_pencil
    {"pencil", XC_pencil},
#endif
#ifdef XC_pirate
    {"pirate", XC_pirate},
#endif
#ifdef XC_plus
    {"plus", XC_plus},
#endif
#ifdef XC_question_arrow
    {"question_arrow", XC_question_arrow},
#endif
#ifdef XC_right_ptr
    {"right_ptr", XC_right_ptr},
#endif
    {"right_side", XC_right_side},
#ifdef XC_right_tee
    {"right_tee", XC_right_tee},
#endif
#ifdef XC_rightbutton
    {"rightbutton", XC_rightbutton},
#endif
#ifdef XC_rtl_logo
    {"rtl_logo", XC_rtl_logo},
#endif
#ifdef XC_sailboat
    {"sailboat", XC_sailboat},
#endif
#ifdef XC_sb_down_arrow
    {"sb_down_arrow", XC_sb_down_arrow},
#endif
#ifdef XC_sb_h_double_arrow
    {"sb_h_double_arrow", XC_sb_h_double_arrow},
#endif
#ifdef XC_sb_left_arrow
    {"sb_left_arrow", XC_sb_left_arrow},
#endif
#ifdef XC_sb_right_arrow
    {"sb_right_arrow", XC_sb_right_arrow},
#endif
#ifdef XC_sb_up_arrow
    {"sb_up_arrow", XC_sb_up_arrow},
#endif
#ifdef XC_sb_v_double_arrow
    {"sb_v_double_arrow", XC_sb_v_double_arrow},
#endif
#ifdef XC_shuttle
    {"shuttle",	XC_shuttle},
#endif XC_sizing
    {"sizing", XC_sizing},
#ifdef XC_spider
    {"spider", XC_spider},
#endif
#ifdef XC_spraycan
    {"spraycan", XC_spraycan},
#endif
#ifdef XC_star
    {"star", XC_star},
#endif
#ifdef XC_target
    {"target", XC_target},
#endif
#ifdef XC_tcross
    {"tcross", XC_tcross},
#endif
    {"top_left_arrow", XC_top_left_arrow},
    {"top_left_corner", XC_top_left_corner},
    {"top_right_corner", XC_top_right_corner},
    {"top_side", XC_top_side},
#ifdef XC_top_tee
    {"top_tee",	XC_top_tee},
#endif
#ifdef XC_trek
    {"trek", XC_trek},
#endif
#ifdef XC_ul_angle
    {"ul_angle", XC_ul_angle},
#endif
#ifdef XC_umbrella
    {"umbrella", XC_umbrella},
#endif
#ifdef XC_ur_angle
    {"ur_angle", XC_ur_angle},
#endif
    {"watch", XC_watch},
    {"x_cursor", XC_X_cursor},
#ifdef XC_xterm
    {"xterm", XC_xterm},
#endif
  };

  int cond;
  int down = 0;
  int up = (sizeof cursor_table / sizeof cursor_table[0]) - 1;
  int middle;
  char temp[20];
  char *s;

  if (!cursor_name || cursor_name[0] == 0 ||
      strlen(cursor_name) >= sizeof temp)
    return -1;
  strcpy(temp, cursor_name);

  for (s = temp; *s != 0; s++)
    if (isupper(*s))
      *s = tolower(*s);

  while (down <= up)
  {
    middle= (down + up) / 2;
    if ((cond = strcmp(temp, cursor_table[middle].name)) < 0)
      up = middle - 1;
    else if (cond > 0)
      down =  middle + 1;
    else
      return cursor_table[middle].number;
  }
  return -1;
}


void CursorStyle(F_CMD_ARGS)
{
  char *cname=NULL, *newcursor=NULL;
  char *errpos = NULL, *path = NULL;
  char *fore = NULL, *back = NULL;
  XColor colors[2];
  int index,nc,i,my_nc;
  FvwmWindow *fw;

  action = GetNextToken(action,&cname);
  action = GetNextToken(action,&newcursor);

  if (!cname)
  {
    fvwm_msg(ERR,"CursorStyle","Bad cursor style");
    if (cname)
      free(cname);
    return;
  }
  if (StrEquals("POSITION",cname)) index = CRS_POSITION;
  else if (StrEquals("DEFAULT",cname)) index = CRS_DEFAULT;
  else if (StrEquals("SYS",cname)) index = CRS_SYS;
  else if (StrEquals("TITLE",cname)) index = CRS_TITLE;
  else if (StrEquals("MOVE",cname)) index = CRS_MOVE;
  else if (StrEquals("RESIZE",cname)) index = CRS_RESIZE;
  else if (StrEquals("MENU",cname)) index = CRS_MENU;
  else if (StrEquals("WAIT",cname)) index = CRS_WAIT;
  else if (StrEquals("SELECT",cname)) index = CRS_SELECT;
  else if (StrEquals("DESTROY",cname)) index = CRS_DESTROY;
  else if (StrEquals("LEFT",cname)) index = CRS_LEFT;
  else if (StrEquals("RIGHT",cname)) index = CRS_RIGHT;
  else if (StrEquals("TOP",cname)) index = CRS_TOP;
  else if (StrEquals("BOTTOM",cname)) index = CRS_BOTTOM;
  else if (StrEquals("TOP_LEFT",cname)) index = CRS_TOP_LEFT;
  else if (StrEquals("TOP_RIGHT",cname)) index = CRS_TOP_RIGHT;
  else if (StrEquals("BOTTOM_LEFT",cname)) index = CRS_BOTTOM_LEFT;
  else if (StrEquals("BOTTOM_RIGHT",cname)) index = CRS_BOTTOM_RIGHT;
  else if (StrEquals("LEFT_EDGE",cname)) index = CRS_LEFT_EDGE;
  else if (StrEquals("RIGHT_EDGE",cname)) index = CRS_RIGHT_EDGE;
  else if (StrEquals("TOP_EDGE",cname)) index = CRS_TOP_EDGE;
  else if (StrEquals("BOTTOM_EDGE",cname)) index = CRS_BOTTOM_EDGE;
  else if (StrEquals("ROOT",cname)) index = CRS_ROOT;
  else if (StrEquals("STROKE",cname)) index = CRS_STROKE;
  else
  {
    fvwm_msg(ERR,"CursorStyle","Unknown cursor name %s",cname);
    free(cname);
    free(newcursor);
    return;
  }
  free(cname);

  /* check if the cursor is given by X11 name */
  if (newcursor)
    my_nc = myCursorNameToIndex(newcursor);
  else
    my_nc = default_cursors[index];

  if (my_nc == -1)
  {
    nc = strtol (newcursor, &errpos, 10);
    if (errpos && *errpos == '\0')
      my_nc = 0;
  }
  else
    nc = my_nc;

  if (my_nc > -1)
    {
      /* newcursor was a number or the name of a X11 cursor */
      if ((nc < 0) || (nc >= XC_num_glyphs) || ((nc % 2) != 0))
	{
	  fvwm_msg(ERR, "CursorStyle", "Bad cursor number %s", newcursor);
	  free(newcursor);
	  return;
	}
      free(newcursor);

      /* replace the cursor defn */
      if (Scr.FvwmCursors[index])
	XFreeCursor(dpy,Scr.FvwmCursors[index]);
      Scr.FvwmCursors[index] = XCreateFontCursor(dpy,nc);
    }
  else
    {
      /* newcursor was not a number neither a X11 cursor name */
#ifdef XPM
      XpmAttributes xpm_attributes;
      Pixmap source, mask;
      unsigned int x;
      unsigned int y;

      path = findImageFile (newcursor, NULL, R_OK);
      if (!path)
	{
	  fvwm_msg (ERR, "CursorStyle", "Cursor %s not found", newcursor);
	  free (newcursor);
	  return;
	}

      xpm_attributes.depth = 1; /* we need source to be a bitmap */
      xpm_attributes.valuemask = XpmSize | XpmDepth | XpmHotspot;
      if (XpmReadFileToPixmap (dpy, Scr.Root, path, &source, &mask,
			       &xpm_attributes) != XpmSuccess)
	{
	  fvwm_msg (ERR, "CursorStyle", "Error reading cursor xpm %s",
		    newcursor);
	  free (newcursor);
	  return;
	}

      colors[0].pixel = GetColor("Black");
      colors[1].pixel = GetColor("White");
      XQueryColors (dpy, Pcmap, colors, 2);

      if (Scr.FvwmCursors[index])
	XFreeCursor (dpy, Scr.FvwmCursors[index]);
      x = xpm_attributes.x_hotspot;
      if (x >= xpm_attributes.width)
	x = xpm_attributes.width / 2;
      y = xpm_attributes.y_hotspot;
      if (y >= xpm_attributes.height)
	y = xpm_attributes.height / 2;
      Scr.FvwmCursors[index] =
	XCreatePixmapCursor(
	  dpy, source, mask, &(colors[0]), &(colors[1]), x, y);

      free (newcursor);
      free (path);
#else /* ! XPM */
      fvwm_msg (ERR, "CursorStyle", "Bad cursor name or number %s", newcursor);
      free (newcursor);
      return;
#endif
    }

  /* look for optional color arguments */
  action = GetNextToken(action, &fore);
  action = GetNextToken(action, &back);
  if (fore && back)
    {
      colors[0].pixel = GetColor (fore);
      colors[1].pixel = GetColor (back);
      XQueryColors (dpy, Pcmap, colors, 2);
      XRecolorCursor (dpy, Scr.FvwmCursors[index], &(colors[0]), &(colors[1]));
    }

  /* redefine all the windows using cursors */
  fw = Scr.FvwmRoot.next;
  while(fw)
  {
    for (i=0;i<4;i++)
    {
      SafeDefineCursor(fw->corners[i],Scr.FvwmCursors[CRS_TOP_LEFT+i]);
      SafeDefineCursor(fw->sides[i],Scr.FvwmCursors[CRS_TOP+i]);
    }
    for (i=0;i<Scr.nr_left_buttons;i++)
    {
      SafeDefineCursor(fw->left_w[i],Scr.FvwmCursors[CRS_SYS]);
    }
    for (i=0;i<Scr.nr_right_buttons;i++)
    {
      SafeDefineCursor(fw->right_w[i],Scr.FvwmCursors[CRS_SYS]);
    }
    SafeDefineCursor(fw->title_w, Scr.FvwmCursors[CRS_TITLE]);
    fw = fw->next;
  }

  /* Do the menus for good measure */
  SetMenuCursor(Scr.FvwmCursors[CRS_MENU]);

  SafeDefineCursor(Scr.PanFrameTop.win, Scr.FvwmCursors[CRS_TOP_EDGE]);
  SafeDefineCursor(Scr.PanFrameBottom.win, Scr.FvwmCursors[CRS_BOTTOM_EDGE]);
  SafeDefineCursor(Scr.PanFrameLeft.win, Scr.FvwmCursors[CRS_LEFT_EDGE]);
  SafeDefineCursor(Scr.PanFrameRight.win, Scr.FvwmCursors[CRS_RIGHT_EDGE]);
  /* migo (04/Nov/1999): don't annoy users which use xsetroot */
  if (index == CRS_ROOT)
    SafeDefineCursor(Scr.Root, Scr.FvwmCursors[CRS_ROOT]);
}

/***********************************************************************
 *
 *  builtin function: (set)BusyCursor
 *  Defines in which cases fvwm "grab" the cursor during execution of
 *  certain functions.
 *
 ***********************************************************************/
void setBusyCursor(F_CMD_ARGS)
{
  char *option = NULL;
  char *optstring = NULL;
  char *args = NULL;
  int flag = -1;
  char *optlist[] = {
    "read", "recapture", "wait", "modulesynchronous",
    "dynamicmenu", "*", NULL
  };

  while (action)
  {
    action = GetQuotedString(action, &optstring, ",", NULL, NULL, NULL);
    if (!optstring)
      break;

    args = GetNextToken(optstring, &option);
    if (!option)
    {
      free(optstring);
      break;
    }

    flag = ParseToggleArgument(args, &args, -1, True);
    if (flag == -1)
    {
      fvwm_msg(ERR, "BusyCursor", "error in boolean specification");
      break;
    }

    switch(GetTokenIndex(option, optlist, 0, NULL))
    {
    case 0: /* read */
      if (flag) Scr.BusyCursor |= BUSY_READ;
      else Scr.BusyCursor &= ~BUSY_READ;
      break;

    case 1: /* recapture */
      if (flag) Scr.BusyCursor |= BUSY_RECAPTURE;
      else Scr.BusyCursor &= ~BUSY_RECAPTURE;
      break;

    case 2: /* wait */
      if (flag) Scr.BusyCursor |= BUSY_WAIT;
      else Scr.BusyCursor &= ~BUSY_WAIT;
      break;

    case 3: /* modulesynchronous */
      if (flag) Scr.BusyCursor |= BUSY_MODULESYNCHRONOUS;
      else Scr.BusyCursor &= ~BUSY_MODULESYNCHRONOUS;
      break;

    case 4: /* dynamicmenu */
      if (flag) Scr.BusyCursor |= BUSY_DYNAMICMENU;
      else Scr.BusyCursor &= ~BUSY_DYNAMICMENU;
      break;

    case 5: /* "*" */
      if (flag) Scr.BusyCursor |= (BUSY_READ | BUSY_RECAPTURE | BUSY_WAIT | BUSY_MODULESYNCHRONOUS | BUSY_DYNAMICMENU);
      else Scr.BusyCursor &= ~(BUSY_READ | BUSY_RECAPTURE | BUSY_WAIT | BUSY_MODULESYNCHRONOUS | BUSY_DYNAMICMENU);
      break;

    default:
      fvwm_msg(ERR, "BusyCursor", "unknown context '%s'", option);
      break;
    }
  }
}

