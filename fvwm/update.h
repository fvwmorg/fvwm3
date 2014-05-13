/* -*-c-*- */

#ifndef _UPDATE_
#define _UPDATE_

/* Ipmortant not:  All the flags below must have a positive syntax.  If a flag
 * is set, this indicates that something has to happen with the window. */

typedef struct
{
	unsigned do_broadcast_focus : 1;
	unsigned do_redecorate : 1;
	unsigned do_redecorate_transient : 1;
	unsigned do_redraw_decoration : 1;
	unsigned do_redraw_icon : 1;
	unsigned do_refresh : 1;
	unsigned do_resize_window : 1;
	unsigned do_setup_focus_policy : 1;
	unsigned do_setup_frame : 1;
	unsigned do_update_cr_motion_method : 1;
	unsigned do_update_ewmh_allowed_actions : 1;
	unsigned do_update_ewmh_icon : 1;
	unsigned do_update_ewmh_mini_icon : 1;
	unsigned do_update_ewmh_stacking_hints : 1;
	unsigned do_update_ewmh_state_hints : 1;
	unsigned do_update_frame_attributes : 1;
	unsigned do_update_icon : 1;
	unsigned do_update_icon_background_cs : 1;
	unsigned do_update_icon_boxes : 1;
	unsigned do_update_icon_font : 1;
	unsigned do_update_icon_placement : 1;
	unsigned do_update_icon_size_limits : 1;
	unsigned do_update_icon_title : 1;
	unsigned do_update_icon_title_cs : 1;
	unsigned do_update_icon_title_cs_hi : 1;
	unsigned do_update_mini_icon : 1;
	unsigned do_update_layer : 1;
	unsigned do_update_modules_flags : 1;
	unsigned do_update_placement_penalty : 1;
	unsigned do_update_rotated_title : 1;
	unsigned do_update_stick : 1;
	unsigned do_update_stick_icon : 1;
	unsigned do_update_title_dir : 1;
	unsigned do_update_title_text_dir : 1;
	unsigned do_update_visible_icon_name : 1;
	unsigned do_update_visible_window_name : 1;
	unsigned do_update_window_color : 1;
	unsigned do_update_window_color_hi : 1;
	unsigned do_update_window_font : 1;
	unsigned do_update_window_font_height : 1;
	unsigned do_update_window_grabs : 1;
	unsigned do_update_working_area : 1;
} update_win;

void destroy_scheduled_windows(void);
void apply_decor_change(FvwmWindow *fw);
void flush_window_updates(void);

#endif /* _UPDATE_ */
