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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include "fvwm.h"
#include "execcontext.h"
#include "commands.h"
#include "cursor.h"
#include "functions.h"
#include "misc.h"
#include "functable.h"

/* ---------------------------- local definitions -------------------------- */

/* The function names in the first field *must* be in lowercase or else the
 * function cannot be called.  The func parameter of the macro is also used
 * for parsing by modules that rely on the old case of the commands.
 */
#define CMD_ENT(cmd, func, cmd_id, flags, cursor) \
	{ cmd, func, cmd_id, flags, cursor }

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

/* ---------------------------- exported variables (globals) --------------- */

/* IMPORTANT: command description should not be longer than 53 characters. */
/* If you change func_table format, change also perllib/FVWM/create-commands */
/* The next line must be blank for modules to properly parse this file. */

const func_t func_table[] =
{
	/* CMD_ENT("#", CMD_Comment, 0, 0, 0), */
	/* # - Comment line (ignored) */

	/* CMD_ENT("*", CMD_Asterisk, 0, 0, 0), */
	/* * - Module configuration line (no space after asterisk) */

	CMD_ENT("+", CMD_Plus, F_ADDMENU2, 0, 0),
	/* + - Continue the last AddToFunc, AddToMenu or AddToDecor */

	CMD_ENT("addbuttonstyle", CMD_AddButtonStyle, F_ADD_BUTTON_STYLE,
		FUNC_DECOR, 0),
	/* - Add to a button style (see ButtonStyle) */

	CMD_ENT("addtitlestyle", CMD_AddTitleStyle, F_ADD_TITLE_STYLE,
		FUNC_DECOR, 0),
	/* - Add to a title style (see TitleStyle) */

	CMD_ENT("addtodecor", CMD_AddToDecor, F_ADD_DECOR, 0, 0),
	/* - Add a decor definition (will be obsolete) */

	CMD_ENT("addtofunc", CMD_AddToFunc, F_ADDFUNC, FUNC_ADD_TO, 0),
	/* - Add a function definition */

	CMD_ENT("addtomenu", CMD_AddToMenu, F_ADDMENU, 0, 0),
	/* - Add a menu definition */

	CMD_ENT("all", CMD_All, F_ALL, 0, 0),
	/* - Operate on all windows matching the given condition */

	CMD_ENT("animatedmove", CMD_AnimatedMove, F_ANIMATED_MOVE,
		FUNC_NEEDS_WINDOW, CRS_MOVE),
	/* - Like Move, but uses animation to move windows */

	CMD_ENT("any", CMD_Any, F_ANY, 0, 0),
	/* - Operate if there is any window matching the condition */

	CMD_ENT("beep", CMD_Beep, F_BEEP, 0, 0),
	/* - Produce a bell */

	CMD_ENT("borderstyle", CMD_BorderStyle, F_BORDERSTYLE,
		FUNC_DECOR, 0),
	/* - Define a window border look (will be reworked) */

	CMD_ENT("break", CMD_Break, F_BREAK, 0, 0),
	/* - Stop executing the current (but not parent) function */

	CMD_ENT("bugopts", CMD_BugOpts, F_BUG_OPTS, 0, 0),
	/* - Set some application bug workarounds */

	CMD_ENT("busycursor", CMD_BusyCursor, F_BUSY_CURSOR, 0, 0),
	/* - Show/don't show the wait cursor in certain operations */

	CMD_ENT("buttonstate", CMD_ButtonState, F_BUTTON_STATE, 0, 0),
	/* - Disable some titlebar button states (not recommended) */

	CMD_ENT("buttonstyle", CMD_ButtonStyle, F_BUTTON_STYLE,
		FUNC_DECOR, 0),
	/* - Define a window button look (will be reworked) */

	CMD_ENT("changedecor", CMD_ChangeDecor, F_CHANGE_DECOR,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Attach decor to a window (will be obsolete) */

	CMD_ENT("changemenustyle", CMD_ChangeMenuStyle, F_CHANGE_MENUSTYLE,
		0, 0),
	/* - Attach menu style to a menu (see MenuStyle) */

	CMD_ENT("cleanupcolorsets", CMD_CleanupColorsets, F_NOP, 0, 0),
	/* - Reset all used colorsets with the default gray colors */

	CMD_ENT("clicktime", CMD_ClickTime, F_CLICK, 0, 0),
	/* - Set a time in milliseconds for click and double click */

	CMD_ENT("close", CMD_Close, F_CLOSE,
		FUNC_NEEDS_WINDOW, CRS_DESTROY),
	/* - Try to Delete a window, if this fails, Destroy it */

	CMD_ENT("colorlimit", CMD_ColorLimit, F_COLOR_LIMIT, 0, 0),
	/* - Set limit on colors used (obsolete) */

	CMD_ENT("colormapfocus", CMD_ColormapFocus, F_COLORMAP_FOCUS, 0, 0),
	/* - Change the colormap behaviour for low-depth X servers */

	CMD_ENT("colorset", CMD_Colorset, F_NOP, 0, 0),
	/* - Manage colors used like fg, bg, image bg, gradient bg */

	CMD_ENT("copymenustyle", CMD_CopyMenuStyle, F_COPY_MENU_STYLE, 0, 0),
	/* - Copy the existing menu style to new or existing one */

	CMD_ENT("current", CMD_Current, F_CURRENT, 0, 0),
	/* - Operate on the currently focused window */

	CMD_ENT("cursormove", CMD_CursorMove, F_MOVECURSOR, 0, 0),
	/* - Move the cursor pointer non interactively */

	CMD_ENT("cursorstyle", CMD_CursorStyle, F_CURSOR_STYLE, 0, 0),
	/* - Define different cursor pointer shapes and colors */

	CMD_ENT("defaultcolors", CMD_DefaultColors, F_DFLT_COLORS, 0, 0),
	/* - Set colors for the feedback window (will be obsolete) */

	CMD_ENT("defaultcolorset", CMD_DefaultColorset, F_DFLT_COLORSET,
		0, 0),
	/* - Set colors for the Move/Resize feedback window */

	CMD_ENT("defaultfont", CMD_DefaultFont, F_DFLT_FONT, 0, 0),
	/* - The default font to use (mainly for feedback window) */

	CMD_ENT("defaulticon", CMD_DefaultIcon, F_DFLT_ICON, 0, 0),
	/* - The default icon to use for iconified windows */

	CMD_ENT("defaultlayers", CMD_DefaultLayers, F_DFLT_LAYERS, 0, 0),
	/* - Set StaysOnBottom, StaysPut, StaysOnTop layer numbers */

	CMD_ENT("delete", CMD_Delete, F_DELETE,
		FUNC_NEEDS_WINDOW, CRS_DESTROY),
	/* - Try to delete a window using the X delete protocol */

	CMD_ENT("deschedule", CMD_Deschedule, F_DESCHEDULE, 0, 0),
	/* - Remove commands sheduled earlier using Schedule */

	CMD_ENT("desk", CMD_Desk, F_GOTO_DESK, 0, 0),
	/* - (obsolete, use GotoDesk instead) */

	CMD_ENT("desktopname", CMD_DesktopName, F_DESKTOP_NAME, 0, 0),
	/* - Define the desktop names used in WindowList, modules */

	CMD_ENT("desktopsize", CMD_DesktopSize, F_SETDESK, 0, 0),
	/* - Set virtual desktop size in units of physical pages */

	CMD_ENT("destroy", CMD_Destroy, F_DESTROY,
		FUNC_NEEDS_WINDOW, CRS_DESTROY),
	/* - Kill a window without any warning to an application */

	CMD_ENT("destroydecor", CMD_DestroyDecor, F_DESTROY_DECOR, 0, 0),
	/* - Delete decor defined by AddToDecor (will be obsolete) */

	CMD_ENT("destroyfunc", CMD_DestroyFunc, F_DESTROY_FUNCTION, 0, 0),
	/* - Delete function defined using AddToFunc */

	CMD_ENT("destroymenu", CMD_DestroyMenu, F_DESTROY_MENU, 0, 0),
	/* - Delete menu defined using AddToMenu */

	CMD_ENT("destroymenustyle", CMD_DestroyMenuStyle, F_DESTROY_MENUSTYLE,
		0, 0),
	/* - Delete menu style defined using MenuStyle */

	CMD_ENT("destroymoduleconfig", CMD_DestroyModuleConfig, F_DESTROY_MOD,
		0, 0),
	/* - Delete matching module config lines defined using "*" */

	CMD_ENT("destroystyle", CMD_DestroyStyle, F_DESTROY_STYLE, 0, 0),
	/* - Delete style defined using Style */

	CMD_ENT("destroywindowstyle", CMD_DestroyWindowStyle,
		F_DESTROY_WINDOW_STYLE, FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Delete style defined using WindowStyle */

	CMD_ENT("direction", CMD_Direction, F_DIRECTION, 0, 0),
	/* - Operate on the next window in the specified direction */

	CMD_ENT("echo", CMD_Echo, F_ECHO, 0, 0),
	/* - Print message to stderr, mainly for debugging */

	CMD_ENT("echofuncdefinition", CMD_EchoFuncDefinition,
		F_ECHO_FUNC_DEFINITION, 0, 0),
	/* - Print the definion of a function */

	CMD_ENT("edgecommand", CMD_EdgeCommand, F_EDGE_COMMAND, 0, 0),
	/* - Bind one or another screen edge to an fvwm action */

	CMD_ENT("edgeleavecommand", CMD_EdgeLeaveCommand, F_EDGE_LEAVE_COMMAND,
		0, 0),
	/* - Bind one or another screen edge to an fvwm action */

	CMD_ENT("edgeresistance", CMD_EdgeResistance, F_EDGE_RES, 0, 0),
	/* - Control viewport scrolling and window move over edge */

	CMD_ENT("edgescroll", CMD_EdgeScroll, F_EDGE_SCROLL, 0, 0),
	/* - Control how much of the viewport is scrolled if any */

	CMD_ENT("edgethickness", CMD_EdgeThickness, F_NOP, 0, 0),
	/* - Control how closely to edge to run command/scrolling */

	CMD_ENT("emulate", CMD_Emulate, F_EMULATE, 0, 0),
	/* - Only used to position the position/size window */

	CMD_ENT("escapefunc", CMD_EscapeFunc, F_ESCAPE_FUNC, 0, 0),
	/* - Abort a wait or ModuleSynchonous command */

	CMD_ENT("ewmhbasestruts", CMD_EwmhBaseStruts, F_EWMH_BASE_STRUTS, 0, 0),
	/* - Define restricted areas of the screen */

	CMD_ENT("ewmhnumberofdesktops", CMD_EwmhNumberOfDesktops,
		F_EWMH_NUMBER_OF_DESKTOPS, 0, 0),
	/* - For ewmh pager, define number of desktops */

	CMD_ENT("exec", CMD_Exec, F_EXEC, 0, 0),
	/* - Execute an external command */

	CMD_ENT("execuseshell", CMD_ExecUseShell, F_EXEC_SETUP, 0, 0),
	/* - The shell to use to execute an external command */

	CMD_ENT("fakeclick", CMD_FakeClick, F_FAKE_CLICK, 0, 0),
	/* - Generate a mouse click */

	CMD_ENT("fakekeypress", CMD_FakeKeypress, F_FAKE_KEYPRESS,
		0, 0),
	/* - Send a keyboard event to a window */

	CMD_ENT("flipfocus", CMD_FlipFocus, F_FLIP_FOCUS,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Focus a window without rotating windowlist order */

	CMD_ENT("focus", CMD_Focus, F_FOCUS,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Give focus to a window */

	CMD_ENT("focusstyle", CMD_FocusStyle, F_FOCUSSTYLE, 0, 0),
	/* - Configure focus and raise policy for windows */

	CMD_ENT("function", CMD_Function, F_FUNCTION, 0, 0),
	/* Function - Execute a user defined function, see AddToFunc */

	CMD_ENT("globalopts", CMD_GlobalOpts, F_GLOBAL_OPTS, 0, 0),
	/* - (obsolete, use corresponding Style * instead) */

	CMD_ENT("gotodesk", CMD_GotoDesk, F_GOTO_DESK, 0, 0),
	/* - Switch viewport to another desk same page */

	CMD_ENT("gotodeskandpage", CMD_GotoDeskAndPage, F_GOTO_DESK, 0, 0),
	/* - Switch viewport to another desk and page */

	CMD_ENT("gotopage", CMD_GotoPage, F_GOTO_PAGE, 0, 0),
	/* - Switch viewport to another page same desk */

	CMD_ENT("hidegeometrywindow", CMD_HideGeometryWindow,
		F_HIDEGEOMWINDOW, 0, 0),
	/* - Hide/show the position/size window */

	CMD_ENT("hilightcolor", CMD_HilightColor, F_HICOLOR, 0, 0),
	/* - (obsolete, use Style * HighlightFore/Back) */

	CMD_ENT("hilightcolorset", CMD_HilightColorset, F_HICOLORSET, 0, 0),
	/* - (obsolete, use Style * HighlightColorset) */

	CMD_ENT("iconfont", CMD_IconFont, F_ICONFONT, 0, 0),
	/* - (obsolete, use Style * IconFont) */

	CMD_ENT("iconify", CMD_Iconify, F_ICONIFY,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Change iconification status of a window (minimize) */

	CMD_ENT("iconpath", CMD_IconPath, F_ICON_PATH, 0, 0),
	/* - (obsolete, use ImagePath instead) */

	CMD_ENT("ignoremodifiers", CMD_IgnoreModifiers, F_IGNORE_MODIFIERS,
		0, 0),
	/* - Modifiers to ignore on mouse and key bindings */

	CMD_ENT("imagepath", CMD_ImagePath, F_IMAGE_PATH, 0, 0),
	/* - Directories to search for images */

	CMD_ENT("infostoreadd", CMD_InfoStoreAdd, F_INFOSTOREADD, 0, 0),
	/* - Adds an entry (key/value pairs) to the infostore */

	CMD_ENT("infostoreclear", CMD_InfoStoreClear, F_INFOSTORECLEAR, 0, 0),
	/* - Clears all entries from the infostore */

	CMD_ENT("infostoreremove", CMD_InfoStoreRemove, F_INFOSTOREREMOVE, 0, 0),
	/* - Removes an entry from the infostore */

	CMD_ENT(PRE_KEEPRC, CMD_KeepRc, F_KEEPRC, 0, 0),
	/* KeepRc - Do not modify the previous command return code */

	CMD_ENT("key", CMD_Key, F_KEY, 0, 0),
	/* - Bind or unbind a key to an fvwm action */

	CMD_ENT("killmodule", CMD_KillModule, F_KILL_MODULE, 0, 0),
	/* - Stops an fvwm module */

	CMD_ENT("layer", CMD_Layer, F_LAYER,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Change the layer of a window */

	CMD_ENT("localepath", CMD_LocalePath, F_LOCALE_PATH, 0, 0),
	/* - Directories/domains to search for locale data */

	CMD_ENT("lower", CMD_Lower, F_LOWER,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Lower a window within a layer */

	CMD_ENT("maximize", CMD_Maximize, F_MAXIMIZE,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Toggle maximal-size status of a window */

	CMD_ENT("menu", CMD_Menu, F_STAYSUP, 0, 0),
	/* - Display (post) a menu */

	CMD_ENT("menustyle", CMD_MenuStyle, F_MENUSTYLE, 0, 0),
	/* - Control appearance and behavior of a menu */

	CMD_ENT("module", CMD_Module, F_MODULE, 0, 0),
	/* - Invoke an fvwm module */

	CMD_ENT("modulelistenonly", CMD_ModuleListenOnly, F_MODULE_LISTEN_ONLY,
		0, 0),
	/* - Invoke an fvwm module */

	CMD_ENT("modulepath", CMD_ModulePath, F_MODULE_PATH, 0, 0),
	/* - Modify the directories to search for an fvwm module */

	CMD_ENT("modulesynchronous", CMD_ModuleSynchronous, F_MODULE_SYNC,
		0, 0),
	/* - Invoke an fvwm module synchronously */

	CMD_ENT("moduletimeout", CMD_ModuleTimeout, F_NOP, 0, 0),
	/* - Set timeout value for response from module */

	CMD_ENT("mouse", CMD_Mouse, F_MOUSE, 0, 0),
	/* - Bind or unbind a mouse button press to an fvwm action */

	CMD_ENT("move", CMD_Move, F_MOVE,
		FUNC_NEEDS_WINDOW, CRS_MOVE),
	/* - Move a window */

	CMD_ENT("movethreshold", CMD_MoveThreshold, F_MOVE_THRESHOLD, 0, 0),
	/* - Set number of pixels in a click and a hold vs. a drag */

	CMD_ENT("movetodesk", CMD_MoveToDesk, F_MOVE_TO_DESK,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Move a window to another desk same page */

	CMD_ENT("movetopage", CMD_MoveToPage, F_MOVE_TO_PAGE,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Move a window to another page same desk */

	CMD_ENT("movetoscreen", CMD_MoveToScreen, F_MOVE_TO_SCREEN,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Move a window to another Xinerama screen */

	CMD_ENT("next", CMD_Next, F_NEXT, 0, 0),
	/* - Operate on the next window matching conditions */

	CMD_ENT("none", CMD_None, F_NONE, 0, 0),
	/* - Perform command if no window matches conditions */

	CMD_ENT("nop", CMD_Nop, F_NOP, FUNC_DONT_REPEAT, 0),
	/* - Do nothing (used internally) */

	CMD_ENT("nowindow", CMD_NoWindow, F_NOP, 0, 0),
	/* - Prefix that runs a command without a window context */

	CMD_ENT("opaquemovesize", CMD_OpaqueMoveSize, F_OPAQUE, 0, 0),
	/* - Set maximum size window fvwm should move opaquely */

	CMD_ENT("pick", CMD_Pick, F_PICK, FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Prefix to force a window context, prompted if needed */

	CMD_ENT("piperead", CMD_PipeRead, F_READ, 0, 0),
	/* - Exec system command interpret output as fvwm commands */

	CMD_ENT("pixmappath", CMD_PixmapPath, F_PIXMAP_PATH, 0, 0),
	/* - (obsolete, use ImagePath instead) */

	CMD_ENT("placeagain", CMD_PlaceAgain, F_PLACEAGAIN,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Replace a window using initial window placement logic */

	CMD_ENT("pointerkey", CMD_PointerKey, F_POINTERKEY, 0, 0),
	/* - Bind an action to a key based on pointer not focus */

	CMD_ENT("pointerwindow", CMD_PointerWindow, F_POINTERWINDOW, 0, 0),
	/* - Operate on window under pointer if it meets conditions */

	CMD_ENT("popup", CMD_Popup, F_POPUP, 0, 0),
	/* - Display (pop-up) a menu, see also Menu */

	CMD_ENT("prev", CMD_Prev, F_PREV, 0, 0),
	/* - Operate on the precious window matching conditions */

	CMD_ENT("printinfo", CMD_PrintInfo, F_PRINTINFO, 0, 0),
	/* - Print information about the state of fvwm */

	CMD_ENT("propertychange", CMD_PropertyChange, F_NOP, 0, 0),
	/* - Internal, used for inter-module communication */

	CMD_ENT("quit", CMD_Quit, F_QUIT, 0, 0),
	/* - Exit fvwm */

	CMD_ENT("quitscreen", CMD_QuitScreen, F_QUIT_SCREEN, 0, 0),
	/* - Stop managing the specified screen */

	CMD_ENT("quitsession", CMD_QuitSession, F_QUIT_SESSION, 0, 0),
	/* - Ask session manager to shut down itself and fvwm */

	CMD_ENT("raise", CMD_Raise, F_RAISE,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Raise a window in a layer */

	CMD_ENT("raiselower", CMD_RaiseLower, F_RAISELOWER,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Alternately raise or lower a window in a layer */

	CMD_ENT("read", CMD_Read, F_READ, 0, 0),
	/* - Read fvwm commands from a file */

	CMD_ENT("readwritecolors", CMD_ReadWriteColors, F_NOP, 0, 0),
	/* - Used for colorset speed hacks (will be removed?) */

	CMD_ENT("recapture", CMD_Recapture, F_RECAPTURE, 0, 0),
	/* - Reapply styles to all windows (will be obsolete) */

	CMD_ENT("recapturewindow", CMD_RecaptureWindow, F_RECAPTURE_WINDOW,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Reapply styles to one window (will be obsolete) */

	CMD_ENT("refresh", CMD_Refresh, F_REFRESH, 0, 0),
	/* - Cause all windows to redraw themselves */

	CMD_ENT("refreshwindow", CMD_RefreshWindow, F_REFRESH,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Cause one window to redraw itself */

	CMD_ENT(PRE_REPEAT, CMD_Repeat, F_REPEAT, FUNC_DONT_REPEAT, 0),
	/* - Repeat (very unreliably) the last command, don't use */

	CMD_ENT("resize", CMD_Resize, F_RESIZE,
		FUNC_NEEDS_WINDOW, CRS_RESIZE),
	/* - Cause a window to be resized */

	CMD_ENT("resizemaximize", CMD_ResizeMaximize, F_RESIZE_MAXIMIZE,
		FUNC_NEEDS_WINDOW, CRS_RESIZE),
	/* - Resize a window and mark window as maximized */

	CMD_ENT("resizemove", CMD_ResizeMove, F_RESIZEMOVE,
		FUNC_NEEDS_WINDOW, CRS_RESIZE),
	/* - Resize and move in one operation */

	CMD_ENT("resizemovemaximize", CMD_ResizeMoveMaximize,
		F_RESIZEMOVE_MAXIMIZE, FUNC_NEEDS_WINDOW, CRS_RESIZE),
	/* - Resize and move in one operation and mark maximized */

	CMD_ENT("restacktransients", CMD_RestackTransients, F_RESTACKTRANSIENTS,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Regroup the window transients in the stack */

	CMD_ENT("restart", CMD_Restart, F_RESTART, 0, 0),
	/* - Restart itself or replace with another window manager */

	CMD_ENT("savequitsession", CMD_SaveQuitSession, F_SAVE_QUIT_SESSION,
		0, 0),
	/* - Cause session manager to save and shutdown fvwm */

	CMD_ENT("savesession", CMD_SaveSession, F_SAVE_SESSION, 0, 0),
	/* - Cause session manager to save the session */

	CMD_ENT("scanforwindow", CMD_ScanForWindow, F_SCANFORWINDOW, 0, 0),
	/* - Operate on the matching window in the given direction */

	CMD_ENT("schedule", CMD_Schedule, F_SCHEDULE, 0, 0),
	/* - Run an fvwm command after a delay */

	CMD_ENT("scroll", CMD_Scroll, F_SCROLL, 0, 0),
	/* - Scroll the desktop viewport */

	CMD_ENT("send_configinfo", CMD_Send_ConfigInfo, F_CONFIG_LIST,
		FUNC_DONT_REPEAT, 0),
	/* - Internal, used for module communication */

	CMD_ENT("send_reply", CMD_Send_Reply, F_SEND_REPLY,
		FUNC_DONT_REPEAT, 0),
	/* - Internal, used for module communication */

	CMD_ENT("send_windowlist", CMD_Send_WindowList, F_SEND_WINDOW_LIST,
		FUNC_DONT_REPEAT, 0),
	/* - Internal, used for module communication */

	CMD_ENT("sendtomodule", CMD_SendToModule, F_SEND_STRING,
		FUNC_DONT_REPEAT, 0),
	/* - Send a string (action) to a module */

	CMD_ENT("set_mask", CMD_set_mask, F_SET_MASK, FUNC_DONT_REPEAT, 0),
	/* - Internal, used for module communication */

	CMD_ENT("set_nograb_mask", CMD_set_nograb_mask, F_SET_NOGRAB_MASK,
		FUNC_DONT_REPEAT, 0),
	/* - Internal, used for module communication */

	CMD_ENT("set_sync_mask", CMD_set_sync_mask, F_SET_SYNC_MASK,
		FUNC_DONT_REPEAT, 0),
	/* - Internal, used for module communication */

	CMD_ENT("setanimation", CMD_SetAnimation, F_SET_ANIMATION, 0, 0),
	/* - Control animated moves and menus */

	CMD_ENT("setenv", CMD_SetEnv, F_SETENV, 0, 0),
	/* - Set an environment variable */

	CMD_ENT(PRE_SILENT, CMD_Silent, F_SILENT, 0, 0),
	/* Silent - Suppress errors on command, avoid window selection */

	CMD_ENT("snapattraction", CMD_SnapAttraction, F_SNAP_ATT, 0, 0),
	/* - Control attraction of windows during move */

	CMD_ENT("snapgrid", CMD_SnapGrid, F_SNAP_GRID, 0, 0),
	/* - Control grid used with SnapAttraction */

	CMD_ENT("state", CMD_State, F_STATE,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Control user defined window states */

	CMD_ENT("stick", CMD_Stick, F_STICK,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Change window stickyness */

	CMD_ENT("stickacrossdesks", CMD_StickAcrossDesks, F_STICKACROSSDESKS,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Change window stickyness on a desk basis */

	CMD_ENT("stickacrosspages", CMD_StickAcrossPages, F_STICKACROSSPAGES,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Change window stickyness on a page basis */

#ifdef HAVE_STROKE
	CMD_ENT("stroke", CMD_Stroke, F_STROKE, 0, 0),
	/* - Bind a stroke to an fvwm action */

	CMD_ENT("strokefunc", CMD_StrokeFunc, F_STROKE_FUNC, 0, 0),
	/* - Record stroke and execute corresponding stroke action */

#endif /* HAVE_STROKE */
	CMD_ENT("style", CMD_Style, F_STYLE, 0, 0),
	/* - Set attributes of windows that match a pattern */

	CMD_ENT("tearmenuoff", CMD_TearMenuOff, F_TEARMENUOFF, 0, 0),
	/* TearMenuOff - Convert a menu to a window, for use in menu items */

	CMD_ENT("test", CMD_Test, F_TEST_, 0, 0),
	/* - Execute command if conditions are met */

	CMD_ENT("testrc", CMD_TestRc, F_TESTRC, 0, 0),
	/* - Conditional switch (may be changed) */

	CMD_ENT("thiswindow", CMD_ThisWindow, F_THISWINDOW, 0, 0),
	/* - Operate on the context window if it meets conditions */

	CMD_ENT("title", CMD_Title, F_TITLE, 0, 0),
	/* Title - Insert title into a menu */

	CMD_ENT("titlestyle", CMD_TitleStyle, F_TITLESTYLE, FUNC_DECOR, 0),
	/* - Control window title */

	CMD_ENT("unsetenv", CMD_UnsetEnv, F_SETENV, 0, 0),
	/* - Remove an environment variable */

	CMD_ENT("updatedecor", CMD_UpdateDecor, F_UPDATE_DECOR, 0, 0),
	/* - Update window decor (obsolete and not needed anymore) */

	CMD_ENT("updatestyles", CMD_UpdateStyles, F_UPDATE_STYLES, 0, 0),
	/* - Cause styles to update while still in a function */

	CMD_ENT("wait", CMD_Wait, F_WAIT, 0, 0),
	/* - Pause until a matching window appears */

	CMD_ENT("warptowindow", CMD_WarpToWindow, F_WARP,
		FUNC_NEEDS_WINDOW | FUNC_ALLOW_UNMANAGED,
		CRS_SELECT),
	/* - Warp the pointer to a window */

	CMD_ENT("windowfont", CMD_WindowFont, F_WINDOWFONT, 0, 0),
	/* - (obsolete, use Style * Font) */

	CMD_ENT("windowid", CMD_WindowId, F_WINDOWID, 0, 0),
	/* - Execute command for window matching the windowid */

	CMD_ENT("windowlist", CMD_WindowList, F_WINDOWLIST, 0, 0),
	/* - Display the window list as a menu to select a window */

	CMD_ENT("windowshade", CMD_WindowShade, F_WINDOW_SHADE,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Shade/unshade a window */

	CMD_ENT("windowshadeanimate", CMD_WindowShadeAnimate, F_SHADE_ANIMATE,
		0, 0),
	/* - (obsolete, use Style * WindowShadeSteps) */

	CMD_ENT("windowstyle", CMD_WindowStyle, F_WINDOW_STYLE,
		FUNC_NEEDS_WINDOW, CRS_SELECT),
	/* - Set styles on the selected window */

	CMD_ENT("xinerama", CMD_Xinerama, F_XINERAMA, 0, 0),
	/* - Control Xinerama support */

	CMD_ENT("xineramaprimaryscreen", CMD_XineramaPrimaryScreen,
		F_XINERAMAPRIMARYSCREEN, 0, 0),
	/* - Identify Xinerama primary screen */

	CMD_ENT("xorpixmap", CMD_XorPixmap, F_XOR, 0, 0),
	/* - Use a pixmap for move/resize rubber-band */

	CMD_ENT("xorvalue", CMD_XorValue, F_XOR, 0, 0),
	/* - Change bits used for move/resize rubber-band */

	CMD_ENT("xsync", CMD_XSync, F_XSYNC, 0, 0),
	/* - For debugging, send all pending requests to X server */

	CMD_ENT("xsynchronize", CMD_XSynchronize, F_XSYNCHRONIZE, 0, 0),
	/* - For debugging, cause all X requests to be synchronous */

	{ "", 0, 0, 0, 0 }
};
