#include "../configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

/***************************************************************************
 * 
 * Check to see if the pointer is on the edge of the screen, and scroll/page
 * if needed 
 ***************************************************************************/
void HandlePaging(int HorWarpSize, int VertWarpSize, int *xl, int *yt, 
		  int *delta_x, int *delta_y,Bool Grab)
{
  int x,y,total;

  *delta_x = 0;
  *delta_y = 0;
  
  if((Scr.ScrollResistance >= 10000)||
     ((HorWarpSize ==0)&&(VertWarpSize==0)))
    return;
  
  /* need to move the viewport */
  if(( Scr.VxMax == 0 ||
       (*xl >= SCROLL_REGION && *xl < Scr.MyDisplayWidth  - SCROLL_REGION)) &&
     ( Scr.VyMax == 0 ||
       (*yt >= SCROLL_REGION && *yt < Scr.MyDisplayHeight - SCROLL_REGION)))
    return;
  
  total = 0;
  while(total < Scr.ScrollResistance)
    {
      sleep_a_little(10000);
      total+=10;

  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x, &y, &JunkX, &JunkY, &JunkMask); 

      if(XCheckWindowEvent(dpy,Scr.PanFrameTop.win,
			   LeaveWindowMask,&Event))
	{
	  StashEventTime(&Event);
	  return;
	}
      if(XCheckWindowEvent(dpy,Scr.PanFrameBottom.win,
			   LeaveWindowMask,&Event))
	{
	  StashEventTime(&Event);
	  return;
	}	
      if(XCheckWindowEvent(dpy,Scr.PanFrameLeft.win,
			   LeaveWindowMask,&Event))
	{
	  StashEventTime(&Event);
	  return;
	}      
      if(XCheckWindowEvent(dpy,Scr.PanFrameRight.win,
			   LeaveWindowMask,&Event))
	{
	  StashEventTime(&Event);
	  return;
	}
      /* check actual pointer location since PanFrames can get buried under
         a window being moved or resized - mab */
      if(( x >= SCROLL_REGION )&&( x < Scr.MyDisplayWidth-SCROLL_REGION )&&
         ( y >= SCROLL_REGION )&&( y < Scr.MyDisplayHeight-SCROLL_REGION ))
           return ;
    }
  
  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x, &y, &JunkX, &JunkY, &JunkMask);   
  
  /* Move the viewport */
  /* and/or move the cursor back to the approximate correct location */
  /* that is, the same place on the virtual desktop that it */
  /* started at */
  if( x<SCROLL_REGION)
    *delta_x = -HorWarpSize;
  else if ( x >= Scr.MyDisplayWidth-SCROLL_REGION)
    *delta_x = HorWarpSize;
  else
    *delta_x = 0;
  if (Scr.VxMax == 0) *delta_x = 0;
  if( y<SCROLL_REGION)
    *delta_y = -VertWarpSize;
  else if ( y >= Scr.MyDisplayHeight-SCROLL_REGION)
    *delta_y = VertWarpSize;
  else
    *delta_y = 0;
  if (Scr.VyMax == 0) *delta_y = 0;
  
  /* Ouch! lots of bounds checking */
  if(Scr.Vx + *delta_x < 0) 
    {
      if (!(Scr.flags & EdgeWrapX ))
	{
	  *delta_x = -Scr.Vx;
	  *xl = x - *delta_x;
	}
      else 
	{
	  *delta_x += Scr.VxMax + Scr.MyDisplayWidth;
	  *xl = x + *delta_x % Scr.MyDisplayWidth + HorWarpSize;
	}
    }
  else if(Scr.Vx + *delta_x > Scr.VxMax)
    {
      if (!(Scr.flags & EdgeWrapX))
	{
	  *delta_x = Scr.VxMax - Scr.Vx;
	  *xl = x - *delta_x;
	}
      else 
	{
	  *delta_x -= Scr.VxMax +Scr.MyDisplayWidth;
	  *xl = x + *delta_x % Scr.MyDisplayWidth - HorWarpSize;
	}
    }
  else
    *xl = x - *delta_x;
  
  if(Scr.Vy + *delta_y < 0) 
    {
      if (!(Scr.flags & EdgeWrapY))
	{
	  *delta_y = -Scr.Vy;
	  *yt = y - *delta_y;
	}
      else
	{
	  *delta_y += Scr.VyMax + Scr.MyDisplayHeight;
	  *yt = y + *delta_y % Scr.MyDisplayHeight + VertWarpSize;
	}
    }
  else if(Scr.Vy + *delta_y > Scr.VyMax) 
    {
      if (!(Scr.flags & EdgeWrapY))
	{
	  *delta_y = Scr.VyMax - Scr.Vy;
	  *yt = y - *delta_y;
	}
      else
	{
	  *delta_y -= Scr.VyMax + Scr.MyDisplayHeight;
	  *yt = y + *delta_y % Scr.MyDisplayHeight - VertWarpSize;
	}
    }
  else
    *yt = y - *delta_y;
  
  if(*xl <= SCROLL_REGION) *xl = SCROLL_REGION+1;
  if(*yt <= SCROLL_REGION) *yt = SCROLL_REGION+1;
  if(*xl >= Scr.MyDisplayWidth - SCROLL_REGION)
    *xl = Scr.MyDisplayWidth - SCROLL_REGION -1;
  if(*yt >= Scr.MyDisplayHeight - SCROLL_REGION)
    *yt = Scr.MyDisplayHeight - SCROLL_REGION -1;
  
  if((*delta_x != 0)||(*delta_y!=0))
    {
      if(Grab)
	MyXGrabServer(dpy);
      /* Turn off the rubberband if its on */
      MoveOutline(Scr.Root,0,0,0,0);
      XWarpPointer(dpy,None,Scr.Root,0,0,0,0,*xl,*yt);
      MoveViewport(Scr.Vx + *delta_x,Scr.Vy + *delta_y,False);
      XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		    xl, yt, &JunkX, &JunkY, &JunkMask);
      if(Grab)
	MyXUngrabServer(dpy);
    }
}



/* the root window is surrounded by four window slices, which are InputOnly.
 * So you can see 'through' them, but they eat the input. An EnterEvent in
 * one of these windows causes a Paging. The windows have the according cursor
 * pointing in the pan direction or are hidden if there is no more panning
 * in that direction. This is mostly intended to get a panning even atop
 * of Motif applictions, which does not work yet. It seems Motif windows
 * eat all mouse events.
 *
 * Hermann Dunkel, HEDU, dunkel@cul-ipn.uni-kiel.de 1/94
*/

/***************************************************************************
 * checkPanFrames hides PanFrames if they are on the very border of the
 * VIRTUELL screen and EdgeWrap for that direction is off. 
 * (A special cursor for the EdgeWrap border could be nice) HEDU
 ****************************************************************************/
void checkPanFrames(void)
{
  int wrapX = (Scr.flags & EdgeWrapX);
  int wrapY = (Scr.flags & EdgeWrapY);

  if(!(Scr.flags & WindowsCaptured))
    return;

  /* Remove Pan frames if paging by edge-scroll is permanently or
   * temporarily disabled */
  if(Scr.EdgeScrollY == 0)
    {
      XUnmapWindow(dpy,Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped=False;   
      XUnmapWindow (dpy,Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped=False;
    }   
  if(Scr.EdgeScrollX == 0)
    {
      XUnmapWindow(dpy,Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped=False;
      XUnmapWindow (dpy,Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped=False;
    }   
  if((Scr.EdgeScrollX == 0)&&(Scr.EdgeScrollY == 0))
    return;

  /* LEFT, hide only if EdgeWrap is off */
  if (Scr.Vx==0 && Scr.PanFrameLeft.isMapped && (!wrapX)) 
    {
      XUnmapWindow(dpy,Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped=False;
    }
  else if (Scr.Vx > 0 && Scr.PanFrameLeft.isMapped==False) 
    {
      XMapRaised(dpy,Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped=True;
    }
  /* RIGHT, hide only if EdgeWrap is off */
  if (Scr.Vx == Scr.VxMax && Scr.PanFrameRight.isMapped && (!wrapX))
    {
      XUnmapWindow (dpy,Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped=False;
    }
  else if (Scr.Vx < Scr.VxMax && Scr.PanFrameRight.isMapped==False) 
    {
      XMapRaised(dpy,Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped=True;
    }
  /* TOP, hide only if EdgeWrap is off */
  if (Scr.Vy==0 && Scr.PanFrameTop.isMapped && (!wrapY)) 
    {
      XUnmapWindow(dpy,Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped=False;
    }
  else if (Scr.Vy > 0 && Scr.PanFrameTop.isMapped==False) 
    {
      XMapRaised(dpy,Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped=True;
    }
  /* BOTTOM, hide only if EdgeWrap is off */
  if (Scr.Vy == Scr.VyMax && Scr.PanFrameBottom.isMapped && (!wrapY)) 
    {
      XUnmapWindow (dpy,Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped=False;
    }
  else if (Scr.Vy < Scr.VyMax && Scr.PanFrameBottom.isMapped==False) 
    {
      XMapRaised(dpy,Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped=True;
    }
}

/****************************************************************************
 *
 * Gotta make sure these things are on top of everything else, or they
 * don't work!
 *
 * For some reason, this seems to be unneeded.
 *
 ***************************************************************************/
void raisePanFrames(void)
{
  if (Scr.PanFrameTop.isMapped) XRaiseWindow(dpy,Scr.PanFrameTop.win);
  if (Scr.PanFrameLeft.isMapped) XRaiseWindow(dpy,Scr.PanFrameLeft.win);
  if (Scr.PanFrameRight.isMapped) XRaiseWindow(dpy,Scr.PanFrameRight.win);
  if (Scr.PanFrameBottom.isMapped) XRaiseWindow(dpy,Scr.PanFrameBottom.win);
}

/****************************************************************************
 *
 * Creates the windows for edge-scrolling 
 *
 ****************************************************************************/
void initPanFrames()
{
  XSetWindowAttributes attributes;    /* attributes for create */
  unsigned long valuemask; 
  
  attributes.event_mask =  (EnterWindowMask | LeaveWindowMask | 
			    VisibilityChangeMask);
  valuemask=  (CWEventMask | CWCursor );
  
  attributes.cursor = Scr.FvwmCursors[TOP];
  Scr.PanFrameTop.win = 
    XCreateWindow (dpy, Scr.Root, 
		   0,0,
		   Scr.MyDisplayWidth,PAN_FRAME_THICKNESS,
		   0,	/* no border */
		   CopyFromParent, InputOnly,
		   CopyFromParent, 
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[LEFT];
  Scr.PanFrameLeft.win = 
    XCreateWindow (dpy, Scr.Root, 
		   0,PAN_FRAME_THICKNESS,
		   PAN_FRAME_THICKNESS,
		   Scr.MyDisplayHeight-2*PAN_FRAME_THICKNESS,
		   0,	/* no border */
		   CopyFromParent, InputOnly, CopyFromParent, 
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[RIGHT];
  Scr.PanFrameRight.win = 
    XCreateWindow (dpy, Scr.Root, 
		   Scr.MyDisplayWidth-PAN_FRAME_THICKNESS,PAN_FRAME_THICKNESS,
		   PAN_FRAME_THICKNESS,
		   Scr.MyDisplayHeight-2*PAN_FRAME_THICKNESS,
		   0,	/* no border */
		   CopyFromParent, InputOnly, CopyFromParent, 
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[BOTTOM];
  Scr.PanFrameBottom.win = 
    XCreateWindow (dpy, Scr.Root, 
		   0,Scr.MyDisplayHeight-PAN_FRAME_THICKNESS,
		   Scr.MyDisplayWidth,PAN_FRAME_THICKNESS,
		   0,	/* no border */
		   CopyFromParent, InputOnly, CopyFromParent, 
		   valuemask, &attributes);
  Scr.PanFrameTop.isMapped=Scr.PanFrameLeft.isMapped=
    Scr.PanFrameRight.isMapped= Scr.PanFrameBottom.isMapped=False;
}


/***************************************************************************
 *
 *  Moves the viewport within thwe virtual desktop
 *
 ***************************************************************************/
void MoveViewport(int newx, int newy, Bool grab)
{
  FvwmWindow *t;
  int deltax,deltay;

  if(grab)
    MyXGrabServer(dpy);


  if(newx > Scr.VxMax)
    newx = Scr.VxMax;
  if(newy > Scr.VyMax)
    newy = Scr.VyMax;
  if(newx <0)
    newx = 0;
  if(newy <0)
    newy = 0;

  deltay = Scr.Vy - newy;
  deltax = Scr.Vx - newx;

  Scr.Vx = newx;
  Scr.Vy = newy;
  Broadcast(M_NEW_PAGE,5,Scr.Vx,Scr.Vy,Scr.CurrentDesk,Scr.VxMax,Scr.VyMax,0,0);

  if((deltax!=0)||(deltay!=0))
    {
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
	  /* If the window is iconified, and sticky Icons is set,
	   * then the window should essentially be sticky */
	  if(!((t->flags & ICONIFIED)&&(t->flags & StickyIcon)) &&
	     (!(t->flags & STICKY)))
	    {
              if(!(t->flags & StickyIcon))
		{
		  t->icon_x_loc += deltax;
		  t->icon_xl_loc += deltax;
		  t->icon_y_loc += deltay;
		  if(t->icon_pixmap_w != None)
		    XMoveWindow(dpy,t->icon_pixmap_w,t->icon_x_loc,
				t->icon_y_loc);
                  if(t->icon_w != None)
                    XMoveWindow(dpy,t->icon_w,t->icon_x_loc,
                                t->icon_y_loc+t->icon_p_height);
		  if(!(t->flags &ICON_UNMAPPED))
		    Broadcast(M_ICON_LOCATION,7,t->w,t->frame,
			      (unsigned long)t,
			      t->icon_x_loc,t->icon_y_loc,
			      t->icon_w_width, 
			      t->icon_w_height+t->icon_p_height);
		}
	      SetupFrame (t, t->frame_x+ deltax, t->frame_y + deltay,
			  t->frame_width, t->frame_height,FALSE);
	    }
	}
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
	  /* If its an icon, and its sticking, autoplace it so
	   * that it doesn't wind up on top a a stationary
	   * icon */
	  if(((t->flags & STICKY)||(t->flags & StickyIcon))&&
	     (t->flags & ICONIFIED)&&(!(t->flags & ICON_MOVED))&&
	     (!(t->flags & ICON_UNMAPPED)))
	    AutoPlace(t);
	}

    }
  checkPanFrames();

  /* do this with PanFrames too ??? HEDU */
  while(XCheckTypedEvent(dpy,MotionNotify,&Event))
    StashEventTime(&Event);    
  if(grab)
    MyXUngrabServer(dpy);
}


/**************************************************************************
 * 
 * Move to a new desktop
 *
 *************************************************************************/
void changeDesks_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,char *action, int *Module)
{
  int n,val1, val1_unit, val2, val2_unit;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  changeDesks(val1,val2);
}

void  changeDesks(int val1, int val2)
{
  int oldDesk;
  FvwmWindow *FocusWin = 0, *t;
  static FvwmWindow *StickyWin = 0;

  oldDesk = Scr.CurrentDesk;

  if(val1 != 0)
    {
      Scr.CurrentDesk = Scr.CurrentDesk + val1;
    }
  else
    {
      Scr.CurrentDesk = val2;
      if(Scr.CurrentDesk == oldDesk)
	return;
    }

  Broadcast(M_NEW_DESK,1,Scr.CurrentDesk,0,0,0,0,0,0);
  /* Scan the window list, mapping windows on the new Desk,
   * unmapping windows on the old Desk */
  MyXGrabServer(dpy);
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      /* Only change mapping for non-sticky windows */
      if(!((t->flags & ICONIFIED)&&(t->flags & StickyIcon)) &&
	 (!(t->flags & STICKY))&&(!(t->flags & ICON_UNMAPPED)))
	{
	  if(t->Desk == oldDesk) 
	    {
	      if (Scr.Focus == t)
		t->FocusDesk = oldDesk;
	      else
		t->FocusDesk = -1;
	      UnmapIt(t);
	    } 
	  else if(t->Desk == Scr.CurrentDesk) 
	    {
	      MapIt(t);
	      if (t->FocusDesk == Scr.CurrentDesk) 
		{
		  FocusWin = t;
		}
	    }
	}
      else
	{
	  /* Window is sticky */
	  t->Desk = Scr.CurrentDesk;
	  if (Scr.Focus == t) 
	    {
	      t->FocusDesk =oldDesk;
	      StickyWin = t;
	    }
	}
    }
  MyXUngrabServer(dpy);
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      /* If its an icon, and its sticking, autoplace it so
       * that it doesn't wind up on top a a stationary
       * icon */
      if(((t->flags & STICKY)||(t->flags & StickyIcon))&&
	 (t->flags & ICONIFIED)&&(!(t->flags & ICON_MOVED))&&
	 (!(t->flags & ICON_UNMAPPED)))
	AutoPlace(t);
    }
  
  if((FocusWin)&&(FocusWin->flags & ClickToFocus))
#ifndef NO_REMEMBER_FOCUS
    SetFocus(FocusWin->w, FocusWin,0);
  /* OK, someone beat me up, but I don't like this. If you are a predominantly
   * focus-follows-mouse person, but put in one sticky click-to-focus window
   * (typically because you don't really want to give focus to this window),
   * then the following lines are screwed up. */
/*  else if (StickyWin && (StickyWin->flags && STICKY))
    SetFocus(StickyWin->w, StickyWin,1);*/
  else
#endif
    SetFocus(Scr.NoFocusWin,NULL,1);
}



/**************************************************************************
 * 
 * Move to a new desktop
 *
 *************************************************************************/
void changeWindowsDesk(XEvent *eventp,Window w,FvwmWindow *t,
		       unsigned long context,char *action, int *Module)
{
  int val1, val2;
  int val1_unit,val2_unit,n;
  
  if (DeferExecution(eventp,&w,&t,&context,SELECT,ButtonRelease))
    return;

  if(t == NULL)
    return;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);

  if(n != 2)
  {
    n = GetOneArgument(action, &val2, &val2_unit);
    val1 = 0;
  }

  if(val1 != 0)
    val1 += t->Desk;
  else
    val1 = val2;

  if(val1 == t->Desk)
    return;

  /* Scan the window list, mapping windows on the new Desk,
   * unmapping windows on the old Desk */
  /* Only change mapping for non-sticky windows */
  if(!((t->flags & ICONIFIED)&&(t->flags & StickyIcon)) &&
     (!(t->flags & STICKY))&&(!(t->flags & ICON_UNMAPPED)))
    {
      if(t->Desk == Scr.CurrentDesk)
	{
	  t->Desk = val1;
	  UnmapIt(t);
	}
      else if(val1 == Scr.CurrentDesk)
	{
	  t->Desk = val1;
	  /* If its an icon, auto-place it */
	  if(t->flags & ICONIFIED)
	    AutoPlace(t);
	  MapIt(t);
	}
      else
	t->Desk = val1;
      
    }
  BroadcastConfig(M_CONFIGURE_WINDOW,t);
}


void scroll(XEvent *eventp,Window w,FvwmWindow *tmp_win,unsigned long context,
	    char *action, int *Module)
{
  int x,y;
  int val1, val2, val1_unit,val2_unit,n;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);

  if((val1 > -100000)&&(val1 < 100000))
    x=Scr.Vx + val1*val1_unit/100;
  else
    x = Scr.Vx + (val1/1000)*val1_unit/100;
  
  if((val2 > -100000)&&(val2 < 100000))
    y=Scr.Vy + val2*val2_unit/100;
  else
    y = Scr.Vy + (val2/1000)*val2_unit/100;
  
  if(((val1 <= -100000)||(val1 >= 100000))&&(x>Scr.VxMax))
    {
      x = 0;
      y += Scr.MyDisplayHeight;
      if(y > Scr.VyMax)
	y=0;
    }
  if(((val1 <= -100000)||(val1 >= 100000))&&(x<0))
    {
      x = Scr.VxMax;
      y -= Scr.MyDisplayHeight;
      if(y < 0)
	y=Scr.VyMax;
    }
  if(((val2 <= -100000)||(val2>= 100000))&&(y>Scr.VyMax))
    {
      y = 0;
      x += Scr.MyDisplayWidth;
      if(x > Scr.VxMax)
	x=0;
    }
  if(((val2 <= -100000)||(val2>= 100000))&&(y<0))
    {
      y = Scr.VyMax;
      x -= Scr.MyDisplayWidth;
      if(x < 0)
	x=Scr.VxMax;
    }
  MoveViewport(x,y,True);
}

void goto_page_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context,char *action, int *Module)
{
  int val1, val2, val1_unit,val2_unit,n,x,y;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if(n != 2)
    {
      fvwm_msg(ERR,"goto_page_func","GotoPage requires two arguments");
      return;
    }

  if((val1_unit != Scr.MyDisplayWidth)||
     (val2_unit != Scr.MyDisplayHeight))
    {
      fvwm_msg(ERR,"goto_page_func","GotoPage arguments should be unitless");
    }
  
  x=val1*Scr.MyDisplayWidth;
  y=val2*Scr.MyDisplayHeight;
  MoveViewport(x,y,True);
}




