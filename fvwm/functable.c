/* -*-c-*- */
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

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include "fvwm.h"
#include "commands.h"
#include "cursor.h"
#include "functions.h"
#include "misc.h"
#include "functable.h"

/* ---------------------------- local definitions --------------------------- */

/* The function names in the first field *must* be in lowercase or else the
 * function cannot be called.  The func parameter of the macro is also used
 * for parsing by modules that rely on the old case of the commands.
 */
#define CMD_ENTRY(cmd, func, cmd_id, flags) \
	{ cmd, func, cmd_id, flags }

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

/* ---------------------------- exported variables (globals) ---------------- */

/* the next line must be blank for modules to properly parse this file */

const func_type func_table[] =
{
	CMD_ENTRY("+", CMD_Plus, F_ADDMENU2, 0),
	CMD_ENTRY("addbuttonstyle", CMD_AddButtonStyle, F_ADD_BUTTON_STYLE,
		  FUNC_DECOR),
	CMD_ENTRY("addtitlestyle", CMD_AddTitleStyle, F_ADD_TITLE_STYLE,
		  FUNC_DECOR),
#ifdef USEDECOR
	CMD_ENTRY("addtodecor", CMD_AddToDecor, F_ADD_DECOR, 0),
#endif /* USEDECOR */
	CMD_ENTRY("addtofunc", CMD_AddToFunc, F_ADDFUNC, FUNC_ADD_TO),
	CMD_ENTRY("addtomenu", CMD_AddToMenu, F_ADDMENU, 0),
	CMD_ENTRY("all", CMD_All, F_ALL, 0),
	CMD_ENTRY("animatedmove", CMD_AnimatedMove, F_ANIMATED_MOVE,
		  FUNC_NEEDS_WINDOW),
	CMD_ENTRY("any", CMD_Any, F_ANY, 0),
	CMD_ENTRY("beep", CMD_Beep, F_BEEP, 0),
	CMD_ENTRY("borderstyle", CMD_BorderStyle, F_BORDERSTYLE, FUNC_DECOR),
	CMD_ENTRY("break", CMD_Break, F_BREAK, 0),
	CMD_ENTRY("bugopts", CMD_BugOpts, F_BUG_OPTS, 0),
	CMD_ENTRY("busycursor", CMD_BusyCursor, F_BUSY_CURSOR, 0),
	CMD_ENTRY("buttonstate", CMD_ButtonState, F_BUTTON_STATE, 0),
	CMD_ENTRY("buttonstyle", CMD_ButtonStyle, F_BUTTON_STYLE, FUNC_DECOR),
#ifdef USEDECOR
	CMD_ENTRY("changedecor", CMD_ChangeDecor, F_CHANGE_DECOR,
		  FUNC_NEEDS_WINDOW),
#endif /* USEDECOR */
	CMD_ENTRY("changemenustyle", CMD_ChangeMenuStyle, F_CHANGE_MENUSTYLE,
		  0),
	CMD_ENTRY("cleanupcolorsets", CMD_CleanupColorsets, F_NOP, 0),
	CMD_ENTRY("clicktime", CMD_ClickTime, F_CLICK, 0),
	CMD_ENTRY("close", CMD_Close, F_CLOSE, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("colorlimit", CMD_ColorLimit, F_COLOR_LIMIT, 0),
	CMD_ENTRY("colormapfocus", CMD_ColormapFocus, F_COLORMAP_FOCUS, 0),
	CMD_ENTRY("colorset", CMD_Colorset, F_NOP, 0),
	CMD_ENTRY("cond", CMD_Cond, F_COND, 0),
	CMD_ENTRY("condcase", CMD_CondCase, F_CONDCASE, 0),
	CMD_ENTRY("copymenustyle", CMD_CopyMenuStyle, F_COPY_MENU_STYLE, 0),
	CMD_ENTRY("current", CMD_Current, F_CURRENT, 0),
	CMD_ENTRY("cursormove", CMD_CursorMove, F_MOVECURSOR, 0),
	CMD_ENTRY("cursorstyle", CMD_CursorStyle, F_CURSOR_STYLE, 0),
	CMD_ENTRY("defaultcolors", CMD_DefaultColors, F_DFLT_COLORS, 0),
	CMD_ENTRY("defaultcolorset", CMD_DefaultColorset, F_DFLT_COLORSET, 0),
	CMD_ENTRY("defaultfont", CMD_DefaultFont, F_DFLT_FONT, 0),
	CMD_ENTRY("defaulticon", CMD_DefaultIcon, F_DFLT_ICON, 0),
	CMD_ENTRY("defaultlayers", CMD_DefaultLayers, F_DFLT_LAYERS, 0),
	CMD_ENTRY("delete", CMD_Delete, F_DELETE, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("deschedule", CMD_Deschedule, F_DESCHEDULE, 0),
	CMD_ENTRY("desk", CMD_Desk, F_GOTO_DESK, 0),
	CMD_ENTRY("desktopname", CMD_DesktopName, F_DESKTOP_NAME, 0),
	CMD_ENTRY("desktopsize", CMD_DesktopSize, F_SETDESK, 0),
	CMD_ENTRY("destroy", CMD_Destroy, F_DESTROY, FUNC_NEEDS_WINDOW),
#ifdef USEDECOR
	CMD_ENTRY("destroydecor", CMD_DestroyDecor, F_DESTROY_DECOR, 0),
#endif /* USEDECOR */
	CMD_ENTRY("destroyfunc", CMD_DestroyFunc, F_DESTROY_FUNCTION, 0),
	CMD_ENTRY("destroymenu", CMD_DestroyMenu, F_DESTROY_MENU, 0),
	CMD_ENTRY("destroymenustyle", CMD_DestroyMenuStyle, F_DESTROY_MENUSTYLE,
		  0),
	CMD_ENTRY("destroymoduleconfig", CMD_DestroyModuleConfig, F_DESTROY_MOD,
		  0),
	CMD_ENTRY("destroystyle", CMD_DestroyStyle, F_DESTROY_STYLE, 0),
	CMD_ENTRY("direction", CMD_Direction, F_DIRECTION, 0),
	CMD_ENTRY("echo", CMD_Echo, F_ECHO, 0),
	CMD_ENTRY("edgecommand", CMD_EdgeCommand, F_EDGE_COMMAND, 0),
	CMD_ENTRY("edgeresistance", CMD_EdgeResistance, F_EDGE_RES, 0),
	CMD_ENTRY("edgescroll", CMD_EdgeScroll, F_EDGE_SCROLL, 0),
	CMD_ENTRY("edgethickness", CMD_EdgeThickness, F_NOP, 0),
	CMD_ENTRY("emulate", CMD_Emulate, F_EMULATE, 0),
	CMD_ENTRY("escapefunc", CMD_EscapeFunc, F_ESCAPE_FUNC, 0),
	CMD_ENTRY("ewmhbasestrut", CMD_EwmhBaseStrut, F_EWMH_BASE_STRUT, 0),
	CMD_ENTRY("ewmhnumberofdesktops", CMD_EwmhNumberOfDesktops,
		  F_EWMH_NUMBER_OF_DESKTOPS, 0),
	CMD_ENTRY("exec", CMD_Exec, F_EXEC, 0),
	CMD_ENTRY("execuseshell", CMD_ExecUseShell, F_EXEC_SETUP, 0),
	CMD_ENTRY("fakeclick", CMD_FakeClick, F_FAKE_CLICK, 0),
	CMD_ENTRY("flipfocus", CMD_FlipFocus, F_FLIP_FOCUS, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("focus", CMD_Focus, F_FOCUS, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("function", CMD_Function, F_FUNCTION, 0),
	CMD_ENTRY("globalopts", CMD_GlobalOpts, F_GLOBAL_OPTS, 0),
#ifdef GNOME
	CMD_ENTRY("gnomebutton", CMD_GnomeButton, F_MOUSE, 0),
	CMD_ENTRY("gnomeshowdesks", CMD_GnomeShowDesks, F_GOTO_DESK, 0),
#endif /* GNOME */
	CMD_ENTRY("gotodesk", CMD_GotoDesk, F_GOTO_DESK, 0),
	CMD_ENTRY("gotodeskandpage", CMD_GotoDeskAndPage, F_GOTO_DESK, 0),
	CMD_ENTRY("gotopage", CMD_GotoPage, F_GOTO_PAGE, 0),
	CMD_ENTRY("hidegeometrywindow", CMD_HideGeometryWindow,
		  F_HIDEGEOMWINDOW, 0),
	CMD_ENTRY("hilightcolor", CMD_HilightColor, F_HICOLOR, 0),
	CMD_ENTRY("hilightcolorset", CMD_HilightColorset, F_HICOLORSET, 0),
	CMD_ENTRY("iconfont", CMD_IconFont, F_ICONFONT, 0),
	CMD_ENTRY("iconify", CMD_Iconify, F_ICONIFY, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("iconpath", CMD_IconPath, F_ICON_PATH, 0),
	CMD_ENTRY("ignoremodifiers", CMD_IgnoreModifiers, F_IGNORE_MODIFIERS,
		  0),
	CMD_ENTRY("imagepath", CMD_ImagePath, F_IMAGE_PATH, 0),
	CMD_ENTRY("key", CMD_Key, F_KEY, 0),
	CMD_ENTRY("killmodule", CMD_KillModule, F_KILL_MODULE, 0),
	CMD_ENTRY("layer", CMD_Layer, F_LAYER, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("lower", CMD_Lower, F_LOWER, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("maximize", CMD_Maximize, F_MAXIMIZE, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("menu", CMD_Menu, F_STAYSUP, 0),
	CMD_ENTRY("menustyle", CMD_MenuStyle, F_MENUSTYLE, 0),
	CMD_ENTRY("module", CMD_Module, F_MODULE, 0),
	CMD_ENTRY("modulepath", CMD_ModulePath, F_MODULE_PATH, 0),
	CMD_ENTRY("modulesynchronous", CMD_ModuleSynchronous, F_MODULE_SYNC, 0),
	CMD_ENTRY("moduletimeout", CMD_ModuleTimeout, F_NOP, 0),
	CMD_ENTRY("mouse", CMD_Mouse, F_MOUSE, 0),
	CMD_ENTRY("move", CMD_Move, F_MOVE, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("movethreshold", CMD_MoveThreshold, F_MOVE_THRESHOLD, 0),
	CMD_ENTRY("movetodesk", CMD_MoveToDesk, F_MOVE_TO_DESK,
		  FUNC_NEEDS_WINDOW),
	CMD_ENTRY("movetopage", CMD_MoveToPage, F_MOVE_TO_PAGE,
		  FUNC_NEEDS_WINDOW),
	CMD_ENTRY("movetoscreen", CMD_MoveToScreen, F_MOVE_TO_SCREEN,
		  FUNC_NEEDS_WINDOW),
	CMD_ENTRY("next", CMD_Next, F_NEXT, 0),
	CMD_ENTRY("none", CMD_None, F_NONE, 0),
	CMD_ENTRY("nop", CMD_Nop, F_NOP, FUNC_DONT_REPEAT),
	CMD_ENTRY("opaquemovesize", CMD_OpaqueMoveSize, F_OPAQUE, 0),
	CMD_ENTRY("pick", CMD_Pick, F_PICK, 0),
	CMD_ENTRY("piperead", CMD_PipeRead, F_READ, 0),
	CMD_ENTRY("pixmappath", CMD_PixmapPath, F_PIXMAP_PATH, 0),
	CMD_ENTRY("placeagain", CMD_PlaceAgain, F_PLACEAGAIN,
		  FUNC_NEEDS_WINDOW),
	CMD_ENTRY("pointerkey", CMD_PointerKey, F_POINTERKEY, 0),
	CMD_ENTRY("pointerwindow", CMD_PointerWindow, F_POINTERWINDOW, 0),
	CMD_ENTRY("popup", CMD_Popup, F_POPUP, 0),
	CMD_ENTRY("prev", CMD_Prev, F_PREV, 0),
	CMD_ENTRY("propertychange", CMD_PropertyChange, F_NOP, 0),
	CMD_ENTRY("quit", CMD_Quit, F_QUIT, 0),
	CMD_ENTRY("quitscreen", CMD_QuitScreen, F_QUIT_SCREEN, 0),
	CMD_ENTRY("quitsession", CMD_QuitSession, F_QUIT_SESSION, 0),
	CMD_ENTRY("raise", CMD_Raise, F_RAISE, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("raiselower", CMD_RaiseLower, F_RAISELOWER,
		  FUNC_NEEDS_WINDOW),
	CMD_ENTRY("read", CMD_Read, F_READ, 0),
	CMD_ENTRY("readwritecolors", CMD_ReadWriteColors, F_NOP, 0),
	CMD_ENTRY("recapture", CMD_Recapture, F_RECAPTURE, 0),
	CMD_ENTRY("recapturewindow", CMD_RecaptureWindow, F_RECAPTURE_WINDOW,
		  0),
	CMD_ENTRY("refresh", CMD_Refresh, F_REFRESH, 0),
	CMD_ENTRY("refreshwindow", CMD_RefreshWindow, F_REFRESH,
		  FUNC_NEEDS_WINDOW),
	CMD_ENTRY(PRE_REPEAT, CMD_Repeat, F_REPEAT, FUNC_DONT_REPEAT),
	CMD_ENTRY("resize", CMD_Resize, F_RESIZE, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("resizemaximize", CMD_ResizeMaximize, F_RESIZE_MAXIMIZE,
		  FUNC_NEEDS_WINDOW),
	CMD_ENTRY("resizemove", CMD_ResizeMove, F_RESIZEMOVE,
		  FUNC_NEEDS_WINDOW),
	CMD_ENTRY("resizemovemaximize", CMD_ResizeMoveMaximize,
		  F_RESIZEMOVE_MAXIMIZE, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("restart", CMD_Restart, F_RESTART, 0),
	CMD_ENTRY("savequitsession", CMD_SaveQuitSession, F_SAVE_QUIT_SESSION,
		  0),
	CMD_ENTRY("savesession", CMD_SaveSession, F_SAVE_SESSION, 0),
	CMD_ENTRY("schedule", CMD_Schedule, F_SCHEDULE, 0),
	CMD_ENTRY("scroll", CMD_Scroll, F_SCROLL, 0),
	CMD_ENTRY("send_configinfo", CMD_Send_ConfigInfo, F_CONFIG_LIST,
		  FUNC_DONT_REPEAT),
	CMD_ENTRY("send_windowlist", CMD_Send_WindowList, F_SEND_WINDOW_LIST,
		  FUNC_DONT_REPEAT),
	CMD_ENTRY("sendtomodule", CMD_SendToModule, F_SEND_STRING,
		  FUNC_DONT_REPEAT),
	CMD_ENTRY("set_mask", CMD_set_mask, F_SET_MASK, FUNC_DONT_REPEAT),
	CMD_ENTRY("set_nograb_mask", CMD_set_nograb_mask, F_SET_NOGRAB_MASK,
		  FUNC_DONT_REPEAT),
	CMD_ENTRY("set_sync_mask", CMD_set_sync_mask, F_SET_SYNC_MASK,
		  FUNC_DONT_REPEAT),
	CMD_ENTRY("setanimation", CMD_SetAnimation, F_SET_ANIMATION, 0),
	CMD_ENTRY("setenv", CMD_SetEnv, F_SETENV, 0),
	CMD_ENTRY(PRE_SILENT, CMD_Silent, F_SILENT, 0),
	CMD_ENTRY("snapattraction", CMD_SnapAttraction, F_SNAP_ATT, 0),
	CMD_ENTRY("snapgrid", CMD_SnapGrid, F_SNAP_GRID, 0),
	CMD_ENTRY("state", CMD_State, F_STATE, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("stick", CMD_Stick, F_STICK, FUNC_NEEDS_WINDOW),
#ifdef HAVE_STROKE
	CMD_ENTRY("stroke", CMD_Stroke, F_STROKE, 0),
	CMD_ENTRY("strokefunc", CMD_StrokeFunc, F_STROKE_FUNC, 0),
#endif /* HAVE_STROKE */
	CMD_ENTRY("style", CMD_Style, F_STYLE, 0),
	CMD_ENTRY("tearmenuoff", CMD_TearMenuOff, F_TEARMENUOFF, 0),
	CMD_ENTRY("thiswindow", CMD_ThisWindow, F_THISWINDOW, 0),
	CMD_ENTRY("title", CMD_Title, F_TITLE, 0),
	CMD_ENTRY("titlestyle", CMD_TitleStyle, F_TITLESTYLE, FUNC_DECOR),
	CMD_ENTRY("unsetenv", CMD_UnsetEnv, F_SETENV, 0),
	CMD_ENTRY("updatedecor", CMD_UpdateDecor, F_UPDATE_DECOR, 0),
	CMD_ENTRY("updatestyles", CMD_UpdateStyles, F_UPDATE_STYLES, 0),
	CMD_ENTRY("wait", CMD_Wait, F_WAIT, 0),
	CMD_ENTRY("warptowindow", CMD_WarpToWindow, F_WARP, FUNC_NEEDS_WINDOW),
	CMD_ENTRY("windowfont", CMD_WindowFont, F_WINDOWFONT, 0),
	CMD_ENTRY("windowid", CMD_WindowId, F_WINDOWID, 0),
	CMD_ENTRY("windowlist", CMD_WindowList, F_WINDOWLIST, 0),
	CMD_ENTRY("windowshade", CMD_WindowShade, F_WINDOW_SHADE,
		  FUNC_NEEDS_WINDOW),
	CMD_ENTRY("windowshadeanimate", CMD_WindowShadeAnimate, F_SHADE_ANIMATE,
		  0),
	CMD_ENTRY("xinerama", CMD_Xinerama, F_XINERAMA, 0),
	CMD_ENTRY("xineramaprimaryscreen", CMD_XineramaPrimaryScreen,
		  F_XINERAMAPRIMARYSCREEN, 0),
	CMD_ENTRY("xineramasls", CMD_XineramaSls, F_XINERAMASLS, 0),
	CMD_ENTRY("xineramaslsscreens", CMD_XineramaSlsScreens,
		  F_XINERAMASLSSCREENS, 0),
	CMD_ENTRY("xineramaslssize", CMD_XineramaSlsSize, F_XINERAMASLSSIZE, 0),
	CMD_ENTRY("xorpixmap", CMD_XorPixmap, F_XOR, 0),
	CMD_ENTRY("xorvalue", CMD_XorValue, F_XOR, 0),
	CMD_ENTRY("xsync", CMD_XSync, F_XSYNC, 0),
	CMD_ENTRY("xsynchronize", CMD_XSynchronize, F_XSYNCHRONIZE, 0),
	{"",0,0,0}
};

/* ---------------------------- local functions ----------------------------- */

/* ---------------------------- interface functions ------------------------- */
