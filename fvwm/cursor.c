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

#include "fvwmlib.h"
#include "screen.h"
#include "borders.h"

#include "cursor.h"
#include <X11/cursorfont.h>

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
  XC_left_side             /* CRS_LEFT_EDGE */
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

void CursorStyle(F_CMD_ARGS)
{
  char *cname=NULL, *newcursor=NULL;
  char *errpos = NULL, *path = NULL;
  char *fore = NULL, *back = NULL;
  XColor colors[2];
  int index,nc,i;
  FvwmWindow *fw;

  action = GetNextToken(action,&cname);
  action = GetNextToken(action,&newcursor);

  if (!cname || !newcursor)
  {
    fvwm_msg(ERR,"CursorStyle","Bad cursor style");
    if (cname)
      free(cname);
    if (newcursor)
      free(newcursor);
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
  else
  {
    fvwm_msg(ERR,"CursorStyle","Unknown cursor name %s",cname);
    free(cname);
    free(newcursor);
    return;
  }
  free(cname);

  nc = strtol (newcursor, &errpos, 10);
  if (errpos && *errpos == '\0')
    {
      /* newcursor was a number */
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
      /* newcursor was not a number */
#ifdef XPM
      XpmAttributes xpm_attributes;
      Pixmap source, mask;

      path = findImageFile (newcursor, NULL, R_OK);
      if (!path)
	{
	  fvwm_msg (ERR, "CursorStyle", "Cursor xpm not found %s", newcursor);
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

      colors[0].pixel = BlackPixel(dpy, Scr.screen);
      colors[1].pixel = WhitePixel(dpy, Scr.screen);
      XQueryColors (dpy, PictureCMap, colors, 2);

      if (Scr.FvwmCursors[index])
	XFreeCursor (dpy, Scr.FvwmCursors[index]);
      Scr.FvwmCursors[index] =
	XCreatePixmapCursor (dpy, source, mask,
			     &(colors[0]), &(colors[1]),
			     xpm_attributes.x_hotspot,
			     xpm_attributes.y_hotspot);

      free (newcursor);
      free (path);
#else /* ! XPM */
      fvwm_msg (ERR, "CursorStyle", "Bad cursor number %s", newcursor);
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
      XQueryColors (dpy, PictureCMap, colors, 2);
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
}
