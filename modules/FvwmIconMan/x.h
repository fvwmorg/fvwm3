#ifndef IN_X_H
#define IN_X_H

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

extern Display *theDisplay;
extern Window theRoot;
extern int theDepth, theScreen;

extern void unmap_manager (WinManager *man);
extern void map_manager (WinManager *man);

extern Window find_frame_window (Window win, int *off_x, int *off_y);

extern void init_display (void);
extern void xevent_loop (void);
extern void create_manager_window (int man_id);
extern void X_init_manager (int man_id);

#endif
