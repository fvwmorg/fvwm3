#include "../../libs/fvwmlib.h"       
char *safemalloc(int length);
void DeadPipe(int nonsense);
void GetTargetWindow(Window *app_win);
void sleep_a_little(int n);
void CopyString(char **dest, char *source);
char *CatString2(char *a, char *b);
void nocolor(char *a, char *b);
char *CatString3(char *a, char *b, char *c);
Window ClientWindow(Window input);

void RelieveWindow(Window win,int x,int y,int w,int h, GC rgc,GC sgc);
void CreateWindow(int x, int y,int w, int h);
Pixel GetShadow(Pixel background);
Pixel GetHilite(Pixel background);
Pixel GetColor(char *name);
void Loop(Window target);
void RedrawWindow(Window target);
void change_window_name(char *str);
extern void send_clientmessage (Display *disp, Window w, Atom a, Time timestamp);
void GrabWindow(Window target);
void change_icon_name(char *str);
void RedrawLeftButton(GC rgc, GC sgc,int x1,int y1);
void RedrawRightButton(GC rgc, GC sgc,int x1,int y1);
void RedrawTopButton(GC rgc, GC sgc,int x1,int y1);
void RedrawBottomButton(GC rgc, GC sgc,int x1,int y1);

#ifdef BROKEN_SUN_HEADERS
#include "../../fvwm/sun_headers.h"
#endif

#ifdef NEEDS_ALPHA_HEADER
#include "../../fvwm/alpha_header.h"
#endif /* NEEDS_ALPHA_HEADER */
                                        
