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

void remove_window_from_stack_ring(FvwmWindow *t);
void add_window_to_stack_ring_after(FvwmWindow *t, FvwmWindow *add_after_win);
FvwmWindow *get_next_window_in_stack_ring(FvwmWindow *t);
FvwmWindow *get_prev_window_in_stack_ring(FvwmWindow *t);
Bool position_new_window_in_stack_ring(FvwmWindow *t, Bool do_lower);
void RaiseWindow(FvwmWindow *t);
void LowerWindow(FvwmWindow *t);
Bool HandleUnusualStackmodes(unsigned int stack_mode,
			     FvwmWindow *r, Window rw,
			     FvwmWindow *sib, Window sibw);
void BroadcastRestackAllWindows(void);
void BroadcastRestackThisWindow(FvwmWindow *t);

int compare_window_layers(FvwmWindow *t, FvwmWindow *s);
void set_default_layer(FvwmWindow *t, int layer);
void set_layer(FvwmWindow *t, int layer);
int get_layer(FvwmWindow *t);
void new_layer(FvwmWindow *t, int layer);

void init_stack_and_layers(void);
Bool is_on_top_of_layer(FvwmWindow *t);

void raiselower_func(F_CMD_ARGS);
void raise_function(F_CMD_ARGS);
void lower_function(F_CMD_ARGS);
void change_layer(F_CMD_ARGS);
void SetDefaultLayers(F_CMD_ARGS);


#endif /* _STACK_H */
