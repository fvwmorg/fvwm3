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
#include "execcontext.h"
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
#define CMD_ENT(cmd, func, cmd_id, flags, cursor, event) \
	{ cmd, func, cmd_id, flags, cursor, event }

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
	CMD_ENT("+", CMD_Plus, F_ADDMENU2, 0, 0, 0),
	CMD_ENT("addbuttonstyle", CMD_AddButtonStyle, F_ADD_BUTTON_STYLE,
		FUNC_DECOR, 0, 0),
	CMD_ENT("addtitlestyle", CMD_AddTitleStyle, F_ADD_TITLE_STYLE,
		FUNC_DECOR, 0, 0),
#ifdef USEDECOR
	CMD_ENT("addtodecor", CMD_AddToDecor, F_ADD_DECOR, 0, 0, 0),
#endif /* USEDECOR */
	CMD_ENT("addtofunc", CMD_AddToFunc, F_ADDFUNC, FUNC_ADD_TO, 0, 0),
	CMD_ENT("addtomenu", CMD_AddToMenu, F_ADDMENU, 0, 0, 0),
	CMD_ENT("all", CMD_All, F_ALL, 0, 0, 0),
	CMD_ENT("animatedmove", CMD_AnimatedMove, F_ANIMATED_MOVE,
		FUNC_NEEDS_WINDOW, CRS_MOVE, ButtonPress),
	CMD_ENT("any", CMD_Any, F_ANY, 0, 0, 0),
	CMD_ENT("beep", CMD_Beep, F_BEEP, 0, 0, 0),
	CMD_ENT("borderstyle", CMD_BorderStyle, F_BORDERSTYLE,
		FUNC_DECOR, 0, 0),
	CMD_ENT("break", CMD_Break, F_BREAK, 0, 0, 0),
	CMD_ENT("bugopts", CMD_BugOpts, F_BUG_OPTS, 0, 0, 0),
	CMD_ENT("busycursor", CMD_BusyCursor, F_BUSY_CURSOR, 0, 0, 0),
	CMD_ENT("buttonstate", CMD_ButtonState, F_BUTTON_STATE, 0, 0, 0),
	CMD_ENT("buttonstyle", CMD_ButtonStyle, F_BUTTON_STYLE,
		FUNC_DECOR, 0, 0),
#ifdef USEDECOR
	CMD_ENT("changedecor", CMD_ChangeDecor, F_CHANGE_DECOR,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
#endif /* USEDECOR */
	CMD_ENT("changemenustyle", CMD_ChangeMenuStyle, F_CHANGE_MENUSTYLE,
		0, 0, 0),
	CMD_ENT("cleanupcolorsets", CMD_CleanupColorsets, F_NOP, 0, 0, 0),
	CMD_ENT("clicktime", CMD_ClickTime, F_CLICK, 0, 0, 0),
	CMD_ENT("close", CMD_Close, F_CLOSE,
		FUNC_NEEDS_WINDOW, CRS_DESTROY, ButtonRelease),
	CMD_ENT("colorlimit", CMD_ColorLimit, F_COLOR_LIMIT, 0, 0, 0),
	CMD_ENT("colormapfocus", CMD_ColormapFocus, F_COLORMAP_FOCUS, 0, 0, 0),
	CMD_ENT("colorset", CMD_Colorset, F_NOP, 0, 0, 0),
	CMD_ENT("cond", CMD_Cond, F_COND, 0, 0, 0),
	CMD_ENT("condcase", CMD_CondCase, F_CONDCASE, 0, 0, 0),
	CMD_ENT("copymenustyle", CMD_CopyMenuStyle, F_COPY_MENU_STYLE, 0, 0, 0),
	CMD_ENT("current", CMD_Current, F_CURRENT, 0, 0, 0),
	CMD_ENT("cursormove", CMD_CursorMove, F_MOVECURSOR, 0, 0, 0),
	CMD_ENT("cursorstyle", CMD_CursorStyle, F_CURSOR_STYLE, 0, 0, 0),
	CMD_ENT("defaultcolors", CMD_DefaultColors, F_DFLT_COLORS, 0, 0, 0),
	CMD_ENT("defaultcolorset", CMD_DefaultColorset, F_DFLT_COLORSET,
		0, 0, 0),
	CMD_ENT("defaultfont", CMD_DefaultFont, F_DFLT_FONT, 0, 0, 0),
	CMD_ENT("defaulticon", CMD_DefaultIcon, F_DFLT_ICON, 0, 0, 0),
	CMD_ENT("defaultlayers", CMD_DefaultLayers, F_DFLT_LAYERS, 0, 0, 0),
	CMD_ENT("delete", CMD_Delete, F_DELETE,
		FUNC_NEEDS_WINDOW, CRS_DESTROY, ButtonRelease),
	CMD_ENT("deschedule", CMD_Deschedule, F_DESCHEDULE, 0, 0, 0),
	CMD_ENT("desk", CMD_Desk, F_GOTO_DESK, 0, 0, 0),
	CMD_ENT("desktopname", CMD_DesktopName, F_DESKTOP_NAME, 0, 0, 0),
	CMD_ENT("desktopsize", CMD_DesktopSize, F_SETDESK, 0, 0, 0),
	CMD_ENT("destroy", CMD_Destroy, F_DESTROY,
		FUNC_NEEDS_WINDOW, CRS_DESTROY, ButtonRelease),
#ifdef USEDECOR
	CMD_ENT("destroydecor", CMD_DestroyDecor, F_DESTROY_DECOR, 0, 0, 0),
#endif /* USEDECOR */
	CMD_ENT("destroyfunc", CMD_DestroyFunc, F_DESTROY_FUNCTION, 0, 0, 0),
	CMD_ENT("destroymenu", CMD_DestroyMenu, F_DESTROY_MENU, 0, 0, 0),
	CMD_ENT("destroymenustyle", CMD_DestroyMenuStyle, F_DESTROY_MENUSTYLE,
		0, 0, 0),
	CMD_ENT("destroymoduleconfig", CMD_DestroyModuleConfig, F_DESTROY_MOD,
		0, 0, 0),
	CMD_ENT("destroystyle", CMD_DestroyStyle, F_DESTROY_STYLE, 0, 0, 0),
	CMD_ENT("direction", CMD_Direction, F_DIRECTION, 0, 0, 0),
	CMD_ENT("echo", CMD_Echo, F_ECHO, 0, 0, 0),
	CMD_ENT("edgecommand", CMD_EdgeCommand, F_EDGE_COMMAND, 0, 0, 0),
	CMD_ENT("edgeresistance", CMD_EdgeResistance, F_EDGE_RES, 0, 0, 0),
	CMD_ENT("edgescroll", CMD_EdgeScroll, F_EDGE_SCROLL, 0, 0, 0),
	CMD_ENT("edgethickness", CMD_EdgeThickness, F_NOP, 0, 0, 0),
	CMD_ENT("emulate", CMD_Emulate, F_EMULATE, 0, 0, 0),
	CMD_ENT("escapefunc", CMD_EscapeFunc, F_ESCAPE_FUNC, 0, 0, 0),
	CMD_ENT("ewmhbasestrut", CMD_EwmhBaseStrut, F_EWMH_BASE_STRUT, 0, 0, 0),
	CMD_ENT("ewmhnumberofdesktops", CMD_EwmhNumberOfDesktops,
		F_EWMH_NUMBER_OF_DESKTOPS, 0, 0, 0),
	CMD_ENT("exec", CMD_Exec, F_EXEC, 0, 0, 0),
	CMD_ENT("execuseshell", CMD_ExecUseShell, F_EXEC_SETUP, 0, 0, 0),
	CMD_ENT("fakeclick", CMD_FakeClick, F_FAKE_CLICK, 0, 0, 0),
	CMD_ENT("flipfocus", CMD_FlipFocus, F_FLIP_FOCUS,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("focus", CMD_Focus, F_FOCUS,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("focusstyle", CMD_FocusStyle, F_FOCUSSTYLE, 0, 0, 0),
	CMD_ENT("function", CMD_Function, F_FUNCTION, 0, 0, 0),
	CMD_ENT("globalopts", CMD_GlobalOpts, F_GLOBAL_OPTS, 0, 0, 0),
#ifdef GNOME
	CMD_ENT("gnomebutton", CMD_GnomeButton, F_MOUSE, 0, 0, 0),
	CMD_ENT("gnomeshowdesks", CMD_GnomeShowDesks, F_GOTO_DESK, 0, 0, 0),
#endif /* GNOME */
	CMD_ENT("gotodesk", CMD_GotoDesk, F_GOTO_DESK, 0, 0, 0),
	CMD_ENT("gotodeskandpage", CMD_GotoDeskAndPage, F_GOTO_DESK, 0, 0, 0),
	CMD_ENT("gotopage", CMD_GotoPage, F_GOTO_PAGE, 0, 0, 0),
	CMD_ENT("hidegeometrywindow", CMD_HideGeometryWindow,
		F_HIDEGEOMWINDOW, 0, 0, 0),
	CMD_ENT("hilightcolor", CMD_HilightColor, F_HICOLOR, 0, 0, 0),
	CMD_ENT("hilightcolorset", CMD_HilightColorset, F_HICOLORSET, 0, 0, 0),
	CMD_ENT("iconfont", CMD_IconFont, F_ICONFONT, 0, 0, 0),
	CMD_ENT("iconify", CMD_Iconify, F_ICONIFY,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("iconpath", CMD_IconPath, F_ICON_PATH, 0, 0, 0),
	CMD_ENT("ignoremodifiers", CMD_IgnoreModifiers, F_IGNORE_MODIFIERS,
		0, 0, 0),
	CMD_ENT("imagepath", CMD_ImagePath, F_IMAGE_PATH, 0, 0, 0),
	CMD_ENT("key", CMD_Key, F_KEY, 0, 0, 0),
	CMD_ENT("killmodule", CMD_KillModule, F_KILL_MODULE, 0, 0, 0),
	CMD_ENT("layer", CMD_Layer, F_LAYER,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("lower", CMD_Lower, F_LOWER,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("maximize", CMD_Maximize, F_MAXIMIZE,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("menu", CMD_Menu, F_STAYSUP, 0, 0, 0),
	CMD_ENT("menustyle", CMD_MenuStyle, F_MENUSTYLE, 0, 0, 0),
	CMD_ENT("module", CMD_Module, F_MODULE, 0, 0, 0),
	CMD_ENT("modulepath", CMD_ModulePath, F_MODULE_PATH, 0, 0, 0),
	CMD_ENT("modulesynchronous", CMD_ModuleSynchronous, F_MODULE_SYNC,
		0, 0, 0),
	CMD_ENT("moduletimeout", CMD_ModuleTimeout, F_NOP, 0, 0, 0),
	CMD_ENT("mouse", CMD_Mouse, F_MOUSE, 0, 0, 0),
	CMD_ENT("move", CMD_Move, F_MOVE,
		FUNC_NEEDS_WINDOW, CRS_MOVE, ButtonPress),
	CMD_ENT("movethreshold", CMD_MoveThreshold, F_MOVE_THRESHOLD, 0, 0, 0),
	CMD_ENT("movetodesk", CMD_MoveToDesk, F_MOVE_TO_DESK,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("movetopage", CMD_MoveToPage, F_MOVE_TO_PAGE,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonPress),
	CMD_ENT("movetoscreen", CMD_MoveToScreen, F_MOVE_TO_SCREEN,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonPress),
	CMD_ENT("next", CMD_Next, F_NEXT, 0, 0, 0),
	CMD_ENT("none", CMD_None, F_NONE, 0, 0, 0),
	CMD_ENT("nop", CMD_Nop, F_NOP, FUNC_DONT_REPEAT, 0, 0),
	CMD_ENT("nowindow", CMD_NoWindow, F_NOP, 0, 0, 0),
	CMD_ENT("opaquemovesize", CMD_OpaqueMoveSize, F_OPAQUE, 0, 0, 0),
	CMD_ENT("pick", CMD_Pick, F_PICK, FUNC_NEEDS_WINDOW, CRS_SELECT,
		ButtonRelease),
	CMD_ENT("piperead", CMD_PipeRead, F_READ, 0, 0, 0),
	CMD_ENT("pixmappath", CMD_PixmapPath, F_PIXMAP_PATH, 0, 0, 0),
	CMD_ENT("placeagain", CMD_PlaceAgain, F_PLACEAGAIN,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("pointerkey", CMD_PointerKey, F_POINTERKEY, 0, 0, 0),
	CMD_ENT("pointerwindow", CMD_PointerWindow, F_POINTERWINDOW, 0, 0, 0),
	CMD_ENT("popup", CMD_Popup, F_POPUP, 0, 0, 0),
	CMD_ENT("prev", CMD_Prev, F_PREV, 0, 0, 0),
	CMD_ENT("printinfo", CMD_PrintInfo, F_PRINTINFO, 0, 0, 0),
	CMD_ENT("propertychange", CMD_PropertyChange, F_NOP, 0, 0, 0),
	CMD_ENT("quit", CMD_Quit, F_QUIT, 0, 0, 0),
	CMD_ENT("quitscreen", CMD_QuitScreen, F_QUIT_SCREEN, 0, 0, 0),
	CMD_ENT("quitsession", CMD_QuitSession, F_QUIT_SESSION, 0, 0, 0),
	CMD_ENT("raise", CMD_Raise, F_RAISE,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("raiselower", CMD_RaiseLower, F_RAISELOWER,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("read", CMD_Read, F_READ, 0, 0, 0),
	CMD_ENT("readwritecolors", CMD_ReadWriteColors, F_NOP, 0, 0, 0),
	CMD_ENT("recapture", CMD_Recapture, F_RECAPTURE, 0, 0, 0),
	CMD_ENT("recapturewindow", CMD_RecaptureWindow, F_RECAPTURE_WINDOW,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("refresh", CMD_Refresh, F_REFRESH, 0, 0, 0),
	CMD_ENT("refreshwindow", CMD_RefreshWindow, F_REFRESH,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT(PRE_REPEAT, CMD_Repeat, F_REPEAT, FUNC_DONT_REPEAT, 0, 0),
	CMD_ENT("resize", CMD_Resize, F_RESIZE,
		FUNC_NEEDS_WINDOW, CRS_RESIZE, ButtonPress),
	CMD_ENT("resizemaximize", CMD_ResizeMaximize, F_RESIZE_MAXIMIZE,
		FUNC_NEEDS_WINDOW, CRS_RESIZE, ButtonPress),
	CMD_ENT("resizemove", CMD_ResizeMove, F_RESIZEMOVE,
		FUNC_NEEDS_WINDOW, CRS_RESIZE, ButtonPress),
	CMD_ENT("resizemovemaximize", CMD_ResizeMoveMaximize,
		F_RESIZEMOVE_MAXIMIZE, FUNC_NEEDS_WINDOW, CRS_RESIZE,
		ButtonPress),
	CMD_ENT("restacktransients", CMD_RestackTransients, F_RESTACKTRANSIENTS,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("restart", CMD_Restart, F_RESTART, 0, 0, 0),
	CMD_ENT("savequitsession", CMD_SaveQuitSession, F_SAVE_QUIT_SESSION,
		0, 0, 0),
	CMD_ENT("savesession", CMD_SaveSession, F_SAVE_SESSION, 0, 0, 0),
	CMD_ENT("schedule", CMD_Schedule, F_SCHEDULE, 0, 0, 0),
	CMD_ENT("scroll", CMD_Scroll, F_SCROLL, 0, 0, 0),
	CMD_ENT("send_configinfo", CMD_Send_ConfigInfo, F_CONFIG_LIST,
		FUNC_DONT_REPEAT, 0, 0),
	CMD_ENT("send_windowlist", CMD_Send_WindowList, F_SEND_WINDOW_LIST,
		FUNC_DONT_REPEAT, 0, 0),
	CMD_ENT("sendtomodule", CMD_SendToModule, F_SEND_STRING,
		FUNC_DONT_REPEAT, 0, 0),
	CMD_ENT("set_mask", CMD_set_mask, F_SET_MASK, FUNC_DONT_REPEAT, 0, 0),
	CMD_ENT("set_nograb_mask", CMD_set_nograb_mask, F_SET_NOGRAB_MASK,
		FUNC_DONT_REPEAT, 0, 0),
	CMD_ENT("set_sync_mask", CMD_set_sync_mask, F_SET_SYNC_MASK,
		FUNC_DONT_REPEAT, 0, 0),
	CMD_ENT("setanimation", CMD_SetAnimation, F_SET_ANIMATION, 0, 0, 0),
	CMD_ENT("setenv", CMD_SetEnv, F_SETENV, 0, 0, 0),
	CMD_ENT(PRE_SILENT, CMD_Silent, F_SILENT, 0, 0, 0),
	CMD_ENT("snapattraction", CMD_SnapAttraction, F_SNAP_ATT, 0, 0, 0),
	CMD_ENT("snapgrid", CMD_SnapGrid, F_SNAP_GRID, 0, 0, 0),
	CMD_ENT("state", CMD_State, F_STATE,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("stick", CMD_Stick, F_STICK,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
#ifdef HAVE_STROKE
	CMD_ENT("stroke", CMD_Stroke, F_STROKE, 0, 0, 0),
	CMD_ENT("strokefunc", CMD_StrokeFunc, F_STROKE_FUNC, 0, 0, 0),
#endif /* HAVE_STROKE */
	CMD_ENT("style", CMD_Style, F_STYLE, 0, 0, 0),
	CMD_ENT("tearmenuoff", CMD_TearMenuOff, F_TEARMENUOFF, 0, 0, 0),
	CMD_ENT("thiswindow", CMD_ThisWindow, F_THISWINDOW, 0, 0, 0),
	CMD_ENT("title", CMD_Title, F_TITLE, 0, 0, 0),
	CMD_ENT("titlestyle", CMD_TitleStyle, F_TITLESTYLE, FUNC_DECOR, 0, 0),
	CMD_ENT("unsetenv", CMD_UnsetEnv, F_SETENV, 0, 0, 0),
	CMD_ENT("updatedecor", CMD_UpdateDecor, F_UPDATE_DECOR, 0, 0, 0),
	CMD_ENT("updatestyles", CMD_UpdateStyles, F_UPDATE_STYLES, 0, 0, 0),
	CMD_ENT("wait", CMD_Wait, F_WAIT, 0, 0, 0),
	CMD_ENT("warptowindow", CMD_WarpToWindow, F_WARP,
		FUNC_NEEDS_WINDOW | FUNC_ALLOW_UNMANAGED,
		CRS_SELECT, ButtonRelease),
	CMD_ENT("windowfont", CMD_WindowFont, F_WINDOWFONT, 0, 0, 0),
	CMD_ENT("windowid", CMD_WindowId, F_WINDOWID, 0, 0, 0),
	CMD_ENT("windowlist", CMD_WindowList, F_WINDOWLIST, 0, 0, 0),
	CMD_ENT("windowshade", CMD_WindowShade, F_WINDOW_SHADE,
		FUNC_NEEDS_WINDOW, CRS_SELECT, ButtonRelease),
	CMD_ENT("windowshadeanimate", CMD_WindowShadeAnimate, F_SHADE_ANIMATE,
		0, 0, 0),
	CMD_ENT("xinerama", CMD_Xinerama, F_XINERAMA, 0, 0, 0),
	CMD_ENT("xineramaprimaryscreen", CMD_XineramaPrimaryScreen,
		F_XINERAMAPRIMARYSCREEN, 0, 0, 0),
	CMD_ENT("xineramasls", CMD_XineramaSls, F_XINERAMASLS, 0, 0, 0),
	CMD_ENT("xineramaslsscreens", CMD_XineramaSlsScreens,
		F_XINERAMASLSSCREENS, 0, 0, 0),
	CMD_ENT("xineramaslssize", CMD_XineramaSlsSize, F_XINERAMASLSSIZE,
		0, 0, 0),
	CMD_ENT("xorpixmap", CMD_XorPixmap, F_XOR, 0, 0, 0),
	CMD_ENT("xorvalue", CMD_XorValue, F_XOR, 0, 0, 0),
	CMD_ENT("xsync", CMD_XSync, F_XSYNC, 0, 0, 0),
	CMD_ENT("xsynchronize", CMD_XSynchronize, F_XSYNCHRONIZE, 0, 0, 0),
	{"",0,0,0,0,0}
};
