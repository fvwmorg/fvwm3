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

/* This module is all new
 * by Rob Nation
 *
 * This code handles colormaps for fvwm.
 *
 * Copyright 1994 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved . No guarantees or
 * warrantees of any sort whatsoever are given or implied or anything.
 */

#ifndef COLORMAP_H
#define COLORMAP_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

/* HandleColormapNotify - colormap notify event handler
 *
 * This procedure handles both a client changing its own colormap, and
 * a client explicitly installing its colormap itself (only the window
 * manager should do that, so we must set it correctly). */
void colormap_handle_colormap_notify(const evh_args_t *ea);

/* Re-Install the active colormap */
void ReInstallActiveColormap(void);

/* InstallWindowColormaps - install the colormaps for one fvwm window
 *
 *  Inputs:
 *      type    - type of event that caused the installation
 *      tmp     - for a subset of event types, the address of the
 *                window structure, whose colormaps are to be installed. */
void InstallWindowColormaps(const FvwmWindow *tmp);

/* <Uni/I>nstallRootColormap - Force (un)loads root colormap(s)
 *
 * These matching routines provide a mechanism to insure that
 * the root colormap(s) is installed during operations like
 * rubber banding that require colors from
 * that colormap.  Calls may be nested arbitrarily deeply,
 * as long as there is one UninstallRootColormap call per
 * InstallRootColormap call.
 *
 * The final UninstallRootColormap will cause the colormap list
 * which would otherwise have be loaded to be loaded, unless
 * Enter or Leave Notify events are queued, indicating some
 * other colormap list would potentially be loaded anyway. */
void InstallRootColormap(void);

/* Unstacks one layer of root colormap pushing
 * If we peel off the last layer, re-install the application colormap  */
void UninstallRootColormap(void);

/* <Uni/I>nstallFvwmColormap - Force (un)loads fvwm colormap(s)
 *
 * This is used to ensure the fvwm colormap is installed during
 * menu operations */
void InstallFvwmColormap(void);
void UninstallFvwmColormap(void);

/* Gets the WM_COLORMAP_WINDOWS property from the window
 *
 * This property typically doesn't exist, but a few applications
 * use it. These seem to occur mostly on SGI machines. */
void FetchWmColormapWindows (FvwmWindow *tmp);

/* clasen@mathematik.uni-freiburg.de - 03/01/1999 - new
 * boolean for handling of client-side InstallColormap
 * as described in the ICCCM 2.0 */

void set_client_controls_colormaps(Bool flag);

/* Looks through the window list for any matching COLORMAP_WINDOWS
 * windows and installs the colormap if one exists. */
void EnterSubWindowColormap(Window win);
void LeaveSubWindowColormap(Window win);

#endif /* COLORMAP_H */
