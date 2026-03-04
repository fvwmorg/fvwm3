/* -*-c-*- */

#ifndef FVWM_COMMANDS_H
#define FVWM_COMMANDS_H

/* ---------------------------- included header files ---------------------- */
#include "fvwm.h"

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

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
	F_CURSOR_BARRIER,
	F_CURSOR_STYLE,
	F_DESCHEDULE,
	F_DESKTOP_CONFIGURATION,
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
	F_EDGE_LEAVE_COMMAND,
	F_EDGE_RES,
	F_EDGE_SCROLL,
	F_EMULATE,
	F_ESCAPE_FUNC,
	F_EWMH_BASE_STRUTS,
	F_EWMH_NUMBER_OF_DESKTOPS,
	F_EXEC,
	F_EXEC_SETUP,
	F_FAKE_CLICK,
	F_FAKE_KEYPRESS,
	F_FOCUSSTYLE,
	F_FUNCTION,
	F_GEOMWINDOW,
	F_GLOBAL_OPTS,
	F_GOTO_DESK,
	F_GOTO_PAGE,
	F_HICOLOR,
	F_IGNORE_MODIFIERS,
	F_IMAGE_PATH,
	F_INFOSTOREADD,
	F_INFOSTOREREMOVE,
	F_INFOSTORECLEAR,
	F_KEEPRC,
	F_KEY,
	F_KILL_MODULE,
	F_LAYER,
	F_LOCALE_PATH,
	F_MENUSTYLE,
	F_MODULE,
	F_MODULE_LISTEN_ONLY,
	F_MODULE_PATH,
	F_MODULE_SYNC,
	F_MOUSE,
	F_MOVECURSOR,
	F_MOVE_TO_DESK,
	F_NEXT,
	F_NONE,
	F_OPAQUE,
	F_PICK,
	F_POINTERKEY,
	F_POINTERWINDOW,
	F_POPUP,
	F_PREV,
	F_PRINTINFO,
	F_QUIT,
	F_QUIT_SESSION,
	F_READ,
	F_REFRESH,
	F_RESTART,
	F_SAVE_SESSION,
	F_SAVE_QUIT_SESSION,
	F_SCANFORWINDOW,
	F_SCHEDULE,
	F_SCROLL,
	F_SETDESK,
	F_SETENV,
	F_SET_ANIMATION,
	F_SET_MASK,
	F_SET_NOGRAB_MASK,
	F_SET_SYNC_MASK,
	F_SILENT,
	F_STAYSUP,
	F_STATUS,
	F_STYLE,
	F_TEARMENUOFF,
	F_TEST_,
	F_TESTRC,
	F_THISWINDOW,
	F_TITLE,
	F_TITLESTYLE,
	F_TOGGLE_PAGE,
	F_UPDATE_STYLES,
	F_WAIT,
	F_WINDOWLIST,
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
	F_DELETE,
	F_DESTROY,
	F_DESTROY_DECOR,
	F_DESTROY_MOD,
	F_DESTROY_WINDOW_STYLE,
	F_ECHO,
	F_ECHO_FUNC_DEFINITION,
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
	F_STICKACROSSDESKS,
	F_STICKACROSSPAGES,
	F_UPDATE_DECOR,
	F_WARP,
	F_WINDOWID,
	F_WINDOW_SHADE,
	F_WINDOW_STYLE,

	F_END_OF_LIST = 999,

	/* Functions for use by modules only! */
	F_SEND_WINDOW_LIST = 1000,
	F_SEND_REPLY
};

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

/* This file contains all command prototypes. */
void CMD_Plus(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_AddButtonStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_AddTitleStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_AddToDecor(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_AddToFunc(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_AddToMenu(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Alias(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_All(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_AnimatedMove(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Any(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Beep(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Break(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_BorderStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_BugOpts(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_BusyCursor(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ButtonState(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ButtonStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ChangeDecor(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ChangeMenuStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_CleanupColorsets(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ClickTime(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Close(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_ColormapFocus(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Colorset(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_CopyMenuStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Current(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_CursorBarrier(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_CursorMove(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_CursorStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DefaultFont(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DefaultIcon(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DefaultLayers(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Delete(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Deschedule(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DesktopConfiguration(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DesktopName(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DesktopSize(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Destroy(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_DestroyDecor(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DestroyFunc(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DestroyMenu(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DestroyMenuStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DestroyModuleConfig(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DestroyStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_DestroyWindowStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Direction(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Echo(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_EchoFuncDefinition(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_EdgeCommand(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_EdgeLeaveCommand(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_EdgeResistance(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_EdgeScroll(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_EdgeThickness(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Emulate(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_EscapeFunc(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_EwmhBaseStruts(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_EwmhNumberOfDesktops(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Exec(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_ExecUseShell(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_FakeClick(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_FakeKeypress(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_FlipFocus(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Focus(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_FocusStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Function(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_GeometryWindow(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_GotoDesk(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_GotoDeskAndPage(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_GotoPage(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Iconify(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_IgnoreModifiers(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ImagePath(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_InfoStoreAdd(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_InfoStoreClear(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_InfoStoreRemove(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_KeepRc(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Key(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_KillModule(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Layer(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_LocalePath(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Lower(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Maximize(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Menu(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_MenuStyle(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Module(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_ModuleListenOnly(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ModulePath(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ModuleSynchronous(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ModuleTimeout(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Mouse(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Move(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_MoveThreshold(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_MoveToDesk(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_MoveToPage(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_MoveToScreen(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Next(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_None(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Nop(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_NoWindow(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_OpaqueMoveSize(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Pick(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_PipeRead(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_PlaceAgain(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_PointerKey(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_PointerWindow(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Popup(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Prev(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_PrintInfo(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_PropertyChange(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Quit(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_QuitSession(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Raise(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_RaiseLower(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Read(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_ReadWriteColors(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Refresh(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_RefreshWindow(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Repeat(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Resize(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_ResizeMaximize(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ResizeMove(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ResizeMoveMaximize(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_RestackTransients(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Restart(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_SaveQuitSession(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_SaveSession(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_ScanForWindow(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Schedule(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Scroll(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Send_ConfigInfo(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Send_Reply(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Send_WindowList(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_SendToModule(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Status(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_set_mask(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_set_nograb_mask(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_set_sync_mask(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_SetAnimation(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_SetEnv(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Silent(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_State(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_Stick(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_StickAcrossDesks(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_StickAcrossPages(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Style(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_TearMenuOff(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Test(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_TestRc(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_ThisWindow(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Title(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_TitleStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Unalias(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_UnsetEnv(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_UpdateDecor(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_UpdateStyles(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_Wait(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_WarpToWindow(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_WindowId(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_WindowList(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_WindowShade(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_WindowStyle(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);
void CMD_XorPixmap(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_XorValue(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_XSync(cond_rc_t *cond_rc, const exec_context_t *exc, char *action,
	cmdparser_context_t *pc);
void CMD_XSynchronize(cond_rc_t *cond_rc, const exec_context_t *exc,
	char *action, cmdparser_context_t *pc);

#endif /* FVWM_COMMANDS_H */
