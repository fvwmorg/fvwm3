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

#ifndef _STYLE_
#define _STYLE_

#include "fvwm.h"
#include "gsfr.h"

typedef struct name_list_struct
{
  struct name_list_struct *next;   /* pointer to the next name */
  char *name;		  	   /* the name of the window */

/*
 * definitely only part of the style
 */

#ifndef GSFR
  char *value;                     /* icon name */
#ifdef MINI_ICONS
  char *mini_value;                /* mini icon name */
#endif
#ifdef USEDECOR
  char *Decor;
#endif

  int Desk;                        /* Desktop number */
/* RBW - 11/02/1998 - page x,y numbers */
  int PageX;
  int PageY;
/**/
  unsigned long on_flags;
  unsigned long off_flags;
  unsigned long on_buttons;
  unsigned long off_buttons;
  char *ForeColor;
  char *BackColor;
  int layer;
  struct {
    unsigned has_layer : 1; /* has layer been set explicitly ? */
    unsigned starts_lowered : 1;
    unsigned has_starts_lowered : 1; /* has starts_lowered been set ? */
  } tmpflags;
  icon_boxes *IconBoxes;                /* pointer to iconbox(s) */
  int border_width;
  int resize_width;
#endif
} name_list;

typedef struct
{
  common_flags_type common_flags;
  unsigned bw : 1; /* BW_FLAG */
  unsigned nobw : 1; /* NOBW_FLAG */
  unsigned color_fore : 1; /* FORE_COLOR_FLAG */
  unsigned color_back : 1; /* BACK_COLOR_FLAG */
  unsigned decorate_transient : 1; /* DECORATE_TRANSIENT_FLAG */
  unsigned grab_focus_when_created : 1; /* was grab_focus */
  unsigned has_no_title : 1; /* NOTITLE_FLAG */
  unsigned has_no_border : 1; /* NOBORDER_FLAG */
  unsigned has_icon : 1; /* ICON_FLAG */
#ifdef MINI_ICONS
  unsigned mini_icon : 1; /* MINIICON_FLAG */
#endif
  unsigned mwm_button : 1; /* MWM_BUTTON_FLAG */
  unsigned mwm_decor : 1; /* MWM_DECOR_FLAG */
  unsigned mwm_functions : 1; /* MWM_FUNCTIONS_FLAG */
  unsigned mwm_override : 1; /* MWM_OVERRIDE_FLAG */
  unsigned mwm_border : 1; /* MWM_BORDER_FLAG */
  unsigned no_pposition : 1; /* NO_PPOSITION_FLAG */
  unsigned ol_decor : 1; /* OL_DECOR_FLAG */
  unsigned place_random : 1; /* RANDOM_PLACE_FLAG */
  unsigned place_smart : 1; /* SMART_PLACE_FLAG */
  unsigned start_lowered : 1; /* starts_lowered */
  unsigned use_start_lowered : 1; /* has_starts_lowered */ /* has starts_lowered been set ? */
  unsigned use_start_on_desk : 1; /* STARTSONDESK_FLAG */
  unsigned use_layer : 1; /* has_layer */ /* has layer been set explicitly ? */
} style_flags;

typedef struct
{
  style_flags on_flags;
  style_flags off_flags;
  unsigned long on_buttons;
  unsigned long off_buttons;
  int border_width;
  int layer;
  int handle_width; /* resize_width */
  int start_desk; /* Desk */
  int start_page_x; /* PageX */
  int start_page_y; /* PageY */
  char *ForeColor;
  char *BackColor;
  char *icon_name; /* value */               /* icon name */
#ifdef MINI_ICONS
  char *mini_icon_name; /* mini_value */               /* mini icon name */
#endif
#ifdef USEDECOR
  char *decor_name; /* Decor */
#endif
  icon_boxes *IconBoxes;                /* pointer to iconbox(s) */
} window_style;



void ProcessNewStyle(F_CMD_ARGS);
void LookInList(FvwmWindow *tmp_win, name_list *styles);
void merge_styles(name_list *, name_list *);

#endif /* _STYLE_ */
