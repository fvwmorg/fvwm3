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
 * This module is based on Twm, but has been siginificantly modified
 * by Rob Nation
 ****************************************************************************/
/*
 * Copyright 1989 Massachusetts Institute of Technology
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

/***********************************************************************
 *
 * fvwm per-screen data include file
 *
 ***********************************************************************/

#ifndef _SCREEN_
#define _SCREEN_

#define SIZE_HINDENT 5
#define SIZE_VINDENT 3

/* colormap focus styes */
#define COLORMAP_FOLLOWS_MOUSE 1 /* default */
#define COLORMAP_FOLLOWS_FOCUS 2


typedef struct
{
  Window win;
  int isMapped;
} PanFrame;

typedef enum
{
    /* button types */
    VectorButton                ,
    SimpleButton                ,
    GradientButton              ,
    PixmapButton                ,
    TiledPixmapButton           ,
#ifdef MINI_ICONS
    MiniIconButton              ,
#endif
    SolidButton
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
  DecorFaceType face_type : 3;
  struct
  {
    JustificationType h_justification : 2;
    JustificationType v_justification : 2;
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
  union
  {
    Picture *p;
    Pixel back;
    struct
    {
      int npixels;
      Pixel *pixels;
      char gradient_type;
    }
    grad;
    struct vector_coords
    {
      int num;
      int *x;
      int *y;
      unsigned long line_style;
    } vector;
  } u;

#ifdef MULTISTYLE
  struct DecorFace *next;
#endif

  struct
  {
    unsigned has_changed : 1;
  } flags;
} DecorFace;

enum ButtonState
{
  ActiveUp,
  ActiveDown,
  Inactive,
  ToggledActiveUp,
  ToggledActiveDown,
  ToggledInactive,
  MaxButtonState
};

typedef enum
{
  /* The first five are used in title buttons.  These can't be renumbered
   * without extending the mwm_decor_flags member below and adapting the style
   * structure. */
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
  JustificationType just : 2;
  struct
  {
    unsigned has_changed : 1;
    mwm_flags mwm_decor_flags : 9;
  } flags;
  DecorFace state[MaxButtonState];
} TitleButton;

#define TB_FLAGS(tb)              ((tb).flags)
#define TB_STATE(tb)              ((tb).state)
#define TB_JUSTIFICATION(tb)      ((tb).just)
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
#ifdef USEDECOR
  char *tag;			/* general style tag */
#endif
  int title_height;           /* explicitly specified title bar height */
  /* titlebar buttons */
  TitleButton buttons[NUMBER_OF_BUTTONS];
  TitleButton titlebar;
  struct BorderStyle
  {
    DecorFace active, inactive;
  } BorderStyle;
#ifdef USEDECOR
  struct FvwmDecor *next;	/* additional user-defined styles */
#endif
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
  int NumberOfScreens;          /* number of screens on display */
  int MyDisplayWidth;		/* my copy of DisplayWidth(dpy, screen) */
  int MyDisplayHeight;	        /* my copy of DisplayHeight(dpy, screen) */

  FvwmWindow FvwmRoot;		/* the head of the fvwm window list */
  Window Root;		        /* the root window */
  Window SizeWindow;		/* the resize dimensions window */
  Window NoFocusWin;            /* Window which will own focus when no other
				 * windows have it */
  PanFrame PanFrameTop;
  PanFrame PanFrameLeft;
  PanFrame PanFrameRight;
  PanFrame PanFrameBottom;

  Pixmap gray_bitmap;           /*dark gray pattern for shaded out menu items*/
  Pixmap gray_pixmap;           /* dark gray pattern for inactive borders */
  Pixmap light_gray_pixmap;     /* light gray pattern for inactive borders */
  Pixmap sticky_gray_pixmap;     /* light gray pattern for sticky borders */

  Binding *AllBindings;

  int root_pushes;		/* current push level to install root
				   colormap windows */
  int fvwm_pushes;		/* current push level to install fvwm
				   colormap windows */
  FvwmWindow *pushed_window;	/* saved window to install when pushes drops
				   to zero */
  Cursor *FvwmCursors;

  int BusyCursor;               /* context where we display the busy cursor */

  char *DefaultIcon;            /* Icon to use when no other icons are found */

  int TopLayer;
  int DefaultLayer;
  int BottomLayer;

  FvwmFunction *functions;

  FvwmFont DefaultFont;     	/* font structure */

  GC TransMaskGC;               /* GC for transparency masks */
  Pixel StdFore, StdBack, StdHilite, StdShadow; /* don't change the order */
  GC StdGC;
  GC StdReliefGC;
  GC StdShadowGC;

  Pixmap ScratchMonoPixmap;     /* A scratch 1x1x1 pixmap */
  GC MonoGC;                    /* GC for drawing into depth 1 drawables */

  GC XorGC;			/* GC to draw lines for move and resize */
  GC ScratchGC1;
  GC ScratchGC2;
  GC TitleGC;
  int SizeStringWidth;	        /* minimum width of size window */
  int BoundaryWidth;	        /* frame width for decorated windows */
  int NoBoundaryWidth;	        /* frame width for decorated windows */

  FvwmDecor DefaultDecor;	/* decoration style(s) */

  int nr_left_buttons;		/* number of left-side title-bar buttons */
  int nr_right_buttons;		/* number of right-side title-bar buttons */

  FvwmWindow *Hilite;		/* the fvwm window that is highlighted
				 * except for networking delays, this is the
				 * window which REALLY has the focus */
  FvwmWindow *Focus;            /* Last window which Fvwm gave the focus to
                                 * NOT the window that really has the focus */
  Window UnknownWinFocused;      /* None, if the focus is nowhere or on an fvwm
				 * managed window. Set to id of otherwindow
				 * with focus otherwise */
  FvwmWindow *Ungrabbed;
  FvwmWindow *PreviousFocus;    /* Window which had focus before fvwm stole it
				 * to do moves/menus/etc. */
  int EdgeScrollX;              /* #pixels to scroll on screen edge */
  int EdgeScrollY;              /* #pixels to scroll on screen edge */
  unsigned char buttons2grab;   /* buttons to grab in click to focus mode */
  int NumBoxes;
  int randomx;                  /* values used for randomPlacement */
  int randomy;
  int VxMax;                    /* Max location for top left of virt desk*/
  int VyMax;
  int Vx;                       /* Current loc for top left of virt desk */
  int Vy;
  int shade_anim_steps;

  int ClickTime;               /*Max button-click delay for Function built-in*/
  int ScrollResistance;        /* resistance to scrolling in desktop */
  int MoveResistance;          /* res to moving windows over viewport edge */
  int MoveSmoothness;
  int MoveThreshold;           /* number of pixels of mouse motion to decide
				* it's a move operation */
  int SnapAttraction;          /* attractiveness of window edges */
  int SnapMode;                /* mode of snap attraction */
  int SnapGridX;               /* snap grid X size */
  int SnapGridY;               /* snap grid Y size */
  int OpaqueSize;
  int CurrentDesk;             /* The current desktop number */
  int ColormapFocus;           /* colormap focus style */
  int ColorLimit;              /* Limit on colors used in pixmaps */
  int DefaultColorset;         /* Default Colorset used by feedback window */

  int use_backing_store;

  /*
  ** some additional global options which will probably become window
  ** specific options later on:
  */
  struct
  {
    unsigned ModifyUSP : 1;                          /* - RBW - 11/02/1998  */
    unsigned CaptureHonorsStartsOnPage : 1;          /* - RBW - 11/02/1998  */
    unsigned RecaptureHonorsStartsOnPage : 1;        /* - RBW - 11/02/1998  */
    unsigned ActivePlacementHonorsStartsOnPage : 1;  /* - RBW - 11/02/1998  */
    unsigned SmartPlacementIsClever : 1;
    unsigned ClickToFocusPassesClick : 1;
    unsigned ClickToFocusRaises : 1;
    unsigned MouseFocusClickRaises : 1;
    unsigned StipledTitles : 1;
    unsigned WindowShadeScrolls : 1;
  } go; /* global options */
  struct
  {
    unsigned ModalityIsEvil : 1;
    unsigned RaiseHackNeeded : 1;
    unsigned DisableConfigureNotify : 1;
    unsigned InstallRootCmap : 1;
    unsigned RaiseOverUnmanaged : 1;
  } bo; /* bug workaround control options */
  struct
  {
    unsigned EmulateMWM : 1;
    unsigned EmulateWIN : 1;
    unsigned use_active_down_buttons : 1;
    unsigned use_inactive_buttons : 1;
    unsigned do_hide_position_window : 1;
    unsigned do_hide_resize_window : 1;
  } gs; /* global style structure */
  struct
  {
    Bool do_save_under : 1;
    unsigned do_need_window_update : 1;
    unsigned has_default_font_changed : 1;
    unsigned has_default_color_changed : 1;
    unsigned has_nr_buttons_changed : 1;
    unsigned silent_functions : 1;
    unsigned windows_captured : 1;
    unsigned edge_wrap_x : 1;
    unsigned edge_wrap_y : 1;
  } flags;

  struct
  {
    last_added_item_type type;
    void *item;
  } last_added_item;
} ScreenInfo;

/*
   Macro which gets specific decor or default decor.
   This saves an indirection in case you don't want
   the UseDecor mechanism.
 */
#ifdef USEDECOR
#define GetDecor(window,part) ((window)->decor->part)
#else
#define GetDecor(window,part) (Scr.DefaultDecor.part)
#endif

/* some protos for the decoration structures */
void LoadDefaultButton(DecorFace *bf, int i);
void ResetAllButtons(FvwmDecor *decor);

/*
 * Diverts a style definition to an FvwmDecor structure (veliaa@rpi.edu)
 */
void AddToDecor(FvwmDecor *decor, char *s);

extern ScreenInfo Scr;

#endif /* _SCREEN_ */
