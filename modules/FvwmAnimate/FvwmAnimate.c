/*
 * FvwmAnimate! Animation module for Fvwm
 *
 * Copyright (c) 1997 Frank Scheelen <scheelen@worldonline.nl>
 * Copyright (c) 1996 Alfredo Kengi Kojima (kojima@inf.ufrgs.br)
 * Copyright (c) 1996 Kaj Groner <kajg@mindspring.com>
 *   Added .steprc parsing and twisty iconify.
 *
 * Copyright (c) 1998 Dan Espen <dane@mk.bellcore.com>
 *   Changed to run under fvwm2.  Lots of changes made.
 *   This used to only animate iconify on M_CONFIGURE_WINDOW.
 *   This module no longer reads M_CONFIGURE_WINDOW.
 *   I added args to M_ICONIFY so iconification takes one message.
 *   The arg parsing is completely redone using library functions.
 *   I also added args to M_DEICONIFY to eliminate the need to read the
 *   window size.
 *   Added AnimateResizeLines animation effect.
 *   Changed option "resize" to "effect", (resize still works).
 *   Changed effect "zoom" to "frame", (zoom still works).
 *   Added myfprintf double parens debugging trick.
 *   Changed so that commands can be sent to this module while it is
 *   running.
 *   Changed so that this module creates its own built in menu.
 *   Added Stop, Save commands.
 *   Changed so this this module uses FvwmForm for complete control on all
 *   settings.
 *
 * This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>                      /* for O_WRONLY */
#include <sys/times.h>                  /* For timing things time() */
#include <limits.h>                     /* For time() */
#include "../../fvwm/module.h"
#include "../../libs/fvwmlib.h"
#include "FvwmAnimate.h"

#define AS_PI 3.14159265358979323846

static Display *dpy;
static int scr;
static Window root;
static GC DrawGC;
static char *MyName;
static int MyNameLen;
static int Channel[2];
static XColor xcol;
static unsigned long color;             /* color for animation */
static XGCValues gcv;
static int animate_none = 0;            /* count bypassed animations */
static char stop_recvd = 'n';           /* got stop command */
static char prior_color = 'n';          /* in case color changes */
static char running = 'n';              /* whether we are initiialized or not */
static char custom_recvd = 'n';         /* got custom command */

/* here is the old double parens trick. */
/* #define DEBUG */
#ifdef DEBUG
#define myfprintf(X) \
  fprintf X;\
  fflush (stderr);
#else
#define myfprintf(X)
#endif


/* #define DEBUG_ANIMATION */
#ifdef DEBUG_ANIMATION
#define myaprintf(X) \
  fprintf X;\
  fflush (stderr);
#else
#define myaprintf(X)
#endif

/* Macros for creating and sending commands:
   CMD1X - Myname+1
   CMD10 - Myname+1,MyName
   CMD11 - Myname+1,MyName+1 */
#define CMD1X(TEXT) \
  sprintf(cmd,TEXT,MyName+1);\
  SendText(Channel,cmd,0);
#define CMD10(TEXT) \
  sprintf(cmd,TEXT,MyName+1,MyName);\
  SendText(Channel,cmd,0);
#define CMD11(TEXT) \
  sprintf(cmd,TEXT,MyName+1,MyName+1);\
  SendText(Channel,cmd,0);

static void Loop();
static void ParseOptions();
static void ParseConfigLine(char *);
static void CreateDrawGC();
static void DefineMe();
static void SaveConfig();
static void StopCmd();
static void AnimateResizeZoom(int, int, int, int, int, int, int, int);
static void AnimateResizeLines(int, int, int, int, int, int, int, int);
static void AnimateResizeFlip(int, int, int, int, int, int, int, int);
static void AnimateResizeTurn(int, int, int, int, int, int, int, int);
static void AnimateResizeRandom(int, int, int, int, int, int, int, int);
static void AnimateResizeZoom3D(int, int, int, int, int, int, int, int);
static void AnimateResizeNone(int, int, int, int, int, int, int, int);
static void AnimateResizeTwist(int, int, int, int, int, int, int, int);
static void DefineForm();

struct ASAnimate Animate = { NULL, ANIM_ITERATIONS, ANIM_DELAY,
                             ANIM_TWIST, ANIM_WIDTH,
                             AnimateResizeTwist, ANIM_TIME };

/* We now have so many effects, that I feel the need for a table. */
typedef struct AnimateEffects {
  char *name;
  char *alias;
  void (*function)(int, int, int, int, int, int, int, int);
  char *button;                 /* used to set custom form */
} ae;
/* Note None and Random must be the first 2 entries. */
struct AnimateEffects effects[] = {
  {"None",0,AnimateResizeNone},
  {"Random",0,AnimateResizeRandom},
  {"Flip",0,AnimateResizeFlip},
  {"Frame","Zoom",AnimateResizeZoom},
  {"Frame3D","Zoom3D",AnimateResizeZoom3D},
  {"Lines",0,AnimateResizeLines},
  {"Turn",0,AnimateResizeTurn},
  {"Twist",0,AnimateResizeTwist}
};
#define NUM_EFFECTS sizeof(effects) / sizeof(struct AnimateEffects)

/*
 * This makes a twisty iconify/deiconify animation for a window, similar to
 * MacOS.  Parameters specify the position and the size of the initial
 * window and the final window
 */
static void AnimateResizeTwist(int x, int y, int w, int h,
                               int fx, int fy, int fw, int fh)
{
    float cx, cy, cw, ch;
    float xstep, ystep, wstep, hstep;
    XPoint points[5];
    float angle, angle_finite, a, d;
    
    x += w/2;
    y += h/2;
    fx += fw/2;
    fy += fh/2;

    xstep = (float)(fx-x)/Animate.iterations;
    ystep = (float)(fy-y)/Animate.iterations;
    wstep = (float)(fw-w)/Animate.iterations;
    hstep = (float)(fh-h)/Animate.iterations;

    cx = (float)x;
    cy = (float)y;
    cw = (float)w;
    ch = (float)h;
    a = atan(ch/cw);
    d = sqrt((cw/2)*(cw/2)+(ch/2)*(ch/2));

    angle_finite = 2*AS_PI*Animate.twist;
    for (angle=0;; angle+=(float)(2*AS_PI*Animate.twist/Animate.iterations)) {
        if (angle > angle_finite)
	    angle = angle_finite;
        points[0].x = cx+cos(angle-a)*d;
        points[0].y = cy+sin(angle-a)*d;
        points[1].x = cx+cos(angle+a)*d;
        points[1].y = cy+sin(angle+a)*d;
        points[2].x = cx+cos(angle-a+AS_PI)*d;
        points[2].y = cy+sin(angle-a+AS_PI)*d;
        points[3].x = cx+cos(angle+a+AS_PI)*d;
        points[3].y = cy+sin(angle+a+AS_PI)*d;
        points[4].x = cx+cos(angle-a)*d;
        points[4].y = cy+sin(angle-a)*d;
	XGrabServer(dpy);
	XDrawLines(dpy, root, DrawGC, points, 5, CoordModeOrigin);
	XFlush(dpy);
	usleep(Animate.delay*1000);
	XDrawLines(dpy, root, DrawGC, points, 5, CoordModeOrigin);
	XUngrabServer(dpy);
	cx+=xstep;
	cy+=ystep;
	cw+=wstep;
	ch+=hstep;
        a = atan(ch/cw);
        d = sqrt((cw/2)*(cw/2)+(ch/2)*(ch/2));
        if (angle >= angle_finite)
	    break;
    }		
    XFlush(dpy);    
}

/*
 * Add even more 3D feel to AfterStep by doing a flipping iconify.
 * Parameters specify the position and the size of the initial and the
 * final window.
 *
 * Idea: how about texture mapped, user definable free 3D movement
 * during a resize? That should get X on its knees all right! :)
 */
void AnimateResizeFlip(int x, int y, int w, int h, int fx, int fy, int fw, int fh)
{
  float cx, cy, cw, ch;
  float xstep, ystep, wstep, hstep;
  XPoint points[5];
  
  float distortx;
  float distortch;
  float midy;
  
  float angle, angle_finite;
  
  xstep = (float) (fx - x) / Animate.iterations;
  ystep = (float) (fy - y) / Animate.iterations;
  wstep = (float) (fw - w) / Animate.iterations;
  hstep = (float) (fh - h) / Animate.iterations;
  
  cx = (float) x;
  cy = (float) y;
  cw = (float) w;
  ch = (float) h;
  
  angle_finite = 2 * AS_PI * Animate.twist;
  for (angle = 0;; angle += (float) (2 * AS_PI * Animate.twist / Animate.iterations)) {
    if (angle > angle_finite)
      angle = angle_finite;
    
    distortx = (cw / 10) - ((cw / 5) * sin(angle));
    distortch = (ch / 2) * cos(angle);
    midy = cy + (ch / 2);
    
    points[0].x = cx + distortx;
    points[0].y = midy - distortch;
    points[1].x = cx + cw - distortx;
    points[1].y = points[0].y;
    points[2].x = cx + cw + distortx;
    points[2].y = midy + distortch;
    points[3].x = cx - distortx;
    points[3].y = points[2].y;
    points[4].x = points[0].x;
    points[4].y = points[0].y;
    
    XGrabServer(dpy);
    XDrawLines(dpy, root, DrawGC, points, 5, CoordModeOrigin);
    XFlush(dpy);
    usleep(Animate.delay * 1000);
    XDrawLines(dpy, root, DrawGC, points, 5, CoordModeOrigin);
    XUngrabServer(dpy);
    cx += xstep;
    cy += ystep;
    cw += wstep;
    ch += hstep;
    if (angle >= angle_finite)
      break;
  }
  XFlush(dpy);
}


/*
 * And another one, this time around the Y-axis.
 */
void AnimateResizeTurn(int x, int y, int w, int h, int fx, int fy, int fw, int fh)
{
    float cx, cy, cw, ch;
    float xstep, ystep, wstep, hstep;
    XPoint points[5];

    float distorty;
    float distortcw;
    float midx;

    float angle, angle_finite;

    xstep = (float) (fx - x) / Animate.iterations;
    ystep = (float) (fy - y) / Animate.iterations;
    wstep = (float) (fw - w) / Animate.iterations;
    hstep = (float) (fh - h) / Animate.iterations;

    cx = (float) x;
    cy = (float) y;
    cw = (float) w;
    ch = (float) h;

    angle_finite = 2 * AS_PI * Animate.twist;
    for (angle = 0;; angle += (float) (2 * AS_PI * Animate.twist / Animate.iterations)) {
	if (angle > angle_finite)
	    angle = angle_finite;

	distorty = (ch / 10) - ((ch / 5) * sin(angle));
	distortcw = (cw / 2) * cos(angle);
	midx = cx + (cw / 2);

	points[0].x = midx - distortcw;
	points[0].y = cy + distorty;
	points[1].x = midx + distortcw;
	points[1].y = cy - distorty;
	points[2].x = points[1].x;
	points[2].y = cy + ch + distorty;
	points[3].x = points[0].x;
	points[3].y = cy + ch - distorty;
	points[4].x = points[0].x;
	points[4].y = points[0].y;

	XGrabServer(dpy);
	XDrawLines(dpy, root, DrawGC, points, 5, CoordModeOrigin);
	XFlush(dpy);
	usleep(Animate.delay * 1000);
	XDrawLines(dpy, root, DrawGC, points, 5, CoordModeOrigin);
	XUngrabServer(dpy);
	cx += xstep;
	cy += ystep;
	cw += wstep;
	ch += hstep;
	if (angle >= angle_finite)
	    break;
    }
    XFlush(dpy);
}

/*
 * This makes a zooming iconify/deiconify animation for a window, like most
 * any other icon animation out there.  Parameters specify the position and
 * the size of the initial window and the final window
 */
static void AnimateResizeZoom(int x, int y, int w, int h,
                               int fx, int fy, int fw, int fh)
{
    float cx, cy, cw, ch;
    float xstep, ystep, wstep, hstep;
    int i;
    
    xstep = (float)(fx-x)/Animate.iterations;
    ystep = (float)(fy-y)/Animate.iterations;
    wstep = (float)(fw-w)/Animate.iterations;
    hstep = (float)(fh-h)/Animate.iterations;

    cx = (float)x;
    cy = (float)y;
    cw = (float)w;
    ch = (float)h;
    for (i=0; i<Animate.iterations; i++) {
	XGrabServer(dpy);
	XDrawRectangle(dpy, root, DrawGC, (int)cx, (int)cy, (int)cw, (int)ch);
	XFlush(dpy);
	usleep(Animate.delay*1000);
	XDrawRectangle(dpy, root, DrawGC, (int)cx, (int)cy, (int)cw, (int)ch);
	XUngrabServer(dpy);
	cx+=xstep;
	cy+=ystep;
	cw+=wstep;
	ch+=hstep;
    }		
    XFlush(dpy);    
}

/*
 * The effect of this is similar to AnimateResizeZoom but this time we
 * add lines to create a 3D effect.  The gotcha is that we have to do
 * something different depending on the direction we are zooming in.
 *
 * Andy Parker <parker_andy@hotmail.com>
 */
void AnimateResizeZoom3D(int x, int y, int w, int h, int fx, int fy, int fw, int fh)
{
    float cx, cy, cw, ch;
    float xstep, ystep, wstep, hstep, srca, dsta;
    int i;

    xstep = (float) (fx - x) / Animate.iterations;
    ystep = (float) (fy - y) / Animate.iterations;
    wstep = (float) (fw - w) / Animate.iterations;
    hstep = (float) (fh - h) / Animate.iterations;
    dsta = (float) (fw + fh);
    srca = (float) (w + h);

    cx = (float) x;

    cy = (float) y;
    cw = (float) w;
    ch = (float) h;

    if (dsta <= srca)
  /* We are going from a Window to an Icon */
    {
	for (i = 0; i < Animate.iterations; i++) {
	    XGrabServer(dpy);
	    XDrawRectangle(dpy, root, DrawGC, (int) cx, (int) cy, (int) cw,
			   (int) ch);
	    XDrawRectangle(dpy, root, DrawGC, (int) fx, (int) fy, (int) fw,
			   (int) fh);
	    XDrawLine(dpy, root, DrawGC, (int) cx, (int) cy, fx, fy);
	    XDrawLine(dpy, root, DrawGC, ((int) cx + (int) cw), (int) cy,
			    (fx + fw), fy);
	    XDrawLine(dpy, root, DrawGC, ((int) cx + (int) cw),
			    ((int) cy + (int) ch), (fx + fw), (fy + fh));
	    XDrawLine(dpy, root, DrawGC, (int) cx, ((int) cy + (int) ch), fx,
			    (fy + fh));
	    XFlush(dpy);
	    usleep(Animate.delay);
	    XDrawRectangle(dpy, root, DrawGC, (int) cx, (int) cy, (int) cw,
			   (int) ch);
	    XDrawRectangle(dpy, root, DrawGC, (int) fx, (int) fy, (int) fw,
			   (int) fh);
	    XDrawLine(dpy, root, DrawGC, (int) cx, (int) cy, fx, fy);
	    XDrawLine(dpy, root, DrawGC, ((int) cx + (int) cw), (int) cy,
			    (fx + fw), fy);
	    XDrawLine(dpy, root, DrawGC, ((int) cx + (int) cw),
			    ((int) cy + (int) ch), (fx + fw), (fy + fh));
	    XDrawLine(dpy, root, DrawGC, (int) cx, ((int) cy + (int) ch), fx,
			    (fy + fh));
	    XUngrabServer(dpy);
	    cx += xstep;
	    cy += ystep;
	    cw += wstep;
	    ch += hstep;
	}
    }
    if (dsta > srca) {
/* We are going from an Icon to a Window */
	for (i = 0; i < Animate.iterations; i++) {
	    XGrabServer(dpy);
	    XDrawRectangle(dpy, root, DrawGC, (int) cx, (int) cy, (int) cw,
			   (int) ch);
	    XDrawRectangle(dpy, root, DrawGC, x, y, w, h);
	    XDrawLine(dpy, root, DrawGC, (int) cx, (int) cy, x, y);
	    XDrawLine(dpy, root, DrawGC, ((int) cx + (int) cw), (int) cy,
			    (x + w), y);
	    XDrawLine(dpy, root, DrawGC, ((int) cx + (int) cw), ((int) cy +
					    (int) ch), (x + w), (y + h));
	    XDrawLine(dpy, root, DrawGC, (int) cx, ((int) cy + (int) ch), x,
			    (y + h));
	    XFlush(dpy);
	    usleep(Animate.delay);
	    XDrawRectangle(dpy, root, DrawGC, (int) cx, (int) cy, (int) cw,
			   (int) ch);
	    XDrawRectangle(dpy, root, DrawGC, x, y, w, h);
	    XDrawLine(dpy, root, DrawGC, (int) cx, (int) cy, x, y);
	    XDrawLine(dpy, root, DrawGC, ((int) cx + (int) cw), (int) cy,
			    (x + w), y);
	    XDrawLine(dpy, root, DrawGC, ((int) cx + (int) cw),
			    ((int) cy + (int) ch), (x + w), (y + h));
	    XDrawLine(dpy, root, DrawGC, (int) cx, ((int) cy + (int) ch), x,
			    (y + h));
	    XUngrabServer(dpy);
	    cx += xstep;
	    cy += ystep;
	    cw += wstep;
	    ch += hstep;
	}
    }
    XFlush(dpy);
}

/*
 * This picks one of the animations and calls it.
 */
void AnimateResizeRandom(int x, int y, int w, int h, int fx, int fy, int fw, int fh)
{
  /* Note, first 2 effects "None" and "Random" should never be chosen */
  effects[(rand() + (x * y + w * h + fx)) % (NUM_EFFECTS - 2) + 2].function
    (x, y, w, h, fx, fy, fw, fh);
}

/*
 * This animation creates 4 lines from each corner of the initial window,
 * to each corner of the final window.
 *
 * Parameters specify the position and the size of the initial window and
 * the final window.
 *
 * Starting with x/y  w/h, need to draw sets  of 4 line segs from initial
 * to final window.
 *
 * The variable "ants" controls whether each  iteration is drawn and then
 * erased vs. draw all the segments and then come back and erase them.
 *
 * Currently I  have this  hardcoded as the  later.   The word  "ants" is
 * used, because if  "ants" is set  to 0 and the  number of iterations is
 * high, it looks a little like ants crawling across the screen.
 */
static void AnimateResizeLines(int x, int y, int w, int h,
                         int fx, int fy, int fw, int fh) {
  int i, j;
  int ants = 1, ant_ctr;
  typedef struct {
    XSegment seg[4];                   /* draw 4 unconnected lines */
    XSegment incr[4];                  /* x/y increments */
  } Endpoints;
  Endpoints ends[2];
  
  /* define the array occurances */
#define UR seg[0]
#define UL seg[1]
#define LR seg[2]
#define LL seg[3]
#define BEG ends[0]
#define INC ends[1]

  if (ants == 1) {                      /* if draw then erase */
    XGrabServer(dpy);                   /* grab for whole animation */
  }
  for (ant_ctr=0;ant_ctr<=ants;ant_ctr++) {
    /*  Put args into arrays: */
    BEG.UR.x1 = x;                      /* upper left */
    BEG.UR.y1 = y;
    /* Temporarily put width and height in Lower Left slot.
     Changed to Lower Left x/y later. */
    BEG.LL.x1 = w;
    BEG.LL.y1 = h;

  /* Use final positions to calc increments. */
    INC.UR.x1 = fx;
    INC.UR.y1 = fy;
    INC.LL.x1 = fw;
    INC.LL.y1 = fh;

  /* The lines look a little better if they start and end a little in
     from the edges.  Allowing tuning might be overkill. */
    for (i=0;i<2;i++) {                   /* for begin and endpoints */
      if (ends[i].LL.x1 > 40) {           /* if width > 40 */
        ends[i].LL.x1 -=16;               /* reduce width a little */
        ends[i].UR.x1 += 8;               /* move in a little */
      }
      if (ends[i].LL.y1 > 40) {           /* if height > 40 */
        ends[i].LL.y1 -=16;               /* reduce height a little */
        ends[i].UR.y1 += 8;               /* move down a little */
      }
      /* Upper Left, Use x from Upper Right + width */
      ends[i].UL.x1 = ends[i].UR.x1 + ends[i].LL.x1;
      ends[i].UL.y1 = ends[i].UR.y1;                /* copy y */
      /* Lower Right, Use y from Upper Right + height */
      ends[i].LR.x1 = ends[i].UR.x1;                /* copy x */
      ends[i].LR.y1 = ends[i].UR.y1 + ends[i].LL.y1;
      /* Now width and height have been used, change LL to endpoints. */
      ends[i].LL.x1 += ends[i].UR.x1;
      ends[i].LL.y1 += ends[i].UR.y1;
    }
    /* Now put the increment in the end x/y slots */
    for (i=0;i<4;i++) {                   /* for each of 4 line segs */
      INC.seg[i].x2 = (INC.seg[i].x1 - BEG.seg[i].x1)/Animate.iterations;
      INC.seg[i].y2 = (INC.seg[i].y1 - BEG.seg[i].y1)/Animate.iterations;
    }
    for (i=0; i<Animate.iterations; i++) {
      for (j=0;j<4;j++) {                 /* all 4 line segs */
        BEG.seg[j].x2 = BEG.seg[j].x1 + INC.seg[j].x2; /* calc end points */
        BEG.seg[j].y2 = BEG.seg[j].y1 + INC.seg[j].y2; /* calc end points */
      }
      myaprintf((stderr,
                 "Lines %dx%d-%dx%d, %dx%d-%dx%d, %dx%d-%dx%d, %dx%d-%dx%d,\
ant_ctr %d\n",
                 BEG.UR.x1, BEG.UR.y1, BEG.UR.x2, BEG.UR.y2,
                 BEG.UL.x1, BEG.UL.y1, BEG.UL.x2, BEG.UL.y2,
                 BEG.LR.x1, BEG.LR.y1, BEG.LR.x2, BEG.LR.y2,
                 BEG.LL.x1, BEG.LL.y1, BEG.LL.x2, BEG.LL.y2, ant_ctr));
      if (ants==0) {
        XGrabServer(dpy);
      }
      XDrawSegments(dpy, root, DrawGC, BEG.seg, 4);
      XFlush(dpy);
      if (ant_ctr == 0) {               /* only pause on draw cycle */
        usleep(Animate.delay*1000);
      }
      if (ants==0) {
        XDrawSegments(dpy, root, DrawGC, BEG.seg, 4);
        XUngrabServer(dpy);
      }
      for (j=0;j<4;j++) {                 /* all 4 lines segs */
        BEG.seg[j].x1 += INC.seg[j].x2;   /* calc new starting point */
        BEG.seg[j].y1 += INC.seg[j].y2;   /* calc new starting point */
      } /* end 4 lines segs */
    } /* end iterations */
  } /* end for ants */
  if (ants == 1) {                      /* if draw then erase */
    XUngrabServer(dpy);                 /* end grab for whole animation */
    myaprintf((stderr,"Did ungrab\n"));
  }
  XFlush(dpy);    
  myaprintf((stderr,"Done animating\n"));
}

/*
 * Animate None  is set on during  configuration.  When set on, it causes
 * this  module to exit  after some  number of  animation events.  (If it
 * just exited immediately, you couldn't use  this module to turn it back
 * on.)
 */
static void AnimateResizeNone(int x, int y, int w, int h,
                         int fx, int fy, int fw, int fh) {
  ++animate_none;
  return;
}
#if 0
/*
 * This makes a animation that looks like that light effect
 * when you turn off an old TV.
 * Used for window destruction
 *
 * This was commented out in Afterstep, I don't know why yet.  dje.
 */
static void AnimateClose(int x, int y, int w, int h)
{
    int i, step;
    
    if (h>4) {
	step = h*4/Animate.iterations;
	if (step==0) {
	    step = 2;
	}
	for (i=h; i>=2; i-=step) {
	    XDrawRectangle(dpy, root, DrawGC, x, y, w, i);
	    XFlush(dpy);
	    usleep(ANIM_DELAY2*600);
	    XDrawRectangle(dpy, root, DrawGC, x, y, w, i);
	    y+=step/2;
	}
    }
    if (w<2) return;
    step = w*4/Animate.iterations;
    if (step==0) {
	step = 2;
    }
    for (i=w; i>=0; i-=step) {
	XDrawRectangle(dpy, root, DrawGC, x, y, i, 2);
	XFlush(dpy);
	usleep(ANIM_DELAY2*1000);
	XDrawRectangle(dpy, root, DrawGC, x, y, i, 2);
	x+=step/2;
    }
    usleep(100000);    
    XFlush(dpy);
}
#endif

void DeadPipe(int foo) {
  myfprintf((stderr,"Dead Pipe, errno %d\n",foo));
  exit(0);
}

int main(int argc, char **argv) {
  char *s;
  char mask_mesg[20];
  char cmd[200];                        /* really big area for a command */

  /* Save our program  name - for error events */
  if ((s=strrchr(argv[0], '/')))	/* strip path */
    s++;
  else				/* no slash */
    s = argv[0];
  if(argc==7)                         /* if override name */
    s = argv[6];                      /* use arg as name */
  MyNameLen=strlen(s)+1;		/* account for '*' */
  MyName = safemalloc(MyNameLen+1);	/* account for \0 */
  *MyName='*';
  strcpy(MyName+1, s);		/* append name */

  myfprintf((stderr,"%s: Starting, argv[0] is %s, len %d\n",MyName+1,
             argv[0],MyNameLen));

  if ((argc != 6)&&(argc != 7)) {	/* Now MyName is defined */
    fprintf(stderr,"%s Version "VERSION" should only be executed by fvwm!\n",
            MyName+1);
    exit(1);
  }
  signal (SIGPIPE, DeadPipe);		/* Dead pipe == Fvwm died */
    
  Channel[0] = atoi(argv[1]);
  Channel[1] = atoi(argv[2]);
    
  dpy = XOpenDisplay("");
  if (dpy==NULL) {
    fprintf(stderr,"%s: could not open display\n",MyName+1);
    exit(1);
  }
  root = DefaultRootWindow(dpy);
  scr = DefaultScreen(dpy);

  sprintf(cmd,"read .%s Quiet",MyName+1); /* read quiet modules config */
  SendText(Channel,cmd,0);
  ParseOptions();                       /* get cmds fvwm2 has parsed */

  sprintf(mask_mesg,"SET_MASK %lu\n",(unsigned long)
          (M_ICONIFY|M_DEICONIFY
           |M_LOCKONSEND|M_SENDCONFIG|M_CONFIG_INFO));
  SendInfo(Channel, mask_mesg, 0);      /* tell fvwm about our mask */
  CreateDrawGC();                       /* create initial GC */
  SendText(Channel,"Nop",0);
  DefineMe();
  running = 'y';                        /* out of initialization phase */
  Loop();                               /* start running */
  exit (0);                             /* Never gets here! */
}

/*
 * Wait for some event like iconify, deiconify and stuff.
 */
static void Loop() {
  unsigned long header[HEADER_SIZE], *body;
  int c;                              /* size of message body */
  clock_t time_start;                     /* for time() */
  clock_t time_end;                     /* for time() */
  clock_t time_accum;
  struct tms time_buffer;              /* for time() */
  char cmd[200];

  myfprintf((stderr,"Starting event loop\n"));
  while (1) {
    c=ReadFvwmPacket(Channel[1], header, &body);
    myfprintf((stderr,"Header[2] is %d, read %d\n", (int)header[2],c));
    if (c>0) {                          /* if message has body */
      switch (header[1]) {            /* check message type */
      case M_DEICONIFY:
        if (header[2] < 15            /* If not all info needed, */
            || (int)body[5] == 0) {   /* or a "noicon" icon */
          break;                      /* don't animate it */
        }
        if (Animate.time != 0) {
          time_start = times(&time_buffer);
        }
        Animate.resize((int)body[3],     /* t->icon_x_loc */
                       (int)body[4],     /* t->icon_y_loc */
                       (int)body[5],     /* t->icon_p_width */
                       (int)body[6],     /* t->icon_p_height */
                       (int)body[7],     /* t->frame_x */
                       (int)body[8],     /* t->frame_y */
                       (int)body[9],     /* t->frame_width */
                       (int)body[10]);   /* t->frame_height */
        if (Animate.time != 0) {
          time_end = times(&time_buffer);
          time_accum = time_end - time_start;
        }
        myaprintf((stderr,
                   "DE_Iconify, args %d+%d+%dx%d %d+%d+%dx%d. took %dx%d\n",
                   (int)body[3],     /* t->icon_x_loc */
                   (int)body[4],     /* t->icon_y_loc */
                   (int)body[5],     /* t->icon_p_width */
                   (int)body[6],     /* t->icon_p_height */
                   (int)body[7],     /* t->frame_x */
                   (int)body[8],     /* t->frame_y */
                   (int)body[9],     /* t->frame_width */
                   (int)body[10],      /* t->frame_height */
                   (int)time_accum,1));
#if 0
        /* So far, clk_tck seems to be non-portable...dje */
/*                    (int)time_accum,(int)CLK_TCK)); */
#endif
        break;
      case M_ICONIFY:
        /* In Afterstep, this logic waited for M_CONFIGURE_WINDOW
             before animating.  To this time, I don't know why.
             (One is sent right after the other.)
          */
        if (header[2] < 15            /* if not enough info */
            || (int)body[3] == -10000    /* or a transient window */
            || (int)body[5] == 0) {   /* or a "noicon" icon */
          break;                    /* don't animate it */
        }
        if (Animate.time != 0) {
          time_start = times(&time_buffer);
        }
        Animate.resize((int)body[7],     /* t->frame_x */
                       (int)body[8],     /* t->frame_y */
                       (int)body[9],     /* t->frame_width */
                       (int)body[10],    /* t->frame_height */
                       (int)body[3],     /* t->icon_x_loc */
                       (int)body[4],     /* t->icon_y_loc */
                       (int)body[5],     /* t->icon_p_width */
                       (int)body[6]);    /* t->icon_p_height */
        if (Animate.time != 0) {
          time_end = times(&time_buffer);
          time_accum = time_end - time_start;
        }
        myaprintf((stderr,
                   "Iconify, args %d+%d+%dx%d %d+%d+%dx%d. Took %d\n",
                   (int)body[7],     /* t->frame_x */
                   (int)body[8],     /* t->frame_y */
                   (int)body[9],     /* t->frame_width */
                   (int)body[10],    /* t->frame_height */
                   (int)body[3],     /* t->icon_x_loc */
                   (int)body[4],     /* t->icon_y_loc */
                   (int)body[5],     /* t->icon_p_width */
                   (int)body[6],
                   (int)time_accum));
        break;
      case M_CONFIG_INFO:
        myfprintf((stderr,"Got command: %s\n", (char *)&body[3]));
        ParseConfigLine((char *)&body[3]);
        break;
      } /* end switch header */
      free(body);                 /* get rid of message body */
    } /* end message had a body */
    myfprintf((stderr,"Sending unlock\n"));
    SendInfo(Channel, "UNLOCK 1\n", 0); /* fvwm can continue now! */
    if ((Animate.resize == AnimateResizeNone  /* If no animation desired */
        && animate_none >= 1)           /* and 1 animation(s) */
        || (stop_recvd == 'y')) {       /* or stop cmd */
      /* This still isn't perfect, if the user turns off animation,
         they would expect the menu to change on the spot.
         On the otherhand, the menu shouldn't change if the module is
         still running.
         This logic is dependent on fvwm sending iconify messages
         to make this module exit.  Currently it is sending messages
         even when "Style NoIcon" is on for everything.
      */
      StopCmd();                        /* update menu */
      myfprintf((stderr,"Exiting, animate none count %d, stop recvd %c\n",
                 animate_none, (int)stop_recvd));
      exit (0);                         /* and stop */
    } /* end stopping */
    /* The execution of the custom command has to be delayed,
       because we first had to send the UNLOCK response.
    */
    if (custom_recvd == 'y') {
      custom_recvd = 'n';
      DefineForm();
      CMD1X("Module FvwmForm Form%s");
    }
  } /* end while forever */
}

/*****************************************************************************
 * 
 * This routine is responsible for reading and parsing the config file
 * Used FvwmEvent as a model.
 *
 ****************************************************************************/
#include <ModParse.h>
static char *table[]= {
  "Color",
#define Color_arg 0
  "Custom",
#define Custom_arg 1
  "Delay",
#define Delay_arg 2
  "Effect",
#define Effect_arg 3
  "Iterations",
#define Iterations_arg 4
  "Resize",
#define Resize_arg 5
  "Save",
#define Save_arg 6
  "Stop",
#define Stop_arg 7
  "Time",
#define Time_arg 8
  "Twist",
#define Twist_arg 9
  "Width"
#define Width_arg 10
};	/* Keep list in alphabetic order, using binary search! */
static void ParseOptions() {
  char *buf;

  myfprintf((stderr,"Reading options\n"));
  while (GetConfigLine(Channel,&buf), buf != NULL) {
    ParseConfigLine(buf);
  } /* end config lines */
} /* end function */


void ParseConfigLine(char *buf) {
  char **e, *p, *q;
  unsigned seed;
  long curtime;
  int i;

  if (buf[strlen(buf)-1] == '\n') {     /* if line ends with newline */
    buf[strlen(buf)-1] = '\0';	/* strip off \n */
  }
  /* Search for MyName (normally *FvwmAnimate) */
  if (strncasecmp(buf, MyName, MyNameLen) == 0) {/* If its for me */
    myfprintf((stderr,"Found line for me: %s\n", buf));
    p = buf+MyNameLen;              /* starting point */
    q = NULL;
    if ((e = FindToken(p,table,char *))) { /* config option ? */
      if ((strcasecmp(*e,"Stop") != 0)
          && (strcasecmp(*e,"Custom") != 0)
          && (strcasecmp(*e,"Save") != 0)) { /* no arg commands */
        p+=strlen(*e);		/* skip matched token */
        q=GetArgument(&p);
        if (!q) {                       /* If arg not found */
          fprintf(stderr,"%s: %s%s needs a parameter\n",
                  MyName+1, MyName+1,*e);
          return;
        }
      }
          
      switch (e - (char**)table) {
      case Stop_arg:                    /* Stop command */
        if (running == 'y') {           /* if not a stored command */
          stop_recvd = 'y';             /* remember to stop */
        }
        break;
      case Save_arg:                    /* Save command */
        SaveConfig();
        break;
      case Custom_arg:                  /* Custom command */
        if (running == 'y') {           /* if not a stored command */
          custom_recvd = 'y';           /* remember someone asked */
        }
        break;
      case Color_arg:                   /* Color */
        if (Animate.color) {
          free(Animate.color);          /* release storage holding color name */
          Animate.color = 0;            /* show its gone */
        }
        if ((strcasecmp(q,"None") != 0) /* If not color "none"  */
            && (strcasecmp(q,"Black^White") != 0) 
            && (strcasecmp(q,"White^Black") != 0)) {
          Animate.color = (char *)strdup(q); /* make copy of name */
        }
        CreateDrawGC();                 /* update GC */
        break;
      case Delay_arg:                   /* Delay */
        Animate.delay = atoi(q);
        break;
      case Iterations_arg:              /* Iterations */
        Animate.iterations = atoi(q);
        break;
      case Effect_arg:                /* Effect */
      case Resize_arg:                /* -or - Resize */
        for (i=0; i < NUM_EFFECTS; i++) {
          if (strcasecmp(q, effects[i].name)==0
              || (effects[i].alias
                  && strcasecmp(q, effects[i].alias)==0)) {
            Animate.resize = effects[i].function;
            break;
          } /* end match */
        } /* end all effects */
        if (i > NUM_EFFECTS) {          /* If not found */
          fprintf(stderr, "%s: Unknown Effect '%s'\n", MyName+1, q);
        }
        /* Logically, you would only reset these when you got a command
           of None, or Random, but it doesn't really matter. */
        animate_none = 0;
        curtime = time(NULL);
        seed = (unsigned) curtime % INT_MAX;
        srand(seed);
        break;
      case Time_arg:                  /* Time in milliseconds */
        Animate.time = (clock_t)atoi(q);
        break;
      case Twist_arg:                 /* Twist */
        Animate.twist = atof(q);
        break;
      case Width_arg:                 /* Width */
        Animate.width = atoi(q);
        CreateDrawGC();                 /* update GC */
        break;
      default:
        fprintf(stderr,"%s: unknown action %s\n",MyName+1,*e);
        break;
      }
    } else {                        /* Match Myname, but a space */
      fprintf(stderr,"%s: unknown command: %s\n",MyName+1,buf);
    }
    if(q) {                             /* if parsed an arg */
      free(q);                          /* free its memory */
    }
  } /* config line for me */
} /* end function */

/* create GC for drawing the various animations */
/* Called during start-up, and whenever the color or line width changes. */
static void CreateDrawGC() {

  myfprintf((stderr,"Creating GC\n"));
  if (DrawGC != NULL) {
    XFreeGC(dpy,DrawGC);                /* free old GC */
  }
  if (prior_color == 'y') {             /* if there was a previous color */
    /*
     * Prior color  if allocated is always freed.   Don't worry about the
     * fact that the color may not have changed.
     */
    XFreeColors(dpy, DefaultColormap(dpy,scr),
                &xcol.pixel,
                1,                      /* # of pixels */
                0);                     /* # of planes */
    prior_color = 'n';                  /* no more prior color */
  }

  color = (BlackPixel(dpy,scr) ^ WhitePixel(dpy,scr));  /* From builtins.c: */
  gcv.function = GXxor;                 /* default is to xor the lines */
  if (Animate.color) {                  /* if color called for */
    if (XParseColor(dpy,DefaultColormap(dpy,scr),Animate.color, &xcol)) {
      if (XAllocColor(dpy, DefaultColormap(dpy,scr), &xcol)) {
        prior_color = 'y';              /* remember to free it */
        color = xcol.pixel;
        /*         gcv.function = GXequiv;  Afterstep used this. */
      } else {
        fprintf(stderr,"%s: could not allocate color '%s'\n",
                MyName+1,Animate.color);
      }	    
    } else {
      fprintf(stderr,"%s: could not parse color '%s'\n",
              MyName+1,Animate.color);
    }
  }
  gcv.line_width = Animate.width;
  gcv.foreground = color;
  gcv.background = color;
  myfprintf((stderr,"Color is %ld\n",gcv.foreground));
  gcv.subwindow_mode = IncludeInferiors;
  DrawGC=XCreateGC(dpy, root, GCFunction|GCForeground|GCLineWidth|GCBackground
                   |GCSubwindowMode, &gcv);
}

/*
 * Send commands to fvwm to define this modules menus.
 *
 * When I first wrote this, I thought it might be a good idea to call the
 * menu  "FvwmAnimate", just like  the  module name.   To my surprise,  I
 * found that fvwm2 treats menus just like functions.  In fact I could no
 * longer start FvwmAnimate  because it kept  finding the menu instead of
 * the  function.   This  probably should   be fixed, but  for  now,  the
 * generated menu is  called  "MenuFvwmAnimate", or  "Menu<ModuleAlias>".
 * dje, 10/11/98.
 */
static void DefineMe() {
  char cmd[200];                        /* really big area for a command */
  myfprintf((stderr,"defining menu\n"));

  CMD1X("DestroyMenu Menu%s");
  CMD1X("DestroyMenu MenuIterations%s");
  CMD1X("DestroyMenu MenuEffects%s");
  CMD1X("DestroyMenu MenuWidth%s");
  CMD1X("DestroyMenu MenuTwist%s");
  CMD1X("DestroyMenu MenuDelay%s");
  CMD1X("DestroyMenu MenuColor%s");

  CMD1X("AddToMenu Menu%s \"Animation Main Menu\" Title");
  CMD11("AddToMenu Menu%s \"&E. Effects\" Popup MenuEffects%s");
  CMD11("AddToMenu Menu%s \"&I. Iterations\" Popup MenuIterations%s");
  CMD11("AddToMenu Menu%s \"&T. Twists\" Popup MenuTwist%s");
  CMD11("AddToMenu Menu%s \"&L. Line Width\" Popup MenuWidth%s");
  CMD11("AddToMenu Menu%s \"&D. Delays\" Popup MenuDelay%s");
  CMD11("AddToMenu Menu%s \"&X. Color for XOR\" Popup MenuColor%s");
  CMD10("AddToMenu Menu%s \"&C. Custom Settings\" %sCustom");
  CMD10("AddToMenu Menu%s \"&S. Save Config\" %sSave");
  CMD10("AddToMenu Menu%s \"&Z. Stop Animation\" %sStop");
  CMD11("AddToMenu Menu%s \"&R. Restart Animation\" FuncRestart%s");

  /* Define function for stopping and restarting Animation. */
  CMD1X("DestroyFunc FuncRestart%s");
  CMD10("AddToFunc FuncRestart%s \"I\" %sStop");
  CMD11("AddToFunc FuncRestart%s \"I\" Module FvwmAnimate %s");

  /* Define the sub menus. */
  CMD1X("AddToMenu MenuIterations%s \"Iterations\" Title");
  CMD10("AddToMenu MenuIterations%s \"&1. Iterations 1\" %sIterations 1");
  CMD10("AddToMenu MenuIterations%s \"&2. Iterations 2\" %sIterations 2");
  CMD10("AddToMenu MenuIterations%s \"&3. Iterations 4\" %sIterations 4");
  CMD10("AddToMenu MenuIterations%s \"&4. Iterations 8\" %sIterations 8");
  CMD10("AddToMenu MenuIterations%s \"&5. Iterations 16\" %sIterations 16");
  CMD10("AddToMenu MenuIterations%s \"&6. Iterations 32\" %sIterations 32");

  CMD1X("AddToMenu MenuWidth%s \"Line Width\" Title");
  CMD10("AddToMenu MenuWidth%s \"&1. Line Width 0 (fast)\" %sWidth 0");
  CMD10("AddToMenu MenuWidth%s \"&2. Line Width 1\" %sWidth 1");
  CMD10("AddToMenu MenuWidth%s \"&3. Line Width 2\" %sWidth 2");
  CMD10("AddToMenu MenuWidth%s \"&4. Line Width 4\" %sWidth 4");
  CMD10("AddToMenu MenuWidth%s \"&5. Line Width 6\" %sWidth 6");
  CMD10("AddToMenu MenuWidth%s \"&6. Line Width 8\" %sWidth 8");

  CMD1X("AddToMenu MenuTwist%s \"Twists (Twist, Turn, Flip only)\" Title");
  CMD10("AddToMenu MenuTwist%s \"&1. Twist .25\" %sTwist .25");
  CMD10("AddToMenu MenuTwist%s \"&2. Twist .50\" %sTwist .50");
  CMD10("AddToMenu MenuTwist%s \"&3. Twist 1\" %sTwist 1");
  CMD10("AddToMenu MenuTwist%s \"&4. Twist 2\" %sTwist 2");

  CMD1X("AddToMenu MenuDelay%s \"Delays\" Title");
  CMD10("AddToMenu MenuDelay%s \"&1. Delay 1/1000 sec\" %sDelay 1");
  CMD10("AddToMenu MenuDelay%s \"&2. Delay 1/100 sec\" %sDelay 10");
  CMD10("AddToMenu MenuDelay%s \"&3. Delay 1/10 sec\" %sDelay 100");

  /* Same as the colors at the front of the colorlimiting table */
  CMD1X("AddToMenu MenuColor%s \"Colors\" Title");
  CMD10("AddToMenu MenuColor%s \"&1. Color Black^White\" %sColor None");
  CMD10("AddToMenu MenuColor%s \"&2. Color White\" %sColor white");
  CMD10("AddToMenu MenuColor%s \"&3. Color Black\" %sColor black");
  CMD10("AddToMenu MenuColor%s \"&4. Color Grey\" %sColor grey");
  CMD10("AddToMenu MenuColor%s \"&5. Color Green\" %sColor green");
  CMD10("AddToMenu MenuColor%s \"&6. Color Blue\" %sColor blue");
  CMD10("AddToMenu MenuColor%s \"&7. Color Red\" %sColor red");
  CMD10("AddToMenu MenuColor%s \"&8. Color Cyan\" %sColor cyan");
  CMD10("AddToMenu MenuColor%s \"&9. Color Yellow\" %sColor yellow");
  CMD10("AddToMenu MenuColor%s \"&A. Color Magenta\" %sColor magenta");
  CMD10("AddToMenu MenuColor%s \"&B. Color DodgerBlue\" %sColor DodgerBlue");
  CMD10("AddToMenu MenuColor%s \"&C. Color SteelBlue\" %sColor SteelBlue");
  CMD10("AddToMenu MenuColor%s \"&D. Color Chartreuse\" %sColor chartreuse");
  CMD10("AddToMenu MenuColor%s \"&E. Color Wheat\" %sColor wheat");
  CMD10("AddToMenu MenuColor%s \"&F. Color Turquoise\" %sColor turquoise");

  CMD1X("AddToMenu MenuEffects%s \"Effects\" Title");
  CMD10("AddToMenu MenuEffects%s \"&1. Effect Random\" %sEffect Random");
  CMD10("AddToMenu MenuEffects%s \"&2. Effect Flip\" %sEffect Flip");
  CMD10("AddToMenu MenuEffects%s \"&3. Effect Frame\" %sEffect Frame");
  CMD10("AddToMenu MenuEffects%s \"&4. Effect Frame3D\" %sEffect Frame3D");
  CMD10("AddToMenu MenuEffects%s \"&5. Effect Lines\" %sEffect Lines");
  CMD10("AddToMenu MenuEffects%s \"&6. Effect Turn\" %sEffect Turn");
  CMD10("AddToMenu MenuEffects%s \"&7. Effect Twist\" %sEffect Twist");
  CMD10("AddToMenu MenuEffects%s \"&N. Effect None\" %sEffect None");

  /* Still to be done:
     Use of FvwmForms for Help.  (Need to fix line spacing in FvwmForms first).
  */
}

/* Write the current config into a file. */
static void SaveConfig() {
  FILE *config_file;
  int i;
  char filename[100];                   /* more than enough room */
  char msg[200];                        /* even more room for msg */
  /* Need to use logic to create fully qualified file name same as in
     read.c, right now, this logic only works well if fvwm is started
     from the users home directory.
  */
  sprintf(filename,".%s",MyName+1);
  config_file = fopen(filename,"w");
  if (config_file == NULL) {
    sprintf(msg,
            "%s: Open config file <%s> for write failed. \
Save not done! Error\n",
            MyName+1, filename);
    perror(msg);
    return;
  }
  fprintf(config_file,"# This file created by %s\n\n",MyName+1);
  for (i=0; i < NUM_EFFECTS; i++) {
    if (effects[i].function == Animate.resize) {
      fprintf(config_file,"*%sEffect %s\n",MyName+1,effects[i].name);
      break;
    } /* found match */
  } /* all possible effects */
  fprintf(config_file,"*%sIterations %d\n",MyName+1,Animate.iterations);
  fprintf(config_file,"*%sWidth %d\n",MyName+1,Animate.width);
  fprintf(config_file,"*%sTwist %f\n",MyName+1,Animate.twist);
  fprintf(config_file,"*%sDelay %d\n",MyName+1,Animate.delay);
  if (Animate.color) {
    fprintf(config_file,"*%sColor %s\n",MyName+1,Animate.color);
  }
  /* Note, add "time" if that ever works. dje. 10/14/98 */
  fclose(config_file);
}

/* Stop is different than KillModule in that it gives this module a chance to
   alter the builtin menu before it exits.
*/
static void StopCmd() {
  char cmd[200];
  myfprintf((stderr,"%s: Defining startup menu in preparation for stop\n",
             MyName+1));
  CMD1X("DestroyMenu Menu%s");
  CMD11("AddToMenu Menu%s \"%s\" Title");
  CMD11("AddToMenu Menu%s \"&0. Start FvwmAnimate\" Module %s");
}
static void DefineForm() {
  int i;
  char cmd[200];
  myfprintf((stderr,"Defining form Form%s\n", MyName+1));
  CMD1X("DestroyModuleConfig Form%s*");
  CMD1X("*Form%sWarpPointer");
  CMD1X("*Form%sFont         9x15bold");
  CMD1X("*Form%sButtonFont   8x13bold");
  CMD1X("*Form%sInputFont    9x15bold");
  CMD1X("*Form%sFore         white");
  CMD1X("*Form%sBack         cornflowerblue");
  CMD1X("*Form%sItemFore     green");
  CMD1X("*Form%sLine         center");
  CMD11("*Form%sText         \"Custom settings for %s\"");
  CMD1X("*Form%sLine         left");
  CMD1X("*Form%sText         \"\"");
  CMD1X("*Form%sLine         left");
  CMD1X("*Form%sFore         Black");
  CMD1X("*Form%sText         \"Effect:\"");
  CMD1X("*Form%sSelection    meth single");
  for (i=0; i < NUM_EFFECTS; i++) {    /* for all effects */
    effects[i].button="off";           /* init the button setting */
    if (Animate.resize == effects[i].function) { /* compare to curr setting */
      effects[i].button="on";          /* turn on one button */
    } /* end if curr setting */
  } /* end all buttons */
  /* Macro for a command with one var */
#define CMD1V(TEXT,VAR) \
  sprintf(cmd,TEXT,MyName+1,VAR);\
  SendText(Channel,cmd,0);

  /* There is a cleaner way (using the array) for this...dje */
  CMD1V("*Form%sChoice RANDOM RANDOM %s \"Random\"",effects[1].button);
  CMD1V("*Form%sChoice FLIP FLIP %s \"Flip\"",effects[2].button);
  CMD1V("*Form%sChoice FRAME FRAME %s \"Frame\"",effects[3].button);
  CMD1V("*Form%sChoice FRAME3D FRAME3D %s \"Frame3d\"",effects[4].button);
  CMD1V("*Form%sChoice LINES LINES %s \"Lines\"",effects[5].button);
  CMD1V("*Form%sChoice TURN TURN %s \"Turn\"",effects[6].button);
  CMD1V("*Form%sChoice TWIST TWIST %s \"Twist\"",effects[7].button);
  CMD1X("*Form%sLine         left");
  CMD1X("*Form%sText         \"\"");
  CMD1X("*Form%sLine         left");
  CMD1X("*Form%sText         \"Iterations:\"");
  CMD1V("*Form%sInput        Iterations 5 \"%d\"",Animate.iterations);
  CMD1X("*Form%sText         \"Twists:\"");
  CMD1V("*Form%sInput        Twists 10 \"%f\"",Animate.twist);
  CMD1X("*Form%sText         \"Linewidth:\"");
  CMD1V("*Form%sInput        Linewidth 3 \"%d\"",Animate.width);
  CMD1X("*Form%sText         \"Delays:\"");
  CMD1V("*Form%sInput        Delays 5 \"%d\"",Animate.delay);
  CMD1X("*Form%sLine         left");
  CMD1X("*Form%sText         \"\"");
  CMD1X("*Form%sLine         left");
  CMD1X("*Form%sText         \"Color:\"");
  CMD1V("*Form%sInput        Color 20 \"%s\"",
        Animate.color ? Animate.color : "Black^White");
  CMD1X("*Form%sLine         left");
  CMD1X("*Form%sText         \"\"");
  /*
    F1 - Apply, F2 - Apply and Save, F3 - Reset, F4 - Dismiss
  */

  CMD1X("*Form%sLine         expand");
  CMD1X("*Form%sButton       continue \"F1 - Apply\" F1");
  CMD11("*Form%sCommand      *%sIterations $(Iterations)");
  CMD11("*Form%sCommand      *%sTwist $(Twists)");
  CMD11("*Form%sCommand      *%sWidth $(Linewidth)");
  CMD11("*Form%sCommand      *%sDelay $(Delays)");
  CMD11("*Form%sCommand      *%sColor $(Color)");
  CMD11("*Form%sCommand      *%sEffect $(RANDOM?Random)\
$(FLIP?Flip)\
$(FRAME?Frame)\
$(FRAME3D?Frame3d)\
$(LINES?Lines)\
$(TURN?Turn)\
$(TWIST?Twist)");

  CMD1X("*Form%sButton       continue \"F2 - Apply & Save\" F2");
  CMD11("*Form%sCommand      *%sIterations $(Iterations)");
  CMD11("*Form%sCommand      *%sTwist $(Twists)");
  CMD11("*Form%sCommand      *%sWidth $(Linewidth)");
  CMD11("*Form%sCommand      *%sColor $(Color)");
  CMD11("*Form%sCommand      *%sEffect $(RANDOM?Random)\
$(FLIP?Flip)\
$(FRAME?Frame)\
$(FRAME3D?Frame3d)\
$(LINES?Lines)\
$(TURN?Turn)\
$(TWIST?Twist)");
  CMD11("*Form%sCommand      *%sSave");

  CMD1X("*Form%sButton       restart   \"F3 - Reset\" F3");

  CMD1X("*Form%sButton       quit \"F4 - Dismiss\" F4");
  CMD1X("*Form%sCommand Nop");
}
