/* -*-c-*- */

#ifndef _STACK_H
#define _STACK_H

#define DEBUG_STACK_RING 1
#ifdef DEBUG_STACK_RING
void verify_stack_ring_consistency(void);
#endif
void remove_window_from_stack_ring(FvwmWindow *t);
void add_window_to_stack_ring_after(FvwmWindow *t, FvwmWindow *add_after_win);
FvwmWindow *get_next_window_in_stack_ring(const FvwmWindow *t);
FvwmWindow *get_prev_window_in_stack_ring(const FvwmWindow *t);
FvwmWindow *get_transientfor_fvwmwindow(const FvwmWindow *t);
Bool position_new_window_in_stack_ring(FvwmWindow *t, Bool do_lower);
void RaiseWindow(FvwmWindow *t, Bool is_client_request);
void LowerWindow(FvwmWindow *t, Bool is_client_request);
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
Bool is_on_top_of_layer_and_above_unmanaged(FvwmWindow *t);

/* This function recursively finds the transients of the window t and sets
 * their is_in_transient_subtree flag.  If a layer is given, only windows in
 * this layer are checked.  If the layer is < 0, all windows are considered. */
#define MARK_RAISE   0
#define MARK_LOWER   1
#define MARK_RESTACK 2
#define MARK_ALL     3
#define MARK_ALL_LAYERS -1
void mark_transient_subtree(
	FvwmWindow *t, int layer, int mark_mode, Bool do_ignore_icons,
	Bool use_window_group_hint);


#endif /* _STACK_H */
