
/* stuff to enable modules to use fvwm visual/colormap/GCs */
#define DEFGRAPHSTR "Default_Graphics "
#define DEFGRAPHLEN 17 /* length of above string */
#define DEFGRAPHNUM 9 /* number of items sent */

typedef struct GraphicsThing {
  Bool create_drawGC : 1;
  Bool create_foreGC : 1;
  Bool create_reliefGC : 1;
  Bool create_shadowGC : 1;
  Bool initialised : 1;
  Bool useFvwmLook : 1;
  Visual *viz;
  Colormap cmap;
  unsigned int depth;
  GC drawGC;
  unsigned int back_bits;
  union {
   Pixel pixel;
    Pixmap pixmap;
  } bg;
  GC foreGC;
  GC reliefGC;
  GC shadowGC;
  XFontStruct *font;
} Graphics;

void InitGraphics(Display *dpy, Graphics *graphics);
Bool ParseGraphics(Display *dpy, char * line, Graphics *graphics);

