/* This irogram is free software; you can redistribute it and/or modify
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
 * Changed 10/06/97 by dje:
 * Change single IconBox into chain of IconBoxes.
 * Allow IconBox to be specified using X Geometry string.
 * Parse optional IconGrid.
 * Parse optional IconFill.
 * Use macros to make parsing more uniform, and easier to read.
 * Rewrote AddToList without tons of arg passing and merging.
 * Added a few comments.
 *
 * This module was all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * code for parsing the fvwm style command
 *
 ***********************************************************************/
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "style.h"
#include "fvwm.h"
#include "fvwmlib.h"
#include "misc.h"
#include "screen.h"
#include "gnome.h"
#include "builtins.h"
#include "libs/Colorset.h"
#include "borders.h"

/* list of window names with attributes */
static window_style *all_styles = NULL;
static window_style *last_style_in_list = NULL;

#define SAFEFREE( p )  {if (p)  free(p);}


static void free_icon_boxes(icon_boxes *ib)
{
  icon_boxes *temp;

  for ( ; ib != NULL; ib = temp)
  {
    temp = ib->next;
    free(ib);
  }
}

static void copy_icon_boxes(icon_boxes **pdest, icon_boxes *src)
{
  icon_boxes *last = NULL;
  icon_boxes *temp;

  *pdest = NULL;
  /* copy the icon boxes */
  for ( ; src != NULL; src = src->next)
  {
    temp = (icon_boxes *)safemalloc(sizeof(icon_boxes));
    memcpy(temp, src, sizeof(icon_boxes));
    temp->next = NULL;
    if (last != NULL)
      last->next = temp;
    else
      *pdest = temp;
    last = temp;
  }
}

/* Check word after IconFill to see if its "Top,Bottom,Left,Right" */
static int Get_TBLR(char *token, unsigned char *IconFill) {
  /* init */
  if (StrEquals(token, "B") ||
      StrEquals(token, "BOT")||
      StrEquals(token, "BOTTOM"))
  {
    /* turn on bottom and verical */
    *IconFill = ICONFILLBOT | ICONFILLHRZ;
  }
  else if (StrEquals(token, "T") ||
	   StrEquals(token, "TOP"))
  {
    /* turn on vertical */
    *IconFill = ICONFILLHRZ;
  }
  else if (StrEquals(token, "R") ||
	   StrEquals(token, "RGT") ||
	   StrEquals(token, "RIGHT"))
  {
    /* turn on right bit */
    *IconFill = ICONFILLRGT;
  }
  else if (StrEquals(token, "L") ||
	   StrEquals(token, "LFT") ||
	   StrEquals(token, "LEFT"))
  {
    *IconFill = 0;
  }
  else
  {
    /* anything else is bad */
    return 0;
  }

  /* return OK */
  return 1;
}

/***********************************************************************
 *
 *  Procedure:
 * merge_styles - For a matching style, merge window_style to window_style
 *
 *  Returned Value:
 *      merged matching styles in callers window_style.
 *
 *  Inputs:
 *      merged_style - style resulting from the merge
 *      add_style    - the style to be added into the merge_style
 *      do_free      - free allocated parts of merge_style that are replaced
 +                     from ad_style
 *
 *  Note:
 *      The only trick here is that on and off flags/buttons are
 *      combined into the on flag/button.
 *
 ***********************************************************************/

static void merge_styles(window_style *merged_style, window_style *add_style,
			 Bool do_free)
{
  int i;
  char *merge_flags;
  char *add_flags;
  char *merge_mask;
  char *add_mask;
  char *merge_change_mask;
  char *add_change_mask;

  if (add_style->flag_mask.has_icon)
  {
    if (do_free)
    {
      SAFEFREE(merged_style->icon_name);
      merged_style->icon_name =
	(add_style->icon_name) ? strdup(add_style->icon_name) : NULL;
    }
    else
    {
      merged_style->icon_name = add_style->icon_name;
    }
  }
#ifdef MINI_ICONS
  if (add_style->flag_mask.has_mini_icon)
  {
    if (do_free)
    {
      SAFEFREE(merged_style->mini_icon_name);
      merged_style->mini_icon_name =
	(add_style->mini_icon_name) ? strdup(add_style->mini_icon_name) : NULL;
    }
    else
    {
      merged_style->mini_icon_name = add_style->mini_icon_name;
    }
  }
#endif
#ifdef USEDECOR
  if (add_style->flag_mask.has_decor)
  {
    if (do_free)
    {
      SAFEFREE(merged_style->decor_name);
      merged_style->decor_name =
	(add_style->decor_name) ? strdup(add_style->decor_name) : NULL;
    }
    else
    {
      merged_style->decor_name = add_style->decor_name;
    }
  }
#endif
  if(add_style->flags.use_start_on_desk)
  /*  RBW - 11/02/1998  */
  {
    merged_style->start_desk = add_style->start_desk;
    merged_style->start_page_x = add_style->start_page_x;
    merged_style->start_page_y = add_style->start_page_y;
  }
  if (add_style->flag_mask.has_color_fore)
  {
    if (do_free)
    {
      SAFEFREE(merged_style->fore_color_name);
      merged_style->fore_color_name =
	(add_style->fore_color_name) ?
	strdup(add_style->fore_color_name) : NULL;
    }
    else
    {
      merged_style->fore_color_name = add_style->fore_color_name;
    }
  }
  if (add_style->flag_mask.has_color_back)
  {
    if (do_free)
    {
      SAFEFREE(merged_style->back_color_name);
      merged_style->back_color_name =
	(add_style->back_color_name) ?
	strdup(add_style->back_color_name) : NULL;
    }
    else
    {
      merged_style->back_color_name = add_style->back_color_name;
    }
  }
  if(add_style->flags.has_border_width)
  {
    merged_style->border_width = add_style->border_width;
  }
  if(add_style->flags.has_handle_width)
  {
    merged_style->handle_width = add_style->handle_width;
  }
  if(add_style->flags.has_max_window_size)
  {
    merged_style->max_window_width = add_style->max_window_width;
    merged_style->max_window_height = add_style->max_window_height;
  }

  /* merge the style flags */
  merge_flags = (char *)&(merged_style->flags);
  add_flags = (char *)&(add_style->flags);
  merge_mask = (char *)&(merged_style->flag_mask);
  add_mask = (char *)&(add_style->flag_mask);
  merge_change_mask = (char *)&(merged_style->change_mask);
  add_change_mask = (char *)&(add_style->change_mask);
  for (i = 0; i < sizeof(style_flags); i++)
  {
    merge_flags[i] |= (add_flags[i] & add_mask[i]);
    merge_flags[i] &= (add_flags[i] | ~add_mask[i]);
    merge_mask[i] |= add_mask[i];
    merge_change_mask[i] &= ~(add_mask[i]);
    merge_change_mask[i] |= add_change_mask[i];
  }

  /* Note, only one style cmd can define a windows iconboxes,
   * the last one encountered. */
  if(add_style->IconBoxes != NULL)
  {
    /* If style has iconboxes */
    /* copy it */
    if (do_free)
    {
      free_icon_boxes(merged_style->IconBoxes);
      copy_icon_boxes(&merged_style->IconBoxes, add_style->IconBoxes);
    }
    else
    {
      merged_style->IconBoxes = add_style->IconBoxes;
    }
  }
  if (add_style->flags.use_layer)
  {
    merged_style->layer = add_style->layer;
  }
  if (add_style->flags.use_colorset)
  {
    merged_style->colorset = add_style->colorset;
  }
  merged_style->has_style_changed |= add_style->has_style_changed;
  return;
}

static void free_style(window_style *style)
{
  /* Free contents of style */
  SAFEFREE( style->name );
  SAFEFREE( style->back_color_name );
  SAFEFREE( style->fore_color_name );
  SAFEFREE( style->mini_icon_name );
  SAFEFREE( style->decor_name );
  SAFEFREE( style->icon_name );
  free_icon_boxes( style->IconBoxes );

  return;
}

static void merge_style_list(void)
{
  window_style *temp;
  window_style *prev = NULL;

  /* not first entry in list, look for styles with the same name */
  for (temp = all_styles; temp != NULL; prev = temp, temp = temp->next)
  {
    if (prev && StrEquals(prev->name, temp->name))
    {
      /* merge style into previous style with same name */
      if (last_style_in_list == temp)
	last_style_in_list = prev;
      merge_styles(prev, temp, True);
      prev->next = temp->next;
      free_style(temp);
      temp = prev;
    }
  }

  return;
}

static void add_style_to_list(window_style *new_style)
{
  /* This used to contain logic that returned if the style didn't contain
   * anything.  I don't see why we should bother. dje.
   *
   * used to merge duplicate entries, but that is no longer
   * appropriate since conficting styles are possible, and the
   * last match should win! */

  if(last_style_in_list != NULL)
  {
    /* not first entry in list chain this entry to the list */
    last_style_in_list->next = new_style;
  }
  else
  {
    /* first entry in list set the list root pointer. */
    all_styles = new_style;
  }
  last_style_in_list = new_style;
  merge_style_list();

  return;
} /* end function */


static void remove_all_of_style_from_list(char *style_ref)
{
  window_style *nptr = all_styles, *lptr = NULL;

  /* loop though styles */
  while (nptr)
  {
    /* Check if it's to be wiped */
    if (!strcmp( nptr->name, style_ref ))
    {
      window_style *tmp_ptr = nptr;

      /* Reset nptr now */
      nptr = nptr->next;

      /* Is it the first one? */
      if (NULL == lptr)
      {
        /* Yup - reset all_styles */
        all_styles = nptr;
      }
      else
      {
        /* Middle of list */
        lptr->next = nptr;
      }
      free_style(tmp_ptr);
      /* Free style */
      free( tmp_ptr );
    }
    else
    {
      /* No match - move on */
      lptr = nptr;
      nptr = nptr->next;
    }
  }

} /* end function */


/* Remove a style. */
void ProcessDestroyStyle( XEvent *eventp, Window w, FvwmWindow *tmp_win,
                          unsigned long context,char *action, int *Module )
{
  char *name;

  /* parse style name */
  action = GetNextToken(action, &name);

  /* in case there was no argument! */
  if(name == NULL)
    return;

  /* Do it */
  remove_all_of_style_from_list( name );
  free( name );
  /* compact the current list of styles */
  merge_style_list();
}


/***********************************************************************
 *
 *  Procedure:
 *      lookup_style - look through a list for a window name, or class
 *
 *  Returned Value:
 *      merged matching styles in callers window_style.
 *
 *  Inputs:
 *      tmp_win - FvwWindow structure to match against
 *      styles - callers return area
 *
 *  Changes:
 *      dje 10/06/97 test for NULL class removed, can't happen.
 *      use merge subroutine instead of coding merges 3 times.
 *      Use structure to return values, not many, many args
 *      and return value.
 *      Point at iconboxes chain, not single iconboxes elements.
 *
 ***********************************************************************/
void lookup_style(FvwmWindow *tmp_win, window_style *styles)
{
  window_style *nptr;

  /* clear callers return area */
  memset(styles, 0, sizeof(window_style));
#ifdef GNOME
  /* initialize with GNOME hints */
  GNOME_GetStyle (tmp_win, styles);
#endif
  /* look thru all styles in order defined. */
  for (nptr = all_styles; nptr != NULL; nptr = nptr->next)
  {
    /* If name/res_class/res_name match, merge */
    if (matchWildcards(nptr->name,tmp_win->class.res_class) == TRUE)
    {
      merge_styles(styles, nptr, False);
    }
    else if (matchWildcards(nptr->name,tmp_win->class.res_name) == TRUE)
    {
      merge_styles(styles, nptr, False);
    }
    else if (matchWildcards(nptr->name,tmp_win->name) == TRUE)
    {
      merge_styles(styles, nptr, False);
    }
  }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *      cmp_masked_flags - compare two flag structures passed as byte
 *                         arrays. Only compare bits set in the mask.
 *
 *  Returned Value:
 *      zero if the flags are the same
 *      non-zero otherwise
 *
 *  Inputs:
 *      flags1 - first byte array of flags to compare
 *      flags2 - second byte array of flags to compare
 *      mask   - byte array of flags to be considered for the comparison
 *      len    - number of bytes to compare
 *
 ***********************************************************************/
int cmp_masked_flags(void *flags1, void *flags2, void *mask, int len)
{
  int i;

  for (i = 0; i < len; i++)
    {
      if ( (((char *)flags1)[i] & ((char *)mask)[i]) !=
	   (((char *)flags2)[i] & ((char *)mask)[i]) )
	/* flags are not the same, return 1 */
	return 1;
    }
  return 0;
}


/* Process a style command.  First built up in a temp area.
 * If valid, added to the list in a malloced area.
 *
 *
 *                    *** Important note ***
 *
 * Remember that *all* styles need a flag, flag_mask and change_mask.
 * It is not enough to add the code for new styles in this function.
 * There *must* be corresponding code in handle_new_window_style()
 * and merge_styles() too.  And don't forget that allocated memory
 * must be freed in ProcessDestroyStyle().
 *
 */
void ProcessNewStyle(XEvent *eventp, Window w, FvwmWindow *tmp_win,
                     unsigned long context,char *action, int *Module )
{
  char *line;
  char *option;
  char *token;
  char *rest;
  window_style *add_style;
  /* work area for button number */
  int butt;
  int num;
  int i;
  /*  RBW - 11/02/1998  */
  int tmpno[3] = { -1, -1, -1 };
  int val[4];
  int spargs = 0;
  Bool found;
/**/
  /* temp area to build name list */
  window_style *ptmpstyle;
  /* which current boxes to chain to */
  icon_boxes *which = 0;

  ptmpstyle = (window_style *)safemalloc(sizeof(window_style));
  /* init temp window_style area */
  memset(ptmpstyle, 0, sizeof(window_style));
  /* mark style as changed */
  ptmpstyle->has_style_changed = 1;
  /* set global flag */
  Scr.flags.has_any_style_changed = 1;
  /* default StartsOnPage behavior for initial capture */
  ptmpstyle->flags.capture_honors_starts_on_page = 1;

  /* parse style name */
  action = GetNextToken(action, &ptmpstyle->name);
  /* in case there was no argument! */
  if(ptmpstyle->name == NULL)
    return;
  if(action == NULL)
  {
    free(ptmpstyle->name);
    return;
  }
  while (isspace((unsigned char)*action))
    action++;
  line = action;

  while (action && *action && *action != '\n')
  {
    action = GetNextFullOption(action, &option);
    if (!option)
      break;
    token = PeekToken(option, &rest);
    if (!token)
    {
      free(option);
      break;
    }

    /* It might make more sense to capture the whole word, fix its
     * case, and use strcmp, but there aren't many caseless compares
     * because of this "switch" on the first letter. */
    found = False;

    switch (tolower(token[0]))
    {
      case 'a':
        if(StrEquals(token, "ACTIVEPLACEMENT"))
        {
	  found = True;
          ptmpstyle->flags.do_place_random = 0;
          ptmpstyle->flag_mask.do_place_random = 1;
          ptmpstyle->change_mask.do_place_random = 1;
        }
	else if(StrEquals(token, "ACTIVEPLACEMENTHONORSSTARTSONPAGE"))
	{
	  found = True;
	  ptmpstyle->flags.active_placement_honors_starts_on_page = 1;
	  ptmpstyle->flag_mask.active_placement_honors_starts_on_page = 1;
	  ptmpstyle->change_mask.active_placement_honors_starts_on_page = 1;
	}
	else if(StrEquals(token, "ACTIVEPLACEMENTIGNORESSTARTSONPAGE"))
	{
	  found = True;
	  ptmpstyle->flags.active_placement_honors_starts_on_page = 0;
	  ptmpstyle->flag_mask.active_placement_honors_starts_on_page = 1;
	  ptmpstyle->change_mask.active_placement_honors_starts_on_page = 1;
	}
        else if(StrEquals(token, "AllowRestack"))
        {
          found = True;
          ptmpstyle->flags.common.s.do_ignore_restack = 0;
          ptmpstyle->flag_mask.common.s.do_ignore_restack = 1;
          ptmpstyle->change_mask.common.s.do_ignore_restack = 1;
        }
        break;

      case 'b':
        if(StrEquals(token, "BACKCOLOR"))
        {
	  found = True;
	  GetNextToken(rest, &token);
          if (token)
          {
            ptmpstyle->back_color_name = token;
            ptmpstyle->flags.has_color_back = 1;
            ptmpstyle->flag_mask.has_color_back = 1;
            ptmpstyle->change_mask.has_color_back = 1;
	    ptmpstyle->flags.use_colorset = 0;
	    ptmpstyle->flag_mask.use_colorset = 1;
	    ptmpstyle->change_mask.use_colorset = 1;
          }
        }
        else if (StrEquals(token, "BUTTON"))
        {
	  found = True;
          butt = -1;
	  GetIntegerArguments(rest, NULL, &butt, 1);
          if (butt == 0)
	    butt = 10;
          if (butt > 0 && butt <= 10)
	  {
            ptmpstyle->flags.is_button_disabled &= ~(1<<(butt-1));
            ptmpstyle->flag_mask.is_button_disabled |= (1<<(butt-1));
            ptmpstyle->change_mask.is_button_disabled |= (1<<(butt-1));
	  }
        }
        else if(StrEquals(token, "BorderWidth"))
        {
	  found = True;
	  if (GetIntegerArguments(rest, NULL, &ptmpstyle->border_width, 1))
	  {
	    ptmpstyle->flags.has_border_width = 1;
	    ptmpstyle->flag_mask.has_border_width = 1;
	    ptmpstyle->change_mask.has_border_width = 1;
	  }
        }
        break;

      case 'c':
	if(StrEquals(token, "CAPTUREHONORSSTARTSONPAGE"))
	{
	  found = True;
	  ptmpstyle->flags.capture_honors_starts_on_page = 1;
	  ptmpstyle->flag_mask.capture_honors_starts_on_page = 1;
	  ptmpstyle->change_mask.capture_honors_starts_on_page = 1;
	}
	else if(StrEquals(token, "CAPTUREIGNORESSTARTSONPAGE"))
	{
	  found = True;
	  ptmpstyle->flags.capture_honors_starts_on_page = 0;
	  ptmpstyle->flag_mask.capture_honors_starts_on_page = 1;
	  ptmpstyle->change_mask.capture_honors_starts_on_page = 1;
	}
        else if(StrEquals(token, "COLORSET"))
	{
	  found = True;
          *val = -1;
	  GetIntegerArguments(rest, NULL, val, 1);
	  ptmpstyle->colorset = *val;
	  ptmpstyle->flags.use_colorset = (*val >= 0);
	  ptmpstyle->flag_mask.use_colorset = 1;
	  ptmpstyle->change_mask.use_colorset = 1;
	}
        else if(StrEquals(token, "COLOR"))
        {
	  char c = 0;
	  char *next;

	  found = True;
	  next = GetNextToken(rest, &token);
	  if (token == NULL)
	    break;
	  if (strncasecmp(token, "rgb:", 4) == 0)
	  {
	    char *s;
	    int i;

	    /* spool to third '/' */
	    for (i = 0, s = token + 4; *s && i < 3; s++)
	    {
	      if (*s == '/')
		i++;
	    }
	    s--;
	    if (i == 3)
	    {
	      *s = 0;
	      /* spool to third '/' in original string too */
	      for (i = 0, s = rest; *s && i < 3; s++)
	      {
		if (*s == '/')
		  i++;
	      }
	      next = s - 1;
	    }
	  }
	  else
	  {
	    free(token);
	    next = DoGetNextToken(rest, &token, NULL, ",/", &c);
	  }
	  rest = next;
	  ptmpstyle->fore_color_name = token;
	  ptmpstyle->flags.has_color_fore = 1;
	  ptmpstyle->flag_mask.has_color_fore = 1;
	  ptmpstyle->change_mask.has_color_fore = 1;
	  ptmpstyle->flags.use_colorset = 0;
	  ptmpstyle->flag_mask.use_colorset = 1;
	  ptmpstyle->change_mask.use_colorset = 1;

	  /* skip over '/' */
	  if (c != '/')
	  {
	    while (rest && *rest && isspace((unsigned char)*rest) &&
		   *rest != ',' && *rest != '/')
	      rest++;
	    if (*rest == '/')
	      rest++;
	  }

	  GetNextToken(rest, &token);
	  if (!token)
	    break;
	  ptmpstyle->back_color_name = token;
	  ptmpstyle->flags.has_color_back = 1;
	  ptmpstyle->flag_mask.has_color_back = 1;
	  ptmpstyle->change_mask.has_color_back = 1;
	  break;
        }
        else if(StrEquals(token, "CirculateSkipIcon"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_circulate_skip_icon = 1;
          ptmpstyle->flag_mask.common.s.do_circulate_skip_icon = 1;
          ptmpstyle->change_mask.common.s.do_circulate_skip_icon = 1;
        }
        else if(StrEquals(token, "CirculateHitIcon"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_circulate_skip_icon = 0;
          ptmpstyle->flag_mask.common.s.do_circulate_skip_icon = 1;
          ptmpstyle->change_mask.common.s.do_circulate_skip_icon = 1;
        }
        else if(StrEquals(token, "CLICKTOFOCUS"))
        {
	  found = True;
          ptmpstyle->flags.common.s.focus_mode = FOCUS_CLICK;
          ptmpstyle->flag_mask.common.s.focus_mode = FOCUS_MASK;
          ptmpstyle->change_mask.common.s.focus_mode = FOCUS_MASK;
          ptmpstyle->flags.common.s.do_grab_focus_when_created = 1;
          ptmpstyle->flag_mask.common.s.do_grab_focus_when_created = 1;
          ptmpstyle->change_mask.common.s.do_grab_focus_when_created = 1;
        }
        else if(StrEquals(token, "CirculateSkip"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_circulate_skip = 1;
          ptmpstyle->flag_mask.common.s.do_circulate_skip = 1;
          ptmpstyle->change_mask.common.s.do_circulate_skip = 1;
        }
        else if(StrEquals(token, "CirculateHit"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_circulate_skip = 0;
          ptmpstyle->flag_mask.common.s.do_circulate_skip = 1;
          ptmpstyle->change_mask.common.s.do_circulate_skip = 1;
        }
        break;

      case 'd':
        if(StrEquals(token, "DepressableBorder"))
	{
	  found = True;
          ptmpstyle->flags.common.s.has_depressable_border = 1;
          ptmpstyle->flag_mask.common.s.has_depressable_border = 1;
          ptmpstyle->change_mask.common.s.has_depressable_border = 1;
	}
        else if(StrEquals(token, "DecorateTransient"))
        {
	  found = True;
          ptmpstyle->flags.do_decorate_transient = 1;
          ptmpstyle->flag_mask.do_decorate_transient = 1;
          ptmpstyle->change_mask.do_decorate_transient = 1;
        }
        else if(StrEquals(token, "DUMBPLACEMENT"))
        {
	  found = True;
          ptmpstyle->flags.do_place_smart = 0;
          ptmpstyle->flag_mask.do_place_smart = 1;
          ptmpstyle->change_mask.do_place_smart = 1;
        }
        else if(StrEquals(token, "DontFlipTransient"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_flip_transient = 0;
          ptmpstyle->flag_mask.common.s.do_flip_transient = 1;
          ptmpstyle->change_mask.common.s.do_flip_transient = 1;
        }
        else if(StrEquals(token, "DONTRAISETRANSIENT"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_raise_transient = 0;
          ptmpstyle->flag_mask.common.s.do_raise_transient = 1;
          ptmpstyle->change_mask.common.s.do_raise_transient = 1;
        }
        else if(StrEquals(token, "DONTLOWERTRANSIENT"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_lower_transient = 0;
          ptmpstyle->flag_mask.common.s.do_lower_transient = 1;
          ptmpstyle->change_mask.common.s.do_lower_transient = 1;
        }
        else if(StrEquals(token, "DontStackTransientParent"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_stack_transient_parent = 0;
          ptmpstyle->flag_mask.common.s.do_stack_transient_parent = 1;
          ptmpstyle->change_mask.common.s.do_stack_transient_parent = 1;
        }
        break;

      case 'e':
        break;

      case 'f':
        if(StrEquals(token, "FirmBorder"))
	{
	  found = True;
          ptmpstyle->flags.common.s.has_depressable_border = 0;
          ptmpstyle->flag_mask.common.s.has_depressable_border = 1;
          ptmpstyle->change_mask.common.s.has_depressable_border = 1;
	}
        else if(StrEquals(token, "FixedPosition"))
	{
	  found = True;
	  ptmpstyle->flags.common.s.is_fixed = 1;
	  ptmpstyle->flag_mask.common.s.is_fixed = 1;
	  ptmpstyle->change_mask.common.s.is_fixed = 1;
	}
        else if(StrEquals(token, "FlipTransient"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_flip_transient = 1;
          ptmpstyle->flag_mask.common.s.do_flip_transient = 1;
          ptmpstyle->change_mask.common.s.do_flip_transient = 1;
        }
        else if(StrEquals(token, "FORECOLOR"))
        {
	  found = True;
	  GetNextToken(rest, &token);
          if (token)
          {
            ptmpstyle->fore_color_name = token;
            ptmpstyle->flags.has_color_fore = 1;
            ptmpstyle->flag_mask.has_color_fore = 1;
            ptmpstyle->change_mask.has_color_fore = 1;
	    ptmpstyle->flags.use_colorset = 0;
	    ptmpstyle->flag_mask.use_colorset = 1;
	    ptmpstyle->change_mask.use_colorset = 1;
          }
        }
        else if(StrEquals(token, "FVWMBUTTONS"))
        {
	  found = True;
          ptmpstyle->flags.common.s.has_mwm_buttons = 0;
          ptmpstyle->flag_mask.common.s.has_mwm_buttons = 1;
          ptmpstyle->change_mask.common.s.has_mwm_buttons = 1;
        }
        else if(StrEquals(token, "FVWMBORDER"))
        {
	  found = True;
          ptmpstyle->flags.common.s.has_mwm_border = 0;
          ptmpstyle->flag_mask.common.s.has_mwm_border = 1;
          ptmpstyle->change_mask.common.s.has_mwm_border = 1;
        }
        else if(StrEquals(token, "FocusFollowsMouse"))
        {
	  found = True;
          ptmpstyle->flags.common.s.focus_mode = FOCUS_MOUSE;
          ptmpstyle->flag_mask.common.s.focus_mode = FOCUS_MASK;
          ptmpstyle->change_mask.common.s.focus_mode = FOCUS_MASK;
          ptmpstyle->flags.common.s.do_grab_focus_when_created = 0;
          ptmpstyle->flag_mask.common.s.do_grab_focus_when_created = 1;
          ptmpstyle->change_mask.common.s.do_grab_focus_when_created = 1;
        }
        break;

      case 'g':
        if(StrEquals(token, "GrabFocusOff"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_grab_focus_when_created = 0;
          ptmpstyle->flag_mask.common.s.do_grab_focus_when_created = 1;
          ptmpstyle->change_mask.common.s.do_grab_focus_when_created = 1;
        }
        else if(StrEquals(token, "GrabFocus"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_grab_focus_when_created = 1;
          ptmpstyle->flag_mask.common.s.do_grab_focus_when_created = 1;
          ptmpstyle->change_mask.common.s.do_grab_focus_when_created = 1;
        }
        else if(StrEquals(token, "GrabFocusTransientOff"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_grab_focus_when_transient_created = 0;
          ptmpstyle->flag_mask.common.s.do_grab_focus_when_transient_created
            = 1;
          ptmpstyle->change_mask.common.s.do_grab_focus_when_transient_created
            = 1;
        }
        else if(StrEquals(token, "GrabFocusTransient"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_grab_focus_when_transient_created = 1;
          ptmpstyle->flag_mask.common.s.do_grab_focus_when_transient_created
            = 1;
          ptmpstyle->change_mask.common.s.do_grab_focus_when_transient_created
            = 1;
        }
        break;

      case 'h':
        if(StrEquals(token, "HINTOVERRIDE"))
        {
	  found = True;
          ptmpstyle->flags.common.s.has_mwm_override = 1;
          ptmpstyle->flag_mask.common.s.has_mwm_override = 1;
          ptmpstyle->change_mask.common.s.has_mwm_override = 1;
        }
        else if(StrEquals(token, "HANDLES"))
        {
	  found = True;
          ptmpstyle->flags.has_no_border = 0;
          ptmpstyle->flag_mask.has_no_border = 1;
          ptmpstyle->change_mask.has_no_border = 1;
        }
        else if(StrEquals(token, "HandleWidth"))
        {
	  found = True;
	  if (GetIntegerArguments(rest, NULL, &ptmpstyle->handle_width, 1))
	  {
	    ptmpstyle->flags.has_handle_width = 1;
	    ptmpstyle->flag_mask.has_handle_width = 1;
	    ptmpstyle->change_mask.has_handle_width = 1;
	  }
        }
        break;

      case 'i':
	if(StrEquals(token, "IconOverride"))
	{
	  found = True;
	  ptmpstyle->flags.icon_override = ICON_OVERRIDE;
	  ptmpstyle->flag_mask.icon_override = ICON_OVERRIDE_MASK;
	  ptmpstyle->change_mask.icon_override = ICON_OVERRIDE_MASK;
	}
        else if(StrEquals(token, "IgnoreRestack"))
        {
	  found = True;
	  ptmpstyle->flags.common.s.do_ignore_restack = 1;
	  ptmpstyle->flag_mask.common.s.do_ignore_restack = 1;
	  ptmpstyle->change_mask.common.s.do_ignore_restack = 1;
        }
        else if(StrEquals(token, "IconTitle"))
        {
	  found = True;
          ptmpstyle->flags.common.s.has_no_icon_title = 0;
          ptmpstyle->flag_mask.common.s.has_no_icon_title = 1;
          ptmpstyle->change_mask.common.s.has_no_icon_title = 1;
        }
        else if(StrEquals(token, "IconBox"))
        {
          icon_boxes *IconBoxes = NULL;

	  found = True;
          IconBoxes = (icon_boxes *)safemalloc(sizeof(icon_boxes));
	  /* clear it */
          memset(IconBoxes, 0, sizeof(icon_boxes));
	  /* init grid x */
          IconBoxes->IconGrid[0] = 3;
	  /* init grid y */
          IconBoxes->IconGrid[1] = 3;

          /* try for 4 numbers x y x y */
	  num = GetIntegerArguments(rest, NULL, val, 4);

	  /* if 4 numbers */
          if (num == 4) {
            for(i = 0; i < 4; i++) {
	      /* make sure the value fits into a short */
	      if (val[i] < -32768)
		val[i] = -32768;
	      if (val[i] > 32767)
		val[i] = 32767;
	      IconBoxes->IconBox[i] = val[i];
	      /* If leading minus sign */
	      token = PeekToken(rest, &rest);
              if (token[0] == '-') {
		/* if a width */
                if (i == 0 || i == 2) {
                  IconBoxes->IconBox[i] += Scr.MyDisplayWidth;
                } else {
                  /* it must be a height */
                  IconBoxes->IconBox[i] += Scr.MyDisplayHeight;
                } /* end width/height */
              } /* end leading minus sign */
            }
            /* Note: here there is no test for valid co-ords, use geom */
          } else {
	    /* Not 4 numeric args dje */
	    /* bigger than =32767x32767+32767+32767 */
            int geom_flags;
	    int l;
	    unsigned int width;
	    unsigned int height;
	    /* read in 1 word w/o advancing */
	    token = PeekToken(rest, NULL);
	    if (!token)
	      break;
	    l = strlen(token);
	    if (l > 0 && l < 24) {
	      /* if word found, not too long */
              geom_flags=XParseGeometry(token,
                                        &IconBoxes->IconBox[0],
					/* x/y */
                                        &IconBoxes->IconBox[1],
					/* width/ht */
                                        &width, &height);
              if (width == 0) {
		/* zero width is invalid */
                fvwm_msg(ERR,"ProcessNewStyle",
                "IconBox requires 4 numbers or geometry! Invalid string <%s>.",
                         token);
		/* Drop the box */
                free(IconBoxes);
		/* forget about it */
                IconBoxes = 0;
              } else {
		/* got valid iconbox geom */
                if (geom_flags & XNegative) {
                  IconBoxes->IconBox[0] =
		    /* screen width */
		    Scr.MyDisplayWidth
		    /* neg x coord */
                    + IconBoxes->IconBox[0]
                    - width -2;
                }
                if (geom_flags & YNegative) {
                  IconBoxes->IconBox[1] =
		    /* scr height */
		    Scr.MyDisplayHeight
		    /* neg y coord */
                    + IconBoxes->IconBox[1]
                    - height -2;
                }
		/* x + wid = right x */
                IconBoxes->IconBox[2] = width + IconBoxes->IconBox[0];
		/* y + height = bottom y */
                IconBoxes->IconBox[3] = height + IconBoxes->IconBox[1];
              } /* end icon geom worked */
            } else {
	      /* no word or too long; drop the box */
              free(IconBoxes);
	      /* forget about it */
              IconBoxes = 0;
            } /* end word found, not too long */
          } /* end not 4 args */
          /* If we created an IconBox, put it in the chain. */
          if (IconBoxes != 0) {
	    /* no error */
            if (ptmpstyle->IconBoxes == 0) {
	      /* first one, chain to root */
	      ptmpstyle->IconBoxes = IconBoxes;
            } else {
	      /* else not first one, add to end of chain */
              which->next = IconBoxes;
            } /* end not first one */
	    /* new current box. save for grid */
            which = IconBoxes;
          } /* end no error */
	  ptmpstyle->flags.has_icon_boxes = !!(ptmpstyle->IconBoxes);
	  ptmpstyle->flag_mask.has_icon_boxes = 1;
	  ptmpstyle->change_mask.has_icon_boxes = 1;
        } /* end iconbox parameter */
        else if(StrEquals(token, "ICONGRID")) {
	  found = True;
          /* The grid always affects the prior iconbox */
          if (which == 0) {
	    /* If no current box */
            fvwm_msg(ERR,"ProcessNewStyle",
                     "IconGrid must follow an IconBox in same Style command");
          } else {
	    /* have a place to grid */
	    /* 2 shorts */

	    num = GetIntegerArguments(rest, NULL, val, 2);
            if (num != 2 || val[0] < 1 || val[1] < 1) {
              fvwm_msg(ERR,"ProcessNewStyle",
                "IconGrid needs 2 numbers > 0. Got %d numbers. x=%d y=%d!",
                       num, val[0], val[1]);
	      /* reset grid */
              which->IconGrid[0] = 3;
              which->IconGrid[1] = 3;
            } else {
	      for (i = 0; i < 2; i++)
	      {
		/* make sure the value fits into a short */
		if (val[i] > 32767)
		  val[i] = 32767;
		which->IconGrid[i] = val[i];
	      }
            } /* end bad grid */
          } /* end place to grid */
        } else if(StrEquals(token, "ICONFILL")) {
	  found = True;
	  /* direction to fill iconbox */
          /* The fill always affects the prior iconbox */
          if (which == 0) {
            /* If no current box */
            fvwm_msg(ERR,"ProcessNewStyle",
                     "IconFill must follow an IconBox in same Style command");
          } else {
	    /* have a place to fill */
	    /* first  type direction parsed */
            unsigned char IconFill_1;
	    /* second type direction parsed */
            unsigned char IconFill_2;
	    token = PeekToken(rest, &rest);
	    /* top/bot/lft/rgt */
            if (Get_TBLR(token, &IconFill_1) == 0) {
	      /* its wrong */
              fvwm_msg(ERR,"ProcessNewStyle",
		       "IconFill must be followed by T|B|R|L, found %s.",
                       token);
            } else {
	      /* first word valid */
	      /* read in second word */
	      token = PeekToken(rest, &rest);
	      /* top/bot/lft/rgt */
              if (Get_TBLR(token, &IconFill_2) == 0) {
		/* its wrong */
                fvwm_msg(ERR,"ProcessNewStyle",
                         "IconFill must be followed by T|B|R|L, found %s.",
                         token);
              } else if ((IconFill_1&ICONFILLHRZ)==(IconFill_2&ICONFILLHRZ)) {
                fvwm_msg(ERR,"ProcessNewStyle",
                 "IconFill must specify a horizontal and vertical direction.");
              } else {
		/* Its valid! */
		/* merge in flags */
                which->IconFlags |= IconFill_1;
		/* ignore horiz in 2nd arg */
                IconFill_2 &= ~ICONFILLHRZ;
		/* merge in flags */
                which->IconFlags |= IconFill_2;
              } /* end second word valid */
            } /* end first word valid */
          } /* end have a place to fill */
        } /* end iconfill */
        else if(StrEquals(token, "ICON"))
        {
	  found = True;
	  GetNextToken(rest, &token);

          ptmpstyle->icon_name = token;
          ptmpstyle->flags.has_icon = (token != NULL);
          ptmpstyle->flag_mask.has_icon = 1;
          ptmpstyle->change_mask.has_icon = 1;

	  ptmpstyle->flags.common.s.is_icon_suppressed = 0;
	  ptmpstyle->flag_mask.common.s.is_icon_suppressed = 1;
	  ptmpstyle->change_mask.common.s.is_icon_suppressed = 1;
        }
        break;

      case 'j':
        break;

      case 'k':
        break;

      case 'l':
        if(StrEquals(token, "LENIENCE"))
        {
	  found = True;
          ptmpstyle->flags.common.s.is_lenient = 1;
          ptmpstyle->flag_mask.common.s.is_lenient = 1;
          ptmpstyle->change_mask.common.s.is_lenient = 1;
        }
        else if (StrEquals(token, "LAYER"))
        {
	  found = True;
	  GetIntegerArguments(rest, NULL, &ptmpstyle->layer, 1);
	  if(ptmpstyle->layer < 0)
	  {
	    fvwm_msg(ERR, "ProcessNewStyle",
		     "Layer must be positive or zero." );
	    /* mark layer unset */
	    ptmpstyle->flags.use_layer = 0;
	    ptmpstyle->flag_mask.use_layer = 1;
	    ptmpstyle->change_mask.use_layer = 1;
	  }
	  else
	  {
	    ptmpstyle->flags.use_layer = 1;
	    ptmpstyle->flag_mask.use_layer = 1;
	    ptmpstyle->change_mask.use_layer = 1;
	  }
	}
        else if(StrEquals(token, "LOWERTRANSIENT"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_lower_transient = 1;
          ptmpstyle->flag_mask.common.s.do_lower_transient = 1;
          ptmpstyle->change_mask.common.s.do_lower_transient = 1;
        }
        break;

      case 'm':
        if(StrEquals(token, "MWMBUTTONS"))
        {
	  found = True;
          ptmpstyle->flags.common.s.has_mwm_buttons = 1;
          ptmpstyle->flag_mask.common.s.has_mwm_buttons = 1;
          ptmpstyle->change_mask.common.s.has_mwm_buttons = 1;
        }
#ifdef MINI_ICONS
        else if (StrEquals(token, "MINIICON"))
        {
	  found = True;
	  GetNextToken(rest, &token);
	  if (token)
          {
            ptmpstyle->mini_icon_name = token;
            ptmpstyle->flags.has_mini_icon = 1;
            ptmpstyle->flag_mask.has_mini_icon = 1;
            ptmpstyle->change_mask.has_mini_icon = 1;
          }
        }
#endif
        else if(StrEquals(token, "MWMBORDER"))
        {
	  found = True;
          ptmpstyle->flags.common.s.has_mwm_border = 1;
          ptmpstyle->flag_mask.common.s.has_mwm_border = 1;
          ptmpstyle->change_mask.common.s.has_mwm_border = 1;
        }
        else if(StrEquals(token, "MWMDECOR"))
        {
	  found = True;
          ptmpstyle->flags.has_mwm_decor = 1;
          ptmpstyle->flag_mask.has_mwm_decor = 1;
          ptmpstyle->change_mask.has_mwm_decor = 1;
        }
        else if(StrEquals(token, "MWMFUNCTIONS"))
        {
	  found = True;
          ptmpstyle->flags.has_mwm_functions = 1;
          ptmpstyle->flag_mask.has_mwm_functions = 1;
          ptmpstyle->change_mask.has_mwm_functions = 1;
        }
        else if(StrEquals(token, "MOUSEFOCUS"))
        {
	  found = True;
          ptmpstyle->flags.common.s.focus_mode = FOCUS_MOUSE;
          ptmpstyle->flag_mask.common.s.focus_mode = FOCUS_MASK;
          ptmpstyle->change_mask.common.s.focus_mode = FOCUS_MASK;
          ptmpstyle->flags.common.s.do_grab_focus_when_created = 0;
          ptmpstyle->flag_mask.common.s.do_grab_focus_when_created = 1;
          ptmpstyle->change_mask.common.s.do_grab_focus_when_created = 1;
        }
        else if(StrEquals(token, "MAXWINDOWSIZE"))
	{
	  int val1;
	  int val2;
	  int val1_unit;
	  int val2_unit;

	  found = True;
	  num = GetTwoArguments(rest, &val1, &val2, &val1_unit, &val2_unit);
	  if (num != 2)
	  {
	    val1 = DEFAULT_MAX_MAX_WINDOW_WIDTH;
	    val2 = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
	  }
	  else
	  {
	    val1 = val1 * val1_unit / 100;
	    val2 = val1 * val1_unit / 100;
	  }
	  if (val1 < DEFAULT_MIN_MAX_WINDOW_WIDTH ||
	      val1 > DEFAULT_MAX_MAX_WINDOW_WIDTH)
	  {
	    val1 = DEFAULT_MAX_MAX_WINDOW_WIDTH;
	  }
	  if (val2 < DEFAULT_MIN_MAX_WINDOW_HEIGHT ||
	      val2 > DEFAULT_MAX_MAX_WINDOW_HEIGHT)
	  {
	    val2 = DEFAULT_MAX_MAX_WINDOW_HEIGHT;
	  }
	  ptmpstyle->max_window_width = val1;
	  ptmpstyle->max_window_height = val2;
	  ptmpstyle->flags.has_max_window_size = 1;
	  ptmpstyle->flag_mask.has_max_window_size = 1;
	  ptmpstyle->change_mask.has_max_window_size = 1;
	}
        break;

      case 'n':
	if(StrEquals(token, "NoActiveIconOverride"))
	{
	  found = True;
	  ptmpstyle->flags.icon_override = NO_ACTIVE_ICON_OVERRIDE;
	  ptmpstyle->flag_mask.icon_override = ICON_OVERRIDE_MASK;
	  ptmpstyle->change_mask.icon_override = ICON_OVERRIDE_MASK;
	}
	else if(StrEquals(token, "NoIconOverride"))
	{
	  found = True;
	  ptmpstyle->flags.icon_override = NO_ICON_OVERRIDE;
	  ptmpstyle->flag_mask.icon_override = ICON_OVERRIDE_MASK;
	  ptmpstyle->change_mask.icon_override = ICON_OVERRIDE_MASK;
	}
        else if(StrEquals(token, "NoIconTitle"))
        {
	  found = True;
          ptmpstyle->flags.common.s.has_no_icon_title = 1;
          ptmpstyle->flag_mask.common.s.has_no_icon_title = 1;
          ptmpstyle->change_mask.common.s.has_no_icon_title = 1;
        }
        else if(StrEquals(token, "NOICON"))
        {
	  found = True;
          ptmpstyle->flags.common.s.is_icon_suppressed = 1;
          ptmpstyle->flag_mask.common.s.is_icon_suppressed = 1;
          ptmpstyle->change_mask.common.s.is_icon_suppressed = 1;
        }
        else if(StrEquals(token, "NOTITLE"))
        {
	  found = True;
          ptmpstyle->flags.has_no_title = 1;
          ptmpstyle->flag_mask.has_no_title = 1;
          ptmpstyle->change_mask.has_no_title = 1;
        }
        else if(StrEquals(token, "NoPPosition"))
        {
	  found = True;
          ptmpstyle->flags.use_no_pposition = 1;
          ptmpstyle->flag_mask.use_no_pposition = 1;
          ptmpstyle->change_mask.use_no_pposition = 1;
        }
        else if(StrEquals(token, "NakedTransient"))
        {
	  found = True;
          ptmpstyle->flags.do_decorate_transient = 0;
          ptmpstyle->flag_mask.do_decorate_transient = 1;
          ptmpstyle->change_mask.do_decorate_transient = 1;
        }
        else if(StrEquals(token, "NODECORHINT"))
        {
	  found = True;
          ptmpstyle->flags.has_mwm_decor = 0;
          ptmpstyle->flag_mask.has_mwm_decor = 1;
          ptmpstyle->change_mask.has_mwm_decor = 1;
        }
        else if(StrEquals(token, "NOFUNCHINT"))
        {
	  found = True;
          ptmpstyle->flags.has_mwm_functions = 0;
          ptmpstyle->flag_mask.has_mwm_functions = 1;
          ptmpstyle->change_mask.has_mwm_functions = 1;
        }
        else if(StrEquals(token, "NOOVERRIDE"))
        {
	  found = True;
          ptmpstyle->flags.common.s.has_mwm_override = 0;
          ptmpstyle->flag_mask.common.s.has_mwm_override = 1;
          ptmpstyle->change_mask.common.s.has_mwm_override = 1;
        }
        else if(StrEquals(token, "NORESIZEOVERRIDE"))
        {
	  found = True;
          ptmpstyle->flags.common.s.has_override_size = 0;
          ptmpstyle->flag_mask.common.s.has_override_size = 1;
          ptmpstyle->change_mask.common.s.has_override_size = 1;
        }
        else if(StrEquals(token, "NOHANDLES"))
        {
	  found = True;
          ptmpstyle->flags.has_no_border = 1;
          ptmpstyle->flag_mask.has_no_border = 1;
          ptmpstyle->change_mask.has_no_border = 1;
        }
        else if(StrEquals(token, "NOLENIENCE"))
        {
	  found = True;
          ptmpstyle->flags.common.s.is_lenient = 0;
          ptmpstyle->flag_mask.common.s.is_lenient = 1;
          ptmpstyle->change_mask.common.s.is_lenient = 1;
        }
        else if (StrEquals(token, "NOBUTTON"))
        {
	  found = True;
          butt = -1;
	  GetIntegerArguments(rest, NULL, &butt, 1);
          if (butt == 0)
	    butt = 10;
          if (butt > 0 && butt <= 10)
	  {
            ptmpstyle->flags.is_button_disabled |= (1<<(butt-1));
            ptmpstyle->flag_mask.is_button_disabled |= (1<<(butt-1));
            ptmpstyle->change_mask.is_button_disabled |= (1<<(butt-1));
	  }
        }
        else if(StrEquals(token, "NOOLDECOR"))
        {
	  found = True;
          ptmpstyle->flags.has_ol_decor = 0;
          ptmpstyle->flag_mask.has_ol_decor = 1;
          ptmpstyle->change_mask.has_ol_decor = 1;
        }
        else if(StrEquals(token, "NEVERFOCUS"))
        {
	  found = True;
          ptmpstyle->flags.common.s.focus_mode = FOCUS_NEVER;
          ptmpstyle->flag_mask.common.s.focus_mode = FOCUS_MASK;
          ptmpstyle->change_mask.common.s.focus_mode = FOCUS_MASK;
          ptmpstyle->flags.common.s.do_grab_focus_when_created = 0;
          ptmpstyle->flag_mask.common.s.do_grab_focus_when_created = 1;
          ptmpstyle->change_mask.common.s.do_grab_focus_when_created = 1;
        }
        break;

      case 'o':
        if(StrEquals(token, "OLDECOR"))
        {
	  found = True;
          ptmpstyle->flags.has_ol_decor = 1;
          ptmpstyle->flag_mask.has_ol_decor = 1;
          ptmpstyle->change_mask.has_ol_decor = 1;
        }
        break;

      case 'p':
        break;

      case 'q':
        break;

      case 'r':
        if(StrEquals(token, "RAISETRANSIENT"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_raise_transient = 1;
          ptmpstyle->flag_mask.common.s.do_raise_transient = 1;
          ptmpstyle->change_mask.common.s.do_raise_transient = 1;
        }
        else if(StrEquals(token, "RANDOMPLACEMENT"))
        {
	  found = True;
          ptmpstyle->flags.do_place_random = 1;
          ptmpstyle->flag_mask.do_place_random = 1;
          ptmpstyle->change_mask.do_place_random = 1;
        }
	else if(StrEquals(token, "RECAPTUREHONORSSTARTSONPAGE"))
	{
	  found = True;
	  ptmpstyle->flags.recapture_honors_starts_on_page = 1;
	  ptmpstyle->flag_mask.recapture_honors_starts_on_page = 1;
	  ptmpstyle->change_mask.recapture_honors_starts_on_page = 1;
	}
	else if(StrEquals(token, "RECAPTUREIGNORESSTARTSONPAGE"))
	{
	  found = True;
	  ptmpstyle->flags.recapture_honors_starts_on_page = 0;
	  ptmpstyle->flag_mask.recapture_honors_starts_on_page = 1;
	  ptmpstyle->change_mask.recapture_honors_starts_on_page = 1;
	}
        else if(StrEquals(token, "RESIZEHINTOVERRIDE"))
        {
	  found = True;
          ptmpstyle->flags.common.s.has_override_size = 1;
          ptmpstyle->flag_mask.common.s.has_override_size = 1;
          ptmpstyle->change_mask.common.s.has_override_size = 1;
        }
        else if(StrEquals(token, "ResizeOpaque"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_resize_opaque = 1;
          ptmpstyle->flag_mask.common.s.do_resize_opaque = 1;
          ptmpstyle->change_mask.common.s.do_resize_opaque = 1;
        }
        else if(StrEquals(token, "ResizeOutline"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_resize_opaque = 0;
          ptmpstyle->flag_mask.common.s.do_resize_opaque = 1;
          ptmpstyle->change_mask.common.s.do_resize_opaque = 1;
        }
        break;

      case 's':
        if(StrEquals(token, "SMARTPLACEMENT"))
        {
	  found = True;
          ptmpstyle->flags.do_place_smart = 1;
          ptmpstyle->flag_mask.do_place_smart = 1;
          ptmpstyle->change_mask.do_place_smart = 1;
        }
        else if(StrEquals(token, "SkipMapping"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_not_show_on_map = 1;
          ptmpstyle->flag_mask.common.s.do_not_show_on_map = 1;
          ptmpstyle->change_mask.common.s.do_not_show_on_map = 1;
        }
        else if(StrEquals(token, "ShowMapping"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_not_show_on_map = 0;
          ptmpstyle->flag_mask.common.s.do_not_show_on_map = 1;
          ptmpstyle->change_mask.common.s.do_not_show_on_map = 1;
        }
        else if(StrEquals(token, "StackTransientParent"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_stack_transient_parent = 1;
          ptmpstyle->flag_mask.common.s.do_stack_transient_parent = 1;
          ptmpstyle->change_mask.common.s.do_stack_transient_parent = 1;
        }
        else if(StrEquals(token, "StickyIcon"))
        {
	  found = True;
          ptmpstyle->flags.common.s.is_icon_sticky = 1;
          ptmpstyle->flag_mask.common.s.is_icon_sticky = 1;
          ptmpstyle->change_mask.common.s.is_icon_sticky = 1;
        }
        else if(StrEquals(token, "SlipperyIcon"))
        {
	  found = True;
          ptmpstyle->flags.common.s.is_icon_sticky = 0;
          ptmpstyle->flag_mask.common.s.is_icon_sticky = 1;
          ptmpstyle->change_mask.common.s.is_icon_sticky = 1;
        }
        else if(StrEquals(token, "SLOPPYFOCUS"))
        {
	  found = True;
          ptmpstyle->flags.common.s.focus_mode = FOCUS_SLOPPY;
          ptmpstyle->flag_mask.common.s.focus_mode = FOCUS_MASK;
          ptmpstyle->change_mask.common.s.focus_mode = FOCUS_MASK;
          ptmpstyle->flags.common.s.do_grab_focus_when_created = 0;
          ptmpstyle->flag_mask.common.s.do_grab_focus_when_created = 1;
          ptmpstyle->change_mask.common.s.do_grab_focus_when_created = 1;
        }
        else if(StrEquals(token, "StartIconic"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_start_iconic = 1;
          ptmpstyle->flag_mask.common.s.do_start_iconic = 1;
          ptmpstyle->change_mask.common.s.do_start_iconic = 1;
        }
        else if(StrEquals(token, "StartNormal"))
        {
	  found = True;
          ptmpstyle->flags.common.s.do_start_iconic = 0;
          ptmpstyle->flag_mask.common.s.do_start_iconic = 1;
          ptmpstyle->change_mask.common.s.do_start_iconic = 1;
        }
        else if(StrEquals(token, "StaysOnBottom"))
        {
	  found = True;
          ptmpstyle->layer = Scr.BottomLayer;
	  ptmpstyle->flags.use_layer = 1;
	  ptmpstyle->flag_mask.use_layer = 1;
	  ptmpstyle->change_mask.use_layer = 1;
        }
        else if(StrEquals(token, "StaysOnTop"))
        {
	  found = True;
          ptmpstyle->layer = Scr.TopLayer;
	  ptmpstyle->flags.use_layer = 1;
	  ptmpstyle->flag_mask.use_layer = 1;
	  ptmpstyle->change_mask.use_layer = 1;
        }
        else if(StrEquals(token, "StaysPut"))
        {
	  found = True;
          ptmpstyle->layer = Scr.DefaultLayer;
	  ptmpstyle->flags.use_layer = 1;
	  ptmpstyle->flag_mask.use_layer = 1;
	  ptmpstyle->change_mask.use_layer = 1;
        }
        else if(StrEquals(token, "Sticky"))
        {
	  found = True;
          ptmpstyle->flags.common.is_sticky = 1;
          ptmpstyle->flag_mask.common.is_sticky = 1;
          ptmpstyle->change_mask.common.is_sticky = 1;
        }
        else if(StrEquals(token, "Slippery"))
        {
	  found = True;
          ptmpstyle->flags.common.is_sticky = 0;
          ptmpstyle->flag_mask.common.is_sticky = 1;
          ptmpstyle->change_mask.common.is_sticky = 1;
        }
        else if(StrEquals(token, "STARTSONDESK"))
        {
	  found = True;
	  /*  RBW - 11/02/1998  */
	  spargs = GetIntegerArguments(rest, NULL, tmpno, 1);
          if (spargs == 1)
	  {
	    ptmpstyle->flags.use_start_on_desk = 1;
	    ptmpstyle->flag_mask.use_start_on_desk = 1;
	    ptmpstyle->change_mask.use_start_on_desk = 1;
	    /*  RBW - 11/20/1998 - allow for the special case of -1  */
	    ptmpstyle->start_desk = (tmpno[0] > -1) ? tmpno[0] + 1 : tmpno[0];
	  }
          else
	  {
	    fvwm_msg(ERR,"ProcessNewStyle",
		     "bad StartsOnDesk arg: %s", rest);
	  }
          /**/
        }
	/*  RBW - 11/02/1998
	 *  StartsOnPage is like StartsOnDesk-Plus
	 */
	else if(StrEquals(token, "STARTSONPAGE"))
	{
	  found = True;
	  spargs = GetIntegerArguments(rest, NULL, tmpno, 3);
	  if (spargs == 1 || spargs == 3)
	  {
	    /* We have a desk no., with or without page. */
	    /* RBW - 11/20/1998 - allow for the special case of -1 */
	    /* Desk is now actual + 1 */
	    ptmpstyle->start_desk = (tmpno[0] > -1) ? tmpno[0] + 1 : tmpno[0];
	  }
	  if (spargs == 2 || spargs == 3)
	  {
	    if (spargs == 3)
	    {
	      /*  RBW - 11/20/1998 - allow for the special case of -1  */
	      ptmpstyle->start_page_x =
		(tmpno[1] > -1) ? tmpno[1] + 1 : tmpno[1];
	      ptmpstyle->start_page_y =
		(tmpno[2] > -1) ? tmpno[2] + 1 : tmpno[2];
	    }
	    else
	    {
	      ptmpstyle->start_page_x =
		(tmpno[0] > -1) ? tmpno[0] + 1 : tmpno[0];
	      ptmpstyle->start_page_y =
		(tmpno[1] > -1) ? tmpno[1] + 1 : tmpno[1];
	    }
	  }
	  if (spargs < 1 || spargs > 3)
	  {
	    fvwm_msg(ERR,"ProcessNewStyle",
		     "bad StartsOnPage args: %s", rest);
	  }
	  else
	  {
	    ptmpstyle->flags.use_start_on_desk = 1;
	    ptmpstyle->flag_mask.use_start_on_desk = 1;
	    ptmpstyle->change_mask.use_start_on_desk = 1;
	  }
	}
	else if(StrEquals(token, "STARTSONPAGEINCLUDESTRANSIENTS"))
	{
	  found = True;
	  ptmpstyle->flags.use_start_on_page_for_transient = 1;
	  ptmpstyle->flag_mask.use_start_on_page_for_transient = 1;
	  ptmpstyle->change_mask.use_start_on_page_for_transient = 1;
	}
	else if(StrEquals(token, "STARTSONPAGEIGNORESTRANSIENTS"))
	{
	  found = True;
	  ptmpstyle->flags.use_start_on_page_for_transient = 0;
	  ptmpstyle->flag_mask.use_start_on_page_for_transient = 1;
	  ptmpstyle->change_mask.use_start_on_page_for_transient = 1;
	}
	/**/
	else if(StrEquals(token, "STARTSANYWHERE"))
	{
	  found = True;
	  ptmpstyle->flags.use_start_on_desk = 0;
	  ptmpstyle->flag_mask.use_start_on_desk = 1;
	  ptmpstyle->change_mask.use_start_on_desk = 1;
	}
        else if (StrEquals(token, "STARTSLOWERED"))
        {
	  found = True;
          ptmpstyle->flags.do_start_lowered = 1;
          ptmpstyle->flag_mask.do_start_lowered = 1;
          ptmpstyle->change_mask.do_start_lowered = 1;
        }
        else if (StrEquals(token, "STARTSRAISED"))
        {
	  found = True;
          ptmpstyle->flags.do_start_lowered = 0;
          ptmpstyle->flag_mask.do_start_lowered = 1;
          ptmpstyle->change_mask.do_start_lowered = 1;
        }
        break;

      case 't':
        if(StrEquals(token, "TITLE"))
        {
	  found = True;
          ptmpstyle->flags.has_no_title = 0;
          ptmpstyle->flag_mask.has_no_title = 1;
          ptmpstyle->change_mask.has_no_title = 1;
        }
        break;

      case 'u':
        if(StrEquals(token, "UsePPosition"))
        {
	  found = True;
          ptmpstyle->flags.use_no_pposition = 0;
          ptmpstyle->flag_mask.use_no_pposition = 1;
          ptmpstyle->change_mask.use_no_pposition = 1;
        }
#ifdef USEDECOR
        if(StrEquals(token, "UseDecor"))
        {
	  found = True;
	  SAFEFREE(ptmpstyle->decor_name);
	  GetNextToken(rest, &token);
	  ptmpstyle->decor_name = token;
          ptmpstyle->flags.has_decor = (token != NULL);
          ptmpstyle->flag_mask.has_decor = (token != NULL);
          ptmpstyle->change_mask.has_decor = (token != NULL);
        }
#endif
        else if(StrEquals(token, "UseStyle"))
        {
	  found = True;
	  token = PeekToken(rest, &rest);
          if (token) {
            int hit = 0;
            /* changed to accum multiple Style definitions (veliaa@rpi.edu) */
            for (add_style = all_styles; add_style;
		 add_style = add_style->next ) {
              if (StrEquals(token, add_style->name)) {
		/* match style */
                if (!hit)
                {
		  /* first match */
                  char *save_name;
                  save_name = ptmpstyle->name;
		  /* copy everything */
                  memcpy((void*)ptmpstyle, (const void*)add_style,
			 sizeof(window_style));
		  /* except the next pointer */
                  ptmpstyle->next = NULL;
		  /* and the name */
                  ptmpstyle->name = save_name;
		  /* set not first match */
                  hit = 1;
                }
                else
                {
		  merge_styles(ptmpstyle, add_style, True);
                } /* end hit/not hit */
              } /* end found matching style */
            } /* end looking at all styles */

	    /* move forward one word */
            if (!hit) {
              fvwm_msg(ERR,"ProcessNewStyle", "UseStyle: %s style not found",
		       token);
            }
          } /* if (len > 0) */
        }
        break;

      case 'v':
        if(StrEquals(token, "VariablePosition"))
	  {
	    found = True;
            ptmpstyle->flags.common.s.is_fixed = 0;
            ptmpstyle->flag_mask.common.s.is_fixed = 1;
            ptmpstyle->change_mask.common.s.is_fixed = 1;
	  }
        break;

      case 'w':
        if(StrEquals(token, "WindowListSkip"))
        {
	  found = True;
	  ptmpstyle->flags.common.s.do_window_list_skip = 1;
	  ptmpstyle->flag_mask.common.s.do_window_list_skip = 1;
	  ptmpstyle->change_mask.common.s.do_window_list_skip = 1;
        }
        else if(StrEquals(token, "WindowListHit"))
        {
	  found = True;
	  ptmpstyle->flags.common.s.do_window_list_skip = 0;
	  ptmpstyle->flag_mask.common.s.do_window_list_skip = 1;
	  ptmpstyle->change_mask.common.s.do_window_list_skip = 1;
        }
        break;

      case 'x':
        break;

      case 'y':
        break;

      case 'z':
        break;

      default:
        break;
    }

    if (found == False)
    {
      fvwm_msg(ERR,"ProcessNewStyle", "bad style command: %s", option);
      /* Can't return here since all malloced memory will be lost. Ignore rest
       * of line instead. */

      /* No, I think we /can/ return here.
       * In fact, /not/ bombing out leaves a half-done style in the list!
       * N.Bird 07-Sep-1999 */

      /* domivogt (01-Oct-1999): Which is exactly what we want! Why should all
       * the styles be thrown away if a single one is mis-spelled? Let's just
       * continue parsing styles. */
    }
    free(option);
  } /* end while still stuff on command */

  /* capture default icons */
  if(StrEquals(ptmpstyle->name, "*"))
  {
    if(ptmpstyle->flags.has_icon == 1)
      Scr.DefaultIcon = ptmpstyle->icon_name;
    ptmpstyle->flags.has_icon = 0;
    ptmpstyle->flag_mask.has_icon = 0;
    ptmpstyle->change_mask.has_icon = 0;
    ptmpstyle->icon_name = NULL;
  }
  /* add temp name list to list */
  add_style_to_list(ptmpstyle);
}


static void handle_window_style_change(FvwmWindow *t)
{
  window_style style;
  int i;
  char *wf;
  char *sf;
  char *sc;
  Bool do_redraw_window = False;

  lookup_style(t, &style);
  if (style.has_style_changed == 0)
  {
    /* nothing to do */
    return;
  }

  /*!!! still to be written */
  /* now we have to do the real work */

  wf = (char *)(&FW_COMMON_STATIC_FLAGS(t));
  sf = (char *)(&style.flags.common.s);
  sc = (char *)(&style.change_mask.common.s);
  /* copy the static common window flags */
  for (i = 0; i < sizeof(style.flags.common.s); i++)
  {
    wf[i] = (wf[i] & ~sc[i]) | (sf[i] & sc[i]);
  }

  if (style.change_mask.common.is_sticky)
  {
    handle_stick(&Event, t->frame, t, C_FRAME, "", 0,
                 style.flags.common.is_sticky);
  }
  if (style.change_mask.common.s.is_icon_sticky)
  {
    if (IS_ICONIFIED(t) && !IS_STICKY(t))
    {
      /* stick and unstick the window to force the icon on the current page */
      handle_stick(&Event, t->frame, t, C_FRAME, "", 0, 1);
      handle_stick(&Event, t->frame, t, C_FRAME, "", 0, 0);
    }
  }

  if (style.change_mask.has_color_fore || style.change_mask.has_color_back)
  {
    do_redraw_window = True;
    update_window_color_style(t, &style);
  }

  if (do_redraw_window)
  {
    FvwmWindow *u = Scr.Hilite;

    SetBorder(t, (Scr.Hilite == t), 2, True, None);
    Scr.Hilite = u;
  }
fprintf(stderr,"updated style for window %s\n", t->name);
  return;
}

void reset_style_changes(void)
{
  window_style *temp;

  for (temp = all_styles; temp != NULL; temp = temp->next)
  {
    temp->has_style_changed = 0;
  }
  Scr.flags.has_any_style_changed = 0;

  return;
}

void handle_style_changes(void)
{
  FvwmWindow *t;

fprintf(stderr,"updating styles\n");
  /* update styles for all windows */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    handle_window_style_change(t);
  }
  /* finally clean up the change flags */
  reset_style_changes();

  return;
}

void update_style_colorset(int colorset)
{
  window_style *temp;

  for (temp = all_styles; temp != NULL; temp = temp->next)
  {
    if (SUSE_COLORSET(&temp->flags) && SGET_COLORSET(*temp) == colorset)
    {
      temp->has_style_changed = 1;
      temp->change_mask.use_colorset = 1;
      Scr.flags.has_any_style_changed = 1;
    }
  }
}

void update_window_color_style(FvwmWindow *tmp_win, window_style *pstyle)
{
  int cs = Scr.DefaultColorset;

  if (SUSE_COLORSET(&pstyle->flags))
  {
    cs = SGET_COLORSET(*pstyle);
  }

  if(SGET_FORE_COLOR_NAME(*pstyle) != NULL && !SUSE_COLORSET(&pstyle->flags))
  {
    tmp_win->TextPixel = GetColor(SGET_FORE_COLOR_NAME(*pstyle));
  }
  else
  {
    tmp_win->TextPixel = Colorset[cs % nColorsets].fg;
  }
  if(SGET_BACK_COLOR_NAME(*pstyle) != NULL && !SUSE_COLORSET(&pstyle->flags))
  {
    tmp_win->BackPixel = GetColor(SGET_BACK_COLOR_NAME(*pstyle));
    tmp_win->ShadowPixel = GetShadow(tmp_win->BackPixel);
    tmp_win->ReliefPixel = GetHilite(tmp_win->BackPixel);
  }
  else
  {
    tmp_win->ReliefPixel = Colorset[cs % nColorsets].hilite;
    tmp_win->ShadowPixel = Colorset[cs % nColorsets].shadow;
    tmp_win->BackPixel = Colorset[cs % nColorsets].bg;
  }
}
