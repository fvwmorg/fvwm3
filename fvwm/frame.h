/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#ifndef FRAME_H
#define FRAME_H

/* ---------------------------- included header files ----------------------- */

/* ---------------------------- global definitions -------------------------- */

/* ---------------------------- global macros ------------------------------- */

/* ---------------------------- type definitions ---------------------------- */

typedef enum
{
	FRAME_MR_SETUP,
	FRAME_MR_OPAQUE,
	FRAME_MR_SHRINK,
	FRAME_MR_SCROLL,
        /* used internally only, do not set this in any calls */
	FRAME_MR_FORCE_SETUP
} frame_move_resize_mode;

typedef struct
{
        rectangle titlebar_g;
        rectangle title_g;
        rectangle button_g[NUMBER_OF_BUTTONS];
} frame_title_layout_type;

/* details are hidden in frame.c */
typedef void *frame_move_resize_args;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- interface functions ------------------------- */

void frame_init(void);
Bool is_frame_hide_window(
	Window w);
void frame_destroyed_frame(
	Window frame_w);
frame_move_resize_args frame_create_move_resize_args(
	FvwmWindow *fw, frame_move_resize_mode mr_mode,
	rectangle *start_g, rectangle *end_g, int anim_steps, int shade_dir);
void frame_update_move_resize_args(
	frame_move_resize_args mr_args, rectangle *end_g);
void frame_free_move_resize_args(
	FvwmWindow *fw, frame_move_resize_args mr_args);
void frame_get_titlebar_dimensions(
	FvwmWindow *fw, rectangle *frame_g, rectangle *diff_g,
        frame_title_layout_type *title_layout);
void frame_get_sidebar_geometry(
	FvwmWindow *fw, DecorFaceStyle *borderstyle, rectangle *frame_g,
	rectangle *ret_g, Bool *ret_has_x_marks, Bool *ret_has_y_marks);
int frame_window_id_to_context(
	FvwmWindow *fw, Window w, int *ret_num);
void frame_move_resize(
	FvwmWindow *fw, frame_move_resize_args mr_args);
void frame_setup_window(
	FvwmWindow *fw, int x, int y, int w, int h,
	Bool do_send_configure_notify);
void frame_force_setup_window(
	FvwmWindow *fw, int x, int y, int w, int h,
	Bool do_send_configure_notify);
void frame_setup_shape(
	FvwmWindow *fw, int w, int h);

#endif /* FRAME_H */
