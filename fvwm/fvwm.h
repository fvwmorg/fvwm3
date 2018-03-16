/* -*-c-*- */
/* This module is based on Twm, but has been siginificantly modified
 * by Rob Nation */
/*
 *       Copyright 1988 by Evans & Sutherland Computer Corporation,
 *                          Salt Lake City, Utah
 *  Portions Copyright 1989 by the Massachusetts Institute of Technology
 *                        Cambridge, Massachusetts
 *
 *                           All Rights Reserved
 *
 *    Permission to use, copy, modify, and distribute this software and
 *    its documentation  for  any  purpose  and  without  fee is hereby
 *    granted, provided that the above copyright notice appear  in  all
 *    copies and that both  that  copyright  notice  and  this  permis-
 *    sion  notice appear in supporting  documentation,  and  that  the
 *    names of Evans & Sutherland and M.I.T. not be used in advertising
 *    in publicity pertaining to distribution of the  software  without
 *    specific, written prior permission.
 *
 *    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD
 *    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-
 *    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR
 *    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-
 *    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA
 *    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE
 *    OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef FVWM_H
#define FVWM_H

/* ---------------------------- included header files ---------------------- */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h>
#include "libs/PictureBase.h"
#include "libs/Flocale.h"
#include "libs/fvwmrect.h"
#include "libs/FScreen.h"
#include "window_flags.h"
#include "condrc.h"

/* ---------------------------- global definitions ------------------------- */

#ifndef WithdrawnState
#define WithdrawnState 0
#endif

/* ---------------------------- global macros ------------------------------ */

/*
 * Fvwm trivia: There were 97 commands in the fvwm command table
 * when the F_CMD_ARGS macro was written.
 * dje 12/19/98.
 */

/* Macro for args passed to fvwm commands... */
#define F_CMD_ARGS \
	cond_rc_t *cond_rc, const exec_context_t *exc, char *action
#define F_PASS_ARGS cond_rc, exc, action
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

/* ---------------------------- forward declarations ----------------------- */

struct size_borders;
struct exec_context_t;

/* definition in screen.h */
struct FvwmDecor;

/* ---------------------------- type definitions --------------------------- */

/* This structure carries information about the initial window state and
 * placement.  This information is gathered at various places: the (re)capture
 * code, AddToWindow(), HandleMapRequestRaised(), ewmh_events.c and others.
 *
 * initial_state
 *   The initial window state.  By default it carries the value DontCareState.
 *   Other states can be set if
 *    - an icon is recaptured or restarted with session management
 *    - the StartIconic style is set
 *    - GNOME, EWMH, foobar hints demand that the icon starts iconic
 *   The final value is calculated in HandleMapRequestRaised().
 *
 * do_override_ppos
 *   This flag is used in PlaceWindow().  If it is set, the position requested
 *   by the program is ignored unconditionally.  This is used during the
 *   initial capture and later recapture operations.
 *
 * is_iconified_by_parent
 *   Preserves the information if the window is a transient window that was
 *   iconified along with its transientfor window.  Set when the window is
 *   recaptured and used in HandleMapRequestRaised() to set the according
 *   window state flag.  Deleted afterwards.
 *
 * is_menu
 *   Set in menus.c or in the recapture code if the new window is a tear off
 *   menu.  Such windows get special treatment in AddWindow() and events.c.
 *
 * is_recapture
 *   Set for the initial capture and later recaptures.
 *
 * default_icon_x/y
 *   The icon position that was requested by the application in the icon
 *   position hint.  May be overridden by a style (0/0 then).  Set in
 *   HandleMapRequestRaised() and used in the icon placement code.
 *
 * initial_icon_x/y
 *   The icon position that is forced during a restart with SM.  If set it
 *   overrides all other methods of icon placement.  Set by session.c and used
 *   in the icon placement code.
 *
 * use_initial_icon_xy
 *   If set, the initial_icon_x/y values are used.  Other wise they are
 *   ignored.
 */
typedef struct
{
	long initial_state;
	struct
	{
		unsigned do_override_ppos : 1;
		unsigned is_iconified_by_parent : 1;
		unsigned is_menu : 1;
		unsigned is_recapture : 1;
		unsigned use_initial_icon_xy : 1;
	} flags;
	int initial_icon_x;
	int initial_icon_y;
	int default_icon_x;
	int default_icon_y;
} initial_window_options_t;

/*
  For 1 style statement, there can be any number of IconBoxes.
  The name list points at the first one in the chain.
*/
typedef struct icon_boxes_struct
{
	struct icon_boxes_struct *next;       /* next icon_boxes or zero */
	unsigned int use_count;
	int IconBox[4];                       /* x/y x/y for iconbox */
	char	*IconScreen;                       /* Xinerama screen */
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
	unsigned is_sticky_across_pages : 1;
	unsigned is_sticky_across_desks : 1;
	unsigned has_icon_font : 1;
	unsigned has_no_border : 1;
	unsigned has_window_font : 1;
	unsigned title_dir : 2;
	unsigned user_states : 32;
	/* static flags that do not change dynamically after the window has
	 * been created */
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
		unsigned do_ewmh_ignore_window_type : 1;
		unsigned do_iconify_window_groups : 1;
		unsigned do_ignore_icon_boxes : 1;
		unsigned do_ignore_restack : 1;
		unsigned do_use_window_group_hint : 1;
		unsigned do_lower_transient : 1;
		unsigned do_not_show_on_map : 1;
		unsigned do_raise_transient : 1;
		unsigned do_resize_opaque : 1;
		unsigned do_shrink_windowshade : 1;
		unsigned do_stack_transient_parent : 1;
		unsigned do_window_list_skip : 1;
		unsigned ewmh_maximize_mode : 2; /* see ewmh.h */
		unsigned has_depressable_border : 1;
		unsigned has_mwm_border : 1;
		unsigned has_mwm_buttons : 1;
		unsigned has_mwm_override : 1;
		unsigned has_no_icon_title : 1;
		unsigned has_override_size : 1;
		unsigned has_stippled_title : 1;
		unsigned has_stippled_icon_title : 1;
		/* default has to be 0, therefore no_, not in macros */
		unsigned has_no_sticky_stippled_title : 1;
		unsigned has_no_sticky_stippled_icon_title : 1;
		unsigned icon_override : 2;
#define NO_ACTIVE_ICON_OVERRIDE 0
#define ICON_OVERRIDE           1
#define NO_ICON_OVERRIDE        2
#define ICON_OVERRIDE_MASK    0x3
		unsigned is_bottom_title_rotated : 1;
		unsigned is_fixed : 1;
		unsigned is_fixed_ppos : 1;
		unsigned is_uniconifiable : 1;
		unsigned is_unmaximizable : 1;
		unsigned is_unclosable : 1;
		unsigned is_maximize_fixed_size_disallowed : 1;
		unsigned is_icon_sticky_across_pages : 1;
		unsigned is_icon_sticky_across_desks : 1;
		unsigned is_icon_suppressed : 1;
		unsigned is_left_title_rotated_cw : 1; /* cw = clock wise */
		unsigned is_lenient : 1;
		unsigned is_size_fixed : 1;
		unsigned is_psize_fixed : 1;
		unsigned is_right_title_rotated_cw : 1;
		unsigned is_top_title_rotated : 1;
		unsigned use_icon_position_hint : 1;
		unsigned use_indexed_window_name : 1;
		unsigned use_indexed_icon_name : 1;
#define WINDOWSHADE_LAZY          0
#define WINDOWSHADE_ALWAYS_LAZY   1
#define WINDOWSHADE_BUSY          2
#define WINDOWSHADE_LAZY_MASK   0x3
		unsigned windowshade_laziness : 2;
		unsigned use_title_decor_rotation : 1;
		focus_policy_t focus_policy;
	} s;
} common_flags_t;

typedef struct
{
	common_flags_t common;
#define CR_MOTION_METHOD_AUTO        0
#define CR_MOTION_METHOD_USE_GRAV    1
#define CR_MOTION_METHOD_STATIC_GRAV 2
#define CR_MOTION_METHOD_MASK      0x3
	unsigned cr_motion_method : 2;
	unsigned was_cr_motion_method_detected : 1;
	unsigned does_wm_delete_window : 1;
	unsigned does_wm_take_focus : 1;
	unsigned do_force_next_cr : 1;
	unsigned do_force_next_pn : 1;
	unsigned do_iconify_after_map : 1;
	unsigned do_disable_constrain_size_fullscreen : 1;
	/* Reuse this struct, don't free it, when destroying/recapturing
	 * window. */
	unsigned do_reuse_destroyed : 1;
	/* Does it have resize handles? */
	unsigned has_handles : 1;
	/* Icon change is pending */
	unsigned has_icon_changed : 1;
	/* Is this decorated with title */
	unsigned has_title : 1;
	/* wm_normal_hints update is pending? */
	unsigned has_new_wm_normal_hints : 1;
	/* ChangeDecor was used for window */
	unsigned is_decor_changed : 1;
	/* Sent an XUnmapWindow for iconifying, but didn't receive an
	 * UnmapNotify yet.*/
	unsigned is_iconify_pending : 1;
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
	/* fvwm places the window itself */
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
	unsigned used_title_dir_for_shading : 1;
	unsigned shaded_dir : 3;
	unsigned using_default_icon_font : 1;
	unsigned using_default_window_font : 1;
#define ICON_HINT_NEVER    0
#define ICON_HINT_ONCE     1
#define ICON_HINT_MULTIPLE 2
	unsigned was_icon_hint_provided : 2;
	unsigned was_icon_name_provided : 1;
	unsigned was_never_drawn : 1;
	unsigned has_ewmh_wm_name : 1;
	unsigned has_ewmh_wm_icon_name : 1;
#define EWMH_NO_ICON     0 /* the application does not provide an ewmh icon */
#define EWMH_TRUE_ICON   1 /* the application does provide an ewmh icon */
#define EWMH_FVWM_ICON   2 /* the ewmh icon has been set by fvwm */
	unsigned has_ewmh_wm_icon_hint : 2;
	/* says if the app have an ewmh icon of acceptable size for a mini
	 * icon in its list of icons */
	unsigned has_ewmh_mini_icon : 1;
	unsigned has_ewmh_wm_pid : 1;
	/* the ewmh icon is used as icon pixmap */
	unsigned use_ewmh_icon : 1;
	unsigned is_ewmh_modal : 1;
	unsigned is_ewmh_fullscreen : 1;
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
} window_flags;


/* Actions allowed by modules. */
typedef struct action_flags
{
	unsigned is_movable : 1;
	unsigned is_deletable : 1;
	unsigned is_destroyable : 1;
	unsigned is_closable : 1;
	unsigned is_maximizable : 1;
	unsigned is_resizable : 1;
	unsigned is_iconifiable : 1;
} action_flags;


/* Window name data structure for window conditions: a list of lists
   of names to match, the boolean operation on the matches being an
   AND of ORs. */
struct namelist			/* matches to names in this list are ORed */
{
	char *name;
	struct namelist *next;
};

struct name_condition		/* matches to namelists in this list are
				   ANDed, after possibly inverting each */
{
	Bool invert;
	struct namelist *namelist;
	struct name_condition *next;
};

/* Window mask for Circulate and Direction functions */
typedef struct WindowConditionMask
{
	struct
	{
		unsigned do_accept_focus : 1;
		unsigned do_check_desk : 1;
		unsigned do_check_screen : 1;
		unsigned do_check_cond_desk : 1;
		unsigned do_check_desk_and_global_page : 1;
		unsigned do_check_desk_and_page : 1;
		unsigned do_check_global_page : 1;
		unsigned do_check_overlapped : 1;
		unsigned do_check_page : 1;
		unsigned do_not_check_screen : 1;
		unsigned needs_current_desk : 1;
		unsigned needs_current_desk_and_global_page : 1;
		unsigned needs_current_desk_and_page : 1;
		unsigned needs_current_global_page : 1;
		unsigned needs_current_page : 1;
#define NEEDS_ANY   0
#define NEEDS_TRUE  1
#define NEEDS_FALSE 2
		unsigned needs_focus : 2;
		unsigned needs_overlapped : 2;
		unsigned needs_pointer : 2;
		unsigned needs_same_layer : 1;
		unsigned use_circulate_hit : 1;
		unsigned use_circulate_hit_icon : 1;
		unsigned use_circulate_hit_shaded : 1;
		unsigned use_do_accept_focus : 1;
	} my_flags;
	window_flags flags;
	window_flags flag_mask;
	struct name_condition *name_condition;
	int layer;
	int desk;
	struct monitor *screen;
	int placed_by_button_mask;
	int placed_by_button_set_mask;
} WindowConditionMask;

typedef struct pl_penalty_struct
{
	float normal;
	float ontop;
	float icon;
	float sticky;
	float below;
	float strut;
} pl_penalty_struct;

typedef struct pl_percent_penalty_struct
{
	int p99;
	int p95;
	int p85;
	int p75;
} pl_percent_penalty_struct;

/* only style.c and add_window.c are allowed to access this struct! */
typedef struct style_flags
{
	common_flags_t common;
	unsigned do_decorate_transient : 1;
	/* old placement flags */
#define PLACE_DUMB            0x0
#define PLACE_SMART           0x1
#define PLACE_CLEVER          0x2
#define PLACE_CLEVERNESS_MASK 0x3
#define PLACE_RANDOM          0x4
	/* new placements value, try to get a minimal backward compatibility
	 * with the old flags:
	 * Dumb+Active = Manual,
	 * Dumb+Random = Cascade,
	 * Smart+Random = TileCascade,
	 * Smart+Active = TileManual,
	 * Random+Smart+Clever = MINOVERLAP which is the original Clever
	 * placement code,
	 * Active+Smart+Clever = MINOVERLAPPERCENT which is the "new" Clever
	 * placement code and was the original Clever placement code. Now the
	 * original placement code said:
	 * Active/Random+Dumb+Clever = Active/Random+Dumb (with Dumb Clever is
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
#define PLACE_POSITION          0x8
#define PLACE_MASK              0xF
	unsigned placement_mode : 4;
	unsigned ewmh_placement_mode : 2; /* see ewmh.h */
#define WS_CR_MOTION_METHOD_AUTO CR_MOTION_METHOD_AUTO
#define WS_CR_MOTION_METHOD_USE_GRAV CR_MOTION_METHOD_USE_GRAV
#define WS_CR_MOTION_METHOD_STATIC_GRAV CR_MOTION_METHOD_STATIC_GRAV
#define WS_CR_MOTION_METHOD_MASK CR_MOTION_METHOD_MASK
	unsigned ws_cr_motion_method : 2;
	unsigned do_save_under : 1;
	unsigned do_start_iconic : 1;
	unsigned do_start_lowered : 1;
 	unsigned do_start_shaded : 1;
 	unsigned start_shaded_dir : 3;
	unsigned has_border_width : 1;
	unsigned has_color_back : 1;
	unsigned has_color_fore : 1;
	unsigned has_color_back_hi : 1;
	unsigned has_color_fore_hi : 1;
	unsigned has_decor : 1;
	unsigned has_edge_delay_ms_move : 1;
	unsigned has_edge_delay_ms_resize : 1;
	unsigned has_edge_resistance_move : 1;
	unsigned has_edge_resistance_xinerama_move : 1;
	unsigned has_handle_width : 1;
	unsigned has_icon : 1;
	unsigned has_icon_boxes : 1;
	unsigned has_icon_size_limits : 1;
	unsigned has_min_window_size : 1;
	unsigned has_max_window_size : 1;
	unsigned has_icon_background_padding : 1;
	unsigned has_icon_background_relief : 1;
	unsigned has_icon_title_relief : 1;
	unsigned has_window_shade_steps : 1;
	unsigned has_mini_icon : 1;
	unsigned has_mwm_decor : 1;
	unsigned has_mwm_functions : 1;
	unsigned has_no_handles : 1;
	unsigned has_no_title : 1;
	unsigned has_ol_decor : 1;
	unsigned has_snap_grid : 1;
	unsigned has_snap_attraction : 1;
#if 0
	unsigned has_condition_mask : 1;
#endif
	unsigned is_button_disabled : NUMBER_OF_TITLE_BUTTONS;
	unsigned is_unmanaged : 1;
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
	unsigned use_icon_title_colorset : 1;
	unsigned use_icon_title_colorset_hi : 1;
	unsigned use_icon_background_colorset : 1;
	unsigned use_layer : 1;
	unsigned use_no_pposition : 1;
	unsigned use_no_usposition : 1;
	unsigned use_no_transient_pposition : 1;
	unsigned use_no_transient_usposition : 1;
	unsigned use_start_on_desk : 1;
	unsigned use_start_on_page_for_transient : 1;
	unsigned use_start_on_screen : 1;
	unsigned manual_placement_honors_starts_on_page : 1;
	unsigned um_placement_honors_starts_on_page : 1;
	unsigned capture_honors_starts_on_page : 1;
	unsigned recapture_honors_starts_on_page : 1;
	unsigned has_placement_penalty : 1;
	unsigned has_placement_percentage_penalty : 1;
	unsigned has_placement_position_string : 1;
	unsigned has_initial_map_command_string : 1;
	unsigned has_title_format_string : 1;
	unsigned has_icon_title_format_string : 1;
} style_flags;

typedef struct style_id_t
{
	char *name;
	XID window_id;
	struct
	{
		unsigned has_name:1;
		unsigned has_window_id:1;
	} flags;
} style_id_t;

typedef struct snap_attraction_t
{
	/* attractiveness of window edges */
	int proximity;
	/* mode of snap attraction */
	int mode;
	/* mode flags to do bit manipulation */
	enum
	{
		SNAP_NONE = 0x00,
		SNAP_WINDOWS = 0x01,
		SNAP_ICONS = 0x02,
		SNAP_SAME = 0x04,
		SNAP_SCREEN = 0x08,
		SNAP_SCREEN_WINDOWS = 0x10,
		SNAP_SCREEN_ICONS = 0x20,
		SNAP_SCREEN_ALL = 0x40,
	} types;
} snap_attraction_t;

/* only style.c and add_window.c are allowed to access this struct! */
typedef struct window_style
{
	struct window_style *next;
	struct window_style *prev;
	style_id_t id;
#if 0
	WindowConditionMask *condition_mask;
#endif
	char *icon_name;
	char *mini_icon_name;
	char *decor_name;
	unsigned char min_icon_width;
	unsigned char max_icon_width;
	unsigned char min_icon_height;
	unsigned char max_icon_height;
#define ICON_RESIZE_TYPE_NONE      0x0
#define ICON_RESIZE_TYPE_STRETCHED 0x1
#define ICON_RESIZE_TYPE_ADJUSTED  0x2
#define ICON_RESIZE_TYPE_SHRUNK    0x3
#define ICON_RESIZE_TYPE_MASK      0x3
	unsigned icon_resize_type : 2;
	unsigned char icon_background_padding;
	signed char icon_background_relief;
	signed char icon_title_relief;
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
	int icon_title_colorset;
	int icon_title_colorset_hi;
	int icon_background_colorset;
	short border_width;
	/* resize handle width */
	short handle_width;
	int layer;
	int start_desk;
	int start_page_x;
	int start_page_y;
	char *start_screen;
	int min_window_width;
	int min_window_height;
	int max_window_width;
	int max_window_height;
	int shade_anim_steps;
#if 1 /*!!!*/
	snap_attraction_t snap_attraction;
	/* snap grid size */
	int snap_grid_x;
	int snap_grid_y;
	int edge_delay_ms_move;
	int edge_delay_ms_resize;
	int edge_resistance_move;
	int edge_resistance_xinerama_move;
#endif
	icon_boxes *icon_boxes;
	float norm_placement_penalty;
	pl_penalty_struct pl_penalty;
	pl_percent_penalty_struct pl_percent_penalty;
	char *pl_position_string;
	char *initial_map_command_string;
	char *title_format_string;
	char *icon_title_format_string;
	style_flags flags;
	style_flags flag_default;
	style_flags flag_mask;
	style_flags change_mask;
	unsigned has_style_changed : 1;
	unsigned has_title_format_string : 1;
	unsigned has_icon_title_format_string : 1;
} window_style;

typedef struct window_g
{
	rectangle frame;
	/* absolute geometry when not maximized */
	rectangle normal;
	/* maximized window geometry */
	rectangle max;
	/* defect between maximized geometry before and after
	 * constraining size. */
	size_rect max_defect;
	/* original delta between normalized and maximized window,
	 * used to keep unmaximized window at same screen position */
	position max_offset;
} window_g;

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
	/* if non-null: Use this instead of any other names for matching
	   styles */
	char *style_name;
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
		Window button_w[NUMBER_OF_TITLE_BUTTONS];
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
		unsigned buttons_drawn : NUMBER_OF_TITLE_BUTTONS;
		unsigned buttons_lit : NUMBER_OF_TITLE_BUTTONS;
		unsigned buttons_inverted : NUMBER_OF_TITLE_BUTTONS;
		unsigned buttons_toggled : NUMBER_OF_TITLE_BUTTONS;
		unsigned parts_drawn : 12;
		unsigned parts_lit : 12;
		unsigned parts_inverted : 12;
	} decor_state;
	int nr_left_buttons;
	int nr_right_buttons;
#define BUTTON_INDEX(b) \
	(((b) == 0) ? (NUMBER_OF_TITLE_BUTTONS - 1) : ((b) - 1))
	struct FvwmDecor *decor;
	/* is this a shaped window */
	int wShaped;
	Pixmap title_background_pixmap;

	/* Note: if the type of this variable is changed, do update the
	 * CONFIGARGSNEW macro in module_interface.c, libs/vpacket.h too! */
	short boundary_width;
	short unshaped_boundary_width;
	short corner_width;
	short visual_corner_width;

	/* title font */
	FlocaleFont *title_font;
	/* /Y coordinate to draw the title name */
	short title_text_offset;
	short title_length;
	/* Note: if the type of this variable is changed, do update the
	 * CONFIGARGSNEW macro in module_interface.c, libs/vpacket.h and too!
	 */
	short title_thickness;
	rotation_t title_text_rotation;
	struct
	{
		/* geometry of the icon picture window */
		rectangle picture_w_g;
		/* geometry of the icon title window */
		rectangle title_w_g;
		/* width of the text in the icon title */
		int title_text_width;
	} icon_g;
	short icon_border_width;

	/* Drawable depth for the icon */
	int iconDepth;
	/* pixmap for the icon */
	Pixmap iconPixmap;
	/* pixmap for the icon mask */
	Pixmap icon_maskPixmap;
	Pixmap icon_alphaPixmap;
	int icon_nalloc_pixels;
	Pixel *icon_alloc_pixels;
	int icon_no_limit;
	FlocaleFont *icon_font;

	/* some parts of the window attributes */
	struct
	{
		int backing_store;
		int border_width;
		int depth;
		int bit_gravity;
		unsigned is_bit_gravity_stored : 1;
		Visual *visual;
		Colormap colormap;
	} attr_backup;
	/* normal hints */
	XSizeHints hints;
	struct
	{
		int width_inc;
		int height_inc;
	} orig_hints;
	/* WM hints */
	XWMHints *wmhints;
	XClassHint class;
	/* Tells which desktop this window is on */
	/* Note: if the type of this variable is changed, do update the
	 * CONFIGARGSNEW macro in module_interface.c, libs/vpacket.h and too!
	 */
	int Desk;
	/* Where (if at all) was it focused */
	int FocusDesk;
	/* Desk to deiconify to, for StubbornIcons */
	int DeIconifyDesk;

	char *mini_pixmap_file;
	FvwmPicture *mini_icon;
	char *icon_bitmap_file;

	struct window_g g;
	long *mwm_hints;
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

	int cs;
	int cs_hi;
	int border_cs;
	int border_cs_hi;
	int icon_title_cs;
	int icon_title_cs_hi;
	int icon_background_cs;

	unsigned long buttons;
	/* zero or more iconboxes */
	icon_boxes *IconBoxes;

	int default_layer;
	/* Note: if the type of this variable is changed, do update the
	 * CONFIGARGSNEW macro in module_interface.c, libs/vpacket.h and too!
	 */
	int layer;

	unsigned char min_icon_width;
	unsigned char max_icon_width;
	unsigned char min_icon_height;
	unsigned char max_icon_height;
	unsigned short icon_resize_type;

	unsigned char icon_background_padding;
	char icon_background_relief;
	char icon_title_relief;

	int min_window_width;
	int min_window_height;
	int max_window_width;
	int max_window_height;
	int shade_anim_steps;
	unsigned char grabbed_buttons;
#if 1 /*!!!*/
	snap_attraction_t snap_attraction;
	/* snap grid size */
	int snap_grid_x;
	int snap_grid_y;
	int edge_delay_ms_move;
	int edge_delay_ms_resize;
	int edge_resistance_move;
	int edge_resistance_xinerama_move;
#endif

#define FM_NO_INPUT        0
#define FM_PASSIVE         1
#define FM_LOCALLY_ACTIVE  2
#define FM_GLOBALLY_ACTIVE 3
	unsigned char focus_model;

	pl_penalty_struct pl_penalty;
	pl_percent_penalty_struct pl_percent_penalty;

	unsigned char placed_by_button;

#define EWMH_WINDOW_TYPE_NONE_ID      0
#define EWMH_WINDOW_TYPE_DESKTOP_ID   1
#define EWMH_WINDOW_TYPE_DIALOG_ID    2
#define EWMH_WINDOW_TYPE_DOCK_ID      3
#define EWMH_WINDOW_TYPE_MENU_ID      4
#define EWMH_WINDOW_TYPE_NORMAL_ID    5
#define EWMH_WINDOW_TYPE_TOOLBAR_ID   6
#define EWMH_WINDOW_TYPE_NOTIFICATION_ID 7
	/* Note: if the type of this variable is changed, do update the
	 * CONFIGARGSNEW macro in module_interface.c, libs/vpacket.h and too!
	 */
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
	/* Note: if the type of this variable is changed, do update the
	 * CONFIGARGSNEW macro in module_interface.c, libs/vpacket.h and too!
	 */
	int ewmh_hint_layer;
	int ewmh_normal_layer; /* for restoring non ewmh layer */
	/* memory for the initial _NET_WM_STATE */
	unsigned long ewmh_hint_desktop;

	/* For the purposes of restoring attributes before/after a window goes
	 * into fullscreen.
	 */
	struct
	{
		struct window_g g;
		int is_iconified;
		int is_shaded;
		int was_maximized;
	} fullscreen;

	/* multi purpose scratch structure */
	struct
	{
		void *p;
		int i;
	} scratch;

	struct monitor *m;
} FvwmWindow;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void SetMWM_INFO(Window window);
void fvmm_deinstall_signals(void);

#endif /* FVWM_H */
