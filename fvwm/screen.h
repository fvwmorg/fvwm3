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

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "functions.h"
#include "misc.h"

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


typedef enum {
    /* button types */
#ifdef VECTOR_BUTTONS
    VectorButton                ,
#endif
    SimpleButton                ,
#ifdef GRADIENT_BUTTONS
    HGradButton                 ,
    VGradButton                 ,
#endif
#ifdef PIXMAP_BUTTONS
    PixmapButton                ,
    TiledPixmapButton           ,
#endif
#ifdef MINI_ICONS
    MiniIconButton              ,
#endif
    SolidButton
    /* max button is 15 (0xF) */
} ButtonFaceStyle;

#define ButtonFaceTypeMask      0x000F

/* button style flags (per-state) */
enum {

    /* specific style flags */
    /* justification bits (3.17 -> 4 bits) */
    HOffCenter                  = (1<<4),
    HRight                      = (1<<5),
    VOffCenter                  = (1<<6),
    VBottom                     = (1<<7),

    /* general style flags */
#ifdef EXTENDED_TITLESTYLE
    UseTitleStyle               = (1<<8),
#endif
#ifdef BORDERSTYLE
    UseBorderStyle              = (1<<9),
#endif
    FlatButton                  = (1<<10),
    SunkButton                  = (1<<11)
};

#ifdef BORDERSTYLE
/* border style flags (uses ButtonFace) */
enum {
    HiddenHandles               = (1<<8),
    NoInset                     = (1<<9)
};
#endif

typedef struct ButtonFace {
    ButtonFaceStyle style;
    union {
#ifdef PIXMAP_BUTTONS
	Picture *p;
#endif
	Pixel back;
#ifdef GRADIENT_BUTTONS
	struct {
	    int npixels;
	    Pixel *pixels;
	} grad;
#endif
#ifdef VECTOR_BUTTONS
    struct vector_coords {
      int num;
      int *x;
      int *y;
      unsigned long line_style;
    } vector;
#endif
    } u;

#ifdef MULTISTYLE
    struct ButtonFace *next;
#endif
} ButtonFace;

/* button style flags (per title button) */
enum {
  /* MWM function hint button assignments */
  MWMDecorMenu                = (1<<0),
  MWMDecorMinimize            = (1<<1),
  MWMDecorMaximize            = (1<<2),
  MWMDecorShade               = (1<<3),
  MWMDecorStick               = (1<<4)
};

enum ButtonState {
    ActiveUp,
#ifdef ACTIVEDOWN_BTNS
    ActiveDown,
#endif
#ifdef INACTIVE_BTNS
    Inactive,
#endif
    ToggledActiveUp,
#ifdef ACTIVEDOWN_BTNS
    ToggledActiveDown,
#endif
#ifdef INACTIVE_BTNS
    ToggledInactive,
#endif
    MaxButtonState
};

typedef struct {
    int flags;
    ButtonFace state[MaxButtonState];
} TitleButton;

typedef struct FvwmDecor {
#ifdef USEDECOR
    char *tag;			/* general style tag */
#endif
    ColorPair HiColors;		/* standard fore/back colors */
    ColorPair HiRelief;
    GC HiReliefGC;		/* GC for highlighted window relief */
    GC HiShadowGC;		/* GC for highlighted window shadow */

    int TitleHeight;            /* height of the title bar window */
    MyFont WindowFont;          /* font structure for window titles */

    /* titlebar buttons */
    TitleButton left_buttons[5];
    TitleButton right_buttons[5];
    TitleButton titlebar;
#ifdef BORDERSTYLE
    struct BorderStyle
    {
	ButtonFace active, inactive;
    } BorderStyle;
#endif
#ifdef USEDECOR
    struct FvwmDecor *next;	/* additional user-defined styles */
#endif
} FvwmDecor;


typedef struct ScreenInfo
{

  unsigned long screen;
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
  FvwmWindow *pushed_window;	/* saved window to install when pushes drops
				   to zero */
  Cursor *FvwmCursors;

  char *DefaultIcon;            /* Icon to use when no other icons are found */

  ColorPair StdColors; 	/* standard fore/back colors */
  ColorPair StdRelief;

  int TopLayer;
  int DefaultLayer;
  int BottomLayer;

  FvwmFunction *functions;

  MyFont StdFont;     	/* font structure */
  MyFont IconFont;      /* for icon labels */

#if defined(PIXMAP_BUTTONS) || defined(GRADIENT_BUTTONS)
  GC TransMaskGC;               /* GC for transparency masks */
#endif
  GC StdGC;
  GC StdReliefGC;
  GC StdShadowGC;
  Visual *viz;
  unsigned int depth;
  Colormap cmap;
  unsigned int bestTileWidth;
  unsigned int bestTileHeight;
  /* this structure defines the default background, used by modules that have
   * been converted to share fvwm's visuals. see libs/ModGraph.h */
  Background *bg;

  GC DrawGC;			/* GC to draw lines for move and resize */
  Picture *DrawPicture;
  GC ScratchGC1;
  GC ScratchGC2;
  GC ScratchGC3;
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
  int MaxWindowWidth;
  int MaxWindowHeight;
  int shade_anim_steps;

  int ClickTime;               /*Max button-click delay for Function built-in*/
  int ScrollResistance;        /* resistance to scrolling in desktop */
  int MoveResistance;          /* res to moving windows over viewport edge */
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

  /*
  ** some additional global options which will probably become window
  ** specific options later on:
  */
  struct
  {
    Bool ModifyUSP : 1;                          /* - RBW - 11/02/1998  */
    Bool CaptureHonorsStartsOnPage : 1;          /* - RBW - 11/02/1998  */
    Bool RecaptureHonorsStartsOnPage : 1;        /* - RBW - 11/02/1998  */
    Bool ActivePlacementHonorsStartsOnPage : 1;  /* - RBW - 11/02/1998  */
    Bool SmartPlacementIsClever : 1;
    Bool ClickToFocusPassesClick : 1;
    Bool ClickToFocusRaises : 1;
    Bool MouseFocusClickRaises : 1;
    Bool StipledTitles : 1;
    Bool WindowShadeScrolls : 1;
    Bool RaiseHackNeeded : 1;
  } go; /* global options */
  struct
  {
    Bool EmulateMWM : 1;
    Bool EmulateWIN : 1;
    Bool use_active_down_buttons : 1;
    Bool use_inactive_buttons : 1;
  } gs; /* global style structure */
  struct
  {
    unsigned has_icon_font : 1;
    unsigned has_window_font : 1;
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
#define GetDecor(window,part) ((window)->fl->part)
#else
#define GetDecor(window,part) (Scr.DefaultDecor.part)
#endif

/* some protos for the decoration structures */
void LoadDefaultLeftButton(ButtonFace *bf, int i);
void LoadDefaultRightButton(ButtonFace *bf, int i);
void LoadDefaultButton(ButtonFace *bf, int i);
void ResetAllButtons(FvwmDecor *fl);
void InitFvwmDecor(FvwmDecor *fl);
void DestroyFvwmDecor(FvwmDecor *fl);

/*
 * Diverts a style definition to an FvwmDecor structure (veliaa@rpi.edu)
 */
void AddToDecor(FvwmDecor *fl, char *s);

extern ScreenInfo Scr;

#endif /* _SCREEN_ */
