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

#ifndef BUILTINS_H
#define BUILTINS_H

void HandleColorset(F_CMD_ARGS);
void WindowShade(F_CMD_ARGS);
void setShadeAnim(F_CMD_ARGS);
void set_animation(F_CMD_ARGS);
void ButtonStyle(F_CMD_ARGS);
#ifdef MULTISTYLE
void AddButtonStyle(F_CMD_ARGS);
#endif
#ifdef USEDECOR
void add_item_to_decor(F_CMD_ARGS);
void ChangeDecor(F_CMD_ARGS);
void DestroyDecor(F_CMD_ARGS);
#endif
void UpdateDecor(F_CMD_ARGS);
void SetColormapFocus(F_CMD_ARGS);
void SetColorLimit(F_CMD_ARGS);
void Bell(F_CMD_ARGS);
void movecursor(F_CMD_ARGS);
void destroy_function(F_CMD_ARGS);
void delete_function(F_CMD_ARGS);
void close_function(F_CMD_ARGS);
void restart_function(F_CMD_ARGS);
void exec_function(F_CMD_ARGS);
void exec_setup(F_CMD_ARGS);
void refresh_function(F_CMD_ARGS);
void refresh_win_function(F_CMD_ARGS);
void stick_function(F_CMD_ARGS);
void wait_func(F_CMD_ARGS);
void quit_func(F_CMD_ARGS);
void quit_screen_func(F_CMD_ARGS);
void echo_func(F_CMD_ARGS);
void Nop_func(F_CMD_ARGS);
void SetGlobalOptions(F_CMD_ARGS);
void SetBugOptions(F_CMD_ARGS);
void Emulate(F_CMD_ARGS);
void destroy_fvwmfunc(F_CMD_ARGS);
void add_another_item(F_CMD_ARGS);
void add_item_to_func(F_CMD_ARGS);
void setModulePath(F_CMD_ARGS);
void imagePath_function(F_CMD_ARGS);
void iconPath_function(F_CMD_ARGS);
void pixmapPath_function(F_CMD_ARGS);
void SetHiColor(F_CMD_ARGS);
void SetDefaultColors(F_CMD_ARGS);
void SetDefaultIcon(F_CMD_ARGS);
void LoadDefaultFont(F_CMD_ARGS);
void LoadIconFont(F_CMD_ARGS);
void LoadWindowFont(F_CMD_ARGS);
void SetTitleStyle(F_CMD_ARGS);
#ifdef MULTISTYLE
void AddTitleStyle(F_CMD_ARGS);
#endif
void SetClick(F_CMD_ARGS);
void Recapture(F_CMD_ARGS);
void RecaptureWindow(F_CMD_ARGS);
void SetEnv(F_CMD_ARGS);

#endif /* BUILTINS_H */
