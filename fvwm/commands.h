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

#ifndef COMMANDS_H
#define COMMANDS_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

enum
{
	F_UNDEFINED = -1,

	/* functions that need no window */
	F_NOP = 0,
	F_ADDFUNC,
	F_ADDMENU,
	F_ADDMENU2,
	F_ALL,
	F_ANY,
	F_BEEP,
	F_BREAK,
	F_BUG_OPTS,
	F_BUSY_CURSOR,
	F_BUTTON_STATE,
	F_BUTTON_STYLE,
	F_CHANGE_MENUSTYLE,
	F_CIRCULATE_DOWN,
	F_CIRCULATE_UP,
	F_CLICK,
	F_CLOSE,
	F_COLORMAP_FOCUS,
	F_COND,
	F_CONDCASE,
	F_CONFIG_LIST,
	F_COPY_MENU_STYLE,
	F_CURRENT,
	F_CURSOR_STYLE,
	F_DESCHEDULE,
	F_DESKTOP_NAME,
	F_DESTROY_FUNCTION,
	F_DESTROY_MENU,
	F_DESTROY_MENUSTYLE,
	F_DESTROY_STYLE,
	F_DFLT_COLORS,
	F_DFLT_COLORSET,
	F_DFLT_FONT,
	F_DFLT_ICON,
	F_DFLT_LAYERS,
	F_DIRECTION,
	F_EDGE_COMMAND,
	F_EDGE_RES,
	F_EDGE_SCROLL,
	F_EMULATE,
	F_ESCAPE_FUNC,
	F_EWMH_BASE_STRUTS,
	F_EWMH_NUMBER_OF_DESKTOPS,
	F_EXEC,
	F_EXEC_SETUP,
	F_FAKE_CLICK,
	F_FOCUSSTYLE,
	F_FUNCTION,
	F_GLOBAL_OPTS,
	F_GOTO_DESK,
	F_GOTO_PAGE,
	F_HICOLOR,
	F_HICOLORSET,
	F_HIDEGEOMWINDOW,
	F_ICONFONT,
	F_ICON_PATH,
	F_IGNORE_MODIFIERS,
	F_IMAGE_PATH,
	F_KEY,
	F_KILL_MODULE,
	F_LAYER,
	F_MENUSTYLE,
	F_MODULE,
	F_MODULE_PATH,
	F_MODULE_SYNC,
	F_MOUSE,
	F_MOVECURSOR,
	F_MOVE_TO_DESK,
	F_NEXT,
	F_NONE,
	F_OPAQUE,
	F_PICK,
	F_PIXMAP_PATH,
	F_POINTERKEY,
	F_POINTERWINDOW,
	F_POPUP,
	F_PREV,
	F_PRINTINFO,
	F_QUIT,
	F_QUIT_SESSION,
	F_QUIT_SCREEN,
	F_READ,
	F_RECAPTURE,
	F_RECAPTURE_WINDOW,
	F_REFRESH,
	F_REPEAT,
	F_RESTART,
	F_SAVE_SESSION,
	F_SAVE_QUIT_SESSION,
	F_SCHEDULE,
	F_SCROLL,
	F_SETDESK,
	F_SETENV,
	F_SET_ANIMATION,
	F_SET_MASK,
	F_SET_NOGRAB_MASK,
	F_SET_SYNC_MASK,
	F_SHADE_ANIMATE,
	F_SILENT,
	F_SNAP_ATT,
	F_SNAP_GRID,
	F_STAYSUP,
	STROKE_ARG(F_STROKE)
	STROKE_ARG(F_STROKE_FUNC)
	F_STYLE,
	F_TEARMENUOFF,
	F_THISWINDOW,
	F_TITLE,
	F_TITLESTYLE,
	F_TOGGLE_PAGE,
	F_UPDATE_STYLES,
	F_WAIT,
	F_WINDOWFONT,
	F_WINDOWLIST,
	F_XINERAMA,
	F_XINERAMAPRIMARYSCREEN,
	F_XINERAMASLS,
	F_XINERAMASLSSCREENS,
	F_XINERAMASLSSIZE,
	F_XOR,
	F_XSYNC,
	F_XSYNCHRONIZE,

	/* functions that need a window to operate on */
	F_ADD_BUTTON_STYLE,
	F_ADD_DECOR,
	F_ADD_TITLE_STYLE,
	F_ANIMATED_MOVE,
	F_BORDERSTYLE,
	F_CHANGE_DECOR,
	F_COLOR_LIMIT,
	F_DELETE,
	F_DESTROY,
	F_DESTROY_DECOR,
	F_DESTROY_MOD,
	F_ECHO,
	F_FLIP_FOCUS,
	F_FOCUS,
	F_ICONIFY,
	F_LOWER,
	F_MAXIMIZE,
	F_MOVE,
	F_MOVE_THRESHOLD,
	F_MOVE_TO_PAGE,
	F_MOVE_TO_SCREEN,
	F_PLACEAGAIN,
	F_RAISE,
	F_RAISELOWER,
	F_RESIZE,
	F_RESIZE_MAXIMIZE,
	F_RESIZEMOVE,
	F_RESIZEMOVE_MAXIMIZE,
	F_RESTACKTRANSIENTS,
	F_SEND_STRING,
	F_STATE,
	F_STICK,
	F_STICKDESK,
	F_STICKPAGE,
	F_UPDATE_DECOR,
	F_WARP,
	F_WINDOWID,
	F_WINDOW_SHADE,

	F_END_OF_LIST = 999,

	/* Functions for use by modules only! */
	F_SEND_WINDOW_LIST = 1000
};

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

/* This file contains all command prototypes. */
void CMD_Plus(F_CMD_ARGS);
void CMD_AddButtonStyle(F_CMD_ARGS);
void CMD_AddTitleStyle(F_CMD_ARGS);
#ifdef USEDECOR
void CMD_AddToDecor(F_CMD_ARGS);
#endif /* USEDECOR */
void CMD_AddToFunc(F_CMD_ARGS);
void CMD_AddToMenu(F_CMD_ARGS);
void CMD_Alias(F_CMD_ARGS);
void CMD_All(F_CMD_ARGS);
void CMD_AnimatedMove(F_CMD_ARGS);
void CMD_Any(F_CMD_ARGS);
void CMD_Beep(F_CMD_ARGS);
void CMD_Break(F_CMD_ARGS);
void CMD_BorderStyle(F_CMD_ARGS);
void CMD_BugOpts(F_CMD_ARGS);
void CMD_BusyCursor(F_CMD_ARGS);
void CMD_ButtonState(F_CMD_ARGS);
void CMD_ButtonStyle(F_CMD_ARGS);
#ifdef USEDECOR
void CMD_ChangeDecor(F_CMD_ARGS);
#endif /* USEDECOR */
void CMD_ChangeMenuStyle(F_CMD_ARGS);
void CMD_CleanupColorsets(F_CMD_ARGS);
void CMD_ClickTime(F_CMD_ARGS);
void CMD_Close(F_CMD_ARGS);
void CMD_ColorLimit(F_CMD_ARGS);
void CMD_ColormapFocus(F_CMD_ARGS);
void CMD_Colorset(F_CMD_ARGS);
void CMD_Cond(F_CMD_ARGS);
void CMD_CondCase(F_CMD_ARGS);
void CMD_CopyMenuStyle(F_CMD_ARGS);
void CMD_Current(F_CMD_ARGS);
void CMD_CursorMove(F_CMD_ARGS);
void CMD_CursorStyle(F_CMD_ARGS);
void CMD_DefaultColors(F_CMD_ARGS);
void CMD_DefaultColorset(F_CMD_ARGS);
void CMD_DefaultFont(F_CMD_ARGS);
void CMD_DefaultIcon(F_CMD_ARGS);
void CMD_DefaultLayers(F_CMD_ARGS);
void CMD_Delete(F_CMD_ARGS);
void CMD_Deschedule(F_CMD_ARGS);
void CMD_Desk(F_CMD_ARGS);
void CMD_DesktopName(F_CMD_ARGS);
void CMD_DesktopSize(F_CMD_ARGS);
void CMD_Destroy(F_CMD_ARGS);
#ifdef USEDECOR
void CMD_DestroyDecor(F_CMD_ARGS);
#endif /* USEDECOR */
void CMD_DestroyFunc(F_CMD_ARGS);
void CMD_DestroyMenu(F_CMD_ARGS);
void CMD_DestroyMenuStyle(F_CMD_ARGS);
void CMD_DestroyModuleConfig(F_CMD_ARGS);
void CMD_DestroyStyle(F_CMD_ARGS);
void CMD_Direction(F_CMD_ARGS);
void CMD_Echo(F_CMD_ARGS);
void CMD_EdgeCommand(F_CMD_ARGS);
void CMD_EdgeResistance(F_CMD_ARGS);
void CMD_EdgeScroll(F_CMD_ARGS);
void CMD_EdgeThickness(F_CMD_ARGS);
void CMD_Emulate(F_CMD_ARGS);
void CMD_EscapeFunc(F_CMD_ARGS);
void CMD_EwmhBaseStruts(F_CMD_ARGS);
void CMD_EwmhNumberOfDesktops(F_CMD_ARGS);
void CMD_Exec(F_CMD_ARGS);
void CMD_ExecUseShell(F_CMD_ARGS);
void CMD_FakeClick(F_CMD_ARGS);
void CMD_FlipFocus(F_CMD_ARGS);
void CMD_Focus(F_CMD_ARGS);
void CMD_FocusStyle(F_CMD_ARGS);
void CMD_Function(F_CMD_ARGS);
void CMD_GlobalOpts(F_CMD_ARGS);
#ifdef GNOME
void CMD_GnomeButton(F_CMD_ARGS);
void CMD_GnomeShowDesks(F_CMD_ARGS);
#endif /* GNOME */
void CMD_GotoDesk(F_CMD_ARGS);
void CMD_GotoDeskAndPage(F_CMD_ARGS);
void CMD_GotoPage(F_CMD_ARGS);
void CMD_HideGeometryWindow(F_CMD_ARGS);
void CMD_HilightColor(F_CMD_ARGS);
void CMD_HilightColorset(F_CMD_ARGS);
void CMD_IconFont(F_CMD_ARGS);
void CMD_Iconify(F_CMD_ARGS);
void CMD_IconPath(F_CMD_ARGS);
void CMD_IgnoreModifiers(F_CMD_ARGS);
void CMD_ImagePath(F_CMD_ARGS);
void CMD_Key(F_CMD_ARGS);
void CMD_KillModule(F_CMD_ARGS);
void CMD_Layer(F_CMD_ARGS);
void CMD_Lower(F_CMD_ARGS);
void CMD_Maximize(F_CMD_ARGS);
void CMD_Menu(F_CMD_ARGS);
void CMD_MenuStyle(F_CMD_ARGS);
void CMD_Module(F_CMD_ARGS);
void CMD_ModulePath(F_CMD_ARGS);
void CMD_ModuleSynchronous(F_CMD_ARGS);
void CMD_ModuleTimeout(F_CMD_ARGS);
void CMD_Mouse(F_CMD_ARGS);
void CMD_Move(F_CMD_ARGS);
void CMD_MoveThreshold(F_CMD_ARGS);
void CMD_MoveToDesk(F_CMD_ARGS);
void CMD_MoveToPage(F_CMD_ARGS);
void CMD_MoveToScreen(F_CMD_ARGS);
void CMD_Next(F_CMD_ARGS);
void CMD_None(F_CMD_ARGS);
void CMD_Nop(F_CMD_ARGS);
void CMD_NoWindow(F_CMD_ARGS);
void CMD_OpaqueMoveSize(F_CMD_ARGS);
void CMD_Pick(F_CMD_ARGS);
void CMD_PipeRead(F_CMD_ARGS);
void CMD_PixmapPath(F_CMD_ARGS);
void CMD_PlaceAgain(F_CMD_ARGS);
void CMD_PointerKey(F_CMD_ARGS);
void CMD_PointerWindow(F_CMD_ARGS);
void CMD_Popup(F_CMD_ARGS);
void CMD_Prev(F_CMD_ARGS);
void CMD_PrintInfo(F_CMD_ARGS);
void CMD_PropertyChange(F_CMD_ARGS);
void CMD_Quit(F_CMD_ARGS);
void CMD_QuitScreen(F_CMD_ARGS);
void CMD_QuitSession(F_CMD_ARGS);
void CMD_Raise(F_CMD_ARGS);
void CMD_RaiseLower(F_CMD_ARGS);
void CMD_Read(F_CMD_ARGS);
void CMD_ReadWriteColors(F_CMD_ARGS);
void CMD_Recapture(F_CMD_ARGS);
void CMD_RecaptureWindow(F_CMD_ARGS);
void CMD_Refresh(F_CMD_ARGS);
void CMD_RefreshWindow(F_CMD_ARGS);
void CMD_Repeat(F_CMD_ARGS);
void CMD_Resize(F_CMD_ARGS);
void CMD_ResizeMaximize(F_CMD_ARGS);
void CMD_ResizeMove(F_CMD_ARGS);
void CMD_ResizeMoveMaximize(F_CMD_ARGS);
void CMD_RestackTransients(F_CMD_ARGS);
void CMD_Restart(F_CMD_ARGS);
void CMD_SaveQuitSession(F_CMD_ARGS);
void CMD_SaveSession(F_CMD_ARGS);
void CMD_Schedule(F_CMD_ARGS);
void CMD_Scroll(F_CMD_ARGS);
void CMD_Send_ConfigInfo(F_CMD_ARGS);
void CMD_Send_WindowList(F_CMD_ARGS);
void CMD_SendToModule(F_CMD_ARGS);
void CMD_set_mask(F_CMD_ARGS);
void CMD_set_nograb_mask(F_CMD_ARGS);
void CMD_set_sync_mask(F_CMD_ARGS);
void CMD_SetAnimation(F_CMD_ARGS);
void CMD_SetEnv(F_CMD_ARGS);
void CMD_Silent(F_CMD_ARGS);
void CMD_SnapAttraction(F_CMD_ARGS);
void CMD_SnapGrid(F_CMD_ARGS);
void CMD_State(F_CMD_ARGS);
void CMD_Stick(F_CMD_ARGS);
void CMD_StickDesk(F_CMD_ARGS);
void CMD_StickPage(F_CMD_ARGS);
#ifdef HAVE_STROKE
void CMD_Stroke(F_CMD_ARGS);
void CMD_StrokeFunc(F_CMD_ARGS);
#endif /* HAVE_STROKE */
void CMD_Style(F_CMD_ARGS);
void CMD_TearMenuOff(F_CMD_ARGS);
void CMD_ThisWindow(F_CMD_ARGS);
void CMD_Title(F_CMD_ARGS);
void CMD_TitleStyle(F_CMD_ARGS);
void CMD_Unalias(F_CMD_ARGS);
void CMD_UnsetEnv(F_CMD_ARGS);
void CMD_UpdateDecor(F_CMD_ARGS);
void CMD_UpdateStyles(F_CMD_ARGS);
void CMD_Wait(F_CMD_ARGS);
void CMD_WarpToWindow(F_CMD_ARGS);
void CMD_WindowFont(F_CMD_ARGS);
void CMD_WindowId(F_CMD_ARGS);
void CMD_WindowList(F_CMD_ARGS);
void CMD_WindowShade(F_CMD_ARGS);
void CMD_WindowShadeAnimate(F_CMD_ARGS);
void CMD_Xinerama(F_CMD_ARGS);
void CMD_XineramaPrimaryScreen(F_CMD_ARGS);
void CMD_XineramaSls(F_CMD_ARGS);
void CMD_XineramaSlsScreens(F_CMD_ARGS);
void CMD_XineramaSlsSize(F_CMD_ARGS);
void CMD_XorPixmap(F_CMD_ARGS);
void CMD_XorValue(F_CMD_ARGS);
void CMD_XSync(F_CMD_ARGS);
void CMD_XSynchronize(F_CMD_ARGS);

#endif /* COMMANDS_H */
