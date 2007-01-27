#ifndef LIB_WINMAGIC
#define LIB_WINMAGIC

void SlideWindow(
	Display *dpy, Window win,
	int s_x, int s_y, int s_w, int s_h,
	int e_x, int e_y, int e_w, int e_h,
	int steps, int delay_ms, float *ppctMovement,
	Bool do_sync, Bool use_hints);

Window GetTopAncestorWindow(Display *dpy, Window child);

int GetEqualSizeChildren(
	Display *dpy, Window parent, int depth, VisualID visualid,
	Colormap colormap, Window **ret_children);

#endif /* LIB_WINMAGIC */
