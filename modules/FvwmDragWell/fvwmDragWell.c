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

/*
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "libs/ftime.h"
#include <sys/stat.h>


#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/Module.h"
#include "libs/Colorset.h"
#include "libs/PictureBase.h"
#include "libs/fvwmsignal.h"
#include "libs/FScreen.h"
#include "libs/FRenderInit.h"

#include "fvwmDragWell.h"
#include "dragSource.h"

#define hostnameStrLen 64

extern DragSource dsg;
extern void xdndInit(Display *, Window);
extern void dragSourceInit(DragSource *,Display *,Window,Window);

void XStartup(char *appName);
void veryLongLoop();
char *shuffleDown(char **,short *,int,int);
int shrinkList(char **,short *,int,int);
int dilateList(char **,short *,int,int);
void changeWindowName(Window, char *);
void readHistory();
void writeHistory();
int dummy(DragWellButton *);
void createRootMenu();
void flushHistory();
void getReliefColors();
void createBackground();
void createWindow();
void parseOptions(void);
void parseFvwmMessage(char *msg);
int myXNextEvent(XEvent *event, char *fvwmMessage);
Atom xdndSrcDoDrag(DragSource *ds, Window srcWin, Atom action,
		   Atom * typelist);
void dragSrcInit(DragSource *ds,Display *dpy,Window root,Window client);
static RETSIGTYPE TerminateHandler(int);

int fd[2];      /*used for reading .fvwm2rc options*/
	       /*fd[0] module2fvwm*/
	       /*fd[1] fvwm2module*/
XGlobals xg;    /*X related globals*/
DragWellMenuGlobals mg; /*menu related globals */
DragWellButton dragBut; /*not really used as a button but rather a drag well*/
char dropData[MAX_PATH_LEN];    /*data used for the drop*/
Bool Swallowed = False;

/* Don't know what this does yet...*/
void deadPipe(int nonsense)
{
  /*writeHistory();*/
  exit(0);
}




#ifdef DRAW_KILLBUTTON
/* drawX - draws an "x" on a button
 *  Dimension (w-8,h-8), roughly centered
 *  Arguments:
 *   but - the button to draw upon
 */
int drawX(DragWellButton *but)
{
  /*upper left to bottom right*/
  XDrawLine(xg.dpy, but->win, xg.antiReliefGC, but->wx+4, but->wy+4,
	    but->wx+but->w-4,but->wy+but->h-4);
  /*bottom left to upper right*/
  XDrawLine(xg.dpy, but->win, xg.antiReliefGC, but->wx+4, but->wy+but->h-5,
	    but->wx+but->w-5,but->wy+4);
}
#endif



int main(int argc, char **argv)
{
  Window app_win;

  fd[MODULE_TO_FVWM] = atoi (argv[1]); /*used to get info from .fvwm2rc*/
  fd[FVWM_TO_MODULE] = atoi (argv[2]);

  sscanf (argv[4], "%x", (unsigned int *)&app_win);

  /*dsg.mfsErr  = fopen("/tmp/mfsErr","w");*/

  XStartup(argv[0]); /*initializes variables*/
  veryLongLoop(); /*event loop*/

  return 0;
}




/* initDragWellButton - initializes a DragWellButton
 *   Doesn't return anything useful
 * Arguments:
 *   but - the button to be initialized
 *   win - the window the button is drawn on
 *   x,y - the upper left corner of the button, in (Window win) coords.
 *   drawFunc - a function pointer for drawing something on the button
 */
void initDragWellButton(DragWellButton *but,Window win, int x, int y, int w,
			int h, int (*drawFunc)(DragWellButton *))
{
  but->win = win;
  but->wx = x;
  but->wy = y;
  but->w = w;
  but->h = h;
  but->state = DRAGWELL_BUTTON_NOT_PUSHED;
  but->drawButFace = drawFunc;
}



/* drawDragWellButton - draws a DragWellButton
 *   Doesn't return anything meaningful
 * Arguments:
 *   but - the button to be drawn
 */
void drawDragWellButton(DragWellButton *but)
{
  (*(but->drawButFace))(but); /*draws face of button*/
  if (but->state==DRAGWELL_BUTTON_NOT_PUSHED)
    RelieveRectangle(xg.dpy, but->win, but->wx, but->wy, but->w - 1, but->h - 1,
		     xg.hiliteGC, xg.shadowGC, 2);
  else
    RelieveRectangle(xg.dpy, but->win, but->wx, but->wy, but->w - 1, but->h - 1,
		     xg.shadowGC, xg.hiliteGC, 2);
}



/* mouseInButton - returns true if the pointer is in a DragWellButton
 *   returns TRUE(1) if in button and FALSE(0) otherwise.
 * Arguments:
 *   but - the button to be drawn
 *   x,y - the pointer coords in (DragWellButton->win) coords
 */
int mouseInButton(DragWellButton *but, int x, int y) {
  if ((but->wx <=x)&&(x<=(but->wx+but->w)))
    if ((but->wy <=y)&&(y<=(but->wy+but->h)))
      return 1;
  return 0;
}



/*dragwellAnimate - does an animation in the dragwell
 *  Does not return anything useful */
void dragwellAnimate() {
  int i;
  int sleepTime;
  /*only one animation type for now*/
  switch (xg.animationType) {
  case DEFAULT_ANIMATION:
    /*animate*/
    if (dragBut.state == DRAGWELL_BUTTON_PUSHED)
      XClearArea(xg.dpy,xg.win,xg.dbx+2,xg.dby+2,xg.dbw-2,xg.dbh-2,False);
    else
      XFillRectangle(
	xg.dpy,xg.win,xg.buttonGC,xg.dbx+2,xg.dby+2,xg.dbw-2,xg.dbh-2);
    /*This way, the animation time is constant regardless of size of
     * box(assuming drawing is instantaneous...*/
    sleepTime = TOTAL_ANIMATION_TIME/(xg.dbh-3);
    for (i=xg.dby+xg.dbh-2;i>xg.dby;i--) {
      if (dragBut.state == DRAGWELL_BUTTON_PUSHED)
	XFillRectangle(
	  xg.dpy,xg.win,xg.buttonGC,xg.dbx+2,i,xg.dbw-3,xg.dby+xg.dbh-1-i);
      else
	XClearArea(xg.dpy,xg.win,xg.dbx+2,i,xg.dbw-3,xg.dby+xg.dbh-1-i,False);
      XFlush(xg.dpy);
      usleep(sleepTime);
    }
    drawDragWellButton(&dragBut);
    break;
  }
}


/* Main event loop*/
void veryLongLoop()
{
  int x,y;
  XEvent xev;
  char fvwmMessage[512];
  int eventType;

  initDragWellButton(&dragBut,xg.win,xg.dbx,xg.dby,xg.dbw,xg.dbh,dummy);
  dragBut.state = DRAGWELL_BUTTON_NOT_PUSHED;
  drawDragWellButton(&dragBut);

  while (!isTerminated)
  {
    /*get next event*/
    if ((eventType=myXNextEvent (&xev,fvwmMessage))==FOUND_XEVENT)
    {
	/* we have a X event*/
	switch(xev.type) {
	case ButtonPress:
	  x = xev.xbutton.x;
	  y = xev.xbutton.y;
	  if (mg.dragState == DRAGWELLMENU_HAVE_DRAG_ITEM) {
	    if (mouseInButton(&dragBut,x,y)) {
	      if (mg.dragDataType==DRAG_DATA_TYPE_PATH)
		mg.typelist[0] = mg.textUriAtom;
	      else
		mg.typelist[0] = XInternAtom(xg.dpy,mg.dragTypeStr,FALSE);
	      xdndSrcDoDrag(&dsg,xg.win,mg.action,mg.typelist);
	    }
	  }
	  break;
	case ClientMessage:
	  /*close window option */
	  if ((xev.xclient.format==32) &&
	      (xev.xclient.data.l[0]==xg.wmAtomDelWin))
	    deadPipe(1);
	      break;
	case Expose:
	  drawDragWellButton(&dragBut); /*draws the drag icon*/
	  break;
	case ConfigureNotify:
	  if (xg.colorset >= 0 && Colorset[xg.colorset].pixmap == ParentRelative)
	    XClearArea(xg.dpy, xg.win, 0,0,0,0, True);
	  break;
	}
    } else if (eventType==FOUND_FVWM_MESSAGE) { /* we have an fvwm message */
      /* two cases, one where item is a file, other where we have a subdir */
      parseFvwmMessage(fvwmMessage);
      dragwellAnimate();
    } else {
      /* no window*/
      /* deadPipe(0);*/
    }
  }
}



/* parseFvwmMessage - decodes a SendToModule command.
 * Arguments:
 *   msg - the fvwm message
 * sets dragBut.state */

static char *dragopts[] = {
  "DragType",
  "DragData"
};

void parseFvwmMessage(char *msg)
{
  char *dragtype = NULL, *dragData = NULL;
  char *optString, *option, *args;
  char hostname[hostnameStrLen];

  /* parse line into comma separated pieces, set dragtype and dragdata */
  while (msg && *msg) {
    msg = GetQuotedString(msg, &optString, ",", NULL, NULL, NULL);
    if (!optString)
      break;
    args = GetNextToken(optString, &option);
    if (!option) {
      free(optString);
      break;
    }
    switch(GetTokenIndex(option, dragopts, 0, NULL))
    {
      case 0:
	CopyString(&dragtype, args);
	break;
      case 1:
	CopyString(&dragData, args);
	break;
    }
    if (option) {
      free(option);
      option = NULL;
    }
    free(optString);
    optString = NULL;
  }

  if (dragtype != NULL) {
    strcpy(mg.dragTypeStr,dragtype);
    mg.dragDataType = DRAG_DATA_TYPE_NONPATH;
  } else {
    mg.dragDataType = DRAG_DATA_TYPE_PATH;
    strcpy(mg.dragTypeStr,"text/uri-list");
  }

  if (dragData == NULL || *dragData == 0) {
    mg.dragState = DRAGWELLMENU_NO_DRAG_ITEM;
    dragBut.state = DRAGWELL_BUTTON_NOT_PUSHED;
  } else {
    mg.dragState = DRAGWELLMENU_HAVE_DRAG_ITEM;
    strcpy(mg.dragData, dragData);
    dragBut.state = DRAGWELL_BUTTON_PUSHED;
  }

  /*setup a drag*/
  if (mg.dragState == DRAGWELLMENU_HAVE_DRAG_ITEM) {
    if (mg.dragDataType == DRAG_DATA_TYPE_PATH) {
      strcpy(dropData, "file://");
      gethostname(hostname, hostnameStrLen);
      strcat(dropData, hostname);
      strcat(dropData, mg.dragData);
    } else {
      strcpy(dropData, mg.dragData);
    }
  }
}


/*
 * TerminateHandler - signal handler to make FvwmIconBox exit cleanly
 */
static RETSIGTYPE
TerminateHandler(int sig)
{
  /*
   * This function might not return - it could "long-jump"
   * right out, so we need to do everything we need to do
   * BEFORE we call it ...
   */
  fvwmSetTerminate(sig);
}




/* XStartup - sets up the X and menu state.  Return value is meaningless.
 * Arguments:
 *   appName - the name of the application.
 */
void XStartup(char *appName)
{
  char *appNameStart;

  /*setup exit stuff*/
#ifdef HAVE_SIGACTION
  struct sigaction  sigact;

#ifdef SA_INTERRUPT
  sigact.sa_flags = SA_INTERRUPT;
#else
  sigact.sa_flags = 0;
#endif
  sigemptyset(&sigact.sa_mask);
  sigaddset(&sigact.sa_mask, SIGPIPE);
  sigaddset(&sigact.sa_mask, SIGTERM);
  sigaddset(&sigact.sa_mask, SIGINT);
  sigact.sa_handler = TerminateHandler;

  sigaction(SIGPIPE, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGINT,  &sigact, NULL);
#else
  /*
   * No sigaction, so fall back onto standard and unreliable signals
   */
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGPIPE) | sigmask(SIGTERM) | sigmask(SIGINT) );
#endif
  signal(SIGPIPE, TerminateHandler);
  signal(SIGTERM, TerminateHandler);
  signal(SIGINT, TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGTERM, 1);
  siginterrupt(SIGINT, 1);
#endif
#endif

  memset(&xg, 0, sizeof(xg));
  if (!(xg.dpy = XOpenDisplay (NULL))) {
    fprintf (stderr, "%s: can't open display %s",xg.appName,
	     XDisplayName (NULL));
     exit (1);
  }

  /*xg.log = fopen("/home/straub/Work/C++/Xdnd/fvwm-snap-19990906/modules/FvwmDragWell/qfs.log","w");*/
  /*Name the starting window name to the invocation name*/
  appNameStart = strrchr(appName,'/')+1;  /*remove the path from the name*/
  if (appNameStart == NULL) {
    appNameStart = appName;
  }

  xg.appName = (char *) malloc(sizeof(char) * (strlen(appNameStart)+1));
  strcpy(xg.appName,appNameStart); /*save the name of the application*/
  PictureInitCMap(xg.dpy);
  FScreenInit(xg.dpy);
  AllocColorset(0);
  FShapeInit(xg.dpy);
  FRenderInit(xg.dpy);

  /*get X stuff*/
  xg.xfd = XConnectionNumber(xg.dpy);
  xg.fdWidth = GetFdWidth();

  xg.screen = DefaultScreen(xg.dpy);
  xg.root = RootWindow(xg.dpy,xg.screen);
  xg.dpyDepth = DefaultDepth(xg.dpy, xg.screen);
  xg.dpyHeight = DisplayHeight(xg.dpy,xg.screen);
  xg.dpyWidth = DisplayWidth(xg.dpy,xg.screen);
  xg.dbx = 9;
  xg.dby = 9;
  xg.dbw = 30;
  xg.dbh = 30;
  xg.w = 48;
  xg.h = 48;
  /*Setup defaults*/
  mg.exportAsURL = True;
  mg.fontname = NULL;
  xg.foreColorStr = NULL;
  xg.backColorStr = NULL;
  xg.shadowColorStr = NULL;
  xg.hiliteColorStr = NULL;
  xg.colorset = -1; /*no colorset*/
  parseOptions(); /*overide defaults from .fvwm2rc*/
  if (xg.foreColorStr==NULL)
    CopyString(&xg.foreColorStr,"grey60");
  if (xg.backColorStr==NULL)
    CopyString(&xg.backColorStr,"black");
  if (xg.shadowColorStr==NULL)
      CopyString(&xg.shadowColorStr,"grey20");
  if (xg.hiliteColorStr==NULL)
    CopyString(&xg.hiliteColorStr,"grey90");
  getReliefColors();
  createWindow();
  createBackground();

  mg.dragState = DRAGWELLMENU_NO_DRAG_ITEM;
  /*more initialization menu stuff, using font info*/
  /*mg.fontHeight = mg.font->max_bounds.ascent + mg.font->max_bounds.descent;*/

  xdndInit(xg.dpy, xg.root);
  dragSrcInit(&dsg,xg.dpy,xg.root,xg.win);

  mg.action = dsg.atomSel->xdndActionMove;
  mg.textUriAtom = XInternAtom(xg.dpy,"text/uri-list",FALSE);
  mg.typelist = (Atom *) malloc(4*sizeof(Atom));
  mg.typelist[1] = None;
  mg.typelist[2] = None;
  mg.typelist[3] = 0;
  xg.animationType = DEFAULT_ANIMATION;

  /* tell fvwm we're running */
  SendFinishedStartupNotification(fd);
}



/*getReliefColors - sets up the colors for the dragwell
 *Arguments:*/
void getReliefColors() {
  if(xg.dpyDepth < 2) {
    /*for mono display, not well implemented...*/
    xg.fore = GetColor("white");
    xg.back = GetColor("black");
    xg.hilite = xg.fore;
    xg.shadow = xg.back;
  } else if (xg.colorset >=0) { /*using a colorset*/
    xg.hilite = Colorset[xg.colorset].hilite;
    xg.shadow  = Colorset[xg.colorset].shadow;
    xg.fore = Colorset[xg.colorset].fg;
    xg.back = Colorset[xg.colorset].bg;
  } else { /*no colorset*/
    xg.fore = GetColor(xg.foreColorStr);
    xg.back = GetColor(xg.backColorStr);
    xg.shadow = GetColor(xg.shadowColorStr);
    xg.hilite = GetColor(xg.hiliteColorStr);
  }
}



/* createBackground - sets the background of the dragwell window
 *  Arguments:*/
void createBackground() {
  unsigned long gcm;
  XGCValues gcv;
  /*Graphic context stuff*/
  gcm = GCForeground|GCBackground|GCSubwindowMode;
  gcv.subwindow_mode = IncludeInferiors;

  gcv.foreground = xg.hilite;
  gcv.background = xg.back;
  xg.hiliteGC = fvwmlib_XCreateGC(xg.dpy, xg.win, gcm, &gcv);

  gcv.foreground = xg.shadow;
  gcv.background = xg.back;
  xg.shadowGC = fvwmlib_XCreateGC(xg.dpy, xg.win, gcm, &gcv);

  gcv.foreground = xg.fore;
  gcv.background = xg.back;
  xg.buttonGC = fvwmlib_XCreateGC(xg.dpy, xg.win, gcm, &gcv);

  if (xg.colorset >=0) /*colorset*/
    SetWindowBackground(xg.dpy, xg.win, xg.w, xg.h, &Colorset[xg.colorset],
      Pdepth, xg.buttonGC, False);
  else /*just use background pixel*/
    XSetWindowBackground(xg.dpy, xg.win, xg.back);
}



/* parseOptions - loads in options from the .fvwm2rc file.
 * Arguments: */
void parseOptions(void)
{
  char *token,*next,*tline=NULL,*tline2,*resource;
  char *arg1,*arg2;

  /* loop that parses through the .fvwm2rc line by line.*/
  for (GetConfigLine(fd,&tline); tline != NULL; GetConfigLine(fd,&tline)) {
    tline2 = GetModuleResource(tline, &resource, xg.appName);
    if (!resource) {/*not a xg.appName resource(*FvwmDragWell) resource*/
      token = PeekToken(tline, &next);
      if (StrEquals(token, "Colorset")) {
	LoadColorset(next);
      }
      else if (StrEquals(token, XINERAMA_CONFIG_STRING)) {
	FScreenConfigureModule(next);
      }
      continue;
    }
    tline2 = GetNextToken(tline2, &arg1);
    if (!arg1)
    {
      arg1 = (char *)safemalloc(1);
      arg1[0] = 0;
    }
    tline2 = GetNextToken(tline2, &arg2);
    if (!arg2)
    {
      arg2 = (char *)safemalloc(1);
      arg2[0] = 0;
    }


    if (StrEquals(resource, "ExportAsURL")) {
      if (StrEquals(arg1,"True"))
	mg.exportAsURL = True;
      else
	mg.exportAsURL = False;
    } else if (StrEquals(resource, "Font")) {
      CopyString(&mg.fontname,arg1);
    } else if (StrEquals(resource, "Fore")) {
      CopyString(&xg.foreColorStr,arg1);
    } else if (StrEquals(resource, "Back")) {
      CopyString(&xg.backColorStr,arg1);
    } else if (StrEquals(resource, "Shadow")) {
      CopyString(&xg.shadowColorStr,arg1);
    } else if (StrEquals(resource, "Hilite")) {
      CopyString(&xg.hiliteColorStr,arg1);
    } else if (StrEquals(resource, "Colorset")) {
      sscanf(arg1, "%d", &xg.colorset);
      AllocColorset(xg.colorset);
    } else if (StrEquals(resource, "Geometry")) {
      int g_x,g_y,flags;
      unsigned width,height;
      xg.w = 48;
      xg.h = 48;
      xg.x = 0;
      xg.y = 0;
      xg.xneg = 0;
      xg.yneg = 0;
      xg.usposition = 0;
      flags = FScreenParseGeometry(arg1,&g_x,&g_y,&width,&height);
      if (flags & WidthValue) {
	xg.w = width;
      }
      if (flags & HeightValue) {
	xg.h= height;
      }
      if (flags & XValue) {
	xg.x = g_x;
	xg.usposition = 1;
      }
      if (flags & YValue) {
	xg.y = g_y;
	xg.usposition = 1;
      }
      if (flags & XNegative) {
	xg.xneg = 1;
      }
      if (flags & YNegative) {
	xg.yneg = 1;
      }
    } else if (StrEquals(resource, "DragWellGeometry")) {
      int g_x,g_y,flags;
      unsigned width,height;
      xg.dbw = 30;
      xg.dbh = 30;
      xg.dbx = 9;
      xg.dby = 9;
      flags = FScreenParseGeometry(arg1,&g_x,&g_y,&width,&height);
      if (flags & WidthValue) {
	xg.dbw = width;
      }
      if (flags & HeightValue) {
	xg.dbh= height;
      }
      if (flags & XValue) {
	xg.dbx = g_x;
      }
      if (flags & YValue) {
	xg.dby = g_y;
      }
    }
    free(resource);
    free(arg1);
    free(arg2);
  }

#ifdef USE_UNUSED
  if (!mg.nw) {
    /*Allocate font here because we need size information to create the main menu*/
    if (mg.fontname==NULL) {
      mg.fontname =
	(char *) malloc(sizeof(char)*(strlen("-adobe-helvetica-bold-r-*-*-10-*-*-*-*-*-*-*")+1));
      strcpy(mg.fontname,"-adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*");
    }
    mg.font = XLoadQueryFont(xg.dpy,mg.fontname);
    if (mg.font == NULL) {
      fprintf(stderr,"FvwmDragWell:No font located. Exiting\n");
      exit(1);
    }
  }
#endif

}



/* changeWindowName - change the window name displayed in the title bar.
 * Arguments:
 *   win - the window
 *   str - the new name of the window
 */
void changeWindowName(Window win, char *str)
{
  XTextProperty name;

  if (XStringListToTextProperty(&str,1,&name) == 0)
    {
      fprintf(stderr,"%s: cannot allocate window name",str);
      return;
    }
  XSetWMName(xg.dpy,win,&name);
  XSetWMIconName(xg.dpy,win,&name);
  XFree(name.value);
}



/*change_colorset - changes the background of wharf
 *Arguments:
 *  colorset - the new colorset */
static void change_colorset(int colorset)
{
  if (colorset == xg.colorset && colorset >= 0)
  {
    /* update GCs */
    XFreeGC(xg.dpy, xg.buttonGC);
    XFreeGC(xg.dpy, xg.shadowGC);
    XFreeGC(xg.dpy, xg.hiliteGC);

    /* update background pixmap */
    getReliefColors();
    createBackground();
    XClearWindow(xg.dpy,xg.win);
    drawDragWellButton(&dragBut); /*draws the drag icon*/
  }
  XSync(xg.dpy, 0);
  return;
}




/*
 *
 * myXNextEvent - waits for the next event, which is either an XEvent,
 *                  or an fvwm event.
 * Arguements:
 *   event - the XEvent that is possibly found.
 *   fvwmMessage - the FvwmMessage that is possibly found.
 * Returns FOUND_XEVENT if a XEvent was found.
 * Returns FOUND_FVWM_MESSAGE if a fvwm M_STRING packet was found.
 * Returns FOUND_NON_FVWM_MESSAGE for any other fvwm packet type.
 * Note that for fvwm packet type M_CONFIG(colorset), it updates the
 *  colorset
 * Borrowed from FvwmPager
 */
int myXNextEvent(XEvent *event, char *fvwmMessage)
{
  fd_set in_fdset;
  static int miss_counter = 0;
  char *msg, *tline, *token;
  unsigned long *lptr;
  int colorset;
  if(FPending(xg.dpy)) /*look for an X event first*/
  {
    FNextEvent(xg.dpy,event); /*got an X event*/
    return FOUND_XEVENT;
  }

  FD_ZERO(&in_fdset);
  FD_SET(xg.xfd,&in_fdset); /*look for x events*/
  FD_SET(fd[1],&in_fdset); /*look for fvwm events*/
  if (select(xg.fdWidth,SELECT_FD_SET_CAST &in_fdset, 0, 0, NULL) > 0) /*wait */
  {
    if(FD_ISSET(xg.xfd, &in_fdset))
    {
      if(FPending(xg.dpy))
      {
	FNextEvent(xg.dpy,event); /*get an X event*/
	miss_counter = 0;
	return FOUND_XEVENT;
      }
      miss_counter++;
#ifdef WORRY_ABOUT_MISSED_XEVENTS
      if(miss_counter > 100)
      {
	deadPipe(0);
      }
#endif
      }

      if(FD_ISSET(fd[1], &in_fdset))
      {
	FvwmPacket* packet = ReadFvwmPacket(fd[1]);
	/*packet format defined in libs/Module.h*/
	if ( packet == NULL )
	{
	  exit(0);
	}
	else
	{
	  if (packet->type==M_CONFIG_INFO)
	  {
	    tline = (char*)&(packet->body[3]);
	    token = PeekToken(tline, &tline);
	    if (StrEquals(token, "Colorset"))
	    {
	      /*colorset packet*/
	      colorset = LoadColorset(tline);
	      change_colorset(colorset);
	    }
	    else if (StrEquals(token, XINERAMA_CONFIG_STRING))
	    {
	      FScreenConfigureModule(tline);
	    }
	    return FOUND_FVWM_NON_MESSAGE;
	  }
	  else if (packet->type==M_STRING)
	  {
	    if (packet->size>3)
	    {
	      /* Note that "SendToModule" calls
	       * SendStrToModule(fvwm/module_interface.c)
	       * which seems to insert three pieces of data at the start of
	       * the body */
	      /* The quoting seems to be wrong in SendToModule */
	      /* Sending "SendToModule "FvwmDragWell"   dragitem "stuff"
	       * yields ["  dragitem "stuff"]*/
	      lptr = packet->body;
	      msg = (char *) &(lptr[3]);
	      strcpy(fvwmMessage,msg);
	      return FOUND_FVWM_MESSAGE;
	    }
	    else
	    {
	      return FOUND_FVWM_NON_MESSAGE;
	    }
	  }
	  else if (packet->type == MX_PROPERTY_CHANGE)
	  {
	    if (packet->body[0] == MX_PROPERTY_CHANGE_BACKGROUND &&
		xg.colorset >= 0 &&
		Colorset[xg.colorset].pixmap == ParentRelative &&
		((!Swallowed && packet->body[2] == 0) ||
		 (Swallowed && packet->body[2] == xg.win)))
	    {
	      XClearArea(xg.dpy, xg.win, 0,0,0,0, True);
	    }
	    else if  (packet->body[0] == MX_PROPERTY_CHANGE_SWALLOW &&
		      packet->body[2] == xg.win)
	    {
	      Swallowed = packet->body[1];
	    }
	    return FOUND_FVWM_NON_MESSAGE;
	  }

	  else
	  {
	    return FOUND_FVWM_NON_MESSAGE;
	  }
	}
      }
    }
  return FOUND_FVWM_NON_MESSAGE;
}


/* dummy - used to draw the face of the dragwell button
 * Arguments:
 *   but - the dragwell button
 * Does not return anything useful.*/
int dummy(DragWellButton *but)
{
  if (but->state==DRAGWELL_BUTTON_PUSHED)
    XFillRectangle(xg.dpy,xg.win,xg.buttonGC,xg.dbx+2,xg.dby+2,xg.dbw-4,
		   xg.dbh-4);
  return 0;
}



/*createWindow - creates the drag window.
 * Arguments:*/
void createWindow()
{
  unsigned long valuemask;
  XSetWindowAttributes attributes;

  xg.sizehints.min_width = 1;
  xg.sizehints.min_height = 1;
  xg.sizehints.width_inc = 1;
  xg.sizehints.height_inc = 1;
  xg.sizehints.base_width = xg.w;
  xg.sizehints.base_height = xg.h;
  xg.sizehints.win_gravity = NorthWestGravity;
  xg.sizehints.flags = (PMinSize | PResizeInc | PBaseSize | PWinGravity);
  if (xg.xneg)
  {
    xg.sizehints.win_gravity = NorthEastGravity;
    xg.x = xg.dpyWidth + xg.x - xg.w;
  }
  if (xg.yneg)
  {
    xg.y = xg.dpyHeight + xg.y - xg.h;
    if(xg.sizehints.win_gravity == NorthEastGravity)
      xg.sizehints.win_gravity = SouthEastGravity;
    else
      xg.sizehints.win_gravity = SouthWestGravity;
  }
  if (xg.usposition)
  {
    xg.sizehints.flags |= USPosition;
    /* hack to prevent mapping on wrong screen with StartsOnScreen */
    FScreenMangleScreenIntoUSPosHints(FSCREEN_XYPOS, &xg.sizehints);
  }

  valuemask = (CWBackPixmap | CWBorderPixel| CWColormap);
  attributes.background_pixel = xg.back;
  attributes.border_pixel = xg.fore;
  attributes.background_pixmap = None;

  attributes.colormap = Pcmap;
  attributes.event_mask = (StructureNotifyMask);
  xg.win = XCreateWindow (xg.dpy, xg.root, xg.x, xg.y, xg.w,
			  xg.h, 0, Pdepth, InputOutput, Pvisual,
			  valuemask, &attributes);

  /*_XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);*/
  xg.wmAtomDelWin = XInternAtom(xg.dpy,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(xg.dpy,xg.win,&xg.wmAtomDelWin,1);

  XSetWMNormalHints(xg.dpy,xg.win,&xg.sizehints);

  changeWindowName(xg.win, xg.appName);
  XSelectInput(xg.dpy, xg.win, MW_EVENTS);
  XMapWindow(xg.dpy,xg.win);
}
