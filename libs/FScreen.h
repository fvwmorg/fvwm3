#ifndef FVWMLIB_FSCRREN_H
#define FVWMLIB_FSCRREN_H

/* needs X11/Xlib.h and X11/Xutil.h */

enum
{
  FSCREEN_GLOBAL  = -1,
  FSCREEN_CURRENT = -2,
  FSCREEN_PRIMARY = -3,
  FSCREEN_XYPOS   = -4
};

enum
{
  FSCREEN_SPEC_GLOBAL = 'g',
  FSCREEN_SPEC_CURRENT = 'c',
  FSCREEN_SPEC_PRIMARY = 'p'
};

typedef union
{
  XEvent *mouse_ev;
  struct
  {
    int x;
    int y;
  } xypos;
} fscreen_scr_arg;

/* Control */
Bool FScreenIsEnabled(void);
Bool FScreenIsSLSEnabled(void);
void FScreenInit(Display *dpy);
void FScreenOnOff(Bool do_enable);
Bool FScreenConfigureSLS(int width, int height);
void FScreenSLSOnOff(Bool do_enable);
/* Intended to be called by modules.  Simply pass in the parameter from the
 * config string sent by fvwm. */
void FScreenConfigureModule(char *args);
const char* FScreenGetConfiguration(void); /* For use by FVWM */
void FScreenDisableRandR(void);

void FScreenSetPrimaryScreen(int scr);

/* Screen info */
Bool FScreenGetScrRect(
  fscreen_scr_arg *arg, int screen, int *x, int *y, int *w, int *h);
void FScreenTranslateCoordinates(
  fscreen_scr_arg *arg_src, int screen_src,
  fscreen_scr_arg *arg_dest, int screen_dest,
  int *x, int *y);
void FScreenGetResistanceRect(
  int wx, int wy, int ww, int wh, int *x0, int *y0, int *x1, int *y1);
Bool FScreenIsRectangleOnScreen(
  fscreen_scr_arg *arg, int screen, rectangle *rec);

/* Clipping/positioning */
int FScreenClipToScreen(
  fscreen_scr_arg *arg, int screen, int *x, int *y, int w, int h);
void FScreenCenterOnScreen(
  fscreen_scr_arg *arg, int screen, int *x, int *y, int w, int h);

/* Geometry management */
int FScreenGetScreenArgument(char *scr_spec, char default_screen);
int FScreenParseGeometryWithScreen(
  char *parsestring, int *x_return, int *y_return, unsigned int *width_return,
  unsigned int *height_return, int *screen_return);
int FScreenParseGeometry(
  char *parsestring, int *x_return, int *y_return, unsigned int *width_return,
  unsigned int *height_return);
int  FScreenGetGeometry(
  char *parsestring, int *x_return, int *y_return,
  int *width_return, int *height_return, XSizeHints *hints, int flags);

/* RandR support */
int  FScreenGetRandrEventType(void);
Bool FScreenHandleRandrEvent(
  XEvent *event, int *old_w, int *old_h, int *new_w, int *new_h);

#endif /* FVWMLIB_FSCRREN_H */
