/* -*-c-*- */
#ifndef _CURSOR_STUFF_H
#define _CURSOR_STUFF_H

#include <X11/Xlib.h>

/* copied and modified from Paul Sheer's xdndv2 library.*/
typedef struct XdndCursorStruct {
  int w;
  int h;
  int x;
  int y;  /*width,height,x offset, y offset*/
  unsigned char *imageData, *maskData; /*Used for initialization*/
  char *actionStr; /*Action string the cursor is associated with.*/
  Atom action;  /*Action the cursor is associated with.*/
  Cursor cursor;  /*The actual cursor*/
} XdndCursor;

#endif
