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
 *
 * $XConsortium: menus.h,v 1.24 89/12/10 17:46:26 jim Exp $
 *
 * twm menus include file
 *
 * 17-Nov-87 Thomas E. LaStrange		File created
 *
 ***********************************************************************/

#ifndef _MENUS_
#define _MENUS_

#include <libs/fvwmlib.h>
#include "fvwm.h"

#define MENU_IS_LEFT  0x01
#define MENU_IS_RIGHT 0x02
#define MENU_IS_UP    0x04
#define MENU_IS_DOWN  0x08

#define MAX_ITEM_LABELS  3

/*************************
 * MENU STYLE STRUCTURES *
 *************************/


typedef enum
{
    /* menu types */
    SimpleMenu = 0,
#ifdef GRADIENT_BUTTONS
    HGradMenu,
    VGradMenu,
    DGradMenu,
    BGradMenu,
#endif
#ifdef PIXMAP_BUTTONS
    PixmapMenu,
    TiledPixmapMenu,
#endif
    SolidMenu
    /* max button is 8 (0x8) */
} MenuFaceType;

typedef struct MenuFeel
{
    struct
    {
      unsigned is_animated : 1;
      unsigned do_popup_immediately : 1;
      unsigned do_title_warp : 1;
      unsigned do_popup_as_root_menu : 1;
      unsigned do_unmap_submenu_on_popdown : 1;
      unsigned use_left_submenus : 1;
    } flags;
    int PopupOffsetPercent;
    int PopupOffsetAdd;
    char *item_format;
} MenuFeel;


typedef struct MenuFace
{
    union
    {
#ifdef PIXMAP_BUTTONS
        Picture *p;
#endif
        Pixel back;
#ifdef GRADIENT_BUTTONS
        struct
	{
            int npixels;
            Pixel *pixels;
        } grad;
#endif
    } u;
    MenuFaceType type;
} MenuFace;

typedef struct MenuLook
{
    MenuFace face;
    struct
    {
      unsigned do_hilight : 1;
      unsigned has_active_fore : 1;
      unsigned has_active_back : 1;
      unsigned has_stipple_fore : 1;
      unsigned has_long_separators : 1;
      unsigned has_triangle_relief : 1;
      unsigned has_side_color : 1;
    } flags;
    unsigned char ReliefThickness;
    unsigned char TitleUnderlines;
    unsigned char BorderWidth;
    Picture *sidePic;
    Pixel sideColor;
    GC MenuGC;
    GC MenuActiveGC;
    GC MenuActiveBackGC;
    GC MenuStippleGC;
    GC MenuReliefGC;
    GC MenuShadowGC;
    ColorPair MenuColors;
    ColorPair MenuActiveColors;
    ColorPair MenuStippleColors;
    ColorPair MenuRelief;
    MyFont *pStdFont;
    int FontHeight;              /* menu font height */
} MenuLook;

typedef struct MenuStyle
{
    char *name;
    struct MenuStyle *next;
    MenuLook look;
    MenuFeel feel;
} MenuStyle;


/************************
 * MENU ITEM STRUCTURES *
 ************************/


struct MenuRoot; /* forward declaration */

/* IMPORTANT NOTE: Don't put members into this struct that can change while the
 * menu is visible! This will wreak havoc on recursive menus when they finally
 * get implemented. */
typedef struct MenuItem
{
    struct MenuItem *next;	/* next menu item */
    struct MenuItem *prev;	/* prev menu item */

    char *label[MAX_ITEM_LABELS]; /* the strings displayed in the item */
    unsigned short label_offset[MAX_ITEM_LABELS]; /* witdh of label[i] */
    unsigned short label_strlen[MAX_ITEM_LABELS]; /* strlen(label[i]) */

    Picture *picture;           /* Pixmap to show above label*/
    Picture *lpicture;          /* Pixmap to show to left of label */

    short y_offset;		/* y offset for item */
    short y_height;		/* y height for item */

    char *action;		/* action to be performed */
    short func_type;		/* type of built in function */
    short hotkey;		/* Hot key offset (pete@tecc.co.uk).
				   0 - No hot key
				   +ve - offset to hot key char in label
				   -ve - offset to hot key char in item2
				   (offsets have 1 added, so +1 or -1
				   refer to the *first* character)
				   */
    char hotkey_column;         /* The column number the hotkey is defined in*/
    char chHotkey;
    struct
    {
      unsigned is_separator : 1;
      unsigned is_title : 1;
      unsigned is_title_centered : 1;
      unsigned is_popup : 1;
      unsigned is_menu : 1;
      unsigned has_text : 1;
      unsigned has_picture : 1;
      unsigned is_selectable : 1;
      /* temporary flags */
      unsigned was_deselected : 1;
    } flags;
} MenuItem;

/* flags */
#define MI_IS_SEPARATOR(i)      ((i)->flags.is_separator)
#define MI_IS_TITLE(i)          ((i)->flags.is_title)
#define MI_IS_TITLE_CENTERED(i) ((i)->flags.is_title_centered)
#define MI_IS_POPUP(i)          ((i)->flags.is_popup)
#define MI_IS_MENU(i)           ((i)->flags.is_menu)
#define MI_HAS_TEXT(i)          ((i)->flags.has_text)
#define MI_HAS_PICTURE(i)       ((i)->flags.has_picture)
#define MI_IS_SELECTABLE(i)     ((i)->flags.is_selectable)
/* temporary flags */
#define MI_WAS_DESELECTED(i)    ((i)->flags.was_deselected)



/************************
 * MENU ROOT STRUCTURES *
 ************************/

/* This struct contains the parts of a root menu that are shared among all
 * copies of the menu */
typedef struct MenuRootStatic
{
  MenuItem *first;            /* first item in menu */
  MenuItem *last;             /* last item in menu */

  int copies;                 /* # of copies, 0 if none except this one */
  int usage_count;            /* # of mapped instances */
  char *name;                 /* name of root */
  unsigned short width;       /* width of the menu */
  unsigned short height;      /* height of the menu */
  unsigned short item_width;          /* width of the actual menu item */
  unsigned short sidepic_x_offset;    /* offset of the sidepic */
  unsigned short icon_x_offset;       /* offset of the mini icon */
  unsigned short triangle_x_offset;   /* offset of the submenu triangle col */
  unsigned short item_text_x_offset;  /* offset of the actual menu item */
  unsigned short item_text_y_offset;  /* y offset for item text. */
  unsigned short hilight_x_offset;    /* start of the area to be hilighted */
  unsigned short hilight_width;       /* width of the area to be hilighted */
  unsigned short y_offset;            /* y coordinate for item */
  unsigned short items;               /* number of items in the menu */
  Picture *sidePic;
  Pixel sideColor;
  /* Menu Face    */
  MenuStyle *ms;
  /* permanent flags */
  struct
  {
    unsigned has_side_color : 1;
    unsigned is_left_triangle : 1;
  } flags;
  struct
  {
    char *popup_action;
    char *popdown_action;
  } dynamic;
} MenuRootStatic;

/* access macros to static menu members */
#define MR_BORDER_WIDTH(m)       ((m)->s->ms->look.BorderWidth)
#define MR_FIRST_ITEM(m)         ((m)->s->first)
#define MR_LAST_ITEM(m)          ((m)->s->last)
#define MR_COPIES(m)             ((m)->s->copies)
#define MR_MAPPED_COPIES(m)      ((m)->s->usage_count)
#define MR_NAME(m)               ((m)->s->name)
#define MR_WIDTH(m)              ((m)->s->width)
#define MR_HEIGHT(m)             ((m)->s->height)
#define MR_ITEM_WIDTH(m)         ((m)->s->item_width)
#define MR_SIDEPIC_X_OFFSET(m)   ((m)->s->sidepic_x_offset)
#define MR_ICON_X_OFFSET(m)      ((m)->s->icon_x_offset)
#define MR_TRIANGLE_X_OFFSET(m)  ((m)->s->triangle_x_offset)
#define MR_ITEM_X_OFFSET(m)      ((m)->s->item_text_x_offset)
#define MR_ITEM_TEXT_Y_OFFSET(m) ((m)->s->item_text_y_offset)
#define MR_HILIGHT_X_OFFSET(m)   ((m)->s->hilight_x_offset)
#define MR_HILIGHT_WIDTH(m)      ((m)->s->hilight_width)
#define MR_ITEMS(m)              ((m)->s->items)
#define MR_SIDEPIC(m)            ((m)->s->sidePic)
#define MR_SIDECOLOR(m)          ((m)->s->sideColor)
#define MR_STYLE(m)              ((m)->s->ms)
/* flags */
#define MR_FLAGS(m)              ((m)->s->flags)
#define MR_DYNAMIC(m)            ((m)->s->dynamic)
#define MR_HAS_SIDECOLOR(m)      ((m)->s->flags.has_side_color)
#define MR_IS_LEFT_TRIANGLE(m)   ((m)->s->flags.is_left_triangle)


/* This struct contains the parts of a root menu that differ in all copies of
 * the menu */
typedef struct MenuRootDynamic
{
  struct MenuRoot *original_menu;     /* the first copy of the current menu */
  struct MenuRoot *next_menu;         /* next in list of root menus */
  struct MenuRoot *continuation_menu; /* continuation of this menu
				       * (too tall for screen) */
  /* can get the menu that this popped up through selected->mr when
   * selected is a popup menu item */
  struct MenuRoot *parent_menu; /* the menu that popped this up, if any */
  struct MenuItem *parent_item; /* the menu item that popped this up, if any */
  Window window;                /* the window of the menu */
  MenuItem *selected_item;      /* the selected item in menu */
  int xanimation;               /* x distance window was moved by animation */
  /* dynamic temp flags */
  struct
  {
    unsigned is_painted : 1;
    unsigned is_background_set : 1; /* is win background set? */
    unsigned is_left : 1;   /* menu direction relative to parent menu */
    unsigned is_right : 1;
    unsigned is_up : 1;
    unsigned is_down : 1;
    unsigned has_popped_up_left : 1;
    unsigned has_popped_up_right : 1;
  } dflags;
#ifdef GRADIENT_BUTTONS
  struct
  {
    Pixmap stored;
    int width;
    int height;
    int y;
  } stored_item;
#endif
} MenuRootDynamic;

/* access macros to static menu members */
#define MR_ORIGINAL_MENU(m)         ((m)->d->original_menu)
#define MR_NEXT_MENU(m)             ((m)->d->next_menu)
#define MR_CONTINUATION_MENU(m)     ((m)->d->continuation_menu)
#define MR_PARENT_MENU(m)           ((m)->d->parent_menu)
#define MR_PARENT_ITEM(m)           ((m)->d->parent_item)
#define MR_WINDOW(m)                ((m)->d->window)
#define MR_SELECTED_ITEM(m)         ((m)->d->selected_item)
#define MR_XANIMATION(m)            ((m)->d->xanimation)
#ifdef GRADIENT_BUTTONS
#define MR_STORED_ITEM(m)           ((m)->d->stored_item)
#endif
/* flags */
#define MR_DYNAMIC_FLAGS(m)         ((m)->d->dflags)
#define MR_IS_PAINTED(m)            ((m)->d->dflags.is_painted)
#define MR_IS_BACKGROUND_SET(m)     ((m)->d->dflags.is_background_set)
#define MR_IS_LEFT(m)               ((m)->d->dflags.is_left)
#define MR_IS_RIGHT(m)              ((m)->d->dflags.is_right)
#define MR_IS_UP(m)                 ((m)->d->dflags.is_up)
#define MR_IS_DOWN(m)               ((m)->d->dflags.is_down)
#define MR_HAS_POPPED_UP_LEFT(m)    ((m)->d->dflags.has_popped_up_left)
#define MR_HAS_POPPED_UP_RIGHT(m)   ((m)->d->dflags.has_popped_up_right)

typedef struct MenuRoot
{
    MenuRootStatic *s;
    MenuRootDynamic *d;
} MenuRoot;
/* don't forget to initialise new members in NewMenuRoot()! */



/***********************************
 * MENU OPTIONS AND POSITION HINTS *
 ***********************************/


typedef struct
{
  int x;                  /* suggested x position */
  int y;                  /* suggested y position */
  float x_factor;         /* to take menu width into account (0, -1 or -0.5) */
  float y_factor;         /* same with height */
  Bool is_relative;       /* FALSE if referring to absolute screen position */
} MenuPosHints;

typedef struct
{
  MenuPosHints pos_hints;
  struct
  {
    unsigned no_warp : 1;
    unsigned warp_title : 1;
    unsigned fixed : 1;
    unsigned select_in_place : 1;
    unsigned select_warp : 1;
    unsigned has_poshints : 1;
  } flags;
} MenuOptions;

extern MenuPosHints lastMenuPosHints;
extern Bool fLastMenuPosHintsValid;



/****************************
 * MISCELLANEOUS MENU STUFF *
 ****************************/

typedef struct
{
  MenuRoot *menu;
  MenuRoot *parent_menu;
  MenuItem *parent_item;
  FvwmWindow **pTmp_win;
  FvwmWindow *button_window;
  int *pcontext;
  XEvent *eventp;
  char **ret_paction;
  MenuOptions *pops;
  struct
  {
    unsigned is_menu_from_frame_or_window_or_titlebar : 1;
    unsigned is_sticky : 1;
    unsigned is_submenu : 1;
    unsigned is_already_mapped : 1;
  } flags;
} MenuParameters;


/* Return values for UpdateMenu, do_menu, menuShortcuts */
/* Just uses enum-s for their constant value, replaced a bunch of #define-s
 * before */
/* This is a lame hack, in that "_BUTTON" is added to mean a button-release
   caused the return-- the macros below help deal with the ugliness */
typedef enum
{
    MENU_ERROR = -1,
    MENU_NOP = 0,
    MENU_DONE = 1,
    MENU_DONE_BUTTON = 2,  /* must be MENU_DONE + 1 */
    MENU_ABORTED = 3,
    MENU_ABORTED_BUTTON = 4, /* must be MENU_ABORTED + 1 */
    MENU_SUBMENU_DONE,
    MENU_DOUBLE_CLICKED,
    MENU_POPUP,
    MENU_POPDOWN,
    MENU_SELECTED,
    MENU_NEWITEM,
    MENU_TEAR_OFF
} MenuStatus;


typedef struct MenuInfo
{
  MenuRoot *all;
  struct MenuStyle *DefaultStyle;
  struct MenuStyle *LastStyle;
  int PopupDelay10ms;
  int DoubleClickTime;
  struct
  {
    unsigned use_animated_menus : 1;
  } flags;
} MenuInfo;

extern MenuInfo Menus;

#define IS_MENU_RETURN(x) ((x)>=MENU_DONE && (x)<=MENU_ABORTED_BUTTON)
#define IS_MENU_BUTTON(x) ((x)==MENU_DONE_BUTTON || (x)==MENU_ABORTED_BUTTON)
#define MENU_ADD_BUTTON(x) ((x)==MENU_DONE || (x)==MENU_ABORTED?(x)+1:(x))
#define MENU_ADD_BUTTON_IF(y,x) (y?MENU_ADD_BUTTON((x)):(x))



/**********************
 * EXPORTED FUNCTIONS *
 **********************/


MenuRoot *FollowMenuContinuations(MenuRoot *mr,MenuRoot **pmrPrior);
void AnimatedMoveOfWindow(Window w,int startX,int startY,int endX, int endY,
			  Bool fWarpPointerToo, int cusDelay,
			  float *ppctMovement );
MenuRoot *NewMenuRoot(char *name);
void AddToMenu(MenuRoot *, char *, char *, Bool, Bool);
void MakeMenu(MenuRoot *);
MenuStatus do_menu(MenuParameters *pmp);
MenuRoot *FindPopup(char *popup_name);
char *GetMenuOptions(char *action, Window w, FvwmWindow *tmp_win,
		     MenuRoot *mr, MenuItem *mi, MenuOptions *pops);
void DestroyMenu(MenuRoot *mr, Bool recreate);
void MakeMenus(void);
void add_item_to_menu(F_CMD_ARGS);
void add_another_menu_item(char *action);
void destroy_menu(F_CMD_ARGS);
void ChangeMenuStyle(F_CMD_ARGS);
void DestroyMenuStyle(F_CMD_ARGS);
void SetMenuStyle(F_CMD_ARGS);
void UpdateAllMenuStyles(void);
void SetMenuCursor(Cursor cursor);


#endif /* _MENUS_ */
