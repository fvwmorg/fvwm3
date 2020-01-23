/* -*-c-*- */
/* This module is based on Twm, but has been siginificantly modified
 * by Rob Nation
 */
/* Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 *
 * fvwm per-screen data include file
 *
 */

#ifndef _SCREEN_
#define _SCREEN_

#include "libs/flist.h"
#include "libs/Bindings.h"

#define SIZE_HINDENT 5
#define SIZE_VINDENT 3

/* colormap focus styes */
#define COLORMAP_FOLLOWS_MOUSE 1 /* default */
#define COLORMAP_FOLLOWS_FOCUS 2

/* title bar multi pixmap parts */
/* those which can be used as UseTitleStyle should be enum first */
typedef enum {
	TBMP_NONE  = -1,
	TBMP_MAIN,
	TBMP_LEFT_MAIN,
	TBMP_RIGHT_MAIN,
	TBMP_LEFT_BUTTONS,
	TBMP_RIGHT_BUTTONS,
	TBMP_UNDER_TEXT,
	TBMP_LEFT_OF_TEXT,
	TBMP_RIGHT_OF_TEXT,
	TBMP_LEFT_END,
	TBMP_RIGHT_END,
	TBMP_BUTTONS,
	TBMP_NUM_PIXMAPS
} TbmpParts;

/* title bar multi pixmap parts which can be use for UseTitleStyle */
typedef enum {
	UTS_TBMP_NONE  = -1,
	UTS_TBMP_MAIN,
	UTS_TBMP_LEFT_MAIN,
	UTS_TBMP_RIGHT_MAIN,
	UTS_TBMP_LEFT_BUTTONS,
	UTS_TBMP_RIGHT_BUTTONS,
	UTS_TBMP_NUM_PIXMAPS
} UtsTbmpParts;

typedef struct
{
	int cs;
	int alpha_percent;
} FvwmAcs;

typedef enum
{
	/* button types */
	DefaultVectorButton,
	VectorButton,
	SimpleButton,
	GradientButton,
	PixmapButton,
	TiledPixmapButton,
	StretchedPixmapButton,
	AdjustedPixmapButton,
	ShrunkPixmapButton,
	MultiPixmap,
	MiniIconButton,
	SolidButton,
	ColorsetButton
} DecorFaceType;

typedef enum
{
	JUST_CENTER = 0,
	JUST_LEFT = 1,
	JUST_TOP = 1,
	JUST_RIGHT = 2,
	JUST_BOTTOM = 2,
	JUST_MASK = 3
} JustificationType;

typedef struct
{
	unsigned face_type : 4;
	struct
	{
		unsigned h_justification : 2;
		unsigned v_justification : 2;
#define DFS_BUTTON_IS_UP   0
#define DFS_BUTTON_IS_FLAT 1
#define DFS_BUTTON_IS_SUNK 2
#define DFS_BUTTON_MASK    3
		unsigned int button_relief : 2;
		/* not used in border styles */
		unsigned int use_title_style : 1;
		unsigned int use_border_style : 1;
		/* only used in border styles */
		unsigned int has_hidden_handles : 1;
		unsigned int has_no_inset : 1;
	} flags;
} DecorFaceStyle;

#define DFS_FACE_TYPE(dfs)          ((dfs).face_type)
#define DFS_FLAGS(dfs)              ((dfs).flags)
#define DFS_H_JUSTIFICATION(dfs)    ((dfs).flags.h_justification)
#define DFS_V_JUSTIFICATION(dfs)    ((dfs).flags.v_justification)
#define DFS_BUTTON_RELIEF(dfs)      ((dfs).flags.button_relief)
#define DFS_USE_TITLE_STYLE(dfs)    ((dfs).flags.use_title_style)
#define DFS_USE_BORDER_STYLE(dfs)   ((dfs).flags.use_border_style)
#define DFS_HAS_HIDDEN_HANDLES(dfs) ((dfs).flags.has_hidden_handles)
#define DFS_HAS_NO_INSET(dfs)       ((dfs).flags.has_no_inset)

typedef struct DecorFace
{
	DecorFaceStyle style;
	struct
	{
		FvwmPicture *p;
		struct {
			FvwmPicture **pixmaps;
			unsigned short stretch_flags;
			FvwmAcs *acs;
			Pixel *pixels;
			unsigned short solid_flags;
		} mp;
		struct
		{
			int cs;
			int alpha_percent;
		} acs;
		Pixel back;
		struct
		{
			int npixels;
			XColor *xcs;
			int do_dither;
			Pixel *d_pixels;
			int d_npixels;
			char gradient_type;
		} grad;
		struct vector_coords
		{
			int num;
			signed char *x;
			signed char *y;
		        signed char *xoff;
		        signed char *yoff;
			signed char *c;
			unsigned use_fgbg : 1;
		} vector;
	} u;

	struct DecorFace *next;

	struct
	{
		unsigned has_changed : 1;
	} flags;
} DecorFace;

typedef enum
{
	BS_All = -1,
	BS_ActiveUp,
	BS_ActiveDown,
	BS_InactiveUp,
	BS_InactiveDown,
	BS_ToggledActiveUp,
	BS_ToggledActiveDown,
	BS_ToggledInactiveUp,
	BS_ToggledInactiveDown,
	BS_MaxButtonState,
	BS_MaxButtonStateMask = BS_MaxButtonState - 1,
	BS_Active,
	BS_Inactive,
	BS_ToggledActive,
	BS_ToggledInactive,
	BS_AllNormal,
	BS_AllToggled,
	BS_AllActive,
	BS_AllInactive,
	BS_AllUp,
	BS_AllDown,
	BS_AllActiveUp,
	BS_AllActiveDown,
	BS_AllInactiveUp,
	BS_AllInactiveDown,
	BS_MaxButtonStateName
} ButtonState;

#define BS_MASK_DOWN     (1 << 0)
#define BS_MASK_INACTIVE (1 << 1)
#define BS_MASK_TOGGLED  (1 << 2)

typedef enum
{
	/* The first five are used in title buttons.  These can't be
	 * renumbered without extending the mwm_decor_flags member below and
	 * adapting the style structure. */
	MWM_DECOR_MENU     = 0x1,
	MWM_DECOR_MINIMIZE = 0x2,
	MWM_DECOR_MAXIMIZE = 0x4,
	MWM_DECOR_SHADE    = 0x8,
	MWM_DECOR_STICK    = 0x10,
	/* --- */
	MWM_DECOR_BORDER   = 0x20,
	MWM_DECOR_RESIZEH  = 0x40,
	MWM_DECOR_TITLE    = 0x80,
	MWM_DECOR_ALL      = 0x100,
	MWM_DECOR_EVERYTHING = 0xff
} mwm_flags;

typedef struct
{
	unsigned just : 2; /* was JustificationType : 2 */
	int layer;
	struct
	{
		unsigned has_changed : 1;
		mwm_flags mwm_decor_flags : 9;
		/* Support {ButtonStyle <number> - Layer 4} construction, so
		 * button can be rendered 'pressed in' when the window is
		 * assigned to a particular layer. */
		unsigned has_layer : 1;
	} flags;
	DecorFace state[BS_MaxButtonState];
} TitleButton;

#define TB_FLAGS(tb)              ((tb).flags)
#define TB_STATE(tb)              ((tb).state)
#define TB_JUSTIFICATION(tb)      ((tb).just)
#define TB_LAYER(tb)              ((tb).layer)
#define TB_MWM_DECOR_FLAGS(tb)    ((tb).flags.mwm_decor_flags)
#define TB_HAS_CHANGED(tb)     \
  (!!((tb).flags.has_changed))
#define TB_HAS_MWM_DECOR_MENU(tb)     \
  (!!((tb).flags.mwm_decor_flags & MWM_DECOR_MENU))
#define TB_HAS_MWM_DECOR_MINIMIZE(tb) \
  (!!((tb).flags.mwm_decor_flags & MWM_DECOR_MINIMIZE))
#define TB_HAS_MWM_DECOR_MAXIMIZE(tb) \
  (!!((tb).flags.mwm_decor_flags & MWM_DECOR_MAXIMIZE))
#define TB_HAS_MWM_DECOR_SHADE(tb)    \
  (!!((tb).flags.mwm_decor_flags & MWM_DECOR_SHADE))
#define TB_HAS_MWM_DECOR_STICK(tb)    \
  (!!((tb).flags.mwm_decor_flags & MWM_DECOR_STICK))

typedef struct FvwmDecor
{
	char *tag;                    /* general style tag */
	int title_height;           /* explicitly specified title bar height */
	int min_title_height;
	/* titlebar buttons */
	TitleButton buttons[NUMBER_OF_TITLE_BUTTONS];
	TitleButton titlebar;
	struct BorderStyle
	{
		DecorFace active, inactive;
	} BorderStyle;
	struct FvwmDecor *next;       /* additional user-defined styles */
	struct
	{
		unsigned has_changed : 1;
		unsigned has_title_height_changed : 1;
	} flags;
} FvwmDecor;

typedef struct ScreenInfo
{
	unsigned long screen;
	Screen *pscreen;
	/* number of screens on display */
	int NumberOfScreens;
	/* the head of the fvwm window list */
	FvwmWindow FvwmRoot;
	/* the root window */
	Window Root;
	/* the resize dimensions window */
	Window SizeWindow;
	/* Window which will own focus when no other windows have it */
	Window NoFocusWin;

	flist *FWScheduledForDestroy;

	/*dark gray pattern for shaded out menu items*/
	Pixmap gray_bitmap;
	/* dark gray pattern for inactive borders */
	Pixmap gray_pixmap;
	/* light gray pattern for inactive borders */
	Pixmap light_gray_pixmap;
	/* light gray pattern for sticky borders */
	Pixmap sticky_gray_pixmap;

	Binding *AllBindings;

	/* current push level to install root colormap windows */
	int root_pushes;
	/* current push level to install fvwm colormap windows */
	int fvwm_pushes;
	/* saved window to install when pushes drops to zero */
	const FvwmWindow *pushed_window;
	Cursor *FvwmCursors;
	/* context where we display the busy cursor */
	int BusyCursor;
	/* Icon to use when no other icons are found */
	char *DefaultIcon;

	int TopLayer;
	int DefaultLayer;
	int BottomLayer;

	struct FvwmFunction *functions;

	/* font structure */
	FlocaleFont *DefaultFont;

	/* GC for transparency masks */
	GC TransMaskGC;
	/* don't change the order */
	Pixel StdFore, StdBack, StdHilite, StdShadow;
	GC StdGC;
	GC StdReliefGC;
	GC StdShadowGC;

	/* A scratch 1x1x1 pixmap */
	Pixmap ScratchMonoPixmap;
	/* GC for drawing into depth 1 drawables */
	GC MonoGC;
	/* A scratch 1x1xalpha_depth pixmap */
	Pixmap ScratchAlphaPixmap;
	/* GC for drawing into depth alpha_depth drawables */
	GC AlphaGC;


	/* GC to draw lines for move and resize */
	GC XorGC;
	GC ScratchGC1;
	GC ScratchGC2;
	GC ScratchGC3;
	GC ScratchGC4;
	GC TitleGC;
	GC BordersGC;
	/* minimum width of size window */
	int SizeStringWidth;

	/* decoration style(s) */
	FvwmDecor DefaultDecor;
	FvwmDecor *cur_decor;

	/* number of left-side title-bar buttons */
	int nr_left_buttons;
	/* number of right-side title-bar buttons */
	int nr_right_buttons;

	/* the fvwm window that is highlighted except for networking delays,
	 * this is the window which REALLY has the focus */
	FvwmWindow *Hilite;
	/* None, if the focus is nowhere or on an fvwm managed window. Set to
	 * id of otherwindow with focus otherwise */
	Window UnknownWinFocused;
	/* The window that the UnknownWinFocused window stole the focus from.
	 */
	Window StolenFocusWin;
	FvwmWindow *StolenFocusFvwmWin;
	FvwmWindow *focus_in_pending_window;
	FvwmWindow *focus_in_requested_window;
	/* buttons to grab in click to focus mode */
	unsigned short buttons2grab;
	int NumBoxes;
	/* values used for CascadePlacement */
	int cascade_x;
	int cascade_y;
	FvwmWindow *cascade_window;
	/*Max button-click delay for Function built-in*/
	int ClickTime;
	/* resistance to scrolling in desktop */
	int ScrollDelay;
	int MoveThreshold;
	int OpaqueSize;
	/* colormap focus style */
	int ColormapFocus;
	/* Limit on colors used in pixmaps */
	int ColorLimit;
	/* Default Colorset used by feedback window */
	int DefaultColorset;

	int use_backing_store;

	/* some additional global options which will probably become window
	 * specific options later on: */
	struct
	{
		unsigned do_debug_cr_motion_method : 1;
		unsigned do_disable_configure_notify : 1;
		unsigned do_display_new_window_names : 1;
		unsigned do_enable_ewmh_iconic_state_workaround : 1;
		unsigned do_enable_flickering_qt_dialogs_workaround : 1;
		unsigned do_enable_qt_drag_n_drop_workaround : 1;
		unsigned do_explain_window_placement : 1;
		unsigned do_install_root_cmap : 1;
		unsigned do_raise_over_unmanaged : 1;
		unsigned is_modality_evil : 1;
		unsigned is_raise_hack_needed : 1;
		unsigned do_debug_randr : 1;
	} bo; /* bug workaround control options */
	struct
	{
		unsigned do_emulate_mwm : 1;
		unsigned do_emulate_win : 1;
		unsigned do_hide_position_window : 1;
		unsigned do_hide_resize_window : 1;
		unsigned use_active_down_buttons : 1;
		unsigned use_inactive_buttons : 1;
		unsigned use_inactive_down_buttons : 1;
	} gs; /* global style structure */
	struct
	{
		unsigned are_functions_silent : 1;
		unsigned are_windows_captured : 1;
		unsigned do_edge_wrap_x : 1;
		unsigned do_edge_wrap_y : 1;
		unsigned do_need_style_list_update : 1;
		unsigned do_need_window_update : 1;
		unsigned do_save_under : 1;
		unsigned has_default_color_changed : 1;
		unsigned has_default_font_changed : 1;
		unsigned has_mouse_binding_changed : 1;
		unsigned has_nr_buttons_changed : 1;
		unsigned has_xinerama_state_changed : 1;
		unsigned is_executing_complex_function : 1;
		unsigned is_executing_menu_function : 1;
		unsigned is_map_desk_in_progress : 1;
		unsigned is_pointer_on_this_screen : 1;
		unsigned is_single_screen : 1;
		unsigned is_window_scheduled_for_destroy : 1;
		unsigned is_wire_frame_displayed : 1;
	} flags;

	/* the window of desktop type if any */
	FvwmWindow *EwmhDesktop;
	struct
	{
		last_added_item_t type;
		void *item;
	} last_added_item;
} ScreenInfo;

#define UPDATE_FVWM_SCREEN(fw) \
	do { \
		rectangle g; \
		struct monitor *mnew; \
		get_unshaded_geometry((fw), &g); \
		mnew = FindScreenOfXY((fw)->g.frame.x, (fw)->g.frame.y); \
		if (mnew == NULL) { \
			fprintf(stderr, "CRAP - WHICH MONITOR?\n"); \
			mnew = monitor_get_current(); \
		} \
		if (mnew != NULL) { \
			if ((fw) && (fw)->m) { \
				(fw)->m->win_count--; \
			} \
			(fw)->m_prev = (fw)->m; \
			(fw)->m = mnew; \
			(fw)->Desk = (fw)->m->virtual_scr.CurrentDesk; \
			(fw)->m->win_count++; \
		} \
	} while(0)

/* A macro to to simplify he "ewmh desktop code" */
#define IS_EWMH_DESKTOP(win) \
	(Scr.EwmhDesktop && win == Scr.EwmhDesktop->wins.client)
#define IS_EWMH_DESKTOP_FW(fwin) \
	(fwin && Scr.EwmhDesktop && Scr.EwmhDesktop == fwin)
/* Macro which gets specific decor or default decor.
 * This saves an indirection in case you don't want
 * the UseDecor mechanism. */
#define GetDecor(window,part) ((window)->decor->part)

/* some protos for the decoration structures */
void LoadDefaultButton(DecorFace *bf, int i);
void ResetAllButtons(FvwmDecor *decor);
void DestroyAllButtons(FvwmDecor *decor);

void simplify_style_list(void);

/*
 * Diverts a style definition to an FvwmDecor structure (veliaa@rpi.edu)
 */
void AddToDecor(F_CMD_ARGS, FvwmDecor *decor);

extern ScreenInfo Scr;

#endif /* _SCREEN_ */
