


#ifndef STEPGFX_H_
#define STEPGFX_H_

#define TEXTURE_SOLID		0
#define TEXTURE_GRADIENT 	1
#define TEXTURE_HGRADIENT	2
#define TEXTURE_HCGRADIENT	3
#define TEXTURE_VGRADIENT	4
#define TEXTURE_VCGRADIENT	5

#define TEXTURE_PIXMAP		128

#define TEXTURE_BUILTIN		255

extern int DrawVGradient(Display *dpy, Drawable d, int x, int y, int w, int h, 
			 int from[3], int to[3], int relief, int maxcols,
			 int type);
extern int DrawHGradient(Display *dpy, Drawable d, int x, int y, int w, int h, 
			 int from[3], int to[3], int relief, int maxcols,
			 int type);
extern int DrawDegradeRelief(Display *dpy, Drawable d, int x, int y, int w, 
			     int h, int from[3], int to[3], int relief,
			     int maxcols);
extern void DrawTexturedText(Display *dpy, Drawable d, XFontStruct *font,
		      int x, int y, Pixmap gradient, char *text, int chars);

extern int MakeShadowColors(Display *dpy, int from[3], int to[3],
			    unsigned long *dark, unsigned long *light);

#endif
