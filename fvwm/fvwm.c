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

/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 * fvwm - "F? Virtual Window Manager"
 ***********************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_GETPWUID
#  include <pwd.h>
#endif

#include "fvwm.h"
#include "fvwmsignal.h"
#include "events.h"
#include "functions.h"
#include "menus.h"
#include "misc.h"
#include "screen.h"
#include "Module.h"
#include "read.h"
#include "session.h"
#include "virtual.h"
#include "stack.h"
#include "gnome.h"
#include "colors.h"
#include "colormaps.h"
#include "module_interface.h"
#include "icccm2.h"
#include "libs/Colorset.h"

#include <X11/Xproto.h>
#include <X11/Xatom.h>
/* need to get prototype for XrmUniqueQuark for XUniqueContext call */
#include <X11/Xresource.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

#ifdef I18N_MB
#include <X11/Xlocale.h>
#endif

#if HAVE_SYS_SYSTEMINFO_H
/* Solaris has sysinfo instead of gethostname.  */
#include <sys/systeminfo.h>
#endif

#define MAXHOSTNAME 255

#ifndef lint
static char sccsid[] __attribute__((__unused__))
    = "@(#)fvwm.c " VERSION " " __DATE__ " fvwm";
#endif

int master_pid;			/* process number of 1st fvwm process */

ScreenInfo Scr;		        /* structures for the screen */
MenuInfo Menus;                 /* structures for menus */
Display *dpy;			/* which display are we talking to */

Bool fFvwmInStartup = True;     /* Set to False when startup has finished */
Bool DoingCommandLine = False;	/* Set True before each cmd line arg */

#define MAX_CFG_CMDS 10
static char *config_commands[MAX_CFG_CMDS];
static int num_config_commands=0;

int FvwmErrorHandler(Display *, XErrorEvent *);
int CatchFatal(Display *);
int CatchRedirectError(Display *, XErrorEvent *);
void InstallSignals(void);
void ChildDied(int nonsense);
void SaveDesktopState(void);
void SetMWM_INFO(Window window);
void SetRCDefaults(void);
void StartupStuff(void);
static int parseCommandArgs(
  const char *command, char **argv, int maxArgc, const char **errorMsg);
static void InternUsefulAtoms(void);
static void InitVariables(void);
static void usage(void);
static int MappedNotOverride(Window w);

XContext FvwmContext;		/* context for fvwm windows */
XContext MenuContext;		/* context for fvwm menus */

int JunkX = 0, JunkY = 0;
Window JunkRoot, JunkChild;		/* junk window */
unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

Boolean debugging = False,PPosOverride;

char **g_argv;
int g_argc;

#ifdef SESSION
char *client_id = NULL;
#endif
char *state_filename = NULL;
char *restart_state_filename = NULL;  /* $HOME/.fs-restart */

/* assorted gray bitmaps for decorative borders */
#define g_width 2
#define g_height 2
static char g_bits[] = {0x02, 0x01};

#define l_g_width 4
#define l_g_height 2
static char l_g_bits[] = {0x08, 0x02};

#define s_g_width 4
#define s_g_height 4
static char s_g_bits[] = {0x01, 0x02, 0x04, 0x08};

#ifdef SHAPE
int ShapeEventBase, ShapeErrorBase;
Boolean ShapesSupported=False;
#endif

long isIconicState = 0;
extern XEvent Event;
Bool Restarting = False;
int fd_width, x_fd;
char *display_name = NULL;
char *user_home_dir;

typedef enum { FVWM_RUNNING=0, FVWM_DONE, FVWM_RESTART } FVWM_STATE;

static volatile sig_atomic_t fvwmRunState = FVWM_RUNNING;

/***********************************************************************
 *
 *  Procedure:
 *	main - start of fvwm
 *
 ***********************************************************************
 */
int main(int argc, char **argv)
{
  unsigned long valuemask;	/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */
  int i;
  extern int x_fd;
  int len;
  char *display_string;
  char message[255];
  Bool single = False;
  Bool replace_wm = False;
  Bool option_error = False;
  int visualClass = -1;
  int visualId = -1;
  int x, y;

  g_argv = argv;
  g_argc = argc;

  DBUG("main","Entered, about to parse args");

#ifdef I18N_MB
  if (setlocale(LC_CTYPE, "") == NULL)
    fvwm_msg(ERR, "main", "Can't set locale. Check your $LC_CTYPE or $LANG.\n");
#endif

  /* Put the default module directory into the environment so it can be used
     later by the config file, etc.  */
  putenv("FVWM_MODULEDIR=" FVWM_MODULEDIR);

  /* Figure out where to read and write config files. */
  user_home_dir = getenv("FVWM_USERHOME");
  if ( user_home_dir == NULL )
    user_home_dir = getenv("HOME");
#ifdef HAVE_GETPWUID
  if ( user_home_dir == NULL ) {
    struct passwd* pw = getpwuid(getuid());
    if ( pw != NULL )
      user_home_dir = strdup( pw->pw_dir );
  }
#endif
  if ( user_home_dir == NULL )
    user_home_dir = "."; /* give up and use current dir */

  for (i = 1; i < argc; i++)
  {
    if (strncasecmp(argv[i],"-debug",6)==0)
    {
      debugging = True;
    }
#ifdef SESSION
    else if (strncasecmp(argv[i], "-clientId", 9) == 0)
      {
	if (++i >= argc)
	  usage();
	client_id = argv[i];
      }
#endif
    else if (strncasecmp(argv[i], "-restore", 8) == 0)
      {
	if (++i >= argc)
	  usage();
	state_filename = argv[i];
      }
    else if (strncasecmp(argv[i],"-s",2)==0)
    {
      single = True;
    }
    else if (strncasecmp(argv[i],"-d",2)==0)
    {
      if (++i >= argc)
        usage();
      display_name = argv[i];
    }
    else if (strncasecmp(argv[i],"-f",2)==0)
    {
      if (++i >= argc)
        usage();
      if (num_config_commands < MAX_CFG_CMDS)
      {
        config_commands[num_config_commands] =
          (char *)malloc(6+strlen(argv[i]));
        strcpy(config_commands[num_config_commands],"Read ");
        strcat(config_commands[num_config_commands],argv[i]);
        num_config_commands++;
      }
      else
      {
        fvwm_msg(ERR,"main","only %d -f and -cmd parms allowed!",MAX_CFG_CMDS);
      }
    }
    else if (strncasecmp(argv[i],"-cmd",4)==0)
    {
      if (++i >= argc)
        usage();
      if (num_config_commands < MAX_CFG_CMDS)
      {
        config_commands[num_config_commands] = strdup(argv[i]);
        num_config_commands++;
      }
      else
      {
        fvwm_msg(ERR,"main","only %d -f and -cmd parms allowed!",MAX_CFG_CMDS);
      }
    }
    else if (strncasecmp(argv[i],"-h",2)==0)
    {
      usage();
      exit(0);
    }
    else if (strncasecmp(argv[i],"-blackout",9)==0)
    {
      /* obsolete option */
      fvwm_msg(WARN, "main", "The -blackout option is obsolete, it may be "
	       "removed in the future.",
              VERSION,__DATE__,__TIME__);
    }
    else if (strncasecmp(argv[i], "-replace", 8) == 0)
    {
      replace_wm = True;
    }
    /* check for visualId before visual to remove ambiguity */
    else if (strncasecmp(argv[i],"-visualId",9)==0) {
      visualClass = -1;
      if (++i >= argc)
        usage();
      if (sscanf(argv[i], "0x%x", &visualId) == 0)
        if (sscanf(argv[i], "%d", &visualId) == 0)
          usage();
    }
    else if (strncasecmp(argv[i],"-visual",7)==0) {
      visualId = None;
      if (++i >= argc)
        usage();
      if (strncasecmp(argv[i], "staticg", 7) == 0)
        visualClass = StaticGray;
      else if (strncasecmp(argv[i], "g", 1) == 0)
        visualClass = GrayScale;
      else if (strncasecmp(argv[i], "staticc", 7) == 0)
        visualClass = StaticColor;
      else if (strncasecmp(argv[i], "p", 1) == 0)
        visualClass = PseudoColor;
      else if (strncasecmp(argv[i], "t", 1) == 0)
        visualClass = TrueColor;
      else if (strncasecmp(argv[i], "d", 1) == 0)
        visualClass = DirectColor;
      else
        usage();
    }
    else if (strncasecmp(argv[i], "-version", 8) == 0)
    {
      fvwm_msg(INFO,"main", "Fvwm Version %s compiled on %s at %s\n",
              VERSION,__DATE__,__TIME__);
      exit(0);
    }
    else
    {
      fvwm_msg(ERR,"main","Unknown option:  `%s'\n", argv[i]);
      option_error = TRUE;
    }
  }

  DBUG("main","Done parsing args");

  if (option_error)
  {
    usage();
  }

  DBUG("main","Installing signal handlers");
  InstallSignals();

  ReapChildren();

  if (!(Pdpy = dpy = XOpenDisplay(display_name)))
  {
    fvwm_msg(ERR,"main","can't open display %s", XDisplayName(display_name));
    exit (1);
  }
  Scr.screen= DefaultScreen(dpy);
  Scr.NumberOfScreens = ScreenCount(dpy);

  master_pid = getpid();

  if(!single)
  {
    int	myscreen = 0;
    char *cp;

    strcpy(message, XDisplayString(dpy));

    for(i=0;i<Scr.NumberOfScreens;i++)
    {
      if (i != Scr.screen && fork() == 0)
      {
        myscreen = i;

        /*
         * Truncate the string 'whatever:n.n' to 'whatever:n',
         * and then append the screen number.
         */
        cp = strchr(message, ':');
        if (cp != NULL)
        {
          cp = strchr(cp, '.');
          if (cp != NULL)
            *cp = '\0';		/* truncate at display part */
        }
        sprintf(message + strlen(message), ".%d", myscreen);
        dpy = XOpenDisplay(message);
        Scr.screen = myscreen;
        Scr.NumberOfScreens = ScreenCount(dpy);

        break;
      }
    }
  }

  x_fd = XConnectionNumber(dpy);
  fd_width = GetFdWidth();

#ifdef HAVE_X11_FD
  if (fcntl(x_fd, F_SETFD, 1) == -1)
  {
    fvwm_msg(ERR,"main","close-on-exec failed");
    exit (1);
  }
#endif

  /*  Add a DISPLAY entry to the environment, incase we were started
   * with fvwm -display term:0.0
   */
  len = strlen(XDisplayString(dpy));
  display_string = safemalloc(len+10);
  sprintf(display_string,"DISPLAY=%s",XDisplayString(dpy));
  putenv(display_string);
  /* Add a HOSTDISPLAY environment variable, which is the same as
   * DISPLAY, unless display = :0.0 or unix:0.0, in which case the full
   * host name will be used for ease in networking . */
  /* Note: Can't free the rdisplay_string after putenv, because it
   * becomes part of the environment! */
  if(strncmp(display_string,"DISPLAY=:",9)==0)
  {
    char client[MAXHOSTNAME], *rdisplay_string;
    gethostname(client,MAXHOSTNAME);
    rdisplay_string = safemalloc(len+14 + strlen(client));
    sprintf(rdisplay_string,"HOSTDISPLAY=%s:%s",client,&display_string[9]);
    putenv(rdisplay_string);
  }
  else if(strncmp(display_string,"DISPLAY=unix:",13)==0)
  {
    char client[MAXHOSTNAME], *rdisplay_string;
    gethostname(client,MAXHOSTNAME);
    rdisplay_string = safemalloc(len+14 + strlen(client));
    sprintf(rdisplay_string,"HOSTDISPLAY=%s:%s",client,
            &display_string[13]);
    putenv(rdisplay_string);
  }
  else
  {
    char *rdisplay_string;
    rdisplay_string = safemalloc(len+14);
    sprintf(rdisplay_string,"HOSTDISPLAY=%s",XDisplayString(dpy));
    putenv(rdisplay_string);
  }

  Scr.Root = RootWindow(dpy, Scr.screen);
  if(Scr.Root == None)
  {
    fvwm_msg(ERR,"main","Screen %d is not a valid screen",(char *)Scr.screen);
    exit(1);
  }

  {
    Cursor cursor = XCreateFontCursor(dpy, XC_watch);
    XGrabPointer(dpy, Scr.Root, 0, 0, GrabModeSync, GrabModeSync,
               None, cursor, CurrentTime);
  }

  if (visualClass != -1) {
    /* grab the best visual of the required class */
    XVisualInfo template, *vizinfo;
    int total, i;

    Pdepth = 0;
    template.screen = Scr.screen;
    template.class = visualClass;
    vizinfo = XGetVisualInfo(dpy, VisualScreenMask | VisualClassMask, &template,
                             &total);
    if (total) {
      for (i = 0; i < total; i++) {
        if (vizinfo[i].depth > Pdepth) {
          Pvisual = vizinfo[i].visual;
          Pdepth = vizinfo[i].depth;
        }
      }
      XFree(vizinfo);
      /* have to have a colormap for non-default visual windows */
      Pcmap = XCreateColormap(dpy, Scr.Root, Pvisual, AllocNone);
      Pdefault = False;
    } else {
      fvwm_msg(ERR, "main","Cannot find visual class %d", visualClass);
      visualClass = -1;
    }
  } else if (visualId != -1) {
    /* grab visual id asked for */
    XVisualInfo template, *vizinfo;
    int total;

    Pdepth = 0;
    template.screen = Scr.screen;
    template.visualid = visualId;
    vizinfo = XGetVisualInfo(dpy, VisualScreenMask | VisualIDMask, &template,
                             &total);
    if (total) {
      /* visualID's are unique so there will only be one */
      Pvisual = vizinfo[0].visual;
      Pdepth = vizinfo[0].depth;
      XFree(vizinfo);
      /* have to have a colormap for non-default visual windows */
      Pcmap = XCreateColormap(dpy, Scr.Root, Pvisual, AllocNone);
      Pdefault = False;
    } else {
      fvwm_msg(ERR, "main", "VisualId 0x%x is not valid ", visualId);
      visualId = -1;
    }
  }

  /* use default visuals if none found so far */
  if (visualClass == -1 && visualId == -1) {
    Pvisual = DefaultVisual(dpy, Scr.screen);
    Pdepth = DefaultDepth(dpy, Scr.screen);
    Pcmap = DefaultColormap(dpy, Scr.screen);
    Pdefault = True;
  }

  /* make a copy of the visual stuff so that XorPixmap can swap with root */
  SaveFvwmVisual();

  /* the following is a workaround for my Exceed server which has problems
   * drawing client supplied icon pixmaps when the display is 16bit, the root
   * is 8 bit Pseudo and fvwm is run with the -visual option (24 bit) */
  /* it might give others a headache in which case the test should be more
   * restrictive i.e. based on the display vendor string */
  Scr.exceed_hack = (Pdepth != DefaultDepth(dpy, Scr.screen));

#ifdef SHAPE
  ShapesSupported = XShapeQueryExtension(dpy, &ShapeEventBase, &ShapeErrorBase);
#endif /* SHAPE */

  InternUsefulAtoms ();

  /* Make sure property priority colors is empty */
  XChangeProperty (dpy, Scr.Root, _XA_MIT_PRIORITY_COLORS,
                   XA_CARDINAL, 32, PropModeReplace, NULL, 0);

  /* create a window which will accept the keyboard focus when no other
     windows have it */
  /* do this before any RC parsing as some GC's are created from this window
   * rather than the root window */
  attributes.event_mask = KeyPressMask|FocusChangeMask;
  attributes.override_redirect = True;
  attributes.colormap = Pcmap;
  attributes.background_pixmap = None;
  attributes.border_pixel = 0;
  Scr.NoFocusWin=XCreateWindow(dpy, Scr.Root, -10, -10, 10, 10, 0, Pdepth,
                               InputOutput, Pvisual,
                               CWEventMask | CWOverrideRedirect | CWColormap
                               | CWBackPixmap | CWBorderPixel, &attributes);
  XMapWindow(dpy, Scr.NoFocusWin);

  SetMWM_INFO(Scr.NoFocusWin);
  XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, CurrentTime);

  XSync(dpy, 0);
  if(debugging)
    XSynchronize(dpy,1);

  SetupICCCM2 (replace_wm);
  XSetErrorHandler(CatchRedirectError);
  XSetIOErrorHandler(CatchFatal);
  XSelectInput(dpy, Scr.Root,
               LeaveWindowMask| EnterWindowMask | PropertyChangeMask |
               SubstructureRedirectMask | KeyPressMask |
               SubstructureNotifyMask|
	       ColormapChangeMask|
#ifdef HAVE_STROKE
		       ButtonMotionMask |
		       Button1MotionMask |
		       Button2MotionMask |
		       Button3MotionMask |
#endif /* HAVE_STROKE */
               ButtonPressMask | ButtonReleaseMask );
  XSync(dpy, 0);

  XSetErrorHandler(FvwmErrorHandler);

  {
    Atom atype;
    int aformat;
    unsigned long nitems, bytes_remain;
    unsigned char *prop;

    if (
      XGetWindowProperty(
        dpy, Scr.Root, _XA_WM_DESKTOP, 0L, 1L, True, _XA_WM_DESKTOP,
        &atype, &aformat, &nitems, &bytes_remain, &prop
      ) == Success
    ) {
      if (prop != NULL) {
        Restarting = True;
        /* single = True; */
      }
    }
  }

  restart_state_filename =
    strdup(CatString2(user_home_dir, "/.fs-restart"));
  if (!state_filename && Restarting)
    state_filename = restart_state_filename;

  /*
     This should be done early enough to have the window states loaded
     before the first call to AddWindow.
   */
  LoadWindowStates(state_filename);

  Scr.FvwmCursors = CreateCursors(dpy);
  InitVariables();
  if (visualClass != -1 || visualId != -1) {
    /* this is so that menus use the (non-default) fvwm colormap */
    Scr.FvwmRoot.w = Scr.NoFocusWin;
    Scr.FvwmRoot.number_cmap_windows = 1;
    Scr.FvwmRoot.cmap_windows = &Scr.NoFocusWin;
  }
  InitEventHandlerJumpTable();
  initModules();

  Scr.gray_bitmap =
    XCreateBitmapFromData(dpy,Scr.Root,g_bits, g_width,g_height);


  DBUG("main","Setting up rc file defaults...");
  SetRCDefaults();

  DBUG("main","Running config_commands...");
  if (num_config_commands > 0)
  {
    int i;
    for(i=0;i<num_config_commands;i++)
    {
      DoingCommandLine = True;
      ExecuteFunction(config_commands[i], NULL,&Event,C_ROOT,-1,
		      EXPAND_COMMAND);
      free(config_commands[i]);
    }
    DoingCommandLine = False;
  } else {
      /** Run startup commands in ~/.fvwm2rc or FVWM_CONFIGDIR/system.fvwm2rc.
	  If these fail, try FVWM_CONFIGDIR/.fvwm2rc for backwards compat. **/
      if ( !run_command_file( CatString3(user_home_dir, "/", FVWMRC),
			      &Event, NULL, C_ROOT, -1 )
	   && !run_command_file( CatString3(FVWM_CONFIGDIR, "/system", FVWMRC),
				 &Event, NULL, C_ROOT, -1 )
	   && !run_command_file( CatString3(FVWM_CONFIGDIR, "/", FVWMRC),
				 &Event, NULL, C_ROOT, -1 ) )
      {
	  fvwm_msg( ERR, "main",
	    "Cannot read startup file.  Tried %s/%s, %s/system%s, and %s/%s",
		    user_home_dir, FVWMRC,
		    FVWM_CONFIGDIR, FVWMRC,
		    FVWM_CONFIGDIR, FVWMRC );
      }
  }

  DBUG("main","Done running config_commands");

  if(Pdepth<2)
  {
    Scr.gray_pixmap =
      XCreatePixmapFromBitmapData(dpy,Scr.NoFocusWin, g_bits, g_width, g_height,
				  Colorset[0].fg, Colorset[0].bg, Pdepth);
    Scr.light_gray_pixmap =
      XCreatePixmapFromBitmapData(dpy, Scr.NoFocusWin, l_g_bits, l_g_width,
				  l_g_height, Colorset[0].fg, Colorset[0].bg,
				  Pdepth);
    Scr.sticky_gray_pixmap =
      XCreatePixmapFromBitmapData(dpy, Scr.NoFocusWin, s_g_bits, s_g_width,
				  s_g_height, Colorset[0].fg, Colorset[0].bg,
				  Pdepth);
  }

  /* create the move/resize feedback window */
  Scr.SizeStringWidth = XTextWidth (Scr.StdFont.font,
                                    " +8888 x +8888 ", 15);
  attributes.background_pixel = Colorset[0].bg;
  attributes.colormap = Pcmap;
  attributes.border_pixel = 0;
  valuemask = CWBackPixel | CWColormap | CWBorderPixel;

  if(!Scr.gs.EmulateMWM)
  {
    x = 0;
    y = 0;
  }
  else
  {
    x = Scr.MyDisplayWidth/2 - (Scr.SizeStringWidth + SIZE_HINDENT*2)/2;
    y = Scr.MyDisplayHeight/2 - (Scr.StdFont.height + SIZE_VINDENT*2)/2;
  }
  Scr.SizeWindow = XCreateWindow (dpy, Scr.Root, x, y,
				  (unsigned int)(Scr.SizeStringWidth +
						 SIZE_HINDENT*2),
				  (unsigned int) (Scr.StdFont.height +
						  SIZE_VINDENT*2),
				  (unsigned int) 0, Pdepth,
				  InputOutput, Pvisual,
				  valuemask, &attributes);
  initPanFrames();

  MyXGrabServer(dpy);
  checkPanFrames();
  MyXUngrabServer(dpy);

  CoerceEnterNotifyOnCurrentWindow();

#ifdef SESSION
  SessionInit(client_id);
#endif

#ifdef GNOME
  GNOME_Init();
#endif

  DBUG("main","Entering HandleEvents loop...");

  HandleEvents();
#ifdef FVWM_DEBUG_MSGS
  if ( debug_term_signal )
  {
    fvwm_msg(DBG, "main", "Terminated by signal %d", debug_term_signal);
  }
#endif
  switch( fvwmRunState )
  {
  case FVWM_DONE:
    Done(0, NULL);     /* does not return */

  case FVWM_RESTART:
    Done(1, *g_argv);  /* does not return */

  default:
    DBUG("main","Unknown FVWM run-state");
  }

  return 0;
}


/*
** StartupStuff
**
** Does initial window captures and runs init/restart function
*/
void StartupStuff(void)
{
  #define startFuncName "StartFunction"
  const char *initFuncName;

  CaptureAllWindows();
  /* Have to do this here too because preprocessor modules have not run to the
   * end when HandleEvents is entered from the main loop. */
  checkPanFrames();

  fFvwmInStartup = False;

  /* Make sure we have the correct click time now. */
  if (Scr.ClickTime < 0)
    Scr.ClickTime = -Scr.ClickTime;

  /* migo (04-Sep-1999): execute StartFunction */
  if (FindFunction(startFuncName)) {
    char *action = "Function " startFuncName;
    ExecuteFunction(action, NULL, &Event, C_ROOT, -1, EXPAND_COMMAND);
  }

  /* migo (03-Jul-1999): execute [Session]{Init|Restart}Function */
  initFuncName = getInitFunctionName(Restarting == True);
  if (FindFunction(initFuncName)) {
    char *action = strdup(CatString2("Function ", initFuncName));
    ExecuteFunction(action, NULL, &Event, C_ROOT, -1, EXPAND_COMMAND);
    free(action);
  }

  /*
     This should be done after the initialization is finished, since
     it directly changes the global state.
   */
  LoadGlobalState(state_filename);

  /*
  ** migo (20-Jun-1999): Remove state file after usage.
  ** migo (09-Jul-1999): but only on restart, otherwise it can be reused.
  */
  if (Restarting) unlink(state_filename);

  XUngrabPointer(dpy, CurrentTime);

} /* StartupStuff */


/***********************************************************************
 *
 *  Procedure:
 *      CaptureOneWindow
 *      CaptureAllWindows
 *
 *   Decorates windows at start-up and during recaptures
 *
 ***********************************************************************/

void CaptureOneWindow(FvwmWindow *fw, Window window)
{
  Window w;
  Atom atype;
  int aformat;
  unsigned long nitems, bytes_remain;
  unsigned char *prop;
  unsigned long data[1];

  if (fw == NULL)
    return;
  if(XFindContext(dpy, window, FvwmContext, (caddr_t *)&fw)!=XCNOENT)
  {
    PPosOverride = True;
    isIconicState = DontCareState;
    if(XGetWindowProperty(dpy, fw->w, _XA_WM_STATE, 0L, 3L, False,
			  _XA_WM_STATE, &atype, &aformat, &nitems,
			  &bytes_remain, &prop) == Success)
    {
      if(prop != NULL)
      {
	isIconicState = *(long *)prop;
	XFree(prop);
      }
    }
    data[0] = (unsigned long) fw->Desk;
    XChangeProperty (dpy, fw->w, _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32,
                     PropModeReplace, (unsigned char *) data, 1);


    XSelectInput(dpy, fw->w, 0);
    w = fw->w;
    XUnmapWindow(dpy,fw->frame);
    XUnmapWindow(dpy,w);
    RestoreWithdrawnLocation (fw,True);
    SET_DO_REUSE_DESTROYED(fw, 1); /* RBW - 1999/03/20 */
    Destroy(fw);
    Event.xmaprequest.window = w;
    HandleMapRequestKeepRaised(None, fw);
    PPosOverride = False;
    return;
  }
}

void CaptureAllWindows(void)
{
  int i,j;
  unsigned int nchildren;
  Window root, parent, *children;
  FvwmWindow *tmp;		/* temp fvwm window structure */

  MyXGrabServer(dpy);

  if(!XQueryTree(dpy, Scr.Root, &root, &parent, &children, &nchildren))
  {
    MyXUngrabServer(dpy);
    return;
  }

  PPosOverride = True;

  if (!(Scr.flags.windows_captured)) /* initial capture? */
  {
    /*
    ** weed out icon windows
    */
    for (i=0;i<nchildren;i++)
    {
      if (children[i])
      {
        XWMHints *wmhintsp = XGetWMHints (dpy, children[i]);
        if (wmhintsp)
        {
          if (wmhintsp->flags & IconWindowHint)
          {
            for (j = 0; j < nchildren; j++)
            {
              if (children[j] == wmhintsp->icon_window)
              {
                children[j] = None;
                break;
              }
            }
          }
          XFree ((char *) wmhintsp);
        }
      }
    }
    /*
    ** map all of the non-override, non-icon windows
    */
    for (i = 0; i < nchildren; i++)
    {
      if (children[i] && MappedNotOverride(children[i]))
      {
        XUnmapWindow(dpy, children[i]);
        Event.xmaprequest.window = children[i];
        HandleMapRequestKeepRaised (None, NULL);
      }
    }
    Scr.flags.windows_captured = 1;
  }
  else /* must be recapture */
  {
    /* reborder all windows */
    for(i=0; i<nchildren; i++)
    {
      if(XFindContext(dpy, children[i], FvwmContext, (caddr_t *)&tmp)!=XCNOENT)
      {
        CaptureOneWindow(tmp, children[i]);
      }
    }
  }

  isIconicState = DontCareState;

  if(nchildren > 0)
    XFree((char *)children);

  /* after the windows already on the screen are in place,
   * don't use PPosition */
  PPosOverride = False;
  MyXUngrabServer(dpy);
  XSync(dpy,0); /* should we do this on initial capture? */
}

/*
** SetRCDefaults
**
** Sets some initial style values & such
*/
void SetRCDefaults(void)
{
  /* set up default colors, fonts, etc */
  char *defaults[] = {
    "HilightColor black grey",
    "XORValue 0",
    "DefaultFont fixed",
    "DefaultColors black grey",
    "MenuStyle * fvwm, Foreground black, Background grey, Greyed slategrey",
    "TitleStyle Centered -- Raised",
    "Style * Color lightgrey/dimgrey",
    "AddToMenu MenuFvwmRoot \"Builtin Menu\" Title",
    "+ \"&1. XTerm\" Exec xterm",
    "+ \"&2. Setup Form\" Module FvwmForm FormFvwmSetup.",
    "+ \"&3. Issue FVWM commands\" Module FvwmConsole",
    "+ \"&X. Exit FVWM\" Quit",
    "Mouse 0 R N Popup MenuFvwmRoot",
    "Read "FVWM_CONFIGDIR"/ConfigFvwmDefaults",
    NULL
  };
  int i=0;

  while (defaults[i])
  {
    ExecuteFunction(defaults[i],NULL,&Event,C_ROOT,-1,EXPAND_COMMAND);
    i++;
  }
} /* SetRCDefaults */

/***********************************************************************
 *
 *  Procedure:
 *	MappedNotOverride - checks to see if we should really
 *		put a fvwm frame on the window
 *
 *  Returned Value:
 *	TRUE	- go ahead and frame the window
 *	FALSE	- don't frame the window
 *
 *  Inputs:
 *	w	- the window to check
 *
 ***********************************************************************/

static int MappedNotOverride(Window w)
{
  XWindowAttributes wa;
  Atom atype;
  int aformat;
  unsigned long nitems, bytes_remain;
  unsigned char *prop;

  isIconicState = DontCareState;

  if((w==Scr.NoFocusWin)||(!XGetWindowAttributes(dpy, w, &wa)))
    return False;

  if(XGetWindowProperty(dpy,w,_XA_WM_STATE,0L,3L,False,_XA_WM_STATE,
			&atype,&aformat,&nitems,&bytes_remain,&prop)==Success)
  {
    if(prop != NULL)
    {
      isIconicState = *(long *)prop;
      XFree(prop);
    }
  }
  if(wa.override_redirect == True)
  {
    XSelectInput(dpy,w,FocusChangeMask);
  }
  return (((isIconicState == IconicState)||(wa.map_state != IsUnmapped)) &&
	  (wa.override_redirect != True));
}


/***********************************************************************
 *
 *  Procedure:
 *	InternUsefulAtoms:
 *            Dont really know what it does
 *
 ***********************************************************************
 */
Atom _XA_MIT_PRIORITY_COLORS;
Atom _XA_WM_CHANGE_STATE;
Atom _XA_WM_STATE;
Atom _XA_WM_COLORMAP_WINDOWS;
extern Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_DESKTOP;
Atom _XA_MwmAtom;
Atom _XA_MOTIF_WM;

Atom _XA_OL_WIN_ATTR;
Atom _XA_OL_WT_BASE;
Atom _XA_OL_WT_CMD;
Atom _XA_OL_WT_HELP;
Atom _XA_OL_WT_NOTICE;
Atom _XA_OL_WT_OTHER;
Atom _XA_OL_DECOR_ADD;
Atom _XA_OL_DECOR_DEL;
Atom _XA_OL_DECOR_CLOSE;
Atom _XA_OL_DECOR_RESIZE;
Atom _XA_OL_DECOR_HEADER;
Atom _XA_OL_DECOR_ICON_NAME;

Atom _XA_WM_WINDOW_ROLE;
Atom _XA_WM_CLIENT_LEADER;
Atom _XA_SM_CLIENT_ID;

static void InternUsefulAtoms (void)
{
  /*
   * Create priority colors if necessary.
   */
  _XA_MIT_PRIORITY_COLORS = XInternAtom(dpy, "_MIT_PRIORITY_COLORS", False);
  _XA_WM_CHANGE_STATE = XInternAtom (dpy, "WM_CHANGE_STATE", False);
  _XA_WM_STATE = XInternAtom (dpy, "WM_STATE", False);
  _XA_WM_COLORMAP_WINDOWS = XInternAtom (dpy, "WM_COLORMAP_WINDOWS", False);
  _XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);
  _XA_WM_TAKE_FOCUS = XInternAtom (dpy, "WM_TAKE_FOCUS", False);
  _XA_WM_DELETE_WINDOW = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  _XA_WM_DESKTOP = XInternAtom (dpy, "WM_DESKTOP", False);
  _XA_MwmAtom=XInternAtom(dpy,"_MOTIF_WM_HINTS",False);
  _XA_MOTIF_WM=XInternAtom(dpy,"_MOTIF_WM_INFO",False);

  _XA_OL_WIN_ATTR=XInternAtom(dpy,"_OL_WIN_ATTR",False);
  _XA_OL_WT_BASE=XInternAtom(dpy,"_OL_WT_BASE",False);
  _XA_OL_WT_CMD=XInternAtom(dpy,"_OL_WT_CMD",False);
  _XA_OL_WT_HELP=XInternAtom(dpy,"_OL_WT_HELP",False);
  _XA_OL_WT_NOTICE=XInternAtom(dpy,"_OL_WT_NOTICE",False);
  _XA_OL_WT_OTHER=XInternAtom(dpy,"_OL_WT_OTHER",False);
  _XA_OL_DECOR_ADD=XInternAtom(dpy,"_OL_DECOR_ADD",False);
  _XA_OL_DECOR_DEL=XInternAtom(dpy,"_OL_DECOR_DEL",False);
  _XA_OL_DECOR_CLOSE=XInternAtom(dpy,"_OL_DECOR_CLOSE",False);
  _XA_OL_DECOR_RESIZE=XInternAtom(dpy,"_OL_DECOR_RESIZE",False);
  _XA_OL_DECOR_HEADER=XInternAtom(dpy,"_OL_DECOR_HEADER",False);
  _XA_OL_DECOR_ICON_NAME=XInternAtom(dpy,"_OL_DECOR_ICON_NAME",False);

  _XA_WM_WINDOW_ROLE=XInternAtom(dpy, "WM_WINDOW_ROLE",False);
  _XA_WM_CLIENT_LEADER=XInternAtom(dpy, "WM_CLIENT_LEADER",False);
  _XA_SM_CLIENT_ID=XInternAtom(dpy, "SM_CLIENT_ID",False);
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	InstallSignals: install the signal handlers, using whatever
 *         means we have at our disposal. The more POSIXy, the better
 *
 ************************************************************************/
void
InstallSignals(void)
{
#ifdef HAVE_SIGACTION
  struct sigaction  sigact;

  /*
   * All signals whose handlers call fvwmSetTerminate()
   * must be mutually exclusive - we mustn't receive one
   * while processing any of the others ...
   */
  sigemptyset(&sigact.sa_mask);
  sigaddset(&sigact.sa_mask, SIGINT);
  sigaddset(&sigact.sa_mask, SIGHUP);
  sigaddset(&sigact.sa_mask, SIGQUIT);
  sigaddset(&sigact.sa_mask, SIGTERM);
  sigaddset(&sigact.sa_mask, SIGUSR1);

#ifdef SA_RESTART
  sigact.sa_flags = SA_RESTART;
#else
  sigact.sa_flags = 0;
#endif
  sigact.sa_handler = DeadPipe;
  sigaction(SIGPIPE, &sigact, NULL);

#ifdef SA_INTERRUPT
  sigact.sa_flags = SA_INTERRUPT;
#else
  sigact.sa_flags = 0;
#endif
  sigact.sa_handler = Restart;
  sigaction(SIGUSR1, &sigact, NULL);

  sigact.sa_handler = SigDone;
  sigaction(SIGINT,  &sigact, NULL);
  sigaction(SIGHUP,  &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
#else
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGUSR1) |
                     sigmask(SIGINT)  |
                     sigmask(SIGHUP)  |
                     sigmask(SIGQUIT) |
                     sigmask(SIGTERM) );
#endif

  /*
   * We don't have sigaction(), so fall back on
   * less reliable methods ...
   */
  signal(SIGPIPE, DeadPipe);
  signal(SIGUSR1, Restart);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGUSR1, 1);
#endif

  signal(SIGINT, SigDone);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGINT, 1);
#endif
  signal(SIGHUP, SigDone);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGHUP, 1);
#endif
  signal(SIGQUIT, SigDone);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGQUIT, 1);
#endif
  signal(SIGTERM, SigDone);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGTERM, 1);
#endif
#endif
}


/*************************************************************************
 * Restart on a signal
 ************************************************************************/
RETSIGTYPE Restart(int sig)
{
  fvwmRunState = FVWM_RESTART;

  /*
   * This function might not return - it could "long-jump"
   * right out, so we need to do everything we need to do
   * BEFORE we call it ...
   */
  fvwmSetTerminate(sig);
}

/***********************************************************************
 *
 *  LoadDefaultLeftButton -- loads default left button # into
 *		assumes associated button memory is already free
 *
 ************************************************************************/

void LoadDefaultLeftButton(ButtonFace *bf, int i)
{
#ifndef VECTOR_BUTTONS
    bf->style = SimpleButton;
#else
    struct vector_coords *v = &bf->u.vector;

    bf->style = VectorButton;
    switch (i % 5)
    {
    case 0:
    case 4:
	v->num = 5;
	v->line_style = 0;
	v->x = (int *) safemalloc (sizeof(int) * v->num);
	v->y = (int *) safemalloc (sizeof(int) * v->num);
	v->x[0] = 22;
	v->y[0] = 39;
	v->line_style |= (1 << 0);
	v->x[1] = 78;
	v->y[1] = 39;
	v->line_style |= (1 << 1);
	v->x[2] = 78;
	v->y[2] = 61;
	v->x[3] = 22;
	v->y[3] = 61;
	v->x[4] = 22;
	v->y[4] = 39;
	v->line_style |= (1 << 4);
	break;
    case 1:
	v->num = 5;
	v->line_style = 0;
	v->x = (int *) safemalloc (sizeof(int) * v->num);
	v->y = (int *) safemalloc (sizeof(int) * v->num);
	v->x[0] = 32;
	v->y[0] = 45;
	v->x[1] = 68;
	v->y[1] = 45;
	v->x[2] = 68;
	v->y[2] = 55;
	v->line_style |= (1 << 2);
	v->x[3] = 32;
	v->y[3] = 55;
	v->line_style |= (1 << 3);
	v->x[4] = 32;
	v->y[4] = 45;
	break;
    case 2:
	v->num = 5;
	v->line_style = 0;
	v->x = (int *) safemalloc (sizeof(int) * v->num);
	v->y = (int *) safemalloc (sizeof(int) * v->num);
	v->x[0] = 49;
	v->y[0] = 49;
	v->line_style |= (1 << 0);
	v->x[1] = 51;
	v->y[1] = 49;
	v->line_style |= (1 << 1);
	v->x[2] = 51;
	v->y[2] = 51;
	v->x[3] = 49;
	v->y[3] = 51;
	v->x[4] = 49;
	v->y[4] = 49;
	v->line_style |= (1 << 4);
	break;
    case 3:
	v->num = 5;
	v->line_style = 0;
	v->x = (int *) safemalloc (sizeof(int) * v->num);
	v->y = (int *) safemalloc (sizeof(int) * v->num);
	v->x[0] = 32;
	v->y[0] = 45;
	v->line_style |= (1 << 0);
	v->x[1] = 68;
	v->y[1] = 45;
	v->line_style |= (1 << 1);
	v->x[2] = 68;
	v->y[2] = 55;
	v->x[3] = 32;
	v->y[3] = 55;
	v->x[4] = 32;
	v->y[4] = 45;
	v->line_style |= (1 << 4);
	break;
    }
#endif /* VECTOR_BUTTONS */
}

/***********************************************************************
 *
 *  LoadDefaultRightButton -- loads default left button # into
 *		assumes associated button memory is already free
 *
 ************************************************************************/
void LoadDefaultRightButton(ButtonFace *bf, int i)
{
#ifndef VECTOR_BUTTONS
    bf->style = SimpleButton;
#else
    struct vector_coords *v = &bf->u.vector;

    bf->style = VectorButton;
    switch (i % 5)
    {
    case 0:
    case 3:
	v->num = 5;
	v->line_style = 0;
	v->x = (int *) safemalloc (sizeof(int) * v->num);
	v->y = (int *) safemalloc (sizeof(int) * v->num);
	v->x[0] = 25;
	v->y[0] = 25;
	v->line_style |= (1 << 0);
	v->x[1] = 75;
	v->y[1] = 25;
	v->line_style |= (1 << 1);
	v->x[2] = 75;
	v->y[2] = 75;
	v->x[3] = 25;
	v->y[3] = 75;
	v->x[4] = 25;
	v->y[4] = 25;
	v->line_style |= (1 << 4);
	break;
    case 1:
	v->num = 5;
	v->line_style = 0;
	v->x = (int *) safemalloc (sizeof(int) * v->num);
	v->y = (int *) safemalloc (sizeof(int) * v->num);
	v->x[0] = 39;
	v->y[0] = 39;
	v->line_style |= (1 << 0);
	v->x[1] = 61;
	v->y[1] = 39;
	v->line_style |= (1 << 1);
	v->x[2] = 61;
	v->y[2] = 61;
	v->x[3] = 39;
	v->y[3] = 61;
	v->x[4] = 39;
	v->y[4] = 39;
	v->line_style |= (1 << 4);
	break;
    case 2:
	v->num = 5;
	v->line_style = 0;
	v->x = (int *) safemalloc (sizeof(int) * v->num);
	v->y = (int *) safemalloc (sizeof(int) * v->num);
	v->x[0] = 49;
	v->y[0] = 49;
	v->line_style |= (1 << 0);
	v->x[1] = 51;
	v->y[1] = 49;
	v->line_style |= (1 << 1);
	v->x[2] = 51;
	v->y[2] = 51;
	v->x[3] = 49;
	v->y[3] = 51;
	v->x[4] = 49;
	v->y[4] = 49;
	v->line_style |= (1 << 4);
	break;
    case 4:
	v->num = 5;
	v->line_style = 0;
	v->x = (int *) safemalloc (sizeof(int) * v->num);
	v->y = (int *) safemalloc (sizeof(int) * v->num);
	v->x[0] = 36;
	v->y[0] = 36;
	v->line_style |= (1 << 0);
	v->x[1] = 64;
	v->y[1] = 36;
	v->line_style |= (1 << 1);
	v->x[2] = 64;
	v->y[2] = 64;
	v->x[3] = 36;
	v->y[3] = 64;
	v->x[4] = 36;
	v->y[4] = 36;
	v->line_style |= (1 << 4);
	break;
    }
#endif /* VECTOR_BUTTONS */
}

/***********************************************************************
 *
 *  LoadDefaultButton -- loads default button # into button structure
 *		assumes associated button memory is already free
 *
 ************************************************************************/
void LoadDefaultButton(ButtonFace *bf, int i)
{
    int n = i / 2;
    if ((n * 2) == i) {
	if (--n < 0) n = 4;
	LoadDefaultRightButton(bf, n);
    } else
	LoadDefaultLeftButton(bf, n);
}

extern void FreeButtonFace(Display *dpy, ButtonFace *bf);

/***********************************************************************
 *
 *  ResetAllButtons -- resets all buttons to defaults
 *                 destroys existing buttons
 *
 ************************************************************************/
void ResetAllButtons(FvwmDecor *fl)
{
    TitleButton *leftp, *rightp;
    int i=0;

    for (leftp=fl->left_buttons, rightp=fl->right_buttons;
         i < 5;
         ++i, ++leftp, ++rightp) {
      ButtonFace *lface, *rface;
      int j;

      leftp->flags = 0;
      rightp->flags = 0;

      lface = leftp->state;
      rface = rightp->state;

      FreeButtonFace(dpy, lface);
      FreeButtonFace(dpy, rface);

      LoadDefaultLeftButton(lface++, i);
      LoadDefaultRightButton(rface++, i);

      for (j = 1; j < MaxButtonState; ++j, ++lface, ++rface) {
        FreeButtonFace(dpy, lface);
        FreeButtonFace(dpy, rface);

	LoadDefaultLeftButton(lface, i);
	LoadDefaultRightButton(rface, i);
      }
    }

    /* standard MWM decoration hint assignments (veliaa@rpi.edu)
       [Menu]  - Title Bar - [Minimize] [Maximize] */
    fl->left_buttons[0].flags |= MWMDecorMenu;
    fl->right_buttons[1].flags |= MWMDecorMinimize;
    fl->right_buttons[0].flags |= MWMDecorMaximize;
}

/***********************************************************************
 *
 *  DestroyFvwmDecor -- frees all memory assocated with an FvwmDecor
 *	structure, but does not free the FvwmDecor itself
 *
 ************************************************************************/
void DestroyFvwmDecor(FvwmDecor *fl)
{
  int i;
  /* reset to default button set (frees allocated mem) */
  ResetAllButtons(fl);
  for (i = 0; i < 3; ++i)
  {
      int j = 0;
      for (; j < MaxButtonState; ++j)
	  FreeButtonFace(dpy, &fl->titlebar.state[i]);
  }
#ifdef BORDERSTYLE
  FreeButtonFace(dpy, &fl->BorderStyle.active);
  FreeButtonFace(dpy, &fl->BorderStyle.inactive);
#endif
#ifdef USEDECOR
  if (fl->tag) {
      free(fl->tag);
      fl->tag = NULL;
  }
#endif
  if (fl->HiReliefGC != NULL) {
      XFreeGC(dpy, fl->HiReliefGC);
      fl->HiReliefGC = NULL;
  }
  if (fl->HiShadowGC != NULL) {
      XFreeGC(dpy, fl->HiShadowGC);
      fl->HiShadowGC = NULL;
  }
  if (fl->WindowFont.font != NULL)
    XFreeFont(dpy, fl->WindowFont.font);
}

/***********************************************************************
 *
 *  InitFvwmDecor -- initializes an FvwmDecor structure to defaults
 *
 ************************************************************************/
void InitFvwmDecor(FvwmDecor *fl)
{
    int i;
    ButtonFace tmpbf;

    fl->HiReliefGC = NULL;
    fl->HiShadowGC = NULL;
    fl->TitleHeight = 0;
    fl->WindowFont.font = NULL;

#ifdef USEDECOR
    fl->tag = NULL;
    fl->next = NULL;

    if (fl != &Scr.DefaultDecor) {
	extern void AddToDecor(FvwmDecor *, char *);
	AddToDecor(fl, "HilightColor black grey");
	AddToDecor(fl, "WindowFont fixed");
    }
#endif

    /* initialize title-bar button styles */
    tmpbf.style = SimpleButton;
#ifdef MULTISTYLE
    tmpbf.next = NULL;
#endif
    for (i = 0; i < 5; ++i) {
	int j = 0;
	for (; j < MaxButtonState; ++j) {
	    fl->left_buttons[i].state[j] =
		fl->right_buttons[i].state[j] =  tmpbf;
	}
    }

    /* reset to default button set */
    ResetAllButtons(fl);

    /* initialize title-bar styles */
    fl->titlebar.flags = 0;

    for (i = 0; i < MaxButtonState; ++i) {
	fl->titlebar.state[i].style = SimpleButton;
#ifdef MULTISTYLE
	fl->titlebar.state[i].next = NULL;
#endif
    }

#ifdef BORDERSTYLE
    /* initialize border texture styles */
    fl->BorderStyle.active.style = SimpleButton;
    fl->BorderStyle.inactive.style = SimpleButton;
#ifdef MULTISTYLE
    fl->BorderStyle.active.next = NULL;
    fl->BorderStyle.inactive.next = NULL;
#endif
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	InitVariables - initialize fvwm variables
 *
 ************************************************************************/
static void InitVariables(void)
{
  FvwmContext = XUniqueContext();
  MenuContext = XUniqueContext();

  /* initialize some lists */
  Scr.AllBindings = NULL;

  Scr.functions = NULL;

  Menus.all = NULL;
  Menus.DefaultStyle = NULL;
  Menus.LastStyle = NULL;
  Menus.PopupDelay10ms = DEFAULT_POPUP_DELAY;
  Menus.DoubleClickTime = DEFAULT_MENU_CLICKTIME;
  memset(&Menus.flags, 0, sizeof(Menus.flags));
  Scr.last_added_item.type = ADDED_NONE;

  Scr.DefaultIcon = NULL;

  AllocColorset(0);
  Scr.StdGC = 0;
  Scr.StdReliefGC = 0;
  Scr.StdShadowGC = 0;
  Scr.DrawGC = 0;

  /* zero all flags */
  memset(&Scr.flags, 0, sizeof(Scr.flags));

  /* create graphics contexts */
  CreateGCs();

  if (Pdepth <= 8) {               /* if the color is limited */
    /* a number > than the builtin table! */
    Scr.ColorLimit = 255;
  }
  Scr.FvwmRoot.w = Scr.Root;
  Scr.FvwmRoot.next = 0;

  init_stack_and_layers();

  XGetWindowAttributes(dpy,Scr.Root,&(Scr.FvwmRoot.attr));
  Scr.root_pushes = 0;
  Scr.fvwm_pushes = 0;
  Scr.pushed_window = &Scr.FvwmRoot;
  Scr.FvwmRoot.number_cmap_windows = 0;


  Scr.MyDisplayWidth = DisplayWidth(dpy, Scr.screen);
  Scr.MyDisplayHeight = DisplayHeight(dpy, Scr.screen);

  Scr.NoBoundaryWidth = 1;
  Scr.BoundaryWidth = BOUNDARY_WIDTH;
  Scr.Hilite = NULL;
  Scr.Focus = NULL;
  Scr.PreviousFocus = NULL;
  Scr.Ungrabbed = NULL;

  Scr.StdFont.font = NULL;
  Scr.IconFont.font = NULL;

  Scr.VxMax = 2*Scr.MyDisplayWidth;
  Scr.VyMax = 2*Scr.MyDisplayHeight;

  Scr.Vx = Scr.Vy = 0;

  Scr.shade_anim_steps = 0;

  Scr.SizeWindow = None;

  /* Sets the current desktop number to zero */
  /* Multiple desks are available even in non-virtual
   * compilations */
  Scr.CurrentDesk = 0;

  Scr.EdgeScrollX = Scr.EdgeScrollY = 100;
  Scr.ScrollResistance = Scr.MoveResistance = 0;
  Scr.SnapAttraction = DEFAULT_SNAP_ATTRACTION;
  Scr.SnapMode = DEFAULT_SNAP_ATTRACTION_MODE;
  Scr.SnapGridX = DEFAULT_SNAP_GRID_X;
  Scr.SnapGridY = DEFAULT_SNAP_GRID_Y;
  Scr.OpaqueSize = DEFAULT_OPAQUE_MOVE_SIZE;
  Scr.MoveSmoothness = DEFAULT_MOVE_SMOOTHNESS;
  Scr.MoveThreshold = DEFAULT_MOVE_THRESHOLD;
  /* ClickTime is set to the positive value upon entering the event loop. */
  Scr.ClickTime = -DEFAULT_CLICKTIME;
  Scr.ColormapFocus = COLORMAP_FOLLOWS_MOUSE;

  /* set major operating modes */
  Scr.NumBoxes = 0;

  Scr.randomx = Scr.randomy = 0;
  Scr.buttons2grab = 7;

  InitFvwmDecor(&Scr.DefaultDecor);
#ifdef USEDECOR
  Scr.DefaultDecor.tag = "Default";
#endif

  Scr.go.WindowShadeScrolls = True;
  Scr.go.SmartPlacementIsClever = False;
  Scr.go.ClickToFocusPassesClick = True;
  Scr.go.ClickToFocusRaises = True;
  Scr.go.MouseFocusClickRaises = False;
  Scr.go.StipledTitles = False;

  /*  RBW - 11/02/1998    */
  Scr.go.ModifyUSP                          =  True;
  Scr.go.CaptureHonorsStartsOnPage          =  True;
  Scr.go.RecaptureHonorsStartsOnPage        =  False;
  Scr.go.ActivePlacementHonorsStartsOnPage  =  False;
  /* Initialize RaiseHackNeeded by identifying X servers
     possibly running under NT. This is probably not an
     ideal solution, since eg NCD also produces X servers
     which do not run under NT.

     "Hummingbird Communications Ltd."
        is the ServerVendor string of the Exceed X server under NT,

     "Network Computing Devices Inc."
        is the ServerVendor string of the PCXware X server under Windows.
  */
  Scr.bo.RaiseHackNeeded =
    (strcmp (ServerVendor (dpy), "Hummingbird Communications Ltd.") == 0) ||
    (strcmp (ServerVendor (dpy), "Network Computing Devices Inc.") == 0);

#ifdef MODALITY_IS_EVIL
  Scr.bo.ModalityIsEvil = 1;
#else
  Scr.bo.ModalityIsEvil = 0;
#endif
#ifdef DISABLE_CONFIGURE_NOTIFY_DURING_MOVE
  Scr.bo.DisableConfigureNotify = 1;
#else
  Scr.bo.DisableConfigureNotify = 0;
#endif

  Scr.gs.EmulateMWM = DEFAULT_EMULATE_MWM;
  Scr.gs.EmulateWIN = DEFAULT_EMULATE_WIN;
  Scr.gs.use_active_down_buttons = DEFAULT_USE_ACTIVE_DOWN_BUTTONS;
  Scr.gs.use_inactive_buttons = DEFAULT_USE_INACTIVE_BUTTONS;
  /* Not the right place for this, should only be called once somewhere .. */

  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	Reborder - Removes fvwm border windows
 *
 ************************************************************************/
void Reborder(void)
{
  FvwmWindow *tmp;			/* temp fvwm window structure */

  /* put a border back around all windows */
  MyXGrabServer (dpy);

  InstallWindowColormaps (&Scr.FvwmRoot);	/* force reinstall */
/*
    RBW - 05/15/1998
    Grab the last window and work backwards: preserve stacking order on restart.
*/
  for (tmp = Scr.FvwmRoot.stack_prev; tmp != &Scr.FvwmRoot;
       tmp = tmp->stack_prev)
  {
    RestoreWithdrawnLocation (tmp,True);
    XUnmapWindow(dpy,tmp->frame);
    XDestroyWindow(dpy,tmp->frame);
  }

  MyXUngrabServer (dpy);
  XSetInputFocus (dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
  XSync(dpy,0);

}

/***********************************************************************
 *
 *  Procedure:
 *	Done - tells FVWM to clean up and exit
 *
 ***********************************************************************
 */
RETSIGTYPE SigDone(int sig)
{
  fvwmRunState = FVWM_DONE;

  /*
   * This function might not return - it could "long-jump"
   * right out, so we need to do everything we need to do
   * BEFORE we call it ...
   */
  fvwmSetTerminate(sig);
}

/* if restart is true, command must not be NULL... */
void Done(int restart, char *command)
{
  const char *exitFuncName;

  if (!restart)
  {
    MoveViewport(0,0,False);
  }

  /* migo (03/Jul/1999): execute [Session]ExitFunction */
  exitFuncName = getInitFunctionName(2);
  if (FindFunction(exitFuncName))
  {
    char *action = strdup(CatString2("Function ", exitFuncName));
    ExecuteFunction(action, NULL, &Event, C_ROOT, -1, EXPAND_COMMAND);
    free(action);
  }

  /* Close all my pipes */
  ClosePipes();

  if (!restart)
  {
    Reborder();
  }

  CloseICCCM2();

  if (restart)
  {
    Bool doPreserveState = True;
    SaveDesktopState();

    if (command)
    {
      while (isspace(command[0])) command++;
      if (strncmp(command, "--dont-preserve-state", 21) == 0)
      {
        doPreserveState = False;
        command += 21;
        while (isspace(command[0])) command++;
      }
    }
    if (command[0] == '\0')
      command = NULL; /* native restart */

    /* won't return under SM on Restart without parameters */
    RestartInSession(restart_state_filename, command == NULL, doPreserveState);

    /*
      RBW - 06/08/1999 - without this, windows will wander to other pages on
      a Restart/Recapture because Restart gets the window position information
      out of sync. There may be a better way to do this (i.e., adjust the
      Restart code), but this works for now.
    */
    MoveViewport(0,0,False);
    Reborder ();

    /* Really make sure that the connection is closed and cleared! */
    XSelectInput(dpy, Scr.Root, 0 );
    XSync(dpy, 0);
    XCloseDisplay(dpy);

    /* really need to destroy all windows, explicitly,
     * not sleep, but this is adequate for now */
    sleep(1);
    ReapChildren();

    if (command)
    {
#define MAX_ARG_SIZE 25
#if 0
      /* This is not allowed by ANSI C! Must use a macro. */
      const int MAX_ARG_SIZE = 25;
#endif
      char *my_argv[MAX_ARG_SIZE];
      const char *errorMsg;
      int n = parseCommandArgs(command, my_argv, MAX_ARG_SIZE, &errorMsg);

      if (n <= 0)
      {
        fvwm_msg(ERR, "Done", "Restart command parsing error in (%s): [%s]",
          command, errorMsg);
      }
      else if (StrEquals(my_argv[0], "--pass-args"))
      {
        if (n != 2)
	{
          fvwm_msg(ERR, "Done",
		   "Restart --pass-args: single name expected. "
		   "(restarting '%s' instead)",
            g_argv[0]);

        }
	else
	{
          int i;
          my_argv[0] = my_argv[1];
          for (i = 1; i < g_argc && i < MAX_ARG_SIZE - 1; i++)
            my_argv[i] = g_argv[i];
          my_argv[i] = NULL;

          execvp(my_argv[0], my_argv);
          fvwm_msg(ERR, "Done",
		   "Call of '%s' failed! (restarting '%s' instead)",
            my_argv[0], g_argv[0]);
          perror("  system error description");
        }

      }
      else
      {
        execvp(my_argv[0], my_argv);
        fvwm_msg(ERR, "Done", "Call of '%s' failed! (restarting '%s' instead)",
          my_argv[0], g_argv[0]);
        perror("  system error description");
      }
    }

    execvp(g_argv[0], g_argv);    /* that _should_ work */
    fvwm_msg(ERR, "Done", "Call of '%s' failed!", g_argv[0]);
    perror("  system error description");
  }
  else
  {
    XCloseDisplay(dpy);
  }
  exit(0);
}

int CatchRedirectError(Display *dpy, XErrorEvent *event)
{
  fvwm_msg(ERR,"CatchRedirectError","another WM is running");
  exit(1);
}

/***********************************************************************
 *
 *  Procedure:
 *	CatchFatal - Shuts down if the server connection is lost
 *
 ************************************************************************/
int CatchFatal(Display *dpy)
{
  /* No action is taken because usually this action is caused by someone
     using "xlogout" to be able to switch between multiple window managers
     */
  ClosePipes();
  exit(1);
}

/***********************************************************************
 *
 *  Procedure:
 *	FvwmErrorHandler - displays info on internal errors
 *
 ************************************************************************/
int FvwmErrorHandler(Display *dpy, XErrorEvent *event)
{
  extern int last_event_type;

  /* some errors are acceptable, mostly they're caused by
   * trying to update a lost  window or free'ing another modules colors */
  if((event->error_code == BadWindow)||(event->request_code == X_GetGeometry)||
     (event->error_code==BadDrawable)||(event->request_code==X_SetInputFocus)||
     (event->request_code==X_GrabButton)||
     (event->request_code==X_ChangeWindowAttributes)||
     (event->request_code == X_InstallColormap) ||
     (event->request_code == X_FreePixmap) ||
     (event->request_code == X_FreeColors))
    return 0;

  fvwm_msg(ERR,"FvwmErrorHandler","*** internal error ***");
  fvwm_msg(ERR,"FvwmErrorHandler","Request %d, Error %d, EventType: %d",
           event->request_code,
           event->error_code,
           last_event_type);
  return 0;
}

static void usage(void)
{
#if 0
  fvwm_msg(INFO,"usage","\nFvwm Version %s Usage:\n\n",VERSION);
  fvwm_msg(INFO,"usage","  %s [-d dpy] [-debug] [-f config_cmd] [-s] [-blackout] [-version] [-h] [-replace] [-clientId id] [-restore file] [-visualId id] [-visual class]\n",g_argv[0]);
#else
  fprintf(stderr,"\nFvwm Version %s Usage:\n\n",VERSION);
  fprintf(stderr,"  %s [-d dpy] [-debug] [-f config_cmd] [-s] [-blackout] [-version] [-h] [-replace] [-clientId id] [-restore file] [-visualId id] [-visual class]\n\n",g_argv[0]);
#endif
  exit( 1 );
}

/****************************************************************************
 *
 * Save Desktop State
 *
 ****************************************************************************/
void SaveDesktopState()
{
  FvwmWindow *t;
  unsigned long data[1];

  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    data[0] = (unsigned long) t->Desk;
    XChangeProperty (dpy, t->w, _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32,
                     PropModeReplace, (unsigned char *) data, 1);
  }

  data[0] = (unsigned long) Scr.CurrentDesk;
  XChangeProperty (dpy, Scr.Root, _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32,
                   PropModeReplace, (unsigned char *) data, 1);

  XSync(dpy, 0);
}


void SetMWM_INFO(Window window)
{
  struct mwminfo
  {
    long flags;
    Window win;
  }  motif_wm_info;

  if (Scr.bo.ModalityIsEvil)
  {
    /* Set Motif WM_INFO atom to make motif relinquish
     * broken handling of modal dialogs */
    motif_wm_info.flags = 2;
    motif_wm_info.win = window;

    XChangeProperty(dpy,Scr.Root,_XA_MOTIF_WM,_XA_MOTIF_WM,32,
		    PropModeReplace,(char *)&motif_wm_info,2);
  }
}


/*
 * parseCommandArgs - parses a given command string into a given limited
 * argument array suitable for execv*. The parsing is similar to shell's.
 * Returns:
 *   positive number of parsed arguments - on success,
 *   0 - on empty command (only spaces),
 *   negative - on no command or parsing error.
 *
 * Any character can be quoted with a backslash (even inside single quotes).
 * Every command argument is separated by a space/tab/new-line from both sizes
 * or is at the start/end of the command. Sequential spaces are ignored.
 * An argument can be enclosed into single quotes (no further expanding)
 * or double quotes (expending environmental variables $VAR or ${VAR}).
 * The character '~' is expanded into user home directory (if not in quotes).
 *
 * In the current implementation, parsed arguments are stored in one
 * large static string pointed by returned argv[0], so they will be lost
 * on the next function call. This can be changed using dynamic allocation,
 * in this case the caller must free the string pointed by argv[0].
 */
static int parseCommandArgs(
  const char *command, char **argv, int maxArgc, const char **errorMsg)
{
  /* It is impossible to guess the exact length because of expanding */
#define MAX_TOTAL_ARG_LEN 256
  /* char *argString = safemalloc(MAX_TOTAL_ARG_LEN); */
  static char argString[MAX_TOTAL_ARG_LEN];
  int totalArgLen = 0;
  int errorCode = 0;
  int argc;
  char *aptr = argString;
  const char *cptr = command;
#define theChar (*cptr)
#define advChar (cptr++)
#define topChar (*cptr     == '\\'? *(cptr+1): *cptr)
#define popChar (*(cptr++) == '\\'? *(cptr++): *(cptr-1))
#define canAddArgChar (totalArgLen < MAX_TOTAL_ARG_LEN-1)
#define addArgChar(ch) (++totalArgLen, *(aptr++) = ch)
#define canAddArgStr(str) (totalArgLen < MAX_TOTAL_ARG_LEN-strlen(str))
#define addArgStr(str) {const char *tmp = str; while (*tmp) { addArgChar(*(tmp++)); }}

  *errorMsg = "";
  if (!command) { *errorMsg = "No command"; return -1; }
  for (argc = 0; argc < maxArgc - 1; argc++) {
    int sQuote = 0;
    argv[argc] = aptr;
    while (isspace(theChar)) advChar;
    if (theChar == '\0') break;
    while ((sQuote || !isspace(theChar)) && theChar != '\0' && canAddArgChar) {
      if (theChar == '"') {
        if (sQuote) { sQuote = 0; }
        else { sQuote = 1; }
        advChar;
      } else if (!sQuote && theChar == '\'') {
        advChar;
        while (theChar != '\'' && theChar != '\0' && canAddArgChar) {
          addArgChar(popChar);
        }
        if (theChar == '\'') advChar;
        else if (!canAddArgChar) break;
        else { *errorMsg = "No closing single quote"; errorCode = -3; break; }
      } else if (!sQuote && theChar == '~') {
        if (!canAddArgStr(user_home_dir)) break;
        addArgStr(user_home_dir);
        advChar;
      } else if (theChar == '$') {
        int beg, len;
        const char *str = getFirstEnv(cptr, &beg, &len);
        if (!str || beg) { addArgChar(theChar); advChar; continue; }
        if (!canAddArgStr(str)) { break; }
        addArgStr(str);
        cptr += len;
      } else {
        if (addArgChar(popChar) == '\0') break;
      }
    }
    if (*(aptr-1) == '\0')
    {
      *errorMsg = "Unexpected last backslash";
      errorCode = -2;
      break;
    }
    if (errorCode)
      break;
    if (theChar == '~' || theChar == '$' || !canAddArgChar)
    {
      *errorMsg = "The command is too long";
      errorCode = -argc - 100;
      break;
    }
    if (sQuote)
    {
      *errorMsg = "No closing double quote";
      errorCode = -4;
      break;
    }
    addArgChar('\0');
  }

#undef theChar
#undef advChar
#undef topChar
#undef popChar
#undef canAddArgChar
#undef addArgChar
#undef canAddArgStr
#undef addArgStr
  argv[argc] = NULL;
  if (argc == 0 && !errorCode) *errorMsg = "Void command";
  return errorCode? errorCode: argc;
}


/*
 * setInitFunctionName - sets one of the init, restart or exit function names
 * getInitFunctionName - gets one of the init, restart or exit function names
 *
 * First parameter defines a function type: 0 - init, 1 - restart, 2 - exit.
 */
static const char *initFunctionNames[4] = {
  "InitFunction", "RestartFunction", "ExitFunction", "Nop"
};

void setInitFunctionName(int n, const char *name)
{
  initFunctionNames[n >= 0 && n < 3? n: 3] = name;
}

const char *getInitFunctionName(int n)
{
  return initFunctionNames[n >= 0 && n < 3? n: 3];
}
