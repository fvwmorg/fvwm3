/* Copyright (C) 1999 Joey Shutup.
 *
 * http://www.streetmap.co.uk/streetmap.dll?postcode2map?BS24+9TZ
 *
 * No guarantees or warranties or anything are provided or implied in any way
 * whatsoever.  Use this program at your own risk.  Permission to use this
 * program for any purpose is given, as long as the copyright is kept intact.
 */
/*
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

#include <stdio.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>

#include "libs/fvwmlib.h"
#include "libs/Module.h"
#include "libs/Picture.h"
#include "libs/Colorset.h"

/* Globals */
static Display *dpy;
static Window win;	/* need a window to create pixmaps */
static GC gc;		/* ditto */
static char *name;
static int color_limit = 0;
static int fd[2];	/* communication pipes */

/* forward declarations */
static RETSIGTYPE signal_handler(int signal);
static void set_signals(void);
static int error_handler(Display *dpy, XErrorEvent *e);
static void parse_config(void);
static void parse_config_line(char *line);
static void parse_message_line(char *line);
static void main_loop(void) __attribute__((__noreturn__));
static void parse_colorset(char *line);

int main(int argc, char **argv) {
  XSetWindowAttributes xswa;
  XGCValues xgcv;

  name = GetFileNameFromPath(argv[0]);

  set_signals();

  /* try to make sure it is fvwm that spawned this module */
  if (argc != 6) {
    fprintf(stderr, "%s "VERSION" should only be executed by fvwm!\n", name);
    exit(1);
  }

  /* note the communication pipes */
  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  /* open the display and initialize the picture library */
  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    fprintf(stderr,"%s: can't open display %s", name, XDisplayName(NULL));
    exit (1);
  }
  InitPictureCMap(dpy);

  /* create a window to work in */
  xswa.background_pixmap = CopyFromParent;
  xswa.border_pixel = 0;
  xswa.colormap = Pcmap;
  win = XCreateWindow(dpy, RootWindow(dpy, DefaultScreen(dpy)), -2, -2, 2, 2, 0,
		      Pdepth, InputOutput, Pvisual,
		      CWBackPixmap | CWBorderPixel | CWColormap, &xswa);

  /* create a GC */
  gc = XCreateGC(dpy, win, 0, &xgcv);

  /* die horribly if either win or gc could not be created */
  XSync(dpy, False);

  /* install a non-fatal error handler */
  XSetErrorHandler(error_handler);

  /* get the initial configuration options */
  parse_config();

  /* garbage collect */
  alloca(0);
  
  /* sit around waiting for something to do */
  main_loop();
}

static void main_loop(void) {
  FvwmPacket *packet;

  while (True) {
    /* garbage collect */
    alloca(0);

    /* wait for an instruction from fvwm */
    packet = ReadFvwmPacket(fd[1]);
    if (!packet)
      exit(0);

    /* handle dynamic config lines and text messages */
    switch (packet->type) {
    case M_CONFIG_INFO:
      parse_config_line((char *)&packet->body[3]);
      break;
    case M_STRING:
      parse_message_line((char *)&packet->body[3]);
      break;
    }
  }
}

/* config options, the first NULL is replaced with *FvwmThemeColorset */
/* the "Colorset" config option is what gets reflected by fvwm */
static char *config_options[] = {"ImagePath", "ColorLimit", "Colorset", NULL, NULL};

static void parse_config_line(char *line) {
  char *rest;

  switch(GetTokenIndex(line, config_options, -1, &rest)) {
  case 0:
    SetImagePath(rest);
    break;
  case 1:
    sscanf(rest, "%d", &color_limit);
    break;
  case 2:
    /* got a colorset config line from fvwm, maybe Defaultcolors: load it */
    LoadColorset(rest);
    break;
  case 3:
    parse_colorset(rest);
    break;
  }    
}

/* background pixmap parsing */
static char *pixmap_options[] = {"TiledPixmap", "Pixmap", "AspectPixmap", NULL};
static char *dash = "-";

/* translate a colorset spec into a colorset structure */
static void parse_colorset(char *line) {
  int n, ret;
  colorset_struct *cs;
  char *token, *fg, *bg;
  Picture *picture;

  /* find out which colorset and make sure it exists */
  line = GetNextToken(line, &token);
  if (!token) {
    return;
  }
  ret = !sscanf(token, "%d", &n);
  free(token);
  if (ret)
    return;
  cs = AllocColorset(n);

  /* get the mandatory foreground and background color names */
  line = GetNextToken(line, &fg);
  line = GetNextToken(line, &bg);
  if (!fg)
    return;
  if (!bg) {
    free(fg);
    return;
  }

  /* fill in the structure if color name is not "-" */
  if (!StrEquals(fg, dash)) {
    XFreeColors(dpy, Pcmap, &cs->fg, 1, 0);
    cs->fg = GetColor(fg);
  }
  if (!StrEquals(bg, dash)) {
    XFreeColors(dpy, Pcmap, &cs->bg, 3, 0);
    cs->bg = GetColor(bg);
    cs->hilite = GetHilite(cs->bg);
    cs->shadow = GetShadow(cs->bg);
  }

  /* look for a pixmap specifier, if not "-" remove the existing one */
  GetNextToken(line, &token);
  if (token) {
    ret = StrEquals(token, dash);
    free(token);
  }
  if (!ret) {
    XFreePixmap(dpy, cs->pixmap);
    cs->pixmap = None;
  }

  /* if one was specified try to load it */
  if (token && !ret) {
    int type = GetTokenIndex(line, pixmap_options, -1, &line);
    char *end;
    unsigned int w, h;

    switch(type) {
    case 0:
    case 1:
    case 2:
      /* all three of these are pixmaps, the differ in size bits */
      GetNextToken(line, &token);
      if (!token)
	break;
      /* load the file using the color reduction routines in Picture.c */
      picture = GetPicture(dpy, win, NULL, token, color_limit);
      if (!picture)
	fprintf(stderr, "%s: can't load picture %s\n", name, token);
      free (token);
      if (!picture)
	break;
      /* don't try to be smart with bitmaps */
      if (picture->depth != Pdepth) {
	fprintf(stderr, "%s: bitmaps not supported\n", name);
	DestroyPicture(dpy, picture);
	break;
      }
      /* copy the picture pixmap, then destroy the picture */
      /* could cache picture but it would require an extension to the struct */
      /* and I don't expect the same pixmap to be re-used very often */
      cs->width = picture->width;
      cs->height = picture->height;
      cs->pixmap = XCreatePixmap(dpy, win, cs->width, cs->height, Pdepth);
      XCopyArea(dpy, picture->picture, cs->pixmap, gc, 0, 0, cs->width,
		cs->height, 0, 0);
      DestroyPicture(dpy, picture);
      cs->keep_aspect = (type == 2);
      cs->stretch_x = cs->stretch_y = (type != 0);
      break;
    default:
      /* test for ?Gradient */
      end = GetNextToken(line, &token);
      if (!token)
	break;
      type = toupper(token[0]);
      if (token[1])
        ret = StrEquals(&token[1], "Gradient");
      free(token);
      if (!ret) {
	fprintf(stderr, "%s: bad colorset pixmap specifier %s\n", name, line);
	break;
      }
      /* create a pixmap of the gradient type */
      cs->pixmap = CreateGradientPixmap(dpy, win, gc, type, end, &w, &h);
      cs->width = w;
      cs->height = h;
      cs->keep_aspect = False;
      cs->stretch_x = !(type == 'V');
      cs->stretch_y = !(type == 'H');
    }
  }

  /* make sure the server has this to avoid races */
  XSync(dpy, False);
  
  /* inform fvwm of the change */
  SendText(fd, DumpColorset(n), 0);
}

/* SendToMessage options */
static char *message_options[] = {"Colorset", NULL};

static void parse_message_line(char *line) {
  char *rest;

  switch(GetTokenIndex(line, message_options, -1, &rest)) {
  case 0:
    parse_colorset(rest);
    break;
  }    
}

static void parse_config(void) {
  char *line;

  /* prepare the tokenizer array, [0,1] are ImagePath and ColorLimit */
  config_options[3] = safemalloc(strlen(name) + 10);
  sprintf(config_options[3], "*%sColorset", name);

  /* set a filter on the config lines sent */
  InitGetConfigLine(fd, config_options[3]);
  
  /* tell fvwm what we want to receive */
  SetMessageMask(fd, M_CONFIG_INFO | M_END_CONFIG_INFO
		 | M_SENDCONFIG | M_STRING);

  /* loop on config lines, a bit strange looking because GetConfigLine
   * is a void(), have to test $line */
  while (GetConfigLine(fd, &line), line)
    parse_config_line(line);
}

static int error_handler(Display *d, XErrorEvent *e) {
  char msg[256];
  
  /* Attempting to free colors or pixmaps that were not allocated by this
   * module or were never allocated at all does not cause problems */
  if (((X_FreeColors == e->request_code) && (BadAccess == e->error_code))
      || ((X_FreePixmap == e->request_code) && (BadPixmap == e->error_code)))
    return 0;

  /* other errors cause diagnostic output and stop execution */
  XGetErrorText(d, e->error_code, msg, sizeof(msg));
  fprintf(stderr, "%s: X error: %s\n", name, msg);
  fprintf(stderr, "Major opcode of failed request: %d \n", e->request_code);
  fprintf(stderr, "Resource id of failed request: 0x%lx \n", e->resourceid);
  exit(0);
}

static void set_signals(void) {
#ifdef HAVE_SIGACTION
  struct sigaction  sigact;

  sigemptyset(&sigact.sa_mask);
  sigaddset(&sigact.sa_mask, SIGPIPE);
  sigaddset(&sigact.sa_mask, SIGINT);
  sigaddset(&sigact.sa_mask, SIGHUP);
  sigaddset(&sigact.sa_mask, SIGTERM);
# ifdef SA_INTERRUPT
  sigact.sa_flags = SA_INTERRUPT;
# else
  sigact.sa_flags = 0;
# endif
  sigact.sa_handler = signal_handler;
  sigaction(SIGPIPE, &sigact, NULL);
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGHUP, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
#else
  /* We don't have sigaction(), so fall back to less robust methods.  */
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask(sigmask(SIGPIPE)
		    | sigmask(SIGINT)
		    | sigmask(SIGHUP)
		    | sigmask(SIGTERM));
#endif
  signal(SIGPIPE, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGTERM, signal_handler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGINT, 1);
  siginterrupt(SIGHUP, 1);
  siginterrupt(SIGTERM, 1);
#endif
#endif
}

static RETSIGTYPE signal_handler(int signal) {
  fprintf(stderr, "%s quiting on signal %d\n", name, signal);
  exit(signal);
}
