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

/* list of window names with attributes */
static window_style *all_styles = NULL;
static window_style *last_style_in_list = NULL;

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
 *      styles - callers return area
 *      nptr - matching window_style
 *
 *  Note:
 *      The only trick here is that on and off flags/buttons are
 *      combined into the on flag/button.
 *
 ***********************************************************************/

void merge_styles(window_style *merged_style, window_style *add_style)
{
  int i;
  char *merge_flags;
  char *add_flags;
  char *merge_mask;
  char *add_mask;

  if(add_style->icon_name != NULL)
    merged_style->icon_name = add_style->icon_name;
#ifdef MINI_ICONS
  if(add_style->mini_icon_name != NULL)
    merged_style->mini_icon_name = add_style->mini_icon_name;
#endif
#ifdef USEDECOR
  if (add_style->decor_name != NULL)
    merged_style->decor_name = add_style->decor_name;
#endif
  if(add_style->flags.use_start_on_desk)
    /*  RBW - 11/02/1998  */
    {
      merged_style->start_desk = add_style->start_desk;
      merged_style->start_page_x = add_style->start_page_x;
      merged_style->start_page_y = add_style->start_page_y;
    }
  if(add_style->flags.has_border_width)
    merged_style->border_width = add_style->border_width;
  if(add_style->flags.has_color_fore)
    merged_style->fore_color_name = add_style->fore_color_name;
  if(add_style->flags.has_color_back)
    merged_style->back_color_name = add_style->back_color_name;
  if(add_style->flags.has_handle_width)
    merged_style->handle_width = add_style->handle_width;
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
  for (i = 0; i < sizeof(style_flags); i++)
    {
      merge_flags[i] |= (add_flags[i] & add_mask[i]);
      merge_flags[i] &= (add_flags[i] | ~add_mask[i]);
      merge_mask[i] |= add_mask[i];
    }
  /* combine buttons */
  merged_style->on_buttons |= add_style->on_buttons;
  merged_style->on_buttons &= ~(add_style->off_buttons);

  /* Note, only one style cmd can define a windows iconboxes,
   * the last one encountered. */
  if(add_style->IconBoxes != NULL) {
    /* If style has iconboxes */
    /* copy it */
    merged_style->IconBoxes = add_style->IconBoxes;
  }
  if (add_style->layer != -9) {
    merged_style->layer = add_style->layer;
  }
  return;
}


static void add_style_to_list(window_style *new_style)
{
  window_style *nptr;

  /* This used to contain logic that returned if the style didn't contain
   * anything.  I don't see why we should bother. dje.
   *
   * used to merge duplicate entries, but that is no longer
   * appropriate since conficting styles are possible, and the
   * last match should win! */

  nptr = (window_style *)safemalloc(sizeof(window_style));
  /* copy term area into list */
  memcpy((void*)nptr, (const void*)new_style, sizeof(window_style));
  if(last_style_in_list != NULL)
    /* not first entry in list chain this entry to the list */
    last_style_in_list->next = nptr;
  else
    /* first entry in list set the list root pointer. */
    all_styles = nptr;
  last_style_in_list = nptr;
} /* end function */


#define SAFEFREE( p )  {if (p)  free(p);}

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

      /* Free contents of style */
      SAFEFREE( tmp_ptr->name );
      SAFEFREE( tmp_ptr->back_color_name );
      SAFEFREE( tmp_ptr->fore_color_name );
      SAFEFREE( tmp_ptr->mini_icon_name );
      SAFEFREE( tmp_ptr->decor_name );
      SAFEFREE( tmp_ptr->icon_name );

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
  /* initialize to default layer */
  styles->layer = Scr.DefaultLayer;
#ifdef GNOME
  /* initialize with GNOME hints */
  GNOME_GetStyle (tmp_win, styles);
#endif
  /* look thru all styles in order defined. */
  for (nptr = all_styles; nptr != NULL; nptr = nptr->next) {
    /* If name/res_class/res_name match, merge */
    if (matchWildcards(nptr->name,tmp_win->class.res_class) == TRUE) {
      merge_styles(styles, nptr);
    } else if (matchWildcards(nptr->name,tmp_win->class.res_name) == TRUE) {
      merge_styles(styles, nptr);
    } else if (matchWildcards(nptr->name,tmp_win->name) == TRUE) {
      merge_styles(styles, nptr);
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
   If valid, added to the list in a malloced area. */
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
  window_style tmpstyle;
  /* which current boxes to chain to */
  icon_boxes *which = 0;

  /* init temp window_style area */
  memset(&tmpstyle, 0, sizeof(window_style));
  /* default uninitialised layer */
  tmpstyle.layer = -9;

  /* parse style name */
  action = GetNextToken(action, &tmpstyle.name);
  /* in case there was no argument! */
  if(tmpstyle.name == NULL)
    return;
  if(action == NULL)
    {
      free(tmpstyle.name);
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
        if(StrEquals(token, "AllowRestack"))
        {
          found = True;
          tmpstyle.flags.common.ignore_restack = 0;
          tmpstyle.flag_mask.common.ignore_restack = 1;
        }
        else if(StrEquals(token, "ACTIVEPLACEMENT"))
        {
	  found = True;
          tmpstyle.flags.do_place_random = 0;
          tmpstyle.flag_mask.do_place_random = 1;
        }
        break;

      case 'b':
        if(StrEquals(token, "BACKCOLOR"))
        {
	  found = True;
	  GetNextToken(rest, &token);
          if (token)
          {
            tmpstyle.back_color_name = token;
            tmpstyle.flags.has_color_back = 1;
            tmpstyle.flag_mask.has_color_back = 1;
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
            tmpstyle.off_buttons |= (1<<(butt-1));
        }
        else if(StrEquals(token, "BorderWidth"))
        {
	  found = True;
	  if (GetIntegerArguments(rest, NULL, &tmpstyle.border_width, 1))
	  {
	    tmpstyle.flags.has_border_width = 1;
	    tmpstyle.flag_mask.has_border_width = 1;
	  }
        }
        break;

      case 'c':
        if(StrEquals(token, "COLOR"))
        {
	  char c = 0;

	  found = True;
	  rest = DoGetNextToken(rest, &token, NULL, ",/", &c);
	  if (token == NULL)
	    break;
	  tmpstyle.fore_color_name = token;
	  tmpstyle.flags.has_color_fore = 1;
	  tmpstyle.flag_mask.has_color_fore = 1;

	  if (c != '/')
	  {
	    /* skip over '/' */
	    while (rest && *rest && isspace((unsigned char)*rest) &&
		   *rest != ',' && *rest != '/')
	      rest++;
	    if (*rest == '/')
	      rest++;
	  }
	  GetNextToken(rest, &token);
	  if (!token)
	    break;
	  tmpstyle.back_color_name = token;
	  tmpstyle.flags.has_color_back = 1;
	  tmpstyle.flag_mask.has_color_back = 1;
	  break;
        }
        else if(StrEquals(token, "CirculateSkipIcon"))
        {
	  found = True;
          tmpstyle.flags.common.do_circulate_skip_icon = 1;
          tmpstyle.flag_mask.common.do_circulate_skip_icon = 1;
        }
        else if(StrEquals(token, "CirculateHitIcon"))
        {
	  found = True;
          tmpstyle.flags.common.do_circulate_skip_icon = 0;
          tmpstyle.flag_mask.common.do_circulate_skip_icon = 1;
        }
        else if(StrEquals(token, "CLICKTOFOCUS"))
        {
	  found = True;
          tmpstyle.flags.common.focus_mode = FOCUS_CLICK;
          tmpstyle.flag_mask.common.focus_mode = FOCUS_MASK;
          tmpstyle.flags.common.do_grab_focus_when_created = 1;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
        }
        else if(StrEquals(token, "CirculateSkip"))
        {
	  found = True;
          tmpstyle.flags.common.do_circulate_skip = 1;
          tmpstyle.flag_mask.common.do_circulate_skip = 1;
        }
        else if(StrEquals(token, "CirculateHit"))
        {
	  found = True;
          tmpstyle.flags.common.do_circulate_skip = 0;
          tmpstyle.flag_mask.common.do_circulate_skip = 1;
        }
        break;

      case 'd':
        if(StrEquals(token, "DepressableBorder"))
	  {
	  found = True;
          tmpstyle.flags.common.has_depressable_border = 1;
          tmpstyle.flag_mask.common.has_depressable_border = 1;
	  }
        else if(StrEquals(token, "DecorateTransient"))
        {
	  found = True;
          tmpstyle.flags.do_decorate_transient = 1;
          tmpstyle.flag_mask.do_decorate_transient = 1;
        }
        else if(StrEquals(token, "DUMBPLACEMENT"))
        {
	  found = True;
          tmpstyle.flags.do_place_smart = 0;
          tmpstyle.flag_mask.do_place_smart = 1;
        }
        else if(StrEquals(token, "DONTRAISETRANSIENT"))
        {
	  found = True;
          tmpstyle.flags.common.do_raise_transient = 0;
          tmpstyle.flag_mask.common.do_raise_transient = 1;
        }
        else if(StrEquals(token, "DONTLOWERTRANSIENT"))
        {
	  found = True;
          tmpstyle.flags.common.do_lower_transient = 0;
          tmpstyle.flag_mask.common.do_lower_transient = 1;
        }
        else if(StrEquals(token, "DontStackTransientParent"))
        {
	  found = True;
          tmpstyle.flags.common.do_stack_transient_parent = 0;
          tmpstyle.flag_mask.common.do_stack_transient_parent = 1;
        }
        break;

      case 'e':
        break;

      case 'f':
        if(StrEquals(token, "FirmBorder"))
	  {
	  found = True;
          tmpstyle.flags.common.has_depressable_border = 0;
          tmpstyle.flag_mask.common.has_depressable_border = 1;
	  }
        else if(StrEquals(token, "FixedPosition"))
	  {
	    found = True;
            tmpstyle.flags.common.is_fixed = 1;
            tmpstyle.flag_mask.common.is_fixed = 1;
	  }
        else if(StrEquals(token, "FORECOLOR"))
        {
	  found = True;
	  GetNextToken(rest, &token);
          if (token)
          {
            tmpstyle.fore_color_name = token;
            tmpstyle.flags.has_color_fore = 1;
            tmpstyle.flag_mask.has_color_fore = 1;
          }
        }
        else if(StrEquals(token, "FVWMBUTTONS"))
        {
	  found = True;
          tmpstyle.flags.common.has_mwm_buttons = 0;
          tmpstyle.flag_mask.common.has_mwm_buttons = 1;
        }
        else if(StrEquals(token, "FVWMBORDER"))
        {
	  found = True;
          tmpstyle.flags.common.has_mwm_border = 0;
          tmpstyle.flag_mask.common.has_mwm_border = 1;
        }
        else if(StrEquals(token, "FocusFollowsMouse"))
        {
	  found = True;
          tmpstyle.flags.common.focus_mode = FOCUS_MOUSE;
          tmpstyle.flag_mask.common.focus_mode = FOCUS_MASK;
          tmpstyle.flags.common.do_grab_focus_when_created = 0;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
        }
        break;

      case 'g':
        if(StrEquals(token, "GrabFocusOff"))
        {
	  found = True;
          tmpstyle.flags.common.do_grab_focus_when_created = 0;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
        }
        else if(StrEquals(token, "GrabFocus"))
        {
	  found = True;
          tmpstyle.flags.common.do_grab_focus_when_created = 1;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
        }
        else if(StrEquals(token, "GrabFocusTransientOff"))
        {
	  found = True;
          tmpstyle.flags.common.do_grab_focus_when_transient_created = 0;
          tmpstyle.flag_mask.common.do_grab_focus_when_transient_created = 1;
        }
        else if(StrEquals(token, "GrabFocusTransient"))
        {
	  found = True;
          tmpstyle.flags.common.do_grab_focus_when_transient_created = 1;
          tmpstyle.flag_mask.common.do_grab_focus_when_transient_created = 1;
        }
        break;

      case 'h':
        if(StrEquals(token, "HINTOVERRIDE"))
        {
	  found = True;
          tmpstyle.flags.common.has_mwm_override = 1;
          tmpstyle.flag_mask.common.has_mwm_override = 1;
        }
        else if(StrEquals(token, "HANDLES"))
        {
	  found = True;
          tmpstyle.flags.has_no_border = 0;
          tmpstyle.flag_mask.has_no_border = 1;
        }
        else if(StrEquals(token, "HandleWidth"))
        {
	  found = True;
	  if (GetIntegerArguments(rest, NULL, &tmpstyle.handle_width, 1))
	  {
	    tmpstyle.flags.has_handle_width = 1;
	    tmpstyle.flag_mask.has_handle_width = 1;
	  }
        }
        break;

      case 'i':
	if(StrEquals(token, "IconOverride"))
	{
	  found = True;
	  tmpstyle.flags.icon_override = ICON_OVERRIDE;
	  tmpstyle.flag_mask.icon_override = ICON_OVERRIDE_MASK;
	}
        else if(StrEquals(token, "IgnoreRestack"))
        {
	  found = True;
	  tmpstyle.flags.common.ignore_restack = 1;
	  tmpstyle.flag_mask.common.ignore_restack = 1;
        }
        else if(StrEquals(token, "IconTitle"))
        {
	  found = True;
          tmpstyle.flags.common.has_no_icon_title = 0;
          tmpstyle.flag_mask.common.has_no_icon_title = 1;
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
	      /* no word or too long */
              fvwm_msg(ERR, "ProcessNewStyle",
                       "IconBox requires 4 numbers or geometry! Too long (%d).",
                       l);
	      /* Drop the box */
              free(IconBoxes);
	      /* forget about it */
              IconBoxes = 0;
            } /* end word found, not too long */
          } /* end not 4 args */
          /* If we created an IconBox, put it in the chain. */
          if (IconBoxes != 0) {
	    /* no error */
            if (tmpstyle.IconBoxes == 0) {
	      /* first one, chain to root */
	      tmpstyle.IconBoxes = IconBoxes;
            } else {
	      /* else not first one, add to end of chain */
              which->next = IconBoxes;
            } /* end not first one */
	    /* new current box. save for grid */
            which = IconBoxes;
          } /* end no error */
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

          tmpstyle.icon_name = token;
          tmpstyle.flags.has_icon = (token != NULL);
          tmpstyle.flag_mask.has_icon = 1;

	  tmpstyle.flags.common.is_icon_suppressed = 0;
	  tmpstyle.flag_mask.common.is_icon_suppressed = 1;
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
          tmpstyle.flags.common.is_lenient = 1;
          tmpstyle.flag_mask.common.is_lenient = 1;
        }
        else if (StrEquals(token, "LAYER"))
        {
	  found = True;
	  GetIntegerArguments(rest, NULL, &tmpstyle.layer, 1);
	  if(tmpstyle.layer < 0)
	  {
	    fvwm_msg(ERR, "ProcessNewStyle",
		     "Layer must be positive or zero." );
	    /* mark layer unset */
	    tmpstyle.layer = -9;
	  }
	}
        else if(StrEquals(token, "LOWERTRANSIENT"))
        {
	  found = True;
          tmpstyle.flags.common.do_lower_transient = 1;
          tmpstyle.flag_mask.common.do_lower_transient = 1;
        }
        break;

      case 'm':
        if(StrEquals(token, "MWMBUTTONS"))
        {
	  found = True;
          tmpstyle.flags.common.has_mwm_buttons = 1;
          tmpstyle.flag_mask.common.has_mwm_buttons = 1;
        }
#ifdef MINI_ICONS
        else if (StrEquals(token, "MINIICON"))
        {
	  found = True;
	  GetNextToken(rest, &token);
	  if (token)
          {
            tmpstyle.mini_icon_name = token;
            tmpstyle.flags.has_mini_icon = 1;
            tmpstyle.flag_mask.has_mini_icon = 1;
          }
        }
#endif
        else if(StrEquals(token, "MWMBORDER"))
        {
	  found = True;
          tmpstyle.flags.common.has_mwm_border = 1;
          tmpstyle.flag_mask.common.has_mwm_border = 1;
        }
        else if(StrEquals(token, "MWMDECOR"))
        {
	  found = True;
          tmpstyle.flags.has_mwm_decor = 1;
          tmpstyle.flag_mask.has_mwm_decor = 1;
        }
        else if(StrEquals(token, "MWMFUNCTIONS"))
        {
	  found = True;
          tmpstyle.flags.has_mwm_functions = 1;
          tmpstyle.flag_mask.has_mwm_functions = 1;
        }
        else if(StrEquals(token, "MOUSEFOCUS"))
        {
	  found = True;
          tmpstyle.flags.common.focus_mode = FOCUS_MOUSE;
          tmpstyle.flag_mask.common.focus_mode = FOCUS_MASK;
          tmpstyle.flags.common.do_grab_focus_when_created = 0;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
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
	  tmpstyle.max_window_width = val1;
	  tmpstyle.max_window_height = val2;
	  tmpstyle.flags.has_max_window_size = 1;
	  tmpstyle.flag_mask.has_max_window_size = 1;
	}
        break;

      case 'n':
	if(StrEquals(token, "NoActiveIconOverride"))
	{
	  found = True;
	  tmpstyle.flags.icon_override = NO_ACTIVE_ICON_OVERRIDE;
	  tmpstyle.flag_mask.icon_override = ICON_OVERRIDE_MASK;
	}
	else if(StrEquals(token, "NoIconOverride"))
	{
	  found = True;
	  tmpstyle.flags.icon_override = NO_ICON_OVERRIDE;
	  tmpstyle.flag_mask.icon_override = ICON_OVERRIDE_MASK;
	}
        else if(StrEquals(token, "NoIconTitle"))
        {
	  found = True;
          tmpstyle.flags.common.has_no_icon_title = 1;
          tmpstyle.flag_mask.common.has_no_icon_title = 1;
        }
        else if(StrEquals(token, "NOICON"))
        {
	  found = True;
          tmpstyle.flags.common.is_icon_suppressed = 1;
          tmpstyle.flag_mask.common.is_icon_suppressed = 1;
        }
        else if(StrEquals(token, "NOTITLE"))
        {
	  found = True;
          tmpstyle.flags.has_no_title = 1;
          tmpstyle.flag_mask.has_no_title = 1;
        }
        else if(StrEquals(token, "NoPPosition"))
        {
	  found = True;
          tmpstyle.flags.use_no_pposition = 1;
          tmpstyle.flag_mask.use_no_pposition = 1;
        }
        else if(StrEquals(token, "NakedTransient"))
        {
	  found = True;
          tmpstyle.flags.do_decorate_transient = 0;
          tmpstyle.flag_mask.do_decorate_transient = 1;
        }
        else if(StrEquals(token, "NODECORHINT"))
        {
	  found = True;
          tmpstyle.flags.has_mwm_decor = 0;
          tmpstyle.flag_mask.has_mwm_decor = 1;
        }
        else if(StrEquals(token, "NOFUNCHINT"))
        {
	  found = True;
          tmpstyle.flags.has_mwm_functions = 0;
          tmpstyle.flag_mask.has_mwm_functions = 1;
        }
        else if(StrEquals(token, "NOOVERRIDE"))
        {
	  found = True;
          tmpstyle.flags.common.has_mwm_override = 0;
          tmpstyle.flag_mask.common.has_mwm_override = 1;
        }
        else if(StrEquals(token, "NORESIZEOVERRIDE"))
        {
	  found = True;
          tmpstyle.flags.common.has_override_size = 0;
          tmpstyle.flag_mask.common.has_override_size = 1;
        }
        else if(StrEquals(token, "NOHANDLES"))
        {
	  found = True;
          tmpstyle.flags.has_no_border = 1;
          tmpstyle.flag_mask.has_no_border = 1;
        }
        else if(StrEquals(token, "NOLENIENCE"))
        {
	  found = True;
          tmpstyle.flags.common.is_lenient = 0;
          tmpstyle.flag_mask.common.is_lenient = 1;
        }
        else if (StrEquals(token, "NOBUTTON"))
        {
	  found = True;
          butt = -1;
	  GetIntegerArguments(rest, NULL, &butt, 1);
          if (butt == 0)
	    butt = 10;
          if (butt > 0 && butt <= 10)
            tmpstyle.on_buttons |= (1<<(butt-1));
        }
        else if(StrEquals(token, "NOOLDECOR"))
        {
	  found = True;
          tmpstyle.flags.has_ol_decor = 0;
          tmpstyle.flag_mask.has_ol_decor = 1;
        }
        else if(StrEquals(token, "NEVERFOCUS"))
        {
	  found = True;
          tmpstyle.flags.common.focus_mode = FOCUS_NEVER;
          tmpstyle.flag_mask.common.focus_mode = FOCUS_MASK;
          tmpstyle.flags.common.do_grab_focus_when_created = 0;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
        }
        break;

      case 'o':
        if(StrEquals(token, "OLDECOR"))
        {
	  found = True;
          tmpstyle.flags.has_ol_decor = 1;
          tmpstyle.flag_mask.has_ol_decor = 1;
        }
        break;

      case 'p':
        break;

      case 'q':
        break;

      case 'r':
        if(StrEquals(token, "RANDOMPLACEMENT"))
        {
	  found = True;
          tmpstyle.flags.do_place_random = 1;
          tmpstyle.flag_mask.do_place_random = 1;
        }
        else if(StrEquals(token, "RESIZEHINTOVERRIDE"))
        {
	  found = True;
          tmpstyle.flags.common.has_override_size = 1;
          tmpstyle.flag_mask.common.has_override_size = 1;
        }
        else if(StrEquals(token, "RAISETRANSIENT"))
        {
	  found = True;
          tmpstyle.flags.common.do_raise_transient = 1;
          tmpstyle.flag_mask.common.do_raise_transient = 1;
        }
        else if(StrEquals(token, "RaiseTransientStrict"))
        {
	  found = True;
          tmpstyle.flags.common.do_raise_transient = 1;
          tmpstyle.flags.common.do_raise_transient_strict = 1;
          tmpstyle.flag_mask.common.do_raise_transient = 1;
          tmpstyle.flag_mask.common.do_raise_transient_strict = 1;
        }
        break;

      case 's':
        if(StrEquals(token, "SMARTPLACEMENT"))
        {
	  found = True;
          tmpstyle.flags.do_place_smart = 1;
          tmpstyle.flag_mask.do_place_smart = 1;
        }
        else if(StrEquals(token, "SkipMapping"))
        {
	  found = True;
          tmpstyle.flags.common.do_not_show_on_map = 1;
          tmpstyle.flag_mask.common.do_not_show_on_map = 1;
        }
        else if(StrEquals(token, "ShowMapping"))
        {
	  found = True;
          tmpstyle.flags.common.do_not_show_on_map = 0;
          tmpstyle.flag_mask.common.do_not_show_on_map = 1;
        }
        else if(StrEquals(token, "StackTransientParent"))
        {
	  found = True;
          tmpstyle.flags.common.do_stack_transient_parent = 1;
          tmpstyle.flag_mask.common.do_stack_transient_parent = 1;
        }
        else if(StrEquals(token, "StickyIcon"))
        {
	  found = True;
          tmpstyle.flags.common.is_icon_sticky = 1;
          tmpstyle.flag_mask.common.is_icon_sticky = 1;
        }
        else if(StrEquals(token, "SlipperyIcon"))
        {
	  found = True;
          tmpstyle.flags.common.is_icon_sticky = 0;
          tmpstyle.flag_mask.common.is_icon_sticky = 1;
        }
        else if(StrEquals(token, "SLOPPYFOCUS"))
        {
	  found = True;
          tmpstyle.flags.common.focus_mode = FOCUS_SLOPPY;
          tmpstyle.flag_mask.common.focus_mode = FOCUS_MASK;
          tmpstyle.flags.common.do_grab_focus_when_created = 0;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
        }
        else if(StrEquals(token, "StartIconic"))
        {
	  found = True;
          tmpstyle.flags.common.do_start_iconic = 1;
          tmpstyle.flag_mask.common.do_start_iconic = 1;
        }
        else if(StrEquals(token, "StartNormal"))
        {
	  found = True;
          tmpstyle.flags.common.do_start_iconic = 0;
          tmpstyle.flag_mask.common.do_start_iconic = 1;
        }
        else if(StrEquals(token, "StaysOnBottom"))
        {
	  found = True;
          tmpstyle.layer = Scr.BottomLayer;
        }
        else if(StrEquals(token, "StaysOnTop"))
        {
	  found = True;
          tmpstyle.layer = Scr.TopLayer;
        }
        else if(StrEquals(token, "StaysPut"))
        {
	  found = True;
          tmpstyle.layer = Scr.DefaultLayer;
        }
        else if(StrEquals(token, "Sticky"))
        {
	  found = True;
          tmpstyle.flags.common.is_sticky = 1;
          tmpstyle.flag_mask.common.is_sticky = 1;
        }
        else if(StrEquals(token, "Slippery"))
        {
	  found = True;
          tmpstyle.flags.common.is_sticky = 0;
          tmpstyle.flag_mask.common.is_sticky = 1;
        }
        else if(StrEquals(token, "STARTSONDESK"))
        {
	  found = True;
	  /*  RBW - 11/02/1998  */
	  spargs = GetIntegerArguments(rest, NULL, tmpno, 1);
          if (spargs == 1)
            {
              tmpstyle.flags.use_start_on_desk = 1;
              tmpstyle.flag_mask.use_start_on_desk = 1;
              /*  RBW - 11/20/1998 - allow for the special case of -1  */
              tmpstyle.start_desk = (tmpno[0] > -1) ? tmpno[0] + 1 : tmpno[0];
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
	    tmpstyle.start_desk = (tmpno[0] > -1) ? tmpno[0] + 1 : tmpno[0];
	  }
	  if (spargs == 2 || spargs == 3)
	  {
	    if (spargs == 3)
	    {
	      /*  RBW - 11/20/1998 - allow for the special case of -1  */
	      tmpstyle.start_page_x =
		(tmpno[1] > -1) ? tmpno[1] + 1 : tmpno[1];
	      tmpstyle.start_page_y =
		(tmpno[2] > -1) ? tmpno[2] + 1 : tmpno[2];
	    }
	    else
	    {
	      tmpstyle.start_page_x =
		(tmpno[0] > -1) ? tmpno[0] + 1 : tmpno[0];
	      tmpstyle.start_page_y =
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
	    tmpstyle.flags.use_start_on_desk = 1;
	    tmpstyle.flag_mask.use_start_on_desk = 1;
	  }
	}
	/**/
	else if(StrEquals(token, "STARTSANYWHERE"))
	{
	  found = True;
	  tmpstyle.flags.use_start_on_desk = 0;
	  tmpstyle.flag_mask.use_start_on_desk = 1;
	}
        else if (StrEquals(token, "STARTSLOWERED"))
        {
	  found = True;
          tmpstyle.flags.do_start_lowered = 1;
          tmpstyle.flag_mask.do_start_lowered = 1;
        }
        else if (StrEquals(token, "STARTSRAISED"))
        {
	  found = True;
          tmpstyle.flags.do_start_lowered = 0;
          tmpstyle.flag_mask.do_start_lowered = 1;
        }
        break;

      case 't':
        if(StrEquals(token, "TITLE"))
        {
	  found = True;
          tmpstyle.flags.has_no_title = 0;
          tmpstyle.flag_mask.has_no_title = 1;
        }
        break;

      case 'u':
        if(StrEquals(token, "UsePPosition"))
        {
	  found = True;
          tmpstyle.flags.use_no_pposition = 0;
          tmpstyle.flag_mask.use_no_pposition = 1;
        }
#ifdef USEDECOR
        if(StrEquals(token, "UseDecor"))
        {
	  found = True;
	  GetNextToken(rest, &token);
	  if (token)
            tmpstyle.decor_name = token;
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
                  save_name = tmpstyle.name;
		  /* copy everything */
                  memcpy((void*)&tmpstyle, (const void*)add_style,
			 sizeof(window_style));
		  /* except the next pointer */
                  tmpstyle.next = NULL;
		  /* and the name */
                  tmpstyle.name = save_name;
		  /* set not first match */
                  hit = 1;
                }
                else
                {
		  merge_styles(&tmpstyle, add_style);
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
            tmpstyle.flags.common.is_fixed = 0;
            tmpstyle.flag_mask.common.is_fixed = 1;
	  }
        break;

      case 'w':
        if(StrEquals(token, "WindowListSkip"))
        {
	  found = True;
	  tmpstyle.flags.common.do_window_list_skip = 1;
	  tmpstyle.flag_mask.common.do_window_list_skip = 1;
        }
        else if(StrEquals(token, "WindowListHit"))
        {
	  found = True;
	  tmpstyle.flags.common.do_window_list_skip = 0;
	  tmpstyle.flag_mask.common.do_window_list_skip = 1;
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
       * N.Bird 07-Sep-1999
       */
      free(tmpstyle.name);
      free(option);
      return;
    }
    free(option);
  } /* end while still stuff on command */

  /* capture default icons */
  if(StrEquals(tmpstyle.name, "*"))
  {
    if(tmpstyle.flags.has_icon == 1)
      Scr.DefaultIcon = tmpstyle.icon_name;
    tmpstyle.flags.has_icon = 0;
    tmpstyle.flag_mask.has_icon = 0;
    tmpstyle.icon_name = NULL;
  }
  /* add temp name list to list */
  add_style_to_list(&tmpstyle);
}
