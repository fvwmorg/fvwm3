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

/* This file contains all command prototypes. */
void CMD_Plus(F_CMD_ARGS);
void CMD_AddButtonStyle(F_CMD_ARGS);
void CMD_AddTitleStyle(F_CMD_ARGS);
#ifdef USEDECOR
void CMD_AddToDecor(F_CMD_ARGS);
#endif /* USEDECOR */
void CMD_AddToFunc(F_CMD_ARGS);
void CMD_AddToMenu(F_CMD_ARGS);
void CMD_All(F_CMD_ARGS);
void CMD_AnimatedMove(F_CMD_ARGS);
void CMD_Beep(F_CMD_ARGS);
void CMD_BorderStyle(F_CMD_ARGS);
void CMD_BugOpts(F_CMD_ARGS);
void CMD_BusyCursor(F_CMD_ARGS);
void CMD_ButtonState(F_CMD_ARGS);
void CMD_ButtonStyle(F_CMD_ARGS);
#ifdef USEDECOR
void CMD_ChangeDecor(F_CMD_ARGS);
#endif /* USEDECOR */
void CMD_ChangeMenuStyle(F_CMD_ARGS);
void CMD_ClickTime(F_CMD_ARGS);
void CMD_Close(F_CMD_ARGS);
void CMD_ColorLimit(F_CMD_ARGS);
void CMD_ColormapFocus(F_CMD_ARGS);
void CMD_Colorset(F_CMD_ARGS);
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
void CMD_Desk(F_CMD_ARGS);
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
void CMD_EdgeResistance(F_CMD_ARGS);
void CMD_EdgeScroll(F_CMD_ARGS);
void CMD_EdgeThickness(F_CMD_ARGS);
void CMD_Emulate(F_CMD_ARGS);
void CMD_EscapeFunc(F_CMD_ARGS);
void CMD_Exec(F_CMD_ARGS);
void CMD_ExecUseShell(F_CMD_ARGS);
void CMD_FakeClick(F_CMD_ARGS);
void CMD_FlipFocus(F_CMD_ARGS);
void CMD_Focus(F_CMD_ARGS);
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
void CMD_OpaqueMoveSize(F_CMD_ARGS);
void CMD_Pick(F_CMD_ARGS);
void CMD_PipeRead(F_CMD_ARGS);
void CMD_PixmapPath(F_CMD_ARGS);
void CMD_PlaceAgain(F_CMD_ARGS);
void CMD_PointerKey(F_CMD_ARGS);
void CMD_Popup(F_CMD_ARGS);
void CMD_Prev(F_CMD_ARGS);
void CMD_Quit(F_CMD_ARGS);
void CMD_QuitScreen(F_CMD_ARGS);
void CMD_QuitSession(F_CMD_ARGS);
void CMD_Raise(F_CMD_ARGS);
void CMD_RaiseLower(F_CMD_ARGS);
void CMD_Read(F_CMD_ARGS);
void CMD_Recapture(F_CMD_ARGS);
void CMD_RecaptureWindow(F_CMD_ARGS);
void CMD_Refresh(F_CMD_ARGS);
void CMD_RefreshWindow(F_CMD_ARGS);
void CMD_Repeat(F_CMD_ARGS);
void CMD_Resize(F_CMD_ARGS);
void CMD_ResizeMove(F_CMD_ARGS);
void CMD_Restart(F_CMD_ARGS);
void CMD_SaveQuitSession(F_CMD_ARGS);
void CMD_SaveSession(F_CMD_ARGS);
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
void CMD_Stick(F_CMD_ARGS);
#ifdef HAVE_STROKE
void CMD_Stroke(F_CMD_ARGS);
void CMD_StrokeFunc(F_CMD_ARGS);
#endif /* HAVE_STROKE */
void CMD_Style(F_CMD_ARGS);
void CMD_Title(F_CMD_ARGS);
void CMD_TitleStyle(F_CMD_ARGS);
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
void CMD_XineramaSlsSize(F_CMD_ARGS);
void CMD_XorPixmap(F_CMD_ARGS);
void CMD_XorValue(F_CMD_ARGS);

#endif /* COMMANDS_H */
