#define POST_24_FEATURES 1
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

#ifndef FVWM_H
#define FVWM_H

/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified
 * by Rob Nation
 ****************************************************************************/
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


/***********************************************************************
 * fvwm include file
 ***********************************************************************/

#define F_CMD_ARGS XEvent *eventp, Window w, FvwmWindow *tmp_win,\
unsigned long context,char *action, int *Module
#define F_PASS_ARGS eventp, w, tmp_win, context, action, Module

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h>
#include <libs/Picture.h>
#include "window_flags.h"

/* Allow GCC extensions to work, if you have GCC */

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__
#  define __attribute__(x)
# endif
/* The __-protected variants of `format' and `printf' attributes
   are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __format__ format
#  define __printf__ printf
# endif
#endif

#ifndef WithdrawnState
#define WithdrawnState 0
#endif

#define NO_FOCUS_WIN_EVMASK KeyPressMask|FocusChangeMask
#define NO_FOCUS_WIN_MENU_EVMASK KeyPressMask|KeyReleaseMask|FocusChangeMask

/* the maximum number of mouse buttons fvwm knows about */
/* don't think that upping this to 5 will make everything
 * hunky-dory with 5 button mouses */
#define MAX_BUTTONS 3

#if RETSIGTYPE != void
#define SIGNAL_RETURN return 0
#else
#define SIGNAL_RETURN return
#endif

#define BOUNDARY_WIDTH 7    	/* border width */

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#define NULLSTR ((char *) NULL)

typedef struct
{
  int x;
  int y;
  int width;
  int height;
} rectangle;

typedef struct
{
  int x;
  int y;
} position;

/*
  For 1 style statement, there can be any number of IconBoxes.
  The name list points at the first one in the chain.
 */
typedef struct icon_boxes_struct
{
  struct icon_boxes_struct *next;       /* next icon_boxes or zero */
  unsigned int use_count;
  int IconBox[4];                       /* x/y x/y for iconbox */
  short IconGrid[2];                    /* x incr, y incr */
  unsigned is_orphan : 1;
  unsigned char IconFlags : 3;          /* some bits */
  /* IconFill only takes 3 bits.  Defaults are top, left, vert co-ord first */
  /* eg: t l = 0,0,0; l t = 0,0,1; b r = 1,1,0 */
#define ICONFILLBOT (1<<0)
#define ICONFILLRGT (1<<1)
#define ICONFILLHRZ (1<<2)
} icon_boxes;

typedef struct
{
  Pixel fore;
  Pixel back;
} ColorPair;

typedef struct
{
  Pixel fore;
  Pixel back;
  Pixel hilight;
  Pixel shadow;
} color_quad;

#ifdef USEDECOR
struct FvwmDecor;		/* definition in screen.h */
#endif

typedef struct
{
  /* common flags (former flags in bits 0-12) */
  unsigned is_sticky : 1;
  unsigned has_icon_font : 1;
  unsigned has_window_font : 1;
  /* static flags that do not change dynamically after the window has been
   * created */
  struct
  {
    unsigned do_circulate_skip : 1;
    unsigned do_circulate_skip_icon : 1;
    unsigned do_circulate_skip_shaded : 1;
    unsigned do_flip_transient : 1;
    unsigned do_grab_focus_when_created : 1;
    unsigned do_grab_focus_when_transient_created : 1;
    unsigned do_ignore_restack : 1;
    unsigned do_lower_transient : 1;
    unsigned do_not_show_on_map : 1;
    unsigned do_not_pass_click_focus_click : 1;
    unsigned do_not_raise_click_focus_click : 1;
    unsigned do_raise_mouse_focus_click : 1;
    unsigned do_raise_transient : 1;
    unsigned do_resize_opaque : 1;
    unsigned do_shrink_windowshade : 1;
    unsigned do_stack_transient_parent : 1;
    unsigned do_start_iconic : 1;
    unsigned do_window_list_skip : 1;
#define FOCUS_MOUSE   0x0
#define FOCUS_CLICK   0x1
#define FOCUS_SLOPPY  0x2
#define FOCUS_NEVER   0x3
#define FOCUS_MASK    0x3
    unsigned focus_mode : 2;
    unsigned has_bottom_title : 1;
    unsigned has_depressable_border : 1;
    unsigned has_mwm_border : 1;
    unsigned has_mwm_buttons : 1;
    unsigned has_mwm_override : 1;
    unsigned has_no_icon_title : 1;
    unsigned has_override_size : 1;
    unsigned has_stippled_title : 1;
    unsigned is_fixed : 1;
    unsigned is_icon_sticky : 1;
    unsigned is_icon_suppressed : 1;
    unsigned is_lenient : 1;
  } s;
} common_flags_type;

typedef struct
{
  common_flags_type common;
  unsigned does_wm_delete_window : 1;
  unsigned does_wm_take_focus : 1;
  unsigned do_iconify_after_map : 1;
  unsigned do_reuse_destroyed : 1;   /* Reuse this struct, don't free it,
				      * when destroying/recapturing window. */
  unsigned has_border : 1; /* Is this decorated with border*/
  unsigned has_title : 1; /* Is this decorated with title */
  unsigned is_deiconify_pending : 1; /* Sent an XUnmapWindow for deiconifying,
				      * but didn't receive a UnmapNotify yet.*/
  unsigned is_fully_visible : 1; /* is the window fully visible */
  unsigned is_iconified : 1; /* is it an icon now? */
  unsigned is_iconified_by_parent : 1; /* To prevent iconified transients in a
					* parent icon from counting for Next */
  unsigned is_icon_entered : 1; /* is the pointer over the icon? */
  unsigned is_icon_font_loaded : 1;
  unsigned is_icon_moved : 1; /* has the icon been moved by the user? */
  unsigned is_icon_ours : 1; /* is the icon window supplied by the app? */
  unsigned is_icon_shaped : 1; /* is the icon shaped? */
  unsigned is_icon_unmapped : 1; /* was the icon unmapped, even though the
				  * window is still iconified (Transients) */
  unsigned is_mapped : 1; /* is it mapped? */
  unsigned is_map_pending : 1; /* Sent an XMapWindow, but didn't receive a
				* MapNotify yet.*/
  unsigned is_maximized : 1; /* is the window maximized? */
  unsigned is_name_changed : 1; /* Set if the client changes its WM_NAME.
				 * The source of twm contains an explanation
				 * why we need this information. */
  unsigned is_partially_visible : 1; /* is the window partially visible */
  unsigned is_pixmap_ours : 1; /* is the icon pixmap ours to free? */
#ifdef POST_24_FEATURES
  unsigned is_placed_wb3 : 1;
#endif
  unsigned is_size_inc_set : 1;
  unsigned is_transient : 1; /* is it a transient window? */
  unsigned is_window_drawn_once : 1;
  unsigned is_viewport_moved : 1; /* To prevent double move in MoveViewport.*/
  unsigned is_window_being_moved_opaque : 1;
  unsigned is_window_font_loaded : 1;
  unsigned is_window_shaded : 1;
} window_flags;

/* Window mask for Circulate and Direction functions */
typedef struct WindowConditionMask
{
  struct
  {
    unsigned needs_current_desk : 1;
    unsigned needs_current_page : 1;
    unsigned needs_name : 1;
    unsigned needs_not_name : 1;
    unsigned use_circulate_hit : 1;
    unsigned use_circulate_hit_icon : 1;
    unsigned use_circulate_skip : 1;
    unsigned use_circulate_skip_icon : 1;
  } my_flags;
  window_flags flags;
  window_flags flag_mask;
  char *name;
  int layer;
} WindowConditionMask;

/* only style.c and add_window.c are allowed to access this struct!! */
typedef struct
{
  common_flags_type common;
  unsigned do_decorate_transient : 1;
#define PLACE_DUMB            0x0
#define PLACE_SMART           0x1
#define PLACE_CLEVER          0x2
#define PLACE_CLEVERNESS_MASK 0x3
#define PLACE_RANDOM          0x4
#define PLACE_MASK            0x7
  unsigned placement_mode : 3;
  unsigned do_save_under : 1;
  unsigned do_start_lowered : 1;
  unsigned has_border_width : 1;
  unsigned has_color_back : 1;
  unsigned has_color_fore : 1;
  unsigned has_color_back_hi : 1;
  unsigned has_color_fore_hi : 1;
  unsigned has_decor : 1;
  unsigned has_handle_width : 1;
  unsigned has_icon : 1;
  unsigned has_icon_boxes : 1;
  unsigned has_max_window_size : 1;
  unsigned has_window_shade_steps : 1;
#ifdef MINI_ICONS
  unsigned has_mini_icon : 1;
#endif
  unsigned has_mwm_decor : 1;
  unsigned has_mwm_functions : 1;
  unsigned has_no_border : 1;
  unsigned has_no_title : 1;
  unsigned has_ol_decor : 1;
#if 0
  unsigned has_condition_mask : 1;
#endif
  unsigned is_button_disabled : 10;
  unsigned use_backing_store : 1;
  unsigned use_parent_relative : 1;
  unsigned use_colorset : 1;
  unsigned use_colorset_hi : 1;
  unsigned use_border_colorset : 1;
  unsigned use_border_colorset_hi : 1;
  unsigned use_layer : 1;
  unsigned use_no_pposition : 1;
  unsigned use_start_on_desk : 1;
  unsigned use_start_on_page_for_transient : 1;
  unsigned active_placement_honors_starts_on_page : 1;
  unsigned capture_honors_starts_on_page : 1;
  unsigned recapture_honors_starts_on_page : 1;
  unsigned icon_override : 2;
#define NO_ACTIVE_ICON_OVERRIDE 0
#define ICON_OVERRIDE           1
#define NO_ICON_OVERRIDE        2
#define ICON_OVERRIDE_MASK      3
} style_flags;

/* only style.c and add_window.c are allowed to access this struct!! */
typedef struct window_style
{
  struct window_style *next;
  struct window_style *prev;
  char *name;
#if 0
  WindowConditionMask *condition_mask;
#endif
  char *icon_name;
#ifdef MINI_ICONS
  char *mini_icon_name;
#endif
#ifdef USEDECOR
  char *decor_name;
#endif
  char *icon_font;
  char *window_font;
  char *fore_color_name;
  char *back_color_name;
  char *fore_color_name_hi;
  char *back_color_name_hi;
  int colorset;
  int colorset_hi;
  int border_colorset;
  int border_colorset_hi;
  int border_width;
  int handle_width; /* resize handle width */
  int layer;
  int start_desk;
  int start_page_x;
  int start_page_y;
  int max_window_width;
  int max_window_height;
  int shade_anim_steps;
  icon_boxes *icon_boxes;
  style_flags flags;
  style_flags flag_mask;
  style_flags change_mask;
  unsigned has_style_changed : 1;
} window_style;

/* for each window that is on the display, one of these structures
 * is allocated and linked into a list
 */
typedef struct FvwmWindow
{
  struct FvwmWindow *next;    /* next fvwm window */
  struct FvwmWindow *prev;    /* prev fvwm window */
  struct FvwmWindow *stack_next; /* next (lower) fvwm window in stacking
				  * order*/
  struct FvwmWindow *stack_prev; /* prev (higher) fvwm window in stacking
				  * order */
  Window w;                   /* the child window */
  int old_bw;                 /* border width before reparenting */
  Window frame;               /* the frame window */
  Window decor_w;             /* parent of decoration windows */
  Window Parent;              /* Ugly Ugly Ugly - it looks like you
			       * HAVE to reparent the app window into
			       * a window whose size = app window,
			       * or else you can't keep xv and matlab
			       * happy at the same time! */
  Window title_w;             /* the title bar window */
  Window sides[4];
  Window corners[4];          /* Corner pieces */
  int nr_left_buttons;
  int nr_right_buttons;
  Window button_w[NUMBER_OF_BUTTONS];
#define BUTTON_INDEX(b) (((b) == 0) ? (NUMBER_OF_BUTTONS - 1) : ((b) - 1))
#ifdef USEDECOR
  struct FvwmDecor *decor;
#endif
  Window icon_w;              /* the icon window */
  Window icon_pixmap_w;       /* the icon window */
#ifdef SHAPE
  int wShaped;               /* is this a shaped window */
#endif

  int boundary_width;
  short corner_width;
  short visual_corner_width;

  rectangle title_g;
  rectangle icon_g;           /* geometry of the icon window */
  int icon_xl_loc;            /* icon label window x coordinate */
  int icon_t_width;           /* width of the icon title window */
  int icon_p_width;           /* width of the icon pixmap window */
  int icon_p_height;          /* height of the icon pixmap window */
  Pixmap iconPixmap;          /* pixmap for the icon */
  int iconDepth;              /* Drawable depth for the icon */
  Pixmap icon_maskPixmap;     /* pixmap for the icon mask */
  char *name;                 /* name of the window */
  char *icon_name;            /* name of the icon */
#ifdef I18N_MB
  char **name_list;           /* window name list */
  char **icon_name_list;      /* icon name list */
#endif
  FvwmFont title_font;
  FvwmFont icon_font;
  XWindowAttributes attr;     /* the child window attributes */
  XSizeHints hints;           /* normal hints */
  XWMHints *wmhints;          /* WM hints */
  XClassHint class;
  int Desk;                   /* Tells which desktop this window is on */
  int FocusDesk;              /* Where (if at all) was it focussed */
  int DeIconifyDesk;          /* Desk to deiconify to, for StubbornIcons */
  Window transientfor;

  window_flags flags;

#ifdef MINI_ICONS
  char *mini_pixmap_file;
  Picture *mini_icon;
#endif
  char *icon_bitmap_file;

  rectangle frame_g;
  rectangle normal_g;         /* absolute geometry when not maximized */
  rectangle max_g;            /* maximized window geometry */
  position max_offset;        /* original delta between normalized and
			       * maximized window, used to keep unmaximized
			       * window at same screen position */
  int *mwm_hints;
  int ol_hints;
  int functions;
  Window *cmap_windows;       /* Colormap windows property */
  int number_cmap_windows;    /* Should generally be 0 */
  color_quad colors;
  color_quad hicolors;
  color_quad border_colors;
  color_quad border_hicolors;

  unsigned long buttons;
  icon_boxes *IconBoxes;              /* zero or more iconboxes */

  int default_layer;
  int layer;
  int max_window_width;
  int max_window_height;
  int shade_anim_steps;
} FvwmWindow;

/* include this down here because FvwmWindows must be defined when including
 * this header file. */
#include "fvwmdebug.h"

#endif /* _FVWM_ */
