/* -*-c-*- */

#ifndef BUILTINS_H
#define BUILTINS_H

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__sun__)
#include <libbson-1.0/bson.h>
#else
#include <bson/bson.h>
#endif

void refresh_window(Window w, Bool window_update);
void ApplyDefaultFontAndColors(void);
void InitFvwmDecor(FvwmDecor *decor);
void reset_decor_changes(void);
Bool ReadDecorFace(char *s, DecorFace *df, int button, int verbose);
void FreeDecorFace(Display *dpy, DecorFace *df);
void update_fvwm_colorset(int cset);
void status_send(void);

#endif /* BUILTINS_H */
