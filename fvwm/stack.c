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

#include "fvwm.h"
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "stack.h"
#include "virtual.h"
#include "defaults.h"
#include "gnome.h"


/* ----------------------------- stack ring code --------------------------- */

/* Remove a window from the stack ring */
void remove_window_from_stack_ring(FvwmWindow *t)
{
  t->stack_prev->stack_next = t->stack_next;
  t->stack_next->stack_prev = t->stack_prev;
  return;
}

/* Add window t to the stack ring after window t2 */
void add_window_to_stack_ring_after(FvwmWindow *t, FvwmWindow *add_after_win)
{
  t->stack_next = add_after_win->stack_next;
  add_after_win->stack_next->stack_prev = t;
  t->stack_prev = add_after_win;
  add_after_win->stack_next = t;
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

/* Takes a window from the top of the stack ring and puts it at the appropriate
 * place. Called when new windows are created. */
Bool position_new_window_in_stack_ring(FvwmWindow *t, Bool do_lower)
{
  if (t->stack_prev != &Scr.FvwmRoot)
    /* Not at top of stack ring, so it is already in place. add_window.c relies
     * on this. */
    return False;
  /* RaiseWindow/LowerWindow will put the window in its layer */
  if (do_lower)
  {
    LowerWindow(t);
  }
  else
  {
    RaiseWindow(t);
  }
  return True;
}

static Bool must_move_transients(FvwmWindow *t, Bool do_lower, Bool *found_transient)
{
  *found_transient = False;

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
	if (!do_lower && !scanning_above_window)
	{
	  /* It's below the window, so it must be raised. */
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

static void inner_RaiseOrLowerWindow(FvwmWindow *t, Bool do_lower, Bool allow_recursion)
{
  FvwmWindow *s, *r, *t2, *next, tmp_r;
  unsigned int flags;
  int i, count;
  XWindowChanges changes;
  Window *wins;
  Bool do_move_transients;
  Bool found_transient;
  Bool no_movement;
  int test_layer;

  if ( allow_recursion )
  {
    /*
    ** This part makes Raise/Lower on a Transient act on its Main and sibling Transients.
    **
    ** The recursion is limited to one level - which caters for most cases.
    ** This code does not handle the case where there are trees of Main+Transient
    ** (ie where a Main_window_with_Transients is itself Transient for another window).
    **
    ** Another strategy is required to handle trees of Main+Transients
    */
    if (IS_TRANSIENT(t) && DO_STACK_TRANSIENT_PARENT(t))
    {
      for (t2 = Scr.FvwmRoot.stack_next; t2 != &Scr.FvwmRoot; t2 = t2->stack_next)
      {
        if (t2->w == t->transientfor)
        {
          if ((!do_lower && DO_RAISE_TRANSIENT(t2)) ||
              (do_lower && DO_LOWER_TRANSIENT(t2))  )
          {
              inner_RaiseOrLowerWindow(t2,do_lower,False);
              return;
          }
        }
      }
    }
  }

  do_move_transients = must_move_transients(t, do_lower, &found_transient);

  /*
  ** This part implements the Strict part of RaiseTransientStrict style.
  **
  ** There is no need to move the main window when
  ** (1) Window has style RaiseTransientStrict
  ** (2) We are Raising
  ** (3) We will not move any transients
  ** (4) We found transients belonging to this Main
  **
  ** This basically means that Main is already correctly positioned
  ** near the top of its layer with only its transients above it.
  ** To move Main now would place it above its transients - contra
  ** to the RaiseTransientStrict style.
  */
  no_movement = DO_RAISE_TRANSIENT_STRICT(t)
                                && !do_lower
                                && !do_move_transients
                                && found_transient;

  if ( ! no_movement ) {

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
	  (t2->layer == t->layer))
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
	t2->stack_next = &tmp_r;
	t2->stack_prev = tmp_r.stack_prev;
	t2->stack_prev->stack_next = t2;
	tmp_r.stack_prev = t2;
      }
    }
  }

  test_layer = t->layer;
  if (do_lower)
    test_layer--;
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
    r->stack_next = tmp_r.stack_next;
    tmp_r.stack_next->stack_prev = r;
    s->stack_prev = tmp_r.stack_prev;
    tmp_r.stack_prev->stack_next = s;
  }

  /* don't forget t itself */
  add_window_to_stack_ring_after(t, s->stack_prev);

  wins = (Window*) safemalloc (count * sizeof (Window));

  i = 0;
  for (t2 = r->stack_next; t2 != s; t2 = t2->stack_next)
    {
      if (i >= count) {
	fvwm_msg (ERR, "RaiseOrLowerWindow", "more transients than expected");
	break;
      }
      wins[i++] = t2->frame;
      if (IS_ICONIFIED(t2) && !IS_ICON_SUPPRESSED(t2))
	{
	  if(t2->icon_w != None)
	    wins[i++] = t2->icon_w;
	  if(t2->icon_pixmap_w != None)
	    wins[i++] = t2->icon_pixmap_w;
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

  if (do_move_transients)
    /* send out M_RESTACK for all windows, to make sure we don't forget
     * anything. */
    BroadcastRestack (Scr.FvwmRoot.stack_next, Scr.FvwmRoot.stack_prev);
  else
    /* send out (one or more) M_RESTACK packets for windows between r and s */
    BroadcastRestack (r, s);

  free (wins);

  }

  if (!do_lower)
  {
    raisePanFrames();

    /*
     * The following is a hack to raise X windows over native windows
     * which is needed for some (all ?) X servers running under windows NT.
     */
    if (Scr.bo.RaiseHackNeeded)
    {
      Window junk;
      Window *tops;
      int i;
      unsigned int num;
      Bool found = False;

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
    }
  }
}

static void RaiseOrLowerWindow(FvwmWindow *t, Bool do_lower)
{
  inner_RaiseOrLowerWindow(t, do_lower, True);
}
/*
   Raise t and its transients to the top of its layer.
   For the pager to work properly it is necessary that
   RaiseWindow *always* sends a proper M_RESTACK packet,
   even if the stacking order didn't change.
*/
void RaiseWindow(FvwmWindow *t)
{
  RaiseOrLowerWindow(t, False);
  return;
}

void LowerWindow(FvwmWindow *t)
{
  RaiseOrLowerWindow(t, True);
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
             intersect (x, y, w, h, r->icon_x_loc, r->icon_y_loc,
			r->icon_p_width, r->icon_p_height)) ||
           ((r->icon_w) &&
             intersect (x, y, w, h, r->icon_xl_loc,
			r->icon_y_loc + r->icon_p_height,
			r->icon_w_width, r->icon_w_height));
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
             overlap_box (s, r->icon_x_loc, r->icon_y_loc,
                             r->icon_p_width, r->icon_p_height)) ||
           ((r->icon_w) &&
             overlap_box (s, r->icon_xl_loc, r->icon_y_loc + r->icon_p_height,
                             r->icon_w_width, r->icon_w_height));
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
    for (t = r->stack_prev; (t != &Scr.FvwmRoot) && !restack; t = t->stack_prev)
    {
      restack = (((s == NULL) || (s == t)) && overlap (t, r));
    }
    if (restack)
    {
      RaiseWindow (r);
    }
    break;
   case BottomIf:
    for (t = r->stack_next; (t != &Scr.FvwmRoot) && !restack; t = t->stack_next)
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
  return restack;
}




/*
    RBW - 01/07/1998  -  this is here temporarily - I mean to move it to
    libfvwm eventually, along with some other chain manipulation functions.
*/

/*
    ResyncFvwmStackRing -
    Rebuilds the stacking order ring of FVWM-managed windows. For use in cases
    where apps raise/lower their own windows in a way that makes it difficult
    to determine exactly where they ended up in the stacking order.
    - Based on code from Matthias Clasen.
*/
void ResyncFvwmStackRing (void)
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
	      if (t1->icon_w == children[i])
	        {
	          break;
	        }
              else if (t1->icon_pixmap_w == children[i])
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
              Move the window to its new position, working from the bottom up
              (that's the way XQueryTree presents the list).
          */
	  /* Pluck from chain. */
	  remove_window_from_stack_ring(t1);
	  add_window_to_stack_ring_after(t1, t2->stack_prev);
          t2 = t1;
	}
    }

  MyXUngrabServer (dpy);

  XFree (children);
}



/* send RESTACK packets for all windows between s1 and s2 */
void BroadcastRestack (FvwmWindow *s1, FvwmWindow *s2)
{
  FvwmWindow *t, *t2;
  int num, i;
  unsigned long *body, *bp, length;
  extern Time lastTimestamp;

  if (s1 == &Scr.FvwmRoot)
   {
      t = s1->stack_next;
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
  for (t2 = t, num = 1 ; t2 != s2; t2 = t2->stack_next, num++)
    ;

  length = FvwmPacketHeaderSize + 3*num;
  body = (unsigned long *) safemalloc (length*sizeof(unsigned long));

  bp = body;
  *(bp++) = START_FLAG;
  *(bp++) = M_RESTACK;
  *(bp++) = length;
  *(bp++) = lastTimestamp;
  for (t2 = t; t2 != s2; t2 = t2->stack_next)
   {
      *(bp++) = t2->w;
      *(bp++) = t2->frame;
      *(bp++) = (unsigned long)t2;
   }
   for (i = 0; i < npipes; i++)
     PositiveWrite(i, body, length*sizeof(unsigned long));

   free (body);
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

void
new_layer (FvwmWindow *tmp_win, int layer)
{
  FvwmWindow *t2, *next;

  if (layer < tmp_win->layer)
    {
      tmp_win->layer = layer;
      /* temporary fix; new transient handling code assumes the layers are
       * properly arranged when inner_RaiseOrLowerWindow() is called. */
      ResyncFvwmStackRing();
      /* domivogt (9-Sep-1999): this was RaiseWindow before. The intent may
       * have been good (put the window on top of the new lower layer), but it
       * doesn't work with applications like the gnome panel that use the layer
       * hint as a poor man's raise/lower, i.e. if panel is on the top level
       * and requests to be lowered to the normal level it stays on top of all
       * normal windows. This is not what intended to do with lowering
       * itself. */
      LowerWindow(tmp_win);
    }
  else if (layer > tmp_win->layer)
    {
      if (DO_RAISE_TRANSIENT(tmp_win))
	{
	  /* this could be done much more efficiently */
	  for (t2 = Scr.FvwmRoot.stack_next; t2 != &Scr.FvwmRoot; t2 = next)
	    {
	      next = t2->stack_next;
	      if ((IS_TRANSIENT(t2)) &&
		  (t2->transientfor == tmp_win->w) &&
		  (t2 != tmp_win) &&
		  (t2->layer >= tmp_win->layer) &&
		  (t2->layer < layer))
		{
		  t2->layer = layer;
		  LowerWindow(t2);
		}
	    }
	}
      tmp_win->layer = layer;
      /* see comment above */
      ResyncFvwmStackRing();
      /* see comment above */
      RaiseWindow(tmp_win);
    }

#ifdef GNOME
  GNOME_SetLayer (tmp_win);
#endif
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

/* ----------------------------- built in functions ------------------------ */

void raise_function(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
    return;

  RaiseWindow(tmp_win);
}

void lower_function(F_CMD_ARGS)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT, ButtonRelease))
    return;

  LowerWindow(tmp_win);
}

void raiselower_func(F_CMD_ARGS)
{
  FvwmWindow *x; /* sorry about this name, couldn't think of a better one */
  Bool ontop = True;

  if (DeferExecution(eventp,&w,&tmp_win,&context, CRS_SELECT,ButtonRelease))
    return;

  for (x = tmp_win->stack_prev; x != &Scr.FvwmRoot; x = x->stack_prev) {
    if (x->layer > tmp_win->layer) {
      break;
    }
    if (overlap(tmp_win, x)) {
      ontop = False;
      break;
    }
  }

  if (ontop)
    LowerWindow(tmp_win);
  else
    RaiseWindow(tmp_win);
}

void change_layer(F_CMD_ARGS)
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

  new_layer (tmp_win, layer);
}

void SetDefaultLayers(F_CMD_ARGS)
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
}
