/* -*-c-*- */
extern char* MyName;
extern Display* dpy;
extern Window Root;
extern int screen;
extern GC hiliteGC, shadowGC;
extern fd_set_size_t fd_width;
extern int fd[2];
extern int x_fd;
extern int ScreenWidth, ScreenHeight;

char *safemalloc(int length);
RETSIGTYPE DeadPipe(int nonsense);
void GetTargetWindow(Window *app_win);
void get_graphics(char *line);

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

