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

#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

typedef struct
{
	const exec_context_t *exc;
} evh_args_t;

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

/* ---------------------------- event handlers ------------------------------ */

void HandleEvents(void);
void HandleExpose(const evh_args_t *ea);
void HandleFocusIn(const evh_args_t *ea);
void HandleFocusOut(const evh_args_t *ea);
void HandleDestroyNotify(const evh_args_t *ea);
void HandleMapRequest(const evh_args_t *ea);
void HandleMapRequestKeepRaised(
	const evh_args_t *ea, Window KeepRaised, FvwmWindow *ReuseWin,
	initial_window_options_type *win_opts);
void HandleMapNotify(const evh_args_t *ea);
void HandleUnmapNotify(const evh_args_t *ea);
void HandleMotionNotify(const evh_args_t *ea);
void HandleButtonRelease(const evh_args_t *ea);
void HandleButtonPress(const evh_args_t *ea);
void HandleEnterNotify(const evh_args_t *ea);
void HandleLeaveNotify(const evh_args_t *ea);
void HandleConfigureRequest(const evh_args_t *ea);
void HandleClientMessage(const evh_args_t *ea);
void HandlePropertyNotify(const evh_args_t *ea);
void HandleKeyPress(const evh_args_t *ea);
void HandleVisibilityNotify(const evh_args_t *ea);
STROKE_CODE(void HandleButtonRelease(const evh_args_t *ea));
STROKE_CODE(void HandleMotionNotify(const evh_args_t *ea));

#endif /* EVENTHANDLER_H */
