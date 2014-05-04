/* -*-c-*- */
/*
 * This file  is  partly derived from   FvwmForm.c, this is  the original
 * copyright:
 *
 * FvwmForm is original work of Thomas Zuwei Feng.
 *
 * Copyright Feb 1995, Thomas Zuwei Feng.  No guarantees or warantees are
 * provided or implied in any way whatsoever.  Use this program at your own
 * risk.  Permission to use, modify, and redistribute this program is hereby
 * given, provided that this copyright is kept intact.
 *
 * And the same goes for me, Dan Espen, March 10, 1999.
 */

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

/*  Modification History */

/*  Changed on 03/19/99 by DanEspen (dje): */
/*  - paste tab as space if there is one input field. */

/*  Changed on 03/10/99 by DanEspen (dje): */
/*  - Make button 2 paste work. */

/*  Changed on 02/27/99 by DanEspen (dje): */
/*  - Add logic to allow international characters, bug id 179 */
/*  - Split into separate file. */

#include "config.h"
#include "libs/fvwmlib.h"
#include "libs/Colorset.h"

#include <stdio.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#include <X11/Xatom.h>                  /* for XA_CUT_BUFFER0 */

#include <FvwmForm.h>

static void process_regular_char_input(unsigned char *buf);
static int process_tabtypes(unsigned char * buf);
static void process_history(int direction);
static void process_paste_request (XEvent *event, Item *item);
static void ToggleChoice (Item *item);
static void ResizeFrame (void);

/* read an X event */
void ReadXServer (void)
{
  static XEvent event;
  int keypress;
  Item *item, *old_item;
  KeySym ks;
  char *sp, *dp;
  static unsigned char buf[10];         /* unsigned for international */
  static int n;

  while (FEventsQueued(dpy, QueuedAfterReading)) {
    FNextEvent(dpy, &event);
    if (event.xany.window == CF.frame) {
      switch (event.type) {
      case ClientMessage:
      {
	      if(event.xclient.format == 32 &&
		 event.xclient.data.l[0] == wm_del_win)
	      {
		      exit(0);
	      }
      }
      break;
      case ConfigureNotify:             /* has window be reconfigured */
      {
	      XEvent tmpe;

	      fev_sanitise_configure_notify(&event.xconfigure);
	      while (FCheckTypedWindowEvent(
		      dpy, CF.frame, ConfigureNotify, &tmpe))
	      {
		      fev_sanitise_configure_notify(&tmpe.xconfigure);
		      if (!tmpe.xconfigure.send_event)
			      continue;
		      event.xconfigure.x = tmpe.xconfigure.x;
		      event.xconfigure.y = tmpe.xconfigure.y;
		      event.xconfigure.send_event = True;
	      }
	      if (CF.max_width != event.xconfigure.width ||
		  CF.total_height != event.xconfigure.height)
	      {
		      /* adjust yourself... do noting */
		      ResizeFrame();
		      CF.max_width = event.xconfigure.width;
		      CF.total_height = event.xconfigure.height;
		      UpdateRootTransapency(False, True);
		      if (!CSET_IS_TRANSPARENT(colorset))
		      {
			  RedrawFrame(NULL);
		      }
	      }
	      else if (event.xconfigure.send_event)
	      {
		      UpdateRootTransapency(False, True);
	      }
      }
      break;
#if 0
      case SelectionClear:
	 selection_clear ();
	break;
      case SelectionNotify:
	selection_paste ();
	break;
      case SelectionRequest:
	 selection_send ();
	break;
#endif
      case Expose:
      {
	      int ex = event.xexpose.x;
	      int ey = event.xexpose.y;
	      int ex2 = event.xexpose.x + event.xexpose.width;
	      int ey2 = event.xexpose.y + event.xexpose.height;
	      while (FCheckTypedWindowEvent(dpy, CF.frame, Expose, &event))
	      {
		      ex = min(ex, event.xexpose.x);
		      ey = min(ey, event.xexpose.y);
		      ex2 = max(ex2, event.xexpose.x + event.xexpose.width);
		      ey2 = max(ey2 , event.xexpose.y + event.xexpose.height);
	      }
	      event.xexpose.x = ex;
	      event.xexpose.y = ey;
	      event.xexpose.width = ex2 - ex;
	      event.xexpose.height = ey2 - ey;
	      RedrawFrame(&event);
	      if (CF.grab_server && !CF.server_grabbed)
	      {
		      if (GrabSuccess ==
			  XGrabPointer(dpy, CF.frame, True, 0,
				       GrabModeAsync, GrabModeAsync,
				       None, None, CurrentTime))
			      CF.server_grabbed = 1;
	      }
      }
      break;
      case VisibilityNotify:
	if (CF.server_grabbed &&
	    event.xvisibility.state != VisibilityUnobscured)
	{
	  /* raise our window to the top */
	  XRaiseWindow(dpy, CF.frame);
	  XSync(dpy, 0);
	}
	break;
      case KeyPress:  /* we do text input here */
	n = XLookupString(&event.xkey, (char *)buf, sizeof(buf), &ks, NULL);
	keypress = buf[0];
	myfprintf((stderr, "Keypress [%s]\n", buf));
	if (n == 0) {  /* not a regular key, translate it into one */
	  switch (ks) {
	  case XK_Home:
	  case XK_Begin:
	    buf[0] = '\001';  /* ^A */
	    break;
	  case XK_End:
	    buf[0] = '\005';  /* ^E */
	    break;
	  case XK_Left:
	    buf[0] = '\002';  /* ^B */
	    break;
	  case XK_Right:
	    buf[0] = '\006';  /* ^F */
	    break;
	  case XK_Up:
	    buf[0] = '\020';            /* ^P */
	    break;
	  case XK_Down:
	    buf[0] = '\016';  /* ^N */
	    break;
	  default:
	    if (ks >= XK_F1 && ks <= XK_F35) {
	      buf[0] = '\0';
	      keypress = 257 + ks - XK_F1;
	    } else
	      goto no_redraw;  /* no action for this event */
	  }
	}
	switch (ks) {                   /* regular key, may need adjustment */
	case XK_Tab:
#ifdef XK_XKB_KEYS
	case XK_ISO_Left_Tab:
#endif
	  if (event.xkey.state & ShiftMask) { /* shifted key */
	    buf[0] = '\020';          /* chg shift tab to ^P */
	  }
	  break;
	case '>':
	  if (event.xkey.state & Mod1Mask) { /* Meta, shift > */
	    process_history(1);
	    goto redraw_newcursor;
	  }
	  break;
	case '<':
	  if (event.xkey.state & Mod1Mask) { /* Meta, shift < */
	    process_history(-1);
	    goto redraw_newcursor;
	  }
	  break;
	}
	if (!CF.cur_input) {  /* no text input fields */
	  for (item = root_item_ptr; item != 0;
	       item = item->header.next) {/* all items */
	    if (item->type == I_BUTTON && item->button.keypress == keypress) {
	      RedrawItem(item, 1, NULL);
	      usleep(MICRO_S_FOR_10MS);
	      RedrawItem(item, 0, NULL);
	      DoCommand(item);
	      goto no_redraw;
	    }
	  }
	  break;
	} else if (CF.cur_input == CF.cur_input->input.next_input) {
	  /* 1 ip field */
	  switch (buf[0]) {
	  case '\020':                  /* ^P previous field */
	    process_history(-1);
	    goto redraw_newcursor;
	    break;
	  case '\016':                  /* ^N  next field */
	    process_history(1);
	    goto redraw_newcursor;
	    break;
	  } /* end switch */
	} /* end one input field */
	switch (buf[0]) {
	case '\001':  /* ^A */
	  CF.rel_cursor = 0;
	  CF.abs_cursor = 0;
	  CF.cur_input->input.left = 0;
	  goto redraw_newcursor;
	  break;
	case '\005':  /* ^E */
	  CF.rel_cursor = CF.cur_input->input.n;
	  if ((CF.cur_input->input.left =
	       CF.rel_cursor - CF.cur_input->input.size) < 0)
	    CF.cur_input->input.left = 0;
	  CF.abs_cursor = CF.rel_cursor - CF.cur_input->input.left;
	  goto redraw_newcursor;
	  break;
	case '\002':  /* ^B */
	  if (CF.rel_cursor > 0) {
	    CF.rel_cursor--;
	    CF.abs_cursor--;
	    if (CF.abs_cursor <= 0 && CF.rel_cursor > 0) {
	      CF.abs_cursor++;
	      CF.cur_input->input.left--;
	    }
	  }
	  goto redraw_newcursor;
	  break;
	case '\006':  /* ^F */
	  if (CF.rel_cursor < CF.cur_input->input.n) {
	    CF.rel_cursor++;
	    CF.abs_cursor++;
	    if (CF.abs_cursor >= CF.cur_input->input.size &&
		CF.rel_cursor < CF.cur_input->input.n) {
	      CF.abs_cursor--;
	      CF.cur_input->input.left++;
	    }
	  }
	  goto redraw_newcursor;
	  break;
	case '\010':  /* ^H */
	  if (CF.rel_cursor > 0) {
	    sp = CF.cur_input->input.value + CF.rel_cursor;
	    dp = sp - 1;
	    for (; *dp = *sp, *sp != '\0'; dp++, sp++);
	    CF.cur_input->input.n--;
	    CF.rel_cursor--;
	    if (CF.rel_cursor < CF.abs_cursor) {
	      CF.abs_cursor--;
	      if (CF.abs_cursor <= 0 && CF.rel_cursor > 0) {
		CF.abs_cursor++;
		CF.cur_input->input.left--;
	      }
	    } else
	      CF.cur_input->input.left--;
	  }
	  goto redraw_newcursor;
	  break;
	case '\177':  /* DEL */
	case '\004':  /* ^D */
	  if (CF.rel_cursor < CF.cur_input->input.n) {
	    sp = CF.cur_input->input.value + CF.rel_cursor + 1;
	    dp = sp - 1;
	    for (; *dp = *sp, *sp != '\0'; dp++, sp++);
	    CF.cur_input->input.n--;
	    goto redraw_newcursor;
	  }
	  break;
	case '\013':  /* ^K */
	  CF.cur_input->input.value[CF.rel_cursor] = '\0';
	  CF.cur_input->input.n = CF.rel_cursor;
	  goto redraw_newcursor;
	case '\025':  /* ^U */
	  CF.cur_input->input.value[0] = '\0';
	  CF.cur_input->input.n = CF.cur_input->input.left = 0;
	  CF.rel_cursor = CF.abs_cursor = 0;
	  goto redraw_newcursor;
	case '\020':                    /* ^P previous field */
	  old_item = CF.cur_input;
	  CF.cur_input = old_item->input.prev_input; /* new current input fld */
	  RedrawItem(old_item, 1, NULL);
	  CF.rel_cursor = CF.abs_cursor = 0; /* home cursor in new input field */
	  goto redraw;
	  break;
	case '\t':
	case '\n':
	case '\015':
	case '\016':  /* LINEFEED, TAB, RETURN, ^N, jump to the next field */
	  switch (process_tabtypes(&buf[0])) {
	    case 0: goto no_redraw;break;
	    case 1: goto redraw;break;
	  }
	  break;
	default:
	  if((buf[0] >= ' ' &&
	      buf[0] < '\177') ||
	     (buf[0] >= 160)) {         /* regular or intl char */
	    process_regular_char_input(&buf[0]); /* insert into input field */
	    goto redraw_newcursor;
	  }
	  /* unrecognized key press, check for buttons */
	  for (item = root_item_ptr; item != 0; item = item->header.next)
	  {
	    /* all items */
	    myfprintf((stderr, "Button: keypress==%d\n",
		    item->button.keypress));
	    if (item->type == I_BUTTON && item->button.keypress == keypress) {
	      RedrawItem(item, 1, NULL);
	      usleep(MICRO_S_FOR_10MS);  /* .1 seconds */
	      RedrawItem(item, 0, NULL);
	      DoCommand(item);
	      goto no_redraw;
	    }
	  }
	  break;
	}
      redraw_newcursor:
	{
	  XSetForeground(dpy, CF.cur_input->header.dt_ptr->dt_item_GC,
			 CF.cur_input->header.dt_ptr->dt_colors[c_item_bg]);
	  /* Since DrawString is being used, I changed this to clear the
	     entire input field.  dje 10/24/99. */
	  XClearArea(dpy, CF.cur_input->header.win,
		     BOX_SPC + TEXT_SPC - 1, BOX_SPC,
		     CF.cur_input->header.size_x
		     - (2 * BOX_SPC) - 2 - TEXT_SPC,
		     (CF.cur_input->header.size_y - 1)
		     - 2 * BOX_SPC + 1, False);
	}
      redraw:
	{
	  int len, x, dy;
	  len = CF.cur_input->input.n - CF.cur_input->input.left;
	  XSetForeground(dpy, CF.cur_input->header.dt_ptr->dt_item_GC,
			 CF.cur_input->header.dt_ptr->dt_colors[c_item_fg]);
	  if (len > CF.cur_input->input.size)
	    len = CF.cur_input->input.size;
	  CF.cur_input->header.dt_ptr->dt_Fstr->win = CF.cur_input->header.win;
	  CF.cur_input->header.dt_ptr->dt_Fstr->gc  =
	    CF.cur_input->header.dt_ptr->dt_item_GC;
	  CF.cur_input->header.dt_ptr->dt_Fstr->flags.has_colorset = False;
	  if (itemcolorset >= 0)
	  {
	    CF.cur_input->header.dt_ptr->dt_Fstr->colorset =
		    &Colorset[itemcolorset];
	    CF.cur_input->header.dt_ptr->dt_Fstr->flags.has_colorset = True;
	  }
	  CF.cur_input->header.dt_ptr->dt_Fstr->str = CF.cur_input->input.value;
	  CF.cur_input->header.dt_ptr->dt_Fstr->x   = BOX_SPC + TEXT_SPC;
	  CF.cur_input->header.dt_ptr->dt_Fstr->y   = BOX_SPC + TEXT_SPC
	    + CF.cur_input->header.dt_ptr->dt_Ffont->ascent;
	  CF.cur_input->header.dt_ptr->dt_Fstr->len = len;
	  FlocaleDrawString(dpy,
			    CF.cur_input->header.dt_ptr->dt_Ffont,
			    CF.cur_input->header.dt_ptr->dt_Fstr,
			    FWS_HAVE_LENGTH);
	  x = BOX_SPC + TEXT_SPC +
		  FlocaleTextWidth(CF.cur_input->header.dt_ptr->dt_Ffont,
				   CF.cur_input->input.value,CF.abs_cursor)
		  - 1;
	  dy = CF.cur_input->header.size_y - 1;
	  XDrawLine(dpy, CF.cur_input->header.win,
		    CF.cur_input->header.dt_ptr->dt_item_GC,
		    x, BOX_SPC, x, dy - BOX_SPC);
	  myfprintf((stderr,"Line %d/%d - %d/%d (char)\n",
		     x, BOX_SPC, x, dy - BOX_SPC));
	}
      no_redraw:
	break;  /* end of case KeyPress */
      }  /* end of switch (event.type) */
      continue;
    }  /* end of if (event.xany.window == CF.frame) */
    for (item = root_item_ptr; item != 0;
	 item = item->header.next) {    /* all items */
      if (event.xany.window == item->header.win) {
	switch (event.type) {
	case Expose:
	{
		int ex = event.xexpose.x;
		int ey = event.xexpose.y;
		int ex2 = event.xexpose.x + event.xexpose.width;
		int ey2 = event.xexpose.y + event.xexpose.height;
		while (FCheckTypedWindowEvent(
			dpy, item->header.win, Expose, &event))
		{
			ex = min(ex, event.xexpose.x);
			ey = min(ey, event.xexpose.y);
			ex2 = max(ex2, event.xexpose.x + event.xexpose.width);
			ey2 = max(ey2 , event.xexpose.y + event.xexpose.height);
		}
		event.xexpose.x = ex;
		event.xexpose.y = ey;
		event.xexpose.width = ex2 - ex;
		event.xexpose.height = ey2 - ey;
		RedrawItem(item, 0, &event);
	}
	break;
	case ButtonPress:
	  if (item->type == I_INPUT) {
	    old_item = CF.cur_input;
	    CF.cur_input = item;
	    RedrawItem(old_item, 1, NULL);
	    {
	      Bool done = False;

	      CF.abs_cursor = 0;
	      while(CF.abs_cursor <= item->input.size && !done)
	      {
		if (FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,
				     item->input.value,
				     CF.abs_cursor) >=
		    event.xbutton.x - BOX_SPC - TEXT_SPC)
		{
		  done = True;
		  CF.abs_cursor--;
		}
		else
		{
		  CF.abs_cursor++;
		}
	      }
	    }
	    if (CF.abs_cursor < 0)
	      CF.abs_cursor = 0;
	    if (CF.abs_cursor > item->input.size)
	      CF.abs_cursor = item->input.size;
	    CF.rel_cursor = CF.abs_cursor + item->input.left;
	    if (CF.rel_cursor < 0)
	      CF.rel_cursor = 0;
	    if (CF.rel_cursor > item->input.n)
	      CF.rel_cursor = item->input.n;
	    if (CF.rel_cursor > 0 && CF.rel_cursor == item->input.left)
	      item->input.left--;
	    if (CF.rel_cursor < item->input.n &&
		CF.rel_cursor == item->input.left + item->input.size)
	      item->input.left++;
	    CF.abs_cursor = CF.rel_cursor - item->input.left;
	    if (event.xbutton.button == Button2) { /* if paste request */
	      process_paste_request (&event, item);
	    }
	    RedrawItem(item, 0, NULL);
	  }
	  if (item->type == I_CHOICE)
	    ToggleChoice(item);
	  if (item->type == I_BUTTON) {
	    RedrawItem(item, 1, NULL);    /* push button in */
	    if (CF.activate_on_press) {
	      usleep(MICRO_S_FOR_10MS);   /* make sure its visible */
	      RedrawItem(item, 0, NULL);  /* pop button out */
	      DoCommand(item);            /* execute the button command */
	    } else {
	      XGrabPointer(dpy, item->header.win,
			   False,         /* owner of events */
			   ButtonReleaseMask, /* events to report */
			   GrabModeAsync, /* keyboard mode */
			   GrabModeAsync, /* pointer mode */
			   None,          /* confine to */
			   /* I sort of like this, the hand points in
			      the other direction and the color is
			      reversed. I don't know what other GUIs do,
			      Java doesn't do anything, neither does anything
			      else I can find...dje */
			   CF.pointer[button_in_pointer],   /* cursor */
			   CurrentTime);
	    } /* end activate on press */
	  }
	  break;
	case ButtonRelease:
	  if (!CF.activate_on_press) {
	    RedrawItem(item, 0, NULL);
	    if (CF.grab_server && CF.server_grabbed) {
	      /* You have to regrab the pointer, or focus
		 can go to another window.
		 grab...
	      */
	      XGrabPointer(dpy, CF.frame, True, 0, GrabModeAsync, GrabModeAsync,
			   None, None, CurrentTime);
	      XFlush(dpy);
	    } else {
	      XUngrabPointer(dpy, CurrentTime);
	      XFlush(dpy);
	    }
	    if (event.xbutton.x >= 0 &&
		event.xbutton.x < item->header.size_x &&
		event.xbutton.y >= 0 &&
		event.xbutton.y < item->header.size_y) {
	      DoCommand(item);
	    }
	  }
	  break;
	}
      }
    }  /* end of for (i = 0) */
  }  /* while loop */
}

/* Each input field has a history, depending on the passed
   direction, get the desired history item into the input field.
   After "Restart" the yank point is one entry beyond the last
   inserted item.
   Normal yanks, going backward, go down the array decrementing
   yank count before extracting.
   Forward yanks increment before extracting. */
static void process_history(int direction) {
  int count;
  if (!CF.cur_input                     /* no input fields */
      || !CF.cur_input->input.value_history_ptr) { /* or no history */
    return;                             /* bail out */
  }
  /* yankat is always one beyond slot to yank from. */
  count = CF.cur_input->input.value_history_yankat + direction;
  if (count < 0 || count >= VH_SIZE ||
	     CF.cur_input->input.value_history_ptr[count] == 0 ) {
    if (direction <= 0) {               /* going down */
      for (count = VH_SIZE - 1;
	   CF.cur_input->input.value_history_ptr[count] == 0;
	   --count);                    /* find last used slot */
    } else {                            /* going up */
      count = 0;                        /* up is the bottom */
    }
  }
  CF.cur_input->input.value =
    xstrdup(CF.cur_input->input.value_history_ptr[count]);
  CF.cur_input->input.n = strlen(CF.cur_input->input.value);
  CF.cur_input->input.value_history_yankat = count; /* new yank point */
  CF.cur_input->input.buf = CF.cur_input->input.n;
  CF.rel_cursor = 0;
  CF.abs_cursor = 0;
  CF.cur_input->input.left = 0;
  return;
}

/* Note that tab, return, linefeed, ^n, all do the same thing
   except when it comes to matching a command button hotkey.

   Return 1 to redraw, 0 for no redraw */
static int process_tabtypes(unsigned char * buf) {
  Item *item, *old_item;
  /* Note: the input field ring used with ^P above
	     could probably make this a lot simpler. dje 12/20/98 */
	  /* Code tracks cursor. */
  item = root_item_ptr;         /* init item ptr */
  if (CF.cur_input != 0) {          /* if in text */
    item = CF.cur_input->header.next; /* move to next item */
  }
  for ( ; item != 0;
	item = item->header.next) {/* find next input item */
    if (item->type == I_INPUT) {
      old_item = CF.cur_input;
      CF.cur_input = item;
      RedrawItem(old_item, 1, NULL);
      CF.rel_cursor = CF.abs_cursor = 0; /* home cursor in new input field */
      return (1);                       /* cause redraw */
    }
  }
  /* end of all text input fields, check for buttons */
  for (item = root_item_ptr; item != 0;
       item = item->header.next) {/* all items */
    myfprintf((stderr, "Button: keypress==%d vs buf %d\n",
	       item->button.keypress, buf[0]));
    if (item->type == I_BUTTON && item->button.keypress == buf[0]) {
      RedrawItem(item, 1, NULL);
      usleep(MICRO_S_FOR_10MS);
      RedrawItem(item, 0, NULL);
      DoCommand(item);
      return (0);                       /* cause no_redraw */
    }
  }
  /* goto the first text input field */
  for (item = root_item_ptr; item != 0;
       item = item->header.next) {/* all items */
    if (item->type == I_INPUT) {
      old_item = CF.cur_input;
      CF.cur_input = item;
      RedrawItem(old_item, 1, NULL);
      CF.abs_cursor = CF.rel_cursor - item->input.left;
      return (1);                       /* goto redraw */
    }
  }
  return (-1);
}

static void process_regular_char_input(unsigned char *buf) {
  char *sp, *dp, *ep;
  /* n is the space actually used.
     buf is the size of the buffer
     size is the size of the field on the screen
     size is used as the increment, arbitrarily. */
  if (++(CF.cur_input->input.n) >= CF.cur_input->input.buf) {
    CF.cur_input->input.buf += CF.cur_input->input.size;
    CF.cur_input->input.value =
      xrealloc(CF.cur_input->input.value, CF.cur_input->input.buf,
		      sizeof(CF.cur_input->input.value));
  }
  dp = CF.cur_input->input.value + CF.cur_input->input.n;
  sp = dp - 1;
  ep = CF.cur_input->input.value + CF.rel_cursor;
  for (; *dp = *sp, sp != ep; sp--, dp--);
  *ep = buf[0];
  CF.rel_cursor++;
  CF.abs_cursor++;
  if (CF.abs_cursor >= CF.cur_input->input.size) {
    if (CF.rel_cursor < CF.cur_input->input.n)
      CF.abs_cursor = CF.cur_input->input.size - 1;
    else
      CF.abs_cursor = CF.cur_input->input.size;
    CF.cur_input->input.left = CF.rel_cursor - CF.abs_cursor;
  }
}
/* Process a paste.  This can be any size.
   Right now, the input loop can't just be fed characters.
   Send plain text and newlines to the 2 subroutines that want them.
   */
static void process_paste_request (XEvent *event, Item *item) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after, nread;
  unsigned char *data;
  unsigned char *c;

  nread = 0;                            /* init read offset */
  do {
    if (XGetWindowProperty (dpy,
			    DefaultRootWindow (dpy),
			    XA_CUT_BUFFER0,
			    nread/4, 1024L,   /* offset, length */
			    False,           /* delete */
			    AnyPropertyType, /* request type */
			    &actual_type,
			    &actual_format, &nitems, &bytes_after,
			    (unsigned char **)&data) != Success) {
      return;                           /* didn't work, give up */
    } /* end didn't work */
    if (actual_type != XA_STRING) {     /* if something other than text */
      return;                           /* give up */
    }
    for (c = data; c != data + nitems; c++) { /* each char */
      switch (*c) {
      case '\t':                        /* TAB */
	if (CF.cur_input == CF.cur_input->input.next_input) { /* 1 ip field */
	  process_regular_char_input((unsigned char *)" "); /* paste space */
	} else {
	  process_tabtypes(c);          /* jump to the next field */
	}
      case '\015':                      /* LINEFEED */
      case '\016':                      /* ^N */
	process_tabtypes(c);            /* jump to the next field */
	break;
      case '\n':
	/* change \n to \r for pasting */
	process_tabtypes((unsigned char *)"\r");
	break;
      default:
	process_regular_char_input(c);
	break;
      } /* end swtich on char type */
    } /* end each char */
    myfprintf((stderr,"See paste data, %s, nread %d, nitems %d\n",
	    data, (int)nread, (int)nitems));
    nread += nitems;
    XFree (data);
  } while (bytes_after > 0);
}
static void ToggleChoice (Item *item)
{
  int i;
  Item *sel = item->choice.sel;

  if (sel->selection.key == IS_SINGLE) {
    if (!item->choice.on) {
      for (i = 0; i < sel->selection.n; i++) {
	if (sel->selection.choices[i]->choice.on) {
	  sel->selection.choices[i]->choice.on = 0;
	  RedrawItem(sel->selection.choices[i], 0, NULL);
	}
      }
      item->choice.on = 1;
      RedrawItem(item, 0, NULL);
    }
  } else {  /* IS_MULTIPLE */
    item->choice.on = !item->choice.on;
    RedrawItem(item, 0, NULL);
  }
}
static void ResizeFrame (void) {
#if 0
  /* unfinished dje. */
  /* If you ever finish this, please make sure to check the return code of
   * XGetGeometry below. dv */
  Window root;
  XEvent dummy;
  int x, y;
  unsigned int border, depth, width, height;
  /* get anything queued */
  while (FCheckTypedWindowEvent(dpy, CF.frame, ConfigureNotify, &dummy))
    ;
  XGetGeometry(dpy, CF.frame, &root, &x, &y, &width, &height, &border, &depth);
  if (width != CF.max_width) {
    if (CF.last_error != 0) {           /* if form has message area */
      fprintf(stderr, "Frame was %d, is %d\n",CF.max_width,width);
      /* RedrawText sets x = item->header.pos_x + TEXT_SPC;
	 MassageConfig does :
	   line->size_x += (line->n + 1) * ITEM_HSPC; =(10)=
	   if (line->size_x > CF.max_width)
	     CF.max_width = line->size_x;
	   (70 + 1) * 10 = 700??
      */
      int delta;
      int curr_x;
      delta = width - CF.max_width;     /* new width - curr width */
      curr_x = CF.last_error->header.pos_x + TEXT_SPC;
      curr_end = curr_x + CF.last_error->size_x;

    }
  }
#endif
}
