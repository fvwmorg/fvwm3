/****************************************************************************
 * This module was originally based on the twm module of the same name.
 * Since its use and contents have changed so dramatically, I have removed
 * the original twm copyright, and inserted my own.
 *
 * by Rob Nation
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/

/**********************************************************************
 *
 * Codes for fvwm builtins
 *
 **********************************************************************/

#ifndef _PARSE_
#define _PARSE_

enum {
    F_NOP = 0,
    F_BEEP,
    F_QUIT,
    F_RESTART,
    F_REFRESH,
    F_TITLE,
    F_SCROLL,
    F_CIRCULATE_UP,
    F_CIRCULATE_DOWN,
    F_TOGGLE_PAGE,
    F_GOTO_PAGE,
    F_WINDOWLIST,
    F_MOVECURSOR,
    F_FUNCTION,
    F_MODULE = 15,
    F_DESK,
    F_CHANGE_WINDOWS_DESK,
    F_EXEC,
    F_POPUP,
    F_WAIT,
    F_CLOSE,
    F_SET_MASK,
    F_ADDMENU,
    F_ADDFUNC,
    F_STYLE,
    F_EDGE_SCROLL,
    F_PIXMAP_PATH,
    F_ICON_PATH,
    F_MODULE_PATH,
    F_HICOLOR,
    F_SETDESK,
    F_MOUSE,
    F_KEY,
    F_OPAQUE,
    F_XOR,
    F_CLICK,
    F_MENUSTYLE,
    F_ICONFONT,
    F_WINDOWFONT,
    F_EDGE_RES,
    F_BUTTON_STYLE,
    F_READ,
    F_ADDMENU2,
    F_DIRECTION,
    F_NEXT,
    F_PREV,
    F_NONE,
    F_STAYSUP,
    F_RECAPTURE,
    F_CONFIG_LIST,
    F_DESTROY_MENU,
    F_ZAP,
    F_QUIT_SCREEN,
    F_COLORMAP_FOCUS,
    F_TITLESTYLE,
    F_EXEC_SETUP,
    F_CURSOR_STYLE,
    F_CURRENT,
    F_SETENV,
    F_SET_ANIMATION,
    F_CHANGE_MENUSTYLE,
    F_DESTROY_MENUSTYLE,
    F_SNAP_ATT,
    F_SNAP_GRID,
    F_DFLT_FONT,
    F_DFLT_COLORS,
    F_GLOBAL_OPTS,
    F_EMULATE,

    F_RESIZE = 100,
    F_RAISE,
    F_LOWER,
    F_DESTROY,
    F_DELETE,
    F_MOVE,
    F_MOVE_TO_PAGE,
    F_ICONIFY,
    F_STICK,
    F_RAISELOWER,
    F_MAXIMIZE,
    F_FOCUS,
    F_WARP,
    F_SEND_STRING,
    F_ADD_MOD,
    F_DESTROY_MOD,
    F_FLIP_FOCUS,
    F_ECHO,
    F_BORDERSTYLE,
    F_WINDOWID,
    F_ADD_BUTTON_STYLE,
    F_ADD_TITLE_STYLE,
    F_ADD_DECOR,
    F_CHANGE_DECOR,
    F_DESTROY_DECOR,
    F_UPDATE_DECOR,
    F_WINDOW_SHADE,
    F_COLOR_LIMIT,
    F_ANIMATED_MOVE,

    F_END_OF_LIST = 999,

/* Functions for use by modules only! */
    F_SEND_WINDOW_LIST = 1000

/* Functions for internal  only! */
    /* F_RAISE_IT = 2000 */
};

#endif /* _PARSE_ */

