typedef struct ScreenInfo
{
  unsigned long screen;
  int d_depth;	        /* copy of DefaultDepth(dpy, screen) */
  int MyDisplayWidth;	/* my copy of DisplayWidth(dpy, screen) */
  int MyDisplayHeight;  /* my copy of DisplayHeight(dpy, screen) */

  char *FvwmRoot;	/* the head of the fvwm window list */
  Window Root;		/* the root window */

  Window Pager_w;

  Font PagerFont;        /* font struct for window labels in pager (optional)*/

  GC NormalGC;		 /* normal GC for menus, pager, resize window */

  char  *Hilite;	 /* the fvwm window that is highlighted
			  * except for networking delays, this is the
			  * window which REALLY has the focus */
  unsigned VScale;       /* Panner scale factor */
  int VxMax;             /* Max location for top left of virt desk*/
  int VyMax;
  int Vx;                /* Current loc for top left of virt desk */
  int Vy;
  int CurrentDesk;
  Pixmap sticky_gray_pixmap;
  Pixmap light_gray_pixmap;
  Pixmap gray_pixmap;

} ScreenInfo;

typedef struct pager_window
{
  char *t;
  Window w;
  Window frame;
  int x;
  int y;
  int width;
  int height;
  int desk;
  int frame_x;
  int frame_y;
  int frame_width;
  int frame_height;
  int title_height;
  int border_width;
  int icon_x;
  int icon_y;
  int icon_width;
  int icon_height;
  Pixel text;
  Pixel back;
  unsigned long flags;
  Window icon_w;
  Window icon_pixmap_w;
  char *icon_name;
  Picture mini_icon;
  int pager_view_width;
  int pager_view_height;
  int icon_view_width;
  int icon_view_height;

  Window PagerView;
  Window IconView;

  struct pager_window *next;
} PagerWindow;


typedef struct balloon_window
{
  Window w;              /* ID of balloon window */
  PagerWindow *pw;       /* pager window it's associated with */
  XFontStruct *font;
  int height;            /* height of balloon window based on font */
  int border;            /* border width */
  int yoffset;           /* pixels above (<0) or below (>0) pager win */
} BalloonWindow;


typedef struct desk_info
{
  Window w;
  Window title_w;
  Window CPagerWin;
  int x;
  int y;
  char *Dcolor;
  char *label;
} DeskInfo;

typedef struct pager_string_list
{
  struct pager_string_list *next;
  int desk;
  char *Dcolor;
  char *label;
} PagerStringList;

#define ON 1
#define OFF 0

/*************************************************************************
 *
 * Subroutine Prototypes
 *
 *************************************************************************/
char *GetNextToken(char *indata,char **token);
void Loop(int *fd);
void SendInfo(int *fd,char *message,unsigned long window);
char *safemalloc(int length);
void DeadPipe(int nonsense);
void process_message(unsigned long type,unsigned long *body);
void ParseOptions(void);

void list_add(unsigned long *body);
void list_configure(unsigned long *body);
void list_destroy(unsigned long *body);
void list_focus(unsigned long *body);
void list_toggle(unsigned long *body);
void list_new_page(unsigned long *body);
void list_new_desk(unsigned long *body);
void list_raise(unsigned long *body);
void list_lower(unsigned long *body);
void list_unknown(unsigned long *body);
void list_iconify(unsigned long *body);
void list_deiconify(unsigned long *body);
void list_window_name(unsigned long *body);
void list_icon_name(unsigned long *body);
void list_class(unsigned long *body);
void list_res_name(unsigned long *body);
void list_mini_icon(unsigned long *body);
void list_end(void);
int My_XNextEvent(Display *dpy, XEvent *event);

/* Stuff in x_pager.c */
void initialize_pager(void);
Pixel GetColor(char *name);
void nocolor(char *a, char *b);
void DispatchEvent(XEvent *Event);
void ReConfigure(void);
void ReConfigureAll(void);
void MovePage(void);
void DrawGrid(int i,int erase);
void DrawIconGrid(int erase);
void SwitchToDesk(int Desk);
void SwitchToDeskAndPage(int Desk, XEvent *Event);
void AddNewWindow(PagerWindow *prev);
void MoveResizePagerView(PagerWindow *t);
void ChangeDeskForWindow(PagerWindow *t,long newdesk);
void MoveStickyWindow(void);
void Hilight(PagerWindow *, int);
void Scroll(int window_w, int window_h, int x, int y, int Desk);
void MoveWindow(XEvent *Event);
void LabelWindow(PagerWindow *t);
void LabelIconWindow(PagerWindow *t);
void PictureWindow(PagerWindow *t);
void PictureIconWindow(PagerWindow *t);
void ReConfigureIcons(void);
void IconSwitchPage(XEvent *Event);
void IconMoveWindow(XEvent *Event,PagerWindow *t);
void HandleExpose(XEvent *Event);
void MoveStickyWindows(void);
void MapBalloonWindow(XEvent *);
void UnmapBalloonWindow(void);
void DrawInBalloonWindow(void);




