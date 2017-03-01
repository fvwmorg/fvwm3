/* -*-c-*- */

/* File: defaults.h
 *
 * Description:
 *      #defines for default values shall go into this file as well as tunable
 *      parameters.
 */

#ifndef _DEFAULTS_
#define _DEFAULTS_

/*** event handling ***/
#define CLOCK_SKEW_MS                  30000 /* ms */

/*** grabbing the pointer ***/
#define NUMBER_OF_GRAB_ATTEMPTS          100
#define TIME_BETWEEN_GRAB_ATTEMPTS        10 /* ms */

/*** bindings and mouse buttons ***/
/* Fvwm needs at least 3 buttons. X currently supports up to 5 buttons, so you
 * can use 3, 4 or 5 here.  Do not set this to values higher than 5!  Use the
 * next macro for that. */
#define NUMBER_OF_MOUSE_BUTTONS            5
/* The "extended" buttons do not provide the full functionality because X has
 * no bit mask value for them.  Things like dragging windows don't work with
 * them. */
#define NUMBER_OF_EXTENDED_MOUSE_BUTTONS   9
#if NUMBER_OF_EXTENDED_MOUSE_BUTTONS > 31
#error No more than 31 mouse buttons can be supported on 32 bit platforms
#endif
#define DEFAULT_ALL_BUTTONS_MASK           \
        ((Button1Mask * ((1 << NUMBER_OF_MOUSE_BUTTONS) - 1)))
#define DEFAULT_ALL_BUTTONS_MOTION_MASK    \
        ((Button1MotionMask * ((1 << NUMBER_OF_MOUSE_BUTTONS) - 1)))
#define DEFAULT_MODS_UNUSED                LockMask


/*
 * These values may be adjusted to fine tune the menu looks.
 */
/* The first option of the default menu style must be fvwm/win/mwm/....
 * fvwm may crash if not. */
#define DEFAULT_MENU_STYLE \
        "MenuStyle * fvwm, Foreground black, Background grey, " \
        "Greyed slategrey, MenuColorset, ActiveColorset, GreyedColorset"
#define DEFAULT_CLICKTIME                150 /* ms */
#define DEFAULT_POPUP_DELAY               15 /* ms*10 */
#define DEFAULT_POPDOWN_DELAY             15 /* ms*10 */
#define DEFAULT_MENU_CLICKTIME             (3*DEFAULT_CLICKTIME)
#define DEFAULT_MOVE_THRESHOLD             3 /* pixels */
#define MAX_MENU_COPIES                   10 /* menu copies */
#define MAX_MENU_ITEM_LABELS               3 /* labels (max. 15) */
#define MAX_MENU_ITEM_MINI_ICONS           2 /* mini icons (max. 15) */
#define DEFAULT_MENU_BORDER_WIDTH          2 /* pixels */
#define MAX_MENU_BORDER_WIDTH             50 /* pixels */
#define MAX_MENU_ITEM_RELIEF_THICKNESS    50 /* pixels */
#define PARENT_MENU_FORCE_VISIBLE_WIDTH   20 /* pixels */
#define DEFAULT_MENU_POPUP_NOW_RATIO      75 /* % of item width */
/*   default item formats for left and right submenus. */
#define DEFAULT_MENU_ITEM_FORMAT          "%s%.1|%.5i%.5l%.5l%.5r%.5i%2.3>%1|"
#define DEFAULT_LEFT_MENU_ITEM_FORMAT     "%.1|%3.2<%5i%5l%5l%5r%5i%1|%s"
/*   size of the submenu triangle. */
#define MENU_TRIANGLE_WIDTH                5 /* pixels */
#define MENU_TRIANGLE_HEIGHT               9 /* pixels */
/*  menu underline parameters */
#define MENU_UNDERLINE_THICKNESS           1 /* pixels */
#define MENU_UNDERLINE_GAP                 1 /* pixels */
#define MENU_UNDERLINE_HEIGHT  (MENU_UNDERLINE_THICKNESS + MENU_UNDERLINE_GAP)
/*   menu separator parameters */
#define MENU_SEPARATOR_SHORT_X_OFFSET      5 /* pixels */
#define MENU_SEPARATOR_Y_OFFSET            2 /* pixels */
#define MENU_SEPARATOR_HEIGHT              2 /* pixels */
#define MENU_SEPARATOR_TOTAL_HEIGHT    \
          (MENU_SEPARATOR_HEIGHT + MENU_SEPARATOR_Y_OFFSET)
/*   menu tear off bar parameters */
#define MENU_TEAR_OFF_BAR_X_OFFSET         1 /* pixels */
#define MENU_TEAR_OFF_BAR_Y_OFFSET         1 /* pixels */
#define MENU_TEAR_OFF_BAR_HEIGHT           4 /* pixels */
#define MENU_TEAR_OFF_BAR_DASH_WIDTH       5 /* pixels */
/*   gap above item text */
#define DEFAULT_MENU_ITEM_TEXT_Y_OFFSET    1 /* pixels */
/*   gap below item text */
#define DEFAULT_MENU_ITEM_TEXT_Y_OFFSET2   2 /* pixels */
/*   same for titles */
#define DEFAULT_MENU_TITLE_TEXT_Y_OFFSET   (DEFAULT_MENU_ITEM_TEXT_Y_OFFSET)
#define DEFAULT_MENU_TITLE_TEXT_Y_OFFSET2  (DEFAULT_MENU_ITEM_TEXT_Y_OFFSET2)
/* minimum for above value */
#define MIN_VERTICAL_SPACING            -100 /* pixels */
/* maximum for above value */
#define MAX_VERTICAL_SPACING             100 /* pixels */
/* maximum for above value */
#define MAX_MENU_MARGIN                   50 /* pixels */
/* width of a tab in the item format of a menu */
#define MENU_TAB_WIDTH                     8 /* spaces */
/* This is the tile width or height for V and H gradients. I guess this should
 * better be a power of two. A value of 5 definitely causes XFree 3.3.3.1 to
 * screw up the V_GRADIENT on an 8 bit display, but 4, 6, 7 etc. work well. */
#define DEFAULT_MENU_GRADIENT_PIXMAP_THICKNESS 4

/*** colours ***/
#define DEFAULT_FORE_COLOR                "black"
#define DEFAULT_BACK_COLOR                "gray"
#define DEFAULT_HILIGHT_COLOR             "white"
#define DEFAULT_SHADOW_COLOR              "black"
#define DEFAULT_CURSOR_FORE_COLOR         "black"
#define DEFAULT_CURSOR_BACK_COLOR         "white"

/*** pager ***/
#define DEFAULT_MOVE_THRESHOLD             3 /* pixels */
#define DEFAULT_PAGER_WINDOW_BORDER_WIDTH  1 /* pixels */

/*** fonts ***/
#define EXTRA_TITLE_FONT_HEIGHT            3 /* pixels */
#define EXTRA_TITLE_FONT_WIDTH             3 /* pixels */
#define MIN_FONT_HEIGHT                    (EXTRA_TITLE_FONT_HEIGHT + 2)
#define MAX_FONT_HEIGHT                  256 /* pixels */

/*** Flocale ***/
/* adding the good number of "*" speed-up font loading with certain X
 * server */
#define FLOCALE_MB_FALLBACK_FONT \
        "-*-fixed-medium-r-semicondensed-*-13-*-*-*-*-*-*-*," \
        "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*," \
        "-*-*-medium-r-normal-*-16-*-*-*-*-*-*-*"
/* rationale: -*-fixed-medium-r-semicondensed-*-13-* should gives "fixed" in
   most non multibyte charset (?).
   -*-fixed-medium-r-normal-*-14-* is ok for jsx
   -*-medium-r-normal-*-16-* is for gb
   Of course this is for XFree. Any help for other X server basic font set
   is welcome.
*/
#define FLOCALE_FALLBACK_FONT             "fixed"
#define FLOCALE_FFT_FALLBACK_FONT         "MonoSpace"

#define FLOCALE_NUMBER_MISS_CSET_ERR_MSG   5

/*** window geometry ***/
#define DEFAULT_MIN_MAX_WINDOW_WIDTH     100 /* pixels */
#define DEFAULT_MIN_MAX_WINDOW_HEIGHT    100 /* pixels */
#define DEFAULT_MAX_MAX_WINDOW_WIDTH   32767 /* pixels */
#define DEFAULT_MAX_MAX_WINDOW_HEIGHT  32767 /* pixels */

/*** icon geometry ***/
#define UNSPECIFIED_ICON_DIMENSION        -1
#define MIN_ALLOWABLE_ICON_DIMENSION       0 /* pixels */
#define MAX_ALLOWABLE_ICON_DIMENSION     255 /* pixels */

/* this value is used in a bugfix */
#define WINDOW_FREAKED_OUT_SIZE        65500 /* pixels */
/* Used in Workaround for broken min and max size hints vs a size request */
#define BROKEN_MAXSIZE_LIMIT 1
#define BROKEN_MINSIZE_LIMIT 30000

/* geometry window */
#define GEOMETRY_WINDOW_BW                 2 /* pixels */
#define GEOMETRY_WINDOW_STRING             " +8888 x +8888 "
#define GEOMETRY_WINDOW_POS_STRING         " %+-4d %+-4d "
#define GEOMETRY_WINDOW_SIZE_STRING        " %4d x %-4d "

/*
 * window title layout
 */
/* the title buttons are shrunk if the title would become smaller than this
 * number of pixels */
#define MIN_WINDOW_TITLE_LENGTH      10 /* pixels */
/* title bar buttons that get smaller than this size are hidden */
#define MIN_WINDOW_TITLEBUTTON_LENGTH 2 /* pixels */
/* height of stick lines */
#define WINDOW_TITLE_STICK_HEIGHT     1 /* pixels */
/* vertical distance between stick lines */
#define WINDOW_TITLE_STICK_VERT_DIST  4 /* pixels */
/* minimum width of stick lines */
#define WINDOW_TITLE_STICK_MIN_WIDTH  1 /* pixels */
/* gap between border of title window and stick lines */
#define WINDOW_TITLE_STICK_OFFSET     4 /* pixels */
/* gap between title and stick lines */
#define WINDOW_TITLE_TO_STICK_GAP     4 /* pixels */
/* gap between border of title window and text */
#define WINDOW_TITLE_TEXT_OFFSET     10 /* pixels */
/* minimal gap between border of title window and text */
#define MIN_WINDOW_TITLE_TEXT_OFFSET  2 /* pixels */
/* left and right padding for the text into under text area (MultiPixmap)*/
#define TBMP_TITLE_PADDING            5
/* minimal left and right area vs left/right of text/end (MultiPixmap) */
#define TBMP_MIN_RL_TITLE_LENGTH      7
/* maximum number of segemnts in a vector button */
#define MAX_TITLE_BUTTON_VECTOR_LINES 10000

/*** window placement ***/
/** MinOverlap(Percent)Placement **/
/** Now these values are configurable by using styles **/
/* The following factors represent the amount of area that these types of
 * windows are counted as.  For example, by default the area of ONTOP windows
 * is counted 5 times as much as normal windows.  So CleverPlacement will
 * cover 5 times as much area of another window before it will cover an ONTOP
 * window.  To treat ONTOP windows the same as other windows, set this to 1.
 * To really, really avoid putting windows under ONTOP windows, set this to a
 * high value, say 1000.  A value of 5 will try to avoid ONTOP windows if
 * practical, but if it saves a reasonable amount of area elsewhere, it will
 * place one there.  The same rules apply for the other "AVOID" factors. */
/* With the advent of layers, the meaning of ONTOP in the following
   explanation has changed to mean any window in a higher layer. */
#define PLACEMENT_AVOID_BELOW              0.05
#define PLACEMENT_AVOID_STICKY             1.0
#define PLACEMENT_AVOID_ONTOP              5
#define PLACEMENT_AVOID_ICON              10
#define PLACEMENT_AVOID_EWMH_STRUT        50
/* used in MinOverlap*Placement to forbid complete covering (99%, 95%
   85% and 75%) of windows */
#define PLACEMENT_AVOID_COVER_99          12
#define PLACEMENT_AVOID_COVER_95           6
#define PLACEMENT_AVOID_COVER_85           4
#define PLACEMENT_AVOID_COVER_75           1
#define PLACEMENT_FALLBACK_CASCADE_STEP   20
/** Other placement related values **/
/* default string for position placement */
#define DEFAULT_PLACEMENT_POSITION_STRING   "0 0"
#define DEFAULT_PLACEMENT_POS_CENTER_STRING "50-50w 50-50w"
#define DEFAULT_PLACEMENT_POS_MOUSE_STRING  "m-50w m-50w"

/*** icon layout ***/
/* width of the relief around the icon and icon title */
#define ICON_RELIEF_WIDTH                  2 /* pixels */
/* height of stick lines */
#define ICON_TITLE_STICK_HEIGHT            WINDOW_TITLE_STICK_HEIGHT
/* vertical distance between stick lines */
#define ICON_TITLE_STICK_VERT_DIST         WINDOW_TITLE_STICK_VERT_DIST
/* vertical offset for icon title */
#define ICON_TITLE_VERT_TEXT_OFFSET       -3 /* pixels */
/* minimum width of stick lines */
#define ICON_TITLE_STICK_MIN_WIDTH         3 /* pixels */
/* number of blank pixels before and after a collapsed title */
#define ICON_TITLE_TEXT_GAP_COLLAPSED      1 /* pixels */
/* number of blank pixels before and after an expanded title */
#define ICON_TITLE_TEXT_GAP_EXPANDED       4 /* pixels */
/* extra blank pixels if the icon is sticky */
#define ICON_TITLE_TO_STICK_EXTRA_GAP      1 /* pixels */
/* minimum distance of icons in icon box */
#define MIN_ICON_BOX_DIST                  3 /* pixels */
/* padding for the icon into its background */
#define ICON_BACKGROUND_PADDING            0 /* pixels */

/*** general keyboard shortcuts used in move, resize, ... ***/
#define DEFAULT_KDB_SHORTCUT_MOVE_DISTANCE 5 /* pixels */
#define KDB_SHORTCUT_MOVE_DISTANCE_SMALL   1 /* pixels */
#define KDB_SHORTCUT_MOVE_DISTANCE_BIG   100 /* pixels */

/*** movement ***/
#define DEFAULT_OPAQUE_MOVE_SIZE           5 /* percent of window area */
#define DEFAULT_SNAP_ATTRACTION            0 /* snap nothing */
#define DEFAULT_SNAP_ATTRACTION_MODE     0x3 /* snap all */
#define DEFAULT_SNAP_GRID_X                1 /* pixels */
#define DEFAULT_SNAP_GRID_Y                1 /* pixels */

/*** paging ***/
#define DEFAULT_EDGE_SCROLL              100 /* % of screen width/height */
/* Don't page if the pointer has moved for more than this many pixels between
 * two samples */
#define MAX_PAGING_MOVE_DISTANCE           5 /* pixels */
#define DEFAULT_MOVE_DELAY                 0 /* ms */
#define DEFAULT_RESIZE_DELAY               -1 /* pixels */
#define DEFAULT_SCROLL_DELAY               0 /* ms */

/*** layers ***/
#define DEFAULT_BOTTOM_LAYER               2
#define DEFAULT_DEFAULT_LAYER              4
#define DEFAULT_TOP_LAYER                  6
#define DEFAULT_ROOT_WINDOW_LAYER         -1

/*** decorations ***/
/* The number of left and right buttons must be equal.  A maximum of 32 buttons
 * can be handled (16 left and 16 right). */
#define NR_LEFT_BUTTONS                    5
#define NR_RIGHT_BUTTONS                   NR_LEFT_BUTTONS
#define NUMBER_OF_TITLE_BUTTONS            (NR_LEFT_BUTTONS + NR_RIGHT_BUTTONS)

/*** window borders ***/
#define DEFAULT_BORDER_WIDTH               1 /* pixels */
#define DEFAULT_HANDLE_WIDTH               7 /* pixels */
#define MAX_BORDER_WIDTH                 500 /* pixels */
#define MAX_HANDLE_WIDTH                   (MAX_BORDER_WIDTH)

/*** module configuration ***/
#define MAX_MODULE_ALIAS_LEN             250 /* bytes */
#define DEFAULT_MODULE_TIMEOUT            30 /* seconds */
#define MAX_MODULE_INPUT_TEXT_LEN       1000 /* bytes */

/*** FvwmConsole configuration */
/* Maximum time FvwmConsole waits for the client to connect. */
#define FVWMCONSOLE_CONNECTION_TO_SECS    60 /* seconds */


/*** misc ***/
#define DEFAULT_EMULATE_MWM            False
#define DEFAULT_EMULATE_WIN            False

#define DEFAULT_USE_ACTIVE_DOWN_BUTTONS   True
#define DEFAULT_USE_INACTIVE_BUTTONS      True
#define DEFAULT_USE_INACTIVE_DOWN_BUTTONS True

/*** Gradients ***/
#define MAX_GRADIENT_SEGMENTS           1000
#define MAX_GRADIENT_COLORS            10000

/*** Xinerama ***/
#define DEFAULT_XINERAMA_ENABLED        True /* Xinerama on by default */
#define XINERAMA_CONFIG_STRING             "XineramaConfig"
/* Replace with -1 to switch off "primary screen" concept by default */
#define DEFAULT_PRIMARY_SCREEN             0

/*** Very long window names (600000 characters or more) seem to hang the X
 *** server. ***/
#define MAX_WINDOW_NAME_LEN              200 /* characters */
#define MAX_ICON_NAME_LEN                200 /* characters */
/* not tested if this hangs the server too */
#define MAX_RESOURCE_LEN                 200 /* characters */
#define MAX_CLASS_LEN                    200 /* characters */

/* The default title and icon title in case the user doesn't supply one. */
#define DEFAULT_TITLE_FORMAT "%n"
#define DEFAULT_ICON_TITLE_FORMAT "%i"

/* Set the maximum size a visible name can be. */
#define MAX_VISIBLE_NAME_LEN		4096

/*** numbered window names ***/
#define MAX_WINDOW_NAME_NUMBER           999
#define MAX_WINDOW_NAME_NUMBER_DIGITS      3 /* number/digits of above number */

/* focus policy style defaults */
#define DEF_FP_FOCUS_ENTER                      1
#define DEF_FP_UNFOCUS_LEAVE                    1
#define DEF_FP_FOCUS_CLICK_CLIENT               0
#define DEF_FP_FOCUS_CLICK_DECOR                0
#define DEF_FP_FOCUS_BY_PROGRAM                 1
#define DEF_FP_FOCUS_BY_FUNCTION                1
#define DEF_FP_WARP_POINTER_ON_FOCUS_FUNC       0
#define DEF_FP_LENIENT                          0
#define DEF_FP_RAISE_FOCUSED_CLIENT_CLICK       0
#define DEF_FP_RAISE_UNFOCUSED_CLIENT_CLICK     1
#define DEF_FP_RAISE_FOCUSED_DECOR_CLICK        0
#define DEF_FP_RAISE_UNFOCUSED_DECOR_CLICK      0
#define DEF_FP_RAISE_FOCUSED_ICON_CLICK         0
#define DEF_FP_RAISE_UNFOCUSED_ICON_CLICK       0
/* use first three buttons by default */
#define DEF_FP_MOUSE_BUTTONS ( \
        ((1 << 0) | (1 << 1) | (1 << 2)) & \
        ((1 << NUMBER_OF_MOUSE_BUTTONS) - 1))
#define DEF_FP_MODIFIERS                        0
#define DEF_FP_PASS_FOCUS_CLICK                 1
#define DEF_FP_PASS_RAISE_CLICK                 1
#define DEF_FP_IGNORE_FOCUS_CLICK_MOTION        0
#define DEF_FP_IGNORE_RAISE_CLICK_MOTION        0
#define DEF_FP_ALLOW_FUNC_FOCUS_CLICK           1
#define DEF_FP_ALLOW_FUNC_RAISE_CLICK           1
#define DEF_FP_GRAB_FOCUS                       0
#define DEF_FP_GRAB_FOCUS_TRANSIENT             1
#define DEF_FP_OVERRIDE_GRAB_FOCUS              0
#define DEF_FP_RELEASE_FOCUS                    0
#define DEF_FP_RELEASE_FOCUS_TRANSIENT          1
#define DEF_FP_OVERRIDE_RELEASE_FOCUS           0
#define DEF_FP_SORT_WINDOWLIST_BY               0

/* Function execution */
#define MAX_FUNCTION_DEPTH    512

/* Tips */
#define FTIPS_DEFAULT_PLACEMENT FTIPS_PLACEMENT_AUTO_UPDOWN
#define FTIPS_DEFAULT_JUSTIFICATION  FTIPS_JUSTIFICATION_LEFT_UP

#define FTIPS_DEFAULT_PLACEMENT_OFFSET 3
#define FTIPS_DEFAULT_JUSTIFICATION_OFFSET 2

#define FTIPS_DEFAULT_BORDER_WIDTH 1

#endif /* _DEFAULTS_ */

