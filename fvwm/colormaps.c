/****************************************************************************
 * This module is all new
 * by Rob Nation
 *
 * This code handles colormaps for fvwm.
 *
 * Copyright 1994 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved . No guarantees or
 * warrantees of any sort whatsoever are given or implied or anything.
 ****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include "fvwm.h"
#include <X11/Xatom.h>
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

FvwmWindow *colormap_win;
Colormap last_cmap = None;
extern FvwmWindow *Tmp_win;

/***********************************************************************
 *
 *  Procedure:
 *	HandleColormapNotify - colormap notify event handler
 *
 * This procedure handles both a client changing its own colormap, and
 * a client explicitly installing its colormap itself (only the window
 * manager should do that, so we must set it correctly).
 *
 ***********************************************************************/
void HandleColormapNotify(void)
{
  XColormapEvent *cevent = (XColormapEvent *)&Event;
  Bool ReInstall = False;


  if(!Tmp_win)
    {
      return;
    }
  if(cevent->new)
    {
      XGetWindowAttributes(dpy,Tmp_win->w,&(Tmp_win->attr));
      if((Tmp_win  == colormap_win)&&(Tmp_win->number_cmap_windows == 0))
	last_cmap = Tmp_win->attr.colormap;
      ReInstall = True;
    }
  else if((cevent->state == ColormapUninstalled)&&
	  (last_cmap == cevent->colormap))
    {
      /* Some window installed its colormap, change it back */
      ReInstall = True;
    }

  while(XCheckTypedEvent(dpy,ColormapNotify,&Event))
    {
      if (XFindContext (dpy, cevent->window,
			FvwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
	Tmp_win = NULL;
      if((Tmp_win)&&(cevent->new))
	{
	  XGetWindowAttributes(dpy,Tmp_win->w,&(Tmp_win->attr));
	  if((Tmp_win  == colormap_win)&&(Tmp_win->number_cmap_windows == 0))
	    last_cmap = Tmp_win->attr.colormap;
	  ReInstall = True;
	}
      else if((Tmp_win)&&
	      (cevent->state == ColormapUninstalled)&&
	      (last_cmap == cevent->colormap))
	{
	  /* Some window installed its colormap, change it back */
	  ReInstall = True;
	}
      else if((Tmp_win)&&
	      (cevent->state == ColormapInstalled)&&
	      (last_cmap == cevent->colormap))
	{
	  /* The last color map installed was the correct one. Don't
	   * change anything */
	  ReInstall = False;
	}
    }

  /* Reinstall the colormap that we think should be installed,
   * UNLESS and unrecognized window has the focus - it might be
   * an override-redirect window that has its own colormap. */
  if((ReInstall)&&(Scr.UnknownWinFocused == None))
    {
      XInstallColormap(dpy,last_cmap);
    }
}

/************************************************************************
 *
 * Re-Install the active colormap
 *
 *************************************************************************/
void ReInstallActiveColormap(void)
{
  InstallWindowColormaps(colormap_win);
}

/***********************************************************************
 *
 *  Procedure:
 *	InstallWindowColormaps - install the colormaps for one fvwm window
 *
 *  Inputs:
 *	type	- type of event that caused the installation
 *	tmp	- for a subset of event types, the address of the
 *		  window structure, whose colormaps are to be installed.
 *
 ************************************************************************/

void InstallWindowColormaps (FvwmWindow *tmp)
{
  int i;
  XWindowAttributes attributes;
  Window w;
  Bool ThisWinInstalled = False;


  /* If no window, then install root colormap */
  if(!tmp)
    tmp = &Scr.FvwmRoot;

  colormap_win = tmp;
  /* Save the colormap to be loaded for when force loading of
   * root colormap(s) ends.
   */
  Scr.pushed_window = tmp;
  /* Don't load any new colormap if root colormap(s) has been
   * force loaded.
   */
  if (Scr.root_pushes)
    {
      return;
    }

  if(tmp->number_cmap_windows > 0)
    {
      for(i=tmp->number_cmap_windows -1; i>=0;i--)
	{
	  w = tmp->cmap_windows[i];
	  if(w == tmp->w)
	    ThisWinInstalled = True;
	  XGetWindowAttributes(dpy,w,&attributes);

          /*
           * On Sun X servers, don't install 24 bit TrueColor colourmaps.
           * Despite what the server says, these colourmaps are always
           * installed.
           */
	  if(last_cmap != attributes.colormap
#if defined(sun) && defined(TRUECOLOR_ALWAYS_INSTALLED)
             && !(attributes.depth == 24 && attributes.visual->class == TrueColor)
#endif
             )
	    {
	      last_cmap = attributes.colormap;
	      XInstallColormap(dpy,attributes.colormap);
	    }
	}
    }

  if(!ThisWinInstalled)
    {
      if(last_cmap != tmp->attr.colormap
#if defined(sun) && defined(TRUECOLOR_ALWAYS_INSTALLED)
         && !(tmp->attr.depth == 24 && tmp->attr.visual->class == TrueColor)
#endif
        )
	{
	  last_cmap = tmp->attr.colormap;
	  XInstallColormap(dpy,tmp->attr.colormap);
	}
    }
}


/***********************************************************************
 *
 *  Procedures:
 *	<Uni/I>nstallRootColormap - Force (un)loads root colormap(s)
 *
 *	   These matching routines provide a mechanism to insure that
 *	   the root colormap(s) is installed during operations like
 *	   rubber banding or menu display that require colors from
 *	   that colormap.  Calls may be nested arbitrarily deeply,
 *	   as long as there is one UninstallRootColormap call per
 *	   InstallRootColormap call.
 *
 *	   The final UninstallRootColormap will cause the colormap list
 *	   which would otherwise have be loaded to be loaded, unless
 *	   Enter or Leave Notify events are queued, indicating some
 *	   other colormap list would potentially be loaded anyway.
 ***********************************************************************/
void InstallRootColormap()
{
  FvwmWindow *tmp;
  if (Scr.root_pushes == 0)
    {
      tmp = Scr.pushed_window;
      InstallWindowColormaps(&Scr.FvwmRoot);
      Scr.pushed_window = tmp;
    }
  Scr.root_pushes++;
  return;
}

/***************************************************************************
 *
 * Unstacks one layer of root colormap pushing
 * If we peel off the last layer, re-install th e application colormap
 *
 ***************************************************************************/
void UninstallRootColormap()
{
  if (Scr.root_pushes)
    Scr.root_pushes--;

  if (!Scr.root_pushes)
    {
      InstallWindowColormaps(Scr.pushed_window);
    }

  return;
}



/*****************************************************************************
 *
 * Gets the WM_COLORMAP_WINDOWS property from the window
 *   This property typically doesn't exist, but a few applications
 *   use it. These seem to occur mostly on SGI machines.
 *
 ****************************************************************************/
void FetchWmColormapWindows (FvwmWindow *tmp)
{
  if(tmp->cmap_windows != (Window *)NULL)
    XFree((void *)tmp->cmap_windows);

  if(!XGetWMColormapWindows (dpy, tmp->w, &(tmp->cmap_windows),
			     &(tmp->number_cmap_windows)))
    {
      tmp->number_cmap_windows = 0;
      tmp->cmap_windows = NULL;
    }
}


