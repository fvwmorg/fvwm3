/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */

/* ---------------------------- included header files ---------------------- */

#ifndef FVWMLIB_FRENDER_INIT_H
#define FVWMLIB_FRENDER_INIT_H

#include "fvwm_x11.h"

void FRenderInit(Display *dpy);
int FRenderGetAlphaDepth(void);
int FRenderGetErrorCodeBase(void);
int FRenderGetMajorOpCode(void);
Bool FRenderGetErrorText(int code, char *msg);
Bool FRenderGetExtensionSupported(void);

#endif
