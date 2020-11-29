/* -*-c-*- */

#ifndef FVWM_MENUITEM_H
#define FVWM_MENUITEM_H

/* ---------------------------- included header files ---------------------- */
#include "libs/defaults.h"
#include "fvwm.h"

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

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
#define MI_IS_TEAR_OFF_BAR(i)   ((i)->flags.is_tear_off_bar)
#define MI_IS_TITLE(i)          ((i)->flags.is_title)
#define MI_IS_TITLE_CENTERED(i) ((i)->flags.is_title_centered)
#define MI_IS_POPUP(i)          ((i)->flags.is_popup)
#define MI_IS_MENU(i)           ((i)->flags.is_menu)
#define MI_IS_CONTINUATION(i)   ((i)->flags.is_continuation)
#define MI_HAS_TEXT(i)          ((i)->flags.has_text)
#define MI_HAS_PICTURE(i)       ((i)->flags.has_picture)
#define MI_HAS_HOTKEY(i)        ((i)->flags.has_hotkey)
#define MI_IS_HOTKEY_AUTOMATIC(i) ((i)->flags.is_hotkey_automatic)
#define MI_IS_SELECTABLE(i)     ((i)->flags.is_selectable)
/* temporary flags */
#define MI_WAS_DESELECTED(i)    ((i)->flags.was_deselected)

/* ---------------------------- forward declarations ----------------------- */

struct MenuStyle;

/* ---------------------------- type definitions --------------------------- */

/* IMPORTANT NOTE: Don't put members into this struct that can change while the
 * menu is visible! This will wreak havoc on recursive menus. */
typedef struct MenuItem
{
	/* next and prev menu items */
	struct MenuItem *next;
	struct MenuItem *prev;

	/* the strings displayed in the item */
	char *label[MAX_MENU_ITEM_LABELS];
	/* witdh of label[i] */
	int label_offset[MAX_MENU_ITEM_LABELS];
	/* strlen(label[i]) */
	int label_strlen[MAX_MENU_ITEM_LABELS];

	/* Pixmap to show above label*/
	FvwmPicture *picture;
	/* Pics to show left/right of label */
	FvwmPicture *lpicture[MAX_MENU_ITEM_MINI_ICONS];

	/* y offset and height for item */
	int y_offset;
	int height;

	/* action to be performed */
	char *action;
	/* type of built in function */
	short func_type;
	/* Hot key offset (pete@tecc.co.uk). */
	int hotkey_coffset;
	/* The column number the hotkey is defined in*/
	char hotkey_column;
	struct
	{
		unsigned is_continuation : 1;
		unsigned is_separator : 1;
		unsigned is_tear_off_bar : 1;
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

typedef struct MenuItemPartSizesT
{
	int label_width[MAX_MENU_ITEM_LABELS];
	int icon_width[MAX_MENU_ITEM_MINI_ICONS];
	int picture_width;
	int triangle_width;
	int title_width;
} MenuItemPartSizesT;

typedef struct MenuPaintItemParameters
{
	struct MenuStyle *ms;
	Window w;
	struct MenuItem *selected_item;
	struct MenuDimensions *dim;
	FvwmWindow *fw;
	XEvent *ev;
	int used_mini_icons;
	struct MenuRoot *cb_mr;
	/* number of item labels present in the item format */
	Bool (*cb_reset_bg)(struct MenuRoot *mr, XEvent *pevent);
	struct
	{
		unsigned is_first_item : 1;
		unsigned is_left_triangle : 1;
	} flags;
} MenuPaintItemParameters;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

struct MenuItem *menuitem_clone(struct MenuItem *mi);
struct MenuItem *menuitem_create(void);
void menuitem_free(struct MenuItem *mi);
void menuitem_get_size(
	struct MenuItem *mi, struct MenuItemPartSizesT *mipst,
	FlocaleFont *font, Bool do_reverse_icon_order);
void menuitem_paint(
	struct MenuItem *mi, struct MenuPaintItemParameters *mpip);
int menuitem_middle_y_offset(struct MenuItem *mi, struct MenuStyle *ms);

#endif /* FVWM_MENUITEM_H */
