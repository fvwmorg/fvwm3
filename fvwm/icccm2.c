#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "screen.h"
#include "parse.h"
#include "module.h"

#include <X11/Xproto.h>
#include <X11/Xatom.h>

Window manager_win = None;
Time managing_since;

Atom _XA_WIN_SX;
Atom _XA_MANAGER;
Atom _XA_ATOM_PAIR;
Atom _XA_WM_COLORMAP_NOTIFY;
#define _XA_TARGETS   conversion_targets[0]
#define _XA_MULTIPLE  conversion_targets[1]
#define _XA_TIMESTAMP conversion_targets[2]
#define _XA_VERSION   conversion_targets[3]
#define MAX_TARGETS                      4

Atom conversion_targets[MAX_TARGETS];

long icccm_version[] = { 2, 0 };

void 
SetupICCCM2 ()
{
  Window running_wm_win;
  XSetWindowAttributes attr;
  XEvent xev;
  XClientMessageEvent ev;
  char win_sx[20];

  sprintf (win_sx, "WIN_S%ld", Scr.screen);
  _XA_WIN_SX =    XInternAtom (dpy, win_sx, False);
  _XA_MANAGER =   XInternAtom (dpy, "MANAGER", False);
  _XA_ATOM_PAIR = XInternAtom (dpy, "ATOM_PAIR", False);
  _XA_TARGETS =   XInternAtom (dpy, "TARGETS", False);
  _XA_MULTIPLE =  XInternAtom (dpy, "MULTIPLE", False);
  _XA_TIMESTAMP = XInternAtom (dpy, "TIMESTAMP", False);
  _XA_VERSION =   XInternAtom (dpy, "VERSION", False);
  _XA_WM_COLORMAP_NOTIFY = XInternAtom (dpy, "WM_COLORMAP_NOTIFY", False);

  /* Check for a running ICCCM 2.0 compliant WM */
  running_wm_win = XGetSelectionOwner (dpy, _XA_WIN_SX);
  if (running_wm_win != None) {
    DBUG("SetupICCCM2", "another ICCCM 2.0 compliant WM is running");
    
    /* We need to know when the old manager is gone.
       Thus we wait until it destroys running_wm_win. */
    attr.event_mask = StructureNotifyMask;
    XChangeWindowAttributes (dpy, running_wm_win, CWEventMask, &attr);
  } 

  attr.event_mask = PropertyChangeMask;
  manager_win = XCreateWindow (dpy, Scr.Root, 0, 0, 1, 1, 0,
			       CopyFromParent, CopyFromParent, 
			       CopyFromParent, CWEventMask, &attr);

  /* We are not yet in the event loop, thus lastTimestamp will not
     be ready. Have to get a timestamp manually by provoking a 
     PropertyNotify. */ 
  XChangeProperty (dpy, manager_win, XA_WM_CLASS, XA_STRING, 8,
		   PropModeAppend, NULL, 0);
  XWindowEvent (dpy, manager_win, PropertyChangeMask, &xev);
  attr.event_mask = 0L;
  XChangeWindowAttributes (dpy, manager_win, CWEventMask, &attr);

  managing_since = xev.xproperty.time;

  XSetSelectionOwner (dpy, _XA_WIN_SX, manager_win, managing_since);
  if (XGetSelectionOwner (dpy, _XA_WIN_SX) != manager_win) {
    fvwm_msg (ERR, "SetupICCCM2", "failed to acquire selection ownership");
    exit (1);
  } 
 
  /* Announce ourself as the new wm */
  ev.type = ClientMessage;
  ev.window = Scr.Root;
  ev.message_type = _XA_MANAGER;
  ev.format = 32;
  ev.data.l[0] = managing_since;
  ev.data.l[1] = _XA_WIN_SX;
  XSendEvent (dpy, Scr.Root, False, StructureNotifyMask, (XEvent*)&ev);
 
  if (running_wm_win != None) {
    /* Wait for the old wm to finish. */
    /* FIXME: need a timeout here. */
    DBUG("SetupICCCM2", "waiting for WM to give up");
    do {
      XWindowEvent (dpy, running_wm_win, StructureNotifyMask, &xev);
    } while (xev.type != DestroyNotify);
  }
}

/* We must make sure that we have released SubstructureRedirect
   before we destroy manager_win, so that another wm can start
   successfully. */
void 
CloseICCCM2 ()
{
  DBUG("CloseICCCM2", "good luck, new wm");
  XSelectInput(dpy, Scr.Root, 0 );
  XSync(dpy, 0);
  XDestroyWindow (dpy, manager_win);
}

/* FIXME: property change actually succeeded */
static Bool
convertProperty (Window w, Atom target, Atom property)
{
  if (target == _XA_TARGETS)
    XChangeProperty (dpy, w, property, 
		     XA_ATOM, 32, PropModeReplace, 
		     (unsigned char *)conversion_targets, MAX_TARGETS); 
  else if (target == _XA_TIMESTAMP)
    XChangeProperty (dpy, w, property, 
		     XA_INTEGER, 32, PropModeReplace, 
		     (unsigned char *)&managing_since, 1); 
  else if (target == _XA_VERSION)
    XChangeProperty (dpy, w, property, 
		     XA_INTEGER, 32, PropModeReplace, 
		     (unsigned char *)icccm_version, 2); 
  else return False;

  /* FIXME: This is ugly. We should rather select for
     PropertyNotify on the window, return to the main loop,
     and send the SelectionNotify once we are sure the property 
     has arrived. Problem: this needs a list of pending 
     SelectionNotifys. */
  XSync (dpy, 0);
  return True;
}

void 
HandleSelectionRequest ()
{
  Atom type, *adata;
  int i, format;
  unsigned long num, rest;
  unsigned char *data;
  XSelectionRequestEvent ev = Event.xselectionrequest;
  XSelectionEvent reply;

  reply.type = SelectionNotify;
  reply.display = dpy;
  reply.requestor = ev.requestor;
  reply.selection = ev.selection;
  reply.target = ev.target;
  reply.property = None;
  reply.time = ev.time;

  if (ev.target == _XA_MULTIPLE) {
    if (ev.property != None) {
      XGetWindowProperty (dpy, ev.requestor, ev.property, 0, 256, False,
			  _XA_ATOM_PAIR, &type, &format, &num, &rest, &data);
      /* FIXME: to be 100% correct, should deal with rest > 0,
	 but since we have 4 possible targets, we will hardly ever
	 meet multiple requests with a length > 8 */
      adata = (Atom*)data;
      for (i = 0; i < num; i += 2) {
	if (!convertProperty(ev.requestor, adata[i], adata[i+1])) 
	  adata[i+1] = None;
      }
      XChangeProperty (dpy, ev.requestor, ev.property, _XA_ATOM_PAIR, 
		       32, PropModeReplace, data, num);
      XFree (data);
    }
  } else {
    if (ev.property == None) ev.property = ev.target;
    if (convertProperty (ev.requestor, ev.target, ev.property)) 
      reply.property = ev.property;
  }

  XSync (dpy, 0);
  XSendEvent (dpy, ev.requestor, False, 0L, (XEvent*)&reply);    
}

/* If another wm is requesting ownership of the selection,
   we receive a SelectionClear event. In that case, we have to 
   release all resources and destroy manager_win. Done() calls
   CloseICCCM2() after undecorating all windows. */
void
HandleSelectionClear ()
{
  fvwm_msg(INFO, "HandleSelectionClear", "I lost my selection!");
  Done(0, NULL);
}

