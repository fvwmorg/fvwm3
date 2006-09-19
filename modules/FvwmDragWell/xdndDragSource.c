/* -*-c-*- */
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
#include "libs/fvwmlib.h"
#include "dragSource.h"
#include "cursorStuff.h"
#include "fvwmDragWell.h"
#define SQR(x) ((x)*(x))

extern int xdndCursorInit(Display *, Window);
extern XdndCursor xdndCursors[];
extern char dropData[];
extern XGlobals xg;

DragSource dsg;
XdndAtoms xdndAtoms;
void xdndSrcSendDrop(DragSource *, Window, Window, unsigned long);
int xdndSrcQueryDndAware (DragSource *ds, Window window, int *version,
			  Atom * typelist);


/* Determines if the cursor is in the rectangle .
 * Returns true if the cursor is in the rectangle.
 *  ds - the drag source, contains the rectangle info.
 *  rx,ry - the cursor in root coordinates */
int cursorInRect(DragSource *ds, unsigned short rx, unsigned short ry) {
  if ((rx > ds->rx) && (rx <ds->rx+ds->w)) {
    if ((ry > ds->ry) && (ry <ds->ry+ds->h))
      return 1;
  }
  return 0;
}



/* Initializes a XEvent client message
 *   xev - the XEvent to initialize
 *   dpy - the display
 *   win - the window
 *   messageType - the message type */
void xevClientInit(XEvent *xev, Display *dpy, Window win, Atom messageType) {
  memset (xev, 0, sizeof (XEvent));
  xev->xany.type = ClientMessage;
  xev->xany.display = dpy;
  xev->xclient.window = win;
  xev->xclient.message_type = messageType;
  xev->xclient.format = 32;
}


/* Sends an "enter" event.
 *   ds - the drag source.
 *   dstWin - the recipient of the event.
 *   srcWin - the drag source win
 *   typelist - the types the drag source supports, assumes that
 *              the list is NULL terminated. */
void xdndSrcSendEnter(DragSource *ds, Window dstWin, Window srcWin, Atom * typelist)
{
  XEvent xevent;
  int i;
  unsigned long nTypes=0,tmpUL=0;

  if (typelist != NULL) {
    while (typelist[nTypes++]!=0); /*get typelist length*/
  }

  xevClientInit(&xevent,ds->display,dstWin,ds->atomSel->xdndEnter);

  /* data.l[0] : XID of source window
     data.l[1] : Bit 0 is set if source supports more than three types.
		 High byte is protocol version(min of source,target).  Version
		 is set in window property of target.
     data.l[2,3,4] : First three types.  Unused set to none. */

  xevent.xclient.data.l[0] = srcWin;
  if (nTypes>3) /*this should never happen for fvwmQFS*/
    tmpUL = 0x1UL;
  xevent.xclient.data.l[1] = (tmpUL | ((ds->version & 0xFFUL) << 24));
  for (i = 0; i < XDND_MIN(nTypes,3) ; i++)
    xevent.xclient.data.l[i+2] = typelist[i];
  for (i = nTypes; i < 3 ; i++)
    xevent.xclient.data.l[i+2] = None;
  FSendEvent(ds->display, dstWin, 0, 0, &xevent);
}



/* Sends a position event
 *   ds - the drag source struct
 *   dstWin - the recipient of the event
 *   srcWin - the drag source window
 *   x,y - cursor position int root coords
 *   time - time
 *   action - the requested action */
void xdndSrcSendPosition(DragSource *ds, Window dstWin, Window srcWin, short x,
			 short y, unsigned long time, Atom action) {
  XEvent xevent;

  xevClientInit(&xevent,ds->display,dstWin,ds->atomSel->xdndPosition);

  /* data.l[0] : XID of source window
     data.l[1] : Reserved for future use.
     data.l[2] : Coordinates of mouse relative to root window(x<<16|y)
     data.l[3] : time stamp for retrieving data
     data.l[4] : action requested by user */
  xevent.xclient.data.l[0] = srcWin;
  xevent.xclient.data.l[2] = (x<<16) | y;
  xevent.xclient.data.l[3] = time;
  xevent.xclient.data.l[4] = action;
  FSendEvent(ds->display, dstWin, 0, 0, &xevent);
}



/* Handles the receipt of a status event
 *   ds - the drag source struct
 *   xev - the status event
 *   cx,cy - the mouse x,y coords, in root frame, used if the event
 *           is cached
 *   time - the time */
void xdndSrcReceiveStatus(DragSource *ds, XEvent *xev,unsigned short *cx,
			  unsigned short *cy, unsigned long time)
{
  Atom action;

  if (ds->dropTargWin != xev->xclient.data.l[0])
  {
    return; /*Got ClientMessage:XdndStatus from wrong client window*/
  }

  /*
   *data.l[0] : XID of target window
   *data.l[1] : Bit 0 set if target accepts drop
   *            Bit 1 set if target wants coordinates while in rectangle
   *data.l[2,3] : Coordinates of box. (x,y) relative to root.  Null box ok.
   *data.l[2] : (x<<16)|y
   *data.l[3] : (w<<16)|h
   *data.l[4] : action accepted by target.  Either should be one of sent
   *            actions, XdndActionCopy, XdndActionPrivate, or None if drop
   *             will not be accepted
   */

  /*Need to remember that we have received at least one status event*/
  ds->state = XDND_SETBIT(ds->state,XDND_STATUS_RECVD_CNT,XDND_GOTONE_OR_MORE);

  /* Can we drop on the target?*/
  if (xev->xclient.data.l[1] & 0x1)
    ds->state = XDND_SETBIT(ds->state,XDND_DROPPABLE,XDND_CAN_DROP);
  else
    ds->state = XDND_SETBIT(ds->state,XDND_DROPPABLE,XDND_CANT_DROP);

  /* How to send position information*/
  if (xev->xclient.data.l[1] & 0x2)
    ds->state = XDND_SETBIT(ds->state,XDND_SEND_POS,XDND_ALWAYS_SEND);
  else
    ds->state = XDND_SETBIT(ds->state,XDND_SEND_POS,XDND_RECTSEND);

  action = xev->xclient.data.l[4];

  ds->rx = (xev->xclient.data.l[2]&0xffff0000)>>16;
  ds->ry = xev->xclient.data.l[2]&0x0000ffff;
  ds->w = (xev->xclient.data.l[3]&0xffff0000)>>16;
  ds->h = xev->xclient.data.l[3]&0x0000ffff;

  if (XDND_GETBIT(ds->state,XDND_DROPPABLE) &&
      XDND_GETBIT(ds->state,XDND_BUTRELEASE_CACHE)) {
    /* button released, but waiting for status*/
    if (action != None)
      xdndSrcSendDrop(ds, ds->dropTargWin, ds->dragSrcWin, time);
  } else {
    if ((XDND_GETBIT(ds->state,XDND_POS_CACHE))&&
	(XDND_GETBIT(ds->state,XDND_WAIT_4_STATUS))) {
      /*only send position if ((always send motion)||(left box))*/
      ds->state = XDND_SETBIT(ds->state,XDND_WAIT_4_STATUS,XDND_NOTWAITING);
      if (XDND_GETBIT(ds->state,XDND_SEND_POS) ||
	  !cursorInRect(ds,ds->cachexRoot,ds->cacheyRoot)) {
	/* position cached*/
	ds->state = XDND_SETBIT(ds->state,XDND_WAIT_4_STATUS,XDND_WAITING);
	xdndSrcSendPosition(ds, ds->dropTargWin, ds->dragSrcWin, ds->cachexRoot,
			    ds->cacheyRoot, time, action);
      }
      ds->state = XDND_SETBIT(ds->state,XDND_POS_CACHE,XDND_NOT_CACHED);
    } else {
      /*Not cacheing position, do nothing*/
      ds->state = XDND_SETBIT(ds->state,XDND_WAIT_4_STATUS,XDND_NOTWAITING);
    }
  }
}




/* Sends a "leave" event
 *   ds - the drag source struct
 *   dstWin - destination(drop) window
 *   srcWin - the source window(drag src) */
void xdndSrcSendLeave(DragSource *ds, Window dstWin, Window srcWin) {
  XEvent xevent;
  xevClientInit(&xevent,ds->display,dstWin,ds->atomSel->xdndLeave);

  /* data.l[0] : XID of source window
     data.l[1] : Reserved for future use. */
  xevent.xclient.data.l[0] = srcWin;
  FSendEvent(ds->display, dstWin, 0, 0, &xevent);
}



/* Sends a "drop" event
 *   ds - the drag source struct
 *   dstWin - destination(drop) window
 *   srcWin - the source window(drag src)
 *   time - the time of last position */
void xdndSrcSendDrop(DragSource *ds, Window dstWin, Window srcWin, unsigned long time) {
  XEvent xevent;

  xevClientInit(&xevent,ds->display,dstWin,ds->atomSel->xdndDrop);

  /* data.l[0] : XID of source window
     data.l[1] : Reserved for future use.
     data.l[2] : Time stamp for retrieving data */
  xevent.xclient.data.l[0] = srcWin;
  xevent.xclient.data.l[2] = time;

  XSetSelectionOwner(xg.dpy,ds->atomSel->xdndSelection,
		     srcWin,CurrentTime);
  FSendEvent(ds->display, dstWin, 0, 0, &xevent);
}


/* Receive a finished event from the drop target
 *   ds - the drag source struct
 *   xev - the finish event */
void xdndSrcReceiveFinished(DragSource *ds, XEvent *xev) {
  /* data.l[0] : XID of target window
     data.l[1] : Reserved for future use. */
  if (xev->xclient.data.l[0]!=ds->dropTargWin) {
    return;
  }
  XDeleteProperty(ds->display,ds->dragSrcWin,ds->dropTargProperty);/*ds->dropTargProperty*/
  ds->state = 0;
}




/* Main drag loop, should be called after a button press event in the drag source.
 *   Returns the atom of the action exported.
 * ds - the drag source struct
 * srcWin - the drag source window
 * action - the action we are exporting
 * typelist - the types we support */

Atom xdndSrcDoDrag(DragSource *ds, Window srcWin, Atom action, Atom * typelist)
{
  XEvent xev;
  float x_mouse, y_mouse, radiusSqr;
  int cacheTime = 0;
  XdndCursor *cursorPtr;
  Window trackWindow, root_return, child_return;
  unsigned int mask_return;
  Bool retBool;
  int version = 0;
  Atom retAction = None;
  Atom target;
  short doneDrag = 0;


  /* User releases the mouse button without any motion->do nothing*/
  do {
    FNextEvent(ds->display,&xev);
    if (xev.type == ButtonRelease) {
      FSendEvent(ds->display, xev.xany.window, 0, ButtonReleaseMask, &xev);
      return None;
    }
  } while (xev.type != MotionNotify);

  /*We moved, wait until motion radius is larger than ds->halo*/
  x_mouse = (float) xev.xmotion.x_root;
  y_mouse = (float) xev.xmotion.y_root;
  if (!ds->dragHalo)
    ds->dragHalo = 4.0;
  for (;;) {
    FNextEvent(ds->display, &xev);
    if (xev.type == MotionNotify) {
      radiusSqr = SQR(x_mouse - xev.xmotion.x_root) +  \
	SQR(y_mouse - xev.xmotion.y_root);
      if (radiusSqr > SQR(ds->dragHalo))
	break;
      if (xev.type == ButtonRelease) {
	/*Button release: radius less than halo->do nothing*/
	FSendEvent(ds->display, xev.xany.window, 0, ButtonReleaseMask, &xev);
	return 0;/*What does this mean?*/
      }
    }
  }
  cursorPtr = &(ds->cursors[0]);
  /*We moved beyond the halo, now change the cursor.  Handles the
    typelist greater than three n = array_length (typelist); */
  if (XGrabPointer(ds->display, ds->rootWindow, False,
		   ButtonMotionMask | PointerMotionMask | ButtonPressMask
		   | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None,
		   cursorPtr->cursor, CurrentTime) != GrabSuccess) {

  }

  /*Main Drag loop.  trackWindow is the current window the pointer is over. Note
    that the actual client window of the top level widget of the application is
    *not* trackWindow, but is actually one of the children of trackWindow. */
  if (FQueryPointer(
	ds->display, XDefaultRootWindow(ds->display), &root_return,
	&child_return, &xev.xmotion.x_root, &xev.xmotion.y_root,
	&xev.xmotion.x, &xev.xmotion.y, &mask_return) == False)
  {
    /* pointer is on a different screen - that's okay here */
  }
  trackWindow = child_return;

  while (!doneDrag) {
    XAllowEvents(ds->display, SyncPointer, CurrentTime);
    FNextEvent(ds->display, &xev);
    switch (xev.type) {
    case MotionNotify:
      retBool = FQueryPointer(
	      ds->display,  XDefaultRootWindow(ds->display),
	      &root_return, &child_return, &xev.xmotion.x_root,
	      &xev.xmotion.y_root, &xev.xmotion.x,
	      &xev.xmotion.y, &mask_return);
      if (retBool == False)
      {
	      /* pointer is on a different screen */
	      xev.xmotion.x_root = 0;
	      xev.xmotion.y_root = 0;
	      xev.xmotion.x = 0;
	      xev.xmotion.y = 0;
      }

      if (trackWindow != child_return) {
	/*Cancel old drag if initiated.*/
	if (XDND_GETBIT(ds->state,XDND_IN_AWAREWIN)) {
	  xdndSrcSendLeave(ds, ds->dropTargWin, srcWin);
	  ds->state = XDND_SETBIT(ds->state,XDND_IN_AWAREWIN,XDND_NOTAWARE);
	}
	trackWindow = child_return;

	if (child_return != None) {
	  /* get new client window if not on root window.*/
	  ds->dropTargWin = fvwmlib_client_window(ds->display, child_return);

	  /*Setup new drag if XDndAware*/
	  if (xdndSrcQueryDndAware (ds, ds->dropTargWin, &version, NULL)) {
	    ds->state = XDND_SETBIT(ds->state,XDND_IN_AWAREWIN,XDND_AWARE);
	    xdndSrcSendEnter (ds, ds->dropTargWin, srcWin,typelist);
	    xdndSrcSendPosition (ds, ds->dropTargWin, srcWin,xev.xmotion.x_root,
			       xev.xmotion.y_root,xev.xmotion.time,action);
	    ds->state = XDND_SETBIT(ds->state,XDND_WAIT_4_STATUS,XDND_WAITING);
	  } else {
	    ds->state = XDND_SETBIT(ds->state,XDND_IN_AWAREWIN,XDND_NOTAWARE);
	  }
	}
      } else { /*haven't changed windows*/

	if (XDND_GETBIT(ds->state,XDND_IN_AWAREWIN))
	{
	  /*in an XdndAware window*/
	  /*Cache the position, and send when status is received*/
	  ds->cachexRoot = xev.xmotion.x_root;
	  ds->cacheyRoot = xev.xmotion.y_root;
	  /*Timeout after no status*/
	  /*if ()&&(XDND_GETBIT(ds->state,XDND_WAIT_4_STATUS)) */
	  if (XDND_GETBIT(ds->state,XDND_WAIT_4_STATUS)) {
	    /*Waiting for a status event.*/
	    cacheTime = xev.xmotion.time;
	    ds->state = XDND_SETBIT(ds->state,XDND_POS_CACHE,XDND_CACHED);
	  } else {
	    /*Not waiting for a status event*/
	    if (XDND_GETBIT(ds->state,XDND_SEND_POS) ||
		!cursorInRect(ds,xev.xmotion.x_root,xev.xmotion.y_root)) {
	      /* if left the box or always send position, then send the
	       * position */
	      xdndSrcSendPosition(
		  ds, ds->dropTargWin, srcWin,xev.xmotion.x_root,
		  xev.xmotion.y_root,xev.xmotion.time,action);
	      ds->state =
		  XDND_SETBIT(ds->state,XDND_WAIT_4_STATUS,XDND_WAITING);
	    }
	  }
	}
      }
      break;
    case ButtonRelease:
      if (XDND_GETBIT(ds->state,XDND_IN_AWAREWIN)) {
	/*We're in an XdndAware win...*/
	if (! XDND_GETBIT(ds->state,XDND_STATUS_RECVD_CNT)) {
	  /* if never got XdndStatus, send a leave.*/
	  xdndSrcSendLeave(ds, ds->dropTargWin, srcWin);
	} else {
	  /*I've received at least one status event...*/
	  if (XDND_GETBIT(ds->state,XDND_WAIT_4_STATUS)) {
	    /*Waiting for status, cache the button release*/
	    ds->cachexRoot = xev.xmotion.x_root;
	    ds->cacheyRoot = xev.xmotion.y_root;
	    cacheTime = xev.xmotion.time;
	    ds->state =
		XDND_SETBIT(ds->state,XDND_BUTRELEASE_CACHE,XDND_BUTCACHED);
	  } else {
	    /*Not waiting for a status, send the drop event*/
	    if (XDND_GETBIT(ds->state,XDND_DROPPABLE)) {
	      xdndSrcSendDrop(ds, ds->dropTargWin, srcWin, CurrentTime);
	    } else {
	      xdndSrcSendLeave(ds, ds->dropTargWin, srcWin);
	      retAction = None;
	    }
	  }
	}
      } else {
	doneDrag = 1;
      }
      XUngrabPointer(ds->display,CurrentTime);
      break;
    case SelectionRequest:
      /* to review how this works, we send the drop target a "drop"
	 event, the target sends a selection request event for the
	 data via XConvertSelection, we receive the request event,
	 prepare the data and send a notify, and the target then gets the data.*/

      if (xev.xselectionrequest.selection==ds->atomSel->xdndSelection) {
	if (xev.xselectionrequest.requestor == ds->dropTargWin) {
	  target = xev.xselectionrequest.target;
	  ds->dropTargProperty = xev.xselectionrequest.property;
	  XChangeProperty(
		  ds->display, ds->dragSrcWin, ds->dropTargProperty, target, 8,
		  PropModeReplace, (unsigned char*)dropData, strlen(dropData)+1);
	  memset(&xev, 0, sizeof (XEvent));

	  xev.xany.type = SelectionNotify;
	  xev.xany.display = ds->display;
	  xev.xselection.requestor = ds->dropTargWin;
	  xev.xselection.selection = ds->atomSel->xdndSelection;
	  xev.xselection.property = ds->dropTargProperty;
	  xev.xselection.target = target;
	  FSendEvent(ds->display, ds->dropTargWin, 0, 0, &xev);
	}
      }
      break;
    case ClientMessage:
      if (xev.xclient.message_type==ds->atomSel->xdndStatus) {
	xdndSrcReceiveStatus(ds,&xev,0,0,cacheTime);
      } else if (xev.xclient.message_type==ds->atomSel->xdndFinished) {
	xdndSrcReceiveFinished(ds,&xev);
	doneDrag = 1;
      } else {
	fprintf(stderr,"FvwmQFS:xdndDrag:Unknown client message in dragSource\n");
      }
      break;

    default:
      break;
    }
  }

  XUngrabPointer(ds->display,CurrentTime);

  return retAction;
}


/* error handler for BadWindow errors.  Does nothing at the moment...
 * dpy - the display
 * errEv - the error Event */
int xdndErrorHandler(Display *dpy, XErrorEvent *errEv)
{
  if (errEv->error_code == BadWindow)
  {
    /* Well, the specification for Xdnd states that we should handle
     * "BadWindow" errors with an error handler.  I'm not real sure how to go
     * beyond this point. One option is to use "goto".  Another is to make
     * doneDrag in the xdndSrcDoDrag a global, and set it to 1 here, but
     * that is more dangerous.  For now, we will just exit */
    exit(1);
    /*goto gotoLabelDone;*/
  }
  return 0;
}


/* xdndInit - initializes the drag environment.  Should only be called once.
 * display - the display
 * rw - the root window */
void xdndInit(Display *display, Window rw) {
  xdndCursorInit(display, rw); /*initializes the cursors*/
  /*Initializes the atoms.  Each DropTarget,DragSource points to this
   *  structure.  Saves space.*/
  xdndAtoms.xdndAware =  XInternAtom(display, "XdndAware", False);;
  xdndAtoms.xdndEnter =  XInternAtom(display, "XdndEnter", False);
  xdndAtoms.xdndLeave =  XInternAtom(display, "XdndLeave", False);
  xdndAtoms.xdndStatus =  XInternAtom(display, "XdndStatus", False);
  xdndAtoms.xdndSelection = XInternAtom(display, "XdndSelection", False);
  xdndAtoms.xdndPosition = XInternAtom(display, "XdndPosition", False);
  xdndAtoms.xdndDrop = XInternAtom(display, "XdndDrop", False);
  xdndAtoms.xdndFinished = XInternAtom(display, "XdndFinished", False);
  xdndAtoms.xdndActionCopy = XInternAtom(display, "XdndActionCopy", False);
  xdndAtoms.xdndActionMove = XInternAtom(display, "XdndActionMove", False);
  xdndAtoms.xdndActionLink = XInternAtom(display, "XdndActionLink", False);
  xdndAtoms.xdndActionAsk = XInternAtom(display, "XdndActionAsk", False);
  xdndAtoms.xdndActionPrivate = XInternAtom(display, "XdndActionPrivate", False);
  xdndAtoms.xdndTypeList = XInternAtom(display, "XdndTypeList", False);
  xdndAtoms.xdndActionList = XInternAtom(display, "XdndActionList", False);
  xdndAtoms.xdndActionDescription =
    XInternAtom(display, "XdndActionDescription", False);
  XSetErrorHandler(xdndErrorHandler);
}



/* dragSrcInit - initializes a drag source
 * ds - the dragSource
 * dpy - the display
 * root - the root window
 * client - the drag source client window */
void dragSrcInit(DragSource *ds,Display *dpy,Window root,Window client) {
  ds->display = dpy;
  ds->rootWindow = root;
  ds->dragSrcWin = client;
  ds->cursors = &(xdndCursors[0]);
  ds->atomSel = &xdndAtoms;
  ds->state = 0;
}



/* xdndSrcQueryDndAware - queries whether the drop window is XdndAware.  Returns 0
 *   if not aware, and returns one if aware
 * ds - the drag source struct
 * window - the drop window we want to query
 * version - what version of Xdnd the drop window supports
 * typelist - not used at this  point, could be used to compare the source type list
 *   with the drop target list if the target puts the supported types in it property list
*/
int xdndSrcQueryDndAware (DragSource *ds, Window window, int *version,
			  Atom * typelist)
{
  Atom actual;
  int format;
  unsigned long count, remaining;
  unsigned char *data = 0;
  Atom *types;
  int result = 1;
  *version = 0;

  XGetWindowProperty(
	  ds->display, window, ds->atomSel->xdndAware, 0L, 0x8000000L, False,
	  XA_ATOM, &actual, &format, &count, &remaining, &data);
  if (actual != XA_ATOM || format != 32 || count == 0 || !data) {
    /* not supported */
    if (data)
      XFree (data);
    return 0;
  }
  types = (Atom *) data;
  *version = ds->version < types[0] ? ds->version : types[0];   /* minimum */
  XFree (data);
  return result;
}
