/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */

/* ---------------------------- included header files ---------------------- */

#ifndef F_RENDER_INIT_H
#define F_RENDER_INIT_H

void FRenderInit(Display *dpy);
int FRenderGetAlphaDepth(void);
int FRenderGetErrorCodeBase(void);
int FRenderGetMajorOpCode(void);
Bool FRenderGetErrorText(int code, char *msg);
Bool FRenderGetExtensionSupported(void);

#endif

