/* -*-c-*- */

#ifndef ICCCM2_h
#define ICCCM2_h

extern void SetupICCCM2(Bool replace_wm);
extern void CloseICCCM2(void);
extern void icccm2_handle_selection_request(const XEvent *e);
extern void icccm2_handle_selection_clear(void);

#endif /* ICCCM2_H */
