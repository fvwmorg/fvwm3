/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified
 * by Rob Nation
 ****************************************************************************/
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


/**********************************************************************
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 **********************************************************************/
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fvwm.h"
#include <X11/Xatom.h>
#include "screen.h"
#include "misc.h"
#ifdef SHAPE
#include <X11/extensions/shape.h>
#include <X11/Xresource.h>
#endif /* SHAPE */
#include "module.h"

/* Used to parse command line of clients for specific desk requests. */
/* Todo: check for multiple desks. */
static XrmDatabase db;
static XrmOptionDescRec table [] = {
  /* Want to accept "-workspace N" or -xrm "fvwm*desk:N" as options
   * to specify the desktop. I have to include dummy options that
   * are meaningless since Xrm seems to allow -w to match -workspace
   * if there would be no ambiguity. */
    {"-workspacf",      "*junk",        XrmoptionSepArg, (caddr_t) NULL},
    {"-workspace",	"*desk",	XrmoptionSepArg, (caddr_t) NULL},
    {"-xrn",		NULL,		XrmoptionResArg, (caddr_t) NULL},
    {"-xrm",		NULL,		XrmoptionResArg, (caddr_t) NULL},
};

extern char *IconPath;
extern char *PixmapPath;

static void merge_styles(name_list *, name_list *); /* prototype */

/***********************************************************************
 *
 *  Procedure:
 *	AddWindow - add a new window to the fvwm list
 *
 *  Returned Value:
 *	(FvwmWindow *) - pointer to the FvwmWindow structure
 *
 *  Inputs:
 *	w	- the window id of the window to add
 *	iconm	- flag to tell if this is an icon manager window
 *
 ***********************************************************************/
FvwmWindow *AddWindow(Window w)
{
  FvwmWindow *tmp_win;		        /* new fvwm window structure */
  unsigned long valuemask;		/* mask for create windows */
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
  Pixmap TexturePixmap = None, TexturePixmapSave = None;
#endif
  unsigned long valuemask_save = 0;
  XSetWindowAttributes attributes;	/* attributes for create windows */
  name_list styles;                     /* area for merged styles */
  int i,width,height;
  int a,b;
/*  RBW - 11/02/1998  */
  int tmpno1 = -1, tmpno2 = -1, tmpno3 = -1, spargs = 0;
/**/
  extern Bool NeedToResizeToo;
  extern FvwmWindow *colormap_win;
  int client_argc;
  char **client_argv = NULL, *str_type;
  Bool status;
  XrmValue rm_value;
  XTextProperty text_prop;
  extern Boolean PPosOverride;

  NeedToResizeToo = False;
  /* allocate space for the fvwm window */
  tmp_win = (FvwmWindow *)calloc(1, sizeof(FvwmWindow));
  if (tmp_win == (FvwmWindow *)0)
    {
      return NULL;
    }
  tmp_win->flags = 0;
  tmp_win->tmpflags.ViewportMoved = 0;
  tmp_win->tmpflags.IconifiedByParent = 0;
  tmp_win->w = w;

  tmp_win->cmap_windows = (Window *)NULL;
#ifdef MINI_ICONS
  tmp_win->mini_pixmap_file = NULL;
  tmp_win->mini_icon = NULL;
#endif

  if(!PPosOverride)
    if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		     &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
      {
	free((char *)tmp_win);
	return(NULL);
      }
  if ( XGetWMName(dpy, tmp_win->w, &text_prop) != 0 )
    tmp_win->name = (char *)text_prop.value;
  else
    tmp_win->name = NoName;

  /* removing NoClass change for now... */
#if 0
  tmp_win->class.res_name = tmp_win->class.res_class = NULL;
#else
  tmp_win->class.res_name = NoResource;
  tmp_win->class.res_class = NoClass;
#endif /* 0 */
  XGetClassHint(dpy, tmp_win->w, &tmp_win->class);
#if 1
  if (tmp_win->class.res_name == NULL)
    tmp_win->class.res_name = NoResource;
  if (tmp_win->class.res_class == NULL)
    tmp_win->class.res_class = NoClass;
#endif /* 1 */

  FetchWmProtocols (tmp_win);
  FetchWmColormapWindows (tmp_win);
  if(!(XGetWindowAttributes(dpy,tmp_win->w,&(tmp_win->attr))))
    tmp_win->attr.colormap = Scr.FvwmRoot.attr.colormap;

  tmp_win->wmhints = XGetWMHints(dpy, tmp_win->w);

  if(XGetTransientForHint(dpy, tmp_win->w,  &tmp_win->transientfor))
    tmp_win->flags |= TRANSIENT;
  else
    tmp_win->flags &= ~TRANSIENT;

  tmp_win->old_bw = tmp_win->attr.border_width;

#ifdef SHAPE
  if (ShapesSupported)
  {
    int xws, yws, xbs, ybs;
    unsigned wws, hws, wbs, hbs;
    int boundingShaped, clipShaped;

    XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);
    XShapeQueryExtents (dpy, tmp_win->w,
			&boundingShaped, &xws, &yws, &wws, &hws,
			&clipShaped, &xbs, &ybs, &wbs, &hbs);
    tmp_win->wShaped = boundingShaped;
  }
#endif /* SHAPE */


  /* if the window is in the NoTitle list, or is a transient,
   *  dont decorate it.
   * If its a transient, and DecorateTransients was specified,
   *  decorate anyway
   */
  /*  Assume that we'll decorate */
  tmp_win->flags |= BORDER;
  tmp_win->flags |= TITLE;

  LookInList(tmp_win, &styles);         /* get merged styles */

  tmp_win->IconBoxes = styles.IconBoxes; /* copy iconboxes ptr (if any) */
  tmp_win->buttons = styles.on_buttons; /* on and off buttons combined. */
#ifdef USEDECOR
  /* search for a UseDecor tag in the Style */
  tmp_win->fl = NULL;
  if (styles.Decor != NULL) {
      FvwmDecor *fl = &Scr.DefaultDecor;
      for (; fl; fl = fl->next)
	  if (strcasecmp(styles.Decor,fl->tag)==0) {
	      tmp_win->fl = fl;
	      break;
	  }
  }
  if (tmp_win->fl == NULL)
      tmp_win->fl = &Scr.DefaultDecor;
#endif

  tmp_win->title_height = GetDecor(tmp_win,TitleHeight) + tmp_win->bw;

  GetMwmHints(tmp_win);
  GetOlHints(tmp_win);

  SelectDecor(tmp_win,styles.on_flags,styles.border_width,styles.resize_width);

#ifdef SHAPE
  /* set boundary width to zero for shaped windows */
  if (tmp_win->wShaped) tmp_win->boundary_width = 0;
#endif /* SHAPE */

  tmp_win->flags |= styles.on_flags & ALL_COMMON_FLAGS;
  /* find a suitable icon pixmap */
  if(styles.on_flags & ICON_FLAG)
    {
      /* an icon was specified */
      tmp_win->icon_bitmap_file = styles.value;
    }
  else if((tmp_win->wmhints)
	  &&(tmp_win->wmhints->flags & (IconWindowHint|IconPixmapHint)))
    {
      /* window has its own icon */
      tmp_win->icon_bitmap_file = NULL;
    }
  else
    {
      /* use default icon */
      tmp_win->icon_bitmap_file = Scr.DefaultIcon;
    }

#ifdef MINI_ICONS
  if (styles.on_flags & MINIICON_FLAG) {
    tmp_win->mini_pixmap_file = styles.mini_value;
  }
  else {
    tmp_win->mini_pixmap_file = NULL;
  }
#endif

  GetWindowSizeHints (tmp_win);

  /* Tentative size estimate */
  tmp_win->frame_width = tmp_win->attr.width+2*tmp_win->boundary_width;
  tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height+
    2*tmp_win->boundary_width;

  ConstrainSize(tmp_win, &tmp_win->frame_width, &tmp_win->frame_height, False,
		0, 0);

  /* Find out if the client requested a specific desk on the command line. */
  /*  RBW - 11/20/1998 - allow a desk of -1 to work.  */
  if (XGetCommand (dpy, tmp_win->w, &client_argv, &client_argc)) {
      XrmParseCommand (&db, table, 4, "fvwm", &client_argc, client_argv);
      XFreeStringList(client_argv);
      status = XrmGetResource (db, "fvwm.desk", "Fvwm.Desk",
                               &str_type, &rm_value);
      if ((status == True) && (rm_value.size != 0)) {
          styles.Desk = atoi(rm_value.addr);
          /*  RBW - 11/20/1998  */
          if (styles.Desk > -1)
            {
              styles.Desk++;
            }
          /**/
	  styles.on_flags |= STARTSONDESK_FLAG;
      }
/*  RBW - 11/02/1998  */
/*  RBW - 11/20/1998 - allow desk or page specs of -1 to work.  */
      /*  Handle the X Resource equivalent of StartsOnPage.  */
      status = XrmGetResource (db, "fvwm.page", "Fvwm.Page", &str_type,
			       &rm_value);
      if ((status == True) && (rm_value.size != 0)) {
          spargs = sscanf (rm_value.addr, "%d %d %d", &tmpno1, &tmpno2,
			   &tmpno3);
          switch (spargs)
            {
            case 1:
              {
                styles.on_flags |= STARTSONDESK_FLAG;
                styles.Desk     =  (tmpno1 > -1) ? tmpno1 + 1 : tmpno1;
                break;
              }
            case 2:
              {
                styles.on_flags |= STARTSONDESK_FLAG;
                styles.PageX    =  (tmpno1 > -1) ? tmpno1 + 1 : tmpno1;
                styles.PageY    =  (tmpno2 > -1) ? tmpno2 + 1 : tmpno2;
                break;
              }
            case 3:
              {
                styles.on_flags |= STARTSONDESK_FLAG;
                styles.Desk     =  (tmpno1 > -1) ? tmpno1 + 1 : tmpno1;
                styles.PageX    =  (tmpno2 > -1) ? tmpno2 + 1 : tmpno2;
                styles.PageY    =  (tmpno3 > -1) ? tmpno3 + 1 : tmpno3;
                break;
              }
            default:
              {
                break;
              }
            }
      }
/**/
      XrmDestroyDatabase (db);
      db = NULL;
  }

/*  RBW - 11/02/1998  */
  if(!PlaceWindow(tmp_win, styles.on_flags, styles.Desk, styles.PageX, styles.PageY))
    return NULL;

  /*
   * Make sure the client window still exists.  We don't want to leave an
   * orphan frame window if it doesn't.  Since we now have the server
   * grabbed, the window can't disappear later without having been
   * reparented, so we'll get a DestroyNotify for it.  We won't have
   * gotten one for anything up to here, however.
   */
  MyXGrabServer(dpy);
  if(XGetGeometry(dpy, w, &JunkRoot, &JunkX, &JunkY,
		  &JunkWidth, &JunkHeight,
		  &JunkBW,  &JunkDepth) == 0)
    {
      free((char *)tmp_win);
      MyXUngrabServer(dpy);
      return(NULL);
    }

  XSetWindowBorderWidth (dpy, tmp_win->w,0);
  if (XGetWMIconName (dpy, tmp_win->w, &text_prop))
    tmp_win->icon_name = (char *)text_prop.value;
  if(tmp_win->icon_name==(char *)NULL)
    tmp_win->icon_name = tmp_win->name;

  tmp_win->flags &= ~ICONIFIED;
  tmp_win->flags &= ~ICON_UNMAPPED;
  tmp_win->flags &= ~MAXIMIZED;

  tmp_win->TextPixel = Scr.StdColors.fore;
  tmp_win->ReliefPixel = Scr.StdRelief.fore;
  tmp_win->ShadowPixel = Scr.StdRelief.back;
  tmp_win->BackPixel = Scr.StdColors.back;

  if(styles.ForeColor != NULL) {
    XColor color;

    if((XParseColor (dpy, Scr.FvwmRoot.attr.colormap, styles.ForeColor, &color))
       &&(XAllocColor (dpy, Scr.FvwmRoot.attr.colormap, &color)))
      {
        tmp_win->TextPixel = color.pixel;
      }
  }
  if(styles.BackColor != NULL) {
    XColor color;

    if((XParseColor (dpy, Scr.FvwmRoot.attr.colormap,styles.BackColor, &color))
       &&(XAllocColor (dpy, Scr.FvwmRoot.attr.colormap, &color)))

      {
        tmp_win->BackPixel = color.pixel;
      }
    tmp_win->ShadowPixel = GetShadow(tmp_win->BackPixel);
    tmp_win->ReliefPixel = GetHilite(tmp_win->BackPixel);
  }


  /* add the window to the end of the fvwm list */
  tmp_win->next = Scr.FvwmRoot.next;
  tmp_win->prev = &Scr.FvwmRoot;
  while (tmp_win->next != NULL)
  {
    tmp_win->prev = tmp_win->next;
    tmp_win->next = tmp_win->next->next;
  }
  /* tmp_win->prev points to the last window in the list, tmp_win->next is NULL.
     Now fix the last window to point to tmp_win */
  tmp_win->prev->next = tmp_win;

  /*
      RBW - 11/13/1998 - add it into the stacking order chain also.
      This chain is anchored at both ends on Scr.FvwmRoot, there are
      no null pointers.
  */
  tmp_win->stack_next = Scr.FvwmRoot.stack_next;
  Scr.FvwmRoot.stack_next->stack_prev = tmp_win;
  tmp_win->stack_prev = &Scr.FvwmRoot;
  Scr.FvwmRoot.stack_next = tmp_win;


  /* create windows */
  tmp_win->frame_x = tmp_win->attr.x + tmp_win->old_bw - tmp_win->bw;
  tmp_win->frame_y = tmp_win->attr.y + tmp_win->old_bw - tmp_win->bw;

  tmp_win->frame_width = tmp_win->attr.width+2*tmp_win->boundary_width;
  tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height+
    2*tmp_win->boundary_width;
  ConstrainSize(tmp_win, &tmp_win->frame_width, &tmp_win->frame_height, False,
		0, 0);

  valuemask = CWBorderPixel | CWCursor | CWEventMask;
  if(Scr.d_depth < 2)
    {
      attributes.background_pixmap = Scr.light_gray_pixmap;
      if(tmp_win->flags & STICKY)
	attributes.background_pixmap = Scr.sticky_gray_pixmap;
      valuemask |= CWBackPixmap;
    }
  else
    {
      attributes.background_pixmap = None;
      attributes.background_pixel = tmp_win->BackPixel;
      valuemask |= CWBackPixel;
    }

  attributes.border_pixel = tmp_win->ShadowPixel;

  attributes.cursor = Scr.FvwmCursors[DEFAULT];
  attributes.event_mask = (SubstructureRedirectMask | ButtonPressMask |
			   ButtonReleaseMask | EnterWindowMask |
			   LeaveWindowMask | ExposureMask |
			   VisibilityChangeMask);

#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
  if ((GetDecor(tmp_win,BorderStyle.inactive.style) & ButtonFaceTypeMask)
      == TiledPixmapButton)
      TexturePixmap = GetDecor(tmp_win,BorderStyle.inactive.u.p->picture);

  if (TexturePixmap) {
      TexturePixmapSave = attributes.background_pixmap;
      attributes.background_pixmap = TexturePixmap;
      valuemask_save = valuemask;
      valuemask = (valuemask & ~CWBackPixel) | CWBackPixmap;
  }
#endif

  /* What the heck, we'll always reparent everything from now on! */
  tmp_win->frame =
    XCreateWindow (dpy, Scr.Root, tmp_win->frame_x,tmp_win->frame_y,
		   tmp_win->frame_width, tmp_win->frame_height,
		   tmp_win->bw,CopyFromParent, InputOutput,
		   CopyFromParent,
		   valuemask,
		   &attributes);

#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
  if (TexturePixmap) {
      attributes.background_pixmap = TexturePixmapSave;
      valuemask = valuemask_save;
  }
#endif

  attributes.save_under = FALSE;
  attributes.event_mask &= ~VisibilityChangeMask;

  /* Thats not all, we'll double-reparent the window ! */
  attributes.cursor = Scr.FvwmCursors[DEFAULT];

  /* make sure this does not have a BackPixel or BackPixmap so that
     that when the window dies there is no flash of BackPixel/BackPixmap */
  valuemask_save = valuemask;
  valuemask = valuemask & ~CWBackPixel & ~CWBackPixmap;
  tmp_win->Parent =
    XCreateWindow (dpy, tmp_win->frame,
		   tmp_win->boundary_width,
		   tmp_win->boundary_width+tmp_win->title_height,
		   (tmp_win->frame_width - 2*tmp_win->boundary_width),
		   (tmp_win->frame_height - 2*tmp_win->boundary_width -
		    tmp_win->title_height),tmp_win->bw, CopyFromParent,
		   InputOutput,CopyFromParent, valuemask,&attributes);
  valuemask = valuemask_save;

  attributes.event_mask = (ButtonPressMask|ButtonReleaseMask|ExposureMask|
			   EnterWindowMask|LeaveWindowMask);
  tmp_win->title_x = tmp_win->title_y = 0;
  tmp_win->title_w = 0;
  tmp_win->title_width = tmp_win->frame_width - 2*tmp_win->corner_width
    - 3 + tmp_win->bw;
  if(tmp_win->title_width < 1)
    tmp_win->title_width = 1;
  if(tmp_win->flags & BORDER)
    {
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
      if (TexturePixmap) {
	  TexturePixmapSave = attributes.background_pixmap;
	  attributes.background_pixmap = TexturePixmap;
	  valuemask_save = valuemask;
	  valuemask = (valuemask & ~CWBackPixel) | CWBackPixmap;
      }
#endif
      /* Just dump the windows any old place and left SetupFrame take
       * care of the mess */
      for(i=0;i<4;i++)
	{
	  attributes.cursor = Scr.FvwmCursors[TOP_LEFT+i];
	  tmp_win->corners[i] =
	    XCreateWindow (dpy, tmp_win->frame, 0,0,
			   tmp_win->corner_width, tmp_win->corner_width,
			   0, CopyFromParent,InputOutput,
			   CopyFromParent,
			   valuemask,
			   &attributes);
	}
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
      if (TexturePixmap) {
	  attributes.background_pixmap = TexturePixmapSave;
	  valuemask = valuemask_save;
      }
#endif
    }

  if (tmp_win->flags & TITLE)
    {
      tmp_win->title_x = tmp_win->boundary_width +tmp_win->title_height+1;
      tmp_win->title_y = tmp_win->boundary_width;
      attributes.cursor = Scr.FvwmCursors[TITLE_CURSOR];
      tmp_win->title_w =
	XCreateWindow (dpy, tmp_win->frame, tmp_win->title_x, tmp_win->title_y,
		       tmp_win->title_width, tmp_win->title_height,0,
		       CopyFromParent, InputOutput, CopyFromParent,
		       valuemask,&attributes);
      attributes.cursor = Scr.FvwmCursors[SYS];
      for(i=4;i>=0;i--)
	{
	  if((i<Scr.nr_left_buttons)&&(tmp_win->left_w[i] > 0))
	    {
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
	      if (TexturePixmap
		  && GetDecor(tmp_win,left_buttons[i].flags) & UseBorderStyle) {
		  TexturePixmapSave = attributes.background_pixmap;
		  attributes.background_pixmap = TexturePixmap;
		  valuemask_save = valuemask;
		  valuemask = (valuemask & ~CWBackPixel) | CWBackPixmap;
	      }
#endif
	      tmp_win->left_w[i] =
		XCreateWindow (dpy, tmp_win->frame, tmp_win->title_height*i, 0,
			       tmp_win->title_height, tmp_win->title_height, 0,
			       CopyFromParent, InputOutput,
			       CopyFromParent,
			       valuemask,
			       &attributes);
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
	      if (TexturePixmap
		  && GetDecor(tmp_win,left_buttons[i].flags) & UseBorderStyle) {
		  attributes.background_pixmap = TexturePixmapSave;
		  valuemask = valuemask_save;
	      }
#endif
	    }
	  else
	    tmp_win->left_w[i] = None;

	  if((i<Scr.nr_right_buttons)&&(tmp_win->right_w[i] >0)) {
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
            if (TexturePixmap
                && GetDecor(tmp_win,right_buttons[i].flags) & UseBorderStyle) {
              TexturePixmapSave = attributes.background_pixmap;
              attributes.background_pixmap = TexturePixmap;
              valuemask_save = valuemask;
              valuemask = (valuemask & ~CWBackPixel) | CWBackPixmap;
            }
#endif
            tmp_win->right_w[i] =
              XCreateWindow (dpy, tmp_win->frame,
                             tmp_win->title_width-
                             tmp_win->title_height*(i+1),
                             0, tmp_win->title_height,
                             tmp_win->title_height,
                             0, CopyFromParent, InputOutput,
                             CopyFromParent,
                             valuemask,
                             &attributes);
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
            if (TexturePixmap
                && GetDecor(tmp_win,right_buttons[i].flags) & UseBorderStyle) {
              attributes.background_pixmap = TexturePixmapSave;
              valuemask = valuemask_save;
            }
#endif
          }
	  else
	    tmp_win->right_w[i] = None;
	}
    }

  if(tmp_win->flags & BORDER)
    {
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
      if (TexturePixmap) {
	  TexturePixmapSave = attributes.background_pixmap;
	  attributes.background_pixmap = TexturePixmap;
	  valuemask_save = valuemask;
	  valuemask = (valuemask & ~CWBackPixel) | CWBackPixmap;
      }
#endif
      for(i=0;i<4;i++)
	{
	  attributes.cursor = Scr.FvwmCursors[TOP+i];
	  tmp_win->sides[i] =
	    XCreateWindow (dpy, tmp_win->frame, 0, 0, tmp_win->boundary_width,
			   tmp_win->boundary_width, 0, CopyFromParent,
			   InputOutput, CopyFromParent,
			   valuemask,
			   &attributes);
	}
#if defined(PIXMAP_BUTTONS) && defined(BORDERSTYLE)
      if (TexturePixmap) {
	  attributes.background_pixmap = TexturePixmapSave;
	  valuemask = valuemask_save;
      }
#endif
    }


#ifdef MINI_ICONS
  if (tmp_win->mini_pixmap_file) {
    tmp_win->mini_icon = CachePicture (dpy, Scr.Root,
                                       IconPath,
                                       PixmapPath,
                                       tmp_win->mini_pixmap_file,
				       Scr.ColorLimit);
  }
  else {
    tmp_win->mini_icon = NULL;
  }
#endif

  XMapSubwindows (dpy, tmp_win->frame);
  XRaiseWindow(dpy,tmp_win->Parent);
  XReparentWindow(dpy, tmp_win->w, tmp_win->Parent,0,0);

  valuemask = (CWEventMask | CWDontPropagate);
  attributes.event_mask = (StructureNotifyMask | PropertyChangeMask |
			   EnterWindowMask | LeaveWindowMask |
			   ColormapChangeMask | FocusChangeMask);

  attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;

  XChangeWindowAttributes (dpy, tmp_win->w, valuemask, &attributes);

  XAddToSaveSet(dpy, tmp_win->w);

  /*
   * Reparenting generates an UnmapNotify event, followed by a MapNotify.
   * Set the map state to FALSE to prevent a transition back to
   * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
   * again in HandleMapNotify.
   */
  tmp_win->flags &= ~MAPPED;
  width =  tmp_win->frame_width;
  tmp_win->frame_width = 0;
  height = tmp_win->frame_height;
  tmp_win->frame_height = 0;
  SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y,width,height, True);

  /* wait until the window is iconified and the icon window is mapped
   * before creating the icon window
   */
  tmp_win->icon_w = None;
  GrabButtons(tmp_win);
  GrabKeys(tmp_win);

  XSaveContext(dpy, tmp_win->w, FvwmContext, (caddr_t) tmp_win);
  XSaveContext(dpy, tmp_win->frame, FvwmContext, (caddr_t) tmp_win);
  XSaveContext(dpy, tmp_win->Parent, FvwmContext, (caddr_t) tmp_win);
  if (tmp_win->flags & TITLE)
    {
      XSaveContext(dpy, tmp_win->title_w, FvwmContext, (caddr_t) tmp_win);
      for(i=0;i<Scr.nr_left_buttons;i++)
	XSaveContext(dpy, tmp_win->left_w[i], FvwmContext, (caddr_t) tmp_win);
      for(i=0;i<Scr.nr_right_buttons;i++)
	if(tmp_win->right_w[i] != None)
	  XSaveContext(dpy, tmp_win->right_w[i], FvwmContext,
		       (caddr_t) tmp_win);
    }
  if (tmp_win->flags & BORDER)
    {
      for(i=0;i<4;i++)
	{
	  XSaveContext(dpy, tmp_win->sides[i], FvwmContext, (caddr_t) tmp_win);
	  XSaveContext(dpy,tmp_win->corners[i],FvwmContext, (caddr_t) tmp_win);
	}
    }
  RaiseWindow(tmp_win);
  KeepOnTop();
  MyXUngrabServer(dpy);

  XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
  XTranslateCoordinates(dpy,tmp_win->frame,Scr.Root,JunkX,JunkY,
			&a,&b,&JunkChild);
  tmp_win->xdiff -= a;
  tmp_win->ydiff -= b;
  if((tmp_win->flags & ClickToFocus) || Scr.MouseFocusClickRaises)
    {
     /* need to grab all buttons for window that we are about to
       * unhighlight */
      for(i=1;i<=3;i++)
	if(Scr.buttons2grab & (1<<i))
	  {
#if 0
	    XGrabButton(dpy,(i),0,tmp_win->frame,True,
			ButtonPressMask, GrabModeSync,GrabModeAsync,None,
			Scr.FvwmCursors[SYS]);
	    XGrabButton(dpy,(i),LockMask,tmp_win->frame,True,
			ButtonPressMask, GrabModeSync,GrabModeAsync,None,
			Scr.FvwmCursors[SYS]);
#else
            /* should we accept any modifier on this button? */
	    /* domivogt (2-Jan-1999): No. Or at least not like this. In the
	     * present form no button presses go through to the title bar
	     * anymore. They are all swallowed by the frame window. */
	    XGrabButton(dpy,(i),AnyModifier,tmp_win->frame,True,
  			ButtonPressMask, GrabModeSync,GrabModeAsync,None,
  			Scr.FvwmCursors[SYS]);
#endif
	  }
    }
  BroadcastConfig(M_ADD_WINDOW,tmp_win);

  BroadcastName(M_WINDOW_NAME,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->name);
  BroadcastName(M_ICON_NAME,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->icon_name);
   if (tmp_win->icon_bitmap_file != NULL &&
       tmp_win->icon_bitmap_file != Scr.DefaultIcon)
     BroadcastName(M_ICON_FILE,tmp_win->w,tmp_win->frame,
		   (unsigned long)tmp_win,tmp_win->icon_bitmap_file);
  BroadcastName(M_RES_CLASS,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->class.res_class);
  BroadcastName(M_RES_NAME,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->class.res_name);
#ifdef MINI_ICONS
  if (tmp_win->mini_icon != NULL)
    BroadcastMiniIcon(M_MINI_ICON,
                      tmp_win->w, tmp_win->frame, (unsigned long)tmp_win,
                      tmp_win->mini_icon->width,
                      tmp_win->mini_icon->height,
                      tmp_win->mini_icon->depth,
                      tmp_win->mini_icon->picture,
                      tmp_win->mini_icon->mask,
                      tmp_win->mini_pixmap_file);
#endif

  FetchWmProtocols (tmp_win);
  FetchWmColormapWindows (tmp_win);
  if(!(XGetWindowAttributes(dpy,tmp_win->w,&(tmp_win->attr))))
    tmp_win->attr.colormap = Scr.FvwmRoot.attr.colormap;
  if(NeedToResizeToo)
    {
      XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth,
		   Scr.MyDisplayHeight,
		   tmp_win->frame_x + (tmp_win->frame_width>>1),
		   tmp_win->frame_y + (tmp_win->frame_height>>1));
      Event.xany.type = ButtonPress;
      Event.xbutton.button = 1;
      Event.xbutton.x_root = tmp_win->frame_x + (tmp_win->frame_width>>1);
      Event.xbutton.y_root = tmp_win->frame_y + (tmp_win->frame_height>>1);
      Event.xbutton.x = (tmp_win->frame_width>>1);
      Event.xbutton.y = (tmp_win->frame_height>>1);
      Event.xbutton.subwindow = None;
      Event.xany.window = tmp_win->w;
      resize_window(&Event , tmp_win->w, tmp_win, C_WINDOW, "", 0);
    }
  InstallWindowColormaps(colormap_win);
  return (tmp_win);
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabButtons - grab needed buttons for the window
 *
 *  Inputs:
 *	tmp_win - the fvwm window structure to use
 *
 ***********************************************************************/
void GrabButtons(FvwmWindow *tmp_win)
{
  Binding *MouseEntry;

  MouseEntry = Scr.AllBindings;
  while(MouseEntry != (Binding *)0)
    {
      if((MouseEntry->Action != NULL)&&(MouseEntry->Context & C_WINDOW)
	 &&(MouseEntry->IsMouse == 1))
	{
	  if(MouseEntry->Button_Key >0)
	    {
	      XGrabButton(dpy, MouseEntry->Button_Key, MouseEntry->Modifier,
			  tmp_win->w,
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None,
			  Scr.FvwmCursors[DEFAULT]);
	      if(MouseEntry->Modifier != AnyModifier)
		{
		  XGrabButton(dpy, MouseEntry->Button_Key,
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w,
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None,
			      Scr.FvwmCursors[DEFAULT]);
		}
	    }
	  else
	    {
	      XGrabButton(dpy, 1, MouseEntry->Modifier,
			  tmp_win->w,
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None,
			  Scr.FvwmCursors[DEFAULT]);
	      XGrabButton(dpy, 2, MouseEntry->Modifier,
			  tmp_win->w,
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None,
			  Scr.FvwmCursors[DEFAULT]);
	      XGrabButton(dpy, 3, MouseEntry->Modifier,
			  tmp_win->w,
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None,
			  Scr.FvwmCursors[DEFAULT]);
	      if(MouseEntry->Modifier != AnyModifier)
		{
		  XGrabButton(dpy, 1,
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w,
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None,
			      Scr.FvwmCursors[DEFAULT]);
		  XGrabButton(dpy, 2,
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w,
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None,
			      Scr.FvwmCursors[DEFAULT]);
		  XGrabButton(dpy, 3,
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w,
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None,
			      Scr.FvwmCursors[DEFAULT]);
		}
	    }
	}
      MouseEntry = MouseEntry->NextBinding;
    }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabKeys - grab needed keys for the window
 *
 *  Inputs:
 *	tmp_win - the fvwm window structure to use
 *
 ***********************************************************************/
void GrabKeys(FvwmWindow *tmp_win)
{
  Binding *tmp;
  for (tmp = Scr.AllBindings; tmp != NULL; tmp = tmp->NextBinding)
    {
      if((tmp->Context & (C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR))&&
	 (tmp->IsMouse == 0))
	{
	  XGrabKey(dpy, tmp->Button_Key, tmp->Modifier, tmp_win->frame, True,
		   GrabModeAsync, GrabModeAsync);
	  if(tmp->Modifier != AnyModifier)
	    {
	      XGrabKey(dpy, tmp->Button_Key, tmp->Modifier|LockMask,
		       tmp_win->frame, True,
		       GrabModeAsync, GrabModeAsync);
	    }
	}
    }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	FetchWMProtocols - finds out which protocols the window supports
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/
void FetchWmProtocols (FvwmWindow *tmp)
{
  unsigned long flags = 0L;
  Atom *protocols = NULL, *ap;
  int i, n;
  Atom atype;
  int aformat;
  unsigned long bytes_remain,nitems;

  if(tmp == NULL) return;
  /* First, try the Xlib function to read the protocols.
   * This is what Twm uses. */
  if (XGetWMProtocols (dpy, tmp->w, &protocols, &n))
    {
      for (i = 0, ap = protocols; i < n; i++, ap++)
	{
	  if (*ap == (Atom)_XA_WM_TAKE_FOCUS) flags |= DoesWmTakeFocus;
	  if (*ap == (Atom)_XA_WM_DELETE_WINDOW) flags |= DoesWmDeleteWindow;
	}
      if (protocols) XFree ((char *) protocols);
    }
  else
    {
      /* Next, read it the hard way. mosaic from Coreldraw needs to
       * be read in this way. */
      if ((XGetWindowProperty(dpy, tmp->w, _XA_WM_PROTOCOLS, 0L, 10L, False,
			      _XA_WM_PROTOCOLS, &atype, &aformat, &nitems,
			      &bytes_remain,
			      (unsigned char **)&protocols))==Success)
	{
	  for (i = 0, ap = protocols; i < nitems; i++, ap++)
	    {
	      if (*ap == (Atom)_XA_WM_TAKE_FOCUS) flags |= DoesWmTakeFocus;
	      if (*ap == (Atom)_XA_WM_DELETE_WINDOW) flags |= DoesWmDeleteWindow;
	    }
	  if (protocols) XFree ((char *) protocols);
	}
    }
  tmp->flags |= flags;
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GetWindowSizeHints - gets application supplied size info
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/
void GetWindowSizeHints(FvwmWindow *tmp)
{
  long supplied = 0;

  if (!XGetWMNormalHints (dpy, tmp->w, &tmp->hints, &supplied))
    tmp->hints.flags = 0;

  /* Beat up our copy of the hints, so that all important field are
   * filled in! */
  if (tmp->hints.flags & PResizeInc)
    {
      if (tmp->hints.width_inc == 0) tmp->hints.width_inc = 1;
      if (tmp->hints.height_inc == 0) tmp->hints.height_inc = 1;
    }
  else
    {
      tmp->hints.width_inc = 1;
      tmp->hints.height_inc = 1;
    }

  /*
   * ICCCM says that PMinSize is the default if no PBaseSize is given,
   * and vice-versa.
   */

  if(!(tmp->hints.flags & PBaseSize))
    {
      if(tmp->hints.flags & PMinSize)
	{
	  tmp->hints.base_width = tmp->hints.min_width;
	  tmp->hints.base_height = tmp->hints.min_height;
	}
      else
	{
	  tmp->hints.base_width = 0;
	  tmp->hints.base_height = 0;
	}
    }
  if(!(tmp->hints.flags & PMinSize))
    {
      tmp->hints.min_width = tmp->hints.base_width;
      tmp->hints.min_height = tmp->hints.base_height;
    }
  if(!(tmp->hints.flags & PMaxSize))
    {
      tmp->hints.max_width = MAX_WINDOW_WIDTH;
      tmp->hints.max_height = MAX_WINDOW_HEIGHT;
    }
  if(tmp->hints.max_width < tmp->hints.min_width)
    tmp->hints.max_width = MAX_WINDOW_WIDTH;
  if(tmp->hints.max_height < tmp->hints.min_height)
    tmp->hints.max_height = MAX_WINDOW_HEIGHT;

  /* Zero width/height windows are bad news! */
  if(tmp->hints.min_height <= 0)
    tmp->hints.min_height = 1;
  if(tmp->hints.min_width <= 0)
    tmp->hints.min_width = 1;

  if(!(tmp->hints.flags & PWinGravity))
    {
      tmp->hints.win_gravity = NorthWestGravity;
      tmp->hints.flags |= PWinGravity;
    }

  if (tmp->hints.flags & PAspect)
  {
    /*
    ** check to make sure min/max aspect ratios look valid
    */
#define maxAspectX tmp->hints.max_aspect.x
#define maxAspectY tmp->hints.max_aspect.y
#define minAspectX tmp->hints.min_aspect.x
#define minAspectY tmp->hints.min_aspect.y
    /*
    ** The math looks like this:
    **
    **   minAspectX    maxAspectX
    **   ---------- <= ----------
    **   minAspectY    maxAspectY
    **
    ** If that is multiplied out, this must be satisfied:
    **
    **   minAspectX * maxAspectY <=  maxAspectX * minAspectY
    **
    ** So, what to do if this isn't met?  Ignoring it entirely
    ** seems safest.
    **
    */
    if ((minAspectX * maxAspectY) > (maxAspectX * minAspectY))
    {
      tmp->hints.flags &= ~PAspect;
      fvwm_msg(WARN,
               "GetWindowSizeHints",
               "window id 0x%08x max_aspect ratio is < min_aspect ratio -> ignoring, but program displaying this window should be fixed!!!!",
               tmp->w);
    }
  }
}



/***********************************************************************
 *
 *  Procedure:
 *	LookInList - look through a list for a window name, or class
 *
 *  Returned Value:
 *	merged matching styles in callers name_list.
 *
 *  Inputs:
 *	tmp_win - FvwWindow structure to match against
 *	styles - callers return area
 *
 *  Changes:
 *      dje 10/06/97 test for NULL class removed, can't happen.
 *      use merge subroutine instead of coding merges 3 times.
 *      Use structure to return values, not many, many args
 *      and return value.
 *      Point at iconboxes chain, not single iconboxes elements.
 *
 ***********************************************************************/
void LookInList(  FvwmWindow *tmp_win, name_list *styles)
{
  name_list *nptr;

  memset(styles, 0, sizeof(name_list)); /* clear callers return area */
  /* look thru all styles in order defined. */
  for (nptr = Scr.TheList; nptr != NULL; nptr = nptr->next) {
    /* If name/res_class/res_name match, merge */
    if (matchWildcards(nptr->name,tmp_win->class.res_class) == TRUE) {
      merge_styles(styles, nptr);
    } else if (matchWildcards(nptr->name,tmp_win->class.res_name) == TRUE) {
      merge_styles(styles, nptr);
    } else if (matchWildcards(nptr->name,tmp_win->name) == TRUE) {
      merge_styles(styles, nptr);
    }
  }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 * merge_styles - For a matching style, merge name_list to name_list
 *
 *  Returned Value:
 *	merged matching styles in callers name_list.
 *
 *  Inputs:
 *	styles - callers return area
 *      nptr - matching name_list
 *
 *  Note:
 *      The only trick here is that on and off flags/buttons are
 *      combined into the on flag/button.
 *
 ***********************************************************************/

static void merge_styles(name_list *styles, name_list *nptr) {
  if(nptr->value != NULL) styles->value = nptr->value;
#ifdef MINI_ICONS
  if(nptr->mini_value != NULL) styles->mini_value = nptr->mini_value;
#endif
#ifdef USEDECOR
  if (nptr->Decor != NULL) styles->Decor = nptr->Decor;
#endif
  if(nptr->off_flags & STARTSONDESK_FLAG)
    /*  RBW - 11/02/1998  */
    {
      styles->Desk = nptr->Desk;
      styles->PageX = nptr->PageX;
      styles->PageY = nptr->PageY;
    }
  if(nptr->off_flags & BW_FLAG)
    styles->border_width = nptr->border_width;
  if(nptr->off_flags & FORE_COLOR_FLAG)
    styles->ForeColor = nptr->ForeColor;
  if(nptr->off_flags & BACK_COLOR_FLAG)
    styles->BackColor = nptr->BackColor;
  if(nptr->off_flags & NOBW_FLAG)
    styles->resize_width = nptr->resize_width;
  styles->on_flags |= nptr->off_flags;  /* combine on and off flags */
  styles->on_flags &= ~(nptr->on_flags);
  styles->on_buttons |= nptr->off_buttons; /* combine buttons */
  styles->on_buttons &= ~(nptr->on_buttons);
  /* Note, only one style cmd can define a windows iconboxes,
     the last one encountered. */
  if(nptr->IconBoxes != NULL) {         /* If style has iconboxes */
    styles->IconBoxes = nptr->IconBoxes; /* copy it */
  }
  return;                               /* return */
}
