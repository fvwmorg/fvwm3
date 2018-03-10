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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"
#include "FvwmIconMan.h"
#include "readconfig.h"
#include "x.h"
#include "xmanager.h"
#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/PictureUtils.h"
#include "libs/FRender.h"
#include "libs/FRenderInit.h"
#include "libs/ColorUtils.h"
#include "libs/Graphics.h"
#include "libs/XError.h"

#define GRAB_EVENTS (ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|\
	EnterWindowMask|LeaveWindowMask)

Display *theDisplay;
Window theRoot;
int theScreen;
static Atom _XA_WM_DEL_WIN;

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
  return;
}


static int lookup_color (char *name, Pixel *ans)
{
#if 0
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
#else
  /* use the lib */
  *ans = GetSimpleColor(name);
  return 1;
#endif
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
  if (b && theEvent->xbutton.button >= 1 &&
      theEvent->xbutton.button <= NUMBER_OF_EXTENDED_MOUSE_BUTTONS) {
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
	    (MouseEntry->Modifier == (modifier& (~mods_unused)))))
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

  while (1)
  {
    if (!XQueryTree (theDisplay, win, &root, &parent, &junkw, &junki))
    {
      fprintf(stderr, "%s: find_frame_window: XQueryTree failed.\n", MyName);
      return win;
    }
    if (junkw)
      XFree (junkw);
    if (parent == root)
      break;
    if (!XGetWindowAttributes (theDisplay, win, &attr))
    {
      fprintf(stderr, "%s: find_frame_window: XGetWindowAttrigutes failed.\n",
	      MyName);
      return win;
    }
    ConsoleDebug (X11, "Adding (%d, %d) += (%d, %d)\n",
		  *off_x, *off_y, attr.x + attr.border_width,
		  attr.y + attr.border_width);
    *off_x += attr.x + attr.border_width;
    *off_y += attr.y + attr.border_width;
    win = parent;
  }

  return win;
}

/*
 *  Event handler routines
 *    everything which has its own routine can be processed recursively
 */

static void reparentnotify_event (WinManager *man, XEvent *ev)
{
  ConsoleDebug (X11, "XEVENT: ReparentNotify\n");
  if (man->can_draw == 0) {
    man->can_draw = 1;
    force_manager_redraw (man);
  }
}

void xevent_loop (void)
{
  XEvent theEvent;
  XEvent saveEvent;
  unsigned int modifier;
  Binding *key;
  Button *b;
  static int flag = 0;
  WinManager *man;
  Bool force_redraw = False;
  static int ex=10000, ey=10000, ex2=0, ey2=0;

  if (flag == 0) {
    flag = 1;
    ConsoleDebug (X11, "A virgin event loop\n");
  }
  while (FPending(theDisplay)) {
    FNextEvent(theDisplay, &theEvent);
    if (theEvent.type == MappingNotify) {
      ConsoleDebug (X11, "XEVENT: MappingNotify\n");
      continue;
    }

    if (FTipsHandleEvents(theDisplay, &theEvent))
    {
	    continue;
    }

    if (theEvent.type == ConfigureNotify)
    {
	    fev_sanitise_configure_notify(&theEvent.xconfigure);
    }
    man = find_windows_manager (theEvent.xany.window);
    if (!man && theEvent.type == ConfigureNotify)
    {
      /* swallowed icon man */
      man = find_windows_manager (theEvent.xconfigure.window);
    }
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
			  fvwm_KeycodeToKeysym(theDisplay,
					   theEvent.xkey.keycode,0,0));
      modifier = (theEvent.xkey.state & MODS_USED);
      ConsoleDebug (X11, "\tKeyPress: %d\n", theEvent.xkey.keycode);

      for (key = man->bindings[KEYPRESS]; key != NULL; key = key->NextBinding)
      {
	if ((key->Button_Key == theEvent.xkey.keycode) &&
	    ((key->Modifier == (modifier&(~mods_unused)))||
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
#if 0
      fprintf (stderr, "Expose: %i,%i,%i,%i %i\n",
	       theEvent.xexpose.x, theEvent.xexpose.y, theEvent.xexpose.width,
	       theEvent.xexpose.height, theEvent.xexpose.count);
#endif
      ex = min(ex, theEvent.xexpose.x);
      ey = min(ey, theEvent.xexpose.y);
      ex2 = max(ex2, theEvent.xexpose.x + theEvent.xexpose.width);
      ey2=max(ey2 , theEvent.xexpose.y + theEvent.xexpose.height);

      /* we use "clip redrawing": but this speedup redrawing without
       * adding to much flickering */
      while (FCheckTypedWindowEvent(
	      theDisplay, theEvent.xany.window, Expose, &theEvent))
      {
#if 0
	      fprintf (stderr, "\t\tExpose Purge: %i,%i,%i,%i %i\n",
		       theEvent.xexpose.x, theEvent.xexpose.y,
		       theEvent.xexpose.width, theEvent.xexpose.height,
		       theEvent.xexpose.count);
#endif
	      ex = min(ex, theEvent.xexpose.x);
	      ey = min(ey, theEvent.xexpose.y);
	      ex2 = max(ex2, theEvent.xexpose.x + theEvent.xexpose.width);
	      ey2= max(ey2 , theEvent.xexpose.y + theEvent.xexpose.height);
      }
#if 0
      fprintf (
	      stderr, "\tExpose Done: %i,%i,%i,%i %i\n",
	      ex, ey, ex2-ex, ey2-ey, theEvent.xexpose.count);
#endif
      if (theEvent.xexpose.count == 0)
      {
	      theEvent.xexpose.x = ex;
	      theEvent.xexpose.y = ey;
	      theEvent.xexpose.width = ex2 - ex;
	      theEvent.xexpose.height = ey2 -ey;
	      man_exposed (man, &theEvent);
	      draw_manager (man);
	      if (globals.transient) {
		      grab_pointer (man);
	      }

	      ex = ey = 10000;
	      ex2 = ey2 = 0;
      }
      break;

    case ButtonPress:
      ConsoleDebug (X11, "XEVENT: ButtonPress\n");
      tips_cancel(man);
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
      if (theEvent.xcrossing.mode == NotifyNormal)
      {
	      tips_on(man, b);
      }
      break;

    case LeaveNotify:
      ConsoleDebug (X11, "XEVENT: LeaveNotify\n");
      if (theEvent.xcrossing.mode == NotifyNormal)
      {
	      tips_cancel(man);
      }
      move_highlight (man, NULL);
      break;

    case ConfigureNotify:
    {
      /* when send_event is true it means a move has taken place, transparent
       * windows must be refreshed since the server won't do this,
       * we must not miss ones of these */
      Bool moved = theEvent.xconfigure.send_event;
      Bool recreate_bg = False;

      ConsoleDebug (X11, "\tcurrent geometry: %d %d %d %d\n",
		    man->geometry.x, man->geometry.y,
		    man->geometry.width, man->geometry.height);
      ConsoleDebug (X11, "\tborderwidth = %d\n",
		    theEvent.xconfigure.border_width);
      ConsoleDebug (X11, "\tsendevent = %d\n", theEvent.xconfigure.send_event);

      saveEvent = theEvent;
      /* eat up all but last ConfigureNotify events */
      while (FPending(theDisplay) &&
	     FCheckTypedEvent(theDisplay, ConfigureNotify, &theEvent))
      {
	fev_sanitise_configure_notify(&theEvent.xconfigure);
	saveEvent = theEvent;
	/* check for movement on all events */
	if (theEvent.xconfigure.send_event)
	  moved = True;
      }
      theEvent = saveEvent;
#if 0
      fprintf (stderr, "Configure: %i,%i,%i,%i %i\n",
	       theEvent.xconfigure.x, theEvent.xconfigure.y,
	       theEvent.xconfigure.width, theEvent.xconfigure.height,
	       moved);
#endif
       if ((man->geometry.width != theEvent.xconfigure.width ||
	   man->geometry.height != theEvent.xconfigure.height ||
	   (moved && CSET_IS_TRANSPARENT(man->colorsets[DEFAULT]))) &&
	  !CSET_IS_TRANSPARENT_PR_PURE(man->colorsets[DEFAULT]))
      {
	      recreate_bg = True;
	      if (moved)
	      {
		      man->geometry.x = theEvent.xconfigure.x;
		      man->geometry.y = theEvent.xconfigure.y;
		      tips_cancel(man);
	      }
      }

      if (man->geometry.width != theEvent.xconfigure.width ||
	  man->geometry.height != theEvent.xconfigure.height)
      {
	      tips_cancel(man);
	      if (man->geometry.dir & GROW_FIXED)
	      {
		      man->geometry.rows =
			      theEvent.xconfigure.height /
			      man->geometry.boxheight;
		      if (man->geometry.rows < 1)
		      {
			      man->geometry.rows = 1;
		      }
		      man->geometry.cols =
			      (man->buttons.num_windows - 1) /
			      man->geometry.rows+1;
	      }
	      man->geometry.width = theEvent.xconfigure.width;
	      man->geometry.height = theEvent.xconfigure.height;
	      ConsoleDebug (X11, "\tcurrent geometry: %d %d %d %d\n",
			    man->geometry.x, man->geometry.y,
			    man->geometry.width, man->geometry.height);
	      force_redraw = 1;
      }

      if (recreate_bg)
      {
	      if (man->colorsets[DEFAULT] >= 0)
	      {
		      recreate_background(man, DEFAULT);
	      }
	      else
	      {
		      recreate_bg = False;
	      }
      }

      /* must refresh transparent buttons when:
       * - moved and buttons is root transparent
       * - bg change and  butons is parental relative */
      if (moved || recreate_bg)
      {
	      Bool r = False;

	      man->geometry.x = theEvent.xconfigure.x;
	      man->geometry.y = theEvent.xconfigure.y;
	      recreate_transparent_bgs(man);
	      if (!force_redraw)
	      {
		      r = draw_transparent_buttons(
			      man, (moved && !recreate_bg), False);
		      if (r)
		      {
			      XSync(theDisplay, False);
		      }
		      break;
	      }
      }

      set_manager_width (man, theEvent.xconfigure.width);
      ConsoleDebug (X11, "\tboxwidth = %d\n", man->geometry.boxwidth);
      if (force_redraw)
      {
	      force_manager_redraw(man);
	      force_redraw = 0;
      }
      else
      {
	      /* draw_manager (man);*/
      }

      break;
    }
    case MotionNotify:
      /* ConsoleDebug (X11, "XEVENT: MotionNotify\n");  */
      b = xy_to_button (man, theEvent.xmotion.x, theEvent.xmotion.y);
      if (b != man->select_button) {
	ConsoleDebug (X11, "\tmoving select\n");
	move_highlight (man, b);
	run_binding (man, SELECT);
	draw_managers();
      }
      tips_on(man, b);
      break;

    case MapNotify:
      ConsoleDebug (X11, "XEVENT: MapNotify\n");
      if (globals.transient) {
	grab_pointer (man);
      }
      force_manager_redraw (man);
      break;

    case UnmapNotify:
      ConsoleDebug (X11, "XEVENT: UnmapNotify\n");
      break;

    case ClientMessage:
      if (theEvent.xclient.format == 32 &&
	  theEvent.xclient.data.l[0]==_XA_WM_DEL_WIN)
      {
	DeadPipe(1);
      }
      break;

    case DestroyNotify:
      ConsoleDebug(X11, "XEVENT: DestroyNotify\n");
      DeadPipe(0);
      break;

    default:
      if (FShapesSupported && theEvent.type == FShapeEventBase + FShapeNotify) {
	FShapeEvent *xev = (FShapeEvent *)&theEvent;
	ConsoleDebug (X11, "XEVENT: ShapeNotify\n");
	ConsoleDebug (X11, "\tx, y, w, h = %d, %d, %d, %d\n",
		      xev->x, xev->y, xev->width, xev->height);
	break;
      }
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
  XWMHints wmhints = {0, 0, 0, 0, 0, 0, 0, 0, 0};

  wmhints.initial_state = NormalState;
  wmhints.flags = StateHint;

  if (XStringListToTextProperty (&name, 1, &win_name) == 0) {
    ConsoleMessage ("%s: cannot allocate window name.\n", MyName);
    return;
  }
  if (XStringListToTextProperty (&icon, 1, &win_icon) == 0) {
    ConsoleMessage ("%s: cannot allocate window icon.\n", MyName);
    return;
  }

  class.res_name = MyName;
  class.res_class = "FvwmIconMan";


  XSetWMProperties (theDisplay, win, &win_name, &win_icon, NULL, 0,
		    sizehints, &wmhints, &class);

  XFree (win_name.value);
  XFree (win_icon.value);
}

static int load_default_context_fore (WinManager *man, int i)
{
  int j = 0;

  if (!PictureUseBWOnly())
    j = 1;

  ConsoleDebug (X11, "Loading: %s\n", contextDefaults[i].backcolor[j]);

  return lookup_color (contextDefaults[i].forecolor[j], &man->forecolor[i]);
}

static int load_default_context_back (WinManager *man, int i)
{
  int j = 0;

  if (!PictureUseBWOnly())
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
      /* wait for a map event to actually do the grab */
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

void X_init_manager (int man_id)
{
  WinManager *man;
  unsigned int width, height;
  int i, x, y, geometry_mask;

  ConsoleDebug (X11, "In X_init_manager\n");

  man = &globals.managers[man_id];

  man->geometry.cols = DEFAULT_NUM_COLS;
  man->geometry.rows = DEFAULT_NUM_ROWS;
  man->gravity = NorthWestGravity;

  man->select_button = NULL;
  man->cursor_in_window = 0;
  man->sizehints_flags = 0;

  ConsoleDebug (X11, "boxwidth = %d\n", man->geometry.boxwidth);

  if (man->fontname == NULL)
    man->FButtonFont = FlocaleLoadFont(theDisplay, NULL, MyName);
  else
    man->FButtonFont = FlocaleLoadFont(theDisplay, man->fontname, MyName);
  if (man->FButtonFont == NULL)
  {
    ConsoleMessage ("Can't get font, exiting\n");
    ShutMeDown (1);
  }

  if (man->tips_fontname == NULL)
    man->tips_conf->Ffont = FlocaleLoadFont(theDisplay, NULL, MyName);
  else
    man->tips_conf->Ffont = FlocaleLoadFont(
	    theDisplay, man->tips_fontname, MyName);

  for ( i = 0; i < NUM_CONTEXTS; i++ ) {
    man->pixmap[i] = None;
    if (man->colorsets[i] >= 0) {
      man->backcolor[i] = Colorset[man->colorsets[i]].bg;
    }
    else if (man->colorsets[i] == -2) {
      man->backcolor[i] = Colorset[man->colorsets[DEFAULT]].bg;
    }
    else if (man->backColorName[i]) {
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

    if (man->colorsets[i] >= 0) {
      man->forecolor[i] = Colorset[man->colorsets[i]].fg;
    }
    else if (man->colorsets[i] == -2) {
      man->forecolor[i] = Colorset[man->colorsets[DEFAULT]].fg;
    }
    else if (man->foreColorName[i]) {
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

    if (!PictureUseBWOnly()) {
      if (man->colorsets[i] >= 0) {
	man->shadowcolor[i] = Colorset[man->colorsets[i]].shadow;
	man->hicolor[i] = Colorset[man->colorsets[i]].hilite;
      }
      else if (man->colorsets[i] == -2) {
	man->shadowcolor[i] = Colorset[man->colorsets[DEFAULT]].shadow;
	man->hicolor[i] = Colorset[man->colorsets[DEFAULT]].hilite;
      }
      else
      {
      man->shadowcolor[i] = GetShadow(man->backcolor[i]);
      man->hicolor[i] = GetHilite(man->backcolor[i]);
      }
    }
  }

  man->fontheight = man->FButtonFont->height;

  /* silly hack to guess the minimum char width of the font
     doesn't have to be perfect. */

  man->fontwidth = FlocaleTextWidth (man->FButtonFont, ".", 1);

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
    x = y = width = height = 0;
    val = XParseGeometry (man->button_geometry_str, &x, &y, &width, &height);
    ConsoleDebug (X11, "button x, y, w, h = %d %d %u %u\n", x, y, width,
		  height);
    if (val & WidthValue)
      man->geometry.boxwidth = width;
    if (val & HeightValue)
    {
      if (height >= man->geometry.boxheight - 4)
	man->geometry.boxheight = height;
    }
  }
  FScreenGetScrRect(NULL, FSCREEN_PRIMARY, &man->managed_g.x, &man->managed_g.y,
		    &man->managed_g.width, &man->managed_g.height);
  man->geometry.x = man->managed_g.x;
  man->geometry.y = man->managed_g.y;
  if (man->geometry_str) {
    int scr;

    geometry_mask = FScreenParseGeometryWithScreen(
      man->geometry_str, &man->geometry.x, &man->geometry.y,
      &man->geometry.cols, &man->geometry.rows, &scr);
    FScreenGetScrRect(
      NULL, scr, &man->managed_g.x, &man->managed_g.y,
      &man->managed_g.width, &man->managed_g.height);

    if (geometry_mask & XValue)
      man->geometry.x += man->managed_g.x;
    if (geometry_mask & YValue)
	man->geometry.y += man->managed_g.y;
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
    }    else {
      man->geometry.dir |= GROW_HORIZ | GROW_FIXED;
    }
  }

  man->geometry.width  = man->geometry.cols * man->geometry.boxwidth;
  man->geometry.height = man->geometry.rows * man->geometry.boxheight;
  if ((geometry_mask & XValue) && (geometry_mask & XNegative))
    man->geometry.x += man->managed_g.width - man->geometry.width;
  if ((geometry_mask & YValue) && (geometry_mask & YNegative))
    man->geometry.y += man->managed_g.height - man->geometry.height;
  if (globals.transient) {
    Window dummyroot, dummychild;
    int junk;
    unsigned int ujunk;
    fscreen_scr_arg fscr;

    if (FQueryPointer(
	  theDisplay, theRoot, &dummyroot, &dummychild, &man->geometry.x,
	  &man->geometry.y, &junk, &junk, &ujunk) == False)
    {
      /* pointer is on a different screen - that's okay here */
      man->geometry.y = 0;
      man->geometry.x = 0;
    }
    fscr.xypos.x = man->geometry.x;
    fscr.xypos.y = man->geometry.y;
    FScreenGetScrRect(
      &fscr, FSCREEN_XYPOS,
      &man->managed_g.x, &man->managed_g.y,
      &man->managed_g.width, &man->managed_g.height);
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

  return;
}

void create_manager_window (int man_id)
{
  XSizeHints sizehints;
  XGCValues gcval;
  unsigned long gcmask = 0;
  unsigned long winattrmask = CWBackPixmap | CWBorderPixel | CWEventMask |
    CWBackingStore | CWBitGravity | CWColormap;
  XSetWindowAttributes winattr;
  unsigned int line_width = 1;
  int line_style = LineSolid;
  int cap_style = CapRound;
  int join_style = JoinRound;
  int i;
  WinManager *man;
  fscreen_scr_t scr;
  ConsoleDebug (X11, "In create_manager_window\n");

  man = &globals.managers[man_id];

  if (man->window_up)
    return;

  size_manager (man);

  sizehints.flags = man->sizehints_flags;


  sizehints.width = man->geometry.width;
  sizehints.height = man->geometry.height;
  sizehints.base_width = sizehints.min_width = 0;
  sizehints.base_height = 0;
  sizehints.max_width = man->managed_g.width;
  if (man->geometry.dir & GROW_FIXED)
  {
    sizehints.min_height = man->geometry.boxheight;
    sizehints.max_height = man->managed_g.height;
  }
  else
  {
    sizehints.min_height = man->geometry.height;
    sizehints.max_height = man->geometry.height;
  }
  sizehints.win_gravity = man->gravity;
  sizehints.width_inc = 1;
  /* This works much better with colour sets and when swallowed in
   * FvwmButtons. */
  sizehints.height_inc = 1;
  sizehints.flags |= PBaseSize | PMinSize | PMaxSize | PWinGravity | PResizeInc;
  sizehints.x = man->geometry.x;
  sizehints.y = man->geometry.y;

  ConsoleDebug (X11, "hints: x, y, w, h = %d %d %d %d)\n",
		sizehints.x, sizehints.y,
		sizehints.base_width, sizehints.base_height);
  ConsoleDebug (X11, "gravity: %d %d\n", sizehints.win_gravity, man->gravity);


  /* set the background to be transparent later if necessary */
  winattr.background_pixmap = None;
  winattr.border_pixel = man->forecolor[DEFAULT];
  winattr.backing_store = WhenMapped;
  winattr.bit_gravity = man->gravity;
  winattr.colormap = Pcmap;
  winattr.event_mask = ExposureMask | PointerMotionMask | EnterWindowMask |
    LeaveWindowMask | KeyPressMask | StructureNotifyMask;

  if (globals.transient)
    winattr.event_mask |= ButtonReleaseMask;
  else
    winattr.event_mask |= ButtonPressMask;

  if (man->res == SHOW_SCREEN || man->res == NO_SHOW_SCREEN)
  {
    fscreen_scr_arg fscr;

    fscr.xypos.x = sizehints.x;
    fscr.xypos.y = sizehints.y;
    FScreenGetScrRect(
      &fscr, FSCREEN_XYPOS,
      &man->managed_g.x, &man->managed_g.y,
      &man->managed_g.width, &man->managed_g.height);
    scr = FSCREEN_XYPOS;
  }
  else
  {
    FScreenGetScrRect(
      NULL, FSCREEN_GLOBAL,
      &man->managed_g.x, &man->managed_g.y,
      &man->managed_g.width, &man->managed_g.height);
    scr = FSCREEN_GLOBAL;
  }

  man->theWindow = XCreateWindow(theDisplay, theRoot, sizehints.x, sizehints.y,
				 man->geometry.width, man->geometry.height,
				 0, Pdepth, InputOutput, Pvisual, winattrmask,
				 &winattr);
  /* hack to prevent mapping on wrong screen with StartsOnScreen */
  FScreenMangleScreenIntoUSPosHints(scr, &sizehints);
  XSetWMNormalHints(theDisplay, man->theWindow, &sizehints);
  FShapeSelectInput (theDisplay, man->theWindow, FShapeNotifyMask);
  if (globals.transient)
  {
    XSetTransientForHint(theDisplay, man->theWindow, theRoot);
  }
  XSetWMProtocols(theDisplay, man->theWindow, &_XA_WM_DEL_WIN, 1);

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
	  if (man->colorsets[i] != -2)
	  {
		  man->backContext[i] =
			  fvwmlib_XCreateGC(
				  theDisplay, man->theWindow, gcmask, &gcval);
		  XSetForeground (
			  theDisplay, man->backContext[i], man->backcolor[i]);
		  XSetLineAttributes (
			  theDisplay, man->backContext[i], line_width,
			  line_style, cap_style,
			  join_style);
	  }
	  man->hiContext[i] = fvwmlib_XCreateGC(
		  theDisplay, man->theWindow, gcmask, &gcval);
	  if (man->FButtonFont->font != NULL)
		  XSetFont(
			  theDisplay, man->hiContext[i],
			  man->FButtonFont->font->fid);
	  XSetForeground (theDisplay, man->hiContext[i], man->forecolor[i]);

	  gcmask = GCForeground | GCBackground;
	  gcval.foreground = man->backcolor[i];
	  gcval.background = man->forecolor[i];
	  man->flatContext[i] = fvwmlib_XCreateGC(
		  theDisplay, man->theWindow, gcmask, &gcval);
	  if (!PictureUseBWOnly())
	  {
		  gcmask = GCForeground | GCBackground;
		  gcval.foreground = man->hicolor[i];
		  gcval.background = man->backcolor[i];
		  man->reliefContext[i] = fvwmlib_XCreateGC (
			  theDisplay, man->theWindow, gcmask, &gcval);

		  gcmask = GCForeground | GCBackground;
		  gcval.foreground = man->shadowcolor[i];
		  gcval.background = man->backcolor[i];
		  man->shadowContext[i] = fvwmlib_XCreateGC (
			  theDisplay, man->theWindow, gcmask, &gcval);
	  }
	  if (man->colorsets[i] >= 0)
	  {
		  recreate_background(man, i);
	  }
	  else if (man->colorsets[i] != -2)
	  {
		  man->pixmap[i] = None;
		  XSetFillStyle(theDisplay, man->backContext[i], FillSolid);
		  if (i == DEFAULT)
		  {
			  XSetWindowBackground (
				  theDisplay, man->theWindow, man->backcolor[i]);
		  }
	  }
  }

  set_window_properties (man->theWindow, man->titlename,
			 man->iconname, &sizehints);
  man->window_up = 1;
  map_manager (man);
}

static int handle_error (Display *d, XErrorEvent *ev)
{
  /* BadDrawable is allowed, it happens when colorsets change too fast */
  switch (ev->error_code)
  {
  case BadDrawable:
  case BadPixmap:
  case BadWindow:
    return 0;
  default:
    break;
  }

  if (FRenderGetErrorCodeBase() + FRenderBadPicture == ev->error_code)
	  return 0;

  /* does not return */
  PrintXErrorAndCoredump(d, ev, MyName);
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
  _XA_WM_DEL_WIN = XInternAtom(theDisplay, "WM_DELETE_WINDOW", 0);
  flib_init_graphics (theDisplay);
  FTipsInit(theDisplay);

  x_fd = XConnectionNumber (theDisplay);
  theScreen = DefaultScreen (theDisplay);
  theRoot = RootWindow (theDisplay, theScreen);
}

void recreate_background(WinManager *man, Contexts i)
{
	int cset = man->colorsets[i];

	if (cset < 0)
	{
		return;
	}

	if (man->pixmap[i] && man->pixmap[i] != ParentRelative)
	{
		XFreePixmap(theDisplay, man->pixmap[i]);
	}
	man->pixmap[i]= None;
	if ((i == DEFAULT && Colorset[cset].pixmap) ||
	    (CSET_IS_TRANSPARENT(cset) && !CSET_IS_TRANSPARENT_PR_TINT(cset)))
	{
		man->pixmap[i] =
			CreateBackgroundPixmap(
				theDisplay, man->theWindow,
				man->geometry.width, man->geometry.height,
				&Colorset[cset],
				Pdepth, man->backContext[i], False);
		XSetTile(theDisplay, man->backContext[i], man->pixmap[i]);
		XSetFillStyle(theDisplay, man->backContext[i], FillTiled);
		if (i == DEFAULT)
		{
			XSetWindowBackgroundPixmap(
				theDisplay, man->theWindow, man->pixmap[i]);
		}
	}
	else if (Colorset[cset].pixmap && !CSET_IS_TRANSPARENT_PR_TINT(cset))
	{
		man->pixmap[i] =
			CreateBackgroundPixmap(
				theDisplay, man->theWindow,
				man->geometry.boxwidth, man->geometry.boxheight,
				&Colorset[cset],
				Pdepth, man->backContext[i], False);
		XSetTile(theDisplay, man->backContext[i], man->pixmap[i]);
		XSetFillStyle(theDisplay, man->backContext[i], FillTiled);
	}
	else
	{
		XSetTile(theDisplay, man->backContext[i], None);
		XSetFillStyle(theDisplay, man->backContext[i], FillSolid);
		if (i == DEFAULT)
		{
			XSetWindowBackground (
				theDisplay, man->theWindow, man->backcolor[i]);
		}
	}
}

void change_colorset(int color)
{
	WinManager *man;
	int i,j;

	for (j = 0; j < globals.num_managers; j++)
	{
		man = &globals.managers[j];
		for (i = 0; i < NUM_CONTEXTS; i++)
		{
			if(man->colorsets[i] != color &&
			   !(man->colorsets[i] == -2 &&
			    man->colorsets[DEFAULT] == color))
			{
				continue;
			}
			man->backcolor[i] = Colorset[color].bg;
			man->forecolor[i] = Colorset[color].fg;
			man->shadowcolor[i] = Colorset[color].shadow;
			man->hicolor[i] = Colorset[color].hilite;
			if (!man->hiContext[i] ||
			    !man->flatContext[i] ||
			    !man->reliefContext[i] ||
			    !man->shadowContext[i])
			{
				/* colorset not properly defined yet, skip it */
				continue;
			}
			if (man->backContext[i])
			{
				XSetForeground(
					theDisplay, man->backContext[i],
					man->backcolor[i]);
			}
			XSetForeground(
				theDisplay, man->hiContext[i],
				man->forecolor[i]);
			XSetBackground (
				theDisplay, man->flatContext[i],
				man->forecolor[i]);
			XSetForeground (
				theDisplay, man->flatContext[i],
				man->backcolor[i]);
			if (!PictureUseBWOnly()) {
				XSetBackground (
					theDisplay, man->reliefContext[i],
					man->backcolor[i]);
				XSetForeground (
					theDisplay, man->reliefContext[i],
					man->hicolor[i]);
				XSetBackground (
					theDisplay, man->shadowContext[i],
					man->backcolor[i]);
				XSetForeground (
					theDisplay, man->shadowContext[i],
					man->shadowcolor[i]);
			}
			recreate_background(man, i);
		}
		force_manager_redraw (man);
		if (man->tips_conf->colorset == color)
		{
			FTipsColorsetChanged(theDisplay, color);
		}
	}
}


void recreate_transparent_bgs(WinManager *man)
{
	int i, color;

	for (i = 0; i < NUM_CONTEXTS; i++ )
	{
		color = man->colorsets[i];
		if (i == DEFAULT || !CSET_IS_TRANSPARENT(color) ||
		    CSET_IS_TRANSPARENT_PR(color))
		{
			continue;
		}
		if (!man->backContext[i])
		{
			/* colorset not properly defined yet, just skip it */
			continue;
		}
		recreate_background(man, i);
	}
}
