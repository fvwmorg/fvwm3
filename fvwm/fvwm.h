/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */
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

#ifndef FVWM_H
#define FVWM_H

/* ---------------------------- included header files ----------------------- */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h>
#include <libs/Picture.h>
#include <libs/Flocale.h>
#include "window_flags.h"

/* ---------------------------- global definitions -------------------------- */

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

#if RETSIGTYPE != void
#define SIGNAL_RETURN return 0
#else
#define SIGNAL_RETURN return
#endif

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#define NULLSTR ((char *) NULL)

/* ---------------------------- global macros ------------------------------- */

/*
 * Fvwm trivia: There were 97 commands in the fvwm command table
 * when the F_CMD_ARGS macro was written.
 * dje 12/19/98.
 */

/* Macro for args passed to fvwm commands... */
#define F_CMD_ARGS fvwm_cond_func_rc *cond_rc, XEvent *eventp, Window w, \
	FvwmWindow *fw, unsigned long context,char *action, int *Module
#define F_PASS_ARGS cond_rc, eventp, w, fw, context, action, Module
#define F_EXEC_ARGS fvwm_cond_func_rc *cond_rc, char *action, \
	FvwmWindow *fw, XEvent *eventp, unsigned long context, int Module
#define F_PASS_EXEC_ARGS cond_rc, action, fw, eventp, context, *Module
#define FUNC_FLAGS_TYPE unsigned char

/* access macros */
#define FW_W_FRAME(fw)        ((fw)->wins.frame)
#define FW_W_PARENT(fw)       ((fw)->wins.parent)
#define FW_W_CLIENT(fw)       ((fw)->wins.client)
#define FW_W(fw)              FW_W_CLIENT(fw)
#define FW_W_TITLE(fw)        ((fw)->wins.title)
#define FW_W_BUTTON(fw,i)     ((fw)->wins.button_w[(i)])
#define FW_W_SIDE(fw,i)       ((fw)->wins.sides[(i)])
#define FW_W_CORNER(fw,i)     ((fw)->wins.corners[(i)])
#define FW_W_ICON_TITLE(fw)   ((fw)->wins.icon_title_w)
#define FW_W_ICON_PIXMAP(fw)  ((fw)->wins.icon_pixmap_w)
#define FW_W_TRANSIENTFOR(fw) ((fw)->wins.transientfor)

/* ---------------------------- forward declarations ------------------------ */

#ifdef USEDECOR
/* definition in screen.h */
struct FvwmDecor;
#endif

/* ---------------------------- type definitions ---------------------------- */

typedef enum
{
	COND_RC_BREAK = -2,
	COND_RC_ERROR = -1,
	COND_RC_NO_MATCH = 0,
	COND_RC_OK = 1
} fvwm_cond_func_rc;

typedef struct
{
	/* return code for conditional commands only */
	fvwm_cond_func_rc *cond_rc;
	/* pointer to the event that caused the function */
	XEvent *eventp;
	/* the fvwm window structure */
	struct FvwmWindow *fw;
	/* the action to execute */
	char *action;
	char **args;
	/* the context in which the button was pressed */
	unsigned long context;
	int module;
	/* If fw is NULL, the is_window_unmanaged flag may be set along
	 * with this field to indicate the function should run with an
	 * unmanaged window. */
	Window win;
	struct
	{
		FUNC_FLAGS_TYPE exec;
		unsigned do_save_tmpwin : 1;
		unsigned is_window_unmanaged : 1;
	} flags;
} exec_func_args_type;

/*
  For 1 style statement, there can be any number of IconBoxes.
  The name list points at the first one in the chain.
*/
typedef struct icon_boxes_struct
{
	struct icon_boxes_struct *next;       /* next icon_boxes or zero */
	unsigned int use_count;
	int IconBox[4];                       /* x/y x/y for iconbox */
	int IconScreen;                       /* Xinerama screen */
	short IconGrid[2];                    /* x incr, y incr */
	char IconSign[4];                     /* because of -0, need to save */
	unsigned is_orphan : 1;
	unsigned IconFlags : 3;               /* some bits */
	/* IconFill only takes 3 bits.  Defaults are top, left, vert co-ord
	 * first eg: t l = 0,0,0; l t = 0,0,1; b r = 1,1,0 */
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

typedef struct
{
	int left;
	int right;
	int top;
	int bottom;
} ewmh_strut;

typedef struct
{
	/* common flags (former flags in bits 0-12) */
	unsigned is_sticky : 1;
	unsigned has_icon_font : 1;
	unsigned has_window_font : 1;
	unsigned title_dir : 2;
	/* static flags that do not change dynamically after the window has been
	 * created */
	struct
	{
		unsigned do_circulate_skip : 1;
		unsigned do_circulate_skip_icon : 1;
		unsigned do_circulate_skip_shaded : 1;
		unsigned do_ewmh_donate_icon : 1;
		unsigned do_ewmh_donate_mini_icon : 1;
		unsigned do_ewmh_ignore_state_hints : 1;
		unsigned do_ewmh_ignore_strut_hints : 1;
		unsigned do_ewmh_mini_icon_override : 1;
		unsigned do_ewmh_use_stacking_hints : 1;
		unsigned do_grab_focus_when_created : 1;
		unsigned do_grab_focus_when_transient_created : 1;
		unsigned do_iconify_window_groups : 1;
		unsigned do_ignore_gnome_hints : 1;
		unsigned do_ignore_icon_boxes : 1;
		unsigned do_ignore_mouse_focus_click_motion : 1;
		unsigned do_ignore_restack : 1;
		unsigned do_use_window_group_hint : 1;
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
		unsigned ewmh_maximize_mode : 2; /* see ewmh.h */
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
		unsigned icon_override : 2;
#define NO_ACTIVE_ICON_OVERRIDE 0
#define ICON_OVERRIDE           1
#define NO_ICON_OVERRIDE        2
#define ICON_OVERRIDE_MASK      3
		unsigned is_fixed : 1;
		unsigned is_fixed_ppos : 1;
		unsigned is_icon_sticky : 1;
		unsigned is_icon_suppressed : 1;
		unsigned is_left_title_rotated_cw : 1; /* cw = clock wise */
		unsigned is_lenient : 1;
		unsigned is_size_fixed : 1;
		unsigned is_psize_fixed : 1;
		unsigned is_right_title_rotated_cw : 1;
		unsigned use_icon_position_hint : 1;
		unsigned use_indexed_window_name : 1;
		unsigned use_indexed_icon_name : 1;
	} s;
} common_flags_type;

typedef struct
{
	common_flags_type common;
	unsigned does_wm_delete_window : 1;
	unsigned does_wm_take_focus : 1;
	/* delete is_icon_moved flag after evaluating it for the first time */
	unsigned do_delete_icon_moved : 1;
	unsigned do_iconify_after_map : 1;
	/* Reuse this struct, don't free it, when destroying/recapturing
	 * window. */
	unsigned do_reuse_destroyed : 1;
	/* Is the window decorated with a border? */
	unsigned has_border : 1;
	/* Does it have resize handles? */
	unsigned has_handles : 1;
	/* Icon change is pending */
	unsigned has_icon_changed : 1;
	/* Is this decorated with title */
	unsigned has_title : 1;
	/* ChangeDecor was used for window */
	unsigned is_decor_changed : 1;
	/* Sent an XUnmapWindow for deiconifying, but didn't receive a
	 * UnmapNotify yet.*/
	unsigned is_deiconify_pending : 1;
	/* window had the focus when the desk was switched. set if the window
	 * was mapped and got focused but the focus change was not announced
	 * to the modules yet. */
	unsigned is_focused_on_other_desk : 1;
	unsigned is_focus_change_broadcast_pending : 1;
	/* is the window fully visible */
	unsigned is_fully_visible : 1;
	/* is it an icon now? */
	unsigned is_iconified : 1;
	/* To prevent iconified transients in a parent icon from counting for
	 * Next */
	unsigned is_iconified_by_parent : 1;
	/* is the pointer over the icon? */
	unsigned is_icon_entered : 1;
	unsigned is_icon_font_loaded : 1;
	/* has the icon been moved by the user? */
	unsigned is_icon_moved : 1;
	/* is the icon window supplied by the app? */
	unsigned is_icon_ours : 1;
	/* is the icon shaped? */
	unsigned is_icon_shaped : 1;
	/* was the icon unmapped, even though the window is still iconified
	 * (Transients) */
	unsigned is_icon_unmapped : 1;
	/* temporary flag used in stack.c */
	unsigned is_in_transient_subtree : 1;
	/* is it mapped? */
	unsigned is_mapped : 1;
	/* Sent an XMapWindow, but didn't receive a MapNotify yet.*/
	unsigned is_map_pending : 1;
	/* is the window maximized? */
	unsigned is_maximized : 1;
	/* Set if the client changes its WM_NAME. The source of twm contains
	 * an explanation why we need this information. */
	unsigned is_name_changed : 1;
	/* is the window partially visible */
	unsigned is_partially_visible : 1;
	/* is the icon pixmap ours to free? */
	unsigned is_pixmap_ours : 1;
	unsigned is_placed_wb3 : 1;
	/* fvwm2 place the window itself */
	unsigned is_placed_by_fvwm : 1;
	/* mark window to be destroyed after last complex func has finished. */
	unsigned is_scheduled_for_destroy : 1;
	/* mark window to be raised after function execution. */
	unsigned is_scheduled_for_raise : 1;
	unsigned is_size_inc_set : 1;
	unsigned is_style_deleted : 1;
	/* the window is a torn out fvwm menu */
	unsigned is_tear_off_menu : 1;
	/* is it a transient window? */
	unsigned is_transient : 1;
	unsigned is_window_drawn_once : 1;
	/* To prevent double move in MoveViewport.*/
	unsigned is_viewport_moved : 1;
	unsigned is_window_being_moved_opaque : 1;
	unsigned is_window_font_loaded : 1;
	unsigned is_window_shaded : 1;
	unsigned shaded_dir : 3;
	unsigned using_default_icon_font : 1;
	unsigned using_default_window_font : 1;
#define ICON_HINT_NEVER    0
#define ICON_HINT_ONCE     1
#define ICON_HINT_MULTIPLE 2
	unsigned was_icon_hint_provided : 2;
	unsigned was_icon_name_provided : 1;
	unsigned has_ewmh_wm_name : 1;
	unsigned has_ewmh_wm_icon_name : 1;
#define EWMH_NO_ICON     0 /* the application does not provide an ewmh icon */
#define EWMH_TRUE_ICON   1 /* the application does provide an ewmh icon */
#define EWMH_FVWM_ICON   2 /* the ewmh icon has been set by fvwm */
	unsigned has_ewmh_wm_icon_hint : 2;
	/* says if the app have an ewmh icon of acceptable size for a mini
	 * icon in its list of icons */
	unsigned has_ewmh_mini_icon : 1;
	/* the ewmh icon is used as icon pixmap */
	unsigned use_ewmh_icon : 1;
	unsigned is_ewmh_modal : 1;
#define EWMH_STATE_UNDEFINED_HINT 0
#define EWMH_STATE_NO_HINT        1
#define EWMH_STATE_HAS_HINT       2
	unsigned has_ewmh_init_fullscreen_state : 2;
	unsigned has_ewmh_init_hidden_state : 2;
	unsigned has_ewmh_init_maxhoriz_state : 2;
	unsigned has_ewmh_init_maxvert_state : 2;
	unsigned has_ewmh_init_modal_state : 2;
	unsigned has_ewmh_init_shaded_state : 2;
	unsigned has_ewmh_init_skip_pager_state : 2;
	unsigned has_ewmh_init_skip_taskbar_state : 2;
	unsigned has_ewmh_init_sticky_state : 2;
	unsigned has_ewmh_init_wm_desktop : 2;
	unsigned user_states : 32;
} window_flags;

/* Window mask for Circulate and Direction functions */
typedef struct WindowConditionMask
{
	struct
	{
		unsigned do_accept_focus : 1;
		unsigned needs_current_desk : 1;
		unsigned needs_current_page : 1;
		unsigned needs_current_global_page : 1;
#define NEEDS_ANY        0
#define NEEDS_TRUE       1
#define NEEDS_FALSE      2
		unsigned needs_focus : 2;
		unsigned needs_name : 1;
		unsigned needs_not_name : 1;
		unsigned needs_pointer : 2;
		unsigned use_circulate_hit : 1;
		unsigned use_circulate_hit_icon : 1;
		unsigned use_circulate_hit_shaded : 1;
		unsigned use_do_accept_focus : 1;
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
	/* old placement flags */
#define PLACE_DUMB            0x0
#define PLACE_SMART           0x1
#define PLACE_CLEVER          0x2
#define PLACE_CLEVERNESS_MASK 0x3
#define PLACE_RANDOM          0x4
	/* new placements value, try to get a minimal backward compatibility
	 * with the old flags:
	 * Dumb+Active=Manual,
	 * Dumb+Random=Cascade,
	 * Smart+Random= TileCascade,
	 * Smart+Active=TileManual,
	 * Random+Smart+Clever=MINOVERLAP which is the original Clever
	 * placement code,
	 * Active+Smart+Clever= MINOVERLAPPERCENT which is the "new" Clever
	 * placement code and was the original Clever placement code. Now the
	 * original placement code said:
	 * Active/Random+Dumb+Clever=Active/Random+Dumb (with Dumb Clever is
	 * ignored); These represent the not use value: 0x2=Active+Dumb+Clever,
	 * 0x6=Random+Dumb+Clever */
#define PLACE_MANUAL            0x0
#define PLACE_TILEMANUAL        0x1
#define PLACE_MANUAL_B          0x2
#define PLACE_MINOVERLAPPERCENT 0x3
#define PLACE_CASCADE           0x4
#define PLACE_TILECASCADE       0x5
#define PLACE_CASCADE_B         0x6
#define PLACE_MINOVERLAP        0x7
#define PLACE_MASK              0x7
	unsigned placement_mode : 3;
	unsigned ewmh_placement_mode : 2; /* see ewmh.h */
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
	unsigned has_mini_icon : 1;
	unsigned has_mwm_decor : 1;
	unsigned has_mwm_functions : 1;
	unsigned has_no_handles : 1;
	unsigned has_no_title : 1;
	unsigned has_ol_decor : 1;
#if 0
	unsigned has_condition_mask : 1;
#endif
	unsigned is_button_disabled : NUMBER_OF_BUTTONS;
#define BACKINGSTORE_DEFAULT 0
#define BACKINGSTORE_ON      1
#define BACKINGSTORE_OFF     2
#define BACKINGSTORE_MASK  0x3
	unsigned use_backing_store : 2;
	unsigned use_parent_relative : 1;
	unsigned use_colorset : 1;
	unsigned use_colorset_hi : 1;
	unsigned use_border_colorset : 1;
	unsigned use_border_colorset_hi : 1;
	unsigned use_layer : 1;
	unsigned use_no_pposition : 1;
	unsigned use_no_usposition : 1;
	unsigned use_no_transient_pposition : 1;
	unsigned use_no_transient_usposition : 1;
	unsigned use_start_on_desk : 1;
	unsigned use_start_on_page_for_transient : 1;
	unsigned use_start_on_screen : 1;
	unsigned manual_placement_honors_starts_on_page : 1;
	unsigned capture_honors_starts_on_page : 1;
	unsigned recapture_honors_starts_on_page : 1;
	unsigned has_placement_penalty : 1;
	unsigned has_placement_percentage_penalty : 1;
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
	char *mini_icon_name;
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
	short border_width;
	/* resize handle width */
	short handle_width;
	int layer;
	int start_desk;
	int start_page_x;
	int start_page_y;
	int start_screen;
	int max_window_width;
	int max_window_height;
	int shade_anim_steps;
	icon_boxes *icon_boxes;
	float norm_placement_penalty;
	float placement_penalty[6];
	int placement_percentage_penalty[4];
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
	/* name of the window */
	FlocaleNameString name;
	/* name of the icon */
	FlocaleNameString icon_name;
	char *visible_name;
	char *visible_icon_name;
	int name_count;
	int icon_name_count;
	/* next fvwm window */
	struct FvwmWindow *next;
	/* prev fvwm window */
	struct FvwmWindow *prev;
	/* next (lower) fvwm window in stacking order*/
	struct FvwmWindow *stack_next;
	/* prev (higher) fvwm window in stacking order */
	struct FvwmWindow *stack_prev;
	/* border width before reparenting */
	struct
	{
		/* the frame window */
		Window frame;
		/* It looks like you HAVE to reparent the app window into a
		 * window whose size = app window, or else you can't keep xv
		 * and matlab happy at the same time! */
		Window parent;
		/* the child window */
		Window client;
		/* the title bar window and button windows */
		Window title;
		Window button_w[NUMBER_OF_BUTTONS];
		/* sides of the border */
		Window sides[4];
		/* corner pieces */
		Window corners[4];
		/* icon title window */
		Window icon_title_w;
		/* icon picture window */
		Window icon_pixmap_w;
		Window transientfor;
	} wins;
	window_flags flags;
	struct
	{
		unsigned buttons_drawn : NUMBER_OF_BUTTONS;
		unsigned buttons_lit : NUMBER_OF_BUTTONS;
		unsigned buttons_inverted : NUMBER_OF_BUTTONS;
		unsigned buttons_toggled : NUMBER_OF_BUTTONS;
		unsigned parts_drawn : 12;
		unsigned parts_lit : 12;
		unsigned parts_inverted : 12;
		unsigned parts_toggled : 12;
	} decor_state;
	int nr_left_buttons;
	int nr_right_buttons;
#define BUTTON_INDEX(b) (((b) == 0) ? (NUMBER_OF_BUTTONS - 1) : ((b) - 1))
#ifdef USEDECOR
	struct FvwmDecor *decor;
#endif
	/* is this a shaped window */
	int wShaped;
	Pixmap title_background_pixmap;
	Pixmap button_background_pixmap[NUMBER_OF_BUTTONS];

	short boundary_width;
	short corner_width;
	short visual_corner_width;

	/* title font */
	FlocaleFont *title_font;
	/* /Y coordinate to draw the title name */
	short title_text_offset;
	short title_length;
	short title_thickness;
	text_rotation_type title_text_rotation;
	struct
	{
		/* geometry of the icon picture window */
		rectangle picture_w_g;
		/* geometry of the icon title window */
		rectangle title_w_g;
		/* width of the text in the icon title */
		int title_text_width;
	} icon_g;

	/* Drawable depth for the icon */
	int iconDepth;
	/* pixmap for the icon */
	Pixmap iconPixmap;
	/* pixmap for the icon mask */
	Pixmap icon_maskPixmap;
	FlocaleFont *icon_font;

	/* some parts of the window attributes */
	struct
	{
		int backing_store;
		int border_width;
		int depth;
		Visual *visual;
		Colormap colormap;
	} attr_backup;
	/* normal hints */
	XSizeHints hints;
	/* WM hints */
	XWMHints *wmhints;
	XClassHint class;
	/* Tells which desktop this window is on */
	int Desk;
	/* Where (if at all) was it focussed */
	int FocusDesk;
	/* Desk to deiconify to, for StubbornIcons */
	int DeIconifyDesk;

	char *mini_pixmap_file;
	Picture *mini_icon;
	char *icon_bitmap_file;

	rectangle frame_g;
	/* absolute geometry when not maximized */
	rectangle normal_g;
	/* maximized window geometry */
	rectangle max_g;
	/* defect between maximized geometry before and after constraining
	 * size. */
	size_rect max_g_defect;
	/* original delta between normalized and maximized window, used to
	 * keep unmaximized window at same screen position */
	position max_offset;
	int *mwm_hints;
	int ol_hints;
	int functions;
	/* Colormap windows property */
	Window *cmap_windows;
	/* Should generally be 0 */
	int number_cmap_windows;
	color_quad colors;
	color_quad hicolors;
	color_quad border_colors;
	color_quad border_hicolors;

	unsigned long buttons;
	/* zero or more iconboxes */
	icon_boxes *IconBoxes;

	int default_layer;
	int layer;
	int max_window_width;
	int max_window_height;
	int shade_anim_steps;
	unsigned char grabbed_buttons;

#define FM_NO_INPUT        0
#define FM_PASSIVE         1
#define FM_LOCALLY_ACTIVE  2
#define FM_GLOBALLY_ACTIVE 3
	unsigned char focus_model;

	float placement_penalty[6];
	int placement_percentage_penalty[4];

#define EWMH_WINDOW_TYPE_NONE_ID      0
#define EWMH_WINDOW_TYPE_DESKTOP_ID   1
#define EWMH_WINDOW_TYPE_DIALOG_ID    2
#define EWMH_WINDOW_TYPE_DOCK_ID      3
#define EWMH_WINDOW_TYPE_MENU_ID      4
#define EWMH_WINDOW_TYPE_NORMAL_ID    5
#define EWMH_WINDOW_TYPE_TOOLBAR_ID   6
	int ewmh_window_type;
	/* icon geometry */
	rectangle ewmh_icon_geometry;
	/* for computing the working area */
	ewmh_strut strut;
	/* for the dynamic working area */
	ewmh_strut dyn_strut;
	/* memories for the icons we set on the */
	int ewmh_icon_height;
	/* _NET_WM_ICON */
	int ewmh_icon_width;
	int ewmh_mini_icon_height;
	int ewmh_mini_icon_width;
	/* memory for the initial _NET_WM_STATE */
	int ewmh_hint_layer;
	/* memory for the initial _NET_WM_STATE */
	unsigned long ewmh_hint_desktop;

	/* multi purpose scratch pointer */
	void *pscratch;
} FvwmWindow;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

void SetMWM_INFO(Window window);

#endif /* FVWM_H */
