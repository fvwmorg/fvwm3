/* -*-c-*- */

#ifndef MENUSTYLE_H
#define MENUSTYLE_H

/* ---------------------------- included header files ---------------------- */

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

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
#define ST_DO_HILIGHT_BACK(s)         ((s)->look.flags.do_hilight_back)
#define MST_DO_HILIGHT_BACK(m)        ((m)->s->ms->look.flags.do_hilight_back)
#define ST_DO_HILIGHT_FORE(s)         ((s)->look.flags.do_hilight_fore)
#define MST_DO_HILIGHT_FORE(m)        ((m)->s->ms->look.flags.do_hilight_fore)
#define ST_DO_HILIGHT_TITLE_BACK(s)   ((s)->look.flags.do_hilight_title_back)
#define MST_DO_HILIGHT_TITLE_BACK(m)			\
	((m)->s->ms->look.flags.do_hilight_title_back)
#define ST_HAS_ACTIVE_FORE(s)         ((s)->look.flags.has_active_fore)
#define MST_HAS_ACTIVE_FORE(m)        ((m)->s->ms->look.flags.has_active_fore)
#define ST_HAS_ACTIVE_BACK(s)         ((s)->look.flags.has_active_back)
#define MST_HAS_ACTIVE_BACK(m)        ((m)->s->ms->look.flags.has_active_back)
#define ST_HAS_STIPPLE_FORE(s)        ((s)->look.flags.has_stipple_fore)
#define MST_HAS_STIPPLE_FORE(m)       ((m)->s->ms->look.flags.has_stipple_fore)
#define ST_HAS_LONG_SEPARATORS(s)     ((s)->look.flags.has_long_separators)
#define MST_HAS_LONG_SEPARATORS(m)			\
	((m)->s->ms->look.flags.has_long_separators)
#define ST_HAS_TRIANGLE_RELIEF(s)     ((s)->look.flags.has_triangle_relief)
#define MST_HAS_TRIANGLE_RELIEF(m)			\
	((m)->s->ms->look.flags.has_triangle_relief)
#define ST_HAS_SIDE_COLOR(s)          ((s)->look.flags.has_side_color)
#define MST_HAS_SIDE_COLOR(m)         ((m)->s->ms->look.flags.has_side_color)
#define ST_HAS_MENU_CSET(s)           ((s)->look.flags.has_menu_cset)
#define MST_HAS_MENU_CSET(m)          ((m)->s->ms->look.flags.has_menu_cset)
#define ST_HAS_ACTIVE_CSET(s)         ((s)->look.flags.has_active_cset)
#define MST_HAS_ACTIVE_CSET(m)        ((m)->s->ms->look.flags.has_active_cset)
#define ST_HAS_GREYED_CSET(s)         ((s)->look.flags.has_greyed_cset)
#define MST_HAS_GREYED_CSET(m)        ((m)->s->ms->look.flags.has_greyed_cset)
#define ST_HAS_TITLE_CSET(s)          ((s)->look.flags.has_title_cset)
#define MST_HAS_TITLE_CSET(m)         ((m)->s->ms->look.flags.has_title_cset)
#define ST_IS_ITEM_RELIEF_REVERSED(s) ((s)->look.flags.is_item_relief_reversed)
#define MST_IS_ITEM_RELIEF_REVERSED(m)				\
	((m)->s->ms->look.flags.is_item_relief_reversed)
#define ST_USING_DEFAULT_FONT(s)      ((s)->look.flags.using_default_font)
#define MST_USING_DEFAULT_FONT(m)			\
	((m)->s->ms->look.flags.using_default_font)
#define ST_USING_DEFAULT_TITLEFONT(s) ((s)->look.flags.using_default_titlefont)
#define MST_USING_DEFAULT_TITLEFONT(m)				\
	((m)->s->ms->look.flags.using_default_titlefont)
#define ST_TRIANGLES_USE_FORE(s)      ((s)->look.flags.triangles_use_fore)
#define MST_TRIANGLES_USE_FORE(m)			\
	((m)->s->ms->look.flags.triangles_use_fore)
#define ST_RELIEF_THICKNESS(s)        ((s)->look.ReliefThickness)
#define MST_RELIEF_THICKNESS(m)       ((m)->s->ms->look.ReliefThickness)
#define ST_TITLE_UNDERLINES(s)        ((s)->look.TitleUnderlines)
#define MST_TITLE_UNDERLINES(m)       ((m)->s->ms->look.TitleUnderlines)
#define ST_BORDER_WIDTH(s)            ((s)->look.BorderWidth)
#define MST_BORDER_WIDTH(m)           ((m)->s->ms->look.BorderWidth)
#define ST_ITEM_GAP_ABOVE(s)          ((s)->look.vertical_spacing.item_above)
#define MST_ITEM_GAP_ABOVE(m)				\
	((m)->s->ms->look.vertical_spacing.item_above)
#define ST_ITEM_GAP_BELOW(s)          ((s)->look.vertical_spacing.item_below)
#define MST_ITEM_GAP_BELOW(m)				\
	((m)->s->ms->look.vertical_spacing.item_below)
#define ST_TITLE_GAP_ABOVE(s)         ((s)->look.vertical_spacing.title_above)
#define MST_TITLE_GAP_ABOVE(m)				\
	((m)->s->ms->look.vertical_spacing.title_above)
#define ST_TITLE_GAP_BELOW(s)         ((s)->look.vertical_spacing.title_below)
#define MST_TITLE_GAP_BELOW(m)				\
	((m)->s->ms->look.vertical_spacing.title_below)
#define ST_SEPARATOR_GAP_ABOVE(s)			\
	((s)->look.vertical_spacing.separator_above)
#define MST_SEPARATOR_GAP_ABOVE(m)				\
	((m)->s->ms->look.vertical_spacing.separator_above)
#define ST_SEPARATOR_GAP_BELOW(s)			\
	((s)->look.vertical_spacing.separator_below)
#define MST_SEPARATOR_GAP_BELOW(m)				\
	((m)->s->ms->look.vertical_spacing.separator_below)
#define ST_CSET_MENU(s)               ((s)->look.cset.menu)
#define MST_CSET_MENU(m)              ((m)->s->ms->look.cset.menu)
#define ST_CSET_ACTIVE(s)             ((s)->look.cset.active)
#define MST_CSET_ACTIVE(m)            ((m)->s->ms->look.cset.active)
#define ST_CSET_TITLE(s)              ((s)->look.cset.title)
#define MST_CSET_TITLE(m)             ((m)->s->ms->look.cset.title)
#define ST_CSET_GREYED(s)             ((s)->look.cset.greyed)
#define MST_CSET_GREYED(m)            ((m)->s->ms->look.cset.greyed)
#define ST_SIDEPIC(s)                 ((s)->look.side_picture)
#define MST_SIDEPIC(m)                ((m)->s->ms->look.side_picture)
#define ST_SIDE_COLOR(s)              ((s)->look.side_color)
#define MST_SIDE_COLOR(m)             ((m)->s->ms->look.side_color)
#define ST_MENU_ACTIVE_GCS(s)         ((s)->look.active_gcs)
#define MST_MENU_ACTIVE_GCS(m)        ((m)->s->ms->look.active_gcs)
#define ST_MENU_INACTIVE_GCS(s)       ((s)->look.inactive_gcs)
#define MST_MENU_INACTIVE_GCS(m)      ((m)->s->ms->look.inactive_gcs)
#define ST_MENU_STIPPLE_GCS(s)        ((s)->look.stipple_gcs)
#define MST_MENU_STIPPLE_GCS(m)       ((m)->s->ms->look.stipple_gcs)
#define ST_MENU_TITLE_GCS(s)          ((s)->look.title_gcs)
#define MST_MENU_TITLE_GCS(m)         ((m)->s->ms->look.title_gcs)
#define FORE_GC(g)                    ((g).fore_gc)
#define BACK_GC(g)                    ((g).back_gc)
#define HILIGHT_GC(g)                 ((g).hilight_gc)
#define SHADOW_GC(g)                  ((g).shadow_gc)
#define ST_MENU_STIPPLE_GC(s)         ((s)->look.MenuStippleGC)
#define MST_MENU_STIPPLE_GC(m)        ((m)->s->ms->look.MenuStippleGC)
#define ST_MENU_COLORS(s)             ((s)->look.MenuColors)
#define MST_MENU_COLORS(m)            ((m)->s->ms->look.MenuColors)
#define ST_MENU_ACTIVE_COLORS(s)      ((s)->look.MenuActiveColors)
#define MST_MENU_ACTIVE_COLORS(m)     ((m)->s->ms->look.MenuActiveColors)
#define ST_MENU_STIPPLE_COLORS(s)     ((s)->look.MenuStippleColors)
#define MST_MENU_STIPPLE_COLORS(m)    ((m)->s->ms->look.MenuStippleColors)
#define ST_PSTDFONT(s)                ((s)->look.pStdFont)
#define MST_PSTDFONT(m)               ((m)->s->ms->look.pStdFont)
#define ST_PTITLEFONT(s)              ((s)->look.pTitleFont)
#define MST_PTITLEFONT(m)             ((m)->s->ms->look.pTitleFont)
#define ST_FONT_HEIGHT(s)             ((s)->look.FontHeight)
#define MST_FONT_HEIGHT(m)            ((m)->s->ms->look.FontHeight)
/* feel */
#define ST_IS_ANIMATED(s)             ((s)->feel.flags.is_animated)
#define MST_IS_ANIMATED(m)            ((m)->s->ms->feel.flags.is_animated)
#define ST_DO_POPUP_IMMEDIATELY(s)    ((s)->feel.flags.do_popup_immediately)
#define MST_DO_POPUP_IMMEDIATELY(m)			\
	((m)->s->ms->feel.flags.do_popup_immediately)
#define ST_DO_POPDOWN_IMMEDIATELY(s)  ((s)->feel.flags.do_popdown_immediately)
#define MST_DO_POPDOWN_IMMEDIATELY(m)			\
	((m)->s->ms->feel.flags.do_popdown_immediately)
#define ST_DO_WARP_TO_TITLE(s)        ((s)->feel.flags.do_warp_to_title)
#define MST_DO_WARP_TO_TITLE(m)       ((m)->s->ms->feel.flags.do_warp_to_title)
#define ST_DO_POPUP_AS(s)             ((s)->feel.flags.do_popup_as)
#define MST_DO_POPUP_AS(m)            ((m)->s->ms->feel.flags.do_popup_as)
#define ST_DO_UNMAP_SUBMENU_ON_POPDOWN(s)		\
	((s)->feel.flags.do_unmap_submenu_on_popdown)
#define MST_DO_UNMAP_SUBMENU_ON_POPDOWN(m)			\
	((m)->s->ms->feel.flags.do_unmap_submenu_on_popdown)
#define ST_USE_LEFT_SUBMENUS(s)       ((s)->feel.flags.use_left_submenus)
#define MST_USE_LEFT_SUBMENUS(m)			\
	((m)->s->ms->feel.flags.use_left_submenus)
#define ST_USE_AUTOMATIC_HOTKEYS(s)   ((s)->feel.flags.use_automatic_hotkeys)
#define MST_USE_AUTOMATIC_HOTKEYS(m)			\
	((m)->s->ms->feel.flags.use_automatic_hotkeys)
#define ST_MOUSE_WHEEL(s)             ((s)->feel.flags.mouse_wheel)
#define MST_MOUSE_WHEEL(m)            ((m)->s->ms->feel.flags.mouse_wheel)
#define ST_SCROLL_OFF_PAGE(s)         ((s)->feel.flags.scroll_off_page)
#define MST_SCROLL_OFF_PAGE(m)        ((m)->s->ms->feel.flags.scroll_off_page)
#define ST_FLAGS(s)                   ((s)->feel.flags)
#define MST_FLAGS(m)                  ((m)->s->ms->feel.flags)
#define ST_POPUP_OFFSET_PERCENT(s)    ((s)->feel.PopupOffsetPercent)
#define MST_POPUP_OFFSET_PERCENT(m)   ((m)->s->ms->feel.PopupOffsetPercent)
#define ST_POPUP_OFFSET_ADD(s)        ((s)->feel.PopupOffsetAdd)
#define MST_POPUP_OFFSET_ADD(m)       ((m)->s->ms->feel.PopupOffsetAdd)
#define ST_ACTIVE_AREA_PERCENT(s)		\
	((s)->feel.ActiveAreaPercent)
#define MST_ACTIVE_AREA_PERCENT(m)		\
	((m)->s->ms->feel.ActiveAreaPercent)
#define ST_POPDOWN_DELAY(s)           ((s)->feel.PopdownDelay10ms)
#define MST_POPDOWN_DELAY(m)          ((m)->s->ms->feel.PopdownDelay10ms)
#define ST_POPUP_DELAY(s)             ((s)->feel.PopupDelay10ms)
#define MST_POPUP_DELAY(m)            ((m)->s->ms->feel.PopupDelay10ms)
#define ST_DOUBLE_CLICK_TIME(s)       ((s)->feel.DoubleClickTime)
#define MST_DOUBLE_CLICK_TIME(m)      ((m)->s->ms->feel.DoubleClickTime)
#define ST_ITEM_FORMAT(s)             ((s)->feel.item_format)
#define MST_ITEM_FORMAT(m)            ((m)->s->ms->feel.item_format)
#define ST_SELECT_ON_RELEASE_KEY(s)   ((s)->feel.select_on_release_key)
#define MST_SELECT_ON_RELEASE_KEY(m)  ((m)->s->ms->feel.select_on_release_key)

/* ---------------------------- type definitions --------------------------- */

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

typedef enum
{
	MDP_POST_MENU = 0,
	MDP_ROOT_MENU = 1,
	MDP_IGNORE = 2,
	MDP_CLOSE = 3
} ms_do_popup_as_t;

typedef enum
{
	MMW_OFF = 0,
	MMW_MENU_BACKWARDS = 1,
	MMW_MENU = 2,
	MMW_POINTER = 3
} ms_mouse_wheel_t;

typedef struct MenuFeel
{
	struct
	{
		unsigned is_animated : 1;
		unsigned do_popdown_immediately : 1;
		unsigned do_popup_immediately : 1;
		unsigned do_popup_as : 2;
		unsigned do_warp_to_title : 1;
		unsigned do_unmap_submenu_on_popdown : 1;
		unsigned use_left_submenus : 1;
		unsigned use_automatic_hotkeys : 1;
		unsigned mouse_wheel : 2;
		unsigned scroll_off_page : 1;
	} flags;
	int PopdownDelay10ms;
	int PopupOffsetPercent;
	int ActiveAreaPercent;
	int PopupOffsetAdd;
	int PopupDelay10ms;
	int DoubleClickTime;
	char *item_format;
	KeyCode select_on_release_key;
} MenuFeel;

typedef struct MenuFace
{
	union
	{
		FvwmPicture *p;
		Pixel back;
		struct
		{
			int npixels;
			XColor *xcs;
			Bool do_dither;
		} grad;
	} u;
	MenuFaceType type;
	char gradient_type;
} MenuFace;

typedef struct
{
	GC fore_gc;
	GC back_gc;
	GC hilight_gc;
	GC shadow_gc;
} gc_quad_t;

typedef struct MenuLook
{
	MenuFace face;
	struct
	{
		unsigned do_hilight_back : 1;
		unsigned do_hilight_fore : 1;
		unsigned has_active_fore : 1;
		unsigned has_active_back : 1;
		unsigned has_stipple_fore : 1;
		unsigned has_long_separators : 1;
		unsigned has_triangle_relief : 1;
		unsigned has_side_color : 1;
		unsigned has_menu_cset : 1;
		unsigned has_active_cset : 1;
		unsigned has_greyed_cset : 1;
		unsigned is_item_relief_reversed : 1;
		unsigned using_default_font : 1;
		unsigned triangles_use_fore : 1;
		unsigned has_title_cset : 1;
		unsigned do_hilight_title_back : 1;
		unsigned using_default_titlefont : 1;
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
		int title;
	} cset;
	FvwmPicture *side_picture;
	Pixel side_color;
	gc_quad_t inactive_gcs;
	gc_quad_t active_gcs;
	gc_quad_t stipple_gcs;
	gc_quad_t title_gcs;
	ColorPair MenuColors;
	ColorPair MenuActiveColors;
	ColorPair MenuStippleColors;
	FlocaleFont *pStdFont;
	FlocaleFont *pTitleFont;
	int FontHeight;
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

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

void menustyle_free(MenuStyle *ms);
MenuStyle *menustyle_find(char *name);
void menustyle_update(MenuStyle *ms);
MenuStyle *menustyle_parse_style(F_CMD_ARGS);
MenuStyle *menustyle_get_default_style(void);
void menustyle_copy(MenuStyle *origms, MenuStyle *destms);

#endif /* MENUSTYLE_H */
