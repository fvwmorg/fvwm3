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
#include <sys/time.h>
#include <signal.h>
#include <stdarg.h>

#include "libs/fvwmlib.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "defaults.h"
#include "module_interface.h"
#include "focus.h"
#include "stack.h"
#include "events.h"
#include "borders.h"
#include "virtual.h"
#include "gnome.h"

/* ----------------------------- stack ring code --------------------------- */

static void RaiseOrLowerWindow(
  FvwmWindow *t, Bool do_lower, Bool allow_recursion, Bool is_new_window);
#if 0
static void ResyncFvwmStackRing(void);
#endif
static void ResyncXStackingOrder(void);
static void BroadcastRestack(FvwmWindow *s1, FvwmWindow *s2);

#define DEBUG_STACK_RING 1
#ifdef DEBUG_STACK_RING
/* debugging function */
static void dump_stack_ring(void)
{
  FvwmWindow *t1;

  if (!debugging_stack_ring)
    return;
  XBell(dpy, 0);
  fprintf(stderr,"dumping stack ring:\n");
  for (t1 = Scr.FvwmRoot.stack_next; t1 != &Scr.FvwmRoot; t1 = t1->stack_next)
  {
    fprintf(stderr,"    l=%d fw=0x%08x f=0x%08x '%s'\n", t1->layer,
	    (int)t1, (int)t1->frame, t1->name);
  }

  return;
}

/* debugging function */
void verify_stack_ring_consistency(void)
{
  Window root, parent, *children;
  unsigned int nchildren, i;
  FvwmWindow *t1, *t2;
  int last_layer;
  int last_index;

  if (!debugging_stack_ring)
    return;
  XSync(dpy, 0);
  t2 = Scr.FvwmRoot.stack_next;
  if (t2 == &Scr.FvwmRoot)
    return;
  last_layer = t2->layer;

  for (t1 = t2->stack_next; t1 != &Scr.FvwmRoot; t2 = t1, t1 = t1->stack_next)
  {
    if (t1->layer > last_layer)
    {
      fprintf(
	stderr,
	"vsrc: stack ring is corrupt! "
	"'%s' (layer %d) is above '%s' (layer %d/%d)\n",
	t1->name, t1->layer, t2->name, t2->layer, last_layer);
      dump_stack_ring();
      return;
    }
    last_layer = t1->layer;
  }
  t2 = &Scr.FvwmRoot;
  for (t1 = t2->stack_next; t1 != &Scr.FvwmRoot; t2 = t1, t1 = t1->stack_next)
  {
    if (t1->stack_prev != t2)
      break;
  }
  if (t1 != &Scr.FvwmRoot || t1->stack_prev != t2)
  {
    fprintf(
      stderr,
      "vsrc: stack ring is corrupt - fvwm will probably crash! "
      "0x%08x -> 0x%08x but 0x%08x <- 0x%08x\n",
      (int)t2, (int)t1, (int)(t1->stack_prev), (int)t1);
    dump_stack_ring();
    return;
  }
  MyXGrabServer(dpy);
  if (!XQueryTree(dpy, Scr.Root, &root, &parent, &children, &nchildren))
  {
    MyXUngrabServer(dpy);
    return;
  }

  last_index = nchildren;
  for (t1 = Scr.FvwmRoot.stack_next; t1 != &Scr.FvwmRoot; t1 = t1->stack_next)
  {
    /* find window in window list */
    for (i = 0; i < nchildren && t1->frame != children[i]; i++)
      ;
    if (i == nchildren)
    {
      fprintf(stderr,"vsrc: window already died: fw=0x%08x w=0x%08x '%s'\n",
	      (int)t1, (int)t1->frame, t1->name);
    }
    else if (i >= last_index)
    {
      fprintf(
	stderr, "vsrc: window is at wrong position in stack ring: "
	"fw=0x%08x f=0x%08x '%s'\n", (int)t1, (int)t1->frame, t1->name);
      dump_stack_ring();
      fprintf(stderr,"dumping X stacking order:\n");
      for (i = nchildren; i-- > 0; )
      {
	for (t1 = Scr.FvwmRoot.stack_next; t1 != &Scr.FvwmRoot;
	     t1 = t1->stack_next)
	{
	  /* only dump frame windows */
	  if (t1->frame == children[i])
	  {
	    fprintf(stderr,"  f=0x%08x\n", (int)children[i]);
	    break;
	  }
	}
      }
      MyXUngrabServer(dpy);
      XFree(children);
      return;
    }
    last_index = i;
  }
  MyXUngrabServer(dpy);
  XFree(children);

  return;
}
#endif

/* Remove a window from the stack ring */
void remove_window_from_stack_ring(FvwmWindow *t)
{
  if (IS_SCHEDULED_FOR_DESTROY(t))
  {
    return;
  }
  t->stack_prev->stack_next = t->stack_next;
  t->stack_next->stack_prev = t->stack_prev;
  /* not really necessary, but gives a little more saftey */
  t->stack_prev = NULL;
  t->stack_next = NULL;
  return;
}

/* Add window t to the stack ring after window t */
void add_window_to_stack_ring_after(FvwmWindow *t, FvwmWindow *add_after_win)
{
  if (IS_SCHEDULED_FOR_DESTROY(t))
  {
    return;
  }
  if (t == add_after_win || t == add_after_win->stack_next)
  {
    /* tried to add the window before or after itself */
    fvwm_msg(ERR, "add_window_to_stack_ring_after",
	     "BUG: tried to add window '%s' %s itself in stack ring\n", t->name,
	     (t == add_after_win) ? "after" : "before");
    return;
  }
  t->stack_next = add_after_win->stack_next;
  add_after_win->stack_next->stack_prev = t;
  t->stack_prev = add_after_win;
  add_after_win->stack_next = t;
  return;
}

/* Add a whole ring of windows. The list_head itself will not be added. */
static void add_windowlist_to_stack_ring_after(
  FvwmWindow *list_head, FvwmWindow *add_after_win)
{
  add_after_win->stack_next->stack_prev = list_head->stack_prev;
  list_head->stack_prev->stack_next = add_after_win->stack_next;
  add_after_win->stack_next = list_head->stack_next;
  list_head->stack_next->stack_prev = add_after_win;
  return;
}

FvwmWindow *get_next_window_in_stack_ring(FvwmWindow *t)
{
  return t->stack_next;
}

FvwmWindow *get_prev_window_in_stack_ring(FvwmWindow *t)
{
  return t->stack_prev;
}

FvwmWindow *get_transientfor_fvwmwindow(FvwmWindow *t)
{
  FvwmWindow *s;

  if (!t || !IS_TRANSIENT(t) || t->transientfor == Scr.Root ||
      t->transientfor == None)
    return NULL;
  for (s = Scr.FvwmRoot.next; s != NULL; s = s->next)
  {
    if (s->w == t->transientfor)
    {
      return (s == t) ? NULL : s;
    }
  }

  return NULL;
}

static FvwmWindow *get_transientfor_top_fvwmwindow(FvwmWindow *t)
{
  FvwmWindow *s;

  s = t;
  while (s && IS_TRANSIENT(s) && DO_STACK_TRANSIENT_PARENT(s))
  {
    s = get_transientfor_fvwmwindow(s);
    if (s)
      t = s;
  }

  return t;
}

/* Takes a window from the top of the stack ring and puts it at the appropriate
 * place. Called when new windows are created. */
Bool position_new_window_in_stack_ring(FvwmWindow *t, Bool do_lower)
{
  if (t->stack_prev != &Scr.FvwmRoot)
    /* Not at top of stack ring, so it is already in place. add_window.c relies
     * on this. */
    return False;
  /* RaiseWindow/LowerWindow will put the window in its layer */
  RaiseOrLowerWindow(t, do_lower, False, True);
  return True;
}


/********************************************************************
 * Raise a target and all higher FVWM-managed windows above any
 *  override_redirects:
 *  - locate the highest override_redirect above our target
 *  - put all the FvwmWindows from the target to the highest FvwmWindow
 *    below the highest override_redirect in the restack list
 *  - configure our target window above the override_redirect sibling,
 *    and restack.
 ********************************************************************/
static void raise_over_unmanaged(FvwmWindow *t)
{
  Window junk;
  Window *tops;
  int i;
  unsigned int num;
  Window OR_Above = None;
  Window *wins;
  int count = 0;
  FvwmWindow *t2 = NULL;
  unsigned int flags;
  XWindowChanges changes;
  XWindowAttributes wa;

  if (!XQueryTree(dpy, Scr.Root, &junk, &junk, &tops, &num))
    return;

  /********************************************************************
   * Locate the highest override_redirect window above our target, and
   * the highest of our windows below it.
   ********************************************************************/
  for (i = 0; i < num && tops[i] != t->frame; i++)
  {
    /* look for target window in list */
  }
  for (; i < num; i++)
  {
    /* It might be just as well (and quicker) just to check for the absence of
     * an FvwmContext instead of for override_redirect... */
    if (!XGetWindowAttributes(dpy, tops[i], &wa))
    {
      continue;
    }
    if (wa.override_redirect == True && wa.class != InputOnly &&
	tops[i] != Scr.NoFocusWin)
    {
      OR_Above = tops[i];
    }
  } /* end for */

  /********************************************************************
   * Count the windows we need to restack, then build the stack list.
   ********************************************************************/
  if (OR_Above)
  {
    for (count = 0, t2 = Scr.FvwmRoot.stack_next; t2 != &Scr.FvwmRoot;
	 t2 = t2->stack_next)
    {
      count++;
      if (IS_ICONIFIED(t2) && ! IS_ICON_SUPPRESSED(t2))
      {
	if (t2->icon_w != None)
	{
	  count++;
        }
	if (t2->icon_pixmap_w != None)
	{
	  count++;
        }
      }
      if (t2 == t)
      {
	break;
      }
    }

    if (count > 0)
    {
      wins = (Window*) safemalloc (count * sizeof (Window));
      for (i = 0, t2 = Scr.FvwmRoot.stack_next; t2 != &Scr.FvwmRoot;
	   t2 = t2->stack_next)
      {
	wins[i++] = t2->frame;
	if (IS_ICONIFIED(t2) && ! IS_ICON_SUPPRESSED(t2))
	{
	  if (t2->icon_w != None)
	  {
	    wins[i++] = t2->icon_w;
          }
	  if (t2->icon_pixmap_w != None)
	  {
	    wins[i++] = t2->icon_pixmap_w;
          }
        }
	if (t2 == t)
	{
	  break;
	}
      }

      memset(&changes, '\0', sizeof(changes));
      changes.sibling = OR_Above;
      changes.stack_mode = Above;
      flags = CWSibling|CWStackMode;

      XConfigureWindow (dpy, t->frame/*topwin*/, flags, &changes);
      XRestackWindows (dpy, wins, i);
      free (wins);
    }
  }/*  end - we found an OR above our target  */

  XFree (tops);
  return;
}


static Bool must_move_transients(
  FvwmWindow *t, Bool do_lower, Bool *found_transient)
{
  *found_transient = False;

  if (IS_ICONIFIED(t))
  {
    return False;
  }
  /* raise */
  if ((!do_lower && DO_RAISE_TRANSIENT(t)) ||
      (do_lower && DO_LOWER_TRANSIENT(t)))
  {
    Bool scanning_above_window = True;
    FvwmWindow *q;

    for (q = Scr.FvwmRoot.stack_next; q != &Scr.FvwmRoot; q = q->stack_next)
    {
      if (t->layer < q->layer)
      {
	/* We're not interested in higher layers. */
	continue;
      }
      if (t->layer > q->layer)
      {
	/* We are at the end of this layer. Stop scanning windows. */
	break;
      }
      else if (t == q)
      {
	/* We found our window. All further transients are below it. */
	scanning_above_window = False;
      }
      else if (IS_TRANSIENT(q) && (q->transientfor == t->w))
      {
	/* found a transient */
	*found_transient = True;

        /* transients are not allowed below main in the same layer */
        if ( !scanning_above_window )
        {
          return True;
        }
      }
      else if ((scanning_above_window && !do_lower) ||
	       (*found_transient && do_lower))
      {
	/* raise: The window is not raised, so itself and all transients will
	 * be raised. */
	/* lower: There is a transient above some other window, so we have to
	 * lower. */
	return True;
      }
    }
  }
  return False;
}

static void restack_windows(
  FvwmWindow *r, FvwmWindow *s, int count, Bool do_broadcast_all)
{
  FvwmWindow *t;
  unsigned int flags;
  int i;
  XWindowChanges changes;
  Window *wins;

  if (count <= 0)
  {
    FvwmWindow *prev;

    for (t = r, prev = NULL; prev != s; prev = t, t = t->stack_next)
    {
      count++;
      if (IS_ICONIFIED(t) && !IS_ICON_SUPPRESSED(t))
      {
	count += 2;
      }
    }
  }
  /* restack the windows between r and s */
  wins = (Window*) safemalloc (count * sizeof (Window));
  i = 0;
  for (t = r->stack_next; t != s; t = t->stack_next)
  {
    if (i >= count)
    {
      fvwm_msg (ERR, "restack_windows", "more transients than expected");
      break;
    }
    wins[i++] = t->frame;
    if (IS_ICONIFIED(t) && !IS_ICON_SUPPRESSED(t))
    {
      if(t->icon_w != None)
	wins[i++] = t->icon_w;
      if(t->icon_pixmap_w != None)
	wins[i++] = t->icon_pixmap_w;
    }
  }

  changes.sibling = s->frame;
  if (changes.sibling != None)
  {
    changes.stack_mode = Above;
    flags = CWSibling|CWStackMode;
  }
  else
  {
    changes.stack_mode = Below;
    flags = CWStackMode;
  }
  XConfigureWindow (dpy, r->stack_next->frame, flags, &changes);
  XRestackWindows (dpy, wins, count);
  free(wins);
  if (do_broadcast_all)
  {
    /* send out M_RESTACK for all windows, to make sure we don't forget
     * anything. */
    BroadcastRestackAllWindows();
  }
  else
  {
    /* send out (one or more) M_RESTACK packets for windows between r and s */
    BroadcastRestack(r, s);
  }

  return;
}

static void RaiseOrLowerWindow(
  FvwmWindow *t, Bool do_lower, Bool allow_recursion, Bool is_new_window)
{
  FvwmWindow *s, *r, *t2, *next, tmp_r;
  int count;
  Bool do_move_transients;
  Bool found_transient;
  Bool no_movement;
  int test_layer;

  /* Do not raise this window after command execution (see HandleButtonPress()).
   */
  SET_SCHEDULED_FOR_RAISE(t, 0);

  /* New windows are simply raised/lowered without touching the transientfor
   * at first.  Then, further down in the code, RaiseOrLowerWindow() is called
   * again to raise/lower the transientfor if necessary.  We can not do the
   * recursion stuff for new windows because the must_move_transients() call
   * needs a properly ordered stack ring - but the new window is still at the
   * front of the stack ring. */
  if (allow_recursion && !is_new_window && !IS_ICONIFIED(t))
  {
    /*
     * This part makes Raise/Lower on a Transient act on its Main and sibling
     * Transients.
     *
     * The recursion is limited to one level - which caters for most cases.
     * This code does not handle the case where there are trees of Main +
     * Transient (ie where a Main_window_with_Transients is itself Transient
     * for another window).
     *
     * Another strategy is required to handle trees of Main+Transients
     */
    if (IS_TRANSIENT(t) && DO_STACK_TRANSIENT_PARENT(t))
    {
      for (t2 = Scr.FvwmRoot.stack_next; t2 != &Scr.FvwmRoot;
	   t2 = t2->stack_next)
      {
        if (t2->w == t->transientfor)
        {
	  if (IS_ICONIFIED(t2))
	  {
	    break;
	  }
          if ((!do_lower && DO_RAISE_TRANSIENT(t2)) ||
              (do_lower && DO_LOWER_TRANSIENT(t2))  )
          {
	    RaiseOrLowerWindow(t2,do_lower,False,False);
	    return;
          }
        }
      }
    }
  }

  if (is_new_window)
  {
    no_movement = False;
    do_move_transients = False;
  }
  else
  {
    do_move_transients = must_move_transients(t, do_lower, &found_transient);

    /*
     * This part implements (Dont)FlipTransient style.
     *
     * must_move_transients() believes that main should always be below
     * its transients. Therefore if it finds a transient but does not wish
     * move it, this means main and its superior transients are already
     * in the correct position at the top or bottom of the layer - depending
     * on do_lower.
     */
    no_movement = found_transient && !do_move_transients;
  }

  if (!no_movement)
  {
    /* detach t, so it doesn't make trouble in the loops */
    remove_window_from_stack_ring(t);

    count = 1;
    if (IS_ICONIFIED(t) && !IS_ICON_SUPPRESSED(t))
    {
      count += 2;
    }

    if (do_move_transients)
    {
      /* collect the transients in a temp list */
      tmp_r.stack_prev = &tmp_r;
      tmp_r.stack_next = &tmp_r;
      for (t2 = Scr.FvwmRoot.stack_next; t2 != &Scr.FvwmRoot; t2 = next)
      {
	next = t2->stack_next;
	if ((IS_TRANSIENT(t2)) && (t2->transientfor == t->w) &&
	    (t2->layer == t->layer) && !IS_ICONIFIED(t2))
	{
	  /* t2 is a transient to lower */
	  count++;
	  if (IS_ICONIFIED(t2) && !IS_ICON_SUPPRESSED(t2))
	  {
	    count += 2;
	  }

	  /* unplug it */
	  remove_window_from_stack_ring(t2);
	  /* put it above tmp_r */
	  add_window_to_stack_ring_after(t2, tmp_r.stack_prev);
	}
      }
      if (tmp_r.stack_next == &tmp_r)
      {
	do_move_transients = False;
      }
    }

    test_layer = t->layer;
    if (do_lower)
    {
      test_layer--;
    }
    /* now find the place to reinsert t and friends */
    for (s = Scr.FvwmRoot.stack_next; s != &Scr.FvwmRoot; s = s->stack_next)
    {
      if (test_layer >= s->layer)
      {
	break;
      }
    }
    r = s->stack_prev;

    if (do_move_transients && tmp_r.stack_next != &tmp_r)
    {
      /* insert all transients between r and s. */
      add_windowlist_to_stack_ring_after(&tmp_r, r);
    }

    /*
    ** Re-insert t - below transients
    */
    add_window_to_stack_ring_after(t, s->stack_prev);

    if (is_new_window && IS_TRANSIENT(t) && DO_STACK_TRANSIENT_PARENT(t) &&
	!IS_ICONIFIED(t))
    {
      /* now that the new transient is properly positioned in the stack ring,
       * raise/lower it again so that its parent is raised/lowered too */
      RaiseOrLowerWindow(t, do_lower, True, False);
      /* make sure the stacking order is correct - may be the sledge-hammer
       * method, but the recursion ist too hard to understand. */
      ResyncXStackingOrder();
      return;
    }
    else
    {
      /* restack the windows between r and s */
      restack_windows(
	r, s, count, (do_move_transients && tmp_r.stack_next != &tmp_r));
    }
  }

  if (!do_lower)
  {
    /* This hack raises the target and all higher FVWM windows over any style
     * grabfocusoff override_redirect windows that may be above it. This is
     * used to cope with ill-behaved applications that insist on using
     * long-lived override_redirects. */
    if (Scr.bo.RaiseOverUnmanaged)
    {
      raise_over_unmanaged(t);
    }


    /*
     * The following is a hack to raise X windows over native windows
     * which is needed for some (all ?) X servers running under Windows
     * or Windows NT.
     */
    if (Scr.bo.RaiseHackNeeded)
    {
      /*
          RBW - 09/20/1999. I find that trying to raise unmanaged windows
          causes problems with some apps. If this seems to work well for
          everyone, I'll remove the #if 0.
      */
#if 0
      /* get *all* toplevels (even including override_redirects) */
      XQueryTree(dpy, Scr.Root, &junk, &junk, &tops, &num);

      /* raise from tmp_win upwards to get them above NT windows */
      for (i = 0; i < num; i++)
      {
        if (tops[i] == t->frame)
	  found = True;
        if (found)
	  XRaiseWindow (dpy, tops[i]);
      }
      XFree (tops);
#endif
      for (t2 = t; t2 != &Scr.FvwmRoot; t2 = t2->stack_prev)
      {
        XRaiseWindow (dpy, t2->frame);
      }
    }


    /*  This needs to be done after all the raise hacks.  */
    raisePanFrames();
  }
  /* If the window has been raised, make sure the decorations are updated
   * immediately in case we are in a complex function (e.g. raise, unshade). */
  if (!do_lower)
  {
    XSync(dpy, 0);
    handle_all_expose();
  }
}

/*
   Raise t and its transients to the top of its layer.
   For the pager to work properly it is necessary that
   RaiseWindow *always* sends a proper M_RESTACK packet,
   even if the stacking order didn't change.
*/
void RaiseWindow(FvwmWindow *t)
{
  BroadcastPacket(M_RAISE_WINDOW, 3, t->w, t->frame, (unsigned long)t);
  RaiseOrLowerWindow(t, False, True, False);
  focus_grab_buttons_on_pointer_window();
#ifdef DEBUG_STACK_RING
  verify_stack_ring_consistency();
#endif
  return;
}

void LowerWindow(FvwmWindow *t)
{
  BroadcastPacket(M_LOWER_WINDOW, 3, t->w, t->frame, (unsigned long)t);
  RaiseOrLowerWindow(t, True, True, False);
  focus_grab_buttons_on_pointer_window();
#ifdef DEBUG_STACK_RING
  verify_stack_ring_consistency();
#endif
  return;
}


static Bool
intersect (int x0, int y0, int w0, int h0,
           int x1, int y1, int w1, int h1)
{
  return !((x0 >= x1 + w1) || (x0 + w0 <= x1) ||
           (y0 >= y1 + h1) || (y0 + h0 <= y1));
}

static Bool
overlap_box (FvwmWindow *r, int x, int y, int w, int h)
{
  if (IS_ICONIFIED(r))
  {
    return ((r->icon_pixmap_w) &&
             intersect (x, y, w, h, r->icon_g.x, r->icon_g.y,
			r->icon_p_width, r->icon_p_height)) ||
           ((r->icon_w) &&
             intersect (x, y, w, h, r->icon_xl_loc,
			r->icon_g.y + r->icon_p_height,
			r->icon_g.width, r->icon_g.height));
  }
  else
  {
    return intersect (x, y, w, h, r->frame_g.x, r->frame_g.y,
                           r->frame_g.width, r->frame_g.height);
  }
}

static Bool
overlap (FvwmWindow *r, FvwmWindow *s)
{
  if (r->Desk != s->Desk)
  {
    return 0;
  }

  if (IS_ICONIFIED(r))
  {
    return ((r->icon_pixmap_w) &&
             overlap_box (s, r->icon_g.x, r->icon_g.y,
                             r->icon_p_width, r->icon_p_height)) ||
           ((r->icon_w) &&
             overlap_box (s, r->icon_xl_loc, r->icon_g.y + r->icon_p_height,
                             r->icon_g.width, r->icon_g.height));
  }
  else
  {
    return overlap_box (s, r->frame_g.x, r->frame_g.y,
                           r->frame_g.width, r->frame_g.height);
  }
}

/* return true if stacking order changed */
Bool
HandleUnusualStackmodes(unsigned int stack_mode, FvwmWindow *r, Window rw,
                                                 FvwmWindow *s, Window sw)
{
  Bool restack = 0;
  FvwmWindow *t;

/*  DBUG("HandleUnusualStackmodes", "called with %d, %lx\n", stack_mode, s);*/

  if (((rw != r->w) ^ IS_ICONIFIED(r)) ||
      (s && (((sw != s->w) ^ IS_ICONIFIED(s)) || (r->Desk != s->Desk))))
  {
    /* one of the relevant windows is unmapped */
    return 0;
  }

  switch (stack_mode)
  {
   case TopIf:
    for (t = r->stack_prev; (t != &Scr.FvwmRoot) && !restack;
	 t = t->stack_prev)
    {
      restack = (((s == NULL) || (s == t)) && overlap (t, r));
    }
    if (restack)
    {
      RaiseWindow (r);
    }
    break;
   case BottomIf:
    for (t = r->stack_next; (t != &Scr.FvwmRoot) && !restack;
	 t = t->stack_next)
    {
      restack = (((s == NULL) || (s == t)) && overlap (t, r));
    }
    if (restack)
    {
      LowerWindow (r);
    }
    break;
   case Opposite:
    restack = (HandleUnusualStackmodes (TopIf, r, rw, s, sw) ||
               HandleUnusualStackmodes (BottomIf, r, rw, s, sw));
    break;
  }
/*  DBUG("HandleUnusualStackmodes", "\t---> %d\n", restack);*/
#ifdef DEBUG_STACK_RING
  verify_stack_ring_consistency();
#endif
  return restack;
}




/*
    RBW - 01/07/1998  -  this is here temporarily - I mean to move it to
    libfvwm eventually, along with some other chain manipulation functions.
*/

#if 0
/*
    ResyncFvwmStackRing -
    Rebuilds the stacking order ring of FVWM-managed windows. For use in cases
    where apps raise/lower their own windows in a way that makes it difficult
    to determine exactly where they ended up in the stacking order.
    - Based on code from Matthias Clasen.
*/
static void ResyncFvwmStackRing (void)
{
  Window root, parent, *children;
  unsigned int nchildren, i;
  FvwmWindow *t1, *t2;

  MyXGrabServer (dpy);

  if (!XQueryTree (dpy, Scr.Root, &root, &parent, &children, &nchildren))
  {
    MyXUngrabServer (dpy);
    return;
  }

  t2 = &Scr.FvwmRoot;
  for (i = 0; i < nchildren; i++)
  {
    for (t1 = Scr.FvwmRoot.next; t1 != NULL; t1 = t1->next)
    {
      if (IS_ICONIFIED(t1) && !IS_ICON_SUPPRESSED(t1))
      {
	if (t1->icon_w == children[i] || t1->icon_pixmap_w == children[i])
	{
	  break;
	}
      }
      else
      {
	if (t1->frame == children[i])
	{
	  break;
	}
      }
    }

    if (t1 != NULL && t1 != t2)
    {
      /*
       * Move the window to its new position, working from the bottom up
       * (that's the way XQueryTree presents the list).
       */
      /* Pluck from chain. */
      remove_window_from_stack_ring(t1);
      add_window_to_stack_ring_after(t1, t2->stack_prev);
      if (t2 != &Scr.FvwmRoot && t2->layer > t1->layer)
      {
	/* oops, now our stack ring is out of order! */
	/* emergency fix */
	t1->layer = t2->layer;
      }
      t2 = t1;
    }
  }

  MyXUngrabServer (dpy);

  XFree (children);
}
#endif

/* same as above but synchronizes the stacking order in X from the stack ring.
 */
static void ResyncXStackingOrder(void)
{
  Window *wins;
  FvwmWindow *t;
  int count;
  int i;

  for (count = 0, t = Scr.FvwmRoot.next; t != NULL; count++, t = t->next)
    ;
  if (count > 0)
  {
    wins = (Window *)safemalloc(3 * count * sizeof (Window));

    for (i = 0, t = Scr.FvwmRoot.stack_next; count--; t = t->stack_next)
    {
      wins[i++] = t->frame;
      if (IS_ICONIFIED(t) && !IS_ICON_SUPPRESSED(t))
      {
	if (t->icon_w != None)
	  wins[i++] = t->icon_w;
	if (t->icon_pixmap_w != None)
	  wins[i++] = t->icon_pixmap_w;
      }
    }
    XRestackWindows(dpy, wins, i);
    free(wins);
    /* send out M_RESTACK for all windows, to make sure we don't forget
     * anything. */
    BroadcastRestackAllWindows();
  }
}


/* send RESTACK packets for all windows between s1 and s2 */
static void BroadcastRestack(FvwmWindow *s1, FvwmWindow *s2)
{
  FvwmWindow *t, *t2;
  int num, i;
  unsigned long *body, *bp, length;
  extern Time lastTimestamp;

  if (s2 == &Scr.FvwmRoot)
  {
    s2 = s2->stack_prev;
    if (s2 == &Scr.FvwmRoot)
      return;
  }
  if (s1 == &Scr.FvwmRoot)
  {
    t = s1->stack_next;
    if (t == &Scr.FvwmRoot)
      return;
    /* t has been moved to the top of stack */
    BroadcastPacket (M_RAISE_WINDOW, 3, t->w, t->frame, (unsigned long)t);
    if (t->stack_next == s2)
    {
      /* avoid sending empty RESTACK packet */
      return;
    }
  }
  else
  {
    t = s1;
  }
  if (s1 == s2)
  {
    /* A useful M_RESTACK packet must contain at least two windows. */
    return;
  }
  for (t2 = t, num = 1 ; t2 != s2 && t != &Scr.FvwmRoot;
       t2 = t2->stack_next, num++)
    ;
  length = FvwmPacketHeaderSize + 3*num;
  body = (unsigned long *) safemalloc (length*sizeof(unsigned long));

  bp = body;
  *(bp++) = START_FLAG;
  *(bp++) = M_RESTACK;
  *(bp++) = length;
  *(bp++) = lastTimestamp;
  for (t2 = t; num != 0; num--, t2 = t2->stack_next)
  {
    *(bp++) = t2->w;
    *(bp++) = t2->frame;
    *(bp++) = (unsigned long)t2;
  }
  for (i = 0; i < npipes; i++)
    PositiveWrite(i, body, length*sizeof(unsigned long));
  free(body);
#ifdef DEBUG_STACK_RING
  verify_stack_ring_consistency();
#endif

  return;
}

void BroadcastRestackAllWindows(void)
{
  BroadcastRestack(Scr.FvwmRoot.stack_next, Scr.FvwmRoot.stack_prev);
  return;
}

/* send RESTACK packets for t, t->stack_prev and t->stack_next */
void BroadcastRestackThisWindow(FvwmWindow *t)
{
  BroadcastRestack(t->stack_prev, t->stack_next);
  return;
}

/* ----------------------------- layer code -------------------------------- */

/* returns 0 if s and t are on the same layer, <1 if t is on a lower layer and
 * >1 if t is on a higher layer. */
int compare_window_layers(FvwmWindow *t, FvwmWindow *s)
{
  return t->layer - s->layer;
}

void set_default_layer(FvwmWindow *t, int layer)
{
  t->default_layer = layer;
  return;
}

void set_layer(FvwmWindow *t, int layer)
{
  t->layer = layer;
  return;
}

int get_layer(FvwmWindow *t)
{
  return t->layer;
}

/* This function recursively finds the transients of the window t and sets their
 * is_in_transient_subtree flag.  If a layer is given, only windows in this
 * layer are checked.  If the layer is < 0, all windows are considered.
 */
void mark_transient_subtree(
  FvwmWindow *t, int layer, int mark_mode, Bool do_ignore_icons,
  Bool use_window_group_hint)
{
  FvwmWindow *s;
  FvwmWindow *start;
  FvwmWindow *end;
  Bool is_finished;

  if (layer >= 0 && t->layer != layer)
    return;
  /* find out on which windows to operate */
  if (layer >= 0)
  {
    /* only work on the given layer */
    start = &Scr.FvwmRoot;
    end = &Scr.FvwmRoot;
    for (s = Scr.FvwmRoot.stack_next; s != &Scr.FvwmRoot && s->layer >= layer;
	 s = s->stack_next)
    {
      if (s->layer == layer)
      {
	if (start == &Scr.FvwmRoot)
	  start = s;
	end = s->stack_next;
      }
    }
  }
  else
  {
    /* work on complete window list */
    start = Scr.FvwmRoot.stack_next;
    end = &Scr.FvwmRoot;
  }
  /* clean the temporary flag in all windows and precalculate the transient
   * frame windows */
  for (s = Scr.FvwmRoot.stack_next; s != &Scr.FvwmRoot; s = s->stack_next)
  {
    SET_IN_TRANSIENT_SUBTREE(s, 0);
    if (IS_TRANSIENT(s) && (layer < 0 || layer == s->layer))
    {
      s->pscratch = get_transientfor_fvwmwindow(s);
    }
    else
    {
      s->pscratch = NULL;
    }
  }
  /* now loop over the windows and mark the ones we need to move */
  SET_IN_TRANSIENT_SUBTREE(t, 1);
  is_finished = False;
  while (!is_finished)
  {
    FvwmWindow *r;
    FvwmWindow *u;

    /* recursively search for all transient windows */
    is_finished = True;
    for (s = start; s != end; s = s->stack_next)
    {
      Bool use_group_hint = False;

      if (IS_IN_TRANSIENT_SUBTREE(s))
	continue;
      if (DO_ICONIFY_WINDOW_GROUPS(s) && s->wmhints &&
	  (s->wmhints->flags & WindowGroupHint) &&
	  (s->wmhints->window_group != None) &&
	  (s->wmhints->window_group != s->w) &&
	  (s->wmhints->window_group != Scr.Root))
      {
	use_group_hint = True;
      }
      if (!IS_TRANSIENT(s) && !use_group_hint)
	continue;
      r = (FvwmWindow *)s->pscratch;
      if (do_ignore_icons && r && IS_ICONIFIED(r))
	continue;
      if (IS_TRANSIENT(s))
      {
	if (r && IS_IN_TRANSIENT_SUBTREE(r) &&
	    ((mark_mode == MARK_ALL) ||
	     (mark_mode == MARK_LOWER && DO_LOWER_TRANSIENT(r)) ||
	     (mark_mode == MARK_RAISE && DO_RAISE_TRANSIENT(r))))
	{
	  /* have to move this one too */
	  SET_IN_TRANSIENT_SUBTREE(s, 1);
	  /* need another scan through the list */
	  is_finished = False;
	  continue;
	}
      }
      if (use_group_hint)
      {
	for (u = start; u != end; u = u->stack_next)
	{
	  if (u->w == s->wmhints->window_group ||
	      (u->wmhints && (u->wmhints->flags & WindowGroupHint) &&
	       u->wmhints->window_group == s->wmhints->window_group))
	  {
	    if (IS_IN_TRANSIENT_SUBTREE(u))
	    {
	      /* have to move this one too */
	      SET_IN_TRANSIENT_SUBTREE(s, 1);
	      /* need another scan through the list */
	      is_finished = False;
	    }
	  }
	}
      }
    } /* for */
  } /* while */

  return;
}

static int collect_transients_recursive(
  FvwmWindow *t, FvwmWindow *list_head, int layer, Bool do_lower)
{
  FvwmWindow *s;
  int count = 0;

  mark_transient_subtree(
    t, layer, (do_lower) ? MARK_LOWER : MARK_RAISE, True, False);
  /* now collect the marked windows in a separate list */
  for (s = Scr.FvwmRoot.stack_next; s != &Scr.FvwmRoot; )
  {
    FvwmWindow *tmp;

    tmp = s->stack_next;
    if (IS_IN_TRANSIENT_SUBTREE(s))
    {
      remove_window_from_stack_ring(s);
      add_window_to_stack_ring_after(s, list_head->stack_prev);
      count++;
      if (IS_ICONIFIED(t) && !IS_ICON_SUPPRESSED(t))
	count += 2;
    }
    s = tmp;
  }

  return count;
}

void new_layer(FvwmWindow *tmp_win, int layer)
{
  FvwmWindow *s;
  FvwmWindow *target;
  FvwmWindow *prev;
  FvwmWindow list_head;
  int add_after_layer;
  int count;

  tmp_win = get_transientfor_top_fvwmwindow(tmp_win);
  if (layer == tmp_win->layer)
    return;
  list_head.stack_next = &list_head;
  list_head.stack_prev = &list_head;
  count = collect_transients_recursive(
    tmp_win, &list_head, tmp_win->layer, (layer < tmp_win->layer));
  if (count == 0)
  {
    /* no windows to move */
    return;
  }

  add_after_layer = layer;
  if (layer < tmp_win->layer)
  {
    /* lower below the windows in the new (lower) layer */
    add_after_layer = layer;
  }
  else
  {
    /* raise above the windows in the new (higher) layer */
    add_after_layer = layer + 1;
  }
  /* find the place to insert the windows */
  for (target = Scr.FvwmRoot.stack_next; target != &Scr.FvwmRoot;
       target = target->stack_next)
  {
    if (target->layer < add_after_layer)
    {
      /* add all windows before the current window */
      break;
    }
  }
  /* insert windows at new position */
  add_windowlist_to_stack_ring_after(&list_head, target->stack_prev);
  prev = NULL;
  for (s = list_head.stack_next; prev != list_head.stack_prev;
       prev = s, s = s->stack_next)
  {
    s->layer = layer;
    GNOME_SetLayer(tmp_win);
  }
  /* move the windows without modifying their stacking order */
  restack_windows(list_head.stack_next->stack_prev, target, count, (count > 1));

  return;
}

/* ----------------------------- common functions -------------------------- */


  /*  RBW - 11/13/1998 - 2 new fields to init - stacking order chain.  */
void init_stack_and_layers(void)
{
  Scr.BottomLayer  = DEFAULT_BOTTOM_LAYER;
  Scr.DefaultLayer = DEFAULT_DEFAULT_LAYER;
  Scr.TopLayer     = DEFAULT_TOP_LAYER;
  Scr.FvwmRoot.stack_next = &Scr.FvwmRoot;
  Scr.FvwmRoot.stack_prev = &Scr.FvwmRoot;
  set_layer(&Scr.FvwmRoot, DEFAULT_ROOT_WINDOW_LAYER);
  return;
}


static Bool is_on_top_of_layer_ignore_rom(FvwmWindow *fw)
{
  FvwmWindow *t;
  Bool ontop = True;

  for (t = fw->stack_prev; t != &Scr.FvwmRoot; t = t->stack_prev)
  {
    if (t->layer > fw->layer)
    {
      break;
    }
    if (t->Desk != fw->Desk)
    {
      continue;
    }
    /* For RaiseOverUnmanaged we can not determine if the window is on top by
     * checking if the window overlaps another one.  If it was below unmanaged
     * windows, but on top of its layer, it would be considered on top. */
    if (Scr.bo.RaiseOverUnmanaged || overlap(fw, t))
    {
      if (!IS_TRANSIENT(t) || t->transientfor != fw->w ||
	  !DO_RAISE_TRANSIENT(fw))
      {
	ontop = False;
	break;
      }
    }
  }

  return ontop;
}

Bool is_on_top_of_layer(FvwmWindow *fw)
{
  if (Scr.bo.RaiseOverUnmanaged)
  {
    return False;
  }
  return is_on_top_of_layer_ignore_rom(fw);
}

/* ----------------------------- built in functions ------------------------ */

void CMD_Raise(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
    return;

  RaiseWindow(tmp_win);
}

void CMD_Lower(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT, ButtonRelease))
    return;

  LowerWindow(tmp_win);
}

void CMD_RaiseLower(F_CMD_ARGS)
{
  Bool ontop;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
  {
#ifdef DEBUG_STACK_RING
    dump_stack_ring();
#endif
    return;
  }
  ontop = is_on_top_of_layer_ignore_rom(tmp_win);
  if (ontop)
    LowerWindow(tmp_win);
  else
    RaiseWindow(tmp_win);

  return;
}

void CMD_Layer(F_CMD_ARGS)
{
  int n, layer, val[2];
  char *token;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
    return;

  if(tmp_win == NULL)
    return;

  token = PeekToken(action, NULL);
  if (StrEquals("default", token))
  {
    layer = tmp_win->default_layer;
  }
  else
  {
    n = GetIntegerArguments(action, NULL, val, 2);

    layer = tmp_win->layer;
    if ((n == 1) ||
	((n == 2) && (val[0] != 0)))
    {
      layer += val[0];
    }
    else if ((n == 2) && (val[1] >= 0))
    {
      layer = val[1];
    }
    else
    {
      layer = tmp_win->default_layer;
    }
  }
  if (layer < 0)
  {
    layer = 0;
  }
  new_layer(tmp_win, layer);

#ifdef DEBUG_STACK_RING
  verify_stack_ring_consistency();
#endif
}

void CMD_DefaultLayers(F_CMD_ARGS)
{
  char *bot = NULL;
  char *def = NULL;
  char *top = NULL;
  int i;

  bot = PeekToken(action, &action);
  if (bot)
  {
    i = atoi (bot);
    if (i < 0)
    {
      fvwm_msg(ERR,"DefaultLayers", "Layer must be non-negative." );
    }
    else
    {
      Scr.BottomLayer = i;
    }
  }

  def = PeekToken(action, &action);
  if (def)
  {
    i = atoi (def);
    if (i < 0)
    {
      fvwm_msg(ERR,"DefaultLayers", "Layer must be non-negative." );
    }
    else
    {
      Scr.DefaultLayer = i;
    }
  }

  top = PeekToken(action, &action);
  if (top)
  {
    i = atoi (top);
    if (i < 0)
    {
      fvwm_msg(ERR,"DefaultLayers", "Layer must be non-negative." );
    }
    else
    {
      Scr.TopLayer = i;
    }
  }

#ifdef DEBUG_STACK_RING
  verify_stack_ring_consistency();
#endif
}
