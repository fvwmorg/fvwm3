/****************************************************************************
 * This module is mostly all new
 * by Rob Nation
 * A little of it is borrowed from ctwm.
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/
/***********************************************************************
 *
 * fvwm icon code
 *
 ***********************************************************************/

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#ifdef NeXT
#include <fcntl.h>
#endif

#include <X11/Intrinsic.h>
#ifdef XPM
#include <X11/xpm.h>
#endif /* XPM */
#include "fvwm.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */


void GrabIconButtons(FvwmWindow *, Window);
void GrabIconKeys(FvwmWindow *, Window);

/****************************************************************************
 *
 * Creates an icon window as needed
 *
 ****************************************************************************/
void CreateIconWindow(FvwmWindow *tmp_win, int def_x, int def_y)
{
  int final_x, final_y;
  unsigned long valuemask;		/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */

  tmp_win->flags |= ICON_OURS;
  tmp_win->flags &= ~PIXMAP_OURS;
  tmp_win->flags &= ~SHAPED_ICON;
  tmp_win->icon_pixmap_w = None;
  tmp_win->iconPixmap = None;
  tmp_win->iconDepth = 0;

  if(tmp_win->flags & SUPPRESSICON)
    return;

  /* First, see if it was specified in the .fvwmrc */
  tmp_win->icon_p_height = 0;
  tmp_win->icon_p_width = 0;

  /* First, check for a monochrome bitmap */
  if(tmp_win->icon_bitmap_file != NULL)
    GetBitmapFile(tmp_win);

#ifdef XPM
  /* Next, check for a color pixmap */
  if((tmp_win->icon_bitmap_file != NULL)&&
     (tmp_win->icon_p_height == 0)&&(tmp_win->icon_p_width == 0))
    GetXPMFile(tmp_win);
#endif /* XPM */

  /* Next, See if the app supplies its own icon window */
  if((tmp_win->icon_p_height == 0)&&(tmp_win->icon_p_width == 0)&&
     (tmp_win->wmhints) && (tmp_win->wmhints->flags & IconWindowHint))
    GetIconWindow(tmp_win);

  /* Finally, try to get icon bitmap from the application */
  if((tmp_win->icon_p_height == 0)&&(tmp_win->icon_p_width == 0)&&
	  (tmp_win->wmhints)&&(tmp_win->wmhints->flags & IconPixmapHint))
    GetIconBitmap(tmp_win);

  /* figure out the icon window size */
  if (!(tmp_win->flags & NOICON_TITLE)||(tmp_win->icon_p_height == 0))
    {
      tmp_win->icon_t_width = XTextWidth(Scr.IconFont.font,
					 tmp_win->icon_name,
                                         strlen(tmp_win->icon_name));
      tmp_win->icon_w_height = ICON_HEIGHT;
    }
  else
    {
      tmp_win->icon_t_width = 0;
      tmp_win->icon_w_height = 0;
    }
  if((tmp_win->flags & ICON_OURS)&&(tmp_win->icon_p_height >0))
    {
      tmp_win->icon_p_width += 4;
      tmp_win->icon_p_height +=4;
    }

  if(tmp_win->icon_p_width == 0)
    tmp_win->icon_p_width = tmp_win->icon_t_width+6;
  tmp_win->icon_w_width = tmp_win->icon_p_width;

  final_x = def_x;
  final_y = def_y;
  if(final_x <0)
    final_x = 0;
  if(final_y <0)
    final_y = 0;

  if(final_x + tmp_win->icon_w_width >=Scr.MyDisplayWidth)
    final_x = Scr.MyDisplayWidth - tmp_win->icon_w_width-1;
  if(final_y + tmp_win->icon_w_height >=Scr.MyDisplayHeight)
    final_y = Scr.MyDisplayHeight - tmp_win->icon_w_height-1;

  tmp_win->icon_x_loc = final_x;
  tmp_win->icon_xl_loc = final_x;
  tmp_win->icon_y_loc = final_y;

  /* clip to fit on screen */
  attributes.background_pixel = Scr.StdColors.back;
  valuemask =  CWBorderPixel | CWCursor | CWEventMask | CWBackPixel;
  attributes.border_pixel = Scr.StdColors.fore;
  attributes.cursor = Scr.FvwmCursors[DEFAULT];
  attributes.event_mask = (ButtonPressMask | ButtonReleaseMask |
			   VisibilityChangeMask |
			   ExposureMask | KeyPressMask|EnterWindowMask |
			   FocusChangeMask );
  if (!(tmp_win->flags & NOICON_TITLE)||(tmp_win->icon_p_height == 0))
    tmp_win->icon_w =
      XCreateWindow(dpy, Scr.Root, final_x, final_y+tmp_win->icon_p_height,
                    tmp_win->icon_w_width, tmp_win->icon_w_height,0,
                    CopyFromParent,
                    CopyFromParent,CopyFromParent,valuemask,&attributes);

  if((tmp_win->flags & ICON_OURS)&&(tmp_win->icon_p_width>0)&&
     (tmp_win->icon_p_height>0))
    {
      tmp_win->icon_pixmap_w =
	XCreateWindow(dpy, Scr.Root, final_x, final_y, tmp_win->icon_p_width,
		      tmp_win->icon_p_height, 0, CopyFromParent,
		      CopyFromParent,CopyFromParent,valuemask,&attributes);
    }
  else
    {
      attributes.event_mask = (ButtonPressMask | ButtonReleaseMask |
			       VisibilityChangeMask |
			       KeyPressMask|EnterWindowMask |
			       FocusChangeMask | LeaveWindowMask );

      valuemask = CWEventMask;
      XChangeWindowAttributes(dpy,tmp_win->icon_pixmap_w,
			      valuemask,&attributes);
    }


#ifdef XPM
#ifdef SHAPE
  if (ShapesSupported && (tmp_win->flags & SHAPED_ICON))
    {
      XShapeCombineMask(dpy, tmp_win->icon_pixmap_w, ShapeBounding,2, 2,
			tmp_win->icon_maskPixmap, ShapeSet);
    }
#endif
#endif

  if(tmp_win->icon_w != None)
    {
      XSaveContext(dpy, tmp_win->icon_w, FvwmContext, (caddr_t)tmp_win);
      XDefineCursor(dpy, tmp_win->icon_w, Scr.FvwmCursors[DEFAULT]);
      GrabIconButtons(tmp_win,tmp_win->icon_w);
      GrabIconKeys(tmp_win,tmp_win->icon_w);
    }
  if(tmp_win->icon_pixmap_w != None)
    {
      XSaveContext(dpy, tmp_win->icon_pixmap_w, FvwmContext, (caddr_t)tmp_win);
      XDefineCursor(dpy, tmp_win->icon_pixmap_w, Scr.FvwmCursors[DEFAULT]);
      GrabIconButtons(tmp_win,tmp_win->icon_pixmap_w);
      GrabIconKeys(tmp_win,tmp_win->icon_pixmap_w);
    }
  return;
}

/****************************************************************************
 *
 * Draws the icon window
 *
 ****************************************************************************/
void DrawIconWindow(FvwmWindow *Tmp_win)
{
  GC Shadow, Relief;
  Pixel TextColor,BackColor;
  int x ;

  if(Tmp_win->flags & SUPPRESSICON)
    return;

  if(Tmp_win->icon_w != None)
    flush_expose (Tmp_win->icon_w);
  if(Tmp_win->icon_pixmap_w != None)
    flush_expose (Tmp_win->icon_pixmap_w);

  if(Scr.Hilite == Tmp_win)
    {
      if(Scr.d_depth < 2) {
	  Relief =
	      Shadow = Scr.DefaultDecor.HiShadowGC;
	  TextColor = Scr.DefaultDecor.HiColors.fore;
	  BackColor = Scr.DefaultDecor.HiColors.back;
      } else {
	Relief = GetDecor(Tmp_win,HiReliefGC);
	Shadow = GetDecor(Tmp_win,HiShadowGC);
	TextColor = GetDecor(Tmp_win,HiColors.fore);
	BackColor = GetDecor(Tmp_win,HiColors.back);
      }
      /* resize the icon name window */
      if(Tmp_win->icon_w != None)
        {
          Tmp_win->icon_w_width = Tmp_win->icon_t_width+6;
          if(Tmp_win->icon_w_width < Tmp_win->icon_p_width)
            Tmp_win->icon_w_width = Tmp_win->icon_p_width;
          Tmp_win->icon_xl_loc = Tmp_win->icon_x_loc -
            (Tmp_win->icon_w_width - Tmp_win->icon_p_width)/2;
          /* start keep label on screen. dje 8/7/97 */
          if (Tmp_win->icon_xl_loc < 0) {  /* if new loc neg (off left edge) */
            Tmp_win->icon_xl_loc = 0;      /* move to edge */
          } else {                         /* if not on left edge */
            /* if (new loc + width) > screen width (off edge on right) */
            if ((Tmp_win->icon_xl_loc + Tmp_win->icon_w_width) >
                Scr.MyDisplayWidth) {      /* off right */
              /* position up against right edge */
              Tmp_win->icon_xl_loc = Scr.MyDisplayWidth - Tmp_win->icon_w_width;
            }
            /* end keep label on screen. dje 8/7/97 */
          }
        }
    }
  else
    {
      if(Scr.d_depth < 2)
	{
	  Relief = Scr.StdReliefGC;
	  Shadow = Scr.StdShadowGC;
	}
      else
	{
	  Globalgcv.foreground = Tmp_win->ReliefPixel;
	  Globalgcm = GCForeground;
	  XChangeGC(dpy,Scr.ScratchGC1,Globalgcm,&Globalgcv);
	  Relief = Scr.ScratchGC1;

	  Globalgcv.foreground = Tmp_win->ShadowPixel;
	  XChangeGC(dpy,Scr.ScratchGC2,Globalgcm,&Globalgcv);
	  Shadow = Scr.ScratchGC2;
	}
      /* resize the icon name window */
      if(Tmp_win->icon_w != None)
        {
          Tmp_win->icon_w_width = Tmp_win->icon_p_width;
          Tmp_win->icon_xl_loc = Tmp_win->icon_x_loc;
        }
      TextColor = Tmp_win->TextPixel;
      BackColor = Tmp_win->BackPixel;

    }
  if((Tmp_win->flags & ICON_OURS)&&(Tmp_win->icon_pixmap_w != None))
    XSetWindowBackground(dpy,Tmp_win->icon_pixmap_w,
			 BackColor);
  if(Tmp_win->icon_w != None)
    XSetWindowBackground(dpy,Tmp_win->icon_w,BackColor);

  /* write the icon label */
  NewFontAndColor(Scr.IconFont.font->fid,TextColor,BackColor);

  if(Tmp_win->icon_pixmap_w != None)
    XMoveWindow(dpy,Tmp_win->icon_pixmap_w,Tmp_win->icon_x_loc,
                Tmp_win->icon_y_loc);
  if(Tmp_win->icon_w != None)
    {
      Tmp_win->icon_w_height = ICON_HEIGHT;
      XMoveResizeWindow(dpy, Tmp_win->icon_w, Tmp_win->icon_xl_loc,
                        Tmp_win->icon_y_loc+Tmp_win->icon_p_height,
                        Tmp_win->icon_w_width,ICON_HEIGHT);

      XClearWindow(dpy,Tmp_win->icon_w);
    }

  if((Tmp_win->iconPixmap != None)&&(!(Tmp_win->flags & SHAPED_ICON)))
    RelieveWindow(Tmp_win,Tmp_win->icon_pixmap_w,0,0,
		  Tmp_win->icon_p_width, Tmp_win->icon_p_height,
		  Relief,Shadow, FULL_HILITE);

  /* need to locate the icon pixmap */
  if(Tmp_win->iconPixmap != None)
    {
      if(Tmp_win->iconDepth == Scr.d_depth)
	{
	  XCopyArea(dpy,Tmp_win->iconPixmap,Tmp_win->icon_pixmap_w,Scr.ScratchGC3,
		    0,0,Tmp_win->icon_p_width-4, Tmp_win->icon_p_height-4,2,2);
	}
      else
	XCopyPlane(dpy,Tmp_win->iconPixmap,Tmp_win->icon_pixmap_w,Scr.ScratchGC3,0,
		   0,Tmp_win->icon_p_width-4, Tmp_win->icon_p_height-4,2,2,1);
    }

  if(Tmp_win->icon_w != None)
    {
      /* text position */
      x = (Tmp_win->icon_w_width - Tmp_win->icon_t_width)/2;
      if(x<3)x=3;

      XDrawString (dpy, Tmp_win->icon_w, Scr.ScratchGC3, x,
                   Tmp_win->icon_w_height-Scr.IconFont.height+
		   Scr.IconFont.y-3,
                   Tmp_win->icon_name, strlen(Tmp_win->icon_name));
      RelieveWindow(Tmp_win,Tmp_win->icon_w,0,0,Tmp_win->icon_w_width,
                    ICON_HEIGHT,Relief,Shadow, FULL_HILITE);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	RedoIconName - procedure to re-position the icon window and name
 *
 ************************************************************************/
void RedoIconName(FvwmWindow *Tmp_win)
{

  if(Tmp_win->flags & SUPPRESSICON)
    return;

  if (Tmp_win->icon_w == (int)NULL)
    return;

  Tmp_win->icon_t_width = XTextWidth(Scr.IconFont.font,Tmp_win->icon_name,
				     strlen(Tmp_win->icon_name));
  /* clear the icon window, and trigger a re-draw via an expose event */
  if (Tmp_win->flags & ICONIFIED)
    XClearArea(dpy, Tmp_win->icon_w, 0, 0, 0, 0, True);
  return;
}




 /***********************************************************************
 *
 *  Procedure:
 *	AutoPlace - Find a home for an icon
 *
 ************************************************************************/
void AutoPlace(FvwmWindow *t)
{
  int tw,th,tx,ty;
  int base_x, base_y;
  int width,height;
  FvwmWindow *test_window;
  Bool loc_ok;
  int real_x=10, real_y=10;
  int new_x, new_y;

  /* New! Put icon in same page as the center of the window */
  /* Not a good idea for StickyIcons */
  if((t->flags & StickyIcon)||(t->flags & STICKY))
    {
      base_x = 0;
      base_y = 0;
      /*Also, if its a stickyWindow, put it on the current page! */
      new_x = t->frame_x % Scr.MyDisplayWidth;
      new_y = t->frame_y % Scr.MyDisplayHeight;
      if(new_x < 0)new_x += Scr.MyDisplayWidth;
      if(new_y < 0)new_y += Scr.MyDisplayHeight;
      SetupFrame(t,new_x,new_y,
		 t->frame_width,t->frame_height,False);
      t->Desk = Scr.CurrentDesk;
    }
  else
    {
      base_x=((t->frame_x+Scr.Vx+(t->frame_width>>1))/Scr.MyDisplayWidth)*
	Scr.MyDisplayWidth - Scr.Vx;
      base_y=((t->frame_y+Scr.Vy+(t->frame_height>>1))/Scr.MyDisplayHeight)*
	Scr.MyDisplayHeight - Scr.Vy;
    }
  if(t->flags & ICON_MOVED)
    {
      /* just make sure the icon is on this screen */
      t->icon_x_loc = t->icon_x_loc % Scr.MyDisplayWidth + base_x;
      t->icon_y_loc = t->icon_y_loc % Scr.MyDisplayHeight + base_y;
      if(t->icon_x_loc < 0)
	t->icon_x_loc += Scr.MyDisplayWidth;
      if(t->icon_y_loc < 0)
	t->icon_y_loc += Scr.MyDisplayHeight;
    }
  else if (t->wmhints && t->wmhints->flags & IconPositionHint)
    {
      t->icon_x_loc = t->wmhints->icon_x;
      t->icon_y_loc = t->wmhints->icon_y;
    }
  /* dje 10/12/97:
     Look thru chain of icon boxes assigned to window.
     Add logic for grids and fill direction.
  */
  else {
    /* A place to hold inner and outer loop variables. */
    typedef struct dimension_struct {
      int step;                         /* grid size (may be negative) */
      int start_at;                     /* starting edge */
      int real_start;                   /* on screen starting edge */
      int end_at;                       /* ending edge */
      int base;                         /* base for screen */
      int icon_dimension;               /* height or width */
      int nom_dimension;                /* nonminal height or width */
      int screen_dimension;             /* screen height or width */
    } dimension;
    dimension dim[3];                   /* space for work, 1st, 2nd dimen */
    icon_boxes *icon_boxes_ptr;         /* current icon box */
    int i;                              /* index for inner/outer loop data */

    /* Hopefully this makes the following more readable. */
#define ICONBOX_LFT icon_boxes_ptr->IconBox[0]
#define ICONBOX_TOP icon_boxes_ptr->IconBox[1]
#define ICONBOX_RGT icon_boxes_ptr->IconBox[2]
#define ICONBOX_BOT icon_boxes_ptr->IconBox[3]
#define BOT_FILL icon_boxes_ptr->IconFlags & ICONFILLBOT
#define RGT_FILL icon_boxes_ptr->IconFlags & ICONFILLRGT
#define HRZ_FILL icon_boxes_ptr->IconFlags & ICONFILLHRZ

    width = t->icon_p_width;          /* unnecessary copy of width */
    height = t->icon_w_height + t->icon_p_height; /* total height */
    loc_ok = False;                   /* no slot found yet */

    /* check all boxes in order */
    for(icon_boxes_ptr= t->IconBoxes;   /* init */
        icon_boxes_ptr != NULL; /* until no more boxes */
        icon_boxes_ptr = icon_boxes_ptr->next) { /* all boxes */
      if (loc_ok == True) {
        break;                          /* leave for loop */
      }
      dim[1].step = icon_boxes_ptr->IconGrid[1]; /* y amount */
      dim[1].start_at = ICONBOX_TOP;    /* init start from */
      dim[1].end_at = ICONBOX_BOT;      /* init end at */
      dim[1].base = base_y;             /* save base */
      dim[1].icon_dimension = height;   /* save dimension */
      dim[1].screen_dimension = Scr.MyDisplayHeight;
      if (BOT_FILL) {                   /* fill from bottom */
        dim[1].step = 0 - dim[1].step;  /* reverse step */
      } /* end fill from bottom */

      dim[2].step = icon_boxes_ptr->IconGrid[0]; /* x amount */
      dim[2].start_at = ICONBOX_LFT;    /* init start from */
      dim[2].end_at = ICONBOX_RGT;      /* init end at */
      dim[2].base   = base_x;           /* save base */
      dim[2].icon_dimension = width;    /* save dimension */
      dim[2].screen_dimension = Scr.MyDisplayWidth;
      if (RGT_FILL) {                   /* fill from right */
        dim[2].step = 0 - dim[2].step;  /* reverse step */
      } /* end fill from right */
      for (i=1;i<=2;i++) {              /* for dimensions 1 and 2 */
        /* If the window is taller than the icon box, ignore the icon height
         * when figuring where to put it. Same goes for the width
         * This should permit reasonably graceful handling of big icons. */
        dim[i].nom_dimension = dim[i].icon_dimension;
        if (dim[i].icon_dimension >= dim[i].end_at - dim[i].start_at) {
          dim[i].nom_dimension = dim[i].end_at - dim[i].start_at - 1;
        }
        if (dim[i].step < 0) {          /* if moving backwards */
          dim[0].start_at = dim[i].start_at; /* save */
          dim[i].start_at = dim[i].end_at; /* swap one */
          dim[i].end_at = dim[0].start_at; /* swap the other */
          dim[i].start_at -= dim[i].icon_dimension;
        } /* end moving backwards */
        dim[i].start_at += dim[i].base; /* adjust both to base */
        dim[i].end_at += dim[i].base;
      } /* end 2 dimensions */
      if (HRZ_FILL) {                   /* if hrz first */
        memcpy(&dim[0],&dim[1],sizeof(dimension)); /* save */
        memcpy(&dim[1],&dim[2],sizeof(dimension)); /* switch one */
        memcpy(&dim[2],&dim[0],sizeof(dimension)); /* switch the other */
      } /* end horizontal dimension first */
      dim[0].start_at = dim[2].start_at; /* save for reseting inner loop */
      while((dim[1].step < 0            /* filling reversed */
             ? (dim[1].start_at + dim[1].icon_dimension - dim[1].nom_dimension
                > dim[1].end_at)        /* check back edge */
             : (dim[1].start_at + dim[1].nom_dimension
                < dim[1].end_at))       /* check front edge */
            && (!loc_ok)) {             /* nothing found yet */
        dim[1].real_start = dim[1].start_at; /* init */
        if (dim[1].start_at + dim[1].icon_dimension >
            dim[1].screen_dimension - 2 + dim[1].base) { /* if off screen */
          dim[1].real_start = dim[1].screen_dimension
            - dim[1].icon_dimension + dim[1].base; /* move on screen */
        } /* end off screen */
        if (dim[1].start_at < dim[1].base) { /* if off other edge */
          dim[1].real_start = dim[1].base; /* move on screen */
        } /* end off other edge */
        dim[2].start_at = dim[0].start_at; /* reset inner loop */
        while((dim[2].step < 0            /* filling reversed */
               ? (dim[2].start_at + dim[2].icon_dimension - dim[2].nom_dimension
                  > dim[2].end_at)        /* check back edge */
               : (dim[2].start_at + dim[2].nom_dimension
                  < dim[2].end_at))       /* check front edge */
              && (!loc_ok)) {             /* nothing found yet */
          dim[2].real_start = dim[2].start_at; /* init */
          if (dim[2].start_at + dim[2].icon_dimension >
              dim[2].screen_dimension - 2 + dim[2].base) { /* if off screen */
            dim[2].real_start = dim[2].screen_dimension
              - dim[2].icon_dimension + dim[2].base; /* move on screen */
          } /* end off screen */
          if (dim[2].start_at < dim[2].base) { /* if off other edge */
            dim[2].real_start = dim[2].base; /* move on screen */
          } /* end off other edge */

          if (HRZ_FILL) {               /* if hrz first */
            real_x = dim[1].real_start; /* unreverse them */
            real_y = dim[2].real_start;
          } else {
            real_x = dim[2].real_start; /* reverse them */
            real_y = dim[1].real_start;
          }

          loc_ok = True;                  /* this may be a good location */
          test_window = Scr.FvwmRoot.next;
          while((test_window != (FvwmWindow *)0)
                &&(loc_ok == True)) { /* test overlap */
            if(test_window->Desk == t->Desk) {
              if((test_window->flags&ICONIFIED) &&
                 (!(test_window->flags&TRANSIENT) ||
		  !test_window->tmpflags.IconifiedByParent) &&
                 (test_window->icon_w||test_window->icon_pixmap_w) &&
                 (test_window != t)) {
                tw=test_window->icon_p_width;
                th=test_window->icon_p_height+
                  test_window->icon_w_height;
                tx = test_window->icon_x_loc;
                ty = test_window->icon_y_loc;

                if((tx<(real_x+width+3))&&((tx+tw+3) > real_x)&&
                   (ty<(real_y+height+3))&&((ty+th + 3)>real_y)) {
                  loc_ok = False;         /* don't accept this location */
                } /* end if icons overlap */
              } /* end if its an icon */
            } /* end if same desk */
            test_window = test_window->next;
          } /* end while icons that may overlap */
          dim[2].start_at += dim[2].step; /* Grid inner value & direction */
        } /* end while room inner dimension */
        dim[1].start_at += dim[1].step; /* Grid outer value & direction */
      } /* end while room outer dimension */
    } /* end for all icon boxes, or found space */
    if(loc_ok == False)               /* If icon never found a home */
      return;                         /* just leave it */
    t->icon_x_loc = real_x;
    t->icon_y_loc = real_y;

    if(t->icon_pixmap_w)
      XMoveWindow(dpy,t->icon_pixmap_w,t->icon_x_loc, t->icon_y_loc);

    t->icon_w_width = t->icon_p_width;
    t->icon_xl_loc = t->icon_x_loc;

    if (t->icon_w != None)
      XMoveResizeWindow(dpy, t->icon_w, t->icon_xl_loc,
                        t->icon_y_loc+t->icon_p_height,
                        t->icon_w_width,ICON_HEIGHT);
    BroadcastPacket(M_ICON_LOCATION, 7,
                    t->w, t->frame,
                    (unsigned long)t,
                    t->icon_x_loc, t->icon_y_loc,
                    t->icon_w_width, t->icon_w_height+t->icon_p_height);
  }

}

/***********************************************************************
 *
 *  Procedure:
 *	GrabIconButtons - grab needed buttons for the icon window
 *
 *  Inputs:
 *	tmp_win - the fvwm window structure to use
 *
 ***********************************************************************/
void GrabIconButtons(FvwmWindow *tmp_win, Window w)
{
  Binding *MouseEntry;

  MouseEntry = Scr.AllBindings;
  while(MouseEntry != (Binding *)0)
    {
      if((MouseEntry->Action != NULL)&&(MouseEntry->Context & C_ICON)&&
	 (MouseEntry->IsMouse == 1))
	{
	  if(MouseEntry->Button_Key >0)
	    XGrabButton(dpy, MouseEntry->Button_Key, MouseEntry->Modifier, w,
			True, ButtonPressMask | ButtonReleaseMask,
			GrabModeAsync, GrabModeAsync, None,
			Scr.FvwmCursors[DEFAULT]);
	  else
	    {
	      XGrabButton(dpy, 1, MouseEntry->Modifier, w,
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None,
			  Scr.FvwmCursors[DEFAULT]);
	      XGrabButton(dpy, 2, MouseEntry->Modifier, w,
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None,
			  Scr.FvwmCursors[DEFAULT]);
	      XGrabButton(dpy, 3, MouseEntry->Modifier, w,
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None,
			  Scr.FvwmCursors[DEFAULT]);
	    }
	}

      MouseEntry = MouseEntry->NextBinding;
    }
  return;
}



/***********************************************************************
 *
 *  Procedure:
 *	GrabIconKeys - grab needed keys for the icon window
 *
 *  Inputs:
 *	tmp_win - the fvwm window structure to use
 *
 ***********************************************************************/
void GrabIconKeys(FvwmWindow *tmp_win,Window w)
{
  Binding *tmp;
  for (tmp = Scr.AllBindings; tmp != NULL; tmp = tmp->NextBinding)
    {
      if ((tmp->Context & C_ICON)&&(tmp->IsMouse == 0))
	XGrabKey(dpy, tmp->Button_Key, tmp->Modifier, w, True,
		 GrabModeAsync, GrabModeAsync);
    }
  return;
}


/****************************************************************************
 *
 * Looks for a monochrome icon bitmap file
 *
 ****************************************************************************/
void GetBitmapFile(FvwmWindow *tmp_win)
{
  char *path = NULL;
  int HotX,HotY;
  extern char *IconPath;

  path = findIconFile(tmp_win->icon_bitmap_file, IconPath,R_OK);

  if(path == NULL)return;
  if(XReadBitmapFile (dpy, Scr.Root,path,
		      (unsigned int *)&tmp_win->icon_p_width,
		      (unsigned int *)&tmp_win->icon_p_height,
		      &tmp_win->iconPixmap,
		      &HotX, &HotY) != BitmapSuccess)
    {
      tmp_win->icon_p_width = 0;
      tmp_win->icon_p_height = 0;
    }

  free(path);
}

/****************************************************************************
 *
 * Looks for a color XPM icon file
 *
 ****************************************************************************/
void GetXPMFile(FvwmWindow *tmp_win)
{
#ifdef XPM
  XWindowAttributes root_attr;
  XpmAttributes xpm_attributes;
  extern char *PixmapPath;
  char *path = NULL;
  XpmImage my_image;
  int rc;

  path = findIconFile(tmp_win->icon_bitmap_file, PixmapPath,R_OK);
  if(path == NULL)return;

  XGetWindowAttributes(dpy,Scr.Root,&root_attr);
  xpm_attributes.colormap = root_attr.colormap;
  xpm_attributes.closeness = 40000; /* Allow for "similar" colors */
  xpm_attributes.valuemask = XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

  rc =XpmReadFileToXpmImage(path, &my_image, NULL);
  if (rc != XpmSuccess) {
    fvwm_msg(ERR,"GetXPMFile","XpmReadFileToXpmImage failed, pixmap %s, rc %d",
           path, rc);
    free(path);
    return;
  }
  free(path);
  color_reduce_pixmap(&my_image,Scr.ColorLimit);
  rc = XpmCreatePixmapFromXpmImage(dpy,Scr.Root, &my_image,
                                   &tmp_win->iconPixmap,
                                   &tmp_win->icon_maskPixmap,
                                   &xpm_attributes);
  if (rc != XpmSuccess) {
    fvwm_msg(ERR,"GetXPMFile",
             "XpmCreatePixmapFromXpmImage failed, rc %d\n", rc);
    XpmFreeXpmImage(&my_image);
    return;
  }
  tmp_win->icon_p_width = my_image.width;
  tmp_win->icon_p_height = my_image.height;
  tmp_win->flags |= PIXMAP_OURS;
  tmp_win->iconDepth = Scr.d_depth;

#ifdef SHAPE
  if (ShapesSupported && tmp_win->icon_maskPixmap)
    tmp_win->flags |= SHAPED_ICON;
#endif

  XpmFreeXpmImage(&my_image);

#endif /* XPM */
}

/****************************************************************************
 *
 * Looks for an application supplied icon window
 *
 ****************************************************************************/
void GetIconWindow(FvwmWindow *tmp_win)
{
  /* We are guaranteed that wmhints is non-null when calling this
   * routine */
  if(XGetGeometry(dpy,   tmp_win->wmhints->icon_window, &JunkRoot,
		  &JunkX, &JunkY,(unsigned int *)&tmp_win->icon_p_width,
		  (unsigned int *)&tmp_win->icon_p_height,
		  &JunkBW, &JunkDepth)==0)
    {
      fvwm_msg(ERR,"GetIconWindow","Help! Bad Icon Window!");
    }
  tmp_win->icon_p_width += JunkBW<<1;
  tmp_win->icon_p_height += JunkBW<<1;
  /*
   * Now make the new window the icon window for this window,
   * and set it up to work as such (select for key presses
   * and button presses/releases, set up the contexts for it,
   * and define the cursor for it).
   */
  tmp_win->icon_pixmap_w = tmp_win->wmhints->icon_window;
#ifdef SHAPE
  if (ShapesSupported)
  {
    if (tmp_win->wmhints->flags & IconMaskHint)
    {
      tmp_win->flags |= SHAPED_ICON;
      tmp_win->icon_maskPixmap = tmp_win->wmhints->icon_mask;
    }
  }
#endif
  /* Make sure that the window is a child of the root window ! */
  /* Olwais screws this up, maybe others do too! */
  XReparentWindow(dpy, tmp_win->icon_pixmap_w, Scr.Root, 0,0);
  tmp_win->flags &= ~ICON_OURS;
}


/****************************************************************************
 *
 * Looks for an application supplied bitmap or pixmap
 *
 ****************************************************************************/
void GetIconBitmap(FvwmWindow *tmp_win)
{
  /* We are guaranteed that wmhints is non-null when calling this
   * routine */
  XGetGeometry(dpy, tmp_win->wmhints->icon_pixmap, &JunkRoot, &JunkX, &JunkY,
	       (unsigned int *)&tmp_win->icon_p_width,
	       (unsigned int *)&tmp_win->icon_p_height, &JunkBW, &JunkDepth);
  tmp_win->iconPixmap = tmp_win->wmhints->icon_pixmap;
  tmp_win->iconDepth = JunkDepth;
#ifdef SHAPE
  if (ShapesSupported)
  {
    if (tmp_win->wmhints->flags & IconMaskHint)
    {
      tmp_win->flags |= SHAPED_ICON;
      tmp_win->icon_maskPixmap = tmp_win->wmhints->icon_mask;
    }
  }
#endif
}



/***********************************************************************
 *
 *  Procedure:
 *	DeIconify a window
 *
 ***********************************************************************/
void DeIconify(FvwmWindow *tmp_win)
{
  FvwmWindow *t,*tmp;

  if(!tmp_win)
    return;

  /* AS dje  RaiseWindow(tmp_win); */
  /* now de-iconify transients */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if ((t == tmp_win)||
	  ((t->flags & TRANSIENT) &&(t->transientfor == tmp_win->w)))
	{
	  t->flags |= MAPPED;
	  t->tmpflags.IconifiedByParent = 0;
	  if(Scr.Hilite == t)
	    SetBorder (t, False,True,True,None);

          /* AS stuff starts here dje */
          if (t->icon_pixmap_w)
            XUnmapWindow(dpy, t->icon_pixmap_w);
	  if (t->icon_w)
	    XUnmapWindow(dpy, t->icon_w);
          XFlush(dpy);
          if (t == tmp_win)
	    BroadcastPacket(M_DEICONIFY, 11,
                            t->w, t->frame,
                            (unsigned long)t,
                            t->icon_x_loc, t->icon_y_loc,
                            t->icon_p_width, t->icon_p_height+t->icon_w_height,
                            t->frame_x, t->frame_y,
                            t->frame_width, t->frame_height);
          else
	    BroadcastPacket(M_DEICONIFY, 7,
                            t->w, t->frame,
                            (unsigned long)t,
                            t->icon_x_loc, t->icon_y_loc,
                            t->icon_p_width, t->icon_p_height+t->icon_w_height);
          /* End AS */
	  XMapWindow(dpy, t->w);
	  if(t->Desk == Scr.CurrentDesk)
	    {
	      XMapWindow(dpy, t->frame);
	      t->flags |= MAP_PENDING;
	    }
	  XMapWindow(dpy, t->Parent);
	  SetMapStateProp(t, NormalState);
	  t->flags &= ~ICONIFIED;
	  t->flags &= ~ICON_UNMAPPED;
	  /* Need to make sure the border is colored correctly,
	   * in case it was stuck or unstuck while iconified. */
	  tmp = Scr.Hilite;
	  Scr.Hilite = t;
	  SetBorder(t,False,True,True,None);
	  Scr.Hilite = tmp;
	  XRaiseWindow(dpy,t->w);
	}
    }

  RaiseWindow(tmp_win); /* moved dje */

  if(tmp_win->flags & ClickToFocus)
    FocusOn(tmp_win,TRUE);

  KeepOnTop();

  return;
}


/****************************************************************************
 *
 * Iconifies the selected window
 *
 ****************************************************************************/
void Iconify(FvwmWindow *tmp_win, int def_x, int def_y)
{
  FvwmWindow *t;
  XWindowAttributes winattrs = {0};
  unsigned long eventMask;

  if(!tmp_win)
    return;
  XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
  eventMask = winattrs.your_event_mask;

  if((tmp_win == Scr.Hilite)&&
     (tmp_win->flags & ClickToFocus)&&(tmp_win->next))
    {
      SetFocus(tmp_win->next->w,tmp_win->next,1);
    }


  /* iconify transients first */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if ((t==tmp_win)||
	  ((t->flags & TRANSIENT) && (t->transientfor == tmp_win->w)))
	{
	  /*
	   * Prevent the receipt of an UnmapNotify, since that would
	   * cause a transition to the Withdrawn state.
	   */
	  t->flags &= ~MAPPED;
	  XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
          XUnmapWindow(dpy, t->w);
          XSelectInput(dpy, t->w, eventMask);
          XUnmapWindow(dpy, t->frame);
	  t->DeIconifyDesk = t->Desk;
	  if (t->icon_w)
	    XUnmapWindow(dpy, t->icon_w);
	  if (t->icon_pixmap_w)
	    XUnmapWindow(dpy, t->icon_pixmap_w);

	  SetMapStateProp(t, IconicState);
	  SetBorder (t, False,False,False,None);
	  if(t != tmp_win)
	    {
	      t->flags |= ICONIFIED|ICON_UNMAPPED;
	      t->tmpflags.IconifiedByParent = 1;

	      BroadcastPacket(M_ICONIFY, 7,
                              t->w, t->frame,
                              (unsigned long)t,
                              -10000, -10000,
                              t->icon_w_width,
                              t->icon_w_height+t->icon_p_height);
	      BroadcastConfig(M_CONFIGURE_WINDOW,t);
	    }
	} /* if */
    } /* for */
  if (tmp_win->icon_w == None) {
    if(tmp_win->flags & ICON_MOVED)
      CreateIconWindow(tmp_win,tmp_win->icon_x_loc,tmp_win->icon_y_loc);
    else
	CreateIconWindow(tmp_win, def_x, def_y);
  }

  /* if no pixmap we want icon width to change to text width every iconify */
  if( (tmp_win->icon_w != None) && (tmp_win->icon_pixmap_w == None) ) {
    tmp_win->icon_t_width =
      XTextWidth(Scr.IconFont.font,tmp_win->icon_name,
                 strlen(tmp_win->icon_name));
    tmp_win->icon_p_width = tmp_win->icon_t_width+6;
    tmp_win->icon_w_width = tmp_win->icon_p_width;
  }

  AutoPlace(tmp_win);
  tmp_win->flags |= ICONIFIED;
  tmp_win->flags &= ~ICON_UNMAPPED;
  BroadcastPacket(M_ICONIFY, 11,
                  tmp_win->w, tmp_win->frame,
                  (unsigned long)tmp_win,
                  tmp_win->icon_x_loc,
                  tmp_win->icon_y_loc,
                  tmp_win->icon_w_width,
                  tmp_win->icon_w_height+tmp_win->icon_p_height,
                  tmp_win->frame_x, /* next 4 added for Animate module */
                  tmp_win->frame_y,
                  tmp_win->frame_width,
                  tmp_win->frame_height);
  BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);

  LowerWindow(tmp_win);
  if(tmp_win->Desk == Scr.CurrentDesk)
    {
      if (tmp_win->icon_w != None)
        XMapWindow(dpy, tmp_win->icon_w);

      if(tmp_win->icon_pixmap_w != None)
	XMapWindow(dpy, tmp_win->icon_pixmap_w);
      KeepOnTop();
    }
  if((tmp_win->flags & ClickToFocus)||(tmp_win->flags & SloppyFocus))
    {
      if (tmp_win == Scr.Focus)
	{
	  if(Scr.PreviousFocus == Scr.Focus)
	    Scr.PreviousFocus = NULL;
	  if((tmp_win->flags & ClickToFocus)&&(tmp_win->next))
	    SetFocus(tmp_win->next->w, tmp_win->next,1);
	  else
	    {
	      SetFocus(Scr.NoFocusWin, NULL,1);
	    }
	}
    }
  return;
}



/****************************************************************************
 *
 * This is used to tell applications which windows on the screen are
 * top level appication windows, and which windows are the icon windows
 * that go with them.
 *
 ****************************************************************************/
void SetMapStateProp(FvwmWindow *tmp_win, int state)
{
  unsigned long data[2];		/* "suggested" by ICCCM version 1 */

  data[0] = (unsigned long) state;
  data[1] = (unsigned long) tmp_win->icon_w;
/*  data[2] = (unsigned long) tmp_win->icon_pixmap_w;*/

  XChangeProperty (dpy, tmp_win->w, _XA_WM_STATE, _XA_WM_STATE, 32,
		   PropModeReplace, (unsigned char *) data, 2);
  return;
}
