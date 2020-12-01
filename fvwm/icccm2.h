/* -*-c-*- */

#ifndef FVWM_ICCCM2_H
#define FVWM_ICCCM2_H

#include "libs/fvwm_x11.h"

extern void SetupICCCM2(Bool replace_wm);
extern void CloseICCCM2(void);
extern void icccm2_handle_selection_request(const XEvent *e);
extern void icccm2_handle_selection_clear(void);

#endif /* FVWM_ICCCM2_H */
