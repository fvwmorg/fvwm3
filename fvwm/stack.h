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

#ifndef _STACK_H
#define _STACK_H

#define DEBUG_STACK_RING 1
#ifdef DEBUG_STACK_RING
void verify_stack_ring_consistency(void);
#endif
void remove_window_from_stack_ring(FvwmWindow *t);
void add_window_to_stack_ring_after(FvwmWindow *t, FvwmWindow *add_after_win);
FvwmWindow *get_next_window_in_stack_ring(FvwmWindow *t);
FvwmWindow *get_prev_window_in_stack_ring(FvwmWindow *t);
FvwmWindow *get_transientfor_fvwmwindow(FvwmWindow *t);
Bool position_new_window_in_stack_ring(FvwmWindow *t, Bool do_lower);
void RaiseWindow(FvwmWindow *t);
void LowerWindow(FvwmWindow *t);
Bool HandleUnusualStackmodes(
	unsigned int stack_mode, FvwmWindow *r, Window rw, FvwmWindow *sib,
	Window sibw);
void BroadcastRestackAllWindows(void);
void BroadcastRestackThisWindow(FvwmWindow *t);

int compare_window_layers(FvwmWindow *t, FvwmWindow *s);
void set_default_layer(FvwmWindow *t, int layer);
void set_layer(FvwmWindow *t, int layer);
int get_layer(FvwmWindow *t);
void new_layer(FvwmWindow *t, int layer);

void init_stack_and_layers(void);
Bool is_on_top_of_layer(FvwmWindow *t);

/* This function recursively finds the transients of the window t and sets their
 * is_in_transient_subtree flag.  If a layer is given, only windows in this
 * layer are checked.  If the layer is < 0, all windows are considered.
 */
#define MARK_RAISE 0
#define MARK_LOWER 1
#define MARK_ALL   2
#define MARK_ALL_LAYERS -1
void mark_transient_subtree(
	FvwmWindow *t, int layer, int mark_mode, Bool do_ignore_icons,
	Bool use_window_group_hint);


#endif /* _STACK_H */
