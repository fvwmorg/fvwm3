#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

Display *dpy;
Window  Root, win;
int     screen;

int has_focus_proto = 0;
int has_delete_proto = 1;
int input_mode = -1;
 
Atom ATOM_NET_WM_WINDOW_TYPE           = None;
Atom ATOM_NET_WM_WINDOW_TYPE_DESKTOP   = None;
Atom ATOM_NET_WM_WINDOW_TYPE_DOCK      = None;
Atom ATOM_NET_WM_WINDOW_TYPE_TOOLBAR   = None;
Atom ATOM_NET_WM_WINDOW_TYPE_MENU      = None;
Atom ATOM_NET_WM_WINDOW_TYPE_DIALOG    = None;
Atom ATOM_NET_WM_WINDOW_TYPE_NORMAL    = None;
Atom ATOM_NET_WM_WINDOW_TYPE_SPLASH       = None;
Atom ATOM_NET_WM_WINDOW_TYPE_UTILITY      = None;
Atom ATOM_KDE_NET_WM_WINDOW_TYPE_OVERRIDE = None;

Atom ATOM_NET_WM_STATE                 = None;
Atom ATOM_NET_WM_STATE_MODAL           = None;
Atom ATOM_NET_WM_STATE_STICKY          = None;
Atom ATOM_NET_WM_STATE_MAXIMIZED_VERT  = None;
Atom ATOM_NET_WM_STATE_MAXIMIZED_HORIZ = None;
Atom ATOM_NET_WM_STATE_SHADED          = None;
Atom ATOM_NET_WM_STATE_SKIP_TASKBAR    = None;
Atom ATOM_NET_WM_STATE_SKIP_PAGER      = None;
Atom ATOM_NET_WM_STATE_HIDDEN          = None;
Atom ATOM_NET_WM_STATE_STAYS_ON_TOP    = None;
Atom ATOM_NET_WM_STATE_FULLSCREEN      = None;

Atom ATOM_NET_WM_DESKTOP = None;

/* wm protocol */
Atom ATOM_WM_DELETE_WINDOW = None;
Atom ATOM_WM_TAKE_FOCUS = None;

/* Motif window hints */
Atom ATOM_MOTIF_WM_HINTS = None;

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3) /* ? */

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

/* values for MwmHints.input_mode */
#define MWM_INPUT_MODELESS                      0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL     1
#define MWM_INPUT_SYSTEM_MODAL                  2
#define MWM_INPUT_FULL_APPLICATION_MODAL        3

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL                 (1L << 0)
#define MWM_DECOR_BORDER              (1L << 1)
#define MWM_DECOR_RESIZEH             (1L << 2)
#define MWM_DECOR_TITLE               (1L << 3)
#define MWM_DECOR_MENU                (1L << 4)
#define MWM_DECOR_MINIMIZE            (1L << 5)
#define MWM_DECOR_MAXIMIZE            (1L << 6)

/*
 * _MWM_HINTS property
 */
typedef struct {
    CARD32 flags;
    CARD32 functions;
    CARD32 decorations;
    INT32 inputMode;
  /*    CARD32 status; ???*/
} PropMotifWmHints;

typedef PropMotifWmHints PropMwmHints;

#define PROP_MOTIF_WM_HINTS_ELEMENTS  4
#define PROP_MWM_HINTS_ELEMENTS       PROP_MOTIF_WM_HINTS_ELEMENTS

void
Xloop(void)
{
  for(;;)
  {
    XEvent ev;
    XWindowAttributes xatt;

    /* Sit and wait for an event to happen */ 
    XNextEvent(dpy,&ev);
    switch(ev.type)
    {
    case ConfigureNotify:
      break;
    case ClientMessage:
      if (has_delete_proto && (ev.xclient.format==32) &&
	  (ev.xclient.data.l[0]== ATOM_WM_DELETE_WINDOW))
	exit(0);
      else if (has_focus_proto && ev.xclient.data.l[0]== ATOM_WM_TAKE_FOCUS)
      {
	printf("WM_TAKE_FOCUS message\n");
	if (input_mode == 1)
	{
	  printf ("\t...do nothing\n");
	}
	else if (input_mode == 0)
	{
	  if (XGetWindowAttributes (dpy, win, &xatt)
	      && (xatt.map_state == IsViewable))
	  {
	    printf ("\t...setting focus on our own: %lu\n",
		    ev.xclient.data.l[1]);
	    XSetInputFocus (dpy, win, RevertToParent, ev.xclient.data.l[1]);
	  }
	  else
	  {
	    printf ("\t...but we are not viewable\n");
	  }
	}
      }
      break;
    default:
      break;
    }
  }
}

#define XIA(a) XInternAtom(dpy,a,False);
void InitAtom(void)
{
  ATOM_NET_WM_WINDOW_TYPE           = XIA("_NET_WM_WINDOW_TYPE");  
  ATOM_NET_WM_WINDOW_TYPE_DESKTOP   = XIA("_NET_WM_WINDOW_TYPE_DESKTOP");
  ATOM_NET_WM_WINDOW_TYPE_DOCK      = XIA("_NET_WM_WINDOW_TYPE_DOCK");
  ATOM_NET_WM_WINDOW_TYPE_TOOLBAR   = XIA("_NET_WM_WINDOW_TYPE_TOOLBAR");
  ATOM_NET_WM_WINDOW_TYPE_MENU      = XIA("_NET_WM_WINDOW_TYPE_MENU");
  ATOM_NET_WM_WINDOW_TYPE_DIALOG    = XIA("_NET_WM_WINDOW_TYPE_DIALOG");
  ATOM_NET_WM_WINDOW_TYPE_NORMAL    = XIA("_NET_WM_WINDOW_TYPE_NORMAL");
  ATOM_NET_WM_WINDOW_TYPE_SPLASH    = XIA("_NET_WM_WINDOW_TYPE_SPLASH");
  ATOM_NET_WM_WINDOW_TYPE_UTILITY   = XIA("_NET_WM_WINDOW_TYPE_UTILITY");
  ATOM_KDE_NET_WM_WINDOW_TYPE_OVERRIDE = XIA("_KDE_NET_WM_WINDOW_TYPE_OVERRIDE");

  ATOM_NET_WM_STATE                 = XIA("_NET_WM_STATE");  
  ATOM_NET_WM_STATE_MODAL           = XIA("_NET_WM_STATE_MODAL");
  ATOM_NET_WM_STATE_STICKY          = XIA("_NET_WM_STATE_STICKY");  
  ATOM_NET_WM_STATE_MAXIMIZED_VERT  = XIA("_NET_WM_STATE_MAXIMIZED_VERT");
  ATOM_NET_WM_STATE_MAXIMIZED_HORIZ = XIA("_NET_WM_STATE_MAXIMIZED_HORIZ");
  ATOM_NET_WM_STATE_SHADED          = XIA("_NET_WM_STATE_SHADED");  
  ATOM_NET_WM_STATE_SKIP_TASKBAR    = XIA("_NET_WM_STATE_SKIP_TASKBAR");
  ATOM_NET_WM_STATE_SKIP_PAGER      = XIA("_NET_WM_STATE_SKIP_PAGER");
  ATOM_NET_WM_STATE_HIDDEN          = XIA("_NET_WM_STATE_HIDDEN");
  ATOM_NET_WM_STATE_FULLSCREEN      = XIA("_NET_WM_STATE_FULLSCREEN");
  ATOM_NET_WM_STATE_STAYS_ON_TOP    = XIA("_NET_WM_STATE_STAYS_ON_TOP");

  ATOM_NET_WM_DESKTOP               = XIA("_NET_WM_DESKTOP");

  ATOM_WM_DELETE_WINDOW = XIA("WM_DELETE_WINDOW");
  ATOM_WM_TAKE_FOCUS =    XIA("WM_TAKE_FOCUS");

  ATOM_MOTIF_WM_HINTS = XIA("_MOTIF_WM_HINTS"); 
}

int
main(int argc, char **argv)
{
  int state_count = 0;
  Atom states[10];
  Atom type = 0;
  int i,x_r,y_r,h_r,w_r,ret;
  int ewmh_state_arg = 0;
  int ewmh_type_arg = 0;
  int mwm_func_arg = 0;
  int mwm_decor_arg = 0;
  int has_ewmh_desktop = 0;
  Atom ewmh_desktop = 0;
  XSizeHints hints;
  XClassHint classhints;
  XWMHints wm_hints;
  PropMwmHints mwm_hints;
  Window trans_win = 0;

  if (!(dpy = XOpenDisplay("")))
  {
    fprintf(stderr,"can't open display\n");
    exit (1);
  }

  screen = DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);
  InitAtom();

  hints.width = 170;
  hints.height = 100;
  hints.x = 0;
  hints.y = 0;

  hints.flags = 0;

  wm_hints.flags = 0;
  mwm_hints.flags = 0;
  mwm_hints.functions = 0;
  mwm_hints.decorations = 0;
  mwm_hints.inputMode = 0;

  win = XCreateSimpleWindow(dpy, Root, 0, 0,
			    hints.width, hints.height, 0, 0, 0);

  for(i=1;i<argc;i++)
  {
    if (strcasecmp(argv[i],"-ewmh-state") == 0)
    {
      ewmh_state_arg = 1;
      ewmh_type_arg = 0;
      mwm_func_arg = 0;
      mwm_decor_arg = 0;
    }
    else if (strcasecmp(argv[i],"-ewmh-type") == 0)
    {
      ewmh_state_arg = 0;
      ewmh_type_arg = 1;
      mwm_func_arg = 0;
      mwm_decor_arg = 0;
    }
    if (strcasecmp(argv[i],"-mwm-func") == 0)
    {
      ewmh_state_arg = 0;
      ewmh_type_arg = 0;
      mwm_func_arg = 1;
      mwm_decor_arg = 0;
    }
    else if (strcasecmp(argv[i],"-mwm-decor") == 0)
    {
      ewmh_state_arg = 0;
      ewmh_type_arg = 0;
      mwm_func_arg = 0;
      mwm_decor_arg = 1;
    }
    else if (strcasecmp(argv[i],"-min-size") == 0)
    {
      i++;
      hints.min_width = atoi(argv[i]);
      i++;
      hints.min_height = atoi(argv[i]);
      hints.flags |= PMinSize;
    }
    else if (strcasecmp(argv[i],"-max-size") == 0)
    {
      i++;
      hints.max_width = atoi(argv[i]);
      i++;
      hints.max_height = atoi(argv[i]);
      hints.flags |= PMaxSize;
    }
    else if (strcasecmp(argv[i],"-inc-size") == 0)
    {
      i++;
      hints.width_inc = atoi(argv[i]);
      i++;
      hints.height_inc = atoi(argv[i]);
      hints.flags |= PResizeInc;
    }
    else if (strcasecmp(argv[i],"-pg") == 0)
    {
      i++;
      ret = XParseGeometry(argv[i], &x_r, &y_r, &w_r, &h_r);
      if ((ret & WidthValue) && (ret & HeightValue))
      {
	hints.width = w_r;
	hints.height = h_r;
	hints.flags |= PSize;
      }
      if ((ret & XValue) && (ret & YValue))
      {
	hints.x = x_r;
	hints.y = y_r;
	hints.win_gravity=NorthWestGravity;
	if (ret & XNegative)
	{
	  hints.x += XDisplayWidth(dpy,screen) - hints.width;
	  hints.win_gravity=NorthEastGravity;
	}
	if (ret & YNegative)
	{
	  hints.y += XDisplayHeight(dpy,screen) - hints.height;
	  if (ret & XNegative)
	  {
	    hints.win_gravity=SouthEastGravity;
	  }
	  else
	  {
	    hints.win_gravity=SouthWestGravity;
	  }
	  hints.flags |= PPosition|PWinGravity;
	}
      }
    }
    else if (strcasecmp(argv[i],"-ug") == 0)
    {
      i++;
      ret = XParseGeometry(argv[i], &x_r, &y_r, &w_r, &h_r);
      if ((ret & WidthValue) && (ret & HeightValue))
      {
	hints.width = w_r;
	hints.height = h_r;
	hints.flags |= USSize;
      }
      if ((ret & XValue) && (ret & YValue))
      {
	hints.x = x_r;
	hints.y = y_r;
	hints.win_gravity=NorthWestGravity;
	if (ret & XNegative)
	{
	  hints.x += XDisplayWidth(dpy,screen) - hints.width;
	  hints.win_gravity=NorthEastGravity;
	}
	if (ret & YNegative)
	{
	  hints.y += XDisplayHeight(dpy,screen) - hints.height;
	  if (ret & XNegative)
	  {
	    hints.win_gravity=SouthEastGravity;
	  }
	  else
	  {
	    hints.win_gravity=SouthWestGravity;
	  }
	}
	hints.flags |= USPosition|PWinGravity;
      }
    }
    else if (strcasecmp(argv[i],"-input") == 0)
    {
      i++;
      if (strcasecmp(argv[i],"true") == 0)
      {
	wm_hints.input = input_mode = True;
	wm_hints.flags |= InputHint;
      }
      else if (strcasecmp(argv[i],"false") == 0)
      {
	wm_hints.input = input_mode = False;
	wm_hints.flags |= InputHint;
      }
    }
    else if (strcasecmp(argv[i],"-focus-proto") == 0)
    {
      has_focus_proto = 1;
    }
    else if (strcasecmp(argv[i],"-no-delete-proto") == 0)
    {
      has_delete_proto = 0;
    }
    else if (strcasecmp(argv[i],"-wm-state") == 0)
    {
      wm_hints.flags |= StateHint;
      i++;
      if (strcasecmp(argv[i],"withdrawn") == 0)
      {
	wm_hints.initial_state = WithdrawnState;
      }
      else if (strcasecmp(argv[i],"normal") == 0)
      {
	wm_hints.initial_state = NormalState;
      }
      else if (strcasecmp(argv[i],"iconic") == 0)
      {
	wm_hints.initial_state = IconicState;
      }
    }
    else if (strcasecmp(argv[i],"-wm-urgency") == 0)
    {
      wm_hints.flags |= XUrgencyHint;
    }
    else if (strcasecmp(argv[i],"-wm-group") == 0)
    {
      wm_hints.flags |= WindowGroupHint;
      i++;
      if (strcasecmp(argv[i],"window") == 0)
      {
	wm_hints.window_group = win;
      }
      else if (strcasecmp(argv[i],"root") == 0)
      {
	wm_hints.window_group = Root;
      }
      else
      {
	wm_hints.window_group = strtoul(argv[i], NULL, 0);
      }
    }
    else if (strcasecmp(argv[i],"-transient") == 0)
    {
      i++;
      if (strcasecmp(argv[i],"root") == 0)
      {
	trans_win = Root;
      }
      else
      {
	trans_win = strtoul(argv[i], NULL, 0);
      }
    }
    else if (strcasecmp(argv[i],"-mwm-input") == 0)
    {
      mwm_hints.flags |= MWM_HINTS_INPUT_MODE;
      i++;
      if (strcasecmp(argv[i],"modless") == 0)
      {
	mwm_hints.inputMode = MWM_INPUT_MODELESS;
      }
      else if (strcasecmp(argv[i],"app_modal") == 0)
      {
	mwm_hints.inputMode = MWM_INPUT_PRIMARY_APPLICATION_MODAL;
      }
      else if (strcasecmp(argv[i],"sys_modal") == 0)
      {
	mwm_hints.inputMode = MWM_INPUT_SYSTEM_MODAL;
      }
      else if (strcasecmp(argv[i],"full_app_modal") == 0)
      {
	mwm_hints.inputMode = MWM_INPUT_FULL_APPLICATION_MODAL;
      }
    }
    else if (strcasecmp(argv[i],"-ewmh-desktop") == 0)
    {
      has_ewmh_desktop = 1;
      i++;
      ewmh_desktop = atol(argv[i]);
    }
    else if (ewmh_state_arg && state_count < 10)
    {
      if (strcasecmp(argv[i],"hidden") == 0)
      {
	states[state_count++] = ATOM_NET_WM_STATE_HIDDEN;
      }
      else if (strcasecmp(argv[i],"shaded") == 0)
      {
	states[state_count++] = ATOM_NET_WM_STATE_SHADED;
      }
      else if (strcasecmp(argv[i],"sticky") == 0)
      {
	states[state_count++] = ATOM_NET_WM_STATE_STICKY;
      }
      else if (strcasecmp(argv[i],"skippager") == 0)
      {
	states[state_count++] = ATOM_NET_WM_STATE_SKIP_PAGER;
      }
      else if (strcasecmp(argv[i],"skiptaskbar") == 0)
      {
	states[state_count++] = ATOM_NET_WM_STATE_SKIP_TASKBAR;
      }
      else if (strcasecmp(argv[i],"maxhoriz") == 0)
      {
	states[state_count++] = ATOM_NET_WM_STATE_MAXIMIZED_HORIZ;
      }
      else if (strcasecmp(argv[i],"maxvert") == 0)
      {
	states[state_count++] = ATOM_NET_WM_STATE_MAXIMIZED_VERT;
      }
      else if (strcasecmp(argv[i],"modal") == 0)
      {
	states[state_count++] = ATOM_NET_WM_STATE_MODAL;
      }
      else if (strcasecmp(argv[i],"staysontop") == 0)
      {
	states[state_count++] = ATOM_NET_WM_STATE_STAYS_ON_TOP;
      }
      else if (strcasecmp(argv[i],"fullscreen") == 0)
      {
	states[state_count++] = ATOM_NET_WM_STATE_FULLSCREEN;
      }
    }
    else if (ewmh_type_arg)
    {
      if (strcasecmp(argv[i],"normal") == 0)
      {
	type = ATOM_NET_WM_WINDOW_TYPE_NORMAL;
      }
      else if (strcasecmp(argv[i],"dock") == 0)
      {
	type = ATOM_NET_WM_WINDOW_TYPE_DOCK;
      }
      else if (strcasecmp(argv[i],"toolbar") == 0)
      {
	type = ATOM_NET_WM_WINDOW_TYPE_TOOLBAR;
      }
      else if (strcasecmp(argv[i],"desktop") == 0)
      {
	type = ATOM_NET_WM_WINDOW_TYPE_DESKTOP;
      }
      else if (strcasecmp(argv[i],"menu") == 0)
      {
	type = ATOM_NET_WM_WINDOW_TYPE_MENU;
      }
      else if (strcasecmp(argv[i],"dialog") == 0)
      {
	type = ATOM_NET_WM_WINDOW_TYPE_DIALOG;
      }
      else if (strcasecmp(argv[i],"splash") == 0)
      {
	type = ATOM_NET_WM_WINDOW_TYPE_SPLASH;
      }
      else if (strcasecmp(argv[i],"utility") == 0)
      {
	type = ATOM_NET_WM_WINDOW_TYPE_UTILITY;
      }
    }
    else if (mwm_func_arg)
    {
      mwm_hints.flags |= MWM_HINTS_FUNCTIONS;
      if (strcasecmp(argv[i],"all") == 0)
      {
	mwm_hints.functions |= MWM_FUNC_ALL;
      }
      else if (strcasecmp(argv[i],"resize") == 0)
      {
	mwm_hints.functions |= MWM_FUNC_RESIZE;
      }
      else if (strcasecmp(argv[i],"move") == 0)
      {
	mwm_hints.functions |= MWM_FUNC_MOVE;
      }
      else if (strcasecmp(argv[i],"minimize") == 0)
      {
	mwm_hints.functions |= MWM_FUNC_MINIMIZE;
      }
      else if (strcasecmp(argv[i],"maximize") == 0)
      {
	mwm_hints.functions |= MWM_FUNC_MAXIMIZE;
      }
      else if (strcasecmp(argv[i],"close") == 0)
      {
	mwm_hints.functions |= MWM_FUNC_CLOSE;
      }
    }
    else if (mwm_decor_arg)
    {
      mwm_hints.flags |= MWM_HINTS_DECORATIONS;
      if (strcasecmp(argv[i],"all") == 0)
      {
	mwm_hints.decorations |= MWM_DECOR_ALL;
      }
      else if (strcasecmp(argv[i],"border") == 0)
      {
	mwm_hints.decorations |= MWM_DECOR_BORDER;
      }
      else if (strcasecmp(argv[i],"resizeh") == 0)
      {
	mwm_hints.decorations |= MWM_DECOR_RESIZEH;
      }
      else if (strcasecmp(argv[i],"title") == 0)
      {
	mwm_hints.decorations |= MWM_DECOR_TITLE;
      }     
      else if (strcasecmp(argv[i],"menu") == 0)
      {
	mwm_hints.decorations |= MWM_DECOR_MENU;
      }     
      else if (strcasecmp(argv[i],"minimize") == 0)
      {
	mwm_hints.decorations |= MWM_DECOR_MINIMIZE;
      }
      else if (strcasecmp(argv[i],"maximize") == 0)
      {
	mwm_hints.decorations |= MWM_DECOR_MAXIMIZE;
      }
    }
  }


  XSelectInput(dpy,win,(StructureNotifyMask));

  if (wm_hints.flags)
  {
    XSetWMHints (dpy, win, &wm_hints);
  }

  if (state_count != 0)
  {
    XChangeProperty(dpy, win, ATOM_NET_WM_STATE, XA_ATOM, 32,
		    PropModeReplace,(unsigned char *)states, state_count);
  }

  if (type != 0)
  {
    XChangeProperty(dpy, win, ATOM_NET_WM_WINDOW_TYPE, XA_ATOM, 32,
		    PropModeReplace,(unsigned char *)&type, 1);
  }

  if (has_ewmh_desktop)
  {
    XChangeProperty(dpy, win, ATOM_NET_WM_DESKTOP, XA_CARDINAL, 32,
		    PropModeReplace,(unsigned char *)&ewmh_desktop, 1);
  }

  if (has_delete_proto || has_focus_proto) 
  {
    Atom proto[2];
    int j = 0;
     
    if (has_delete_proto)
      proto[j++] = ATOM_WM_DELETE_WINDOW;
    if (has_focus_proto)
      proto[j++] = ATOM_WM_TAKE_FOCUS;

    XSetWMProtocols(dpy, win, proto, j);
  }

  {
    XTextProperty nametext;
    char *list[]={NULL,NULL};
    list[0] = "hints_test";

    classhints.res_name=strdup("hints_test");
    classhints.res_class=strdup("HintsTest");

    if(!XStringListToTextProperty(list,1,&nametext))
    {
      fprintf(stderr,"Failed to convert name to XText\n");
      exit(1);
    }
    XSetWMProperties(dpy,win,&nametext,&nametext,
                    NULL,0,&hints,NULL,&classhints);
    XFree(nametext.value);
  }

  if (mwm_hints.flags != 0)
  {
    XChangeProperty(dpy, win, ATOM_MOTIF_WM_HINTS, ATOM_MOTIF_WM_HINTS, 32,
		    PropModeReplace,(unsigned char *)&mwm_hints,
		    PROP_MWM_HINTS_ELEMENTS);
  }
  if (trans_win !=0)
    XSetTransientForHint(dpy, win, trans_win);

  XMapWindow(dpy,win);
  XSetWindowBackground(dpy,win,0);

  Xloop();
  return 1;
}
