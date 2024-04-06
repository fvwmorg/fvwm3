/* -*-c-*- */

#ifndef FVWM_MOVE_RESIZE_H
#define FVWM_MOVE_RESIZE_H

#include "fvwm.h"
#include "execcontext.h"

struct MenuRepaintTransparentParameters;

void switch_move_resize_grid(Bool state);
void AnimatedMoveOfWindow(Window w, position start, position end,
    Bool fWarpPointerToo, int cusDelay, float *ppctMovement,
    struct MenuRepaintTransparentParameters *pmrtp);
void AnimatedMoveFvwmWindow(FvwmWindow *fw, Window w, position start,
    position end, Bool fWarpPointerToo, int cmsDelay, float *ppctMovement);
Bool __move_loop(const exec_context_t *exc, position offset, size_rect sz,
    position *pFinal, Bool do_move_opaque, int cursor);
int  is_window_sticky_across_pages(FvwmWindow *fw);
int  is_window_sticky_across_desks(FvwmWindow *fw);
void handle_stick(F_CMD_ARGS, int toggle_page, int toggle_desk, int do_not_draw,
    int do_silently);
void resize_geometry_window(void);
void __move_icon(FvwmWindow *fw, position new, position old,
    Bool do_move_animated, Bool do_warp_pointer);
int  placement_binding(int button, KeySym keysym, int modifier, char *action);
int  GetMoveArguments(FvwmWindow *fw, char **paction, size_rect s,
     position *pFinal, Bool *fWarp, Bool *fPointer, Bool fKeep);

#endif /* FVWM_MOVE_RESIZE_H */
