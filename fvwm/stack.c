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
#include "parse.h"
#include "screen.h"
#include "module_interface.h"
#include "stack.h"
#include "virtual.h"


/*
   Raise t and its transients to the top of its layer.
   For the pager to work properly it is necessary that
   RaiseWindow *always* sends a proper M_RESTACK packet,
   even if the stacking order didn't change.
*/
void RaiseWindow(FvwmWindow *t)
{
  FvwmWindow *s, *r, *t2, *next, tmp_r, tmp_s;
  unsigned int flags;
  int i, count;
  XWindowChanges changes;
  Window *wins;

  /* detach t early, so it doesn't make trouble in the loops */
  t->stack_prev->stack_next = t->stack_next;
  t->stack_next->stack_prev = t->stack_prev;

  count = 1;
  if (IS_ICONIFIED(t) && !IS_ICON_SUPPRESSED(t))
    {
      count += 2;
    }

#ifndef DONT_RAISE_TRANSIENTS
  /* collect the transients in a temp list */
  tmp_s.stack_prev = &tmp_r;
  tmp_r.stack_next = &tmp_s;
  for (t2 = Scr.FvwmRoot.stack_next; t2 != &Scr.FvwmRoot; t2 = next)
    {
      next = t2->stack_next;
      if ((IS_TRANSIENT(t2)) && (t2->transientfor == t->w) &&
	  (t2->layer == t->layer))
        {
          /* t2 is a transient to raise */
          count++;
          if (IS_ICONIFIED(t2) && !IS_ICON_SUPPRESSED(t2))
            {
	      count += 2;
            }

          /* unplug it */
          t2->stack_next->stack_prev = t2->stack_prev;
          t2->stack_prev->stack_next = t2->stack_next;

          /* put it above tmp_s */
          t2->stack_next = &tmp_s;
          t2->stack_prev = tmp_s.stack_prev;
          t2->stack_next->stack_prev = t2;
          t2->stack_prev->stack_next = t2;
	}
    }
#endif /* DONT_RAISE_TRANSIENTS */

  /* now find the place to reinsert t and friends */
  for (s = Scr.FvwmRoot.stack_next; s != &Scr.FvwmRoot; s = s->stack_next)
    {
      if (t->layer >= s->layer)
	{
	  break;
        }
    }
  r = s->stack_prev;

#ifndef DONT_RAISE_TRANSIENTS
  /* insert all transients between r and s. */
  r->stack_next = tmp_r.stack_next;
  r->stack_next->stack_prev = r;
  s->stack_prev = tmp_s.stack_prev;
  s->stack_prev->stack_next = s;
#endif /* DONT_RAISE_TRANSIENTS */

  /* don't forget t itself */
  t->stack_next = s;
  t->stack_prev = s->stack_prev;
  t->stack_prev->stack_next = t;
  t->stack_next->stack_prev = t;

  wins = (Window*) safemalloc (count * sizeof (Window));

  i = 0;
  for (t2 = r->stack_next; t2 != s; t2 = t2->stack_next)
    {
      if (i >= count) {
	fvwm_msg (ERR, "RaiseWindow", "more transients than expected");
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

  /* send out (one or more) M_RESTACK packets for windows between r and s */
  BroadcastRestack (r, s);

  free (wins);

  raisePanFrames();

  /*
     The following is a hack to raise X windows over native windows
     which is needed for some (all ?) X servers running under windows NT.
   */
  if (Scr.go.RaiseHackNeeded)
    {
      Window junk;
      Window *tops;
      int i, num;
      Bool found = False;

      /* get *all* toplevels (even including override_redirects) */
      XQueryTree (dpy, Scr.Root, &junk, &junk, &tops, &num);

      /* raise from tmp_win upwards to get them above NT windows */
      for (i = 0; i < num; i++) {
        if (tops[i] == t->frame) found = True;
        if (found) XRaiseWindow (dpy, tops[i]);
      }

      XFree (tops);
    }
}


void LowerWindow(FvwmWindow *t)
{
  FvwmWindow *s;
  XWindowChanges changes;
  unsigned int flags;

  for (s = Scr.FvwmRoot.stack_next; s != &Scr.FvwmRoot; s = s->stack_next)
    {
      if (t->layer > s->layer)
        {
	  break;
        }
    }

  if (s == t->stack_next)
    {
      return;
    }


  t->stack_prev->stack_next = t->stack_next;
  t->stack_next->stack_prev = t->stack_prev;

  t->stack_next = s;
  t->stack_prev = s->stack_prev;
  t->stack_prev->stack_next = t;
  t->stack_next->stack_prev = t;

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

  XConfigureWindow(dpy, t->frame, flags, &changes);

  if (IS_ICONIFIED(t) && !IS_ICON_SUPPRESSED(t))
    {
      changes.sibling = t->frame;
      changes.stack_mode = Below;
      flags = CWSibling|CWStackMode;
      if (t->icon_w != None)
	XConfigureWindow(dpy, t->icon_w, flags, &changes);
      if (t->icon_pixmap_w != None)
	XConfigureWindow(dpy, t->icon_pixmap_w, flags, &changes);
    }

  BroadcastRestack (t->stack_prev, t->stack_next);
}


void
new_layer (FvwmWindow *tmp_win, int layer)
{
  FvwmWindow *t2, *next;

  if (layer < tmp_win->layer)
    {
      tmp_win->layer = layer;
      RaiseWindow(tmp_win);
    }
  else if (layer > tmp_win->layer)
    {
#ifndef DONT_RAISE_TRANSIENTS
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
#endif
      tmp_win->layer = layer;
      LowerWindow(tmp_win);
    }

#ifdef GNOME
  GNOME_SetLayer (tmp_win);
#endif
}

static Bool
intersect (int x0, int y0, int w0, int h0,
           int x1, int y1, int w1, int h1)
{
  return !((x0 > x1 + w1) || (x0 + w0 < x1) ||
           (y0 > y1 + h1) || (y0 + h0 < y1));
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
             intersect (x, y, w, h, r->icon_xl_loc, r->icon_y_loc + r->icon_p_height,
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
void  ResyncFvwmStackRing (void)
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
          t1->stack_prev->stack_next = t1->stack_next;  /* Pluck from chain. */
          t1->stack_next->stack_prev = t1->stack_prev;
          t1->stack_next = t2;                          /* Set new pointers. */
          t1->stack_prev = t2->stack_prev;
          t2->stack_prev->stack_next = t1;              /* Insert in new
							 * position. */
          t2->stack_prev = t1;
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

/* return false if the only windows above tmp_win in the same
   layer are its own transients
*/
Bool
CanBeRaised (FvwmWindow *tmp_win)
{
  FvwmWindow *t;

  for (t = tmp_win->stack_prev; t != &Scr.FvwmRoot; t = t->stack_prev)
    {
      if (t->layer > tmp_win->layer) return False;
      if (!IS_TRANSIENT(t) || (t->transientfor != tmp_win->w)) return True;
    }
  return False;
}
