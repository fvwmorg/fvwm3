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

#define MENU_IS_LEFT  0x01
#define MENU_IS_RIGHT 0x02
#define MENU_IS_UP    0x04
#define MENU_IS_DOWN  0x08

#define MAX_ITEM_LABELS  3
#define MAX_MINI_ICONS   2

/*************************
 * MENU STYLE STRUCTURES *
 *************************/


typedef enum
{
  /* menu types */
  SimpleMenu = 0,
  GradientMenu,
  PixmapMenu,
  TiledPixmapMenu,
  SolidMenu
  /* max button is 8 (0x8) */
} MenuFaceType;

typedef struct MenuFeel
{
  struct
  {
    unsigned is_animated : 1;
    unsigned do_popup_immediately : 1;
    unsigned do_warp_to_title : 1;
    unsigned do_popup_as_root_menu : 1;
    unsigned do_unmap_submenu_on_popdown : 1;
    unsigned use_left_submenus : 1;
    unsigned use_automatic_hotkeys : 1;
  } flags;
  int PopupOffsetPercent;
  int PopupOffsetAdd;
  char *item_format;
  KeyCode select_on_release_key;
} MenuFeel;


typedef struct MenuFace
{
  union
  {
    Picture *p;
    Pixel back;
    struct
    {
      int npixels;
      Pixel *pixels;
    } grad;
  } u;
  MenuFaceType type;
  char gradient_type;
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
    unsigned has_menu_cset : 1;
    unsigned has_active_cset : 1;
    unsigned has_greyed_cset : 1;
  } flags;
  unsigned char ReliefThickness;
  unsigned char TitleUnderlines;
  unsigned char BorderWidth;
  struct
  {
    signed char item_above;
    signed char item_below;
    signed char title_above;
    signed char title_below;
    signed char separator_above;
    signed char separator_below;
  } vertical_spacing;
  struct
  {
    int menu;
    int active;
    int greyed;
  } cset;
  Picture *side_picture;
  Pixel side_color;
  GC MenuGC;
  GC MenuActiveGC;
  GC MenuActiveBackGC;
  GC MenuStippleGC;
  GC MenuReliefGC;
  GC MenuShadowGC;
  GC MenuActiveReliefGC;
  GC MenuActiveShadowGC;
  ColorPair MenuColors;
  ColorPair MenuActiveColors;
  ColorPair MenuStippleColors;
  ColorPair MenuReliefColors;
  FvwmFont *pStdFont;
  int FontHeight;              /* menu font height */
} MenuLook;

typedef struct MenuStyle
{
  char *name;
  struct MenuStyle *next_style;
  unsigned int usage_count;
  MenuLook look;
  MenuFeel feel;
  struct
  {
    unsigned is_updated : 1;
  } flags;
} MenuStyle;

#define ST_NAME(s)                    ((s)->name)
#define MST_NAME(m)                   ((m)->s->ms->name)
#define ST_NEXT_STYLE(s)              ((s)->next_style)
#define MST_NEXT_STYLE(m)             ((m)->s->ms->next_style)
#define ST_USAGE_COUNT(s)             ((s)->usage_count)
#define MST_USAGE_COUNT(m)            ((m)->s->ms->usage_count)
/* flags */
#define ST_IS_UPDATED(s)              ((s)->flags.is_updated)
#define MST_IS_UPDATED(m)             ((m)->s->ms->flags.is_updated)
/* look */
#define ST_FACE(s)                    ((s)->look.face)
#define MST_FACE(m)                   ((m)->s->ms->look.face)
#define ST_DO_HILIGHT(s)              ((s)->look.flags.do_hilight)
#define MST_DO_HILIGHT(m)             ((m)->s->ms->look.flags.do_hilight)
#define ST_HAS_ACTIVE_FORE(s)         ((s)->look.flags.has_active_fore)
#define MST_HAS_ACTIVE_FORE(m)        ((m)->s->ms->look.flags.has_active_fore)
#define ST_HAS_ACTIVE_BACK(s)         ((s)->look.flags.has_active_back)
#define MST_HAS_ACTIVE_BACK(m)        ((m)->s->ms->look.flags.has_active_back)
#define ST_HAS_STIPPLE_FORE(s)        ((s)->look.flags.has_stipple_fore)
#define MST_HAS_STIPPLE_FORE(m)       ((m)->s->ms->look.flags.has_stipple_fore)
#define ST_HAS_LONG_SEPARATORS(s)     ((s)->look.flags.has_long_separators)
#define MST_HAS_LONG_SEPARATORS(m)    ((m)->s->ms->look.flags.has_long_separators)
#define ST_HAS_TRIANGLE_RELIEF(s)     ((s)->look.flags.has_triangle_relief)
#define MST_HAS_TRIANGLE_RELIEF(m)    ((m)->s->ms->look.flags.has_triangle_relief)
#define ST_HAS_SIDE_COLOR(s)          ((s)->look.flags.has_side_color)
#define MST_HAS_SIDE_COLOR(m)         ((m)->s->ms->look.flags.has_side_color)
#define ST_HAS_MENU_CSET(s)           ((s)->look.flags.has_menu_cset)
#define MST_HAS_MENU_CSET(m)          ((m)->s->ms->look.flags.has_menu_cset)
#define ST_HAS_ACTIVE_CSET(s)         ((s)->look.flags.has_active_cset)
#define MST_HAS_ACTIVE_CSET(m)        ((m)->s->ms->look.flags.has_active_cset)
#define ST_HAS_GREYED_CSET(s)         ((s)->look.flags.has_greyed_cset)
#define MST_HAS_GREYED_CSET(m)        ((m)->s->ms->look.flags.has_greyed_cset)
#define ST_RELIEF_THICKNESS(s)        ((s)->look.ReliefThickness)
#define MST_RELIEF_THICKNESS(m)       ((m)->s->ms->look.ReliefThickness)
#define ST_TITLE_UNDERLINES(s)        ((s)->look.TitleUnderlines)
#define MST_TITLE_UNDERLINES(m)       ((m)->s->ms->look.TitleUnderlines)
#define ST_BORDER_WIDTH(s)            ((s)->look.BorderWidth)
#define MST_BORDER_WIDTH(m)           ((m)->s->ms->look.BorderWidth)
#define ST_ITEM_GAP_ABOVE(s)          ((s)->look.vertical_spacing.item_above)
#define MST_ITEM_GAP_ABOVE(m)         ((m)->s->ms->look.vertical_spacing.item_above)
#define ST_ITEM_GAP_BELOW(s)          ((s)->look.vertical_spacing.item_below)
#define MST_ITEM_GAP_BELOW(m)         ((m)->s->ms->look.vertical_spacing.item_below)
#define ST_TITLE_GAP_ABOVE(s)         ((s)->look.vertical_spacing.title_above)
#define MST_TITLE_GAP_ABOVE(m)        ((m)->s->ms->look.vertical_spacing.title_above)
#define ST_TITLE_GAP_BELOW(s)         ((s)->look.vertical_spacing.title_below)
#define MST_TITLE_GAP_BELOW(m)        ((m)->s->ms->look.vertical_spacing.title_below)
#define ST_SEPARATOR_GAP_ABOVE(s)     ((s)->look.vertical_spacing.separator_above)
#define MST_SEPARATOR_GAP_ABOVE(m)    ((m)->s->ms->look.vertical_spacing.separator_above)
#define ST_SEPARATOR_GAP_BELOW(s)     ((s)->look.vertical_spacing.separator_below)
#define MST_SEPARATOR_GAP_BELOW(m)    ((m)->s->ms->look.vertical_spacing.separator_below)
#define ST_CSET_MENU(s)               ((s)->look.cset.menu)
#define MST_CSET_MENU(m)              ((m)->s->ms->look.cset.menu)
#define ST_CSET_ACTIVE(s)             ((s)->look.cset.active)
#define MST_CSET_ACTIVE(m)            ((m)->s->ms->look.cset.active)
#define ST_CSET_GREYED(s)             ((s)->look.cset.greyed)
#define MST_CSET_GREYED(m)            ((m)->s->ms->look.cset.greyed)
#define ST_SIDEPIC(s)                 ((s)->look.side_picture)
#define MST_SIDEPIC(m)                ((m)->s->ms->look.side_picture)
#define ST_SIDE_COLOR(s)              ((s)->look.side_color)
#define MST_SIDE_COLOR(m)             ((m)->s->ms->look.side_color)
#define ST_MENU_GC(s)                 ((s)->look.MenuGC)
#define MST_MENU_GC(m)                ((m)->s->ms->look.MenuGC)
#define ST_MENU_ACTIVE_GC(s)          ((s)->look.MenuActiveGC)
#define MST_MENU_ACTIVE_GC(m)         ((m)->s->ms->look.MenuActiveGC)
#define ST_MENU_ACTIVE_BACK_GC(s)     ((s)->look.MenuActiveBackGC)
#define MST_MENU_ACTIVE_BACK_GC(m)    ((m)->s->ms->look.MenuActiveBackGC)
#define ST_MENU_ACTIVE_RELIEF_GC(s)   ((s)->look.MenuActiveReliefGC)
#define MST_MENU_ACTIVE_RELIEF_GC(m)  ((m)->s->ms->look.MenuActiveReliefGC)
#define ST_MENU_ACTIVE_SHADOW_GC(s)   ((s)->look.MenuActiveShadowGC)
#define MST_MENU_ACTIVE_SHADOW_GC(m)  ((m)->s->ms->look.MenuActiveShadowGC)
#define ST_MENU_STIPPLE_GC(s)         ((s)->look.MenuStippleGC)
#define MST_MENU_STIPPLE_GC(m)        ((m)->s->ms->look.MenuStippleGC)
#define ST_MENU_RELIEF_GC(s)          ((s)->look.MenuReliefGC)
#define MST_MENU_RELIEF_GC(m)         ((m)->s->ms->look.MenuReliefGC)
#define ST_MENU_SHADOW_GC(s)          ((s)->look.MenuShadowGC)
#define MST_MENU_SHADOW_GC(m)         ((m)->s->ms->look.MenuShadowGC)
#define ST_MENU_COLORS(s)             ((s)->look.MenuColors)
#define MST_MENU_COLORS(m)            ((m)->s->ms->look.MenuColors)
#define ST_MENU_ACTIVE_COLORS(s)      ((s)->look.MenuActiveColors)
#define MST_MENU_ACTIVE_COLORS(m)     ((m)->s->ms->look.MenuActiveColors)
#define ST_MENU_STIPPLE_COLORS(s)     ((s)->look.MenuStippleColors)
#define MST_MENU_STIPPLE_COLORS(m)    ((m)->s->ms->look.MenuStippleColors)
#define ST_MENU_RELIEF_COLORS(s)      ((s)->look.MenuReliefColors)
#define MST_MENU_RELIEF_COLORS(m)     ((m)->s->ms->look.MenuReliefColors)
#define ST_PSTDFONT(s)                ((s)->look.pStdFont)
#define MST_PSTDFONT(m)               ((m)->s->ms->look.pStdFont)
#define ST_FONT_HEIGHT(s)             ((s)->look.FontHeight)
#define MST_FONT_HEIGHT(m)            ((m)->s->ms->look.FontHeight)
/* feel */
#define ST_IS_ANIMATED(s)             ((s)->feel.flags.is_animated)
#define MST_IS_ANIMATED(m)            ((m)->s->ms->feel.flags.is_animated)
#define ST_DO_POPUP_IMMEDIATELY(s)    ((s)->feel.flags.do_popup_immediately)
#define MST_DO_POPUP_IMMEDIATELY(m) \
        ((m)->s->ms->feel.flags.do_popup_immediately)
#define ST_DO_WARP_TO_TITLE(s)        ((s)->feel.flags.do_warp_to_title)
#define MST_DO_WARP_TO_TITLE(m)       ((m)->s->ms->feel.flags.do_warp_to_title)
#define ST_DO_POPUP_AS_ROOT_MENU(s)   ((s)->feel.flags.do_popup_as_root_menu)
#define MST_DO_POPUP_AS_ROOT_MENU(m) \
        ((m)->s->ms->feel.flags.do_popup_as_root_menu)
#define ST_DO_UNMAP_SUBMENU_ON_POPDOWN(s) \
        ((s)->feel.flags.do_unmap_submenu_on_popdown)
#define MST_DO_UNMAP_SUBMENU_ON_POPDOWN(m) \
        ((m)->s->ms->feel.flags.do_unmap_submenu_on_popdown)
#define ST_USE_LEFT_SUBMENUS(s)       ((s)->feel.flags.use_left_submenus)
#define MST_USE_LEFT_SUBMENUS(m) \
        ((m)->s->ms->feel.flags.use_left_submenus)
#define ST_USE_AUTOMATIC_HOTKEYS(s)   ((s)->feel.flags.use_automatic_hotkeys)
#define MST_USE_AUTOMATIC_HOTKEYS(m) \
        ((m)->s->ms->feel.flags.use_automatic_hotkeys)
#define ST_FLAGS(s)                   ((s)->feel.flags)
#define MST_FLAGS(m)                  ((m)->s->ms->feel.flags)
#define ST_POPUP_OFFSET_PERCENT(s)    ((s)->feel.PopupOffsetPercent)
#define MST_POPUP_OFFSET_PERCENT(m)   ((m)->s->ms->feel.PopupOffsetPercent)
#define ST_POPUP_OFFSET_ADD(s)        ((s)->feel.PopupOffsetAdd)
#define MST_POPUP_OFFSET_ADD(m)       ((m)->s->ms->feel.PopupOffsetAdd)
#define ST_ITEM_FORMAT(s)             ((s)->feel.item_format)
#define MST_ITEM_FORMAT(m)            ((m)->s->ms->feel.item_format)
#define ST_SELECT_ON_RELEASE_KEY(s)   ((s)->feel.select_on_release_key)
#define MST_SELECT_ON_RELEASE_KEY(m)  ((m)->s->ms->feel.select_on_release_key)



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
    Picture *lpicture[MAX_MINI_ICONS]; /* Pics to show left/right of label */

    short y_offset;		/* y offset for item */
    short height;		/* y height for item */

    char *action;		/* action to be performed */
    short func_type;		/* type of built in function */
    short hotkey_coffset;	/* Hot key offset (pete@tecc.co.uk). */
    char hotkey_column;         /* The column number the hotkey is defined in*/
    struct
    {
      unsigned is_separator : 1;
      unsigned is_title : 1;
      unsigned is_title_centered : 1;
      unsigned is_popup : 1;
      unsigned is_menu : 1;
      unsigned has_text : 1;
      unsigned has_picture : 1;
      unsigned has_hotkey : 1;
      unsigned is_hotkey_automatic : 1;
      unsigned is_selectable : 1;
      /* temporary flags */
      unsigned was_deselected : 1;
    } flags;
} MenuItem;

#define MI_NEXT_ITEM(i)         ((i)->next)
#define MI_PREV_ITEM(i)         ((i)->prev)
#define MI_LABEL(i)             ((i)->label)
#define MI_LABEL_OFFSET(i)      ((i)->label_offset)
#define MI_LABEL_STRLEN(i)      ((i)->label_strlen)
#define MI_PICTURE(i)           ((i)->picture)
#define MI_MINI_ICON(i)         ((i)->lpicture)
#define MI_Y_OFFSET(i)          ((i)->y_offset)
#define MI_HEIGHT(i)            ((i)->height)
#define MI_ACTION(i)            ((i)->action)
#define MI_FUNC_TYPE(i)         ((i)->func_type)
#define MI_HOTKEY_COFFSET(i)    ((i)->hotkey_coffset)
#define MI_HOTKEY_COLUMN(i)     ((i)->hotkey_column)
/* flags */
#define MI_IS_SEPARATOR(i)      ((i)->flags.is_separator)
#define MI_IS_TITLE(i)          ((i)->flags.is_title)
#define MI_IS_TITLE_CENTERED(i) ((i)->flags.is_title_centered)
#define MI_IS_POPUP(i)          ((i)->flags.is_popup)
#define MI_IS_MENU(i)           ((i)->flags.is_menu)
#define MI_HAS_TEXT(i)          ((i)->flags.has_text)
#define MI_HAS_PICTURE(i)       ((i)->flags.has_picture)
#define MI_HAS_HOTKEY(i)        ((i)->flags.has_hotkey)
#define MI_IS_HOTKEY_AUTOMATIC(i) ((i)->flags.is_hotkey_automatic)
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
  unsigned short icon_x_offset[MAX_MINI_ICONS]; /* offsets of the mini icons */
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
    unsigned is_updated : 1;
  } flags;
  struct
  {
    char *popup_action;
    char *popdown_action;
    char *missing_submenu_func;
  } dynamic;
} MenuRootStatic;

/* access macros to static menu members */
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
#define MR_POPUP_ACTION(m)       ((m)->s->dynamic.popup_action)
#define MR_POPDOWN_ACTION(m)     ((m)->s->dynamic.popdown_action)
#define MR_MISSING_SUBMENU_FUNC(m) ((m)->s->dynamic.missing_submenu_func)
#define MR_HAS_SIDECOLOR(m)      ((m)->s->flags.has_side_color)
#define MR_IS_LEFT_TRIANGLE(m)   ((m)->s->flags.is_left_triangle)
#define MR_IS_UPDATED(m)         ((m)->s->flags.is_updated)


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
  struct
  {
    Pixmap stored;
    int width;
    int height;
    int y;
  } stored_item;
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
#define MR_STORED_ITEM(m)           ((m)->d->stored_item)
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
  int x_offset;           /* additional offset to x */
  int menu_width;         /* width of the parent menu or item */
  float x_factor;         /* to take menu width into account (0, -1 or -0.5) */
  float context_x_factor; /* additional offset factor to x */
  float y_factor;         /* same with height */
  Bool is_relative;       /* FALSE if referring to absolute screen position */
  Bool is_menu_relative;  /* TRUE if referring to a part of a menu */
} MenuPosHints;

typedef struct
{
  MenuPosHints pos_hints;
  struct
  {
    unsigned do_not_warp : 1;
    unsigned do_warp_on_select : 1;
    unsigned do_warp_title : 1;
    unsigned do_select_in_place : 1;
    unsigned has_poshints : 1;
    unsigned is_fixed : 1;
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
    unsigned has_default_action : 1;
    unsigned is_invoked_by_key_press : 1;
    unsigned is_menu_from_frame_or_window_or_titlebar : 1;
    unsigned is_sticky : 1;
    unsigned is_submenu : 1;
    unsigned is_already_mapped : 1;
  } flags;
} MenuParameters;

/* Return values for UpdateMenu, do_menu, menuShortcuts.  This is a lame
 * hack, in that "_BUTTON" is added to mean a button-release caused the
 * return-- the macros below help deal with the ugliness. */
typedef enum
{
  MENU_ERROR = -1,
  MENU_NOP = 0,
  MENU_DONE,
  MENU_ABORTED,
  MENU_SUBMENU_DONE,
  MENU_DOUBLE_CLICKED,
  MENU_POPUP,
  MENU_POPDOWN,
  MENU_SELECTED,
  MENU_NEWITEM,
  MENU_POST,
  MENU_TEAR_OFF,
  /* propagate the event to a different menu */
  MENU_PROPAGATE_EVENT
} MenuRC;

typedef struct
{
  MenuRC rc;
  MenuRoot *menu;
  MenuRoot *parent_menu;
  struct
  {
    unsigned is_first_item_selected : 1;
    unsigned is_key_press : 1;
    unsigned is_menu_pinned : 1;
    unsigned is_menu_posted : 1;
  } flags;
} MenuReturn;


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

#define IS_MENU_RETURN(x) ((x)==MENU_DONE || (x)==MENU_ABORTED)



/**********************
 * EXPORTED FUNCTIONS *
 **********************/


MenuRoot *FollowMenuContinuations(MenuRoot *mr,MenuRoot **pmrPrior);
void AnimatedMoveOfWindow(Window w,int startX,int startY,int endX, int endY,
			  Bool fWarpPointerToo, int cusDelay,
			  float *ppctMovement );
MenuRoot *NewMenuRoot(char *name);
void AddToMenu(MenuRoot *, char *, char *, Bool, Bool);
void do_menu(MenuParameters *pmp, MenuReturn *pret);
MenuRoot *FindPopup(char *popup_name);
char *GetMenuOptions(char *action, Window w, FvwmWindow *tmp_win,
		     MenuRoot *mr, MenuItem *mi, MenuOptions *pops);
void DestroyMenu(MenuRoot *mr, Bool recreate);
void add_item_to_menu(F_CMD_ARGS);
void add_another_menu_item(char *action);
void destroy_menu(F_CMD_ARGS);
void popup_func(F_CMD_ARGS);
void staysup_func(F_CMD_ARGS);
void change_mr_menu_style(MenuRoot *mr, char *stylename);
void ChangeMenuStyle(F_CMD_ARGS);
void DestroyMenuStyle(F_CMD_ARGS);
void SetMenuStyle(F_CMD_ARGS);
void UpdateAllMenuStyles(void);
void UpdateMenuColorset(int cset);
void SetMenuCursor(Cursor cursor);


#endif /* _MENUS_ */
