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
#include "FvwmIconMan.h"
#include "readconfig.h"
#include "x.h"
#include "xmanager.h"

static char const rcsid[] =
  "$Id$";

#define GRAB_EVENTS (ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|EnterWindowMask|LeaveWindowMask)

#ifdef SHAPE
static int shapeEventBase, shapeErrorBase;
#endif

Display *theDisplay;
Window theRoot;
int theScreen;

static enum {
  NOT_GRABBED = 0,
  NEED_TO_GRAB = 1,
  HAVE_GRABBED = 2
} grab_state = NOT_GRABBED;

static void reparentnotify_event (WinManager *man, XEvent *ev);

static void grab_pointer (WinManager *man)
{
  /* This should only be called after we get our EXPOSE event */
  if (grab_state == NEED_TO_GRAB) {
    if (XGrabPointer (theDisplay, man->theWindow, True, GRAB_EVENTS,
		      GrabModeAsync, GrabModeAsync, None,
		      None, CurrentTime) != GrabSuccess) {
      ConsoleMessage ("Couldn't grab pointer\n");
      ShutMeDown (0);
    }
    grab_state = HAVE_GRABBED;
  }
}

static int lookup_color (char *name, Pixel *ans)
{
  XColor color;

  color.pixel = 0;
  if (!XParseColor (theDisplay, Pcmap, name, &color)) {
    ConsoleDebug(X11, "Could not parse color '%s'\n", name);
    return 0;
  }
  else if(!XAllocColor (theDisplay, Pcmap, &color)) {
    ConsoleDebug(X11, "Could not allocate color '%s'\n", name);
    return 0;
  }
  *ans = color.pixel;
  return 1;
}


WinManager *find_windows_manager (Window win)
{
  int i;

  for (i = 0; i < globals.num_managers; i++) {
    if (globals.managers[i].theWindow == win)
      return &globals.managers[i];
  }

/*  ConsoleMessage ("error in find_windows_manager:\n");
  ConsoleMessage ("Window: %x\n", win);
  for (i = 0; i < globals.num_managers; i++) {
    ConsoleMessage ("manager: %d %x\n", i, globals.managers[i].theWindow);
  }
*/
  return NULL;
}

static void handle_buttonevent (XEvent *theEvent, WinManager *man)
{
  Button *b;
  WinData *win;
  unsigned int modifier;
  Binding *MouseEntry;

  b = xy_to_button (man, theEvent->xbutton.x, theEvent->xbutton.y);
  if (b && theEvent->xbutton.button >= 1 && theEvent->xbutton.button <= 3) {
    win = b->drawn_state.win;
    if (win != NULL) {
      ConsoleDebug (X11, "Found the window:\n");
      ConsoleDebug (X11, "\tid:        %ld\n", win->app_id);
      ConsoleDebug (X11, "\tdesknum:   %ld\n", win->desknum);
      ConsoleDebug (X11, "\tx, y:      %ld %ld\n", win->x, win->y);
      ConsoleDebug (X11, "\ticon:      %ld\n", (unsigned long)win->iconname);
      ConsoleDebug (X11, "\ticonified: %d\n", win->iconified);
      ConsoleDebug (X11, "\tcomplete:  %d\n", win->complete);

      modifier = (theEvent->xbutton.state & MODS_USED);
      /* need to search for an appropriate mouse binding */
      for (MouseEntry = man->bindings[MOUSE]; MouseEntry != NULL;
	   MouseEntry= MouseEntry->NextBinding) {
	if(((MouseEntry->Button_Key == theEvent->xbutton.button)||
	    (MouseEntry->Button_Key == 0))&&
	   ((MouseEntry->Modifier == AnyModifier)||
	    (MouseEntry->Modifier == (modifier& (~LockMask)))))
	{
	  Function *ftype = (Function *)(MouseEntry->Action2);
	  ConsoleDebug (X11, "\tgot a mouse binding\n");
	  if (ftype && ftype->func) {
	    run_function_list (ftype);
	    draw_managers();
	  }
	  break;
	}
      }
    }
  }
}

Window find_frame_window (Window win, int *off_x, int *off_y)
{
  Window root, parent, *junkw;
  unsigned int junki;
  XWindowAttributes attr;

  ConsoleDebug (X11, "In find_frame_window: 0x%lx\n", win);

  while (1) {
    XQueryTree (theDisplay, win, &root, &parent, &junkw, &junki);
    if (junkw)
      XFree (junkw);
    if (parent == root)
      break;
    XGetWindowAttributes (theDisplay, win, &attr);
    ConsoleDebug (X11, "Adding (%d, %d) += (%d, %d)\n",
		  *off_x, *off_y, attr.x + attr.border_width,
		  attr.y + attr.border_width);
    *off_x += attr.x + attr.border_width;
    *off_y += attr.y + attr.border_width;
    win = parent;
  }

  return win;
}

/***************************************************************************/
/*  Event handler routines                                                 */
/*    everything which has its own routine can be processed recursively    */
/***************************************************************************/

static void reparentnotify_event (WinManager *man, XEvent *ev)
{
  ConsoleDebug (X11, "XEVENT: ReparentNotify\n");
#if 0
  ConsoleMessage ("seen %d expected %d\n", num_reparents_seen,
		  num_reparents_expected);
  man->off_x = ev->xreparent.x;
  man->off_y = ev->xreparent.y;
  ConsoleDebug (X11, "reparent: off = (%d, %d)\n",man->off_x, man->off_y);
  man->theFrame = find_frame_window (ev->xany.window,
				     &man->off_x, &man->off_y);
  ConsoleMessage ("\twin = 0x%x, frame = 0x%x\n", man->theWindow,
		  man->theFrame);
#endif
  if (man->can_draw == 0) {
    man->can_draw = 1;
    force_manager_redraw (man);
  }
}

void xevent_loop (void)
{
  XEvent theEvent;
  unsigned int modifier;
  Binding *key;
  Button *b;
  static int flag = 0;
  WinManager *man;

  if (flag == 0) {
    flag = 1;
    ConsoleDebug (X11, "A virgin event loop\n");
  }
  while (XPending (theDisplay)) {
    XNextEvent (theDisplay, &theEvent);
    if (theEvent.type == MappingNotify) {
      ConsoleDebug (X11, "XEVENT: MappingNotify\n");
      continue;
    }

    man = find_windows_manager (theEvent.xany.window);
    if (!man) {
      ConsoleDebug (X11, "Event doesn't belong to a manager\n");
      continue;
    }

    switch (theEvent.type) {
    case ReparentNotify:
      reparentnotify_event (man, &theEvent);
      break;

    case KeyPress:
      ConsoleDebug (X11, "XEVENT: KeyPress\n");
      /* Here's a real hack - some systems have two keys with the
       * same keysym and different keycodes. This converts all
       * the cases to one keycode. */
      theEvent.xkey.keycode =
	XKeysymToKeycode (theDisplay,
			  XKeycodeToKeysym(theDisplay,
					   theEvent.xkey.keycode,0));
      modifier = (theEvent.xkey.state & MODS_USED);
      ConsoleDebug (X11, "\tKeyPress: %d\n", theEvent.xkey.keycode);

      for (key = man->bindings[KEYPRESS]; key != NULL; key = key->NextBinding)
      {
	if ((key->Button_Key == theEvent.xkey.keycode) &&
	    ((key->Modifier == (modifier&(~LockMask)))||
	     (key->Modifier == AnyModifier)))
	{
	  Function *ftype = (Function *)(key->Action2);
	  if (ftype && ftype->func) {
	    run_function_list (ftype);
	    draw_managers();
	  }
	  break;
	}
      }
    break;

    case Expose:
      ConsoleDebug (X11, "XEVENT: Expose\n");
      if (theEvent.xexpose.count == 0) {
	man_exposed (man, &theEvent);
	draw_manager (man);
	if (globals.transient) {
	  grab_pointer (man);
	}
      }
      break;

    case ButtonPress:
      ConsoleDebug (X11, "XEVENT: ButtonPress\n");
      if (!globals.transient)
	handle_buttonevent (&theEvent, man);
      break;

    case ButtonRelease:
      ConsoleDebug (X11, "XEVENT: ButtonRelease\n");
      if (globals.transient) {
	handle_buttonevent (&theEvent, man);
	ShutMeDown (0);
      }
      break;

    case EnterNotify:
      ConsoleDebug (X11, "XEVENT: EnterNotify\n");
      man->cursor_in_window = 1;
      b = xy_to_button (man, theEvent.xcrossing.x, theEvent.xcrossing.y);
      move_highlight (man, b);
      run_binding (man, SELECT);
      draw_managers();
      break;

    case LeaveNotify:
      ConsoleDebug (X11, "XEVENT: LeaveNotify\n");
      move_highlight (man, NULL);
      break;

    case ConfigureNotify:
      ConsoleDebug (X11, "XEVENT: Configure Notify: %d %d %d %d\n",
		    theEvent.xconfigure.x, theEvent.xconfigure.y,
		    theEvent.xconfigure.width, theEvent.xconfigure.height);
      ConsoleDebug (X11, "\tcurrent geometry: %d %d %d %d\n",
		    man->geometry.x, man->geometry.y,
		    man->geometry.width, man->geometry.height);
      ConsoleDebug (X11, "\tborderwidth = %d\n",
		    theEvent.xconfigure.border_width);
      ConsoleDebug (X11, "\tsendevent = %d\n", theEvent.xconfigure.send_event);
#if 0
      set_manager_width (man, theEvent.xconfigure.width);
      ConsoleDebug (X11, "\tboxwidth = %d\n", man->geometry.boxwidth);
      draw_manager (man);

      /* pointer may not be in the same box as before */
      if (XQueryPointer (theDisplay, man->theWindow, &root, &child, &glob_x,
			 &glob_y,
			 &x, &y, &mask)) {
	b = xy_to_button (man, x, y);
	if (b != man->select_button) {
	  move_highlight (man, b);
	  run_binding (man, SELECT);
	  draw_managers();
	}
      }
      else {
	if (man->select_button != NULL)
	  move_highlight (NULL, NULL);
      }
#endif
      break;

    case MotionNotify:
      /* ConsoleDebug (X11, "XEVENT: MotionNotify\n");  */
      b = xy_to_button (man, theEvent.xmotion.x, theEvent.xmotion.y);
      if (b != man->select_button) {
	ConsoleDebug (X11, "\tmoving select\n");
	move_highlight (man, b);
	run_binding (man, SELECT);
	draw_managers();
      }
      break;

    case MapNotify:
      ConsoleDebug (X11, "XEVENT: MapNotify\n");
      force_manager_redraw (man);
      break;

    case UnmapNotify:
      ConsoleDebug (X11, "XEVENT: UnmapNotify\n");
      break;

    case DestroyNotify:
      ConsoleDebug(X11, "XEVENT: DestroyNotify\n");
      DeadPipe(0);
      break;

    default:
#ifdef SHAPE
      if (theEvent.type == shapeEventBase + ShapeNotify) {
	XShapeEvent *xev = (XShapeEvent *)&theEvent;
	ConsoleDebug (X11, "XEVENT: ShapeNotify\n");
	ConsoleDebug (X11, "\tx, y, w, h = %d, %d, %d, %d\n",
		      xev->x, xev->y, xev->width, xev->height);
	break;
      }
#endif
      ConsoleDebug (X11, "XEVENT: unknown\n");
      break;
    }
  }
  check_managers_consistency();
  XFlush (theDisplay);
}

static void set_window_properties (Window win, char *name, char *icon,
				   XSizeHints *sizehints)
{
  XTextProperty win_name;
  XTextProperty win_icon;
  XClassHint class;
  XWMHints wmhints = {0};

  wmhints.initial_state = NormalState;
  wmhints.flags = StateHint;

  if (XStringListToTextProperty (&name, 1, &win_name) == 0) {
    ConsoleMessage ("%s: cannot allocate window name.\n",Module);
    return;
  }
  if (XStringListToTextProperty (&icon, 1, &win_icon) == 0) {
    ConsoleMessage ("%s: cannot allocate window icon.\n",Module);
    return;
  }

  class.res_name = Module + 1;
  class.res_class = "FvwmModule";


  XSetWMProperties (theDisplay, win, &win_name, &win_icon, NULL, 0,
		    sizehints, &wmhints, &class);

  XFree (win_name.value);
  XFree (win_icon.value);
}

static int load_default_context_fore (WinManager *man, int i)
{
  int j = 0;

  if (Pdepth > 2)
    j = 1;

  ConsoleDebug (X11, "Loading: %s\n", contextDefaults[i].backcolor[j]);

  return lookup_color (contextDefaults[i].forecolor[j], &man->forecolor[i]);
}

static int load_default_context_back (WinManager *man, int i)
{
  int j = 0;

  if (Pdepth > 2)
    j = 1;

  ConsoleDebug (X11, "Loading: %s\n", contextDefaults[i].backcolor[j]);

  return lookup_color (contextDefaults[i].backcolor[j], &man->backcolor[i]);
}

void map_manager (WinManager *man)
{
  if (man->window_mapped == 0 && man->geometry.height > 0) {
    XMapWindow (theDisplay, man->theWindow);
    set_manager_window_mapping (man, 1);
    XFlush (theDisplay);
    if (globals.transient) {
      /* wait for an expose event to actually do the grab */
      grab_state = NEED_TO_GRAB;
    }
  }
}

void unmap_manager (WinManager *man)
{
  if (man->window_mapped == 1) {
    XUnmapWindow (theDisplay, man->theWindow);
    set_manager_window_mapping (man, 0);
    XFlush (theDisplay);
  }
}

#if 0
void read_all_reparent_events (WinManager *man, int block)
{
  XEvent evs[2];
  int i = 0, got_one = 0, the_event;

  /* We're going to be junking ConfigureNotify events, but that's ok,
     since this is only going to be called from resize_manager() */
  /* This shouldn't slow things down terribly, since we'd just have to read
     these events anyway */

  assert (man->can_draw);

  if (block) {
    while (1) {
      XWindowEvent (theDisplay, man->theWindow, StructureNotifyMask, &evs[0]);
      if (evs[0].type == ReparentNotify) {
	the_event = 0;
	got_one = 1;
	break;
      }
    }
  }
  else {
    while (XCheckWindowEvent (theDisplay, man->theWindow, StructureNotifyMask,
			      &evs[i])) {
      if (evs[i].type == ReparentNotify) {
	got_one = 1;
	the_event = i;
	i ^= 1;
      }
    }
  }
  if (got_one) {
    reparentnotify_event (man, &evs[the_event]);
  }
}
#endif

void X_init_manager (int man_id)
{
  WinManager *man;
  unsigned int width, height;
  int i, x, y, geometry_mask;
#ifdef I18N_MB
  char **ml;
  int mc;
  char *ds;
  XFontStruct **fs_list;
#endif
  ConsoleDebug (X11, "In X_init_manager\n");

  man = &globals.managers[man_id];

  man->geometry.cols = DEFAULT_NUM_COLS;
  man->geometry.rows = DEFAULT_NUM_ROWS;
  man->geometry.x = 0;
  man->geometry.y = 0;
  man->gravity = NorthWestGravity;

  man->select_button = NULL;
  man->cursor_in_window = 0;
  man->sizehints_flags = 0;

  ConsoleDebug (X11, "boxwidth = %d\n", man->geometry.boxwidth);

  if (man->fontname) {
#ifdef I18N_MB
    man->ButtonFontset = XCreateFontSet (theDisplay, man->fontname, &ml, &mc, &ds);
    if (!man->ButtonFontset) {
      if (!(man->ButtonFontset = XCreateFontSet (theDisplay, FONT_STRING, &ml, &mc, &ds))) {
        ConsoleMessage ("Can't get fontset\n");
        ShutMeDown (1);
      }
    }
    XFontsOfFontSet(man->ButtonFontset,&fs_list,&ml);
    man->ButtonFont = fs_list[0];
#else
    man->ButtonFont = XLoadQueryFont (theDisplay, man->fontname);
    if (!man->ButtonFont) {
      if (!(man->ButtonFont = XLoadQueryFont (theDisplay, FONT_STRING))) {
	ConsoleMessage ("Can't get font\n");
	ShutMeDown (1);
      }
    }
#endif
  }
  else {
#ifdef I18N_MB
    if (!(man->ButtonFontset = XCreateFontSet (theDisplay, FONT_STRING, &ml, &mc, &ds))) {
      ConsoleMessage ("Can't get fontset\n");
      ShutMeDown (1);
    }
    XFontsOfFontSet(man->ButtonFontset,&fs_list,&ml);
    man->ButtonFont = fs_list[0];
#else
    if (!(man->ButtonFont = XLoadQueryFont (theDisplay, FONT_STRING))) {
      ConsoleMessage ("Can't get font\n");
      ShutMeDown (1);
    }
#endif
  }

  for ( i = 0; i < NUM_CONTEXTS; i++ ) {
    if (man->backColorName[i]) {
      if (!lookup_color (man->backColorName[i], &man->backcolor[i])) {
        if (!load_default_context_back (man, i)) {
	  ConsoleMessage ("Can't load %s background color\n",
			  contextDefaults[i].name);
        }
      }
    }
    else if (!load_default_context_back (man, i)) {
      ConsoleMessage ("Can't load %s background color\n",
		      contextDefaults[i].name);
    }

    if (man->foreColorName[i]) {
      if (!lookup_color (man->foreColorName[i], &man->forecolor[i])) {
        if (!load_default_context_fore (man, i)) {
    	ConsoleMessage ("Can't load %s foreground color\n",
			contextDefaults[i].name);
        }
      }
    }
    else if (!load_default_context_fore (man, i)) {
      ConsoleMessage ("Can't load %s foreground color\n",
		      contextDefaults[i].name);
    }

    if (Pdepth > 2) {
      man->shadowcolor[i] = GetShadow(man->backcolor[i]);
      man->hicolor[i] = GetHilite(man->backcolor[i]);
#if 0
      /* thing about message id bg vs fg */
      if (!lookup_shadow_color (man->backcolor[i], &man->shadowcolor[i])) {
	ConsoleMessage ("Can't load %s shadow color\n",
			contextDefaults[i].name);
      }
      if (!lookup_hilite_color (man->backcolor[i], &man->hicolor[i])) {
	ConsoleMessage ("Can't load %s hilite color\n",
			contextDefaults[i].name);
      }
#endif
    }
  }

  man->fontheight = man->ButtonFont->ascent +
    man->ButtonFont->descent;

  /* silly hack to guess the minimum char width of the font
     doesn't have to be perfect. */

  man->fontwidth = XTextWidth (man->ButtonFont, ".", 1);

  /* First:  get button geometry
     Second: get geometry from geometry string
     Third:  determine the final width and height
     Fourth: determine final x, y coords */

  man->geometry.boxwidth = DEFAULT_BUTTON_WIDTH;
  man->geometry.boxheight = man->fontheight + 4;;
  man->geometry.dir = 0;

  geometry_mask = 0;

  if (man->button_geometry_str) {
    int val;
    val = XParseGeometry (man->button_geometry_str, &x, &y, &width, &height);
    ConsoleDebug (X11, "button x, y, w, h = %d %d %d %d\n", x, y, width,
		  height);
    if (val & WidthValue)
      man->geometry.boxwidth = width;
    if (val & HeightValue)
      man->geometry.boxheight = max (man->geometry.boxheight, height);
  }
  if (man->geometry_str) {
    geometry_mask = XParseGeometry (man->geometry_str, &man->geometry.x,
				    &man->geometry.y, &man->geometry.cols,
				    &man->geometry.rows);

    if ((geometry_mask & XValue) || (geometry_mask & YValue)) {
      man->sizehints_flags |= USPosition;
      if (geometry_mask & XNegative)
	man->geometry.dir |= GROW_LEFT;
      else
	man->geometry.dir |= GROW_RIGHT;
      if (geometry_mask & YNegative)
	man->geometry.dir |= GROW_UP;
      else
	man->geometry.dir |= GROW_DOWN;
    }
  }

  if (man->geometry.rows == 0) {
    if (man->geometry.cols == 0) {
      ConsoleMessage ("You specified a 0x0 window\n");
      ShutMeDown (0);
    }
    else {
      man->geometry.dir |= GROW_VERT;
    }
    man->geometry.rows = 1;
  }
  else {
    if (man->geometry.cols == 0) {
      man->geometry.dir |= GROW_HORIZ;
      man->geometry.cols = 1;
    }
    else {
      man->geometry.dir |= GROW_HORIZ | GROW_FIXED;
    }
  }

  man->geometry.width  = man->geometry.cols * man->geometry.boxwidth;
  man->geometry.height = man->geometry.rows * man->geometry.boxheight;

  if ((geometry_mask & XValue) && (geometry_mask & XNegative))
    man->geometry.x += globals.screenx - man->geometry.width;
  if ((geometry_mask & YValue) && (geometry_mask & YNegative))
    man->geometry.y += globals.screeny - man->geometry.height;

  if (globals.transient) {
    Window dummyroot, dummychild;
    int junk;
    unsigned int ujunk;

    XQueryPointer(theDisplay, theRoot, &dummyroot, &dummychild,
		  &man->geometry.x,
		  &man->geometry.y, &junk, &junk, &ujunk);
    man->geometry.dir |= GROW_DOWN | GROW_RIGHT;
    man->sizehints_flags |= USPosition;
  }

  if (man->sizehints_flags & USPosition) {
    if (man->geometry.dir & GROW_DOWN) {
      if (man->geometry.dir & GROW_RIGHT)
	man->gravity = NorthWestGravity;
      else if (man->geometry.dir & GROW_LEFT)
	man->gravity = NorthEastGravity;
      else
	ConsoleMessage ("Internal error in X_init_manager\n");
    }
    else if (man->geometry.dir & GROW_UP) {
      if (man->geometry.dir & GROW_RIGHT)
	man->gravity = SouthWestGravity;
      else if (man->geometry.dir & GROW_LEFT)
	man->gravity = SouthEastGravity;
      else
	ConsoleMessage ("Internal error in X_init_manager\n");
    }
    else {
      ConsoleMessage ("Internal error in X_init_manager\n");
    }
  }
  else {
    man->geometry.dir |= GROW_DOWN | GROW_RIGHT;
    man->gravity = NorthWestGravity;
  }
}

void create_manager_window (int man_id)
{
  XSizeHints sizehints;
  XGCValues gcval;
  unsigned long gcmask = 0;
  unsigned long winattrmask = CWBackPixel | CWBorderPixel | CWEventMask |
    CWBackingStore | CWBitGravity | CWColormap;
  XSetWindowAttributes winattr;
  unsigned int line_width = 1;
  int line_style = LineSolid;
  int cap_style = CapRound;
  int join_style = JoinRound;
  int i;
  WinManager *man;
  ConsoleDebug (X11, "In create_manager_window\n");

  man = &globals.managers[man_id];

  if (man->window_up)
    return;

  size_manager (man);

  sizehints.flags = man->sizehints_flags;


  sizehints.base_width = sizehints.width = man->geometry.width;
  sizehints.base_height = sizehints.height = man->geometry.height;
  sizehints.min_width = 0;
  sizehints.max_width = globals.screenx;
  sizehints.min_height = man->geometry.height;
  sizehints.max_height = man->geometry.height;
  sizehints.win_gravity = man->gravity;
  sizehints.flags |= PBaseSize | PMinSize | PMaxSize | PWinGravity;
  sizehints.x = man->geometry.x;
  sizehints.y = man->geometry.y;


  ConsoleDebug (X11, "hints: x, y, w, h = %d %d %d %d)\n",
		sizehints.x, sizehints.y,
		sizehints.base_width, sizehints.base_height);
  ConsoleDebug (X11, "gravity: %d %d\n", sizehints.win_gravity, man->gravity);


  winattr.background_pixel = man->backcolor[PLAIN_CONTEXT];
  winattr.border_pixel = man->forecolor[PLAIN_CONTEXT];
  winattr.backing_store = WhenMapped;
  winattr.bit_gravity = man->gravity;
  winattr.colormap = Pcmap;
  winattr.event_mask = ExposureMask | PointerMotionMask | EnterWindowMask |
    LeaveWindowMask | KeyPressMask | StructureNotifyMask;

  if (globals.transient)
    winattr.event_mask |= ButtonReleaseMask;
  else
    winattr.event_mask |= ButtonPressMask;

  man->theWindow = XCreateWindow(theDisplay, theRoot, sizehints.x, sizehints.y,
				 man->geometry.width, man->geometry.height,
				 0, Pdepth, InputOutput, Pvisual, winattrmask,
				 &winattr);
#ifdef SHAPE
  XShapeSelectInput (theDisplay, man->theWindow, ShapeNotifyMask);
#endif

  /* We really want the bit gravity to be NorthWest, so that can minimize
     redraws. But, we had to have an appropriate bit gravity when creating
     the window so that fvwm would place the window correctly if it has
     a border. I don't know if I should wait until an event is received
     before doing this. Sigh */
  winattr.bit_gravity = NorthWestGravity;
  XChangeWindowAttributes (theDisplay, man->theWindow,
			   CWBitGravity, &winattr);
  set_shape (man);

  man->theFrame = man->theWindow;
  man->off_x = 0;
  man->off_y = 0;

  for (i = 0; i < NUM_CONTEXTS; i++) {
    man->backContext[i] =
      XCreateGC (theDisplay, man->theWindow, gcmask, &gcval);
    XSetForeground (theDisplay, man->backContext[i], man->backcolor[i]);
    XSetLineAttributes (theDisplay, man->backContext[i], line_width,
			line_style, cap_style,
			join_style);

    man->hiContext[i] =
      XCreateGC (theDisplay, man->theWindow, gcmask, &gcval);
    XSetFont (theDisplay, man->hiContext[i], man->ButtonFont->fid);
    XSetForeground (theDisplay, man->hiContext[i], man->forecolor[i]);

    gcmask = GCForeground | GCBackground;
    gcval.foreground = man->backcolor[i];
    gcval.background = man->forecolor[i];
    man->flatContext[i] = XCreateGC (theDisplay, man->theWindow,
						 gcmask, &gcval);
    if (Pdepth > 2) {
      gcmask = GCForeground | GCBackground;
      gcval.foreground = man->hicolor[i];
      gcval.background = man->backcolor[i];
      man->reliefContext[i] = XCreateGC (theDisplay, man->theWindow,
						     gcmask, &gcval);

      gcmask = GCForeground | GCBackground;
      gcval.foreground = man->shadowcolor[i];
      gcval.background = man->backcolor[i];
      man->shadowContext[i] = XCreateGC (theDisplay, man->theWindow,
						     gcmask, &gcval);
    }
  }

  set_window_properties (man->theWindow, man->titlename,
			 man->iconname, &sizehints);
  man->window_up = 1;
  map_manager (man);
}

static int handle_error (Display *d, XErrorEvent *ev)
{
  ConsoleMessage ("X Error:\n");
  ConsoleMessage ("         error code: %d\n", ev->error_code);
  ConsoleMessage ("         request code: %d\n", ev->request_code);
  ConsoleMessage ("         minor code: %d\n", ev->minor_code);
  ConsoleMessage ("Leaving a core dump now\n");
  abort();
  return 0;
}

void init_display (void)
{
  theDisplay = XOpenDisplay ("");
  if (theDisplay == NULL) {
    ConsoleMessage ("Can't open display: %s\n", XDisplayName (""));
    ShutMeDown (1);
  }
  XSetErrorHandler (handle_error);
  InitPictureCMap (theDisplay);
  x_fd = XConnectionNumber (theDisplay);
  theScreen = DefaultScreen (theDisplay);
  theRoot = RootWindow (theDisplay, theScreen);
#ifdef TEST_MONO
  Pdepth = 2;
#endif
  globals.screenx = DisplayWidth (theDisplay, theScreen);
  globals.screeny = DisplayHeight (theDisplay, theScreen);

#ifdef SHAPE
  globals.shapes_supported = XShapeQueryExtension (theDisplay, &shapeEventBase,
						   &shapeErrorBase);
#endif

  ConsoleDebug (X11, "screen width: %ld\n", globals.screenx);
  ConsoleDebug (X11, "screen height: %ld\n", globals.screeny);
}
